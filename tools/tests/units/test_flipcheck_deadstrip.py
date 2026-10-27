"""flipcheck's dead-strip model (`lib.objcompare.dead_symbols`/`strip_section`, `flipcheck.deadstrip_model`) and its
reverse-reference row, on objects shaped like the BTE units whose GUESS bodies of retail-stripped functions keep their
strings in the pool: BTE/gap_utils (one stripped function, 4-aligned), BTE/btm_link_key (the stripped functions take
their own `.data` strings with them) and BTE/gki_buffer (16-aligned functions with zero gaps; a `.bss` label another
object reads)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "BTE/gap_utils"
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
OTHER_REL = os.path.normpath("build/RMHE08/obj/BTE/gap_api.o")
SRC_REL = os.path.normpath("build/RMHE08/src/%s.o" % UNIT)
NOP = b"\x60\x00\x00\x00"


def code(word: int, n: int) -> bytes:
    """`n` instructions of one recognisable word."""
    return (b"\x38\x60\x00" + bytes([word])) * n


#: (name, bytes) in our object's order: `dead` sits between two kept functions, as in gap_utils.
LIVE_A = ("gap_find_free_ccb", code(1, 4))
DEAD = ("gap_inq_results_cb", code(2, 3))
LIVE_B = ("gap_convert_btm_status", code(3, 5))


def object_with(funcs, align=4, comment_align=None, gap=b"", relocs=(), data=None, extra=()):
    """An MWCC-shaped object: `funcs` [(name, bytes)] laid out in order at `align`, each gap filled with `gap` (zero
    by default) - `relocs` are (section, offset, symbol, type) - `data` an optional `.data` with (name, offset, size)
    `extra` symbols."""
    b = ElfBuilder()
    body, offsets = bytearray(), []
    for _name, blob in funcs:
        if body and len(body) % align:
            pad = align - len(body) % align
            body += (gap * pad)[:pad] if gap else bytes(pad)
        offsets.append(len(body))
        body += blob
    b.section(".text", bytes(body), align=align)
    if data is not None:
        b.section(".data", data, align=8)
    for (name, blob), off in zip(funcs, offsets):
        b.symbol(name, ".text", off, len(blob), type="func", comment=(comment_align or align, 0, 0))
    for name, sec, off, size in extra:
        b.symbol(name, sec, off, size, bind="local", type="object")
    for sec, off, sym, typ in relocs:
        if not any(s.name == sym for s in b._symbols):
            b.symbol(sym)
        b.reloc(sec, off, sym, typ)
    b.comment()
    return b.build()


#: What the rest of the link references in the fixtures: every function but the stripped one.
KEPT_REFS = ("gap_find_free_ccb", "gap_convert_btm_status", "gap_short")


def setup(t, ours, target, claim_size, map_rows=(), other_refs=KEPT_REFS, providers=None, flipped=False):
    """Write our object and the target, and return `(claim, link_ctx, map_rows)` for `flipcheck.check`. `flipped`: the
    unit is already `Object(Matching)`, so the link input is our object (with its own references), not the target."""
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, target)
    t.write("config/RMHE08/splits.txt", "%s.c:\n\t.text       start:0x80480000 end:0x%08X\n"
            % (UNIT, 0x80480000 + claim_size))
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    refs = {TARGET_REL: set(), OTHER_REL: set(other_refs)}
    if flipped:
        del refs[TARGET_REL]
        refs[SRC_REL] = objcompare.reloc_facts(ours)["refs"]
    count = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    ctx = {"refs": refs, "ref_count": count, "providers": providers or {}}
    return {".text": (claim_size, 2)}, ctx, set(map_rows)


def run(claim, ctx, rows):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, claim, set(), ctx, rows)
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


def target_obj(funcs=(LIVE_A, LIVE_B), align=4, data=None):
    return object_with(list(funcs), align=align, data=data)


def test_accepted(c):
    """The gap_utils shape: a function no link input references, absent from the target and the map, is stripped and
    the rest is the target's `.text` byte for byte."""
    with testing.FixtureTree() as t:
        claim, ctx, rows = setup(t, object_with([LIVE_A, DEAD, LIVE_B]), target_obj(), 0x24)
        problems, notes = run(claim, ctx, rows)
        c.check("a stripped GUESS body between two kept functions: READY", problems, [])
        c.check("... and the note names the stripped function and the kept size",
                any("dead-stripped function(s) gap_inq_results_cb" in n and "0x24 bytes the link keeps" in n
                    for n in notes), True)
    with testing.FixtureTree() as t:
        claim, ctx, rows = setup(t, object_with([LIVE_A, DEAD, LIVE_B], align=16),
                                 target_obj(align=16), 0x24)
        problems, _notes = run(claim, ctx, rows)
        c.check("the gki_buffer shape: 16-aligned functions with zero gaps, the stripped one's gap goes too",
                problems, [])
    with testing.FixtureTree() as t:
        dead2 = ("gap_dead_caller", code(4, 2))
        ours = object_with([LIVE_A, DEAD, dead2, LIVE_B], relocs=[(".text", 0x1C, DEAD[0], "R_PPC_REL24")])
        claim, ctx, rows = setup(t, ours, target_obj(), 0x24)
        problems, _notes = run(claim, ctx, rows)
        c.check("a function only another stripped function calls is stripped with it", problems, [])


