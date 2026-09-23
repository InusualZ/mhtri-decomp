#!/usr/bin/env python3
"""The registry of shared types and helpers - what already exists, and who copied it.

The project's rule (AGENTS.md -> Conventions; docs/plan.md section 6.5 rule 1) is that a type more than
one unit needs is declared **once**, under `include/`, and included where needed - a declaration moves
there the *second* time a unit needs it. Nothing told a worker what already existed, so two units created
`include/ef.h` and `include/nw4r/math.h` independently while three others each defined the same
`VEC3`/`EfWork` locally and deferred the consolidation to nobody. This tool is the missing lookup: it
scans `include/**` and every `src/**` file, and answers, per declaration:

* **where it is defined** - every file (a header, a unit, or both);
* **which units use it** - a unit uses a declaration when it includes the defining header, names the
  declaration in its source, or owns a map symbol whose (mangled) name encodes it;
* **whether it is duplicated debt** - a shared header's name that a unit re-defines (rule 1), a name
  two units both define with no header owning it, or a name one unit defines that another names without
  including it (rule 1, before there is a header);
* **whether a helper prototype is repeated** - the same `extern` carried by several files (rule 2).

    python tools/units/typeregistry.py                     # the duplication report, human-readable
    python tools/units/typeregistry.py --json              # the same, machine-readable
    python tools/units/typeregistry.py --unit Pl/pl_act    # the shared headers that unit should use
    python tools/units/typeregistry.py --report <path.md>  # write the report to a file
    python tools/units/typeregistry.py --selftest

`brief.py` consumes `registry()` + `relevant_headers()` directly, so a worker's brief names the shared
headers its unit should reuse instead of re-create. The scan is textual by design - it is a *lookup* for
the worker, not a compiler - and every finding names the file and line a human can open.

What is scanned, and what a "declaration" is:

| kind | how it is found |
| --- | --- |
| `struct` / `union` / `enum` / `class` | the keyword plus its body; the tag and any trailing `typedef` alias are both recorded |
| `typedef` | a non-aggregate `typedef`; the declarator (including `(*fn)` function pointers and arrays) |
| `using` | a C++ alias (`using Foo = ...;`) |
| `inline` | an `inline`/`__inline` function definition (a helper meant to be shared) |
| `macro` | a `#define` (object- or function-like) |
| `extern` | a file-scope `extern` declaration (a helper prototype - rule 2's duplicate when two files both carry it) |

`include/types.h` is the project's scalar base: every unit including it is normal, so it is only reported
when a unit *redefines* one of its names, and `src/Camellia/camellia.c` is the documented vendor
exception (EXCEPTIONS below).
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import posixpath
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(HERE))

SHARED_DIR = "include"
UNIT_DIR = "src"
HEADER_SUFFIXES = (".h", ".hpp", ".hh")
SOURCE_SUFFIXES = (".c", ".cpp", ".cp", ".cc", ".h", ".hpp", ".hh")

# The scalar base every unit includes. Its names are deliberately not offered as "use this header"
# advice (they would drown the real finding); only a *redefinition* is debt.
BASE_HEADERS = ("include/types.h",)

# Deliberate duplicates: AGENTS.md / types.h's own header says these copies must stay.
EXCEPTIONS = {
    "src/Camellia/camellia.c": "vendor file mirrors upstream and keeps its own typedefs (include/types.h)",
}

# Words that show up in a declarator but are never the declared name.
NOT_NAMES = {
    "const", "volatile", "struct", "union", "enum", "class", "signed", "unsigned", "int", "char",
    "short", "long", "float", "double", "void", "typedef", "restrict", "register", "static", "extern",
    "inline", "typename", "__attribute__", "attribute", "packed", "aligned", "__declspec", "constexpr",
}

TYPE_KEYWORD = re.compile(r"\b(typedef\s+)?(struct|union|enum|class)\b")
SIMPLE_TYPEDEF = re.compile(r"(?m)^[ \t]*typedef[ \t]+(?!struct\b|union\b|enum\b|class\b)([^;{}]+);")
USING = re.compile(r"(?m)^[ \t]*using[ \t]+([A-Za-z_]\w*)[ \t]*=")
MACRO = re.compile(r"(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)")
INLINE = re.compile(r"\b(inline|__inline)\b[^;{}()]*?\b([A-Za-z_]\w*)[ \t]*\(([^;{}]*)\)[ \t]*\{")
EXTERN = re.compile(r"(?m)^[ \t]*extern[ \t]+([^;{}]+);")
IDENT = re.compile(r"[A-Za-z_]\w*")


# ------------------------------------------------------------------------------------------------------------------
# text helpers
# ------------------------------------------------------------------------------------------------------------------

def strip_comments(text: str) -> str:
    """Blank comments and string/char literal bodies, preserving length and every newline.

    A line number derived from the result is the original line (the same contract `stylelint.py` keeps),
    which is what makes a finding openable. Strings are blanked too: a string never contains a declarator,
    and leaving its contents in would only add junk tokens to the usage scan.
    """
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in ('"', "'"):
            quote = c
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\\":
                    out[i] = " "
                    i += 1
                if i < n and text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] in ("/", "*"):
            if text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "
                    i += 1
                continue
            out[i] = out[i + 1] = " "
            i += 2
            while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                if text[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                i += 1
            continue
        i += 1
    return "".join(out)


def identifiers(text: str) -> set:
    """Every identifier word in already-cleaned text."""
    return set(IDENT.findall(text))


def tokens_of_name(name: str) -> set:
    """The identifier tokens a (possibly mangled) symbol name encodes.

    MWCC's mangling length-prefixes each component (`Q34nw4r4math4VEC3`), so a plain identifier split
    never sees `VEC3`. This peels every `<digits><exactly that many chars>` component, which is what makes
    a map symbol like `Pl_get_gunner_pos__FP4_PLWQ34nw4r4math4VEC3l` count as a use of `VEC3`.
    """
    out = identifiers(name)
    i, n = 0, len(name)
    while i < n:
        m = re.match(r"\d+", name[i:])
        if m:
            count = int(m.group(0))
            j = i + len(m.group(0))
            token = name[j:j + count]
            if len(token) == count and re.match(r"[A-Za-z_]", token or " "):
                out.add(token)
                i = j + count
                continue
        i += 1
    return out


def _match_brace(text: str, open_index: int) -> int:
    depth = 0
    for j in range(open_index, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return j
    return len(text) - 1


def _split_semicolons(body: str) -> list:
    """Split an aggregate body on the `;`s that sit at bracket depth 0."""
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
    """A field-type fingerprint of an aggregate body, insensitive to field names.

    `Vec { f32 x; f32 y; f32 z; }` and `VEC3 { f32 x; f32 y; f32 z; }` share `f32 f32 f32`, which is how
    the report shows one 3-float vector recomputed under three names. Padding arrays keep their type
    (`u8[]`), a function pointer becomes `fnptr`, and a nested aggregate `agg`.
    """
    parts = []
    for stmt in _split_semicolons(body):
        stmt = stmt.strip()
        if not stmt:
            continue
        if "{" in stmt:
            parts.append("agg")
            continue
        if re.search(r"\(\s*\*", stmt):
            parts.append("fnptr")
            continue
        dims = re.findall(r"\[[^\]]*\]", stmt)
        core = re.sub(r"\[[^\]]*\]", "", stmt)
        ids = [x for x in IDENT.findall(core) if x not in NOT_NAMES]
        if not ids:
            continue
        parts.append((" ".join(ids[:-1]) or "?") + "[]" * len(dims))
    return " ".join(parts)


def declarator_name(region: str) -> str | None:
    """The name a `typedef`/`extern` declarator introduces.

    Handles the three shapes the tree actually carries: `unsigned int u32` (last identifier), a function
    pointer `void (*EfPmSpawn)(...)` (the identifier inside `(*...)`), and an array
    `KEY_TABLE_TYPE[CAMELLIA_TABLE_WORD_LEN]` (the identifier before `[`).
    """
    region = re.sub(r"\[[^\]]*\]", " ", region)
    m = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", region)
    if m:
        return m.group(1)
    region = re.sub(r"\([^()]*\)", " ", region)
    ids = [x for x in IDENT.findall(region) if x not in NOT_NAMES]
    return ids[-1] if ids else None


def alias_names(tail: str) -> list:
    """The alias name(s) after a `}` closing an aggregate definition, in order."""
    return [x for x in IDENT.findall(tail) if x not in NOT_NAMES]


# ------------------------------------------------------------------------------------------------------------------
# declaration scanning
# ------------------------------------------------------------------------------------------------------------------

def extract_decls(clean: str, rel: str, shared: bool) -> list:
    """Every declaration in one already-cleaned file, as `Decl` dicts.

    One entry per name per file: a `typedef struct Foo { ... } Foo;` records `Foo` once (as `struct`).
    A name may still appear in several files - that is exactly what the duplication report measures.
    """
    decls, seen = [], set()

    def add(name, kind, pos, shape=""):
        if not name or name in NOT_NAMES or name in seen:
            return
        seen.add(name)
        decls.append({"name": name, "kind": kind, "file": rel,
                      "line": clean.count("\n", 0, pos) + 1, "shape": shape, "shared": shared})

    for m in TYPE_KEYWORD.finditer(clean):
        is_typedef, keyword = bool(m.group(1)), m.group(2)
        j = m.end()
        tm = re.match(r"[ \t\r\n]*([A-Za-z_]\w*)", clean[j:])
        tag = tm.group(1) if tm else None
        after = j + (tm.end() if tm else 0)
        k = after
        while k < len(clean) and clean[k] in " \t\r\n":
            k += 1
        if k < len(clean) and clean[k] == "{":
            end = _match_brace(clean, k)
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
            nm = declarator_name(region)
            if nm:
                add(nm, "typedef", m.start())

    for m in SIMPLE_TYPEDEF.finditer(clean):
        nm = declarator_name(m.group(1))
        if nm:
            add(nm, "typedef", m.start())
    for m in USING.finditer(clean):
        add(m.group(1), "using", m.start())
    for m in MACRO.finditer(clean):
        add(m.group(1), "macro", m.start())
    for m in INLINE.finditer(clean):
        add(m.group(2), "inline", m.start(), shape_of(m.group(3)))
    for m in EXTERN.finditer(clean):
        nm = declarator_name(m.group(1))
        if nm:
            add(nm, "extern", m.start())
    return decls


def _rel(root: str, path: str) -> str:
    return os.path.relpath(path, root).replace("\\", "/")


def scan_tree(root: str) -> dict:
    """Scan `<root>/include` (shared) and `<root>/src` (units): `{relpath: [decl, ...]}` for each."""
    result = {"shared": {}, "units": {}}
    for sub, suffixes, key in ((SHARED_DIR, HEADER_SUFFIXES, "shared"),
                               (UNIT_DIR, SOURCE_SUFFIXES, "units")):
        base = os.path.join(root, sub)
        if not os.path.isdir(base):
            continue
        for dirpath, dirs, files in os.walk(base):
            dirs.sort()
            for fname in sorted(files):
                if not fname.endswith(suffixes):
                    continue
                abspath = os.path.join(dirpath, fname)
                try:
                    text = open(abspath, encoding="utf-8", errors="replace").read()
                except OSError:
                    continue
                rel = _rel(root, abspath)
                result[key][rel] = extract_decls(strip_comments(text), rel, shared=(key == "shared"))
    return result


_REGISTRY_CACHE: dict = {}


def registry(root: str) -> dict:
    """`scan_tree(root)`, cached per root for the life of the process (`brief.py --pool` briefs 100+ units)."""
    root = os.path.abspath(root)
    if root not in _REGISTRY_CACHE:
        _REGISTRY_CACHE[root] = scan_tree(root)
    return _REGISTRY_CACHE[root]


def clear_cache() -> None:
    _REGISTRY_CACHE.clear()


# ------------------------------------------------------------------------------------------------------------------
# who uses what
# ------------------------------------------------------------------------------------------------------------------

def _includes(text: str) -> set:
    return set(re.findall(r'#[ \t]*include[ \t]+"([^"]+)"', text))


def reference_sets(reg: dict, root: str, symbols_by_unit: dict | None = None) -> dict:
    """`{unit_rel: {"defined", "tokens", "includes"}}` - the raw material for "which units use it".

    `symbols_by_unit` maps a unit to the map symbol names it owns; their mangled components count as used
    tokens, so a function whose map name spells `Q34nw4r4math4VEC3` counts as a use of `VEC3` even when the
    source was never written.
    """
    symbols_by_unit = symbols_by_unit or {}
    refs = {}
    for rel, decls in reg["units"].items():
        path = os.path.join(root, rel)
        try:
            raw = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            raw = ""
        toks = identifiers(strip_comments(raw))
        for name in symbols_by_unit.get(rel, ()):
            toks |= tokens_of_name(name)
        refs[rel] = {"defined": {d["name"] for d in decls},
                     "tokens": toks,
                     "includes": _includes(raw)}
    return refs


def _header_included(header: str, includes: set) -> bool:
    """Whether an include line names `header` (both spellings: `include/ef.h` and `ef.h`)."""
    variants = {header, header[len(SHARED_DIR) + 1:] if header.startswith(SHARED_DIR + "/") else header,
                posixpath.basename(header)}
    return bool(variants & includes)


def analyze(reg: dict, refs: dict, limit: int = 0) -> dict:
    """The whole picture: per-declaration definition/use, and the duplication debt.

    Debt kinds, in the order they matter:
    * `shared`     - a name a shared header declares and one or more units re-define (rule 1);
    * `cross`      - a name two or more units define with no header owning it (rule 1, pre-header);
    * `repeated`   - the same helper prototype (`extern`) carried by several files (rule 2).
    """
    # definitions per name, split by side
    defs_shared, defs_units = {}, {}
    for header, ds in reg["shared"].items():
        for d in ds:
            defs_shared.setdefault(d["name"], []).append(d)
    for unit, ds in reg["units"].items():
        for d in ds:
            defs_units.setdefault(d["name"], []).append(d)

    # usage per name: a unit includes the defining header, names it, and did not define it itself
    used_by = {}
    for name in set(defs_shared) | set(defs_units):
        users = []
        for unit, ref in refs.items():
            if name in ref["defined"]:
                continue
            files = [d["file"] for d in defs_shared.get(name, [])]
            if any(_header_included(f, ref["includes"]) for f in files):
                users.append(unit)
            elif name in ref["tokens"]:
                users.append(unit)
        used_by[name] = sorted(users)

    def cap(rows):
        return rows[:limit] if limit else rows

    debt_shared, debt_cross, debt_repeated, cross_use, unused = [], [], [], [], []
    for name in sorted(set(defs_shared) | set(defs_units)):
        sh = defs_shared.get(name, [])
        un = defs_units.get(name, [])
        # an `extern` declaration is not a definition: only a real re-definition is rule-1 debt
        own_units = sorted({d["file"] for d in un if d["kind"] != "extern" and not is_exempt(d["file"])})
        extern_files = sorted({d["file"] for d in un if d["kind"] == "extern" and not is_exempt(d["file"])})
        kinds = sorted({d["kind"] for d in sh + un})
        if sh and own_units:
            debt_shared.append({"name": name, "kind": kinds[0], "kinds": kinds,
                                "headers": sorted({d["file"] for d in sh}),
                                "units": cap(own_units), "shape": sh[0]["shape"]})
        elif not sh and len(own_units) >= 2:
            debt_cross.append({"name": name, "kind": kinds[0], "kinds": kinds,
                               "units": cap(own_units), "shape": un[0]["shape"]})
        elif not sh and len(own_units) == 1 and used_by.get(name):
            # one definition, but another unit names it: the definition already wants to be shared.
            # A definition that lives in a unit-local header and is *included* by the user is the
            # sanctioned "beside its owner" form - only a name reached by spelling it out is a candidate.
            definer = own_units[0]
            users = used_by[name]
            if definer.endswith(HEADER_SUFFIXES):
                users = [u for u in users if not _header_included(definer, refs[u]["includes"])]
            if users:
                kinds_own = {d["kind"] for d in un if d["kind"] != "extern"}
                cross_use.append({"name": name, "kind": sorted(kinds_own)[0], "defined_in": definer,
                                  "used_by": cap(users)})
        if len(extern_files) >= 2:
            debt_repeated.append({"name": name, "files": cap(extern_files)})
        if sh and not used_by.get(name) and not own_units:
            unused.append({"name": name, "headers": sorted({d["file"] for d in sh})})

    tables = {}
    for header, ds in reg["shared"].items():
        rows = []
        for d in ds:
            rows.append({"name": d["name"], "kind": d["kind"], "line": d["line"], "shape": d["shape"],
                         "used_by": cap(used_by.get(d["name"], [])),
                         "duplicated_in": cap(sorted({x["file"] for x in defs_units.get(d["name"], [])
                                                      if x["kind"] != "extern" and not is_exempt(x["file"])}))})
        tables[header] = sorted(rows, key=lambda r: r["name"].lower())

    summary = {
        "headers": len(reg["shared"]),
        "headers_decls": sum(len(v) for v in reg["shared"].values()),
        "units": len(reg["units"]),
        "units_decls": sum(len(v) for v in reg["units"].values()),
        "debt_shared": len(debt_shared),
        "debt_cross": len(debt_cross),
        "debt_cross_use": len(cross_use),
        "debt_repeated": len(debt_repeated),
        "unused_shared": len(unused),
        "exceptions": sorted(EXCEPTIONS),
    }
    return {"summary": summary,
            "debt": {"shared": debt_shared, "cross": debt_cross, "cross_use": cross_use,
                     "repeated": debt_repeated, "unused": cap(unused)},
            "shared": tables}


def is_exempt(unit_file: str) -> bool:
    return unit_file in EXCEPTIONS


# ------------------------------------------------------------------------------------------------------------------
# the brief's question: which shared headers should this unit use?
# ------------------------------------------------------------------------------------------------------------------

def relevant_headers(reg: dict, text: str, symbol_names=(), unit_file: str = "") -> list:
    """The shared headers a unit should reuse, most actionable first.

    Reasons, in the order the brief renders them:
    * `duplicated` - the unit defines a name a shared header already declares (this is the debt);
    * `included`   - the unit already includes the header;
    * `used`       - the unit (or a symbol it owns) names a declaration in the header;
    * `mentioned`  - a *type* the stub's file-header comment names, before any body exists (an extern or
      macro in prose is ignored - a stub header mentions `fn_*` constantly).

    A header with only a `used` match on the `types.h` scalar base is dropped: including the base is
    normal and listing `u8`/`s32` as advice is noise. A duplication exemption (Camellia) is honoured too.
    """
    clean = strip_comments(text)
    defined = {d["name"] for d in extract_decls(clean, unit_file or "unit", shared=False)
              if d["kind"] != "extern"}  # a re-declared prototype is rule 2, not a redefinition
    toks = identifiers(clean)
    for name in symbol_names:
        toks |= tokens_of_name(name)
    includes = _includes(text)
    exempt = is_exempt(unit_file)
    # A stub's file-header comment says what the unit is before any body exists (`... the nw4r::ef
    # module ...`), so a *named type* mentioned only in a comment is still a hint worth carrying; an
    # extern/macro name in prose is not (a stub header mentions `fn_*` all the time).
    comment_toks = identifiers(text) - toks
    TYPE_KINDS = {"struct", "union", "enum", "class", "typedef", "using", "inline"}

    out = []
    for header, ds in reg["shared"].items():
        dup = [] if exempt else sorted({d["name"] for d in ds if d["name"] in defined})
        used = sorted({d["name"] for d in ds if d["name"] in toks} - set(dup))
        mentioned = sorted({d["name"] for d in ds
                            if d["kind"] in TYPE_KINDS and d["name"] in comment_toks}
                           - set(dup) - set(used))
        included = _header_included(header, includes)
        # the scalar base is included everywhere by design: only a redefinition of it is a finding
        if header in BASE_HEADERS and not dup:
            continue
        reasons = []
        if dup:
            reasons.append("duplicated")
        if included:
            reasons.append("included")
        if used:
            reasons.append("used")
        if mentioned:
            reasons.append("mentioned")
        if not reasons:
            continue
        names = dup + [n for n in used + mentioned if n not in dup]
        out.append({"header": header, "reasons": reasons, "duplicated": dup,
                    "used": [n for n in used if n not in dup],
                    "mentioned": mentioned, "names": names, "decls": len(ds)})
    order = {"duplicated": 0, "included": 1, "used": 2, "mentioned": 3}
    out.sort(key=lambda h: (min(order[r] for r in h["reasons"]) if h["reasons"] else 3,
                            -len(h["names"]), h["header"]))
    return out


# ------------------------------------------------------------------------------------------------------------------
# the registry's own view of the map (which symbols a unit owns), for the standalone report
# ------------------------------------------------------------------------------------------------------------------

_MAP_CACHE: dict = {}


def _map_rows(root: str) -> list:
    path = os.path.join(root, "config", "RMHE08", "symbols.txt")
    if not os.path.exists(path):
        return []
    key = (path, os.path.getmtime(path))
    if key not in _MAP_CACHE:
        rows = []
        pattern = re.compile(r"^(\S+) = (\S+):(0x[0-9A-Fa-f]+);")
        with open(path, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = pattern.match(line)
                if m:
                    rows.append((int(m.group(3), 16), m.group(1)))
        rows.sort()
        _MAP_CACHE.clear()
        _MAP_CACHE[key] = rows
    return _MAP_CACHE[key]


def symbols_by_unit(root: str) -> dict:
    """`{unit source path (as in splits.txt): [symbol names]}` from `splits.txt` + `symbols.txt`."""
    splits_path = os.path.join(root, "config", "RMHE08", "splits.txt")
    if not os.path.exists(splits_path):
        return {}
    rows = _map_rows(root)
    if not rows:
        return {}
    addrs = [a for a, _ in rows]
    out, current, ranges = {}, None, {}
    for line in open(splits_path, encoding="utf-8", errors="replace"):
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":") and not line.startswith("#"):
            current = line.strip()[:-1]
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and current:
            ranges[current] = (int(m.group(1), 16), int(m.group(2), 16))
    for unit, (start, end) in ranges.items():
        lo, hi = bisect.bisect_left(addrs, start), bisect.bisect_left(addrs, end)
        out[unit] = [rows[i][1] for i in range(lo, hi)]
    # splits.txt names the source file; reg keys are `src/...` - bridge the two spellings
    bridged = {}
    for unit, names in out.items():
        key = posixpath.normpath(posixpath.join(UNIT_DIR, unit.lstrip("/")))
        bridged[key] = names
    return bridged


# ------------------------------------------------------------------------------------------------------------------
# report
# ------------------------------------------------------------------------------------------------------------------

def _fmt(rows, key, limit=6):
    vals = rows.get(key) if isinstance(rows, dict) else rows
    vals = vals or []
    shown = ", ".join("`%s`" % v for v in vals[:limit])
    return shown + (" (+%d more)" % (len(vals) - limit) if len(vals) > limit else "")


def _clip(text: str, width: int = 60) -> str:
    """A shape fingerprint clipped for the report - `_PLW`'s is 700+ characters and is not a table cell."""
    text = text or "-"
    return text if len(text) <= width else text[:width - 1] + "\u2026"


