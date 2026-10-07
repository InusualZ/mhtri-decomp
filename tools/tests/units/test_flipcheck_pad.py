"""flipcheck's trailing-alignment-pad rule (`lib.objcompare.trailing_pad`) on objects shaped like the three real cases -
g3d/g3d_gpu (.data 0x42 of 0x48), Network/PatConnection (.data 0x3B24 of 0x3B28, .sdata 0x7 of 0x8), Network/NetworkPool
(.sdata 0x5, .sdata2 0x4, .sbss 0x4 of 0x8) - and the shapes it must refuse."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.units import flipcheck

TIER = "fixture"

#: (unit, section, claim start, our size, claim size): the measured shortfalls of the three units.
REAL = [("g3d/g3d_gpu", ".data", 0x80591900, 0x42, 0x48),
        ("Network/PatConnection", ".data", 0x805FCE50, 0x3B24, 0x3B28),
        ("Network/PatConnection", ".sdata", 0x80793960, 0x7, 0x8),
        ("Network/NetworkPool", ".sdata", 0x80793988, 0x5, 0x8),
        ("Network/NetworkPool", ".sdata2", 0x8079C868, 0x4, 0x8)]


def obj(sections: dict, symbols=()):
    """An object with `{name: (bytes or ("nobits", size), align)}` sections and `(name, section, offset)` symbols."""
    b = ElfBuilder()
    for name, (data, align) in sections.items():
        if isinstance(data, tuple):
            b.nobits(name, data[1], align=align)
        else:
            b.section(name, data, align=align)
    for name, sec, off in symbols:
        b.symbol(name, sec, off, 1, bind="local")
    return b.build()


def tree(t, unit, sec, start, ours_size, claim_size, tail=None, tail_symbol=None, next_align=8, next_unit=True,
         nobits=False):
    """Our object (`ours_size` bytes of 0x11), the target (the same plus the tail), the next unit's target at the
    claim's end with `next_align`, and the splits.txt that claims both."""
    body = b"\x11" * ours_size
    tail = b"\0" * (claim_size - ours_size) if tail is None else tail
    syms = [("gap_09_%08X_%s" % (start + ours_size, sec.strip(".")), sec, ours_size)]
    if tail_symbol:
        syms.append((tail_symbol, sec, ours_size))
    ours = {sec: (("nobits", ours_size) if nobits else body, 8)}
    target = {sec: (("nobits", claim_size) if nobits else body + tail, 8)}
    t.write("build/RMHE08/src/%s.o" % unit, obj(ours))
    t.write("build/RMHE08/obj/%s.o" % unit, obj(target, syms))
    splits = "%s.cpp:\n\t%-11s start:0x%08X end:0x%08X\n" % (unit, sec, start, start + claim_size)
    if next_unit:
        t.write("build/RMHE08/obj/next/after.o", obj({sec: (("nobits", 8) if nobits else b"\0" * 8, next_align)}))
        splits += "\nnext/after.cpp:\n\t%-11s start:0x%08X end:0x%08X\n" % (sec, start + claim_size, start + claim_size + 8)
    t.write("config/RMHE08/splits.txt", splits)
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    return {sec: (claim_size, 2)}


def run(unit, claim):
    """`flipcheck.check` with the two diagnoses that read the live tree's seams and pools stubbed out (they explain a
    differing section; they are not this rule)."""
    saved = flipcheck.data_seam_problems, flipcheck.pool_group_problems
    flipcheck.data_seam_problems = lambda *a, **k: []
    flipcheck.pool_group_problems = lambda *a, **k: []
    try:
        return flipcheck.check(unit, claim, None)
    finally:
        flipcheck.data_seam_problems, flipcheck.pool_group_problems = saved


def test_real_shapes_are_fill(c):
    for unit, sec, start, ours, claim_size in REAL:
        with testing.FixtureTree() as t:
            problems, notes = run(unit, tree(t, unit, sec, start, ours, claim_size))
            c.check("%s %s 0x%X of 0x%X: no problem, the fill is a note" % (unit, sec, ours, claim_size),
                    (problems, any("alignment fill" in n for n in notes)), ([], True))
    with testing.FixtureTree() as t:
        problems, notes = run("Network/NetworkPool", tree(t, "Network/NetworkPool", ".sbss", 0x80794CB8, 4, 8,
                                                          nobits=True))
        c.check("NetworkPool .sbss (NOBITS) 0x4 of 0x8 is fill", (problems, len(notes)), ([], 1))


