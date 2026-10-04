"""The per-file and whole-tree lint: every rule over one `Source`, and the header-tree walks.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

from tools.units.stylelint_rules import (
    r03_size, r04_offset, r05_field_name, r06_pointer_arith, r07_generated_name, r08_goto, r09_mangled,
)
from tools.units.stylelint_rules.common import (
    Source, all_sources, field_walk, header_files, is_shared_header, is_unsplit_header, read_text, rel_of,
    struct_defs, unsplit_header_files,
)
from tools.units.stylelint_rules.context import load_ownership
from tools.units.stylelint_rules.r01_shared_type import rule1_findings
from tools.units.stylelint_rules.r02_extern import (
    rule2_band_findings, rule2_findings, rule2_header_findings, stopgap_findings,
)
from tools.units.stylelint_rules.r10_pragma import codegen_pragma_findings
from tools.units.stylelint_rules.r11_untyped import rule11_findings, rule11_local_count
from tools.units.stylelint_rules.r12_unclaimed_data import rule12_findings
from tools.units.stylelint_rules.r13_method import rule13_findings, rule13_static_like, set_rule13_context


def lint_source(src: Source, ownership: "Ownership | None" = None) -> list[dict]:
    """All section 6.5 findings for one file, in rule then line order.

    `ownership` carries the symbols.txt + splits.txt index for rule 2; when it is None (the map is
    absent, or a caller that only wants the source-local rules), rule 2 is not reported for `src`.

    The unsplit band is declaration-only glue, so for a file under `include/unsplit/` only rule 2 runs -
    and there it means the opposite of its `src/` reading: a symbol a registered unit owns must not be
    declared here.
    """
    out: list[dict] = []
    if is_unsplit_header(src.rel):
        if ownership is not None:
            out.extend(rule2_band_findings(src, ownership))
            out.extend(rule12_findings(src, ownership))
        out.sort(key=lambda f: (f["rule"], f["line"]))
        return out
    if is_shared_header(src.rel):
        out.extend(stopgap_findings(src))
        # an ordinary `include/` header: rules 2 and 12 are the section-6.5 rules it carries. Rules 3-9 are
        # body/`src/` rules, and rules 10/11 for headers are reported by `header_pragma_findings` and
        # `header_rule11_findings` rather than here (2026-09-28). Rule 12 is here because a header is where
        # the unowned data a `src/` unit reads is declared (2026-09-28).
        if ownership is not None:
            out.extend(rule2_header_findings(src, ownership))
            out.extend(rule12_findings(src, ownership))
        out.sort(key=lambda f: (f["rule"], f["line"]))
        return out
    # the body rules, each its own module; the final sort is stable, so the order within one (rule, line) is the
    # order each rule's module reports it in
    defs = struct_defs(src)
    fields = field_walk(src, defs)
    out.extend(r03_size.findings(src, defs))
    out.extend(r04_offset.findings(src, fields))
    out.extend(r05_field_name.findings(src, fields))
    out.extend(r06_pointer_arith.findings(src))
    out.extend(r07_generated_name.findings(src, fields))
    out.extend(r08_goto.findings(src))
    out.extend(r09_mangled.findings(src))

    if ownership is not None:
        out.extend(rule2_findings(src, ownership))
        out.extend(rule12_findings(src, ownership))

    out.extend(rule11_findings(src))
    out.extend(rule13_findings(src))
    out.extend(stopgap_findings(src))

    out.sort(key=lambda f: (f["rule"], f["line"]))
    return out


def header_pragma_findings(root: str) -> list[dict]:
    """Rule 10 over the whole shared-header tree (`include/`).  Not part of `lint_source`, which runs
    rules 3-9 on `src/` files and rule 2 on the band; a header is judged by this rule only."""
    out = []
    for path in header_files(root):
        out.extend(codegen_pragma_findings(Source(path, rel_of(root, path), read_text(path))))
    return out


def header_rule11_findings(root: str) -> list[dict]:
    """Rule 11 over the whole shared-header tree (`include/`), including the unsplit band.

    A shared header is a declaration a batch is judged against, and a `void *` parameter there is the same
    defect as one in a `src/` file. The band is covered here - `lint_source` returns early for it.
    """
    out = []
    for path in header_files(root):
        out.extend(rule11_findings(Source(path, rel_of(root, path), read_text(path))))
    return out


def header_rule13_findings(root: str) -> list[dict]:
    """Rule 13 over the whole shared-header tree (`include/`), including the unsplit band.

    The declaration of `Type_name(Type* self)` lives in a header (`include/Network/network_socket_streams.h`),
    so the finding has to be reachable there; `lint_source` leaves headers to their own walks, as for rule 11.
    """
    set_rule13_context(root)
    out = []
    for path in header_files(root):
        out.extend(rule13_findings(Source(path, rel_of(root, path), read_text(path))))
    return out


def rule13_static_like_total(root: str) -> int:
    """Distinct `<Type>_<name>` static-shaped names in `src/` and `include/` (each is a rule 13 finding)."""
    set_rule13_context(root)
    names: set = set()
    for s in all_sources(root):
        names |= rule13_static_like(s)
    for path in header_files(root):
        names |= rule13_static_like(Source(path, rel_of(root, path), read_text(path)))
    return len(names)


def header_rule12_findings(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Rule 12 over the whole shared-header tree (`include/`), the unsplit band included.

    A header declares the unowned data a `src/` unit reads (`include/Network/network_state.h`'s
    `sessionTimeoutParam` block, `include/unsplit/NetworkData.h`'s constants), so the finding has to be
    reachable there; `lint_source` returns early for both header classes, exactly as it does for rule 11.
    """
    if ownership is None:
        ownership = load_ownership(root)
    if ownership is None:
        return []
    out = []
    for path in header_files(root):
        rel = rel_of(root, path)
        out.extend(rule12_findings(Source(path, rel, read_text(path)), ownership))
    return out


