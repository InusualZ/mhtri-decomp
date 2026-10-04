"""C/C++ lexical scanning for the source tools: comments, literals, braces, declarations, fields, includes.
Spec: docs/tools/spec/lib-cscan.md. CLI: none (library)."""
from __future__ import annotations

import bisect
import os
import re
from dataclasses import dataclass
from typing import Callable, Iterable

# --- the one lexer -------------------------------------------------------------------------------------------

#: A comment or a string/char literal. Scanned left to right, so a quote inside a comment and a comment opener
#: inside a literal are what they look like. A literal ends at its closing quote or *before* an unescaped
#: newline (an unterminated literal never swallows the next line); an escape takes the next character,
#: whatever it is; an unterminated block comment runs to the end of the text.
_LEX = re.compile(r"""/\*[\s\S]*?(?:\*/|\Z)
                    | //[^\n]*
                    | "(?:\\[\s\S]|[^"\\\n])*(?:"|\\\Z)?
                    | '(?:\\[\s\S]|[^'\\\n])*(?:'|\\\Z)?""", re.X)
_NOT_NL = re.compile(r"[^\n]")


@dataclass(frozen=True)
class Span:
    """One lexical span: `kind` is `block`, `line`, `string` or `char`; `end` is exclusive."""
    kind: str
    start: int
    end: int


def spans(text: str) -> list[Span]:
    """Every comment and literal of `text`, in order."""
    kinds = {"/*": "block", "//": "line"}
    out = []
    for m in _LEX.finditer(text):
        s = m.group()
        out.append(Span(kinds.get(s[:2]) or ("string" if s[0] == '"' else "char"), m.start(), m.end()))
    return out


def _blank_keep_nl(s: str) -> str:
    return _NOT_NL.sub(" ", s)


def strip(text: str) -> tuple[str, str]:
    """`(code, comments)`, each exactly `len(text)` long.

    `code` has every comment blanked (newlines kept) and every literal blanked entirely; `comments` is the text
    with only the literals blanked, so an annotation comment (`/* size: 0x10 */`) is found in it and the same
    words inside a string are not. A literal is blanked character for character, including an escaped newline,
    so offsets map to the original text exactly; line numbers come from the original text.
    """
    def code(m: re.Match) -> str:
        s = m.group()
        return _blank_keep_nl(s) if s[0] == "/" else " " * len(s)

    def comments(m: re.Match) -> str:
        s = m.group()
        return s if s[0] == "/" else " " * len(s)

    return _LEX.sub(code, text), _LEX.sub(comments, text)


def strip_comments(text: str) -> str:
    """`text` with comments and literals blanked, length and every newline preserved (a line count over the
    result is the original line)."""
    return _LEX.sub(lambda m: _blank_keep_nl(m.group()), text)


def remove_comments(text: str) -> str:
    """`text` with every comment removed and everything else (literals included) kept - not length-preserving;
    for normalised-source comparison."""
    return _LEX.sub(lambda m: "" if m.group()[0] == "/" else m.group(), text)


def mask_preproc(code: str) -> str:
    """Blank every preprocessor line and its `\\` continuations, positions and newlines preserved."""
    out = []
    cont = False
    for line in code.split("\n"):
        directive = cont or line.lstrip().startswith("#")
        cont = directive and line.rstrip().endswith("\\")
        out.append(" " * len(line) if directive else line)
    return "\n".join(out)


# --- brackets ------------------------------------------------------------------------------------------------

def _match(code: str, open_pos: int, opener: str, closer: str) -> int:
    depth = 0
    for i in range(open_pos, len(code)):
        c = code[i]
        if c == opener:
            depth += 1
        elif c == closer:
            depth -= 1
            if depth == 0:
                return i
    return -1


def match_brace(code: str, open_pos: int) -> int:
    """Index of the `}` matching the `{` at `open_pos` in stripped code, or -1."""
    return _match(code, open_pos, "{", "}")


