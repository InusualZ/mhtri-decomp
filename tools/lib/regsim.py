"""MWCC Wii/1.3 GPR colouring: parse a mwcc-debugger regalloc dump, replay simplify/select, search renumberings.
Spec: docs/tools/spec/lib-regsim.md. CLI: none (library; tools/mwcc-debugger/regsim.py is the front end)."""
from __future__ import annotations

import itertools
import math
import random
import re
from dataclasses import dataclass, field
from pathlib import Path

from tools.lib import cextract, cscan, declperm, ppc

#: allocatable GPRs: r0, r3-r12 (volatile) and r14-r31 (callee-saved); r1, r2 and r13 are reserved
K_GPR = 29
#: the order select tries the volatile registers in
VOLATILE = (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)
#: the first virtual register number (lower numbers are physical registers)
FIRST_VIRTUAL = 32
#: below this a callee-saved pick has run out of registers: the node would be spilled
LOWEST_SAVED = 14
#: exhaustive declaration orders up to this many locals (8! = 40320 simulations)
EXHAUSTIVE_MAX = 8

_NODE = re.compile(r"^r(\d+) -> (\S+)[ \t]*(.*)$")
_REGS = re.compile(r"\br(\d+)\b")


# --- the dump ---------------------------------------------------------------------------------------------------

@dataclass
class Node:
    """One interference-graph node as the dump prints it (`phys` is the register the compiler assigned)."""
    vreg: int
    phys: int
    name: str = ""
    flags: tuple[str, ...] = ()
    cost: int = 0
    neighbors: tuple[int, ...] = ()

    @property
    def coalesced(self) -> bool:
        return "fCoalesced" in self.flags

    @property
    def tier(self) -> str:
        """`frontend` for an `@NNNN` temporary, `named` for a source variable, `backend` for an unnamed one."""
        if self.name.startswith("@"):
            return "frontend"
        return "named" if self.name else "backend"

    def label(self) -> str:
        return "r%d(%s)" % (self.vreg, self.name or "-")


@dataclass
class Graph:
    """The GPR interference graph of one function plus the dump's own assignment order."""
    nodes: dict[int, Node]
    assigned: list[int] = field(default_factory=list)
    function: str = ""
    defs: dict[int, str] = field(default_factory=dict)

    def virtual(self) -> list[int]:
        """The simulated nodes: every virtual register that was not coalesced into another, ascending."""
        return [v for v in sorted(self.nodes) if v >= FIRST_VIRTUAL and not self.nodes[v].coalesced]

    def copy(self) -> "Graph":
        return Graph({v: Node(n.vreg, n.phys, n.name, n.flags, n.cost, n.neighbors) for v, n in self.nodes.items()},
                     list(self.assigned), self.function, dict(self.defs))

    def coalesce(self, gone: int, into: int) -> "Graph":
        """A copy where node `gone` is folded into node `into` (union of their interference, `gone` removed)."""
        if gone in self.nodes[into].neighbors:
            raise ValueError("r%d and r%d interfere: they cannot share a register" % (gone, into))
        g = self.copy()
        merged = set(g.nodes[into].neighbors) | set(g.nodes[gone].neighbors)
        merged.discard(gone)
        merged.discard(into)
        g.nodes[into].neighbors = tuple(sorted(merged))
        for v, n in g.nodes.items():
            if gone in n.neighbors and v != into:
                nb = set(n.neighbors)
                nb.discard(gone)
                nb.add(into)
                n.neighbors = tuple(sorted(nb))
        g.nodes[gone].flags = g.nodes[gone].flags + ("fCoalesced",)
        g.assigned = [v for v in g.assigned if v != gone]
        return g


def parse_nodes(text: str) -> dict[int, Node]:
    """`regalloc-gpr-pass-N-all.txt` (or `-assigned.txt`) -> {vreg: Node}."""
    nodes: dict[int, Node] = {}
    cur: Node | None = None
    for line in text.splitlines():
        m = _NODE.match(line)
        if m:
            phys = m.group(2)
            cur = Node(int(m.group(1)), int(phys[1:]) if re.fullmatch(r"r\d+", phys) else -1, m.group(3).strip())
            nodes[cur.vreg] = cur
            continue
        if cur is None:
            continue
        s = line.strip()
        if s.startswith("flags:"):
            cur.flags = tuple(s[len("flags:"):].split())
        elif s.startswith("cost:"):
            try:
                cur.cost = int(s[len("cost:"):].strip())
            except ValueError:
                pass
        elif s.startswith("neighbors:"):
            inner = s.split("(", 1)[1] if "(" in s else ""
            cur.neighbors = tuple(int(x) for x in _REGS.findall(inner))
    return nodes


