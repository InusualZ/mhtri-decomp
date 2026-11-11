"""extabdiff's parsers and pairing on objdump text fixtures: extab record symbols, extabindex relocations paired per
function, the section dump bytes, and the differing-record report (changed, missing, identical)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.units import extabdiff

TIER = "fixture"

SYMTAB = """
00000000 l     O extab\t00000008 @etb_1
00000008 l     O extab\t00000010 @etb_2
00000000 g     F .text\t00000040 fa
"""
RELOCS = """
00000000 R_PPC_ADDR32      fa
00000008 R_PPC_ADDR32      @etb_1
0000000c R_PPC_ADDR32      fb
00000014 R_PPC_ADDR32      @etb_2
"""
DUMP = """
Contents of section extab:
 0000 01020304 05060708 090a0b0c 0d0e0f10  ................
 0010 11121314                             ....
"""


def test_parsers(c):
    sym = extabdiff.parse_symbols(SYMTAB)
    c.check("extab symbols only", sym, {"@etb_1": (0, 8), "@etb_2": (8, 16)})
    ents = extabdiff.parse_index(RELOCS)
    c.check("index entries by 12-byte slot", ents, {0: {0: "fa", 8: "@etb_1"}, 1: {0: "fb", 8: "@etb_2"}})
    raw = extabdiff.parse_dump(DUMP)
    c.check("dump bytes", len(raw), 20)
    rec = extabdiff.pair_records(sym, ents, raw)
    c.check("records paired per function", {k: len(v) for k, v in rec.items()}, {"fa": 8, "fb": 12})


def test_differing(c):
    t = {"fa": b"12345678", "fb": b"abcd", "fc": b"zz"}
    o = {"fa": b"12345678", "fb": b"abcdef"}
    c.check("changed and missing records", extabdiff.differing(t, o), [("fb", 4, 6), ("fc", 2, None)])
    c.check("identical", extabdiff.differing(t, t), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
