"""Rule 4: every field carries its offset annotation (`/* +0xNN */`).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Field, Source, _finding

# `/* +0x1C */` is the canonical form; the `/* 0x1C */` variant the older units use is accepted
OFFSET_RE = re.compile(r"/\*\s*\+?0x[0-9A-Fa-f]+")


def findings(src: Source, fields: list[Field]) -> list[dict]:
    """A field with no offset comment on its own line(s) - from its declaration's first line to its name's."""
    out = []
    for f in fields:
        has_offset = any(
            OFFSET_RE.search(src.comments[src._starts[l - 1]: (src._starts[l] if l < len(src._starts) else len(src.comments))])
            for l in range(f.first_line, max(f.first_line, f.line) + 1)
        )
        if not has_offset:
            out.append(_finding(src, 4, f.line, "field `%s` has no offset annotation" % f.name, token=f.name))
    return out