def parse_assigned(text: str) -> list[int]:
    """The vregs of `regalloc-gpr-pass-N-assigned.txt` in the order the compiler coloured them."""
    return [int(m.group(1)) for m in (_NODE.match(line) for line in text.splitlines()) if m]


def parse_defs(text: str) -> tuple[str, dict[int, str]]:
    """(function name, {vreg: first PCode line naming it}) of a backend dump - how a backend temp is recognised."""
    function = ""
    first = text.splitlines()[0] if text else ""
    m = re.match(r"^; function (.+?)(?:, dump \d+.*)?$", first)
    if m:
        function = m.group(1)
    defs: dict[int, str] = {}
    for line in text.splitlines():
        if not line.startswith(" "):
            continue
        body = " ".join(line.split()[1:])
        for r in _REGS.findall(body):
            v = int(r)
            if v >= FIRST_VIRTUAL and v not in defs:
                defs[v] = body
    return function, defs


def regalloc_input(directory: Path) -> Path | None:
    """The last backend dump before the GPR colouring (the PCode the graph was built from)."""
    dumps = sorted(directory.glob("backend-*.txt"))
    before = None
    for p in dumps:
        if "after-regalloc" in p.name:
            return before
        before = p
    return before


def load(directory: str | Path, pass_no: int = 1, cls: str = "gpr") -> Graph:
    """The graph of `DIR/regalloc-<cls>-pass-<pass_no>-all.txt` with its `-assigned.txt` order and PCode defs."""
    d = Path(directory)
    all_path = d / ("regalloc-%s-pass-%d-all.txt" % (cls, pass_no))
    if not all_path.is_file():
        raise FileNotFoundError("%s: no %s (run tools/mwcc-debugger/mwcc_debugger.py first)" % (d, all_path.name))
    g = Graph(parse_nodes(all_path.read_text(encoding="utf-8", errors="replace")))
    assigned = d / ("regalloc-%s-pass-%d-assigned.txt" % (cls, pass_no))
    if assigned.is_file():
        g.assigned = parse_assigned(assigned.read_text(encoding="utf-8", errors="replace"))
    pcode = regalloc_input(d)
    if pcode is not None:
        g.function, g.defs = parse_defs(pcode.read_text(encoding="utf-8", errors="replace"))
    return g


# --- the colouring ----------------------------------------------------------------------------------------------

@dataclass
class Push:
    """How simplify removed one node: the scan it went in, its degree then, and why."""
    vreg: int
    scan: int
    degree: int
    stalled: bool    # not removable in the first scan it was seen in: coloured before every node pushed earlier
    forced: bool     # pushed by the spill pick because a whole scan made no progress


@dataclass
class Colouring:
    pushes: list[Push]
    colour: dict[int, int]

    @property
    def order(self) -> list[int]:
        """The select order (the compiler's `-assigned.txt` order)."""
        return [p.vreg for p in reversed(self.pushes)]

    def push_of(self, vreg: int) -> Push | None:
        return next((p for p in self.pushes if p.vreg == vreg), None)


