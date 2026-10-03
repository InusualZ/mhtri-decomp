"""ELF objects: one reader (sections, symbols, relocations, the CodeWarrior .comment) and one in-place editor.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

import os
import struct
from dataclasses import dataclass, field

TYPE_CHECKING = False
if TYPE_CHECKING:  # annotations only: no `typing` import on the per-object ninja steps' path
    from collections.abc import Iterable, Sequence

# --- constants ------------------------------------------------------------------------------------------------

SHT_NULL, SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS, SHT_REL = 0, 1, 2, 3, 4, 8, 9
STB_LOCAL, STB_GLOBAL, STB_WEAK = 0, 1, 2
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION, STT_FILE = 0, 1, 2, 3, 4
SHN_UNDEF, SHN_ABS, SHN_COMMON = 0, 0xFFF1, 0xFFF2
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR = 0x1, 0x2, 0x4

#: The PowerPC (SysV + EABI) relocation names, by type number.
RELOC_NAMES = {
    0: "R_PPC_NONE", 1: "R_PPC_ADDR32", 2: "R_PPC_ADDR24", 3: "R_PPC_ADDR16", 4: "R_PPC_ADDR16_LO",
    5: "R_PPC_ADDR16_HI", 6: "R_PPC_ADDR16_HA", 7: "R_PPC_ADDR14", 8: "R_PPC_ADDR14_BRTAKEN",
    9: "R_PPC_ADDR14_BRNTAKEN", 10: "R_PPC_REL24", 11: "R_PPC_REL14", 12: "R_PPC_REL14_BRTAKEN",
    13: "R_PPC_REL14_BRNTAKEN", 14: "R_PPC_GOT16", 15: "R_PPC_GOT16_LO", 16: "R_PPC_GOT16_HI",
    17: "R_PPC_GOT16_HA", 18: "R_PPC_PLTREL24", 19: "R_PPC_COPY", 20: "R_PPC_GLOB_DAT", 21: "R_PPC_JMP_SLOT",
    22: "R_PPC_RELATIVE", 23: "R_PPC_LOCAL24PC", 24: "R_PPC_UADDR32", 25: "R_PPC_UADDR16", 26: "R_PPC_REL32",
    27: "R_PPC_PLT32", 28: "R_PPC_PLTREL32", 29: "R_PPC_PLT16_LO", 30: "R_PPC_PLT16_HI", 31: "R_PPC_PLT16_HA",
    32: "R_PPC_SDAREL16", 33: "R_PPC_SECTOFF", 34: "R_PPC_SECTOFF_LO", 35: "R_PPC_SECTOFF_HI",
    36: "R_PPC_SECTOFF_HA", 37: "R_PPC_ADDR30",
    101: "R_PPC_EMB_NADDR32", 102: "R_PPC_EMB_NADDR16", 103: "R_PPC_EMB_NADDR16_LO",
    104: "R_PPC_EMB_NADDR16_HI", 105: "R_PPC_EMB_NADDR16_HA", 106: "R_PPC_EMB_SDAI16",
    107: "R_PPC_EMB_SDA2I16", 108: "R_PPC_EMB_SDA2REL", 109: "R_PPC_EMB_SDA21", 110: "R_PPC_EMB_MRKREF",
    111: "R_PPC_EMB_RELSEC16", 112: "R_PPC_EMB_RELST_LO", 113: "R_PPC_EMB_RELST_HI",
    114: "R_PPC_EMB_RELST_HA", 115: "R_PPC_EMB_BIT_FLD", 116: "R_PPC_EMB_RELSDA",
}
RELOC_NUMBERS = {name: number for number, name in RELOC_NAMES.items()}

#: The CodeWarrior `.comment` header (docs/comment_section.md): magic, then one 8-byte entry per ELF symbol.
COMMENT_MAGIC = b"CodeWarrior"
COMMENT_HEADER = 0x2C


def reloc_name(rtype: int, fallback: str = "R_PPC_%d") -> str:
    """The `R_PPC_*` name of relocation type `rtype`, or `fallback % rtype` for an unknown number."""
    return RELOC_NAMES.get(rtype) or fallback % rtype


class ElfError(ValueError):
    """The bytes are not an ELF object the reader can walk (bad magic, truncated header table)."""


# --- value types ------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Section:
    """One section header. `raw` is the file slice at (offset, size) whatever the type; `data` is that slice,
    or empty for `SHT_NOBITS` (`.bss` occupies no file bytes)."""
    index: int
    name: str
    type: int
    flags: int
    addr: int
    offset: int
    size: int
    link: int
    info: int
    align: int
    entsize: int
    header: int  # file offset of this section header
    name_offset: int = 0
    raw: bytes = field(default=b"", repr=False)

    @property
    def data(self) -> bytes:
        return b"" if self.type == SHT_NOBITS else self.raw

    @property
    def is_nobits(self) -> bool:
        return self.type == SHT_NOBITS


@dataclass(frozen=True)
class Symbol:
    """One `.symtab` row; `section` is the defining section's name ("" when undefined, absolute or common)."""
    index: int
    name: str
    value: int
    size: int
    info: int
    other: int
    shndx: int
    section: str = ""
    header: int = 0  # file offset of this symbol entry

    @property
    def bind(self) -> int:
        return self.info >> 4

    @property
    def type(self) -> int:
        return self.info & 0xF

    @property
    def defined(self) -> bool:
        return self.shndx != SHN_UNDEF


