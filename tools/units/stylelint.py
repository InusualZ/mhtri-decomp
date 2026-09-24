#!/usr/bin/env python3
"""Lint `src/` against the type and naming discipline of docs/plan.md section 6.5 (roadmap 7.21).

A rule enforced by remembering is not a rule: eight conformance rules were agreed with the owner and the
only thing keeping them is the reviewer's attention. This tool turns the mechanically checkable ones into
`file:line` findings, and `land.py verify` calls it with `--diff <base>` so a batch that **adds** a
violation is refused before it is committed.

    python tools/units/stylelint.py --budget          # the per-unit backlog over src/
    python tools/units/stylelint.py --diff <ref>      # exit 0 = the batch adds no violation
    python tools/units/stylelint.py --json            # machine-readable findings + budget
    python tools/units/stylelint.py --selftest

Rules checked (each finding is `file:line`):

| # | rule | how it is decided |
| --- | --- | --- |
| 1 | a shared type lives in one header | the same `struct`/`class`/`union` name defined with a body in more than one `src/` file: one finding per (type, extra file), naming both files |
| 2 | an extern lives with the TU that owns it | an `extern` declaration of a symbol whose registered owner (symbols.txt address + splits.txt range) is another unit, or of an unsplit symbol whose address band names a module (`include/unsplit/<module>.h`) |
| 3 | a reconstructed `struct`/`class` states its size | a `size: 0xNN` comment within four lines of the definition (or two lines after its closing brace) |
| 4 | every field carries its offset | an offset comment on the field's own line(s); `/* +0x1C */` is the canonical form and the `/* 0x1C */` variant the existing units use is accepted |
| 5 | no field is left named `unk*` | a field name matching `unk`, `unkNN`; `pad_0xNN` / `unused_0xNN` are the exception |
| 6 | no pointer arithmetic reaches a field | a `(T*)base + 0xNN` / `(T*)(base + 0xNN)` cast-plus-literal-offset expression, except an offset passed straight to `memset`/`memcpy`/`memmove` (the rule's own byte-range exception) |
| 7 | no auto-generated name survives | `fn_XXXXXXXX` anywhere, and `unk*` used for anything that is not a struct field (a field is rule 5's); **exempt per file** - see below |
| 8 | `goto` is forbidden | the `goto` keyword |
| 9 | a mangled symbol is called/declared through its owner | a callee identifier that carries a compiler mangling (`Name__FP...`, `Name__Q34nw4r...`, a class member `name__<len>ClassF...`) used as a call **or** as a declaration; an `fn_XXXXXXXX` stem has no `__` and stays legal |

Rule 2 is checked from `config/RMHE08/symbols.txt` (a symbol's section and address) + `config/RMHE08/splits.txt`
(each registered unit's ranges): an `extern` a file declares for a symbol another registered unit owns is a
finding - move it to that unit's header and `#include` it. An unsplit symbol (no registered owner) is judged
the same way, into `include/unsplit/<module>.h`, but only where the module is sound: the registered bands
interleave across modules (a `sound` unit sits inside the `ef` band), so an address whose bracketing units
name different modules is left as a counted gap (`Ownership.gaps`) rather than a guessed header. A symbol
missing from the map, a duplicate map row, and an address the map gives no section are gaps too.

Comments and string/char literals are stripped before matching, and the two are stripped separately: the
size/offset annotations of rules 3-4 live *in comments*, while every other rule must not fire on text
inside one. The stripped strings keep the file's exact length and newlines, so a reported line is the
original line. `Pl/pl_act.cpp`'s header comment naming the banned `goto` dispatch is the incident this
exists for.

The backlog is a burn-down, not a gate: `--diff` fails only when a (rule, file) count rises, so touching a
unit with 300 `unk*` fields is allowed as long as the touch adds none. A **new** file starts from zero, so
its violations are all additions - new work is held to the rules from its first commit.

**Rule 7 is keyed on the file, not the directory.** Three exemption keys, narrowest last (`rule7_state`):

1. under `src/auto/` - a **temporary grandfather** for the legacy scaffolding units that already have
   bodies. The `auto/` bucket is retired and its units are being named and moved to their final homes,
   so this entry goes with the migration (`EXEMPT` below).
2. **no bodies yet** - a stub is a file-header comment and forward declarations; it has nothing to name,
   so rule 7 cannot apply. This is exactly `brief.text_has_bodies` (a brace outside the comments), so the
   pool and the gate never disagree about what a stub is.
3. **`rule 7 deferred: <reason>`** in a comment - the durable key, and the one that covers a unit that
   already has bodies at its final `src/<module>/<name>` home: a worker's landing commit registers the
   stub and writes the bodies together, so keys 1 and 2 cannot. It is per-unit and reviewable -
   `grep -rn "rule 7 deferred" src/` is the complete list - and key 3 defers the `fn_` half only: a bare
   `unk*` local is nameable from its context, so it still reports.

A key is checkable from the file alone, and a **finished** unit cannot hide behind keys 2 or 3 silently:
key 2 needs the absence of every body, and key 3 is one greppable line whose reason must name the evidence.
**Rules 1-6, 8 and 9 still apply under every key** (sized types, fields with offsets and context names, no
pointer arithmetic, no `goto`, no mangled spelling used as a callable identifier).
"""

from __future__ import annotations

import argparse
import collections
import json
import os
import re
import subprocess
import sys

# `brief.text_has_bodies` is the one definition of "a file with no bodies yet" (rule 7 key 2, `rule7_state`).
# Reuse it rather than keep a second brace scanner. Imported as a package module under land.py/promote.py,
# as a sibling script for `python tools/units/stylelint.py`.
try:
    from units import brief as _brief  # noqa: E402
except ImportError:  # `python tools/units/stylelint.py ...`
    import brief as _brief  # type: ignore  # noqa: E402

SRC = "src"
SUFFIXES = (".c", ".cpp", ".cp", ".cc", ".h", ".hpp", ".hh")

# Rule 7's path-keyed exemption table. This is now only a **temporary grandfather** for the legacy
# `src/auto/` units that already have bodies; the auto/ migration is naming and moving them, and this
# entry is removed with it. Do not add entries: the durable keys are per-file (`rule7_state`).
# One entry per (rule, path prefix); every other rule still applies under the prefix.
EXEMPT = [(7, "src/auto/", "temporary grandfather: legacy scaffolding with bodies, until the auto/ migration lands")]

# The per-file rule-7 keys, reported alongside `EXEMPT` because they are conditions, not path prefixes.
# `rule7_state` is the authority; these strings only keep the human/JSON output honest.
RULE7_NOTES = [
    ("a file with no bodies yet", "a stub has nothing to name"),
    ("a file declaring `rule 7 deferred: <reason>` in a comment", "per-unit and greppable; defers `fn_` only"),
]

RULE_NAMES = {
    1: "a shared type is defined once (in the owner's header)",
    2: "an extern lives with the TU that owns it (or include/unsplit/<module>.h)",
    3: "struct/class states its size (/* size: 0xNN */)",
    4: "field carries its offset (/* +0xNN */)",
    5: "no field left named unkNN (pad_0xNN / unused_0xNN are the exception)",
    6: "no pointer arithmetic to reach a field",
    7: "no fn_XXXXXXXX / bare unkNN identifier",
    8: "goto is forbidden",
    9: "no mangled spelling used as a callable identifier (call/declare the owner)",
}
# No rule is unchecked any more. Rule 2's remaining gap is dynamic (an unsplit address whose bracketing
# registered units name different modules), so it is reported from `Ownership.gaps`, not from here.
UNCHECKED: list[tuple[int, str]] = []


