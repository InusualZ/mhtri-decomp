"""Rule 8: `goto` is forbidden.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Source, _finding

RULE8_RE = re.compile(r"\bgoto\b")

# Owner-granted exemptions (docs/pipeline.md 11): file -> (function, the most `goto`s allowed inside its body).
# A `goto` outside the named function, or past the count, still fires.
EXEMPT = {
    "src/ARC/arc.c": ("ARCConvertPathToEntrynum", 2),  # owner ruling 2026-10-14: the SDK's goto shape, one function
    "src/DVD/dvdfs.c": ("DVDConvertPathToEntrynum", 1),  # owner ruling 2026-10-14: the SDK's `goto next_hier`
    "src/DVD/dvddevice.c": ("__DVDCheckDevice", 2),  # owner ruling 2026-10-14: the SDK's shared `error` exit
}

FUNC_HEAD_RE = r"\b{name}\s*\([^;{{}}]*\)\s*\{{"


def _exempt_span(src: Source) -> "tuple[int, int, int] | None":
    """(body start, body end, allowed count) of the file's exempt function, or None."""
    entry = EXEMPT.get(src.rel.replace("\\", "/"))
    if entry is None:
        return None
    name, allowed = entry
    m = re.search(FUNC_HEAD_RE.format(name=re.escape(name)), src.code)
    if m is None:
        return None
    depth, pos = 0, m.end() - 1
    for i in range(pos, len(src.code)):
        if src.code[i] == "{":
            depth += 1
        elif src.code[i] == "}":
            depth -= 1
            if depth == 0:
                return pos, i, allowed
    return None


def findings(src: Source) -> list[dict]:
    """Every `goto` keyword in code (a comment or a literal naming it is not one), less an owner exemption."""
    span = _exempt_span(src)
    out, used = [], 0
    for m in RULE8_RE.finditer(src.code):
        if span and span[0] < m.start() < span[1] and used < span[2]:
            used += 1
            continue
        out.append(_finding(src, 8, src.line_of(m.start()), "goto statement"))
    return out
