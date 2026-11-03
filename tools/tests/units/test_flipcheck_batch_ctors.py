"""flipcheck's two false-READY classes: a named `.ctors` word the link dead-strips (`enemy/em008_prog`'s
`__declspec(section ".ctors") em008_ctor`, `lib.objcompare.TABLE_SECTIONS`) and a READY that holds only while a NOT
READY member of the same batch flips too (`homebutton/hkb_window` beside `homebutton/hkb_layout`, whose earlier weak
`ut::Color` ctor drops ours: `flipcheck.settle_batch`)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "enemy/em008_prog"
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
OTHER_REL = os.path.normpath("build/RMHE08/obj/other/user.o")
INIT = ("em008_static_init", b"\x38\x60\x00\x01" * 3)
MAIN_FN = ("em008_move", b"\x38\x60\x00\x02" * 4)


def em008(ctor_name=None, bind="local"):
    """`.text` (a function and the static initialiser) and a 4-byte `.ctors` word relocated to the initialiser:
    anonymous (MWCC's own entry, `ctor_name` None) or a named `__declspec(section ".ctors")` variable."""
    b = ElfBuilder()
    b.section(".text", MAIN_FN[1] + INIT[1], align=4)
    b.section(".ctors", bytes(4), align=4)
    b.symbol(".ctors", ".ctors", 0, 0, bind="local", type="section")
    b.symbol(MAIN_FN[0], ".text", 0, len(MAIN_FN[1]), type="func", comment=(4, 0, 0))
    b.symbol(INIT[0], ".text", len(MAIN_FN[1]), len(INIT[1]), type="func", comment=(4, 0, 0))
    if ctor_name:
        b.symbol(ctor_name, ".ctors", 0, 4, bind=bind, type="object")
    b.reloc(".ctors", 0, INIT[0], "R_PPC_ADDR32")
    b.comment()
    return b.build()


def setup(t, ours, other_refs=(MAIN_FN[0], INIT[0])):
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, em008())
    t.write("config/RMHE08/splits.txt", "%s.cpp:\n\t.text       start:0x8015D860 end:0x%08X\n"
            "\t.ctors      start:0x8056F328 end:0x8056F32C\n" % (UNIT, 0x8015D860 + len(MAIN_FN[1]) + len(INIT[1])))
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    refs = {TARGET_REL: set(), OTHER_REL: set(other_refs)}
    count = {n: 1 for n in other_refs}
    claim = {".text": (len(MAIN_FN[1]) + len(INIT[1]), 2), ".ctors": (4, 2)}
    return claim, {"refs": refs, "ref_count": count, "providers": {}}


def run(claim, ctx):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, claim, set(), ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


def test_named_table_word(c):
    with testing.FixtureTree() as t:
        claim, ctx = setup(t, em008())
        c.check("the compiler's anonymous .ctors word: READY", run(claim, ctx)[0], [])
    for bind in ("local", "global"):
        with testing.FixtureTree() as t:
            claim, ctx = setup(t, em008("em008_ctor", bind))
            problems = run(claim, ctx)[0]
            c.check("a named %s .ctors word nothing references (em008_ctor): refused" % bind,
                    [p.split(" - ")[0] for p in problems],
                    ["%s: the link dead-strips the named table entry(ies) em008_ctor (0x4 B of .ctors)" % UNIT])
    with testing.FixtureTree() as t:
        claim, ctx = setup(t, em008("em008_ctor", "global"), other_refs=(MAIN_FN[0], INIT[0], "em008_ctor"))
        c.check("... READY when another input references it", run(claim, ctx)[0], [])
    dead = lambda blob, roots=(MAIN_FN[0],): [s.name for s in objcompare.dead_symbols(blob, set(roots))]
    c.check("library: the named word is dead, and so is the initialiser it alone kept",
            dead(em008("em008_ctor")), [INIT[0], "em008_ctor"])
    c.check("... a FORCEACTIVE initialiser stays", dead(em008("em008_ctor"), (MAIN_FN[0], INIT[0])), ["em008_ctor"])
    c.check("... the runtime's reserved name is kept, with what it relocates",
            dead(em008("__init_cpp_exceptions_reference", "global")), [])
    c.check("... an anonymous word keeps its target", dead(em008()), [])
    c.check("... `.ctors$10` and `.dtors$15` are tables too; `.ctorsx` is not",
            [objcompare.table_section(n) for n in (".ctors$10", ".dtors$15", ".ctorsx", ".data")],
            [True, True, False, False])


def test_settle_batch(c):
    """The batch fixpoint with a stand-in `check` whose verdicts depend on who else is in `BATCH`."""
    # hkb_window is READY only while hkb_layout flips; widget is READY only while hkb_window does; solo needs nobody
    needs = {"homebutton/hkb_window": "homebutton/hkb_layout", "homebutton/widget": "homebutton/hkb_window"}
    base = {"homebutton/hkb_layout": [".text: object is 0x1880, splits.txt claims 0x187C (+4)"]}

    def fake_check(unit, claim, refs, link_ctx=None, map_rows=None, verbose=False):
        if unit in base:
            return list(base[unit]), []
        if unit in needs and needs[unit] not in flipcheck.BATCH:
            return [".text: object is 0x25C0, splits.txt claims 0x25B4 (+12)"], ["n"]
        return [], ["ok"]

    def settle(units):
        flipcheck.BATCH.clear()
        flipcheck.BATCH.update(units)
        wanted = {u: {} for u in units}
        results = {}
        for u in units:
            p, n = fake_check(u, {}, None)
            results[u] = {"ready": not p, "problems": p, "notes": n}
        saved = flipcheck.check
        flipcheck.check = fake_check
        try:
            flipcheck.settle_batch(wanted, results, None, None, None)
        finally:
            flipcheck.check = saved
            flipcheck.BATCH.clear()
        return results

    r = settle(["homebutton/hkb_layout", "homebutton/hkb_window", "homebutton/widget", "homebutton/solo"])
    c.check("hkb_window: READY only beside the NOT READY hkb_layout - now NOT READY, naming it, then the +12 line",
            r["homebutton/hkb_window"]["problems"],
            [flipcheck.BATCH_DEPENDENCY_LINE % "homebutton/hkb_layout",
             ".text: object is 0x25C0, splits.txt claims 0x25B4 (+12)"])
    c.check("... and what relied on hkb_window falls in the next round", r["homebutton/widget"]["ready"], False)
    c.check("... a unit that relies on nobody stays READY", r["homebutton/solo"]["ready"], True)
    c.check("... the NOT READY member keeps its own lines", r["homebutton/hkb_layout"]["problems"],
            base["homebutton/hkb_layout"])
    base.clear()
    r = settle(["homebutton/hkb_layout", "homebutton/hkb_window", "homebutton/widget"])
    c.check("a batch whose members are all READY is left as it is",
            [r[u]["ready"] for u in sorted(r)], [True, True, True])
    with testing.FixtureTree():
        pass


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
