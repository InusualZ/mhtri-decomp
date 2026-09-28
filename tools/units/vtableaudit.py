#!/usr/bin/env python3
"""Sweep registered units for a **vtable they own but do not emit** - the rule-10 defect made mechanical.

Why this tool exists (owner's backlog #0, 2026-09-26). `docs/plan.md` §6.5 rule 10 (playbook row 52) says a
table of code pointers inside a unit's own registered ranges is **compiler output**: declare the class with its
`virtual` methods and let MWCC emit the table. A hand-modelled table - or a table the unit simply never emits -
is **not evidence of inheritance**, and for a `NonMatching` unit nothing fails: the bytes come from the DOL, the
unit scores 100 %, and the defect only surfaces at the flip, when a `Matching` object replaces the original and
the DOL hash moves. The audit that found this was done **by hand once** (0 instances, 2026-09-26); this is that
audit as a tool so it stays true.

Three forms of the same defect, and what the tool reports for each:

**(a) code-pointer runs inside the unit's own data ranges.** For every unit, walk the data sections it owns in
`config/RMHE08/splits.txt` (`.data`, `.rodata`, `.sdata`, `.sdata2`, `.ctors`, `.dtors`, `extab`,
`extabindex`) and find maximal runs of consecutive words whose value is an address inside `.text`. A word's
value is read from the unit's own target object (`build/RMHE08/obj/<Unit>.o`): a word carrying a relocation is
resolved through that relocation (`section base + symbol + addend`; the object's bytes hold the unlinked addend
there, the *same* reason `dataclaim` masks reloc sites), a word without one is a plain linked value. A run needs
**at least two entries** - a single code pointer in a data section is an ordinary pointer, not a table.

**(b) whether OUR side is legitimate.** A run our object **emits** (the section covers it and every word is a
code pointer) or **references** (a relocation anywhere in our object resolves to an address inside the run) is
fine. A run we own and **neither emit nor reference** is the violation - the table is absent from our object
entirely. The legal rule-10 Case 2 shape is the mirror image and is explicitly **not** flagged, only recorded:
`self->vtbl = lbl_XXXXXXXX;` where the address is outside every registered range is another TU's table, and
storing its address is the correct way to reproduce the call without dragging a class into this TU. The source
scan for that shape reports every hit with its classification: `external` (no registered range owns it -
legal), `foreign` (another unit owns it - legal), `own` (this unit's range - the shape rule 10 forbids,
reported).

**The scan keys on the assignment and on OWNERSHIP, never on the table symbol's spelling** (fixed
2026-09-27). It used to match `vtable = lbl_XXXXXXXX` and nothing else, so
`self->vtable = &NetworkSessionManagerVTable;` in `Network/fn_803D3CE8.cpp` - a table at 0x805FA908, inside
that unit's own band - was invisible to it and survived a landing review; the owner found it by reading the
file. A rule about *ownership* cannot be enforced by a scan keyed on a *name*. The symbol on the right is now
resolved through `symbols.txt` (any spelling) and classified against the unit's own registered ranges, and
the member it is assigned to is matched either by the legacy `vtable`/`vtbl` name or by **the definition
index**: any struct/class member at `+0x00` whose type is a pointer to a struct whose members are function
pointers (the owner's heuristic - a function-pointer-table pointer at `+0x00` IS a class with inheritance).
That is `fn_table_fields`, and it is what makes the `_VTable`-named blind spot impossible to repeat.

**(c) section completeness.** The same defect seen from the other side: a `NonMatching` unit hides a missing
`.data`/`.rodata` section entirely, because the target's bytes are scored against nothing of ours. Every
non-`.text` section whose size differs between our object and the target object is reported, marked `missing`
(ours is 0), `extra` (the target has none), `short` or `long`.

What the tool **cannot** decide, said plainly: whether an emitted table came from a `virtual` class or from a
hand-written array of the same bytes. Both compile to the same object; the difference is in the source (`virtual`
methods plus the constructor that stores the table vs. an array initializer). The tool's job is the part that is
decidable - a table that is *absent* from our object - and it names the emitted ones so a reviewer can check the
source. `docs/plan.md` §6.5 rule 10's "a table we wrote is not evidence of inheritance" therefore still lands on
the reviewer; the tool removes the invisible case.

Read-only by construction: no `ninja`, no compile, no link, no write anywhere. Section parsing is
`tools/elf/elfsect.py`; the registered-unit list is `langcheck.registered_units` (the one place that reads
`config.libs`); the DOL header supplies the `.text` ranges.

    python tools/units/vtableaudit.py                    # every registered unit, runs + refs + section diffs
    python tools/units/vtableaudit.py --runs             # only the owned code-pointer runs
    python tools/units/vtableaudit.py --sections         # only the section-size differences
    python tools/units/vtableaudit.py --fields           # only the fn-table-pointer fields at +0x00
    python tools/units/vtableaudit.py --unit Pl/pl_master
    python tools/units/vtableaudit.py --diff <ref>       # exit 0 = the batch adds no rule-10 violation
    python tools/units/vtableaudit.py --at 0x80050F28   # one vtable, its slots and each target's owner
    python tools/units/vtableaudit.py --json
    python tools/units/vtableaudit.py --selftest

`--main` points the sweep at a tree holding `configure.py` and `build/` (default: the tree this file lives in),
which is how a worktree audits `MAIN`'s built objects without a build of its own. `--diff <ref>` is the
`land.py` gate row's core: the violation set is computed twice - once with the batched tree and once with
`<ref>`'s text (`configure.py`, `symbols.txt`, `splits.txt` and the `src/**` that carries an assignment, read
with `git show`) - and only a set that *grew* is a refusal. The run half is judged with the working tree's
objects on both sides, so the ownership change a batch makes (a new `.data` claim) is what the diff sees;
a batch that only *removed* an emission is not refused by that half - it is named in the report instead.

`--at <addr>` is the census mode: it reads the vtable at `<addr>` straight out of the DOL and prints every
slot with the registered unit that **owns** its target (by address, never by name) and the symbol the map
names there, plus the reference object's relocation symbol for that slot (`dossier.parse_elf`). One lane
hand-built that list twice and got 62 of 114 slots wrong, each time by parsing the DOL header's grouped
offset/address/size fields by hand - this mode parses them once, in `dol_segments`/`dol_read`.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _p in (HERE, os.path.join(ROOT, "tools", "elf")):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import elfsect  # noqa: E402  (the project's ELF section reader)
import langcheck  # noqa: E402  (registered_units - the same list every other unit tool uses)
import dossier  # noqa: E402  (parse_elf - the one ELF object reader, for the `--at` reference side)

GAME = "RMHE08"
SPLITS_REL = os.path.join("config", GAME, "splits.txt")
SYMBOLS_REL = os.path.join("config", GAME, "symbols.txt")
DOL_REL = os.path.join("orig", GAME, "sys", "main.dol")

# A code pointer is an address in one of these object sections, or (for an unresolved/raw word) an address
# inside the DOL's own text sections.
CODE_SECTIONS = (".text", ".init")

# The sections a table of code pointers can live in. `.bss`/`.sbss` carry no file bytes, so nothing can be
# read out of them; they are still compared by size in the section-completeness report.
DATA_SECTIONS = (".data", ".rodata", ".sdata", ".sdata2", ".ctors", ".dtors", "extab", "extabindex")
NOBITS_SECTIONS = (".bss", ".sbss", ".sbss2")

# A table needs at least two entries: one code pointer in a data section is an ordinary pointer.
MIN_RUN_WORDS = 2

# Section-level bookkeeping, never part of the comparison: the symbol/string tables, the compiler banner, the
# splitter's own note, relocation companions and debug sections.
META_PREFIXES = (".rela", ".debug", ".group", ".llvm")
META_SECTIONS = (".symtab", ".strtab", ".shstrtab", ".comment", ".note.split")

# How a run is labelled in the report, by the section it lives in.
RUN_KIND = {
    ".ctors": "initializer", ".dtors": "initializer",
    "extab": "exception", "extabindex": "exception",
}

# `self->vtable = (const SoundVtbl*)lbl_80597DA8;` and `self->vtable = &NetworkSessionManagerVTable;` -
# the rule-10 shape: an ASSIGNMENT to a struct/class member that holds a table of code pointers. The scan
# keys on the assignment and on the member's POSITION (a table pointer at +0x00 of a class), never on the
# SPELLING of the table symbol on the right: `lbl_XXXXXXXX` was the only spelling the old pattern matched,
# which is exactly how `Network/fn_803D3CE8.cpp`'s two `&NetworkSessionManagerVTable` writes survived a
# landing review (2026-09-27). The symbol is resolved through `symbols.txt` and classified by OWNERSHIP, so
# any name works. A member named `vtable_...` is a different member and is not matched.
VTABLE_ASSIGN_RE = re.compile(
    r"(?:->|\.)\s*(?P<field>[A-Za-z_]\w*)\s*=\s*(?!=)(?:\([^()]*\)\s*)*(?P<amp>&\s*)?"
    r"(?P<sym>[A-Za-z_]\w*)\b")

# The member names that count with no definition to prove them: the two spellings the by-hand audit and the
# campaign's units use. `fn_table_fields` ADDS every other name the tree gives a function-pointer table at
# `+0x00`, so a member spelled `pVtbl` is caught too.
LEGACY_VTABLE_FIELDS = ("vtable", "vtbl")

# A member that holds a table of code pointers at `+0x00` of a class is the rule-10 heuristic (owner,
# 2026-09-27): `struct X { X_VTable* vtable; /* +0x00 */ ... }` is a CLASS WITH INHERITANCE - the field means
# the original was a class with `virtual` methods and MWCC emitted the table and the store itself. A
# function-pointer member is `RET (*name)(args);`; a struct with at least one of them is the pointee of such
# a field.
FN_PTR_RE = re.compile(r"\(\s*[*&]*\s*(?P<name>[A-Za-z_]\w*)\s*\)\s*\(")
CLASS_OPEN_RE = re.compile(r"\b(?:typedef\s+)?(class|struct)\b\s*([A-Za-z_]\w*)?\s*(?::[^{;]*)?\{")

SOURCE_EXT = (".c", ".cc", ".cpp", ".cxx", ".cp", ".c++")

UNIT_RE = re.compile(r"^(\S+):\s*$")
RANGE_RE = re.compile(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)"
                     r"(?:\s+rename:(\S+))?\s*$")
SYMBOL_RE = re.compile(r"^(\S+)\s*=\s*([.\w]+):(0x[0-9A-Fa-f]+);")
HEX_SUFFIX_RE = re.compile(r"_([0-9A-Fa-f]{8})$")


# --------------------------------------------------------------------------------------------------
# the pure core (everything the selftest drives without a repository)
# --------------------------------------------------------------------------------------------------
def parse_splits(text: str) -> dict:
    """`{unit: [{section, start, end, object}]}` from `config/RMHE08/splits.txt` - which ranges a unit owns.

    `section` is the split's own section (`.ctors`, `.data`, ...) and drives the run kind; `object` is the
    name the section carries **in the object**, which `rename:` overrides (dtk renames a duplicated section
    to `.ctors$10`/`.dtors$15` so two units' initializer entries stay distinct). Reading the object by the
    split's name instead would miss every renamed section - `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp`
    owns three of them.
    """
    units, cur = {}, None
    for line in text.splitlines():
        if not line.strip() or line.startswith("Sections:"):
            continue
        m = UNIT_RE.match(line)
        if m:
            cur = m.group(1)
            units[cur] = []
            continue
        m = RANGE_RE.match(line)
        if m and cur is not None:
            units[cur].append({"section": m.group(1), "start": int(m.group(2), 16),
                               "end": int(m.group(3), 16), "object": m.group(4) or m.group(1)})
    return units


def parse_symbols(text: str) -> dict:
    """`{name: (section, address, size)}` from `config/RMHE08/symbols.txt`.

    Needed to resolve a relocation whose symbol is **undefined** in the object (the splitter names it with
    the map's spelling but has no address for it), to name the symbol a run sits at, and - for the rule-10
    ownership test - to read the table's own extent out of the DOL.
    """
    out = {}
    for line in text.splitlines():
        m = SYMBOL_RE.match(line)
        if m:
            size = re.search(r"size:(0x[0-9A-Fa-f]+)", line)
            out[m.group(1)] = (m.group(2), int(m.group(3), 16),
                               int(size.group(1), 16) if size else 0)
    return out


def dol_segments(blob: bytes) -> list:
    """`[(start, size, file_offset)]` for every section the DOL header declares - the raw byte reader."""
    if len(blob) < 0x100:
        return []
    text_off = struct.unpack_from(">7I", blob, 0x00)
    data_off = struct.unpack_from(">11I", blob, 0x1C)
    text_addr = struct.unpack_from(">7I", blob, 0x48)
    data_addr = struct.unpack_from(">11I", blob, 0x64)
    text_size = struct.unpack_from(">7I", blob, 0x90)
    data_size = struct.unpack_from(">11I", blob, 0xAC)
    out = []
    for i in range(7):
        if text_size[i]:
            out.append((text_addr[i], text_size[i], text_off[i]))
    for i in range(11):
        if data_size[i]:
            out.append((data_addr[i], data_size[i], data_off[i]))
    return out


def dol_read(blob: bytes, address: int, length: int) -> bytes:
    """The DOL's own bytes at `address`, or None when the address is in no section."""
    for start, size, offset in dol_segments(blob):
        if start <= address < start + size:
            return blob[offset + address - start:offset + address - start + length]
    return None


def dol_text_ranges(blob: bytes) -> list:
    """`[(start, end)]` for every text section the DOL header declares - the authoritative `.text`."""
    if len(blob) < 0x100:
        return []
    addr = struct.unpack_from(">7I", blob, 0x48)
    size = struct.unpack_from(">7I", blob, 0x90)
    return [(addr[i], addr[i] + size[i]) for i in range(7) if size[i]]


def merge_ranges(ranges) -> list:
    """Sort and coalesce touching/overlapping half-open ranges, so `in_ranges` is one binary-search-free test."""
    out = []
    for start, end in sorted(ranges):
        if out and start <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], end))
        else:
            out.append((start, end))
    return out


