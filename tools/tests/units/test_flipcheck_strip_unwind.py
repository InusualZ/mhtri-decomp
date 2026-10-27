"""flipcheck's dead-strip model for what a flip's link removes beyond whole functions (`flipcheck.deadstrip_model`,
`lib.objcompare.dead_symbols`' `removed`, `strip_moves`' trailing fill), on objects shaped like
Runtime.PPCEABI.H/Gecko_ExceptionPPC: an unreferenced static function whose `extab` record and `extabindex` entry go with
it (the `extab` padded after an 18-byte record), and a weak `__RTTI__` copy an earlier input defines whose type-name
string - a local `.rodata` literal only that copy reads - leaves `.rodata` with it. The negative case: the same
function referenced from another input keeps its bytes and its unwind entries counted."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import struct

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

UNIT = "Runtime.PPCEABI.H/Gecko_ExceptionPPC"
OWNER = "Runtime.PPCEABI.H/CPlusLibPPC"
TARGET_REL = os.path.normpath("build/RMHE08/obj/%s.o" % UNIT)
OWNER_SRC = os.path.normpath("build/RMHE08/src/%s.o" % OWNER)
USER_REL = os.path.normpath("build/RMHE08/obj/other/user.o")
START = {".text": 0x80457490, "extab": 0x8001E3F0, "extabindex": 0x8003F174, ".rodata": 0x80572450,
         ".sdata": 0x80793CD0}

LIVE = ("__unexpected", b"\x38\x60\x00\x01" * 4)
ANCHOR = ("ExPPC_SchedulerAnchor__Fi", b"\x38\x60\x00\x02" * 2)
THROW = ("__throw", b"\x38\x60\x00\x03" * 3)
#: each function's extab record: distinct bytes, and `__throw`'s is 18 bytes, padded to 20 in the section
RECORD = {LIVE[0]: b"\x00\x08\x00\x00\x00\x00\x00\x01", ANCHOR[0]: b"\x00\x08\x00\x00\x00\x00\x00\x02",
          THROW[0]: b"\x10\x08\x00\x00" + bytes(range(1, 15))}
RTTI_OWN = "__RTTI__Q23std13bad_exception"
RTTI_WEAK = "__RTTI__Q23std9exception"
NAME_OWN = b"std::bad_exception\0\0"           # 20 bytes, a 19-byte string padded to 4 by the next one
NAME_WEAK = b"std::exception\0\0"               # 16 bytes


def etb(i: int) -> str:
    return "@etb_%08X" % (START["extab"] + 8 * i)


def eti(i: int) -> str:
    return "@eti_%08X" % (START["extabindex"] + 12 * i)


def build(funcs, names, local=(), weak_rtti=True, extab_bind="global", with_weak_name=True,
          calls_anchor=False, throw_reads_name=False) -> bytes:
    """`.text` of `funcs` (a name in `local` is a static), one extab record and one 12-byte extabindex entry per
    function spelled `names` [(extab, extabindex)], `.sdata` with the RTTI records (the weak one when `weak_rtti`) and
    `.rodata` with their type-name strings, the weak record's first when `with_weak_name`. The last function loads
    the weak RTTI (`__throw` names the base class), so only a model that drops the copy frees its string;
    `calls_anchor` adds a `bl` from the first function to the anchor, `throw_reads_name` a load of `@966` from
    `__throw`."""
    b = ElfBuilder()
    b.section(".text", b"".join(blob for _n, blob in funcs), align=4)
    extab, record_at = b"", []
    for name, _blob in funcs:
        record_at.append(len(extab))
        extab += RECORD[name] + bytes(-len(RECORD[name]) % 4)
    b.section("extab", extab, align=4)
    b.section("extabindex", b"".join(struct.pack(">III", 0, len(blob), 0) for _n, blob in funcs), align=4)
    rodata = (NAME_WEAK if with_weak_name else b"") + NAME_OWN
    b.section(".rodata", rodata, align=4)
    b.section(".sdata", bytes(16 if weak_rtti else 8), align=8)
    for i, (name, _blob) in enumerate(funcs):
        bind = extab_bind if names[i][0].startswith("@et") else "local"
        b.symbol(names[i][0], "extab", record_at[i], len(RECORD[name]), bind=bind, type="object", comment=(4, 0, 0))
        b.symbol(names[i][1], "extabindex", 12 * i, 12, bind=bind, type="object", comment=(4, 0, 0))
    off = 0
    for name, blob in funcs:
        b.symbol(name, ".text", off, len(blob), bind="local" if name in local else "global", type="func",
                 comment=(4, 0, 0))
        off += len(blob)
    own_at = len(NAME_WEAK) if with_weak_name else 0
    if with_weak_name:
        b.symbol("@966", ".rodata", 0, len(NAME_WEAK), bind="local", type="object", comment=(4, 0, 0))
    b.symbol("@964", ".rodata", own_at, 19, bind="local", type="object", comment=(4, 0, 0))
    b.symbol(RTTI_OWN, ".sdata", 0, 8, type="object", comment=(4, 0, 0))
    b.reloc(".sdata", 0, "@964", "R_PPC_ADDR32")
    if weak_rtti:
        b.symbol(RTTI_WEAK, ".sdata", 8, 8, bind="weak", type="object", comment=(4, 0, 0))
        if with_weak_name:
            b.reloc(".sdata", 8, "@966", "R_PPC_ADDR32")
    else:
        b.symbol(RTTI_WEAK)
    for i, (name, _blob) in enumerate(funcs):
        b.reloc("extabindex", 12 * i, name, "R_PPC_ADDR32")
        b.reloc("extabindex", 12 * i + 8, names[i][0], "R_PPC_ADDR32")
    b.reloc(".text", sum(len(blob) for _n, blob in funcs) - 2, RTTI_WEAK, "R_PPC_ADDR16_LO")
    if calls_anchor:
        b.reloc(".text", 4, ANCHOR[0], "R_PPC_REL24")
    if throw_reads_name:
        b.reloc(".text", sum(len(blob) for _n, blob in funcs) - len(THROW[1]) + 2, "@966", "R_PPC_ADDR16_HA")
    b.comment()
    return b.build()


#: the target `dol split` writes: the two functions retail kept, the map's names, no weak copy and no anchor
TARGET = build([LIVE, THROW], [(etb(0), eti(0)), (etb(1), eti(1))], weak_rtti=False, extab_bind="local",
               with_weak_name=False)
#: ours as `objextab` leaves it: the anchor's entries ordinal, the kept ones named where they land
OURS = build([LIVE, ANCHOR, THROW], [(etb(0), eti(0)), ("@774", "@775"), (etb(1), eti(1))], local=(ANCHOR[0],))
CLAIM = {".text": (len(LIVE[1]) + len(THROW[1]), 2), "extab": (8 + 20, 2), "extabindex": (24, 2),
         ".rodata": (len(NAME_OWN), 2), ".sdata": (8, 3)}


def map_text() -> str:
    rows = [(LIVE[0], ".text", START[".text"], len(LIVE[1]), "function"),
            (THROW[0], ".text", START[".text"] + len(LIVE[1]), len(THROW[1]), "function"),
            (RTTI_OWN, ".sdata", START[".sdata"], 8, "object")]
    for i in range(2):
        rows += [(etb(i), "extab", START["extab"] + 8 * i, 8, "object"),
                 (eti(i), "extabindex", START["extabindex"] + 12 * i, 12, "object")]
    return "".join("%s = %s:0x%08X; // type:%s size:0x%X scope:global\n" % (n, s, a, t, z) for n, s, a, z, t in rows)


def setup(t, ours, providers, user_refs=(LIVE[0], THROW[0], RTTI_OWN, RTTI_WEAK)):
    t.write("build/RMHE08/src/%s.o" % UNIT, ours)
    t.write("build/RMHE08/obj/%s.o" % UNIT, TARGET)
    lines = ["%s.cp:" % UNIT]
    for sec, (size, _a) in CLAIM.items():
        lines.append("\t%-11s start:0x%08X end:0x%08X" % (sec, START[sec], START[sec] + size))
    t.write("config/RMHE08/splits.txt", "\n".join(lines) + "\n")
    t.write("config/RMHE08/symbols.txt", map_text())
    flipcheck._RANGES.clear()
    flipcheck._MAP_ADDRESSES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    refs = {OWNER_SRC: set(), TARGET_REL: set(), USER_REL: set(user_refs)}
    count: dict = {}
    for names in refs.values():
        for n in names:
            count[n] = count.get(n, 0) + 1
    return {"refs": refs, "ref_count": count, "providers": providers, "inputs": [OWNER_SRC, TARGET_REL, USER_REL]}


def run(ctx):
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(UNIT, CLAIM, set(), ctx, set())
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved
        flipcheck.BATCH.clear()


def sized(problems):
    return sorted(p.split(":")[0] for p in problems if "splits.txt claims" in p)


def test_gecko_shape_ready(c):
    """Mutations, each measured on main's flipcheck before this rule: without the anchor's unwind entries the
    extab/extabindex size lines stay (+8/+12); without `removed` the weak copy keeps `@966` live and `.rodata` reads
    +0x10; without the trailing fill `extab` is not modelled (+8)."""
    with testing.FixtureTree() as t:
        problems, notes = run(setup(t, OURS, {RTTI_WEAK: {OWNER_SRC}}))
        c.check("an unreferenced static, its unwind entries and a dropped weak copy's string: READY", problems, [])
        c.check("... the anchor's extab record and extabindex entry are named as removed",
                sorted(n.split(":")[0] for n in notes if "unwind entries @774" in n or "unwind entries @775" in n),
                ["extab", "extabindex"])
        c.check("... the weak copy's type-name string leaves .rodata",
                any(n.startswith(".rodata:") and "dead-stripped local data @966" in n for n in notes), True)
        c.check("... extab keeps the 18-byte record's fill: 0x1C kept of 0x24",
                any(n.startswith("extab: 0x8 of the object's 0x24 bytes") and "0x1C bytes the link keeps" in n
                    for n in notes), True)


def test_referenced_function_counts(c):
    """The negative case: the same static reached from a kept function, or the copy defined by no other input."""
    called = build([LIVE, ANCHOR, THROW], [(etb(0), eti(0)), ("@774", "@775"), (etb(1), eti(1))],
                   local=(ANCHOR[0],), calls_anchor=True)
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, called, {RTTI_WEAK: {OWNER_SRC}}))
        c.check("a static a kept function calls: .text, extab and extabindex stay over the claim",
                sized(problems), [".text", "extab", "extabindex"])
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, OURS, {}))
        c.check("a weak copy no other input defines is kept, and so is the string it reads",
                [s for s in sized(problems) if s in (".rodata", ".sdata")], [".rodata", ".sdata"])
    global_anchor = build([LIVE, ANCHOR, THROW], [(etb(0), eti(0)), ("@774", "@775"), (etb(1), eti(1))])
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, global_anchor, {RTTI_WEAK: {OWNER_SRC}},
                                     user_refs=(LIVE[0], THROW[0], RTTI_OWN, ANCHOR[0])))
        c.check("a global another input references: kept, with its unwind entries",
                [s for s in sized(problems) if s in (".text", "extab", "extabindex")], [".text", "extab", "extabindex"])
    reads = build([LIVE, ANCHOR, THROW], [(etb(0), eti(0)), ("@774", "@775"), (etb(1), eti(1))], local=(ANCHOR[0],),
                  throw_reads_name=True)
    with testing.FixtureTree() as t:
        problems, _notes = run(setup(t, reads, {RTTI_WEAK: {OWNER_SRC}}, user_refs=(LIVE[0], RTTI_OWN)))
        c.check("a string a dead function the target names reads: that function is counted, so the string is too",
                [s for s in sized(problems) if s == ".rodata"], [".rodata"])


def test_dead_symbols_removed(c):
    """`removed` and `kept` at the library level: a dropped copy's references keep nothing, and it is not reported
    dead; a held symbol keeps what it references."""
    def dead(removed_names):
        elf = objcompare.be32(OURS)
        removed = {s.index for s in elf.symbols if s.name in removed_names}
        return sorted(s.name for s in objcompare.dead_symbols(OURS, {LIVE[0], THROW[0], RTTI_OWN, RTTI_WEAK},
                                                               removed))
    c.check("without removal the weak copy keeps its string", dead(()), [ANCHOR[0]])
    c.check("with the copy removed, its string is dead and the copy is not listed", dead({RTTI_WEAK}),
            ["@966", ANCHOR[0]])
    called = build([LIVE, ANCHOR, THROW], [(etb(0), eti(0)), ("@774", "@775"), (etb(1), eti(1))],
                   local=(ANCHOR[0],), calls_anchor=True)
    elf = objcompare.be32(called)
    held = {s.index for s in elf.symbols if s.name == LIVE[0]}
    c.check("`kept` holds a symbol no root names live, and what it calls with it",
            [s.name for s in objcompare.dead_symbols(called, {THROW[0]}, None, held)
             if s.section == ".text"], [])
    b = ElfBuilder()
    b.section(".text", bytes(8), align=4)
    b.section(".sdata2", bytes(8), align=4)
    b.symbol("@12", ".sdata2", 0, 4, bind="local", type="object")
    b.symbol("@13", ".sdata2", 4, 4, bind="local", type="object")
    b.symbol("covered", ".text", 4, 4, bind="local", type="func")
    b.reloc(".text", 2, "@12", "R_PPC_EMB_SDA21")
    b.reloc(".text", 6, "@13", "R_PPC_EMB_SDA21")
    c.check("bytes no symbol covers cannot be stripped: what they load stays, a dead symbol's load does not",
            [s.name for s in objcompare.dead_symbols(b.build(), set())], ["covered", "@13"])


def test_strip_moves_trailing_fill(c):
    """`strip_moves` accepts a section that ends in zero fill up to its alignment and keeps that fill; a non-zero tail
    or one past the alignment is still not modelled."""
    def section_obj(data, syms):
        b = ElfBuilder()
        b.section("extab", data, align=4)
        for name, off, size in syms:
            b.symbol(name, "extab", off, size, bind="local", type="object", comment=(4, 0, 0))
        return b.build()
    obj = section_obj(b"\x01" * 8 + b"\x02" * 8 + b"\x03" * 18 + bytes(2), [("a", 0, 8), ("b", 8, 8), ("c", 16, 18)])
    elf = objcompare.be32(obj)
    gone = [s for s in elf.symbols if s.name == "b"]
    c.check("18-byte record padded to 4: modelled, the fill kept", objcompare.strip_section(obj, "extab", gone),
            b"\x01" * 8 + b"\x03" * 18 + bytes(2))
    bad = section_obj(b"\x01" * 8 + b"\x02" * 8 + b"\x03" * 18 + b"\x00\x09", [("a", 0, 8), ("b", 8, 8), ("c", 16, 18)])
    c.check("a non-zero tail: not modelled", objcompare.strip_section(
        bad, "extab", [s for s in objcompare.be32(bad).symbols if s.name == "b"]), None)
    far = section_obj(b"\x01" * 8 + b"\x02" * 8 + b"\x03" * 18 + bytes(6), [("a", 0, 8), ("b", 8, 8), ("c", 16, 18)])
    c.check("a tail past the alignment: not modelled", objcompare.strip_section(
        far, "extab", [s for s in objcompare.be32(far).symbols if s.name == "b"]), None)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
