"""objextab's weak-copy rule (`objextab.dropped_entries`, `lib.binary.extab.unwind_entries`), on objects shaped like
ef/ef_drawlinestrategy: MWCC emits the inline `DrawStrategyImpl`/`DrawStrategy` dtors as weak `.text` copies with
their own `extab`/`extabindex` entries between the ctor's and `Draw`'s; the target object (retail's link kept the
copies in an earlier TU) defines neither. The copies' entries stay ordinal and local - a global map name on them makes
mwld die with `ELF_linker.c` line 7133 - and every later entry is named where it lands once they are dropped."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.elf import objextab
from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import Elf
from tools.lib.binary.extab import unwind_entries, weak_text_functions

TIER = "fixture"

UNIT = "ef/ef_line"
STARTS = {"extab": 0x8000A33C, "extabindex": 0x80023880}

CTOR = "__ct__Q34nw4r2ef16DrawLineStrategyFv"
IMPL_DTOR = "__dt__Q34nw4r2ef16DrawStrategyImplFv"
BASE_DTOR = "__dt__Q34nw4r2ef12DrawStrategyFv"
DRAW = "Draw__Q34nw4r2ef16DrawLineStrategyFv"
DTOR = "__dt__Q34nw4r2ef16DrawLineStrategyFv"
#: (function, size, weak) in `.text` order; each has one 8-byte extab record and one extabindex entry, in this order
FUNCS = [(CTOR, 0x3C, False), (IMPL_DTOR, 0x44, True), (BASE_DTOR, 0x44, True), (DRAW, 0x40, False),
         (DTOR, 0x5C, False)]


def candidate(weak=(IMPL_DTOR, BASE_DTOR), by_section=False) -> bytes:
    """Our object as MWCC writes it: ordinal local extab/extabindex symbols. `by_section`: the extabindex entries
    relocate `.text`'s section symbol plus the function's offset instead of the function's name."""
    b = ElfBuilder(keep_order=True)
    b.section(".text", bytes(sum(size for _n, size, _w in FUNCS)), align=4)
    b.section("extab", bytes(8 * len(FUNCS)), align=4)
    b.section("extabindex", bytes(12 * len(FUNCS)), align=4)
    b.symbol("", ".text", 0, 0, bind="local", type="section")
    offsets = {}
    off = 0
    for i, (name, size, _w) in enumerate(FUNCS):
        offsets[name] = off
        b.symbol("@%d" % (500 + 2 * i), "extab", 8 * i, 8, bind="local", type="object")
        b.symbol("@%d" % (501 + 2 * i), "extabindex", 12 * i, 12, bind="local", type="object")
        off += size
    for name, size, _w in FUNCS:
        b.symbol(name, ".text", offsets[name], size, bind="weak" if name in weak else "global", type="func")
    for i, (name, _s, _w) in enumerate(FUNCS):
        if by_section:
            b.reloc("extabindex", 12 * i, 1, "R_PPC_ADDR32", offsets[name])
        else:
            b.reloc("extabindex", 12 * i, name, "R_PPC_ADDR32")
        b.reloc("extabindex", 12 * i + 8, "@%d" % (500 + 2 * i), "R_PPC_ADDR32")
    return b.build()


def target(defines=(CTOR, DRAW, DTOR)) -> bytes:
    """The `dol split` object: it defines the functions retail kept in this unit."""
    b = ElfBuilder()
    b.section(".text", bytes(0x200), align=4)
    off = 0
    for name in defines:
        b.symbol(name, ".text", off, 4, type="func")
        off += 4
    return b.build()


def renamed(tree, ours: bytes, tgt: bytes | None) -> dict[str, tuple[int, int]]:
    """Run objextab on `ours` beside `tgt` (none: no target object); `{name: (st_value, binding)}` of the result."""
    path = tree.write("build/RMHE08/src/%s.o" % UNIT, ours)
    if tgt is not None:
        tree.write("build/RMHE08/obj/%s.o" % UNIT, tgt)
    objextab.rename_object(str(path), STARTS, False, False)
    elf = Elf.read(str(path))
    return {s.name: (s.section, s.value, s.bind) for s in elf.symbols if s.section in STARTS}