@dataclass(frozen=True)
class Reloc:
    """One RELA entry; `section` is the section it applies to, `rela` (and `rela_index`) the relocation section
    it came from - an object can repeat a section name, the index cannot."""
    offset: int
    symbol: int
    symbol_name: str
    type: int
    addend: int
    section: str
    rela: str
    rela_index: int = 0

    @property
    def type_name(self) -> str:
        return reloc_name(self.type)


@dataclass(frozen=True)
class CommentEntry:
    """One per-symbol `.comment` entry: `[align:4][visibility:1][active_flags:1][pad:2]`."""
    align: int
    visibility: int
    active_flags: int


@dataclass(frozen=True)
class Comment:
    """The CodeWarrior `.comment` section: `version` is the byte after the magic (dtk's `mw_comment_version`)."""
    version: int
    compiler: bytes
    pool_data: int
    float_type: int
    processor: int
    quirks: int
    entries: tuple[CommentEntry, ...]
    raw: bytes = field(default=b"", repr=False)


@dataclass(frozen=True)
class ProgramHeader:
    type: int
    offset: int
    vaddr: int
    paddr: int
    filesz: int
    memsz: int
    flags: int
    align: int


# --- the reader -------------------------------------------------------------------------------------------------

def _maker(cls):
    """A positional constructor for frozen dataclass `cls` that skips the frozen `__setattr__` path (the reader
    builds tens of thousands of rows per link check; this halves the cost). The values are the fields in
    declaration order, every one given; the instance is the same frozen value `cls(...)` makes."""
    names = tuple(f for f in cls.__dataclass_fields__)
    new = object.__new__

    def make(*values):
        obj = new(cls)
        obj.__dict__.update(zip(names, values))
        return obj
    return make


_new_section, _new_symbol, _new_reloc = _maker(Section), _maker(Symbol), _maker(Reloc)


def _cstr(blob: bytes, start: int, end: int | None = None) -> str:
    """The NUL-terminated latin-1 string at `start` ("" when `start` is outside `blob`)."""
    if start < 0 or start >= len(blob):
        return ""
    stop = blob.find(b"\0", start, len(blob) if end is None else end)
    if stop < 0:
        stop = len(blob) if end is None else end
    return blob[start:stop].decode("latin-1")