def in_ranges(ranges, address: int) -> bool:
    return any(start <= address < end for start, end in ranges)


def find_runs(flags, min_words: int = MIN_RUN_WORDS) -> list:
    """Maximal `(start_index, length)` blocks of truthy flags, keeping only those of `min_words` or more."""
    out, i, n = [], 0, len(flags)
    while i < n:
        if flags[i]:
            j = i
            while j < n and flags[j]:
                j += 1
            if j - i >= min_words:
                out.append((i, j - i))
            i = j
        else:
            i += 1
    return out


def resolve_symbol(sym: dict, section_names, bases: dict, symbols_by_name: dict):
    """`(absolute_address, section_or_None)` for one symbol row, or `(None, None)` when it cannot be resolved.

    A defined symbol's address is `its own section's base + its offset`; the base comes from `splits.txt`
    (the object is relocatable and every section sits at 0). An undefined symbol has no address in the
    object at all - the splitter names it with the map's spelling - so it is resolved through
    `symbols.txt`, falling back to the `_XXXXXXXX` address every `lbl_`/`fn_`/`jumptable_` name carries.
    """
    name, value, shndx = sym.get("name", ""), sym.get("value", 0), sym.get("shndx", 0)
    if shndx == 0:
        named = symbols_by_name.get(name)
        if named:
            return named[1], named[0]
        m = HEX_SUFFIX_RE.search(name or "")
        return (int(m.group(1), 16), None) if m else (None, None)
    if shndx >= 0xFF00:                                     # SHN_ABS / SHN_COMMON
        return (value, None) if shndx == 0xFFF1 else (None, None)
    if 0 < shndx < len(section_names):
        sec = section_names[shndx]
        base = bases.get(sec)
        return (base + value, sec) if base is not None else (None, None)
    return None, None


