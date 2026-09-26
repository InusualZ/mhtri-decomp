#!/usr/bin/env python3
"""Clamp a compiled object's section alignment to what its link address allows.

MWCC emits every section with ``sh_addralign = 8``.  That is fine for a section
whose claimed start is 8-aligned, but a unit whose claimed start is only
4-mod-8 cannot be linked at that address: mwld rounds the section up to the next
8-byte boundary and every later section shifts with it, which breaks the DOL
hash while the object itself stays byte-identical.

Retail really does have such units, and dtk's own ``dol split`` writes the value the
address can honour into the target objects it synthesises: the target's
``sh_addralign`` is ``lowbit(section start)``, bounded only by what MWCC emitted
(never more than 8 for a data section).  ``Pl/fn_8023C2D0.os`` ``.data`` is the worked
example: our object has size 0x84C alignment 8, the target has the same size with
alignment 4, and setting that one field to 4 makes the flip link to the original DOL
byte for byte.

This tool applies the same rule to the object MWCC produced, chained after
``dtk extab clean`` in the compile rule (``tools/project.py``).  It is a **strict
lowering** - a section whose start is 8-aligned keeps alignment 8 - so it is a
no-op for every unit that links today, and it cannot move the DOL on its own.

Evidence and the reproduction recipe: ``docs/matching.md``, "An odd-start
``.data`` claim cannot be linked with MWCC's alignment".

Usage:
    python tools/elf/objalign.py <object> [--splits config/RMHE08/splits.txt]
                                           [--unit <splits key>] [-v] [--dry-run]
    python tools/elf/objalign.py --selftest
"""

from __future__ import annotations

import argparse
import os
import re
import struct
import sys

DEFAULT_SPLITS = os.path.join("config", "RMHE08", "splits.txt")

# The section dtk owns itself (its `extab clean` step rewrites them); never touched.
SKIP_SECTIONS = {"extab", "extabindex"}

# A section never needs more than the lowbit of its address; this bounds the
# comparison so an absurd address cannot raise anything.  The emitted value is the
# real ceiling - we only ever lower it.
MAX_ALIGN = 1 << 30

_SECTION_RE = re.compile(
    r"^\s+(?P<name>[.\w$]+)\s+start:0x(?P<start>[0-9A-Fa-f]+)\s+end:0x(?P<end>[0-9A-Fa-f]+)\s*$"
)


def allowed_align(address: int) -> int:
    """The largest power of two that divides `address`.

    This is the alignment a section starting at `address` can honour, and it is
    exactly what dtk uses for the target objects.  It is deliberately uncapped: a
    `.text` at a 16-aligned address must keep MWCC's 16 (lowering it moved
    `Runtime.PPCEABI.H/__start.o` and broke the DOL).
    """
    if address <= 0:
        return MAX_ALIGN
    return min(1 << ((address & -address).bit_length() - 1), MAX_ALIGN)


def parse_splits(path: str) -> dict[str, dict[str, int]]:
    """{unit key: {section name (no dot): start address}} from splits.txt."""
    units: dict[str, dict[str, int]] = {}
    current: str | None = None
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            if line[:1] not in ("", " ", "\t") and line.rstrip().endswith(":"):
                current = line.strip()[:-1]
                units[current] = {}
            elif current:
                match = _SECTION_RE.match(line)
                if match:
                    units[current][match.group("name").lstrip(".")] = int(
                        match.group("start"), 16
                    )
    return units


def unit_key(obj_path: str, units: dict[str, dict[str, int]]) -> str | None:
    """The splits.txt key for an object path like `build/RMHE08/src/Pl/x.o`."""
    normalised = str(obj_path).replace("\\", "/")
    if "/src/" in normalised:
        stem = normalised.split("/src/", 1)[1]
    else:
        stem = normalised.rsplit("/", 1)[-1]
    if stem.endswith(".o"):
        stem = stem[:-2]
    for extension in (".cpp", ".cp", ".c", ".cc", ""):
        if stem + extension in units:
            return stem + extension
    return None