def simulate(graph: Graph, order: list[int] | None = None, k: int = K_GPR) -> Colouring:
    """Replay MWCC's simplify/select on `graph`, scanning the nodes in `order` (default: ascending vreg).

    Simplify scans the order repeatedly and pushes every node whose current degree (physical neighbours included)
    is below `k`; a scan with no progress pushes the first remaining node. Select pops in reverse: a node takes the
    first free of r0, r3..r12, else the lowest callee-saved register handed out so far that no coloured neighbour
    holds, else the next new one down from r31."""
    virt = set(graph.virtual())
    seq = [v for v in (order if order is not None else sorted(virt)) if v in virt]
    if len(seq) != len(virt):
        missing = sorted(virt - set(seq))
        seq += missing
    vnb = {v: [m for m in graph.nodes[v].neighbors if m in virt] for v in seq}
    deg = {v: len(graph.nodes[v].neighbors) for v in seq}
    pushes: list[Push] = []
    remaining = seq
    scan = 0
    while remaining:
        scan += 1
        rest = []
        for v in remaining:
            if deg[v] < k:
                pushes.append(Push(v, scan, deg[v], scan > 1, False))
                for m in vnb[v]:
                    deg[m] -= 1
            else:
                rest.append(v)
        if len(rest) == len(remaining):
            v = rest.pop(0)
            pushes.append(Push(v, scan, deg[v], True, True))
            for m in vnb[v]:
                deg[m] -= 1
        remaining = rest
    colour: dict[int, int] = {}
    used: list[int] = []
    for p in reversed(pushes):
        n = graph.nodes[p.vreg]
        forbidden = {m for m in n.neighbors if m < FIRST_VIRTUAL}
        forbidden.update(colour[m] for m in vnb[p.vreg] if m in colour)
        pick = next((r for r in VOLATILE if r not in forbidden), None)
        if pick is None:
            low = min(used) if used else FIRST_VIRTUAL
            pick = next((r for r in range(low, FIRST_VIRTUAL) if r not in forbidden), low - 1)
            used.append(pick)
        colour[p.vreg] = pick
    return Colouring(pushes, colour)


@dataclass
class Divergence:
    kind: str        # "colour" or "order"
    vreg: int
    model: str
    dump: str


def verify(graph: Graph, result: Colouring) -> list[Divergence]:
    """Every node whose simulated register differs from the dump's, then the first select-order difference."""
    out = []
    for v in graph.virtual():
        if result.colour.get(v) != graph.nodes[v].phys:
            out.append(Divergence("colour", v, "r%s" % result.colour.get(v), "r%d" % graph.nodes[v].phys))
    if graph.assigned:
        dump = [v for v in graph.assigned if v in result.colour]
        model = result.order
        for i, (a, b) in enumerate(zip(model, dump)):
            if a != b:
                out.append(Divergence("order", a, "#%d r%d" % (i, a), "#%d r%d" % (i, b)))
                break
        else:
            if len(model) != len(dump):
                out.append(Divergence("order", -1, "%d nodes" % len(model), "%d nodes" % len(dump)))
    return out


# --- retail's registers -----------------------------------------------------------------------------------------

_FP_INDEXED = frozenset({535, 567, 599, 631, 663, 695, 727, 759, 983})
_NO_REG_X = frozenset({339, 467, 371, 19, 144, 598, 854, 86, 470, 982, 1014, 246, 278, 54})


def gpr_fields(word: int) -> tuple[str, ...]:
    """The fields of an instruction word that name a GPR (`rt`, `ra`, `rb`); () for an instruction this does not
    read (branches, float arithmetic, paired singles, special-register moves)."""
    o = ppc.op(word)
    if o in (10, 11):
        return ("ra",)
    if o in (7, 8, 12, 13, 14, 15, 20, 21, 24, 25, 26, 27, 28, 29) or 32 <= o <= 47:
        return ("rt", "ra")
    if o == 23:
        return ("rt", "ra", "rb")
    if o in (48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 60, 61):
        return ("ra",)
    if o == 31:
        x = ppc.xo10(word)
        if x in (0, 32):
            return ("ra", "rb")
        if x in _FP_INDEXED:
            return ("ra", "rb")
        if x in _NO_REG_X:
            return ()
        if x in (824, 597, 725):   # srawi, lswi, stswi: the rb field is an immediate
            return ("rt", "ra")
        return ("rt", "ra", "rb")
    return ()


_SHIFT = {"rt": 21, "ra": 16, "rb": 11}


_BLOCK = re.compile(r"^B(\d+):.*?offset=(0x[0-9a-fA-F]+)")


