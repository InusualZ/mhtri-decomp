"""flipcheck's weak-copy rule for the unwind tables (`flipcheck.dropped_unwind_symbols`, `lib.binary.extab`), on objects
shaped like ef/ef_drawlinestrategy: a weak `.text` copy of an inline dtor that an earlier link input defines sits
between the ctor and the class's own dtor, with its own `extab` record and `extabindex` entry. The link drops the
copy together with both entries, so the `extab`/`extabindex` the link keeps are the target's byte for byte and their
relocations land where the target's do; `tools/elf/objextab.py` has named the kept entries after where they land."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import struct

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "ef/ef_line"
OWNER = "ef/ef_impl"
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
OWNER_SRC = os.path.normpath("build/RMHE08/src/%s.o" % OWNER)
START = {".text": 0x800BEF5C, "extab": 0x8000A33C, "extabindex": 0x80023880}

CTOR = ("__ct__Q34nw4r2ef16DrawLineStrategyFv", b"\x38\x60\x00\x01" * 4)
WEAK = ("__dt__Q34nw4r2ef16DrawStrategyImplFv", b"\x38\x60\x00\x02" * 5)
DTOR = ("__dt__Q34nw4r2ef16DrawLineStrategyFv", b"\x38\x60\x00\x03" * 6)
#: each function's extab record (distinct bytes, so a record laid at the wrong offset is a byte difference)
RECORD = {CTOR[0]: b"\x00\x08\x00\x00\x00\x00\x00\x01", WEAK[0]: b"\x00\x08\x00\x00\x00\x00\x00\x02",
          DTOR[0]: b"\x00\x08\x00\x00\x00\x00\x00\x03"}


def etb(i: int) -> str:
    return "@etb_%08X" % (START["extab"] + 8 * i)


def eti(i: int) -> str:
    return "@eti_%08X" % (START["extabindex"] + 12 * i)


def build(funcs, weak=(), names=None, extab_bind="global") -> bytes:
    """`.text` of `funcs`, one 8-byte extab record and one 12-byte extabindex entry per function. `names` spells each
    entry's (extab, extabindex) symbols; None spells MWCC's ordinals. A name in `weak` is a weak function."""
    b = ElfBuilder()
    text = b"".join(blob for _n, blob in funcs)
    b.section(".text", text, align=4)
    b.section("extab", b"".join(RECORD[n] for n, _b in funcs), align=4)
    index = b""
    for name, blob in funcs:
        index += struct.pack(">III", 0, len(blob), 0)
    b.section("extabindex", index, align=4)
    spelled = names or [("@%d" % (500 + 2 * i), "@%d" % (501 + 2 * i)) for i in range(len(funcs))]
    for i in range(len(funcs)):
        bind = extab_bind if spelled[i][0].startswith("@et") else "local"
        b.symbol(spelled[i][0], "extab", 8 * i, 8, bind=bind, type="object", comment=(4, 0, 0))
        b.symbol(spelled[i][1], "extabindex", 12 * i, 12, bind=bind, type="object", comment=(4, 0, 0))
    off = 0
    for name, blob in funcs:
        b.symbol(name, ".text", off, len(blob), bind="weak" if name in weak else "global", type="func",
                 comment=(4, 0, 0))
        off += len(blob)
    for i, (name, _blob) in enumerate(funcs):
        b.reloc("extabindex", 12 * i, name, "R_PPC_ADDR32")
        b.reloc("extabindex", 12 * i + 8, spelled[i][0], "R_PPC_ADDR32")
    b.comment()
    return b.build()


#: the target `dol split` writes: the map's names, local, for the two functions retail kept in this unit
TARGET = build([CTOR, DTOR], names=[(etb(0), eti(0)), (etb(1), eti(1))], extab_bind="local")
#: ours as `objextab` now leaves it: the copy's entries ordinal and local, the dtor's named where it lands
OURS = build([CTOR, WEAK, DTOR], weak=(WEAK[0],), names=[(etb(0), eti(0)), ("@502", "@503"), (etb(1), eti(1))])
#: ours as `objextab` named it before the weak-copy rule: every entry at its own offset, the dtor's two slots late
OURS_OLD_NAMES = build([CTOR, WEAK, DTOR], weak=(WEAK[0],),
                       names=[(etb(0), eti(0)), (etb(1), eti(1)), (etb(2), eti(2))])
CLAIM = {".text": (len(CTOR[1]) + len(DTOR[1]), 2), "extab": (16, 2), "extabindex": (24, 2)}


def map_text() -> str:
    rows = [(CTOR[0], ".text", START[".text"], len(CTOR[1]), "function"),
            (DTOR[0], ".text", START[".text"] + len(CTOR[1]), len(DTOR[1]), "function")]
    for i in range(3):  # the third pair is the next unit's first entries, right after this claim
        rows += [(etb(i), "extab", START["extab"] + 8 * i, 8, "object"),
                 (eti(i), "extabindex", START["extabindex"] + 12 * i, 12, "object")]
    return "".join("%s = %s:0x%08X; // type:%s size:0x%X scope:global\n" % (n, s, a, t, z) for n, s, a, z, t in rows)


def setup(t, ours, providers):
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, TARGET)
    lines = ["%s.cpp:" % UNIT]
    for sec, (size, _a) in CLAIM.items():
        lines.append("\t%-11s start:0x%08X end:0x%08X" % (sec, START[sec], START[sec] + size))
    t.write("config/RMHE08/splits.txt", "\n".join(lines) + "\n")
    t.write("config/RMHE08/symbols.txt", map_text())
    flipcheck._RANGES.clear()
    flipcheck._MAP_ADDRESSES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    refs = {OWNER_SRC: set(), TARGET_REL: set(),
            os.path.normpath("build/RMHE08/obj/other/user.o"): {CTOR[0], WEAK[0], DTOR[0]}}
    count: dict = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    return {"refs": refs, "ref_count": count, "providers": providers, "inputs": [OWNER_SRC, TARGET_REL]}


def run(ctx):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, CLAIM, set(), ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved
        flipcheck.BATCH.clear()


def test_unwind_entries_dropped_with_the_copy(c):
    """Mutation: before the rule this object read NOT READY - `extab: object is 0x18, splits.txt claims 0x10`,
    `extabindex: object is 0x24 ... 0x18`, the bytes from +0x8 on and the extabindex relocations."""
    with testing.FixtureTree() as t:
        problems, notes = run(setup(t, OURS, {WEAK[0]: {OWNER_SRC}}))
        c.check("a weak copy an earlier input defines takes its unwind entries with it: READY", problems, [])
        c.check("... one note per unwind section naming the dropped entries",
                sorted(n.split(":")[0] for n in notes if "unwind entries @502" in n or "unwind entries @503" in n),
                ["extab", "extabindex"])


def test_near_misses_refused(c):
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, OURS, {}))
        c.check("no other input defines the copy (the link keeps it): the unwind size lines stay",
                sorted(p.split(":")[0] for p in problems if "splits.txt claims" in p), [".text", "extab", "extabindex"])
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, OURS_OLD_NAMES, {WEAK[0]: {OWNER_SRC}}))
        c.check("entries named at their own offsets (the old objextab) land elsewhere: refused on the relocations",
                any(p.startswith("extabindex:") and "resolve to a different place" in p for p in problems), True)
    strong = build([CTOR, WEAK, DTOR], names=[(etb(0), eti(0)), ("@502", "@503"), (etb(1), eti(1))])
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, strong, {WEAK[0]: {OWNER_SRC}}))
        c.check("a GLOBAL function is not a dropped copy: its entries stay counted",
                any(p.startswith("extab:") and "splits.txt claims" in p for p in problems), True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
