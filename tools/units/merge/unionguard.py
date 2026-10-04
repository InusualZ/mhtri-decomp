"""Refuse a union of a conflicted apply unless every conflict is a disjoint addition; undo the apply on refusal.
Spec: docs/tools/spec/merge.md. CLI: none (module of the landing gate)."""
from __future__ import annotations

import argparse
import os

from tools.lib.git import Git, GitError
import tools.units.merge.unionprose as union

BASE = "main"


def _git(cwd: str, *args: str) -> bytes:
    """`git <args>`'s stdout bytes; a failure raises `GitError` (a `RuntimeError`) naming git's stderr."""
    return Git(cwd).run_bytes(*args, check=True).stdout


def unmerged(cwd: str) -> dict[str, dict[int, str]]:
    """`git ls-files -u` as {path: {stage: blob-sha}}; stage 1=base, 2=ours, 3=theirs."""
    return Git(cwd).unmerged_stages()


def _cat(cwd: str, sha: str) -> bytes:
    return _git(cwd, "cat-file", "blob", sha)


def has_base_region(text: str) -> bool:
    """True iff any conflict hunk in a `--diff3` merge output has a non-empty base section (`union`'s reading)."""
    return union.has_base_region(text)


def three_way_overlap(cwd: str, stage1: str, stage2: str, stage3: str) -> bool:
    """Whether both sides changed the same *existing* content of this path (any hunk with a non-empty base)."""
    merged, conflicts = Git(cwd).merge_bytes(_cat(cwd, stage2), _cat(cwd, stage1), _cat(cwd, stage3),
                                             labels=("ours", "base", "theirs"))
    return bool(conflicts) and has_base_region(merged.decode("utf-8", "replace"))


def rename_sets(cwd: str, base: str, ref: str) -> tuple[set[str], set[str]]:
    """(rename sources, rename destinations) for `base..ref`, using git's own rename detection."""
    try:
        pairs = Git(cwd).renames(base, ref)
    except GitError:
        return set(), set()
    return {old for old, _new in pairs}, {new for _old, new in pairs}


def classify(cwd: str, path: str, stages: dict[int, str],
             ours_renames: tuple[set[str], set[str]],
             theirs_renames: tuple[set[str], set[str]]) -> dict:
    """One conflicted path's finding: what each side did and whether a union is safe."""
    stage1, stage2, stage3 = stages.get(1), stages.get(2), stages.get(3)
    ours_deleted = bool(stage1 and not stage2)
    theirs_deleted = bool(stage1 and not stage3)
    both_added = bool(not stage1 and stage2 and stage3)
    overlap = bool(stage1 and stage2 and stage3) and three_way_overlap(cwd, stage1, stage2, stage3)

    renamed_by = []
    if path in ours_renames[0] or path in ours_renames[1]:
        renamed_by.append("ours")
    if path in theirs_renames[0] or path in theirs_renames[1]:
        renamed_by.append("theirs")

    reasons = []
    if ours_deleted:
        reasons.append("deleted by ours")
    if theirs_deleted:
        reasons.append("deleted by theirs")
    if both_added:
        reasons.append("added by both sides")
    if overlap:
        reasons.append("both sides modified the same region")
    if renamed_by:
        reasons.append("renamed by " + "+".join(renamed_by))

    return {
        "path": path,
        "ours": "deleted" if ours_deleted else ("present" if stage2 else "-"),
        "theirs": "deleted" if theirs_deleted else ("present" if stage3 else "-"),
        "both_modified": overlap,
        "renamed": renamed_by,
        "unsafe": bool(reasons),
        "reasons": reasons,
    }


def diff_paths(cwd: str, base: str | None, branch: str | None) -> list[str]:
    """Every path the branch's own diff touches, both halves of a rename, in diff order (`-z`, so a path with
    a space or a rename parses unambiguously)."""
    if not (base and branch):
        return []
    fields = _git(cwd, "diff", "--name-status", "-z", "-M", base, branch).decode("utf-8", "replace").split("\0")
    paths: list[str] = []
    i = 0
    while i < len(fields):
        status = fields[i]
        i += 1
        if not status:
            continue
        count = 2 if status[0] in ("R", "C") else 1
        for _ in range(count):
            if i < len(fields) and fields[i]:
                paths.append(fields[i])
            i += 1
    return list(dict.fromkeys(paths))