def read_word(obj: dict, section: str, offset: int, bases: dict, symbols_by_name: dict):
    """`(kind, value, sym_section)` for the word at `offset` of `section` in one object.

    `kind` is `reloc` when the object carries a relocation there - the object's bytes hold the unlinked
    addend, not the address, which is exactly why `dataclaim` masks reloc sites - and `raw` otherwise (the
    bytes are the linked value; a split object's file bytes are copied straight out of the DOL). `value` is
    the absolute address for a reloc, or the literal word for a raw read; `None` when unresolvable.
    """
    rows = obj["relocs"].get(section) or {}
    if offset in rows:
        _type, sym_index, addend = rows[offset]
        syms = obj["symbols"]
        if sym_index >= len(syms):
            return "reloc", None, None
        address, sym_section = resolve_symbol(syms[sym_index], obj["order"], bases, symbols_by_name)
        return "reloc", (None if address is None else address + addend), sym_section
    sec = obj["sections"].get(section)
    if sec and offset + 4 <= len(sec["data"]):
        return "raw", struct.unpack_from(">I", sec["data"], offset)[0], None
    return "raw", None, None


def is_code_pointer(kind: str, value, sym_section, text_ranges) -> bool:
    """A word is a code pointer when its value is an address inside `.text`.

    A relocation whose symbol *is* a code symbol is decisive on its own (that is the same statement and does
    not depend on any address arithmetic); everything else is tested against the DOL's text ranges.
    """
    if sym_section in CODE_SECTIONS:
        return True
    return value is not None and in_ranges(text_ranges, value)


def section_diff(ours: dict, target: dict) -> list:
    """Every non-`.text` section whose size differs, with a `kind` that says which way.

    `missing` is the case this report exists for - a section our object does not carry at all, the one a
    `NonMatching` unit hides completely.
    """
    names = set(ours) | set(target)
    out = []
    for name in names:
        if name in META_SECTIONS or name == ".text" or name.startswith(META_PREFIXES):
            continue
        a, b = ours.get(name, 0), target.get(name, 0)
        if a == b:
            continue
        if a == 0:
            kind = "missing"
        elif b == 0:
            kind = "extra"
        else:
            kind = "short" if a < b else "long"
        out.append({"section": name, "ours": a, "target": b, "kind": kind})
    return sorted(out, key=lambda d: (d["section"], d["kind"]))


def classify_reference(address: int, unit_ranges, all_ranges) -> str:
    """`own` | `foreign` | `external` for a `vtable = <table>` address.

    `external` (no registered range owns it) is rule 10 Case 2 and legal; `foreign` (another unit owns it)
    is that unit's table, legal to reference; `own` is the shape the rule forbids - this unit's own table
    referenced instead of declared.
    """
    if in_ranges(unit_ranges, address):
        return "own"
    if in_ranges(all_ranges, address):
        return "foreign"
    return "external"


def table_belongs_to_unit(blob, named, unit_text_ranges) -> bool:
    """Whether a table at `named = (section, address, size)` is structurally the unit's own vtable.

    The second half of the ownership test, and the one that catches the defect **before** the unit claims
    the range: rule 10's inheritance evidence is the object's structure - the slot addresses read out of the
    DOL - so a table whose code pointers land inside this unit's own `.text` is this unit's table, whether
    the unit has claimed its `.data` yet or not. `NetworkSessionManagerVTable` (0x805FA908, 51 entries, all
    51 in `Network/fn_803D3CE8.cpp`'s `.text`) is exactly that case, and it was `external` - and therefore
    silent - for as long as the unit claimed no `.data`.
    """
    if blob is None or not named or not named[2]:
        return False
    data = dol_read(blob, named[1], named[2])
    if not data:
        return False
    words = [struct.unpack_from(">I", data, i)[0] for i in range(0, len(data) - 3, 4)]
    inside = [w for w in words if in_ranges(unit_text_ranges, w)]
    return len(inside) >= MIN_RUN_WORDS


def symbol_at(symbols_by_name: dict, section: str, address: int):
    """The name `symbols.txt` gives the address, or the nearest symbol at or below it in the same section."""
    for name, row in symbols_by_name.items():
        if row[0] == section and row[1] == address:
            return name
    best = None
    for name, row in symbols_by_name.items():
        if row[0] == section and row[1] <= address and (best is None or row[1] > best[1]):
            best = (name, row[1])
    if best is None:
        return None
    return "%s+0x%X" % (best[0], address - best[1]) if best[1] != address else best[0]


# --------------------------------------------------------------------------------------------------
# the impure edges: read the repository
# --------------------------------------------------------------------------------------------------
def read_text(path: str):
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            return fh.read()
    except OSError:
        return None


def object_paths(main: str, source_path: str):
    """`(our_object, target_object)` for a registered source path - the same convention every tool uses."""
    stem = os.path.splitext(source_path.replace("\\", "/"))[0]
    return (os.path.join(main, "build", GAME, "src", stem + ".o"),
            os.path.join(main, "build", GAME, "obj", stem + ".o"))


