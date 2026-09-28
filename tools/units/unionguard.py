#!/usr/bin/env python3
"""Refuse to union-resolve a conflicted branch-apply when the conflict is not a disjoint addition.

`.pi/bin/applybranch.sh` lands a worker branch as `git apply -3 <merge-base diff>` and then resolves every
conflicted path with a plain **union** (`.pi/bin/union.py`: ours block, then theirs block).  That union is
only correct when the two sides touched *disjoint* things - two units each appending a registration at the
same anchor.  When the two sides changed the *same* existing content it silently writes a tree that cannot
build, and the damage is only found later by `ninja`:

* **Deletes and renames (measured 2026-09-25, commit `6fceb8f7`).**  A `.c -> .cpp` rename made the branch's
  copy of `configure.py` still carry the old `g3d/g3d_resanmcamera.c` registration while main had already
  dropped it.  The conflict's base side was non-empty; the union wrote ours-then-theirs and kept the stale
  `.c` line, so configure.py registered both `g3d_resanmcamera.cpp` **and** `g3d_resanmcamera.c`, one of
  which does not exist.  (A rename arrives as delete+add, so the union resurrects the deleted half.)
* **Both sides editing one file (measured 2026-09-25, worker/80093990).**  Ten shared `g3d` headers had moved
  on in main; the union kept both sides' declarations and 12 objects failed to compile ("illegal function
  overloading").

The signal that separates the two is the **base section** of each conflict hunk, which `git merge-file
--diff3` makes explicit (this tool recomputes the three-way merge from the index's stage blobs, so it does
not depend on the working tree's conflict-marker style):

* base section **empty**  -> both sides *inserted* new lines at the same anchor.  Disjoint addition.  Union.
* base section **non-empty** -> one or both sides changed/removed existing lines.  Overlap.  Refuse.
* a stage 1 (base) entry with **no stage 2/3** -> that side *deleted* the path.  Refuse.
* a rename in either side's diff that touches a conflicted path -> Refuse (the deleted half would be
  resurrected).

For every conflicted path the tool prints what each side did (deleted / renamed / modified) and exits
**non-zero** with the offending paths named, so `applybranch.sh` can stop *before* it stages a tree that
cannot build.  A tree whose conflicts are all disjoint additions still unions exactly as before.

**A refusal also undoes the apply.**  `applybranch.sh` runs this guard *after* `git apply -3`, which has
already merged into the index: UU/AA entries for the conflicts and cleanly-applied hunks staged.  Refusing
and then only printing "resolve these by hand" left `main` mid-conflict - staged files, conflict stages,
and a `ninja` that cannot regenerate `build.ninja` - until somebody cleaned it up by hand.  So on refusal
the guard restores every path the branch's diff touched (`git checkout HEAD -- <path>` when HEAD has it,
a delete when it does not) and runs `git reset` to drop the index stages, printing exactly what it
restored and anything still dirty.  `--no-cleanup` keeps the old behaviour for a deliberate inspection.

    python tools/units/unionguard.py --branch worker/<slug> [--base <ref>] [path ...]
    python tools/units/unionguard.py --branch worker/<slug> --no-cleanup [--base <ref>] [path ...]
    python tools/units/unionguard.py --selftest

`applybranch.sh` passes the branch and the conflicted paths; with no paths it inspects `git ls-files -u`.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile

BASE = "main"


def _git(cwd: str, *args: str) -> bytes:
    p = subprocess.run(["git", *args], cwd=cwd, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args),
                                                  p.stderr.decode("utf-8", "replace").strip()))
    return p.stdout


def unmerged(cwd: str) -> dict[str, dict[int, str]]:
    """`git ls-files -u` as {path: {stage: blob-sha}}; stage 1=base, 2=ours, 3=theirs."""
    entries: dict[str, dict[int, str]] = {}
    raw = _git(cwd, "ls-files", "-u", "-z").decode("utf-8", "replace")
    for record in raw.split("\0"):
        if not record:
            continue
        meta, _, path = record.partition("\t")
        _mode, sha, stage = meta.split()
        entries.setdefault(path, {})[int(stage)] = sha
    return entries


def _cat(cwd: str, sha: str) -> bytes:
    p = subprocess.run(["git", "cat-file", "blob", sha], cwd=cwd, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError("git cat-file blob %s failed" % sha)
    return p.stdout


def has_base_region(text: str) -> bool:
    """True iff any conflict hunk in a `--diff3` merge output has a non-empty base section.

    An empty base section is the disjoint-additions case (both sides inserted at the anchor); a non-empty
    one means existing lines were changed or removed, which ours-then-theirs cannot merge.
    """
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        if lines[i].startswith("<<<<<<<"):
            i += 1
            while i < len(lines) and not lines[i].startswith(("|||||||", "=======")):
                i += 1
            base: list[str] = []
            if i < len(lines) and lines[i].startswith("|||||||"):
                i += 1
                while i < len(lines) and not lines[i].startswith("======="):
                    base.append(lines[i])
                    i += 1
            if any(line.strip() for line in base):
                return True
            while i < len(lines) and not lines[i].startswith(">>>>>>>"):
                i += 1
        i += 1
    return False


def three_way_overlap(cwd: str, stage1: str, stage2: str, stage3: str) -> bool:
    """Whether both sides changed the same *existing* content of this path."""
    with tempfile.TemporaryDirectory() as tmp:
        paths = {}
        for label, sha in (("ours", stage2), ("base", stage1), ("theirs", stage3)):
            path = os.path.join(tmp, label)
            with open(path, "wb") as fh:
                fh.write(_cat(cwd, sha))
            paths[label] = path
        p = subprocess.run(["git", "merge-file", "-p", "--diff3",
                            "-L", "ours", "-L", "base", "-L", "theirs",
                            paths["ours"], paths["base"], paths["theirs"]],
                           capture_output=True)
    if p.returncode not in (0, 1):
        raise RuntimeError("git merge-file failed: " + p.stderr.decode("utf-8", "replace").strip())
    if p.returncode == 0:
        return False
    return has_base_region(p.stdout.decode("utf-8", "replace"))


def rename_sets(cwd: str, base: str, ref: str) -> tuple[set[str], set[str]]:
    """(rename sources, rename destinations) for `base..ref`, using git's own rename detection."""
    sources: set[str] = set()
    destinations: set[str] = set()
    try:
        out = _git(cwd, "diff", "--name-status", "-M", "--find-renames", base, ref)
    except RuntimeError:
        return sources, destinations
    for line in out.decode("utf-8", "replace").splitlines():
        parts = line.split("\t")
        if parts and parts[0].startswith("R") and len(parts) == 3:
            sources.add(parts[1])
            destinations.add(parts[2])
    return sources, destinations


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
    """Every path the branch's own diff touches, both halves of a rename, in diff order.

    `git apply -3 <merge-base diff>` touches exactly these paths, so undoing them all restores the tree.
    `--name-status -z` is used so a path with a space (or a rename) parses unambiguously; `--name-only`
    would drop the source half of a rename.
    """
    if not (base and branch):
        return []
    fields = _git(cwd, "diff", "--name-status", "-z", "-M", base, branch).decode(
        "utf-8", "replace").split("\0")
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
    seen: set[str] = set()
    ordered: list[str] = []
    for path in paths:
        if path not in seen:
            seen.add(path)
            ordered.append(path)
    return ordered