def parse_blocks(text: str) -> list[tuple[int, int, list[tuple[str, tuple[str, ...]]]]]:
    """[(block number, code offset, [(mnemonic, operands)])] of a backend dump, in dump order."""
    out: list[tuple[int, int, list]] = []
    for line in text.splitlines():
        m = _BLOCK.match(line)
        if m:
            out.append((int(m.group(1)), int(m.group(2), 16), []))
            continue
        if out and line.startswith(" ") and line.strip():
            parts = line.split(None, 2)
            if len(parts) >= 2:
                ops = tuple(t.strip() for t in parts[2].split(",")) if len(parts) > 2 else ()
                out[-1][2].append((parts[1], ops))
    return out


def _regno(token: str) -> int | None:
    m = re.fullmatch(r"r(\d+)", token)
    return int(m.group(1)) if m else None


def operand_vregs(before: str, after: str, final: str) -> list[dict[int, int | None]]:
    """Per instruction of the final dump: {physical GPR: vreg it holds there} (None when two vregs share it in one
    instruction). The colouring's input (`before`) and output (`after`) dumps have the same instructions, so their
    operands pair up; a final instruction inherits them from the identical instruction of its block in `after`.
    An instruction a later pass made (prologue, a fused form) maps nothing."""
    pre, post = parse_blocks(before), parse_blocks(after)
    if [(b, len(i)) for b, _, i in pre] != [(b, len(i)) for b, _, i in post]:
        raise ValueError("the dumps around the colouring do not have the same instructions")
    pool: dict[int, list[tuple[tuple[str, tuple[str, ...]], dict[int, int | None]]]] = {}
    for (blk, _, ins_pre), (_, _, ins_post) in zip(pre, post):
        for (mn, ops_pre), (_, ops_post) in zip(ins_pre, ins_post):
            m: dict[int, int | None] = {}
            for a, b in zip(ops_pre, ops_post):
                v, p = _regno(a), _regno(b)
                if v is None or p is None or v < FIRST_VIRTUAL:
                    continue
                m[p] = v if m.get(p, v) == v else None
            pool.setdefault(blk, []).append(((mn, ops_post), m))
    out: list[dict[int, int | None]] = []
    for blk, offset, ins in sorted(parse_blocks(final), key=lambda b: b[1]):
        cands = pool.get(blk, [])
        for item in ins:
            hit = next((i for i, (key, _) in enumerate(cands) if key == item), None)
            out.append(cands.pop(hit)[1] if hit is not None else {})
    return out


def vreg_pairs(ours: bytes, retail: bytes, per_insn: list[dict[int, int | None]]
               ) -> tuple[dict[int, dict[int, int]], int]:
    """({vreg: {retail GPR: count}}, instructions read): each GPR field of our code word is attributed to the vreg the
    final dump says it holds, and paired with the same field of retail's word (words equal outside their GPR
    fields only)."""
    out: dict[int, dict[int, int]] = {}
    used = 0
    for i, m in enumerate(per_insn):
        if not m or 4 * i + 4 > min(len(ours), len(retail)):
            continue
        a = int.from_bytes(ours[4 * i:4 * i + 4], "big")
        b = int.from_bytes(retail[4 * i:4 * i + 4], "big")
        fields = gpr_fields(a)
        mask = 0
        for f in fields:
            mask |= 31 << _SHIFT[f]
        if (a & ~mask) != (b & ~mask):
            continue
        used += 1
        for f in fields:
            p, r = (a >> _SHIFT[f]) & 31, (b >> _SHIFT[f]) & 31
            v = m.get(p)
            if v is None:
                continue
            out.setdefault(v, {}).setdefault(r, 0)
            out[v][r] += 1
    return out, used


def want_from_vregs(pairs: dict[int, dict[int, int]]) -> tuple[dict[int, int], list[str]]:
    """({vreg: retail register}, notes): the register retail uses for every occurrence; a vreg retail splits over two
    registers is reported, not guessed."""
    want, notes = {}, []
    for v, seen in sorted(pairs.items()):
        if len(seen) == 1:
            want[v] = next(iter(seen))
        else:
            notes.append("r%d is %s in retail: not used as a target"
                         % (v, ", ".join("r%d x%d" % (r, c) for r, c in sorted(seen.items()))))
    return want, notes


