"""unittry: the spec formats, the edit kinds (counted replace, guarded site sweep, block permutation with the
split initialiser), the keep-only-when-better verdict, and the files restored byte-for-byte and mtime-for-mtime
after a losing candidate, a failed compile and an interrupt."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os

from tools.lib import testing
from tools.lib import units
from tools.lib.binary.elf import Elf
from tools.units import unittry as tv

TIER = "fixture"

#: A stand-in for MWCC: `-o DIR -c SRC` writes `DIR/<stem>.o` carrying the source in `.note.src`; `BROKEN` fails.
STUB_CC = r'''
import os, sys
sys.path.insert(0, os.environ["TRYVAR_TOOLS_ROOT"])
from tools.lib.binary.build import ElfBuilder
args = sys.argv[1:]
out, src = args[args.index("-o") + 1], args[args.index("-c") + 1]
text = open(src, encoding="utf-8").read()
if "BROKEN" in text:
    print("Error: syntax error")
    sys.exit(1)
blob = ElfBuilder().section(".text", b"\0" * 8).section(".note.src", text.encode()).build()
with open(os.path.join(out, os.path.splitext(os.path.basename(src))[0] + ".o"), "wb") as fh:
    fh.write(blob)
'''

SOURCE = """\
int helper(void) {
    return 1;
}

void Demo::step(int mode,
                int x)
{
    int a = 1;
    Demo::Kind* k = lookup(a);
    int b;
    switch (mode) {
    case 1:
        if (x) {
            return;
        }
        break;
    case 2:
        while (x) {
            return;
        }
    }
}

