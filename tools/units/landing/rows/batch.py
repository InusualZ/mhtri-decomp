"""The batch rows: every unit's outbox validates and its branch carries its work as commits (BOOKKEEPING).
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os
import sys

from tools.lib.lanes import naming
from tools.lib.lanes import registry
import tools.units.handoff as handoff_mod
from tools.units.landing.common import Batch, KIND_BOOKKEEPING, run


def branch_commits(main: str, unit: str) -> int:
    """How many commits the unit's worker branch has that main does not already have.

    The batch base is deliberately *not* part of this. A worker's branch is cut when the unit is claimed,
    and the orchestrator records a fresh base for every batch it lands, so `base..branch` is only
    meaningful while that base is main's HEAD; in a multi-batch round the recorded base is stale, which
    makes the count report 0 for work that *is* committed (or miss it entirely). The check's intent is
    simply "the worker's work exists as commits of its own on its branch that landing has not taken yet" -
    `main..branch`. A branch that predates the base but carries its own commit passes; a branch with
    nothing beyond main fails.

    The branch comes from the claim (`registry.claim_branch`), not from re-deriving it here: the unit may be
    spelled with or without its source extension, and the stored branch is the lock.
    """
    unit = naming.norm_unit(unit.strip("/"))
    branch = registry.claim_branch(main, unit)
    if not registry.branch_exists(main, branch):
        return 0
    p = run(["git", "rev-list", "--count", "main..%s" % branch], main)
    return int(p.stdout.strip()) if p.returncode == 0 and p.stdout.strip().isdigit() else 0


def outbox_skip_note(branch: str | None) -> str | None:
    """Why the outbox row is skipped for `land --branch <branch>`, or None when it is checked.

    Only a `worker/<slug>` branch is a claim, and only a claim's lane writes `.pi/outbox/<slug>.json`; a pilot or
    plain worktree branch never has one, so demanding it made every such landing pass `--no-outbox`, which also
    silences the row for a real claim named beside it. A `worker/*` branch stays strict, and so does a landing
    with no `--branch` (`--units` names claims)."""
    if not branch or branch.startswith(naming.BRANCH_PREFIX):
        return None
    return ("outbox row skipped: %s is not a %s<slug> claim branch, so no per-unit outbox is expected"
            % (branch, naming.BRANCH_PREFIX))


def outbox_units(main: str, units: list[str], branch: str | None = None) -> tuple[list[str], list[str]]:
    """-> (units whose outbox validates, problems).

    `branch`, when given (`land --branch`), locates the outbox by the branch's slug - the name `brief.py`
    writes - instead of re-deriving it from the unit path.  That is what covers a unit **renamed at
    registration**: its outbox keeps the pre-registration slug, so the unit-derived path misses it while
    the branch-derived one finds it.  When the branch-derived path is missing too, the failure names
    `--no-outbox` as the remedy, because a missing record must be stated plainly, not hidden.

    The outbox is validated against the **batch's** owned symbols, not one unit's: a branch that registers or
    touches several units writes one outbox naming all of them, and a per-unit check flagged every other
    unit's symbols as "not owned".
    """
    ok, problems = [], []
    branch_slug = naming.slug_of_branch(branch) if branch else None
    # The ownership check reads the *batch's* units as one set: a branch that registers or touches several
    # units writes one outbox naming all of them, and validating it against a single unit flags every symbol
    # of the others as "not owned" - a refusal whose only documented escape is `--no-outbox`, which turns the
    # outbox check off entirely. A symbol owned by no unit in the batch is still an error, and a single-unit
    # batch is unchanged.
    batch_owned = handoff_mod.owned_symbols(main, [naming.norm_unit(u.strip("/")) for u in units])
    for unit in units:
        unit = naming.norm_unit(unit.strip("/"))
        if branch_slug:
            path = os.path.join(main, ".pi", "outbox", branch_slug + ".json")
        else:
            path = handoff_mod.outbox_path(main, unit)
        if not os.path.exists(path):
            hint = (" (a unit renamed at registration keeps its outbox under the pre-registration slug; "
                    "if the record is demonstrably fine, --no-outbox is the remedy)")
            problems.append("%s: no outbox at %s%s" % (unit, path, hint))
            continue
        entry = json.loads(open(path, encoding="utf-8").read())
        errors, _warnings = handoff_mod.validate(entry, batch_owned)
        if errors:
            problems.extend("%s: %s" % (unit, e) for e in errors)
        else:
            ok.append(unit)
    return ok, problems


def restore_rescued_branch(main: str, unit: str) -> str | None:
    """Put a `--force`-released worker's branch back from its rescue ref. Returns the branch, or None.

    A `release --force` deletes the branch (it is the lock and must go) and parks its only copy of the work
    at `refs/rescue/<slug>`. What the gate cares about is that the work *exists as commits*, so rather than
    refuse a missing branch whose work is demonstrably preserved - the 2026-09-26 dead end, where land.py's
    own refusal named the rescue ref and the exact `git branch` command and the round still had to run it by
    hand before a manual commit - the gate restores the branch itself and carries on. Nothing is destroyed:
    the branch points at the rescue ref's commit, and a later teardown sees it exactly as the normal flow
    would (merged into main after the landing, so the release removes it).
    """
    branch = registry.claim_branch(main, unit)
    if registry.branch_exists(main, branch):
        return None
    rescue = registry.rescue_exists(main, unit)
    if not rescue:
        return None
    p = run(["git", "update-ref", "refs/heads/%s" % branch, rescue], main)
    return branch if p.returncode == 0 else None


def commits_ahead_of_main(main: str, ref: str) -> bool:
    """True when `ref` carries commits `main` does not already have - the gate's "the work exists" test."""
    p = run(["git", "rev-list", "--count", "main..%s" % ref], main)
    return p.returncode == 0 and p.stdout.strip().isdigit() and int(p.stdout.strip()) > 0


