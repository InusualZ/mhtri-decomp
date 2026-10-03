#!/usr/bin/env python3
"""Post-compile ninja step: rename MWCC's ordinal extab/extabindex symbols to the map's @etb_/@eti_ names, global.
Spec: docs/tools/spec/objextab.md. CLI: objextab.py <object> [--splits PATH] [--unit KEY] [-v] [--dry-run] | --selftest."""

from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import re

from tools.elf.objalign import parse_splits, unit_key  # one splits grammar and one object -> unit rule
from tools.lib.binary.elf import Elf, ElfEditor, ElfError

DEFAULT_SPLITS = os.path.join("config", "RMHE08", "splits.txt")

# The two sections `dtk dol split` names after the map, and the prefix it spells them with.
EXTAB_PREFIX = {"extab": "@etb_", "extabindex": "@eti_"}

# MWCC's anonymous name for a compiler-generated symbol.  Only these are candidates; the map's own
# spelling (which means the entry is already done) and ordinary `lbl_*` names are left alone.
ORDINAL_RE = re.compile(r"^@\d+$")

GLOBAL_BINDING = 0x10
BINDING_MASK = 0x0F


def map_name(section: str, start: int, value: int) -> str:
    """The name `dol split` gives the entry at `start + value` of `section`, case included."""
    return "%s%08X" % (EXTAB_PREFIX[section], start + value)


def ordinal_entries(elf: Elf) -> list[dict]:
    """The ordinal-named extab/extabindex symbols of this object (`index`, `name`, `section`, `value`, `info`).

    Empty for an object with nothing to do, and that is decided from the object alone - no `splits.txt`
    read.  That matters: most of the link's objects are C and carry no extab at all.
    """
    wanted = {s.index: s.name for s in elf.sections if s.name.lstrip(".") in EXTAB_PREFIX}
    if not wanted:
        return []
    symtab = elf.symtab
    if symtab is None or symtab.link >= len(elf.sections):
        return []
    entries = []
    for sym in elf.symbols:
        if sym.shndx not in wanted or not ORDINAL_RE.match(sym.name):
            continue
        entries.append({"index": sym.index, "name": sym.name, "section": wanted[sym.shndx],
                        "value": sym.value, "info": sym.info})
    return entries


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


def write_object(path: str, elf: Elf, changes: list[tuple[dict, str]]) -> None:
    """Apply the renames in place: the new names are appended to `.strtab` (every existing `st_name` stays
    valid), each renamed symbol is bound global, and the later sections shift (`lib.binary.elf.ElfEditor`)."""
    editor = ElfEditor(elf)
    for entry, _name in changes:
        editor.set_symbol_info(entry["index"], GLOBAL_BINDING | (entry["info"] & BINDING_MASK))
    editor.rename_symbols([(entry["index"], name) for entry, name in changes])
    editor.write(path)


def read_object(path: str) -> Elf:
    """The object at `path`, or SystemExit with objextab's wording when it is not an ELF32 big-endian file."""
    try:
        return Elf.read(path).require_be32()
    except ElfError as exc:
        raise SystemExit(f"objextab: {exc}")


def rename_object(path: str, starts: dict[str, int], verbose: bool, dry_run: bool) -> int:
    """Rename the object's ordinal extab/extabindex symbols to the map's names; return the count."""
    elf = read_object(path)
    entries = ordinal_entries(elf)
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
    write_object(path, elf, candidates)
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
        elf = Elf.read(path)
        return {s.name: (elf.section_name(s.shndx), s.info) for s in elf.symbols}

    def section_bytes(path: str) -> dict[str, bytes]:
        return {s.name: s.raw for s in Elf.read(path).sections}

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
        for section in Elf.read(path).sections:
            if section.offset:
                check(f"{section.name} keeps alignment", section.offset % section.align, 0)

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
        check("no extab/extabindex: nothing to do", ordinal_entries(Elf.read(path3)), [])

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
    """A minimal MWCC-shaped object for the selftest: sections [(name, data, align)], symbols
    [(name, st_value, st_info, section)] in the given order; `.symtab`/`.strtab`/`.shstrtab` follow."""
    from tools.lib.binary.build import ElfBuilder  # selftest only: the ninja step never builds an object

    builder = ElfBuilder(keep_order=True)
    for name, data, align in sections:
        builder.section(name, data, align=align)
    for name, value, info, section in symbols:
        builder.symbol(name, section, value, bind=info >> 4, type=info & 0xF)
    return builder.build()


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
    entries = ordinal_entries(read_object(args.object))
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