def _in_head(cwd: str, path: str) -> bool:
    p = subprocess.run(["git", "cat-file", "-e", "HEAD:" + path], cwd=cwd, capture_output=True)
    return p.returncode == 0


def cleanup_applied(cwd: str, base: str | None, branch: str | None
                    ) -> tuple[list[str], list[str], list[str]]:
    """Undo `git apply -3 <merge-base diff>` so a refusal leaves a clean tree, not a conflict.

    Every path the diff touched is restored: `git checkout HEAD -- <path>` for a path HEAD has (this also
    resolves a UU entry back to HEAD's stage-0 blob) and a working-tree delete for a path only the branch
    adds.  A final `git reset` drops the index stages `git apply -3` wrote, including the cleanly-applied
    hunks that never conflicted.  Returns (restored, removed, leftover status lines) so the caller prints
    exactly what happened and reveals anything the cleanup could not fix.
    """
    restored: list[str] = []
    removed: list[str] = []
    for path in diff_paths(cwd, base, branch):
        if _in_head(cwd, path):
            p = subprocess.run(["git", "checkout", "HEAD", "--", path], cwd=cwd, capture_output=True)
            if p.returncode == 0:
                restored.append(path)
        else:
            target = os.path.join(cwd, path)
            if os.path.lexists(target):
                os.remove(target)
            removed.append(path)
    subprocess.run(["git", "reset", "-q"], cwd=cwd, capture_output=True)
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


# --------------------------------------------------------------------------------------------------
# self-test: real git repos and real unmerged indexes, no repository state and no build.
# --------------------------------------------------------------------------------------------------

def _run(cwd: str, *args: str):
    return subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")


def _init(cwd: str) -> None:
    _run(cwd, "init", "-q", "-b", "main")
    _run(cwd, "config", "user.email", "t@example.com")
    _run(cwd, "config", "user.name", "t")
    _run(cwd, "config", "core.autocrlf", "false")


