"""The tree rows: ground truth, the batch base, the batch-path guard and scratch, conflict markers, branch guards.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import subprocess
import sys

import tools.git.prepcommit as pc
from tools.lib.git import Git
from tools.units.landing import common
from tools.units.landing.base import base_dirty_paths
from tools.units.landing.common import (Batch, KIND_BOOKKEEPING, changed_paths, changed_status, git, outside_batch,
    scratch_paths)
from tools.units.landing.stage import land_stageable


def branch_error(main: str) -> str | None:
    """Refuse to run the gate anywhere but `main`.

    The incident (2026-09-24): a worker told to "branch and commit there" ran
    `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout, so MAIN's HEAD left `main` and the
    next **14 landings** went onto that branch while the `main` ref sat at `e3ade082`. Nothing failed -
    the landing path keys off `main` - but a stale `main` silently changes what
    it *means*: the merge-base slides backwards and the branch's diff starts describing already-landed
    units, re-applying them or listing them as deletions (it nearly deleted landed units the same day). The
    gate names the branch it found and refuses before any check runs.
    """
    branch = git(["rev-parse", "--abbrev-ref", "HEAD"], main).strip()
    if branch != "main":
        return ("HEAD is on %r, not main: `git checkout main` first - a batch landed off main puts its "
                "commits on the wrong ref and slides the merge-base" % branch)
    return None


def caller_branch_error(start: str | None = None) -> str | None:
    """`branch_error` asked about the tree the *caller* is in - the guard `record-base`/`verify` run under.

    Both are manual entry points on the landing path, and both read MAIN's tree. But `main` is resolved by
    `rc.main_root`, which walks the worktree list from wherever the caller stands and always answers with the
    first worktree git lists - MAIN. So a `record-base` run from inside a worker's worktree would quietly
    record MAIN's HEAD (or, if MAIN's HEAD had left `main`, a stale one) with nothing saying the wrong tree
    was asked. `land` refuses a HEAD that is not `main` through `branch_error(main)`; these two ask the same
    helper about the caller's own tree (`rc.worktree_root`), so a call from any worktree but MAIN is refused
    and names the branch it found.
    """
    return branch_error(common.worktree_root(start))


def conflict_marker_files(main: str, paths: list[str]) -> list[tuple[str, int, str]]:
    """The batch's own files that carry a git conflict marker, as `(path, line, marker)`.

    A committed conflict marker is the cheapest defect to catch and one of the more expensive ones to find
    late: the build reports it as a syntax error in whichever file carries it, so a full compile buys one
    line's worth of news, and the marker survives review because it looks like ordinary text.  Only the two
    markers a conflict writes are looked for - `=======` on its own is a legal banner comment, so a file
    full of those is not evidence of anything.
    """
    found: list[tuple[str, int, str]] = []
    for rel in paths:
        path = os.path.join(main, rel)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                for number, line in enumerate(fh, 1):
                    head = line.lstrip()
                    for marker in ("<<<<<<<", ">>>>>>>"):
                        if head.startswith(marker + " ") or head.rstrip() == marker:
                            found.append((rel, number, marker))
        except OSError:
            continue
    return found


def _exists_at(main: str, base: str | None, entry: str) -> bool:
    """True when `entry` is a file or directory in `base`'s tree (`git cat-file -e <base>:<entry>`)."""
    if not base:
        return False
    try:
        return subprocess.run(["git", "cat-file", "-e", "%s:%s" % (base, entry)], cwd=main,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    except OSError:
        return False


def is_batch_path(main: str, entry: str, base: str | None = None) -> bool:
    """True when a `--units` entry names a repo PATH the batch stages, not a translation unit.

    A unit is named at an extensionless path (`<module>/<name>`, or `src/<module>/<name>` - `norm_unit`
    strips the source extension) whose source file is `<name>.c`/`.cpp`, so the bare name is **not** a file
    in the tree.  Three signals therefore say "path": the entry still carries a file extension, the tree
    has something at that exact path - which `.gitignore`, `LICENSE`, `Makefile` and a directory like `docs`
    do, and a unit never does - or the batch BASE's tree has it.  The last is for a batch that DELETES (or
    renames away) an extension-less file such as `tools/git/hooks/post-commit`: the gate runs after the
    apply, where the file no longer exists, so the tree test alone read it as a unit and demanded an
    `Object(...)` line and an outbox for it (2026-09-28, the hook removal).
    """
    if os.path.splitext(entry)[1]:
        return True
    if os.path.exists(os.path.join(main, entry)):
        return True
    # A source-looking file OUTSIDE `src/` (a playbook demo `docs/matching/043-x.cpp`, a fixture) is a path too:
    # `norm_unit` strips its `.cpp` before it reaches here, but a unit's source always lives under `src/`.
    if not entry.startswith("src/"):
        for ext in (".cpp", ".cp", ".c"):
            if os.path.isfile(os.path.join(main, entry + ext)) or _exists_at(main, base, entry + ext):
                return True
    return _exists_at(main, base, entry)


def unit_rows(main: str, units: list[str], base: str | None = None) -> list[str]:
    """The unit-shaped subset of a `--units` list: translation units, not paths the batch stages.

    `is_batch_path` draws the line for every unit-shaped gate row (outbox, branch, registration, compile,
    target drift), so this is the one place that decision is made.  The outbox row used to run over the raw
    `--units` list instead, so a batch whose `--units` named a HEADER
    (`include/Network/network_state.h`) had `outbox_units` look for that path as if it were a unit and demand
    `residual`/`flags_probed` from a non-unit record - a BOOKKEEPING refusal while every real gate row passed
    (2026-09-28, cost one round-trip and was landed with `--no-outbox`).
    """
    return [u for u in units if not is_batch_path(main, u, base)]


def require_clean_tree(main: str) -> str | None:
    """None when MAIN's tree is clean; else the exact commands to clean it.

    One implementation and one message for every entry point, because a dirty tree blocks the gate twice
    over: `record-base` records the dirt as foreign work (so `land_stageable` excludes the batch's own
    paths), and the pick aborts on a file the dirt already touched.  Every row is dirt, CLAUDE.md included.
    """
    rows = changed_status(main)
    dirty = ["%s %s" % (code or "??", path) for code, path in rows]
    if not dirty:
        return None
    paths = " ".join(path for _code, path in rows)
    return ("main's tree is not clean: %s\n  a dirty tree is recorded as foreign work at `record-base` "
            "and aborts the pick, so clean it first, e.g.:\n"
            "    git -C %s stash push --include-untracked -- %s\n"
            "  (or `git -C %s checkout -- <path>` for a tracked edit and `git -C %s clean -fd` for "
            "untracked files), then re-run" % (", ".join(dirty), main, paths, main, main))


def scratch_note(paths: list[str]) -> str:
    """The line that NAMES tolerated scratch - a tolerated path is never silently dropped."""
    noun = "path" if len(paths) == 1 else "paths"
    return ("NOTE: %d tool scratch %s outside this batch - named here, never staged, never a refusal: %s"
            % (len(paths), noun, ", ".join(paths)))


def unstage_scratch(main: str, paths: list[str]) -> tuple[list[str], list[str]]:
    """De-index tolerated scratch; -> (paths whose index entry is gone, paths still staged).

    A staged copy is possible (the landing flow's `git add -A`, or a hand `git add`), and a staged file
    bypasses `.gitignore` - which is exactly how `d910.json` reached a gate that then refused it. `git reset
    HEAD -- <path>` drops the index entry and leaves the file in the worktree untouched, so the caller's dump
    is not destroyed. A `git reset` that fails is reported, never claimed: the caller has to unstage by hand
    before a `git commit` without a pathspec.
    """
    staged = set(git(["diff", "--cached", "--name-only"], main).splitlines())
    victims = [p for p in paths if p in staged]
    if not victims:
        return [], []
    p = Git(main).run_paths(["reset", "-q", "HEAD"], victims)
    return (victims, []) if p.returncode == 0 else ([], victims)


def tolerate_scratch(main: str, paths: list[str], act: bool = True) -> str:
    """Note (and, unless `act` is False, de-index) tolerated scratch; return the note line.

    `verify --dry-run` touches nothing, so it passes `act=False` and only names what it would have removed.
    """
    note = scratch_note(paths)
    if act:
        removed, held = unstage_scratch(main, paths)
        if removed:
            note += " (a staged copy was removed from the index)"
        if held:
            note += (" (WARNING: %s is staged and `git reset` failed - unstage it by hand before any "
                     "`git commit` without a pathspec)" % ", ".join(held))
    return note


# A foreign path whose *name* looks like a lane's scratch names the likely cause, so the pre-flight's report
# is actionable the moment it is printed - not after a 5-minute build.  A lane launched with its cwd set to
# MAIN leaves exactly these behind (`.tmp-mwcc/upstream/` was the 2026-09-27 case).
LANE_SCRATCH_MARKERS = (".tmp-", ".ws-", "tmp-")


def likely_cause(path: str, main: str | None = None) -> str | None:
    """A named cause for a foreign path that looks like lane scratch - `None` when the name says nothing.

    Deliberately name-based and conservative: this only *suggests* a cause in the pre-flight report, it never
    changes the verdict.  What it names is the failure this lane exists for - a lane launched in MAIN rather
    than in its slot.  A refused `config.yml` (with `main`) names the hook's own reason.
    """
    if main is not None and path == common.CONFIG_PATH:
        return ("config.yml may change only in block_relocations/add_relocations: %s"
                % common.config_verdict(main)["reason"])
    parts = path.replace("\\", "/").split("/")
    if any(p.startswith(m) for p in parts for m in LANE_SCRATCH_MARKERS):
        return ("looks like lane scratch in MAIN - a lane was launched with its cwd set to MAIN (or cloned "
                "its upstream there) instead of working in its slot; delete it and relaunch the lane with "
                "the slot as its cwd (`queue.py next` prints that line)")
    if any(p.startswith(".slot") for p in parts):
        return ("a slot directory sits inside MAIN - slots are siblings of MAIN, never inside it")
    if "upstream" in parts:
        return ("a cloned upstream repository left in MAIN - typically the same mis-launched lane")
    return None


def preflight_foreign(main: str) -> list[dict]:
    """Foreign paths already in `main` **before** the build, each with a likely cause.  Read-only.

    The same information the post-build refusal prints (`land`'s `paths outside the batch appeared during the
    build`), delivered *before* the expensive work: a refusal that arrives after a 5-minute build is the same
    information, late.  It does not change the verdict - a path that appears during the build is still refused
    afterwards - it just makes the common case (a foreign path already there) cost a second, not minutes.
    """
    rows = changed_status(main)
    outside = outside_batch([path for _code, path in rows], main=main)
    scratch = set(scratch_paths(outside))
    return [{"path": p, "cause": likely_cause(p, main)} for p in outside if p not in scratch]


def preflight_report(main: str, foreign: list[dict] | None = None) -> str | None:
    """The pre-flight report line(s), or `None` when the tree already holds no foreign path."""
    foreign = preflight_foreign(main) if foreign is None else foreign
    if not foreign:
        return None
    lines = ["PRE-FLIGHT | %d path(s) outside this batch are already in %s BEFORE the build:"
             % (len(foreign), main)]
    for row in foreign:
        lines.append("  %s%s" % (row["path"], "  <- %s" % row["cause"] if row["cause"] else ""))
    lines.append("  the post-build gate would refuse these too; they are another lane's or a mis-launch's "
                 "scratch, not this batch's - remove them first")
    return "\n".join(lines)


# --- the rows -------------------------------------------------------------------------------------------------

def ground_truth_row(b: Batch) -> None:
    """1. `build.sha1` still states the original DOL's hash (`prepcommit.ground_truth_error`), read in the tree
    the gate judges (`b.main`), not in the tree the tool's code happens to live in."""
    truth = pc.ground_truth_error(os.path.join(b.main, pc.ORIGINAL_DOL),
                                  os.path.join(b.main, "config", "RMHE08", "build.sha1"))
    b.check("ground truth (build.sha1 == the DOL's hash)", not truth, "; ".join(truth))


def base_row(b: Batch) -> None:
    """2. main has not moved since the batch base, and a base was recorded at all (BOOKKEEPING)."""
    head = git(["rev-parse", "HEAD"], b.main).strip()
    if b.base:
        b.check("main has not moved since the batch base", head == b.base,
                "HEAD %s != base %s - a worker committed to main, or another stream landed"
                % (head[:8], b.base[:8]), info="base %s" % (b.base or "?")[:8],
                kind=KIND_BOOKKEEPING,
                remedy="re-record the batch base (`python tools/units/land.py record-base`) once main is the "
                       "tree the batch applies to; if a worker committed to main, undo that first")
    else:
        b.check("batch base recorded", False, "no base: run `land.py record-base` when the batch opens",
                kind=KIND_BOOKKEEPING,
                remedy="run `python tools/units/land.py record-base` when the batch opens, then re-run the "
                       "landing")


def paths_row(b: Batch) -> None:
    """3. every changed path belongs to a batch; tool scratch is tolerated, named and (unless dry-run) de-indexed."""
    b.paths = changed_paths(b.main)
    bad = outside_batch(b.paths, main=b.main)
    b.scratch = scratch_paths(bad)
    bad = [p for p in bad if p not in b.scratch]
    if b.scratch:
        # tolerated, but NAMED: the guard's intent is a loud refusal for foreign work, and a path it refuses
        # must never have been staged by this gate - so tolerated scratch is reported, not swallowed.
        print(tolerate_scratch(b.main, b.scratch, act=not b.dry_run), file=sys.stderr)
    retired = common.retired_paths(b.main)
    why = []
    if bad:
        why.append("not allowed in a batch: %s" % ", ".join(bad))
    if retired:
        why.append("added under a retired root (include/ is gone - a header lives beside its source under src/, "
                   "owner's ruling 2026-10-05): %s" % ", ".join(retired))
    b.check("every changed path belongs to a batch", not bad and not retired, "; ".join(why),
            info=("tool scratch tolerated (not staged): %s" % ", ".join(b.scratch)) if b.scratch else "")


def conflict_marker_row(b: Batch) -> None:
    """3b. no batch file carries a git conflict marker (scoped to the batch's own stageable files)."""
    markers = conflict_marker_files(b.main, land_stageable(b.units, changed_status(b.main),
                                                         base_dirty_paths(b.main), main=b.main))
    b.check("no batch file carries a git conflict marker", not markers,
            "; ".join("%s:%d %s" % (p, n, m) for p, n, m in markers[:6]),
            remedy="resolve the conflict in that file and re-commit it - a marker is not source, and the build "
                   "only reports it as a syntax error, in a file that need not be the one the merge touched")


def report_base_row(b: Batch) -> None:
    """2b. the base's `report.json` was rebuilt at `record-base` (`base.rebuild_report`) without failing and without
    changing the tree (BOOKKEEPING). No row when it was, or when the base predates the rebuild (WP4)."""
    built = b.recorded.get("report_build")
    if not built or (built.get("returncode") == 0 and not built.get("dirtied")):
        return
    why = ("`ninja build/RMHE08/report.json` exited %d: %s" % (built.get("returncode"), built.get("detail", ""))
           if built.get("returncode") != 0 else
           "the rebuild changed tracked or untracked paths: %s" % ", ".join(built.get("dirtied", [])[:6]))
    b.check("the batch base's report.json was rebuilt at record-base", False, why, kind=KIND_BOOKKEEPING,
            remedy="make `ninja build/RMHE08/report.json` succeed on MAIN at the base (and leave the tree clean), "
                   "then re-run the landing - it re-records the base; a base whose scores are not its own tree's "
                   "judges the batch against the wrong numbers")