def final_dump(directory: Path) -> Path | None:
    """The last backend dump of the run (the code as emitted)."""
    dumps = sorted(directory.glob("backend-*.txt"))
    return dumps[-1] if dumps else None


def after_regalloc(directory: Path) -> Path | None:
    return next(iter(sorted(directory.glob("backend-*-after-regalloc.txt"))), None)


# --- the source -------------------------------------------------------------------------------------------------

@dataclass
class SourceLocal:
    """One local declaration of the function's body, in textual order: `scope` 0 is the function's own block,
    1.. the nested blocks in the order they open; `text` is the declaration's line."""
    name: str
    line: int
    scope: int
    text: str
    init: str


_FOR_DECL = re.compile(r"\bfor\s*\(\s*(?:const\s+)?[A-Za-z_][\w:]*(?:\s*\*)*\s+\**\s*(\w+)\s*=")


def find_definition(text: str, function: str) -> "cscan.Declaration | None":
    """The definition of `function` (plain or mangled: lib.cextract's identifiers) in `text`, or None."""
    names = cextract.target_identifiers(function)
    t = cscan.Text(text)
    found = [d for d in cscan.function_declarations(t) if d.body and d.name.split("::")[-1] in names]
    return found[0] if len(found) == 1 else None


def param_count(decl: "cscan.Declaration") -> int:
    """How many parameters the definition declares (`(void)` and `()` are none)."""
    chunks = [c.strip() for c, _ in cscan.split_params(decl.params, 0, len(decl.params))]
    return len([c for c in chunks if c and c != "void"])


def source_locals(text: str, function: str) -> list[SourceLocal]:
    """Every single-line local declaration (and `for` loop variable) of `function`'s body, in textual order."""
    decl = find_definition(text, function)
    if decl is None:
        return []
    t = cscan.Text(text)
    open_pos, close_pos = decl.body
    out: list[SourceLocal] = []
    stack = [0]
    blocks = 0
    pos = open_pos + 1
    pending_for: list[str] = []
    for raw in t.code[pos:close_pos].split("\n"):
        line_no = t.line_of(pos)
        pos += len(raw) + 1
        stripped = raw.strip()
        m = _FOR_DECL.search(stripped)
        if m:
            pending_for.append(m.group(1))
        got = declperm.declared("    " + stripped) if stripped else None
        if got and not stripped.startswith("for"):
            out.append(SourceLocal(got[0], line_no, stack[-1], t.line_text(line_no).strip(), got[1]))
        for ch in raw:
            if ch == "{":
                blocks += 1
                stack.append(blocks)
                for name in pending_for:
                    out.append(SourceLocal(name, line_no, blocks, t.line_text(line_no).strip(), ""))
                pending_for = []
            elif ch == "}" and len(stack) > 1:
                stack.pop()
    return out


def map_locals(graph: Graph, params: int, locals_: list[SourceLocal]) -> dict[int, SourceLocal]:
    """{vreg: declaration} for the dump's named locals (coalesced ones included: they still hold a number), matched in
    order against the source's declarations (first declared = highest vreg). A declaration with no node (its
    address is taken, it lives in memory) is skipped; {} when the names do not line up."""
    allv = sorted(v for v in graph.nodes if v >= FIRST_VIRTUAL)
    pset = set(allv[:params])
    nodes = sorted((v for v in allv if v not in pset and graph.nodes[v].tier == "named"), reverse=True)
    out: dict[int, SourceLocal] = {}
    i = 0
    for v in nodes:
        while i < len(locals_) and locals_[i].name != graph.nodes[v].name:
            i += 1
        if i == len(locals_):
            return {}
        out[v] = locals_[i]
        i += 1
    return out


def scope_breaks(decl: list[int], scope: dict[int, int]) -> int:
    """How far an order is from one the source can spell without moving a declaration between blocks: scope changes
    between neighbours, plus blocks whose runs come out of textual order."""
    seq = [scope.get(v, 0) for v in decl]
    breaks = sum(1 for a, b in zip(seq, seq[1:]) if a != b)
    runs = [s for i, s in enumerate(seq) if s and (i == 0 or seq[i - 1] != s)]
    return breaks + sum(1 for a, b in zip(runs, runs[1:]) if a > b)


# --- the search -------------------------------------------------------------------------------------------------