def read_object(path: str):
    """`{sections, order, symbols, relocs}` for an ELF32 object, or `None` when it is not one.

    Section headers come from `tools/elf/elfsect.py`. The `.symtab` rows are kept **with their indices**,
    because a relocation names its symbol by index and the reader must not drop a row (the undefined rows
    are exactly the ones an object-only reader has no address for). Relocations are grouped by the section
    they apply to, so a lookup is `relocs[section][offset]`.
    """
    try:
        _, headers = elfsect.sections(path)
    except (OSError, AssertionError, struct.error, ValueError):
        return None
    sections, order = {}, []
    for h in headers:
        name = h["name"]
        order.append(name)
        sections[name] = {"name": name, "typ": h["typ"], "size": h["size"], "link": h["link"],
                          "entsize": h["entsize"], "data": b"" if h["typ"] == 8 else h["data"]}
    obj = {"sections": sections, "order": order, "symbols": [], "relocs": {}}
    st = sections.get(".symtab")
    if st and st["data"]:
        link = st["link"]
        strtab = order[link] if link < len(order) else None
        sdata = sections[strtab]["data"] if strtab in sections else b""
        step = st["entsize"] or 16
        for off in range(0, len(st["data"]) - step + 1, step):
            nm, value, size, info, _other, shndx = struct.unpack_from(">IIIBBH", st["data"], off)
            end = sdata.find(b"\0", nm)
            name = sdata[nm:end].decode("latin-1") if nm and end >= 0 else ""
            obj["symbols"].append({"name": name, "value": value, "size": size,
                                   "bind": (info >> 4) & 0xF, "type": info & 0xF, "shndx": shndx})
    for name, sec in sections.items():
        if not name.startswith(".rela") or not sec["data"]:
            continue
        rows = {}
        step = sec["entsize"] or 12
        for off in range(0, len(sec["data"]) - step + 1, step):
            r_off, r_info, r_add = struct.unpack_from(">IIi", sec["data"], off)
            rows[r_off] = (r_info & 0xFF, r_info >> 8, r_add)
        obj["relocs"][name[5:]] = rows
    return obj


def load_tree(main: str, ref: str | None = None) -> dict:
    """Everything the sweep reads once: the splits, the symbol map and the DOL's `.text` ranges.

    With `ref`, `splits.txt`/`symbols.txt` (and, through `unit_list`, the registered set) come from that
    revision via `git show` - the `--diff` back side is judged by the map it was written against. The DOL
    itself is immutable and always read from the tree.
    """
    def text(rel):
        if ref:
            got = _git(main, "show", "%s:%s" % (ref, rel.replace(os.sep, "/")))
            if got:
                return got
        return read_text(os.path.join(main, rel)) or ""
    splits = parse_splits(text(SPLITS_REL))
    symbols = parse_symbols(text(SYMBOLS_REL))
    blob = None
    dol = os.path.join(main, DOL_REL)
    try:
        with open(dol, "rb") as fh:
            blob = fh.read()
    except OSError:
        blob = None
    text = dol_text_ranges(blob) if blob else []
    source = "dol"
    if not text:                    # a tree with no DOL: the registered code ranges are the fallback
        text = merge_ranges([(r["start"], r["end"]) for u in splits.values() for r in u
                             if r["section"] in CODE_SECTIONS])
        source = "splits"
    return {"main": main, "splits": splits, "symbols": symbols, "blob": blob,
            "text_ranges": merge_ranges(text), "text_source": source}


def unit_list(main: str, tree: dict, ref: str | None = None) -> list:
    """`[{"path", "flag"}]` - `configure.py`'s registered units, or `ref`'s splits when there is a ref.

    The `--diff` back side needs the unit set that revision registered, and `splits.txt` is the file that
    says which ranges a unit owns - a unit with a splits block and no `configure.py` line has no runs to
    audit anyway, and one with a line and no block has no ranges.
    """
    if ref:
        return [{"path": p, "flag": ""} for p in sorted(tree["splits"])]
    return langcheck.registered_units(main)


# --------------------------------------------------------------------------------------------------
# the definition index: a member at +0x00 that holds a function-pointer table
# --------------------------------------------------------------------------------------------------
_BLANKERS = (re.compile(r"/\*.*?\*/", re.S), re.compile(r"//[^\n]*"),
             re.compile(r'"(?:\\.|[^"\\])*"'), re.compile(r"'(?:\\.|[^'\\])*'"))


def blank_literals(text: str) -> str:
    """Replace comments and string/char literals with same-length blanks, keeping newlines.

    Brace matching and field splitting must not see a `{` inside a comment or a `;` inside a literal - a
    declaration comment with an address in it is common here. Positions are preserved, so a reported line
    is still the source's line.
    """
    def rep(m):
        return "".join("\n" if c == "\n" else " " for c in m.group(0))
    for rx in _BLANKERS:
        text = rx.sub(rep, text)
    return text


