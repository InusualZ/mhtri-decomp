"""Rule 7: no auto-generated name survives (`fn_`/`lbl_`/`loc_` + 8 hex digits, a bare `unk*`), whoever owns it.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Field, Source, _finding

RULE7_FN_RE = re.compile(r"\bfn_[0-9A-Fa-f]{8}\b")
RULE7_UNK_RE = re.compile(r"\bunk\w*\b")
# Rule 7's data half: dtk's stems for an unrenamed data label - `lbl_XXXXXXXX` and its `loc_XXXXXXXX`
# sibling. Ownership is deliberately **not** consulted any more: a label left generated is a finding in
# every file that spells it, own, foreign or unowned alike.
RULE7_LBL_RE = re.compile(r"\b(?:lbl|loc)_[0-9A-Fa-f]{8}\b")
# The `rule 7 deferred: <reason>` spelling is not a key - a comment exempts nothing - and nothing matches it
# any more (the gate row that judged its growth was deleted 2026-10-05; rule 7 fires on the names themselves).


def findings(src: Source, fields: list[Field]) -> list[dict]:
    """Every generated spelling, unconditionally: no path is exempt, a file with no bodies is held to it, and a
    `rule 7 deferred` comment is inert - whoever owns the symbol. A field named `unk*` is rule 5's (`fields`)."""
    spans = [f.span for f in fields if f.span is not None]

    def in_field(pos: int) -> bool:
        return any(a <= pos < b for a, b in spans)

    out = []
    for m in RULE7_FN_RE.finditer(src.code):
        out.append(_finding(src, 7, src.line_of(m.start()), "auto-generated name `%s`" % m.group(0), token=m.group(0)))
    for m in RULE7_UNK_RE.finditer(src.code):
        if not in_field(m.start()):
            out.append(_finding(src, 7, src.line_of(m.start()), "bare `%s` identifier" % m.group(0), token=m.group(0)))
    for m in RULE7_LBL_RE.finditer(src.code):
        out.append(_finding(src, 7, src.line_of(m.start()),
                            "data label `%s` - name it from what it holds and where it is used, and "
                            "rename the map row" % m.group(0), token=m.group(0)))
    return out
