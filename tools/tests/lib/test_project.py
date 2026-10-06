"""lib.project contract: the splits round trip and lookups, the map parser and its planned edits, the configure
evaluator, and the one ownership index - all on fixtures (no live-tree read)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.project import (AutoObjects, Configure, Ownership, Range, Refused, ShapeError, Splits, SymbolMap,
                               object_calls, parse_line)
from tools.lib.project import configure as conf_mod
from tools.lib.project import splits as splits_mod
from tools.lib.project import symbols as sym_mod
from tools.units import symbolpreflight
from tools.units import stylelint

TIER = "fixture"

SPLITS = """Sections:
\t.init       type:code align:4
\textab       type:rodata align:32
\t.text       type:code align:32
\t.ctors      type:rodata align:16
\t.data       type:data align:32

a/one.c:
\t.text       start:0x80004000 end:0x80004100
\t.data       start:0x80500000 end:0x80500010

b/two.cpp: comment:0
\textab       start:0x80001000 end:0x80001008
\t.text       start:0x80004100 end:0x80004200
\t.ctors      start:0x80400000 end:0x80400004 rename:.ctors$10

c/three.cpp:
\t.text       start:0x80004400 end:0x80004500
"""

MAP = [
    ("memcpy", ".init:0x80003000", "type:function size:0x20 scope:global"),
    ("fn_80004000", ".text:0x80004000", "type:function size:0x100"),
    ("fn_80004100", ".text:0x80004100", "type:function size:0x100 scope:local"),
    ("fn_80004300", ".text:0x80004300", "type:function size:0x10"),
    ("lbl_80500000", ".data:0x80500000", "type:object size:0x10 data:string"),
    ("dup", ".data:0x80500100", "type:object size:0x4"),
    ("dup", ".data:0x80500104", "type:object size:0x4"),
    ("alias", ".text:0x80004100", "type:label"),
]


def map_text(rows, nl="\n"):
    return nl.join("%s = %s;%s" % (n, loc, (" // " + c) if c else "") for n, loc, c in rows) + nl


def write(tmp, name, text):
    p = Path(tmp) / name
    p.write_bytes(text.encode("utf-8"))
    return p


# --- splits ------------------------------------------------------------------------------------------------

def test_splits_round_trip(c):
    for nl in ("\n", "\r\n"):
        text = SPLITS.replace("\n", nl)
        c.check("render reproduces the parsed text byte for byte (%r)" % nl, Splits.parse(text).render(), text)
    odd = "Sections:\n\t.text type:code align:4\n\n# a comment\nx.c:\n\t.text  start:0x10 end:0x20   \n\n\nstray\n"
    c.check("comments, odd spacing, trailing blank lines and junk survive", Splits.parse(odd).render(), odd)
    c.check("a text with no final newline keeps none", Splits.parse("a.c:\n\t.text start:0x1 end:0x2").render(),
            "a.c:\n\t.text start:0x1 end:0x2")
    c.check("an empty text renders empty", Splits.parse("").render(), "")
    c.check("a one-line fragment is a unit", Splits.parse("Pl/pl_act.cpp:").units, ["Pl/pl_act.cpp"])
    c.check("`Sections:` is the legend, never a unit", Splits.parse(SPLITS).units,
            ["a/one.c", "b/two.cpp", "c/three.cpp"])
    late = "a.c:\n\t.text start:0x1 end:0x2\nSections:\nb.c:\n\t.text start:0x2 end:0x3\n"
    c.check("... not even where it is not the first line", Splits.parse(late).units, ["a.c", "b.c"])
    c.check("... and that text still round-trips", Splits.parse(late).render(), late)


def test_splits_views(c):
    sp = Splits.parse(SPLITS)
    c.check("the legend's sections", sp.sections(), [".init", "extab", ".text", ".ctors", ".data"])
    c.check("ranges in file order", [(r.unit, r.section) for r in sp.ranges],
            [("a/one.c", ".text"), ("a/one.c", ".data"), ("b/two.cpp", "extab"), ("b/two.cpp", ".text"),
             ("b/two.cpp", ".ctors"), ("c/three.cpp", ".text")])
    ctor = sp.claims("b/two.cpp")[2]
    c.check("a rename attribute names the object section", (ctor.rename, ctor.object_section), (".ctors$10", ".ctors$10"))
    c.check("a plain range's object section is its own", sp.claims("a/one.c")[0].object_section, ".text")
    c.check("unit attributes are kept", sp.block("b/two.cpp").attrs, "comment:0")
    c.check("claims by stem (either spelling)", [r.section for r in sp.claims("b/two")], ["extab", ".text", ".ctors"])
    c.check("an unknown unit has no claims", sp.claims("zz/none"), [])
    c.check("by_section is sorted (start, end, unit)", sp.by_section()[".text"],
            [(0x80004000, 0x80004100, "a/one.c"), (0x80004100, 0x80004200, "b/two.cpp"),
             (0x80004400, 0x80004500, "c/three.cpp")])
    c.check("by_unit", sp.by_unit()["a/one.c"], {".text": (0x80004000, 0x80004100), ".data": (0x80500000, 0x80500010)})
    c.check("text_ranges", [r.unit for r in sp.text_ranges()], ["a/one.c", "b/two.cpp", "c/three.cpp"])
    c.check("covering: the first byte", sp.covering(".text", 0x80004100).unit, "b/two.cpp")
    c.check("covering: the last byte", sp.covering(".text", 0x800040FF).unit, "a/one.c")
    c.check("covering: a gap is None", sp.covering(".text", 0x80004300), None)
    c.check("covering: past the end is None", sp.covering(".text", 0x80004500), None)
    c.check("covering: another section is None", sp.covering(".bss", 0x80004000), None)
    c.check("neighbours bracket a gap", [r.unit for r in sp.neighbours(".text", 0x80004300)], ["b/two.cpp", "c/three.cpp"])
    c.check("overlap in a section", [r.unit for r in sp.overlap(".text", 0x800040F0, 0x80004110)], ["a/one.c", "b/two.cpp"])
    c.check("overlap across sections", len(sp.overlap(None, 0x80000000, 0x90000000)), 6)
    over = Splits.parse("a.c:\n\t.text start:0x100 end:0x400\nb.c:\n\t.text start:0x200 end:0x300\n")
    c.check("overlapping claims: the first in address order wins", over.covering(".text", 0x250).unit, "a.c")
    c.check("... and the inner one is not reported past its end", over.covering(".text", 0x350).unit, "a.c")
    dup = Splits.parse("a.c:\n\t.text start:0x100 end:0x200\na.c:\n\t.data start:0x800 end:0x900\n")
    c.check("a duplicated key is listed twice", dup.units, ["a.c", "a.c"])
    c.check("... and its claims are both blocks'", [r.section for r in dup.claims("a.c")], [".text", ".data"])


def test_splits_edits(c):
    sp = Splits.parse(SPLITS)
    added = sp.add_block("d/four.c", [(".text", 0x80004600, 0x80004700)])
    c.check("add_block appends a canonical block", added.render(),
            SPLITS + "\nd/four.c:\n\t.text       start:0x80004600 end:0x80004700\n")
    c.check("... and the original value is untouched", sp.render(), SPLITS)
    before = sp.add_block("x/new.c", [(".text", 0x80004300, 0x80004400)], before="c/three.cpp")
    c.check("add_block(before=) inserts in link order", before.units, ["a/one.c", "b/two.cpp", "x/new.c", "c/three.cpp"])
    renamed = sp.rename_unit("a/one.c", "a/uno.c")
    c.check("rename_unit changes only the header token", renamed.render(), SPLITS.replace("a/one.c:", "a/uno.c:"))
    c.check("... and every range's unit", {r.unit for r in renamed.claims("a/uno.c")}, {"a/uno.c"})
    c.raises("rename_unit of an unknown key", KeyError, sp.rename_unit, "zz.c", "yy.c")
    c.check("remove_block", sp.remove_block("b/two.cpp").units, ["a/one.c", "c/three.cpp"])
    rows = [(r.unit, r.section, r.start, r.end) for r in sp.ranges]
    c.check("first_overlap finds a collision", splits_mod.first_overlap(rows, 0x800040F0, 0x80004110, ".text").unit, "a/one.c")
    c.check("first_overlap: the same unit's identical .text is a re-apply",
            splits_mod.first_overlap(rows, 0x80004000, 0x80004100, ".text", unit="a/one.c"), None)
    c.check("first_overlap: a free range", splits_mod.first_overlap(sp.ranges, 0x80004200, 0x80004400, ".text"), None)


# --- symbols -----------------------------------------------------------------------------------------------

def test_symbol_line_parser(c):
    e = parse_line("fn_80004100 = .text:0x80004100; // type:function size:0x100 scope:local align:4 hidden\n")
    c.check("the fields", (e.name, e.section, e.address, e.type, e.size), ("fn_80004100", ".text", 0x80004100, "function", 0x100))
    c.check("the attributes", (e.scope, e.align, e.hidden, e.sized, e.end), ("local", 4, True, True, 0x80004200))
    c.check("the line keeps no ending", e.line.endswith("\n"), False)
    k = parse_line("s = .data:0x8058F750; // type:object size:0x8 data:string")
    c.check("`data:` is the attribute, never the `.data:` of the address", k.kind, "string")
    c.check("a row with no `data:` has no kind", parse_line("x = .data:0x80500000; // type:object").kind, "")
    named = parse_line("hidden = .data:0x80500000; // type:object scope:local")
    c.check("the attributes are the comment's, never the name's", (named.comment, named.hidden), ("type:object scope:local", False))
    c.check("no size: 0 and not sized", (lambda s: (s.size, s.sized))(parse_line("x = .text:0x100; // type:label")), (0, False))
    c.check("no comment parses", parse_line("x = .text:0x80001000;").type, "")
    c.check("an address without 0x parses", parse_line("x = .text:80001000;").address, 0x80001000)
    c.check("a CRLF line parses", parse_line("x = .text:0x10; // type:function\r\n").type, "function")
    c.check("a non-row is None", [parse_line(t) for t in ("", "# x", "x .text:0x1;", "x = nope;")], [None] * 4)
    c.check("to_dict is symedit's shape", sorted(parse_line("x = .text:0x10; // type:function").to_dict()),
            ["address", "line", "name", "section", "size", "type"])


def test_symbol_map_reads(c):
    with tempfile.TemporaryDirectory() as tmp:
        p = write(tmp, "symbols.txt", map_text(MAP) + "garbage line\n# comment\n\n")
        m = SymbolMap(p)
        c.check("rows stream every parsed row with its line number", [(e.name, e.lineno) for e in m.rows()][:2],
                [("memcpy", 1), ("fn_80004000", 2)])
        c.check("by_name keeps duplicates", len(m.by_name()["dup"]), 2)
        c.check("by_section is address-sorted", [e.name for e in m.by_section()[".text"]],
                ["fn_80004000", "fn_80004100", "alias", "fn_80004300"])
        c.check("find by regex, section and type", [e.name for e in m.find("^fn_", ".text", "function")],
                ["fn_80004000", "fn_80004100", "fn_80004300"])
        c.check("in_range is half-open", [e.name for e in m.in_range(0x80004100, 0x80004300, ".text")],
                ["fn_80004100", "alias"])
        c.check("infer_section: a data address picks .data", m.infer_section(0x80500008), ".data")
        c.check("infer_section: an explicit section wins", m.infer_section(0x80500008, ".text"), ".text")
        c.check("infer_section: a gap picks the nearest", m.infer_section(0x80600000), ".data")
        c.check("infer_section of nothing is .text", sym_mod.infer_section([], 0x1), ".text")
        c.check("at centres on the last row at or below", [e.name for e in m.at(0x80004150, 1)],
                ["fn_80004100", "alias", "fn_80004300"])
        c.check("at before the first row starts at the first", [e.name for e in m.at(0x80000000, 1, ".text")],
                ["fn_80004000", "fn_80004100"])
        res = m.check()
        c.check("check: duplicates with their lines", res.duplicates, (("dup", (6, 7)),))
        c.check("check: unparsed lines (comments and blanks excepted)", res.unparsed, ((9, "garbage line"),))
        c.check("check: alias groups", (res.aliases, res.ok), (1, False))
        c.check("names", "alias" in m.names() and "garbage" not in m.names(), True)


def _plan(rows, fn, *args, nl="\n"):
    with tempfile.TemporaryDirectory() as tmp:
        p = write(tmp, "symbols.txt", map_text(rows, nl))
        before = p.read_bytes()
        try:
            out = fn(SymbolMap(p), *args)
        finally:
            untouched = p.read_bytes() == before
        return out, untouched


def test_rename_plans(c):
    rows = [("foo", ".text:0x80040600", "type:function size:0x4"), ("bar", ".text:0x80040610", "type:function size:0x4")]
    for nl in ("\n", "\r\n"):
        plan, untouched = _plan(rows, lambda m: m.plan_rename([("foo", "baz")]), nl=nl)
        c.check("plan: one edit (%r)" % nl, [x[:3] for x in plan.changed], [("foo", "baz", 0)])
        c.check("render keeps the ending and changes one line (%r)" % nl, plan.render(),
                map_text(rows, nl).replace("foo =", "baz ="))
        c.check("planning writes nothing (%r)" % nl, untouched, True)

    def refused(pairs, force=False, map_rows=rows):
        try:
            _plan(map_rows, lambda m: m.plan_rename(pairs, force))
        except Refused as exc:
            return str(exc)
        return ""
    c.contains("a taken name is refused", refused([("foo", "bar")]), "is already defined")
    c.contains("force cannot put one name at two addresses", refused([("foo", "bar")], True), "two addresses")
    c.contains("an absent name (and absent new) is a typo", refused([("nope", "x")]), "is not defined")
    c.contains("an invalid name", refused([("foo", "1bad")]), "not a valid symbol name")
    c.contains("a name defined twice", refused([("dup", "x")], map_rows=rows + [("dup", ".text:0x1", ""), ("dup", ".text:0x2", "")]),
               "defined 2 times")
    c.contains("a name twice in the batch", refused([("foo", "x"), ("foo", "y")]), "")
    plan, _u = _plan([("foo", ".text:0x1", ""), ("bar", ".text:0x1", "")], lambda m: m.plan_rename([("foo", "bar")], True))
    c.check("force allows an alias at the same address", len(plan.changed), 1)
    plan, _u = _plan(rows, lambda m: m.plan_rename([("gone", "bar"), ("foo", "foo")]))
    c.check("an applied rename and a self-rename are no-ops", (plan.changed, plan.applied), ((), (("gone", "bar"), ("foo", "foo"))))
    c.raises("the shape gate refuses a non-row", ShapeError, sym_mod.rewrite_name, "foo .text:0x1;", "foo", "bar")
    c.raises("... and a row that is not the name", ShapeError, sym_mod.rewrite_name, "foo = .text:0x1;", "bar", "baz")
    with tempfile.TemporaryDirectory() as tmp:
        p = write(tmp, "symbols.txt", map_text(rows))
        m = SymbolMap(p)
        written = []
        c.check("apply writes once through the caller's writer", m.apply(m.plan_rename([("foo", "baz")]),
                                                                       lambda path, text: written.append((path, text))), True)
        c.check("... the planned text", written[0][1], map_text(rows).replace("foo =", "baz ="))
        c.check("a no-op plan writes nothing", m.apply(m.plan_rename([("gone", "bar")]), lambda *a: written.append(a)), False)
        c.check("... still one write", len(written), 1)


#: Real names dtk and objdiff carry (the map holds the first four; the last three were re-split and linked as a probe).
REAL_NAMES = ["__ct__Q34nw4r2ut19TagProcessorBase<c>Fv",
              "Process__Q34nw4r2ut19TagProcessorBase<c>FUsPQ34nw4r2ut15PrintContext<c>",
              "@LOCAL@GXInit__FPvUl@shutdownFuncRegistered", "__sinit_\\PatConnection_cpp",
              "probe_tmpl__Q24nw4r9Probe<i,c>Fv", "@GUARD@probe_guard__Fv@x", "ofs_to_obj<Q34nw4r3g3d7ResNode>__FPCvl"]


def test_template_and_local_names(c):
    rows = [("fn_80501FA8", ".text:0x80501FA8", "type:function size:0x10")]
    for name in REAL_NAMES:
        c.check("valid: %s" % name, bool(sym_mod.VALID_NAME_RE.fullmatch(name)), True)
        plan, _u = _plan(rows, lambda m, n=name: m.plan_rename([("fn_80501FA8", n)]))
        c.check("... renamed and parsed back: %s" % name, sym_mod.parse_line(plan.render().split("\n")[0]).name, name)
    for bad in ("two words", "a=b", "a;b", "sec:tion", "path/name", "1lead"):
        c.check("invalid: %r" % bad, bool(sym_mod.VALID_NAME_RE.fullmatch(bad)), False)
    pat = sym_mod.name_pattern("@LOCAL@f__Fv@x")
    c.check("a name opening with @ is found in source", bool(pat.search("extern int @LOCAL@f__Fv@x;")), True)
    pat = sym_mod.name_pattern("__ct__Q34nw4r2ut19TagProcessorBase<c>Fv")
    c.check("a template name is found, and not inside a longer one",
            (bool(pat.search("x = __ct__Q34nw4r2ut19TagProcessorBase<c>Fv;")),
             bool(pat.search("x = __ct__Q34nw4r2ut19TagProcessorBase<c>Fv2;"))), (True, False))
    c.check("a generated stem inside a path is still found (symedit classifies it as a path)",
            bool(sym_mod.name_pattern("fn_805113B0").search('#include "DWCi/fn_805113B0.h"')), True)


def test_write_text_transaction(c):
    with tempfile.TemporaryDirectory() as tmp:
        p = write(tmp, "symbols.txt", "a = .text:0x1;\r\n")
        before = p.read_bytes()

        def boom(src, dst):
            raise OSError("injected")
        c.raises("a failed rename raises", OSError, sym_mod.write_text, p, "b = .text:0x1;\r\n", boom)
        c.check("... and the previous bytes survive", p.read_bytes(), before)

        def corrupt(src, dst):
            os.replace(src, dst)
            with open(dst, "ab") as fh:
                fh.write(b"X")
        c.raises("bytes that are not the plan raise", IOError, sym_mod.write_text, p, "b = .text:0x1;\r\n", corrupt)
        c.check("... and the previous bytes are restored exactly", p.read_bytes(), before)
        sym_mod.write_text(p, "b = .text:0x1;\r\n")
        c.check("a clean write keeps the planned CRLF bytes", p.read_bytes(), b"b = .text:0x1;\r\n")
        c.check("no temp file is left", sorted(x.name for x in Path(tmp).iterdir()), ["symbols.txt"])
    c.expect("a shape failure is an AnchorError", issubclass(ShapeError, sym_mod.AnchorError))


def test_merge_plans(c):
    rows = [("prev", ".text:0x80041000", "type:function size:0x20"),
            ("fn_80041020", ".text:0x80041020", "type:function size:0x4"),
            ("after", ".text:0x80041024", "type:function size:0x8")]
    row = ("fn_80041020", "prev", 0x24)
    for nl in ("\n", "\r\n"):
        plan, untouched = _plan(rows, lambda m: m.plan_merge([row]), nl=nl)
        want = nl.join(["prev = .text:0x80041000; // type:function size:0x24",
                        "after = .text:0x80041024; // type:function size:0x8", ""])
        c.check("merge grows the previous and drops the phantom (%r)" % nl, plan.render(), want)
        c.check("planning writes nothing (%r)" % nl, untouched, True)
    plan, _u = _plan([("prev", ".text:0x80041000", "type:function size:0x24")], lambda m: m.plan_merge([row]))
    c.check("an absent phantom with the new size is the re-apply", (plan.changed, plan.applied), (False, (row,)))

    def refused(map_rows, batch, scan=None):
        try:
            _plan(map_rows, lambda m: m.plan_merge(batch, scan))
        except Refused as exc:
            return str(exc)
        return ""
    adj = rows[:2]
    c.contains("a gap", refused([("prev", ".text:0x80041000", "type:function size:0x1C"), adj[1]], [row]), "ends at 0x8004101C")
    c.contains("a wrong size", refused(adj, [("fn_80041020", "prev", 0x28)]), "is not")
    c.contains("two sections", refused([("prev", ".init:0x80041000", "type:function size:0x20"), adj[1]], [row]), "is in .init")
    c.contains("a scope mismatch", refused([adj[0], ("fn_80041020", ".text:0x80041020", "type:function size:0x4 scope:local")], [row]),
               "disagree on scope")
    c.contains("an alias", refused(adj + [("alias", ".text:0x80041020", "type:function size:0x4")], [row]), "shares its address")
    c.contains("a non-function", refused([("prev", ".text:0x80041000", "type:object size:0x20"), adj[1]], [row]), "not both type:function")
    c.contains("the same row twice", refused(adj, [row, row]), "appears twice")
    c.contains("a stale previous names the symbol ending there",
               refused([("new_prev", ".text:0x80041000", "type:function size:0x20"), adj[1]], [("fn_80041020", "old_prev", 0x24)]),
               "the symbol ending at 0x80041020 is new_prev (size:0x20)")
    c.contains("a half-applied row", refused([adj[0]], [row]), "is absent but prev has size:0x20")
    c.contains("neither defined", refused([], [row]), "neither")
    c.contains("a referenced phantom", refused(adj, [row], lambda names: {n: [("src/x.c", 7, "call")] for n in names}),
               "is referenced at src/x.c:7")
    c.check("a clean reference scan passes", refused(adj, [row], lambda names: {}), "")
    c.raises("resize refuses another name's line", ShapeError, sym_mod.resize, "x = .text:0x1; // size:0x4", "y", 8)
    c.raises("resize refuses a row with no size", ShapeError, sym_mod.resize, "x = .text:0x1; // type:function", "x", 8)


def test_data_merges(c):
    """The data half of merge-batch: NET-A's patPacketTable (0x1732 + the stray label lbl_80600042 0x4BE = 0x1BF0),
    a label inside one object, and a plain resize."""
    table = [("patPacketTable", ".data:0x805FE910", "type:object size:0x1732 data:byte"),
             ("lbl_80600042", ".data:0x80600042", "type:object size:0x4BE"),
             ("lbl_80600500", ".data:0x80600500", "type:object size:0x30")]
    plan, _u = _plan(table, lambda m: m.plan_merge([("lbl_80600042", "patPacketTable", 0x1BF0)]))
    c.check("a label at a data object's end folds in (scopes may differ, data: kept)", plan.render(),
            "patPacketTable = .data:0x805FE910; // type:object size:0x1BF0 data:byte\n"
            "lbl_80600500 = .data:0x80600500; // type:object size:0x30\n")
    inner = [("obj", ".data:0x80001000", "type:object size:0x40"), ("lbl_80001010", ".data:0x80001010", "type:object size:0x8")]
    plan, _u = _plan(inner, lambda m: m.plan_merge([("lbl_80001010", "obj", 0x40)]))
    c.check("a stray label inside one object is folded, the size unchanged", plan.render(),
            "obj = .data:0x80001000; // type:object size:0x40\n")
    plan, _u = _plan(inner[:1], lambda m: m.plan_merge([(None, "obj", 0x48)]))
    c.check("size: a data object resized", plan.render(), "obj = .data:0x80001000; // type:object size:0x48\n")
    plan, _u = _plan(inner[:1], lambda m: m.plan_merge([(None, "obj", 0x40)]))
    c.check("size: the same size is a no-op", (plan.changed, plan.applied), (False, ((None, "obj", 0x40),)))

    def refused(map_rows, batch):
        try:
            _plan(map_rows, lambda m: m.plan_merge(batch))
        except Refused as exc:
            return str(exc)
        return ""
    c.contains("a fold size that is not the union", refused(table, [("lbl_80600042", "patPacketTable", 0x1BF4)]),
               "the extent of patPacketTable")
    c.contains("a label past the object's end is not folded",
               refused([inner[0], ("lbl_80001048", ".data:0x80001048", "type:object size:0x8")],
                       [("lbl_80001048", "obj", 0x50)]), "neither inside")
    c.contains("a fold that would swallow another label", refused(
        table[:2] + [("lbl_80600100", ".data:0x80600100", "type:object size:0x4")],
        [("lbl_80600042", "patPacketTable", 0x1BF0)]), "would also cover lbl_80600100")
    c.contains("size: growing over a neighbour", refused(table, [(None, "lbl_80600042", 0x500)]), "would cover lbl_80600500")
    c.check("size: shrinking (dtk fills the gap) is allowed", refused(table, [(None, "lbl_80600500", 0x20)]), "")
    c.contains("one row per object per batch", refused(inner, [("lbl_80001010", "obj", 0x40), (None, "obj", 0x48)]),
               "appears twice")
    from tools.symbols import symedit
    with tempfile.TemporaryDirectory() as tmp:
        batch = write(tmp, "batch.txt", "# NET-A\nfold lbl_80600042 patPacketTable 1BF0\nmerge fn_1 fn_0 24\nsize obj 48\n")
        c.check("symedit reads fold, merge and size rows", symedit.read_merge_rows(str(batch)),
                [("lbl_80600042", "patPacketTable", 0x1BF0), ("fn_1", "fn_0", 0x24), (None, "obj", 0x48)])
        bad = write(tmp, "bad.txt", "size obj\n")
        c.raises("... and refuses a short row", SystemExit, symedit.read_merge_rows, str(bad))
    c.contains("size: a function is not resized",
               refused([("f", ".text:0x80001000", "type:function size:0x4")], [(None, "f", 0x8)]), "only a data object")


# --- configure ---------------------------------------------------------------------------------------------

CONFIGURE = '''
import argparse
parser = argparse.ArgumentParser()
args = parser.parse_args()
config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version) if False else 0
cflags_base = [
    "-O4,p",
    "-Cpp_exceptions off",  # a comment Object(Matching, "commented/out.c")
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
]
if args.debug:
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")
if args.warn == "all":
    cflags_base.append("-W all")
cflags_runtime = [*cflags_base, "-inline auto"]
cflags_fast = [*[f for f in cflags_base if f not in ("-O4,p", "-Cpp_exceptions off")], "-O3", "-Cpp_exceptions on"]
cflags_one = [f for f in cflags_runtime if f != "-inline auto"] + ["-lang", "c++"]
cflags_unknown = [*undefined_group, "-x"]
def Helper(lib_name, objects):
    """A helper lib."""
    return {"lib": lib_name, "mw_version": "GC/1.2.5n", "cflags": cflags_runtime, "objects": objects}
def MatchingFor(*versions):
    return config.version in versions
Matching = True
NonMatching = False
config.libs = [
    {
        "lib": "game",
        "mw_version": "Wii/1.3",
        "cflags": cflags_fast,
        "objects": [
            Object(NonMatching, "game/a.cpp"),
            Object(Matching, "game/b.c", cflags=cflags_one),
            Object(NonMatching, "game/c.cpp", cflags=[*cflags_fast, "-pool off"]),
            Object(MatchingFor("RMHE08"), "game/d.c"),
            Object(NonMatching, "game/e.c", cflags=cflags_missing),
        ],
    },
    Helper("sdk", [Object(Matching, "sdk/x.c")]),
    Object(NonMatching, "loose/y.c"),
]
'''


def test_configure_evaluator(c):
    conf = Configure.parse(CONFIGURE)
    base = ("-O4,p", "-Cpp_exceptions off", "-i build/RMHE08/include", "-DBUILD_VERSION=0", "-DNDEBUG=1")
    c.check("a group with f-strings and the no-flag `else` append", conf.cflags("cflags_base"), base)
    c.check("a spread", conf.cflags("cflags_runtime"), base + ("-inline auto",))
    c.check("a filtered spread (not in)", conf.cflags("cflags_fast"),
            ("-i build/RMHE08/include", "-DBUILD_VERSION=0", "-DNDEBUG=1", "-O3", "-Cpp_exceptions on"))
    c.check("a filter (!=) plus a concatenation", conf.cflags("cflags_one"), base + ("-lang", "c++"))
    c.check("an unresolvable group is absent, not guessed", conf.cflags("cflags_unknown"), None)
    c.check("groups in definition order", list(conf.groups()), ["cflags_base", "cflags_runtime", "cflags_fast", "cflags_one"])
    c.check("a lib name resolves to its group", conf.cflags("game"), conf.cflags("cflags_fast"))
    objs = conf.objects()
    c.check("every object in file order, loose ones too", [o.path for o in objs],
            ["game/a.cpp", "game/b.c", "game/c.cpp", "game/d.c", "game/e.c", "sdk/x.c", "loose/y.c"])
    c.check("a commented-out Object is not registered", conf.object("commented/out.c"), None)
    a, b, cc, d, e, x, y = objs
    c.check("lib attributes", (a.lib, a.mw_version, a.lib_cflags, a.cflags_name), ("game", "Wii/1.3", "cflags_fast", "cflags_fast"))
    c.check("an object's cflags=<group> wins", (b.cflags_name, b.cflags), ("cflags_one", conf.cflags("cflags_one")))
    c.check("an object's cflags expression resolves", (cc.cflags_name, cc.cflags[-1]), (None, "-pool off"))
    c.check("... and its source text is kept as spelled", cc.option("cflags"), '[*cflags_fast, "-pool off"]')
    c.check("MatchingFor is evaluated against the version", (d.flag, d.linked), ('MatchingFor("RMHE08")', True))
    c.check("an undefined group keeps its name, not the object", (e.path, e.cflags_name), ("game/e.c", "cflags_missing"))
    c.check("a helper lib is evaluated", (x.lib, x.mw_version, x.lib_cflags, x.linked), ("sdk", "GC/1.2.5n", "cflags_runtime", True))
    c.check("a loose object has no lib", (y.lib, y.flag), (None, "NonMatching"))
    c.check("matching_units", conf.matching_units(), ["game/b.c", "sdk/x.c"])
    c.check("object() by stem", conf.object("game/a").path, "game/a.cpp")
    c.check("object_line_text", conf.object_line_text("game/b.c").strip(), 'Object(Matching, "game/b.c", cflags=cflags_one),')
    c.check("libs", [(lib.name, len(lib.objects)) for lib in conf.libs()], [("game", 5), ("sdk", 1)])
    c.check("a debug build takes the other branch", Configure.parse(CONFIGURE, args={"debug": True}).cflags("cflags_base")[-2:],
            ("-sym on", "-DDEBUG=1"))
    c.raises("a configure.py that does not parse raises", SyntaxError, Configure.parse, "config.libs = [\n")


def test_object_calls(c):
    calls = object_calls(CONFIGURE)
    c.check("every spelled call, comments skipped",
            [x.path for x in calls], ["game/a.cpp", "game/b.c", "game/c.cpp", "game/e.c", "sdk/x.c", "loose/y.c"])
    c.check("closed is the one-line, no-option shape", [x.closed for x in calls], [True, False, False, False, True, True])
    c.check("a diff line fragment", [x.normalised() for x in object_calls('+            Object(Matching, "Pl/pl.cpp"),')],
            ['Object(Matching, "Pl/pl.cpp")'])
    c.check("an unclosed multi-line call still names its path", [x.path for x in object_calls('Object(NonMatching, "a.c",')], ["a.c"])
    c.check("a string or comment mention is not a call",
            object_calls('s = "Object(Matching, \\"x.c\\")"  # Object(NonMatching, "y.c")'), [])
    c.check("an attribute named Object is not the call", object_calls('x.Object(Matching, "z.c")'), [])
    calls_conf = [o.path for o in Configure.parse(CONFIGURE).objects() if o.flag in conf_mod.FLAGS]
    c.check("the token scan and the evaluator agree on the plain calls", calls_conf,
            [x.path for x in calls if x.flag in conf_mod.FLAGS])


def test_fixture_tree_configure(c):
    with testing.FixtureTree() as tree:
        tree.add_cflags("cflags_x", ["-O3"])
        tree.add_unit("Dir/a.c", lib="mod", cflags="cflags_x")
        tree.add_unit("Dir/b.cpp", lib="mod", flag="Matching", cflags="cflags_x")
        conf = Configure.load(tree.path("configure.py"))
        c.check("FixtureTree's configure.py evaluates", [(o.path, o.flag, o.cflags) for o in conf.objects()],
                [("Dir/a.c", "NonMatching", ("-O3",)), ("Dir/b.cpp", "Matching", ("-O3",))])


# --- ownership ---------------------------------------------------------------------------------------------

def _tree():
    tree = testing.FixtureTree()
    tree.add_unit("a/one.c", ranges={".text": (0x80004000, 0x80004100), ".data": (0x80500000, 0x80500010)})
    tree.add_unit("a/two.c", ranges={".text": (0x80004200, 0x80004300)})
    tree.add_unit("b/three.c", ranges={".text": (0x80004400, 0x80004500)})
    for name, sec, addr, size in (("one_fn", ".text", 0x80004000, 0x100), ("gap_fn", ".text", 0x80004100, 0x100),
                                  ("two_fn", ".text", 0x80004200, 0x100), ("band_gap", ".text", 0x80004300, 0x100),
                                  ("three_fn", ".text", 0x80004400, 0x100), ("one_data", ".data", 0x80500000, 0x10)):
        tree.add_symbol(name, sec, addr, size)
    os.remove(tree.path("src/a/two.c"))          # registered, no source yet
    return tree


def test_ownership(c):
    with _tree() as tree:
        own = Ownership.load(tree.root, auto=True)
        c.check("resolve: owned", own.resolve("one_fn"),
                {"kind": "owned", "unit": "a/one.c", "section": ".text", "address": 0x80004000, "type": "function"})
        c.check("resolve: unsplit, inside one module's band", own.resolve("gap_fn")["module"], "a")
        c.check("resolve: unsplit between two modules has no band", own.resolve("band_gap")["module"], None)
        c.check("resolve: absent", own.resolve("nope"), None)
        c.check("name_at / resolution_at", (own.name_at(".data", 0x80500000), own.resolution_at(".data", 0x80500000)["unit"]),
                ("one_data", "a/one.c"))
        c.check("owner_of: a unit with source is reconstructed", own.owner_of(".text", 0x80004010).state, "reconstructed")
        c.check("owner_of: a unit without source is registered", (own.owner_of(".text", 0x80004210).state,
                                                                  own.owner_of(".text", 0x80004210).range), ("registered", (0x80004200, 0x80004300)))
        c.check("owner_of: unsplit carries the band", (own.owner_of(".text", 0x80004150).state, own.owner_of(".text", 0x80004150).band),
                ("unsplit", "a"))
        c.check("unit_of_symbol", (own.unit_of_symbol("three_fn"), own.unit_of_symbol("gap_fn")), ("b/three.c", None))
        c.check("symbols_of_unit", own.symbols_of_unit("a/one.c"), ["one_fn", "one_data"])
        c.check("symbols_of_unit in one section", own.symbols_of_unit("a/one.c", ".data"), ["one_data"])
        c.check("load is cached per mtime", Ownership.load(tree.root, auto=True) is own, True)
        tree.write("build/RMHE08/config.json", json.dumps({"units": [
            {"name": "auto_03_80004100_text", "code_size": 0x80, "data_size": 0},
            {"name": "auto_03_80004300_text", "code_size": 0, "data_size": 0}]}))
        own2 = Ownership.from_files(tree.path("config/RMHE08/symbols.txt"), tree.path("config/RMHE08/splits.txt"),
                                    auto=AutoObjects.load(tree.path("build/RMHE08/config.json")), root=tree.root)
        c.check("owner_of: an auto object", (own2.owner_of(".text", 0x80004110).state, own2.owner_of(".text", 0x80004110).unit),
                ("auto", "auto_03_80004100_text"))
        c.check("... past its size is unsplit", own2.owner_of(".text", 0x80004190).state, "unsplit")
        c.check("... an unsized last object is open-ended", own2.owner_of(".text", 0x80009000).state, "auto")


def test_owner_label(c):
    from tools.lib.project.ownership import owner_label, source_exists
    with _tree() as tree:
        own = Ownership.load(tree.root)
        exists = source_exists(tree.root)
        c.check("a registered unit with a source is reconstructed", owner_label(own.resolve("one_fn"), exists),
                ("a/one.c", "reconstructed", "a/one.c"))
        c.check("an unsplit name inside one module's band names the band", owner_label(own.resolve("gap_fn"), exists),
                ("unsplit (a)", "unsplit", None))
        c.check("an unsplit name between two modules", owner_label(own.resolve("band_gap"), exists),
                ("unsplit address", "unsplit", None))
        c.check("absent, and a duplicate row", (owner_label(None, exists), owner_label({"kind": "dup"}, exists)),
                (("not in the symbol map", "unmapped", None), ("duplicate row in the symbol map", "duplicate", None)))
        c.check("a claim with no source yet is registered", owner_label({"kind": "owned", "unit": "z/none.c"}, exists),
                ("z/none.c (no source yet)", "registered", "z/none.c"))
        c.check("source_exists: no unit is no source", (exists(None), exists("")), (False, False))


def test_ownership_at_ref_and_subclass(c):
    with _tree() as tree:
        texts = {"config/RMHE08/symbols.txt": tree.read("config/RMHE08/symbols.txt"),
                 "config/RMHE08/splits.txt": tree.read("config/RMHE08/splits.txt").replace("a/two.c:", "a/renamed.c:")}
        shown = []

        def show(ref, rel):
            shown.append((ref, rel))
            return texts[rel].encode("utf-8") if ref == "base" else None
        old = Ownership.at_ref(tree.root, "base", show)
        c.check("at_ref judges by the ref's files", old.resolve("two_fn")["unit"], "a/renamed.c")
        c.check("at_ref asks for both files at that ref", shown, [("base", "config/RMHE08/symbols.txt"), ("base", "config/RMHE08/splits.txt")])
        c.check("at_ref is cached", Ownership.at_ref(tree.root, "base", show) is old and len(shown) == 2, True)
        c.check("an absent ref is None", Ownership.at_ref(tree.root, "nope", show), None)
        lint = stylelint.Ownership.load(tree.root)
        c.check("a subclass gets its own cached instance", (type(lint).__name__, hasattr(lint, "gaps")), ("Ownership", True))
        c.check("... the same answers", lint.resolve("one_fn"), Ownership.load(tree.root).resolve("one_fn"))


def test_covering_agrees_with_every_retired_search(c):
    """The retired interval searches (copied from the tools at `ec2609b46`) against `covering` on a fixture with
    gaps, touching ranges and an overlap, at every boundary and every byte between."""
    text = ("a.c:\n\t.text start:0x100 end:0x200\n\t.data start:0x900 end:0x920\n"
            "b.c:\n\t.text start:0x200 end:0x280\n"
            "c.c:\n\t.text start:0x300 end:0x400\n\t.data start:0x920 end:0x940\n"
            "d.c:\n\t.text start:0x380 end:0x390\n")
    sp = Splits.parse(text)
    by_section = sp.by_section()
    blocks = [{"unit": b.unit, "ranges": [{"section": r.section, "start": r.start, "end": r.end, "rename": r.rename}
                                          for r in b.ranges]} for b in sp.blocks]
    data_starts = {r.start: (r.unit, r.end) for r in sp.ranges if r.section == ".data"}
    own = stylelint.Ownership({}, by_section)

    def stylelint_resolve(section, address):          # stylelint.Ownership.resolve:528 (linear, sorted)
        for start, end, unit in sorted(by_section.get(section, [])):
            if start <= address < end:
                return unit
        return None

    def poolseams_owner_of(ranges, section, address):  # poolseams.owner_of:80
        for start, end, unit in ranges.get(section, ()):
            if start <= address < end:
                return unit
        return None

    def dataorder_unit_of(ranges, addr):               # dataorder.unit_of:290
        for start, (unit, end) in ranges.items():
            if start <= addr < end:
                return unit, start, end
        return None

    def datagap_classify(section, address, ranges, unit):   # datagap.classify_address:263 (owner part)
        for start, end, owner in ranges.get(section, []):
            if start <= address < end:
                return owner
        return None

    disagree = []
    for section in (".text", ".data"):
        for address in range(0xF0, 0x960):
            hit = sp.covering(section, address)
            want = stylelint_resolve(section, address)
            got = hit.unit if hit else None
            pre = symbolpreflight.covering(blocks, section, address)
            others = {
                "stylelint.resolve": want,
                "stylelint._covering_range": (lambda r: r and [u for s, e, u in by_section[section] if (s, e) == r][0])(
                    stylelint._covering_range(own, section, address)),
                "Ownership.covering": (own.covering(section, address) or (None, None, None))[2],
                "symbolpreflight.covering": pre["unit"] if pre else None,
                "poolseams.owner_of": poolseams_owner_of(by_section, section, address),
                "datagap.classify_address": datagap_classify(section, address, by_section, "a.c"),
            }
            if section == ".data":
                hit2 = dataorder_unit_of(data_starts, address)
                others["dataorder.unit_of"] = hit2[0] if hit2 else None
            for name, value in others.items():
                if value != got:
                    disagree.append((name, section, hex(address), value, got))
    c.check("covering agrees with every retired search at every byte", disagree[:5], [])
    c.check("... and the overlap case is the first in address order", sp.covering(".text", 0x385).unit, "c.c")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