def _match_brace(code: str, open_pos: int):
    """Index of the `}` matching the `{` at `open_pos`, or None. Comment/literal-free input."""
    depth = 0
    for i in range(open_pos, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return None


def _members(body: str):
    """`[(type_text, name, is_fn_ptr)]` for the top-level `;`-terminated members of a class body.

    Nested braces (an inline function body, an anonymous union) are skipped as one chunk, and a member whose
    declarator ends in `= ...` (an initializer) keeps the name before the `=`. An access specifier is not a
    member.
    """
    out, start, depth, i = [], 0, 0, 0
    while i < len(body):
        c = body[i]
        if c in "{({":
            depth += 1
        elif c in "})":
            depth = max(0, depth - 1)
        elif c == ";" and depth == 0:
            chunk = body[start:i].strip()
            start = i + 1
            if chunk:
                head = chunk.split("=", 1)[0].strip()
                fp = FN_PTR_RE.search(head)
                if fp:                              # `RET (*name)(args);` - a function pointer member
                    out.append((head[:fp.start()] + head[fp.end():], fp.group("name"), True))
                else:
                    m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$", head)
                    if m and not re.fullmatch(r"(public|private|protected|virtual)", m.group(1)):
                        out.append((head[:m.start(1)] + head[m.end(1):], m.group(1), False))
        i += 1
    return out


def type_definitions(texts: dict) -> dict:
    """`{type_name: [(member_type_text, member_name, source_file)]}` for every struct/class in `texts`.

    `texts` is `{path: text}`; a typedef'd anonymous definition takes the name after the closing brace. A
    heavier parser would be wrong here - the index feeds a heuristic, and the shape it needs ("is this
    member a pointer to a struct of function pointers") is decidable from the declarators alone.
    """
    defs = {}
    for path, text in texts.items():
        code = blank_literals(text)
        for m in CLASS_OPEN_RE.finditer(code):
            close = _match_brace(code, m.end() - 1)
            if close is None:
                continue
            name = m.group(2)
            if not name:
                tail = re.match(r"\s*([A-Za-z_]\w*)?\s*;", code[close + 1:close + 120])
                name = (tail.group(1) if tail else None) if tail else None
            if not name:
                continue
            defs.setdefault(name, []).extend(
                (t.strip(), n, path, fn) for t, n, fn in _members(code[m.end():close]))
    return defs


def _is_fn_table(members) -> bool:
    """Whether a type is a table of CODE pointers: >=2 function pointers and nothing else but RTTI/padding.

    The tight half of the heuristic, and the reason it is not just "points at something callable": a linked
    list's `next` (pointing at a struct that happens to hold a destructor) and a module record with function
    pointers among its fields are both *data* structures, not vtables. A vtable is function pointers plus the
    two RTTI words plus `pad` slots for the ones this file did not name.
    """
    if sum(1 for m in members if m[3]) < 2:
        return False
    for decl, name, _path, is_fn in members:
        if is_fn:
            continue
        if decl.strip() in ("void*", "char*", "void *"):
            continue
        if re.fullmatch(r"(u8|u16|u32|u64|s8|s16|s32)( \[[^\]]*\])?", decl.strip()) \
                and re.match(r"(rtti|pad|unused|unknown|reserved|_)", name):
            continue
        return False
    return True


def fn_table_fields(defs: dict) -> dict:
    """`{member_name: {"pointee", "file", "type"}}` for every definition whose FIRST member is a
    fn-table pointer.

    The owner's heuristic, made mechanical: a struct/class whose member at `+0x00` (the first one, or one
    carrying an explicit `+0x00` offset) is a pointer to a type whose members are function pointers is a
    class with inheritance. The member's NAME is whatever the source called it - the point is that no scan
    keys on a spelling any more.
    """
    fn_tables = {name for name, members in defs.items() if _is_fn_table(members)}
    out = {}
    for name, members in defs.items():
        if not members:
            continue
        for index, (decl, field, _path, _fn) in enumerate(members):
            if index and "+0x00" not in decl:
                break
            pointee = _pointee_type(decl)
            if pointee is None:
                break
            if pointee in fn_tables:
                out.setdefault(field, {"pointee": pointee, "file": _path, "type": name})
            break
    return out


def _pointee_type(decl: str):
    """The struct name a member declarator points at (`Foo* f[2]` -> `Foo`), or None."""
    decl = decl.replace("const ", " ").strip()
    m = re.match(r"(struct\s+|class\s+)?([A-Za-z_]\w*)\s*\*", decl)
    return m.group(2) if m else None


# --------------------------------------------------------------------------------------------------
# the source scan
# --------------------------------------------------------------------------------------------------
def working_texts(main: str) -> dict:
    """`{relpath: text}` for every source and header in the working tree - the definition index's input.

    `include/` matters as much as `src/`: the class a member belongs to is usually declared in a header
    (`include/Network/fn_803D3CE8.h` holds `NetworkSessionManager`), and reading only the `.cpp` would miss
    the member entirely.
    """
    out = {}
    for top in ("src", "include"):
        root_dir = os.path.join(main, top)
        for dirpath, dirnames, filenames in os.walk(root_dir):
            dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
            for fn in sorted(filenames):
                if os.path.splitext(fn)[1] not in SOURCE_EXT + (".h", ".hpp"):
                    continue
                path = os.path.join(dirpath, fn)
                text = read_text(path)
                if text is not None:
                    out[os.path.relpath(path, main).replace("\\", "/")] = text
    return out


def scan_text_assignments(text: str, fields: dict) -> list:
    """Every `->member = <symbol>;` in one source text whose member is a `+0x00` fn-table pointer.

    Comments and string literals are blanked first (`blank_literals`), so a header comment that *quotes*
    the defect - `self->vtable = &NetworkSessionManagerVTable;` - is not read as one. Positions are
    preserved, so the reported line is the source's.
    """
    out = []
    for lineno, line in enumerate(blank_literals(text).splitlines(), 1):
        for m in VTABLE_ASSIGN_RE.finditer(line):
            field = m.group("field")
            if field not in fields and field not in LEGACY_VTABLE_FIELDS:
                continue
            out.append({"line": lineno, "field": field, "symbol": m.group("sym"),
                        "cast": bool(m.group("amp"))})
    return out


def scan_source_references(main: str, fields: dict, symbols_by_name: dict,
                          ref: str | None = None) -> list:
    """Every fn-table-pointer assignment in `src/`, classified by the OWNERSHIP of its symbol.

    The working tree is walked; with `ref` the file list comes from `git grep` at that revision and each
    hit's text from `git show`, which is the `--diff` back side. The reference's address comes from
    `symbols.txt` (any spelling), falling back to the `_XXXXXXXX` suffix a `lbl_`/`fn_` name carries.
    """
    out = []
    if ref:
        # `git grep -l` prefixes every hit with the revision (`HEAD:src/x.cpp`); the blob is then read with
        # `git show`, so a file that HEAD does not have is simply not in the list.
        paths = [line.split(":", 1)[-1]
                 for line in _git(main, "grep", "-l", "-E", _GREP_PATTERN, ref, "--", "src").splitlines()]
        paths = [p for p in paths if os.path.splitext(p)[1] in SOURCE_EXT]
        if not paths:
            return out
        files = [(p, _git(main, "show", "%s:%s" % (ref, p))) for p in paths]
    else:
        files = []
        for dirpath, dirnames, filenames in os.walk(os.path.join(main, "src")):
            dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
            for fn in sorted(filenames):
                if os.path.splitext(fn)[1] not in SOURCE_EXT:
                    continue
                path = os.path.join(dirpath, fn)
                rel = os.path.relpath(path, main).replace("\\", "/")
                text = read_text(path)
                if text is not None:
                    files.append((rel, text))
    for rel, text in files:
        rel = rel.replace("\\", "/")
        unit = rel[len("src/"):] if rel.startswith("src/") else rel
        for hit in scan_text_assignments(text, fields):
            address, section = _symbol_address(hit["symbol"], symbols_by_name)
            out.append(dict(hit, file=rel, unit=unit, symbol_section=section,
                            address=address))
    return out


# the `git grep` prefilter for the `--diff` side: a member assignment, whatever the table is called
_GREP_PATTERN = r"(->|\\.)[ \t]*[A-Za-z_][A-Za-z0-9_]*[ \t]*=[^=]"


def _symbol_address(name: str, symbols_by_name: dict):
    """`(address, section)` for a source symbol, from the map first and the `_XXXXXXXX` suffix second."""
    named = symbols_by_name.get(name)
    if named:
        return named[1], named[0]
    m = HEX_SUFFIX_RE.search(name)
    return (int(m.group(1), 16), None) if m else (None, None)


def _git(root: str, *args: str) -> str:
    """`git <args>` stdout, or `""` - a missing ref or a file absent at it is a fact, not a crash."""
    try:
        p = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, encoding="utf-8",
                           errors="replace")
    except OSError:
        return ""
    return p.stdout if p.returncode == 0 else ""


# --------------------------------------------------------------------------------------------------
# the sweep
# --------------------------------------------------------------------------------------------------
def audit_unit(tree: dict, path: str, flag: str) -> dict:
    """One unit's record: its owned runs, their verdicts, and its section-size differences."""
    main = tree["main"]
    our_path, target_path = object_paths(main, path)
    own = tree["splits"].get(path, [])
    our = read_object(our_path)
    target = read_object(target_path)
    rel = lambda p: os.path.relpath(p, main).replace("\\", "/")
    rec = {"unit": path, "flag": flag, "our": rel(our_path), "target": rel(target_path),
           "status": "ok", "runs": [], "sections": [], "missing": [], "range_mismatch": []}

    if our is None or target is None:
        rec["status"] = "unbuilt"
        rec["missing"] = [rel(p) for p, o in ((our_path, our), (target_path, target)) if o is None]
        return rec

    # -- (c) section completeness, ours vs the target, every non-.text section ------------------------
    ours_sizes = {n: s["size"] for n, s in our["sections"].items()}
    target_sizes = {n: s["size"] for n, s in target["sections"].items()}
    rec["sections"] = section_diff(ours_sizes, target_sizes)

    # -- (a)+(b) the owned runs and their verdict ------------------------------------------------
    our_bases = {r["object"]: r["start"] for r in own}
    for rng in own:
        section, object_section = rng["section"], rng["object"]
        start, end = rng["start"], rng["end"]
        if section not in DATA_SECTIONS:
            continue
        sec = target["sections"].get(object_section)
        if sec is None or not sec["data"]:
            rec["missing"].append("target:" + object_section)
            continue
        if sec["size"] != end - start:
            rec["range_mismatch"].append({"section": object_section, "splits": end - start,
                                          "object": sec["size"]})
        count = min(sec["size"], end - start) // 4
        resolved = [read_word(target, object_section, 4 * i, our_bases, tree["symbols"])
                    for i in range(count)]
        flags = [is_code_pointer(k, v, s, tree["text_ranges"]) for k, v, s in resolved]
        for first, words in find_runs(flags):
            address = start + 4 * first
            run = {"unit": path, "flag": flag, "section": object_section,
                   "kind": RUN_KIND.get(section, "table"), "address": address, "words": words,
                   "symbol": symbol_at(tree["symbols"], object_section, address),
                   "targets": [resolved[first + i][1] for i in range(words)],
                   "verdict": None, "emitted": None, "referenced": None, "values_match": None,
                   "our_section": None}
            _verdict_run(run, object_section, first, words, start, our, our_bases, tree["symbols"],
                         tree["text_ranges"])
            rec["runs"].append(run)

    rec["verdicts"] = {v: sum(1 for r in rec["runs"] if r["verdict"] == v)
                       for v in ("emitted", "referenced", "violation", "n/a")}
    return rec


