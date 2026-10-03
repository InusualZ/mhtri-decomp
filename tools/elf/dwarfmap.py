#!/usr/bin/env python3
"""Dump the DWARF2 local-variable -> stack-slot map MWCC produced for one function (-gdwarf-2 objects).
Spec: docs/tools/spec/dwarfmap.md. CLI: dwarfmap.py <object.o> <function> [--loc] [--all] [--trace]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import struct

from tools.lib.binary import dwarf
from tools.lib.binary.elf import Elf


def main():
    path, func = sys.argv[1], sys.argv[2]
    elf = Elf.read(path)
    trace, everything = "--trace" in sys.argv, "--all" in sys.argv

    def on_die(offset, code, depth, tag, name):
        if trace:
            print("  i=0x%x code=%d depth=%d" % (offset, code, depth))
        if everything:
            print("  %s%s tag=0x%x name=%r" % ("  " * depth, "  ", tag, name))

    info = dwarf.debug_info(elf)
    unit_len, = struct.unpack_from(">I", info, 0)
    version, abbrev_off, addr_size = struct.unpack_from(">HIB", info, 4)
    print("CU len=%d version=%d abbrev_off=0x%x addr_size=%d" % (unit_len, version, abbrev_off, addr_size))
    cu = dwarf.compile_unit(elf, on_die if (trace or everything) else None)
    loc_sec = elf.section(".debug_loc")
    loc = loc_sec.raw if loc_sec is not None else b""

    print("\n== %s (depth-first variable list)" % func)
    for die in cu.functions.get(func, []):
        lv = die.attrs.get(dwarf.DW_AT_location)
        if trace:
            print("   DBG %-8s keys=%s" % (die.name, sorted(hex(k) for k in die.attrs)))
        if lv is None:
            continue
        if isinstance(lv, bytes):
            txt = dwarf.decode_expr(lv)
        else:
            txt = "loclist@0x%x" % lv
            if "--loc" in sys.argv:
                txt += " " + "; ".join("[%d-%d] %s" % (b, e, dwarf.decode_expr(x))
                                       for b, e, x in dwarf.loclist(loc, lv))
        print("  %-14s %s" % (die.name, txt))


if __name__ == "__main__":
    main()