def test_refused_shapes(c):
    cases = {
        "a non-zero tail is content": dict(tail=b"\0\0\0\x01\0\0"),
        "a target symbol in the tail is content": dict(tail_symbol="g3d_gpu_table_tail"),
        "a next section aligned to 4 would move up": dict(next_align=4, ours=0x44),
        "no registered next section: not judged": dict(next_unit=False),
        "a shortfall past the alignment is not fill": dict(ours=0x3E),
    }
    for what, kw in cases.items():
        ours = kw.pop("ours", 0x42)
        with testing.FixtureTree() as t:
            problems, _notes = run("g3d/g3d_gpu", tree(t, "g3d/g3d_gpu", ".data", 0x80591900, ours, 0x48, **kw))
            c.check(what, any(".data: object is 0x" in p for p in problems), True)
    c.check(".text is never fill", objcompare.trailing_pad(".text", 4, 8, 8, b"\0" * 8, {}, 0x80000000, [8]), None)
    c.check("extab is never fill", objcompare.trailing_pad("extab", 4, 8, 8, b"\0" * 8, {}, 0x80000000, [8]), None)
    c.check("an alignment above 8 is not fill", objcompare.trailing_pad(".data", 4, 16, 16, b"\0" * 16, {},
                                                                        0x80000000, [16]), None)


CONFIGURE = '''Matching = True
NonMatching = False
config.libs = [{"lib": "os", "mw_version": "Wii/1.3", "cflags": [], "objects": [
    Object(NonMatching, "OS/OSLink.c"),
    Object(%s, "OS/OSMessage.c"),
]}]
'''


def link_tree(t, flag, succ_align_exp=4, tgt_align_exp=2, tail=b"\0" * 8, batch=()):
    """OS/OSLink-shaped: `.text` 0x18 of a 0x20 claim at 0x804D1440, the successor OS/OSMessage at 0x804D1460 whose
    target object is `tgt_align_exp` aligned and whose built object (ours) is `succ_align_exp` aligned."""
    start = 0x804D1440
    body = b"\x60" * 0x18
    t.write("build/RMHE08/src/OS/OSLink.o", obj({".text": (body, 4)}))
    t.write("build/RMHE08/obj/OS/OSLink.o", obj({".text": (body + tail, 4)}, [("gap_00_804D1458_text", ".text", 0x18)]))
    t.write("build/RMHE08/src/OS/OSMessage.o", obj({".text": (b"\0" * 0x20, 1 << succ_align_exp)}))
    t.write("build/RMHE08/obj/OS/OSMessage.o", obj({".text": (b"\0" * 0x20, 1 << tgt_align_exp)}))
    t.write("config/RMHE08/splits.txt",
            "OS/OSLink.c:\n\t.text       start:0x%08X end:0x%08X\n\nOS/OSMessage.c:\n\t.text       start:0x%08X end:0x%08X\n"
            % (start, start + 0x20, start + 0x20, start + 0x40))
    t.write("configure.py", CONFIGURE % flag)
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    flipcheck.BATCH.clear()
    flipcheck.BATCH.update(batch)
    return {".text": (0x20, 2)}


