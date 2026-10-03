"""Remove a worktree without letting a junction reach outside it, and prove `orig/` survived.

Why this exists (2026-09-24).  A lane teardown ran `git worktree remove --force` against a worktree that
carried junctions into MAIN's tree - documented practice here (`.pi/notes/pl-act-09c6.md` junctioned
`build/tools` and `build/RMHE08/obj`).  A Windows directory junction is a **reparse point**, and git's
worktree removal - like Python's `shutil.rmtree` - sees it as an ordinary directory and recurses through
it, so removing the worktree deleted the *target's* contents: MAIN's `orig/RMHE08/sys` and
`orig/RMHE08/files` were emptied.  The damage pattern is the fingerprint: only the two junctioned
directories lost their contents, while the plain files beside them survived.

A junction is only ever safe to **unlink**, never to walk through.  So every worktree removal in this repo
goes through `remove_worktree()` here, which unlinks the reparse points first.

Second line of defence: `verify_orig()` recomputes the pinned hashes of the original files (read from
`config/RMHE08/config.yml`, never hard-coded) so a loss is loud instead of silent.  The caller pairs it
with a snapshot taken before the removal.

Measured, not assumed (the tool's selftest asserts both): `git worktree remove --force` **does** destroy a
junction's target, while Python's `shutil.rmtree` unlinks reparse points and leaves the target alone.

    python tools/units/wtsafe.py --check      # verify orig/ against its pinned hashes
    python tools/units/wtsafe.py --selftest
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from tools.lib.git import Git
from tools.lib import repo as librepo

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CONFIG = os.path.join("config", "RMHE08", "config.yml")


def main_worktree(start: str | None = None) -> str:
    """The primary worktree (MAIN) - the tree whose `orig/` the pinned hashes describe.

    A worker worktree carries no `orig/` of its own (`claims.py` seeds `build/tools`, not `orig/`), and the
    teardown being checked deletes the very worktree it runs in, so reading `orig/` through this file's own
    location is the read that cannot survive the removal it guards.  `git worktree list --porcelain` names
    the main worktree first, so MAIN is resolved from git rather than guessed from the path.  A caller with
    a known root passes it as `start` (the lane selftest points this at its throwaway repo).
    """
    cwd = start or os.path.dirname(os.path.abspath(__file__))
    try:
        p = subprocess.run(["git", "worktree", "list", "--porcelain"], cwd=cwd,
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
    except OSError:
        return REPO
    if p.returncode == 0:
        for line in (p.stdout or "").splitlines():
            if line.startswith("worktree "):
                path = line[len("worktree "):].strip()
                return os.path.abspath(path) if path else REPO
    return REPO


def is_reparse_point(path: str) -> bool:
    """Whether `path` is a junction or a symlink - i.e. a removal must not recurse through it."""
    try:
        st = os.lstat(path)
    except OSError:
        return False
    if os.path.islink(path):
        return True
    return bool(getattr(st, "st_reparse_tag", 0))


def _unlink_one(path: str) -> None:
    """Delete a link itself.  `os.rmdir` on a junction removes the reparse point, never the target."""
    try:
        os.rmdir(path)
    except OSError:
        os.unlink(path)


def unlink_reparse_points(root: str) -> list[str]:
    """Delete every junction/symlink at or under `root`, as a link.  Returns what was removed.

    Bottom-up is not needed: a link is removed the moment it is met and its entry dropped from the walk,
    so nothing is ever traversed through.  A real directory that is not a reparse point is left alone.
    """
    removed: list[str] = []
    for dirpath, dirnames, filenames in os.walk(root, topdown=True, followlinks=False):
        for name in list(dirnames):
            p = os.path.join(dirpath, name)
            if is_reparse_point(p):
                _unlink_one(p)
                dirnames.remove(name)
                removed.append(p)
        for name in filenames:
            p = os.path.join(dirpath, name)
            if is_reparse_point(p):
                _unlink_one(p)
                removed.append(p)
    return removed


def remove_worktree(path: str, repo: str | None = None) -> list[str]:
    """Unlink the reparse points under `path`, then `git worktree remove --force` it.

    `repo` is the working tree git is run from.  The lane teardown passes the main worktree it resolved,
    so the removal does not depend on the teardown's own cwd - which is inside the worktree being deleted.
    """
    removed = unlink_reparse_points(path)
    subprocess.run(["git", "worktree", "remove", "--force", path], cwd=repo, check=False,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return removed


def sha1(path: str) -> str:
    h = hashlib.sha1()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def ground_truth(repo: str | None = None) -> dict[str, str]:
    return librepo.ground_truth(repo or main_worktree())


def verify_orig(repo: str | None = None, want: dict[str, str] | None = None) -> list[tuple[str, str, str]]:
    """`[(path, want, got)]` for every pinned original that is missing or changed.  Empty means intact."""
    repo = repo or main_worktree()
    want = ground_truth(repo) if want is None else want
    bad = []
    for rel, expected in sorted(want.items()):
        p = os.path.join(repo, rel)
        got = "MISSING" if not os.path.exists(p) else sha1(p)
        if got != expected:
            bad.append((rel, expected, got))
    return bad


def snapshot(repo: str | None = None) -> str:
    """One line per pinned original, for a before/after comparison in a teardown log.

    `repo` defaults to the main worktree and is passed explicitly by the caller when it must outlive the
    worktree being removed: snapshotting the teardown's own worktree after its removal is the read that
    crashed the first fix attempt.
    """
    repo = repo or main_worktree()
    return "\n".join("%s %s" % (sha1(os.path.join(repo, r)), r) if os.path.exists(os.path.join(repo, r))
                     else "MISSING %s" % r for r in sorted(ground_truth(repo)))


def _make_junction(link: str, target: str) -> bool:
    """A junction (not a symlink) - no admin rights needed and it is what the worktrees carried."""
    if os.name != "nt":
        return False
    r = subprocess.run(["cmd", "/c", "mklink", "/J", os.path.abspath(link), os.path.abspath(target)],
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    return r.returncode == 0 and os.path.exists(link)


def _git(args: list[str], cwd: str) -> None:
    Git(cwd).run(*args)


def _junction_roundtrip(tmp: str) -> tuple[bool, bool] | None:
    """Aiming the hazard at a throwaway repo: does a removal destroy the junction's target?

    Returns `(survived_without_the_fix, survived_with_the_fix)`.  The first is the 2026-09-24 incident - a
    `git worktree remove --force` through a junction into MAIN's tree emptied `orig/RMHE08/sys` and
    `files` - and is asserted as False, so the test fails loudly if a future git stops recursing (the
    protection would then be unnecessary rather than wrong, and this test says which).
    """
    main, target = os.path.join(tmp, "m"), os.path.join(tmp, "target")
    os.makedirs(main)
    os.makedirs(target)
    precious = os.path.join(target, "data.txt")
    _git(["init", "-q", "."], main)
    open(os.path.join(main, "f"), "w").write("hi\n")
    _git(["add", "f"], main)
    _git(["-c", "user.email=a@b", "-c", "user.name=c", "commit", "-qm", "init"], main)
    out = []
    for name, protected in (("hazard", False), ("protected", True)):
        open(precious, "w").write("ground truth\n")
        wt = os.path.join(tmp, "wt_" + name)
        _git(["worktree", "add", "-q", wt], main)
        if not _make_junction(os.path.join(wt, "junc"), target):
            return None
        if protected:
            remove_worktree(wt)
        else:
            _git(["worktree", "remove", "--force", wt], main)
        out.append(os.path.exists(precious))
        _git(["worktree", "prune"], main)
    return out[0], out[1]


def selftest() -> int:
    fails = []
    checks = [0]

    def check(name, got, want):
        checks[0] += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    with tempfile.TemporaryDirectory() as tmp:
        plain = os.path.join(tmp, "plain")
        os.makedirs(os.path.join(plain, "sub"))
        check("a real directory is not a reparse point", is_reparse_point(plain), False)
        check("... nor a real file", is_reparse_point(os.path.join(plain, "sub")), False)

        # unlinking a junction is safe: the link goes, the target and its contents stay
        target = os.path.join(tmp, "keep")
        os.makedirs(target)
        precious = os.path.join(target, "data.txt")
        open(precious, "w").write("ground truth\n")
        wt = os.path.join(tmp, "unlink_me")
        os.makedirs(wt)
        link = os.path.join(wt, "junc")
        if _make_junction(link, target):
            check("a junction IS a reparse point", is_reparse_point(link), True)
            check("... and reads through to the target", open(os.path.join(link, "data.txt")).read(),
                  "ground truth\n")
            check("unlink_reparse_points removes it", unlink_reparse_points(wt), [link])
            check("... the link is gone", os.path.exists(link), False)
            check("... the target survives", os.path.isdir(target), True)
            check("... and so does its contents", open(precious).read(), "ground truth\n")
        else:
            print("  (junction creation unavailable - link checks skipped)")

        # the incident itself, aimed at a throwaway repo
        round_trip = _junction_roundtrip(tmp)
        if round_trip is None:
            print("  (git hazard check skipped - no junction available)")
        else:
            hazard, protected = round_trip
            check("git worktree remove DOES destroy a junction's target (the 2026-09-24 hazard)",
                  hazard, False)
            check("remove_worktree() stops it (target intact)", protected, True)

    # the main worktree's originals must match the hashes config.yml pins
    main = main_worktree()
    check("the main worktree's orig/ matches config.yml's pinned hashes", verify_orig(main), [])
    check("ground_truth reads both pins", sorted(ground_truth(main)),
          ["orig/RMHE08/files/mh3.sel", "orig/RMHE08/sys/main.dol"])

    for f in fails:
        print("  FAIL " + f)
    print("wtsafe: %d check(s), %d failure(s)" % (checks[0], len(fails)))
    return 1 if fails else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--check", action="store_true", help="verify orig/ against its pinned hashes")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--unlink", metavar="DIR", help="unlink the reparse points under DIR")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    if a.unlink:
        for p in unlink_reparse_points(a.unlink):
            print("unlinked %s" % p)
        return 0
    main = main_worktree()
    bad = verify_orig(main)
    for rel, want, got in bad:
        print("CHANGED %s want %s got %s" % (rel, want, got))
    print("orig/ (%s): %d pinned file(s), %s"
          % (main, len(ground_truth(main)), "intact" if not bad else "CHANGED"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