void tail_after(int mode) {
    switch (mode) {
    case 1:
        return;
    }
    finish();
}
"""


def scores_of(text: str) -> dict:
    """The stub's metric, read off the compiled text: `step` rises with each `break;` that replaced the guarded
    `return;` and is 100 % with `b` declared first; `helper` falls when it returns 2; `tail_after` is fixed."""
    step = 40.0
    if "{\n    int b;\n    int a = 1;" in text or "{\n    int b;\n    int a;" in text:
        step = 100.0
    elif "            break;\n        }\n        break;" in text:
        step = 60.0
    helper = 50.0 if "return 2;" in text else 80.0
    if "return 3;" in text:
        helper, step = 90.0, step + 5.0
    if "return 4;" in text:                   # the unit rises (+20 B on step) while helper falls (-0.8 B)
        helper, step = 70.0, step + 20.0
    return {"helper": (helper, 8), "Demo::step": (step, 100), "tail_after": (70.0, 20)}


class FakeMeasure:
    """A measurer that reads the file the run wrote (so a candidate that is not on disk scores as the source)."""

    def __init__(self, path, interrupt_at=None):
        self.path, self.calls, self.interrupt_at = path, 0, interrupt_at

    def __call__(self):
        self.calls += 1
        if self.interrupt_at and self.calls == self.interrupt_at:
            raise KeyboardInterrupt
        text = open(self.path, encoding="utf-8", newline="").read().replace("\r\n", "\n")
        if "BROKEN" in text:
            return tv.Measurement(False, error="Error: syntax error")
        funcs = scores_of(text)
        return tv.Measurement(True, funcs, sum(p * s for p, s in funcs.values()) / 128.0)


def make_runner(tree, text=SOURCE, crlf=False, interrupt_at=None, measure=None):
    tree.add_unit("demo/unit.cpp", source=text.replace("\n", "\r\n") if crlf else text,
                  ranges={".text": (0x80004000, 0x80004100)})
    unit = units.Unit.resolve("demo/unit", str(tree.root))
    ws = tv.Workspace({os.path.normpath(unit.source): "source"})
    m = measure or FakeMeasure(unit.source, interrupt_at)
    runner = tv.Runner(ws, m, unit, str(tree.root), out=lambda _s: None)
    runner.baseline()
    return unit, runner


def test_block_and_json_specs(c):
    spec = ("//@@ replace count=2\n    return;\n//@@ with\n    break;\n"
            "//@@ trial pair\n//@@ replace\nint a;\n//@@ with\nint b;\n//@@ replace all file=src/demo/unit.h\nx\n"
            "//@@ with\ny\n"
            "//@@ trial sweep\n//@@ sites guard=return-break function=Demo::step\nreturn;\n//@@ with\nbreak;\n"
            "//@@ trial perm\n//@@ permute function=Demo::step split-init max=7\n    int a = 1;\n    int b;\n")
    trials = tv.parse_blocks(spec)
    c.check("an edit before any trial line is a trial of its own; a trial collects its replaces",
            [(t.name, t.kind, len(t.replaces)) for t in trials],
            [("replace-1", "replace", 1), ("pair", "replace", 2), ("sweep", "sites", 0), ("perm", "permute", 0)])
    c.check("count=2 and all read as counts", (trials[0].replaces[0].count, trials[1].replaces[1].count), (2, None))
    c.check("bodies are verbatim, the one trailing newline dropped", trials[0].replaces[0].old, "    return;")
    c.check("file= names the edited file", trials[1].replaces[1].file, "src/demo/unit.h")
    c.check("sites options", (trials[2].sites.guard, trials[2].sites.function), ("return-break", "Demo::step"))
    c.check("permute options and block", (trials[3].permute.split_init, trials[3].permute.cap,
                                           trials[3].permute.block), (True, 7, "    int a = 1;\n    int b;"))
    c.raises("a replace with no with block is refused", SystemExit, tv.parse_blocks, "//@@ replace\na\n")
    c.raises("an unknown guard is refused", SystemExit, tv.parse_blocks,
             "//@@ sites guard=nope\na\n//@@ with\nb\n")
    c.raises("a sweep in a trial that already replaces is refused", SystemExit, tv.parse_blocks,
             "//@@ trial t\n//@@ replace\na\n//@@ with\nb\n//@@ sites\nc\n//@@ with\nd\n")
    js = tv.parse_json({"trials": [{"name": "r", "replace": [["a", "b"], {"old": "c", "new": "d", "count": "all"}]},
                                   {"name": "s", "sites": {"regex": "return;", "repl": "break;",
                                                           "guard": "return-break"}},
                                   {"name": "p", "permute": {"function": "f", "lines": [3, 5], "hill": True}}]})
    c.check("JSON: the same three kinds", [(t.name, t.kind) for t in js], [("r", "replace"), ("s", "sites"),
                                                                          ("p", "permute")])
    c.check("JSON: pair and object replaces, count all", [(r.old, r.count) for r in js[0].replaces],
            [("a", 1), ("c", None)])
    c.check("JSON: lines as a list", (js[2].permute.lines, js[2].permute.hill), ((3, 5), True))
    c.raises("JSON: two kinds in one trial is refused", SystemExit, tv.parse_json,
             [{"replace": [["a", "b"]], "sites": {"regex": "a", "repl": "b"}}])


def test_functions_statements_and_the_guard(c):
    c.check("a multi-line signature with a qualified name, by its qualified, plain and mangled names",
            {tv.function_body(SOURCE, n) for n in ("Demo::step", "step", "step__4DemoFii")},
            {tv.function_body(SOURCE, "Demo::step")})
    c.raises("an absent function is refused", tv.Refused, tv.function_body, SOURCE, "nothing")
    two = "void A::f(void) {\n}\nvoid B::f(void) {\n}\n"
    c.raises("a name two definitions share is refused (qualify it)", tv.Refused, tv.function_body, two, "f")
    c.check("... and the qualified name picks one", tv.function_body(two, "B::f"),
            (two.index("{", 20), two.index("}", 20)))
    verdicts = []
    for line, needle in ((14, "            return;\n        }\n        break;"), (19, "            return;\n        }\n    }"),
                         (27, "        return;\n    }\n    finish")):
        pos = SOURCE.index(needle)
        verdicts.append(tv.return_break_safe(SOURCE, pos, tv.function_body_at(SOURCE, pos))[0])
    c.check("return->break: safe in a switch that ends the function; a loop inside the case and code after the "
            "switch are refused", verdicts, [True, False, False])
    loop = "void f(int m) {\n    while (1) {\n        switch (m) {\n        case 1:\n            return;\n        }\n    }\n}\n"
    pos = loop.index("return;")
    c.check("... and a switch inside a loop is refused", tv.return_break_safe(loop, pos, tv.function_body_at(loop, pos)),
            (False, "the switch sits inside another loop"))
    units_ = tv.statement_units("    int a = f(1,\n              2);\n    // note\n    b = 3;\n")
    c.check("a statement runs to its closing line; a comment goes with the next one", units_,
            ["    int a = f(1,\n              2);\n", "    // note\n    b = 3;\n"])
    c.raises("a block with an else is refused", tv.Refused, tv.statement_units,
             "    if (a) {\n        b();\n    }\n    else {\n        c();\n    }\n")
    c.check("split initialiser: T* x = f(); -> T* x; + x = f();",
            tv.split_initialiser("    Demo::Kind* k = lookup(a);\n"), ("    Demo::Kind* k;\n", "    k = lookup(a);\n"))
    c.check("... a const one is not split", tv.split_initialiser("    const int k = 3;\n"), None)
    _s, _e, _u, cands, deps = tv.permutations(SOURCE, tv.Permute("Demo::step"))
    c.check("the leading run keeps k after a (its initialiser reads a)", (deps, [n for n, _b in cands]),
            (((), (0,), ()), ["order-0,2,1", "order-2,0,1"]))
    _s, _e, _u, cands, _d = tv.permutations(SOURCE, tv.Permute("Demo::step", split_init=True))
    c.check("split-init: the identity order is a candidate too, the assignments after every declaration",
            cands[0], ("split-0,1,2", "    int a;\n    Demo::Kind* k;\n    int b;\n    a = 1;\n    k = lookup(a);\n"))
    c.check("... and every declaration order is tried", len(cands), 6)


def test_replace_keeps_a_win_and_restores_a_loss(c):
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree, crlf=True)
        before = pathlib.Path(unit.source).read_bytes()
        os.utime(unit.source, ns=(1_600_000_000_000_000_000, 1_600_000_000_000_000_000))
        mtime = os.stat(unit.source).st_mtime_ns
        trials = tv.parse_blocks("//@@ trial loses\n//@@ replace\n    return 1;\n//@@ with\n    return 2;\n"
                                 "//@@ trial broken\n//@@ replace\n    return 1;\n//@@ with\n    BROKEN\n"
                                 "//@@ trial net-up\n//@@ replace\n    return 1;\n//@@ with\n    return 4;\n"
                                 "//@@ trial twice\n//@@ replace\n            return;\n//@@ with\n            break;\n"
                                 "//@@ trial twice-counted\n//@@ replace count=2\n            return;\n//@@ with\n"
                                 "            break;\n")
        runner.execute(trials[:3])
        c.check("a losing edit, a failed compile and a net gain that drops one row are all reverted",
                [r.verdict for r in runner.rows], ["reverted", "failed", "reverted"])
        c.check("... the net gain is refused for its dropped row, not for the total",
                (runner.rows[2].delta > 0, runner.rows[2].why.startswith("1 row(s) drop: helper")), (True, True))
        c.check("... the file is byte-identical (CRLF kept)", pathlib.Path(unit.source).read_bytes(), before)
        c.check("... and has its old mtime, so ninja sees nothing", os.stat(unit.source).st_mtime_ns, mtime)
        runner.rows.clear()
        runner.execute(trials[3:])
        c.check("an old that occurs twice is refused by the default count of one, with its lines",
                (runner.rows[0].verdict, "lines 14, 19" in runner.rows[0].why), ("skip", True))
        c.check("count=2 replaces both; the unit rises and no row drops, so it is kept", runner.rows[1].verdict, "KEPT")
        after = pathlib.Path(unit.source).read_bytes()
        c.check("the kept edit is on disk with the file's CRLF",
                (after.count(b"            break;\r\n"), b"\n" in after.replace(b"\r\n", b"")), (2, False))


def test_sites_sweep_with_the_guard(c):
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree)
        runner.execute(tv.parse_blocks("//@@ sites guard=return-break\nreturn;\n//@@ with\nbreak;\n"))
        verdicts = [(r.name.split("@")[1], r.verdict) for r in runner.rows]
        c.check("each match is a site of its own; the guard skips the two it cannot prove, uncompiled",
                verdicts, [("L14", "KEPT"), ("L19", "guarded"), ("L27", "guarded")])
        c.check("only the guarded-safe site compiled (baseline + 1)", runner.measure.calls, 2)
        text = pathlib.Path(unit.source).read_text()
        c.check("the kept site is written, the guarded ones untouched",
                (text.count("break;"), "while (x) {\n            return;" in text), (2, True))
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree)
        runner.execute(tv.parse_blocks("//@@ sites\nreturn;\n//@@ with\nbreak;\n"))
        c.check("without the guard every site compiles (the stub cannot tell them apart)", runner.measure.calls, 4)


def test_permute_keeps_the_best_order(c):
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree)
        runner.execute([tv.Trial("p", permute=tv.Permute("Demo::step", split_init=True))])
        kept = [r.name for r in runner.rows if r.verdict == "KEPT"]
        c.check("split-init finds the order with b first (the stub's 100 %)", kept, ["p:split-2,0,1"])
        c.check("... after trying all six split orders (other rows are below 100 %, so no early stop)",
                runner.measure.calls, 1 + 6)
        c.contains("the winner is on disk", pathlib.Path(unit.source).read_text(),
                   "    int b;\n    int a;\n    Demo::Kind* k;\n    a = 1;\n")
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree)
        runner.execute([tv.Trial("h", permute=tv.Permute("Demo::step", hill=True))])
        c.check("hill: b moved first in one accepted step", [r.name for r in runner.rows if r.verdict == "KEPT"],
                ["h:order-2,0,1"])


def test_interrupt_restores_the_accepted_state(c):
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree, interrupt_at=3)
        before = pathlib.Path(unit.source).read_bytes()
        trials = tv.parse_blocks("//@@ trial win\n//@@ replace count=2\n            return;\n//@@ with\n"
                                 "            break;\n//@@ trial cut\n//@@ replace\n    return 1;\n//@@ with\n"
                                 "    return 3;\n")
        interrupted = runner.execute(trials)
        text = pathlib.Path(unit.source).read_text()
        c.check("Ctrl-C mid-candidate is reported", interrupted, True)
        c.check("... the in-flight candidate is reverted and the kept one stays",
                ("return 3;" in text, text.count("            break;")), (False, 2))
    with testing.FixtureTree() as tree:
        unit, runner = make_runner(tree)
        before = pathlib.Path(unit.source).read_bytes()
        runner.execute(trials[:1], restore=True)
        c.check("--restore: a kept edit is put back at the end too", pathlib.Path(unit.source).read_bytes(), before)
        c.check("... while the row still says what won", [r.verdict for r in runner.rows], ["KEPT"])


def test_headers_own_and_shared(c):
    with testing.FixtureTree() as tree:
        tree.write("src/demo/unit.h", "int own;\n")
        tree.write("src/demo/shared.h", "int shared;\n")
        text = '#include "demo/unit.h"\n#include "demo/shared.h"\n' + SOURCE
        unit, runner = make_runner(tree, text=text)
        tree.write("src/demo/other.cpp", '#include "demo/shared.h"\n')
        ws, root = runner.ws, str(tree.root)
        own = tv.resolve_file("src/demo/unit.h", unit, root, ws, False, {})
        c.check("the unit's own header may be edited", ws.allowed[own], "own header")
        c.raises("a header another unit includes is refused", SystemExit, tv.resolve_file, "src/demo/shared.h",
                 unit, root, ws, False, {})
        c.check("... unless --shared-header", ws.allowed[tv.resolve_file("demo/shared.h", unit, root, ws, True, {})],
                "shared header (1 other units)")
        c.raises("a file outside the include closure is refused", SystemExit, tv.resolve_file, "src/demo/other.cpp",
                 unit, root, ws, False, {})
        runner.execute(tv.parse_blocks("//@@ trial both\n//@@ replace\n    return 1;\n//@@ with\n    return 3;\n"
                                       "//@@ replace file=src/demo/unit.h\nint own;\n//@@ with\nint own2;\n"))
        c.check("one trial spans the source and its header, kept together",
                (runner.rows[0].verdict, tree.read("src/demo/unit.h")), ("KEPT", "int own2;\n"))


def test_real_measurer_compiles_into_its_scratch(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("demo/unit.cpp", source=SOURCE, ranges={".text": (0x80004000, 0x80004100)})
        stub = tree.write("stub_cc.py", STUB_CC)
        unit = units.Unit.resolve("demo/unit", str(tree.root))
        objdir = str(tree.path("build/tmp/unittry/demo_unit/obj"))
        seen = []

        def score(obj):
            seen.append(obj)
            text = Elf.read(obj).section_bytes(".note.src").decode()
            return scores_of(text), None, None

        m = tv.Measurer([sys.executable, str(stub), "-o", "unused", "-c", unit.source], str(tree.root), objdir,
                        "unused", score)
        os.environ["TRYVAR_TOOLS_ROOT"] = str(testing.LIVE_ROOT)
        try:
            ws = tv.Workspace({os.path.normpath(unit.source): "source"})
            runner = tv.Runner(ws, m, unit, str(tree.root), out=lambda _s: None)
            runner.baseline()
            runner.execute(tv.parse_blocks("//@@ replace\n    return 1;\n//@@ with\n    BROKEN\n"
                                           "//@@ replace count=2\n            return;\n//@@ with\n            break;\n"))
        finally:
            del os.environ["TRYVAR_TOOLS_ROOT"]
        c.check("a failed compile is reverted, the scored win kept", [r.verdict for r in runner.rows],
                ["failed", "KEPT"])
        c.check("every score read the scratch object, never build/RMHE08/src",
                {os.path.dirname(p) for p in seen}, {objdir})
        c.check("the unit's real object was never written", os.path.exists(unit.obj_ours), False)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
