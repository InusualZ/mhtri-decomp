"""flipcheck's whole-link liveness (`flipcheck.object_graph`, `link_liveness`, `unit_liveness`): what keeps a symbol
alive is a reference from bytes the link KEEPS, transitively. Two shapes, both measured on trial links: (1)
homebutton/hkb_layout's weak `__dt__Q34nw4r3lyt13AnimTransformFv`, kept by first-wins because a later input
(homebutton/hkb_animation) defines it weakly too, whose only referencer is that later input's implicit weak vtable,
itself unreferenced - mwld dead-strips both, so the dtor leaves our `.text`; (2) sound/snd_voice_pool's
`__dt__13SndVoiceListsFv`, referenced only from the pool ctor's `extab` record (a destroy-member action) - mwld keeps
what a live function's record names, so row 36 must not ask for `__declspec(export)`; a dead function's record and an
`extabindex` entry keep nothing."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"


def code(word: int, n: int) -> bytes:
    """`n` instructions of one recognisable word."""
    return (b"\x38\x60\x00" + bytes([word])) * n


def rel(side: str, unit: str) -> str:
    return os.path.normpath("build/RMHE08/%s/%s.o" % (side, unit))


#: an input the fixtures never write: unreadable, so everything it references counts (every doubt is live)
USER_REL = rel("obj", "other/user")


def context(t, inputs, user_refs, providers=None) -> dict:
    """A `link_ctx` over `inputs` (link order) as `link_reference_context` reads it - each written input's global
    references from its relocations - plus `USER_REL` referencing `user_refs`."""
    refs = {}
    for r in inputs:
        facts = objcompare.reloc_facts(str(t.path(r))) if t.path(r).exists() else None
        refs[r] = set(facts["refs"]) if facts else set()
    refs[USER_REL] = set(user_refs)
    count: dict[str, int] = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    return {"refs": refs, "ref_count": count, "providers": providers or {}, "inputs": list(inputs)}


def prepare(t, unit, ours, target, splits):
    t.write("build/RMHE08/src/%s.o" % unit, ours)
    t.write("build/RMHE08/obj/%s.o" % unit, target)
    t.write("config/RMHE08/splits.txt", splits)
    flipcheck._RANGES.clear()
    flipcheck._LIVENESS.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()


def run(unit, claim, ctx):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        # the row-36 set `link_setup` passes: every name some input references
        return flipcheck.check(unit, claim, set(ctx["ref_count"]), ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


# --- (1) the hkb_layout shape: a weak copy whose only referencer is a dead weak vtable of a later input ----------

LAYOUT = "homebutton/hkb_layout"
ANIM = "homebutton/hkb_animation"
THIRD = "homebutton/hkb_pane"
CTOR = ("__ct__Q34nw4r3lyt6LayoutFv", code(1, 5))
WEAK = ("__dt__Q34nw4r3lyt13AnimTransformFv", code(2, 4))
DTOR = ("__dt__Q34nw4r3lyt6LayoutFv", code(3, 4))
VT = "__vt__Q34nw4r3lyt13AnimTransform"
USER = "__ct__Q34nw4r3lyt13AnimTransformFv"
LAYOUT_CLAIM = {".text": (len(CTOR[1]) + len(DTOR[1]), 2)}
LAYOUT_SPLITS = "%s.cpp:\n\t.text       start:0x80502900 end:0x%08X\n" % (LAYOUT, 0x80502900 + LAYOUT_CLAIM[".text"][0])


def layout_object(funcs, weak=()) -> bytes:
    b = ElfBuilder()
    b.section(".text", b"".join(blob for _n, blob in funcs), align=4)
    off = 0
    for name, blob in funcs:
        b.symbol(name, ".text", off, len(blob), bind="weak" if name in weak else "global", type="func",
                 comment=(4, 0, 0))
        off += len(blob)
    b.comment()
    return b.build()


def anim_object(vt_bind="weak", user=False, exported_user=False) -> bytes:
    """hkb_animation's built object: its own weak copy of the dtor, a vtable (weak: MWCC's implicit copy) whose slot
    relocates it, and - with `user` - a function that loads the vtable (exported with `exported_user`)."""
    b = ElfBuilder()
    funcs = [WEAK] + ([(USER, code(9, 3))] if user else [])
    b.section(".text", b"".join(blob for _n, blob in funcs), align=4)
    b.section(".data", bytes(12), align=4)
    b.symbol(WEAK[0], ".text", 0, len(WEAK[1]), bind="weak", type="func", comment=(4, 0, 0))
    if user:
        b.symbol(USER, ".text", len(WEAK[1]), 12, type="func", comment=(4, 0, 8 if exported_user else 0))
    b.symbol(VT, ".data", 0, 12, bind=vt_bind, type="object", comment=(4, 0, 0))
    b.reloc(".data", 8, WEAK[0], "R_PPC_ADDR32")
    if user:
        b.reloc(".text", len(WEAK[1]) + 2, VT, "R_PPC_ADDR16_HA")
    b.comment()
    return b.build()


def third_object(exported: bool) -> bytes:
    """A third input whose one function calls hkb_animation's vtable user."""
    b = ElfBuilder()
    b.section(".text", code(7, 2), align=4)
    b.symbol("__ct__Q34nw4r3lyt4PaneFv", ".text", 0, 8, type="func", comment=(4, 0, 8 if exported else 0))
    b.symbol(USER)
    b.reloc(".text", 4, USER, "R_PPC_REL24")
    b.comment()
    return b.build()


