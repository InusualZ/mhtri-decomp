"""lib.binary: builder -> reader round trips (ELF, DOL), the editor, DWARF decoding and the disassembly tokenizer."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import dataclasses
import os
import struct
import tempfile

from tools.lib import testing
from tools.lib.binary import dwarf, objdump
from tools.lib.binary.build import DolBuilder, ElfBuilder
from tools.lib.binary.dol import Dol, DolError
from tools.lib.binary.elf import (SHN_ABS, SHT_NOBITS, SHT_RELA, STB_GLOBAL, STB_LOCAL, STT_FILE, STT_FUNC, Elf,
                                  ElfEditor, ElfError, Symbol, reloc_name)

TIER = "fixture"


def _object() -> ElfBuilder:
    b = ElfBuilder()
    b.section(".text", bytes(range(0x20)), align=4)
    b.section(".data", b"\x11\x22\x33\x44" * 2, align=8)
    b.nobits(".bss", 0x40, align=8)
    b.symbol("global_fn", ".text", 0x10, 0x10, bind="global", type="func")
    b.symbol("local_fn", ".text", 0x0, 0x10, bind="local", type="func")
    b.file_symbol("unit.c")
    b.symbol("table", ".data", 0x0, 0x8, bind="global", type="object")
    b.symbol("weak_one", ".data", 0x4, 0x4, bind="weak", type="object")
    b.symbol("extern_fn")
    b.reloc(".text", 0x4, "extern_fn", "R_PPC_REL24")
    b.reloc(".text", 0xA, "table", "R_PPC_ADDR16_HA", 4)
    b.reloc(".data", 0x0, "global_fn", 1, -8)
    return b.comment(14)


def test_elf_round_trip_every_field(c):
    elf = Elf.read(_object().build())
    names = [s.name for s in elf.sections]
    c.check("sections in builder order, then rela, symtab, strtab, shstrtab", names,
            ["", ".text", ".data", ".bss", ".comment", ".rela.text", ".rela.data", ".symtab", ".strtab",
             ".shstrtab"])
    text = elf.section(".text")
    c.check(".text bytes", text.data, bytes(range(0x20)))
    c.check(".text size/align/flags", (text.size, text.align, text.flags), (0x20, 4, 6))
    c.check(".data align", elf.section(".data").align, 8)
    bss = elf.section(".bss")
    c.check(".bss is NOBITS with a size and no bytes", (bss.type, bss.size, bss.data), (SHT_NOBITS, 0x40, b""))
    c.check("section_bytes", elf.section_bytes(".data"), b"\x11\x22\x33\x44" * 2)
    c.check("section_bytes of a missing section", elf.section_bytes(".nope"), None)
    c.check("be32", (elf.ei_class, elf.ei_data, elf.is64, elf.big_endian), (1, 2, False, True))

    syms = elf.symbols
    c.check("null symbol first", (syms[0].index, syms[0].name, syms[0].shndx), (0, "", 0))
    by = {s.name: s for s in syms if s.name}
    c.check("locals are below sh_info", sorted(s.name for s in syms[1:elf.locals_end]), ["local_fn", "unit.c"])
    c.expect("... and every symbol at or above sh_info is non-local",
             all(s.bind != STB_LOCAL for s in syms[elf.locals_end:]))
    g = by["global_fn"]
    c.check("symbol fields", (g.value, g.size, g.bind, g.type, g.section), (0x10, 0x10, STB_GLOBAL, STT_FUNC, ".text"))
    c.check("file symbol", (by["unit.c"].type, by["unit.c"].shndx, by["unit.c"].section), (STT_FILE, SHN_ABS, ""))
    c.check("undefined", elf.undefined, {"extern_fn"})
    c.check("defined", elf.defined, {"global_fn", "local_fn", "unit.c", "table", "weak_one"})
    c.check("globals (global + weak)", elf.globals, {"global_fn", "table", "weak_one"})

    rel = elf.relocs(".text")
    c.check(".rela.text pairs with .text",
            [(r.offset, r.symbol_name, r.type_name, r.addend, r.section, r.rela) for r in rel],
            [(4, "extern_fn", "R_PPC_REL24", 0, ".text", ".rela.text"),
             (0xA, "table", "R_PPC_ADDR16_HA", 4, ".text", ".rela.text")])
    c.check("negative addend", [(r.symbol_name, r.addend) for r in elf.relocs(".data")], [("global_fn", -8)])
    c.check("all relocs", len(elf.relocs()), 3)
    c.check("rela sh_info names the target", [(s.name, t, s.type) for s, t in elf.rela_sections()],
            [(".rela.text", ".text", SHT_RELA), (".rela.data", ".data", SHT_RELA)])

    c.raises("a read symbol is a frozen value", dataclasses.FrozenInstanceError, setattr, g, "name", "x")
    c.check("... equal to the one the constructor makes", g,
            Symbol(g.index, "global_fn", 0x10, 0x10, g.info, g.other, g.shndx, ".text", g.header))
    com = elf.comment
    c.check("comment version", com.version, 14)
    c.check("one comment entry per symbol (null included)", len(com.entries), len(syms))
    c.check("reloc_name fallback", (reloc_name(10), reloc_name(250), reloc_name(250, "type-%d")),
            ("R_PPC_REL24", "R_PPC_250", "type-250"))


def test_repeated_section_name(c):
    # dtk can write two pieces under one name (`.data` twice in a split target object)
    b = ElfBuilder().section(".data", b"AAAA").section(".data", b"BBBBBBBB")
    b.symbol("a", ".data", 0, 4, type="object").symbol("ext")
    elf = Elf.read(b.reloc(".data", 0, "ext", 1).build())
    first, second = [s for s in elf.sections if s.name == ".data"]
    c.check("section() is the first of a repeated name", elf.section(".data").index, first.index)
    c.check("a symbol names the piece it sits in by index", next(s for s in elf.symbols if s.name == "a").shndx,
            first.index)
    rel = elf.relocs(".data")
    c.check("relocations carry their relocation section's index, not just its name",
            [(r.rela, r.rela_index > second.index, r.symbol_name) for r in rel], [(".rela.data", True, "ext")])
    # a relocation addressed by section index lands on the second piece, with its own `.rela.data`
    b2 = ElfBuilder().section(".data", b"AAAA").section(".data", b"BBBBBBBB").symbol("ext")
    elf2 = Elf.read(b2.reloc(".data", 0, "ext", 1).reloc(2, 4, "ext", 1).build())
    first2, second2 = [s for s in elf2.sections if s.name == ".data"]
    c.check("reloc(<index>) targets that piece: one `.rela.data` per piece, each paired by sh_info",
            sorted((rela.info, len(rela.data)) for rela, _t in elf2.rela_sections()),
            [(first2.index, 12), (second2.index, 12)])


def test_fixture_tree_takes_a_builder(c):
    with testing.FixtureTree() as tree:
        path = tree.add_object("Dir/file.o", _object(), side="src")
        c.check("FixtureTree.add_object builds an ElfBuilder", Elf.read(path).section_bytes(".text"),
                bytes(range(0x20)))


def test_elf_refusals_and_leniency(c):
    c.raises("not ELF", ElfError, Elf.read, b"\0" * 64)
    c.raises("short", ElfError, Elf.read, b"\x7fELF")
    blob = bytearray(_object().build())
    struct.pack_into(">I", blob, 0x20, len(blob) + 0x1000)  # e_shoff past the end
    c.raises("header table outside the file", ElfError, Elf.read, bytes(blob))
    le = bytearray(_object().build())
    le[5] = 1
    c.raises("require_be32 refuses little-endian", ElfError, lambda: Elf(bytes(le)).require_be32())
    with tempfile.TemporaryDirectory() as tmp:
        path = _object().write(os.path.join(tmp, "a.o"))
        c.check("read from a path", (Elf.read(path).path, len(Elf.read(path).sections)), (path, 10))


def test_editor_is_byte_neutral_outside_symtab(c):
    elf = Elf.read(_object().build())
    ed = ElfEditor(elf)
    ed.set_section_align(elf.section(".data").index, 4)
    c.check("set_section_align", Elf.read(ed.to_bytes()).section(".data").align, 4)
    local = next(s for s in elf.symbols if s.name == "local_fn")
    ed.set_symbol_info(local.index, (STB_GLOBAL << 4) | STT_FUNC)
    ed.rename_symbols([(local.index, "renamed_fn")])
    after = Elf.read(ed.to_bytes())
    sym = after.symbols[local.index]
    c.check("renamed and rebound", (sym.name, sym.bind, sym.value), ("renamed_fn", STB_GLOBAL, 0))
    c.check("other names unchanged", [s.name for s in after.symbols if s.index != local.index],
            [s.name for s in elf.symbols if s.index != local.index])
    for sec in elf.sections[1:]:
        if sec.name not in (".symtab", ".strtab"):
            c.check("%s contents unchanged" % sec.name, after.sections[sec.index].data, sec.data)
    c.expect("the shifted sections keep their alignment",
             all(s.offset % max(s.align, 1) == 0 for s in after.sections if s.offset))
    c.check("relocations still resolve", [(r.symbol_name, r.offset) for r in after.relocs(".text")],
            [("extern_fn", 4), ("table", 0xA)])
    c.check("no-op editor returns the input", ElfEditor(elf).to_bytes(), elf.raw)


def test_dol_round_trip(c):
    blob = (DolBuilder(entry=0x80003100).text(0x80003100, b"\x48\x00\x00\x01" * 4)
            .text(0x80004000, b"\x60\x00\x00\x00" * 2).data(0x80500000, b"abc\0defg")
            .bss(0x80600000, 0x1000).build())
    dol = Dol.read(blob)
    c.check("segments", [(s.name, s.address, s.size) for s in dol.segments],
            [("text0", 0x80003100, 16), ("text1", 0x80004000, 8), ("data0", 0x80500000, 8)])
    c.check("eighteen slots", len(dol.slots), 18)
    c.check("bss/entry", (dol.bss_address, dol.bss_size, dol.entry), (0x80600000, 0x1000, 0x80003100))
    c.check("word", dol.word(0x80004004), 0x60000000)
    c.check("words", dol.words(0x80003100, 2), [0x48000001, 0x48000001])
    c.check("bytes_at inside", dol.bytes_at(0x80500004, 4), b"defg")
    c.check("bytes_at straddling the end is None", dol.bytes_at(0x80500006, 4), None)
    c.check("bytes_from straddling reads on", dol.bytes_from(0x80500006, 2), b"fg")
    c.check("unmapped", (dol.word(0x90000000), dol.section_of(0x90000000)), (None, None))
    c.check("section_of", dol.section_of(0x80004004).name, "text1")
    c.check("text/data ranges", (dol.text_ranges, dol.data_ranges),
            ([(0x80003100, 0x80003110), (0x80004000, 0x80004008)], [(0x80500000, 0x80500008)]))
    c.check("in_text", (dol.in_text(0x80003104), dol.in_text(0x80500000)), (True, False))
    c.check("cstr", dol.cstr(0x80500000), b"abc")
    c.raises("short DOL", DolError, Dol.read, b"\0" * 0x80)
    padded = Dol.read(DolBuilder(pad=0x20).text(0x80003100, b"\0" * 4).build())
    c.check("pad rounds the slot size", padded.segments[0].size, 0x20)


def _uleb(n: int) -> bytes:
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def _sleb(n: int) -> bytes:
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        done = (n == 0 and not b & 0x40) or (n == -1 and b & 0x40)
        out.append(b | (0 if done else 0x80))
        if done:
            return bytes(out)


def test_dwarf_local_slots(c):
    c.check("uleb", dwarf.uleb(_uleb(624485), 0), (624485, 3))
    c.check("sleb negative", dwarf.sleb(_sleb(-123456), 0), (-123456, 3))
    c.check("decode fbreg/basereg", dwarf.decode_expr(b"\x91" + _sleb(8) + b"\x93" + _uleb(1) + _sleb(-4)),
            "fbreg(+8) basereg(r1,-4)")
    # abbrevs: 1 = compile unit (children), 2 = subprogram (name, children), 3 = variable (name, location block1)
    abbrev = (_uleb(1) + _uleb(0x11) + b"\x01" + b"\x00\x00"
              + _uleb(2) + _uleb(0x2E) + b"\x01" + _uleb(0x03) + _uleb(0x08) + b"\x00\x00"
              + _uleb(3) + _uleb(0x34) + b"\x00" + _uleb(0x03) + _uleb(0x08) + _uleb(0x02) + _uleb(0x0A)
              + b"\x00\x00" + _uleb(4) + _uleb(0x34) + b"\x00" + _uleb(0x03) + _uleb(0x08) + _uleb(0x02)
              + _uleb(0x06) + b"\x00\x00" + b"\x00")
    body = (_uleb(1) + _uleb(2) + b"fn\0"
            + _uleb(3) + b"i\0" + bytes([2]) + b"\x91" + _sleb(8)
            + _uleb(4) + b"j\0" + b"\0\0\0\0"          # data4 location -> a loclist offset, relocated to 0x0
            + b"\x00" + b"\x00")
    info = struct.pack(">IHIB", 7 + len(body), 2, 0, 4) + body
    loc = struct.pack(">II", 0, 0x10) + struct.pack(">H", 2) + b"\x91" + _sleb(12) + struct.pack(">II", 0, 0)
    off_j = info.index(b"j\0") + 2
    b = ElfBuilder()
    b.section(".debug_info", info, align=1, flags=0).section(".debug_abbrev", abbrev, align=1, flags=0)
    b.section(".debug_loc", loc, align=1, flags=0)
    b.symbol(".debug_loc", ".debug_loc", 0, 0, bind="local", type="section")
    b.reloc(".debug_info", off_j, ".debug_loc", "R_PPC_ADDR32", 0)
    elf = Elf.read(b.build())
    seen = []
    cu = dwarf.compile_unit(elf, on_die=lambda off, code, depth, tag, name: seen.append((code, depth, name)))
    c.check("CU header", (cu.version, cu.abbrev_offset, cu.address_size), (2, 0, 4))
    c.check("walk order", seen, [(1, 0, ""), (2, 1, "fn"), (3, 2, "i"), (4, 2, "j")])
    slots = dwarf.local_slots(elf, "fn")
    c.check("local slots", [(s.name, s.location, s.ranges) for s in slots],
            [("i", "fbreg(+8)", ()), ("j", "loclist@0x0", ((0, 0x10, "fbreg(+12)"),))])
    c.raises("no .debug_info", dwarf.DwarfError, dwarf.local_slots, Elf.read(ElfBuilder().build()), "fn")


def test_tokenizer_three_shapes(c):
    t = objdump.tokenize
    c.check("objdump section", t("Disassembly of section .text:"), objdump.Line("section", name=".text"))
    c.check("objdump label", t("80004000 <main>:"), objdump.Line("label", 0x80004000, name="main"))
    ins = t("80004000:\t94 21 ff f0 \tstwu    r1,-16(r1)")
    c.check("objdump insn with raw bytes", (ins.kind, ins.address, ins.mnemonic, ins.operands, ins.raw),
            ("insn", 0x80004000, "stwu", "r1,-16(r1)", "94 21 ff f0"))
    ins = t("   c:\tbl      0 <fn>")
    c.check("object insn, no raw bytes", (ins.address, ins.mnemonic, ins.operands, ins.indented),
            (0xC, "bl", "0 <fn>", True))
    c.check("record form keeps its dot", t("  10:\trlwinm. r0,r3,0,31,31").mnemonic, "rlwinm.")
    c.check("body is the whole text, raw column included", t("80004000:\t94 21 ff f0 \tstwu    r1,-16(r1)").body,
            "94 21 ff f0 \tstwu    r1,-16(r1)")
    r = t("\t\t\t2: R_PPC_ADDR16_HA\tlbl_80500000")
    c.check("objdump -r relocation", (r.kind, r.address, r.reloc, r.name),
            ("reloc", 2, "R_PPC_ADDR16_HA", "lbl_80500000"))
    d = t("/* 80004000 00000100  94 21 FF F0 */\tstwu r1, -0x10(r1)")
    c.check("dtk insn", (d.kind, d.address, d.mnemonic, d.operands, d.raw),
            ("insn", 0x80004000, "stwu", "r1, -0x10(r1)", "00000100  94 21 FF F0"))
    h = t("# .text:0x10 | 0x80004010 | size: 0x20")
    c.check("dtk header", (h.kind, h.name, h.offset, h.address), ("header", ".text", 0x10, 0x80004010))
    c.check("dtk fn / endfn", (t(".fn main, global").name, t(".endfn main").kind), ("main", "endfn"))
    c.check("noise is None", [t(""), t("foo.o:     file format elf32-powerpc"), t("\t...")], [None, None, None])
    c.check("split_text", objdump.split_text("li r3,1"), ("li", "r3,1"))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
