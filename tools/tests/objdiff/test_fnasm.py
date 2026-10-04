"""fnasm: one function out of an objdump listing, relocations folded into the operands, two sides paired by row."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import fnasm

TIER = "fixture"

#: What `objdump -dr -j .text` prints for a two-function object: `g` at +0x0 (two insns), `f` at +0x8.
LISTING = """
x.o:     file format elf32-powerpc


Disassembly of section .text:

00000000 <g>:
   0:\t38 60 00 00 \tli      r3,0
   4:\t4e 80 00 20 \tblr

00000008 <f>:
   8:\t94 21 ff f0 \tstwu    r1,-16(r1)
   c:\t48 00 00 01 \tbl      c <f+0x4>
\t\t\tc: R_PPC_REL24\tmemset
  10:\t3c 60 00 00 \tlis     r3,0
\t\t\t12: R_PPC_ADDR16_HA\ttable
  14:\t38 63 00 00 \taddi    r3,r3,0
\t\t\t16: R_PPC_ADDR16_LO\ttable
  18:\t80 6d 00 00 \tlwz     r3,0(r13)
\t\t\t1a: R_PPC_EMB_SDA21\tcounter
  1c:\t41 82 00 08 \tbeq     24 <f+0x1c>
  20:\t4e 80 00 20 \tblr
"""
OURS = LISTING.replace("41 82 00 08 \tbeq     24 <f+0x1c>", "40 82 00 08 \tbne     24 <f+0x1c>")


def obj(names=(("g", 0, 8), ("f", 8, 0x1C))):
    b = ElfBuilder().section(".text", b"\x00" * 0x24)
    for name, value, size in names:
        b.symbol(name, ".text", value, size, type="func")
    return b.build()


def test_listing(c):
    rows = fnasm.parse_listing(LISTING, ".text", 8, 0x24)
    c.check("only the function's rows, offsets from its start", [r["offset"] for r in rows],
            [0, 4, 8, 0xC, 0x10, 0x14, 0x18])
    c.check("a relocated branch reads as its symbol", rows[1]["text"], "bl       memset")
    c.check("an @ha/@l reloc at insn+2 belongs to that insn and spells the operand",
            (rows[2]["text"], rows[3]["text"]), ("lis      r3,table@ha", "addi     r3,r3,table@l"))
    c.check("an sda21 displacement spells sym@sda21(r13)", rows[4]["text"], "lwz      r3,counter@sda21(r13)")
    c.check("a local branch reads as its offset from the function start", rows[5]["text"], "beq      +0x1C")
    c.check("the reloc rows are kept as data", rows[2]["relocs"], [{"type": "R_PPC_ADDR16_HA", "symbol": "table"}])
    p = fnasm.pair(rows, fnasm.parse_listing(OURS, ".text", 8, 0x24))
    c.check("pair: the first divergence is the changed row", (p["same"], p["first_divergence"]), (6, 5))
    c.check("pair: a row only one side has is < or >",
            "".join(m for m, _a, _b in fnasm.pair(rows, rows[:5])["rows"]), "     <<")
    c.check("a branch target is relative to the function, so the same code at another offset compares equal",
            fnasm.render_operands("beq", "124 <f+0x1c>", [], 0x108), fnasm.render_operands("beq", "24 <f+0x1c>", [], 8))


def test_cli(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("demo/unit.cpp", ranges={".text": (0x80004000, 0x80004024)})
        tree.add_symbol("g", ".text", 0x80004000, 8)
        tree.add_symbol("f", ".text", 0x80004008, 0x1C)
        tree.add_object("demo/unit.o", obj(), side="obj")
        tree.add_object("demo/unit.o", obj(), side="src")
        calls = []

        def runner(path, section):
            calls.append((pathlib.Path(path).parent.name, section))
            return LISTING if pathlib.Path(path).parent.parent.name == "obj" else OURS

        def run(*argv):
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc = fnasm.main(list(argv), runner=runner, root=str(tree.root))
            return rc, out.getvalue()

        rc, out = run("-u", "demo/unit", "f")
        c.check("both sides side by side, exit 0", (rc, "6 of 7 rows the same, first divergence row 5" in out),
                (0, True))
        c.check("... with the marker on the changed row", any(l.rstrip().endswith("bne      +0x1C") and " | " in l
                                                               for l in out.splitlines()), True)
        rc, out = run("0x80004010", "--json")
        got = json.loads(out)
        c.check("an address resolves through the map, the unit through splits.txt",
                (rc, got["symbol"], got["resolved"]["offset"], sorted(got["sides"])), (0, "f", 8, ["ours", "target"]))
        c.check("... the JSON carries the pairing", (got["pair"]["first_divergence"], got["pair"]["markers"]),
                (5, "     | "))
        c.check("objects are named relative to the tree", got["sides"]["target"]["object"], "build/RMHE08/obj/demo/unit.o")
        rc, out = run("-u", "demo/unit", "g", "--side", "target")
        c.check("--side target reads one object", (rc, out.count("\n  "), "li       r3,0" in out), (0, 2, True))
        rc, out = run("-u", "demo/unit", "nosuch")
        c.check("a symbol neither object defines is exit 2 with the reason", (rc, "defines no code symbol" in out),
                (2, True))
        rc, _out = run("0x90000000")
        c.check("an address no map row covers is exit 2", rc, 2)
        rc, out = run("--object", str(tree.path("build/RMHE08/obj/demo/unit.o")), "g")
        c.check("--object reads an arbitrary object", (rc, "blr" in out), (0, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
