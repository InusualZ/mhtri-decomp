#!/usr/bin/env python3
"""Give an MWCC object the map's names for its ``extab``/``extabindex`` entries.

`dtk dol split` names the exception tables it synthesises after the map address each entry
occupies - ``@etb_800093A8`` for the ``extab`` entry at 0x800093A8, ``@eti_800222FC`` for the
``extabindex`` entry at 0x800222FC - and MWCC emits the same bytes under its own anonymous
ordinals (``@905``) with a **local** binding.  While a unit is ``NonMatching`` the target object
``dol split`` writes defines the map's name, so the link resolves; the moment the unit is
``Matching`` our object is the only definition, and if any *other* object in the link relocates
that name ``mwldeppc`` fails with ``undefined: '@eti_800222FC'``.  No source or compiler flag can
reach it: ``@eti_800222FC`` is not an expressible identifier and ``mwcceppc.exe -help`` has no
option that names or exports an extab symbol - the name and the binding are assembler-level
output (``.pi/notes/resfile-flip.md``).

This tool closes that gap as a post-compile step next to ``tools/elf/objalign.py``, chained into
every MWCC rule by ``tools/project.py``.  For each symbol the object defines under an ordinal name
in its ``extab``/``extabindex`` section it computes the address the entry lands at - the section's
claimed start from ``splits.txt`` plus ``st_value`` - renames the symbol to the map's spelling for
that address (``@etb_%08X`` / ``@eti_%08X``, uppercase hex, dtk's own spelling) and sets the
symbol's binding global.  The rename is the half the linker needs for the *name*; the binding is
the half it needs to accept the definition across objects - measured on `g3d/g3d_resfile`: rename
alone, and rename plus a `.comment` `active_flags=0x08`, both still fail; rename **plus** a global
binding links green and reproduces the DOL hash exactly.

Only ``.symtab`` (``st_name``, ``st_info``) and ``.strtab`` (the appended names) are touched: no
section's *contents* change, so the step is byte-neutral for the DOL by construction.  It is a
no-op for an object with no ordinal-named extab/extabindex symbol - the fast path decides that
from the object alone, before reading ``splits.txt`` - and idempotent: a second run sees the map
name rather than an ordinal and skips.

One thing is deliberately left unrepaired: a promoted symbol still sits inside ``.symtab``'s *local*
index range (``sh_info``), because restoring the local/global ordering would renumber the symbol
table and with it every relocation's ``r_info`` symbol index - i.e. rewrite ``.rela*`` contents, the
one thing this step must not touch.  ``mwld`` resolves from ``st_info`` rather than from the
ordering: measured over the whole build, 20 of the already-``Matching`` link inputs carry 94 renamed
symbols and ``main.dol`` stays byte-identical.

Usage:
    python tools/elf/objextab.py <object> [--splits config/RMHE08/splits.txt]
                                           [--unit <splits key>] [-v] [--dry-run]
    python tools/elf/objextab.py --selftest

One summary line per modified object; `-v` also prints every rename.
"""

from __future__ import annotations

import argparse
import os
import re
import struct
import sys

# objalign owns the splits.txt grammar (section names and claimed starts) and the object-path ->
# unit-key rule.  Importing them is deliberate: the two steps must map an object to the same splits
# entry, and a second copy of either rule would drift from the first.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from objalign import parse_splits, unit_key  # noqa: E402

DEFAULT_SPLITS = os.path.join("config", "RMHE08", "splits.txt")

# The two sections `dtk dol split` names after the map, and the prefix it spells them with.
EXTAB_PREFIX = {"extab": "@etb_", "extabindex": "@eti_"}

# MWCC's anonymous name for a compiler-generated symbol.  Only these are candidates; the map's own
# spelling (which means the entry is already done) and ordinary `lbl_*` names are left alone.
ORDINAL_RE = re.compile(r"^@\d+$")

GLOBAL_BINDING = 0x10
BINDING_MASK = 0x0F

SHT_SYMTAB = 2

# ELF32 section header field offsets, and the symbol entry stride.
SH_OFFSET, SH_SIZE = 16, 20
SYM_STRIDE = 16
SYM_ST_NAME, SYM_ST_INFO = 0, 12


def map_name(section: str, start: int, value: int) -> str:
    """The name `dol split` gives the entry at `start + value` of `section`, case included."""
    return "%s%08X" % (EXTAB_PREFIX[section], start + value)


