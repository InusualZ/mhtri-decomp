"""lib.regsim and its CLI tools/mwcc-debugger/regsim.py: a real dump (`wpadSendReadMemory`) replays exactly and a corrupted
register is reported, the stalled parameter is marked, select's register order, the per-vreg attribution of retail's
registers, the edit and the declaration order the search finds, the source's locals and their scopes."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import importlib
from pathlib import Path

from tools.lib import regsim, testing

TIER = "fixture"
cli = importlib.import_module("tools.mwcc-debugger.regsim")

#: wpadSendReadMemory's GPR graph (WPAD/wpad, -O4,p), one node per line: vreg, register, name, flags, neighbours
READMEM = """\
32 31 queue - 0 1 3 4 5 6 7 8 9 10 11 12 33 37 39 40 42 43 46 47 48 49 50 51 52 53 54 56 58 59 62 63 64 66 67 69 70 73 74 75 76 78 79 80 81 83 84 85 86 87 88 89 91 92
33 29 dest - 0 1 3 4 5 6 7 8 9 10 11 12 32 42 43 46 47 48 49 50 51 52 53 54 56 58 59
34 0 size -
35 0 addr -
36 0 callback -
37 0 @4205 - 1 3 32 40 84
38 3 @4198 fCoalesced
39 29 @4197 - 0 1 3 4 5 6 7 8 9 10 11 12 32 40 66 69 70
40 30 @4192 - 0 1 3 4 5 6 7 8 9 10 11 12 32 37 39 62 63 64 66 67 69 70 73 74 75 76 78 79 80 81
41 3 @4188 fCoalesced
42 5 - - 1 3 4 6 7 32 33 43
43 0 - - 1 3 4 5 6 7 32 33 42
44 0 - -
45 0 - -
46 0 - - 1 32 33
47 12 - - 1 32 33 48 49 50 51 52 53 54 56 58 59
48 11 - - 1 32 33 47 49 50 51 52 53 54 56 58 59
49 10 - - 1 32 33 47 48 50 51 52 53 54 56 58 59
50 9 - - 1 32 33 47 48 49 51 52 53 54 56 58 59
51 8 - - 1 32 33 47 48 49 50 52 53 54 56 58 59
52 7 - - 1 32 33 47 48 49 50 51 53 54 56 58 59
53 6 - - 1 32 33 47 48 49 50 51 52 54 56 58 59
54 5 - - 1 32 33 47 48 49 50 51 52 53 56 58 59
55 0 - -
56 4 - - 1 32 33 47 48 49 50 51 52 53 54 58 59
57 0 - -
58 3 - - 1 32 33 47 48 49 50 51 52 53 54 56 59
59 0 - - 1 32 33 47 48 49 50 51 52 53 54 56 58
60 0 - -
61 0 - -
62 4 - - 1 3 32 40 63
63 0 - - 1 3 32 40 62
64 0 - - 1 3 32 40
65 0 - -
66 0 - - 1 3 32 39 40
67 0 - - 1 3 32 40
68 0 - -
69 3 - - 0 1 32 39 40
70 0 - - 1 32 39 40
71 0 - -
72 0 - -
73 3 - - 1 4 5 32 40 74 75 76
74 0 - - 1 4 5 32 40 73
75 0 - - 1 4 5 32 40 73
76 0 - - 1 4 5 32 40 73
77 0 - -
78 3 - - 1 4 5 32 40 79 80 81
79 0 - - 1 4 5 32 40 78
80 0 - - 1 4 5 32 40 78
81 0 - - 1 4 5 32 40 78
82 0 - -
83 6 - - 0 1 3 32 84 85 86 92
84 4 - - 0 1 3 32 37 83
85 4 - - 1 3 32 83 86 92
86 5 - - 1 3 32 83 85 87 92
87 4 - - 1 3 32 86 92
88 4 - - 1 3 32 92
89 4 - - 1 3 32 92
90 0 - -
91 0 - - 1 3 32
92 0 - - 1 3 32 83 85 86 87 88 89
93 0 - -
94 0 - -
"""
READMEM_ASSIGNED = ("32 94 93 92 91 90 89 88 87 86 85 84 83 82 81 80 79 78 77 76 75 74 73 72 71 70 69 68 67 66 65 64 63 62 61 "
                    "60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 40 39 37 36 35 34 33")


def dump_text(compact: str, override: dict | None = None) -> tuple[str, str]:
    """(`-all.txt`, `-assigned.txt`) in mwcc_debugger.py's format from the compact rows."""
    rows = {}
    for line in compact.strip().splitlines():
        parts = line.split()
        v, phys, name, flags, nb = int(parts[0]), int(parts[1]), parts[2], parts[3], parts[4:]
        phys = (override or {}).get(v, phys)
        name = "" if name == "-" else " " + name
        flags = "" if flags == "-" else " " + flags.replace(",", " ")
        body = "r%d -> r%d%s\n  flags:%s\n  cost: 0\n" % (v, phys, name, flags)
        body += ("  neighbors: %d (%s)\n" % (len(nb), " ".join("r" + n for n in nb))) if nb else "  neighbors: 0\n"
        rows[v] = body
    assigned = "".join(rows[int(v)] for v in READMEM_ASSIGNED.split())
    return "".join(rows[v] for v in sorted(rows)), assigned