# --------------------------------------------------------------------------------------------------
# stripping: comments and literals blanked out, positions and newlines preserved
# --------------------------------------------------------------------------------------------------
def strip(text: str) -> tuple[str, str]:
    """Return `(code, comments)`, each the same length as `text` with newlines in place.

    `code` has every comment and string/char literal replaced by spaces; `comments` has the comment
    bodies preserved and everything else blanked. Both keep one character per input character, so an
    offset into either maps to the original line/column exactly.
    """
    n = len(text)
    code = list(text)
    comments = list(text)
    i = 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            comments[i] = "/"
            comments[i + 1] = "*"
            code[i] = code[i + 1] = " "
            i += 2
            while i < n:
                comments[i] = text[i]
                if text[i] != "\n":
                    code[i] = " "
                if text[i] == "*" and i + 1 < n and text[i + 1] == "/":
                    comments[i + 1] = "/"
                    code[i + 1] = " "
                    i += 2
                    break
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                comments[i] = text[i]
                code[i] = " "
                i += 1
            continue
        if c in "\"'":
            quote = c
            code[i] = comments[i] = " "
            i += 1
            while i < n:
                if text[i] == "\\" and i + 1 < n:
                    code[i] = code[i + 1] = comments[i] = comments[i + 1] = " "
                    i += 2
                    continue
                if text[i] == "\n":
                    break  # unterminated literal / line continuation: do not swallow the newline
                code[i] = comments[i] = " "
                if text[i] == quote:
                    i += 1
                    break
                i += 1
            continue
        i += 1
    return "".join(code), "".join(comments)


