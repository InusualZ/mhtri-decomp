"""`verify`: the gate's rows in their run order - the cheap pre-build rows, then the build and the rows that read it.
Spec: docs/tools/spec/landing.md. CLI: none (`land.py verify` / `land.py land` call it)."""
from __future__ import annotations

import os

from tools.lib import findings as _findings
from tools.lib.lanes import naming
from tools.units.landing import release
from tools.units.landing import state
from tools.units.landing.rows import batch
from tools.units.landing.rows import build
from tools.units.landing.rows import data
from tools.units.landing.rows import manifest
from tools.units.landing.rows import objects
from tools.units.landing.rows import regression
from tools.units.landing.rows import rules
from tools.units.landing.rows import selftests
from tools.units.landing.rows import subject
from tools.units.landing.rows import tree
from tools.units.landing.base import ledger_numbers, summary
from tools.units.landing.common import Batch, failing_checks, failure_summary, read_base
from tools.units.landing.message import clear_land_message, write_land_message
from tools.units.landing.rows.build import flips_objects
from tools.units.landing.rows.tree import unit_rows


#: The rows that need no build, in their run order (`docs/tools/spec/land.md`, "The rows of verify" 1-9).
PRE_BUILD = (
    tree.ground_truth_row, tree.base_row, tree.report_base_row, tree.paths_row, tree.conflict_marker_row,
    manifest.manifest_row, batch.outbox_rows, rules.style_lint_row, selftests.selftests_row, rules.band_row, objects.new_unit_name_row,
    subject.subject_row,
)

DRY_RUN_PLAN = ("\nwould then: delete build/RMHE08/ok%s, run configure.py -> registration gate (configure.py "
                "+ splits.txt + build graph) -> compile gate (ninja -k 0, scoped to the batch's own "
                "objects) -> ninja -> report.json -> target-object drift + independent per-symbol "
                "re-measure -> regression scan -> ok -> ledger -> baseline")


def verify(main: str, units: list[str], base: str | None, dry_run: bool, no_build: bool,
           allow_regression: list[str] | None = None, check_outbox: bool = True,
           release_claims: bool = True, problems: list[str] | None = None,
           branch: str | None = None, no_selftests: bool = False, warnings: list[str] | None = None,
           seam_moves: list[str] | None = None) -> int:
    """Run every row; 0 when every row passed, 1 otherwise.

    `problems` is the out-parameter an automated caller (`land`) reads: `"<failing check> [<KIND>]: <what it
    printed> (remedy: ...)"` per failed row (`failing_checks`). A unit's name is its path without the source
    extension (`norm_unit`), and a `--units` entry that is a batch PATH (`is_batch_path`) stays out of the
    unit-shaped rows (`unit_rows`). `--dry-run` runs the pre-build rows only and touches nothing; a pre-build
    failure refuses before the build. Only an all-green run writes the commit message (`.git/land_msg.txt`).
    `warnings`, the second out-parameter, receives every WARNING row's findings (`Batch.warn`) whatever the verdict;
    `seam_moves` the pure seam moves the regression row credited (`"A -> B (N functions)"`)."""
    units = [naming.norm_unit(u.strip("/")) for u in units]
    recorded = read_base(main)
    b = Batch(main=main, units=units, unit_units=unit_rows(main, units, base or recorded.get("base")),
              base=base or recorded.get("base"), recorded=recorded, dry_run=dry_run, no_build=no_build,
              branch=branch, check_outbox=check_outbox, release_claims=release_claims, no_selftests=no_selftests,
              allow_regression=[a.strip() for a in (allow_regression or []) if a.strip()])
    try:
        return _verify(b, main, recorded, dry_run, no_build, problems)
    finally:
        if warnings is not None:
            warnings.extend(b.warnings)
        if seam_moves is not None:
            seam_moves.extend(b.seam_moves)


