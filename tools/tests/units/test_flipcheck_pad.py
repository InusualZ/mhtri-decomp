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


def test_prefix_still_compared(c):
    with testing.FixtureTree() as t:
        claim = tree(t, "g3d/g3d_gpu", ".data", 0x80591900, 0x42, 0x48)
        t.write("build/RMHE08/src/g3d/g3d_gpu.o", obj({".data": (b"\x22" + b"\x11" * 0x41, 8)}))
        problems, _notes = run("g3d/g3d_gpu", claim)
        c.check("with the tail excused, a difference in our own bytes is still a problem",
                any("bytes differ from the target object at +0x0" in p for p in problems), True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