def test_weak_copies_keep_local_ordinals(c):
    """Mutation: before the rule, the weak copies' entries read `@etb_8000A344`/`@etb_8000A34C` (global) and `Draw`'s
    `@etb_8000A354` - two slots late, and the link crashes."""
    with testing.FixtureTree() as t:
        got = renamed(t, candidate(), target())
    c.check("the ctor's entries keep their own addresses",
            (got.get("@etb_8000A33C"), got.get("@eti_80023880")), (("extab", 0, 1), ("extabindex", 0, 1)))
    c.check("the dropped copies' entries stay ordinal and local",
            [got.get(n) for n in ("@502", "@503", "@504", "@505")],
            [("extab", 8, 0), ("extabindex", 12, 0), ("extab", 16, 0), ("extabindex", 24, 0)])
    c.check("Draw's entries are named where they land (two entries earlier)",
            (got.get("@etb_8000A344"), got.get("@eti_8002388C")), (("extab", 24, 1), ("extabindex", 36, 1)))
    c.check("the dtor's too",
            (got.get("@etb_8000A34C"), got.get("@eti_80023898")), (("extab", 32, 1), ("extabindex", 48, 1)))
    c.check("no name past the target's last entry", sorted(n for n in got if n.startswith("@et")),
            ["@etb_8000A33C", "@etb_8000A344", "@etb_8000A34C", "@eti_80023880", "@eti_8002388C", "@eti_80023898"])


def test_section_symbol_relocations(c):
    with testing.FixtureTree() as t:
        got = renamed(t, candidate(by_section=True), target())
    c.check("an extabindex entry that relocates `.text`+offset is matched to the copy at that offset",
            (got.get("@502"), got.get("@etb_8000A344")), (("extab", 8, 0), ("extab", 24, 1)))


def test_kept_copies_are_named_in_place(c):
    with testing.FixtureTree() as t:
        got = renamed(t, candidate(), target(defines=(CTOR, IMPL_DTOR, BASE_DTOR, DRAW, DTOR)))
    c.check("a weak copy the target defines is retail's own: every entry named at its offset",
            [got.get("@etb_%08X" % (0x8000A33C + 8 * i), (None,))[1] for i in range(5)], [0, 8, 16, 24, 32])
    with testing.FixtureTree() as t:
        got = renamed(t, candidate(), None)
    c.check("no target object: named at their offsets, as before",
            [got.get("@etb_%08X" % (0x8000A33C + 8 * i), (None,))[1] for i in range(5)], [0, 8, 16, 24, 32])
    with testing.FixtureTree() as t:
        got = renamed(t, candidate(weak=()), target())
    c.check("a strong function the target lacks is not a dropped copy (only weak copies are)",
            got.get("@etb_8000A344"), ("extab", 8, 1))


def test_idempotent(c):
    with testing.FixtureTree() as t:
        renamed(t, candidate(), target())
        path = str(t.path("build/RMHE08/src/%s.o" % UNIT))
        first = open(path, "rb").read()
        c.check("a second run finds only the dropped entries' ordinals and renames nothing",
                objextab.rename_object(path, STARTS, False, False), 0)
        c.check("... and leaves the bytes alone", open(path, "rb").read(), first)


def test_unwind_entries(c):
    elf = Elf.read(candidate())
    c.check("weak_text_functions", weak_text_functions(elf), {IMPL_DTOR, BASE_DTOR})
    c.check("unwind_entries: the copies' records and index entries",
            unwind_entries(elf, {IMPL_DTOR, BASE_DTOR}), {"extab": {8: 8, 16: 8}, "extabindex": {12: 12, 24: 12}})
    c.check("unwind_entries: nothing asked, nothing found", unwind_entries(elf, set()), {"extab": {}, "extabindex": {}})
    c.check("target_object maps src/ to obj/",
            objextab.target_object(os.path.join("build", "RMHE08", "src", "ef", "a.o")),
            os.path.join("build", "RMHE08", "obj", "ef", "a.o"))
    c.check("target_object: no src component, no target", objextab.target_object(os.path.join("tmp", "a.o")), "")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
