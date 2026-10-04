"""What `land` stages and commits: the batch's own paths, by pathspec, never another stream's work.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import subprocess

from tools.units.landing.common import (KIND_BOOKKEEPING, KIND_GATE, git, is_scratch, outside_batch, run,
    unit_owned_paths)


def land_decision(gate_ok: bool, stageable: list[str],
                  failed: list[str] | None = None,
                  kinds: set[str] | None = None) -> tuple[str, str]:
    """What `land` does after the gate: -> (`"commit"` | `"refuse"`, reason).

    The one command has to be safe when its output is piped (the exit status is then lost): the gate's verdict
    *is* the decision, and a red gate can never reach `git commit`. A green gate with nothing to stage is also
    a refusal - there is no batch to land. A refusal on a failed gate NAMES the failing check(s) and what each
    printed (`failing_checks`): a bare "the gate failed" is not actionable, and a reader who cannot see which
    gate failed cannot tell a real defect from a passing check.

    `kinds` is the set of check KINDS that failed. A BOOKKEEPING-only refusal says so and never "the gate
    failed": the batch is fine and the fix is to the landing's own state, so a reader must not reach for the
    manual-landing fallback that a real gate failure would (and must) stop. With no `kinds` the conservative
    GATE wording is kept, so an unfurnished caller is never told a red batch is merely bookkeeping.
    """
    if not gate_ok:
        kinds = set(kinds or [KIND_GATE])
        if kinds == {KIND_BOOKKEEPING}:
            why = ("BOOKKEEPING refusal - the batch itself passed, the landing's own state is stale (this is "
                   "NOT a gate failure) - nothing staged or committed")
        elif kinds == {KIND_GATE}:
            why = "the gate failed - nothing staged or committed"
        else:
            why = ("BOTH kinds failed - GATE (the batch itself) and BOOKKEEPING (the landing's own state) - "
                   "nothing staged or committed")
        return "refuse", why + (": %s" % "; ".join(failed) if failed else "")
    if not stageable:
        return "refuse", "the gate passed but no batch path is stageable - nothing to commit"
    return "commit", ""


def land_stageable(units: list[str], rows: list[tuple[str, str]],
                   base_dirty: set[str] | None = None) -> list[str]:
    """The paths `land` stages: the batch's own files, never another stream's in-flight work.

    A *tracked* change inside the allowed set is part of the batch (the cherry-pick, the shared-file edits) -
    except under `tools/`: that tree is where every worker keeps its own in-flight tools, so a tracked
    `tools/` change belongs to the batch only when the batch *names* that path as one of its units. Without
    this, `tools/units/langcheck.py` was a tracked change inside the allowed set, `land` staged it, and the
    batch's commit carried another stream's work (`85f3d4b5`, `d50fdd32`). An **untracked** file is staged
    when it is a named unit's own path or lives under `src/`/`include/` (source is the batch's), but not
    otherwise.

    `base_dirty` is the set `record_base` snapshotted when the batch opened: a path that was already dirty
    then is foreign, not batch material, even inside the allowed set (`docs/plan.md` under `85ddd7b6`,
    `src/RSO/runtime.c` under `890631e8`). The batch still stages a path it *names* as one of its units, so a
    unit the batch is genuinely working on keeps its existing behaviour.

    Tool scratch (`is_scratch`) is never staged - the batch did not receive it (the `d910.json` refusal).
    """
    owned = unit_owned_paths(units)
    foreign = base_dirty or set()
    stageable = []
    for code, path in rows:
        if outside_batch([path]):
            continue
        if is_scratch(path):
            continue          # an objdiff dump the batch never received, staged or not (the crossing point)
        if path in foreign and path not in owned:
            continue          # already dirty at the batch base: another stream's work, leave it alone
        if code.startswith("??") and path not in owned and not path.startswith(("src/", "include/")):
            continue
        if path.startswith("tools/") and path not in owned:
            continue
        stageable.append(path)
    return stageable


def looks_already_applied(rows: list[tuple[str, str]], base_dirty: set[str] | None) -> bool:
    """True when the batch was applied to the tree *before* `record-base` ran.

    The signature: every changed path the batch guard allows is in the base's `dirty_at_base` snapshot, so
    `land_stageable` excludes them all as another stream's work and there is nothing left to stage. That is
    exactly what `record-base` running *after* the apply looks like - the snapshot it took recorded the
    batch's own edits as pre-existing. The batch is fine; the *ordering* was wrong (the 2026-09-26 case (b),
    where `land` refused with "the gate passed but no batch path is stageable" straight after "READY: every
    check passed", and the round had to commit by hand).

    A batch with no changed path at all is not this: there is genuinely nothing to commit, and the plain
    "nothing to commit" refusal is the right one. Neither is a batch that adds a path the base did not
    already hold - only a *wholly* pre-existing dirty set has the signature.
    """
    allowed = [p for _code, p in rows if not outside_batch([p]) and not is_scratch(p)]
    if not allowed:
        return False
    return set(allowed) <= (base_dirty or set())


def staged_elsewhere(main: str, stageable: list[str]) -> list[str]:
    """Paths already in the index that are not part of this batch - another stream's in-flight work.

    `land` commits with a pathspec, so these are never swept in; naming them is the warning that keeps the
    accident visible. The 2026-09-23 collision (a staged `tools/units/langcheck.py` landed under two unrelated
    unit commits, `85f3d4b5` and `d50fdd32`) happened because the commit had no pathspec and took the whole
    index.
    """
    staged = git(["diff", "--cached", "--name-only"], main).splitlines()
    return [p for p in staged if p and p not in stageable]


def foreign_warning(foreign: list[str]) -> str:
    """The warning `land` prints when the index holds paths outside the batch (a note, never a refusal)."""
    noun = "path" if len(foreign) == 1 else "paths"
    return ("WARNING: the index holds %d %s outside this batch - left staged, not committed: %s"
            % (len(foreign), noun, ", ".join(foreign)))


def commit_pathspec(main: str, msg_file: str, stageable: list[str]) -> subprocess.CompletedProcess:
    """Commit exactly the batch's paths. `git commit` with no pathspec commits the whole index; with one it
    commits the named paths (read from the working tree) and leaves every other staged path staged."""
    return run(["git", "commit", "-F", msg_file, "--", *stageable], main)


def stage_batch(main: str, stageable: list[str]) -> None:
    """`git add` the batch's paths. Deleted paths are left to the commit's pathspec: `git add` refuses a
    pathspec that matches no working-tree file, while `git commit -- <path>` records the deletion (staged or
    not) on its own. So a rename's source path can stay in the pathspec without breaking the staging step."""
    existing = [p for p in stageable if os.path.exists(os.path.join(main, p))]
    if existing:
        git(["add", "--", *existing], main)