@dataclass
class Layout:
    """A renumbering: the parameters, the declared locals in declaration order (first declared = highest number),
    and every other node in its original relative order. `edits` names the source shape each change stands for;
    `graph`/`merged` carry a fold (a temp coalesced into a local) when there is one."""
    params: list[int]
    decl: list[int]
    rest: list[int]
    edits: tuple[str, ...] = ()
    graph: Graph | None = None
    merged: dict[int, int] = field(default_factory=dict)

    def scan(self) -> list[int]:
        return self.params + list(reversed(self.decl)) + self.rest

    def with_decl(self, decl) -> "Layout":
        return Layout(self.params, list(decl), self.rest, self.edits, self.graph, self.merged)


@dataclass
class Plan:
    """One evaluated layout: hits on the callee-saved and on the volatile targets, the colouring, the rewrite size."""
    layout: Layout
    saved: tuple[int, int]
    volatile: tuple[int, int]
    colour: dict[int, int]
    stalled: list[int]
    distance: int

    @property
    def rank(self) -> tuple:
        return (self.saved[0], self.volatile[0], -self.distance)

    @property
    def saved_exact(self) -> bool:
        return self.saved[0] == self.saved[1]

    @property
    def exact(self) -> bool:
        return self.saved_exact and self.volatile[0] == self.volatile[1]


def tiers(graph: Graph, params: int) -> tuple[list[int], list[int], list[int]]:
    """(parameters, declared locals in declaration order, the rest) of the dump's numbering: parameters take the
    first `params` vregs, the named non-`@` nodes above them are the locals (first declared = highest)."""
    virt = graph.virtual()
    allv = sorted(v for v in graph.nodes if v >= FIRST_VIRTUAL)
    pset = set(allv[:params])
    p = [v for v in virt if v in pset]
    loc = [v for v in virt if v not in pset and graph.nodes[v].tier == "named"]
    rest = [v for v in virt if v not in pset and v not in loc]
    return p, sorted(loc, reverse=True), rest


def inversions(order: list[int], base: list[int]) -> int:
    """Kendall distance between two orders of (mostly) the same nodes - how many pairs a rewrite swaps."""
    pos = {v: i for i, v in enumerate(base)}
    seq = [pos[v] for v in order if v in pos]
    return sum(1 for i in range(len(seq)) for j in range(i + 1, len(seq)) if seq[i] > seq[j])


#: what one source edit costs against one swapped pair of declarations, when plans are ranked
EDIT_COST = 4


