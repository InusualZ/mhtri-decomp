"""One teardown: the step list (done / skipped / planned / failed, first failure stops the rest), the junction-safe
worktree removal, and the `orig/` integrity snapshot every removal is checked against.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import hashlib
import os
import shutil
import subprocess

from tools.lib import proc
from tools.lib import repo as librepo
from tools.lib.git import Git


# --- the step list ----------------------------------------------------------------------------------------------

def step(label: str, action) -> dict:
    """A step to run: `action()` returns a note (or None) on success and raises to fail the teardown."""
    return {"label": label, "action": action, "status": "planned", "why": ""}


def skip(label: str, why: str) -> dict:
    """A step with nothing to do, its reason recorded rather than hidden."""
    return {"label": label, "action": None, "status": "skipped", "why": why}


def done(label: str, why: str = "") -> dict:
    """A step already performed (reported after the fact, e.g. the registry entry)."""
    return {"label": label, "action": None, "status": "done", "why": why}


def run(steps: list[dict]) -> bool:
    """Run a plan in order -> complete. A skipped step stays skipped; the first failure marks every later step
    `skipped` ("not attempted: <label> failed first") instead of running it against a half-torn-down lane."""
    complete, failed = True, None
    for s in steps:
        if s["status"] == "skipped":
            continue
        if failed:
            s["status"] = "skipped"
            s["why"] = "not attempted: %s failed first" % failed
            continue
        try:
            note = s["action"]()
            s["status"] = "done"
            if note:
                s["why"] = note
        except (SystemExit, Exception) as exc:  # noqa: BLE001 - a refuser is recorded, never propagated
            detail = str(exc).strip().splitlines()
            s["status"] = "failed"
            s["why"] = detail[0] if detail else exc.__class__.__name__
            complete = False
            failed = s["label"]
    return complete


def public(steps: list[dict]) -> list[dict]:
    """The steps without their callables (what `--json` prints)."""
    return [{k: v for k, v in s.items() if k != "action"} for s in steps]


# --- reparse points ----------------------------------------------------------------------------------------------

def is_reparse_point(path: str) -> bool:
    """A junction or symlink - a removal must unlink it, never recurse through it."""
    try:
        st = os.lstat(path)
    except OSError:
        return False
    return os.path.islink(path) or bool(getattr(st, "st_reparse_tag", 0))


def _unlink_one(path: str) -> None:
    """Delete a link itself: `os.rmdir` on a junction removes the reparse point, never the target."""
    try:
        os.rmdir(path)
    except OSError:
        os.unlink(path)


def unlink_reparse_points(root: str) -> list[str]:
    """Delete every junction/symlink at or under `root` as a link (dropped from the walk the moment it is met,
    so nothing is traversed through). Returns what was removed."""
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


def make_junction(link: str, target: str) -> bool:
    """Point `link` at `target`: a Windows directory junction (`mklink /J`, no admin rights), a directory
    symlink elsewhere. Both paths are absolutised (`mklink` resolves a relative target against its cwd)."""
    link, target = os.path.abspath(link), os.path.abspath(target)
    if os.name == "nt":
        r = proc.run(["cmd", "/c", "mklink", "/J", link, target])
        return r.returncode == 0 and os.path.exists(link)
    try:
        os.symlink(target, link, target_is_directory=True)
        return True
    except OSError:
        return False


# --- worktree removal ----------------------------------------------------------------------------------------------

def remove_worktree(path: str, repo: str | None = None) -> list[str]:
    """Unlink the reparse points under `path`, then `git worktree remove --force` it (from `repo`, so the
    removal never depends on a cwd inside the tree being deleted). Returns the links removed; a git failure is
    not raised (the `orig/` snapshot is the caller's proof)."""
    removed = unlink_reparse_points(path)
    subprocess.run(["git", "worktree", "remove", "--force", path], cwd=repo, check=False,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return removed


def remove_worktree_checked(main: str, path: str) -> str:
    """Remove one worktree and repair the two states git refuses -> what was done; raises SystemExit otherwise.

    A submodule inside it needs `--force` twice; a directory git no longer tracks (a half-teardown) is deleted
    as a plain tree after its reparse points are unlinked."""
    unlink_reparse_points(path)
    p = Git(main).run("worktree", "remove", "--force", path)
    if p.returncode == 0:
        return "removed"
    message = "git worktree remove --force %s failed in %s: %s" % (path, main, p.stderr.strip())
    if "submodule" in message:
        q = Git(main).run("worktree", "remove", "--force", "--force", path)
        if q.returncode != 0:
            raise SystemExit("git worktree remove --force --force %s failed in %s: %s" % (path, main, q.stderr.strip()))
        return "removed (force twice, for the submodule)"
    known = _worktree_paths(main)
    if known is not None and not any(_same_path(k, path) for k in known):
        unlink_reparse_points(path)
        shutil.rmtree(path)
        return "removed the leftover directory (git no longer tracked it)"
    raise SystemExit(message)


def _worktree_paths(main: str) -> list[str] | None:
    p = Git(main).run("worktree", "list", "--porcelain")
    if p.returncode != 0:
        return None
    return [line[len("worktree "):].strip() for line in p.stdout.splitlines() if line.startswith("worktree ")]


def _same_path(a: str, b: str) -> bool:
    return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))


# --- the orig/ guard ---------------------------------------------------------------------------------------------

def main_worktree(start: str, fallback: str | None = None) -> str:
    """The primary worktree (the first `git worktree list` entry) seen from `start`; `fallback` (default
    `start`) when git cannot answer. A lane teardown resolves it before removing anything."""
    fallback = os.path.abspath(fallback or start)
    try:
        p = proc.run(["git", "worktree", "list", "--porcelain"], cwd=start)
    except OSError:
        return fallback
    if p.returncode == 0:
        for line in (p.stdout or "").splitlines():
            if line.startswith("worktree "):
                path = line[len("worktree "):].strip()
                return os.path.abspath(path) if path else fallback
    return fallback


def sha1(path: str) -> str:
    h = hashlib.sha1()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_orig(repo: str, want: dict[str, str] | None = None) -> list[tuple[str, str, str]]:
    """`[(path, want, got)]` for every pinned original (from `config.yml`) that is missing or changed."""
    want = librepo.ground_truth(repo) if want is None else want
    bad = []
    for rel, expected in sorted(want.items()):
        p = os.path.join(repo, rel)
        got = "MISSING" if not os.path.exists(p) else sha1(p)
        if got != expected:
            bad.append((rel, expected, got))
    return bad


def snapshot(repo: str) -> str:
    """One line per pinned original - the before/after pair a removal is checked against."""
    return "\n".join("%s %s" % (sha1(os.path.join(repo, r)), r) if os.path.exists(os.path.join(repo, r))
                     else "MISSING %s" % r for r in sorted(librepo.ground_truth(repo)))
