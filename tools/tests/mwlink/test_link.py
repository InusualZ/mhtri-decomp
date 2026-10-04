"""mwlink.link, .align and .records on fixtures: unit names of link inputs, the claimed-start reader, the record table
and the object-header reader the record proof cross-checks with."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.mwlink.align import _splits_starts
from tools.mwlink.link import unit_of_object
from tools.mwlink.records import FILE_RECORD_FIELDS, _object_header_bits
from tools.tests.mwlink.fixtures import input_object

TIER = "fixture"


def test_unit_of_object(c):
    c.check("a src/ input", unit_of_object("build/RMHE08/src/Network/NetworkWiiMediator.o"), "Network/NetworkWiiMediator")
    c.check("an obj/ input with backslashes", unit_of_object("build\\RMHE08\\obj\\Runtime.PPCEABI.H\\__start.o"),
            "Runtime.PPCEABI.H/__start")
    c.check("a flat name", unit_of_object("build/RMHE08/obj/auto_00_80004380_init.o"), "auto_00_80004380_init")
    c.check("an absolute path", unit_of_object("C:/r/build/RMHE08/src/DWCi/dwc_error.o"), "DWCi/dwc_error")
    c.check("not a build path: the stem", unit_of_object("somewhere/x.o"), "x")
    c.check("not an object: the name", unit_of_object("lib/runtime.a"), "runtime.a")


def test_splits_starts(c):
    with tempfile.TemporaryDirectory() as td:
        sp = Path(td) / "splits.txt"
        sp.write_text("Pl/fn_8023C2D0.cpp:\n\t.text       start:0x8023C2D0 end:0x80241558\n"
                      "\t.data       start:0x805C34D4 end:0x805C3D20\n\n"
                      "main.cpp:\n\t.text       start:0x8003F200 end:0x80040478\n", encoding="utf-8")
        starts = _splits_starts(sp)
        c.check("the claimed .data start", starts.get("Pl/fn_8023C2D0.cpp", {}).get(".data"), 0x805C34D4)
        c.check("a second unit", starts.get("main.cpp", {}).get(".text"), 0x8003F200)
        c.check("a missing file reads as no claims", _splits_starts(Path(td) / "none.txt"), {})


def test_record_table(c):
    stride, prev_end, ordered = 0x2C, 0, True
    for off, size, _name, _meaning, _ev in FILE_RECORD_FIELDS:
        ordered = ordered and off >= prev_end
        prev_end = off + size
    c.expect("the record fields are ordered and do not overlap", ordered)
    c.expect("they fit the 0x2c stride the parser's imul states", prev_end <= stride, hex(prev_end))
    c.check("the last field is the comment version", FILE_RECORD_FIELDS[-1][2], "comment_version")


def test_object_header_bits(c):
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "head.o").write_bytes(input_object())
        c.check("e_shnum, and no comment version without a .comment", _object_header_bits(td / "head.o"), (7, None))
        (td / "cw.o").write_bytes(ElfBuilder().section(".text", bytes(4)).symbol("f", ".text").comment(14).build())
        shnum, ver = _object_header_bits(td / "cw.o")
        c.check("the .comment version byte", ver, 14)
        c.check("a missing object is no header, not a crash", _object_header_bits(td / "nope.o"), (None, None))
        (td / "junk.o").write_bytes(b"not an elf" * 10)
        c.check("a non-ELF is no header", _object_header_bits(td / "junk.o"), (None, None))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
