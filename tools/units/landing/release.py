"""The claim teardown a landing ends with: which units release (`release_plan`) and the release itself (`claims.release`).
Spec: docs/tools/spec/landing.md. CLI: none (the `land.py` gate's package)."""
from __future__ import annotations

from tools.lib import findings as _findings
import tools.units.claims as claims
from tools.units.landing.common import Batch, KIND_BOOKKEEPING


def release_plan(checks: list, units: list[str], release_claims: bool,
                 check_outbox: bool = True) -> list[str]:
    """The units whose claim `verify` releases: every gated unit, but only once every check so far passed.

    Releasing is a side effect, so it must not run behind a failed gate - a refused batch has to leave its
    worker's branch and worktree exactly as they are, or the retry has nothing to re-run. `--no-release`
    turns the step off entirely.

    `check_outbox` is an input that **does not affect the answer**, and that is the point: the old
    `--no-worker-units` flag skipped the outbox check *and* the release, so a round that passed it to quiet
    an outbox problem silently stopped tearing its workers down (18 worktrees and 3 dead claims left behind,
    2026-09-23). Outbox checking and release are separate opt-outs now. `checks` are `Row`s or gate tuples.
    """
    del check_outbox  # independent of the release decision by design
    if not release_claims or not units:
        return []
    if any(r.failed for r in _findings.rows_of(checks)):
        return []
    return list(units)


def release_unit(unit: str, main: str) -> dict:
    """`claims.release(unit, main)` without force or dry-run - the one release call the gate and `land` make."""
    return claims.release(unit, main, force=False, dry_run=False)


def release_rows(b: Batch) -> None:
    """21. release the claim of every unit just gated, after every row above passed (BOOKKEEPING); a refused
    batch keeps its branch and worktree for the retry, and `--no-release` skips the step."""
    to_release = release_plan(b.checks, b.units, b.release_claims, b.check_outbox)
    for unit_name in to_release:
        out = release_unit(unit_name, b.main)
        failed_step = next((s for s in out["steps"] if s["status"] == "failed"), None)
        note = out.get("refused") or (("failed: %s - %s" % (failed_step["label"], failed_step["why"]))
                                      if failed_step else "")
        info = "branch %s" % out["branch"]
        if out.get("release_ref"):
            info += "; un-merged commits rescued to %s" % out["release_ref"]
        b.check("claim released: %s" % unit_name, bool(out.get("complete")), note, info=info,
                kind=KIND_BOOKKEEPING,
                remedy="the batch is committed - finish the teardown by hand (close the pane / remove the "
                       "worktree), then re-run")
    if b.release_claims and b.units and not to_release:
        b.check("claim release deferred", True, info="a check above failed - the claim is left alone")
    elif b.units and not b.release_claims:
        b.check("claim release skipped", True, info="--no-release")