def test_post_flip(c):
    """The verdict is the same once the unit is `Object(Matching)` (the gate checks the flipped tree): our object is
    then the link input, and its own references - a stripped function calling another - root nothing. Mutation: with
    the unit's own object counted as another input, or the model skipped for a linked unit, this fails."""
    dead2 = ("gap_dead_caller", code(4, 2))
    ours = object_with([LIVE_A, DEAD, dead2, LIVE_B], relocs=[(".text", 0x1C, DEAD[0], "R_PPC_REL24")])
    for flipped in (False, True):
        with testing.FixtureTree() as t:
            claim, ctx, rows = setup(t, ours, target_obj(), 0x24, flipped=flipped)
            problems, notes = run(claim, ctx, rows)
            c.check("%s: READY with the dead-strip note" % ("after the flip" if flipped else "before the flip"),
                    (problems, any("dead-stripped function(s)" in n for n in notes)), ([], True))
    called = object_with([LIVE_A, DEAD, LIVE_B], relocs=[(".text", 0x4, DEAD[0], "R_PPC_REL24")])
    with testing.FixtureTree() as t:
        claim, ctx, rows = setup(t, called, target_obj(), 0x24, flipped=True)
        c.check("after the flip, a function a kept one calls is still kept: refused",
                bool(run(claim, ctx, rows)[0]), True)


def test_refused(c):
    """Each near miss must still be refused: what a trial flip would not link byte-identical."""
    def refused(ours, target=None, claim_size=0x24, **kw):
        with testing.FixtureTree() as t:
            claim, ctx, rows = setup(t, ours, target or target_obj(), claim_size, **kw)
            return run(claim, ctx, rows)[0]

    base = object_with([LIVE_A, DEAD, LIVE_B])
    got = refused(base, other_refs=KEPT_REFS + (DEAD[0],))
    c.check("another link input references the function: it is kept, the size line stays",
            any(p.startswith(".text: object is 0x30, splits.txt claims 0x24") for p in got), True)
    got = refused(base, map_rows=(DEAD[0],))
    c.check("a symbols.txt row names it (retail kept it): no strip, refused", bool(got), True)
    called = object_with([LIVE_A, DEAD, LIVE_B], relocs=[(".text", 0x4, DEAD[0], "R_PPC_REL24")])
    got = refused(called)
    c.check("a kept function calls it: it is kept, refused", bool(got), True)
    with testing.FixtureTree() as t:
        claim, ctx, rows = setup(t, base, target_obj(), 0x24)
        t.write("build/RMHE08/ldscript.lcf", "FORCEACTIVE {\n    %s\n}\n" % DEAD[0])
        got = run(claim, ctx, rows)[0]
    c.check("the linker script's FORCEACTIVE keeps it: refused", bool(got), True)
    got = refused(object_with([LIVE_A, DEAD, (LIVE_B[0], code(9, 5))]))
    c.check("the kept bytes differ from the target: refused on the bytes",
            any("bytes differ from the target object" in p for p in got), True)
    got = refused(object_with([LIVE_A, DEAD, LIVE_B], align=16, gap=b"\x01"), target_obj(align=16), 0x24)
    c.check("a non-zero gap: the layout is not modelled, refused", bool(got), True)
    short = ("gap_short", code(5, 3))
    got = refused(object_with([short, DEAD, LIVE_B], align=4, comment_align=16), target_obj([short, LIVE_B], align=16),
                  0x24)
    c.check("functions packed tighter than their .comment alignment: not modelled, refused", bool(got), True)
    strings = b"used\0\0\0\0dead-only\0\0\0\0\0\0\0"
    ours = object_with([LIVE_A, DEAD, LIVE_B], data=strings,
                       extra=[("@10", ".data", 0, 5), ("@11", ".data", 8, 10)],
                       relocs=[(".text", 0x2, "@10", "R_PPC_ADDR16_HA"), (".text", 0x12, "@11", "R_PPC_ADDR16_HA")])
    with testing.FixtureTree() as t:
        claim, ctx, rows = setup(t, ours, target_obj(data=strings), 0x24)
        claim[".data"] = (len(strings), 3)
        got = run(claim, ctx, rows)[0]
    c.check("the btm_link_key shape: a string only the stripped function reads goes with it, and the target carries "
            "its bytes - refused on the size the link keeps",
            any(p.startswith(".data: object is 0x8, splits.txt claims 0x18") and "after the link drops 0x10 B" in p
                for p in got), True)