def _verify(b: Batch, main: str, recorded: dict, dry_run: bool, no_build: bool, problems: list[str] | None) -> int:
    """`verify`'s rows on a built `Batch`, in run order (the function above only builds `b` and hands out warnings)."""
    for row in PRE_BUILD:
        row(b)

    before = recorded.get("ledger") or ledger_numbers(main)
    b.extra["flip"] = flips_objects(main)

    if dry_run:
        if b.checks:
            print(_findings.render_lines(b.checks))
        print(DRY_RUN_PLAN % (" and main.elf (this batch flips an object)" if b.extra["flip"] else ""))
        if problems is not None:
            problems.extend(failing_checks(b.checks))
        return 0 if b.ok else 1

    if not b.ok:
        print(_findings.render_lines(b.checks))
        if problems is not None:
            problems.extend(failing_checks(b.checks))
        print("\n" + failure_summary(b.checks))
        return 1
    if problems is not None:
        problems.extend(failing_checks(b.checks))

    # the build, with the proof that the `ok` we read is this run's
    build.clear_stamps(b)
    objects.target_snapshot_before(b)
    rules.rule10_before(b)
    built = build.configure_rows(b)
    if built:
        objects.registration_row(b)
    if built:
        build.compile_row(b)
        objects.undefrefs_rows(b)
    built = build.command_row(b, "ninja", ["ninja"]) and built
    build.command_row(b, "report.json", ["ninja", "build/RMHE08/report.json"])
    rules.rule10_row(b)
    data.data_closure_rows(b)
    objects.drift_row(b)
    objects.remeasure_row(b)
    # the regression scan reads build/RMHE08/report_changes.json, which only `ninja changes` writes
    build.command_row(b, "ninja changes (DOL-level totals, informational)", ["ninja", "changes"])
    regression.regression_check_rows(b)
    fresh = build.ok_fresh_row(b)

    after = ledger_numbers(main)
    # the baseline, so the next batch's `ninja changes` compares against this one (7.16)
    if not no_build:
        build.command_row(b, "ninja baseline", ["ninja", "baseline"])
    release.release_rows(b)

    print(_findings.render_table(b.checks))
    if not b.ok:
        # a failed gate must not leave a message a `git commit -F .git/land_msg.txt` could pick up (2026-09-23)
        stale = clear_land_message(main)
        if stale:
            print("removed the stale %s (a failed gate has no committable message)" % os.path.relpath(stale, main))
        print("\nledger: %s" % summary(before, after))
        print(failure_summary(b.checks, prefix="FAILED"))
        if problems is not None:
            # the post-build rows too: without them `land` refused a red build with no row named and the landing
            # log recorded `nothing to stage` (the 2026-10-05 06:57 refusal: the undefined-reference row)
            problems.extend(failing_checks(b.checks))
        return 1
    message = write_land_message(main, message_body(b, before, after, fresh))
    print("\nledger: %s" % summary(before, after))
    print("message written to %s - review it, then `git commit -F .git/land_msg.txt`" % message)
    print("READY: every check passed")
    return 0


def message_body(b: Batch, before: dict, after: dict, fresh: bool) -> str:
    """The commit message an all-green gate writes: the subject, the ledger and gate summary, one `allow:` line
    per allowance class the invocation granted (`state.allow_lines`), one `warning:` line per WARNING-row finding,
    the tolerated scratch and the rule-2 boundary warnings (part of the record, so a batch that landed with one is
    greppable)."""
    allow = state.allowances(b.allow_regression, b.check_outbox, b.no_selftests)
    body = [b.subject,
            "",
            "ledger: %s" % summary(before, after),
            "gates: ground truth ok, base %s, %d check(s), ok recreated=%s%s"
            % ((b.base or "?")[:8], len(b.checks), fresh, ", main.elf relinked" if b.extra.get("flip") else "")]
    body += state.allow_lines(allow)
    if state.MANIFEST:
        body.append("manifest: %s" % state.MANIFEST)
    body += ["warning: %s" % w for w in b.warnings]
    body.append("")
    if b.scratch:
        body.append("scratch: tool scratch outside the batch, not staged, not committed: %s" % ", ".join(b.scratch))
        body.append("")
    if b.band_warnings:
        body.append("rule 2 boundary: %d newly-owned symbol declaration(s) left in the unsplit band (src/unsplit/*.h)"
                    % len(b.band_warnings))
        for warning in b.band_warnings[:10]:
            body.append("  " + warning)
        if len(b.band_warnings) > 10:
            body.append("  ... (%d more)" % (len(b.band_warnings) - 10))
        body.append("")
    return "\n".join(body)
