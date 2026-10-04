"""The seam evidence: `.data` emission order (classify, seams, the inline tail, the cut), the literal pool rules, the
source-file name rule, the map/splits/image readers - on hand-built symbols and a FixtureTree with a built DOL."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import struct

from tools.lib import testing
from tools.lib.binary.build import DolBuilder
from tools.splits.seams import evidence as ev

TIER = "fixture"
TLO, THI = 0x80010000, 0x80020000


def word(x):
    return x.to_bytes(4, "big")


def vtable(*ptrs):
    return word(0) + word(0) + b"".join(word(p) for p in ptrs)


def run(*spec):
    """A `.data` run: `(kind, text)` per symbol, 0x40 apart; a vtable's owner is 0x100."""
    return [ev.Sym(0x1000 + i * 0x40, 16, "%s%d" % (kind, i), kind, 0x100 if kind == ev.VTABLE else None,
                   text if kind == ev.STRING else None) for i, (kind, text) in enumerate(spec)]


def test_classify(c):
    c.check("a 0,0-headed table of code pointers is a vtable, owned by its first slot",
            ev.classify(vtable(0x80010100, 0x80010200), TLO, THI), (ev.VTABLE, 0x80010100))
    c.check("a jump table (no header) is data", ev.classify(word(0x80010100) * 3, TLO, THI)[0], ev.DATA)
    c.check("a header with no code pointer is data", ev.classify(vtable(0), TLO, THI)[0], ev.DATA)
    c.check("a pointer outside .text is data", ev.classify(vtable(0x80030000), TLO, THI)[0], ev.DATA)
    c.check("a printable NUL-terminated blob is a string", ev.classify(b"hello world\n\0\0\0", TLO, THI)[0], ev.STRING)
    c.check("one character is not a string; nothing to read is data",
            (ev.classify(b"a\0\0\0", TLO, THI)[0], ev.classify(None, TLO, THI)), (ev.DATA, (ev.DATA, None)))
    rows = [(".text", 0x80010000, 0x100, "f"), (".data", 0x100, 0x10, "a"), (".data", 0x110, None, "b"),
            (".data", 0x140, 8, "c")]
    c.check("text_range and data_symbols come from the map (a missing size from the next symbol)",
            (ev.text_range(rows), ev.data_symbols(rows)),
            ((0x80010000, 0x80010100), [(0x100, 0x10, "a"), (0x110, 0x30, "b"), (0x140, 8, "c")]))


def test_seams_and_cuts(c):
    V, S, D = ev.VTABLE, ev.STRING, ev.DATA
    one_tu = [ev.Sym(0x1000, 16, "s1", S), ev.Sym(0x1040, 16, "vC", V, 0x300), ev.Sym(0x1080, 16, "vB", V, 0x200)]
    c.check("strings then descending vtables are one TU", (ev.seams(one_tu), len(ev.fragments(one_tu))), ([], 1))
    up = [ev.Sym(0x1000, 16, "vA", V, 0x100), ev.Sym(0x1040, 4, "pad", D), ev.Sym(0x1080, 16, "vB", V, 0x200)]
    c.check("two vtables going up (padding skipped) are a zigzag, counted as up",
            ([s["kind"] for s in ev.seams(up)], dict(ev.zigzag_pairs(up))), (["zigzag"], {"up": 1}))
    weak = [ev.Sym(0x1000, 16, "vA", V, 0x100), ev.Sym(0x1040, 64, "tbl", D)]
    c.check("a vtable then other data is a weak V->D, cut only on request",
            ([s["kind"] for s in ev.seams(weak)], len(ev.fragments(weak)), len(ev.fragments(weak, weak=True))),
            (["V->D"], 1, 2))
    pairs = run((V, None), (S, "NW4R:Failed assertion IsValid()"), (S, "g3d_fog.h"), (S, "g3d_scnobj.cpp"),
                (S, "NW4R:Pointer Error"), (V, None))
    row = ev.seams(pairs)[0]
    c.check("V->S is a gap [first string, next vtable) with its inline tail (up to the last header name)",
            (row["kind"], row["addr"], row["latest"], row["width"], row["tail"]), ("V->S", 0x1040, 0x1140, 4, 2))
    c.check("strong_seams carries the cut: the first symbol after the tail", ev.strong_seams(pairs)[0]["cut"], 0x10C0)
    c.check("a narrow gap is cut after its tail", [[s.name for s in f] for f in ev.fragments(pairs)][1][0], "S3")
    wide = run((V, None), (S, "m"), (S, "a.h"), *[(S, "text%d" % i) for i in range(12)], (V, None))
    c.check("a gap wider than NARROW is not cut", (ev.seams(wide)[0]["width"] > ev.NARROW, len(ev.fragments(wide))),
            (True, 1))
    tail = run((V, None), (S, "x_ac.h"), (S, "A::f failed"))
    c.check("strings with no later vtable are only a weak V->tail, never strong",
            ([s["kind"] for s in ev.seams(tail)], ev.strong_seams(tail)), (["V->tail"], []))
    c.check("too many plain strings between headers break the tail",
            ev.inline_tail(run((S, "a"), (S, "b"), (S, "c"), (S, "d"), (S, "e.h"))), 0)