#: hkb_pane's `.data`: the class's own vtable (global, in the target) and a base class's implicit vtable that MWCC
#: emits as a weak copy, read only by the base's weak dtor - which the link dead-strips, and the vtable with it.
VT_OWN = ("__vt__Q34nw4r3lyt4Pane", bytes(range(1, 17)))
VT_BASE = ("__vt__Q44nw4r3lyt6detail8PaneBase", bytes(range(40, 52)))


def data_object(funcs, data, weak=(), reader=None, reads=None):
    """`.text` [(name, bytes)] and `.data` [(name, bytes)] back to back (`.data` 8-aligned, each symbol 4-aligned);
    a name in `weak` is bound weak, the function `reader` loads the data symbol `reads` and `LIVE_B` loads `VT_OWN`."""
    b = ElfBuilder()
    b.section(".text", b"".join(blob for _n, blob in funcs), align=4)
    if data:
        b.section(".data", b"".join(blob for _n, blob in data), align=8)
    offsets = {}
    for sec, items in ((".text", funcs), (".data", data)):
        off = 0
        for name, blob in items:
            offsets[name] = off
            b.symbol(name, sec, off, len(blob), bind="weak" if name in weak else "global",
                     type="func" if sec == ".text" else "object", comment=(4, 0, 0))
            off += len(blob)
    if reader is not None:
        b.reloc(".text", offsets[reader] + 2, reads, "R_PPC_ADDR16_HA")
    if VT_OWN[0] in offsets:        # the class's ctor (kept) stores its own vtable
        b.reloc(".text", offsets[LIVE_B[0]] + 6, VT_OWN[0], "R_PPC_ADDR16_HA")
    b.comment()
    return b.build()


def test_weak_dead_data(c):
    """The hkb_pane shape: a weak data copy (a base class's implicit vtable) that is dead, absent from the target and
    from the map is stripped like a dead function - its bytes leave `.data` and the rest is compared - while the
    btm_link_key class (a data object the target does define, a global, a map row, a kept reader) is still refused.
    Mutations: the rule off fails the accepted checks; the bind, target-name and map-row guards off each fail one."""
    target = data_object([LIVE_A, LIVE_B], [VT_OWN])

    def run_data(ours, target=target, claim_data=len(VT_OWN[1]), map_rows=()):
        with testing.FixtureTree() as t:
            claim, ctx, rows = setup(t, ours, target, 0x24, map_rows=map_rows)
            claim[".data"] = (claim_data, 3)
            return run(claim, ctx, rows)

    problems, notes = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],),
                                           reader=DEAD[0], reads=VT_BASE[0]))
    c.check("a weak vtable only a stripped function reads goes with it: READY", problems, [])
    c.check("... and the note names it and the kept size",
            any("dead-stripped weak data %s" % VT_BASE[0] in n and "0x10 bytes the link keeps" in n for n in notes),
            True)
    problems, _notes = run_data(data_object([LIVE_A, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],)))
    c.check("a weak vtable nothing reads, no function stripped: READY", problems, [])

    got = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],),
                               reader=LIVE_A[0], reads=VT_BASE[0]))[0]
    c.check("a kept function reads the weak vtable: it is kept, the .data size line stays",
            any(p.startswith(".data: object is 0x1C, splits.txt claims 0x10") for p in got), True)
    got = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],),
                               reader=DEAD[0], reads=VT_BASE[0]), map_rows=(VT_BASE[0],))[0]
    c.check("a symbols.txt row names the weak vtable: not stripped, refused, named",
            any("dead-strips 1 data symbol(s)" in p and VT_BASE[0] in p for p in got), True)
    both = data_object([LIVE_A, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],))
    got = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, VT_BASE], weak=(VT_BASE[0],),
                               reader=DEAD[0], reads=VT_BASE[0]), target=both, claim_data=0x1C)[0]
    c.check("the btm_link_key near miss, weak: the target defines the object a stripped function reads - refused",
            any("dead-strips 1 data symbol(s)" in p and "%s 0xC B of .data" % VT_BASE[0] in p for p in got), True)
    table = ("btm_key_table", VT_BASE[1])
    got = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, table], reader=DEAD[0], reads=table[0]),
                   target=data_object([LIVE_A, LIVE_B], [VT_OWN, table]), claim_data=0x1C)[0]
    c.check("the btm_link_key near miss, global: the target's object read only by a stripped function - refused",
            any("dead-strips 1 data symbol(s)" in p and "btm_key_table 0xC B of .data" in p for p in got), True)
    got = run_data(data_object([LIVE_A, DEAD, LIVE_B], [VT_OWN, table], reader=DEAD[0], reads=table[0]))[0]
    c.check("a global (not weak) object absent from the target, read only by a stripped function - refused",
            any("dead-strips 1 data symbol(s)" in p and "btm_key_table" in p for p in got), True)