def _write(cwd: str, name: str, text: str) -> None:
    with open(os.path.join(cwd, name), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)


def _read(cwd: str, name: str) -> str:
    with open(os.path.join(cwd, name), encoding="utf-8", newline="") as fh:
        return fh.read()


def _union_file(path: str) -> None:
    """Local mirror of `.pi/bin/union.py`: ours block, then theirs, for the pass-through test."""
    with open(path, encoding="utf-8", newline="") as fh:
        lines = fh.read().replace("\r\n", "\n").split("\n")
    out, i = [], 0
    while i < len(lines):
        if lines[i].startswith("<<<<<<<"):
            ours, theirs = [], []
            i += 1
            while not lines[i].startswith("======="):
                ours.append(lines[i]); i += 1
            i += 1
            while not lines[i].startswith(">>>>>>>"):
                theirs.append(lines[i]); i += 1
            i += 1
            out.extend(ours); out.extend(theirs)
            continue
        out.append(lines[i]); i += 1
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write("\n".join(out))


def _commit(cwd: str, message: str) -> None:
    _run(cwd, "add", "-A")
    _run(cwd, "commit", "-q", "-m", message)


def _stages(cwd: str, path: str, sha_map: dict[str, str]) -> None:
    """Build an unmerged index entry directly: sha_map is {stage: blob-sha}.

    `git apply -3` cannot express a modify/delete conflict (it aborts instead), so the delete and rename
    cases are pinned by constructing the same index stages it would leave.  A real conflict has *only*
    stage 1/2/3 entries, so the path's existing stage-0 entry is cleared first (otherwise the index keeps a
    stage 0 alongside the conflict stages, which is a state no apply produces).  The payload is bytes, not
    text: the Windows text layer turns `\\n` into `\\r\\n` and git then treats the trailing `\\r` as part of
    the path.
    """
    lines = ["0 0000000000000000000000000000000000000000\t%s\n" % path]
    lines += ["100644 %s %s\t%s\n" % (sha, stage, path)
              for stage, sha in sorted(sha_map.items())]
    payload = "".join(lines).encode()
    p = subprocess.run(["git", "update-index", "--index-info"], cwd=cwd, input=payload,
                       capture_output=True)
    if p.returncode != 0:
        raise RuntimeError(p.stderr.decode("utf-8", "replace"))


def _apply_conflict(cwd: str, base: str, branch: str) -> int:
    patch = _git(cwd, "diff", "--binary", base, branch)
    p = subprocess.run(["git", "apply", "-3", "-"], cwd=cwd, input=patch, capture_output=True)
    return p.returncode


def _conflict_paths(cwd: str) -> list[str]:
    return sorted(unmerged(cwd))


