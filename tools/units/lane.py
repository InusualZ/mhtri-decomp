#!/usr/bin/env python3
"""Teardown for a lane that is not a claim: rescue its unlanded commits to `refs/rescue/<slug>`, then remove it.
Spec: docs/tools/spec/lane.md. CLI: python tools/units/lane.py list | teardown <branch> [--dry-run] [--force]
[--base B]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import cli
from tools.lib import repo as librepo
from tools.lib.git import Git
from tools.lib.lanes import naming, teardown as td

TOOL = cli.Tool("lane", "docs/tools/spec/lane.md", description=(__doc__ or "").splitlines()[0], common=())
LANE_PREFIXES = naming.LANE_PREFIXES
BASE = "main"
slug = naming.lane_slug


def _root(repo: str | None) -> str:
    return repo or librepo.repo_root()


def git(args: list[str], cwd: str | None = None, check: bool = True) -> str:
    p = Git(_root(cwd)).run(*args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed:\n%s%s" % (" ".join(args), p.stdout, p.stderr))
    return (p.stdout or "").strip()


def worktree_for(branch: str, repo: str | None = None) -> str | None:
    """The worktree checked out on `branch` (from `git worktree list --porcelain`), or None."""
    path = None
    for line in git(["worktree", "list", "--porcelain"], repo).splitlines():
        if line.startswith("worktree "):
            path = line[len("worktree "):]
        elif line.startswith("branch ") and path and line[len("branch "):] == "refs/heads/" + branch:
            return path
    return None


def unlanded(branch: str, base: str = BASE, repo: str | None = None) -> list[tuple[str, str]]:
    """(sha, subject) of every commit on `branch` that `base` lacks, newest last."""
    out = git(["log", "--reverse", "--format=%H\t%s", "%s..%s" % (base, branch)], repo)
    return [tuple(line.split("\t", 1)) for line in out.splitlines() if "\t" in line]


def ref_exists(ref: str, repo: str | None = None) -> bool:
    return bool(git(["rev-parse", "--verify", "--quiet", ref], repo, check=False))


def rescue(branch: str, base: str = BASE, dry_run: bool = False, repo: str | None = None) -> dict:
    """Point `refs/rescue/<slug>` at the branch's tip when it holds unlanded commits."""
    ref = naming.rescue_ref_for_branch(branch)
    commits = unlanded(branch, base, repo)
    existed = ref_exists(ref, repo)
    tip = git(["rev-parse", branch], repo)
    if not commits:
        return {"branch": branch, "ref": ref, "commits": [], "action": "nothing unlanded", "tip": tip,
                "previous": existed}
    if dry_run:
        return {"branch": branch, "ref": ref, "commits": commits, "action": "would set", "tip": tip,
                "previous": existed}
    git(["update-ref", ref, tip], repo)
    return {"branch": branch, "ref": ref, "commits": commits, "action": "set", "tip": tip, "previous": existed}


def teardown(branch: str, base: str = BASE, dry_run: bool = False, force: bool = False,
             repo: str | None = None, remove=None) -> dict:
    """Rescue, then remove the worktree (junction-safe, `orig/` snapshot before and after, read from MAIN) and
    the branch. Refuses a non-lane branch unless `force`; idempotent. `remove` is the removal seam (tests)."""
    if not naming.is_lane_branch(branch) and not force:
        raise SystemExit("refusing: %r is not a lane branch (%s). Pass --force if it really is one."
                         % (branch, ", ".join(LANE_PREFIXES)))
    here = _root(repo)
    main = td.main_worktree(here)
    branch_present = bool(git(["rev-parse", "--verify", "--quiet", "refs/heads/" + branch], here, check=False))
    wt = worktree_for(branch, here)
    ref = naming.rescue_ref_for_branch(branch)
    if not branch_present and wt is None:
        return {"branch": branch, "ref": ref, "commits": [], "action": "nothing to tear down", "tip": None,
                "previous": ref_exists(ref, here), "worktree": None, "steps": [], "dry_run": dry_run}
    if branch_present:
        res = rescue(branch, base, dry_run, here)
    else:
        res = {"branch": branch, "ref": ref, "commits": [], "action": "branch already gone", "tip": None,
               "previous": ref_exists(ref, here)}
    steps = []
    if wt:
        links: list[str] = []
        if not dry_run:
            before = td.snapshot(main)
            links = (remove or td.remove_worktree)(wt, main)
            after = td.snapshot(main)
            if after != before:
                raise SystemExit("orig/ changed while removing %s - restore it from the pinned hashes:\n%s" % (wt, after))
        steps.append(("worktree", wt))
        if links:
            steps.append(("unlinked reparse points", ", ".join(os.path.basename(p) for p in links)))
    if not dry_run:
        if branch_present:
            git(["branch", "-D", branch], main)
        git(["worktree", "prune"], main)
    steps.append(("branch", branch))
    return dict(res, worktree=wt, steps=steps, dry_run=dry_run)


def lanes(base: str = BASE, repo: str | None = None) -> list[dict]:
    """Every lane branch with its unlanded count, whether a rescue ref names it, and its worktree."""
    out = []
    for line in git(["branch", "--format=%(refname:short)"], repo).splitlines():
        b = line.strip().lstrip("+* ").strip()
        if not b or not naming.is_lane_branch(b):
            continue
        out.append({"branch": b, "unlanded": len(unlanded(b, base, repo)),
                    "rescue": ref_exists(naming.rescue_ref_for_branch(b), repo), "worktree": worktree_for(b, repo)})
    return out


def main() -> int:
    ap = TOOL.parser()
    ap.add_argument("--base", default=BASE, help="the branch the work must have landed on (default main)")
    sub = ap.add_subparsers(dest="cmd")
    sub.add_parser("list")
    t = sub.add_parser("teardown")
    t.add_argument("branch")
    t.add_argument("--dry-run", action="store_true")
    t.add_argument("--force", action="store_true", help="allow a branch that is not a lane prefix")
    args = ap.parse_args()
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
        if out.get("action") == "nothing to tear down":
            print("  nothing to tear down on %s - the worktree and the branch are already gone" % args.branch)
            return 0
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