class Elf:
    """A parsed ELF file (32- or 64-bit, either endianness; MWCC objects are ELF32 big-endian).

    The reader is lenient past the header table: a string offset outside its table reads as "", a section
    whose bytes run past the file is truncated. It refuses (ElfError) only what it cannot walk: a missing
    magic or a header table that is not in the file.
    """

    def __init__(self, raw: bytes, path: str | None = None) -> None:
        self.raw = bytes(raw)
        self.path = path
        data = self.raw
        if len(data) < 0x34 or data[:4] != b"\x7fELF":
            raise ElfError("not an ELF file")
        self.ei_class, self.ei_data = data[4], data[5]
        self.is64 = self.ei_class == 2
        self.big_endian = self.ei_data != 1
        e = ">" if self.big_endian else "<"
        self._e = e
        try:
            if self.is64:
                (self.e_type, self.machine, _ver, self.entry, self.phoff, self.shoff, _flags, _ehsize,
                 self.phentsize, self.phnum, self.shentsize, self.shnum, self.shstrndx) = \
                    struct.unpack_from(e + "HHIQQQIHHHHHH", data, 0x10)
            else:
                (self.e_type, self.machine, _ver, self.entry, self.phoff, self.shoff, _flags, _ehsize,
                 self.phentsize, self.phnum, self.shentsize, self.shnum, self.shstrndx) = \
                    struct.unpack_from(e + "HHIIIIIHHHHHH", data, 0x10)
        except struct.error as exc:
            raise ElfError("truncated ELF header: %s" % exc) from None
        self.sections: tuple[Section, ...] = self._read_sections()
        self._symbols: tuple[Symbol, ...] | None = None

    # construction
    @classmethod
    def read(cls, source: str | os.PathLike | bytes | bytearray | memoryview) -> "Elf":
        """Parse a path or the bytes of an ELF file."""
        if isinstance(source, (bytes, bytearray, memoryview)):
            return cls(bytes(source))
        with open(source, "rb") as handle:
            return cls(handle.read(), os.fspath(source))

    def require_be32(self) -> "Elf":
        """Raise ElfError unless this is ELF32 big-endian (the only shape MWCC writes); returns self."""
        if self.ei_class != 1 or self.ei_data != 2:
            raise ElfError("expected a 32-bit big-endian ELF")
        return self

    def _read_sections(self) -> tuple[Section, ...]:
        data, e = self.raw, self._e
        if not self.shnum or not self.shoff:
            return ()
        fmt = e + ("IIQQQQIIQQ" if self.is64 else "IIIIIIIIII")
        minimum = struct.calcsize(fmt)
        if self.shentsize < minimum:
            raise ElfError("unexpected section header size %d" % self.shentsize)
        rows = []
        try:
            for index in range(self.shnum):
                base = self.shoff + index * self.shentsize
                rows.append((index, base) + struct.unpack_from(fmt, data, base))
        except struct.error as exc:
            raise ElfError("section header table outside the file: %s" % exc) from None
        names = b""
        if 0 <= self.shstrndx < len(rows):
            # (index, base, name, type, flags, addr, offset, size, ...)
            s_off, s_size = rows[self.shstrndx][6], rows[self.shstrndx][7]
            names = data[s_off:s_off + s_size]
        out = []
        for index, base, name_off, typ, flags, addr, offset, size, link, info, align, entsize in rows:
            out.append(_new_section(index, _cstr(names, name_off), typ, flags, addr, offset, size, link, info,
                                    align, entsize, base, name_off,
                                    data[offset:offset + size] if typ != SHT_NULL else b""))
        return tuple(out)

    # sections
    def section(self, name: str) -> Section | None:
        """The first section called `name`, or None."""
        for sec in self.sections:
            if sec.name == name:
                return sec
        return None

    def section_bytes(self, name: str) -> bytes | None:
        """The file bytes of section `name` (empty for NOBITS), or None when there is no such section."""
        sec = self.section(name)
        return None if sec is None else sec.data

    def section_name(self, index: int) -> str:
        """The name of section `index`, "" for a reserved or out-of-range index."""
        return self.sections[index].name if 0 <= index < len(self.sections) else ""

    @property
    def symtab(self) -> Section | None:
        return next((s for s in self.sections if s.type == SHT_SYMTAB), None)

    @property
    def locals_end(self) -> int:
        """`.symtab`'s `sh_info`: the index of the first non-local symbol (every local sits below it)."""
        st = self.symtab
        return st.info if st is not None else 0

    # symbols
    @property
    def symbols(self) -> tuple[Symbol, ...]:
        """Every `.symtab` row in index order, the null symbol (index 0) included."""
        if self._symbols is None:
            self._symbols = self._read_symbols()
        return self._symbols

    def _read_symbols(self) -> tuple[Symbol, ...]:
        st = self.symtab
        if st is None:
            return ()
        strings = self.sections[st.link].raw if 0 <= st.link < len(self.sections) else b""
        e = self._e
        if self.is64:
            fmt, default = e + "IBBHQQ", 24
        else:
            fmt, default = e + "IIIBBH", 16
        step = st.entsize or default
        size = struct.calcsize(fmt)
        out = []
        raw = st.raw
        nsec = len(self.sections)
        for i in range(len(raw) // step if step else 0):
            off = i * step
            if off + size > len(raw):
                break
            if self.is64:
                name_off, info, other, shndx, value, sym_size = struct.unpack_from(fmt, raw, off)
            else:
                name_off, value, sym_size, info, other, shndx = struct.unpack_from(fmt, raw, off)
            section = self.sections[shndx].name if 0 < shndx < nsec else ""
            out.append(_new_symbol(i, _cstr(strings, name_off) if name_off else "", value, sym_size, info, other,
                                   shndx, section, st.offset + off))
        return tuple(out)

    def symbol_name(self, index: int) -> str:
        syms = self.symbols
        return syms[index].name if 0 <= index < len(syms) else ""

    @property
    def defined(self) -> set[str]:
        """Names the object defines (any binding)."""
        return {s.name for s in self.symbols if s.name and s.defined}

    @property
    def undefined(self) -> set[str]:
        """Names the object references and does not define (`SHN_UNDEF`)."""
        return {s.name for s in self.symbols[1:] if s.name and not s.defined}

    @property
    def globals(self) -> set[str]:
        """Defined names with global or weak binding - the ones that can satisfy another object's reference."""
        return {s.name for s in self.symbols if s.name and s.defined and s.bind in (STB_GLOBAL, STB_WEAK)}

    # relocations
    def rela_sections(self) -> list[tuple[Section, str]]:
        """`(relocation section, name of the section it applies to)` for every SHT_RELA section.

        The target is `sh_info` when it names a real section, else the name with `.rela` dropped - the two
        agree for every MWCC object."""
        out = []
        for sec in self.sections:
            if sec.type != SHT_RELA:
                continue
            if 0 < sec.info < len(self.sections):
                target = self.sections[sec.info].name
            else:
                target = sec.name[5:] if sec.name.startswith(".rela") else ""
            out.append((sec, target))
        return out

    def relocs(self, section: str | None = None) -> list[Reloc]:
        """RELA entries in file order, of every relocation section or only those applying to `section`."""
        e = self._e
        out = []
        syms = self.symbols
        nsym = len(syms)
        for rela, target in self.rela_sections():
            if section is not None and target != section:
                continue
            if self.is64:
                fmt, default = e + "QQq", 24
            else:
                fmt, default = e + "IIi", 12
            step = rela.entsize or default
            size = struct.calcsize(fmt)
            raw = rela.raw
            for off in range(0, len(raw) - size + 1, step):
                r_offset, r_info, addend = struct.unpack_from(fmt, raw, off)
                if self.is64:
                    index, rtype = r_info >> 32, r_info & 0xFFFFFFFF
                else:
                    index, rtype = r_info >> 8, r_info & 0xFF
                name = syms[index].name if index < nsym else ""
                out.append(_new_reloc(r_offset, index, name, rtype, addend, target, rela.name, rela.index))
        return out

    # the CodeWarrior .comment
    @property
    def comment(self) -> Comment | None:
        """The parsed `.comment` section, or None when the object has none or it is not CodeWarrior's."""
        sec = self.section(".comment")
        if sec is None or not sec.raw.startswith(COMMENT_MAGIC) or len(sec.raw) < 0x16:
            return None
        raw = sec.raw
        entries = []
        for off in range(COMMENT_HEADER, len(raw) - 7, 8):
            align, visibility, flags = struct.unpack_from(">IBB", raw, off)
            entries.append(CommentEntry(align, visibility, flags))
        return Comment(raw[0x0B], raw[0x0C:0x10], raw[0x10], raw[0x11], struct.unpack_from(">H", raw, 0x12)[0],
                       raw[0x15], tuple(entries), raw)

    # program headers (linked images)
    @property
    def program_headers(self) -> tuple[ProgramHeader, ...]:
        e, out = self._e, []
        fmt = e + ("IIQQQQQQ" if self.is64 else "IIIIIIII")
        for i in range(self.phnum if self.phoff else 0):
            vals = struct.unpack_from(fmt, self.raw, self.phoff + i * self.phentsize)
            if self.is64:
                typ, flags, offset, vaddr, paddr, filesz, memsz, align = vals
            else:
                typ, offset, vaddr, paddr, filesz, memsz, flags, align = vals
            out.append(ProgramHeader(typ, offset, vaddr, paddr, filesz, memsz, flags, align))
        return tuple(out)


# --- the editor -------------------------------------------------------------------------------------------------

class ElfEditor:
    """In-place edits of an ELF32 big-endian object that keep every section's contents.

    `set_section_align` and `set_symbol_info` patch a header field; `rename_symbols` appends the new names to
    the symbol string table and splices it back, shifting every later section (padded to their largest
    alignment, so each keeps its own `sh_addralign`) and the header-table offsets. Nothing else is rewritten.
    """

    def __init__(self, elf: Elf) -> None:
        elf.require_be32()
        self.elf = elf
        self.data = bytearray(elf.raw)
        self._renames: list[tuple[int, str]] = []

    def set_section_align(self, index: int, align: int) -> None:
        struct.pack_into(">I", self.data, self.elf.sections[index].header + 32, align)

    def set_symbol_info(self, index: int, info: int) -> None:
        struct.pack_into(">B", self.data, self.elf.symbols[index].header + 12, info)

    def rename_symbols(self, renames: Sequence[tuple[int, str]]) -> None:
        """Queue `(symbol index, new name)` pairs; applied by `to_bytes`/`write` in this order."""
        self._renames.extend(renames)

    def to_bytes(self) -> bytes:
        if not self._renames:
            return bytes(self.data)
        elf = self.elf
        st = elf.symtab
        if st is None or not (0 <= st.link < len(elf.sections)):
            raise ElfError("rename needs a .symtab with a string table")
        strtab = elf.sections[st.link]
        data = self.data
        appended = bytearray()
        for index, name in self._renames:
            struct.pack_into(">I", data, elf.symbols[index].header, strtab.size + len(appended))
            appended += name.encode("latin-1") + b"\0"
        str_off, old_size = strtab.offset, strtab.size
        moved = [s.align for s in elf.sections if s.offset >= str_off + old_size]
        align = max(moved) if moved else 1
        if align > 1:
            appended += b"\0" * ((-len(appended)) % align)
        delta = len(appended)
        out = bytearray(data[:str_off]) + bytes(data[str_off:str_off + old_size]) + bytes(appended) \
            + bytearray(data[str_off + old_size:])

        def shifted(position: int) -> int:
            return position + delta if position > str_off else position

        struct.pack_into(">I", out, 0x1C, shifted(struct.unpack_from(">I", out, 0x1C)[0]))  # e_phoff
        struct.pack_into(">I", out, 0x20, shifted(struct.unpack_from(">I", out, 0x20)[0]))  # e_shoff
        for sec in elf.sections:
            header = shifted(sec.header)
            struct.pack_into(">I", out, header + 16, shifted(sec.offset))
            if sec.index == strtab.index:
                struct.pack_into(">I", out, header + 20, old_size + delta)
        return bytes(out)

    def write(self, path: str | os.PathLike) -> None:
        with open(path, "wb") as handle:
            handle.write(self.to_bytes())


def iter_objects(paths: Iterable[str | os.PathLike]) -> Iterable[tuple[str, Elf | None]]:
    """`(path, Elf or None)` for each path; None when it cannot be read or is not an ELF file."""
    for path in paths:
        try:
            yield os.fspath(path), Elf.read(path)
        except (OSError, ElfError):
            yield os.fspath(path), None