def branch_problems(main: str, units: list[str], branch: str | None = None) -> list[str]:
    """One line per unit whose worker branch does not carry its work as commits.

    A missing branch is the 2026-09-23 shape: `release --force` on an unreported worker deleted the branch
    (its only copy of the work) and left it at `refs/rescue/<slug>`. What the gate actually needs is that the
    work *exists as commits*, so a rescue ref that carries commits `main` does not is accepted as the
    branch's work - the 2026-09-26 report's case (a). The landing path additionally restores the real branch
    from that ref (`restore_rescued_branch`, and only when not `--dry-run`, which touches nothing), so the
    teardown still has a branch to release. A missing branch with no rescue ref behind it, or a rescue ref
    with no commits of its own, is still reported.

    `branch`, when given (`land --branch`), is the branch being landed and is checked directly - a unit
    renamed at registration is not reachable through its registered name, but its branch is what was named.
    """
    if branch:
        if registry.branch_exists(main, branch) and commits_ahead_of_main(main, branch):
            return []
        return ["%s (branch %s has no commits of its own)" % (branch, branch)]
    problems = []
    for u in units:
        branch = registry.claim_branch(main, u)
        if not registry.branch_exists(main, branch):
            rescue = registry.rescue_exists(main, u)
            if rescue and commits_ahead_of_main(main, rescue):
                continue          # the work exists as commits at the rescue ref: that is what the gate wants
            if rescue:
                problems.append("%s (rescue ref %s has no commits of its own; no branch %s)"
                                % (u, rescue, branch))
            else:
                problems.append("%s (no branch %s)" % (u, branch))
        elif branch_commits(main, u) == 0:
            problems.append("%s (branch %s has no commits of its own)" % (u, branch))
    return problems


# --- the rows -------------------------------------------------------------------------------------------------

def outbox_rows(b: Batch) -> None:
    """4. every unit's outbox validates and its branch carries its work as commits (BOOKKEEPING); an
    orchestrator-only batch says so; a batch that names no unit is refused."""
    if b.unit_units and b.check_outbox:
        # NOTE: a fresh name for the outbox problems. Reusing the `problems` out-parameter here rebound it
        # locally and the failed-check list never reached the caller's `land` refusal (2026-09-26).
        skip_note = outbox_skip_note(b.branch)
        if skip_note:
            print("NOTE: " + skip_note, file=sys.stderr)
            b.check("every unit's outbox validates", True, info=skip_note)
        else:
            _ok_units, outbox_problems = outbox_units(b.main, b.unit_units, branch=b.branch)
            b.check("every unit's outbox validates", not outbox_problems, "; ".join(outbox_problems[:4]),
                    kind=KIND_BOOKKEEPING,
                    remedy="the source is fine - have the worker re-run brief.py to rewrite its outbox, or re-run "
                           "with --no-outbox for an orchestrator-only batch")
        # a `--force` release leaves the work at refs/rescue/<slug>. `branch_problems` already accepts that
        # ref as the branch's work, and the landing path (never `--dry-run`, which touches nothing) restores
        # the real branch from it so the teardown still has a branch to release (2026-09-26 case (a)).
        if not b.dry_run:
            for u in b.unit_units:
                restored = restore_rescued_branch(b.main, u)
                if restored:
                    print("NOTE: %s's branch %s was gone but its work is preserved at %s - restored the "
                          "branch from the rescue ref (a `--force` release had parked it there)"
                          % (u, restored, naming.rescue_ref(u)), file=sys.stderr)
        uncommitted = branch_problems(b.main, b.unit_units, branch=b.branch)
        b.check("every unit's branch carries its work as commits", not uncommitted,
                "no commits of its own on the branch (work left uncommitted in the worktree?): %s"
                % ", ".join(uncommitted),
                kind=KIND_BOOKKEEPING,
                remedy="if the commits are at refs/rescue/<slug> land.py restores the branch for you; "
                       "otherwise have the worker commit its work on the branch, then re-run")
    elif b.units:
        b.check("orchestrator-only batch (no worker outboxes to check)", True,
                info="%d unit(s): %s" % (len(b.units), ", ".join(b.units)))
    else:
        b.check("batch units named", False, "pass --units (or --no-outbox for an orchestrator-only batch)",
                kind=KIND_BOOKKEEPING,
                remedy="pass --units a,b (or --no-outbox for an orchestrator-only batch)")
