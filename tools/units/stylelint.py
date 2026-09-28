#!/usr/bin/env python3
"""Lint `src/` against the type and naming discipline of docs/plan.md section 6.5 (roadmap 7.21).

A rule enforced by remembering is not a rule: the conformance rules were agreed with the owner and the
only thing keeping them is the reviewer's attention. This tool turns the mechanically checkable ones into
`file:line` findings, and `land.py verify` calls it with `--diff <base>` so a batch that **adds** a
violation is refused before it is committed.

    python tools/units/stylelint.py --budget          # the per-unit backlog over src/
    python tools/units/stylelint.py --diff <ref>      # exit 0 = the working tree adds no violation
    python tools/units/stylelint.py --ref <branch>    # read-only: judge a held branch's committed tree
    python tools/units/stylelint.py --json            # machine-readable findings + budget
    python tools/units/stylelint.py --selftest

Rules checked (each finding is `file:line`):

| # | rule | how it is decided |
| --- | --- | --- |
| 1 | a shared type lives in one header | the same `struct`/`class`/`union` name defined with a body in more than one `src/` file: one finding per (type, extra file), naming both files |
| 2 | an extern lives with the TU that owns it | a declaration (the `extern` keyword or a plain function prototype) of a symbol whose registered owner (symbols.txt address + splits.txt range) is another unit, or of a symbol with **no registered owner** at all (an unsplit address), which belongs in a band header under `include/unsplit/`; an ordinary `include/<module>/*.h` header is judged too (a foreign declaration there is a finding); and, in `include/unsplit/*.h` itself, any declaration of a symbol a registered unit owns (that header is a fallback, not the owner) |
| 3 | a reconstructed `struct`/`class` states its size | a `size: 0xNN` comment within four lines of the definition (or two lines after its closing brace) |
| 4 | every field carries its offset | an offset comment on the field's own line(s); `/* +0x1C */` is the canonical form and the `/* 0x1C */` variant the existing units use is accepted |
| 5 | no field is left named `unk*` | a field name matching `unk`, `unkNN`; `pad_0xNN` / `unused_0xNN` are the exception |
| 6 | no pointer arithmetic reaches a field | a `(T*)base + 0xNN` / `(T*)(base + 0xNN)` cast-plus-literal-offset expression, except an offset passed straight to `memset`/`memcpy`/`memmove` (the rule's own byte-range exception) |
| 7 | no auto-generated name survives | `fn_XXXXXXXX` anywhere, `lbl_XXXXXXXX`/`loc_XXXXXXXX` anywhere, and `unk*` used for anything that is not a struct field (a field is rule 5's) - **whoever owns the symbol**; no exemption, no deferral |
| 8 | `goto` is forbidden | the `goto` keyword |
| 9 | a mangled symbol is called/declared through its owner | a callee identifier that carries a compiler mangling (`Name__FP...`, `Name__Q34nw4r...`, a class member `name__<len>ClassF...`) used as a call **or** as a declaration; an `fn_XXXXXXXX` stem has no `__` and stays legal |
| 10 | a codegen pragma lives in the TU that needs it | a `#pragma` whose name is codegen-affecting (`peephole`, `optimization_level`, `fp_contract`, ...) in a file under `include/` (a `.c`/`.cpp` is never reported). A pragma leaks into every TU that includes the header |
| 11 | no `void *` parameter or return type | a `void` `*` in a function declaration's parameter list or return type (a declaration, never a cast). Erasing the type hides what a heterogeneous call site is actually passing; the only exemption is a per-declaration `/* untyped: <reason> */` marker whose reason names which genuinely-untyped case it is - a byte range, an opaque handle passed through, or a caller-owned payload |
| 12 | data a unit uses that no registered range claims is the unit's to claim | an `extern` declaration (the `extern` keyword) of a symbol whose `symbols.txt` type is an **object** and whose address falls in **no** registered `splits.txt` range. The declaration is the defect: the unit that reads or writes the bytes claims the range in its own section and matches it, so the finding names the address the claim covers |

Rule 2 is checked from `config/RMHE08/symbols.txt` (a symbol's section and address) + `config/RMHE08/splits.txt`
(each registered unit's ranges) and reads **both** declaration shapes a file can make: the `extern` keyword
and a plain function prototype (`void foo(void);` - the `extern`-only scanner could not see the latter, so
`src/Network/fn_8041A87C.cpp`'s local `memset`/`memcpy` and `src/DWCi/fn_805113B0.c`'s `DWCi_GetStringLength`
were invisible while four real sites existed in the Network scope). A declaration a file makes for a symbol
another registered unit owns is a finding - move it to that unit's header and `#include` it. A symbol with
**no registered owner** (the map resolves it to an unsplit address) is a finding too: the local declaration
is the defect and it belongs in a band header under `include/unsplit/`. When the registered bands interleave
across modules (a `sound` unit sits inside the `ef` band) so no module is sound, the finding names the band
directory only rather than guess a `<module>.h`, and the local declaration is still wrong in the `src/` file.
A symbol missing from the map, a duplicate map row, and an address the map gives no section stay counted gaps
(`Ownership.gaps`) - the map cannot judge them, so they are not guessed. The owning unit's own public header
is not a finding: `_owns` recognises `include/<module>/<stem>.h` as the owner's header (before that fix the
own header read as foreign, so extending rule 2 to headers would have reported ~30 owners' own headers in the
Network scope alone). An ordinary `include/<module>/*.h` header is judged by this rule (a foreign declaration
there is a finding; an unowned symbol stays, because the module header carries public names the splits map
has not registered and the band is the detector for those). The band is checked as well -
`include/unsplit/*.h` is a file a batch may change, and a declaration there of a
symbol a registered unit owns is a finding, because the owner's typed definition collides with it
(`(10197) illegal function overloading`); an unowned symbol stays, which is the band's purpose. A
definition in the band is not a declaration and is left alone.

Comments and string/char literals are stripped before matching, and the two are stripped separately: the
size/offset annotations of rules 3-4 live *in comments*, while every other rule must not fire on text
inside one. The stripped strings keep the file's exact length and newlines, so a reported line is the
original line. `Pl/pl_act.cpp`'s header comment naming the banned `goto` dispatch is the incident this
exists for.

The backlog is a burn-down, not a gate: `--diff` fails only when a (rule, file) count rises, so touching a
unit with 300 `unk*` fields is allowed as long as the touch adds none. A **new** file starts from zero, so
its violations are all additions - new work is held to the rules from its first commit.

**Rule 7 has no exemption and no deferral.** Every `fn_XXXXXXXX`, `lbl_XXXXXXXX`, `loc_XXXXXXXX` and bare
`unk*` identifier in `src/` is a finding, whoever owns the symbol: this unit's, another unit's, an unowned
one, or a name the map does not know. A data label is not "the owner's to name" any more - leaving a
generated spelling on either side of an ownership line is the defect. The rule applies to a file with no
bodies too, and **no comment exempts anything**: a `rule 7 deferred` line is now just inert text, not a key.

The **only** grandfather is the gate's own `--diff`: touching a file that already carries findings is
allowed (an existing finding never blocks a landing), while adding one is refused. That is the owner's
"do not revoke committed progress" - the mounted debt is worked slowly through the backlog register
(`tools/units/backlog.py`), never through a per-file escape hatch. **Rules 1-6, 8, 9, 10 and 11 apply as
before.**

`--ref <branch>` is the **read-only** sibling of `--diff`: it judges a *held branch's committed tree*
against the merge base `--diff` would resolve (the branch's tip for the `after` side, exactly as if the
branch were checked out), so a lane can prove a `splits.txt` claim cleared a held branch's rows without
checking it out and without writing anything. The comparison itself is `--diff`'s - each side judged by
the map it was written against - and `--diff`'s behaviour and its "REF is not an ancestor" warning are
untouched.

**Rule 11 has no per-file key either.** A `void *` parameter or return type is a finding by default, and
the exemption is a **per-declaration** marker comment - `/* untyped: <reason> */` on the declaration or the
line above it (a marker on the line above must stand alone, so a trailing marker on one declaration never
exempts the next) - whose reason says which genuinely-untyped case it is (a byte range, an opaque handle
passed through, or a caller-owned payload). A file cannot exempt itself, exactly as rule 7's per-file keys
were removed; `grep -rn "untyped:" src include` is the complete, reviewable list of exemptions, and a
marker with an empty or vague reason is still a finding. The scan reads **declarations** (the same
`_declared_name` parser rule 2 uses), so a `(void*)p` cast inside a body is never a finding; a `void *`
**local variable** is out of the rule's scope and is only counted (the report prints the number) so the
owner can decide later. The rule is ticked in the register as its own `untyped` kind.

**Rule 12 (owner, 2026-09-28).** An `extern` declaration of a **data** symbol - `symbols.txt` says
`type:object` - whose address no registered `splits.txt` range covers is a finding: the unit that reads or
writes those bytes **claims the range in its own `splits.txt`** and matches it as part of its own object.
The old "no registered owner; declared, never defined" header comment was the finding naming itself, so a
band header under `include/unsplit/` is judged by this rule too - rule 2's band is the fallback for a
symbol no unit can claim, not the answer for data a unit demonstrably uses. The same `Ownership` index as
rule 2 decides it, so the map-absent / duplicate-row / not-in-the-map cases stay counted gaps here as well;
`rule 2` and `rule 12` may both name one `extern` line and that is intended - rule 2 says whose header the
declaration belongs in, rule 12 says the bytes must be claimed. The declare-never-define carve-out is
unchanged and is *not* checked here: when the address is already inside the file's own registered range,
`resolve` returns `owned` and rule 12 does not fire (playbook 29). A **function** declaration is rule 2's,
never rule 12's.
"""

from __future__ import annotations

import argparse
import collections
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

SRC = "src"
# The unsplit band (`include/unsplit/<module>.h`) is the legitimate home for a symbol with no registered
# owner. It is declaration-only glue, so only rule 2 applies to it - but rule 2 *does*: an owned symbol
# declared here collides with the owner's typed definition (MWCC `(10197) illegal function overloading`)
# in every translation unit that includes the band.
UNSPLIT = "include/unsplit"
# The pseudo-module rule 2 reports when the registered bands bracketing an unsplit address name different
# modules (a `sound` unit inside the `ef` band): no `<module>.h` is sound, so the finding names the band
# directory instead. It is not a path component, so it cannot collide with a real module name.
UNSPLIT_UNRESOLVED = "<band unresolved>"
# Every shared header lives under `include/` (the unsplit band is `include/unsplit/`).  Rule 10 scans this
# whole tree: a codegen pragma is lexically scoped to the rest of every TU that includes the header, so
# one in the tree silently changes code that does not belong to the header's author.
HEADERS = "include"
HEADER_SUFFIXES = (".h", ".hpp", ".hh")
SUFFIXES = (".c", ".cpp", ".cp", ".cc", ".h", ".hpp", ".hh")

# Rule 7's path-keyed exemption table is **empty and stays empty** (owner's ruling, 2026-09-27): the
# `src/auto/` bucket is retired and no path exempts a rule. The table is kept so `exemptions()` and the
# "not enforced" report lines stay honest - an empty table is a statement, not an omission.
# One entry per (rule, path prefix); every other rule still applies under the prefix.
EXEMPT: list = []

# The per-file rule-7 keys are gone too: a file with no bodies is fired on like any other, and a
# `rule 7 deferred: <reason>` comment is inert. Reported alongside `EXEMPT` because both are conditions
# rather than path prefixes; empty now, and the JSON output says so.
RULE7_NOTES: list = []

RULE_NAMES = {
    1: "a shared type is defined once (in the owner's header)",
    2: "an extern lives with the TU that owns it (or a header under include/unsplit/)",
    3: "struct/class states its size (/* size: 0xNN */)",
    4: "field carries its offset (/* +0xNN */)",
    5: "no field left named unkNN (pad_0xNN / unused_0xNN are the exception)",
    6: "no pointer arithmetic to reach a field",
    7: "no auto-generated name survives (`fn_XXXXXXXX` / `lbl_XXXXXXXX` / `loc_XXXXXXXX` / bare `unkNN`)",
    8: "goto is forbidden",
    9: "no mangled spelling used as a callable identifier (call/declare the owner)",
    10: "a codegen pragma lives in the TU that needs it, not in a shared header",
    11: "no `void *` parameter or return type (mark the declaration `/* untyped: <reason> */` if genuinely untyped)",
    12: "data no registered range claims is the unit's to claim and match (an `extern` for it is the finding)",
}