def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # --- disjoint additions: both sides append at the same anchor -> union is safe -------------------
    with tempfile.TemporaryDirectory() as tmp:
        _init(tmp)
        _write(tmp, "reg.txt", "anchor\n")
        _write(tmp, "keep.c", "int keep;\n")
        _commit(tmp, "base")
        _run(tmp, "checkout", "-q", "-b", "branch")
        _write(tmp, "reg.txt", "anchor\nbranch added\n")
        _commit(tmp, "branch append")
        _run(tmp, "checkout", "-q", "main")
        _write(tmp, "reg.txt", "anchor\nmain added\n")
        _commit(tmp, "main append")
        base = _run(tmp, "merge-base", "main", "branch").stdout.strip()
        check("disjoint: apply landed the conflict", _apply_conflict(tmp, base, "branch") in (0, 1), True)
        check("disjoint: reg.txt is the conflict", _conflict_paths(tmp), ["reg.txt"])
        check("disjoint: guard allows the union", report(tmp, base, "branch", []), 0)

    # --- both sides modified the same region -> refuse -----------------------------------------------
    with tempfile.TemporaryDirectory() as tmp2:
        _init(tmp2)
        _write(tmp2, "cfg.txt", "value = 0\n")
        _commit(tmp2, "base")
        _run(tmp2, "checkout", "-q", "-b", "branch")
        _write(tmp2, "cfg.txt", "value = 1\n")
        _commit(tmp2, "branch edits")
        _run(tmp2, "checkout", "-q", "main")
        _write(tmp2, "cfg.txt", "value = 2\n")
        _commit(tmp2, "main edits")
        base2 = _run(tmp2, "merge-base", "main", "branch").stdout.strip()
        check("both-modified: apply landed the conflict",
              _apply_conflict(tmp2, base2, "branch") in (0, 1), True)
        findings = {p: classify(tmp2, p, unmerged(tmp2)[p], (set(), set()),
                                rename_sets(tmp2, base2, "branch"))
                    for p in _conflict_paths(tmp2)}
        check("both-modified: cfg.txt is the conflict", sorted(findings), ["cfg.txt"])
        check("both-modified: flagged as overlap", findings["cfg.txt"]["both_modified"], True)
        check("both-modified: guard refuses", report(tmp2, base2, "branch", []), 1)

    # --- delete on one side: ours deleted, theirs modified -> refuse ----------------------------------
    with tempfile.TemporaryDirectory() as tmp3:
        _init(tmp3)
        _write(tmp3, "gone.c", "int a;\n")
        _write(tmp3, "reg.txt", "see gone.c\n")
        _commit(tmp3, "base")
        _run(tmp3, "checkout", "-q", "-b", "branch")
        _write(tmp3, "gone.c", "int a;\nint b;\n")
        _commit(tmp3, "branch edits gone.c")
        _run(tmp3, "checkout", "-q", "main")
        _run(tmp3, "rm", "-q", "gone.c")
        _commit(tmp3, "main deletes gone.c")
        base3 = _run(tmp3, "merge-base", "main", "branch").stdout.strip()
        base_blob = _git(tmp3, "rev-parse", base3 + ":gone.c").decode().strip()
        theirs_blob = _git(tmp3, "rev-parse", "branch:gone.c").decode().strip()
        _stages(tmp3, "gone.c", {"1": base_blob, "3": theirs_blob})
        check("delete-one-side: unmerged entry exists", "gone.c" in unmerged(tmp3), True)
        finding = classify(tmp3, "gone.c", unmerged(tmp3)["gone.c"],
                           (set(), set()), rename_sets(tmp3, base3, "branch"))
        check("delete-one-side: flagged deleted by ours", finding["ours"], "deleted")
        check("delete-one-side: guard refuses", report(tmp3, base3, "branch", []), 1)
        check("delete-one-side: cleanup left a clean tree",
              _run(tmp3, "status", "--porcelain").stdout.strip(), "")

    # --- rename pair: the renamed path is conflicted -> refuse ----------------------------------------
    with tempfile.TemporaryDirectory() as tmp4:
        _init(tmp4)
        _write(tmp4, "a.c", "int a;\n")
        _write(tmp4, "reg.txt", "see a.c\n")
        _commit(tmp4, "base")
        _run(tmp4, "checkout", "-q", "-b", "branch")
        _run(tmp4, "mv", "a.c", "a.cpp")
        _write(tmp4, "reg.txt", "see a.cpp\n")
        _commit(tmp4, "rename a.c -> a.cpp")
        _run(tmp4, "checkout", "-q", "main")
        _write(tmp4, "a.c", "int a; // main edit\n")
        _commit(tmp4, "main edits a.c")
        base4 = _run(tmp4, "merge-base", "main", "branch").stdout.strip()
        srcs, _dsts = rename_sets(tmp4, base4, "branch")
        check("rename: git sees the rename", "a.c" in srcs, True)
        base_blob4 = _git(tmp4, "rev-parse", base4 + ":a.c").decode().strip()
        ours_blob4 = _git(tmp4, "rev-parse", "main:a.c").decode().strip()
        _stages(tmp4, "a.c", {"1": base_blob4, "2": ours_blob4})
        finding = classify(tmp4, "a.c", unmerged(tmp4)["a.c"],
                           rename_sets(tmp4, base4, "HEAD"),
                           rename_sets(tmp4, base4, "branch"))
        check("rename: flagged as renamed", "theirs" in finding["renamed"], True)
        check("rename: flagged as deleted by theirs", finding["theirs"], "deleted")
        check("rename: guard refuses", report(tmp4, base4, "branch", []), 1)
        check("rename: cleanup left a clean tree",
              _run(tmp4, "status", "--porcelain").stdout.strip(), "")

    # --- a refusal undoes the apply: clean tree, no UU/AA, nothing staged -----------------------------
    with tempfile.TemporaryDirectory() as tmp5:
        _init(tmp5)
        _write(tmp5, "cfg.txt", "value = 0\n")
        _write(tmp5, "shared.c", "int keep;\n")
        _commit(tmp5, "base")
        _run(tmp5, "checkout", "-q", "-b", "branch")
        _write(tmp5, "cfg.txt", "value = 1\n")
        _write(tmp5, "shared.c", "int keep;\nint from_branch;\n")
        _write(tmp5, "new.c", "int fresh;\n")
        _commit(tmp5, "branch edits cfg.txt, appends shared.c, adds new.c")
        _run(tmp5, "checkout", "-q", "main")
        _write(tmp5, "cfg.txt", "value = 2\n")
        _commit(tmp5, "main edits cfg.txt")
        base5 = _run(tmp5, "merge-base", "main", "branch").stdout.strip()
        check("cleanup: apply landed the conflict",
              _apply_conflict(tmp5, base5, "branch") in (0, 1), True)
        head_cfg = _git(tmp5, "show", "HEAD:cfg.txt").decode("utf-8", "replace")
        check("cleanup: guard refuses", report(tmp5, base5, "branch", []), 1)
        check("cleanup: no unmerged entries left", _conflict_paths(tmp5), [])
        check("cleanup: nothing staged or dirty",
              _run(tmp5, "status", "--porcelain").stdout.strip(), "")
        check("cleanup: cfg.txt is HEAD's version again", _read(tmp5, "cfg.txt"), head_cfg)
        check("cleanup: the cleanly-applied hunk was undone too",
              _read(tmp5, "shared.c"), "int keep;\n")
        check("cleanup: the branch's added file was removed",
              os.path.exists(os.path.join(tmp5, "new.c")), False)

    # --- --no-cleanup keeps the conflicted index for a deliberate inspection --------------------------
    with tempfile.TemporaryDirectory() as tmp6:
        _init(tmp6)
        _write(tmp6, "cfg.txt", "value = 0\n")
        _commit(tmp6, "base")
        _run(tmp6, "checkout", "-q", "-b", "branch")
        _write(tmp6, "cfg.txt", "value = 1\n")
        _commit(tmp6, "branch edits")
        _run(tmp6, "checkout", "-q", "main")
        _write(tmp6, "cfg.txt", "value = 2\n")
        _commit(tmp6, "main edits")
        base6 = _run(tmp6, "merge-base", "main", "branch").stdout.strip()
        check("no-cleanup: apply landed the conflict",
              _apply_conflict(tmp6, base6, "branch") in (0, 1), True)
        check("no-cleanup: exit stays non-zero",
              main(["--cwd", tmp6, "--base", base6, "--branch", "branch", "--no-cleanup"]), 1)
        check("no-cleanup: conflict is still there", _conflict_paths(tmp6), ["cfg.txt"])
        check("no-cleanup: the stage is still UU",
              _run(tmp6, "status", "--short").stdout.startswith("UU "), True)

    # --- disjoint additions still pass through and union to both additions ----------------------------
    with tempfile.TemporaryDirectory() as tmp7:
        _init(tmp7)
        _write(tmp7, "reg.txt", "anchor\n")
        _commit(tmp7, "base")
        _run(tmp7, "checkout", "-q", "-b", "branch")
        _write(tmp7, "reg.txt", "anchor\nbranch added\n")
        _commit(tmp7, "branch append")
        _run(tmp7, "checkout", "-q", "main")
        _write(tmp7, "reg.txt", "anchor\nmain added\n")
        _commit(tmp7, "main append")
        base7 = _run(tmp7, "merge-base", "main", "branch").stdout.strip()
        check("disjoint-union: apply landed the conflict",
              _apply_conflict(tmp7, base7, "branch") in (0, 1), True)
        check("disjoint-union: guard passes it through", report(tmp7, base7, "branch", []), 0)
        check("disjoint-union: conflict still present for union.py",
              _conflict_paths(tmp7), ["reg.txt"])
        _union_file(os.path.join(tmp7, "reg.txt"))
        text = _read(tmp7, "reg.txt")
        check("disjoint-union: union keeps both additions",
              "main added" in text and "branch added" in text, True)
        check("disjoint-union: union left no markers", "<<<<<<<" in text, False)

    if fails:
        print("unionguard: %d check(s), %d failure(s)" % (checks, len(fails)))
        for f in fails:
            print("  FAIL " + f)
        return 1
    print("unionguard: %d check(s), 0 failure(s)" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="*", help="conflicted paths (default: every unmerged path)")
    ap.add_argument("--cwd", default=os.getcwd(), help="repository the apply ran in (default: cwd)")
    ap.add_argument("--base", default=None, help="merge-base ref (default: git merge-base main <branch>)")
    ap.add_argument("--branch", default=None, help="the branch being landed")
    ap.add_argument("--no-cleanup", action="store_true",
                    help="on refusal, leave the conflicted index in place for inspection")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()

    base = args.base
    if base is None:
        base = (_git(args.cwd, "merge-base", BASE, args.branch).decode("utf-8", "replace").strip()
                if args.branch else None)
    return report(args.cwd, base, args.branch, args.paths, cleanup=not args.no_cleanup)


if __name__ == "__main__":
    sys.exit(main())