def _verdict_run(run, section, first, words, address, our, our_bases, symbols_by_name, text_ranges):
    """Fill in whether OUR object emits or references this target-side run.

    `emitted` = our object carries the same section, covers the run, and every word of it is a code
    pointer (so MWCC/the splitter put a table there). `values_match` says whether the entries are the
    *same* code in the same order - a difference there is a matching defect the score already owns, not a
    rule-10 one, so it is recorded rather than flagged. `referenced` = some relocation anywhere in our
    object resolves to an address inside the run (a store of the table's address is the usual shape).
    """
    sec = our["sections"].get(section)
    run["our_section"] = sec["size"] if sec else 0
    emitted, values_match = False, False
    if sec and sec["size"] >= 4 * (first + words):
        mine = [read_word(our, section, 4 * (first + i), our_bases, symbols_by_name)
                for i in range(words)]
        emitted = all(is_code_pointer(k, v, s, text_ranges) for k, v, s in mine)
        their = run["targets"]
        values_match = emitted and [m[1] for m in mine] == their
    start, end = address, address + 4 * words
    referenced = any(start <= value < end
                     for other_sec, rows in our["relocs"].items()
                     for offset in rows
                     for _k, value, _s in [read_word(our, other_sec, offset, our_bases, symbols_by_name)]
                     if value is not None)
    run["emitted"], run["referenced"] = emitted, referenced
    run["values_match"] = bool(values_match)
    if run["kind"] != "table":
        # An initializer (`.ctors`/`.dtors`) or exception (`extab`/`extabindex`) run is not a vtable and cannot
        # be one - it is a run of code pointers the linker/compiler puts there. Its absence from our object is
        # real, but it belongs to form (c), section completeness, so it is not a rule-10 violation here (the
        # by-hand audit called exactly these its detector's "honest false positives").
        run["verdict"] = "n/a"
    elif emitted:
        run["verdict"] = "emitted"
    elif referenced:
        run["verdict"] = "referenced"
    else:
        run["verdict"] = "violation"


def sweep(main: str, only: str | None = None, text_ref: str | None = None) -> dict:
    """Audit every registered unit. Returns the runs, the source references, the section diffs and counts.

    `text_ref` judges the text half (`configure.py`/`symbols.txt`/`splits.txt`/`src/**`) as of that git
    revision - the `--diff` back side. The object half is the working tree's on both sides, so the diff sees
    an ownership change (a new `.data` claim) exactly; see the module docstring for what that implies.
    """
    main = os.path.abspath(main)
    t0 = time.time()
    tree = load_tree(main, ref=text_ref)
    units = unit_list(main, tree, ref=text_ref)
    if only:
        want = only.replace("\\", "/").strip("/")
        units = [u for u in units
                 if u["path"] == want or os.path.splitext(u["path"])[0] == os.path.splitext(want)[0]]
    records = [audit_unit(tree, u["path"], u.get("flag", "")) for u in units]

    all_ranges = merge_ranges([(r["start"], r["end"]) for u in tree["splits"].values() for r in u])
    fields = fn_table_fields(type_definitions(working_texts(main)))
    refs = []
    for ref in scan_source_references(main, fields, tree["symbols"], ref=text_ref):
        ranges = merge_ranges([(r["start"], r["end"]) for r in tree["splits"].get(ref["unit"], [])])
        text_ranges = merge_ranges([(r["start"], r["end"]) for r in tree["splits"].get(ref["unit"], [])
                                    if r["section"] in CODE_SECTIONS])
        structural = False
        if ref["address"] is None:
            kind = "unresolved"
        else:
            kind = classify_reference(ref["address"], ranges, all_ranges)
            if kind != "own" and table_belongs_to_unit(tree["blob"], tree["symbols"].get(ref["symbol"]),
                                                       text_ranges):
                # the table's OWN entries are this unit's code: it is this unit's vtable even though the
                # range is unclaimed, which is the state the hand assignment shipped in
                kind, structural = "own", True
        refs.append(dict(ref, kind=kind, structural=structural))
    refs = [r for r in refs if not only or _same_unit(r["unit"], only)]

    runs = [r for rec in records for r in rec["runs"]]
    sections = [dict(d, unit=rec["unit"]) for rec in records for d in rec["sections"]]
    violations = [r for r in runs if r["verdict"] == "violation"]
    return {
        "root": main,
        "text_source": tree["text_source"],
        "text_ref": text_ref,
        "units_total": len(records),
        "units_built": sum(1 for r in records if r["status"] != "unbuilt"),
        "unbuilt": [r for r in records if r["status"] == "unbuilt"],
        "records": records,
        "runs": runs,
        "run_kinds": {k: sum(1 for r in runs if r["kind"] == k) for k in ("table", "initializer", "exception")},
        "run_verdicts": {v: sum(1 for r in runs if r["verdict"] == v)
                         for v in ("emitted", "referenced", "violation", "n/a")},
        "violations": violations,
        "references": refs,
        "reference_kinds": {k: sum(1 for r in refs if r["kind"] == k)
                            for k in ("external", "foreign", "own", "unresolved")},
        "fn_table_fields": {k: v for k, v in sorted(fields.items())},
        "sections": sections,
        "section_kinds": {k: sum(1 for d in sections if d["kind"] == k)
                          for k in ("missing", "extra", "short", "long")},
        "units_with_section_diff": len({d["unit"] for d in sections}),
        "range_mismatch": [dict(m, unit=r["unit"]) for r in records for m in r["range_mismatch"]],
        "absent_target_sections": [{"unit": r["unit"], "sections": r["missing"]}
                                   for r in records if r["missing"] and r["status"] == "ok"],
        "elapsed_s": round(time.time() - t0, 3),
    }


def _same_unit(path: str, spec: str) -> bool:
    a, b = path.replace("\\", "/"), spec.replace("\\", "/").strip("/")
    return a == b or os.path.splitext(a)[0] == os.path.splitext(b)[0]