def _in_head(cwd: str, path: str) -> bool:
    return Git(cwd).ok("cat-file", "-e", "HEAD:" + path)


def cleanup_applied(cwd: str, base: str | None, branch: str | None
                    ) -> tuple[list[str], list[str], list[str]]:
    """Undo `git apply -3 <merge-base diff>`: HEAD's copy back for a path HEAD has, a delete for one only the
    branch adds, then `git reset`. Returns (restored, removed, leftover status lines)."""
    g = Git(cwd)
    restored: list[str] = []
    removed: list[str] = []
    for path in diff_paths(cwd, base, branch):
        if _in_head(cwd, path):
            if g.ok("checkout", "HEAD", "--", path):
                restored.append(path)
        else:
            target = os.path.join(cwd, path)
            if os.path.lexists(target):
                os.remove(target)
            removed.append(path)
    g.run("reset", "-q")
    leftover = [line for line in _git(cwd, "status", "--short").decode(
        "utf-8", "replace").splitlines() if line.strip()]
    return restored, removed, leftover


def report(cwd: str, base: str | None, branch: str | None, paths: list[str],
           cleanup: bool = True) -> int:
    stages = unmerged(cwd)
    if not paths:
        paths = sorted(stages)
    ours_renames = rename_sets(cwd, base, "HEAD") if base else (set(), set())
    theirs_renames = rename_sets(cwd, base, branch) if (base and branch) else (set(), set())

    findings = [classify(cwd, p, stages.get(p, {}), ours_renames, theirs_renames) for p in paths]
    unsafe = [f for f in findings if f["unsafe"]]

    print("union guard: %d conflicted path(s)%s" % (
        len(paths), "" if not branch else ", branch %s" % branch))
    for f in findings:
        detail = ", ".join(f["reasons"]) if f["unsafe"] else "additions only"
        print("  %-40s %-7s (%s)" % (f["path"], "REFUSE" if f["unsafe"] else "ok", detail))

    if unsafe:
        print("*** unsafe union: %d path(s): %s" % (
            len(unsafe), ", ".join(f["path"] for f in unsafe)))
        if cleanup and base and branch:
            restored, removed, leftover = cleanup_applied(cwd, base, branch)
            print("*** the apply has been undone - fix the branch's change by hand; do not union them.")
            for path in restored:
                print("    restored %s" % path)
            for path in removed:
                print("    removed  %s (not in HEAD)" % path)
            if leftover:
                print("*** still dirty after cleanup - inspect these by hand:")
                for line in leftover:
                    print("    " + line)
            else:
                print("*** clean tree: no conflict stages and nothing staged")
        elif cleanup:
            print("*** resolve these by hand - do not union them.")
            print("*** (no --base/--branch given, so the apply could not be undone automatically)")
        else:
            print("*** --no-cleanup: leaving the conflicted index in place for inspection.")
            print("*** resolve these by hand - do not union them:")
            print("***   git status --short    # UU/AA mark the conflicts")
            print("***   git diff              # the conflicted hunks")
        return 1
    print("union guard: all conflicted paths are disjoint additions - union is safe")
    return 0


def parser(description: str) -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=description)
    ap.add_argument("paths", nargs="*", help="conflicted paths (default: every unmerged path)")
    ap.add_argument("--cwd", default=os.getcwd(), help="repository the apply ran in (default: cwd)")
    ap.add_argument("--base", default=None, help="merge-base ref (default: git merge-base main <branch>)")
    ap.add_argument("--branch", default=None, help="the branch being landed")
    ap.add_argument("--no-cleanup", action="store_true",
                    help="on refusal, leave the conflicted index in place for inspection")
    return ap


def main(argv: list[str] | None = None, description: str = "") -> int:
    args = parser(description).parse_args(argv)
    base = args.base
    if base is None:
        base = (_git(args.cwd, "merge-base", BASE, args.branch).decode("utf-8", "replace").strip()
                if args.branch else None)
    return report(args.cwd, base, args.branch, args.paths, cleanup=not args.no_cleanup)