def layout_case(t, anim=None, third=None, user_refs=(CTOR[0], DTOR[0])):
    """Write the layout unit (ours with the weak dtor, the target without it), hkb_animation's built object after it
    in link order and optionally a third input; return the `check` result."""
    ours = layout_object([CTOR, WEAK, DTOR], weak=(WEAK[0],))
    prepare(t, LAYOUT, ours, layout_object([CTOR, DTOR]), LAYOUT_SPLITS)
    inputs = [rel("obj", LAYOUT), rel("src", ANIM)]
    t.write("build/RMHE08/src/%s.o" % ANIM, anim if anim is not None else anim_object())
    if third is not None:
        t.write("build/RMHE08/src/%s.o" % THIRD, third)
        inputs.append(rel("src", THIRD))
    ctx = context(t, inputs, user_refs, providers={WEAK[0]: {rel("src", ANIM)}})
    return run(LAYOUT, LAYOUT_CLAIM, ctx)


def stripped(notes) -> bool:
    return any("dead-stripped function(s) %s" % WEAK[0] in n for n in notes)


def test_dead_weak_referencer(c):
    """The weak copy a later input also defines is kept by first-wins, but its only referencer is a dead weak vtable:
    the link strips it, and the rest is the target's `.text`. Against main's flipcheck: refused on `.text` +16."""
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t)
        c.check("hkb_layout: only a dead weak vtable of a later input references the weak dtor: READY",
                (problems, stripped(notes)), ([], True))
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t, anim=anim_object(vt_bind="global"))
        c.check("... the same with a GLOBAL vtable nothing references (dead too): READY", (problems, stripped(notes)),
                ([], True))
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t, anim=anim_object(user=True), third=third_object(exported=False))
        c.check("transitive: the vtable's only user is called only by a dead function of a third input: READY",
                (problems, stripped(notes)), ([], True))
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t, anim=anim_object(user=True, exported_user=True))
        c.check("the vtable is loaded by an exported (live) function: the dtor is kept, refused on .text",
                (any(p.startswith(".text: object is") for p in problems), stripped(notes)), (True, False))
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t, anim=anim_object(user=True), third=third_object(exported=True))
        c.check("... loaded by a function a live function of a third input calls: kept, refused",
                (any(p.startswith(".text: object is") for p in problems), stripped(notes)), (True, False))
    with testing.FixtureTree() as t:
        problems, notes = layout_case(t, user_refs=(CTOR[0], DTOR[0], VT))
        c.check("an unreadable input names the vtable (every doubt is live): kept, refused",
                (bool(problems), stripped(notes)), (True, False))


# --- (2) the snd_voice_pool shape: a function only an extab record names ---------------------------------------

POOL = "sound/snd_voice_pool"
POOL_CTOR = ("__ct__12SndVoicePoolFv", code(4, 6))
MEMBER_DTOR = ("__dt__13SndVoiceListsFv", code(5, 4))
POOL_DTOR = ("__dt__12SndVoicePoolFv", code(6, 4))
#: one 8-byte extab record per function, in `.text` order; a destroy-member action relocates at +4
RECORD = 8


