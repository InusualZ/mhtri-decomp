"""DWARF2 as MWCC emits it (-gdwarf-2): .debug_info with its relocations applied, abbrevs, DIEs, locations.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library; tools/elf/dwarfmap.py is the CLI)."""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import Callable

from tools.lib.binary.elf import Elf

DW_TAG_array_type = 0x01
DW_TAG_formal_parameter = 0x05
DW_TAG_subprogram = 0x2E
DW_TAG_variable = 0x34
DW_AT_location = 0x02
DW_AT_name = 0x03

#: Location-expression opcodes, by number, for `decode_expr`.
OPS = {0x03: "addr", 0x06: "deref", 0x08: "const1u", 0x09: "const1s", 0x0a: "const2u",
       0x0b: "const2s", 0x0c: "const4u", 0x0d: "const4s", 0x10: "constu", 0x11: "consts",
       0x12: "dup", 0x19: "drop", 0x1c: "over", 0x22: "plus", 0x23: "plus_uconst",
       0x30: "lit0", 0x8f: "breg31", 0x91: "fbreg", 0x93: "basereg", 0x94: "regx",
       0x96: "reg", 0x97: "fbregx"}


class DwarfError(ValueError):
    """A section is missing or a form is not one MWCC's DWARF2 uses."""


def uleb(b: bytes, i: int) -> tuple[int, int]:
    """(value, next index) of the unsigned LEB128 at `b[i]`."""
    r = s = 0
    while True:
        c = b[i]
        i += 1
        r |= (c & 0x7F) << s
        if not c & 0x80:
            return r, i
        s += 7


def sleb(b: bytes, i: int) -> tuple[int, int]:
    """(value, next index) of the signed LEB128 at `b[i]`."""
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


@dataclass(frozen=True)
class Die:
    """A variable/parameter/array DIE: its name, attribute values by `DW_AT_*` number, and its children."""
    name: str
    tag: int
    attrs: dict = field(hash=False, compare=False)
    children: tuple = ()


@dataclass(frozen=True)
class CompileUnit:
    length: int
    version: int
    abbrev_offset: int
    address_size: int
    functions: dict = field(hash=False, compare=False)  # subprogram name -> [Die] (depth-first variables)


def debug_info(elf: Elf) -> bytearray:
    """`.debug_info` with `.rela.debug_info` applied (RELA: the symbol's value plus the addend, stored as u32)."""
    sec = elf.section(".debug_info")
    if sec is None:
        raise DwarfError("no .debug_info (compile with -gdwarf-2)")
    info = bytearray(sec.raw)
    syms = elf.symbols
    for r in elf.relocs(".debug_info"):
        value = (syms[r.symbol].value if r.symbol < len(syms) else 0) + r.addend
        struct.pack_into(">I", info, r.offset, value & 0xFFFFFFFF)
    return info


def abbrevs(abbrev: bytes, offset: int) -> dict[int, tuple[int, int, list[tuple[int, int]]]]:
    """`{code: (tag, has_children, [(attribute, form)])}` of the abbreviation table at `offset`."""
    out = {}
    i = offset
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
        out[code] = (tag, children, attrs)
    return out


def read_form(info: bytes, fm: int, i: int):
    """(value, next index) of one attribute value in form `fm` (DWARF2, 32-bit, as MWCC emits it)."""
    if fm == 0x01:                                   # addr
        return struct.unpack_from(">I", info, i)[0], i + 4
    if fm == 0x03:                                   # block2
        n = struct.unpack_from(">H", info, i)[0]
        return bytes(info[i + 2:i + 2 + n]), i + 2 + n
    if fm == 0x04:                                   # block4
        n = struct.unpack_from(">I", info, i)[0]
        return bytes(info[i + 4:i + 4 + n]), i + 4 + n
    if fm == 0x05:                                   # data2
        return struct.unpack_from(">H", info, i)[0], i + 2
    if fm == 0x06:                                   # data4
        return struct.unpack_from(">I", info, i)[0], i + 4
    if fm == 0x07:                                   # data8
        return struct.unpack_from(">Q", info, i)[0], i + 8
    if fm == 0x08:                                   # string
        j = info.index(bytes([0]), i)
        return bytes(info[i:j]).decode("latin-1"), j + 1
    if fm in (0x0b, 0x0c):                           # data1 / flag
        return info[i], i + 1
    if fm == 0x0d:                                   # sdata
        return sleb(info, i)
    if fm == 0x0e:                                   # strp
        return struct.unpack_from(">I", info, i)[0], i + 4
    if fm in (0x0f, 0x15, 0x16):                     # udata / ref_udata / indirect
        return uleb(info, i)
    if fm == 0x10:                                   # ref_addr
        return struct.unpack_from(">I", info, i)[0], i + 4
    if fm == 0x11:                                   # ref1
        return info[i], i + 1
    if fm == 0x12:                                   # ref2
        return struct.unpack_from(">H", info, i)[0], i + 2
    if fm == 0x13:                                   # ref4
        return struct.unpack_from(">I", info, i)[0], i + 4
    if fm == 0x14:                                   # ref8
        return struct.unpack_from(">Q", info, i)[0], i + 8
    if fm == 0x09:                                   # block
        n, j = uleb(info, i)
        return bytes(info[j:j + n]), j + n
    if fm == 0x0a:                                   # block1
        n = info[i]
        return bytes(info[i + 1:i + 1 + n]), i + 1 + n
    raise DwarfError("unhandled form 0x%x at 0x%x" % (fm, i))