def write_dump(directory: Path, override: dict | None = None) -> Path:
    directory.mkdir(parents=True, exist_ok=True)
    all_text, assigned = dump_text(READMEM, override)
    (directory / "regalloc-gpr-pass-1-all.txt").write_text(all_text, encoding="utf-8")
    (directory / "regalloc-gpr-pass-1-assigned.txt").write_text(assigned, encoding="utf-8")
    return directory


def test_replay_reproduces_the_dump(c):
    with testing.temp_dir() as tmp:
        g = regsim.load(write_dump(Path(tmp) / "ok"))
        res = regsim.simulate(g)
        c.check("61 nodes are simulated (the two coalesced ones are not)", len(g.virtual()), 61)
        c.check("every register and the select order reproduce the dump", regsim.verify(g, res), [])
        c.check("the parameter `queue` stalls (pushed in scan 2) and is coloured first",
                (res.order[0], res.push_of(32).stalled, res.push_of(32).scan), (32, True, 2))
        c.check("... and takes r31; @4192 r30, @4197 and dest r29",
                [res.colour[v] for v in (32, 40, 39, 33)], [31, 30, 29, 29])
        bad = regsim.load(write_dump(Path(tmp) / "bad", {40: 28}))
        div = regsim.verify(bad, regsim.simulate(bad))
        c.check("a corrupted register is a divergence", [(d.kind, d.vreg, d.model, d.dump) for d in div],
                [("colour", 40, "r30", "r28")])


def test_select_order(c):
    nodes = {
        32: regsim.Node(32, 0, "a", (), 0, (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 33, 34)),
        33: regsim.Node(33, 0, "b", (), 0, (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 32, 34)),
        34: regsim.Node(34, 0, "c", (), 0, (32, 33)),
        35: regsim.Node(35, 0, "d", (), 0, (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)),
    }
    res = regsim.simulate(regsim.Graph(nodes))
    c.check("select pops the highest number first", res.order, [35, 34, 33, 32])
    c.check("a call-crossing node takes r31, a volatile one r0, the next crossing one reuses r31 (no neighbour holds "
            "it), the last one a new r30",
            [res.colour[v] for v in (35, 34, 33, 32)], [31, 0, 31, 30])
    c.check("`-> none` parses as -1", regsim.parse_nodes("r78 -> none\n  flags: fCoalesced\n  neighbors: 0\n")[78].phys,
            -1)


BEFORE = """; function f, dump 20 after-interference-graph
B1: Successors = { }  Predecessors = { }  Labels = { L1 }  line=1 pcode=3 offset=0x0
     1  mr       r40,r3
     1  add      r3,r40,r41
     1  mr       r3,r40
"""
AFTER = BEFORE.replace("r40", "r31").replace("r41", "r30").replace("dump 20 after-interference-graph", "dump 21 after-regalloc")
FINAL = """; function f, dump 39 after-code-labels
B0: Successors = { B1 }  Predecessors = { }  Labels = { L0 }  line=1 pcode=1 offset=0x0
     1  stwu     r1,r1,-0x10
B1: Successors = { }  Predecessors = { B0 }  Labels = { L1 }  line=1 pcode=3 offset=0x4
     1  mr       r31,r3
     1  add      r3,r31,r30
     1  mr       r3,r31
"""


def test_retail_registers_per_vreg(c):
    per = regsim.operand_vregs(BEFORE, AFTER, FINAL)
    c.check("the prologue maps nothing, the block's instructions map their registers",
            per, [{}, {31: 40}, {31: 40, 30: 41}, {31: 40}])
    ours = bytes.fromhex("9421fff0" "7c7f1b78" "7c7ff214" "7fe3fb78")
    retail = bytes.fromhex("9421fff0" "7c7e1b78" "7c7efa14" "387e0000")
    pairs, used = regsim.vreg_pairs(ours, retail, per)
    c.check("a renamed register pairs per vreg; an opcode difference is never read", (pairs, used),
            ({40: {30: 2}, 41: {31: 1}}, 2))
    c.check("the targets", regsim.want_from_vregs(pairs)[0], {40: 30, 41: 31})
    c.check("a vreg retail spells two ways is a note, not a target",
            regsim.want_from_vregs({40: {30: 2, 29: 1}}), ({}, ["r40 is r29 x1, r30 x2 in retail: not used as a target"]))


