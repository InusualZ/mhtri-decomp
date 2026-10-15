"""flipcheck's relocation-place check (`lib.objcompare.reloc_places`/`reloc_place_diffs`, `flipcheck.relocation_place_problems`)
on objects shaped like g3d/g3d_resvtx: two accessors in `.text` each load one of two identical `"ref"` copies in `.sdata`.
The masked bytes are equal whichever copy a relocation names, so only the place it resolves to tells a swap apart."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import RELOC_NUMBERS
from tools.units import flipcheck

TIER = "fixture"

UNIT = "g3d/g3d_resvtx"
SDA21 = RELOC_NUMBERS["R_PPC_EMB_SDA21"]
TEXT, SDATA = 0x80077000, 0x80793A10
#: two `addi r4, r13, 0` loads (the SDA21 field is zero in both objects) and two `"ref"` copies
CODE = b"\x38\x8d\x00\x00\x38\x8d\x00\x00"
STRINGS = b"ref\0ref\0"


def resvtx(ours_order=("@1", "@2"), target_order=("lbl_80793A10", "lbl_80793A14"), ours_type="R_PPC_EMB_SDA21",
           extern=None):
    """`(ours, target)` objects: each `.text` load at +0x0/+0x4 relocates the string named by `*_order` (ours' `@1` sits
    at `.sdata`+0x0, `@2` at +0x4; the target's `lbl_` names the same way). `extern` adds a third load of an undefined
    global name to each side (`(ours name, target name)`)."""
    def build(names, strings, typ, ext):
        b = ElfBuilder()
        b.section(".text", CODE + (b"\x38\x8d\x00\x00" if ext else b""), align=4)
        b.section(".sdata", STRINGS, align=4)
        for name, off in strings:
            b.symbol(name, ".sdata", off, 4, bind="local", type="object")
        for i, name in enumerate(names):
            b.reloc(".text", 4 * i + 2, name, typ)
        if ext:
            b.symbol(ext)
            b.reloc(".text", 8 + 2, ext, "R_PPC_EMB_SDA21")
        return b.build()
    ours = build(ours_order, [("@1", 0), ("@2", 4)], ours_type, extern and extern[0])
    target = build(target_order, [("lbl_80793A10", 0), ("lbl_80793A14", 4)], "R_PPC_EMB_SDA21", extern and extern[1])
    return ours, target


def tree(t, ours, target, map_rows=""):
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, target)
    size = len(objcompare.section_data(ours, ".text"))
    t.write("config/RMHE08/splits.txt", "%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n\t.sdata      start:0x%08X "
            "end:0x%08X\n" % (UNIT, TEXT, TEXT + size, SDATA, SDATA + 8))
    t.write("config/RMHE08/symbols.txt", map_rows)
    flipcheck._RANGES.clear()
    flipcheck._MAP_ADDRESSES.clear()
    flipcheck.set_root(str(t.root))
    return {".text": (size, 2), ".sdata": (8, 2)}


def run(claim):
    """`flipcheck.check` with the live tree's seam and pool diagnoses stubbed out (they explain a differing section)."""
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, claim, None)
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


def reloc_lines(problems):
    return [p for p in problems if "different place or type" in p]


def test_swapped_copies_refused(c):
    """Mutation: before the check, the swapped object read READY (equal bytes, equal sizes, no undefined name)."""
    with testing.FixtureTree() as t:
        ours, target = resvtx()
        problems, _n = run(tree(t, ours, target))
        c.check("the same copies under other names (`@1` vs `lbl_80793A10`): ready", problems, [])
    with testing.FixtureTree() as t:
        ours, target = resvtx(ours_order=("@2", "@1"))
        problems, _n = run(tree(t, ours, target))
        c.check("each load naming the other identical copy: bytes equal, so the relocation line is the only refusal",
                problems, [".text: 2 relocation(s) resolve to a different place or type than the target's at the same "
                           "offset, so the link writes other addresses even where the bytes are equal - +0x2 ours "
                           "0x80793A14, target 0x80793A10; +0x6 ours 0x80793A10, target 0x80793A14"])
    with testing.FixtureTree() as t:
        ours, target = resvtx(ours_type="R_PPC_ADDR16_LO")
        lines = reloc_lines(run(tree(t, ours, target))[0])
        c.check("the same place under another relocation type is refused and names both types",
                (len(lines), "(R_PPC_ADDR16_LO)" in lines[0], "(R_PPC_EMB_SDA21)" in lines[0]), (1, True, True))


