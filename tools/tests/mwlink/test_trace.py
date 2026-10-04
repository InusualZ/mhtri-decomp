"""mwlink.trace: the input-object reader, the relocation read-back and one object traced end to end on fixtures."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.elf import STB_GLOBAL, STB_LOCAL, STT_FUNC, STT_SECTION
from tools.mwlink.trace import MwObject, build_trace, reloc_field, reloc_name, render_trace
from tools.tests.mwlink.fixtures import A, input_object, linked_elf, object_map

TIER = "fixture"


def test_object_reader(c):
    with tempfile.TemporaryDirectory() as td:
        op = Path(td) / "fixture.o"
        op.write_bytes(input_object())
        obj = MwObject(op)
        c.check("sections, in file order", [s["name"] for s in obj.sections][:4], ["", ".text", ".ctors$10", ".rela.text"])
        c.check("symbols: locals first, then globals", [s["name"] for s in obj.symbols], ["", "", "", "foo", "bar"])
        foo = obj.symbols[3]
        c.check("bind/type from st_info", (foo["bind"], foo["type"]), (STB_GLOBAL, STT_FUNC))
        c.check("a section symbol", (obj.symbols[1]["bind"], obj.symbols[1]["type"]), (STB_LOCAL, STT_SECTION))
        c.check("shndx_name names the defining section", foo["shndx_name"], ".text")
        c.check("one RELA relocation, as the linker reads it",
                [(r["section"], r["target"], r["offset"], r["type"], r["sym_name"]) for r in obj.relocs],
                [(".rela.text", ".text", 0, 1, "foo")])
        c.check("ctor/dtor sections recognised", [s["name"] for s in obj.ctor_dtor_sections()], [".ctors$10"])
        c.check("contents of an allocated section", obj.contents(obj.section(".text")), b"\0\0\0\0")
        (Path(td) / "le.o").write_bytes(b"\x7fELF\x01\x01" + bytes(58))
        c.raises("a little-endian object is refused", ValueError, MwObject, Path(td) / "le.o")


def test_reloc_names(c):
    c.check("a standard type", reloc_name(1), "ADDR32")
    c.check("REL24", reloc_name(10), "REL24")
    c.check("an EABI type is named (lib.binary.elf's table)", reloc_name(109), "EMB_SDA21")
    c.check("a number with no name stays a number", reloc_name(200), "type 200")


def test_reloc_field(c):
    c.check("ADDR32", reloc_field(1, 0x80004000, 0x80004000, 0, 0)[0:3], (True, 0x80004000, 0x80004000))
    c.check("REL24 is (S+A-P) masked to bits 2..25", reloc_field(10, 0x4800004C, 0x8000404C, 0, 0x80004000)[0:3],
            (True, 0x4C, 0x4C))
    c.check("the bl's LK bit survives (the real __register_fragment branch)",
            reloc_field(10, 0x4800004D, 0x80457490, 0, 0x80457444)[1], 0x4C)
    c.check("REL24's field is bits 2..25 only (a misaligned target's low bits are not the field)",
            reloc_field(10, 0x48000000, 0x80004002, 0, 0x80004000)[0:3], (True, 0, 0))
    c.check("ADDR16_HA rounds", reloc_field(6, 0x3C008004, 0x8003F1C8, 0, 0)[0:3], (True, 0x8004, 0x8004))
    c.check("ADDR16_LO", reloc_field(4, 0x3863F1C8, 0x8003F1C8, 0, 0)[0:3], (True, 0xF1C8, 0xF1C8))
    c.check("ADDR16_HI", reloc_field(5, 0x3C008003, 0x8003F1C8, 0, 0)[0:3], (True, 0x8003, 0x8003))
    c.check("SDA disp from _SDA_BASE_", reloc_field(109, 0x800DAEA8, 0x80793CC8, 0, 0, 0x80798E20)[0:3],
            (True, 0xAEA8, 0xAEA8))
    c.check("type 109 is not claimed without a base", reloc_field(109, 0x800DAEA8, 0x80793CC8, 0, 0, None)[0], False)
    c.check("an un-derived type is reported unchecked", reloc_field(40, 0x12345678, 0, 0, 0)[0], False)
    c.check("a .sdata2 symbol's disp is relative to _SDA2_BASE_",
            reloc_field(109, 0x800D8020, 0x80795AC0, 0, 0, 0x8079DAA0)[0:3], (True, 0x8020, 0x8020))
    c.check("... and _SDA_BASE_ for it is the old wrong answer",
            reloc_field(109, 0x800D8020, 0x80795AC0, 0, 0, 0x80798E20)[0:3], (True, 0xCCA0, 0x8020))


def test_trace_end_to_end(c):
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        op = td / "fixture.o"
        op.write_bytes(input_object())
        elf = td / "fixture.elf"
        elf.write_bytes(linked_elf(A))
        rep = build_trace(op, object_map(), elf)
        c.check("kept", rep["kept"], True)
        c.check("MATCH when map, ELF and object agree", (rep["verdict"], rep["problems"]), ("MATCH", []))
        rel = rep["relocations"][0] if rep["relocations"] else {}
        c.check("the relocation is verified from the ELF's word", (rel.get("verdict"), rel.get("actual")), ("applied", A))
        text = [s for s in rep["sections"] if s["name"] == ".text"]
        c.check("the section's bytes are read back", text and text[0].get("bytes_identical"), True)
        c.check("the defined symbol lands where the map says",
                [(s["name"], s["addr"]) for s in rep["symbols"] if s["name"] == "foo"], [("foo", A)])
        lines = render_trace(rep)
        c.contains("the report says KEPT", lines, "KEPT:     YES - 4 map row(s) name this object")
        c.contains("... and the verdict", lines, "VERDICT: MATCH")
        bad = td / "bad.elf"
        bad.write_bytes(linked_elf(A + 4))
        rep = build_trace(op, object_map(), bad)
        c.check("a word the relocation did not write FAILs", rep["verdict"], "FAIL")
        c.expect("... naming what it holds", any("holds" in p for p in rep["problems"]), rep["problems"])
        rep = build_trace(op, object_map("other.o"), elf, rsp_path=None)
        c.check("an object no row names is DROPPED", rep["verdict"], "DROPPED")
        c.contains("... dead-stripped, not traced", rep["why"], "dead-stripped")
        rsp = td / "link.rsp"
        rsp.write_text("build/RMHE08/obj/other.o\n")
        rep = build_trace(op, object_map("other.o"), elf, rsp_path=rsp)
        c.contains("an object not in the response file is not an input at all", rep["why"], "not an input")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
