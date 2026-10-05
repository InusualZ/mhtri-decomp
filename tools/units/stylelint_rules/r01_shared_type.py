"""Rule 1: a shared type is defined once - a type defined in more than one file, one finding per extra file.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

from tools.units.stylelint_rules.common import Source, _finding, is_header, type_defs


def rule1_findings(sources: list[Source]) -> list[dict]:
    """One finding per (type, extra file) for a type defined in more than one file.

    The lexicographically first file is the owner; every later file that defines the same name gets a
    finding that names both files, so a type in N files yields N-1 findings - the count is the number of
    extra definitions to delete, not the number of files involved. Reporting per extra file (rather than
    one finding per type) keeps the budget actionable and lets a batch that only adds a duplicate be
    refused on the file it touched.

    Only sources are compared, never headers (`is_header`, by suffix): a header's definition is the shared one this
    rule points at, and a header-vs-source copy is `typeregistry`'s `shared` debt. The rule read `src/` while every
    header sat under `include/`; keying the exclusion on the suffix keeps that reading after the owner's 2026-10-05
    move put the headers in `src/` (71 header-involving duplicates would otherwise appear, and two owners change).
    """
    where: dict[str, dict[str, int]] = {}
    sources = [src for src in sources if not is_header(src.rel)]
    by_rel = {src.rel: src for src in sources}
    for src in sources:
        for name, line in type_defs(src):
            where.setdefault(name, {}).setdefault(src.rel, line)
    out: list[dict] = []
    for name in sorted(where):
        files = sorted(where[name])
        if len(files) < 2:
            continue
        owner = files[0]
        for extra in files[1:]:
            out.append(_finding(by_rel[extra], 1, where[name][extra],
                                "type `%s` is defined in `%s` and again in `%s` - one definition, in the "
                                "owner's header" % (name, owner, extra), token=name))
    return out