def read_sections(data: bytes) -> tuple[list[dict], int, int]:
    """Parse the ELF32 big-endian section headers into dicts."""
    if data[:4] != b"\x7fELF":
        raise SystemExit("objextab: not an ELF file")
    if data[4] != 1 or data[5] != 2:
        raise SystemExit("objextab: expected a 32-bit big-endian ELF")
    shoff = struct.unpack_from(">I", data, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", data, 0x2E)
    if shentsize < 40 or not shnum or shstrndx >= shnum:
        raise SystemExit("objextab: unexpected section header table")
    shstr_off = struct.unpack_from(">I", data, shoff + shstrndx * shentsize + 0x10)[0]
    sections = []
    for index in range(shnum):
        base = shoff + index * shentsize
        name_off, typ, _flags, _addr, offset, size, link, _info, align, entsize = \
            struct.unpack_from(">IIIIIIIIII", data, base)
        end = data.index(b"\0", shstr_off + name_off)
        sections.append(
            {
                "index": index,
                "name": data[shstr_off + name_off : end].decode("latin1"),
                "type": typ,
                "offset": offset,
                "size": size,
                "link": link,
                "align": align,
                "entsize": entsize,
                "header": base,
            }
        )
    return sections, shoff, shentsize


def ordinal_entries(data: bytes, sections: list[dict]) -> tuple[dict | None, dict | None, list[dict]]:
    """(symtab, strtab, entries) for the ordinal-named extab/extabindex symbols of this object.

    `entries` is empty for an object with nothing to do, and that is decided from the object alone -
    no `splits.txt` read.  That matters: most of the link's objects are C and carry no extab at all.
    """
    wanted = {s["index"]: s["name"] for s in sections if s["name"].lstrip(".") in EXTAB_PREFIX}
    if not wanted:
        return None, None, []
    symtab = next((s for s in sections if s["type"] == SHT_SYMTAB), None)
    if symtab is None or symtab["link"] >= len(sections):
        return None, None, []
    strtab = sections[symtab["link"]]
    strings = data[strtab["offset"] : strtab["offset"] + strtab["size"]]
    entries = []
    for index in range(symtab["size"] // SYM_STRIDE):
        header = symtab["offset"] + index * SYM_STRIDE
        name_off, value, _size, info, _other, shndx = struct.unpack_from(">IIIBBH", data, header)
        if shndx not in wanted:
            continue
        end = strings.find(b"\0", name_off) if name_off < len(strings) else -1
        name = strings[name_off:end].decode("latin1") if end != -1 else ""
        if not ORDINAL_RE.match(name):
            continue
        entries.append(
            {
                "index": index,
                "header": header,
                "name": name,
                "section": wanted[shndx],
                "value": value,
                "info": info,
            }
        )
    return symtab, strtab, entries


def plan(entries: list[dict], starts: dict[str, int], verbose: bool) -> list[tuple[dict, str]]:
    """(entry, map name) for every entry whose section has a claimed start in `starts`."""
    changes = []
    taken: set[str] = set()
    for entry in entries:
        start = starts.get(entry["section"])
        if start is None:
            if verbose:
                print(f"  {entry['name']}: {entry['section']} has no start in splits.txt, left alone")
            continue
        name = map_name(entry["section"], start, entry["value"])
        if name in taken:  # two symbols at one address would be a duplicate global definition
            if verbose:
                print(f"  {entry['name']}: {name} is already used by an earlier entry, left alone")
            continue
        taken.add(name)
        changes.append((entry, name))
    return changes


def write_object(path: str, data: bytearray, sections: list[dict], symtab: dict, strtab: dict,
                 changes: list[tuple[dict, str]]) -> None:
    """Apply the renames in place, growing `.strtab` by the new names.

    The new names are appended to the string table, so every existing `st_name` offset stays valid;
    the table's data is then spliced back into the file at the same position and every later section
    shifts with it.  The shift is padded to the largest alignment among the sections that move, so a
    moved section still satisfies its own `sh_addralign`.  Nothing outside `.symtab`/`.strtab` and
    the section headers that follow `.strtab` is rewritten.
    """
    appended = bytearray()
    name_offsets = []
    for _entry, name in changes:
        name_offsets.append(strtab["size"] + len(appended))
        appended += name.encode("latin1") + b"\0"

    str_off, old_size = strtab["offset"], strtab["size"]
    moved = [s["align"] for s in sections if s["offset"] >= str_off + old_size]
    align = max(moved) if moved else 1
    if align > 1:
        appended += b"\0" * ((-len(appended)) % align)

    for (entry, _name), name_off in zip(changes, name_offsets):
        struct.pack_into(">I", data, entry["header"] + SYM_ST_NAME, name_off)
        struct.pack_into(">B", data, entry["header"] + SYM_ST_INFO,
                         GLOBAL_BINDING | (entry["info"] & BINDING_MASK))

    delta = len(appended)
    out = bytearray(data[:str_off]) + bytes(data[str_off : str_off + old_size]) + bytes(appended) \
        + bytearray(data[str_off + old_size :])

    def shifted(position: int) -> int:
        return position + delta if position > str_off else position

    struct.pack_into(">I", out, 0x1C, shifted(struct.unpack_from(">I", out, 0x1C)[0]))  # e_phoff
    struct.pack_into(">I", out, 0x20, shifted(struct.unpack_from(">I", out, 0x20)[0]))  # e_shoff
    for section in sections:
        header = shifted(section["header"])
        offset = shifted(section["offset"])
        struct.pack_into(">I", out, header + SH_OFFSET, offset)
        if section is strtab:
            struct.pack_into(">I", out, header + SH_SIZE, old_size + delta)

    with open(path, "r+b") as handle:
        handle.write(out)


def rename_object(path: str, starts: dict[str, int], verbose: bool, dry_run: bool) -> int:
    """Rename the object's ordinal extab/extabindex symbols to the map's names; return the count."""
    with open(path, "rb") as handle:
        data = bytearray(handle.read())
    sections, _shoff, _shentsize = read_sections(data)
    symtab, strtab, entries = ordinal_entries(data, sections)
    if not entries:
        return 0
    candidates = plan(entries, starts, verbose)
    if not candidates:
        return 0
    for entry, name in candidates:
        if verbose:
            print(f"  {entry['name']} -> {name} ({entry['section']} +0x{entry['value']:X}, global)")
    if dry_run:
        print(f"objextab: {path}: would rename {len(candidates)} symbol(s) to the map's names")
        return len(candidates)
    write_object(path, data, sections, symtab, strtab, candidates)
    print(f"objextab: {path}: renamed {len(candidates)} extab/extabindex symbol(s) "
          f"to the map's names (global)")
    return len(candidates)


def selftest() -> int:
    failures = 0

    def check(label: str, got, want) -> None:
        nonlocal failures
        if got != want:
            failures += 1
            print(f"  FAIL {label}: got {got!r}, want {want!r}")

    # The naming rule, against symbols dtk itself wrote (all three verified in the build's target
    # objects: `@etb_800093A8` in obj/g3d/g3d_resfile.o, `@eti_800222FC` at extabindex +0x1A4,
    # `@etb_80008080` in obj/g3d/g3d_calcvtx.o at extab +0x78).
    check("map_name extab", map_name("extab", 0x800093A8, 0x0), "@etb_800093A8")
    check("map_name extabindex", map_name("extabindex", 0x80022158, 0x1A4), "@eti_800222FC")
    check("map_name calcvtx", map_name("extab", 0x80008008, 0x78), "@etb_80008080")
    check("map_name uppercase hex", map_name("extabindex", 0x80030000, 0x3C), "@eti_8003003C")

    units = {"Pl/fn_8023C2D0.cpp": {"text": 0x8023C2D0, "extab": 0x80010000}}
    check("unit_key", unit_key("build/RMHE08/src/Pl/fn_8023C2D0.o", units), "Pl/fn_8023C2D0.cpp")
    check("unit_key (windows)", unit_key("build\\RMHE08\\src\\Pl\\fn_8023C2D0.o", units),
          "Pl/fn_8023C2D0.cpp")
    check("unit_key (unknown)", unit_key("build/RMHE08/src/Pl/nope.o", units), None)

    import tempfile

    def fixture() -> bytes:
        """An MWCC-shaped object: .text with ordinal-free symbols, extab/extabindex ordinals,
        a `.sdata` ordinal (not a candidate) and a section behind `.strtab` to shift."""
        return build_elf(
            sections=[(".text", bytes(range(0x20)), 4),
                      ("extab", bytes(0x10), 4),
                      ("extabindex", bytes(0x18), 4),
                      (".sdata", bytes(4), 8),
                      (".comment", b"CodeWarrior\x0e" + bytes(0x20), 1)],
            # locals first, globals last - the order MWCC writes and `sh_info` describes
            symbols=[("lbl_1", 0x04, 0x01, ".text"),        # local, untouched
                     ("@376", 0x00, 0x01, "extab"),         # -> @etb_80010000
                     ("@398", 0x08, 0x03, "extab"),         # -> @etb_80010008, keeps size+object
                     ("@377", 0x00, 0x01, "extabindex"),    # -> @eti_80020000
                     ("@191", 0x00, 0x01, ".sdata"),        # ordinal outside extab: untouched
                     ("fn_A", 0x00, 0x12, ".text")],        # global, untouched
        )

    def symtab_of(path: str) -> dict[str, tuple[str, int]]:
        """{name: (section, st_info)} straight from the file, as flipcheck reads it."""
        data = open(path, "rb").read()
        sections, _shoff, _size = read_sections(data)
        symtab = next(s for s in sections if s["type"] == SHT_SYMTAB)
        strtab = sections[symtab["link"]]
        strings = data[strtab["offset"] : strtab["offset"] + strtab["size"]]
        out = {}
        for i in range(symtab["size"] // SYM_STRIDE):
            name_off, _value, _size, info, _other, shndx = \
                struct.unpack_from(">IIIBBH", data, symtab["offset"] + i * SYM_STRIDE)
            end = strings.index(b"\0", name_off)
            out[strings[name_off:end].decode()] = (sections[shndx]["name"], info)
        return out

    def section_bytes(path: str) -> dict[str, bytes]:
        data = open(path, "rb").read()
        sections, _shoff, _size = read_sections(data)
        return {s["name"]: data[s["offset"] : s["offset"] + s["size"]] for s in sections}

    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "unit.o")
        with open(path, "wb") as handle:
            handle.write(fixture())
        before_sections = section_bytes(path)
        before_symbols = symtab_of(path)

        starts = {"extab": 0x80010000, "extabindex": 0x80020000}
        check("rename count", rename_object(path, starts, False, False), 3)

        after_symbols = symtab_of(path)
        check("map name for extab entry", after_symbols.get("@etb_80010000"), ("extab", 0x11))
        check("map name for the second extab entry (binding bits kept)",
              after_symbols.get("@etb_80010008"), ("extab", 0x13))
        check("map name for extabindex entry", after_symbols.get("@eti_80020000"), ("extabindex", 0x11))
        check("ordinal is gone", [n for n in after_symbols if ORDINAL_RE.match(n) and
                                 after_symbols[n][0] in EXTAB_PREFIX], [])
        check("unrelated global untouched", after_symbols["fn_A"], (".text", 0x12))
        check("unrelated local untouched", after_symbols["lbl_1"], (".text", 0x01))
        check(".sdata ordinal untouched", after_symbols["@191"], (".sdata", 0x01))
        check("symbol count unchanged", len(after_symbols), len(before_symbols))

        # Only the symbol tables may differ; every real section keeps its bytes (and the sections
        # after `.strtab` keep their `sh_offset`/alignment across the shift).
        after_sections = section_bytes(path)
        check("section count unchanged", sorted(after_sections), sorted(before_sections))
        for name, content in before_sections.items():
            if name in (".symtab", ".strtab"):
                continue
            check(f"{name} contents unchanged", after_sections[name], content)
        check("only .symtab/.strtab changed",
              sorted(n for n in before_sections if after_sections[n] != before_sections[n]),
              [".strtab", ".symtab"])
        for section in read_sections(open(path, "rb").read())[0]:
            if section["offset"]:
                check(f"{section['name']} keeps alignment", section["offset"] % section["align"], 0)

        # A second run sees map names, not ordinals: no work, and the file is not rewritten.
        first = open(path, "rb").read()
        check("idempotent", rename_object(path, starts, False, False), 0)
        check("idempotent: bytes unchanged", open(path, "rb").read(), first)

        # No claimed start for the section: the entry is left exactly as MWCC wrote it.
        path2 = os.path.join(tmp, "unit2.o")
        with open(path2, "wb") as handle:
            handle.write(fixture())
        check("no claim: no work", rename_object(path2, {"text": 0x80000000}, False, False), 0)
        check("no claim: bytes unchanged", open(path2, "rb").read(), fixture())

        # An object with no extab at all: the fast path answers before splits.txt is ever read.
        path3 = os.path.join(tmp, "plain.o")
        with open(path3, "wb") as handle:
            handle.write(build_elf(sections=[(".text", bytes(4), 4)],
                                   symbols=[("@191", 0x00, 0x01, ".text")]))
        with open(path3, "rb") as handle:
            sections3, _a, _b = read_sections(handle.read())
        check("no extab/extabindex: nothing to do", ordinal_entries(open(path3, "rb").read(), sections3),
              (None, None, []))

        # A dry run reports without writing.
        path4 = os.path.join(tmp, "unit4.o")
        with open(path4, "wb") as handle:
            handle.write(fixture())
        check("dry run reports the count", rename_object(path4, starts, False, True), 3)
        check("dry run: bytes unchanged", open(path4, "rb").read(), fixture())

    print(f"selftest: {'OK' if failures == 0 else str(failures) + ' FAILURE(S)'}")
    return 1 if failures else 0


def build_elf(sections: list[tuple[str, bytes, int]],
              symbols: list[tuple[str, int, int, str]]) -> bytes:
    """A minimal ELF32 big-endian relocatable object, for the selftest only.

    sections: [(name, data, align)]; symbols: [(name, st_value, st_info, section name)]; the null
    symbol is implicit at index 0.  `.symtab`/`.strtab`/`.shstrtab` are appended in that order after
    the caller's sections - the same order MWCC uses, so a section always follows `.strtab` here.
    """
    names = [""] + [name for name, _data, _align in sections] + [".symtab", ".strtab", ".shstrtab"]
    symtab_index = 1 + len(sections)
    strtab_index = symtab_index + 1
    shstr_index = strtab_index + 1

    strtab = bytearray(b"\0")
    name_offsets = {}
    for name, _value, _info, _section in symbols:
        name_offsets[name] = len(strtab)
        strtab += name.encode() + b"\0"
    index_of = {name: i + 1 for i, name in enumerate(n for n, _d, _a in sections)}
    symtab = bytearray(SYM_STRIDE)  # the null symbol
    locals_count = 1
    for name, value, st_info, section in symbols:
        symtab += struct.pack(">IIIBBH", name_offsets[name], value, 0, st_info, 2,
                              index_of.get(section, 0))
        if st_info >> 4 == 0:
            locals_count += 1

    shstr = bytearray(b"\0")
    for name in names[1:]:
        shstr += name.encode() + b"\0"

    blocks = list(sections)
    blocks += [(".symtab", bytes(symtab), 4), (".strtab", bytes(strtab), 1),
               (".shstrtab", bytes(shstr), 1)]
    offsets = []
    cursor = 52
    for _name, data, align in blocks:
        cursor = (cursor + align - 1) // align * align
        offsets.append(cursor)
        cursor += len(data)
    shoff = (cursor + 3) // 4 * 4
    out = bytearray(shoff + 40 * len(names))
    for (name, data, _align), offset in zip(blocks, offsets):
        out[offset : offset + len(data)] = data
    struct.pack_into(">4sBBBBB7s", out, 0, b"\x7fELF", 1, 2, 1, 0, 0, b"\0" * 7)
    struct.pack_into(">HHIIIIIHHHHHH", out, 0x10, 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                     len(names), shstr_index)
    # the section-name offsets inside `.shstrtab`
    str_offsets = {}
    pos = 1
    for name in names[1:]:
        str_offsets[name] = pos
        pos += len(name) + 1
    for i, (name, data, align) in enumerate(blocks):
        base = shoff + (i + 1) * 40
        typ = SHT_SYMTAB if name == ".symtab" else 3 if name in (".strtab", ".shstrtab") else 1
        link = strtab_index if name == ".symtab" else 0
        sinfo = locals_count if name == ".symtab" else 0
        struct.pack_into(">IIIIIIIIII", out, base, str_offsets[name], typ, 0, 0, offsets[i],
                         len(data), link, sinfo, align, SYM_STRIDE if name == ".symtab" else 0)
    return bytes(out)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", nargs="?", help="object file to rename (in place)")
    parser.add_argument("--splits", default=DEFAULT_SPLITS, help="splits.txt to read")
    parser.add_argument("--unit", help="splits key, when the path cannot be mapped")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--dry-run", action="store_true", help="report without writing")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.object:
        parser.error("an object file is required")

    # The fast path: an object that defines no ordinal-named extab/extabindex symbol cannot need
    # this tool, and most of the link's objects are exactly that.  Nothing is read from splits.txt
    # before this answers.
    with open(args.object, "rb") as handle:
        data = bytearray(handle.read())
    sections, _shoff, _shentsize = read_sections(data)
    _symtab, _strtab, entries = ordinal_entries(data, sections)
    if not entries:
        if args.verbose:
            print(f"objextab: {args.object}: no ordinal extab/extabindex symbol, left alone")
        return 0

    units = parse_splits(args.splits)
    key = args.unit or unit_key(args.object, units)
    if key is None:
        # Not a registered unit (or not reachable from splits) - nothing to name the entries after.
        if args.verbose:
            print(f"objextab: {args.object}: no splits entry, {len(entries)} symbol(s) left alone")
        return 0
    if key not in units:
        raise SystemExit(f"objextab: {key} is not in {args.splits}")

    rename_object(args.object, units[key], args.verbose, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
