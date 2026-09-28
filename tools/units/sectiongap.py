#!/usr/bin/env python3
"""Per-section gap between a unit's target object and ours: size, bytes, and relocations.

A unit can read 100 % in objdiff and still not be the target object. `datagap.py` compares section
*sizes* and looks at the data sections only, so the two compiler-generated exception sections it skips -
`extab` and `extabindex` - are exactly where a flip can die; and a record can be the **right size** with
its relocations at the **wrong offsets**, which no size check can see.

This tool compares a unit's two objects section by section:

    build/RMHE08/obj/<path>.o   the target, split out of the DOL
    build/RMHE08/src/<path>.o   ours, compiled from src/

and prints, per differing section, the three things a lane acts on - the two sizes, the first differing
byte and the differing-byte count, and the **relocation list** (type, offset, symbol name) - in the
`unit  section  ours  target  why` voice `datagap.py` and `vtableaudit.py` use. A clean unit says so.

Usage:
    python tools/units/sectiongap.py --unit Network/NetworkWiiMediator
    python tools/units/sectiongap.py --unit Network/fn_803D3CE8
    python tools/units/sectiongap.py --unit <u> --all-sections   # add .comment/.symtab/.strtab churn
    python tools/units/sectiongap.py --selftest

Two filings asked for this tool:

* **F39** (`datagap.py` ignores `extab`/`extabindex`) - filed first by the `constructNetworkWiiMediator`
  lane and voted again in `.pi/notes/initnetworksessionstable-d599.md` ("Tooling and environment"): on
  the pre-fix object `datagap.py` printed `0 unit(s) listed, 0 of them with a real ours-extra gap` for a
  unit whose object was 16 bytes short in `extab`, because `extab`/`extabindex` are not in its
  `DATA_SECTIONS` set; and `flipcheck.py` reports the sizes and byte counts but not the relocated symbol
  names *inside* the record. The lane's words - "a `sectiongap.py --unit <u>` printing each differing
  section with its reloc names would have named this defect in one command" - are this tool's shape.
* **F41** (`.pi/notes/production-trial.md`, "PIPELINE 1, PHASE 1"): a record can have the **right size**
  with its relocations at the **wrong offsets** - `Network/NetworkWiiMediator` carries the correct `extab`
  0x1a8 with its three `__dl__FPv` relocations at +0x14/+0xac/+0xb4 against the target's
  +0x2c/+0x34/+0x54 - and `Network/fn_803D3CE8` is missing two-thirds of its record (0x2e4 against
  0x4ec). A size check alone cannot see either.

**Why a sibling tool and not a `datagap.py` patch.** Teaching `datagap.py` the two exception sections
would only add a *size* row; `Network/NetworkWiiMediator`'s `extab` is 0x1a8 on **both** sides, so no size
comparison - extended or not - can name F41. Naming it needs each section's bytes and relocation list, a
different question from `datagap.py`'s data-gap scan (and F39's own wording asks for `sectiongap.py`).

Known answers on this tree (both objects present):
    Network/NetworkWiiMediator      -> `extab` same size, `__dl__FPv` relocations at both offset sets
    Network/fn_803D3CE8             -> `extab` 0x2e4 against the target's 0x4ec
    Network/initNetworkSessionStable -> clean (the negative control: `.text`, `extab`, `extabindex` and
        every relocation are identical).

What it does **not** compare, deliberately - the honest limit: section *order* (dtk's synthesised object
lays sections out differently from MWCC's), alignment, the `.comment` active-flags table, and anything
beyond the single object pair. Whether a relocation's name resolves in the link, and what `splits.txt`
claims for a range, are `flipcheck.py`'s questions, not this tool's. A section present on one side only is
reported by name and size; a section whose bytes are shorter is compared over the bytes that exist, and
the size line carries the rest.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
MAIN = os.path.dirname(TOOLS)
sys.path.insert(0, os.path.join(TOOLS, "elf"))
sys.path.insert(0, TOOLS)
import elfsect  # noqa: E402  (the project's object section reader)
import unitutil  # noqa: E402  (unit spec -> build/RMHE08/{src,obj} paths)

# Sections that are bookkeeping, not the unit's code or data. `datagap.py` ignores them for the same
# reason: they move with symbol-table and compiler-version churn, never with the source.
META_SECTIONS = {"", ".comment", ".note.split", ".symtab", ".strtab", ".shstrtab", ".dynsym", ".dynstr"}
# MWCC spells a relocation section `.rela<target>` (`relaextab` targets `extab`).
RELOC_PREFIX = ".rela"

# PPC relocation kinds the objects here carry, by `r_info & 0xff`; anything else is printed numerically.
RELOC_TYPES = {
    0: "R_PPC_NONE", 1: "R_PPC_ADDR32", 2: "R_PPC_ADDR24", 3: "R_PPC_ADDR16",
    4: "R_PPC_ADDR16_LO", 5: "R_PPC_ADDR16_HI", 6: "R_PPC_ADDR16_HA",
    7: "R_PPC_ADDR14", 8: "R_PPC_ADDR14_BRTAKEN", 9: "R_PPC_ADDR14_BRNTAKEN",
    10: "R_PPC_REL24", 11: "R_PPC_REL14", 12: "R_PPC_REL14_BRTAKEN", 13: "R_PPC_REL14_BRNTAKEN",
    14: "R_PPC_GOT16", 18: "R_PPC_PLTREL24", 21: "R_PPC_JMP_SLOT", 24: "R_PPC_UADDR32",
    26: "R_PPC_REL32", 109: "R_PPC_EMB_SDA21", 116: "R_PPC_EMB_RELSDA",
}

# How many relocation differences, and how many offsets of one symbol, a line prints before summarising.
MAX_RELOC_DIFFS = 6
MAX_RELOC_OFFSETS = 8
SHT_NOBITS = 8


def _type_name(typ: int) -> str:
    return RELOC_TYPES.get(typ, "R_PPC_%d" % typ)


def read_object(path: str, all_sections: bool = False) -> dict:
    """`{sections, order, relocs}` for one ELF32 big-endian object.

    `sections` maps a content section name to `{size, data}` (`.rela*` sections are folded into
    `relocs[target]`, a list of `(offset, type, symbol name)`); `order` is the section-header order,
    so the report reads in the object's own layout. Metadata sections are omitted unless
    `all_sections`, and `.bss`/`.sbss` (SHT_NOBITS) keep their size with no bytes.
    """
    _, headers = elfsect.sections(path)
    symtab = next((h for h in headers if h["name"] == ".symtab"), None)
    symbols: list[str] = []
    if symtab and symtab["data"]:
        strtab = headers[symtab["link"]]["data"] if symtab["link"] < len(headers) else b""
        step = symtab["entsize"] or 16
        for off in range(0, len(symtab["data"]) - step + 1, step):
            nm, _val, _size, _info, _other, _shndx = struct.unpack_from(">IIIBBH", symtab["data"], off)
            end = strtab.find(b"\0", nm)
            symbols.append(strtab[nm:end].decode("latin1") if nm and end >= 0 else "")
    order: list[str] = []
    sections: dict[str, dict] = {}
    relocs: dict[str, list] = {}
    for h in headers:
        name = h["name"]
        if name.startswith(RELOC_PREFIX):
            rows = []
            step = h["entsize"] or 12
            for off in range(0, len(h["data"]) - step + 1, step):
                r_offset, r_info, _addend = struct.unpack_from(">IIi", h["data"], off)
                index = r_info >> 8
                rows.append((r_offset, r_info & 0xFF,
                             symbols[index] if index < len(symbols) else "?%d" % index))
            relocs[name[len(RELOC_PREFIX):]] = rows
            continue
        if h["typ"] == 0 or (not all_sections and name in META_SECTIONS):
            continue
        order.append(name)
        sections[name] = {"size": h["size"], "data": b"" if h["typ"] == SHT_NOBITS else h["data"]}
    return {"sections": sections, "order": order, "relocs": relocs}


def _first_byte_diff(mine: bytes, theirs: bytes):
    """First offset where two byte strings differ, or None when the shared prefix is equal."""
    for i in range(min(len(mine), len(theirs))):
        if mine[i] != theirs[i]:
            return i
    return None


def _differing_bytes(mine: bytes, theirs: bytes) -> int:
    """How many bytes differ, counting a length difference as its extra bytes."""
    shared = min(len(mine), len(theirs))
    return sum(1 for i in range(shared) if mine[i] != theirs[i]) + abs(len(mine) - len(theirs))


def _fmt_offsets(pairs: list, limit: int = MAX_RELOC_OFFSETS) -> str:
    """`+0x14, +0xAC, +0xB4 (R_PPC_ADDR32)` for a symbol's `(offset, type)` pairs, capped."""
    pairs = sorted(pairs)
    kinds = {typ for _off, typ in pairs}
    shown = pairs[:limit]
    if len(kinds) == 1 and kinds:
        body = ", ".join("+0x%X" % off for off, _typ in shown) + " (%s)" % _type_name(next(iter(kinds)))
    else:
        body = ", ".join("+0x%X %s" % (off, _type_name(typ)) for off, typ in shown)
    if len(pairs) > limit:
        body += ", ... (%d more)" % (len(pairs) - limit)
    return body