def test_names(c):
    c.check("source names: out-of-line __FILE__ (a path and .c++ allowed), not a header or a message",
            [ev.is_source_name(t) for t in ("g3d_anmvis.cpp", "a.c", "dir/x.cp", "n.c++", "g3d_fog.h", "see x.c for it",
                                            ".hidden.c", "")],
            [True, True, True, True, False, False, False, False])
    c.check("header names: an inline function's __FILE__",
            [ev.is_header_name(t) for t in ("g3d_resnode_ac.h", "x.inl", "a.c", "see foo.h for details")],
            [True, True, False, False])


def test_pool_rules(c):
    c.check("literals: an .sdata2 4/8-byte object, an .sdata string",
            [ev.is_literal(*a) for a in ((".sdata2", 4, "float"), (".sdata2", 8, ""), (".sdata2", 0x20, ""),
                                         (".sdata", 3, "string"), (".sdata", 4, "4byte"), (".data", 4, "float"),
                                         (".sdata2", 4, "float", "label"))],
            [True, True, False, True, False, False, False])
    c.check("value witnesses: only a typed .sdata2 float/double",
            [ev.is_value_witness(*a) for a in ((".sdata2", 4, "float"), (".sdata2", 8, "double"), (".sdata2", 4, "4byte"),
                                               (".sdata", 3, "string"), (".sdata2", 0x10, "float"))],
            [True, True, False, False, False])
    c.check("components: union-find over unit pairs, singletons dropped, first-seen order",
            ev.components([("A", "B"), ("C", "D"), ("B", "E"), ("F", "F")]), [{"A", "B", "E"}, {"C", "D"}])
    c.check("the int->float magic constants", ev.MAGIC[0].hex(), "4330000080000000")


def test_tree_readers(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("net/a.cpp", ranges={".text": (0x80010000, 0x80010100), ".data": (0x80500000, 0x80500040)})
        tree.add_unit("net/b.cpp", ranges={".data": (0x80500040, 0x80500080)})
        sym = tree.config_dir / "symbols.txt"
        tree.write(sym, "\n".join([
            "f = .text:0x80010000; // type:function size:0x100 scope:global",
            "lf = .text:0x80010100; // type:function size:0x10 scope:local",
            "vA = .data:0x80500000; // type:object size:0x10 scope:global",
            "s1 = .data:0x80500010; // type:object size:0x10 scope:local data:string",
            "vB = .data:0x80500040; // type:object size:0x10 scope:global",
            "lit = .sdata2:0x80600000; // type:object size:0x4 scope:local data:float", ""]))
        image = DolBuilder().text(0x80010000, bytes(0x110)).data(
            0x80500000, vtable(0x80010000, 0x80010004) + b"a message here\0\0" + bytes(0x20) + vtable(0x80010008, 0)
            + bytes(0x30)).data(0x80600000, struct.pack(">f", 1.0))
        dol = tree.write("orig/RMHE08/sys/main.dol", image.build())
        fns, labels = ev.map_tables(str(sym))
        c.check("map_tables: functions with their scope, every other row a label with its data kind",
                (fns["lf"], labels["s1"]["kind"], labels["lit"]["local"], "vA" in fns),
                ({"addr": 0x80010100, "size": 0x10, "scope": "local"}, "string", True, False))
        syms = ev.retail_symbols(str(sym), str(dol))
        c.check("the image classifies the map's .data rows", [(s.name, s.kind) for s in syms],
                [("vA", ev.VTABLE), ("s1", ev.STRING), ("vB", ev.VTABLE)])
        c.check("... a vtable, a string, a later vtable: one strong V->S at the string",
                [(s["kind"], s["addr"], s["cut"]) for s in ev.strong_seams(syms)], [("V->S", 0x80500010, 0x80500010)])
        ranges = ev.section_ranges(str(tree.config_dir / "splits.txt"), ".data")
        c.check("section_ranges / range_of: the registered .data range holding an address",
                (ev.range_of(ranges, 0x80500044), ev.range_of(ranges, 0x80700000)),
                (("net/b.cpp", 0x80500040, 0x80500080), None))
        c.check("Image.read may run past a symbol into the next bytes of its segment",
                ev.Image(str(dol)).read(0x80600000, 4), struct.pack(">f", 1.0))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