def header_rule2_findings(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Rule 2 over the shared-header tree (`include/`) - the non-unsplit headers.

    The unsplit band has its own reading (`rule2_band_findings`) and is reached through `lint_source`; this
    is the extension the `extern`-keyword, `src/`-only rule could not see: a foreign declaration in
    `include/<module>/*.h` (`include/Network/network_state.h`'s `setMediatorState68A` pair).  The map
    being absent leaves rule 2 unreported, exactly as it does for `src/`.
    """
    if ownership is None:
        ownership = load_ownership(root)
    if ownership is None:
        return []
    out = []
    for path in header_files(root):
        rel = rel_of(root, path)
        if is_unsplit_header(rel):
            continue
        out.extend(rule2_header_findings(Source(path, rel, read_text(path)), ownership))
    out.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    return out


def band_rule2_findings(sources: list["Source"], ownership: "Ownership | None") -> list[dict]:
    """Rule 2 over a set of unsplit-band sources - the reading `header_rule2_findings` deliberately skips.

    `all_sources` walks only `src/`, and `header_rule2_findings` skips `include/unsplit/` (that is
    `rule2_band_findings`' territory), so the band's rule-2 findings appear in no `--budget` column even
    though `--diff` counts them. The declaration of a symbol a registered unit owns must not sit in the
    band; this is the function `--budget --headers` adds that missing column from. Pure in `sources`, so
    the aggregation is testable without the tree.
    """
    out: list[dict] = []
    if ownership is None:
        return out
    for src in sources:
        out.extend(rule2_band_findings(src, ownership))
    out.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    return out


def header_rule2_band_findings(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """`rule2_band_findings` over the whole `include/unsplit/` band, read from `root`.

    `--budget --headers` adds this to the per-file table. The map being absent leaves rule 2 unreported,
    exactly as it does for `src/`.
    """
    if ownership is None:
        ownership = load_ownership(root)
    if ownership is None:
        return []
    return band_rule2_findings(
        [Source(path, rel_of(root, path), read_text(path)) for path in unsplit_header_files(root)],
        ownership)


def rule11_local_total(root: str) -> int:
    """Every `void *` local variable in `src/` and `include/` - the rule-11 scope note's count."""
    total = sum(rule11_local_count(s) for s in all_sources(root))
    for path in header_files(root):
        total += rule11_local_count(Source(path, rel_of(root, path), read_text(path)))
    return total


def lint_tree(root: str, paths: list[str] | None = None,
              ownership: "Ownership | None" = None) -> list[dict]:
    """Per-file findings (rules 2-9) for `paths`, or for the whole tree when `paths` is None.

    Rule 1 is cross-file and is *not* included here: with `paths` limited to a batch's changed files it
    would miss a duplicate whose partner is untouched. Use `lint_all` for the whole rule set. `ownership`
    is the rule-2 index; it is loaded from `root` when not passed, and the map being absent simply leaves
    rule 2 unreported.
    """
    if ownership is None:
        ownership = load_ownership(root)
    set_rule13_context(root)
    out = []
    sources = all_sources(root) if paths is None else [
        Source(path, rel_of(root, path), read_text(path)) for path in paths]
    for src in sources:
        out.extend(lint_source(src, ownership))
    return out


def lint_all(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Every checked finding: the per-file rules plus cross-file rule 1, in rule/file/line order."""
    if ownership is None:
        ownership = load_ownership(root)
    set_rule13_context(root)
    sources = all_sources(root)
    out = []
    for src in sources:
        out.extend(lint_source(src, ownership))
    out.extend(rule1_findings(sources))
    out.extend(header_pragma_findings(root))
    out.extend(header_rule2_findings(root, ownership))
    out.extend(header_rule11_findings(root))
    out.extend(header_rule13_findings(root))
    out.extend(header_rule12_findings(root, ownership))
    out.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    return out