class Searcher:
    """Finds renumberings whose simulated colouring gives `want`, the smallest rewrite first."""

    def __init__(self, graph: Graph, want: dict[int, int], params: int, max_evals: int = 20000, seed: int = 0,
                 scope: dict[int, int] | None = None):
        self.graph = graph
        self.want = want
        self.params = params
        self.max_evals = max_evals
        self.evals = 0
        self.rng = random.Random(seed)
        self.scope = scope or {}
        p, d, r = tiers(graph, params)
        self.base = Layout(p, d, r)
        self.base_breaks = scope_breaks(d, self.scope)

    def evaluate(self, layout: Layout) -> Plan:
        self.evals += 1
        g = layout.graph or self.graph
        res = simulate(g, layout.scan())
        saved = [0, 0]
        vol = [0, 0]
        for v, r in self.want.items():
            hit = res.colour.get(layout.merged.get(v, v)) == r
            bucket = saved if r >= LOWEST_SAVED else vol
            bucket[0] += hit
            bucket[1] += 1
        hoists = max(0, scope_breaks(layout.decl, self.scope) - self.base_breaks)
        dist = inversions(layout.decl, self.base.decl) + EDIT_COST * (len(layout.edits) + hoists)
        return Plan(layout, tuple(saved), tuple(vol), res.colour, [p.vreg for p in res.pushes if p.stalled], dist)

    def orders(self, layout: Layout, budget: int | None = None) -> list[Plan]:
        """The best declaration orders of `layout`'s locals: every order for a short list, a restarted insertion hill
        climb otherwise. The plans found, best first."""
        budget = budget if budget is not None else self.max_evals
        n = len(layout.decl)
        start = self.evals
        if n <= 1:
            return [self.evaluate(layout)]
        if n <= EXHAUSTIVE_MAX and math.factorial(n) <= budget:
            return best([self.evaluate(layout.with_decl(p)) for p in itertools.permutations(layout.decl)])
        cur = self.evaluate(layout)
        found = [cur]
        while self.evals - start < budget and not cur.exact:
            moves = [(i, j) for i in range(n) for j in range(n) if i != j]
            self.rng.shuffle(moves)
            improved = False
            for i, j in moves:
                if self.evals - start >= budget:
                    break
                decl = list(cur.layout.decl)
                decl.insert(j, decl.pop(i))
                cand = self.evaluate(layout.with_decl(decl))
                if cand.rank > cur.rank:
                    cur = cand
                    found.append(cur)
                    improved = True
                    break
            if not improved and not cur.exact:
                decl = list(layout.decl)
                self.rng.shuffle(decl)
                cur = self.evaluate(layout.with_decl(decl))
                found.append(cur)
        return best(found)

    def polish(self, plan: Plan, budget: int = 20000) -> Plan:
        """`plan` moved as close to the source's declaration order as it goes without losing a hit: single
        declaration moves that shorten the rewrite are taken until none does."""
        cur = plan
        n = len(cur.layout.decl)
        start = self.evals
        improved = True
        while improved and self.evals - start < budget:
            improved = False
            for i in range(n):
                for j in range(n):
                    if i == j:
                        continue
                    decl = list(cur.layout.decl)
                    decl.insert(j, decl.pop(i))
                    cand = self.evaluate(cur.layout.with_decl(decl))
                    if cand.saved[0] >= cur.saved[0] and cand.volatile[0] >= cur.volatile[0] \
                            and cand.distance < cur.distance:
                        cur = cand
                        improved = True
                        break
                if improved:
                    break
        return cur

    def candidates(self) -> list[int]:
        """Non-local nodes worth renumbering: the ones crossing a call (callee-saved) or with a callee-saved target."""
        return [v for v in self.base.rest
                if self.graph.nodes[v].phys >= LOWEST_SAVED or self.want.get(v, 0) >= LOWEST_SAVED]

    def moves(self) -> list[Layout]:
        """One source edit each: a node given its own declared local at every declaration position (an `@` web split
        off by hand, an accessor's value spelled in place), or a backend temp folded into a local it does not
        interfere with (a local typed like the value it copies)."""
        out = []
        b = self.base
        for v in self.candidates():
            node = self.graph.nodes[v]
            rest = [x for x in b.rest if x != v]
            for pos in range(len(b.decl) + 1):
                out.append(Layout(b.params, b.decl[:pos] + [v] + b.decl[pos:], rest,
                                  ("declare %s as a local at declaration position %d" % (node.label(), pos),)))
            if node.tier != "backend":
                continue
            for loc in b.decl:
                if v in self.graph.nodes[loc].neighbors:
                    continue
                out.append(Layout(b.params, list(b.decl), rest,
                                  ("fold %s into local %s" % (node.label(), self.graph.nodes[loc].label()),),
                                  self.graph.coalesce(v, loc), {v: loc}))
        return out

    def run(self, edits: bool = True, top: int = 5, widen: int = 8) -> list[Plan]:
        """Declaration orders first; when none is exact, every single edit, then a budgeted order search on the
        `widen` most promising. The best `top` plans."""
        plans = self.orders(self.base)
        if edits and not plans[0].exact:
            firsts = best([self.evaluate(lay) for lay in self.moves()])
            each = max(200, self.max_evals // max(1, min(len(firsts), widen)))
            for first in firsts[:widen]:
                plans += self.orders(first.layout, budget=each)
                if best(plans)[0].exact:
                    break
            plans = best(plans + firsts)
        return best([self.polish(p) for p in plans[:top]])[:top]


def best(plans: list[Plan]) -> list[Plan]:
    """`plans` without repeats, best first."""
    seen = set()
    out = []
    for p in sorted(plans, key=lambda p: p.rank, reverse=True):
        key = (tuple(p.layout.decl), p.layout.edits)
        if key not in seen:
            seen.add(key)
            out.append(p)
    return out