def render_text(analysis: dict) -> str:
    s = analysis["summary"]
    lines = ["# Shared type/helper registry", "",
             "Generated by `python tools/units/typeregistry.py`. Scanned **%d shared header(s)** (%d "
             "declarations) and **%d unit file(s)** (%d declarations)."
             % (s["headers"], s["headers_decls"], s["units"], s["units_decls"]), "",
             "**Duplication debt: %d name(s)** - %d declared in a shared header and re-defined in a unit "
             "(rule 1), %d defined by two or more units with no header owning it, %d helper declaration(s) "
             "repeated across files (rule 2). %d shared declaration(s) no unit currently uses."
             % (s["debt_shared"] + s["debt_cross"], s["debt_shared"], s["debt_cross"],
                s["debt_repeated"], s["unused_shared"]), ""]

    lines += ["## 1 \u00b7 A shared header already declares it, and a unit re-defines it", ""]
    if not analysis["debt"]["shared"]:
        lines.append("(none)")
    else:
        lines += ["| name | kind | shape | shared header | re-defined in |", "| --- | --- | --- | --- | --- |"]
        for d in analysis["debt"]["shared"]:
            lines.append("| `%s` | %s | `%s` | %s | %s |"
                         % (d["name"], d["kind"], _clip(d["shape"]), _fmt(d, "headers"), _fmt(d, "units")))
    lines.append("")

    lines += ["## 2 \u00b7 Two or more units define it, no header owns it", ""]
    if not analysis["debt"]["cross"]:
        lines.append("(none)")
    else:
        lines += ["| name | kind | shape | units |", "| --- | --- | --- | --- |"]
        for d in analysis["debt"]["cross"]:
            lines.append("| `%s` | %s | `%s` | %s |" % (d["name"], d["kind"], _clip(d["shape"]),
                                                        _fmt(d, "units")))
    lines.append("")

    lines += ["## 3 · Defined in one unit, but another unit names it (move it to a header)", ""]
    if not analysis["debt"].get("cross_use"):
        lines.append("(none)")
    else:
        lines += ["| name | kind | defined in | named by |", "| --- | --- | --- | --- |"]
        for d in analysis["debt"]["cross_use"]:
            lines.append("| `%s` | %s | `%s` | %s |"
                         % (d["name"], d["kind"], d["defined_in"], _fmt(d, "used_by")))
    lines.append("")

    lines += ["## 4 · A helper prototype repeated across files (rule 2)", ""]
    if not analysis["debt"]["repeated"]:
        lines.append("(none)")
    else:
        lines += ["| name | files |", "| --- | --- |"]
        for d in analysis["debt"]["repeated"]:
            lines.append("| `%s` | %s |" % (d["name"], _fmt(d, "files")))
    lines.append("")

    lines += ["## 5 \u00b7 The shared headers, declaration by declaration", ""]
    for header, rows in analysis["shared"].items():
        lines += ["### `%s` (%d)" % (header, len(rows)), ""]
        lines += ["| name | kind | line | used by | re-defined in |", "| --- | --- | --- | --- | --- |"]
        for r in rows:
            lines.append("| `%s` | %s | %d | %s | %s |"
                         % (r["name"], r["kind"], r["line"], _fmt(r, "used_by") or "-",
                            _fmt(r, "duplicated_in") or "-"))
        lines.append("")
    if analysis["debt"]["unused"]:
        lines += ["## 6 \u00b7 Declared, no unit uses it yet", ""]
        for d in analysis["debt"]["unused"]:
            lines.append("* `%s` in %s" % (d["name"], _fmt(d, "headers")))
        lines.append("")
    if EXCEPTIONS:
        lines += ["## Exceptions", ""]
        for path, why in sorted(EXCEPTIONS.items()):
            lines.append("* `%s` - %s" % (path, why))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


