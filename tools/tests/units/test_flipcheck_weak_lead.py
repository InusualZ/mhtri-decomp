"""flipcheck's weak-duplicate model (`flipcheck.weak_duplicate_symbols`) and its leading-fill rule
(`lib.objcompare.leading_fill`, `flipcheck.leading_fill_of`), on objects shaped like the real cases: nw4r/ut_ResFontBase
(weak `ut::Font` dtor and vtable that nw4r/ut_Font, earlier in link order, defines), homebutton/hkb_window (a weak
`ut::Color` ctor no other input defines) and DVD/dvd (`.sbss` claimed from 0x80795090 with 0x10 bytes of fill before its
32-aligned data)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "nw4r/ut_ResFontBase"
OWNER = "nw4r/ut_Font"
LATER = "nw4r/ut_CharWriter"
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
OWNER_OBJ = os.path.normpath("build/RMHE08/obj/%s.o" % OWNER)
OWNER_SRC = os.path.normpath("build/RMHE08/src/%s.o" % OWNER)
LATER_REL = os.path.normpath("build/RMHE08/obj/%s.o" % LATER)


def code(word: int, n: int) -> bytes:
    """`n` instructions of one recognisable word."""
    return (b"\x38\x60\x00" + bytes([word])) * n


CTOR = ("__ct__Q44nw4r2ut6detail11ResFontBaseFv", code(1, 5))
WEAK = ("__dt__Q34nw4r2ut4FontFv", code(2, 4))
DTOR = ("__dt__Q44nw4r2ut6detail11ResFontBaseFv", code(3, 4))
VT_OWN = ("__vt__Q44nw4r2ut6detail11ResFontBase", bytes(range(1, 13)))
VT_WEAK = ("__vt__Q34nw4r2ut4Font", bytes(range(20, 32)))


def object_with(funcs, data=(), weak=(), comment=True):
    """`funcs`/`data` [(name, bytes)] laid out back to back in `.text`/`.data`; a name in `weak` is bound weak."""
    b = ElfBuilder()
    for sec, items in ((".text", funcs), (".data", data)):
        if not items:
            continue
        b.section(sec, b"".join(blob for _n, blob in items), align=4)
        off = 0
        for name, blob in items:
            b.symbol(name, sec, off, len(blob), bind="weak" if name in weak else "global",
                     type="func" if sec == ".text" else "object", comment=(4, 0, 0))
            off += len(blob)
    if comment:
        b.comment()
    return b.build()


def setup(t, ours, target, claim, inputs, providers, other_refs=(WEAK[0], VT_WEAK[0], CTOR[0], DTOR[0]),
          batch=(), batch_objects=None):
    """Write our object, the target and splits.txt; return the `link_ctx` (`inputs` in link order)."""
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, target)
    for unit, blob in (batch_objects or {}).items():
        t.write("build/RMHE08/src/%s.o" % unit, blob)
    lines = ["%s.cpp:" % UNIT]
    start = {".text": 0x80502900, ".data": 0x8062FB80}
    for sec, (size, _a) in sorted(claim.items()):
        lines.append("\t%-11s start:0x%08X end:0x%08X" % (sec, start[sec], start[sec] + size))
    t.write("config/RMHE08/splits.txt", "\n".join(lines) + "\n")
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    flipcheck.BATCH.update(batch)
    refs = {rel: set() for rel in inputs}
    refs[os.path.normpath("build/RMHE08/obj/other/user.o")] = set(other_refs)
    count = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    return {"refs": refs, "ref_count": count, "providers": providers, "inputs": list(inputs)}


def run(claim, ctx):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, claim, set(), ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved
        flipcheck.BATCH.clear()


TARGET = object_with([CTOR, DTOR], [VT_OWN])
OURS = object_with([CTOR, WEAK, DTOR], [VT_OWN, VT_WEAK], weak=(WEAK[0], VT_WEAK[0]))
CLAIM = {".text": (len(CTOR[1]) + len(DTOR[1]), 2), ".data": (len(VT_OWN[1]), 2)}
EARLY = [OWNER_SRC, TARGET_REL]


def test_weak_copy_dropped(c):
    """The ut_ResFontBase shape: both weak copies are defined by an input placed before ours; the link drops them and
    the rest of `.text` and `.data` is the target's byte for byte."""
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}})
        problems, notes = run(CLAIM, ctx)
        c.check("weak copies an earlier input defines: READY", problems, [])
        c.check("... with one note per section naming the copy",
                sorted(n.split(":")[0] for n in notes if "weak copies" in n), [".data", ".text"])
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}}, other_refs=())
        c.check("a weak copy nothing references (dead too) is still dropped once", run(CLAIM, ctx)[0], [])


