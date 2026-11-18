"""lib.regsim and its CLI tools/mwcc-debugger/regsim.py: a real dump (`wpadSendReadMemory`) replays exactly and a corrupted
register is reported, the stalled parameter is marked, select's register order, the per-vreg attribution of retail's
registers, the edit and the declaration order the search finds, the source's locals and their scopes."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import importlib
from pathlib import Path

from tools.lib import localedit, regsim, testing

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


def clique(costs: dict[int, int]) -> regsim.Graph:
    """19 call-crossing nodes that all interfere: one more than r14..r31, so simplify stalls and one node spills."""
    vs = sorted(costs)
    return regsim.Graph({v: regsim.Node(v, 0, "", (), costs[v], (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)
                                        + tuple(m for m in vs if m != v)) for v in vs})


def test_stall_pick(c):
    costs = {v: 100 for v in range(32, 51)}
    costs[40] = 10
    res = regsim.simulate(clique(costs))
    c.check("a stall pushes the lowest cost / degree (r40), not the first remaining node (r32)",
            [(p.vreg, p.forced) for p in res.pushes if p.forced], [(40, True)])
    c.check("... so r40 is coloured last and spills; the others take r31..r14",
            (res.spilled, sorted(res.colour[v] for v in costs if v != 40)), ([40], list(range(14, 32))))
    costs[44] = 10
    res = regsim.simulate(clique(costs))
    c.check("a tie goes to the highest vreg", res.spilled, [44])
    res = regsim.simulate(clique(costs), order=[44] + [v for v in sorted(costs) if v != 44])
    c.check("... i.e. to the latest in the scan order", res.spilled, [40])
    costs = {v: 100 for v in range(32, 52)}
    costs.update({40: 10, 44: 20})
    res = regsim.simulate(clique(costs))
    c.check("a 20-clique stalls twice, cheapest first, and both spill (no r13, no r12)",
            ([p.vreg for p in res.pushes if p.forced], res.spilled), ([40, 44], [40, 44]))


#: a pass's PCode: loop weight, mnemonic, operands (`fixup[...]` is a relocated immediate)
COST_PCODE = """; function f, dump 19 after-interference-graph
B1: Successors = { B2 }  Predecessors = { }  Labels = { L1 }  line=1 pcode=5 offset=0x0
     1  mr       r32,r3
     1  li       r33,5
     1  li       r34,0
     1  lis      r35,fixup[0x0000]
     1  addi     r36,r35,fixup[0x0000]
B2: Successors = { B2 }  Predecessors = { B1 }  Labels = { L2 }  line=8 pcode=6 offset=0x0
     8  add      r37,r32,r33
     8  stw      r37,r32,0
     8  li       r34,1
     8  cmpli    cr0,r34,1
     8  lbzx     r5,r36,r37
     8  bt       cr0,2,B2
"""


def test_spill_costs(c):
    costs = regsim.spill_costs(COST_PCODE)
    c.check("a def weighs its loop weight once, a use twice; a store's and a compare's registers are uses",
            (costs[32], costs[37]), (1 + 2 * 8 + 2 * 8, 8 + 2 * 8 + 2 * 8))
    c.check("a single `li` def is rematerialised: its uses' weight less its def's", costs[33], 8 - 1)
    c.check("... not a web with two defs", costs[34], 1 + 8 + 2 * 8)
    c.check("... and a `lis`/`addi fixup` address is rematerialised too", (costs[35], costs[36]), (1 - 1, 8 - 1))
    with testing.temp_dir() as tmp:
        d = write_dump(Path(tmp) / "d")
        for n, name in ((18, "interference-graph"), (19, "interference-graph"), (20, "regalloc")):
            (d / ("backend-%d-after-%s.txt" % (n, name))).write_text(COST_PCODE.replace("r32", "r42"), encoding="utf-8")
        c.check("pass 1 of a one-pass dump reads the last interference-graph dump",
                regsim.cost_input(d, 1).name, "backend-19-after-interference-graph.txt")
        g = regsim.load(d)
        c.check("a dump that printed no cost gets the computed ones", (g.cost_source, g.nodes[42].cost),
                ("computed", 1 + 2 * 8 + 2 * 8))
        text = (d / "regalloc-gpr-pass-1-all.txt").read_text(encoding="utf-8")
        (d / "regalloc-gpr-pass-1-all.txt").write_text(text.replace("cost: 0", "cost: 3", 1), encoding="utf-8")
        g = regsim.load(d)
        c.check("... a dump with costs keeps its own", (g.cost_source, g.nodes[32].cost, g.nodes[42].cost),
                ("dump", 3, 0))


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


TWO_OWNERS = """struct A { int create(int a); };
struct B { int create(int a, int b); };

int A::create(int a)
{
    return a;
}

