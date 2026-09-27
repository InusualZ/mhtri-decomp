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

import argparse
import re
import struct
import sys
from dataclasses import dataclass

import capstone


@dataclass
class Section:
    name: str
    va: int
    vsize: int
    raw: int
    rawsize: int
    index: int  # 1-based, as used by the CodeView records


class Pe:
    def __init__(self, path: str):
        self.path = path
        self.data = open(path, "rb").read()
        d = self.data
        e = struct.unpack_from("<I", d, 0x3C)[0]
        self.pe_off = e
        self.machine = struct.unpack_from("<H", d, e + 4)[0]
        nsec = struct.unpack_from("<H", d, e + 6)[0]
        optsz = struct.unpack_from("<H", d, e + 20)[0]
        opt = e + 24
        optmagic = struct.unpack_from("<H", d, opt)[0]
        assert optmagic == 0x10B, f"not a PE32 image (magic {optmagic:#x})"
        self.image_base = struct.unpack_from("<I", d, opt + 28)[0]
        self.optsz = optsz
        self.opt_off = opt
        self.sections: list[Section] = []
        so = opt + optsz
        for i in range(nsec):
            s = so + 40 * i
            name = d[s : s + 8].rstrip(b"\0").decode("latin-1")
            vs, va, rs, rp = struct.unpack_from("<IIII", d, s + 8)
            self.sections.append(Section(name, va, vs, rp, rs, i + 1))
        self.syms: list[tuple[int, str, str]] = []

    def rva2off(self, rva: int) -> int | None:
        for s in self.sections:
            if s.va <= rva < s.va + max(s.vsize, s.rawsize):
                if rva - s.va >= s.rawsize:
                    return None
                return s.raw + (rva - s.va)
        return None

    def read_rva(self, rva: int, n: int) -> bytes:
        o = self.rva2off(rva)
        if o is None:
            raise ValueError(f"RVA {rva:#x} not backed by file data")
        return self.data[o : o + n]

    # ---- CodeView symbol blob ------------------------------------------------
    def load_symbols(self):
        if self.syms:
            return self.syms
        d = self.data
        dd = self.opt_off + 96 + 8 * 6
        rva, size = struct.unpack_from("<II", d, dd)
        o = self.rva2off(rva)
        ch, ts, maj, mnr, typ, dsize, daddr, ptr = struct.unpack_from("<IIHHIIII", d, o)
        blob = d[ptr : ptr + dsize]
        self.debug_type = typ
        self.debug_sig = blob[:4]
        secs = self.sections
        i = 0
        n = len(blob)
        recs = []
        while i < n - 12:
            ln = struct.unpack_from("<H", blob, i)[0]
            if 8 <= ln <= 0x400 and i + 2 + ln <= n:
                t = struct.unpack_from("<H", blob, i + 2)[0]
                z = struct.unpack_from("<I", blob, i + 4)[0]
                if z == 0 and 0x1000 <= t <= 0x10FF:
                    off = struct.unpack_from("<I", blob, i + 8)[0]
                    sec = struct.unpack_from("<H", blob, i + 12)[0]
                    nlen = blob[i + 14]
                    name = blob[i + 15 : i + 15 + nlen]
                    if (
                        1 <= sec <= len(secs)
                        and 3 <= nlen <= 200
                        and all(32 <= c < 127 for c in name)
                        and 15 + nlen <= ln + 2
                    ):
                        recs.append((secs[sec - 1].va + off, name.decode(), secs[sec - 1].name))
                        i += 2 + ln
                        continue
            i += 1
        recs.sort()
        self.syms = recs
        return recs

    def symmap(self) -> dict[int, str]:
        return {rva: name for rva, name, _ in self.load_symbols()}

    def next_sym_after(self, rva: int) -> tuple[int, str] | None:
        best = None
        for r, name, _ in self.load_symbols():
            if r > rva and (best is None or r < best[0]):
                best = (r, name)
        return best


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
        for rva, name, sec in pe.load_symbols():
            if rx.search(name):
                print(f"{rva:08x}\t{pe.image_base+rva:08x}\t{name}\t{sec}")
        return

    start, end = addr(args.start), addr(args.end)
    symmap = pe.symmap()
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