def read_sections(data: bytes) -> tuple[list[dict], int, int]:
    """Parse the ELF32 big-endian section headers into dicts."""
    if data[:4] != b"\x7fELF":
        raise SystemExit("objalign: not an ELF file")
    if data[4] != 1 or data[5] != 2:
        raise SystemExit("objalign: expected a 32-bit big-endian ELF")
    shoff = struct.unpack_from(">I", data, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", data, 0x2E)
    if shentsize < 40:
        raise SystemExit("objalign: unexpected section header size")
    shstr_off = struct.unpack_from(">I", data, shoff + shstrndx * shentsize + 0x10)[0]
    sections = []
    for index in range(shnum):
        base = shoff + index * shentsize
        name_off = struct.unpack_from(">I", data, base)[0]
        end = data.index(b"\0", shstr_off + name_off)
        sections.append(
            {
                "index": index,
                "name": data[shstr_off + name_off : end].decode(),
                "header_offset": base,
            }
        )
    return sections, shoff, shentsize


def align_offset(base: int) -> int:
    """Byte offset of `sh_addralign` within a 40-byte ELF32 section header."""
    return base + 32


def align_object(path: str, starts: dict[str, int], verbose: bool, dry_run: bool) -> int:
    """Lower every placed section's alignment to what its start address allows."""
    with open(path, "rb") as handle:
        data = bytearray(handle.read())
    sections, shentsize, _ = read_sections(data)
    changes = []
    for section in sections:
        name = section["name"].lstrip(".")
        if not name or name in SKIP_SECTIONS or name not in starts:
            continue
        offset = align_offset(section["header_offset"])
        current = struct.unpack_from(">I", data, offset)[0]
        wanted = allowed_align(starts[name])
        if verbose:
            print(f"  {section['name']:10s} start=0x{starts[name]:08X} "
                  f"align={current} allowed={wanted}")
        if current > wanted:
            if not dry_run:
                struct.pack_into(">I", data, offset, wanted)
            changes.append((section["name"], current, wanted))

    if changes and not dry_run:
        with open(path, "r+b") as handle:
            handle.write(data)
    for name, before, after in changes:
        verb = "would be" if dry_run else "->"
        print(f"objalign: {path}: {name} align {before} {verb} {after}")
    return len(changes)


def selftest() -> int:
    failures = 0

    def check(label: str, got, want) -> None:
        nonlocal failures
        if got != want:
            failures += 1
            print(f"  FAIL {label}: got {got!r}, want {want!r}")

    # The alignment rule, including the two real units this tool exists for.
    for address, want in [
        (0x805C34D4, 4),  # Pl/fn_8023C2D0  .data  (the worked example)
        (0x805C1F94, 4),  # Pl/fn_80230FBC  .data
        (0x805C3D20, 32), # Pl/fn_80241558  .data (lowbit 32; MWCC emits 8, so no change)
        (0x805C2C60, 32), # Pl/fn_802373AC  .data (same)
        (0x80570E98, 8),  # Camellia       .rodata
        (0x80000004, 4),
        (0x80000002, 2),
        (0x80000001, 1),
        (0x80000000, MAX_ALIGN),      # a pathological address: bounded, never raising
        (0x80003400, 1024),           # Runtime.PPCEABI.H/__start.o .text (must stay 16)
        (0x80003420, 32),             # ... so a more aligned .text keeps its own value
    ]:
        check(f"allowed_align(0x{address:08X})", allowed_align(address), want)

    # splits parsing and object-path -> unit key mapping
    units = {"Pl/fn_8023C2D0.cpp": {"text": 0x8023C2D0, "data": 0x805C34D4}}
    check("unit_key", unit_key("build/RMHE08/src/Pl/fn_8023C2D0.o", units),
          "Pl/fn_8023C2D0.cpp")
    check("unit_key (windows)", unit_key("build\\RMHE08\\src\\Pl\\fn_8023C2D0.o", units),
          "Pl/fn_8023C2D0.cpp")
    check("unit_key (unknown)", unit_key("build/RMHE08/src/Pl/nope.o", units), None)

    # A synthetic ELF: one section to clamp, one already fine, one to leave alone.
    def build_elf() -> bytearray:
        names = b"\0.text\0.data\0.sdata\0.shstrtab\0"
        offsets = {"": 0, ".text": 1, ".data": 7, ".sdata": 13, ".shstrtab": 20}
        data = bytearray(0x100)
        shstr_va = 0xE0  # after the 4 section headers (0x40..0xE0)
        data[shstr_va : shstr_va + len(names)] = names
        headers = [(0, 3, 0x805C34D4, 8), (0, 1, 0x805C3D20, 8), (0, 1, 0x805C34D0, 8)]
        for slot, (_, _, _, _) in enumerate(headers):
            base = 0x40 + slot * 40
            name = [".text", ".data", ".sdata"][slot]
            struct.pack_into(">IIIIIIIIII", data, base, offsets[name], 1, 0, 0, 0, 4, 0, 0, headers[slot][3], 0)
        # shstrtab entry
        base = 0x40 + 3 * 40
        struct.pack_into(">IIIIIIIIII", data, base, offsets[".shstrtab"], 3, 0, 0, shstr_va, len(names), 0, 0, 1, 0)
        data[0:4] = b"\x7fELF"
        data[4], data[5] = 1, 2
        struct.pack_into(">I", data, 0x20, 0x40)  # e_shoff
        struct.pack_into(">HHH", data, 0x2E, 40, 4, 3)  # e_shentsize, e_shnum, e_shstrndx
        return data

    import tempfile

    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "synthetic.o")
        with open(path, "wb") as handle:
            handle.write(build_elf())
        # .data starts at 0x805C34D4 -> 4; .sdata at 0x805C34D0 -> stays 8 (no-op).
        changed = align_object(path, {"data": 0x805C34D4, "sdata": 0x805C34D0}, False, False)
        check("synthetic: sections changed", changed, 1)
        after = open(path, "rb").read()
        check("synthetic: .data clamped", struct.unpack_from(">I", after, 0x40 + 40 + 32)[0], 4)
        check("synthetic: .sdata untouched", struct.unpack_from(">I", after, 0x40 + 80 + 32)[0], 8)
        check("synthetic: .text untouched (not in splits)",
              struct.unpack_from(">I", after, 0x40 + 32)[0], 8)
        # Idempotent.
        check("synthetic: idempotent", align_object(path, {"data": 0x805C34D4, "sdata": 0x805C34D0}, False, False), 0)

    print(f"selftest: {'OK' if failures == 0 else str(failures) + ' FAILURE(S)'}")
    return 1 if failures else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", nargs="?", help="object file to normalise (in place)")
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

    units = parse_splits(args.splits)
    key = args.unit or unit_key(args.object, units)
    if key is None:
        # Not a registered unit (or not reachable from splits) - nothing to do.
        if args.verbose:
            print(f"objalign: {args.object}: no splits entry, left alone")
        return 0
    if key not in units:
        raise SystemExit(f"objalign: {key} is not in {args.splits}")

    align_object(args.object, units[key], args.verbose, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