def compile_unit(elf: Elf, on_die: Callable[[int, int, int, int, str], None] | None = None) -> CompileUnit:
    """The first compile unit: its header and, per subprogram, the depth-first variable/parameter/array DIEs.

    `on_die(offset, code, depth, tag, name)` is called for every DIE in walk order (dwarfmap's --trace/--all).
    """
    info = debug_info(elf)
    abbrev_sec = elf.section(".debug_abbrev")
    if abbrev_sec is None:
        raise DwarfError("no .debug_abbrev")
    (unit_len,) = struct.unpack_from(">I", info, 0)
    version, abbrev_off, addr_size = struct.unpack_from(">HIB", info, 4)
    table = abbrevs(abbrev_sec.raw, abbrev_off)
    by_func: dict[str, list[Die]] = {}

    def walk(i: int, depth: int) -> tuple[list[Die], int]:
        found: list[Die] = []
        while i < len(info):
            code, i = uleb(info, i)
            if code == 0:
                return found, i
            start = i - 1
            tag, children, attrs = table[code]
            vals = {}
            for a, fm in attrs:
                v, i = read_form(info, fm, i)
                vals[a] = v
            name = vals.get(DW_AT_name, "")
            if on_die is not None:
                on_die(start, code, depth, tag, name)
            if tag == DW_TAG_subprogram:
                kids, i = walk(i, depth + 1)
                by_func[name] = kids
                continue
            if children:
                kids, i = walk(i, depth + 1)
            else:
                kids = []
            if tag in (DW_TAG_variable, DW_TAG_formal_parameter):
                found.append(Die(name, tag, vals, tuple(kids)))
            elif tag == DW_TAG_array_type:
                found.append(Die("<array>", tag, vals, tuple(kids)))
        return found, i

    walk(11, 0)
    return CompileUnit(unit_len, version, abbrev_off, addr_size, by_func)


def loclist(loc: bytes, off: int) -> list[tuple[int, int, bytes]]:
    """`[(begin, end, expression)]` of the `.debug_loc` list at `off`."""
    out = []
    i = off
    while True:
        b, e = struct.unpack_from(">II", loc, i)
        i += 8
        if b == 0 and e == 0:
            break
        n = struct.unpack_from(">H", loc, i)[0]
        i += 2
        out.append((b, e, bytes(loc[i:i + n])))
        i += n
    return out


def decode_expr(expr: bytes) -> str:
    """A location expression as text: `fbreg(+8)`, `basereg(r1,+8)`, `breg31(-4)`, `+4`, opcode names."""
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


@dataclass(frozen=True)
class LocalSlot:
    """One local's location: `location` is the decoded expression, or `loclist@0x..` with `ranges` filled."""
    name: str
    location: str
    ranges: tuple[tuple[int, int, str], ...] = ()


def local_slots(obj: Elf | str, function: str) -> list[LocalSlot]:
    """The variable -> stack-slot map of `function` (the dwarfmap core): one row per DIE that has a location."""
    elf = obj if isinstance(obj, Elf) else Elf.read(obj)
    cu = compile_unit(elf)
    loc_sec = elf.section(".debug_loc")
    loc = loc_sec.raw if loc_sec is not None else b""
    out = []
    for die in cu.functions.get(function, []):
        lv = die.attrs.get(DW_AT_location)
        if lv is None:
            continue
        if isinstance(lv, bytes):
            out.append(LocalSlot(die.name, decode_expr(lv)))
        else:
            ranges = tuple((b, e, decode_expr(x)) for b, e, x in loclist(loc, lv))
            out.append(LocalSlot(die.name, "loclist@0x%x" % lv, ranges))
    return out