def _group_by_symbol(relocs: list) -> dict:
    out: dict[str, list] = {}
    for offset, typ, symbol in relocs:
        out.setdefault(symbol, []).append((offset, typ))
    return out


def reloc_reasons(ours: list, target: list, limit: int = MAX_RELOC_DIFFS) -> list:
    """The relocation-list difference of one section, as reason strings (empty when identical).

    Relocations are paired by symbol name, because that is the spelling a lane fixes; a name present on
    one side only, or whose `(offset, type)` set moved, is named with both sides' offsets. This is the
    F41 class: equal section sizes whose records sit at different offsets.
    """
    ours_by, target_by = _group_by_symbol(ours), _group_by_symbol(target)
    reasons = []
    for name in sorted(set(ours_by) | set(target_by)):
        mine, theirs = ours_by.get(name), target_by.get(name)
        if mine is None:
            reasons.append("the target relocates `%s` at %s, ours does not" % (name, _fmt_offsets(theirs)))
        elif theirs is None:
            reasons.append("ours relocates `%s` at %s, the target does not" % (name, _fmt_offsets(mine)))
        elif sorted(mine) != sorted(theirs):
            reasons.append("relocations for `%s` differ: ours %s; target %s"
                           % (name, _fmt_offsets(mine), _fmt_offsets(theirs)))
    if len(reasons) > limit:
        reasons = reasons[:limit] + ["... and %d more relocation difference(s)" % (len(reasons) - limit)]
    return reasons


