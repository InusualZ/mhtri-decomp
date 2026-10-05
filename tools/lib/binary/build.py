"""Fixture builders: ElfBuilder (an ELF32 big-endian relocatable object), PeBuilder (a PE32 host tool), DolBuilder.
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
    section: str | int
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

    def reloc(self, section: str | int, offset: int, symbol: str | int, type: int | str,
              addend: int = 0) -> "ElfBuilder":
        """`section` is a name (its first section) or a 1-based section index in call order (a repeated name)."""
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
            rows = [r for r in self._relocs
                    if r.section == sec_index or (r.section == sec.name and index_of.get(sec.name) == sec_index)]
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
class PeBuilder:
    """Build a PE32 image shaped like the Metrowerks host tools for a fixture (nothing in it runs).

    `section(name, data, vsize, va)` adds a section at `va` (default: the next free 0x1000 page, call order);
    `rva(name, offset)` is where an offset of it lands. `imports({dll: [names]})` adds `.idata`,
    `strings({block_id: [text, ...]})` an `.rsrc` with RT_STRING blocks, `codeview([(section, offset, name)])` a
    debug directory with an `NB11` blob; those generated sections follow the caller's.
    """
    image_base: int = 0x400000
    machine: int = 0x014C
    entry: int = 0
    _sections: list[tuple[str, bytes, int, int]] = field(default_factory=list)
    _imports: dict[str, list[str]] = field(default_factory=dict)
    _strings: dict[int, list[str]] = field(default_factory=dict)
    _codeview: list[tuple[str, int, str]] = field(default_factory=list)

    @staticmethod
    def _page(n: int) -> int:
        return (n + 0xFFF) // 0x1000 * 0x1000

    def _next_va(self) -> int:
        end = 0x1000
        for _name, data, vsize, va in self._sections:
            end = max(end, self._page(va + max(vsize, len(data), 1)))
        return end

    def section(self, name: str, data: bytes = b"", vsize: int | None = None, va: int | None = None) -> "PeBuilder":
        self._sections.append((name, bytes(data), len(data) if vsize is None else vsize,
                               self._next_va() if va is None else va))
        return self

    def imports(self, table: dict[str, list[str]]) -> "PeBuilder":
        self._imports = {dll: list(names) for dll, names in table.items()}
        return self

    def strings(self, blocks: dict[int, list[str]]) -> "PeBuilder":
        self._strings = {bid: list(texts) for bid, texts in blocks.items()}
        return self

    def codeview(self, symbols: list[tuple[str, int, str]]) -> "PeBuilder":
        self._codeview = list(symbols)
        return self

    def rva(self, name: str, offset: int = 0) -> int:
        """The RVA of `offset` inside the caller's section `name`."""
        for sec, _data, _vsize, va in self._sections:
            if sec == name:
                return va + offset
        raise KeyError(name)

    def _idata(self, va: int) -> bytes:
        """Descriptors, then per DLL its thunk array (used as both lookup and address table), names, hint/names."""
        dlls = list(self._imports.items())
        blob = bytearray(20 * (len(dlls) + 1))
        for i, (dll, names) in enumerate(dlls):
            thunks_at = len(blob)
            blob += bytes(4 * (len(names) + 1))
            dll_at = len(blob)
            blob += dll.encode("latin-1") + b"\0"
            for j, name in enumerate(names):
                if len(blob) % 2:
                    blob += b"\0"
                struct.pack_into("<I", blob, thunks_at + 4 * j, va + len(blob))
                blob += struct.pack("<H", 0) + name.encode("latin-1") + b"\0"
            struct.pack_into("<IIIII", blob, 20 * i, 0, 0, 0, va + dll_at, va + thunks_at)
        return bytes(blob)

    def _rsrc(self, va: int) -> bytes:
        """type RT_STRING -> one name entry per block -> one language entry -> the data entry and the block."""
        blocks = sorted(self._strings.items())
        n = len(blocks)
        type_dir = 16 + 8
        name_dirs = type_dir + 16 + 8 * n
        data_entries = name_dirs + n * (16 + 8)
        blob = bytearray(data_entries + 16 * n)
        struct.pack_into("<IIHHHH", blob, 0, 0, 0, 0, 0, 0, 1)
        struct.pack_into("<II", blob, 16, 6, 0x80000000 | type_dir)
        struct.pack_into("<IIHHHH", blob, type_dir, 0, 0, 0, 0, 0, n)
        for k, (bid, texts) in enumerate(blocks):
            lang_dir = name_dirs + k * 24
            struct.pack_into("<II", blob, type_dir + 16 + 8 * k, bid, 0x80000000 | lang_dir)
            struct.pack_into("<IIHHHH", blob, lang_dir, 0, 0, 0, 0, 0, 1)
            struct.pack_into("<II", blob, lang_dir + 16, 0x409, data_entries + 16 * k)
            body = b"".join(struct.pack("<H", len(t)) + t.encode("utf-16le") for t in (texts + [""] * 16)[:16])
            struct.pack_into("<IIII", blob, data_entries + 16 * k, va + len(blob), len(body), 0, 0)
            blob += body
        return bytes(blob)

    def _nb11(self, laid: list[list]) -> bytes:
        blob = bytearray(b"NB11")
        for sec, off, sym in self._codeview:
            sec_index = next(j for j, s in enumerate(laid) if s[0] == sec) + 1
            raw = sym.encode("latin-1")
            rec = struct.pack("<HIIH", 0x1009, 0, off, sec_index) + bytes([len(raw)]) + raw
            blob += struct.pack("<H", len(rec)) + rec
        return bytes(blob)

    def build(self) -> bytes:
        laid = [[name, data, vsize, va] for name, data, vsize, va in self._sections]
        va = self._next_va()
        dirs: dict[int, tuple[int, int]] = {}
        for name, index, make in ((".idata", 1, self._idata if self._imports else None),
                                  (".rsrc", 2, self._rsrc if self._strings else None),
                                  (".debug", 6, (lambda _va: b"") if self._codeview else None)):
            if make is None:
                continue
            data = make(va)
            laid.append([name, data, len(data), va])
            dirs[index] = (va, len(data))
            va += 0x1000
        nsec = len(laid)
        raw_cursor = (0x40 + 4 + 20 + 0xE0 + 40 * nsec + 0x1FF) // 0x200 * 0x200
        raws = []
        for entry in laid:
            if entry[0] == ".debug" and self._codeview:
                # the directory entry, then the NB11 blob it points at (by file offset, as the linker writes it)
                blob = self._nb11(laid)
                entry[1] = struct.pack("<IIHHIIII", 0, 0, 0, 0, 2, len(blob), 0, raw_cursor + 28) + blob
                entry[2] = len(entry[1])
                dirs[6] = (entry[3], 28)
            raws.append((raw_cursor, len(entry[1])))
            raw_cursor += (len(entry[1]) + 0x1FF) // 0x200 * 0x200
        out = bytearray(raw_cursor)
        out[0:2] = b"MZ"
        struct.pack_into("<I", out, 0x3C, 0x40)
        out[0x40:0x44] = b"PE\0\0"
        struct.pack_into("<HHIIIHH", out, 0x44, self.machine, nsec, 0, 0, 0, 0xE0, 0x0102)
        opt = 0x58
        struct.pack_into("<H", out, opt, 0x10B)
        struct.pack_into("<I", out, opt + 16, self.entry)
        struct.pack_into("<I", out, opt + 28, self.image_base)
        struct.pack_into("<II", out, opt + 32, 0x1000, 0x200)
        struct.pack_into("<I", out, opt + 56, va)
        struct.pack_into("<I", out, opt + 92, 16)
        for index, (dva, size) in dirs.items():
            struct.pack_into("<II", out, opt + 96 + 8 * index, dva, size)
        so = opt + 0xE0
        for i, ((name, data, vsize, sva), (raw_off, raw_size)) in enumerate(zip(laid, raws)):
            hdr = name.encode("latin-1")[:8].ljust(8, b"\0")
            struct.pack_into("<8sIIIIIIHHI", out, so + 40 * i, hdr, vsize, sva, raw_size, raw_off, 0, 0, 0, 0,
                             0x40000040)
            out[raw_off:raw_off + len(data)] = data
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
