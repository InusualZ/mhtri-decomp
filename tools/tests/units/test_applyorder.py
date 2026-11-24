"""applyorder: the row list parser (commas in the function name, the address column, the pct from the right), the unit-disjoint
partition, each spelling shape of a plan's declaration order (several names on a line, an initialiser with commas, a
`for` header variable, block scope) with its refusals, and the keep-on-strict-gain / restore-byte-exact loop on a CRLF
source with a scripted measure."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os

from tools.lib import regsim, regsimorder as ro, testing, units
from tools.lib import trialrun as tv
from tools.units import applyorder as ao

TIER = "fixture"


def test_rows(c):
    row = ao.parse_row("236,sound/fn_800D7F54,se_frame_req,99.8305")
    c.check("a plain row", (row.size, row.unit, row.function, row.pct), (236, "sound/fn_800D7F54", "se_frame_req", 99.8305))
    row = ao.parse_row("12,Network/x,Foo::operator,(Bar, int),50.5")
    c.check("a function name with commas is cut from the left twice and the pct from the right",
            (row.unit, row.function, row.pct), ("Network/x", "Foo::operator,(Bar, int)", 50.5))
    row = ao.parse_row("1436,g3d/g3d_draw,PrepareIndMtxOp__Q24nw4r3g3dFQ34,99.9777,0x800777B0")
    c.check("the csv's address column is dropped", (row.function, row.pct), ("PrepareIndMtxOp__Q24nw4r3g3dFQ34", 99.9777))
    c.check("a header, a blank line and a comment are no rows",
            [ao.parse_row(x) for x in ("size_bytes,unit,function,match_percent,address", "", "# note")], [None] * 3)
    c.check("a pct that is no number is no row", ao.parse_row("5,u/v,f,abc"), None)
    rows = [ao.Row(1, "a/x", "f%d" % i, 1.0) for i in range(3)] + [ao.Row(1, "b/y", "g", 1.0), ao.Row(1, "c/z", "h", 1.0)]
    lanes = ao.partition(rows, 2)
    c.check("a unit is whole in one list", sorted(sorted({r.unit for r in lane}) for lane in lanes), [["a/x"], ["b/y", "c/z"]])
    c.check("no more lists than units", len(ao.partition(rows, 9)), 3)
    c.check("every row once", sorted(r.function for lane in ao.partition(rows, 3) for r in lane),
            sorted(r.function for r in rows))


def test_declarators(c):
    ds = regsim.declarators("u8* p, q = 2, *r[3];")
    c.check("names, initialisers and arrays are read apart", [(d.name, d.init, d.array) for d in ds],
            [("p", "", ""), ("q", "2", ""), ("r", "", "[3]")])
    c.check("each name keeps its own stars", [d.text() for d in ds], ["u8* p;", "u8 q = 2;", "u8 *r[3];"])
    c.check("a call's commas are not declarators", [d.name for d in regsim.declarators("SeSlot* s = lookup(a, b);")], ["s"])
    c.check("a call statement is no declaration", regsim.declarators("work(a, b);"), [])
    c.check("an assignment list is no declaration", regsim.declarators("a = 1, b = 2;"), [])
    c.check("a keyword statement is no declaration", regsim.declarators("return a, b;"), [])
    locs = regsim.source_locals("void f(int n) {\n    s32 a, b;\n    work(a, b);\n}\n", "f")
    c.check("source_locals reads both names with their line and how many share it",
            [(l.name, l.line, l.shared) for l in locs], [("a", 2, 2), ("b", 2, 2)])


def locals_map(text, fn):
    locs = regsim.source_locals(text, fn)
    return {40 - i: loc for i, loc in enumerate(locs)}, locs


def spell(text, fn, names, label=None):
    """([spellings], [refusals]) of the plan declaring `names` in order."""
    mapping, _locs = locals_map(text, fn)
    vreg = {}
    for v, loc in mapping.items():
        vreg.setdefault(loc.name, v)
    found, refused = ro.spellings([vreg[n] for n in names], mapping, text, fn)
    if label:
        found = [s for s in found if s.label == label]
        refused = [r for r in refused if r[0] == label]
    return found, refused


MULTI = """void f(int n) {
    SeSlot* s = lookup(n, 3);
    s32 a, b;
    u8* p, q = 2;
    work(s, a, b, p, q);
}
"""


def test_multi_name_and_initialiser(c):
    got, _ = spell(MULTI, "f", ["b", "a", "s", "p", "q"], "scoped")
    c.check("a plan moving names of a shared line splits it and swaps the lines", got[0].text,
            "void f(int n) {\n    s32 b;\n    s32 a;\n    SeSlot* s = lookup(n, 3);\n    u8* p;\n    u8 q = 2;\n"
            "    work(s, a, b, p, q);\n}\n")
    c.contains("... and the summary says it split", got[0].summary, "split 2 multi-name line(s)")
    got, _ = spell(MULTI, "f", ["p", "b", "a", "s", "q"], "scoped")
    c.check("the initialised locals keep their order here, so the lines swap", got[0].text.split("\n")[1:7],
            ["    u8* p;", "    s32 b;", "    s32 a;", "    SeSlot* s = lookup(n, 3);", "    u8 q = 2;", "    work(s, a, b, p, q);"])
    got, _ = spell(MULTI, "f", ["q", "p", "b", "a", "s"], "scoped")
    c.check("two initialised locals the plan swaps take the split form", got[0].text.split("\n")[1:7],
            ["    u8 q;", "    u8* p;", "    s32 b;", "    s32 a;", "    SeSlot* s;", "    s = lookup(n, 3);"])
    # an initialiser with commas that must stay in place: the plan moves the initialised declaration after a statement
    split = """void h(int n) {
    int a;
    stmt(n);
    SeSlot* s = lookup(n, 3);
    int d;
}
"""
    got, _ = spell(split, "h", ["d", "s", "a"], "scoped")
    c.check("an initialiser across a statement becomes a declaration up top and an assignment in place", got[0].text,
            "void h(int n) {\n    int d;\n    SeSlot* s;\n    int a;\n    stmt(n);\n    s = lookup(n, 3);\n}\n")
    both = "void k(int n) {\n    SeSlot* s = lookup(n, 3);\n    SeSlot* t = lookup(n, 4);\n}\n"
    got, why = spell(both, "k", ["t", "s"], "scoped")
    c.check("swapping two initialised lines would swap their calls: the split form is written instead", got[0].text,
            "void k(int n) {\n    SeSlot* t;\n    SeSlot* s;\n    s = lookup(n, 3);\n    t = lookup(n, 4);\n}\n")


def test_refusals(c):
    dep = "void f(int n) {\n    int a = n;\n    int b = a + 1;\n}\n"
    got, why = spell(dep, "f", ["b", "a"], "scoped")
    c.check("an initialiser using a local the plan declares after it goes to the split form (assignments stay)", got[0].text,
            "void f(int n) {\n    int b;\n    int a;\n    a = n;\n    b = a + 1;\n}\n")
    const = "void f(int n) {\n    int x;\n    stmt(n);\n    const int c = n;\n    int y;\n}\n"
    got, why = spell(const, "f", ["y", "c", "x"], "scoped")
    c.check("a const initialiser that would cross a statement is refused", (got, "const" in why[0][1].reason),
            ([], True))
    klass = "void f(int n) {\n    Foo a;\n    stmt(n);\n    Foo b;\n}\n"
    got, why = spell(klass, "f", ["b", "a"], "scoped")
    c.check("a class-typed local would move across a statement: refused", (got, "class-typed" in why[0][1].reason), ([], True))
    cls_init = "void f(int n) {\n    int a;\n    stmt(n);\n    Foo b = make(n);\n    int d;\n}\n"
    got, why = spell(cls_init, "f", ["d", "b", "a"], "scoped")
    c.check("a class-typed initialised local is refused in the split form", got, [])
    c.check("... with the reason", "class" in why[0][1].reason, True)
    used = "void f(int n) {\n    int a;\n    int b;\n    stmt(n);\n    int c;\n}\n"
    got, why = spell(used, "f", ["b", "c", "a"], "scoped")
    c.check("a scalar declaration moves across a statement it is used after", len(got), 1)
    c.raises("a plan naming no declared local is refused", ro.Refused, ro.scoped, used, "f", [99, 98], {})
    c.raises("the source's own order is `Nothing`", ro.Nothing, ro.scoped, used, "f",
             list(locals_map(used, "f")[0]), locals_map(used, "f")[0])
    c.raises("one local alone is `Few`", ro.Few, ro.scoped, used, "f", [40], {40: locals_map(used, "f")[0][40]})


FORS = """void g(int n) {
    int a;
    int b;
    for (u16 i = 0; i < n; i++) {
        int c = a + i;
        use(c, b);
    }
}
"""


def test_for_and_blocks(c):
    got, why = spell(FORS, "g", ["b", "a", "i", "c"], "scoped")
    c.check("a `for` variable the plan keeps in place does not stop the scope's reorder", got[0].text.split("\n")[1:3],
            ["    int b;", "    int a;"])
    got, why = spell(FORS, "g", ["a", "i", "b", "c"], "scoped")
    c.check("a plan that declares the `for` variable between function-scope locals is not `scoped`'s",
            (got, isinstance(why[0][1], ro.Nothing)), ([], True))
    got, why = spell(FORS, "g", ["a", "i", "b", "c"], "hoist+for")
    c.check("... `hoist+for` writes `u16 i;` in the plan's place and `for (i = 0;`", got[0].text,
            "void g(int n) {\n    int a;\n    u16 i;\n    int b;\n    int c;\n    for (i = 0; i < n; i++) {\n"
            "        c = a + i;\n        use(c, b);\n    }\n}\n")
    plain, _ = spell(FORS, "g", ["a", "i", "b", "c"], "hoist")
    c.check("plain `hoist` leaves the header variable in its block", "for (u16 i = 0;" in plain[0].text, True)
    twice = FORS.replace("}\n}\n", "}\n    for (u16 i = 0; i < n; i++) {\n        use(i);\n    }\n}\n")
    c.check("a name declared twice is not hoisted out of its header", spell(twice, "g", ["a", "i", "b", "c"], "hoist+for")[0], [])
    wide = "void w(int n) {\n    int a;\n    {\n        int p, q;\n        use(p, q);\n    }\n    int b;\n}\n"
    got, _ = spell(wide, "w", ["q", "p", "a", "b"], "hoist")
    c.check("a block's several names are written one per declaration at function scope", got[0].text.split("\n")[1:5],
            ["    int q;", "    int p;", "    int a;", "    int b;"])


SOURCE = "void f(int n) {\r\n    int a;\r\n    int b;\r\n    work(a, b);\r\n}\r\n"


class Plan:
    def __init__(self, decl, distance=1):
        self.distance = distance
        self.layout = type("L", (), {"decl": decl, "edits": ()})()


def test_keep_and_restore(c):
    """A spelling is kept on a strict gain only and every loser is restored byte for byte, line endings included."""
    with testing.FixtureTree() as tree:
        tree.add_unit("demo/unit.cpp", source=SOURCE, ranges={".text": (0x80004000, 0x80004100)})
        unit = units.Unit.resolve("demo/unit", str(tree.root))
        seen = {"wins": "int b;\n    int a;"}

        class Measure:
            def __call__(self):
                text = open(unit.source, encoding="utf-8", newline="").read().replace("\r\n", "\n")
                pct = 90.0 if seen["wins"] in text else 80.0
                return tv.Measurement(True, {"f": (pct, 100), "other": (50.0, 20)}, pct)

        ws = tv.Workspace({os.path.normpath(unit.source): "source"})
        runner = tv.TrialRunner(ws, Measure(), unit, str(tree.root), out=lambda _s: None)
        runner.baseline()
        path = os.path.normpath(unit.source)
        text = ws.text(path)
        mapping, _ = locals_map(text, "f")
        names = {loc.name: v for v, loc in mapping.items()}
        raw = open(unit.source, "rb").read()
        tried, refusals = set(), []
        kept = ao.try_plans(runner, path, text, [Plan([names["a"], names["b"]], 0), Plan([names["b"], names["a"]])],
                            mapping, "f", tried, refusals, False, lambda _s: None)
        c.check("the plan that is the source's own order is skipped, the next one measured and kept", len(kept), 1)
        c.contains("... saying which plan and what it wrote", kept[0], "plan 2 scoped: scope 0: b a")
        c.check("the kept source is on disk with its CRLF endings", open(unit.source, "rb").read(),
                b"void f(int n) {\r\n    int b;\r\n    int a;\r\n    work(a, b);\r\n}\r\n")
        c.check("the score rose", runner.cur.matched - runner.base.matched, 10.0)
        seen["wins"] = "never present"
        later = ws.text(path)
        mapping2, _ = locals_map(later, "f")
        names2 = {loc.name: v for v, loc in mapping2.items()}
        before = open(unit.source, "rb").read()
        tried2, refusals2 = set(), []
        kept2 = ao.try_plans(runner, path, later, [Plan([names2["a"], names2["b"]])], mapping2, "f", tried2, refusals2,
                             False, lambda _s: None)
        c.check("a spelling that gains nothing is not kept", kept2, [])
        c.check("... and the file is back byte for byte", open(unit.source, "rb").read(), before)
        c.check("... the verdict was `reverted`", [r.verdict for r in runner.rows][-1], "reverted")
        shown = []
        kept3 = ao.try_plans(runner, path, later, [Plan([names2["a"], names2["b"]])], mapping2, "f", set(), [], True, shown.append)
        c.check("--dry-run shows the diff and writes nothing", (kept3, "+    int a;" in shown[0], open(unit.source, "rb").read()),
                ([], True, before))
        ws.restore(original=True)
        c.check("restoring the original puts the found bytes back", open(unit.source, "rb").read(), raw)


def test_read_plans(c):
    """The plans `regsim.py run --plans-json` writes come back as plans and a vreg -> declaration mapping."""
    data = {"function": "g", "plans": [{"distance": 2, "edits": ["declare r50(-) as a local"], "decl": [
        {"vreg": 40, "name": "b", "line": 3, "scope": 0}, {"vreg": 39, "name": "a", "line": 2, "scope": 0},
        {"vreg": 50, "name": "r50(-)", "line": None, "scope": None}]}]}
    plans, mapping = ao.read_plans(data, FORS, "g")
    c.check("the plan keeps its order, its cost and its edits", (plans[0].layout.decl, plans[0].distance, plans[0].layout.edits),
            ([40, 39, 50], 2, ("declare r50(-) as a local",)))
    c.check("each named entry is found again in the source's locals; the new local has none",
            {v: (m.name, m.line) for v, m in mapping.items()}, {40: ("b", 3), 39: ("a", 2)})
    got, _ = ro.spellings(plans[0].layout.decl, mapping, FORS, "g", plans[0].layout.edits)
    c.contains("... and the order is written without the edit", got[0].summary, "the plan's edit (declare r50(-)) is not written")
    c.check("an entry whose line moved (a stale dump) is no declaration", ao.read_plans(
        {"function": "g", "plans": [{"distance": 1, "edits": [], "decl": [{"vreg": 1, "name": "a", "line": 9, "scope": 0}]}]},
        FORS, "g")[1], {})


def test_commands(c):
    args = ao.argparse.Namespace(plans=2, rounds=3, top=1, max_evals=50, pass_no=1, seed=0, timeout=60.0, gdb=None, size="0x40",
                                 params=None, want=None, saved_only=True, no_edits=False, dry_run=True, restore=False,
                                 no_refresh=False)
    cmd = ao.child_command(args, ao.Row(1, "u/v", "f", 1.0))
    c.check("a batch row runs as `applyorder.py row UNIT FUNCTION --json` with the options forwarded",
            (cmd[2:6], "--dry-run" in cmd, "--saved-only" in cmd, cmd[cmd.index("--size") + 1]),
            (["row", "u/v", "f", "--json"], True, True, "0x40"))
    cmd = ao.regsim_command(args, "u/v", "f", pathlib.Path("out.json"), "ROOT")
    c.check("a round's search is `regsim.py run` writing the plans, at least --plans of them",
            (cmd[2:4], cmd[cmd.index("--plans-json") + 1], cmd[cmd.index("--top") + 1]), (["run", "u/v"], "out.json", "2"))


def test_run_hands_over(c):
    """`regsim.py run --apply-order` and `--rows-file` become an `applyorder.py row|batch` process with the same options."""
    import importlib
    cli = importlib.import_module("tools.mwcc-debugger.regsim")
    import argparse
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd")
    run = sub.add_parser("run")
    run.add_argument("unit", nargs="?")
    run.add_argument("function", nargs="?")
    cli.run_options(run)
    args = ap.parse_args(["run", "sound/x", "f", "--apply-order", "--plans", "2", "--restore", "--saved-only"])
    cmd = cli.applyorder_command(args, "ROOT")
    c.check("one function is `applyorder.py row UNIT FUNCTION` with its options", (cmd[2:5], cmd[cmd.index("--plans") + 1],
            "--restore" in cmd, "--saved-only" in cmd, "--dry-run" in cmd), (["row", "sound/x", "f"], "2", True, True, False))
    args = ap.parse_args(["run", "--rows-file", "rows.txt", "--apply-order", "--workers", "5"])
    cmd = cli.applyorder_command(args, "ROOT")
    c.check("a row list is `applyorder.py batch ROWS` with --workers", (cmd[2:4], cmd[cmd.index("--workers") + 1]),
            (["batch", "rows.txt"], "5"))


def test_classify(c):
    c.check("only own-order refusals is SAME", ao.classify(["plan 1: the source's own order"], True)[0], "SAME")
    c.check("no plan at all is SAME", ao.classify([], False), ("SAME", "the search found no plan"))
    c.check("a scope with one local is FEWFS", ao.classify(["plan 1 scoped: no scope lists two locals of the plan"], True)[0],
            "FEWFS")
    c.check("a real refusal is BADLINE with its text", ao.classify(["plan 2 scoped: `x` is a `const`"], True),
            ("BADLINE", "plan 2 scoped: `x` is a `const`"))


def test_cli_surface(c):
    import contextlib, io
    import importlib
    cli = importlib.import_module("tools.mwcc-debugger.regsim")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        try:
            cli.main(["run", "--help"])
        except SystemExit:
            pass
    text = " ".join(buf.getvalue().split())
    c.contains("run takes --rows-file", text, "--rows-file")
    c.contains("run takes --workers", text, "--workers")
    c.contains("run takes --plans and --rounds", text, "--plans PLANS")
    c.raises("run with neither a unit and a function nor a row file is a usage error", SystemExit, cli.main, ["run"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
