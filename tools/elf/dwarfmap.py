#!/usr/bin/env python3
"""Dump the DWARF2 local-variable -> stack-slot map MWCC produced for one function.

Usage: python tools/elf/dwarfmap.py <object.o> <function> [--loc]

The object must be compiled with -gdwarf-2 (codegen is unchanged by -g, verified).
Relocations in .rela.debug_info are applied manually (RELA, symbol value 0 + addend).
"""
import struct
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

DW_TAG_array_type = 0x01
DW_TAG_subprogram = 0x2E
DW_TAG_variable = 0x34
DW_TAG_formal_parameter = 0x05

OPS = {0x03: "addr", 0x06: "deref", 0x08: "const1u", 0x09: "const1s", 0x0a: "const2u",
       0x0b: "const2s", 0x0c: "const4u", 0x0d: "const4s", 0x10: "constu", 0x11: "consts",
       0x12: "dup", 0x19: "drop", 0x1c: "over", 0x22: "plus", 0x23: "plus_uconst",
       0x30: "lit0", 0x8f: "breg31", 0x91: "fbreg", 0x93: "basereg", 0x94: "regx",
       0x96: "reg", 0x97: "fbregx"}


def uleb(b, i):
    r = s = 0
    while True:
        c = b[i]
        i += 1
        r |= (c & 0x7F) << s
        if not c & 0x80:
            return r, i
        s += 7


def sleb(b, i):
    r = s = 0
    while True:
        c = b[i]
        i += 1
        r |= (c & 0x7F) << s
        s += 7
        if not c & 0x80:
            if c & 0x40:
                r -= 1 << s
            return r, i