# The codegen-affecting pragma names for rule 10.  A `#pragma` is lexically scoped to the rest of the
# translation unit that reaches it, so one in a shared header leaks the pass onto every including TU -
# measured 2026-09-27 in both directions: a lane matched a file only because of a leaked
# `#pragma peephole off`, and another lost rows until it was restated where it was wanted.  The pragma
# belongs in the `.c`/`.cpp` that measured the dependency.  The list is section 6.5's codegen subset;
# extend it when a new codegen pragma is used.  Deliberately absent: `once` (include guard), `pack`
# (layout/ABI, judged elsewhere), `legacy_messages`/`warn*` (diagnostics only).
CODEGEN_PRAGMAS = (
    "peephole", "optimization_level", "fp_contract", "optimize_for_size", "inline",
    "pool", "scheduling", "scheduling_priority", "unroll", "vectorize", "ipa", "profile",
    "opt_propagation", "opt_common_subs", "opt_lifetimes",
)
CODEGEN_PRAGMA_RE = re.compile(
    r"^[ \t]*#[ \t]*pragma[ \t]+(" + "|".join(CODEGEN_PRAGMAS) + r")\b", re.M)
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

    Two views of the same rows.  `resolve(name)` answers "what does this *name* mean"; `resolution_at`
    answers "what did this *address* mean, whatever the row was called" - the view a rename needs, because
    a rename keeps the address and changes the name (`owed_rename_completion`).
    """

    def __init__(self, symbols: dict, ranges: dict):
        self.symbols = symbols
        self.ranges = {s: sorted(v) for s, v in ranges.items()}
        self.gaps: "collections.Counter" = collections.Counter()
        self.unsplit_modules: "collections.Counter" = collections.Counter()
        self.unsplit_symbols: dict[str, set] = {}
        self.foreign_units: "collections.Counter" = collections.Counter()
        self._at_address: "dict | None" = None

    def name_at(self, section: str, address: int) -> "str | None":
        """The map's row name for an address, or None when the map has no row there at all.

        Built lazily as one inverted index over the rows (no range scan per query); a name with duplicate
        rows is skipped, exactly as `resolve` refuses to guess it.
        """
        if self._at_address is None:
            index: dict = {}
            for name, entries in self.symbols.items():
                if len(entries) == 1:
                    index.setdefault((entries[0][0], entries[0][1]), name)
            self._at_address = index
        return self._at_address.get((section, address))

    def resolution_at(self, section: str, address: int) -> "dict | None":
        """`resolve` of whatever row sits at `(section, address)`, or `None` when the map has none.

        The **address** view: a rename changes the name a file spells, not the address it lands on, so this
        is what tells "the batch renamed a referrer to a row the base already had" from "the batch made a
        declaration foreign" (an address with no row at base, or one owned by somebody else).
        """
        name = self.name_at(section, address)
        return self.resolve(name) if name else None

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


_OWNERSHIP_REF_CACHE: dict = {}


def load_ownership_at_ref(root: str, ref: str) -> "Ownership | None":
    """The rule-2 index **as of `ref`** - each side of a `--diff` is judged by the map it was written against.

    Judging the base side by the *working* map makes a rename read as a regression: the base copy of a file
    still says `fn_80043EA8`, this batch renamed that row to `VEC3_ctor`, so the base side's local declarations
    stop resolving to an owner, their rule-2 findings vanish from the `before` count, and `diff_deltas` reports
    them as **additions** - a real batch measured "+62 added rule-2 violations" for a pure rename, and spent
    ~30 minutes proving it was an artefact.

    Judging each side by its own map keeps them comparable and still charges a batch for the ownership *it*
    creates: a newly registered unit's ranges exist only in the working map, so the declarations it newly
    orphans are additions there and absent from the base.
    """
    if not ref:
        return load_ownership(root)
    if (root, ref) in _OWNERSHIP_REF_CACHE:
        return _OWNERSHIP_REF_CACHE[(root, ref)]
    tmp = tempfile.mkdtemp(prefix="stylelint-ref-")
    try:
        paths = {}
        for rel in ("config/RMHE08/symbols.txt", "config/RMHE08/splits.txt"):
            try:
                text = git_bytes(root, "show", "%s:%s" % (ref, rel)).decode("utf-8", "replace")
            except RuntimeError:
                return None
            paths[rel] = os.path.join(tmp, os.path.basename(rel))
            with open(paths[rel], "w", encoding="utf-8", newline="") as fh:
                fh.write(text)
        own = Ownership(_parse_symbols(paths["config/RMHE08/symbols.txt"]),
                        _parse_splits(paths["config/RMHE08/splits.txt"]))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    _OWNERSHIP_REF_CACHE[(root, ref)] = own
    return own


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


def is_unsplit_header(rel: str) -> bool:
    """Whether `rel` is a declaration-only header in the unsplit band."""
    rel = rel.replace("\\", "/")
    return rel.startswith(UNSPLIT + "/") and rel.endswith(SUFFIXES)


def is_shared_header(rel: str) -> bool:
    """Whether `rel` is a shared header under `include/` (the unsplit band included)."""
    rel = rel.replace("\\", "/")
    return rel.startswith(HEADERS + "/") and rel.endswith(HEADER_SUFFIXES)


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
    """Whether `rel` is `unit`'s own source file or its own header.

    Three spellings: the translation unit itself (`src/<unit>`), a private header beside it
    (`src/<stem>.h`), and the unit's **public** header under `include/` (`include/**/<stem>.h`, any depth,
    where `<stem>` is the unit's module-qualified stem).  The public-header case is the one the original
    test missed: `_owns('include/NHTTP/NHTTP_bgnend.h', 'NHTTP/NHTTP_bgnend.c')` was False, so an owner's
    own header read as a foreign declaration and rule 2 could not be extended to headers without reporting
    ~30 owners' own headers in the Network scope alone (2026-09-28).  The match is by the module-qualified
    stem suffix, not the bare basename, so a same-named header in another module stays foreign.
    """
    rel = rel.replace("\\", "/")
    if rel == SRC + "/" + unit:
        return True
    stem = os.path.splitext(unit)[0]
    if any(rel == SRC + "/" + stem + ext for ext in (".h", ".hpp", ".hh")):
        return True
    # the unit's public header under `include/`: its path carries the unit's module-qualified stem
    # (`include/Network/network_state.h` for `Network/network_state.cpp`, `include/NHTTP/NHTTP_bgnend.h`
    # for `NHTTP/NHTTP_bgnend.c`). The suffix match keeps a same-basename header in another module
    # (`include/other/network_state.h`) out of the owner's set, which a bare-basename match would admit.
    return (rel.startswith(HEADERS + "/")
            and any(rel.endswith("/" + stem + ext) for ext in HEADER_SUFFIXES))


_LINKAGE_OPEN_RE = re.compile(r"\bextern\s*$")
_TYPE_ONLY_RE = re.compile(r"^\s*(?:typedef\s+)?(?:struct|class|union|enum)\b")


def _file_scope_declarations(src: Source) -> list[tuple[str, str, int]]:
    """`(name, segment, segment_start)` for every file-scope statement that introduces a name.

    The walk `header_declarations` and `prototype_declarations` share.  An unsplit-band prototype sits
    inside `extern "C" { ... }`; `strip` blanks the `"C"`, so the linkage block is just a brace opened by
    an `extern`, and it is *transparent*: its contents are file scope, which is where declarations live.
    A file-scope statement ending in `;` that introduces a name is returned; a type forward declaration
    (`struct Foo;`) introduces no symbol from the map and is skipped; a function body's `{` is a real
    scope, so a definition is never returned.
    """
    out: list[tuple[str, str, int]] = []
    code = src.code
    depth = 0
    transparent: list[bool] = []
    stmt_start = 0
    for i, c in enumerate(code):
        if c == "{":
            linkage = bool(_LINKAGE_OPEN_RE.search(code[stmt_start:i]))
            transparent.append(linkage)
            if not linkage:
                depth += 1
            stmt_start = i + 1
        elif c == "}":
            if transparent:
                if not transparent.pop():
                    depth -= 1
            stmt_start = i + 1
        elif c == ";" and depth == 0:
            seg = code[stmt_start:i]
            seg_start = stmt_start
            stmt_start = i + 1
            if _TYPE_ONLY_RE.match(seg) and "(" not in seg:
                continue
            name = _declared_name(seg)
            if name is None:
                continue
            out.append((name, seg, seg_start))
    return out


def _seg_line(src: Source, name: str, seg: str, seg_start: int) -> int:
    """The line of `name` inside `seg`, falling back to the statement's first line."""
    m = re.search(r"\b%s\b" % re.escape(name), seg)
    return src.line_of(seg_start + (m.start() if m else 0))


def header_declarations(src: Source) -> list[tuple[str, int]]:
    """`(name, line)` for every declaration a header makes at file scope.

    See `_file_scope_declarations` for the walk.  A declaration here needs no `extern` keyword - an
    ordinary `void foo(void);` is the header's normal shape.
    """
    return [(name, _seg_line(src, name, seg, seg_start))
            for name, seg, seg_start in _file_scope_declarations(src)]