def test_weak_copy_batch(c):
    """A unit flipped in the same batch provides the definition from its built object at its link position."""
    owner = object_with([WEAK], [VT_WEAK])
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, [OWNER_OBJ, TARGET_REL], {}, batch=(UNIT, OWNER),
                    batch_objects={OWNER: owner})
        c.check("the earlier definer is in the batch (its built object defines them): READY", run(CLAIM, ctx)[0], [])
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, [OWNER_OBJ, TARGET_REL], {}, batch=(UNIT, OWNER),
                    batch_objects={OWNER: object_with([CTOR])})
        c.check("a batch unit whose built object does not define them: refused", bool(run(CLAIM, ctx)[0]), True)


def test_weak_copy_kept(c):
    """Each near miss is refused: the link keeps our copy, so the bytes stay counted."""
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, EARLY, {})
        problems, _notes = run(CLAIM, ctx)
        c.check("a weak copy no other input defines (hkb_window's Color ctor): the size lines stay",
                sorted(p.split(":")[0] for p in problems if "splits.txt claims" in p), [".data", ".text"])
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, [TARGET_REL, LATER_REL], {WEAK[0]: {LATER_REL}, VT_WEAK[0]: {LATER_REL}})
        problems, notes = run(CLAIM, ctx)
        c.check("only a LATER input defines it (first wins, ours is kept): refused, with a note",
                (bool(problems), any("places after this unit" in n for n in notes)), (True, True))
    strong = object_with([CTOR, WEAK, DTOR], [VT_OWN, VT_WEAK])
    with testing.FixtureTree() as t:
        ctx = setup(t, strong, TARGET, CLAIM, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}})
        c.check("a GLOBAL copy an earlier input defines is not dropped: refused", bool(run(CLAIM, ctx)[0]), True)
    with testing.FixtureTree() as t:
        ctx = setup(t, OURS, TARGET, CLAIM, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}})
        del ctx["inputs"]
        c.check("no link order known: nothing dropped, refused", bool(run(CLAIM, ctx)[0]), True)
    with testing.FixtureTree() as t:
        wrong = object_with([CTOR, WEAK, (DTOR[0], code(9, 4))], [VT_OWN, VT_WEAK], weak=(WEAK[0], VT_WEAK[0]))
        ctx = setup(t, wrong, TARGET, CLAIM, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}})
        c.check("the kept bytes differ from the target: refused on the bytes",
                any("bytes differ from the target object" in p for p in run(CLAIM, ctx)[0]), True)
    with testing.FixtureTree() as t:
        target = object_with([CTOR, WEAK, DTOR], [VT_OWN], weak=(WEAK[0],))
        claim = {".text": (len(CTOR[1]) + len(WEAK[1]) + len(DTOR[1]), 2), ".data": (len(VT_OWN[1]), 2)}
        ctx = setup(t, OURS, target, claim, EARLY, {WEAK[0]: {OWNER_SRC}, VT_WEAK[0]: {OWNER_SRC}})
        problems, _notes = run(claim, ctx)
        c.check("the target defines the name (retail kept this unit's copy): `.text` is not stripped",
                [p for p in problems if p.startswith(".text")], [])


# --- the leading fill: DVD/dvd's .sbss ---------------------------------------------------------------------------

DVD = "DVD/dvd"
PREV = "DVD/dvdfs"
SBSS_START, LEAD, OURS_SIZE = 0x80795090, 0x10, 0x98


def lead_tree(t, ours_align=32, target_syms=None, nobits=True, lead_bytes=None, prev=True, prev_built=None,
              ours_syms=None):
    """DVD/dvd's `.sbss` claim and its predecessor's range ending at the claim's start; returns the claim."""
    sec = ".sbss" if nobits else ".sdata"
    claim_size = LEAD + OURS_SIZE
    ours = ElfBuilder()
    tgt = ElfBuilder()
    if nobits:
        ours.nobits(sec, OURS_SIZE, align=ours_align)
        tgt.nobits(sec, claim_size, align=16)
    else:
        body = bytes(range(1, OURS_SIZE + 1))
        ours.section(sec, body, align=ours_align)
        tgt.section(sec, (lead_bytes if lead_bytes is not None else bytes(LEAD)) + body, align=16)
    for name, off, size in (ours_syms if ours_syms is not None else [("CommandInfoCounter", 0, 4), ("PauseFlag", 4, 4)]):
        ours.symbol(name, sec, off, size, bind="local", type="object")
    for name, off, size in (target_syms if target_syms is not None else
                            [("pad_10_%08X_%s" % (SBSS_START, sec.strip(".")), 0, LEAD),
                             ("CommandInfoCounter", LEAD, 4), ("PauseFlag", LEAD + 4, 4)]):
        tgt.symbol(name, sec, off, size, bind="local", type="object")
    t.write("build/RMHE08/src/%s.o" % DVD, ours.build())
    t.write("build/RMHE08/obj/%s.o" % DVD, tgt.build())
    splits = "%s.c:\n\t%-11s start:0x%08X end:0x%08X\n" % (DVD, sec, SBSS_START, SBSS_START + claim_size)
    if prev:
        splits = "%s.c:\n\t%-11s start:0x%08X end:0x%08X\n\n" % (PREV, sec, SBSS_START - 8, SBSS_START) + splits
    if prev_built is not None:
        b = ElfBuilder()
        b.nobits(sec, prev_built, align=4) if nobits else b.section(sec, bytes(prev_built), align=4)
        t.write("build/RMHE08/src/%s.o" % PREV, b.build())
    t.write("config/RMHE08/splits.txt", splits)
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    return {sec: (claim_size, 2)}


