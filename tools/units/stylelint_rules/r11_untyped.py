"""Rule 11: no `void *` parameter or return type without a per-declaration `/* untyped: <reason> */` marker.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.lib import cscan
from tools.units.stylelint_rules.common import (
    Source, _finding, _mask_preproc, _split_parameters, _untyped_marker, function_declarations,
)


# --------------------------------------------------------------------------------------------------
# rule 11: a `void *` parameter or return type is a finding
# --------------------------------------------------------------------------------------------------
# A `void *` parameter or return type is banned outright: erasing the type hides what the call sites
# actually pass, and a heterogeneous call site is evidence the *sites* disagree, not that the declaration
# is untyped. The rule reads **declarations**, so it can never fire on a cast: a cast lives in a body, and
# the scanner walks only file-scope/namespace/class-scope statements and function definition headers.
RULE11_VOID_PTR_RE = re.compile(r"\bvoid\s*\*")
# The marker's reason must say which genuinely-untyped case it is: a byte range (memcpy-shaped), an opaque
# handle passed through, or a caller-owned payload. An empty or vague reason is still a finding.
RULE11_REASON_RES = (
    re.compile(r"\bbyte|\brange\b|\braw\b|\bbuffer\b|\bblob\b|mem(?:cpy|move|set)\b", re.I),
    re.compile(r"\bopaque\b|\bhandle\b|\btoken\b|\bcookie\b|\bcontext\b|pass(?:ed)?[- ]?through\b|"
               r"\bpassthrough\b|\bforward(?:ed)?\b", re.I),
    re.compile(r"\bpayload\b|\bcaller\b|\bowned\b|\bownership\b|user[- ]data", re.I),
)
# `RULE11_LOCAL_RE` counts a `void *` local (out of the rule's scope): an identifier must follow the star(s),
# which a cast cannot satisfy - `(void*)p` has `)` there - so the count is of declarations, not casts.
RULE11_LOCAL_RE = re.compile(r"\bvoid\s*\*+\s*([A-Za-z_]\w*)")
# A statement head that opens a `{` but is not a declaration: a control-flow block, or a macro/keyword that
# is an expression at file scope (`static_assert(sizeof(void*) == 4)`). The rule must not read such a
# parenthesised expression as a parameter list.
RULE11_NON_DECL_HEADS = cscan.NON_DECL_HEADS


def _untyped_reason_ok(reason: str) -> bool:
    """Whether a marker's reason names a genuinely-untyped case rather than restating the ban."""
    return bool(reason.strip()) and any(rx.search(reason) for rx in RULE11_REASON_RES)


_RULE11_PARAM_DETAIL = ("`void *` parameter type - name the real type (every call site passes one), or "
                        "mark the declaration `/* untyped: <byte range|opaque handle|caller-owned "
                        "payload> */`")
_RULE11_RET_DETAIL = ("`void *` return type - name the real type, or mark the declaration "
                      "`/* untyped: <byte range|opaque handle|caller-owned payload> */`")


def rule11_findings(src: Source) -> list[dict]:
    """Rule 11 for one file: every `void *` parameter or return type without a valid marker."""
    code = _mask_preproc(src.code)
    out = []
    for d in function_declarations(src):
        marker = _untyped_marker(src, d["start_line"], d["end_line"])
        if marker is not None and _untyped_reason_ok(marker):
            continue
        for chunk, off in _split_parameters(code, d["params_pos"], d["params_pos"] + len(d["params"])):
            if RULE11_VOID_PTR_RE.search(chunk):
                first = off + (len(chunk) - len(chunk.lstrip()))
                out.append(_finding(src, 11, src.line_of(first), _RULE11_PARAM_DETAIL, token=d["name"]))
        if RULE11_VOID_PTR_RE.search(d["ret"]):
            out.append(_finding(src, 11, d["line"], _RULE11_RET_DETAIL, token=d["name"]))
    return out


def rule11_local_count(src: Source) -> int:
    """The `void *` locals in the file's function bodies - out of rule 11's scope, counted for the owner.

    Bodies are merged so a nested block is not counted twice; a cast cannot match because an identifier
    must follow the star(s).
    """
    ranges = sorted((d["body"] for d in function_declarations(src)
                     if d.get("body") and d["body"][1] > d["body"][0]), key=lambda r: r[0])
    merged: list[list[int]] = []
    for a, b in ranges:
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return sum(len(RULE11_LOCAL_RE.findall(src.code[a + 1:b])) for a, b in merged)