def test_search(c):
    with testing.temp_dir() as tmp:
        g = regsim.load(write_dump(Path(tmp) / "d"))
        want = {32: 30, 40: 31}
        plans = regsim.Searcher(g, want, 5).run()
        c.check("one edit reproduces the readmem residual: @4192 declared as a local",
                (plans[0].exact, plans[0].layout.edits),
                (True, ("declare r40(@4192) as a local at declaration position 0",)))
        c.check("... and nothing does without an edit", regsim.Searcher(g, want, 5).run(edits=False)[0].exact, False)
    a = (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)
    nodes = {32: regsim.Node(32, 29, "p", (), 0, a + (33, 34)), 33: regsim.Node(33, 30, "y", (), 0, a + (32, 34)),
             34: regsim.Node(34, 31, "x", (), 0, a + (32, 33))}
    g = regsim.Graph(nodes)
    c.check("the dump's numbering: x declared first", regsim.tiers(g, 1), ([32], [34, 33], []))
    best = regsim.Searcher(g, {33: 31, 34: 30}, 1).run()[0]
    c.check("the search swaps two declarations", (best.exact, best.layout.decl, best.distance), (True, [33, 34], 1))


SOURCE = """static int g(int v);

void f(int a, int b)
{
    int x = a;
    int y;
    for (int i = 0; i < b; i++) {
        int z = i;
        g(z);
    }
    {
        int w;
    }
}
"""


def test_source_locals(c):
    locs = regsim.source_locals(SOURCE, "f")
    c.check("declarations in textual order with their scopes",
            [(l.name, l.scope, l.line) for l in locs], [("x", 0, 5), ("y", 0, 6), ("i", 1, 7), ("z", 1, 8), ("w", 2, 12)])
    c.check("two parameters", regsim.param_count(regsim.find_definition(SOURCE, "f")), 2)
    names = {32: "a", 33: "b", 34: "w", 35: "z", 36: "i", 37: "y", 38: "x"}
    g = regsim.Graph({v: regsim.Node(v, 0, n) for v, n in names.items()})
    c.check("the dump's locals map to the declarations (first declared = highest)",
            {v: l.name for v, l in regsim.map_locals(g, 2, locs).items()},
            {38: "x", 37: "y", 36: "i", 35: "z", 34: "w"})
    c.check("an order that splits a block costs a scope break", regsim.scope_breaks([38, 36, 37, 35], {36: 1, 35: 1}),
            regsim.scope_breaks([38, 37, 36, 35], {36: 1, 35: 1}) + 2)


REORDER = """void f(int a)
{
    int x = a;
    int y;
    g(x, y);
}
"""


def test_tryvar_spelling(c):
    nodes = {32: regsim.Node(32, 31, "a"), 33: regsim.Node(33, 30, "y"), 34: regsim.Node(34, 29, "x")}
    g = regsim.Graph(nodes)
    mapping = regsim.map_locals(g, 1, regsim.source_locals(REORDER, "f"))
    base = regsim.Layout([32], [34, 33], [])
    plan = regsim.Plan(base.with_decl([33, 34]), (2, 2), (0, 0), {}, [], 1)
    c.check("a reorder with an initialiser: plain declarations first, the initialisation stays in place",
            cli.rewrite_runs(plan, base, mapping, REORDER),
            [("    int x = a;\n    int y;", "    int y;\n    int x;\n    x = a;")])
    c.check("the source's own order is nothing to write", cli.rewrite_runs(regsim.Plan(base, (2, 2), (0, 0), {}, [], 0),
                                                                            base, mapping, REORDER), [])
    edit = regsim.Plan(regsim.Layout([32], [34, 33], [], ("declare r40(@1) as a local at declaration position 0",)),
                       (2, 2), (0, 0), {}, [], 4)
    c.raises("a plan that needs an edit is not spelled", SystemExit, cli.rewrite_runs, edit, base, mapping, REORDER)


def test_cli(c):
    with testing.temp_dir() as tmp:
        ok = write_dump(Path(tmp) / "ok")
        rc, out = testing.capture(cli.main, ["verify", str(ok)])
        c.check("verify exits 0 on the real dump", rc, 0)
        c.contains("... and says EXACT", out, "EXACT")
        c.contains("... marking the stalled parameter", out, "stalled")
        bad = write_dump(Path(tmp) / "bad", {40: 28})
        rc, out = testing.capture(cli.main, ["verify", str(bad)])
        c.check("verify exits 1 on a corrupted register", rc, 1)
        c.contains("... naming it", out, "DIVERGES colour r40")
        rc, out = testing.capture(cli.main, ["search", str(ok), "--params", "5", "--want", "r32=r30,r40=r31"])
        c.check("search exits 0 when a plan reproduces the targets", rc, 0)
        c.contains("... and names the edit", out, "declare r40(@4192) as a local")
        rc, _ = testing.capture(cli.main, ["search", str(ok), "--params", "5", "--want", "r32=r30,r40=r31",
                                           "--no-edits"])
        c.check("search exits 1 when no declaration order does", rc, 1)
        c.raises("search refuses without the parameter count", SystemExit, cli.main,
                 ["search", str(ok), "--want", "r32=r30"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