def match_paren(code: str, open_pos: int) -> int:
    """Index of the `)` matching the `(` at `open_pos` in stripped code, or -1."""
    return _match(code, open_pos, "(", ")")


# --- a scanned file ------------------------------------------------------------------------------------------

class Text:
    """One file's text, its two stripped views (`strip`) and a line index."""

    def __init__(self, text: str):
        self.text = text
        self.code, self.comments = strip(text)
        self._starts = [0] + [m.end() for m in re.finditer("\n", text)]

    def line_of(self, pos: int) -> int:
        """The 1-based line holding offset `pos`."""
        return max(1, bisect.bisect_right(self._starts, pos))

    def line_text(self, line: int) -> str:
        start = self._starts[line - 1]
        end = self.text.find("\n", start)
        return self.text[start:] if end < 0 else self.text[start:end]

    def span_lines(self, start: int, end: int) -> tuple[int, int]:
        return self.line_of(start), self.line_of(max(start, end - 1))


# --- type definitions ----------------------------------------------------------------------------------------

STRUCT_RE = re.compile(r"\b(?:struct|class)\s*([A-Za-z_]\w*)?\s*\{")
_TAIL_NAME = re.compile(r"\s*([A-Za-z_]\w*)")


@dataclass(frozen=True)
class TypeDef:
    """A `struct`/`class` definition: name (an anonymous typedef takes its alias), offsets and lines."""
    name: str
    start: int
    open: int
    close: int
    line: int
    end_line: int

    def to_dict(self) -> dict:
        return {"name": self.name, "start": self.start, "open": self.open, "close": self.close,
                "line": self.line, "end_line": self.end_line}


def struct_defs(t: Text) -> list[TypeDef]:
    """Every `struct`/`class` definition with a body; `typedef struct { ... } Name;` is named `Name`."""
    out = []
    for m in STRUCT_RE.finditer(t.code):
        open_pos = m.end() - 1
        close_pos = match_brace(t.code, open_pos)
        if close_pos < 0:
            continue
        name = m.group(1)
        if not name:
            tail = _TAIL_NAME.match(t.code, close_pos + 1)
            name = tail.group(1) if tail else "<anonymous>"
        out.append(TypeDef(name, m.start(), open_pos, close_pos, t.line_of(m.start()), t.line_of(close_pos)))
    return out