def violation_rows(s: dict, rename: dict | None = None) -> dict:
    """`{key: {"unit", "where", "kind"}}` for every rule-10 violation in a sweep result.

    The keys are `--diff`'s comparison unit, and they are deliberately **rename-stable**, because this
    campaign re-homes placeholder-path units routinely (plan §12: `fn_80429B94.cpp` ->
    `Network/network_pat_control.cpp`), and a whole-tree diff keyed by path read one such rename as seven
    ADDED violations:

    * `run:<section>:<address>` - an address is unique in the DOL (two units cannot own the same range)
      and a re-home keeps it, so the unit name does not belong in the key. Keying on the unit made the
      same seven `.data` runs under a new path read as seven additions.
    * `ref:<file>:<line>:<symbol>` - `<file>` is translated through `rename` (`{path_at_ref: path_now}`,
      `rename_map`) so the base side's copy lines up with the working tree's. This is the same move
      `stylelint.py`'s `rule1_counts_at_ref` makes for a renamed `src/` file.
    """
    rename = rename or {}
    rows: dict = {}
    for run in s["violations"]:
        key = "run:%s:%08X" % (run["section"], run["address"])
        rows[key] = {"unit": run["unit"], "kind": "run",
                     "where": "%s %s 0x%08X (%d words)" % (run["unit"], run["section"],
                                                           run["address"], run["words"])}
    for ref in s["references"]:
        if ref["kind"] != "own":
            continue
        key = "ref:%s:%d:%s" % (rename.get(ref["file"], ref["file"]), ref["line"], ref["symbol"])
        rows[key] = {"unit": ref["unit"], "kind": "ref",
                     "where": "%s:%d assigns %s" % (ref["file"], ref["line"], ref["symbol"])}
    return rows


def violation_keys(s: dict, rename: dict | None = None) -> list:
    """Stable identifiers for every rule-10 violation in a sweep result - the `--diff` comparison's unit.

    `land.py`'s gate row refuses a batch whose set *grew* - an existing violation is grandfathered exactly
    the way the lint's `--diff` grandfathers a finding - so the existing ones (`fn_80423E74.cpp`,
    `ai/fn_802CC794.cpp`, `enemy/em_act_step.cpp`'s own-range assignment) do not refuse every batch
    forever. `rename` maps a path at the compared ref to the path it has now (`rename_map`).
    """
    return sorted(violation_rows(s, rename))


def rename_map(main: str, ref: str) -> dict:
    """`{path_at_ref: path_now}` for every `src/` rename the working tree made against `ref`.

    The base side of `--diff` scans the files the ref carried, so a unit the batch re-homed keeps its old
    path in `ref:` keys unless it is translated here. `git diff -M` is the same rename detection the lint
    uses (`stylelint.py`'s `changed_src_files`), so both tools agree on what a rename is.
    """
    out: dict = {}
    for line in _git(main, "diff", "--name-status", "-M", "--diff-filter=d", ref, "--", "src").splitlines():
        parts = line.split("\t")
        if len(parts) >= 3 and parts[0].startswith("R"):
            out[parts[1]] = parts[2]
    return out


# --------------------------------------------------------------------------------------------------
# `--at`: one vtable, read out of the DOL, with the owner of every target
# --------------------------------------------------------------------------------------------------
def owner_at(tree: dict, address: int, sections=None) -> str | None:
    """The registered unit whose range covers `address`, or None.

    `sections` limits the search (`.text` for a code-pointer target); None searches every range. The
    hand-built slot censuses matched owners by *name* and got 62 of one vtable's 114 slots wrong, so the
    lookup here is by address - the one thing a rename or a re-split cannot move.
    """
    for unit, ranges in tree["splits"].items():
        for r in ranges:
            if sections is not None and r["section"] not in sections:
                continue
            if r["start"] <= address < r["end"]:
                return unit
    return None


