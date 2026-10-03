"""Fixture builders: ElfBuilder (an ELF32 big-endian relocatable object) and DolBuilder (a DOL image).
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

import os
import struct
from dataclasses import dataclass, field

from tools.lib.binary.elf import (COMMENT_HEADER, COMMENT_MAGIC, RELOC_NUMBERS, SHF_ALLOC, SHF_EXECINSTR,
                                  SHF_WRITE, SHT_NOBITS, SHT_PROGBITS, SHT_RELA, SHT_STRTAB, SHT_SYMTAB,
                                  STB_GLOBAL, STB_LOCAL, STB_WEAK, STT_FILE, STT_FUNC, STT_NOTYPE, STT_OBJECT,
                                  STT_SECTION, SHN_ABS)

BINDS = {"local": STB_LOCAL, "global": STB_GLOBAL, "weak": STB_WEAK}
TYPES = {"notype": STT_NOTYPE, "object": STT_OBJECT, "func": STT_FUNC, "function": STT_FUNC,
         "section": STT_SECTION, "file": STT_FILE}
#: Default flags by section name, so a fixture `.text` is executable and a `.bss` writable like MWCC's.
_FLAGS = {".text": SHF_ALLOC | SHF_EXECINSTR, ".init": SHF_ALLOC | SHF_EXECINSTR}


@dataclass
class _Sec:
    name: str
    data: bytes
    align: int
    type: int
    flags: int
    size: int
    addr: int


@dataclass
class _Sym:
    name: str
    section: str | None
    value: int
    size: int
    bind: int
    type: int
    shndx: int | None
    comment: tuple[int, int, int] | None = None


@dataclass
class _Rel:
    section: str
    offset: int
    symbol: str | int
    type: int
    addend: int


@dataclass
class ElfBuilder:
    """Build an MWCC-shaped ELF32 big-endian relocatable object for a fixture.

    Layout: ELF header, the caller's sections (in call order, each at its alignment), then `.rela<sec>` for
    every section with relocations, `.symtab` (null symbol, locals, then globals; `sh_info` = first global),
    `.strtab`, `.shstrtab`, and the section-header table. `comment(version)` adds a CodeWarrior `.comment`
    with one entry per symbol. Relocations name their symbol by name (or by final index).
    """
    machine: int = 20  # EM_PPC
    keep_order: bool = False  # True: symbols stay in call order (no locals-first sort)
    _sections: list[_Sec] = field(default_factory=list)
    _symbols: list[_Sym] = field(default_factory=list)
    _relocs: list[_Rel] = field(default_factory=list)
    _comment: int | None = None

    # content
    def section(self, name: str, data: bytes = b"", align: int = 4, type: int = SHT_PROGBITS,
                flags: int | None = None, size: int | None = None, addr: int = 0) -> "ElfBuilder":
        if flags is None:
            flags = _FLAGS.get(name, SHF_ALLOC | SHF_WRITE if type == SHT_NOBITS else SHF_ALLOC)
        self._sections.append(_Sec(name, bytes(data), align, type, flags,
                                   len(data) if size is None else size, addr))
        return self

    def nobits(self, name: str, size: int, align: int = 4) -> "ElfBuilder":
        return self.section(name, b"", align, SHT_NOBITS, size=size)

    def symbol(self, name: str, section: str | None = None, value: int = 0, size: int = 0,
               bind: str | int = "global", type: str | int = "notype", shndx: int | None = None,
               comment: tuple[int, int, int] | None = None) -> "ElfBuilder":
        """`section=None` is undefined (or pass `shndx`, e.g. SHN_ABS); `comment` is (align, visibility, flags)."""
        b = BINDS[bind] if isinstance(bind, str) else bind
        t = TYPES[type] if isinstance(type, str) else type
        self._symbols.append(_Sym(name, section, value, size, b, t, shndx, comment))
        return self

    def file_symbol(self, name: str) -> "ElfBuilder":
        return self.symbol(name, None, bind="local", type="file", shndx=SHN_ABS)

    def reloc(self, section: str, offset: int, symbol: str | int, type: int | str, addend: int = 0) -> "ElfBuilder":
        t = RELOC_NUMBERS[type] if isinstance(type, str) else type
        self._relocs.append(_Rel(section, offset, symbol, t, addend))
        return self

    def comment(self, version: int = 14) -> "ElfBuilder":
        self._comment = version
        return self

    # output
    def ordered_symbols(self) -> list[_Sym]:
        if self.keep_order:
            return list(self._symbols)
        return [s for s in self._symbols if s.bind == STB_LOCAL] + [s for s in self._symbols if s.bind != STB_LOCAL]

    def build(self) -> bytes:
        secs = list(self._sections)
        index_of: dict[str, int] = {}
        for i, s in enumerate(secs):  # a repeated name resolves to its first section (symbols and relocations)
            index_of.setdefault(s.name, i + 1)
        syms = self.ordered_symbols()
        sym_index = {s.name: i + 1 for i, s in enumerate(syms)}
        if self._comment is not None:
            body = bytearray(COMMENT_HEADER)
            body[:len(COMMENT_MAGIC)] = COMMENT_MAGIC
            body[0x0B] = self._comment
            body[0x0C:0x10] = b"\x04\x00\x00\x01"
            body[0x10], body[0x11] = 1, 2
            body[0x12:0x14] = b"\x00\x16"
            body[0x14] = 0x2C
            body += bytes(8)  # the null symbol's entry
            for s in syms:
                align, vis, flags = s.comment or (4 if s.type in (STT_FUNC, STT_OBJECT) else 0, 0, 0)
                body += struct.pack(">IBBH", align, vis, flags, 0)
            secs.append(_Sec(".comment", bytes(body), 1, SHT_PROGBITS, 0, len(body), 0))
        # symbol table + strings
        strtab = bytearray(b"\0")
        symtab = bytearray(16)
        locals_count = 1
        for s in syms:
            if s.bind != STB_LOCAL:
                break
            locals_count += 1
        for s in syms:
            name_off = 0
            if s.name:
                name_off = len(strtab)
                strtab += s.name.encode("latin-1") + b"\0"
            shndx = s.shndx if s.shndx is not None else (index_of[s.section] if s.section else 0)
            symtab += struct.pack(">IIIBBH", name_off, s.value, s.size, (s.bind << 4) | s.type, 0, shndx)
        # relocation sections, one per target section, in section order
        rela: list[tuple[str, bytes, int]] = []
        for sec_index, sec in enumerate(secs, 1):
            rows = [r for r in self._relocs if r.section == sec.name and index_of.get(sec.name) == sec_index]
            if not rows:
                continue
            blob = bytearray()
            for r in rows:
                idx = r.symbol if isinstance(r.symbol, int) else sym_index[r.symbol]
                blob += struct.pack(">IIi", r.offset, (idx << 8) | r.type, r.addend)
            rela.append((".rela" + sec.name, bytes(blob), sec_index))
        n_user = len(secs)
        symtab_index = n_user + len(rela) + 1
        blocks = [(s.name, s.type, s.flags, s.data, s.size, 0, 0, s.align, 0, s.addr) for s in secs]
        blocks += [(name, SHT_RELA, 0, blob, len(blob), symtab_index, info, 4, 12, 0) for name, blob, info in rela]
        blocks.append((".symtab", SHT_SYMTAB, 0, bytes(symtab), len(symtab), symtab_index + 1, locals_count, 4, 16, 0))
        blocks.append((".strtab", SHT_STRTAB, 0, bytes(strtab), len(strtab), 0, 0, 1, 0, 0))
        shstr = bytearray(b"\0")
        name_offs = []
        for b in blocks + [(".shstrtab",)]:
            name_offs.append(len(shstr))
            shstr += b[0].encode("latin-1") + b"\0"
        blocks.append((".shstrtab", SHT_STRTAB, 0, bytes(shstr), len(shstr), 0, 0, 1, 0, 0))
        offsets, cursor = [], 52
        for _n, typ, _f, data, _sz, _l, _i, align, _e, _a in blocks:
            align = max(align, 1)
            cursor = (cursor + align - 1) // align * align
            offsets.append(cursor)
            cursor += 0 if typ == SHT_NOBITS else len(data)
        shoff = (cursor + 3) // 4 * 4
        out = bytearray(shoff + 40 * (len(blocks) + 1))
        for (_n, typ, _f, data, _sz, _l, _i, _al, _e, _a), off in zip(blocks, offsets):
            if typ != SHT_NOBITS:
                out[off:off + len(data)] = data
        struct.pack_into(">4sBBBB8s", out, 0, b"\x7fELF", 1, 2, 1, 0, b"\0" * 8)
        struct.pack_into(">HHIIIIIHHHHHH", out, 0x10, 1, self.machine, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                         len(blocks) + 1, len(blocks))
        for i, ((_n, typ, flags, _d, size, link, info, align, entsize, addr), off) in enumerate(zip(blocks, offsets)):
            struct.pack_into(">IIIIIIIIII", out, shoff + (i + 1) * 40, name_offs[i], typ, flags, addr, off, size,
                             link, info, align, entsize)
        return bytes(out)

    def write(self, path: str | os.PathLike) -> str:
        with open(path, "wb") as handle:
            handle.write(self.build())
        return os.fspath(path)


@dataclass
class DolBuilder:
    """Build a DOL image: `text(address, bytes)` / `data(address, bytes)` fill text0.. / data0.. in call order.

    Segment bodies follow the 0x100 header back to back; `pad=0x20` rounds each slot's size up the way the
    retail linker does (the header's size then includes the padding)."""
    pad: int = 1
    bss_address: int = 0
    bss_size: int = 0
    entry: int = 0
    _text: list[tuple[int, bytes]] = field(default_factory=list)
    _data: list[tuple[int, bytes]] = field(default_factory=list)

    def text(self, address: int, blob: bytes) -> "DolBuilder":
        if len(self._text) >= 7:
            raise ValueError("a DOL has seven text slots")
        self._text.append((address, bytes(blob)))
        return self

    def data(self, address: int, blob: bytes) -> "DolBuilder":
        if len(self._data) >= 11:
            raise ValueError("a DOL has eleven data slots")
        self._data.append((address, bytes(blob)))
        return self

    def bss(self, address: int, size: int) -> "DolBuilder":
        self.bss_address, self.bss_size = address, size
        return self

    def build(self) -> bytes:
        header = bytearray(0x100)
        body = bytearray()
        for base_off, base_addr, base_size, items in ((0x00, 0x48, 0x90, self._text), (0x1C, 0x64, 0xAC, self._data)):
            for i, (address, blob) in enumerate(items):
                size = (len(blob) + self.pad - 1) // self.pad * self.pad
                struct.pack_into(">I", header, base_off + 4 * i, 0x100 + len(body))
                struct.pack_into(">I", header, base_addr + 4 * i, address)
                struct.pack_into(">I", header, base_size + 4 * i, size)
                body += blob + bytes(size - len(blob))
        struct.pack_into(">III", header, 0xD8, self.bss_address, self.bss_size, self.entry)
        return bytes(header) + bytes(body)

    def write(self, path: str | os.PathLike) -> str:
        with open(path, "wb") as handle:
            handle.write(self.build())
        return os.fspath(path)