def lead_run(claim, batch=()):
    flipcheck.BATCH.update(batch)
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(DVD, claim, None)
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved
        flipcheck.BATCH.clear()


def test_leading_fill_accepted(c):
    with testing.FixtureTree() as t:
        problems, notes = lead_run(lead_tree(t))
        c.check("DVD/dvd's .sbss: 0x10 of fill before a 32-aligned 0x98: READY", problems, [])
        c.check("... the note names the lead", any("0x10-byte lead is alignment fill" in n for n in notes), True)
    with testing.FixtureTree() as t:
        c.check("a PROGBITS section with a zero lead: READY", lead_run(lead_tree(t, nobits=False))[0], [])
    with testing.FixtureTree() as t:
        c.check("a linked predecessor that fills its claim: READY",
                lead_run(lead_tree(t, prev_built=8), batch=(DVD, PREV))[0], [])


def test_leading_fill_refused(c):
    with testing.FixtureTree() as t:
        got = lead_run(lead_tree(t, nobits=False, lead_bytes=b"\x01" + bytes(LEAD - 1)))[0]
        c.check("non-zero lead bytes: refused", any(p.startswith(".sdata: object is 0x98") for p in got), True)
    with testing.FixtureTree() as t:
        got = lead_run(lead_tree(t, target_syms=[("StrayVar", 0, 4), ("CommandInfoCounter", LEAD, 4),
                                                 ("PauseFlag", LEAD + 4, 4)]))[0]
        c.check("a real target symbol inside the lead: refused", bool(got), True)
    with testing.FixtureTree() as t:
        c.check("no registered predecessor: refused", bool(lead_run(lead_tree(t, prev=False))[0]), True)
    with testing.FixtureTree() as t:
        c.check("our section aligned to 16 (no lead at the claim's start): refused",
                bool(lead_run(lead_tree(t, ours_align=16))[0]), True)
    with testing.FixtureTree() as t:
        c.check("a linked predecessor short of its claim: refused",
                bool(lead_run(lead_tree(t, prev_built=4), batch=(DVD, PREV))[0]), True)
    with testing.FixtureTree() as t:
        got = lead_run(lead_tree(t, ours_syms=[("PauseFlag", 0, 4), ("CommandInfoCounter", 4, 4)]))[0]
        c.check("our symbols in another order after the lead: refused on the layout",
                any("symbol layout differs" in p for p in got), True)


def test_leading_fill_library(c):
    """`objcompare.leading_fill` on its own: the arithmetic and the refusals."""
    syms = {"pad_10_80795090_sbss": (0, 0x10)}
    c.check("lead 0x10 at align 32", objcompare.leading_fill(".sbss", 0x98, 32, 0xA8, None, syms, SBSS_START, True),
            0x10)
    c.check("the predecessor not judged to end at the start: None",
            objcompare.leading_fill(".sbss", 0x98, 32, 0xA8, None, syms, SBSS_START, False), None)
    c.check("a real symbol inside the lead: None",
            objcompare.leading_fill(".sbss", 0x98, 32, 0xA8, None, dict(syms, StrayVar=(4, 4)), SBSS_START, True), None)
    c.check("extab never", objcompare.leading_fill("extab", 0x98, 32, 0xA8, None, syms, SBSS_START, True), None)
    c.check("our size plus the lead past the claim: None",
            objcompare.leading_fill(".sbss", 0xA0, 32, 0xA8, None, syms, SBSS_START, True), None)
    c.check("a real `pad_` function name is not a fill label", objcompare.is_fill_label("pad_input_clear"), False)
    c.check("dtk's fill labels are", (objcompare.is_fill_label("pad_10_80795090_sbss"),
                                      objcompare.is_fill_label("gap_09_8079398D_sdata")), (True, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