def fields(code: str, open_pos: int, close_pos: int) -> list[tuple[int, int]]:
    """`(start, end)` of every `;`-terminated chunk at the top level of the body `code[open_pos+1:close_pos]`."""
    out = []
    depth = 0
    start = open_pos + 1
    for i in range(open_pos + 1, close_pos):
        c = code[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif c == ";" and depth == 0:
            out.append((start, i))
            start = i + 1
    return out


# --- declarations --------------------------------------------------------------------------------------------

#: Keywords that start a statement, never a declaration.
STATEMENT_KEYWORDS = frozenset({"return", "if", "while", "for", "switch", "case", "else", "do",
                                "sizeof", "break", "continue", "goto"})
#: Heads whose parenthesis is an expression, not a parameter list.
NON_DECL_HEADS = STATEMENT_KEYWORDS | {"catch", "alignof", "__alignof", "static_assert", "_Static_assert",
                                       "assert", "asm", "__asm"}
#: A `{` whose statement ends in `extern` opens a linkage block (`strip` blanks the `"C"`).
LINKAGE_OPEN_RE = re.compile(r"\bextern\s*$")
_IDENT = re.compile(r"[A-Za-z_]\w*")
_FNPTR_NAME = re.compile(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)")
_FNPTR_FN_NAME = re.compile(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\(")


def declared_name(segment: str) -> str | None:
    """The identifier a declaration statement introduces (function, function pointer or variable).

    The first `(` decides: `(*name)` is a function-pointer variable, `(*name(` a function returning one, anything
    else a function named by the last identifier before the `(`. Without a `(`, the last identifier before any
    `=` with array dimensions removed.
    """
    i = segment.find("(")
    if i >= 0:
        m = _FNPTR_NAME.match(segment, i) or _FNPTR_FN_NAME.match(segment, i)
        if m:
            return m.group(1)
        ids = _IDENT.findall(segment, 0, i)
        return ids[-1] if ids else None
    segment = re.sub(r"\[[^\]]*\]", " ", segment.split("=")[0])
    ids = _IDENT.findall(segment)
    return ids[-1] if ids else None


@dataclass(frozen=True)
class Declaration:
    """A function declaration or definition header: name, offsets, lines, parameter and return text.

    `body` is the `(open, close)` brace pair of a definition, None for a prototype.
    """
    name: str
    pos: int
    line: int
    start_line: int
    end_line: int
    params: str
    params_pos: int
    ret: str
    ret_pos: int
    body: tuple[int, int] | None = None

    def to_dict(self) -> dict:
        """The dict shape (`body` only on a definition)."""
        d = {"name": self.name, "pos": self.pos, "line": self.line, "start_line": self.start_line,
             "end_line": self.end_line, "params": self.params, "params_pos": self.params_pos,
             "ret": self.ret, "ret_pos": self.ret_pos}
        if self.body is not None:
            d["body"] = self.body
        return d


def _declared_name_pos(segment: str) -> tuple[str | None, int]:
    name = declared_name(segment)
    if name is None:
        return None, -1
    i = segment.find("(")
    if i >= 0:
        for rx in (_FNPTR_NAME, _FNPTR_FN_NAME):
            m = rx.match(segment, i)
            if m and m.group(1) == name:
                return name, m.start(1)
        k = i
        while k > 0 and (segment[k - 1].isascii() and (segment[k - 1].isalnum() or segment[k - 1] == "_")):
            k -= 1
        return name, k
    m = re.search(r"\b%s\b" % re.escape(name), segment)
    return (name, m.start()) if m else (None, -1)


def _declarator_parens(segment: str, name_pos: int, name_len: int) -> tuple[int, int] | None:
    j = name_pos + name_len
    while j < len(segment) and segment[j].isspace():
        j += 1
    if j < len(segment) and segment[j] == "(":
        open_pos = j
    elif j < len(segment) and segment[j] == ")":
        k = j + 1
        while k < len(segment) and segment[k].isspace():
            k += 1
        if k >= len(segment) or segment[k] != "(":
            return None
        open_pos = k
    else:
        return None
    close = match_paren(segment, open_pos)
    if close < 0:
        return None
    tail = segment[close + 1:].lstrip()
    if tail and not tail.startswith(("const", "volatile", "noexcept", "override", "final",
                                     "__attribute__", ":", "&", "throw", ")")):
        return None
    return open_pos, close


def _declaration_from(t: Text, start: int, end: int, code: str, body=None) -> Declaration | None:
    seg = code[start:end]
    name, name_pos = _declared_name_pos(seg)
    if name is None or name in NON_DECL_HEADS:
        return None
    parens = _declarator_parens(seg, name_pos, len(name))
    if parens is None:
        return None
    open_pos, close_pos = parens
    first = start + (len(seg) - len(seg.lstrip()))
    return Declaration(name, start + name_pos, t.line_of(start + name_pos), t.line_of(first),
                       t.line_of(start + close_pos), seg[open_pos + 1:close_pos], start + open_pos + 1,
                       seg[:name_pos], start, body)


def function_declarations(t: Text) -> list[Declaration]:
    """Every function declaration and definition header at file, namespace or class scope - never a statement
    inside a function body (so a cast is never read as a declaration). A linkage block is transparent."""
    code = mask_preproc(t.code)
    out: list[Declaration] = []
    scopes: list[str] = []          # "function" for a body, "other" for a type/namespace/block, "linkage"
    stmt_start = 0
    for i, c in enumerate(code):
        if c == "{":
            if LINKAGE_OPEN_RE.search(code, stmt_start, i):
                scopes.append("linkage")
            else:
                decl = _declaration_from(t, stmt_start, i, code)
                if decl is not None:
                    out.append(Declaration(**{**decl.__dict__, "body": (i, match_brace(code, i))}))
                    scopes.append("function")
                else:
                    scopes.append("other")
            stmt_start = i + 1
        elif c == "}":
            if scopes:
                scopes.pop()
            stmt_start = i + 1
        elif c == ";":
            if "function" not in scopes:
                decl = _declaration_from(t, stmt_start, i, code)
                if decl is not None:
                    out.append(decl)
            stmt_start = i + 1
    return out


# --- the names a file declares (types, typedefs, aliases, macros, inline helpers, externs) --------------------

#: Words that show up in a declarator but are never the declared name.
NOT_NAMES = frozenset({
    "const", "volatile", "struct", "union", "enum", "class", "signed", "unsigned", "int", "char",
    "short", "long", "float", "double", "void", "typedef", "restrict", "register", "static", "extern",
    "inline", "typename", "__attribute__", "attribute", "packed", "aligned", "__declspec", "constexpr",
})
_TYPE_KEYWORD = re.compile(r"\b(typedef\s+)?(struct|union|enum|class)\b")
_TAG = re.compile(r"[ \t\r\n]*([A-Za-z_]\w*)")
_SIMPLE_TYPEDEF = re.compile(r"(?m)^[ \t]*typedef[ \t]+(?!struct\b|union\b|enum\b|class\b)([^;{}]+);")
_USING = re.compile(r"(?m)^[ \t]*using[ \t]+([A-Za-z_]\w*)[ \t]*=")
_MACRO = re.compile(r"(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)")
_INLINE = re.compile(r"\b(inline|__inline)\b[^;{}()]*?\b([A-Za-z_]\w*)[ \t]*\(([^;{}]*)\)[ \t]*\{")
_EXTERN = re.compile(r"(?m)^[ \t]*extern[ \t]+([^;{}]+);")
_DIMS = re.compile(r"\[[^\]]*\]")


def _split_top(body: str) -> list[str]:
    """`body` split on the `;`s at bracket depth 0 (`()`, `[]`, `{}`), the tail included."""
    parts, stack, start = [], [], 0
    pairs = {")": "(", "]": "[", "}": "{"}
    for i, c in enumerate(body):
        if c in "([{":
            stack.append(c)
        elif c in ")]}":
            if stack and stack[-1] == pairs[c]:
                stack.pop()
        elif c == ";" and not stack:
            parts.append(body[start:i])
            start = i + 1
    parts.append(body[start:])
    return parts


def shape_of(body: str) -> str:
    """A field-type fingerprint of an aggregate body, insensitive to field names: `f32 f32 f32` for a 3-float
    vector under any name; a padding array keeps its type (`u8[]`), a function pointer is `fnptr`, a nested
    aggregate `agg`."""
    parts = []
    for stmt in _split_top(body):
        stmt = stmt.strip()
        if not stmt:
            continue
        if "{" in stmt:
            parts.append("agg")
            continue
        if re.search(r"\(\s*\*", stmt):
            parts.append("fnptr")
            continue
        dims = _DIMS.findall(stmt)
        core = _DIMS.sub("", stmt)
        ids = [x for x in _IDENT.findall(core) if x not in NOT_NAMES]
        if not ids:
            continue
        parts.append((" ".join(ids[:-1]) or "?") + "[]" * len(dims))
    return " ".join(parts)


def declarator_name(region: str) -> str | None:
    """The name a `typedef`/`extern` declarator introduces: the last identifier, the one inside `(*...)` for a
    function pointer, the one before `[` for an array."""
    region = _DIMS.sub(" ", region)
    m = _FNPTR_NAME.search(region)
    if m:
        return m.group(1)
    region = re.sub(r"\([^()]*\)", " ", region)
    ids = [x for x in _IDENT.findall(region) if x not in NOT_NAMES]
    return ids[-1] if ids else None


def alias_names(tail: str) -> list[str]:
    """The alias names after the `}` closing an aggregate definition, in order."""
    return [x for x in _IDENT.findall(tail) if x not in NOT_NAMES]


@dataclass(frozen=True)
class NameDecl:
    """A name a file declares: `kind` is the aggregate keyword, `typedef`, `using`, `macro`, `inline` or
    `extern`; `shape` is the `shape_of` fingerprint of an aggregate or of an inline's parameters."""
    name: str
    kind: str
    line: int
    shape: str = ""


def declared_names(clean: str) -> list[NameDecl]:
    """Every name `clean` (a `strip_comments` text) declares, one entry per name, first declaration wins:
    `typedef struct Foo { ... } Foo;` records `Foo` once, as `struct`."""
    decls: list[NameDecl] = []
    seen: set[str] = set()

    def add(name: str | None, kind: str, pos: int, shape: str = "") -> None:
        if not name or name in NOT_NAMES or name in seen:
            return
        seen.add(name)
        decls.append(NameDecl(name, kind, clean.count("\n", 0, pos) + 1, shape))

    for m in _TYPE_KEYWORD.finditer(clean):
        is_typedef, keyword = bool(m.group(1)), m.group(2)
        j = m.end()
        tm = _TAG.match(clean, j)
        tag = tm.group(1) if tm else None
        after = tm.end() if tm else j
        k = after
        while k < len(clean) and clean[k] in " \t\r\n":
            k += 1
        if k < len(clean) and clean[k] == "{":
            end = match_brace(clean, k)
            if end < 0:
                end = len(clean) - 1
            semi = clean.find(";", end)
            tail = clean[end + 1:semi if semi >= 0 else len(clean)]
            shape = shape_of(clean[k + 1:end])
            if tag:
                add(tag, keyword, m.start(), shape)
            for nm in alias_names(tail):
                add(nm, "typedef", m.start(), shape)
        elif is_typedef:
            semi = clean.find(";", after)
            region = clean[after:semi if semi >= 0 else len(clean)]
            if tag:
                add(tag, keyword, m.start())
            add(declarator_name(region), "typedef", m.start())

    for m in _SIMPLE_TYPEDEF.finditer(clean):
        add(declarator_name(m.group(1)), "typedef", m.start())
    for m in _USING.finditer(clean):
        add(m.group(1), "using", m.start())
    for m in _MACRO.finditer(clean):
        add(m.group(1), "macro", m.start())
    for m in _INLINE.finditer(clean):
        add(m.group(2), "inline", m.start(), shape_of(m.group(3)))
    for m in _EXTERN.finditer(clean):
        add(declarator_name(m.group(1)), "extern", m.start())
    return decls


# --- includes ------------------------------------------------------------------------------------------------

#: An `#include "..."` or `#include <...>` directive at the start of a line.
INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*(["<])([^">\n]+)[">]', re.M)


def includes(text: str, angle: bool = True) -> list[str]:
    """The names `text` includes, in order (`angle=False`: the quoted form only)."""
    return [m.group(2) for m in INCLUDE_RE.finditer(text) if angle or m.group(1) == '"']


def resolve_include(name: str, bases: Iterable[str]) -> str | None:
    """`os.path.join(base, name)` for the first base where that file exists, else None."""
    for base in bases:
        candidate = os.path.join(base, name)
        if os.path.isfile(candidate):
            return candidate
    return None


def include_closure(start: str, resolve: Callable[[str, str], str | None],
                    read: Callable[[str], str] | None = None, angle: bool = False) -> list[str]:
    """Every file reachable from `start` through its includes, depth first, `start` first, each file once.

    `resolve(name, includer)` names the file an include means (None: outside the tree); `read(path)` gives a
    file's text (default UTF-8). An unreadable file ends its branch.
    """
    def default_read(path: str) -> str:
        with open(path, encoding="utf-8", newline="") as fh:
            return fh.read()

    read = read or default_read
    seen: set[str] = set()
    order: list[str] = []

    def visit(path: str) -> None:
        key = os.path.normcase(os.path.abspath(path))
        if key in seen:
            return
        seen.add(key)
        order.append(path)
        try:
            text = read(path)
        except OSError:
            return
        for name in includes(text, angle=angle):
            found = resolve(name, path)
            if found:
                visit(found)

    visit(start)
    return order


# --- identifier rewriting (a rename's other half) ------------------------------------------------------------

#: The suffixes a path mention ends with (`fn_805113B0.h` is a file, never the symbol).
PATH_SUFFIXES = (".c", ".h", ".cpp", ".hpp", ".cp", ".cc", ".o", ".s", ".inc")
#: A character beside the name that makes it a path segment (`/`, `\`) or a string-table / member spelling
#: (`@stringBase0`, `name$1`, `.name`), never the identifier itself.
_PATH_ADJ = "/\\"
_STRTAB_BEFORE = "@$."
_STRTAB_AFTER = "@$"
_INCLUDE_LINE = re.compile(r"[ \t]*#[ \t]*include\b")


def rewrite_identifiers(text: str, pairs: dict[str, str], comments: bool = False) -> tuple[str, dict[str, dict]]:
    """`text` with every whole-word occurrence of a `pairs` key replaced by its value, and the counts per name.

    Rewritten: a code token; with `comments`, also a comment's mention. Never rewritten (counted `skipped`): a
    string or char literal (program data - a name in a log string is the binary's bytes), an `#include` line, a
    token beside `/` or `\\` or followed by a source suffix (a path - the file keeps its name), and a token after
    `@`, `$` or `.` or before `@`, `$` (a string-table, section or member spelling). `\\b` treats `_` as a word
    character, so a mangled `name__Fv` never matches `name`. Counts: `{name: {"code", "comment", "kept_comment",
    "skipped"}}` (`kept_comment`: a comment mention left because `comments` is off).
    """
    counts = {k: {"code": 0, "comment": 0, "kept_comment": 0, "skipped": 0} for k in pairs}
    if not pairs or not any(k in text for k in pairs):
        return text, counts
    rx = re.compile(r"\b(%s)\b" % "|".join(re.escape(n) for n in sorted(pairs, key=len, reverse=True)))
    regions = spans(text)
    starts = [s.start for s in regions]
    out: list[str] = []
    last = 0
    for m in rx.finditer(text):
        name, a, b = m.group(1), m.start(), m.end()
        j = bisect.bisect_right(starts, a) - 1
        kind = regions[j].kind if j >= 0 and regions[j].start <= a < regions[j].end else "code"
        before = text[a - 1] if a else ""
        after = text[b] if b < len(text) else ""
        path_like = (before in _PATH_ADJ or after in _PATH_ADJ
                     or any(text.startswith(s, b) and not text[b + len(s):b + len(s) + 1].isalnum()
                            for s in PATH_SUFFIXES))
        strtab_like = (before and before in _STRTAB_BEFORE) or (after and after in _STRTAB_AFTER)
        include_line = _INCLUDE_LINE.match(text, text.rfind("\n", 0, a) + 1) is not None
        if kind in ("string", "char") or include_line or path_like or strtab_like:
            rewrite = False
        elif kind in ("block", "line"):
            rewrite = comments
        else:
            rewrite = True
        if rewrite:
            out.append(text[last:a])
            out.append(pairs[name])
            last = b
            counts[name]["comment" if kind in ("block", "line") else "code"] += 1
        else:
            counts[name]["kept_comment" if kind in ("block", "line") and rewrite is False and not (
                include_line or path_like or strtab_like) else "skipped"] += 1
    out.append(text[last:])
    return "".join(out), counts
