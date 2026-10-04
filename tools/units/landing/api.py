"""Every name the landing package defines, in one namespace: what `land.py` re-exports for the tools that import it.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

from tools.units.landing import base, branch, cli, common, flow, gate, message, release, stage, state  # noqa: F401
from tools.units.landing.rows import (batch, build, data, knowledge, objects, regression, rules,  # noqa: F401
    selftests, subject, tree)
from tools.units.landing.common import (SELF_REPO, ALLOWED_PREFIXES, ALLOWED_FILES, BASE_FILE, SCRATCH_JSON,
    KIND_GATE, KIND_BOOKKEEPING, KIND_TAG, KIND_REMEDY, check_kind, check_remedy, failed_kinds, kinds_from_failures,
    failure_summary, run, git, read_base, failing_checks, command_detail, changed_status, changed_paths,
    _OBJECT_TARGET, _FAILED_LINE, compile_targets, failed_compile_outputs, worktree_root, main_root, Batch,
    unit_owned_paths, outside_batch, is_scratch, scratch_paths)  # noqa: F401
from tools.units.landing.state import (set_allow_rule10, set_allow_orphan, set_unit_renames, rename_snapshot_keys,
    set_allow_rule12)  # noqa: F401
from tools.units.landing.message import (land_message_path, write_land_message, message_body_with_subject,
    clear_land_message, message_error, answer_line)  # noqa: F401
from tools.units.landing.base import (record_base, base_dirty_paths, report_snapshot, ledger_numbers, summary,
    ledger_line)  # noqa: F401
from tools.units.landing.stage import (land_decision, land_stageable, looks_already_applied, staged_elsewhere,
    foreign_warning, commit_pathspec, stage_batch)  # noqa: F401
from tools.units.landing.branch import (UNION_SCOPE, _resolve_result, _tree_text, _union_conflicts,
    resolve_conflicts, scratch_resolve, resolve_helper_slug, resolve_helper_refs, _is_ancestor, resolve_helper_state,
    _resolve_helper_worktree, delete_resolve_helper, _sweep_resolve_helpers, claim_unit_for_branch,
    units_from_branch, apply_branch, undo_apply)  # noqa: F401
from tools.units.landing.release import (release_plan, release_unit, release_rows)  # noqa: F401
from tools.units.landing.rows.tree import (branch_error, caller_branch_error, conflict_marker_files, _exists_at,
    is_batch_path, unit_rows, require_clean_tree, scratch_note, unstage_scratch, tolerate_scratch,
    LANE_SCRATCH_MARKERS, likely_cause, preflight_foreign, preflight_report, ground_truth_row, base_row, paths_row,
    conflict_marker_row)  # noqa: F401
from tools.units.landing.rows.batch import (branch_commits, outbox_skip_note, outbox_units, restore_rescued_branch,
    commits_ahead_of_main, branch_problems, outbox_rows)  # noqa: F401
from tools.units.landing.rows.rules import (RULE2_CALLSITE_CAVEAT, _split_rows, _merge_intervals,
    _subtract_intervals, added_split_ranges, _band_header_paths, _changed_band_headers, _declared_names,
    _declared_names_at, _defer_count, _GENERATED_STEM_RE, _GENERATED_FN_RE, generated_fn_definitions,
    rule7_defer_growth, rule10_violations, rule10_growth, rule12_verdict, rule12_lint_row, band_ownership_warnings,
    STYLE_LINT_REMEDY, style_lint_row, band_row, rule7_row, rule10_before, rule10_row)  # noqa: F401
from tools.units.landing.rows.build import (_added_object_calls, flips_objects, flipped_units,
    batch_compile_failures, compile_check, command_row, ok_paths, clear_stamps, configure_rows, compile_row,
    ok_fresh_row)  # noqa: F401
from tools.units.landing.rows.objects import (flipcheck_problems, undefrefs_snapshot, target_snapshot_before,
    registration_row, undefrefs_rows, drift_row, remeasure_row)  # noqa: F401
from tools.units.landing.rows.data import (_git_unit_renames_since, orphans_snapshot, ORPHAN_REMEDY,
    SOLE_OWNED_REMEDY, data_closure_rows)  # noqa: F401
from tools.units.landing.rows.regression import (regression_rows, unit_grew, report_regressions,
    regression_check_rows)  # noqa: F401
from tools.units.landing.rows.selftests import (selftest_detail, selftest_tails, SUITE_ROW,
    selftests_row)  # noqa: F401
from tools.units.landing.rows.subject import (land_subject, subject_lint, subject_row)  # noqa: F401
from tools.units.landing.rows.knowledge import (knowledge_row)  # noqa: F401
from tools.units.landing.gate import (PRE_BUILD, DRY_RUN_PLAN, verify, message_body)  # noqa: F401
from tools.units.landing.flow import (land_branch, land)  # noqa: F401
from tools.units.landing.cli import (INTEGRATE, integrate_command, main, TESTS, selftest)  # noqa: F401

#: The mutable invocation state (`ALLOW_*`, `UNIT_RENAMES`) is read as `state.<NAME>`, never re-exported: a
#: copy here would not see a setter's rebinding.
__all__ = ["SELF_REPO", "ALLOWED_PREFIXES", "ALLOWED_FILES", "BASE_FILE", "SCRATCH_JSON", "KIND_GATE",
           "KIND_BOOKKEEPING", "KIND_TAG", "KIND_REMEDY", "check_kind", "check_remedy", "failed_kinds",
           "kinds_from_failures", "failure_summary", "run", "git", "read_base", "failing_checks", "command_detail",
           "changed_status", "changed_paths", "_OBJECT_TARGET", "_FAILED_LINE", "compile_targets",
           "failed_compile_outputs", "worktree_root", "main_root", "Batch", "unit_owned_paths", "outside_batch",
           "is_scratch", "scratch_paths", "set_allow_rule10", "set_allow_orphan", "set_unit_renames",
           "rename_snapshot_keys", "set_allow_rule12", "land_message_path", "write_land_message",
           "message_body_with_subject", "clear_land_message", "message_error", "answer_line", "record_base",
           "base_dirty_paths", "report_snapshot", "ledger_numbers", "summary", "ledger_line", "land_decision",
           "land_stageable", "looks_already_applied", "staged_elsewhere", "foreign_warning", "commit_pathspec",
           "stage_batch", "UNION_SCOPE", "_resolve_result", "_tree_text", "_union_conflicts", "resolve_conflicts",
           "scratch_resolve", "resolve_helper_slug", "resolve_helper_refs", "_is_ancestor", "resolve_helper_state",
           "_resolve_helper_worktree", "delete_resolve_helper", "_sweep_resolve_helpers", "claim_unit_for_branch",
           "units_from_branch", "apply_branch", "undo_apply", "release_plan", "release_unit", "release_rows",
           "branch_error", "caller_branch_error", "conflict_marker_files", "_exists_at", "is_batch_path",
           "unit_rows", "require_clean_tree", "scratch_note", "unstage_scratch", "tolerate_scratch",
           "LANE_SCRATCH_MARKERS", "likely_cause", "preflight_foreign", "preflight_report", "ground_truth_row",
           "base_row", "paths_row", "conflict_marker_row", "branch_commits", "outbox_skip_note", "outbox_units",
           "restore_rescued_branch", "commits_ahead_of_main", "branch_problems", "outbox_rows",
           "RULE2_CALLSITE_CAVEAT", "_split_rows", "_merge_intervals", "_subtract_intervals", "added_split_ranges",
           "_band_header_paths", "_changed_band_headers", "_declared_names", "_declared_names_at", "_defer_count",
           "_GENERATED_STEM_RE", "_GENERATED_FN_RE", "generated_fn_definitions", "rule7_defer_growth",
           "rule10_violations", "rule10_growth", "rule12_verdict", "rule12_lint_row", "band_ownership_warnings",
           "STYLE_LINT_REMEDY", "style_lint_row", "band_row", "rule7_row", "rule10_before", "rule10_row",
           "_added_object_calls", "flips_objects", "flipped_units", "batch_compile_failures", "compile_check",
           "command_row", "ok_paths", "clear_stamps", "configure_rows", "compile_row", "ok_fresh_row",
           "flipcheck_problems", "undefrefs_snapshot", "target_snapshot_before", "registration_row",
           "undefrefs_rows", "drift_row", "remeasure_row", "_git_unit_renames_since", "orphans_snapshot",
           "ORPHAN_REMEDY", "SOLE_OWNED_REMEDY", "data_closure_rows", "regression_rows", "unit_grew",
           "report_regressions", "regression_check_rows", "selftest_detail", "selftest_tails", "SUITE_ROW",
           "selftests_row", "land_subject", "subject_lint", "subject_row", "knowledge_row", "PRE_BUILD",
           "DRY_RUN_PLAN", "verify", "message_body", "land_branch", "land", "INTEGRATE", "integrate_command", "main",
           "TESTS", "selftest", "base", "branch", "cli", "common", "flow", "gate", "message", "release", "stage",
           "state", "batch", "build", "data", "knowledge", "objects", "regression", "rules", "selftests", "subject",
           "tree"]
