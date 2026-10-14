"""Rule 6: no pointer arithmetic reaches a field (a cast plus a literal byte offset, `memset`/`memcpy` ranges excepted).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Source, _finding


# --------------------------------------------------------------------------------------------------
# rules
# --------------------------------------------------------------------------------------------------
_PTR_TYPE = (r"(?:const\s+)?(?:unsigned\s+|signed\s+)?"
             r"(?:u8|u16|u32|u64|s8|s16|s32|s64|char|short|int|long|float|double|f32|f64|void|BOOL)")
RULE6_RE = re.compile(
    r"\(\s*" + _PTR_TYPE + r"\s*\*+\s*\)"      # a cast to a pointer
    r"\s*\(?\s*"                               # optional opening paren
    r"[A-Za-z_0-9\.\->\[\]\*&\s]*?"            # the base expression (no operators/separators)
    r"[+\-]\s*(?:0x[0-9A-Fa-f]+|\d+)"          # plus/minus a literal byte offset
)

# rule 6's own exception: a raw byte offset is allowed where no field is being named - a `memset`/`memcpy`
# range is the common case, so a cast-plus-offset that is an argument to one of these is not reported.
MEM_FUNCS = {"memset", "memcpy", "memmove", "memcmp", "bzero", "__memcpy", "__fill_mem"}

# Owner-granted exemption (docs/pipeline.md 11, 2026-10-14): Broadcom's BT_HDR payload idiom
# `(u8*)(p_buf + 1) + p_buf->offset` (the bytes after the header, at the buffer's own offset), under src/BTE/ only.
# The same pointer must appear on both sides; any other cast-plus-offset still fires.
BT_HDR_DIRS = ("src/BTE/",)
BT_HDR_RE = re.compile(r"\(\s*u8\s*\*\s*\)\s*\(\s*([A-Za-z_]\w*)\s*\+\s*1\s*\)\s*\+\s*\1\s*->\s*offset\b")


def bt_hdr_payload(src: Source, pos: int) -> bool:
    """True when the match at `pos` is the exempt BT_HDR payload idiom in a BTE source."""
    rel = src.rel.replace("\\", "/")
    return rel.startswith(BT_HDR_DIRS) and BT_HDR_RE.match(src.code, pos) is not None


def enclosing_call(code: str, pos: int) -> str | None:
    """The identifier of the call whose argument list contains `pos`, or None."""
    depth = 0
    i = pos - 1
    while i >= 0:
        c = code[i]
        if c == ")":
            depth += 1
        elif c == "(":
            if depth == 0:
                m = re.search(r"([A-Za-z_]\w*)\s*$", code[:i])
                return m.group(1) if m else None
            depth -= 1
        elif c in ";{}":
            return None
        i -= 1
    return None


def findings(src: Source) -> list[dict]:
    """A cast-plus-literal-offset expression, unless it is an argument of a `memset`/`memcpy`-shaped call."""
    out = []
    for m in RULE6_RE.finditer(src.code):
        if enclosing_call(src.code, m.start()) in MEM_FUNCS:
            continue
        if bt_hdr_payload(src, m.start()):
            continue
        out.append(_finding(src, 6, src.line_of(m.start()),
                            "pointer arithmetic: `%s`" % m.group(0).strip(), token=m.group(0).strip()))
    return out