def vtable_slots(tree: dict, address: int, max_words: int = 512) -> list:
    """The vtable at `address`, read out of the DOL: one row per slot.

    The DOL is the linked image, so each slot already holds the *resolved* target address (no relocation
    to chase, and no `parse_elf` on this side). A word that is not a code pointer ends the table, and
    fewer than `MIN_RUN_WORDS` slots is not a table. Each row carries the slot address, the target, the
    target's **owner** (by address, `owner_at`) and the symbol `symbols.txt` names at it - the census a
    lane hand-built twice and got wrong the second time.
    """
    blob = tree.get("blob")
    if not blob:
        return []
    # a registered range that covers the address bounds the table: the DOL has no section boundary for a
    # vtable, so without this the walk reads past it into the next table's pointers.
    limit = max_words
    for ranges in tree["splits"].values():
        for r in ranges:
            if r["start"] <= address < r["end"]:
                limit = min(limit, (r["end"] - address) // 4)
                break
    rows = []
    for i in range(limit):
        raw = dol_read(blob, address + 4 * i, 4)
        if raw is None or len(raw) < 4:
            break
        value = struct.unpack(">I", raw)[0]
        if not is_code_pointer("raw", value, None, tree["text_ranges"]):
            break
        rows.append({"index": i, "address": address + 4 * i, "target": value,
                     "owner": owner_at(tree, value, sections=CODE_SECTIONS),
                     "symbol": symbol_at(tree["symbols"], ".text", value)})
    return rows if len(rows) >= MIN_RUN_WORDS else []


def reference_slots(main: str, tree: dict, address: int, count: int) -> dict:
    """`{slot_index: symbol}` from the **reference object**'s relocations over the table at `address`.

    The DOL resolves addresses, so the symbol behind a slot is only in the reference object's `.rela`
    rows - and `dossier.parse_elf` is the one ELF reader (this tool's `read_object` re-implements it; the
    mode exists so nobody writes a third). `{}` when the table's unit has no built object.
    """
    unit = owner_at(tree, address)
    if unit is None:
        return {}
    rng = next((r for r in tree["splits"][unit] if r["start"] <= address < r["end"]), None)
    if rng is None:
        return {}
    obj_path = os.path.join(main, "build", GAME, "obj", os.path.splitext(unit)[0] + ".o")
    try:
        with open(obj_path, "rb") as fh:
            blob = fh.read()
    except OSError:
        return {}
    try:
        _sections, _symbols, relocs = dossier.parse_elf(blob)
    except ValueError:
        return {}
    base_off = address - rng["start"]
    out = {}
    # the relocation section's own name (`.rela<object>`) is the robust match: `parse_elf`'s `target`
    # comes from `sh_info`, which a hand-built fixture may leave 0, while the section name is always set.
    rela_name = ".rela" + rng["object"]
    for rel in relocs:
        if rel["section"] != rela_name or not (base_off <= rel["offset"] < base_off + 4 * count):
            continue
        name = rel["symbol"] or ""
        if rel["addend"]:
            name = "%s + 0x%X" % (name, rel["addend"])
        out[(rel["offset"] - base_off) // 4] = name
    return out


# --------------------------------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------------------------------
def render_run(run: dict, out=sys.stdout) -> None:
    detail = ", values match" if run["values_match"] else \
             ("" if run["verdict"] in ("n/a", None, "emitted") else ", values differ")
    if run["verdict"] in ("referenced", "violation"):
        detail += ", our %s is %d bytes" % (run["section"], run["our_section"] or 0)
    print("    [%-11s] %-44s %-10s 0x%08X  %2d words  %-9s %s%s%s"
          % (run["kind"], run["unit"], run["section"], run["address"], run["words"],
             run["flag"] or "-", run["verdict"], detail,
             ("  (%s)" % run["symbol"] if run["symbol"] else "")), file=out)


def render(s: dict, out=sys.stdout, show_runs=True, show_refs=True, show_sections=True) -> None:
    print("vtableaudit: %d registered units, %d built, %d unbuilt (text ranges from %s; elapsed %.2fs)"
          % (s["units_total"], s["units_built"], len(s["unbuilt"]), s["text_source"], s["elapsed_s"]),
          file=out)
    for r in s["unbuilt"]:
        print("  unbuilt %s: missing %s" % (r["unit"], ", ".join(r["missing"])), file=out)

    runs = s["runs"]
    print("  owned code-pointer runs (>=%d consecutive words in the unit's own ranges, all targets in .text): %d"
          % (MIN_RUN_WORDS, len(runs)), file=out)
    print("    %d data tables (a jump table or a vtable lives here), %d initializer (.ctors/.dtors), "
          "%d exception (extab/extabindex) - the last two are not vtables and are checked by (c)"
          % (s["run_kinds"]["table"], s["run_kinds"]["initializer"], s["run_kinds"]["exception"]),
          file=out)
    print("    data tables: %d emitted, %d referenced, %d OWNED-BUT-NOT-EMITTED (the rule-10 violation)"
          % (s["run_verdicts"]["emitted"], s["run_verdicts"]["referenced"],
             s["run_verdicts"]["violation"]), file=out)
    print("    (the 2026-09-26 by-hand audit counted the same six: the three .ctors initializer runs plus the "
          "RSO and exception function tables,", file=out)
    print("     and its one genuine look-alike - Pl/pl_master.cpp's compiler-emitted switch table)", file=out)
    if show_runs:
        for run in sorted(runs, key=lambda r: (r["kind"] != "table", r["address"], r["unit"])):
            render_run(run, out=out)
    for run in s["violations"]:
        print("  VIOLATION %s %s 0x%08X: %d code pointers our object neither emits nor references"
              % (run["unit"], run["section"], run["address"], run["words"]), file=out)

    refs = s["references"]
    kinds = s["reference_kinds"]
    print("  fn-table-pointer fields at +0x00 (a class with inheritance): %d"
          % len(s["fn_table_fields"]), file=out)
    for field, info in s["fn_table_fields"].items():
        print("    %-14s -> %-28s %s" % (field, info["pointee"], info["file"]), file=out)
    print("  assignments to such a member anywhere in src/ (any symbol spelling): %d" % len(refs),
          file=out)
    print("    external %d (rule 10 Case 2 - another TU's table, legal), foreign %d, own-range %d "
          "(reported), unresolved %d"
          % (kinds["external"], kinds["foreign"], kinds["own"], kinds["unresolved"]), file=out)
    for ref in refs:
        if ref["kind"] in ("own", "unresolved") or show_refs:
            print("    %-10s %s:%d  %s%s"
                  % (ref["kind"], ref["file"], ref["line"], ref["symbol"],
                     "  (the table's own entries are this unit's code)" if ref.get("structural") else ""),
                  file=out)
    for ref in refs:
        if ref["kind"] == "own":
            print("  VIOLATION %s:%d assigns %s (0x%08X), which THIS unit owns - model the class and let "
                  "MWCC emit the table"
                  % (ref["file"], ref["line"], ref["symbol"], ref["address"]), file=out)

    diffs = s["sections"]
    print("  section completeness (ours vs the target object, every non-.text section): %d differences "
          "over %d units" % (len(diffs), s["units_with_section_diff"]), file=out)
    print("    missing %d (our object has no such section), extra %d, short %d, long %d"
          % (s["section_kinds"]["missing"], s["section_kinds"]["extra"],
             s["section_kinds"]["short"], s["section_kinds"]["long"]), file=out)
    for row in s["range_mismatch"]:
        print("    SPLITS/OBJECT SIZE DISAGREEMENT %s %s: splits %d, object %d"
              % (row["unit"], row["section"], row["splits"], row["object"]), file=out)
    for row in s["absent_target_sections"]:
        print("    REGISTERED RANGE WITH NO TARGET SECTION %s: %s"
              % (row["unit"], ", ".join(row["sections"])), file=out)
    if show_sections:
        for d in sorted(diffs, key=lambda d: (d["section"], d["unit"])):
            print("    %-11s %-44s ours %6d  target %6d  %s"
                  % (d["section"], d["unit"], d["ours"], d["target"], d["kind"]), file=out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--main", default=None, help="tree holding configure.py and build/ (default: this file's)")
    ap.add_argument("--unit", default=None, help="one registered unit (path, with or without extension)")
    ap.add_argument("--runs", action="store_true", help="report only the owned code-pointer runs")
    ap.add_argument("--sections", action="store_true", help="report only the section-size differences")
    ap.add_argument("--fields", action="store_true", help="report only the +0x00 fn-table-pointer fields")
    ap.add_argument("--diff", metavar="REF", default=None,
                    help="compare the working tree with REF; exit 1 when the rule-10 set grows")
    ap.add_argument("--at", metavar="ADDR", default=None,
                    help="read the vtable at ADDR out of the DOL and list its slots with each target's "
                         "owner (the census, not by hand)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        import vtableaudit_selftest
        return vtableaudit_selftest.selftest()
    main_tree = args.main or ROOT
    if args.at is not None:
        try:
            address = int(args.at, 0)
        except ValueError:
            print("vtableaudit: --at wants an address (`0x80050000` or `2147745792`)", file=sys.stderr)
            return 2
        tree = load_tree(main_tree)
        slots = vtable_slots(tree, address)
        refs = reference_slots(main_tree, tree, address, len(slots)) if slots else {}
        if args.json:
            print(json.dumps({"address": address,
                              "slots": [dict(s, reference=refs.get(s["index"])) for s in slots]},
                             indent=2))
            return 0 if slots else 2
        if not slots:
            print("vtableaudit --at 0x%08X: no vtable there (a table needs %d consecutive code pointers; "
                  "check the address, or that the DOL covers it)" % (address, MIN_RUN_WORDS))
            return 2
        print("vtableaudit --at 0x%08X: %d slot(s)" % (address, len(slots)))
        for s in slots:
            print("  +0x%03X  0x%08X  %-28s %-30s %s"
                  % (4 * s["index"], s["target"], s["owner"] or "(unowned)", s["symbol"] or "",
                     ("ref %s" % refs[s["index"]]) if s["index"] in refs else ""))
        return 0
    s = sweep(main_tree, only=args.unit)
    if args.diff is not None:
        back = sweep(main_tree, only=args.unit, text_ref=args.diff)
        # the batch's renames (plan §12 re-homes a unit routinely): the base side scanned the ref's files,
        # so its `ref:` keys have to be translated to the paths the working tree now spells.  Run keys are
        # already keyed on the range, which a rename keeps.
        rename = rename_map(main_tree, args.diff)
        before = violation_keys(back, rename)
        after = violation_keys(s)
        added = sorted(set(after) - set(before))
        if args.json:
            print(json.dumps({"ref": args.diff, "added": added,
                              "before": before, "after": after}, indent=2))
            return 1 if added else 0
        print("vtableaudit --diff %s: %d rule-10 violation(s) before, %d after, %d added"
              % (args.diff, len(before), len(after), len(added)))
        for key in added:
            print("  ADDED %s" % key)
        return 1 if added else 0
    if args.json:
        print(json.dumps(s, indent=2))
    else:
        render(s, show_runs=not (args.sections or args.fields),
               show_refs=not (args.runs or args.sections or args.fields),
               show_sections=not (args.runs or args.fields))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