int B::create(int a, int b)
{
    return a + b;
}
"""


def test_find_definition_by_owner(c):
    params = lambda text, fn: (lambda d: None if d is None else regsim.param_count(d))(regsim.find_definition(text, fn))
    c.check("a plain name two classes share names no definition", params(TWO_OWNERS, "create"), None)
    c.check("a mangled name picks its class's definition", params(TWO_OWNERS, "create__1AFi"), 1)
    c.check("... and the other class's", params(TWO_OWNERS, "create__1BFii"), 2)
    c.check("Class::method picks the same", params(TWO_OWNERS, "B::create"), 2)
    c.check("a class that defines none finds none", params(TWO_OWNERS, "create__1CFi"), None)


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
    new = localedit.NewLocal(40, "p", "int* p;", 0)
    plan3 = regsim.Plan(regsim.Layout([32], [34, 40, 33], [], ("declare r40(@1) as a local at declaration position 1",)),
                        (2, 2), (0, 0), {}, [], 4)
    c.check("a new local joins the declaration run at the plan's position",
            cli.rewrite_runs(plan3, base, mapping, REORDER, {40: new}),
            [("    int x = a;\n    int y;", "    int x;\n    int* p;\n    int y;\n    x = a;")])


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
        rc, out = testing.capture(cli.main, ["search", str(ok), "--params", "5", "--want", "r32=r30,r40=r31",
                                             "--no-edits"])
        c.check("search exits 0 when it prints its plans, exact or not", rc, 0)
        c.contains("... and says no plan is exact", out, "no plan reproduces every callee-saved target")
        rc, _ = testing.capture(cli.main, ["search", str(ok), "--params", "5", "--want", "r32=r30,r40=r31",
                                           "--no-edits", "--require-exact"])
        c.check("search --require-exact exits 1 when no declaration order does", rc, 1)
        rc, out = testing.capture(cli.main, ["search", str(ok), "--params", "5", "--want", "r32=r30,r40=r31,r198=r29"])
        c.check("a target that is no node of the pass's graph does not crash the search", rc, 0)
        c.contains("... and is named in a note", out, "not used as a target: r198")
        c.contains("... while the plan still reaches the rest", out, "EXACT")
        c.raises("search refuses without the parameter count", SystemExit, cli.main,
                 ["search", str(ok), "--want", "r32=r30"])


BLOCKS = """extern "C" void take(int* p);
extern "C" void o(int n)
{
    int a;
    int b = n;

    for (a = 0; a < n; a++) {
        int other = a;
        take(&other);
    }
    take(&b);
}
"""


def test_apply_order(c):
    """`--apply-order`: the best plan that lists block-scope locals is spelled at function scope; a plan with an edit or a
    plan of function-scope locals only is refused; --dry-run writes and measures nothing."""
    import argparse
    g = regsim.Graph({32: regsim.Node(32, 31, "n"), 35: regsim.Node(35, 30, "a"), 34: regsim.Node(34, 29, "b"),
                      33: regsim.Node(33, 28, "other")})
    mapping = regsim.map_locals(g, 1, regsim.source_locals(BLOCKS, "o"))
    base = regsim.Layout([32], [35, 34, 33], [])
    hoist = regsim.Plan(base.with_decl([33, 35, 34]), (3, 3), (0, 0), {}, [], 2)
    plain = regsim.Plan(base.with_decl([34, 35]), (3, 3), (0, 0), {}, [], 1)
    edited = regsim.Plan(regsim.Layout([32], [33, 35], [], ("declare r40(-) as a local at declaration position 1",)),
                         (3, 3), (0, 0), {}, [], 3)
    args = argparse.Namespace(unit="x/y", function="o", dry_run=True)
    rc, out = testing.capture(cli.cmd_apply_order, args, g, [hoist, plain], base, mapping, BLOCKS)
    c.check("a hoistable plan is written to the screen with --dry-run, exit 0", rc, 0)
    c.contains("the summary names the hoisted local", out, "hoist `other` to function scope and declare 3 local(s)")
    c.contains("the new order is shown", out, "+     int other;\n  +     int a;\n  +     int b;")
    c.contains("... and the block keeps the use as an assignment", out, "+         other = a;")
    c.contains("--dry-run says nothing was measured", out, "--dry-run, nothing written or measured")
    rc, out = testing.capture(cli.cmd_apply_order, args, g, [plain], base, mapping, BLOCKS)
    c.check("a plan of function-scope locals only is refused with exit 3", rc, 3)
    c.contains("... saying the order alone is --tryvar's", out, "--tryvar's")
    rc, out = testing.capture(cli.cmd_apply_order, args, g, [edited], base, mapping, BLOCKS)
    c.check("a plan that needs an edit is refused", (rc, "every plan needs an edit" in out), (3, True))
    c.raises("it needs a unit", SystemExit, cli.cmd_apply_order, argparse.Namespace(unit=None, function="o", dry_run=True),
             g, [hoist], base, mapping, BLOCKS)
    import contextlib, io
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        try:
            cli.main(["run", "--help"])
        except SystemExit:
            pass
    c.contains("run --help says --no-search prints no plans", " ".join(buf.getvalue().split()), "prints no plans")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