def prototype_declarations(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every plain function prototype at file scope (no `extern` keyword).

    The `extern`-keyword form is `extern_declarations`' job; this is the shape that scanner could not
    see - a plain prototype in a `.cpp`/`.c` (`void foo(void);`), which is how the Network scope's
    `memset`/`memcpy`/`DWCi_GetStringLength` sites are written.  A statement with no `(` introduces a
    variable, not a callable, and is left to the other rules; a statement that carries the `extern`
    keyword is excluded so the two scanners never report one site twice.
    """
    out = []
    for name, seg, seg_start in _file_scope_declarations(src):
        if "(" not in seg or EXTERN_RE.search(seg):
            continue
        out.append((name, seg_start, _seg_line(src, name, seg, seg_start)))
    return out


def declaration_sites(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every declaration rule 2 judges in a `src/` file: the union of the
    `extern`-keyword form and the plain function prototype, with one site never counted twice."""
    seen: set[tuple[str, int]] = set()
    out = []
    for name, pos, line in list(extern_declarations(src)) + prototype_declarations(src):
        if (name, pos) in seen:
            continue
        seen.add((name, pos))
        out.append((name, pos, line))
    return out


def rule2_band_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Declarations in `include/unsplit/<band>.h` of a symbol a registered unit already owns.

    The band exists for a symbol with no owner, so only an `owned` resolution is a finding; an unsplit
    name stays (that is the band's purpose), and a name the map cannot judge is left alone rather than
    counted - the band is where such names legitimately live. The message matches the `src/` rule
    exactly; a definition here is not a declaration (`header_declarations` never returns one).
    """
    out = []
    for name, line in header_declarations(src):
        r = ownership.resolve(name)
        if r is None or r["kind"] != "owned":
            continue
        out.append(_rule2_finding(src, line, name,
                                  "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                  "#include it" % (name, r["unit"])))
    return out


def rule2_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Every declaration the file makes for a symbol it does not own.

    Both declaration shapes are judged (`declaration_sites`): the `extern` keyword and the plain function
    prototype.  Three outcomes: `owned` by this file (no finding), owned by another registered unit (move
    the declaration to that unit's header and `#include` it), or `unsplit` (the symbol has no registered
    owner: move the declaration to `include/unsplit/<module>.h`, or - when the bracketing bands name
    different modules so no module is sound - to a header under `include/unsplit/`). A name not in the
    map and a name with duplicate map rows are left as counted gaps rather than guessed.
    """
    out = []
    for name, _pos, line in declaration_sites(src):
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
            out.append(_rule2_finding(src, line, name,
                                      "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                      "#include it" % (name, r["unit"])))
            continue
        module = r["module"]
        if module is None:
            # The band cannot place this address: the registered units bracketing it name different
            # modules (a `sound` unit inside the `ef` band is the canonical case), so there is no sound
            # `<module>.h` to name. The local declaration is still the defect - a symbol with no
            # registered owner belongs in the band, not in a `src/` file - so it is reported without
            # guessing the module header (this used to be a counted `Ownership.gaps` entry).
            module = UNSPLIT_UNRESOLVED
        ownership.unsplit_modules[module] += 1
        ownership.unsplit_symbols.setdefault(module, set()).add(name)
        if module == UNSPLIT_UNRESOLVED:
            out.append(_rule2_finding(src, line, name,
                                      "`%s` has no registered owner - declare it in a header under "
                                      "`include/unsplit/`" % name))
        else:
            out.append(_rule2_finding(src, line, name,
                                      "`%s` has no registered owner - declare it in `include/unsplit/%s.h`"
                                      % (name, module)))
    return out


def rule2_header_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Declarations in an ordinary `include/` header for a symbol another registered unit owns.

    A header is not a unit - it is the public face of the unit(s) in its module - so a declaration there
    is clean only when `_owns` recognises the file as the owner's own header (`include/<module>/<stem>.h`,
    which the `_owns` fix made it do).  A declaration of a symbol another unit owns is a finding: the
    declaration belongs in that unit's header.

    A symbol with no registered owner is left alone here (unlike the `src/` reading): the detector for
    those is the unsplit band, and a module header carries many public names whose addresses the splits
    map has not registered yet - reporting them would bury the real foreign declarations.
    """
    out = []
    for name, line in header_declarations(src):
        r = ownership.resolve(name)
        if r is None or r["kind"] != "owned":
            continue
        if _owns(src.rel, r["unit"]):
            continue
        ownership.foreign_units[r["unit"]] += 1
        out.append(_rule2_finding(src, line, name,
                                  "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                  "#include it" % (name, r["unit"])))
    return out


def rule12_findings(src: Source, ownership: "Ownership | None") -> list[dict]:
    """Rule 12 for one file: every `extern` of a **data** symbol no registered range claims.

    The owner's ruling (2026-09-28): data a unit reads or writes that nothing claims is the unit's to
    **claim and match** - claim the range in its own `splits.txt`, in the section the bytes live in, and
    reconstruct the bytes so they byte-match the target. The `extern` declaration is the finding, wherever
    it sits (`src/` file, `include/<module>/` header, or the `include/unsplit/` band: the band is rule 2's
    fallback for a symbol nobody can claim, not the answer for data a unit demonstrably uses).

    Only the `extern` keyword is read, and only symbols the map types as an object: a function declaration
    is rule 2's. The `Ownership` index is the same one rule 2 resolves through, so an `owned` address is
    clean - which is the declare-never-define carve-out (playbook 29: the range is already the unit's own,
    so defining the constants would rebuild the pool) - while a name absent from the map or a duplicate row
    is left alone (rule 2's `Ownership.gaps` is the one place those are counted). Rule 2 may name the same
    line: rule 2 says whose header the declaration belongs in, rule 12 says the bytes must be claimed.
    """
    if ownership is None:
        return []
    out = []
    for name, _pos, line in extern_declarations(src):
        r = ownership.resolve(name)
        if r is None or r["kind"] != "unsplit" or r.get("type") != "object":
            continue
        out.append(_finding(src, 12, line,
                            "`%s` is unowned data - no registered range covers `%s:0x%X`; the unit that "
                            "uses it claims the range in its own `splits.txt` and matches the bytes "
                            "(rule 12)" % (name, r["section"], r["address"])))
    return out


# --------------------------------------------------------------------------------------------------
# `--diff`: an owed rename is not ownership the batch created
# --------------------------------------------------------------------------------------------------

def unresolved_declarations(src: Source, ownership: "Ownership") -> set[str]:
    """The declaration names in `src` that the map cannot resolve - the file's rule-2 **gaps**.

    Rule 2 reports *nothing* for these (`rule2_findings` counts them in `Ownership.gaps` and refuses to
    guess at a name with no row), so a file's gap names are invisible in the findings list - and they are
    exactly the signal `--diff` needs.  A file that stops spelling an unmapped name has **completed** a
    rename the base map had already made (the old spelling was the referrer half); a file that only ever
    adds declarations gives up no gap at all.  That is what keeps a credit from excusing a new violation.
    """
    out: set[str] = set()
    for name, _line in header_declarations(src):
        resolved = ownership.resolve(name)
        if resolved is None or resolved["kind"] == "dup":
            out.add(name)
    return out


def unresolved_declarations_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                                   ownership: "Ownership") -> dict[str, set]:
    """`{path now: gap names}` for the **ref's** copies of the changed files (the `before` side)."""
    out: dict[str, set] = {}
    for before, after in pairs:
        if before is None:
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, before)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out[after] = unresolved_declarations(Source(before, after, text), ownership)
    return out


def owed_rename_completion(finding: dict, base: "Ownership | None", working: "Ownership") -> bool:
    """Whether this rule-2 finding is the **referrer half of a rename the base map already made**.

    The incident (2026-09-28).  `cb7d49aaa` renamed four map rows (`getGameSpyInterfaceThread` ->
    `GameSpyInterfaceThread_getInstance`, `clearPatInterface` -> `PatInterface_clear`,
    `isPatInterfaceReady` -> `PatInterface_isReady`, `lbl_80794CE4` -> `sGameSpyInterfaceThread`) and
    landed the map alone; the referrers kept spelling the old names.  Completing that rename is the *only*
    way the two units can ever link - but `--diff` judges each side by the map it was written against, and
    the base copy's *old* spelling resolves to nothing at all, so the corrected spelling read as an
    addition: `+1 rule 2 include/Network/fn_8041A87C.h (3 -> 4)`, `+3 ... NetworkWiiMediator.cpp (39 ->
    42)` - a pure artefact, the same class as the `+62` rename artefact `load_ownership_at_ref` fixed.

    The judgement is therefore by the **ownership of the address**, not by the spelling of the name: a
    working-side declaration whose symbol resolves to an address that the **base map already carried, with
    the same owner** is not an addition - it is the other half of a rename the base made.  Three things
    still refuse, and they are the reason this is a narrow rule rather than a blanket one:

    * the address has **no row in the base map** - the batch newly registered the range (or added the row),
      so the ownership *is* the batch's, which is exactly what `--diff` exists to charge;
    * the base row at that address is owned by a **different** unit (or is in a different module) - the
      declaration is foreign for a reason the base did not have;
    * the name resolves to nothing in the working map either (a genuine gap, `Ownership.gaps`) - there is no
      address to compare, so the finding stands.
    """
    if base is None:
        return False
    name = finding.get("symbol")
    if not name:
        return False
    working_r = working.resolve(name)
    if working_r is None or working_r["kind"] not in ("owned", "unsplit"):
        return False
    section, address = working_r.get("section"), working_r.get("address")
    if section is None or address is None:
        return False
    base_r = base.resolution_at(section, address)
    if base_r is None or base_r["kind"] != working_r["kind"]:
        return False
    if working_r["kind"] == "owned":
        return base_r.get("unit") == working_r.get("unit")
    return base_r.get("module") == working_r.get("module")


def rename_credits(findings: list[dict], base: "Ownership | None", working: "Ownership",
                   base_symbols: "dict[str, set] | None" = None,
                   freed: "dict[str, int] | None" = None) -> dict[tuple[int, str], int]:
    """`(rule, file) -> how many of that pair's findings are an owed rename's referrer half.

    Three conditions, each of which a genuinely new declaration fails:

    * `base_symbols` - the BASE copy's rule-2 symbols per file: a symbol the file already declared is part
      of `before`, not one of this batch's findings, so it can never be credited;
    * `owed_rename_completion` - the symbol must resolve to an address the base map owned, for the same
      owner (or the same unsplit module);
    * `freed` - how many unmapped names the file **stopped spelling** (`unresolved_declarations`): each
      credit costs one, so a file that adds a foreign declaration while giving up no gap is refused.
    """
    credits: dict[tuple[int, str], int] = {}
    if base is None:
        return credits
    known = base_symbols or {}
    for f in findings:
        if f.get("rule") != 2 or not owed_rename_completion(f, base, working):
            continue
        if f.get("symbol") in known.get(f["file"], ()):
            continue
        key = (2, f["file"])
        if credits.get(key, 0) >= (freed or {}).get(f["file"], 0):
            continue
        credits[key] = credits.get(key, 0) + 1
    return credits


def apply_rename_credits(added: list[dict], findings: list[dict], base: "Ownership | None",
                         working: "Ownership",
                         base_symbols: "dict[str, set] | None" = None,
                         freed: "dict[str, int] | None" = None) -> tuple[list[dict], dict]:
    """`(added, credits)`: subtract the owed-rename credits from `added`, dropping pairs that reach zero.

    Only rule 2 is credited (`rename_credits`), so no other rule's growth is ever excused; the credit can
    never exceed what the count comparison reported as added (a subtraction from a positive delta); and a
    pair is kept, with its reduced count, as soon as one genuine finding remains - a file that completes an
    owed rename *and* adds a foreign declaration still refuses, and the refusal names the file.
    """
    credits = rename_credits(findings, base, working, base_symbols, freed)
    if not credits:
        return added, {}
    out, used = [], {}
    for a in added:
        key = (a["rule"], a["file"])
        c = credits.get(key, 0)
        if c:
            # report only what was actually subtracted: a file whose credit found no addition to cancel
            # (its count fell, or never rose) must not appear in the credit lines
            used[key] = min(c, a["added"])
        left = a["added"] - c
        if left > 0:
            out.append(dict(a, added=left))
    return out, used


def rename_credit_lines(credits: dict[tuple[int, str], int],
                        freed: "dict[str, set] | None" = None) -> list[str]:
    """The credited findings as printable lines - a credit is **said**, never a silent tolerance.

    The line names the unmapped names the file stopped spelling, so a reader can check the judgement
    instead of trusting it: those names are the base's half of the rename.
    """
    lines = []
    for (rule, path), n in sorted(credits.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        gave_up = sorted((freed or {}).get(path, ()))[:3]
        more = ""
        gone = ""
        if gave_up:
            gone = "; stopped spelling %s%s" % (", ".join(gave_up),
                                                 " (+%d more)" % (len((freed or {})[path]) - 3)
                                                 if len((freed or {})[path]) > 3 else "")
        lines.append("  ~%d rule %d  %s  (completing a rename the base map already made: same address, "
                     "same owner at base%s)" % (n, rule, path, gone))
    return lines


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
# Rule 7's data half: dtk's stems for an unrenamed data label - `lbl_XXXXXXXX` and its `loc_XXXXXXXX`
# sibling. Ownership is deliberately **not** consulted any more: a label left generated is a finding in
# every file that spells it, own, foreign or unowned alike.
RULE7_LBL_RE = re.compile(r"\b(?:lbl|loc)_[0-9A-Fa-f]{8}\b")
# The `rule 7 deferred: <reason>` spelling is no longer a key - a comment exempts nothing. The regex is
# kept because `land.py`'s `rule7_defer_growth` still refuses a batch that *adds* the escape (a second,
# stricter row on top of rule 7 firing on the generated names themselves). `[ \t]*` rather than `\s*`
# keeps the declaration and its non-empty reason on one line.
RULE7_DEFER_RE = re.compile(r"rule[ \t]*7[ \t]+deferred[ \t]*:[ \t]*\S")
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


def _rule2_finding(src: Source, line: int, name: str, detail: str) -> dict:
    """A rule-2 finding, carrying the declared **symbol** as data and not only inside the message.

    `--diff` judges an added rule-2 finding by the *address* its symbol resolves to, so the name has to
    survive as a field (the `detail` string is prose meant for a human); `owed_rename_completion` reads it.
    """
    return dict(_finding(src, 2, line, detail), symbol=name)


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
RULE11_MARKER_RE = re.compile(r"untyped\s*:\s*([^\n]*)")
# A statement head that opens a `{` but is not a declaration: a control-flow block, or a macro/keyword that
# is an expression at file scope (`static_assert(sizeof(void*) == 4)`). The rule must not read such a
# parenthesised expression as a parameter list.
RULE11_NON_DECL_HEADS = _DECL_KEYWORDS | {
    "catch", "alignof", "__alignof", "static_assert", "_Static_assert", "assert", "asm", "__asm",
}


def match_paren(code: str, open_pos: int) -> int:
    """Index of the `)` matching the `(` at `open_pos`, or -1."""
    depth = 0
    for i in range(open_pos, len(code)):
        if code[i] == "(":
            depth += 1
        elif code[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def _mask_preproc(code: str) -> str:
    """Blank every preprocessor line (and its continuations), positions and newlines preserved.

    A macro body is not a declaration and its braces need not be balanced, so leaving one in place could
    corrupt the scope walk. Blanking keeps a reported line number exact.
    """
    out = []
    cont = False
    for line in code.split("\n"):
        directive = cont or line.lstrip().startswith("#")
        cont = directive and line.rstrip().endswith("\\")
        out.append(" " * len(line) if directive else line)
    return "\n".join(out)


def _declared_name_pos(segment: str) -> tuple[str | None, int]:
    """`(name, position)` for the declaration `segment`, using rule 2's `_declared_name` for the name.

    The position is recovered with the same discrimination `_declared_name` uses: a function pointer
    (`(*name)` or `(*name(`) names the identifier inside the parens, anything else names the last
    identifier before the first `(`. Reusing the parser keeps rule 11 from disagreeing with rule 2 about
    what a declaration's name is.
    """
    name = _declared_name(segment)
    if name is None:
        return None, -1
    i = segment.find("(")
    if i >= 0:
        tail = segment[i:]
        for rx in (r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", r"\(\s*\*\s*([A-Za-z_]\w*)\s*\("):
            m = re.match(rx, tail)
            if m and m.group(1) == name:
                return name, i + m.start(1)
        k = i
        while k > 0 and (segment[k - 1].isascii() and (segment[k - 1].isalnum() or segment[k - 1] == "_")):
            k -= 1
        return name, k
    m = re.search(r"\b%s\b" % re.escape(name), segment)
    return (name, m.start()) if m else (None, -1)


def _declarator_parens(segment: str, name_pos: int, name_len: int) -> tuple[int, int] | None:
    """`(open, close)` of a function declarator's parameter list, or None when this is not a function.

    A direct declarator has `(` right after the name; a function-pointer declarator has `)` (closing
    `(*name)`) before its own `(`. The text after `)` must be empty, a qualifier, a constructor-init `:`,
    or a trailing `{`/`;` (the delimiters are outside `segment`), which is what rejects a variable
    initializer (`void (*cb)(void *) = 0;`) and an expression (`static_assert`) rather than reading its
    parenthesised operand as a parameter list.
    """
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


def _declaration_from(src: Source, start: int, end: int, code: str) -> dict | None:
    """The function declaration `code[start:end]` describes, or None when the segment is not one."""
    seg = code[start:end]
    name, name_pos = _declared_name_pos(seg)
    if name is None or name in RULE11_NON_DECL_HEADS:
        return None
    parens = _declarator_parens(seg, name_pos, len(name))
    if parens is None:
        return None
    open_pos, close_pos = parens
    first = start + (len(seg) - len(seg.lstrip()))
    return {"name": name, "pos": start + name_pos, "line": src.line_of(start + name_pos),
            "start_line": src.line_of(first),
            "end_line": src.line_of(start + close_pos),
            "params": seg[open_pos + 1:close_pos], "params_pos": start + open_pos + 1,
            "ret": seg[:name_pos], "ret_pos": start}


def function_declarations(src: Source) -> list[dict]:
    """Every function declaration/definition header in the file, with its parameter and return text.

    The walk is rule 2's `header_declarations` shape (brace depth decides scope, a linkage block is
    transparent), with the scope stack extended so a declaration inside a `namespace`/`class` is seen too
    while a statement inside a function **body** is not: that is what keeps a cast out of the rule. A `{`
    ends a statement wherever it is (an inline method body sits at class scope), and the declarator check
    separates a function header from a type body, an initializer and a control block.
    """
    code = _mask_preproc(src.code)
    out: list[dict] = []
    scopes: list[str] = []          # "function" for a body, "other" for a type/namespace/block, "linkage"
    stmt_start = 0
    for i, c in enumerate(code):
        if c == "{":
            if _LINKAGE_OPEN_RE.search(code[stmt_start:i]):
                scopes.append("linkage")
            else:
                decl = _declaration_from(src, stmt_start, i, code)
                if decl is not None:
                    decl["body"] = (i, match_brace(code, i))
                    scopes.append("function")
                    out.append(decl)
                else:
                    scopes.append("other")
            stmt_start = i + 1
        elif c == "}":
            if scopes:
                scopes.pop()
            stmt_start = i + 1
        elif c == ";":
            if "function" not in scopes:
                decl = _declaration_from(src, stmt_start, i, code)
                if decl is not None:
                    out.append(decl)
            stmt_start = i + 1
    return out


def _untyped_marker(src: Source, start_line: int, end_line: int) -> str | None:
    """The reason of a `/* untyped: <reason> */` marker on the declaration or the line above it.

    The window is `start_line - 1 .. end_line`, exactly the owner's "on the declaration or the line above
    it". The line above must be a **standalone** marker (its code view is blank): a trailing marker on the
    previous declaration's own line is that declaration's, not the next one's, so one marker can never
    exempt two declarations. Per-file keys are gone (rule 7's removal is the precedent): a file may never
    exempt itself.
    """
    for line in range(max(1, start_line - 1), end_line + 1):
        seg_start = src._starts[line - 1]
        seg_end = src._starts[line] if line < len(src._starts) else len(src.comments)
        m = RULE11_MARKER_RE.search(src.comments[seg_start:seg_end])
        if not m:
            continue
        if line == start_line - 1 and src.code[seg_start:seg_end].strip():
            continue
        reason = m.group(1)
        if reason.endswith("*/"):
            reason = reason[:-2]
        return reason.strip()
    return None


def _untyped_reason_ok(reason: str) -> bool:
    """Whether a marker's reason names a genuinely-untyped case rather than restating the ban."""
    return bool(reason.strip()) and any(rx.search(reason) for rx in RULE11_REASON_RES)


def _split_parameters(code: str, start: int, end: int) -> list[tuple[str, int]]:
    """`(chunk, absolute_offset)` for each top-level comma-separated parameter in `code[start:end]`."""
    out = []
    depth = 0
    chunk_start = start
    for i in range(start, end):
        c = code[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            out.append((code[chunk_start:i], chunk_start))
            chunk_start = i + 1
    out.append((code[chunk_start:end], chunk_start))
    return out


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
                out.append(_finding(src, 11, src.line_of(first), _RULE11_PARAM_DETAIL))
        if RULE11_VOID_PTR_RE.search(d["ret"]):
            out.append(_finding(src, 11, d["line"], _RULE11_RET_DETAIL))
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



def lint_source(src: Source, ownership: "Ownership | None" = None) -> list[dict]:
    """All section 6.5 findings for one file, in rule then line order.

    `ownership` carries the symbols.txt + splits.txt index for rule 2; when it is None (the map is
    absent, or a caller that only wants the source-local rules), rule 2 is not reported for `src`.

    The unsplit band is declaration-only glue, so for a file under `include/unsplit/` only rule 2 runs -
    and there it means the opposite of its `src/` reading: a symbol a registered unit owns must not be
    declared here.
    """
    out: list[dict] = []
    if is_unsplit_header(src.rel):
        if ownership is not None:
            out.extend(rule2_band_findings(src, ownership))
            out.extend(rule12_findings(src, ownership))
        out.sort(key=lambda f: (f["rule"], f["line"]))
        return out
    if is_shared_header(src.rel):
        # an ordinary `include/` header: rules 2 and 12 are the section-6.5 rules it carries. Rules 3-9 are
        # body/`src/` rules, and rules 10/11 for headers are reported by `header_pragma_findings` and
        # `header_rule11_findings` rather than here (2026-09-28). Rule 12 is here because a header is where
        # the unowned data a `src/` unit reads is declared (2026-09-28).
        if ownership is not None:
            out.extend(rule2_header_findings(src, ownership))
            out.extend(rule12_findings(src, ownership))
        out.sort(key=lambda f: (f["rule"], f["line"]))
        return out
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

    def in_field(pos: int) -> bool:
        return any(a <= pos < b for a, b in field_spans)

    # Rule 7 is unconditional: no path is exempt, a file with no bodies is held to it, and a
    # `rule 7 deferred` comment is inert. Every generated spelling fires, whoever owns the symbol.
    for m in RULE7_FN_RE.finditer(src.code):
        out.append(_finding(src, 7, src.line_of(m.start()), "auto-generated name `%s`" % m.group(0)))
    for m in RULE7_UNK_RE.finditer(src.code):
        if not in_field(m.start()):
            out.append(_finding(src, 7, src.line_of(m.start()), "bare `%s` identifier" % m.group(0)))
    for m in RULE7_LBL_RE.finditer(src.code):
        out.append(_finding(src, 7, src.line_of(m.start()),
                            "data label `%s` - name it from what it holds and where it is used, and "
                            "rename the map row" % m.group(0)))

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
        out.extend(rule12_findings(src, ownership))

    out.extend(rule11_findings(src))

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


def codegen_pragma_findings(src: "Source") -> list[dict]:
    """Rule 10 for one file: a codegen pragma in a **shared header**.  The path gate is here, not in the
    caller, so a `.c`/`.cpp` can never be reported however this is invoked.  `src.code` blanks
    comments/literals, so a pragma *named* in a comment is not a finding - only a real directive is."""
    if not src.rel.replace("\\", "/").startswith(HEADERS + "/"):
        return []
    out = []
    for m in CODEGEN_PRAGMA_RE.finditer(src.code):
        out.append(_finding(src, 10, src.line_of(m.start()),
                            "codegen pragma `#pragma %s` in a shared header - state it in the "
                            "`.c`/`.cpp` that needs it, never in the header" % m.group(1)))
    return out


def header_files(root: str) -> list[str]:
    """Every shared header under `include/`, in path order (the unsplit band included - it is a header)."""
    out = []
    for dirpath, dirnames, filenames in os.walk(os.path.join(root, HEADERS)):
        dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
        for name in sorted(filenames):
            if name.endswith(HEADER_SUFFIXES):
                out.append(os.path.join(dirpath, name))
    return out


def header_pragma_findings(root: str) -> list[dict]:
    """Rule 10 over the whole shared-header tree (`include/`).  Not part of `lint_source`, which runs
    rules 3-9 on `src/` files and rule 2 on the band; a header is judged by this rule only."""
    out = []
    for path in header_files(root):
        out.extend(codegen_pragma_findings(Source(path, rel_of(root, path), read_text(path))))
    return out


def header_pragma_counts_at_ref(root: str, ref: str) -> dict:
    """Rule-10 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side."""
    out: dict = {}
    for path in git(root, "ls-tree", "-r", "--name-only", ref, "--", HEADERS).splitlines():
        if not path.endswith(HEADER_SUFFIXES):
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, path)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        for f in codegen_pragma_findings(Source(path, path, text)):
            out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def header_rule11_findings(root: str) -> list[dict]:
    """Rule 11 over the whole shared-header tree (`include/`), including the unsplit band.

    A shared header is a declaration a batch is judged against, and a `void *` parameter there is the same
    defect as one in a `src/` file. The band is covered here - `lint_source` returns early for it.
    """
    out = []
    for path in header_files(root):
        out.extend(rule11_findings(Source(path, rel_of(root, path), read_text(path))))
    return out


def header_rule12_findings(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Rule 12 over the whole shared-header tree (`include/`), the unsplit band included.

    A header declares the unowned data a `src/` unit reads (`include/Network/network_state.h`'s
    `sessionTimeoutParam` block, `include/unsplit/NetworkData.h`'s constants), so the finding has to be
    reachable there; `lint_source` returns early for both header classes, exactly as it does for rule 11.
    """
    if ownership is None:
        ownership = load_ownership(root)
    if ownership is None:
        return []
    out = []
    for path in header_files(root):
        rel = rel_of(root, path)
        out.extend(rule12_findings(Source(path, rel, read_text(path)), ownership))
    return out


def header_rule2_findings(root: str, ownership: "Ownership | None" = None) -> list[dict]:
    """Rule 2 over the shared-header tree (`include/`) - the non-unsplit headers.

    The unsplit band has its own reading (`rule2_band_findings`) and is reached through `lint_source`; this
    is the extension the `extern`-keyword, `src/`-only rule could not see: a foreign declaration in
    `include/<module>/*.h` (`include/Network/network_state.h`'s `setMediatorState68A` pair).  The map
    being absent leaves rule 2 unreported, exactly as it does for `src/`.
    """
    if ownership is None:
        ownership = load_ownership(root)
    if ownership is None:
        return []
    out = []
    for path in header_files(root):
        rel = rel_of(root, path)
        if is_unsplit_header(rel):
            continue
        out.extend(rule2_header_findings(Source(path, rel, read_text(path)), ownership))
    out.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    return out


def header_rule11_counts_at_ref(root: str, ref: str) -> dict:
    """Rule-11 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side."""
    out: dict = {}
    for path in git(root, "ls-tree", "-r", "--name-only", ref, "--", HEADERS).splitlines():
        if not path.endswith(HEADER_SUFFIXES):
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, path)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        for f in rule11_findings(Source(path, path, text)):
            out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def header_rule12_counts_at_ref(root: str, ref: str, ownership: "Ownership | None" = None) -> dict:
    """Rule-12 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side.

    Each side of a `--diff` is judged by the map it was written against (the rule-2 precedent): a rename
    that moves a data symbol out of a registered range must not read as a rule-12 addition.
    """
    if ownership is None:
        ownership = load_ownership_at_ref(root, ref)
    if ownership is None:
        return {}
    out: dict = {}
    for path in git(root, "ls-tree", "-r", "--name-only", ref, "--", HEADERS).splitlines():
        if not path.endswith(HEADER_SUFFIXES):
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, path)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        for f in rule12_findings(Source(path, path, text), ownership):
            out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def rule11_local_total(root: str) -> int:
    """Every `void *` local variable in `src/` and `include/` - the rule-11 scope note's count."""
    total = sum(rule11_local_count(s) for s in all_sources(root))
    for path in header_files(root):
        total += rule11_local_count(Source(path, rel_of(root, path), read_text(path)))
    return total


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
    out.extend(header_pragma_findings(root))
    out.extend(header_rule2_findings(root, ownership))
    out.extend(header_rule11_findings(root))
    out.extend(header_rule12_findings(root, ownership))
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
            "label_names": len(names(7, "data")),
            "unk_fields": len(names(5, "")), "types": len(names(3, "")),
            "shared_types": len(names(1, "")), "extern_symbols": len(names(2, "")),
            "mangled_names": len(names(9, "")), "unowned_data_symbols": len(names(12, ""))}


def rule_enforced(rule: int, rel: str, src: "Source | None" = None) -> bool:
    """Whether `rule` is enforced for `rel`. It always is: the rule-7 exemptions and per-file keys are
    gone (owner's ruling, 2026-09-27) and no other rule ever had one. Kept as an API because the brief
    and `promote.py` ask before they count."""
    return True


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
    p = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), (p.stderr or "").strip()))
    return p.stdout


def git_bytes(root: str, *args: str) -> bytes:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), p.stderr.decode("utf-8", "replace").strip()))
    return p.stdout


def changed_src_files(root: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_ref, path_now)` for every `src/` file and shared header the tree changed against `ref`.

    A rename is one entry carrying both names, so the file's violations are compared against its old
    copy rather than counting as new. The whole `include/` tree is included: rule 2 applies to an ordinary
    header too (a foreign declaration in `include/<module>/*.h`), so a batch that edits one must be judged
    against it - and the unsplit band is included because rule 2 applies to it as the fallback file.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", ref, "--",
                    SRC, HEADERS).splitlines():
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
    # `git diff` cannot see an untracked file, so a batch that lints BEFORE staging its new unit would be
    # checked against the wrong set - and a new unit's violations are all additions, so it is the one file
    # guaranteed to matter. This blind spot refused two units on 2026-09-25 whose own lint run had reported
    # "no new section 6.5 violation" (it had compared two headers main changed and not the unit at all).
    for path in git(root, "ls-files", "--others", "--exclude-standard", "--", SRC, HEADERS).splitlines():
        if path and path.endswith(SUFFIXES):
            out.append((None, path))
    return out


def findings_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                    ownership: "Ownership | None" = None) -> list[dict]:
    """The ref's copies of the changed files, linted, judged by the **ref's** rule-2 index.

    A file absent at the ref (`before is None`) contributes nothing: it is new, so every violation in it is
    an addition.  A rename is keyed by its new path (`Source(before, after, text)`), which keeps the two
    sides comparable.  `ownership` is the map that side was written against (`load_ownership_at_ref`), and
    the findings keep the paths they have now, so a caller can line them up with the working side's.

    The findings themselves are returned rather than their counts because a `--diff` credit is a judgement
    about a *finding*: `rename_credits` reads the `symbol` rule 2 attaches and the address it resolves to.
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
    return findings


def changed_src_files_between(root: str, base: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_base, path_at_ref)` for every `src/`/`include/` file `ref` changed against `base`.

    The read-only counterpart of `changed_src_files`: both sides come from git objects, so a **held branch**
    can be judged without checking it out and without touching the working tree.  There is no untracked-file
    pass here - a commit has no untracked files.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", base, ref, "--",
                    SRC, HEADERS).splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        status, paths = parts[0], parts[1:]
        after = paths[-1]
        if status.startswith("R"):
            before = paths[0]
        elif status.startswith("A"):
            before = None
        else:
            before = after
        if after.endswith(SUFFIXES):
            out.append((before, after))
    return out


def findings_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                    ownership: "Ownership | None" = None) -> list[dict]:
    """The ref's copies of the changed files, linted - the `after` side of a comparison against a ref tree.

    Unlike `findings_at_ref` (the `before` side, where a file the base did not carry contributes nothing),
    every `after` path exists at `ref`, including one the comparison added.
    """
    findings = []
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(after, after, text), ownership))
    return findings


def unresolved_declarations_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                                   ownership: "Ownership") -> dict[str, set]:
    """`{path: gap names}` for the **ref's** copies of the changed files (the `after` side)."""
    out: dict[str, set] = {}
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out[after] = unresolved_declarations(Source(after, after, text), ownership)
    return out


def sources_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> list["Source"]:
    """The ref's copies of the changed files as `Source`s (for the rename-credit gap comparison)."""
    out = []
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out.append(Source(after, after, text))
    return out


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
                           % ("unresolved band" if m == UNSPLIT_UNRESOLVED else m, n,
                              len(ownership.unsplit_symbols.get(m, ())))
                           for m, n in sorted(ownership.unsplit_modules.items()))))
    for reason, n in sorted(ownership.gaps.items()):
        print("rule 2 gap: %d declaration site(s) - %s (documented, not guessed)" % (n, reason))
    if not ownership.foreign_units and not ownership.unsplit_modules and not ownership.gaps:
        print("rule 2: every extern declaration was judged")


def print_budget(findings: list[dict], ownership: "Ownership | None" = None,
                 root: str | None = None) -> None:
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
          "rule 5 %d field(s), rule 7 %d fn_* + %d unk identifier(s) + %d data label(s), "
          "rule 9 %d mangled name(s)"
          % (u["shared_types"], u["extern_symbols"], u["types"], u["unk_fields"], u["fn_names"],
             u["unk_identifiers"], u["label_names"], u["mangled_names"]))
    print("%d finding(s) over %d unit(s), %d file(s) with findings"
          % (b["findings"], len(b["units"]), len(source_files_of(findings))))
    r11 = [f for f in findings if f["rule"] == 11]
    print("rule 11 (banned outright): %d finding(s) over %d file(s) with a `void *` parameter/return type"
          % (len(r11), len(source_files_of(r11))))
    if root is not None:
        print("rule 11 note: %d `void *` local variable(s) - out of the rule's scope, counted so the owner "
              "can decide" % rule11_local_total(root))
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

    # --- rule 7 has no exemption: every generated name in src/ fires, everywhere -------------------
    auto = "src/auto/802B2978_fn_802B2978.c"
    check("rule7: a src/auto/ file's own fn_ name fires (the bucket exemption is gone)",
          lines_of("void fn_802B2978(void) {}\n", 7, auto), [1])
    check("rule7: a src/auto/ body calling fn_ fires",
          lines_of("void fn_802B2978(void) {\n    fn_80040598();\n}\n", 7, auto), [1, 2])
    check("rule7: a src/auto/ bare unk local fires too",
          lines_of("void fn_802B2978(void) {\n    u32 unk4 = 0;\n}\n", 7, auto), [1, 2])
    check("rule7: a subdirectory of src/auto/ fires (no path exempts anything)",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto/deep/x.c"), [1])
    check("rule7: the same shape under src/Pl/ fires",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/Pl/pl_act.cpp"), [1])
    check("rule7: only rule 7 ever had an exemption - rule 4 still fires under src/auto/",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4, auto), [3])
    check("rule7: the EXEMPT table is empty (every path is enforced)", EXEMPT, [])
    check("rule7: the reported exemption list is empty", exemptions(), [])
    check("rule7: rule_enforced is True everywhere, with or without the file",
          [rule_enforced(7, "src/auto/x.c"),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "/* stub: only a header comment */\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c",
                         "/* rule 7 deferred: the map has no name */\nvoid fn_80040598(void) {}\n")),
           rule_enforced(6, "src/auto/x.c"), rule_enforced(7, "src/auto\\x.c")],
          [True, True, True, True, True])

    # --- rule 7 key 2 is gone: a file with no bodies is held to it too ----------------------------
    pl = "src/Pl/pl_act.cpp"
    check("rule7 bodyless: a fn_ prototype fires with no body in the file",
          lines_of("void fn_802B2978(void);\n", 7, pl), [1])
    check("rule7 bodyless: a fn_ call with no body fires",
          lines_of("fn_80040598();\n", 7, pl), [1])
    check("rule7 bodyless: a bare unk declaration fires",
          lines_of("u32 unk4;\n", 7, pl), [1])
    check("rule7 bodyless: adding a body changes nothing about rule 7",
          lines_of("void fn_802B2978(void) {}\n", 7, pl), [1])
    check("rule7 bodyless: rule 6 still fires without a body",
          lines_of("u32 v = *(u32*)((u8*)p + 4);\n", 6, pl), [1])
    check("rule7 bodyless: rule 8 still fires without a body",
          lines_of("goto out;\n", 8, pl), [1])

    # --- rule 7 key 3 is gone: no comment exempts anything ----------------------------------------
    defer = "/* rule 7 deferred: the map has only fn_XXXXXXXX for this range */\n"
    check("rule7 deferred: the comment does not hide a fn_ definition",
          lines_of(defer + "void fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: the comment does not hide a fn_ call",
          lines_of(defer + "void f(void) {\n    fn_80040598();\n}\n", 7, pl), [3])
    check("rule7 deferred: the comment does not hide a bare unk local",
          lines_of(defer + "void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [3])
    check("rule7 deferred: a line-comment spelling is inert too",
          lines_of("// rule 7 deferred: the map has no name\nvoid fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: the spelling inside a string is not even a comment",
          lines_of('const char* s = "rule 7 deferred: x";\nvoid fn_802B2978(void) {}\n', 7, pl), [2])
    check("rule7 deferred: a comment naming a generated symbol is not itself a finding",
          lines_of("/* fn_80040598 unk4 lbl_80010000 */\nvoid f(void) {}\n", 7), [])
    check("rule7 deferred: rules 1-6 and 8 still fire in a file carrying the comment",
          [r for r, _l in rules_of(
              defer
              + "/* size: 0x8 */\nstruct A {\n    u32 x;\n    /* +0x04 */ u32 unk04;\n};\n"
              + "void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n    goto out;\nout:\n    return;\n}\n",
              pl)],
          [4, 5, 6, 7, 8])
    # The dead-key proof on the real incident file: `src/ef/eft053.cpp` carries a `rule 7 deferred:`
    # comment AND unrenamed `fn_` references, and must still report every one of them. If this ever goes
    # quiet the comment is suppressing again - which is the defect this whole change exists to remove.
    if os.path.exists("src/ef/eft053.cpp"):
        real = [f for f in lint_source(Source("src/ef/eft053.cpp", "src/ef/eft053.cpp",
                                              read_text("src/ef/eft053.cpp"))) if f["rule"] == 7]
        check("rule7 deferred: the real ef/eft053.cpp (comment + generated names) still reports rule 7",
              bool(real), True)
        check("... and every finding is a generated name, not an unk field",
              all(f["detail"].startswith(("auto-generated name", "data label", "bare ")) for f in real), True)

    # --- rule 7: the data-label half fires on every lbl_/loc_ reference ----------------------------
    lbl = Ownership({"lbl_80010000": [(".data", 0x80010000, "object")],
                     "loc_80010120": [(".data", 0x80010120, "object")],
                     "lbl_80020000": [(".data", 0x80020000, "object")],
                     "lbl_80030000": [(".data", 0x80030000, "object")]},
                    {".data": [(0x80010000, 0x80010200, "mod/a.c"),
                               (0x80020000, 0x80020100, "mod/b.c")]})
    own_lbl = "void f(void) {\n    u32 v = (u32)lbl_80010000;\n}\n"
    check("rule7 lbl: this unit's own lbl_ fires",
          lines_of(own_lbl, 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the finding names the label and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/mod/a.c", own_lbl), lbl) if f["rule"] == 7],
          ["data label `lbl_80010000` - name it from what it holds and where it is used, and rename the "
           "map row"])
    check("rule7 lbl: loc_ is the same unrenamed-data stem and fires",
          lines_of("void f(void) {\n    u32 v = (u32)loc_80010120;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the owning unit's header is covered too",
          lines_of(own_lbl, 7, "src/mod/a.h", lbl), [2])
    check("rule7 lbl: another unit's label fires in this file too (no own/foreign split)",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_80020000;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the same label in a non-owner file fires",
          lines_of(own_lbl, 7, "src/other/c.c", lbl), [2])
    check("rule7 lbl: an unowned label fires",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_80030000;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: a label the map does not know fires",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_DEADBEEF;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: a bodyless file's lbl_ declaration fires (key 2 is gone)",
          lines_of("extern u32 lbl_80010000;\n", 7, "src/mod/a.c", lbl), [1])
    check("rule7 lbl: no ownership index is needed for the lbl_ half",
          lines_of(own_lbl, 7, "src/mod/a.c", None), [2])
    check("rule7 lbl: the advertised name is counted as a distinct label",
          unique_names([f for f in lint_source(Source("x", "src/mod/a.c", own_lbl), lbl)
                        if f["rule"] == 7])["label_names"], 1)

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
    check("rule2: an unowned symbol whose brackets disagree is a finding (no module guessed)",
          lines_of("extern void gap(void);\n", 2, "src/other/c.c", gap), [1])
    check("rule2: the unresolved-band detail names the band directory, not a header",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void gap(void);\n"), gap)
           if f["rule"] == 2],
          ["`gap` has no registered owner - declare it in a header under `include/unsplit/`"])
    check("rule2: the unresolved band is not counted as a gap", sum(gap.gaps.values()), 0)
    check("rule2: the unresolved band is reported under the sentinel module",
          sorted(gap.unsplit_modules), [UNSPLIT_UNRESOLVED])
    check("rule2: extern declarations are found by the scanner",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void a(void);\nextern u16 b[2];\nextern void (*c)(int);\n"))], ["a", "b", "c"])
    check("rule2: a function-pointer parameter is not the declared name",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void* f(s32 a, void (*cb)(void));\n"))], ["f"])
    check("rule2: a function returning a function pointer is named",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void (*f(int))(void);\n"))], ["f"])

    # --- rule 2 extended: plain prototypes in `src/` and foreign declarations in a module header -----
    # The `extern`-keyword-only scanner could not see a plain prototype (the Network scope's real sites),
    # and rule 2 never ran on a non-unsplit header at all.  Both are judged now.
    check("rule2 prototype: a plain prototype for another unit's symbol is a finding",
          lines_of("void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2 prototype: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2 prototype: a prototype for a symbol the file owns is clean",
          lines_of("void foo(void);\n", 2, "src/mod/a.c", idx), [])
    check("rule2 prototype: a prototype inside a body is not a file-scope declaration",
          lines_of("void f(void) {\n    void (*fp)(void);\n}\n", 2, "src/other/c.c", idx), [])
    check("rule2 prototype: a variable statement is not a prototype",
          lines_of("u32 g_var;\n", 2, "src/other/c.c", idx), [])
    check("rule2 prototype: `static` does not exempt the site",
          lines_of("static void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2 prototype: the extern scanner and the prototype scanner never report one site twice",
          sorted(n for n, _p, _l in declaration_sites(Source("x", "x.c",
              "extern void foo(void);\n"))), ["foo"])

    # `_owns` must accept the owner's *public* header, or extending rule 2 to headers reports the owner's
    # own declarations - ~30 false rows in the Network scope alone (the review note).
    check("_owns: the unit's source", _owns("src/mod/a.c", "mod/a.c"), True)
    check("_owns: the unit's private header", _owns("src/mod/a.h", "mod/a.c"), True)
    check("_owns: the unit's public header", _owns("include/mod/a.h", "mod/a.c"), True)
    check("_owns: a header in a deeper include path", _owns("include/sub/mod/a.h", "mod/a.c"), True)
    check("_owns: another unit's header is not owned", _owns("include/other/a.h", "mod/a.c"), False)
    check("_owns: the proof from the review note",
          _owns("include/NHTTP/NHTTP_bgnend.h", "NHTTP/NHTTP_bgnend.c"), True)

    # an ordinary `include/<module>/*.h` header: a foreign declaration is a finding, the owner's own
    # header and an unowned symbol are not (the module header carries public names the map has not split).
    mhdr = "include/mod/user.h"
    check("rule2 header: another unit's owned symbol is a finding",
          lines_of("void foo(void);\n", 2, mhdr, idx), [1])
    check("rule2 header: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", mhdr, "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2 header: the owner's own header is clean",
          lines_of("void foo(void);\n", 2, "include/mod/a.h", idx), [])
    check("rule2 header: an `extern` declaration is judged too",
          lines_of("extern u16 bar[2];\n", 2, mhdr, idx), [1])
    check("rule2 header: an unowned symbol is left to the band, not reported here",
          lines_of("void mid(void);\n", 2, mhdr, mid), [])
    check("rule2 header: a definition is not a declaration",
          lines_of("void foo(void) {\n}\n", 2, mhdr, idx), [])
    check("rule2 header: a type forward declaration is not a symbol declaration",
          lines_of("struct Vec;\n", 2, mhdr, idx), [])
    check("rule2 header: only rule 2 applies, so a fn_ prototype is not rule 7",
          rules_of("void fn_80040598(void);\n", mhdr, idx), [])

    # --- rule 2 in the unsplit band: an owned symbol must not be declared there --------------------
    band = "include/unsplit/mod.h"
    check("rule2 band: another unit's owned symbol declared in the band is a finding",
          lines_of("void foo(void);\n", 2, band, idx), [1])
    check("rule2 band: the detail names the owner and the same fix",
          [f["detail"] for f in lint_source(Source("x", band, "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2 band: the extern \"C\" linkage wrapper is transparent",
          lines_of('extern "C" {\nvoid foo(void);\n}\n', 2, band, idx), [2])
    check("rule2 band: a semicolon in a comment does not split a declaration",
          lines_of("/* a; b */\nvoid foo(void);\n", 2, band, idx), [2])
    check("rule2 band: an unowned symbol stays in the band",
          lines_of("void mid(void);\n", 2, band, mid), [])
    bandmid = Ownership({"mid": [(".text", 0x2500, "function")]},
                        {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2 band: an unowned name is not turned into a counted gap",
          (lines_of("void mid(void);\n", 2, band, bandmid), sum(bandmid.gaps.values()))[1], 0)
    check("rule2 band: a name not in the map stays in the band",
          lines_of("void not_in_map(void);\n", 2, band, idx), [])
    check("rule2 band: a definition is not a declaration",
          lines_of("void foo(void) {\n}\n", 2, band, idx), [])
    check("rule2 band: a type forward declaration is not a symbol declaration",
          lines_of("struct Vec;\n", 2, band, idx), [])
    check("rule2 band: only rule 2 applies, so a fn_ prototype is not rule 7",
          rules_of("void fn_80040598(void);\n", band, idx), [])
    check("rule2 band: a plain prototype in a src/ file is now rule 2's too",
          lines_of("void foo(void);\n", 2, "src/other/c.c", idx), [1])

    # --- rule 12: an `extern` of data no registered range claims is the finding ---------------------
    # Owner's ruling 2026-09-28: the unit that reads/writes the bytes claims the range and matches it, so
    # the declaration is the finding - in a `src/` file, a module header, or the unsplit band.
    dat = Ownership({"lbl_8079C7D8": [(".sdata2", 0x8079C7D8, "object")],
                     "maskedUserName": [(".sdata", 0x80793968, "object")],
                     "lbl_805FA908": [(".data", 0x805FA908, "object")],
                     "fn_80010000": [(".text", 0x80010000, "function")],
                     "owned_data": [(".data", 0x1500, "object")]},
                    {".data": [(0x1000, 0x2000, "mod/a.c")],
                     ".sdata2": [(0x80800000, 0x80800100, "mod/a.c")]})
    check("rule12: an extern of unowned data is a finding",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "src/other/c.c", dat), [1])
    check("rule12: the detail names the address the claim covers",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c",
              "extern const u16 lbl_8079C7D8;\n"), dat) if f["rule"] == 12],
          ["`lbl_8079C7D8` is unowned data - no registered range covers `.sdata2:0x8079C7D8`; the unit that "
           "uses it claims the range in its own `splits.txt` and matches the bytes (rule 12)"])
    check("rule12: an extern of OWNED data is clean (the declare-never-define carve-out)",
          lines_of("extern u32 owned_data;\n", 12, "src/other/c.c", dat), [])
    check("rule12: a function declaration is rule 2's, never rule 12's",
          lines_of("extern void fn_80010000(void);\n", 12, "src/other/c.c", dat), [])
    check("rule12: an array of unowned data is a finding too",
          lines_of("extern const char maskedUserName[7];\n", 12, "src/other/c.c", dat), [1])
    check("rule12: a module header is judged (the unowned data is declared there)",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "include/mod/user.h", dat), [1])
    check("rule12: the unsplit band is judged too - the band is not the answer for data a unit uses",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "include/unsplit/mod.h", dat), [1])
    check("rule12: a name not in the map is not a finding",
          lines_of("extern u32 not_in_map;\n", 12, "src/other/c.c", dat), [])
    check("rule12: a definition is not a declaration",
          lines_of("const u16 lbl_8079C7D8 = 1;\n", 12, "src/other/c.c", dat), [])
    check("rule12: no map means unchecked, not a crash",
          [f for f in lint_source(Source("x", "src/other/c.c",
                                         "extern const u16 lbl_8079C7D8;\n"), None)
           if f["rule"] == 12], [])
    check("rule12: an unsplit FUNCTION is rule 2's, not rule 12's",
          lines_of("extern void mid(void);\n", 12, "src/other/c.c", mid), [])
    check("rule12 and rule 2 both name an unowned data extern (different remedies)",
          sorted(r for r, _l in rules_of("extern const u16 lbl_8079C7D8;\n", "src/other/c.c", dat)),
          [2, 7, 12])

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
    check("budget: unique data labels", b["unique"]["label_names"], 0)
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
    # the grandfather, proved on a *header* rule-2 count (the behaviour that had to survive the extension):
    # an existing finding never blocks, an added one refuses.
    hdr_before = {(2, "include/mod/a.h"): 2}
    check("diff grandfather: a header's pre-existing rule-2 findings are not additions when untouched",
          diff_deltas(hdr_before, hdr_before), [])
    check("... even when the file is rewritten but the count does not rise",
          diff_deltas(hdr_before, {(2, "include/mod/a.h"): 2}), [])
    check("... and one added rule-2 finding in that header refuses",
          [(x["rule"], x["file"], x["added"]) for x in
           diff_deltas(hdr_before, {(2, "include/mod/a.h"): 3})], [(2, "include/mod/a.h", 1)])

    # --- `--diff`: the OWNERSHIP OF THE ADDRESS, not the spelling of the name -----------------------
    # The 2026-09-28 incident.  `cb7d49aaa` renamed four map rows and landed the map alone; completing the
    # rename in the referrers then read as "+4 added rule-2 violations", because the base copy's *old*
    # spelling resolves to nothing at all while the corrected one resolves (owned or unsplit).  A credit is
    # granted only for the other half of a rename the **base map already made**, and three conditions bound
    # it: the name is new to the file, the address was owned at base for the same owner (or the same unsplit
    # module), and the file gave up an unmapped name to pay for it.

    ADDR = 0x803D6A98
    site = "include/Network/fn_8041A87C.h"
    finding = {"rule": 2, "file": site, "line": 419,
               "text": "void* GameSpyInterfaceThread_getInstance(void);", "detail": "(detail)",
               "symbol": "GameSpyInterfaceThread_getInstance"}
    new_map = Ownership({"GameSpyInterfaceThread_getInstance": [(".text", ADDR, "function")]},
                        {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    old_map = Ownership({"getGameSpyInterfaceThread": [(".text", ADDR, "function")]},
                        {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    no_row = Ownership({}, {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    not_yet_registered = Ownership({"GameSpyInterfaceThread_getInstance": [(".text", ADDR, "function")]},
                                   {})
    someone_else = Ownership({"patched_at_this_address": [(".text", ADDR, "function")]},
                             {".text": [(0x803D0000, 0x803D8000, "Network/other.cpp")]})
    check("address view: the map resolves an address to the row's owner, by address",
          new_map.resolution_at(".text", ADDR)["unit"], "Network/fn_803D3CE8.cpp")
    check("... and answers None for an address it carries no row at",
          new_map.resolution_at(".text", 0x803D7000), None)

    # (a) completing an owed rename -> PASS
    check("rename credit: a completion resolves to an address the base map already owned",
          owed_rename_completion(finding, new_map, new_map), True)
    check("... and the base map's row NAME is irrelevant - the address is the identity",
          owed_rename_completion(finding, old_map, new_map), True)
    added_row = {"rule": 2, "file": site, "added": 1, "before": 3, "after": 4}
    check("... so the addition is credited",
          apply_rename_credits([added_row], [finding], new_map, new_map, {site: {"DWCi_htons"}}, {site: 1}),
          ([], {(2, site): 1}))
    check("... and the same holds when the map rename rode the same batch",
          apply_rename_credits([added_row], [finding], old_map, new_map, {site: {"DWCi_htons"}}, {site: 1}),
          ([], {(2, site): 1}))
    check("... reported, never silent",
          rename_credit_lines({(2, site): 1}, {site: {"getGameSpyInterfaceThread"}}),
          ["  ~1 rule 2  %s  (completing a rename the base map already made: same address, same owner at "
           "base; stopped spelling getGameSpyInterfaceThread)" % site])

    # (b) a genuinely new foreign declaration at an address UNOWNED at base -> still REFUSE
    check("rename credit: no row at that address in the base map is not a completion",
          owed_rename_completion(finding, no_row, new_map), False)
    check("... nor is a row whose range the base had not registered (unsplit there)",
          owed_rename_completion(finding, not_yet_registered, new_map), False)
    check("... so the batch still adds a violation",
          apply_rename_credits([added_row], [finding], no_row, new_map, {site: {"DWCi_htons"}},
                               {site: 1}),
          ([added_row], {}))

    # (c) the same address owned at base by a DIFFERENT owner -> still REFUSE
    check("rename credit: an address the base map owned for somebody else is not a completion",
          owed_rename_completion(finding, someone_else, new_map), False)
    check("... so the batch still adds a violation",
          apply_rename_credits([added_row], [finding], someone_else, new_map, {site: {"DWCi_htons"}},
                               {site: 1}),
          ([added_row], {}))

    # (d) both spellings were map rows at each side: the count does not rise, and nothing is credited
    check("rename credit: a count that does not rise is untouched",
          diff_deltas({(2, site): 1}, {(2, site): 1}), [])
    check("... and a credit with nothing to subtract is not reported",
          apply_rename_credits([], [finding], old_map, new_map, {}, {site: 1}), ([], {}))

    # the three bounds, each of which a genuinely new declaration fails
    check("rename credit: a symbol the base copy of the file already declared is never credited",
          rename_credits([finding], old_map, new_map, {site: {finding["symbol"]}}, {site: 1}), {})
    check("... nor is one when the file gave up no unmapped name",
          rename_credits([finding], old_map, new_map, {}, {site: 0}), {})
    two = [finding, dict(finding, symbol="PatInterface_clear")]
    check("... and each credit costs one freed gap",
          rename_credits(two, old_map, new_map, {}, {site: 1}), {(2, site): 1})
    check("... an unrelated rule is never credited",
          apply_rename_credits([{"rule": 7, "file": site, "added": 1, "before": 0, "after": 1}],
                               [dict(finding, rule=7)], old_map, new_map, {}, {site: 1}),
          ([{"rule": 7, "file": site, "added": 1, "before": 0, "after": 1}], {}))
    check("... and a file that also adds a foreign declaration still refuses, by name",
          apply_rename_credits([dict(added_row, added=2, after=5)], [finding], old_map, new_map,
                               {site: {"DWCi_htons"}}, {site: 1}),
          ([dict(added_row, added=1, after=5)], {(2, site): 1}))
    # the gap a credit is paid with is a name the map cannot resolve, read from the file itself
    check("rename credit: an unmapped declaration is a gap",
          unresolved_declarations(Source("x", site, "void getGameSpyInterfaceThread(void);\n"), new_map),
          {"getGameSpyInterfaceThread"})
    check("... and a resolved one is not",
          unresolved_declarations(Source("x", site, "void GameSpyInterfaceThread_getInstance(void);\n"),
                                  new_map), set())
    check("... so completing the rename frees exactly one",
          len(unresolved_declarations(Source("x", site, "void getGameSpyInterfaceThread(void);\n"), new_map)
              - unresolved_declarations(Source("x", site,
                                               "void GameSpyInterfaceThread_getInstance(void);\n"),
                                        new_map)), 1)

    # --- rule 10: a codegen pragma belongs to a TU, not to a shared header ------------------------
    hdr = "include/stage/fn_802B2AA0.h"

    def pragmas_of(text: str, rel: str = hdr) -> list[int]:
        return [f["line"] for f in codegen_pragma_findings(Source("x", rel, text))]

    check("rule10: peephole in a shared header is a finding", pragmas_of("#pragma peephole off\n"), [1])
    check("rule10: optimization_level in a shared header is a finding",
          pragmas_of("#pragma optimization_level 2\n"), [1])
    check("rule10: fp_contract in a shared header is a finding",
          pragmas_of("#pragma fp_contract on\n"), [1])
    check("rule10: an indented pragma is still found", pragmas_of("    #pragma peephole off\n"), [1])
    check("rule10: a pragma in a .cpp is not reported",
          codegen_pragma_findings(Source("x", "src/enemy/em019_prog.cpp", "#pragma peephole off\n")), [])
    check("rule10: a pragma in a .c is not reported",
          codegen_pragma_findings(Source("x", "src/foo.c", "#pragma peephole off\n")), [])
    check("rule10: a commented pragma is clean", pragmas_of("/* #pragma peephole off */\n"), [])
    check("rule10: a pragma named in a line comment is clean",
          pragmas_of("// #pragma peephole off\n"), [])
    check("rule10: `#pragma once` is not a codegen pragma", pragmas_of("#pragma once\n"), [])
    check("rule10: `#pragma pack` is not a codegen pragma", pragmas_of("#pragma pack(4)\n"), [])
    check("rule10: the finding names the pragma and the fix",
          [f["detail"] for f in codegen_pragma_findings(Source("x", hdr, "#pragma peephole off\n"))],
          ["codegen pragma `#pragma peephole` in a shared header - state it in the `.c`/`.cpp` that "
           "needs it, never in the header"])
    check("rule10: the report is a rule-10 finding",
          [f["rule"] for f in codegen_pragma_findings(Source("x", hdr, "#pragma peephole off\n"))], [10])
    if os.path.exists(hdr):
        check("rule10: the motivating header is clean after the fix",
              codegen_pragma_findings(Source(hdr, hdr, read_text(hdr))), [])

    # --- rule 11: no `void *` parameter or return type -------------------------------------------
    check("rule11: an unmarked void* parameter is a finding",
          lines_of("void f(void *p) {\n}\n", 11), [1])
    check("rule11: an unmarked void* return type is a finding",
          lines_of("void *f(void) {\n}\n", 11), [1])
    check("rule11: a prototype is judged too", lines_of("void f(void *p);\n", 11), [1])
    check("rule11: a `void **` return is still a void pointer",
          lines_of("void **f(void) {\n}\n", 11), [1])
    check("rule11: `const void *` is a void pointer",
          lines_of("void f(const void *src) {\n}\n", 11), [1])
    check("rule11: two void* parameters are two findings",
          lines_of("void f(void *a, void *b) {\n}\n", 11), [1, 1])
    check("rule11: a parameter and a return are both reported",
          lines_of("void *f(void *p) {\n}\n", 11), [1, 1])
    check("rule11: void alone is clean", lines_of("void f(void) {\n}\n", 11), [])
    check("rule11: a real parameter type is clean",
          lines_of("void f(Vec *out, u32 n) {\n}\n", 11), [])
    check("rule11: a function pointer parameter with a clean signature is clean",
          lines_of("void f(void (*cb)(int)) {\n}\n", 11), [])
    check("rule11: a `void*` local's parenthesised cast is not read as a type",
          lines_of("void f(void) {\n    u32 v = (u32)(void *)p;\n}\n", 11), [])
    check("rule11: a file-scope initializer cast is clean",
          lines_of("void *p = (void *)0;\n", 11), [])
    check("rule11: a parameter on a continuation line is reported on its own line",
          lines_of("void f(s32 a, void *first,\n       void *second);\n", 11), [1, 2])
    check("rule11: a function returning a function pointer is judged",
          lines_of("void (*f(void *self, int n))(void);\n", 11), [1])
    check("rule11: a file-scope void* variable is not a parameter/return",
          lines_of("void *g_buffer;\n", 11), [])
    check("rule11: a static_assert operand is not a parameter list",
          lines_of("static_assert(sizeof(void *) == 4);\n", 11), [])
    check("rule11: a macro body is not a declaration",
          lines_of("#define PTR ((void *)0)\nvoid f(void) {\n}\n", 11), [])
    check("rule11: a call-shaped control block is not a declaration",
          lines_of("void f(void) {\n    while (memcmp(p, (void *)q, 4)) {\n    }\n}\n", 11), [])
    check("rule11: a namespace-scoped definition is judged",
          lines_of("namespace nw4r {\nvoid f(void *p) {\n}\n}\n", 11), [2])
    check("rule11: a class-scoped inline method is judged",
          lines_of("class A {\npublic:\n    void f(void *p) {\n    }\n};\n", 11), [3])

    # the marker: per-declaration, on the declaration or the line above it
    check("rule11 marker: on the line above exempts the declaration",
          lines_of("/* untyped: opaque handle */\nvoid f(void *h) {\n}\n", 11), [])
    check("rule11 marker: trailing on the same line exempts it",
          lines_of("void f(void *h); /* untyped: opaque handle */\n", 11), [])
    check("rule11 marker: a memcpy-shaped byte range is an accepted reason",
          lines_of("/* untyped: memcpy-shaped byte range */\nvoid f(void *dst) {\n}\n", 11), [])
    check("rule11 marker: a caller-owned payload is an accepted reason",
          lines_of("/* untyped: caller-owned payload */\nvoid f(void *data) {\n}\n", 11), [])
    check("rule11 marker: an empty reason is still a finding",
          lines_of("/* untyped: */\nvoid f(void *p) {\n}\n", 11), [2])
    check("rule11 marker: a vague reason is still a finding",
          lines_of("/* untyped: TODO, it is untyped */\nvoid f(void *p) {\n}\n", 11), [2])
    check("rule11 marker: a marker in a string is not a marker",
          lines_of('const char* s = "untyped: opaque handle";\nvoid f(void *p) {\n}\n', 11), [2])
    check("rule11 marker: a marker on an unrelated declaration does not leak",
          lines_of("/* untyped: opaque handle */\nvoid g(void *h);\n\nvoid f(void *p) {\n}\n", 11), [4])
    check("rule11 marker: a trailing marker on the previous declaration does not exempt the next",
          lines_of("void g(void *h); /* untyped: opaque handle */\nvoid f(void *p);\n", 11), [2])
    check("rule11 marker: each declaration needs its own marker",
          lines_of("/* untyped: opaque handle */\nvoid g(void *h);\n"
                   "/* untyped: opaque handle */\nvoid f(void *p);\n", 11), [])

    # the scope note: a `void *` local is out of scope, only counted
    check("rule11 locals: a local void* is not a finding but is counted",
          (lines_of("void f(void) {\n    void *p = 0;\n}\n", 11),
           rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    void *p = 0;\n}\n"))), ([], 1))
    check("rule11 locals: a cast is not counted",
          rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    u32 v = (u32)(void *)p;\n}\n")), 0)
    check("rule11 locals: a parameter is not counted",
          rule11_local_count(Source("x.c", "x.c", "void f(void *p) {\n}\n")), 0)
    check("rule11 locals: a nested block is not counted twice",
          rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    if (1) {\n        void *p = 0;\n    }\n}\n")), 1)

    # --- end-to-end over the fixtures -------------------------------------------------------------
    check("e2e: rule list is complete", sorted(RULE_NAMES), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
    check("e2e: no rule is declared unchecked", UNCHECKED, [])
    check("e2e: findings sort by rule then line",
          rules_of(text), sorted(rules_of(text)))

    # --- `--diff` judges each side by its own map -------------------------------------------------
    # Before this, a rename read as a regression: the base copy still said the old name, the *working* map
    # no longer resolved it, so the base side's rule-2 findings vanished and the delta called them additions
    # (a real batch measured "+62 added rule-2 violations" for a pure rename). Stub the single call that
    # reads REF, so the check needs no repository.
    ref_map = "zzz_selftest_symbol = .text:0x80040598; // type:function size:0x8\n"
    ref_splits = "Pl/pl_act.cpp:\n\t.text\tstart:0x80040598 end:0x800405A0\n"
    real_git_bytes = globals()["git_bytes"]
    try:
        globals()["git_bytes"] = lambda root, *a: (
            ref_splits if a[-1].endswith("splits.txt") else ref_map).encode()
        ref_own = load_ownership_at_ref(".", "HEAD")
    finally:
        globals()["git_bytes"] = real_git_bytes
    check("the ref's own map resolves a name the working map cannot",
          ref_own is not None and ref_own.resolve("zzz_selftest_symbol") is not None, True)
    working = load_ownership(".")
    check("...and the working map really cannot resolve it",
          working is None or working.resolve("zzz_selftest_symbol"), None)
    check("an absent ref map falls back rather than dying", load_ownership_at_ref(".", ""), working)

    # --- `--diff REF`: the ref the comparison actually uses ----------------------------------------
    # `--diff main` in a branch whose main has moved must measure *this batch*, so a ref that is not an
    # ancestor resolves to the merge base and says so. 2026-09-27: a lane diagnosed another lane's landing
    # this way, and the orchestrator had to pass the merge base by hand three times in one night.
    import contextlib
    import io
    with tempfile.TemporaryDirectory() as tmp:
        def sgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)

        def rev(where: str = "HEAD") -> str:
            return subprocess.run(["git", "rev-parse", where], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        sgit("init", "-q")
        sgit("checkout", "-q", "-b", "main")
        open(os.path.join(tmp, "a.txt"), "w").write("1\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "base")
        cut = rev()
        sgit("checkout", "-q", "-b", "lane")
        open(os.path.join(tmp, "b.txt"), "w").write("2\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "the batch's own work")
        sgit("checkout", "-q", "main")
        open(os.path.join(tmp, "c.txt"), "w").write("3\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "another lane landed")
        moved_main = rev()
        sgit("checkout", "-q", "lane")
        check("an ancestor ref is used exactly as given", _resolve_diff_ref(tmp, cut), cut)
        check("... and so is HEAD", _resolve_diff_ref(tmp, "HEAD"), "HEAD")
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            resolved = _resolve_diff_ref(tmp, "main")
        check("a ref that is not an ancestor resolves to the merge base", resolved, cut)
        check("... not to the ref itself", resolved == moved_main, False)
        check("... and the run says which base it used",
              "merge base" in buf.getvalue() and cut[:12] in buf.getvalue(), True)
        check("... so the gate's own ancestor base is silent",
              _resolve_diff_ref(tmp, cut) == cut and buf.getvalue().count("merge base"), 1)

    # --- `--ref BRANCH`: judge a held branch in read-only mode ---------------------------------------
    # 2026-09-28: a claim lane re-implemented the tool's before/after merge in ~40 lines of scratch to
    # prove a `splits.txt` claim cleared a held branch's rows, and the reproduction was only approximately
    # trusted.  `--ref` reads both sides from git, so the branch is never checked out and the working tree
    # is never touched.  The rows must be the rows `--diff` would print with the branch checked out.
    with tempfile.TemporaryDirectory() as tmp:
        def rgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)

        def rput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def rrev(where: str = "HEAD") -> str:
            return subprocess.run(["git", "rev-parse", where], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        rgit("init", "-q")
        rgit("checkout", "-q", "-b", "main")
        rput("config/RMHE08/symbols.txt",
             "owned_fn = .text:0x80002000; // type:function size:0x10\n"
             "unowned_data = .data:0x80003000; // type:object size:0x10\n")
        rput("config/RMHE08/splits.txt", "other/other_unit.c:\n\t.text       start:0x80002000 end:0x80002010\n")
        rput("src/other/other_unit.c", "void owned_fn(void) {}\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "base")
        base_sha = rrev()
        # the held branch adds two declarations that are findings only there
        rgit("checkout", "-q", "-b", "held")
        rput("src/held/held_unit.c", "extern void owned_fn(void);\nextern u8 unowned_data[];\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "held branch adds the declarations")
        # main moves on, so `held` is nobody's ancestor and the merge base is `base_sha`
        rgit("checkout", "-q", "main")
        rput("src/main_moved.c", "int main_moved;\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "main moved on")
        head_before, tree_before = rrev(), sorted(os.listdir(os.path.join(tmp, "src")))
        own = load_ownership(tmp)
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_ref = ref_comparison(tmp, "held", own, as_json=False)
        text_out = out.getvalue()
        check("--ref judges a held branch and fails on a real addition", (rc_ref, "rule 2" in text_out), (1, True))
        check("--ref reports the rule-12 row too", "rule 12" in text_out, True)
        check("--ref names the branch it judged", "held" in text_out, True)
        check("--ref is read-only: HEAD did not move", rrev(), head_before)
        check("... and the working tree is untouched", sorted(os.listdir(os.path.join(tmp, "src"))), tree_before)
        check("... the branch's file was never materialised",
              os.path.exists(os.path.join(tmp, "src/held/held_unit.c")), False)
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_ref_json = ref_comparison(tmp, "held", own, as_json=True)
        ref_data = json.loads(out.getvalue())
        check("--ref --json names the branch and its merge base",
              (ref_data["ref"], ref_data["base"]), ("held", base_sha))
        check("... and lists the added rows exactly",
              sorted((a["rule"], a["file"], a["added"]) for a in ref_data["added"]),
              [(2, "src/held/held_unit.c", 2), (12, "src/held/held_unit.c", 1)])
        # reproduce the rows `--diff <base>` prints with the branch CHECKED OUT - the strongest proof
        rgit("checkout", "-q", "held")
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_diff = main(["--diff", base_sha, "--json"])
            diff_data = json.loads(out.getvalue())
        finally:
            os.chdir(old_cwd)
        check("--ref reproduces --diff's rows for the branch",
              sorted((a["rule"], a["file"], a["added"]) for a in diff_data["added"]),
              sorted((a["rule"], a["file"], a["added"]) for a in ref_data["added"]))
        check("... and the exit codes agree", (rc_ref_json, rc_diff), (1, 1))
        rgit("checkout", "-q", "main")

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
def _resolve_diff_ref(root: str, ref: str) -> str:
    """The ref `--diff` actually compares against: REF when it is an ancestor of HEAD, else its merge base.

    `--diff REF` compares the working tree with REF, so when REF is not an ancestor of HEAD the diff also
    contains whatever landed on REF after this branch was cut - which reads as *this* batch's regression.
    Measured 2026-09-27: a lane spent a diagnosis on another lane's landing exactly this way, and the same
    night the orchestrator had to be told `--diff <merge-base>` by hand three times. The merge base is the
    only base that measures *this* batch, so it is selected automatically and named on stderr. The land gate
    passes the batch base, which is an ancestor by construction, so its comparison is untouched.
    """
    if subprocess.run(["git", "merge-base", "--is-ancestor", ref, "HEAD"],
                      cwd=root, capture_output=True).returncode == 0:
        return ref
    base = subprocess.run(["git", "merge-base", ref, "HEAD"], cwd=root,
                          capture_output=True, text=True, encoding="utf-8", errors="replace").stdout.strip()
    if not base:
        print("stylelint: warning: %s is not an ancestor of HEAD and has no merge base with it - comparing "
              "against %s itself" % (ref, ref), file=sys.stderr)
        return ref
    print("stylelint: %s is not an ancestor of HEAD, so the comparison uses the merge base %s - only that "
          "base measures this batch" % (ref, base[:12]), file=sys.stderr)
    return base


def ref_comparison(root: str, branch: str, ownership: "Ownership | None", as_json: bool) -> int:
    """Judge a **held branch** read-only: its committed tree against the merge base `--diff` would use.

    `--diff REF` compares the *working tree* with REF, so it cannot judge a branch that is not checked out -
    the `after` side would be whatever the worktree happens to hold, and a lane had to re-implement this
    comparison in ~40 lines of scratch (2026-09-28) to prove a `splits.txt` claim cleared a held branch's
    rows.  `--ref B` reads **both** sides from git objects: `B` is the `after` tree and
    `_resolve_diff_ref(root, B)` (the merge base, resolved exactly as `--diff` resolves a non-ancestor REF)
    is the `before`.  Nothing is checked out and nothing is written; each side is judged by the map it was
    written against, exactly as `--diff` judges them.
    """
    base = _resolve_diff_ref(root, branch)
    pairs = changed_src_files_between(root, base, branch)
    rels = [after for _before, after in pairs]
    base_ownership = load_ownership_at_ref(root, base) or ownership
    after_ownership = load_ownership_at_ref(root, branch) or ownership
    base_findings = findings_at_ref(root, base, pairs, base_ownership)
    # the base copy's rule-2 symbols, so a rename credit can only ever touch a name *new* to the file
    base_symbols: dict = {}
    for f in base_findings:
        if f.get("rule") == 2 and f.get("symbol"):
            base_symbols.setdefault(f["file"], set()).add(f["symbol"])
    base_gaps = unresolved_declarations_at_ref(root, base, pairs, base_ownership)
    before = merge_counts(
        rule_counts(base_findings),
        rule1_counts_at_ref(root, base, pairs),
        header_pragma_counts_at_ref(root, base),
        header_rule11_counts_at_ref(root, base),
        header_rule12_counts_at_ref(root, base, base_ownership))
    touched = findings_of_ref(root, branch, pairs, after_ownership)
    after_sources = sources_of_ref(root, branch, pairs)
    freed_gaps = {src.rel: base_gaps.get(src.rel, set()) - unresolved_declarations(src, after_ownership)
                  for src in after_sources}
    after = merge_counts(
        rule_counts(touched),
        rule1_counts_at_ref(root, branch, []),
        header_pragma_counts_at_ref(root, branch),
        header_rule11_counts_at_ref(root, branch),
        header_rule12_counts_at_ref(root, branch, after_ownership))
    added, credits = apply_rename_credits(diff_deltas(before, after), touched, base_ownership,
                                         after_ownership, base_symbols,
                                         {p: len(names) for p, names in freed_gaps.items()})
    credit_lines = rename_credit_lines(credits, freed_gaps)
    if as_json:
        print(json.dumps({"ref": branch, "base": base, "added": added, "changed": rels,
                          "rename_credits": [{"rule": r, "file": p, "count": n,
                                              "stopped_spelling": sorted(freed_gaps.get(p, ()))}
                                             for (r, p), n in sorted(credits.items())],
                          "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                          "exempt": exemptions()}, indent=2))
    elif added:
        print("stylelint: %s adds %d section 6.5 violation(s) over %d changed file(s):"
              % (branch, sum(a["added"] for a in added), len(rels)))
        for a in added:
            print("  +%d rule %d  %s  (%d -> %d)" % (a["added"], a["rule"], a["file"], a["before"],
                                                      a["after"]))
        for line in credit_lines:
            print(line)
        for num, what in UNCHECKED:
            print("  not checked (cross-file): rule %d - %s" % (num, what))
        for rule, prefix, why in EXEMPT:
            print("  not enforced: rule %d under %s (%s)" % (rule, prefix, why))
        for cond, why in RULE7_NOTES:
            print("  not enforced: rule 7 for %s (%s)" % (cond, why))
    else:
        print("stylelint: %s adds no section 6.5 violation over %d changed file(s) (read-only: judged "
              "against %s, nothing checked out)" % (branch, len(rels), base[:12]))
        for line in credit_lines:
            print(line)
    return 1 if added else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Lint src/ against docs/plan.md 6.5 (roadmap 7.21).")
    ap.add_argument("--diff", metavar="REF",
                    help="fail only if the working tree adds a violation relative to REF")
    ap.add_argument("--ref", metavar="BRANCH",
                    help="read-only: judge the named branch's committed tree against its merge base, so a "
                         "held branch can be checked without checking it out (same comparison as --diff)")
    ap.add_argument("--budget", action="store_true", help="report the backlog per unit over src/")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()

    if args.ref is not None and args.diff is not None:
        ap.error("--ref and --diff are two different comparisons; pass one")

    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True, encoding="utf-8", errors="replace")
    root = root.stdout.strip() if root.returncode == 0 else os.getcwd()
    ownership = load_ownership(root)

    if args.ref is not None:
        return ref_comparison(root, args.ref, ownership, args.json)

    if args.diff is not None:
        args.diff = _resolve_diff_ref(root, args.diff)
        try:
            pairs = changed_src_files(root, args.diff)
            rels = [after for _before, after in pairs]
            # Each side against its own map: the working tree's index cannot resolve a name this batch
            # renamed, so using it here turned every rename into a batch of phantom rule-2 additions.
            base_ownership = load_ownership_at_ref(root, args.diff) or ownership
            base_findings = findings_at_ref(root, args.diff, pairs, base_ownership)
            before = merge_counts(
                rule_counts(base_findings),
                rule1_counts_at_ref(root, args.diff, pairs),
                header_pragma_counts_at_ref(root, args.diff),
                header_rule11_counts_at_ref(root, args.diff),
                header_rule12_counts_at_ref(root, args.diff,
                                            load_ownership_at_ref(root, args.diff) or ownership))
            # the base copy's rule-2 symbols, so a credit can only ever touch a name that is *new* to the
            # file: one it already declared is part of `before`, never one of the batch's additions
            base_symbols: dict = {}
            for f in base_findings:
                if f.get("rule") == 2 and f.get("symbol"):
                    base_symbols.setdefault(f["file"], set()).add(f["symbol"])
            base_gaps = unresolved_declarations_at_ref(root, args.diff, pairs, base_ownership)
        except RuntimeError as exc:
            print("stylelint: %s" % exc, file=sys.stderr)
            return 2
        # `lint_tree` covers the changed files only, which is exactly what a credit may consider: a finding
        # in a file the batch did not touch can never be one of its additions.  Kept as a list (not just
        # its counts) because a credit is a judgement about a *finding* - its symbol and its resolution -
        # and `diff_deltas` alone cannot tell a completed rename from a newly foreign declaration.
        touched = lint_tree(root, [os.path.join(root, a) for _b, a in pairs], ownership)
        # the gaps each file gave up: the old spelling of a renamed row is an *unmapped* name, so a file that
        # completed a rename has strictly fewer of them.  One credit costs one freed gap (see `rename_credits`).
        # The sources are rebuilt the way `lint_tree` builds them, so `rel` (the key both sides are compared
        # by) is spelled identically.
        after_sources = [Source(os.path.join(root, a), rel_of(root, os.path.join(root, a)),
                                read_text(os.path.join(root, a))) for _b, a in pairs]
        freed_gaps = {src.rel: base_gaps.get(src.rel, set()) - unresolved_declarations(src, ownership)
                      for src in after_sources}
        after = merge_counts(
            rule_counts(touched),
            rule_counts(rule1_findings(all_sources(root))),
            rule_counts(header_pragma_findings(root)),
            rule_counts(header_rule11_findings(root)),
            rule_counts(header_rule12_findings(root, ownership)))
        added, credits = apply_rename_credits(diff_deltas(before, after), touched, base_ownership,
                                             ownership, base_symbols,
                                             {p: len(names) for p, names in freed_gaps.items()})
        credit_lines = rename_credit_lines(credits, freed_gaps)
        if args.json:
            print(json.dumps({"ref": args.diff, "added": added, "changed": rels,
                              "rename_credits": [{"rule": r, "file": p, "count": n,
                                                  "stopped_spelling": sorted(freed_gaps.get(p, ()))}
                                                 for (r, p), n in sorted(credits.items())],
                              "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                              "exempt": exemptions()}, indent=2))
        elif added:
            print("stylelint: the batch adds %d section 6.5 violation(s) over %d changed file(s):"
                  % (sum(a["added"] for a in added), len(rels)))
            for a in added:
                print("  +%d rule %d  %s  (%d -> %d)" % (a["added"], a["rule"], a["file"], a["before"],
                                                          a["after"]))
            for line in credit_lines:
                print(line)
            for num, what in UNCHECKED:
                print("  not checked (cross-file): rule %d - %s" % (num, what))
            for rule, prefix, why in EXEMPT:
                print("  not enforced: rule %d under %s (%s)" % (rule, prefix, why))
            for cond, why in RULE7_NOTES:
                print("  not enforced: rule 7 for %s (%s)" % (cond, why))
        else:
            print("stylelint: no new section 6.5 violation over %d changed file(s) "
                  "(rule 2 resolves every extern to an owner or the unsplit band; rule 7 fires on every "
                  "auto-generated name; rule 11 fires on every unmarked `void *` parameter/return type; "
                  "rule 12 fires on every `extern` of unowned data; only pre-existing findings and a "
                  "rename's referrer half are grandfathered)" % len(rels))
            for line in credit_lines:
                print(line)
        return 1 if added else 0

    findings = lint_all(root, ownership)
    if args.json:
        print(json.dumps({"budget": budget(findings),
                          "rule11_locals": rule11_local_total(root),
                          "rule2_gaps": dict(ownership.gaps) if ownership else {},
                          "rule2_unsplit_modules": (
                              {m: {"sites": n, "symbols": len(ownership.unsplit_symbols.get(m, ()))}
                               for m, n in ownership.unsplit_modules.items()} if ownership else {}),
                          "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                          "exempt": exemptions()}, indent=2))
    elif args.budget:
        print_budget(findings, ownership, root)
    else:
        print_findings(findings)
    return 0


if __name__ == "__main__":
    sys.exit(main())
