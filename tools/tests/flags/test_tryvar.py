"""tryvar: marker and permutation variant files, every variant compiled as a probe by a stub compiler and ranked by
the symbol's score and first divergence, the real source left byte-identical."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os

from tools.lib import testing
from tools.lib import units
from tools.lib.binary.elf import Elf
from tools.flags import tryvar

TIER = "fixture"

SOURCE = "int a;\nint b;\nint c;\nint f(void) { return a + b + c; }\n"

#: A stand-in for MWCC: `-o DIR -c SRC` writes `DIR/<stem>.o`, an ELF whose `.text` holds one word per source line and
#: whose `.note.src` carries the source, so the stub scorer can read what was compiled. `BROKEN` fails the compile.
STUB_CC = r'''
import os, sys
sys.path.insert(0, os.environ["TRYVAR_TOOLS_ROOT"])
from tools.lib.binary.build import ElfBuilder
args = sys.argv[1:]
out, src = args[args.index("-o") + 1], args[args.index("-c") + 1]
text = open(src, encoding="utf-8").read()
if "BROKEN" in text:
    print("### mwcceppc.exe Compiler:\nError: syntax error")
    sys.exit(1)
size = 4 * len(text.splitlines())
blob = (ElfBuilder().section(".text", b"\0" * size).section(".note.src", text.encode())
        .symbol("f", ".text", 0, size, type="func").build())
with open(os.path.join(out, os.path.splitext(os.path.basename(src))[0] + ".o"), "wb") as fh:
    fh.write(blob)
'''


def order_of(obj):
    """The declaration order a probe object was compiled from (`a`, `b`, `c` as they appear)."""
    text = Elf.read(obj).section_bytes(".note.src").decode()
    return [line[4] for line in text.splitlines() if line.startswith("int ") and line.endswith(";") and len(line) == 6]


def score(obj, _target):
    """`c, a, b` is the retail order: f is 100 % there, 80 % with `c` first, 50 % otherwise."""
    order = order_of(obj)
    pct = 100.0 if order == ["c", "a", "b"] else (80.0 if order[:1] == ["c"] else 50.0)
    return {"f": (pct, 16, 16)}


def diverge(obj, _target, _symbol):
    """The first divergence is later the more declarations sit where retail has them."""
    return sum(1 for got, want in zip(order_of(obj), ["c", "a", "b"]) if got == want)


def test_loaders(c):
    marker = "//@@ old\nint a;\nint b;\n//@@ variant swap\nint b;\nint a;\n//@@ variant drop-b\nint a;\n"
    c.check("marker file: each variant replaces the latest old block",
            tryvar.parse_marker_variants(marker),
            [("swap", [("int a;\nint b;", "int b;\nint a;")]), ("drop-b", [("int a;\nint b;", "int a;")])])
    c.raises("a variant before any old block is refused", SystemExit, tryvar.parse_marker_variants,
             "//@@ variant x\nint a;\n")
    perms = tryvar.permutation_variants("//@@ permute\n//@@ item\nint a;\n//@@ item\nint b;\n//@@ item\nint c;\n")
    c.check("permute: every order but the identity", [n for n, _r in perms],
            ["order-0,2,1", "order-1,0,2", "order-1,2,0", "order-2,0,1", "order-2,1,0"])
    c.check("... each rewriting the contiguous run", perms[3][1], [("int a;\nint b;\nint c;\n", "int c;\nint a;\nint b;\n")])
    c.raises("permute: more orders than --max-variants is refused", SystemExit, tryvar.permutation_variants,
             "//@@ item\na\n//@@ item\nb\n//@@ item\nc\n//@@ item\nd\n", 10)


def test_divergence_of(c):
    left = {"symbols": [{"name": "g", "instructions": [{"diff_kind": "DIFF_REPLACE"}]},
                        {"name": "f", "instructions": [{"instruction": {}}, {"instruction": {}},
                                                       {"diff_kind": "DIFF_DELETE"}]}]}
    c.check("the first target instruction with a diff_kind (objdiff diff's left.symbols)",
            tryvar.divergence_of({"left": left}, "f"), 2)
    c.check("none diverging is None, an absent symbol '?'",
            (tryvar.divergence_of({"left": {"symbols": [{"name": "f", "instructions": [{}]}]}}, "f"),
             tryvar.divergence_of({"left": left}, "h")), (None, "?"))


def test_variants_compile_rank_and_restore(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("demo/unit.cpp", source=SOURCE, ranges={".text": (0x80004000, 0x80004010)})
        tree.write("build/RMHE08/obj/demo/unit.o", b"\x7fELF")
        stub = tree.write("stub_cc.py", STUB_CC)
        unit = units.Unit.resolve("demo/unit", str(tree.root))
        before = pathlib.Path(unit.source).read_bytes()
        tokens = [sys.executable, str(stub), "-o", "unused", "-c", unit.source]
        os.environ["TRYVAR_TOOLS_ROOT"] = str(testing.LIVE_ROOT)
        try:
            perms = tryvar.permutation_variants("//@@ item\nint a;\n//@@ item\nint b;\n//@@ item\nint c;\n")
            extra = tryvar.parse_marker_variants("//@@ old\nint c;\n//@@ variant broken\nBROKEN\n"
                                                 "//@@ old\nint z;\n//@@ variant absent\nint y;\n")
            lines = []
            results = tryvar.try_all(unit, tokens, perms + extra, symbol="f", score=score, diverge=diverge,
                                     as_is=True, out=lines.append)
        finally:
            del os.environ["TRYVAR_TOOLS_ROOT"]
        by = {r["name"]: r for r in results}
        c.check("the as-is source is tried first", results[0]["name"], tryvar.AS_IS)
        c.check("a variant that does not compile is compile-failed, one that does not apply is skip",
                (by["broken"]["status"], by["absent"]["status"]), ("compile-failed", "skip"))
        ranked = tryvar.rank(results, "f")
        c.check("ranked by the symbol's score: the retail order wins", ranked[0]["name"], "order-2,0,1")
        c.check("... then by first divergence among equal scores (later is better)",
                [r["name"] for r in ranked[1:3]], ["order-2,1,0", "order-0,2,1"])
        c.check("a full match ranks with no divergence", ranked[0]["first_divergence"], None)
        c.check("every scored variant is in the ranking", len(ranked), 6)
        c.check("the old one-line verdict is printed per variant", lines[0].startswith(tryvar.AS_IS), True)
        c.check("the real source is byte-identical afterwards", pathlib.Path(unit.source).read_bytes(), before)
        c.check("no probe source is left beside it", sorted(os.listdir(os.path.dirname(unit.source))), ["unit.cpp"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