def test_dead_symbols(c):
    """The liveness rules at the library level."""
    def dead(ours, roots=("gap_find_free_ccb", "gap_convert_btm_status")):
        return [s.name for s in objcompare.dead_symbols(ours, set(roots))]

    c.check("unreferenced, unexported: dead", dead(object_with([LIVE_A, DEAD, LIVE_B])), [DEAD[0]])
    b = ElfBuilder()
    b.section(".text", LIVE_A[1] + DEAD[1], align=4)
    b.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
    b.symbol(DEAD[0], ".text", len(LIVE_A[1]), len(DEAD[1]), type="func", comment=(4, 0, 0x08))
    b.comment()
    c.check(".comment force-active (0x08) keeps it", dead(b.build()), [])
    b = ElfBuilder()
    b.section(".text", LIVE_A[1] + DEAD[1], align=4)
    b.section("extabindex", bytes(12), align=4)
    b.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
    b.symbol(DEAD[0], ".text", len(LIVE_A[1]), len(DEAD[1]), type="func")
    b.reloc("extabindex", 0, DEAD[0], "R_PPC_ADDR32")
    c.check("an extabindex entry roots nothing", dead(b.build()), [DEAD[0]])
    b = ElfBuilder()
    b.section(".text", LIVE_A[1] + DEAD[1], align=4)
    b.section(".ctors", bytes(4), align=4)
    b.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
    b.symbol(DEAD[0], ".text", len(LIVE_A[1]), len(DEAD[1]), type="func")
    b.reloc(".ctors", 0, DEAD[0], "R_PPC_ADDR32")
    c.check("a .ctors entry keeps its target", dead(b.build()), [])
    b = ElfBuilder()
    b.section(".text", LIVE_A[1] + DEAD[1], align=4)
    b.section(".data", bytes(8), align=8)
    b.symbol(".text", ".text", 0, 0, bind="local", type="section")
    b.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
    b.symbol(DEAD[0], ".text", len(LIVE_A[1]), len(DEAD[1]), type="func")
    b.symbol("table", ".data", 0, 8, type="object")
    b.reloc(".data", 0, ".text", "R_PPC_ADDR32")
    c.check("a section-symbol reference keeps the whole section", dead(b.build(), roots=("table",)), [])
    c.check("an unreadable object is None, not 'nothing dead'", objcompare.dead_symbols(b"not an elf", set()), None)


def test_reverse_reference(c):
    """The gki_buffer shape: the target defines a global label inside `.bss` that another input reads; ours does not."""
    def problems(ours_label: bool, providers=None):
        tb = ElfBuilder()
        tb.section(".text", LIVE_A[1], align=4)
        tb.nobits(".bss", 0x20, align=8)
        tb.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
        tb.symbol("gki_cb", ".bss", 0, 0x10, type="object")
        tb.symbol("lbl_8070DF3E", ".bss", 0x10, 0x10, type="object")
        ob = ElfBuilder()
        ob.section(".text", LIVE_A[1], align=4)
        ob.nobits(".bss", 0x20, align=8)
        ob.symbol(LIVE_A[0], ".text", 0, len(LIVE_A[1]), type="func")
        ob.symbol("gki_cb", ".bss", 0, 0x20 if not ours_label else 0x10, type="object")
        if ours_label:
            ob.symbol("lbl_8070DF3E", ".bss", 0x10, 0x10, type="object")
        with testing.FixtureTree() as t:
            claim, ctx, rows = setup(t, ob.build(), tb.build(), len(LIVE_A[1]),
                                     other_refs=(LIVE_A[0], "lbl_8070DF3E"), providers=providers)
            claim[".bss"] = (0x20, 3)
            return [p for p in run(claim, ctx, rows)[0] if "reverse-reference" in p]

    got = problems(False)
    c.check("a label another input reads and ours does not define: refused, naming it and the reader",
            (len(got), "lbl_8070DF3E (referenced from gap_api.o)" in got[0] if got else None), (1, True))
    c.check("ours defines it (global): silent", problems(True), [])
    c.check("another input provides it: silent", problems(False, providers={"lbl_8070DF3E": {OTHER_REL}}), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