def pool_object(funcs, exported=(), action_in=None, index_only=False) -> bytes:
    """`.text` of `funcs`, one extab record and one extabindex entry each; the record of `action_in` relocates
    `MEMBER_DTOR` (its destroy-member action). `index_only`: no action at all (only the member dtor's own index entry
    names it). A name in `exported` carries `.comment`'s 0x08."""
    b = ElfBuilder()
    b.section(".text", b"".join(blob for _n, blob in funcs), align=4)
    b.section("extab", bytes(RECORD * len(funcs)), align=4)
    b.section("extabindex", bytes(12 * len(funcs)), align=4)
    off = 0
    for i, (name, blob) in enumerate(funcs):
        b.symbol(name, ".text", off, len(blob), type="func", comment=(4, 0, 8 if name in exported else 0))
        b.symbol("@etb_%d" % i, "extab", RECORD * i, RECORD, bind="local", type="object")
        b.symbol("@eti_%d" % i, "extabindex", 12 * i, 12, bind="local", type="object")
        b.reloc("extabindex", 12 * i, name, "R_PPC_ADDR32")
        b.reloc("extabindex", 12 * i + 8, "@etb_%d" % i, "R_PPC_ADDR32")
        if name == action_in and not index_only:
            b.reloc("extab", RECORD * i + 4, MEMBER_DTOR[0], "R_PPC_ADDR32")
        off += len(blob)
    b.comment()
    return b.build()


def pool_row36(t, ours, user_refs=(POOL_CTOR[0],), with_order=True):
    funcs = [MEMBER_DTOR, POOL_CTOR]
    target = pool_object(funcs, exported={MEMBER_DTOR[0], POOL_CTOR[0]}, action_in=POOL_CTOR[0])
    prepare(t, POOL, ours, target, "%s.cpp:\n\t.text       start:0x800ED780 end:0x800ED7C8\n" % POOL)
    ctx = context(t, [rel("obj", POOL)], user_refs)
    if not with_order:
        del ctx["inputs"]
    problems, _notes = run(POOL, {".text": (len(MEMBER_DTOR[1]) + len(POOL_CTOR[1]), 2)}, ctx)
    return [p for p in problems if p.startswith("row 36")]


def test_extab_reference(c):
    """Row 36 counts a live function's extab action as a reference; a dead function's record and an extabindex
    entry do not. Against main's flipcheck: the first check fails (it asks for `__declspec(export)`)."""
    with testing.FixtureTree() as t:
        ours = pool_object([MEMBER_DTOR, POOL_CTOR], action_in=POOL_CTOR[0])
        c.check("snd_voice_pool: only the live pool ctor's extab record names the member dtor: no row 36",
                pool_row36(t, ours), [])
    with testing.FixtureTree() as t:
        ours = pool_object([MEMBER_DTOR, POOL_CTOR, POOL_DTOR], action_in=POOL_DTOR[0])
        got = pool_row36(t, ours)
        c.check("... named only by the record of a dead function (the pool dtor): row 36 names it",
                (len(got), MEMBER_DTOR[0] in (got or [""])[0]), (1, True))
    with testing.FixtureTree() as t:
        ours = pool_object([MEMBER_DTOR, POOL_CTOR], action_in=POOL_CTOR[0], index_only=True)
        got = pool_row36(t, ours)
        c.check("... named only by its own extabindex entry (bookkeeping): row 36 names it",
                (len(got), MEMBER_DTOR[0] in (got or [""])[0]), (1, True))
    with testing.FixtureTree() as t:
        ours = pool_object([MEMBER_DTOR, POOL_CTOR], action_in=POOL_CTOR[0])
        c.check("... the pool ctor's record, but the ctor itself is dead (nothing references it): row 36 names both",
                len(pool_row36(t, ours, user_refs=())), 1)
    with testing.FixtureTree() as t:
        ours = pool_object([MEMBER_DTOR, POOL_CTOR], action_in=POOL_CTOR[0])
        c.check("... without the link order the link-wide reference set decides (the passed set is empty): flagged",
                len(pool_row36(t, ours, with_order=False)), 1)


def test_graph_rules(c):
    """`object_graph`'s edges: an extab relocation belongs to the function the record unwinds, an extabindex one roots
    nothing, a `.data` slot belongs to the symbol over it."""
    with testing.FixtureTree() as t:
        path = t.write("build/RMHE08/src/x.o", pool_object([MEMBER_DTOR, POOL_CTOR], action_in=POOL_CTOR[0]))
        g = flipcheck.object_graph(str(path))
        by_name = {g["names"][i]: ts for i, ts in g["edges"].items()}
        c.check("the ctor's record action is an edge of the ctor", by_name.get(POOL_CTOR[0]),
                [("name", MEMBER_DTOR[0])])
        c.check("no always-live reference (extabindex roots nothing)", g["always"], [])
        path = t.write("build/RMHE08/src/y.o", anim_object())
        g = flipcheck.object_graph(str(path))
        c.check("the vtable slot is an edge of the vtable", {g["names"][i]: ts for i, ts in g["edges"].items()},
                {VT: [("name", WEAK[0])]})


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
