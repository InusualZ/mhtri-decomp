"""mwlink.mapfile: the map parser, the generated-symbol listing, the row classifier, `verify` and the order check."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.mwlink.mapfile import (OutputElf, _map_rows, is_section_row, parse_map, parse_map_symbols,
                                  validate_order_against_map, verify_map)
from tools.tests.mwlink.fixtures import CTORS_MAP, output_elf

TIER = "fixture"
ORDER = [".ctors$00", ".ctors$10", ".ctors", ".ctors$99"]


def test_parse_map(c):
    block = parse_map(CTORS_MAP)[".ctors"]["blocks"][0]
    c.check("map start", block["start"], 0x8056F2C0)
    c.check("map extent is max(offset + size), not the sum", block["extent"], 0x10)
    c.check("map fragments", len(block["fragments"]), 3)
    both = ("\n.text section layout\n  00000000 000008 80004000 00000100  1 .text \tx.o \n"
            "  00000000 000008 80004000 00000100  4 foo \tx.o \n")
    c.check("a symbol row inside its fragment is not counted twice", parse_map(both)[".text"]["blocks"][0]["extent"], 8)
    rows = {r["name"]: r for _s, r in _map_rows(CTORS_MAP)}
    c.check("a row carries the input file it came from", rows[".ctors$10"]["source"], "__init_cpp_exceptions.o")
    c.check("... and its alignment column", rows[".ctors$10"]["flags"], 1)
    ent = ("\n.init section layout\n"
           "  00000514 000000 80004514 00000714    lbl_80004514 (entry of pad_00_80004380_init) \t"
           "auto_00_80004380_init.o \n")
    row = parse_map(ent)[".init"]["blocks"][0]["fragments"]
    c.check("an empty-alignment row keeps its whole name",
            [r["name"] for r in row], ["lbl_80004514 (entry of pad_00_80004380_init)"])
    c.check("... its size and address", [(r["size"], r["addr"]) for r in row], [(0, 0x80004514)])
    c.check("... and its input file", [r["source"] for r in row], ["auto_00_80004380_init.o"])


def test_generated_symbols(c):
    gen = parse_map_symbols("Link map of __start\n\nLinker generated symbols:\n"
                            "                 _f_text 80004000\n                 _f_data 8057c820\n"
                            "             _SDA2_BASE_ 8079daa0\n")
    c.check("the generated-symbol listing", gen, {"_f_text": 0x80004000, "_f_data": 0x8057C820, "_SDA2_BASE_": 0x8079DAA0})
    c.check("rows before the listing are not read", parse_map_symbols("  _f_text 80004000\n"), {})


def test_row_kind(c):
    def row(name, flags):
        return {"name": name, "offset": 0, "size": 1, "addr": 0, "file_off": 0, "source": "x.o", "flags": flags}
    c.expect("a section row: name == section", is_section_row(".text", row(".text", 1)))
    c.expect("a $NN section row", is_section_row(".ctors", row(".ctors$10", 1)))
    c.expect("extab has no leading dot", is_section_row("extab", row("extab", 1)))
    c.expect("a fill row, whatever it prints", is_section_row(".text", row("*fill*", 16)))
    c.expect("a 1-byte label is a symbol even though it prints 1", not is_section_row(".sbss", row("lbl_807947A5", 1)))
    c.expect("a pooled symbol has an inner dot", not is_section_row(".data", row(".data.0", 8)))
    c.expect("an ordinary symbol", not is_section_row(".text", row("fn_8003F200", 4)))
    c.expect("an `(entry of ...)` row is a symbol",
             not is_section_row(".init", row("lbl_80004514 (entry of pad_00_80004380_init)", 0)))


def test_verify(c):
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "x.MAP").write_text(CTORS_MAP)
        (td / "x.elf").write_bytes(output_elf([(".ctors", 0x8056F2C0, bytes(0x10))]))
        rep = verify_map(td / "x.elf", td / "x.MAP")
        c.check("MATCH when the map is the ELF", (rep["status"], rep["problems"]), ("match", []))
        c.check("the compared section", [(s["section"], s["start"], s["size"]) for s in rep["sections_compared"]],
                [(".ctors", 0x8056F2C0, 0x10)])
        (td / "bad.elf").write_bytes(output_elf([(".ctors", 0x80000000, bytes(0x10))]))
        rep = verify_map(td / "bad.elf", td / "x.MAP")
        c.check("a different address FAILs", rep["status"], "fail")
        c.contains("... and names the divergence", rep["first_divergence"] or "", "!= ELF addr 0x80000000")
        (td / "small.elf").write_bytes(output_elf([(".ctors", 0x8056F2C0, bytes(8))]))
        rep = verify_map(td / "small.elf", td / "x.MAP")
        c.check("a size mismatch FAILs", rep["status"], "fail")
        c.contains("... naming the coverage", rep["first_divergence"] or "", "map covers 0x10 bytes, ELF size 0x8")
        (td / "none.elf").write_bytes(output_elf([(".text", 0x80004000, bytes(4))]))
        rep = verify_map(td / "none.elf", td / "x.MAP")
        c.contains("a section the ELF lacks is named", rep["problems"], "section '.ctors' is in the map but not in the ELF")
        (td / "not.elf").write_bytes(b"MZ" + bytes(62))
        c.raises("a non-ELF is refused", ValueError, OutputElf, td / "not.elf")


def test_output_elf_symbols(c):
    with tempfile.TemporaryDirectory() as td:
        b = ElfBuilder().section(".text", bytes(8), addr=0x80004000)
        b.symbol("_f_text", ".text", value=0x80004000)
        b.symbol("dup", ".text", value=0x80004000, bind="local")
        b.symbol("dup", ".text", value=0x80004004, bind="local")
        path = Path(td) / "o.elf"
        b.write(path)
        syms = OutputElf(path).symbols()
        c.check("the ELF's own table: every value of a repeated name", syms.get("dup"), [0x80004000, 0x80004004])
        c.check("a named global", syms.get("_f_text"), [0x80004000])
        c.check("the null and section symbols are not names", "" in syms, False)


def test_order_check(c):
    with tempfile.TemporaryDirectory() as td:
        good = Path(td) / "good.MAP"
        good.write_text(CTORS_MAP)
        c.expect("an in-order map is accepted", validate_order_against_map(ORDER, good)["ok"])
        swapped = CTORS_MAP.replace(
            "  00000000 000004 8056f2c0 0056b4c0  1 .ctors$10 \t__init_cpp_exceptions.o \n"
            "  00000004 00000c 8056f2c4 0056b4c4  1 .ctors \tfoo.o \n",
            "  00000000 00000c 8056f2c0 0056b4c0  1 .ctors \tfoo.o \n"
            "  0000000c 000004 8056f2cc 0056b4cc  1 .ctors$10 \t__init_cpp_exceptions.o \n")
        c.expect("the swap fixture differs", swapped != CTORS_MAP)
        bad = Path(td) / "bad.MAP"
        bad.write_text(swapped)
        rep = validate_order_against_map(ORDER, bad)
        c.expect("the Row 46 swap is rejected", not rep["ok"])
        c.contains("... loudly", rep["verdict"], "FAIL")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