def main():
    path, func = sys.argv[1], sys.argv[2]
    f = open(path, "rb")
    elf = ELFFile(f)
    info = bytearray(elf.get_section_by_name(".debug_info").data())
    loc = elf.get_section_by_name(".debug_loc").data()
    abbrev = elf.get_section_by_name(".debug_abbrev").data()

    # apply .rela.debug_info
    symtab = elf.get_section_by_name(".symtab")
    for sec in elf.iter_sections():
        if not isinstance(sec, RelocationSection) or sec.name != ".rela.debug_info":
            continue
        target = elf.get_section(sec["sh_info"])
        tname = target.name
        for r in sec.iter_relocations():
            sym = symtab.get_symbol(r["r_info_sym"])
            val = sym["st_value"] + r["r_addend"]
            struct.pack_into(">I", info, r["r_offset"], val)

    # CU header
    (unit_len,) = struct.unpack_from(">I", info, 0)
    version, abbrev_off, addr_size = struct.unpack_from(">HIB", info, 4)
    print("CU len=%d version=%d abbrev_off=0x%x addr_size=%d" % (unit_len, version, abbrev_off, addr_size))

    # abbrev table
    abbr = {}
    i = abbrev_off
    while True:
        code, i = uleb(abbrev, i)
        if code == 0:
            break
        tag, i = uleb(abbrev, i)
        children = abbrev[i]
        i += 1
        attrs = []
        while True:
            a, i = uleb(abbrev, i)
            fm, i = uleb(abbrev, i)
            if a == 0 and fm == 0:
                break
            attrs.append((a, fm))
        abbr[code] = (tag, children, attrs)

    def read_form(fm, i):
        """DWARF2 forms as MWCC emits them (32-bit DWARF)."""
        if fm == 0x01:                                   # addr
            return struct.unpack_from(">I", info, i)[0], i + 4
        if fm in (0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0b, 0x0c, 0x0d, 0x0e,
                  0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16):
            if fm == 0x03:                               # block2
                n = struct.unpack_from(">H", info, i)[0]
                return bytes(info[i + 2:i + 2 + n]), i + 2 + n
            if fm == 0x04:                               # block4
                n = struct.unpack_from(">I", info, i)[0]
                return bytes(info[i + 4:i + 4 + n]), i + 4 + n
            if fm == 0x05:                               # data2
                return struct.unpack_from(">H", info, i)[0], i + 2
            if fm == 0x06:                               # data4
                return struct.unpack_from(">I", info, i)[0], i + 4
            if fm == 0x07:                               # data8
                return struct.unpack_from(">Q", info, i)[0], i + 8
            if fm == 0x08:                               # string
                j = info.index(bytes([0]), i)
                return info[i:j].decode("latin-1"), j + 1
            if fm == 0x0b or fm == 0x0c:                 # data1 / flag
                return info[i], i + 1
            if fm == 0x0d:                               # sdata
                return sleb(info, i)
            if fm == 0x0e:                               # strp
                return struct.unpack_from(">I", info, i)[0], i + 4
            if fm == 0x0f or fm == 0x15 or fm == 0x16:   # udata / ref_udata / indirect
                return uleb(info, i)
            if fm == 0x10:                               # ref_addr
                return struct.unpack_from(">I", info, i)[0], i + 4
            if fm == 0x11:                               # ref1
                return info[i], i + 1
            if fm == 0x12:                               # ref2
                return struct.unpack_from(">H", info, i)[0], i + 2
            if fm == 0x13:                               # ref4
                return struct.unpack_from(">I", info, i)[0], i + 4
            if fm == 0x14:                               # ref8
                return struct.unpack_from(">Q", info, i)[0], i + 8
        if fm == 0x09:                                   # block
            n, j = uleb(info, i)
            return bytes(info[j:j + n]), j + n
        if fm == 0x0a:                                   # block1
            n = info[i]
            return bytes(info[i + 1:i + 1 + n]), i + 1 + n
        raise SystemExit("unhandled form 0x%x at 0x%x" % (fm, i))

    by_func = {}

    def walk(i, depth):
        found = []
        while i < len(info):
            code, i = uleb(info, i)
            if code == 0:
                return found, i
            if "--trace" in sys.argv:
                print("  i=0x%x code=%d depth=%d" % (i - 1, code, depth))
            tag, children, attrs = abbr[code]
            vals = {}
            for a, fm in attrs:
                v, i = read_form(fm, i)
                vals[a] = v
            name = vals.get(0x03, "")           # DW_AT_name
            if "--all" in sys.argv:
                print("  %s%s tag=0x%x name=%r" % ("  " * depth, "  ", tag, name))
            if tag == DW_TAG_subprogram:
                kids, i = walk(i, depth + 1)
                by_func[name] = kids
                continue
            if children:
                kids, i = walk(i, depth + 1)
            else:
                kids = []
            if tag in (DW_TAG_variable, DW_TAG_formal_parameter):
                found.append((name, vals, kids))
            elif tag == DW_TAG_array_type:
                found.append(("<array>", vals, kids))
        return found, i

    walk(11, 0)
    dies = by_func.get(func, [])

    def loclist(off):
        out = []
        i = off
        while True:
            b, e = struct.unpack_from(">II", loc, i)
            i += 8
            if b == 0 and e == 0:
                break
            n = struct.unpack_from(">H", loc, i)[0]
            i += 2
            expr = bytes(loc[i:i + n])
            i += n
            out.append((b, e, expr))
        return out

    def decode(expr):
        i = 0
        parts = []
        while i < len(expr):
            op = expr[i]
            i += 1
            if op == 0x91:                     # fbreg
                v, i = sleb(expr, i)
                parts.append("fbreg(%+d)" % v)
            elif op == 0x93:                   # basereg <reg> <sleb>
                reg, i = uleb(expr, i)
                v, i = sleb(expr, i)
                parts.append("basereg(r%d,%+d)" % (reg, v))
            elif 0x70 <= op <= 0x8f:           # breg0..31
                v, i = sleb(expr, i)
                parts.append("breg%d(%+d)" % (op - 0x70, v))
            elif op == 0x23:
                v, i = uleb(expr, i)
                parts.append("+%d" % v)
            else:
                parts.append(OPS.get(op, "op%02x" % op))
        return " ".join(parts)

    print("\n== %s (depth-first variable list)" % func)
    for name, vals, kids in dies:
        lv = vals.get(0x02)                    # DW_AT_location
        if "--trace" in sys.argv:
            print("   DBG %-8s keys=%s" % (name, sorted(hex(k) for k in vals)))
        if lv is None:
            continue
        if isinstance(lv, bytes):
            txt = decode(lv)
        else:
            txt = "loclist@0x%x" % lv
            if "--loc" in sys.argv:
                txt += " " + "; ".join("[%d-%d] %s" % (b, e, decode(x)) for b, e, x in loclist(lv))
        print("  %-14s %s" % (name, txt))


main()