# ------------------------------------------------------------------------------------------------------------------
# CLI
# ------------------------------------------------------------------------------------------------------------------

def build_report(root: str, use_map: bool = True) -> dict:
    reg = registry(root)
    symbols = symbols_by_unit(root) if use_map else {}
    refs = reference_sets(reg, root, symbols)
    return analyze(reg, refs)


def selftest() -> int:
    import tempfile

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # -- text helpers ---------------------------------------------------------------------------------------------
    src = 'int a; /* struct Fake { */\n// typedef struct Nope {\nchar* s = "struct AlsoFake {";\n'
    clean = strip_comments(src)
    check("strip_comments preserves length", len(clean), len(src))
    check("strip_comments keeps newlines", clean.count("\n"), src.count("\n"))
    check("strip_comments blanks a block comment", "Fake" in src and "Fake" not in clean, True)
    check("strip_comments blanks a line comment", "Nope" not in clean, True)
    check("strip_comments blanks a string body", "AlsoFake" not in clean, True)

    check("tokens_of_name peels mangled components",
          {"VEC3", "nw4r", "math"} <= tokens_of_name("Pl_get_gunner_pos__FP4_PLWQ34nw4r4math4VEC3l"), True)
    check("tokens_of_name sees _PLW", "_PLW" in tokens_of_name("foo__FP4_PLW"), True)
    check("tokens_of_name keeps the plain name", "plain" in tokens_of_name("plain"), True)

    # -- declaration extraction ------------------------------------------------------------------------------------
    text = """
typedef struct Vec { f32 x; f32 y; f32 z; } Vec;
typedef struct { f32 x; f32 y; f32 z; } VEC3;
struct Named { u8 a; u16 b; };
typedef struct Fwd Fwd;
typedef unsigned int u32;
typedef void (*SpawnFn)(void* self, u16 id);
typedef unsigned int KEY_TABLE[16];
enum Color { RED, GREEN };
using Alias = int;
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LIMIT 32
inline int IsValid(u32 ptr) { return ptr != 0; }
extern void Panic(const char* f, int line, ...);
"""
    decls = extract_decls(strip_comments(text), "src/x.c", shared=False)
    names = {d["name"]: d["kind"] for d in decls}
    check("named struct + alias records the tag", names.get("Vec"), "struct")
    check("anonymous typedef records the alias", names.get("VEC3"), "typedef")
    check("plain struct records its tag", names.get("Named"), "struct")
    check("forward typedef keeps the tag kind", names.get("Fwd"), "struct")
    check("simple typedef records its declarator", names.get("u32"), "typedef")
    check("function-pointer typedef records its name", names.get("SpawnFn"), "typedef")
    check("array typedef records its name", names.get("KEY_TABLE"), "typedef")
    check("enum records its tag", names.get("Color"), "enum")
    check("using records its alias", names.get("Alias"), "using")
    check("function-like macro recorded", "MAX" in names, True)
    check("object-like macro recorded", "LIMIT" in names, True)
    check("inline helper recorded", names.get("IsValid"), "inline")
    check("extern prototype recorded", names.get("Panic"), "extern")
    check("a name is recorded once per file", [d["name"] for d in decls].count("Vec"), 1)

    check("shape ignores field names",
          extract_decls(strip_comments("struct A { f32 x; f32 y; f32 z; };"), "a.h", True)[0]["shape"],
          "f32 f32 f32")
    check("shape keeps padding type", extract_decls(strip_comments("struct B { u8 p[0x14]; u16 n; };"),
                                                    "b.h", True)[0]["shape"], "u8[] u16")
    check("shape marks function pointers",
          extract_decls(strip_comments("struct C { void (*spawn)(int); };"), "c.h", True)[0]["shape"],
          "fnptr")
    check("two same-shape types share a fingerprint",
          extract_decls(strip_comments("struct D { f32 a; f32 b; f32 c; };"), "d.h", True)[0]["shape"]
          == extract_decls(strip_comments("struct E { f32 q; f32 w; f32 e; };"), "e.h", True)[0]["shape"],
          True)

    # -- scan + debt -----------------------------------------------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "include", "nw4r"))
        os.makedirs(os.path.join(tmp, "src", "auto"))
        os.makedirs(os.path.join(tmp, "src", "Vendor"))
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "include", "ef.h"), "w").write(
            "#include \"types.h\"\n"
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\n"
            "typedef struct EfWork { u16 split; u32 progress; } EfWork;\n"
            "#define EF_ASSERT_PTR(p) do { } while (0)\n"
            "inline int IsValidPointer(u32 p) { return p != 0; }\n"
            "extern void Panic(const char* f, int line, ...);\n")
        open(os.path.join(tmp, "include", "types.h"), "w").write(
            "typedef unsigned long u32;\ntypedef float f32;\n")
        open(os.path.join(tmp, "include", "nw4r", "math.h"), "w").write(
            "namespace nw4r { namespace math { struct VEC3 { f32 x; f32 y; f32 z; }; } }\n")
        # unit A includes ef.h and uses Vec / EF_ASSERT_PTR (no debt)
        open(os.path.join(tmp, "src", "auto", "uses_ef.c"), "w").write(
            "#include \"ef.h\"\n"
            "extern void SharedHelper(int);\n"
            "void a(Vec* v, EfWork* w, struct Lone* lone) { EF_ASSERT_PTR(v); SharedHelper(1); "
            "(void)lone; w->split = 0; }\n")
        # unit B defines Vec/WORK locally - the debt
        open(os.path.join(tmp, "src", "auto", "copies_ef.c"), "w").write(
            "extern void SharedHelper(int);\n"
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\n"
            "typedef struct EfWork { u16 split; u32 progress; } EfWork;\n"
            "void b(Vec* v) { SharedHelper(2); (void)v; }\n")
        # unit C owns a mangled symbol that encodes VEC3 but never names it in source, and defines a
        # type another unit names (the header-move candidate)
        open(os.path.join(tmp, "src", "auto", "mangled.c"), "w").write(
            "struct Lone { u8 a; };\nvoid fn_1(void) { }\n")
        # a bodyless stub whose file-header comment names the type it will need
        open(os.path.join(tmp, "src", "auto", "hinted.c"), "w").write(
            "/* the nw4r::math VEC3 shape, no body yet */\n")
        # a unit-local header beside its owner and included - the sanctioned 'beside its owner' form
        open(os.path.join(tmp, "src", "auto", "local.h"), "w").write("struct LocalType { u8 a; };\n")
        open(os.path.join(tmp, "src", "auto", "local_user.c"), "w").write(
            "#include \"local.h\"\nvoid lu(struct LocalType* t) { (void)t; }\n")
        open(os.path.join(tmp, "src", "Vendor", "vendor.c"), "w").write(
            "typedef unsigned long u32;\ntypedef float f32;\nvoid v(void) { }\n")
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [{"lib": "auto", "objects": [\n'
            '    Object(NonMatching, "auto/uses_ef.c"),\n'
            '    Object(NonMatching, "auto/copies_ef.c"),\n'
            '    Object(NonMatching, "auto/mangled.c"),\n'
            '    Object(NonMatching, "Vendor/vendor.c"),\n]}]')
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/uses_ef.c:\n\t.text start:0x80000000 end:0x80000004\n"
            "auto/copies_ef.c:\n\t.text start:0x80000004 end:0x80000008\n"
            "auto/mangled.c:\n\t.text start:0x80000008 end:0x8000000C\n"
            "Vendor/vendor.c:\n\t.text start:0x8000000C end:0x80000010\n")
        open(os.path.join(tmp, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_1 = .text:0x80000008; // type:func size:0x4\n"
            "malloc__FPQ34nw4r4math4VEC3 = .text:0x8000000A; // type:func size:0x2\n")
        # registry.EXCEPTIONS is keyed on the real repo path; point one at the fixture for the check
        fake_exemption = "src/Vendor/vendor.c"
        EXCEPTIONS[fake_exemption] = "fixture vendor exception"
        try:
            clear_cache()
            reg = registry(tmp)
            check("scan finds the shared headers", sorted(reg["shared"]),
                  ["include/ef.h", "include/nw4r/math.h", "include/types.h"])
            check("scan finds the unit files", len(reg["units"]), 7)
            check("a shared tag and its typedef alias are one row",
                  sorted({d["name"] for d in reg["shared"]["include/ef.h"]}),
                  ["EF_ASSERT_PTR", "EfWork", "IsValidPointer", "Panic", "Vec"])

            refs = reference_sets(reg, tmp, symbols_by_unit(tmp))
            check("the base header is excluded from 'used' advice",
                  {h["header"] for h in relevant_headers(
                      reg, '#include "types.h"\nvoid a(f32 x) { (void)x; }', ["fn_1"])},
                  set())
            a = relevant_headers(reg, open(os.path.join(tmp, "src", "auto", "uses_ef.c")).read(), [])
            by_header = {h["header"]: h for h in a}
            check("a unit including ef.h is told ef.h", "include/ef.h" in by_header, True)
            check("ef.h is not marked duplicated for the consumer",
                  "duplicated" in by_header["include/ef.h"]["reasons"], False)
            b = relevant_headers(reg, open(os.path.join(tmp, "src", "auto", "copies_ef.c")).read(), [])
            bmap = {h["header"]: h for h in b}
            check("the copying unit is told ef.h", "include/ef.h" in bmap, True)
            check("ef.h is marked duplicated for the copier",
                  "duplicated" in bmap["include/ef.h"]["reasons"], True)
            check("the duplicated names are named", bmap["include/ef.h"]["duplicated"], ["EfWork", "Vec"])
            check("the copier is first (most actionable)", b[0]["header"], "include/ef.h")
            m = relevant_headers(reg, "void fn_1(void) { }\n",
                                 ["malloc__FPQ34nw4r4math4VEC3"])
            check("a mangled symbol name brings in math.h",
                  "include/nw4r/math.h" in {h["header"]: h for h in m}, True)
            check("math.h is listed as used",
                  "used" in {h["header"]: h for h in m}["include/nw4r/math.h"]["reasons"], True)
            h = relevant_headers(reg, open(os.path.join(tmp, "src", "auto", "hinted.c")).read(), [])
            check("a bodyless stub's header comment still names the header",
                  [x["header"] for x in h], ["include/nw4r/math.h"])
            check("the stub's match is 'mentioned', not 'used'", h[0]["reasons"], ["mentioned"])
            check("a comment naming an extern does not drag its header in",
                  [x["header"] for x in relevant_headers(reg, "/** Panic and fn_80043EA8 */", [])], [])
            check("the vendor exception suppresses its duplication",
                  [h["header"] for h in relevant_headers(
                      reg, open(os.path.join(tmp, "src", "Vendor", "vendor.c")).read(), [],
                      unit_file=fake_exemption)], [])

            analysis = analyze(reg, refs)
            sh = {d["name"]: d for d in analysis["debt"]["shared"]}
            check("Vec is shared duplication debt", "Vec" in sh, True)
            check("the debt points at the header", sh["Vec"]["headers"], ["include/ef.h"])
            check("the debt points at the copying unit", sh["Vec"]["units"], ["src/auto/copies_ef.c"])
            check("the exception unit is not debt",
                  any("src/Vendor/vendor.c" in d["units"] for d in analysis["debt"]["shared"]), False)
            check("EfWork has the same shape as its copy",
                  sh["EfWork"]["shape"], "u16 u32")
            check("the header table marks the copier",
                  [r for r in analysis["shared"]["include/ef.h"] if r["name"] == "Vec"][0]["duplicated_in"],
                  ["src/auto/copies_ef.c"])
            check("IsValidPointer is reported as used by the consumer",
                  "src/auto/uses_ef.c" in [r for r in analysis["shared"]["include/ef.h"]
                                           if r["name"] == "IsValidPointer"][0]["used_by"], True)
            check("the repeated extern is rule-2 debt",
                  any(d["name"] == "SharedHelper" for d in analysis["debt"]["repeated"]), True)
            check("a repeated extern is not counted as a re-definition",
                  any(d["name"] == "SharedHelper" for d in analysis["debt"]["cross"]), False)
            check("a type one unit defines and another names is a header candidate",
                  [(d["name"], d["defined_in"]) for d in analysis["debt"]["cross_use"]],
                  [("Lone", "src/auto/mangled.c")])
            check("the header candidate names the unit that uses it",
                  analysis["debt"]["cross_use"][0]["used_by"], ["src/auto/uses_ef.c"])
            check("a unit-local header included by its user is not a move candidate",
                  any(d["name"] == "LocalType" for d in analysis["debt"]["cross_use"]), False)
            check("the summary counts the shared debt", analysis["summary"]["debt_shared"], 2)
            report = render_text(analysis)
            check("the report names the duplication section", "shared header already declares it" in report, True)
            check("the report names the header", "include/ef.h" in report, True)
        finally:
            EXCEPTIONS.pop(fake_exemption, None)
            clear_cache()

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=REPO_ROOT, help="repository root (default: this checkout)")
    ap.add_argument("--unit", default=None, help="show the shared headers one unit should reuse")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--report", default=None, help="write the markdown report to this path")
    ap.add_argument("--no-map", action="store_true", help="do not consult symbols.txt for owned symbols")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    root = os.path.abspath(args.root)
    reg = registry(root)

    if args.unit:
        rel = args.unit.replace("\\", "/").lstrip("/")
        if rel.startswith(UNIT_DIR + "/"):
            rel = rel[len(UNIT_DIR) + 1:]
        # resolve the real extension, so the extensionless spelling (`Pl/pl_act`) works too
        if not rel.endswith(SOURCE_SUFFIXES):
            for ext in SOURCE_SUFFIXES:
                if os.path.exists(os.path.join(root, UNIT_DIR, *rel.split("/")) + ext):
                    rel += ext
                    break
        path = os.path.join(root, UNIT_DIR, *rel.split("/"))
        text = ""
        if os.path.exists(path):
            text = open(path, encoding="utf-8", errors="replace").read()
        unit_file = posixpath.normpath(posixpath.join(UNIT_DIR, rel))
        symbols = symbols_by_unit(root) if not args.no_map else {}
        names = symbols.get(unit_file, [])
        found = relevant_headers(reg, text, names, unit_file=unit_file)
        if args.json:
            print(json.dumps(found, indent=2))
            return 0
        if not found:
            print("%s: no shared header declares anything this unit names - check the rule before "
                  "defining a type locally (docs/plan.md 6.5 rule 1)." % args.unit)
            return 0
        print("%s should reuse:" % args.unit)
        for h in found:
            print("  %-28s %-11s %s" % (h["header"], ",".join(h["reasons"]), ", ".join(h["names"][:12])))
        return 0

    analysis = build_report(root, use_map=not args.no_map)
    if args.json:
        print(json.dumps(analysis, indent=2))
        return 0
    text = render_text(analysis)
    if args.report:
        os.makedirs(os.path.dirname(os.path.abspath(args.report)), exist_ok=True)
        open(args.report, "w", encoding="utf-8", newline="\n").write(text)
        print("wrote %s (%d lines)" % (args.report, text.count("\n")))
        print("debt: %d shared+unit, %d unit+unit, %d repeated; %d unused shared declaration(s)"
              % (analysis["summary"]["debt_shared"], analysis["summary"]["debt_cross"],
                 analysis["summary"]["debt_repeated"], analysis["summary"]["unused_shared"]))
        return 0
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