def test_linked_successor_pad(c):
    """A short unit links correctly when the next unit in link order is a flipped 16-aligned object: the link pads it."""
    with testing.FixtureTree() as t:
        problems, notes = run("OS/OSLink", link_tree(t, "Matching", succ_align_exp=4))
        c.check("OSLink .text 0x18 of 0x20 before a Matching 16-aligned OSMessage: ready, a link-padding note",
                (problems, any("link padding" in n for n in notes)), ([], True))
    with testing.FixtureTree() as t:
        problems, _n = run("OS/OSLink", link_tree(t, "NonMatching"))
        size = [p for p in problems if p.startswith(".text: object is 0x")]
        c.check("an unflipped successor (target aligned to 4): refused, and the hint says to flip from the tail",
                (len(size), "flip from the tail of each run" in size[0], "OS/OSMessage" in size[0]), (1, True, True))
    with testing.FixtureTree() as t:
        problems, _n = run("OS/OSLink", link_tree(t, "NonMatching", batch={"OS/OSMessage"}))
        c.check("... accepted when the successor is flipped in the same batch", problems, [])
    with testing.FixtureTree() as t:
        problems, _n = run("OS/OSLink", link_tree(t, "Matching", succ_align_exp=2))
        c.check("a flipped successor our object aligns to 4 would not pad: refused",
                len([p for p in problems if p.startswith(".text: object is 0x")]), 1)
    with testing.FixtureTree() as t:
        problems, _n = run("OS/OSLink", link_tree(t, "Matching", tail=b"\0\0\0\0\0\0\0\x4e"))
        c.check("a non-zero tail is content even before a flipped successor",
                len([p for p in problems if p.startswith(".text: object is 0x")]), 1)
    c.check(".text before a flipped 16-aligned successor is fill at the objcompare level",
            bool(objcompare.linked_trailing_pad(".text", 0x18, 0x20, b"\0" * 0x20, {}, 0x804D1440, [16])), True)
    c.check("extab never is", objcompare.linked_trailing_pad("extab", 0x18, 0x20, b"\0" * 0x20, {}, 0x804D1440, [16]), None)
    c.check("no successor alignment known: not judged",
            objcompare.linked_trailing_pad(".text", 0x18, 0x20, b"\0" * 0x20, {}, 0x804D1440, []), None)
    c.check(".sdata 4 B short before a flipped 8-aligned successor is fill (OSAlloc/OSArena/OSIpc class)",
            bool(objcompare.linked_trailing_pad(".sdata", 4, 8, b"\0" * 8, {}, 0x80793980, [8])), True)


def sbss_tree(t, ours_syms, target_syms, section=".sbss"):
    """One unit whose NOBITS `section` is 8 bytes in both objects, with the given `(name, offset)` symbols."""
    t.write("build/RMHE08/src/lib/u.o", obj({section: (("nobits", 8), 4)}, [(n, section, o) for n, o in ours_syms]))
    t.write("build/RMHE08/obj/lib/u.o", obj({section: (("nobits", 8), 4)}, [(n, section, o) for n, o in target_syms]))
    t.write("config/RMHE08/splits.txt", "lib/u.c:\n\t%-11s start:0x80795000 end:0x80795008\n" % section)
    flipcheck._RANGES.clear()
    flipcheck.set_root(str(t.root))
    return {section: (8, 2)}


def test_positioned_symbols(c):
    """A section equal in size and bytes still moves the DOL when its symbols sit elsewhere (NOBITS has no bytes to
    compare): a swapped order, a spare global the target has, a different name offset. Mutation: with the check off
    the three refused shapes read READY."""
    def layout_problems(**kw):
        with testing.FixtureTree() as t:
            problems, _n = run("lib/u", sbss_tree(t, **kw))
            return [p for p in problems if "symbol layout differs" in p]

    c.check("the same symbols at the same offsets: clean",
            layout_problems(ours_syms=[("a", 0), ("b", 4)], target_syms=[("a", 0), ("b", 4)]), [])
    swapped = layout_problems(ours_syms=[("a", 0), ("b", 4)], target_syms=[("b", 0), ("a", 4)])
    c.check("a swapped order is refused, naming both symbols", (len(swapped), "a: at +0x0" in swapped[0], "b: at +0x4" in swapped[0]),
            (1, True, True))
    spare = layout_problems(ours_syms=[("a", 0)], target_syms=[("a", 0), ("spare", 4)])
    c.check("a spare global the target has and ours dropped is refused", (len(spare), "spare: the target defines it" in spare[0]), (1, True))
    c.check(".sdata2 is positioned too", len(layout_problems(ours_syms=[("a", 0)], target_syms=[("a", 4)], section=".sdata2")), 1)
    c.check("compiler-pooled `lbl_`/`@` names and `name$688` statics carry no identity",
            layout_problems(ours_syms=[("a", 0), ("@12", 4)], target_syms=[("a", 0), ("lbl_80795004", 4), ("lo$688", 4)]), [])
    c.check("a symbol's size is not compared (the section size and offsets are what the link uses)",
            objcompare.data_symbol_gaps({"errno": (0, 4)}, {"errno": (0, 8)}), [])


def test_prefix_still_compared(c):
    with testing.FixtureTree() as t:
        claim = tree(t, "g3d/g3d_gpu", ".data", 0x80591900, 0x42, 0x48)
        t.write("build/RMHE08/src/g3d/g3d_gpu.o", obj({".data": (b"\x22" + b"\x11" * 0x41, 8)}))
        problems, _notes = run("g3d/g3d_gpu", claim)
        c.check("with the tail excused, a difference in our own bytes is still a problem",
                any("bytes differ from the target object at +0x0" in p for p in problems), True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
