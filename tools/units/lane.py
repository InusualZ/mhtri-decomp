#!/usr/bin/env python3
"""Teardown for a lane that is **not** a claim: rescue its unlanded commits, then remove it.

`claims.py release` is the teardown for a *worker* - it owns the branch, the worktree and the registry
entry, and it makes a `refs/rescue/<slug>` ref before deleting anything. An **orchestrator's own lane**
(the `experiment/*` branches a fan-out uses) has no claim, so `claims.py` does not apply to it, and the
orchestrator has to do the same three steps by hand. Doing that by hand is how a verified commit was
nearly lost on 2026-09-24: `git worktree remove --force` + `git branch -D` deleted
`experiment/wP-promote`, whose only commit held 17 symbol renames - it survived solely because git had
not yet pruned the object. This tool makes that mistake impossible.

    python tools/units/lane.py list                      # every lane, its unlanded commits, its rescue ref
    python tools/units/lane.py teardown <branch>          # rescue, then remove the worktree and the branch
    python tools/units/lane.py teardown <branch> --dry-run
    python tools/units/lane.py --selftest

**The order is the whole point**: every commit in `main..<branch>` is copied to `refs/rescue/<slug>`
*before* the branch is deleted, and a lane with nothing unlanded says so instead of quietly making an
empty ref. A rescue ref is never deleted, by anything (docs/plan.md, "A branch is never the only copy of
work"), so a teardown is always reversible with:

    git branch <branch> refs/rescue/<slug>

`teardown` refuses to touch a branch that is not a lane prefix (`experiment/`, `worker/`, `wip/`) unless
`--force`, so a mistyped branch name cannot delete `main`.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import wtsafe  # noqa: E402

# The prefixes a lane may use. `main` and a release ref are never lanes.
LANE_PREFIXES = ("experiment/", "worker/", "wip/", "lane/")
BASE = "main"


def git(args: list[str], cwd: str | None = None, check: bool = True) -> str:
    p = subprocess.run(["git"] + args, cwd=cwd or unitutil.repo_root(),
                       capture_output=True, text=True, errors="replace")
    if check and p.returncode != 0:
        raise SystemExit("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return (p.stdout or "").strip()


def slug(branch: str) -> str:
    """The rescue ref's name for a branch: `experiment/wP-promote` -> `wP-promote`.

    `claims.slug_of_branch` does the same for a claim, and the rule is one slug per branch, so a lane's
    ref is derivable from the branch alone - which is what makes recovery a one-liner.
    """
    return re.sub(r"^[^/]+/", "", branch)


def worktree_for(branch: str) -> str | None:
    """The worktree checked out on `branch`, or None. `git worktree list --porcelain` is the only
    reliable source: a path convention (`../<repo>.ws-<slug>`) is a convention, not a fact."""
    out = git(["worktree", "list", "--porcelain"])
    path = None
    for line in out.splitlines():
        if line.startswith("worktree "):
            path = line[len("worktree "):]
        elif line.startswith("branch ") and path:
            ref = line[len("branch "):]
            if ref == "refs/heads/" + branch:
                return path
    return None


def unlanded(branch: str, base: str = BASE) -> list[tuple[str, str]]:
    """(sha, subject) for every commit on `branch` that `base` does not have - newest last."""
    out = git(["log", "--reverse", "--format=%H\t%s", "%s..%s" % (base, branch)])
    return [tuple(l.split("\t", 1)) for l in out.splitlines() if "\t" in l]


def rescue(branch: str, base: str = BASE, dry_run: bool = False) -> dict:
    """Copy the branch's unlanded tip to `refs/rescue/<slug>` - the step that makes teardown safe.

    The ref points at the branch's **tip**, not at each commit: a ref is enough to keep the whole chain
    reachable, which is the property that matters (the 2026-09-24 near-loss was a tip that no ref named).
    """
    ref = "refs/rescue/" + slug(branch)
    commits = unlanded(branch, base)
    existed = ref_exists(ref)
    tip = git(["rev-parse", branch])
    if not commits:
        return {"branch": branch, "ref": ref, "commits": [], "action": "nothing unlanded",
                "tip": tip, "previous": existed}
    if dry_run:
        return {"branch": branch, "ref": ref, "commits": commits, "action": "would set",
                "tip": tip, "previous": existed}
    git(["update-ref", ref, tip])
    return {"branch": branch, "ref": ref, "commits": commits, "action": "set",
            "tip": tip, "previous": existed}


def teardown(branch: str, base: str = BASE, dry_run: bool = False, force: bool = False) -> dict:
    """Rescue, then remove the worktree and the branch. Refuses anything that is not a lane."""
    if not any(branch.startswith(p) for p in LANE_PREFIXES) and not force:
        raise SystemExit("refusing: %r is not a lane branch (%s). Pass --force if it really is one."
                         % (branch, ", ".join(LANE_PREFIXES)))
    if git(["rev-parse", "--verify", "--quiet", "refs/heads/" + branch], check=False) is None:
        raise SystemExit("no such branch: %s" % branch)
    res = rescue(branch, base, dry_run)
    wt = worktree_for(branch)
    steps = []
    if wt:
        # Measured 2026-09-24: `git worktree remove --force` follows a Windows directory junction and
        # deletes the TARGET's contents - that is how MAIN's orig/RMHE08/sys and files were emptied when
        # this loop ran over seven worktrees. wtsafe unlinks the reparse points first, and the snapshot
        # pair turns any other loss into a loud failure instead of a silent one.
        before = wtsafe.snapshot()
        links: list[str] = []
        if not dry_run:
            links = wtsafe.remove_worktree(wt)
            after = wtsafe.snapshot()
            if after != before:
                raise SystemExit("orig/ changed while removing %s - restore it from the pinned hashes:\n%s"
                                 % (wt, after))
        steps.append(("worktree", wt))
        if links:
            steps.append(("unlinked reparse points", ", ".join(os.path.basename(p) for p in links)))
    if not dry_run:
        git(["branch", "-D", branch])
        git(["worktree", "prune"])
    steps.append(("branch", branch))
    return dict(res, worktree=wt, steps=steps, dry_run=dry_run)


def ref_exists(ref: str) -> bool:
    """Whether a ref resolves. `git()` returns `""` on failure, not None, so this is the only honest
    test - and a false positive here would tell a user a lane is safe when its work has no ref at all."""
    return bool(git(["rev-parse", "--verify", "--quiet", ref], check=False))


def lanes(base: str = BASE) -> list[dict]:
    """Every lane branch with its unlanded commit count and whether a rescue ref already names it."""
    out = []
    for line in git(["branch", "--format=%(refname:short)"]).splitlines():
        b = line.strip().lstrip("+* ").strip()
        if not b or not any(b.startswith(p) for p in LANE_PREFIXES):
            continue
        commits = unlanded(b, base)
        out.append({"branch": b, "unlanded": len(commits),
                    "rescue": ref_exists("refs/rescue/" + slug(b)),
                    "worktree": worktree_for(b)})
    return out


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("slug strips the lane prefix", slug("experiment/wP-promote"), "wP-promote")
    check("slug keeps a worker branch readable", slug("worker/800a99b4-fn-800a99b4-0403"),
          "800a99b4-fn-800a99b4-0403")
    check("slug leaves an unprefixed name alone", slug("wip"), "wip")

    # a real repo, so the git plumbing is exercised rather than mocked
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        def run(*a):
            return subprocess.run(["git"] + list(a), cwd=tmp, capture_output=True, text=True,
                                  errors="replace")
        run("init", "-q", "-b", "main")
        run("config", "user.email", "t@example.com")
        run("config", "user.name", "t")
        open(os.path.join(tmp, "a.txt"), "w").write("a\n")
        run("add", "a.txt")
        run("commit", "-q", "-m", "base")
        run("checkout", "-q", "-b", "experiment/lane-x")
        open(os.path.join(tmp, "b.txt"), "w").write("b\n")
        run("add", "b.txt")
        run("commit", "-q", "-m", "lane work")
        run("checkout", "-q", "main")

        real_root = unitutil.repo_root
        unitutil.repo_root = lambda *a, **k: tmp
        try:
            check("unlanded finds the lane's commit", len(unlanded("experiment/lane-x")), 1)
            check("unlanded is empty for main itself", unlanded("main"), [])
            check("teardown refuses a non-lane branch",
                  _refuses(lambda: teardown("main")), True)
            r = rescue("experiment/lane-x")
            check("rescue sets the ref", r["action"], "set")
            check("the ref names the lane's tip",
                  run("rev-parse", "refs/rescue/lane-x").stdout.strip(),
                  run("rev-parse", "experiment/lane-x").stdout.strip())
            check("a second rescue is idempotent",
                  rescue("experiment/lane-x")["action"], "set")
            check("the rescue ref survives the branch deletion",
                  (run("branch", "-D", "experiment/lane-x").returncode, run(
                      "rev-parse", "--verify", "--quiet", "refs/rescue/lane-x").returncode)[1], 0)
            check("recovery is one command from the ref",
                  run("branch", "recovered", "refs/rescue/lane-x").returncode, 0)
            check("the recovered branch holds the work",
                  len(unlanded("recovered")), 1)
            check("lanes reports a missing rescue ref as missing",
                  _lanes_missing_ref(tmp), True)
        finally:
            unitutil.repo_root = real_root

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def _lanes_missing_ref(tmp: str) -> bool:
    """A lane with unlanded commits and no rescue ref must be reported as NOT rescued.

    This is the check that caught a real bug: `git rev-parse --verify --quiet` returns `""` on failure,
    and `"" is not None` is True, so the first version reported every lane as rescued.
    """
    real_root = unitutil.repo_root
    unitutil.repo_root = lambda *a, **k: tmp
    try:
        run = lambda *a: subprocess.run(["git"] + list(a), cwd=tmp, capture_output=True, text=True)
        run("checkout", "-q", "-b", "experiment/lane-y")
        open(os.path.join(tmp, "c.txt"), "w").write("c\n")
        run("add", "c.txt")
        run("commit", "-q", "-m", "lane y")
        run("checkout", "-q", "main")
        rows = {l["branch"]: l for l in lanes()}
        y = rows.get("experiment/lane-y")
        return bool(y) and y["unlanded"] == 1 and y["rescue"] is False
    finally:
        unitutil.repo_root = real_root


def _refuses(fn) -> bool:
    try:
        fn()
        return False
    except SystemExit:
        return True


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--base", default=BASE, help="the branch the work must have landed on (default main)")
    sub = ap.add_subparsers(dest="cmd")
    sub.add_parser("list")
    t = sub.add_parser("teardown")
    t.add_argument("branch")
    t.add_argument("--dry-run", action="store_true")
    t.add_argument("--force", action="store_true", help="allow a branch that is not a lane prefix")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if args.cmd == "list":
        rows = lanes(args.base)
        if not rows:
            print("no lanes")
            return 0
        for r in rows:
            print("%-34s unlanded=%-3d rescue=%-5s worktree=%s"
                  % (r["branch"], r["unlanded"], "yes" if r["rescue"] else "NO", r["worktree"] or "-"))
        missing = [r["branch"] for r in rows if r["unlanded"] and not r["rescue"]]
        if missing:
            print("\nWARNING: %d lane(s) hold unlanded commits with no rescue ref:" % len(missing))
            for b in missing:
                print("  %s   -> python tools/units/lane.py teardown %s" % (b, b))
        return 0
    if args.cmd == "teardown":
        out = teardown(args.branch, args.base, args.dry_run, args.force)
        for sha, subj in out["commits"]:
            print("  %s unlanded: %s" % (sha[:8], subj))
        if out["commits"]:
            print("  %s %s (%s)" % ("would set" if args.dry_run else "set", out["ref"], out["tip"][:8]))
        else:
            print("  nothing unlanded on %s - no rescue ref needed" % args.branch)
        for kind, what in out["steps"]:
            print("  %s %s %s" % ("would remove" if args.dry_run else "removed", kind, what))
        return 0
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
