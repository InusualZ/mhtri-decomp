"""A one-function translation unit: a C/C++ file's preamble and declarations, one function's definition, and every
other function reduced to a declaration (or dropped), line numbers preserved.
Spec: docs/tools/spec/lib-cextract.md. CLI: none (library)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from tools.lib import cscan, names

#: Heads whose definition is kept whole: the compiler needs the body to inline or instantiate it.
KEEP_HEAD = re.compile(r"\b(inline|__inline|template)\b")


@dataclass
class Extraction:
    """The rewritten text and what happened to each top-level function definition (`name@line`)."""
    text: str
    kept: list = field(default_factory=list)       # the target's definition(s)
    whole: list = field(default_factory=list)      # inline / template definitions kept for the compiler
    stubbed: list = field(default_factory=list)    # free functions reduced to a declaration
    dropped: list = field(default_factory=list)    # out-of-class member definitions removed

    def to_dict(self) -> dict:
        return {"kept": self.kept, "whole": self.whole, "stubbed": self.stubbed, "dropped": self.dropped}


def target_identifiers(function: str) -> set[str]:
    """The source identifiers a dump target names: itself when unmangled, the stem of a mangled name
    (`send__16NetworkSingleTcpFPCUcl` -> `send`), and the class of a constructor/destructor (`__ct__3FooFv` ->
    `Foo`, the last component of a `Q` qualifier)."""
    owner = names.owner_stem(function)
    if owner == function and not names.is_mangled(function):
        return {function}
    if not function.startswith(("__ct__", "__dt__")):
        i = owner.find("__", 1)
        return {owner[:i] if i > 0 else owner}
    stem, rest = function[:4], function[6:]
    q = re.match(r"Q(\d)", rest)
    parts, k = [], 2 if q else 0
    for _ in range(int(q.group(1)) if q else 1):
        d = re.match(r"\d+", rest[k:])
        if not d:
            break
        k += len(d.group())
        parts.append(rest[k:k + int(d.group())])
        k += int(d.group())
    return {parts[-1]} if parts else {stem}


def _directive_lines(code: str) -> set[int]:
    """0-based numbers of the preprocessor lines (and their continuations) of comment-stripped `code`."""
    out, cont = set(), False
    for i, line in enumerate(code.split("\n")):
        directive = cont or line.lstrip().startswith("#")
        cont = directive and line.rstrip().endswith("\\")
        if directive:
            out.add(i)
    return out


def extract(text: str, function: str) -> Extraction:
    """`text` with every top-level function definition except `function`'s reduced: an inline or template one is
    kept whole, an out-of-class member definition (`A::f() {...}`) is removed (its class declares it), and any other
    becomes its declaration (`int f(int x);`). Removed text is blanked with its newlines and preprocessor lines
    kept, so every line keeps its number and every `#if` its partner. SystemExit when no definition is the target."""
    t = cscan.Text(text)
    masked = cscan.mask_preproc(t.code)
    classes = [(d.open, d.close) for d in cscan.struct_defs(t)]
    defs = [d for d in cscan.function_declarations(t) if d.body]
    bodies = [d.body for d in defs]
    top = [d for d in defs
           if not any(o < d.pos < c for o, c in classes)
           and not any(o < d.pos < c for o, c in bodies if (o, c) != d.body)]
    wanted = target_identifiers(function)
    directives = _directive_lines(t.code)
    out = list(text)
    result = Extraction(text)

    def blank(a: int, b: int):
        for i in range(a, b):
            if out[i] != "\n" and t.line_of(i) - 1 not in directives:
                out[i] = " "

    for d in top:
        head_start = d.ret_pos + (len(masked[d.ret_pos:d.body[0]]) - len(masked[d.ret_pos:d.body[0]].lstrip()))
        head = masked[head_start:d.body[0]]
        tag = "%s@%d" % (d.name, d.line)
        qualified = re.search(r"::\s*~?\s*%s\s*\(" % re.escape(d.name), head) is not None
        if d.name in wanted:
            result.kept.append(tag)
        elif KEEP_HEAD.search(head):
            result.whole.append(tag)
        elif qualified:
            result.dropped.append(tag)
            blank(head_start, d.body[1] + 1)
        else:
            result.stubbed.append(tag)
            blank(d.body[0], d.body[1] + 1)
            out[d.body[0]] = ";"  # the body's `{` becomes the declaration's `;`
    if not result.kept:
        raise SystemExit("extract: no top-level definition of %s (looked for %s)"
                         % (function, ", ".join(sorted(wanted))))
    result.text = "".join(out)
    return result