def test_external_names_by_address(c):
    """An undefined name resolves through the map: two spellings of one address agree, two addresses do not."""
    rows = ("ResName = .sdata:0x80793B00; // type:object size:0x4\n"
            "ResNameAlias = .sdata:0x80793B00; // type:object size:0x4\n"
            "Other = .sdata:0x80793B08; // type:object size:0x4\n")
    with testing.FixtureTree() as t:
        ours, target = resvtx(extern=("ResNameAlias", "ResName"))
        c.check("two map names at one address: ready", run(tree(t, ours, target, rows))[0], [])
    with testing.FixtureTree() as t:
        ours, target = resvtx(extern=("Other", "ResName"))
        lines = reloc_lines(run(tree(t, ours, target, rows))[0])
        c.check("a name at another map address: refused at +0xA",
                (len(lines), "+0xA ours 0x80793B08, target 0x80793B00" in lines[0] if lines else False), (1, True))
    with testing.FixtureTree() as t:
        ours, target = resvtx(extern=("Unmapped", "Unmapped"))
        c.check("an unmapped name compares by name: the same name agrees", run(tree(t, ours, target))[0], [])


def test_places_follow_the_strip(c):
    """Our offsets are mapped through the dead-strip layout (`strip_moves`): a load after a stripped function is compared
    at the offset the link gives it, and a relocation inside the stripped function is not compared."""
    b = ElfBuilder()
    b.section(".text", b"\x38\x8d\x00\x00" * 3, align=4)
    b.section(".sdata", STRINGS, align=4)
    b.symbol("dead", ".text", 0, 4, type="func", comment=(4, 0, 0))
    b.symbol("live", ".text", 4, 8, type="func", comment=(4, 0, 0))
    b.symbol("@1", ".sdata", 0, 4, bind="local", type="object")
    b.symbol("@2", ".sdata", 4, 4, bind="local", type="object")
    for off, name in ((2, "@1"), (6, "@1"), (10, "@2")):
        b.reloc(".text", off, name, "R_PPC_EMB_SDA21")
    b.comment()
    obj = b.build()
    elf = objcompare.load(obj)
    laid = objcompare.strip_moves(obj, ".text", [s for s in elf.symbols if s.name == "dead"])
    c.check("strip_moves: `live` moves from +0x4 to +0x0", laid[1], [(4, 12, 0)])
    c.check("strip_section is apply_moves of that layout", objcompare.strip_section(
        obj, ".text", [s for s in elf.symbols if s.name == "dead"]), objcompare.apply_moves(*laid))
    places = objcompare.reloc_places(obj, {".text": TEXT, ".sdata": SDATA}, moves={".text": laid[1]})
    c.check("the stripped function's relocation is dropped, the kept ones move by 4",
            places[".text"], {2: [(SDA21, ("addr", SDATA))], 6: [(SDA21, ("addr", SDATA + 4))]})
    c.check("reloc_place_diffs: an offset only one side relocates is not this question",
            objcompare.reloc_place_diffs({2: [(SDA21, ("addr", 1))]}, {6: [(SDA21, ("addr", 1))]}), [])
    c.check("reloc_place_diffs: a place None (a local inside a stripped span) is not compared",
            objcompare.reloc_place_diffs({2: [(SDA21, None)]}, {2: [(SDA21, ("addr", 1))]}), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
