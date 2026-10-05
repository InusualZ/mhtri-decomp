"""The stylelint package's one public surface: every name `stylelint.py` had, re-exported from its module.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

from tools.units.stylelint_rules.common import (  # noqa: F401
    SRC, UNSPLIT, UNSPLIT_UNRESOLVED, HEADERS, HEADER_SUFFIXES, SUFFIXES, EXEMPT, RULE7_NOTES, RULE_NAMES, AUDIT_RULES, UNCHECKED,
    strip, Source, STRUCT_RE, match_brace, struct_defs, iter_fields, field_name, RULE1_TYPE_RE, type_defs,
    is_unsplit_header, is_shared_header, _finding, _rule2_finding, RULE11_MARKER_RE, match_paren, _mask_preproc,
    function_declarations, _untyped_marker, _split_parameters, source_files, rel_of, read_text, all_sources,
    header_files, unsplit_header_files, rule_enforced, exemptions,
)
from tools.units.stylelint_rules.context import (  # noqa: F401
    module_name, _OWNERSHIP_CACHE, Ownership, _parse_symbols, _parse_splits, load_ownership, _REQUEST_DIRS,
    _OPEN_IDS, set_request_dirs, open_request_ids,
)
from tools.units.stylelint_rules.r01_shared_type import (  # noqa: F401
    rule1_findings,
)
from tools.units.stylelint_rules.r02_extern import (  # noqa: F401
    EXTERN_RE, _declared_name, extern_declarations, _owns, _LINKAGE_OPEN_RE, _TYPE_ONLY_RE, _file_scope_declarations,
    _seg_line, header_declarations, prototype_declarations, declaration_sites, rule2_band_findings, rule2_findings,
    leaf_header_owner, rule2_header_findings, stopgap_findings,
)
from tools.units.stylelint_rules.r03_size import (  # noqa: F401
    SIZE_RE, struct_has_size,
)
from tools.units.stylelint_rules.r04_offset import (  # noqa: F401
    OFFSET_RE,
)
from tools.units.stylelint_rules.r05_field_name import (  # noqa: F401
    UNK_FIELD_RE,
)
from tools.units.stylelint_rules.r06_pointer_arith import (  # noqa: F401
    _PTR_TYPE, RULE6_RE, MEM_FUNCS, enclosing_call,
)
from tools.units.stylelint_rules.r07_generated_name import (  # noqa: F401
    RULE7_FN_RE, RULE7_UNK_RE, RULE7_LBL_RE,
)
from tools.units.stylelint_rules.r08_goto import (  # noqa: F401
    RULE8_RE,
)
from tools.units.stylelint_rules.r09_mangled import (  # noqa: F401
    RULE9_MANGLED_RE, RULE9_CALL_RE, _DECL_HEAD_RE, _DECL_KEYWORDS, _statement_head, looks_like_declaration,
)
from tools.units.stylelint_rules.r14_pragma import (  # noqa: F401
    CODEGEN_PRAGMAS, CODEGEN_PRAGMA_RE, codegen_pragma_findings,
)
from tools.units.stylelint_rules.r11_untyped import (  # noqa: F401
    RULE11_VOID_PTR_RE, RULE11_REASON_RES, RULE11_LOCAL_RE, RULE11_NON_DECL_HEADS, _untyped_reason_ok,
    _RULE11_PARAM_DETAIL, _RULE11_RET_DETAIL, rule11_findings, rule11_local_count,
)
from tools.units.stylelint_rules.r12_unclaimed_data import (  # noqa: F401
    rule12_findings,
)
from tools.units.stylelint_rules.r13_method import (  # noqa: F401
    RULE13_MARKER_RE, RULE13_REASON_RE, RULE13_FIRST_PARAM_RE, RULE13_CTOR_HELPER_RE, RULE13_CPP_SUFFIXES,
    Rule13Context, _RULE13_CTX, _RULE13_CACHE, _resolve_include, build_rule13_context, set_rule13_context,
    _rule13_in_scope, _rule13_self_type, _rule13_prefix_type, _rule13_scan, rule13_findings, rule13_static_like,
)
from tools.units.stylelint_rules.lint import (  # noqa: F401
    lint_source, header_pragma_findings, header_rule11_findings, header_rule13_findings, rule13_static_like_total,
    header_rule12_findings, header_rule2_findings, band_rule2_findings, header_rule2_band_findings,
    rule11_local_total, lint_tree, lint_all,
)
from tools.units.stylelint_rules.diff import (  # noqa: F401
    unresolved_declarations, owed_rename_completion, rename_credits, apply_rename_credits, rename_credit_lines,
    rule_counts, diff_deltas, finding_identity, _covering_range, rename_map, renamed_finding, added_identities,
    removed_identities, _unit_stem, file_absorbers, derive_file_absorbers, apply_move_credits, move_credit_lines,
    added_rows, added_finding_detail, added_detail_lines, merge_counts, renames_of,
)
from tools.units.stylelint_rules.refs import (  # noqa: F401
    load_ownership_at_ref, unresolved_declarations_at_ref, header_pragma_findings_at_ref,
    header_pragma_counts_at_ref, header_rule13_findings_at_ref, header_rule11_findings_at_ref,
    header_rule11_counts_at_ref, header_rule12_findings_at_ref, header_rule12_counts_at_ref, git, git_bytes,
    changed_src_files, findings_at_ref, deleted_src_files, findings_of_deleted, changed_src_files_between,
    findings_of_ref, unresolved_declarations_of_ref, sources_of_ref, src_paths_at_ref, rule1_findings_at_ref,
    rule1_counts_at_ref,
)
from tools.units.stylelint_rules.report import (  # noqa: F401
    budget, rule10_counts, unique_names, print_rule2_report, print_budget, source_files_of, select_findings,
    print_findings_listing,
    print_findings,
)
from tools.units.stylelint_rules.cli import (  # noqa: F401
    _same_commit, _fork_point, _resolve_diff_ref, ref_comparison, main,
)

__all__ = [
    "SRC", "UNSPLIT", "UNSPLIT_UNRESOLVED", "HEADERS", "HEADER_SUFFIXES", "SUFFIXES", "EXEMPT", "RULE7_NOTES",
    "RULE_NAMES", "AUDIT_RULES", "UNCHECKED", "strip", "Source", "STRUCT_RE", "match_brace", "struct_defs", "iter_fields",
    "field_name", "RULE1_TYPE_RE", "type_defs", "is_unsplit_header", "is_shared_header", "_finding",
    "_rule2_finding", "RULE11_MARKER_RE", "match_paren", "_mask_preproc", "function_declarations", "_untyped_marker",
    "_split_parameters", "source_files", "rel_of", "read_text", "all_sources", "header_files",
    "unsplit_header_files", "rule_enforced", "exemptions",
    "module_name", "_OWNERSHIP_CACHE", "Ownership", "_parse_symbols", "_parse_splits", "load_ownership",
    "_REQUEST_DIRS", "_OPEN_IDS", "set_request_dirs", "open_request_ids",
    "rule1_findings",
    "EXTERN_RE", "_declared_name", "extern_declarations", "_owns", "_LINKAGE_OPEN_RE", "_TYPE_ONLY_RE",
    "_file_scope_declarations", "_seg_line", "header_declarations", "prototype_declarations", "declaration_sites",
    "rule2_band_findings", "rule2_findings", "leaf_header_owner", "rule2_header_findings", "stopgap_findings",
    "SIZE_RE", "struct_has_size",
    "OFFSET_RE",
    "UNK_FIELD_RE",
    "_PTR_TYPE", "RULE6_RE", "MEM_FUNCS", "enclosing_call",
    "RULE7_FN_RE", "RULE7_UNK_RE", "RULE7_LBL_RE",
    "RULE8_RE",
    "RULE9_MANGLED_RE", "RULE9_CALL_RE", "_DECL_HEAD_RE", "_DECL_KEYWORDS", "_statement_head",
    "looks_like_declaration",
    "CODEGEN_PRAGMAS", "CODEGEN_PRAGMA_RE", "codegen_pragma_findings",
    "RULE11_VOID_PTR_RE", "RULE11_REASON_RES", "RULE11_LOCAL_RE", "RULE11_NON_DECL_HEADS", "_untyped_reason_ok",
    "_RULE11_PARAM_DETAIL", "_RULE11_RET_DETAIL", "rule11_findings", "rule11_local_count",
    "rule12_findings",
    "RULE13_MARKER_RE", "RULE13_REASON_RE", "RULE13_FIRST_PARAM_RE", "RULE13_CTOR_HELPER_RE", "RULE13_CPP_SUFFIXES",
    "Rule13Context", "_RULE13_CTX", "_RULE13_CACHE", "_resolve_include", "build_rule13_context",
    "set_rule13_context", "_rule13_in_scope", "_rule13_self_type", "_rule13_prefix_type", "_rule13_scan",
    "rule13_findings", "rule13_static_like",
    "lint_source", "header_pragma_findings", "header_rule11_findings", "header_rule13_findings",
    "rule13_static_like_total", "header_rule12_findings", "header_rule2_findings", "band_rule2_findings",
    "header_rule2_band_findings", "rule11_local_total", "lint_tree", "lint_all",
    "unresolved_declarations", "owed_rename_completion", "rename_credits", "apply_rename_credits",
    "rename_credit_lines", "rule_counts", "diff_deltas", "finding_identity", "_covering_range", "rename_map",
    "renamed_finding", "added_identities", "removed_identities", "_unit_stem", "file_absorbers",
    "derive_file_absorbers", "apply_move_credits", "move_credit_lines", "added_rows", "added_finding_detail",
    "added_detail_lines", "merge_counts", "renames_of",
    "load_ownership_at_ref", "unresolved_declarations_at_ref", "header_pragma_findings_at_ref",
    "header_pragma_counts_at_ref", "header_rule13_findings_at_ref", "header_rule11_findings_at_ref",
    "header_rule11_counts_at_ref", "header_rule12_findings_at_ref", "header_rule12_counts_at_ref", "git",
    "git_bytes", "changed_src_files", "findings_at_ref", "deleted_src_files", "findings_of_deleted",
    "changed_src_files_between", "findings_of_ref", "unresolved_declarations_of_ref", "sources_of_ref",
    "src_paths_at_ref", "rule1_findings_at_ref", "rule1_counts_at_ref",
    "budget", "rule10_counts", "unique_names", "print_rule2_report", "print_budget", "source_files_of",
    "select_findings",
    "print_findings_listing", "print_findings",
    "_same_commit", "_fork_point", "_resolve_diff_ref", "ref_comparison", "main",
]

# new with the split (no old spelling): the one field walk, the shared comparison, the one-process tree read
from tools.units.stylelint_rules.common import Field, field_walk  # noqa: E402,F401
from tools.units.stylelint_rules.diff import Judgement, base_rule2_symbols, judge  # noqa: E402,F401
from tools.units.stylelint_rules.refs import headers_at_ref, texts_at_ref  # noqa: E402,F401
from tools.units.stylelint_rules.cli import report_comparison  # noqa: E402,F401

__all__ += ["Field", "field_walk", "Judgement", "base_rule2_symbols", "judge", "headers_at_ref", "texts_at_ref",
            "report_comparison"]

# the 2026-10-05 classifier: a header is a `.h` anywhere, the walk roots, the band's rule-2 budget filter
from tools.units.stylelint_rules.common import (  # noqa: E402,F401
    LINT_ROOTS, all_header_files, all_lint_sources, is_header, lint_files,
)
from tools.units.stylelint_rules.lint import is_band_rule2  # noqa: E402,F401
from tools.units.stylelint_rules.cli import untouched  # noqa: E402,F401

__all__ += ["LINT_ROOTS", "all_header_files", "all_lint_sources", "is_header", "lint_files", "is_band_rule2",
            "untouched"]

# rule 7 exact (2026-10-05): the detail of a generated identifier and the one verdict it reads
from tools.units.stylelint_rules.r07_generated_name import TOKEN_RE, INCLUDE_LINE_RE, name_detail  # noqa: E402,F401
from tools.lib.names import generated_name_kind  # noqa: E402,F401

__all__ += ["TOKEN_RE", "INCLUDE_LINE_RE", "name_detail", "generated_name_kind"]

# rule 7 file names (2026-10-05)
from tools.units.stylelint_rules.r07_generated_name import path_detail, path_findings  # noqa: E402,F401

__all__ += ["path_detail", "path_findings"]

# rule 15 (2026-10-05): comment hygiene - the refusing and advisory classes, the address prefix, the tree context
from tools.units.stylelint_rules.r15_comments import (  # noqa: E402,F401
    ADDRESS_PREFIX_RE, ADVISORY, REFUSING, address_findings, advisory_findings, function_comments, is_advisory,
    set_rule15_context, stale_path_findings,
)
from tools.units.stylelint_rules.r15_comments import findings as rule15_findings  # noqa: E402,F401

__all__ += ["ADDRESS_PREFIX_RE", "ADVISORY", "REFUSING", "address_findings", "advisory_findings", "function_comments",
            "is_advisory", "set_rule15_context", "stale_path_findings", "rule15_findings"]
