"""flipcheck's positioned-symbol check against a target pool the map names (`lib.objcompare.unnamed_spellings`,
`flipcheck.positioned_symbol_problems`), on objects shaped like ef/ef_postfield: the target's `.sdata2` words carry map
names (`ef_pf_one`, `ef_pf_two`), our object emits the same bytes as MWCC's local `@NNN` literals, and `.text` loads
them by SDA21. The name is satisfied only when the bytes, the offset and the binding agree and no other link input
references it; a moved, renamed-global, differing or referenced word is still refused."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "ef/ef_pool"
TEXT, SDATA2 = 0x800B0000, 0x807960F0
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
#: two `lfs f1, 0(r2)` loads (the SDA21 field is zero in both objects) and the pool 1.0f, 2.0f
CODE = b"\xc0\x22\x00\x00\xc0\x22\x00\x00"
POOL = b"\x3f\x80\x00\x00\x40\x00\x00\x00"
MAP = ("ef_pf_one = .sdata2:0x807960F0; // type:object size:0x4 data:float\n"
       "ef_pf_two = .sdata2:0x807960F4; // type:object size:0x4 data:float\n")


def build(pool: bytes, symbols, loads, section=".sdata2"):
    """An object: `.text` loading `loads` (one name per word, SDA21) and `section` holding `pool` (NOBITS when `pool` is
    an int) with `(name, offset, bind)` symbols of 4 bytes."""
    b = ElfBuilder()
    b.section(".text", CODE, align=4)
    if isinstance(pool, int):
        b.nobits(section, pool, align=4)
    else:
        b.section(section, pool, align=4)
    for name, off, bind in symbols:
        b.symbol(name, section, off, 4, bind=bind, type="object")
    for i, name in enumerate(loads):
        b.reloc(".text", 4 * i + 2, name, "R_PPC_EMB_SDA21")
    return b.build()


TARGET = build(POOL, [("ef_pf_one", 0, "global"), ("ef_pf_two", 4, "global")], ("ef_pf_one", "ef_pf_two"))
OURS = build(POOL, [("@12", 0, "local"), ("@13", 4, "local")], ("@12", "@13"))


def tree(t, ours, target=TARGET, section=".sdata2"):
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, target)
    t.write("config/RMHE08/splits.txt", "%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n\t%-11s start:0x%08X "
            "end:0x%08X\n" % (UNIT, TEXT, TEXT + len(CODE), section, SDATA2, SDATA2 + len(POOL)))
    t.write("config/RMHE08/symbols.txt", MAP)
    flipcheck._RANGES.clear()
    flipcheck._MAP_ADDRESSES.clear()
    flipcheck.set_root(str(t.root))
    return {".text": (len(CODE), 2), section: (len(POOL), 2)}


def link(other_refs=()):
    """A link context in which the target object references its own pool names and `other.o` references
    `other_refs`."""
    refs = {TARGET_REL: {"ef_pf_one", "ef_pf_two"}, "other.o": set(other_refs)}
    count: dict = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    return {"refs": refs, "ref_count": count, "providers": {"ef_pf_one": {TARGET_REL}, "ef_pf_two": {TARGET_REL}},
            "inputs": list(refs)}


def run(claim, link_ctx):
    """`flipcheck.check` with the live tree's seam and pool diagnoses stubbed out (they explain a differing section)."""
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, claim, None, link_ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


def layout(problems):
    return [p for p in problems if "symbol layout differs" in p]


def first(lines) -> str:
    return lines[0] if lines else ""


def test_unnamed_pool_words_accepted(c):
    """Mutation: before `unnamed_spellings`, this object read NOT READY with both names `our object does not`."""
    with testing.FixtureTree() as t:
        problems, notes = run(tree(t, OURS), link())
        c.check("the map's pool names over our equal local `@` literals, referenced by nothing else: ready",
                problems, [])
        c.check("... and the excused names are a note", [n for n in notes if "emits unnamed" in n],
                [".sdata2: 2 target name(s) our object emits unnamed - same bytes at the same offset, only local "
                 "symbols of ours over them, no other link input references them: ef_pf_one +0x0, ef_pf_two +0x4"])
    bare = build(POOL, [("@12", 0, "local")], ("@12",))
    c.check("no symbol of ours over a word is the same case",
            objcompare.unnamed_spellings({"ef_pf_two": (4, 4)}, objcompare.section_symbol_rows(bare, ".sdata2"),
                                         POOL, POOL), {"ef_pf_two": (4, 4)})


def test_layout_differences_still_refused(c):
    with testing.FixtureTree() as t:
        problems, _n = run(tree(t, OURS), link(other_refs=("ef_pf_one",)))
        lines = layout(problems)
        c.check("a name another link input references stays missing (and only that one)",
                (len(lines), "ef_pf_one: the target defines it" in first(lines), "ef_pf_two" in first(lines)), (1, True, False))
    with testing.FixtureTree() as t:
        swapped = build(POOL[4:] + POOL[:4], [("@12", 0, "local"), ("@13", 4, "local")], ("@13", "@12"))
        lines = layout(run(tree(t, swapped), link())[0])
        c.check("a reordered pool (bytes differ at the offsets) keeps both names missing",
                (len(lines), "ef_pf_one: the target" in first(lines), "ef_pf_two: the target" in first(lines)), (1, True, True))
    with testing.FixtureTree() as t:
        renamed = build(POOL, [("kOne", 0, "global"), ("@13", 4, "local")], ("kOne", "@13"))
        lines = layout(run(tree(t, renamed), link())[0])
        c.check("a global of ours under another name over the word keeps it missing",
                (len(lines), "ef_pf_one: the target" in first(lines), "ef_pf_two" in first(lines)), (1, True, False))
    with testing.FixtureTree() as t:
        moved = build(POOL[4:] + POOL[:4], [("ef_pf_two", 0, "global"), ("ef_pf_one", 4, "global")],
                      ("ef_pf_one", "ef_pf_two"))
        lines = layout(run(tree(t, moved), link())[0])
        c.check("a moved named symbol is still refused at both offsets",
                (len(lines), "ef_pf_one: at +0x4 in our object, +0x0 in the target" in first(lines)), (1, True))
    with testing.FixtureTree() as t:
        lines = layout(run(tree(t, OURS), None)[0])
        c.check("without a link context nothing is excused", len(lines), 1)
    with testing.FixtureTree() as t:
        target = build(8, [("ef_pf_one", 0, "global"), ("ef_pf_two", 4, "global")], ("ef_pf_one", "ef_pf_two"),
                       section=".sbss")
        ours = build(8, [("@12", 0, "local"), ("@13", 4, "local")], ("@12", "@13"), section=".sbss")
        lines = layout(run(tree(t, ours, target, section=".sbss"), link())[0])
        c.check("a NOBITS section has no bytes to prove: the dropped global stays missing", len(lines), 1)


def test_loads_still_compared(c):
    """The excused name does not excuse the code: a load of the other literal is the relocation-place line."""
    with testing.FixtureTree() as t:
        crossed = build(POOL, [("@12", 0, "local"), ("@13", 4, "local")], ("@13", "@12"))
        problems, _n = run(tree(t, crossed), link())
        c.check("loads crossed over an equal pool: only the relocation line refuses",
                (layout(problems), len([p for p in problems if "different place or type" in p])), ([], 1))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
