"""Rule 9: a mangled spelling is never the callable identifier - a call or a declaration of `Name__F...`.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.lib import cscan
from tools.units.stylelint_rules.common import Source, _finding


# Rule 9: a compiler-mangled name used as a callable identifier. MWCC's manglings carry an argument list
# (`Name__FP...`, `Name__Fv`) or a qualified owner (`Name__Q34nw4r...`), and a class member is
# `member__<len>Class<Fargs>` (`move__6MHcharFUs`). An `fn_XXXXXXXX` stem has no `__` and is the map's own
# placeholder (rule 7's), not a mangling, so it is never matched here.
RULE9_MANGLED_RE = re.compile(r"^[A-Za-z_]\w*__(?:[FQ]\w*|\d\w*F\w*)$")
RULE9_CALL_RE = re.compile(r"(?<![\w])([A-Za-z_]\w*)\s*\(")
# The statement head of a declaration: an optional `extern` (its `"C"` is blanked by `strip`, so it shows
# as spaces), declaration specifiers, a return type (a builtin, a unit alias or a namespaced name), and
# nothing an expression could end with. A call's head is empty (`foo()`), has an operator, or is a control
# keyword, so it fails this and is reported as a call.
_DECL_HEAD_RE = re.compile(
    r"^\s*(?:extern\s+)?"
    r"(?:(?:static|inline|virtual|const|unsigned|signed|struct|class|union|register|typedef)\s+)*"
    r"(?:void|bool|BOOL|char|short|int|long|float|double"
    r"|[us](?:8|16|32|64)|f(?:32|64)"
    r"|[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*)"
    r"\s*[\*&]*\s*$"
)
_DECL_KEYWORDS = cscan.STATEMENT_KEYWORDS


def _statement_head(code: str, pos: int) -> str:
    """The text between the previous statement terminator and `pos` (a mangled identifier)."""
    start = 0
    for i in range(pos - 1, -1, -1):
        if code[i] in ";{}":
            start = i + 1
            break
    return code[start:pos]


def looks_like_declaration(code: str, pos: int) -> bool:
    """Whether the mangled identifier at `pos` is being declared rather than called.

    A declaration puts a return type (or `extern`) in front of the identifier and nothing an expression
    could end with, so the statement head decides it: `void foo__Fv(void);` and `extern "C" void
    foo__Fv(void);` are declarations, while `foo__Fv()`, `x = foo__Fv()`, `return foo__Fv()` and
    `if (foo__Fv())` are calls.
    """
    head = _statement_head(code, pos)
    ids = re.findall(r"[A-Za-z_]\w*", head)
    if ids and ids[-1] in _DECL_KEYWORDS:
        return False
    return bool(_DECL_HEAD_RE.match(head))


def findings(src: Source) -> list[dict]:
    """Both halves of rule 9: a mangled identifier called (`foo__Fv()`) or declared (`extern void foo__Fv(void);`).
    The declaration is where playbook row 50's double-mangle is born; the fix differs only in emphasis - a call is
    rewritten `obj->method(args)` / `ns::function(args)`, a declaration to the owner's real declaration."""
    out = []
    for m in RULE9_CALL_RE.finditer(src.code):
        name = m.group(1)
        if not RULE9_MANGLED_RE.match(name):
            continue
        line = src.line_of(m.start())
        if looks_like_declaration(src.code, m.start()):
            out.append(_finding(src, 9, line, "mangled name `%s` is declared - declare its owner and include it" % name,
                                token=name))
        else:
            out.append(_finding(src, 9, line, "mangled name `%s` is called - call it through its owner" % name,
                                token=name))
    return out