def section_reasons(ours: dict | None, target: dict | None, ours_relocs: list, target_relocs: list) -> list:
    """The reasons one section differs, in the order a lane reads them: size, bytes, relocations."""
    if ours is None:
        return ["missing from our object (the target's section is %d B)" % target["size"]]
    if target is None:
        return ["ours-extra: the target has no such section (%d B)" % ours["size"]]
    reasons = []
    if ours["size"] != target["size"]:
        delta = ours["size"] - target["size"]
        if delta > 0:
            reasons.append("ours-extra 0x%X (%d B): our section is longer" % (delta, delta))
        else:
            reasons.append("target-extra 0x%X (%d B): the target's section is longer" % (-delta, -delta))
    diff = _first_byte_diff(ours["data"], target["data"])
    if diff is not None:
        mine = ours["data"][diff] if diff < len(ours["data"]) else 0
        theirs = target["data"][diff] if diff < len(target["data"]) else 0
        reasons.append("bytes differ at +0x%X (ours %02x, target %02x) in %d of %d bytes"
                       % (diff, mine, theirs, _differing_bytes(ours["data"], target["data"]),
                          max(len(ours["data"]), len(target["data"]))))
    reasons.extend(reloc_reasons(ours_relocs, target_relocs))
    return reasons


def compare_objects(ours: dict, target: dict) -> list:
    """One row per differing section, in the target object's section order.

    Returns `[{"section", "ours", "target", "why"}]` - empty when the two objects agree on every
    compared section's size, bytes and relocations.
    """
    names = list(target["order"]) + [n for n in ours["order"] if n not in target["sections"]]
    rows = []
    for name in names:
        mine, theirs = ours["sections"].get(name), target["sections"].get(name)
        reasons = section_reasons(mine, theirs, ours["relocs"].get(name, []), target["relocs"].get(name, []))
        if reasons:
            rows.append({"section": name, "ours": mine["size"] if mine else 0,
                         "target": theirs["size"] if theirs else 0, "why": "; ".join(reasons)})
    return rows


def _size(n: int) -> str:
    return "0x%X (%d B)" % (n, n)


def selftest() -> int:
    """Delegates to `sectiongap_selftest.py` (fixtures only, no build and no repository state)."""
    import sectiongap_selftest
    return sectiongap_selftest.selftest()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append", default=[],
                    help="unit spec, e.g. Network/NetworkWiiMediator (repeatable)")
    ap.add_argument("--all-sections", action="store_true",
                    help="also compare .comment/.symtab/.strtab/.shstrtab bookkeeping")
    ap.add_argument("--selftest", action="store_true", help="run the fixtures-only selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.unit:
        ap.error("--unit is required (this tool compares one unit's two objects at a time)")

    diffs = units = clean = 0
    printed_header = False
    for spec in args.unit:
        try:
            unit = unitutil.resolve_unit(spec)
        except SystemExit as exc:
            print("sectiongap: cannot resolve %r: %s" % (spec, exc), file=sys.stderr)
            continue
        units += 1
        missing = [p for p in (unit.obj, unit.target) if not os.path.exists(p)]
        if missing:
            print("sectiongap: %s: object missing (%s)" % (unit.name, ", ".join(missing)), file=sys.stderr)
            continue
        rows = compare_objects(read_object(unit.obj, args.all_sections),
                               read_object(unit.target, args.all_sections))
        if not rows:
            clean += 1
            print("%s: clean - every compared section has the same size, the same bytes and the same "
                  "relocations" % unit.name)
            continue
        if not printed_header:
            print("unit  section  ours  target  why")
            printed_header = True
        diffs += len(rows)
        for row in rows:
            print("%s  %s  ours %s  target %s  %s"
                  % (unit.name, row["section"], _size(row["ours"]), _size(row["target"]), row["why"]))

    print("\n%d section difference(s) over %d unit(s); %d clean" % (diffs, units, clean))
    return 0    # a reporting tool: the output is the verdict, the exit status is not


if __name__ == "__main__":
    sys.exit(main())
