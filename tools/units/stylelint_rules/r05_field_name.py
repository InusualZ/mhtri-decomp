"""Rule 5: no field is left named `unk*` (`pad_0xNN` / `unused_0xNN` are the exception).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Field, Source, _finding

UNK_FIELD_RE = re.compile(r"^unk\w*$")


def findings(src: Source, fields: list[Field]) -> list[dict]:
    """A field named `unk`/`unkNN` - it needs a name from its context, or `pad_0xNN` when it is padding."""
    return [_finding(src, 5, f.line, "field `%s` needs a context name or pad_0xNN" % f.name, token=f.name)
            for f in fields if UNK_FIELD_RE.match(f.name)]
