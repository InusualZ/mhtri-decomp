#!/usr/bin/env python3
"""MWCC PE symbol/disassembly helper.

Two jobs, both driven by the CodeView 'NB11' symbol blob that Metrowerks
embeds in mwcceppc.exe:

  syms  <exe> [regex]        list symbols (RVA, VA, name, section)
  dis   <exe> <start> <end>  disassemble an address range, annotating call
                             targets with their symbol names

Addresses on the command line may be hex RVAs (default) or VAs if prefixed
with 'va:'.  The image base is read from the PE header.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import re

import capstone

from tools.lib.binary.pe import Pe




def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p1 = sub.add_parser("syms")
    p1.add_argument("exe")
    p1.add_argument("pattern", nargs="?", default=".")
    p2 = sub.add_parser("dis")
    p2.add_argument("exe")
    p2.add_argument("start")
    p2.add_argument("end")
    args = ap.parse_args()

    pe = Pe(args.exe)

    def addr(s: str) -> int:
        if s.startswith("va:"):
            return int(s[3:], 16) - pe.image_base
        return int(s, 16)

    if args.cmd == "syms":
        rx = re.compile(args.pattern, re.I)
        for rva, name, sec in pe.codeview_symbols():
            if rx.search(name):
                print(f"{rva:08x}\t{pe.image_base+rva:08x}\t{name}\t{sec}")
        return

    start, end = addr(args.start), addr(args.end)
    symmap = pe.symbol_map()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    code = pe.read_rva(start, end - start)
    for ins in md.disasm(code, pe.image_base + start):
        rva = ins.address - pe.image_base
        ann = ""
        m = re.match(r"^(call|jmp|j\w+)$", ins.mnemonic)
        if m and ins.operands and ins.operands[0].type == capstone.x86.X86_OP_IMM:
            tgt = ins.operands[0].imm - pe.image_base
            if tgt in symmap:
                ann = f"  ; {symmap[tgt]}"
        print(f"{rva:08x}  {ins.mnemonic:<7} {ins.op_str}{ann}")


if __name__ == "__main__":
    main()
