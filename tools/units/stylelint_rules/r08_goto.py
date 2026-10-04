"""Rule 8: `goto` is forbidden.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Source, _finding

RULE8_RE = re.compile(r"\bgoto\b")


def findings(src: Source) -> list[dict]:
    """Every `goto` keyword in code (a comment or a literal naming it is not one)."""
    return [_finding(src, 8, src.line_of(m.start()), "goto statement") for m in RULE8_RE.finditer(src.code)]