class Source:
    """One file's text plus its two stripped views and a line index."""

    def __init__(self, path: str, rel: str, text: str):
        self.path = path
        self.rel = rel
        self.text = text
        self.code, self.comments = strip(text)
        self._starts = [0]
        for i, ch in enumerate(text):
            if ch == "\n":
                self._starts.append(i + 1)

    def line_of(self, pos: int) -> int:
        lo, hi = 0, len(self._starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if self._starts[mid] <= pos:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1

    def line_text(self, line: int) -> str:
        start = self._starts[line - 1]
        end = self.text.find("\n", start)
        return self.text[start:] if end < 0 else self.text[start:end]

    def span_lines(self, start: int, end: int) -> tuple[int, int]:
        return self.line_of(start), self.line_of(max(start, end - 1))


# --------------------------------------------------------------------------------------------------
# struct/class definitions and their fields
# --------------------------------------------------------------------------------------------------
STRUCT_RE = re.compile(r"\b(?:struct|class)\s*([A-Za-z_]\w*)?\s*\{")
SIZE_RE = re.compile(r"size\s*:\s*0x[0-9A-Fa-f]+")
OFFSET_RE = re.compile(r"/\*\s*\+?0x[0-9A-Fa-f]+")
UNK_FIELD_RE = re.compile(r"^unk\w*$")


def match_brace(code: str, open_pos: int) -> int:
    """Index of the `}` matching the `{` at `open_pos`, or -1."""
    depth = 0
    for i in range(open_pos, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def struct_defs(src: Source) -> list[dict]:
    """Every `struct`/`class` definition in the file, with its name, line range and body range."""
    out = []
    for m in STRUCT_RE.finditer(src.code):
        open_pos = m.end() - 1
        close_pos = match_brace(src.code, open_pos)
        if close_pos < 0:
            continue
        name = m.group(1)
        if not name:  # `typedef struct { ... } Name;`
            tail = re.match(r"\s*([A-Za-z_]\w*)", src.code[close_pos + 1:])
            name = tail.group(1) if tail else "<anonymous>"
        out.append({
            "name": name,
            "start": m.start(),
            "open": open_pos,
            "close": close_pos,
            "line": src.line_of(m.start()),
            "end_line": src.line_of(close_pos),
        })
    return out


def iter_fields(code: str, open_pos: int, close_pos: int) -> list[tuple[int, int]]:
    """`(start, end)` of every field declaration chunk at the body's top level."""
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


def field_name(chunk: str) -> str | None:
    """The declared name of a field chunk, or None when the chunk is not a plain field."""
    t = chunk.strip().rstrip(";").strip()
    if not t:
        return None
    if "{" in t:  # a nested type definition: its own struct_def entry covers it
        return None
    if re.fullmatch(r"(public|private|protected|typedef)", t):
        return None
    m = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)", t)  # function pointer: void (*name)(void)
    if m:
        return m.group(1)
    if "(" in t:  # a method declaration, not a field
        return None
    t = t.split(":")[0]  # drop a bitfield width
    t = re.sub(r"\[[^\]]*\]", "", t)  # drop array dimensions
    ids = re.findall(r"[A-Za-z_]\w*", t)
    return ids[-1] if ids else None


def struct_has_size(src: Source, defs: list[dict]) -> set[int]:
    """Indices into `defs` that carry a `size: 0xNN` comment near their definition.

    Each annotation is assigned to its nearest definition (the following one wins a tie) so one comment
    cannot certify two adjacent types.
    """
    anns = [src.line_of(m.start()) for m in SIZE_RE.finditer(src.comments)]
    ok = set()
    for a in anns:
        best, best_key = None, None
        for idx, d in enumerate(defs):
            if d["line"] <= a <= d["end_line"]:
                dist = 0
            else:
                dist = min(abs(a - d["line"]), abs(a - d["end_line"]))
            key = (dist, abs(a - d["line"]))
            if best_key is None or key < best_key:
                best, best_key = idx, key
        if best is not None and best_key[0] <= 4:
            ok.add(best)
    return ok


# --------------------------------------------------------------------------------------------------
# rule 1: a shared type is defined once (cross-file)
# --------------------------------------------------------------------------------------------------
# `union` is included (the rule names struct/class/union), and an anonymous `typedef struct { ... } Foo;`
# takes `Foo` from the tail exactly as `struct_defs` does. A forward declaration (`struct Foo;`) has no
# body and is not a definition, so it never counts.
RULE1_TYPE_RE = re.compile(r"\b(?:struct|class|union)\s+([A-Za-z_]\w*)?\s*(?::[^{;]*)?\{")


def type_defs(src: Source) -> list[tuple[str, int]]:
    """`(name, line)` for every named struct/class/union definition in `src`, first definition per name."""
    out = []
    seen: set[str] = set()
    for m in RULE1_TYPE_RE.finditer(src.code):
        open_pos = m.end() - 1
        name = m.group(1)
        if not name:  # `typedef struct { ... } Foo;`
            close_pos = match_brace(src.code, open_pos)
            if close_pos < 0:
                continue
            tail = re.match(r"\s*([A-Za-z_]\w*)", src.code[close_pos + 1:])
            name = tail.group(1) if tail else None
        if not name or name in seen:
            continue
        seen.add(name)
        out.append((name, src.line_of(m.start())))
    return out


def rule1_findings(sources: list[Source]) -> list[dict]:
    """One finding per (type, extra file) for a type defined in more than one file.

    The lexicographically first file is the owner; every later file that defines the same name gets a
    finding that names both files, so a type in N files yields N-1 findings - the count is the number of
    extra definitions to delete, not the number of files involved. Reporting per extra file (rather than
    one finding per type) keeps the budget actionable and lets a batch that only adds a duplicate be
    refused on the file it touched.
    """
    where: dict[str, dict[str, int]] = {}
    by_rel = {src.rel: src for src in sources}
    for src in sources:
        for name, line in type_defs(src):
            where.setdefault(name, {}).setdefault(src.rel, line)
    out: list[dict] = []
    for name in sorted(where):
        files = sorted(where[name])
        if len(files) < 2:
            continue
        owner = files[0]
        for extra in files[1:]:
            out.append(_finding(by_rel[extra], 1, where[name][extra],
                                "type `%s` is defined in `%s` and again in `%s` - one definition, in the "
                                "owner's header" % (name, owner, extra)))
    return out


# --------------------------------------------------------------------------------------------------
# rule 2: an extern lives with the TU that owns it (symbols.txt + splits.txt)
# --------------------------------------------------------------------------------------------------
def module_name(unit: str) -> str:
    """The module of a registered unit: its directory, or its stem for a root-level file."""
    unit = unit.replace("\\", "/")
    d = unit.rsplit("/", 1)[0] if "/" in unit else ""
    return d or os.path.splitext(unit)[0]


class Ownership:
    """`symbol -> owning registered unit`, from `symbols.txt` + `splits.txt`.

    Both files are parsed programmatically and never printed (`symbols.txt` is 4.5 MB); the result is
    cached per mtime by `load_ownership`, as `brief.py` caches the map. `gaps`, `unsplit_modules` and
    `foreign_units` accumulate what the lookup can and cannot judge, so the report states the classes it
    leaves alone instead of guessing them.
    """

    def __init__(self, symbols: dict, ranges: dict):
        self.symbols = symbols
        self.ranges = {s: sorted(v) for s, v in ranges.items()}
        self.gaps: "collections.Counter" = collections.Counter()
        self.unsplit_modules: "collections.Counter" = collections.Counter()
        self.unsplit_symbols: dict[str, set] = {}
        self.foreign_units: "collections.Counter" = collections.Counter()

    def resolve(self, name: str) -> "dict | None":
        """`None` when the name is not in the map; else a dict with `kind` owned/unsplit/dup."""
        entries = self.symbols.get(name)
        if not entries:
            return None
        if len(entries) != 1:
            return {"kind": "dup"}
        section, address, type_ = entries[0]
        for start, end, unit in self.ranges.get(section, []):
            if start <= address < end:
                return {"kind": "owned", "unit": unit, "section": section,
                        "address": address, "type": type_}
        return {"kind": "unsplit", "section": section, "address": address, "type": type_,
                "module": self.module(section, address)}

    def module(self, section: str, address: int) -> "str | None":
        """The module of the registered units bracketing `address` in `section`, or None.

        The registered bands interleave across modules - a `sound` unit sits inside the `ef` band - so
        when the nearest unit below and the nearest above disagree there is no sound answer and this
        returns None. The caller leaves those sites a documented gap rather than name a wrong module.
        """
        rows = self.ranges.get(section)
        if not rows:
            return None
        lo = hi = None
        for start, _end, unit in rows:
            if start <= address:
                lo = unit
            elif hi is None:
                hi = unit
        if lo is None or hi is None:
            return None
        a, b = module_name(lo), module_name(hi)
        return a if a == b else None


def _parse_symbols(path: str) -> dict:
    pattern = re.compile(r"^(\S+)\s*=\s*([.\w]+):(0x[0-9A-Fa-f]+);\s*//\s*type:(\w+)")
    symbols: dict = {}
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = pattern.match(line)
            if m:
                symbols.setdefault(m.group(1), []).append(
                    (m.group(2), int(m.group(3), 16), m.group(4)))
    return symbols


def _parse_splits(path: str) -> dict:
    ranges: dict = {}
    current = None
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if line.startswith("Sections:"):
                continue
            m = re.match(r"^([^\s:][^:]*):\s*$", line)
            if m and not line.startswith(("\t", " ")):
                current = m.group(1)
                continue
            m = re.match(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
            if m and current:
                ranges.setdefault(m.group(1), []).append(
                    (int(m.group(2), 16), int(m.group(3), 16), current))
    return ranges


_OWNERSHIP_CACHE: dict = {}


def load_ownership(root: str) -> "Ownership | None":
    """Parse `symbols.txt` + `splits.txt` once per mtime; None when either is absent.

    A scratch lint with no map simply has rule 2 unchecked rather than dying. The cache is keyed on both
    MTimes, so a rename or a re-split invalidates it within one process (a pool brief, a batch check).
    """
    sym = os.path.join(root, "config", "RMHE08", "symbols.txt")
    spl = os.path.join(root, "config", "RMHE08", "splits.txt")
    if not os.path.exists(sym) or not os.path.exists(spl):
        return None
    key = (sym, spl, os.path.getmtime(sym), os.path.getmtime(spl))
    if key not in _OWNERSHIP_CACHE:
        _OWNERSHIP_CACHE[key] = Ownership(_parse_symbols(sym), _parse_splits(spl))
    return _OWNERSHIP_CACHE[key]


EXTERN_RE = re.compile(r"\bextern\b")


def _declared_name(segment: str) -> "str | None":
    """The identifier an `extern` declaration introduces (function, function pointer, or variable).

    The first `(` is always at depth 0, so what follows it decides the shape: `(*name)` is a function
    pointer variable, `(*name(` is a function returning a function pointer, anything else is a function
    whose name is the last identifier before the `(`. A function-pointer *parameter* is nested, so it is
    never reached from the first `(`.
    """
    i = segment.find("(")
    if i >= 0:
        tail = segment[i:]
        m = re.match(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", tail)
        if m:
            return m.group(1)
        m = re.match(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\(", tail)
        if m:
            return m.group(1)
        ids = re.findall(r"[A-Za-z_]\w*", segment[:i])
        return ids[-1] if ids else None
    segment = re.sub(r"\[[^\]]*\]", " ", segment.split("=")[0])
    ids = re.findall(r"[A-Za-z_]\w*", segment)
    return ids[-1] if ids else None


def extern_declarations(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every `extern` function/variable declaration in the file.

    A definition (`extern "C" void f(void) { ... }`) is skipped: it defines the symbol, so the file owns
    it by construction. A linkage block (`extern "C" {`) has no name and is skipped too.
    """
    out = []
    for m in EXTERN_RE.finditer(src.code):
        j = m.end()
        while j < len(src.code) and src.code[j] not in ";{}":
            j += 1
        if j < len(src.code) and src.code[j] == "{":
            continue  # a definition, not a declaration
        name = _declared_name(src.code[m.end():j])
        if name is None:
            continue
        out.append((name, m.start(), src.line_of(m.start())))
    return out


def _owns(rel: str, unit: str) -> bool:
    """Whether the `src/` file `rel` is the registered unit `unit` (its source file or its header)."""
    rel = rel.replace("\\", "/")
    if rel == SRC + "/" + unit:
        return True
    stem = os.path.splitext(unit)[0]
    return any(rel == SRC + "/" + stem + ext for ext in (".h", ".hpp", ".hh"))


def rule2_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Every `extern` declaration the file makes for a symbol it does not own.

    Three outcomes: `owned` by this file (no finding), owned by another registered unit (move the
    declaration to that unit's header and `#include` it), or `unsplit` (move it to
    `include/unsplit/<module>.h`). A name not in the map, a name with duplicate map rows, and an unsplit
    address whose bracketing units name different modules are left as counted gaps rather than guessed.
    """
    out = []
    for name, _pos, line in extern_declarations(src):
        r = ownership.resolve(name)
        if r is None:
            ownership.gaps["not in symbols.txt"] += 1
            continue
        if r["kind"] == "dup":
            ownership.gaps["duplicate symbol name in the map"] += 1
            continue
        if r["kind"] == "owned":
            if _owns(src.rel, r["unit"]):
                continue
            ownership.foreign_units[r["unit"]] += 1
            out.append(_finding(src, 2, line,
                                "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                "#include it" % (name, r["unit"])))
            continue
        module = r["module"]
        if module is None:
            ownership.gaps["unsplit: address band interleaves modules"] += 1
            continue
        ownership.unsplit_modules[module] += 1
        ownership.unsplit_symbols.setdefault(module, set()).add(name)
        out.append(_finding(src, 2, line,
                            "`%s` has no registered owner - declare it in `include/unsplit/%s.h`"
                            % (name, module)))
    return out


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
RULE7_FN_RE = re.compile(r"\bfn_[0-9A-Fa-f]{8}\b")
RULE7_UNK_RE = re.compile(r"\bunk\w*\b")
# Rule 7 key 3: the per-unit `rule 7 deferred: <reason>` declaration, matched only against the comment
# view (see `rule7_deferral`). `[ \t]*` rather than `\s*` keeps the declaration and its non-empty reason
# on one line, so `rule 7 deferred:` at the end of a comment cannot borrow the next line's first token,
# and a normal comment's closing `*/` cannot count as the reason.
RULE7_DEFER_RE = re.compile(r"rule[ \t]*7[ \t]+deferred[ \t]*:[ \t]*\S")
_COMMENT_DELIM_RE = re.compile(r"/\*|\*/|//")
RULE8_RE = re.compile(r"\bgoto\b")

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
_DECL_KEYWORDS = {"return", "if", "while", "for", "switch", "case", "else", "do",
                  "sizeof", "break", "continue", "goto"}


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

# rule 6's own exception: a raw byte offset is allowed where no field is being named - a `memset`/`memcpy`
# range is the common case, so a cast-plus-offset that is an argument to one of these is not reported.
MEM_FUNCS = {"memset", "memcpy", "memmove", "memcmp", "bzero", "__memcpy", "__fill_mem"}


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


def _finding(src: Source, rule: int, line: int, detail: str) -> dict:
    return {"rule": rule, "file": src.rel, "line": line,
            "text": src.line_text(line).strip()[:160], "detail": detail}


def lint_source(src: Source, ownership: "Ownership | None" = None) -> list[dict]:
    """All section 6.5 findings for one file, in rule then line order.

    `ownership` carries the symbols.txt + splits.txt index for rule 2; when it is None (the map is
    absent, or a caller that only wants the source-local rules), rule 2 is not reported for `src`.
    """
    out: list[dict] = []
    defs = struct_defs(src)
    sized = struct_has_size(src, defs)

    for idx, d in enumerate(defs):
        if idx not in sized:
            out.append(_finding(src, 3, d["line"], "type `%s` has no `/* size: 0xNN */`" % d["name"]))

    field_spans: list[tuple[int, int]] = []
    for d in defs:
        for start, end in iter_fields(src.code, d["open"], d["close"]):
            chunk = src.code[start:end]
            name = field_name(chunk)
            if name is None:
                continue
            name_pos = src.code.find(name, start, end)
            line = src.line_of(name_pos if name_pos >= 0 else start)
            first = start + (len(chunk) - len(chunk.lstrip()))
            first_line = src.line_of(first)
            if name_pos >= 0:
                field_spans.append((name_pos, name_pos + len(name)))
            has_offset = any(
                OFFSET_RE.search(src.comments[src._starts[l - 1]: (src._starts[l] if l < len(src._starts) else len(src.comments))])
                for l in range(first_line, max(first_line, line) + 1)
            )
            if not has_offset:
                out.append(_finding(src, 4, line, "field `%s` has no offset annotation" % name))
            if UNK_FIELD_RE.match(name):
                out.append(_finding(src, 5, line, "field `%s` needs a context name or pad_0xNN" % name))

    for m in RULE6_RE.finditer(src.code):
        if enclosing_call(src.code, m.start()) in MEM_FUNCS:
            continue
        out.append(_finding(src, 6, src.line_of(m.start()), "pointer arithmetic: `%s`" % m.group(0).strip()))

    fn7_enforced, unk7_enforced = rule7_state(src)
    if fn7_enforced or unk7_enforced:
        def in_field(pos: int) -> bool:
            return any(a <= pos < b for a, b in field_spans)

        if fn7_enforced:
            for m in RULE7_FN_RE.finditer(src.code):
                out.append(_finding(src, 7, src.line_of(m.start()), "auto-generated name `%s`" % m.group(0)))
        if unk7_enforced:
            for m in RULE7_UNK_RE.finditer(src.code):
                if not in_field(m.start()):
                    out.append(_finding(src, 7, src.line_of(m.start()), "bare `%s` identifier" % m.group(0)))

    for m in RULE8_RE.finditer(src.code):
        out.append(_finding(src, 8, src.line_of(m.start()), "goto statement"))

    # Rule 9: the mangled spelling is forbidden as the callable identifier. Both halves are reported -
    # a call (`foo__Fv()`) and a declaration (`extern void foo__Fv(void);`) - because the declaration is
    # where playbook row 50's double-mangle is born, and the fix differs only in emphasis: a call is
    # rewritten `obj->method(args)` / `ns::function(args)`, a declaration to the owner's real declaration.
    for m in RULE9_CALL_RE.finditer(src.code):
        name = m.group(1)
        if not RULE9_MANGLED_RE.match(name):
            continue
        line = src.line_of(m.start())
        if looks_like_declaration(src.code, m.start()):
            out.append(_finding(src, 9, line,
                                "mangled name `%s` is declared - declare its owner and include it" % name))
        else:
            out.append(_finding(src, 9, line,
                                "mangled name `%s` is called - call it through its owner" % name))

    if ownership is not None:
        out.extend(rule2_findings(src, ownership))

    out.sort(key=lambda f: (f["rule"], f["line"]))
    return out


# --------------------------------------------------------------------------------------------------
# tree walking / budget
# --------------------------------------------------------------------------------------------------
def source_files(root: str) -> list[str]:
    out = []
    base = os.path.join(root, SRC)
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
        for name in sorted(filenames):
            if name.endswith(SUFFIXES):
                out.append(os.path.join(dirpath, name))
    return out


def rel_of(root: str, path: str) -> str:
    return os.path.relpath(path, root).replace(os.sep, "/")


def read_text(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        return fh.read()


def all_sources(root: str) -> list["Source"]:
    """Every `src/` file as a `Source`, in path order."""
    return [Source(path, rel_of(root, path), read_text(path)) for path in source_files(root)]


def lint_tree(root: str, paths: list[str] | None = None,
              ownership: "Ownership | None" = None) -> list[dict]:
    """Per-file findings (rules 2-9) for `paths`, or for the whole tree when `paths` is None.

    Rule 1 is cross-file and is *not* included here: with `paths` limited to a batch's changed files it
    would miss a duplicate whose partner is untouched. Use `lint_all` for the whole rule set. `ownership`
    is the rule-2 index; it is loaded from `root` when not passed, and the map being absent simply leaves
    rule 2 unreported.
    """
    if ownership is None:
        ownership = load_ownership(root)
    out = []
    sources = all_sources(root) if paths is None else [
        Source(path, rel_of(root, path), read_text(path)) for path in paths]
    for src in sources:
        out.extend(lint_source(src, ownership))
    return out


def lint_all(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Every checked finding: the per-file rules plus cross-file rule 1, in rule/file/line order."""
    if ownership is None:
        ownership = load_ownership(root)
    sources = all_sources(root)
    out = []
    for src in sources:
        out.extend(lint_source(src, ownership))
    out.extend(rule1_findings(sources))
    out.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    return out


def budget(findings: list[dict]) -> dict:
    """Per-unit rule counts plus the totals, both keyed by unit (path relative to the repo root)."""
    per: dict[str, dict] = {}
    for f in findings:
        row = per.setdefault(f["file"], {"file": f["file"], "total": 0,
                                         "rules": {str(r): 0 for r in RULE_NAMES}})
        row["rules"][str(f["rule"])] += 1
        row["total"] += 1
    totals = {str(r): sum(row["rules"][str(r)] for row in per.values()) for r in RULE_NAMES}
    return {"units": [per[k] for k in sorted(per)], "totals": totals,
            "total": sum(totals.values()), "findings": len(findings), "unique": unique_names(findings)}


def unique_names(findings: list[dict]) -> dict:
    """Distinct names behind the name-based rules - the burn-down number a rename closes in one edit."""
    def names(rule: int, prefix: str) -> set:
        return {m.group(1) for f in findings if f["rule"] == rule and f["detail"].startswith(prefix)
                for m in [re.search(r"`([^`]+)`", f["detail"])] if m}
    return {"fn_names": len(names(7, "auto")), "unk_identifiers": len(names(7, "bare")),
            "unk_fields": len(names(5, "")), "types": len(names(3, "")),
            "shared_types": len(names(1, "")), "extern_symbols": len(names(2, "")),
            "mangled_names": len(names(9, ""))}


def rule7_deferral(comments: str) -> bool:
    """Whether a file's comment view carries a `rule 7 deferred: <reason>` declaration.

    `comments` is `Source.comments`, which keeps comment bodies and blanks string/char literals, so a
    declaration inside a literal cannot match. Comment delimiters are blanked before the search: an empty
    `rule 7 deferred:` followed by a `*/` must not borrow the `*` as its reason.
    """
    return bool(RULE7_DEFER_RE.search(_COMMENT_DELIM_RE.sub(" ", comments)))


def rule7_state(src: "Source") -> tuple[bool, bool]:
    """`(fn_enforced, unk_enforced)` for rule 7 in `src`, per the three keys.

    1. under `src/auto/` (temporary grandfather, `EXEMPT`): the whole rule is off;
    2. no bodies yet (`brief.text_has_bodies`): the whole rule is off;
    3. a `rule 7 deferred: <reason>` comment declaration: the `fn_` half is off, the `unk` half stays.
    """
    rel = src.rel.replace("\\", "/")
    if any(r == 7 and rel.startswith(prefix) for r, prefix, _why in EXEMPT):
        return (False, False)
    if not _brief.text_has_bodies(src.text):
        return (False, False)
    if rule7_deferral(src.comments):
        return (False, True)
    return (True, True)


def rule_enforced(rule: int, rel: str, src: "Source | None" = None) -> bool:
    """Whether `rule` is enforced at all for `rel` (rule 7 may be half-deferred, see `rule7_state`).

    With `src` this consults all three rule-7 keys; without it only the path-keyed `EXEMPT` table can be
    consulted. Rules other than 7 are enforced everywhere.
    """
    if rule == 7 and src is not None:
        return any(rule7_state(src))
    norm = rel.replace("\\", "/")
    return not any(rule == r and norm.startswith(prefix) for r, prefix, _why in EXEMPT)


def exemptions() -> list[dict]:
    """The `EXEMPT` table plus the per-file rule-7 keys, in the shape the JSON output reports it."""
    out = [{"rule": r, "prefix": p, "why": w} for r, p, w in EXEMPT]
    out += [{"rule": 7, "prefix": None, "condition": cond, "why": why} for cond, why in RULE7_NOTES]
    return out


def rule_counts(findings: list[dict]) -> dict[tuple[int, str], int]:
    out: dict[tuple[int, str], int] = {}
    for f in findings:
        out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def diff_deltas(before: dict[tuple[int, str], int], after: dict[tuple[int, str], int]) -> list[dict]:
    """The (rule, file) pairs whose count rose, with the delta. Pure function - the `--diff` core."""
    added = []
    for key in sorted(set(before) | set(after)):
        delta = after.get(key, 0) - before.get(key, 0)
        if delta > 0:
            added.append({"rule": key[0], "file": key[1], "added": delta,
                          "before": before.get(key, 0), "after": after.get(key, 0)})
    return added


def git(root: str, *args: str) -> str:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), (p.stderr or "").strip()))
    return p.stdout


def git_bytes(root: str, *args: str) -> bytes:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), p.stderr.decode("utf-8", "replace").strip()))
    return p.stdout


def changed_src_files(root: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_ref, path_now)` for every `src/` file the tree changed against `ref`.

    A rename is one entry carrying both names, so the file's violations are compared against its old
    copy rather than counting as new.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", ref, "--", SRC).splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        status, paths = parts[0], parts[1:]
        after = paths[-1]
        if status.startswith("R"):
            before = paths[0]
        elif status.startswith("A"):
            before = None  # added by this batch: every violation in it is an addition
        else:
            before = after
        if after.endswith(SUFFIXES):
            out.append((before, after))
    return out


def findings_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                    ownership: "Ownership | None" = None) -> dict[tuple[int, str], int]:
    """Rule counts for the ref's copy of each file, keyed by the file's path *now*.

    A file absent at the ref (`before is None`) counts as zero: it is new, so every violation in it is an
    addition. Keying a rename by its new path keeps the two sides comparable. `ownership` is the rule-2
    index (the working tree's, which is what the batch lands against).
    """
    findings = []
    for before, after in pairs:
        if before is None:
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, before)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(before, after, text), ownership))
    return rule_counts(findings)


def merge_counts(*counts: dict) -> dict:
    """Sum several `rule_counts` dicts (each `{(rule, file): n}`) into one."""
    out: dict = {}
    for c in counts:
        for key, n in c.items():
            out[key] = out.get(key, 0) + n
    return out


def src_paths_at_ref(root: str, ref: str) -> list[str]:
    """Every `src/` path (relative, slash-separated) that exists at `ref`."""
    return [line for line in git(root, "ls-tree", "-r", "--name-only", ref, "--", SRC).splitlines()
            if line.endswith(SUFFIXES)]


def rule1_counts_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> dict:
    """Rule-1 counts for the ref's whole `src/` tree, keyed by the path each file has now.

    Rule 1 is cross-file, so unlike `findings_at_ref` (which lints only the changed files) it has to see
    every file: a batch can duplicate a type that already lives in an untouched file. A rename is keyed by
    its new path so the two sides stay comparable.
    """
    rename = {before: after for before, after in pairs if before and before != after}
    sources = []
    for path in src_paths_at_ref(root, ref):
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, path)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        sources.append(Source(path, rename.get(path, path), text))
    return rule_counts(rule1_findings(sources))


# --------------------------------------------------------------------------------------------------
# reporting
# --------------------------------------------------------------------------------------------------
def print_rule2_report(ownership: "Ownership | None") -> None:
    """The rule-2 backlog shape: owners needing a header, the unsplit modules, and the counted gaps."""
    if ownership is None:
        print("rule 2: unchecked (config/RMHE08/symbols.txt or splits.txt is absent)")
        return
    if ownership.foreign_units:
        print("rule 2: %d declaration site(s) into %d other owner unit(s); top: %s"
              % (sum(ownership.foreign_units.values()), len(ownership.foreign_units),
                 ", ".join("%s (%d)" % (u, n) for u, n in ownership.foreign_units.most_common(5))))
    if ownership.unsplit_modules:
        print("rule 2: include/unsplit/*.h would carry %d declaration site(s) for %d symbol(s): %s"
              % (sum(ownership.unsplit_modules.values()),
                 sum(len(v) for v in ownership.unsplit_symbols.values()),
                 ", ".join("%s %d site(s)/%d symbol(s)"
                           % (m, n, len(ownership.unsplit_symbols.get(m, ())))
                           for m, n in sorted(ownership.unsplit_modules.items()))))
    for reason, n in sorted(ownership.gaps.items()):
        print("rule 2 gap: %d declaration site(s) - %s (documented, not guessed)" % (n, reason))
    if not ownership.foreign_units and not ownership.unsplit_modules and not ownership.gaps:
        print("rule 2: every extern declaration was judged")


def print_budget(findings: list[dict], ownership: "Ownership | None" = None) -> None:
    b = budget(findings)
    width = max([len(row["file"]) for row in b["units"]] + [len("TOTAL")])
    head = "".join("  r%d" % r for r in RULE_NAMES)
    print("%-*s  %4s%s" % (width, "unit", "tot", head))
    for row in b["units"]:
        print("%-*s  %4d%s" % (width, row["file"], row["total"],
                               "".join("  %2d" % row["rules"][str(r)] for r in RULE_NAMES)))
    print("%-*s  %4d%s" % (width, "TOTAL", b["total"],
                           "".join("  %2d" % b["totals"][str(r)] for r in RULE_NAMES)))
    print("")
    u = unique_names(findings)
    print("distinct names: rule 1 %d shared type(s), rule 2 %d extern symbol(s), rule 3 %d type(s), "
          "rule 5 %d field(s), rule 7 %d fn_* + %d unk identifier(s), rule 9 %d mangled name(s)"
          % (u["shared_types"], u["extern_symbols"], u["types"], u["unk_fields"], u["fn_names"],
             u["unk_identifiers"], u["mangled_names"]))
    print("%d finding(s) over %d unit(s), %d file(s) with findings"
          % (b["findings"], len(b["units"]), len(source_files_of(findings))))
    print_rule2_report(ownership)
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))


def source_files_of(findings: list[dict]) -> set[str]:
    return {f["file"] for f in findings}


def print_findings(findings: list[dict]) -> None:
    for f in findings:
        print("%s:%d: rule %d: %s [%s]" % (f["file"], f["line"], f["rule"], f["detail"], f["text"]))
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))


# --------------------------------------------------------------------------------------------------
# selftest
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def rules_of(text: str, rel: str = "x.c", ownership=None) -> list[tuple[int, int]]:
        return [(f["rule"], f["line"]) for f in lint_source(Source("x.c", rel, text), ownership)]

    def lines_of(text: str, rule: int, rel: str = "x.c", ownership=None) -> list[int]:
        return [f["line"] for f in lint_source(Source("x.c", rel, text), ownership) if f["rule"] == rule]

    # --- stripping --------------------------------------------------------------------------------
    code, comm = strip("a /* goto */ b\nc // goto\nd \"goto\" 'x'\n")
    check("strip: block comment blanked", "goto" in code, False)
    check("strip: line comment blanked", code.count("goto"), 0)
    check("strip: string blanked", code.count("goto"), 0)
    check("strip: comment text kept", comm.count("goto"), 2)
    check("strip: length preserved", len(code), len(comm))
    check("strip: newlines preserved", code.count("\n"), 3)
    multi = "/* a\ngoto\nb */\nx\ngoto done;\n"
    check("strip: multi-line comment keeps line numbers", [f["line"] for f in lint_source(Source("x.c", "x.c", multi)) if f["rule"] == 8], [5])
    esc = 'char* s = "a \\" goto b";\ngoto x;\n'
    check("strip: escaped quote in a literal", lines_of(esc, 8), [2])
    check("strip: unterminated line literal stops at newline",
          lines_of('char c = \'a;\ngoto x;\n', 8), [2])
    check("strip: comment body has the annotation", bool(SIZE_RE.search(strip("/* size: 0x10 */\n")[1])), True)
    check("strip: annotation inside a string is not seen", bool(SIZE_RE.search(strip('"size: 0x10"\n')[1])), False)

    # --- rule 3: size -----------------------------------------------------------------------------
    check("rule3: annotated before the definition", lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 3), [])
    check("rule3: annotated after the closing brace", lines_of("struct A {\n    u32 x;\n};\n/* size: 0x8 */\n", 3), [])
    check("rule3: no annotation is a violation", lines_of("struct A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: anonymous typedef gets the name after the brace",
          lines_of("typedef struct {\n    u32 x;\n} A;\n", 3), [1])
    check("rule3: annotation too far away", lines_of("/* size: 0x8 */\n\n\n\n\n\nstruct A {\n    u32 x;\n};\n", 3), [7])
    check("rule3: a forward declaration is not a definition", lines_of("struct A;\n", 3), [])
    check("rule3: a use in a parameter list is not a definition", lines_of("void f(struct A* a);\n", 3), [])
    check("rule3: two types, one annotation - only the near one is certified",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\nstruct B {\n    u32 y;\n};\n", 3), [5])
    check("rule3: class counts too", lines_of("class A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: annotation in a string does not certify",
          lines_of('const char* s = "size: 0x8";\nstruct A {\n    u32 x;\n};\n', 3), [2])

    # --- rule 4/5: fields -------------------------------------------------------------------------
    check("rule4: canonical +0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: the existing 0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* 0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: trailing annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x; /* 0x00 - counter */\n};\n", 4), [])
    check("rule4: missing offset is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4), [3])
    check("rule4: two fields, one annotated",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n    u32 y;\n};\n", 4), [4])
    check("rule4: function pointer field is a field",
          lines_of("/* size: 0x8 */\nstruct A {\n    void (*cb)(void);\n};\n", 4), [3])
    check("rule5: unk field is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 5), [3])
    check("rule5: pad_/unused_ pass",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 pad_0x00[4];\n    /* +0x04 */ u8 unused_0x04[4];\n};\n", 5), [])
    check("rule5: a named field passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id;\n};\n", 5), [])
    check("rule5: unk in a comment does not count",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id; /* was unk00 */\n};\n", 5), [])
    check("rule5: an array field name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 unk00[4];\n};\n", 5), [3])
    check("rule5: a bitfield name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00 : 3;\n};\n", 5), [3])
    check("rule4: only the top-level body counts, not a nested type",
          lines_of("/* size: 0x8 */\nstruct A {\n    struct B { /* +0x00 */ u32 z; } b;\n    /* +0x00 */ u32 x;\n};\n", 4), [])

    # --- rule 6: pointer arithmetic ---------------------------------------------------------------
    check("rule6: the classic field poke",
          lines_of("void f(u8* self) {\n    *(u32*)((u8*)self + 0x1C) = 1;\n}\n", 6), [2])
    check("rule6: (char*)obj + 0x10",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((char*)obj + 0x10);\n}\n", 6), [2])
    check("rule6: decimal offset",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((u8*)obj + 4);\n}\n", 6), [2])
    check("rule6: a member access is clean", lines_of("void f(A* self) {\n    self->field = 1;\n}\n", 6), [])
    check("rule6: memset is clean", lines_of("void f(void* p) {\n    memset(p, 0, 0x10);\n}\n", 6), [])
    check("rule6: offsetof is clean", lines_of("u32 n = offsetof(A, x);\n", 6), [])
    check("rule6: a cast with no offset is clean", lines_of("void f(void* p) {\n    u32 v = *(u32*)p;\n}\n", 6), [])
    check("rule6: memset's allowed byte range is clean",
          lines_of("void f(u8* p) {\n    memset((u8*)p + 4, 0, 8);\n}\n", 6), [])
    check("rule6: memcpy's allowed byte range is clean",
          lines_of("void f(u8* p, u8* q) {\n    memcpy(p, (u8*)q + 8, 4);\n}\n", 6), [])
    check("rule6: a cast offset passed to another callee is not allowed",
          lines_of("void f(u8* p) {\n    foo((u8*)p + 4);\n}\n", 6), [2])
    check("rule6: the memset exception does not cover a dereference",
          lines_of("void f(u8* p) {\n    memset(p, 0, *(u32*)((u8*)p + 4));\n}\n", 6), [2])
    check("rule6: inside a comment it is clean",
          lines_of("/* written as *(s16*)((u8*)self + 0x1C) */\nvoid f(void) {}\n", 6), [])
    check("rule6: two sites on one line are two findings",
          lines_of("void f(u8* p) {\n    a = *(u32*)((u8*)p + 4); b = *(u32*)((u8*)p + 8);\n}\n", 6), [2, 2])

    # --- rule 7: names ----------------------------------------------------------------------------
    check("rule7: fn_ name is a violation", lines_of("void fn_80040598(void) {}\n", 7), [1])
    check("rule7: a named function is clean", lines_of("void Pl_Skill_ck(void) {}\n", 7), [])
    check("rule7: bare unk local is a violation", lines_of("void f(void) {\n    u32 unk4 = 0;\n}\n", 7), [2])
    check("rule7: a field's unk is rule 5's, not rule 7's",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 7), [])
    check("rule7: unk in a comment does not count", lines_of("/* unk4 fn_80040598 */\nvoid f(void) {}\n", 7), [])
    check("rule7: unk in a string does not count", lines_of('const char* s = "unk4";\n', 7), [])
    check("rule7: a name that merely contains unk is clean", lines_of("void f(void) {\n    u32 junk = 0;\n}\n", 7), [])
    check("rule7: fn_ with the wrong digit count is clean", lines_of("void fn_1234(void) {}\n", 7), [])

    # --- rule 7 exemption under src/auto/ (docs/plan.md, "The breadth blocker") --------------------
    auto = "src/auto/802B2978_fn_802B2978.c"
    check("rule7 exempt: an auto file's own fn_ name is clean",
          lines_of("void fn_802B2978(void) {}\n", 7, auto), [])
    check("rule7 exempt: an auto body calling fn_ is clean",
          lines_of("void fn_802B2978(void) {\n    fn_80040598();\n}\n", 7, auto), [])
    check("rule7 exempt: a bare unk local is clean under src/auto/ too",
          lines_of("void fn_802B2978(void) {\n    u32 unk4 = 0;\n}\n", 7, auto), [])
    check("rule7 exempt: a subdirectory of src/auto/ is covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto/deep/x.c"), [])
    check("rule7 exempt: a path that merely contains auto is not covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto_tools/x.c"), [1])
    check("rule7 exempt: a sibling prefix is not covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/automaton/x.c"), [1])
    check("rule7 exempt: the same code under src/Pl/ is still a violation",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/Pl/pl_act.cpp"), [1])
    check("rule7 exempt: a src/Pl/ body calling fn_ is still a violation",
          lines_of("void Pl_x(void) {\n    fn_80040598();\n}\n", 7, "src/Pl/pl_act.cpp"), [2])
    check("rule7 exempt: only rule 7 is exempt - rule 4 still fires under src/auto/",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4, auto), [3])
    check("rule7 exempt: rule 6 still fires under src/auto/",
          lines_of("void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n}\n", 6, auto), [2])
    check("rule7 exempt: rule 8 still fires under src/auto/",
          lines_of("void fn_802B2978(void) {\n    goto out;\nout:\n    return;\n}\n", 8, auto), [2])
    check("rule7 keys: rule_enforced consults all three keys when given the file",
          [rule_enforced(7, "src/auto/x.c"),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "/* stub: only a header comment */\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "void fn_80040598(void) {}\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c",
                         "/* rule 7 deferred: the map has no name */\nvoid fn_80040598(void) {}\n")),
           rule_enforced(6, "src/auto/x.c"), rule_enforced(7, "src/auto\\x.c")],
          [False, False, True, True, True, False])
    check("rule7 keys: the temporary src/auto/ entry is still in the table",
          [(r, p) for r, p, _w in EXEMPT], [(7, "src/auto/")])

    # --- rule 7 key 2: no bodies yet (the re-key - the first landing at a final path) -------------
    pl = "src/Pl/pl_act.cpp"
    check("rule7 bodyless: a src/Pl fn_ prototype is clean",
          lines_of("void fn_802B2978(void);\n", 7, pl), [])
    check("rule7 bodyless: a src/Pl fn_ call with no body is clean",
          lines_of("fn_80040598();\n", 7, pl), [])
    check("rule7 bodyless: a src/Pl bare unk local is clean",
          lines_of("u32 unk4;\n", 7, pl), [])
    check("rule7 bodyless: adding a body and no declaration removes the exemption",
          lines_of("void fn_802B2978(void) {}\n", 7, pl), [1])
    check("rule7 bodyless: a bodyfull unit at another final path is a violation too",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/enemy/em_act.c"), [1])
    check("rule7 bodyless: rule 6 still fires without a body",
          lines_of("u32 v = *(u32*)((u8*)p + 4);\n", 6, pl), [1])
    check("rule7 bodyless: rule 8 still fires without a body",
          lines_of("goto out;\n", 8, pl), [1])
    check("rule7 unk: a bare unk local in a bodyfull, undeclared src/Pl file still violates",
          lines_of("void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [2])

    # --- rule 7 key 3: the per-unit `rule 7 deferred: <reason>` declaration ------------------------
    defer = "/* rule 7 deferred: the map has only fn_XXXXXXXX for this range */\n"
    check("rule7 deferred: a declared bodyfull src/Pl fn_ definition is clean",
          lines_of(defer + "void fn_802B2978(void) {}\n", 7, pl), [])
    check("rule7 deferred: a declared fn_ call inside a body is clean",
          lines_of(defer + "void f(void) {\n    fn_80040598();\n}\n", 7, pl), [])
    check("rule7 deferred: an empty reason does not defer",
          lines_of("/* rule 7 deferred: */\nvoid fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: a declaration inside a string does not defer",
          lines_of('const char* s = "rule 7 deferred: x";\nvoid fn_802B2978(void) {}\n', 7, pl), [2])
    check("rule7 deferred: a declaration in a line comment defers",
          lines_of("// rule 7 deferred: the map has no name\nvoid fn_802B2978(void) {}\n", 7, pl), [])
    check("rule7 deferred: the declaration does not defer a bare unk local",
          lines_of(defer + "void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [3])
    check("rule7 deferred: rules 4/5/6/8 still fire in a deferred file",
          [r for r, _l in rules_of(
              defer
              + "/* size: 0x8 */\nstruct A {\n    u32 x;\n    /* +0x04 */ u32 unk04;\n};\n"
              + "void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n    goto out;\nout:\n    return;\n}\n",
              pl)],
          [4, 5, 6, 8])

    # --- rule 8: goto -----------------------------------------------------------------------------
    check("rule8: goto is a violation", lines_of("void f(void) {\n    goto out;\nout:\n    return;\n}\n", 8), [2])
    check("rule8: goto in a comment is clean",
          lines_of("/* the old shape was a goto dispatch */\nvoid f(void) {}\n", 8), [])
    check("rule8: goto in a string is clean", lines_of('const char* s = "goto";\n', 8), [])
    check("rule8: a plain label is not reported by this check", lines_of("void f(void) {\nout:\n    return;\n}\n", 8), [])
    check("rule8: a name containing goto is clean", lines_of("void f(void) {\n    u32 gotot = 1;\n}\n", 8), [])

    # --- rule 9: a mangled symbol is reached through its owner -----------------------------------
    check("rule9: a mangled call is a violation",
          lines_of("void f(void) {\n    get_now_areano__Fv();\n}\n", 9), [2])
    check("rule9: a namespaced mangled call is a violation",
          lines_of("void f(void) {\n    Panic__Q24nw4r2dbFPCciPCce(a, 1, b);\n}\n", 9), [2])
    check("rule9: a class-member mangling is a violation",
          lines_of("void f(void) {\n    move__6MHcharFUs(x, 0);\n}\n", 9), [2])
    check("rule9: an fn_XXXXXXXX call is not a mangling",
          lines_of("void f(void) {\n    fn_80040598();\n}\n", 9), [])
    check("rule9: a member call through its owner is clean",
          lines_of("void f(A* a) {\n    a->method(1);\n}\n", 9), [])
    check("rule9: a namespaced call through its owner is clean",
          lines_of("void f(void) {\n    ns::func(1);\n}\n", 9), [])
    check("rule9: a declaration is a finding too (row 50's other half)",
          lines_of("extern void get_now_areano__Fv(void);\n", 9), [1])
    check("rule9: an extern \"C\" mangled declaration is a finding",
          lines_of('extern "C" void Panic__Q24nw4r2dbFPCciPCce(const char*, int, const char*);\n', 9), [1])
    check("rule9: a mangled name in a comment is clean",
          lines_of("/* call get_now_areano__Fv() through the owner */\nvoid f(void) {}\n", 9), [])
    check("rule9: a mangled name in a string is clean",
          lines_of('const char* s = "get_now_areano__Fv";\n', 9), [])
    check("rule9: an ordinary name with underscores is clean",
          lines_of("void f(void) {\n    get_thing();\n}\n", 9), [])
    check("rule9: a call in an expression is a call",
          lines_of("void f(void) {\n    if (get_now_areano__Fv() == 1) {}\n}\n", 9), [2])
    check("rule9: a definition is a declaration",
          lines_of("void get_now_areano__Fv(void) {\n}\n", 9), [1])
    decl = "extern void get_now_areano__Fv(void);\n"
    call = "void f(void) {\n    get_now_areano__Fv();\n}\n"
    check("rule9: the classifier reads a declaration",
          looks_like_declaration(decl, decl.index("get_now_areano__Fv")), True)
    check("rule9: the classifier reads a call",
          looks_like_declaration(call, call.index("get_now_areano__Fv")), False)
    check("rule9: the classifier reads a return expression as a call",
          looks_like_declaration("return get_now_areano__Fv();\n",
                                 "return get_now_areano__Fv();".index("get_now")), False)

    # --- rule 1: a shared type is defined once (cross-file) ---------------------------------------
    def r1(*files: tuple[str, str]) -> list[tuple[int, str, int]]:
        sources = [Source("%s.c" % name, "src/%s.c" % name, text) for name, text in files]
        return [(f["rule"], f["file"], f["line"]) for f in rule1_findings(sources)]

    dup = "/* size: 0x8 */\nstruct Foo {\n    /* +0x00 */ u32 x;\n};\n"
    check("rule1: the same type in two files is a finding in the extra file",
          r1(("a", dup), ("b", dup)), [(1, "src/b.c", 2)])
    check("rule1: a type defined once is clean", r1(("a", dup)), [])
    check("rule1: three files give two findings, one per extra file",
          [f for _r, f, _l in r1(("a", dup), ("b", dup), ("c", dup))], ["src/b.c", "src/c.c"])
    check("rule1: the message names both files",
          [f["detail"] for f in rule1_findings([Source("x", "src/a.c", dup), Source("y", "src/b.c", dup)])],
          ["type `Foo` is defined in `src/a.c` and again in `src/b.c` - one definition, in the owner's header"])
    check("rule1: the owner is the lexicographically first file",
          [f["detail"] for f in rule1_findings([Source("x", "src/z.c", dup), Source("y", "src/a.c", dup)])],
          ["type `Foo` is defined in `src/a.c` and again in `src/z.c` - one definition, in the owner's header"])
    check("rule1: a union is a type too",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", "/* size: 0x8 */\nunion Foo {\n    /* +0x00 */ u32 x;\n};\n"),
              Source("y", "src/b.c", "/* size: 0x8 */\nunion Foo {\n    /* +0x00 */ u32 x;\n};\n")])],
          [1])
    check("rule1: a forward declaration is not a definition",
          r1(("a", "struct Foo;\n"), ("b", "struct Foo;\n")), [])
    check("rule1: an anonymous typedef gets the name after the brace",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", "typedef struct {\n    u32 x;\n} Foo;\n"),
              Source("y", "src/b.c", "typedef struct {\n    u32 x;\n} Foo;\n")])],
          [1])
    check("rule1: two different types are not duplicates",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", dup), Source("y", "src/b.c", dup.replace("Foo", "Bar"))])],
          [])

    # --- rule 2: an extern lives with the TU that owns it -----------------------------------------
    idx = Ownership({"foo": [(".text", 0x1000, "function")], "bar": [(".text", 0x3000, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2: an extern for another unit's symbol is a finding",
          lines_of("extern void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2: the same extern for a symbol the file owns is clean",
          lines_of("extern void foo(void);\n", 2, "src/mod/a.c", idx), [])
    check("rule2: the owner's header is clean too",
          lines_of("extern void foo(void);\n", 2, "src/mod/a.h", idx), [])
    check("rule2: an extern variable for another unit's symbol is a finding",
          lines_of("extern u16 bar[2];\n", 2, "src/other/c.c", idx), [1])
    check("rule2: a symbol not in the map is not a finding",
          lines_of("extern void not_in_map(void);\n", 2, "src/other/c.c", idx), [])
    check("rule2: the missing name is counted as a gap",
          idx.gaps["not in symbols.txt"], 1)
    mid = Ownership({"mid": [(".text", 0x2500, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2: an unsplit symbol with one bracketing module is a finding",
          lines_of("extern void mid(void);\n", 2, "src/other/c.c", mid), [1])
    check("rule2: the unsplit detail names the module header",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void mid(void);\n"), mid)
           if f["rule"] == 2],
          ["`mid` has no registered owner - declare it in `include/unsplit/mod.h`"])
    gap = Ownership({"gap": [(".text", 0x2500, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "other/b.c")]})
    check("rule2: an unsplit symbol whose brackets disagree is a documented gap",
          lines_of("extern void gap(void);\n", 2, "src/other/c.c", gap), [])
    check("rule2: the interleaved case is counted in ownership.gaps",
          gap.gaps["unsplit: address band interleaves modules"], 1)
    check("rule2: extern declarations are found by the scanner",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void a(void);\nextern u16 b[2];\nextern void (*c)(int);\n"))], ["a", "b", "c"])
    check("rule2: a function-pointer parameter is not the declared name",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void* f(s32 a, void (*cb)(void));\n"))], ["f"])
    check("rule2: a function returning a function pointer is named",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void (*f(int))(void);\n"))], ["f"])
    # the real map: the ownership lookup resolves a symbol we name, from the tree's own data
    if os.path.exists(os.path.join(".", "config", "RMHE08", "symbols.txt")):
        real = load_ownership(".")
        r = real.resolve("em_act_ck__FP11_ENEMY_WORKUcUc") if real else None
        check("rule2: the real map resolves a named symbol to its owner unit",
              (r or {}).get("kind"), "owned")
        check("rule2: the real map names the enemy unit that owns it",
              (r or {}).get("unit"), "enemy/fn_8012BDF4.cpp")

    # --- budget aggregation -----------------------------------------------------------------------
    text = ("/* size: 0x8 */\nstruct A {\n    u32 unk00;\n};\n"
            "void f(u8* p) {\n    *(u32*)((u8*)p + 0x4) = 1;\n}\n")
    f1 = lint_source(Source("a.c", "src/A/a.c", text))
    f2 = lint_source(Source("b.c", "src/B/b.c", "void fn_80040598(void) {}\n"))
    b = budget(f1 + f2)
    check("budget: two units", [row["file"] for row in b["units"]], ["src/A/a.c", "src/B/b.c"])
    check("budget: unit row total", b["units"][0]["total"], 3)
    check("budget: rule 5 counted once", b["totals"]["5"], 1)
    check("budget: rule 7 counted once", b["totals"]["7"], 1)
    check("budget: empty rule is zero", b["totals"]["8"], 0)
    check("budget: overall total", b["total"], 4)
    check("budget: units are sorted", b["units"] == sorted(b["units"], key=lambda r: r["file"]), True)
    check("budget: findings count", b["findings"], 4)
    check("budget: unique fn names", b["unique"]["fn_names"], 1)
    check("budget: unique unk fields", b["unique"]["unk_fields"], 1)
    check("budget: unique types", b["unique"]["types"], 0)
    check("budget: unique shared types", b["unique"]["shared_types"], 0)
    check("budget: unique extern symbols", b["unique"]["extern_symbols"], 0)
    check("budget: unique mangled names", b["unique"]["mangled_names"], 0)
    check("unique: repeated sites collapse", unique_names([
        {"rule": 5, "detail": "field `unk1`"}, {"rule": 5, "detail": "field `unk1`"},
        {"rule": 7, "detail": "bare `unk1` identifier"},
    ])["unk_fields"], 1)

    # --- diff comparison --------------------------------------------------------------------------
    before = rule_counts(f1)
    after = rule_counts(f1 + f2)
    d = diff_deltas(before, after)
    check("diff: one rising pair", [(x["rule"], x["file"], x["added"]) for x in d], [(7, "src/B/b.c", 1)])
    check("diff: an unchanged tree is clean", diff_deltas(before, before), [])
    check("diff: a falling count is not an addition",
          diff_deltas({(5, "x"): 3}, {(5, "x"): 1}), [])
    check("diff: a fixed rule still fails for a different one",
          [(x["rule"]) for x in diff_deltas({(5, "x"): 1, (8, "x"): 0}, {(5, "x"): 1, (8, "x"): 1})], [8])

    # --- end-to-end over the fixtures -------------------------------------------------------------
    check("e2e: rule list is complete", sorted(RULE_NAMES), [1, 2, 3, 4, 5, 6, 7, 8, 9])
    check("e2e: no rule is declared unchecked", UNCHECKED, [])
    check("e2e: findings sort by rule then line",
          rules_of(text), sorted(rules_of(text)))

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --------------------------------------------------------------------------------------------------
# cli
# --------------------------------------------------------------------------------------------------
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Lint src/ against docs/plan.md 6.5 (roadmap 7.21).")
    ap.add_argument("--diff", metavar="REF",
                    help="fail only if the working tree adds a violation relative to REF")
    ap.add_argument("--budget", action="store_true", help="report the backlog per unit over src/")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()

    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True)
    root = root.stdout.strip() if root.returncode == 0 else os.getcwd()
    ownership = load_ownership(root)

    if args.diff is not None:
        try:
            pairs = changed_src_files(root, args.diff)
            rels = [after for _before, after in pairs]
            before = merge_counts(findings_at_ref(root, args.diff, pairs, ownership),
                                  rule1_counts_at_ref(root, args.diff, pairs))
        except RuntimeError as exc:
            print("stylelint: %s" % exc, file=sys.stderr)
            return 2
        after = merge_counts(
            rule_counts(lint_tree(root, [os.path.join(root, a) for _b, a in pairs], ownership)),
            rule_counts(rule1_findings(all_sources(root))))
        added = diff_deltas(before, after)
        if args.json:
            print(json.dumps({"ref": args.diff, "added": added, "changed": rels,
                              "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                              "exempt": exemptions()}, indent=2))
        elif added:
            print("stylelint: the batch adds %d section 6.5 violation(s) over %d changed file(s):"
                  % (sum(a["added"] for a in added), len(rels)))
            for a in added:
                print("  +%d rule %d  %s  (%d -> %d)" % (a["added"], a["rule"], a["file"], a["before"], a["after"]))
            for num, what in UNCHECKED:
                print("  not checked (cross-file): rule %d - %s" % (num, what))
            for rule, prefix, why in EXEMPT:
                print("  not enforced: rule %d under %s (%s)" % (rule, prefix, why))
            for cond, why in RULE7_NOTES:
                print("  not enforced: rule 7 for %s (%s)" % (cond, why))
        else:
            print("stylelint: no new section 6.5 violation over %d changed file(s) "
                  "(rule 2 checked where an owner resolves; rule 7 exempt per rule7_state)" % len(rels))
        return 1 if added else 0

    findings = lint_all(root, ownership)
    if args.json:
        print(json.dumps({"budget": budget(findings),
                          "rule2_gaps": dict(ownership.gaps) if ownership else {},
                          "rule2_unsplit_modules": (
                              {m: {"sites": n, "symbols": len(ownership.unsplit_symbols.get(m, ()))}
                               for m, n in ownership.unsplit_modules.items()} if ownership else {}),
                          "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                          "exempt": exemptions()}, indent=2))
    elif args.budget:
        print_budget(findings, ownership)
    else:
        print_findings(findings)
    return 0


if __name__ == "__main__":
    sys.exit(main())
