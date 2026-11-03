"""Rule 8, its second check: a `case`/`default` label sits directly in its innermost switch's body, never inside another block
(`if (0) { case N: }`, another case's block, a loop - owner ruling 2026-11-03, docs/pipeline.md 11).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.lib import cscan
from tools.units.stylelint_rules.common import Source, _finding

#: The `check` a finding of this module carries, beside rule 8's `goto` findings (which carry none).
CHECK = "case-in-block"

_WORD_RE = re.compile(r"\b(switch|case|default)\b")
_HEAD_LABEL_RE = re.compile(r"^(?:(?:case\b(?:[^:]|::)*|default\s*):(?!:)\s*)+")
_QUALIFIER_RE = re.compile(r"((?:[A-Za-z_]\w*\s*::\s*)+)$")
_HEAD_IF0_RE = re.compile(r"^if\s*\(\s*0\s*\)$")


def _label_end(code: str, pos: int) -> int:
    """Index of the `:` that ends the label starting at `pos` (a `::` scope and a bracketed operand are skipped),
    or -1 when a `;`/`{`/`}` comes first - the keyword is not a label."""
    depth = 0
    i = pos
    while i < len(code):
        c = code[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c in ";{}":
            return -1
        elif c == ":" and depth == 0:
            if code.startswith("::", i):
                i += 2
                continue
            return i
        i += 1
    return -1


def _form(code: str, open_pos: int) -> str:
    """What opened the block at `open_pos`: `if (0)`, `if`, `else`, `loop` or a bare `block`."""
    start, depth = 0, 0
    for i in range(open_pos - 1, -1, -1):
        c = code[i]
        if c == ")":
            depth += 1
        elif c == "(":
            depth -= 1
        elif c in ";{}" and depth <= 0:
            start = i + 1
            break
    head = _HEAD_LABEL_RE.sub("", " ".join(code[start:open_pos].split()))
    if _HEAD_IF0_RE.match(head):
        return "if (0)"
    if re.match(r"^(?:else\s+)?if\b", head):
        return "if"
    if re.match(r"^else$", head):
        return "else"
    if re.match(r"^(?:while|for)\b", head) or head == "do":
        return "loop"
    return "block"


def _enclosing_function(defs: list, pos: int, src_code: str) -> str | None:
    """The name of the innermost function definition whose body holds `pos`, with its `Class::` qualifier."""
    best = None
    for d in defs:
        if d.body is not None and d.body[0] < pos < d.body[1]:
            if best is None or d.body[0] > best.body[0]:
                best = d
    if best is None:
        return None
    qual = _QUALIFIER_RE.search(src_code[max(0, best.pos - 200):best.pos])
    return (re.sub(r"\s+", "", qual.group(1)) if qual else "") + best.name


def sites(src: Source) -> list[dict]:
    """Every case/default label whose innermost enclosing block is not its switch's body:
    `{line, label, form, function}`. Comments, literals and preprocessor lines are not code."""
    code = cscan.mask_preproc(src.code)
    switch_bodies: set[int] = set()
    for m in re.finditer(r"\bswitch\b\s*\(", code):
        close = cscan.match_paren(code, m.end() - 1)
        if close < 0:
            continue
        j = close + 1
        while j < len(code) and code[j].isspace():
            j += 1
        if j < len(code) and code[j] == "{":
            switch_bodies.add(j)
    labels: dict[int, str] = {}
    for m in _WORD_RE.finditer(code):
        if m.group(1) == "switch":
            continue
        end = _label_end(code, m.end())
        if end < 0:
            continue
        labels[m.start()] = " ".join(src.text[m.start():end].split())
    if not labels:
        return []
    defs = None
    out: list[dict] = []
    stack: list[int] = []
    for i, c in enumerate(code):
        if c == "{":
            stack.append(i)
        elif c == "}":
            if stack:
                stack.pop()
        elif i in labels:
            owner = next((k for k in range(len(stack) - 1, -1, -1) if stack[k] in switch_bodies), None)
            if owner is None or owner == len(stack) - 1:
                continue
            if defs is None:
                defs = cscan.function_declarations(src)
            out.append({"line": src.line_of(i), "label": labels[i], "form": _form(code, stack[-1]),
                        "block_line": src.line_of(stack[-1]),
                        "function": _enclosing_function(defs, i, code)})
    return out


def findings(src: Source) -> list[dict]:
    """Rule 8 findings, one per nested label. The token is the enclosing function and the detail names the label
    and the block, so the add-only `--diff` refuses a new site and never a line move of an existing one."""
    out = []
    for s in sites(src):
        detail = "`%s:` label inside a nested %s block of its switch" % (
            s["label"], "`if (0) { ... }`" if s["form"] == "if (0)" else "`%s`" % s["form"])
        out.append(dict(_finding(src, 8, s["line"], detail, token=s["function"]), check=CHECK))
    return out
