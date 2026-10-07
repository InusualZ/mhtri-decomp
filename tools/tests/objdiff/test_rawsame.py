"""rawsame: raw words and relocation types of every 100 % function, target against ours, relocated operand bits masked."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
from tools.lib import rawsame, testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import rawsame as tool

TIER = "fixture"

LWZ_R13 = 0x806D0000      # lwz r3, sym@sda21(r13)
LWZ_R0 = 0x80600000       # the same with rA = 0
BLR = 0x4E800020
LIS_R3 = 0x3C600000
LIS_R4 = 0x3C800000


def make(funcs):
    """An object of one `.text`: `funcs` is `[(name, [words], [(reloc offset, type)])]` laid out back to back."""
    blob, at, syms, rels = b"", 0, [], []
    for name, words, relocs in funcs:
        syms.append((name, at, 4 * len(words)))
        rels += [(at + off, typ) for off, typ in relocs]
        blob += b"".join(w.to_bytes(4, "big") for w in words)
        at += 4 * len(words)
    b = ElfBuilder().section(".text", blob).symbol("ext", None)
    for name, value, size in syms:
        b.symbol(name, ".text", value, size, type="func")
    for off, typ in rels:
        b.reloc(".text", off, "ext", typ)
    return b


TARGET = [
    ("f_same", [0x48000001, BLR], [(0, "R_PPC_REL24")]),
    ("f_sda", [LWZ_R13, BLR], [(0, "R_PPC_EMB_SDA21")]),
    ("f_lis", [LIS_R3, BLR], [(2, "R_PPC_ADDR16_HA")]),
    ("f_type", [0x48000001, BLR], [(0, "R_PPC_REL24")]),
    ("f_size", [BLR, BLR], []),
    ("f_gone", [BLR, BLR], []),
    ("f_plain", [0x38600001, BLR], []),
]
OURS = [
    ("f_same", [0x4BFFFFFD, BLR], [(0, "R_PPC_REL24")]),             # another LI field: masked, so equal
    ("f_sda", [LWZ_R0, BLR], [(0, "R_PPC_EMB_SDA21")]),              # rA differs beside the SDA21
    ("f_lis", [LIS_R4, BLR], [(2, "R_PPC_ADDR16_HA")]),              # rD differs beside the HA
    ("f_type", [0x48000001, BLR], [(0, "R_PPC_ADDR24")]),            # another relocation type
    ("f_size", [BLR, BLR, BLR], []),
    ("f_plain", [0x38600002, BLR], []),                              # no relocation: an immediate differs
]


def test_compare(c):
    rows = {r.name: r for r in rawsame.compare(make(TARGET).build(), make(OURS).build(), [n for n, _w, _r in TARGET])}
    c.check("a relocated LI field that differs is masked: same", rows["f_same"].status, "same")
    c.check("an SDA21 whose rA differs shows (only the low 16 bits are masked)",
            (rows["f_sda"].status, rows["f_sda"].diffs), ("bytes", [(0, "word", "806D0000", "80600000")]))
    c.check("a register field beside an ADDR16_HA shows (the halfword reloc owns the low 16 bits only)",
            (rows["f_lis"].status, rows["f_lis"].diffs), ("bytes", [(0, "word", "3C600000", "3C800000")]))
    c.check("a different relocation type at the same word shows",
            rows["f_type"].diffs, [(0, "reloc", "R_PPC_REL24", "R_PPC_ADDR24")])
    c.check("a size difference is its own status", (rows["f_size"].status, rows["f_size"].target_size,
                                                    rows["f_size"].ours_size), ("size", 8, 12))
    c.check("a function ours lacks", rows["f_gone"].status, "missing-ours")
    c.check("an unrelocated word is compared in full", rows["f_plain"].diffs, [(0, "word", "38600001", "38600002")])
    c.check("the row line names the first differences",
            rows["f_sda"].line("u"), "u: f_sda: +0x0 word 806D0000 vs 80600000")
    c.check("only the differing functions are rows", sorted(n for n, r in rows.items() if r.differs),
            ["f_gone", "f_lis", "f_plain", "f_sda", "f_size", "f_type"])


def test_check_unit(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("demo/unit.cpp", source="int f;\n", ranges={".text": (0x80004000, 0x80004040)})
        tree.add_object("demo/unit.o", make(TARGET), side="obj")
        tree.add_object("demo/unit.o", make(OURS), side="src")
        tree.set_report({"main/demo/unit": {"functions": {"f_same": (8, 100.0), "f_sda": (8, 100.0),
                                                           "f_lis": (8, 100.0), "f_plain": (8, 60.0),
                                                           "f_gone": (8, None)}}})
        root = str(tree.root)
        rep = str(tree.build_dir / "report.json")
        rec = tool.check_unit("demo/unit", root, rep)
        c.check("only the functions the report scores 100 % are judged", rec["checked"], 3)
        c.check("... and the two with raw differences are rows", sorted(r["name"] for r in rec["rows"]),
                ["f_lis", "f_sda"])
        low = tool.check_unit("demo/unit", root, rep, minimum=50.0)
        c.check("--min-percent widens the set", (low["checked"], low["differing"]), (4, 3))
        c.check("an unreadable report is an error, not a clean unit",
                tool.check_unit("demo/unit", root, str(tree.build_dir / "nope.json"))["error"] is not None, True)
        tree.add_unit("demo/other.cpp", source="int g;\n", ranges={".text": (0x80004040, 0x80004080)})
        c.check("a unit with no objects is an error naming the object",
                "does not exist" in (tool.check_unit("demo/other", root, rep)["error"] or ""), True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
