"""textorder's parsers and comparison on objdump text fixtures: `.text` symbol order, the deferred-inline tags, the
out-of-order report and ours-only names."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.units import textorder

TIER = "fixture"

SYMTAB = """
00000040 g     F .text\t00000010 B
00000000 g     F .text\t00000040 A
00000000 g     O .data\t00000004 table
00000050 l     F .text\t00000010 C
"""
DISASM = """
00000000 <A>:
   0:\t48 00 00 01 \tbl      0 <B>
\t\t\t0: R_PPC_REL24\tB
\t\t\t4: R_PPC_REL24\tOutside
00000040 <B>:
   40:\t4e 80 00 20 \tblr
00000050 <C>:
   50:\t48 00 00 01 \tbl      0 <A>
\t\t\t50: R_PPC_REL24\tA
"""


def test_parse_functions(c):
    c.check(".text functions by address, data symbols skipped", textorder.parse_functions(SYMTAB), ["A", "B", "C"])


def test_deferred_tags(c):
    order, calls = textorder.parse_refs(DISASM)
    c.check("function order", order, ["A", "B", "C"])
    c.check("calls per function", calls, {"A": ["B", "Outside"], "B": [], "C": ["A"]})
    rows = textorder.deferred_tags(order, calls)
    c.check("B is first referenced by the earlier A: deferred; A is referenced only by the later C: real",
            [(r[1], r[3]) for r in rows], [("R", ["B"]), ("D", []), ("R", ["A"])])


def test_compare_order(c):
    bad, extra, common = textorder.compare_order(["A", "B", "C"], ["A", "Z", "C", "B"])
    c.check("ours-only name", extra, ["Z"])
    c.check("common functions in our order", common, ["A", "C", "B"])
    c.check("positions that disagree", bad, [1, 2])
    c.check("same order is clean", textorder.compare_order(["A", "B"], ["A", "B"]), ([], [], ["A", "B"]))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
