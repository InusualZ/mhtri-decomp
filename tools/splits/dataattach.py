#!/usr/bin/env python3
"""dataattach.py - phase 2 of the splits program: which unit of the candidate owns each unowned data symbol, with evidence.

    python tools/splits/dataattach.py [--window LO HI] [--lane NAME] [--out F] [--stats F] [--proposal F ...] [--linker] [--no-settle]
    python tools/splits/dataattach.py --explain UNIT_REGEX|ADDR [--section S[,S]]     # one unit's segments, or the run holding an address
    python tools/splits/dataattach.py --holdout [--sections S[,S]]                    # the solver's precision on the registered data
    python tools/splits/dataattach.py --vtable-order                                  # per unit: up/down/tie vtable pairs (the zigzag premise)
    python tools/splits/dataattach.py --selftest

Input is the **candidate** the phase 1 proposals render to (default `docs/splits/proposals/phase1-a..g.json` + `phase1-reconcile.json` + `phase2-folds.json`
when it exists, through `splitcheck.render`, so there is one implementation of the proposal format; a `--proposal` file with `units` - a text fold, a
data-only unit - joins them, its `attach`/`unowned_data` rows are dropped: they are what this tool derives).  For every data section the symbols no unit range covers form **runs**
(a run lies between two owned ranges); a run's **chain** is the text-having units in link (= text) order from the unit owning the data before it to the
one owning the data after it.  Each symbol gets evidence and a monotone assignment of symbols to the chain decides who owns what:

* **strong evidence** - the units whose text reads the symbol (`splitcheck.Ctx`: `lis`/`addi`/load decode of the retail text, pool literals by loads
  only), and for a `jumptable_` the unit its branch targets lie in.  A symbol no one reads has none (it is *free*);
* **derived evidence** (weight 1, never overrides a strong row) - the units whose functions a table's words point at, and the units of the data
  symbols it points to or is pointed from (pointer graph, two rounds; owned symbols count by their owner, decided ones by the unit they got);
* **link order**: units link in text order in every section, so the assignment is non-decreasing along the run.  A dynamic program minimises the
  violated evidence; a symbol is **decided** when every optimal assignment gives it one unit (forced between two anchors of one unit, or between an
  anchor and the run edge), **ambiguous** when the optimum leaves an interval of units.  Output is a pure function of the input: no set order leaks.

A strong row the optimum violates is a **contradiction**: a global read where it is not defined (stays, graded medium, noted), or - when the symbol is
static (`jumptable_`, `scope:local`), or three or more between two adjacent data-bearing units - an **interleave**: the data of the two units alternates,
so they are one TU or one reads the other's data.  An interleave zone is not attached; it is listed in `unowned_data` with both units as candidates.
Inside one owner's block of a pool, only the longest first-use-ordered stretch is decided (`pool-order`: a pool is one per TU, in first-use order; the
rest is given back as ambiguous).  A **data-only** registered unit whose neighbours' data is decided to ONE unit on both sides is that unit's fragment:
a `fold-data-only` row (`takes_from`).  `--linker` adds what no unit owns (the linker's `.init` tables, `_eti_init_info`); the `extab`/`extabindex`
entries past the last registered range always go to the unit holding their function.

**Settling.**  The decided attachments are rendered with the proposals and every invariant is run; an attachment that makes a unit fail an invariant the
phase 1 candidate does not fail it on is blocked: it becomes an `unowned_data` item (`multi-tu`) and a `guess` row (never applied), and the loop repeats
until nothing new fails.  The finding is evidence in its own right: the unit holds more than one TU, or one of its reads is not its own.

Output (`--out`) is a phase 2 proposal for `splitcheck.py --proposal` (canonical `attach` / `unowned_data`: `docs/splits-program.md`), for the units whose
`.text` starts in `--window` (default: all).  `--holdout` hides every third owned data range of a section (batches, so a held-out run is one range between
two owned neighbours), decides it again as an unowned run and compares with the registered owner.
"""
from __future__ import annotations

import argparse
import bisect
import collections
import glob
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (os.path.dirname(HERE), HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import splitcheck as sc  # noqa: E402

DATA_SECS = (".rodata", ".data", ".sdata", ".sdata2", ".bss", ".sbss", ".sbss2")
PTR_SECS = (".data", ".rodata", ".sdata")
POOL_SECS = (".sdata", ".sdata2")
W_STRONG, W_DERIVED = 1000, 1
INF = 10 ** 9
PAD = 0x40                      # the largest gap between two symbols that is alignment padding, not an unmapped object
LINKER_GENERATED = ("_eti_init_info", "_rom_copy_info", "_bss_init_info")      # emitted by mwldeppc (docs/init-section.md): no TU owns them
PROPOSAL_DIR = os.path.join(os.path.dirname(os.path.dirname(HERE)), "docs", "splits", "proposals")
PHASE2_KEYS = ("attach", "unowned_data", "data_attach", "attachments")


def hx(a):
    return "0x%08X" % a


# ---- the dynamic program ----------------------------------------------------------------------------------------------------

def dp_admissible(sets, weights, K):
    """Monotone assignment of `len(sets)` symbols to chain positions `0..K-1`.  `sets[i]` is the set of positions the symbol's evidence
    names (empty = none), `weights[i]` what violating it costs.  Returns `(optimum, [(lo, hi) per symbol])`: the lowest and highest chain
    position the symbol takes in any optimal assignment (`lo == hi` = decided)."""
    n = len(sets)
    if n == 0:
        return 0, []

    def cost(i, k):
        e = sets[i]
        return 0 if (not e or k in e) else weights[i]

    fwd, prev = [], None
    for i in range(n):
        row = [0] * K
        run = INF
        for k in range(K):
            if i:
                run = min(run, prev[k])
            row[k] = (run if i else 0) + cost(i, k)
        fwd.append(row)
        prev = row
    bwd = [None] * n
    nxt = None
    for i in range(n - 1, -1, -1):
        row = [0] * K
        run = INF
        for k in range(K - 1, -1, -1):
            if i != n - 1:
                run = min(run, nxt[k])
            row[k] = (run if i != n - 1 else 0) + cost(i, k)
        bwd[i] = row
        nxt = row
    opt = min(fwd[n - 1])
    out = []
    for i in range(n):
        ks = [k for k in range(K) if fwd[i][k] + bwd[i][k] - cost(i, k) == opt]
        out.append((ks[0], ks[-1]) if ks else (None, None))
    return opt, out


def find_zones(strong, adm, static):
    """Interleave groups of one run: `[(kmin, kmax, [symbol indices of the contradictions])]`.

    A contradiction is a strong row the optimum violates.  Its reader unit (`kr`, the evidence position nearest the admissible interval) and
    the unit the link order puts it in (`ka`) are united when the symbol is static, or when three or more symbols make the same
    contradiction and no other data-bearing unit lies between the two (adjacent units whose data alternates).  A static contradiction across
    more than two other data-bearing units is not united (a read that far away is not an interleave of neighbours)."""
    carriers = sorted({k for e in strong for k in e})
    clusters = collections.defaultdict(list)
    for i, e in enumerate(strong):
        if not e:
            continue
        lo, hi = adm[i]
        if lo is None or any(lo <= k <= hi for k in e):
            continue
        clusters[(tuple(sorted(e)), lo, hi)].append(i)
    groups = []
    for (e, lo, hi), idxs in sorted(clusters.items(), key=lambda kv: kv[1][0]):
        kr = min(e, key=lambda k: min(abs(k - lo), abs(k - hi)))
        ka = lo if abs(lo - kr) <= abs(hi - kr) else hi
        a, b = sorted((kr, ka))
        between = [c for c in carriers if a < c < b]
        if any(static(i) for i in idxs):
            ok = len(between) <= 2
        else:
            ok = len(idxs) >= 3 and not between
        if ok:
            groups.append([a, b, list(idxs)])
    groups.sort()
    merged = []
    for g in groups:
        if merged and g[0] <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], g[1])
            merged[-1][2] += g[2]
        else:
            merged.append(g)
    return [(a, b, idxs) for a, b, idxs in merged]


# ---- the data of the candidate ----------------------------------------------------------------------------------------------

class Run:
    """The unowned symbols of one section between two owned ranges."""

    def __init__(self, sec, syms, prev_owner, next_owner, chain):
        self.sec, self.syms, self.prev_owner, self.next_owner, self.chain = sec, syms, prev_owner, next_owner, chain
        self.pos = {u.name: k for k, u in enumerate(chain)}


def link_units(cand):
    """The text-having units in link order."""
    return sorted((u for u in cand.units if u.first(".text") is not None), key=lambda u: u.first(".text"))


def collect_runs(cand, ctx, secs=DATA_SECS):
    tu = link_units(cand)
    out = []
    for sec in secs:
        v = ctx.sec_index.get(sec, [])
        starts = [t[0] for t in v]
        unowned = [s for s in ctx.data_syms if s["section"] == sec and ctx.owner(sec, s["addr"]) is None
                   and not (s["type"] == "label" and not s["size"])]

        def anchor(i, step):
            """`(text anchor, inclusive)` of the first text-having owner from `i` on; a data-only owner in between (a range of its own the
            neighbour cannot grow across) makes that unit's own anchor exclusive."""
            adjacent = True
            while 0 <= i < len(v):
                a = v[i][2].first(".text")
                if a is not None:
                    return a, adjacent
                adjacent = False
                i += step
            return None, True

        cur = None
        for s in unowned:
            i = bisect.bisect_right(starts, s["addr"]) - 1
            if cur is not None and cur[0] == i:
                cur[1].append(s)
                continue
            cur = [i, [s]]
            (lo, lo_in), (hi, hi_in) = anchor(i, -1), anchor(i + 1, 1)
            chain = [u for u in tu if (lo is None or u.first(".text") > lo or (lo_in and u.first(".text") == lo))
                     and (hi is None or u.first(".text") < hi or (hi_in and u.first(".text") == hi))]
            out.append(Run(sec, cur[1], v[i][2] if i >= 0 else None, v[i + 1][2] if i + 1 < len(v) else None, chain))
    return out


def sym_end(s):
    return s["addr"] + (s["size"] or 4)


def reader_names(ctx, s):
    rd = ctx.literal_readers(s) if sc.is_literal(s) else ctx.readers(s)
    c = collections.Counter()
    for x in rd:
        u = ctx.text_owner(x)
        if u is not None:
            c[u.name] += 1
    return c


#: a holder of at least this many non-zero words of which fewer than `MIN_POINTER_SHARE` point into the map is data (an SJIS/lookup table), not a
#: pointer table: its few "pointers" are values that happen to fall in the address range (measured: six holders at <= 0.7 %, the next at 4 %)
MIN_HOLDER_WORDS, MIN_POINTER_SHARE = 64, 0.01


def holder_pointers(ctx, s):
    """`[(word address, word, target data symbol or None, target function or None)]` of the aligned non-zero words of `s` that point into the map
    (a data symbol other than `s`, or a function), `[]` for a holder that is mostly non-pointer data (`MIN_HOLDER_WORDS`, `MIN_POINTER_SHARE`) -
    a word at an address that is not 4-aligned (a `.4byte` inside a byte array) is never a pointer."""
    key = s["addr"]
    cache = ctx.__dict__.setdefault("_holder_cache", {})
    if key in cache and cache[key][0] is s:
        return cache[key][1]
    out, nonzero = [], 0
    for i in range((s["size"] or 0) // 4):
        a = s["addr"] + 4 * i
        w = ctx.dol.word(a)
        if not w:
            continue
        nonzero += 1
        if a % 4 or w < 0x80004000:
            continue
        t = None
        if w >= 0x80500000:
            _j, t = ctx.data_sym_at(w)
            if t is s:
                t = None
        f = ctx.fn_at(w) if w < 0x80580000 else None
        if t is not None or f is not None:
            out.append((a, w, t, f))
    if nonzero >= MIN_HOLDER_WORDS and len(out) < MIN_POINTER_SHARE * nonzero:
        out = []
    cache[key] = (s, out)
    return out


def code_target_names(ctx, s):
    c = collections.Counter()
    for _a, w, _t, f in holder_pointers(ctx, s):
        if f is not None and 0x80004000 <= w < 0x80580000:
            u = ctx.text_owner(w)
            if u is not None:
                c[u.name] += 1
    return c


def pointer_edges(ctx):
    """`{symbol address: set of neighbour addresses}` over every data symbol: a word of one that points into another (`holder_pointers`)."""
    edges = collections.defaultdict(set)
    for s in ctx.data_syms:
        if s["section"] not in PTR_SECS or not s["size"]:
            continue
        for _a, _w, t, _f in holder_pointers(ctx, s):
            if t is not None and t["section"] in DATA_SECS + (".ctors", ".dtors"):
                edges[s["addr"]].add(t["addr"])
                edges[t["addr"]].add(s["addr"])
    return edges


def file_name_string(ctx, s):
    """Whether the string `s` is a real `__FILE__` / file-name string (`g3d_anmvis.cpp`, `g3d_resnode_ac.h`): the one signal `file-string` stands on."""
    import dataorder as do
    b = ctx.dol.read(s["addr"], s["size"] or 0)
    text = b.rstrip(b"\0").decode("latin-1") if b else None
    return do.is_source_name(text) or do.is_header_name(text)


def strong_label(s, names, vtable, ctx=None):
    """The signal a strong row stands on (docs: reader=1, pool-order=3, vtable-store=4, local-static=5, jumptable=6, file-string=7, string-reader=8,
    sinit=9).  `file-string` is only for a real `__FILE__` / file-name string; any other string literal one unit reads is a `string-reader`."""
    if s["name"].startswith("jumptable_"):
        return "jumptable"
    if vtable:
        return "vtable-store"
    one = len(names) == 1
    if s["scope"] == "local" and one:
        return "local-static"
    if sc.is_literal(s) and s["section"] == ".sdata2":
        return "pool-order" if one else "pool-readers"
    if s["section"] in (".data", ".sdata", ".rodata") and s.get("kind") == "string":
        if not one:
            return "readers"
        return "file-string" if ctx is not None and file_name_string(ctx, s) else "string-reader"
    return "reader" if one else "readers"


def names_counter(names):
    return collections.Counter({x: 1 for x in names})


def kind_of(kind, s):
    y = kind.get(s["addr"]) if s["section"] == ".data" else None
    return y.kind if y is not None else ""


def nonzero_words(ctx, s):
    return sum(1 for i in range((s["size"] or 0) // 4) if ctx.dol.word(s["addr"] + 4 * i))


#: the pointer closure runs to a fixpoint, at most this many rounds (a chain of N hops needs N rounds)
MAX_CLOSURE_ROUNDS = 16
#: a block of N symbols with fewer than `max(ANCHOR_MIN, N * ANCHOR_SHARE)` read by its unit is graded medium (derived/forced evidence is not strong)
ANCHOR_MIN, ANCHOR_SHARE = 3, 1.0 / 50
#: contiguous equal-size elements named by one pointer table: at least this many make an array (one object, never split between units)
MIN_ARRAY = 4
#: a symbol whose pointer neighbours name more units than this is an index, not an owner signal
MAX_POINTER_UNITS = 2


class Analysis:
    """The evidence and the monotone assignment of every unowned run of a candidate (`cand`, with `ctx` its decoded reference index)."""

    def __init__(self, cand, ctx, kind):
        self.cand, self.ctx, self.kind = cand, ctx, kind
        self.runs = collect_runs(cand, ctx)
        self.edges = pointer_edges(ctx)
        self.res = []                           # per run: dict
        self.rvp_groups = []                    # reader-vs-pointer groups: {"kind", "units", "why"}
        self.ovr = {}                           # (section, address) -> resolved override row (`set_overrides`)
        self.overrides = []
        self.do_syms, self.seams, self.do_by_addr = [], [], {}
        if kind:
            import dataorder as do
            self.do_syms = sorted(kind.values(), key=lambda y: y.addr)
            self.seams = do.seams(self.do_syms)
            self.do_by_addr = {y.addr: y for y in self.do_syms}
        self._strong_pass()
        self._derived_pass()
        for r in self.res:
            self._weighted(r)
        self._rvp_pass()

    # evidence ----------------------------------------------------------------------------------------------------------------
    def _strong_pass(self):
        ctx = self.ctx
        for run in self.runs:
            n = len(run.syms)
            sets, names, labels, foreign = [], [], [], []
            for s in run.syms:
                nm = reader_names(ctx, s)
                dn = {x for x in ctx.definers(s) if x in run.pos}
                ks = {run.pos[x] for x in (dn or nm) if x in run.pos}
                label = ""
                if dn:                          # a unit's own __sinit constructs it: that unit defines it, whoever else reads it
                    nm = names_counter(dn)
                    label = "sinit"
                elif ks:
                    k = kind_of(self.kind, s)
                    label = strong_label(s, nm, k == "V", ctx)
                elif s["name"].startswith("jumptable_"):
                    ct = code_target_names(ctx, s)
                    if len(ct) == 1 and next(iter(ct)) in run.pos:
                        nm = ct
                        ks = {run.pos[next(iter(ct))]}
                        label = "jumptable"
                sets.append(ks)
                names.append(set(nm) if ks else set())
                labels.append(label)
                foreign.append(sorted(x for x in nm if x not in run.pos))
            opt, adm = dp_admissible(sets, [1] * n, len(run.chain))
            self.res.append({"run": run, "strong": sets, "names": names, "label": labels, "foreign": foreign,
                             "adm_strong": adm, "opt_strong": opt, "derived": [set() for _ in range(n)], "dlabel": [""] * n,
                             "dsrc": [None] * n, "dpure": [False] * n, "ddepth": [0] * n, "rvp": {}})

    def _unit_evidence(self, addr, prev=None):
        """`(unit names, kind)` the evidence of the data symbol at `addr` names: `base` (the units that read it), `decided` (the strong optimum places
        it), `derived` (an earlier round of the closure, only with `prev`), `owned` (a registered unit with text holds it), else `("", set())`."""
        if addr in self._base:
            return self._base[addr], "base"
        if addr in self._decided:
            return self._decided[addr], "decided"
        if prev is not None and addr in prev:
            return prev[addr], "derived"
        if addr not in self._owner_cache:
            i, t = self.ctx.data_sym_at(addr)
            u = self.ctx.owner(t["section"], t["addr"]) if t is not None else None
            self._owner_cache[addr] = ({u.name}, "owned") if (u is not None and u.first(".text") is not None) else (set(), "")
        return self._owner_cache[addr]

    def _derived_pass(self):
        ctx = self.ctx
        self._decided, self._base, self._owner_cache = {}, {}, {}
        for r in self.res:
            for i, s in enumerate(r["run"].syms):
                lo, hi = r["adm_strong"][i]
                if lo is not None and lo == hi:
                    self._decided[s["addr"]] = {r["run"].chain[lo].name}
                if r["names"][i]:
                    self._base[s["addr"]] = set(r["names"][i])
        prev = {}
        cands = []
        for r in self.res:
            run = r["run"]
            for i, s in enumerate(run.syms):
                if not r["strong"][i]:
                    ct = code_target_names(ctx, s)
                    nm = {x for x in ct if x in run.pos}
                    if nm:
                        r["derived"][i] = {run.pos[x] for x in nm}
                        r["dlabel"][i] = "code-pointers"
                        r["dpure"][i] = len(ct) == 1 and sum(ct.values()) == nonzero_words(ctx, s)       # every non-zero word enters the one unit's text
                        prev[s["addr"]] = nm
                    else:
                        cands.append((r, i))
        for rnd in range(1, MAX_CLOSURE_ROUNDS + 1):            # the pointer closure, to a fixpoint (bounded)
            new = {}
            for r, i in cands:
                s = r["run"].syms[i]
                acc, src = set(), []
                for a in sorted(self.edges.get(s["addr"], ())):
                    names, kd = self._unit_evidence(a, prev)
                    got = {x for x in names if x in r["run"].pos}
                    if got:
                        src.append((a, frozenset(got), kd, ctx.section_of(a)))
                    acc |= names
                acc = {x for x in acc if x in r["run"].pos}
                if acc:
                    new[(id(r), i)] = (r, i, acc, src)
            if not new:
                break
            for (_k, (r, i, acc, src)) in new.items():
                s = r["run"].syms[i]
                r["derived"][i] = {r["run"].pos[x] for x in acc}
                r["dlabel"][i] = "pointer-graph"
                r["dsrc"][i] = src
                r["ddepth"][i] = rnd
                prev[s["addr"]] = acc
            cands = [(r, i) for r, i in cands if not r["derived"][i]]

    def derived_strong(self, r, i, k):
        """Whether a symbol placed by derived evidence in unit `k` is as good as a reader: a table of code pointers whose every non-zero word enters
        that unit's text, or a pointer-graph symbol whose every holder/target is a `.data` symbol the unit's registered ranges already hold."""
        unit = r["run"].chain[k].name
        if r["dlabel"][i] == "code-pointers":
            return r["dpure"][i] and r["derived"][i] == {k}
        if r["dlabel"][i] == "pointer-graph" and r["dsrc"][i]:
            return all(kd == "owned" and sec == ".data" and units == {unit} for _a, units, kd, sec in r["dsrc"][i])
        return False

    def _weighted(self, r):
        run = r["run"]
        n = len(run.syms)
        sets = [r["strong"][i] or r["derived"][i] for i in range(n)]
        weights = [W_STRONG if r["strong"][i] else W_DERIVED for i in range(n)]
        opt, adm = dp_admissible(sets, weights, len(run.chain))
        r["opt_all"], r["adm"] = opt, adm
        static = lambda i: run.syms[i]["scope"] == "local" or run.syms[i]["name"].startswith("jumptable_")
        r["zones"] = find_zones(r["strong"], adm, static)
        zone_of = [None] * n
        for z, (a, b, _idxs) in enumerate(r["zones"]):
            for i in range(n):
                lo, hi = adm[i]
                if lo is not None and lo >= a and hi <= b:
                    zone_of[i] = z
        r["zone_of"] = zone_of
        contra = []
        for i in range(n):
            e = r["strong"][i]
            lo, hi = adm[i]
            if e and (lo is None or not any(lo <= k <= hi for k in e)):
                contra.append(i)
        r["contra"] = contra

    def _rvp_pass(self):
        """Reader against pointer: a symbol one unit reads (or constructs) while the pointer tables around it name another unit, and an array (contiguous
        equal-size elements one pointer table names) whose elements the readers split between units, are deferred as `reader-vs-pointer` with both
        sides listed - a strong reader must not override a disagreeing pointer table silently, and an array is one object."""
        pos = {}
        for r in self.res:
            for i, s in enumerate(r["run"].syms):
                pos[s["addr"]] = (r, i)

        def add(members, kind, units, why):
            gid = len(self.rvp_groups)
            self.rvp_groups.append({"kind": kind, "units": units, "why": why})
            for r, i in members:
                r["rvp"].setdefault(i, gid)

        seen = set()                                            # (a) arrays: one object, never split between units (named first: an element is not also a lone disagreement)
        for h in self.ctx.data_syms:
            if h["section"] not in PTR_SECS or not h["size"]:
                continue
            elems = {}
            for _a, w, t, _f in holder_pointers(self.ctx, h):
                if t is not None and t["addr"] == w and t["size"]:
                    elems[t["addr"]] = t
            cur = []
            for t in [elems[a] for a in sorted(elems)] + [None]:
                if cur and t is not None and t["section"] == cur[-1]["section"] and t["size"] == cur[-1]["size"] and t["addr"] == cur[-1]["addr"] + cur[-1]["size"]:
                    cur.append(t)
                    continue
                if len(cur) >= MIN_ARRAY and (cur[0]["addr"], len(cur)) not in seen:
                    seen.add((cur[0]["addr"], len(cur)))
                    members = [pos[e["addr"]] for e in cur if e["addr"] in pos]
                    units = sorted({r["run"].chain[r["adm"][i][0]].name for r, i in members if r["adm"][i][0] is not None and r["adm"][i][0] == r["adm"][i][1]
                                    and r["zone_of"][i] is None})
                    if len(units) >= 2:
                        add(members, "array", units,
                            "%d elements of 0x%X B at 0x%08X..0x%08X, named by %s, are one object but their readers split it between %s"
                            % (len(cur), cur[0]["size"], cur[0]["addr"], sym_end(cur[-1]), h["name"], ", ".join(units)))
                cur = [t] if t is not None else []
        for r in self.res:                                      # (b) one object, the reader and the pointer neighbours disagree
            run = r["run"]
            for i, s in enumerate(run.syms):
                st, (lo, hi) = r["strong"][i], r["adm"][i]
                if not st or lo is None or lo != hi or lo not in st or r["zone_of"][i] is not None:
                    continue
                unit = run.chain[lo].name
                near = set()
                for a in sorted(self.edges.get(s["addr"], ())):
                    names, kd = self._unit_evidence(a)
                    near |= names
                if near and unit not in near and len(near) <= MAX_POINTER_UNITS:
                    sig = "constructed by %s's own __sinit" % unit if r["label"][i] == "sinit" else "read by %s" % unit
                    add([(r, i)], "disagree", [unit] + sorted(near),
                        "%s is %s, but the pointer tables around it name %s" % (s["name"], sig, ", ".join(sorted(near))))
        self._rvp_enclosed()

    def _rvp_enclosed(self):
        """A disagreement the link order encloses is not deferred: a symbol between two symbols of one unit lies in that unit's data (taking it out
        would leave the unit two ranges in one section - dtk takes one), so it stays attached and the row records both signals instead
        (`rvp_note`, graded medium).  Only a symbol at the edge of its unit's block, or an array the readers split, is carved out."""
        for r in self.res:
            run = r["run"]
            r["rvp_note"] = {}
            for i in sorted(r["rvp"]):
                g = self.rvp_groups[r["rvp"][i]]
                if g["kind"] != "disagree":
                    continue
                lo, hi = r["adm"][i]
                j = i - 1
                while j >= 0 and j in r["rvp"]:
                    j -= 1
                k = i + 1
                while k < len(run.syms) and k in r["rvp"]:
                    k += 1
                left = r["adm"][j] if j >= 0 else (None, None)
                right = r["adm"][k] if k < len(run.syms) else (None, None)
                if left[0] is not None and left[0] == left[1] == lo == right[0] == right[1]:
                    r["rvp_note"][i] = "reader-vs-pointer: " + g["why"]
                    del r["rvp"][i]

    # segments ----------------------------------------------------------------------------------------------------------------
    def segments(self, r):
        """Consecutive symbols of a run with the same fate: an orchestrator override, decided to a unit, in an interleave zone, deferred as reader-vs-pointer,
        or ambiguous over an interval."""
        run = r["run"]
        out = []
        for i, s in enumerate(run.syms):
            lo, hi = r["adm"][i]
            ov = self.ovr.get((run.sec, s["addr"]))
            if ov is not None:
                key = ("ovr", ov["id"])
            elif r["zone_of"][i] is not None:
                key = ("zone", r["zone_of"][i])
            elif i in r["rvp"]:
                key = ("rvp", r["rvp"][i])
            elif lo is not None and lo == hi:
                key = ("unit", lo)
            else:
                key = ("amb", lo, hi)
            if out and out[-1]["key"] == key:
                out[-1]["idx"].append(i)
            else:
                out.append({"key": key, "idx": [i]})
        return out

    def decisions(self):
        """`{(section, address): unit name}` of every symbol the analysis decides (an override that attaches counts)."""
        out = {}
        for r in self.res:
            run = r["run"]
            for seg in self.segments(r):
                if seg["key"][0] == "unit":
                    for i in seg["idx"]:
                        out[(run.sec, run.syms[i]["addr"])] = run.chain[seg["key"][1]].name
                elif seg["key"][0] == "ovr":
                    ov = self.overrides[seg["key"][1]]
                    if ov["action"] == "attach":
                        for i in seg["idx"]:
                            out[(run.sec, run.syms[i]["addr"])] = ov["unit"]
        return out

    def cand_unit(self, name):
        """The unit of the candidate named `name` (`None` when it has none)."""
        return self.ctx.by_name.get(name)

    def set_overrides(self, rows):
        """Install the resolved orchestrator overrides (`resolve_overrides`): their symbols leave the solver's segments (`segments`)."""
        self.overrides = rows
        self.ovr = {}
        for r in self.res:
            for s in r["run"].syms:
                for ov in rows:
                    if ov["section"] == r["run"].sec and ov["start"] <= s["addr"] < ov["end"]:
                        self.ovr[(r["run"].sec, s["addr"])] = ov
                        break


# ---- pool order, from segments to proposal rows -------------------------------------------------------------------------------

def pool_cutback(ctx, run, idx, unit, dec=None):
    """A pool is one per TU in first-use order (idea 94): inside one owner's block of `.sdata` strings the first use of the strings the owner uses
    must not go down - a string a pointer initialiser of the owner's table names is used first (`splitcheck.pool_first_use`).  Returns
    `(keep, given_back, inversions)`: the symbol indices of the longest in-order stretch (every index when the block is in order), the lists of the
    indices before and after it, and the numeric `.sdata2` literals whose first use goes down.

    A numeric literal is TU-local - the unit that loads it defines it - so an inversion among them cuts nothing back: the block stays the reader's and
    the inversion is the two-TU signal the caller records as an open question of the unit (`inversions`: `[(literal, previous literal)]`)."""
    if run.sec not in POOL_SECS or len(idx) < 2:
        return idx, [], []
    dec = dec or {}

    def held(a):
        o = ctx.owner(ctx.section_of(a), a)
        if o is not None:
            return o is unit
        _j, t = ctx.data_sym_at(a)
        return t is not None and dec.get((t["section"], t["addr"])) == unit.name

    seq, inv = [], []                                          # (position in idx, first-use key of the owner's own use)
    for t, i in enumerate(idx):
        sym = run.syms[i]
        if not sc.is_literal(sym):
            continue
        use = sc.pool_first_use(ctx, sym, unit, held=held)
        if use is None:
            continue
        if run.sec == ".sdata2" and seq and use[0] < seq[-1][1]:
            inv.append((sym["name"], seq[-1][2]))
        seq.append((t, use[0], sym["name"]))
    if run.sec == ".sdata2":
        return idx, [], inv
    stretches, cur = [], []
    for item in seq:
        if cur and item[1] < cur[-1][1]:
            stretches.append(cur)
            cur = []
        cur.append(item)
    if cur:
        stretches.append(cur)
    if len(stretches) < 2:
        return idx, [], []
    keep = max(stretches, key=len)                             # the first of equal length
    lo, hi = keep[0][0], keep[-1][0] + 1
    given = [idx[:lo], idx[hi:]]
    breaks = [(b[0][2], a[-1][2]) for a, b in zip(stretches, stretches[1:])]
    return idx[lo:hi], [g for g in given if g], breaks


def pool_candidates(an, run, idx, k, block):
    """The units a block `idx` of pool symbols handed back by the pool order may belong to: the units of the chain that read them (a literal is
    defined where it is loaded), the owner `k` included; the owner's neighbours when nothing reads them.  A symbol only another unit reads that lies
    between two symbols the owner reads (`block`: the owner's whole block) is a foreign read the link order forces into the owner's data, not a
    second owner."""
    owner = run.chain[k].name
    mine = [i for i in block if owner in reader_names(an.ctx, run.syms[i])]
    rd = set()
    for i in idx:
        names = {x for x in reader_names(an.ctx, run.syms[i]) if x in run.pos}
        if names and owner not in names and mine and mine[0] < i < mine[-1]:
            continue
        rd |= names
    if not rd:
        return [run.chain[x].name for x in (k - 1, k, k + 1) if 0 <= x < len(run.chain)]
    rd.add(owner)
    return sorted(rd, key=lambda n: run.pos[n])


def stem_of(name):
    return os.path.splitext(os.path.basename(name))[0]


def in_window(u, window):
    a = u.first(".text")
    return a is not None and window[0] <= a < window[1]


def tight_range(an, r, seg, k, idx=None):
    """`(start, end)` of a decided segment: abutting the owner's own range when the gap is alignment padding, else the symbols' extent."""
    run, ctx = r["run"], an.ctx
    idx = idx if idx is not None else seg["idx"]
    syms = [run.syms[i] for i in idx]
    start, end = syms[0]["addr"], sym_end(syms[-1])
    first, last = idx[0], idx[-1]
    unit = run.chain[k]
    if first == 0 and run.prev_owner is not None and run.prev_owner.name == unit.name:
        pe = max((e for s0, e, _x in unit.ranges.get(run.sec, [])), default=None)
        if pe is not None and pe <= start and start - pe <= PAD:
            start = pe
    if last == len(run.syms) - 1 and run.next_owner is not None and run.next_owner.name == unit.name:
        ns = min((s0 for s0, e, _x in unit.ranges.get(run.sec, []) if s0 >= end), default=None)
        if ns is not None and ns - end <= PAD:
            end = ns
    elif last + 1 < len(run.syms):
        nxt = run.syms[last + 1]
        lo, hi = r["adm"][last + 1]
        if r["zone_of"][last + 1] is None and lo is not None and lo == hi and lo != k and 0 <= nxt["addr"] - end <= PAD:
            end = nxt["addr"]
    return start, end


def signal_counts(r, idx):
    """The signals of a block by symbol: the reader-side label first (`reader`, `sinit`, ...), else the derived one (`pointer-graph`, `code-pointers`),
    else `forced` (placed only by the non-decreasing link order between anchors)."""
    c = collections.Counter()
    for i in idx:
        c[r["label"][i] or r["dlabel"][i] or "forced"] += 1
    return dict(c)


def finding_for(an, r, i, unit_name):
    s = r["run"].syms[i]
    if r["label"][i] == "sinit":
        sites = an.ctx.definers(s).get(unit_name, [])
        f = an.ctx.fn_at(sites[0]) if sites else None
        return "%s (%s 0x%08X, 0x%X B) is constructed at 0x%08X by %s's own __sinit (%s)" % (
            s["name"], s["section"], s["addr"], s["size"], sites[0] if sites else 0, unit_name, f["name"] if f else "?")
    sites = (an.ctx.literal_readers(s) if sc.is_literal(s) else an.ctx.readers(s))
    mine = [x for x in sites if (an.ctx.text_owner(x) is not None and an.ctx.text_owner(x).name == unit_name)]
    if mine:
        return "%s (%s 0x%08X, 0x%X B) is read at 0x%08X in %s" % (s["name"], s["section"], s["addr"], s["size"], min(mine), unit_name)
    ct = code_target_names(an.ctx, s)
    return "%s (%s 0x%08X, 0x%X B): %s" % (s["name"], s["section"], s["addr"], s["size"],
                                          "its branch targets lie in %s" % ", ".join(sorted(ct)) if ct else "no decoded reader")


def explain_cmd(unit, sec):
    return "python tools/splits/dataattach.py --explain %s --section %s" % (stem_of(unit), sec)


def seam_pieces(an, run, idx, unit):
    """`idx` cut at the strong `.data` seams (V->S, zigzag; `splitcheck.seams_inside`, the data-order rule, plus the V->S seams of instantiated vtables) that
    lie inside the block: each piece is a row of its own, graded by its own anchors - a seam says another TU starts there, so one row must not claim both
    sides as one TU's strong block."""
    if run.sec != ".data" or not an.seams or len(idx) < 2:
        return [idx]
    lo, hi = run.syms[idx[0]]["addr"], sym_end(run.syms[idx[-1]])
    kept, inst = sc.seams_inside(an.ctx, unit, lo, hi, an.do_syms, an.seams, an.do_by_addr, sc.unit_sinit_closure(an.ctx, unit))
    kind_at = {x["addr"]: x["kind"] for x in an.seams}
    # a V->S seam whose later vtable the unit's own sinit instantiates is no TU edge for the data-order check, but it is still where the strings of the
    # first vtable group end and the next group starts: the row is cut there so that each side is graded by its own anchors
    cuts = sorted({x["addr"] for x in kept} | {i["addr"] for i in inst if kind_at.get(i["addr"]) == "V->S"})
    if not cuts:
        return [idx]
    pieces, cur, c = [], [], 0
    for i in idx:
        a = run.syms[i]["addr"]
        if c < len(cuts) and a >= cuts[c]:
            if cur:
                pieces.append(cur)
                cur = []
            while c < len(cuts) and a >= cuts[c]:
                c += 1
        cur.append(i)
    if cur:
        pieces.append(cur)
    return pieces


def foreign_first_reader_note(an, r, idx, unit_name):
    """The uniform note of a medium row: another unit also reads the row's first symbol (an extern, or a read the link order forces into this data)."""
    first = r["run"].syms[idx[0]]
    others = sorted(x for x in reader_names(an.ctx, first) if x != unit_name)
    if not others:
        return ""
    return "the first symbol %s is also read by %s (a foreign read: an extern, or one the link order forces into this unit's data)" % (
        first["name"], ", ".join(others[:3]) + (" ..." if len(others) > 3 else ""))


def ov_finding(ov):
    """What an override says for itself: its note, else its first evidence finding, else that it is the orchestrator's decision."""
    return ov["note"] or (ov["evidence"][0]["finding"] if ov["evidence"] else "decided by the orchestrator")


def row_note(*parts):
    return "; ".join(p for p in parts if p)


def build_records(an, window):
    """The proposal entries of every run as records: `(records, unowned)`.

    A record is `{"entry": attach row, "inw": the owner starts in the window, "sec", "start", "end", "run", "idx"}`; `unowned` holds the window's
    deferrals (`ambiguous` over an interval of units, `interleave` of two units' alternating data, `reader-vs-pointer` where a reader and the pointer
    tables disagree, `pool-order` given back, `unread`).  A decided block is cut at the strong `.data` seams inside it (`seam_pieces`) and graded by
    its anchors: strong needs every symbol decided by readers (or a derived signal as good as one, `Analysis.derived_strong`), no contradiction and at
    least `max(ANCHOR_MIN, N * ANCHOR_SHARE)` symbols read by the unit."""
    records, unowned = [], []
    dec = an.decisions()
    for ri, r in enumerate(an.res):
        run = r["run"]
        sec = run.sec
        sd = [(lo is not None and lo == hi) for lo, hi in r["adm_strong"]]
        sd_same = [sd[i] and r["adm_strong"][i][0] == r["adm"][i][0] for i in range(len(run.syms))]
        contra = set(r["contra"])
        pending = []                                           # (segment, candidate names, kind, owner) given back by the pool order, or open

        segno = [0]

        def emit(unit, k, idx, inversions, override=None):
            nsym = len(idx)
            counts = signal_counts(r, idx)
            strong_ok = [sd_same[i] or (k is not None and an.derived_strong(r, i, k)) for i in idx]
            weak = [i for i, ok in zip(idx, strong_ok) if not ok]
            touched = [i for i in idx if i in contra]
            anchors = [i for i in idx if r["label"][i]]
            n_read = len(anchors)
            need = min(nsym, max(ANCHOR_MIN, nsym * ANCHOR_SHARE))
            thin = n_read < need
            rv_notes = [r["rvp_note"][i] for i in idx if i in r["rvp_note"]]          # a reader-vs-pointer disagreement the link order kept in this row
            grade = "strong" if not weak and not touched and not thin and not rv_notes else "medium"
            start, end = tight_range(an, r, None, k, idx) if k is not None else (run.syms[idx[0]]["addr"], sym_end(run.syms[idx[-1]]))
            cmd = explain_cmd(unit.name, sec)
            evid = []
            for i in ([anchors[0], anchors[-1]] if len(anchors) > 1 else anchors):
                evid.append({"tool": "callers", "command": "python tools/units/callers.py 0x%08X" % run.syms[i]["addr"],
                             "finding": finding_for(an, r, i, unit.name)})
            n_derived = sum(1 for i in idx if not r["label"][i] and r["dlabel"][i])
            n_forced = nsym - n_read - n_derived
            dl = ", ".join(sorted({r["dlabel"][i] for i in idx if r["dlabel"][i] and not r["label"][i]}))
            if anchors:
                a0, a1 = run.syms[anchors[0]], run.syms[anchors[-1]]
                finding = ("%d symbol(s): %d read by %s (first 0x%08X, last 0x%08X)%s%s" % (
                    nsym, n_read, unit.name, a0["addr"], a1["addr"],
                    (", %d unread one(s) forced to it by the non-decreasing link order between its neighbours' anchors" % n_forced) if n_forced else "",
                    (", %d placed by pointer-derived evidence (%s)" % (n_derived, dl)) if n_derived else ""))
            else:
                finding = ("%d symbol(s), none read by %s's text: %d forced by link order, %d placed by pointer-derived evidence (%s)"
                           % (nsym, unit.name, n_forced, n_derived, dl or "none"))
            evid.append({"tool": "dataattach", "command": cmd, "finding": finding})
            entry = {"unit": unit.name, "section": sec, "range": [hx(start), hx(end)], "grade": grade,
                     "signal": max(sorted(counts), key=counts.get) if counts else "forced", "symbols": nsym, "bytes": end - start,
                     "signals": counts, "evidence": evid, "reproduce": cmd}
            if unit.first(".text") is not None:
                entry["text_addr"] = hx(unit.first(".text"))
            notes = []
            if inversions:
                notes.append("the first use of the unit's %s pool goes down at %s (after %s): its literals are TU-local (the reader owns them), so "
                             "the unit holds two TUs' pool order - an open question of the unit" % (sec, inversions[0][0], inversions[0][1]))
            if touched:
                notes.append("%d symbol(s) are read by another unit than the one link order puts them in (a global read where it is not defined): %s"
                             % (len(touched), ", ".join("%s read by %s" % (run.syms[i]["name"], "/".join(sorted(r["names"][i]))) for i in touched[:4])))
            if thin:
                notes.append("only %d of %d symbol(s) are read by the unit (a strong block needs %g, max(%d, N/%d)): the rest is placed by link order or pointer evidence"
                             % (n_read, nsym, need, ANCHOR_MIN, round(1 / ANCHOR_SHARE)))
            notes += rv_notes
            if grade == "medium":
                notes.append(foreign_first_reader_note(an, r, idx, unit.name))
            if notes:
                entry["note"] = row_note(*notes)
            return {"entry": entry, "inw": in_window(unit, window), "sec": sec, "start": start, "end": end, "run": ri, "idx": idx, "inversions": inversions,
                    "seg": (ri, segno[0])}

        for seg in an.segments(r):
            kk = seg["key"]
            segno[0] += 1
            if kk[0] == "unit":
                k = kk[1]
                unit = run.chain[k]
                keep, given, inversions = pool_cutback(an.ctx, run, seg["idx"], unit, dec)
                breaks = inversions if sec != ".sdata2" else []        # a string block's breaks count only when the block is kept whole
                if sec != ".sdata2":
                    inversions = []
                for g in given:
                    nb = pool_candidates(an, run, g, k, seg["idx"])
                    if nb == [unit.name]:                       # nobody else uses them: the unit's own, its pool just holds two TUs' order
                        keep = sorted(keep + g)
                        inversions = breaks
                        continue
                    pending.append(({"key": ("amb", k, k), "idx": g}, nb, "pool-order", unit.name))
                for piece in seam_pieces(an, run, keep, unit):
                    records.append(emit(unit, k, piece, inversions))
            elif kk[0] == "ovr":
                ov = an.overrides[kk[1]]
                if ov["action"] == "attach" and ov.get("takes_from"):
                    pass                                       # one direct row over the whole range (`takes_from_records`)
                elif ov["action"] == "attach":
                    rec = emit(an.cand_unit(ov["unit"]), None, seg["idx"], [], override=ov)
                    e = rec["entry"]
                    e.update(grade=ov["grade"], signal="orchestrator", signals={"orchestrator": len(seg["idx"])}, evidence=ov["evidence"],
                             reproduce=ov["reproduce"])
                    e["note"] = row_note("orchestrator override %d: %s" % (ov["id"], ov_finding(ov)))
                    rec["override"] = ov["id"]
                    records.append(rec)
                elif ov["action"] == "defer":
                    pending.append((seg, ov["candidates"], "override", ov))
                # exclude: the symbols are neither attached nor deferred
            else:
                pending.append((seg, None, None, None))
        for seg, cnames, forced_kind, owner in pending:
            idx = seg["idx"]
            nsym = len(idx)
            kk = seg["key"]
            if forced_kind == "pool-order":
                cv, kindname = None, "pool-order"
                cands = cnames
            elif forced_kind == "override":
                cv, kindname = None, "orchestrator"
                cands = cnames
            elif kk[0] == "zone":
                a, b, _ix = r["zones"][kk[1]]
                cv, kindname = list(range(a, b + 1)), "interleave"
                cands = [run.chain[x].name for x in cv]
            elif kk[0] == "rvp":
                grp = an.rvp_groups[kk[1]]
                cv, kindname = None, "reader-vs-pointer"
                cands = grp["units"]
            else:
                cv, kindname = list(range(kk[1], kk[2] + 1)), "ambiguous"
                cands = [run.chain[x].name for x in cv]
            if cv is not None and not any(in_window(run.chain[x], window) for x in cv):
                continue
            if cv is None and not any(in_window(u, window) for u in run.chain if u.name in cands) and not any(
                    in_window(an.cand_unit(c), window) for c in cands if an.cand_unit(c) is not None):
                continue
            if kindname == "ambiguous":
                # the units that read the segment's symbols and lie in the interval are the plausible owners (a global is defined by one of its readers)
                rd = sorted({k2 for i in idx for k2 in r["strong"][i] if kk[1] <= k2 <= kk[2]})
                if rd:
                    cands = [run.chain[x].name for x in rd]
                    if not any(in_window(run.chain[x], window) for x in rd):
                        continue                 # every plausible owner is another window's unit
                else:
                    kindname = "unread"          # nothing reads these symbols: no unit has a use of them to be undecided about
            start, end = run.syms[idx[0]]["addr"], sym_end(run.syms[idx[-1]])
            cmd = explain_cmd(cands[0], sec) if cands else "python tools/splits/dataattach.py --explain 0x%08X" % start
            ud = {"section": sec, "range": [hx(start), hx(end)], "kind": kindname, "symbols": nsym, "bytes": end - start,
                  "candidates": cands if len(cands) <= 8 else cands[:4] + ["... (%d units)" % len(cands)] + cands[-3:],
                  "evidence": [{"tool": "dataattach", "command": cmd, "finding": unowned_finding(an, r, idx, kindname, cands, kk, owner)}],
                  "reproduce": cmd}
            if kindname == "interleave":
                ud["signals"] = signal_counts(r, idx)
            elif kindname == "pool-order":
                ud["note"] = ("the pool of %s would go down in first-use order here (a pool is one per TU: two TUs' pools in one block); only its in-order "
                              "stretch is decided, the rest is open between it and its neighbours" % owner)
            elif kindname == "reader-vs-pointer":
                ud["note"] = an.rvp_groups[kk[1]]["why"]
            elif kindname == "orchestrator":
                ud["note"] = "orchestrator override %d: %s" % (owner["id"], ov_finding(owner))
                ud["reason"] = "deferred by an orchestrator override"
            else:
                dn = sorted({(run.chain[k].name, r["dlabel"][i]) for i in idx for k in r["derived"][i] if not kk[1] <= k <= kk[2]})
                if dn:
                    ud["note"] = ("derived evidence names %s, outside the interval: the monotone link order puts the symbol before data that a unit "
                                  "inside the interval reads (that unit may be the head of the named one's TU)" % ", ".join("%s (%s)" % t for t in dn[:3]))
            unowned.append(ud)
    return records, unowned


def unowned_finding(an, r, idx, kindname, cands, kk=None, owner=None):
    run = r["run"]
    if kindname == "interleave":
        zone = r["zone_of"][idx[0]]
        cn = [i for i in r["contra"] if r["zone_of"][i] == zone]
        sample = ", ".join("%s read by %s" % (run.syms[i]["name"], "/".join(sorted(r["names"][i]))) for i in cn[:3])
        return ("the data of %s alternates (%d symbol(s) are read by a unit other than the one their position implies: %s): one TU, or one reads "
                "the other's data" % (" / ".join(stem_of(c) for c in cands[:4]), len(cn), sample))
    if kindname == "pool-order":
        return "%d pool symbol(s) first used in an earlier function than the literal before them inside one owner's block: not first-use order" % len(idx)
    if kindname == "reader-vs-pointer":
        return "%d symbol(s): %s" % (len(idx), an.rvp_groups[kk[1]]["why"])
    if kindname == "orchestrator":
        return ov_finding(owner)
    read = [i for i in idx if r["strong"][i]]
    if read:
        return ("%d symbol(s), %d read (by %s): the monotone link order leaves the owner open among those units - a global is defined in one of its readers"
                % (len(idx), len(read), ", ".join(stem_of(c) for c in cands[:5])))
    return ("%d symbol(s) nothing reads: the boundary between %s and %s lies somewhere in them (last anchor of the first, first anchor of the second)"
            % (len(idx), stem_of(cands[0]), stem_of(cands[-1])))


# ---- data-only units that are a fragment of a neighbour, extab entries, linker tables ------------------------------------------------

def find_folds(an, window):
    """A data-only registered unit Z whose neighbours' data in a section belongs to ONE text unit X on both sides is a fragment of X's TU: X's data is one
    contiguous fragment of the section, so the range between its two parts is its own.  The left flank is the symbol before Z's range (X owns it,
    registered or decided); the right flank is the first symbol after Z's range that anything reads (the run after Z cannot name X as a candidate - Z
    stands between - so its readers are looked at directly): read by X alone, or already X's.  -> `(rows, questions)`: one `fold-data-only` attach row
    per range of Z (`takes_from` Z; strong when X's text reads both flanks, else medium) and one question per Z."""
    ctx = an.ctx
    dec = an.decisions()
    by_sec = collections.defaultdict(list)
    for s in ctx.data_syms:
        if not (s["type"] == "label" and not s["size"]):
            by_sec[s["section"]].append(s)
    rows, questions = [], []

    def owner_of(sec, s):
        u = ctx.owner(sec, s["addr"])
        return u.name if u is not None else dec.get((sec, s["addr"]))

    for z in an.cand.units:
        if z.first(".text") is not None or z.first(".init") is not None or not z.ranges:
            continue
        found = []
        for sec in DATA_SECS:
            lst = by_sec[sec]
            starts = [x["addr"] for x in lst]
            for z0, z1, _a in z.ranges.get(sec, []):
                sin = collections.defaultdict(list)            # unit -> [(symbol, site)] its own __sinit constructs inside this range
                for sym in lst:
                    if z0 <= sym["addr"] < z1:
                        for un, sites in ctx.definers(sym).items():
                            sin[un].append((sym, sites[0]))
                if len(sin) == 1:                              # one unit's own __sinit constructs what this unit holds: the unit is a fragment of that TU
                    un = next(iter(sin))
                    u2 = ctx.by_name.get(un)
                    if u2 is not None and un != z.name and u2.first(".text") is not None and in_window(u2, window):
                        found.append((sec, z0, z1, un, ("sinit", sin[un]), None, u2))
                        continue
                li, ri = bisect.bisect_left(starts, z0) - 1, bisect.bisect_left(starts, z1)
                if li < 0 or ri >= len(lst):
                    continue
                left = lst[li]
                x = owner_of(sec, left)
                xu = ctx.by_name.get(x) if x else None
                if xu is None or x == z.name or xu.first(".text") is None or not in_window(xu, window):
                    continue
                right = None
                for s in lst[ri:]:                             # the first symbol after Z that something reads, or a registered unit owns
                    own = ctx.owner(sec, s["addr"])
                    rd = {own.name} if own is not None else set(reader_names(ctx, s))
                    if rd:
                        right = (s, rd)
                        break
                    if s["addr"] - z1 > 0x10000:
                        break
                if right is None or right[1] != {x}:
                    continue
                found.append((sec, z0, z1, x, left, right[0], xu))
        if len({f[3] for f in found}) > 1:
            questions.append({"unit": z.name, "section": ",".join(sorted({f[0] for f in found})),
                              "question": "registered data-only unit %s lies between the data of %s: its sections disagree on the unit it is a fragment of; no fold proposed"
                                          % (z.name, " and ".join(sorted({f[3] for f in found}))),
                              "reproduce": "python tools/splits/dataattach.py --explain %s" % stem_of(found[0][3])})
            continue
        if found:                                              # a unit with no text is one fragment: every range of it follows the unit one section proved
            x, xu = found[0][3], found[0][6]
            have = {(f[0], f[1]) for f in found}
            for sec in DATA_SECS:
                for z0, z1, _a in z.ranges.get(sec, []):
                    if (sec, z0) not in have:
                        lst = by_sec[sec]
                        found.append((sec, z0, z1, x, None, None, xu))
            found.sort(key=lambda f: (sc.SECTION_ORDER.index(f[0]), f[1]))
        for sec, z0, z1, x, left, right, xu in found:
            if left is None:
                cmd = "python tools/splits/dataattach.py --explain %s --section %s" % (stem_of(x), sec)
                rows.append({"unit": x, "text_addr": hx(xu.first(".text")), "section": sec, "range": [hx(z0), hx(z1)], "grade": "medium",
                             "kind": "fold-data-only", "takes_from": z.name,
                             "symbols": len([s for s in by_sec[sec] if z0 <= s["addr"] < z1]),
                             "evidence": [{"tool": "dataattach", "command": cmd,
                                           "finding": "%s has no text: it is one fragment of %s's TU (its other sections lie between that unit's data), so this range follows" % (z.name, x)}],
                             "reproduce": cmd})
                continue
            cmd = "python tools/splits/dataattach.py --explain %s --section %s" % (stem_of(x), sec)
            if isinstance(left, tuple):                        # the sinit route: strong
                (sym0, site0) = left[1][0]
                fn0 = ctx.fn_at(site0)
                rows.append({"unit": x, "text_addr": hx(xu.first(".text")), "section": sec, "range": [hx(z0), hx(z1)], "grade": "strong",
                             "kind": "fold-data-only", "takes_from": z.name, "signal": "sinit",
                             "symbols": len([t for t in by_sec[sec] if z0 <= t["addr"] < z1]),
                             "evidence": [{"tool": "callers", "command": "python tools/units/callers.py %s" % hx(sym0["addr"]),
                                           "finding": "%d symbol(s) of %s's %s range are constructed by %s's own __sinit; first %s at 0x%08X in %s: %s is a fragment of that TU"
                                                      % (len(left[1]), z.name, sec, x, sym0["name"], site0, fn0["name"] if fn0 else "?", z.name)}],
                             "reproduce": cmd})
                continue
            strong = bool(reader_names(ctx, left).get(x)) and bool(reader_names(ctx, right).get(x))
            rows.append({"unit": x, "text_addr": hx(xu.first(".text")), "section": sec, "range": [hx(z0), hx(z1)], "grade": "strong" if strong else "medium",
                         "kind": "fold-data-only", "takes_from": z.name,
                         "symbols": len([s for s in by_sec[sec] if z0 <= s["addr"] < z1]),
                         "evidence": [{"tool": "dataattach", "command": cmd,
                                       "finding": "%s holds %s just before %s's %s range and %s just after it: a unit's data is one fragment, so the data-only "
                                                  "range between them is %s's" % (x, left["name"], z.name, sec, right["name"], x)},
                                      {"tool": "callers", "command": "python tools/units/callers.py %s" % hx(left["addr"]),
                                       "finding": "left flank: " + finding_for_sym(ctx, left, x)},
                                      {"tool": "callers", "command": "python tools/units/callers.py %s" % hx(right["addr"]),
                                       "finding": "right flank: " + finding_for_sym(ctx, right, x)}],
                         "reproduce": cmd})
        if found:
            xs = sorted({f[3] for f in found})
            secs = sorted({f[0] for f in found})
            questions.append({"unit": z.name, "section": ",".join(secs),
                              "question": "registered data-only unit %s is a fragment of %s's TU (its data lies between that unit's data in %s); fold it "
                                          "(its data goes to %s by the rows above; the unit has no text)" % (z.name, ", ".join(xs), ", ".join(secs), ", ".join(xs)),
                              "reproduce": "python tools/splits/dataattach.py --explain %s" % stem_of(xs[0])})
    return rows, questions


def sinit_owner_questions(an, folded, window):
    """The data a unit's range holds although ANOTHER unit's own `__sinit` constructs it (`vec_pair_*` of em010's range, built by em008's and em011's sinits): the
    solver does not move data a range already owns, so each such group is an open question naming both units and the interval - a recut the orchestrator
    decides (an `attach` override with `takes_from`).  A unit already folded into the definer (a data-only fragment) is that fold's, not a question."""
    ctx = an.ctx
    groups = collections.defaultdict(list)
    for i, d in sorted(ctx.sinit_definers().items()):
        sym = ctx.data_syms[i]
        o = ctx.owner(sym["section"], sym["addr"])
        if o is None or o.name in d or o.name in folded:
            continue
        groups[(o.name, sym["section"], tuple(sorted(d)))].append(sym)
    out = []
    for (owner, sec, definers), syms in sorted(groups.items()):
        if not any(in_window(ctx.by_name[x], window) for x in definers if x in ctx.by_name) and not in_window(ctx.by_name[owner], window):
            continue
        s0, s1 = syms[0], syms[-1]
        fn = ctx.fn_at(ctx.definers(s0)[definers[0]][0])
        out.append({"unit": owner, "section": sec,
                    "question": "%d symbol(s) of %s's %s range (%s..%s) are constructed by %s's own __sinit (%s at 0x%08X), not by %s: the range may be %s's - a recut "
                                "(an `attach` override with `takes_from`)" % (len(syms), owner, sec, s0["name"], s1["name"], ", ".join(definers), fn["name"] if fn else "?",
                                                                              ctx.definers(s0)[definers[0]][0], owner, definers[0]),
                    "candidate_interval": [hx(s0["addr"]), hx(sym_end(s1))],
                    "reproduce": "python tools/units/callers.py %s" % hx(s0["addr"])})
    return out


def finding_for_sym(ctx, s, unit_name):
    rd = [x for x in (ctx.literal_readers(s) if sc.is_literal(s) else ctx.readers(s)) if ctx.text_owner(x) is not None and ctx.text_owner(x).name == unit_name]
    if rd:
        return "%s (%s 0x%08X) is read at 0x%08X in %s" % (s["name"], s["section"], s["addr"], min(rd), unit_name)
    return "%s (%s 0x%08X) is held by %s (owned or decided), no decoded read of it there" % (s["name"], s["section"], s["addr"], unit_name)


def extab_rows(cand, ctx, dol, symbols, window):
    """The `extab`/`extabindex` entries past the last registered range: each goes to the unit that owns its function (`derive_attached`'s own rule);
    -> `(attach rows, unowned rows)`; the linker's `_eti_init_info` is nobody's (a deferral with `linker-generated`)."""
    etb = {s["addr"]: s["size"] for s in symbols if s["section"] == "extab"}
    atts, unowned = [], []
    per = collections.OrderedDict()
    for s in sorted((x for x in symbols if x["section"] == "extabindex"), key=lambda x: x["addr"]):
        a = s["addr"]
        if ctx.owner("extabindex", a) is not None:
            continue
        if s["name"] in LINKER_GENERATED:
            continue
        fn, ex = dol.word(a), dol.word(a + 8)
        o = ctx.text_owner(fn) if fn else None
        if o is None or not in_window(o, window):
            continue
        d = per.setdefault(o.name, {"unit": o, "eti": [], "etb": [], "fn": []})
        d["eti"].append((a, a + 12))
        d["etb"].append((ex, ex + etb.get(ex, 8)))
        d["fn"].append(fn)
    for name, d in per.items():
        for sec, rr in (("extabindex", d["eti"]), ("extab", d["etb"])):
            for a, b in sc.coalesce(rr):
                cmd = "python tools/splits/dataattach.py --explain %s --section %s" % (hx(a), sec)
                atts.append({"unit": name, "text_addr": hx(d["unit"].first(".text")), "section": sec, "range": [hx(a), hx(b)], "grade": "strong",
                             "signal": "extab-owner", "symbols": len([x for x in d["eti"] if a <= x[0] < b]) if sec == "extabindex" else len(d["etb"]),
                             "evidence": [{"tool": "dataattach", "command": cmd,
                                           "finding": "%d extabindex entries name functions %s inside %s (derive_attached's own rule: the entry follows its function)"
                                                      % (len(d["eti"]), ", ".join(hx(f) for f in d["fn"][:3]) + (" ..." if len(d["fn"]) > 3 else ""), name)}],
                             "reproduce": cmd})
    return atts, unowned


def linker_rows(ctx, symbols):
    """What no unit can own: the linker's own tables (`.init` `_rom_copy_info`/`_bss_init_info`, `extabindex` `_eti_init_info`)."""
    out = []
    for s in sorted(symbols, key=lambda x: (sc.SECTION_ORDER.index(x["section"]) if x["section"] in sc.SECTION_ORDER else 99, x["addr"])):
        if s["name"] in LINKER_GENERATED and ctx.owner(s["section"], s["addr"]) is None:
            cmd = "python tools/units/callers.py %s" % hx(s["addr"])
            out.append({"section": s["section"], "range": [hx(s["addr"]), hx(s["addr"] + (s["size"] or 4))], "symbols": 1, "bytes": s["size"] or 4,
                        "kind": "linker-generated", "candidates": [],
                        "reason": "`%s` is a table mwldeppc emits (docs/init-section.md, playbook 74): no unit owns it" % s["name"],
                        "evidence": [{"tool": "callers", "command": cmd, "finding": "no unit's text defines it; the linker materialises it"}], "reproduce": cmd})
    return out


# ---- orchestrator overrides ----------------------------------------------------------------------------------------------------------

OVERRIDE_ACTIONS = ("attach", "defer", "exclude")


def load_overrides(path):
    """The rows of an overrides file: `{"overrides": [row, ...]}` (or a bare list); each row is one range the engine cannot derive."""
    with open(path, encoding="utf-8") as fh:
        d = json.load(fh)
    rows = d.get("overrides") if isinstance(d, dict) else d
    if not isinstance(rows, list):
        raise SystemExit("%s: want {\"overrides\": [...]}" % path)
    return rows


def resolve_overrides(an, rows, path=None):
    """Validate the override rows against the candidate and give each an `id` (its position): `[{"id", "action", "unit", "section", "start", "end",
    "grade", "signal", "evidence", "reproduce", "note", "candidates"}]`.

    A row is `{unit | text_addr, section, start, end (or range), action: attach|defer|exclude, grade, signal: "orchestrator", evidence: [{tool, command,
    finding}], note, reproduce, candidates, takes_from}`: `attach` gives the range to the unit (grade `strong|medium`, evidence required; with
    `takes_from` it is one row over bytes another unit owns, a recut, and the unit it names gives them up), `defer` lists it in
    `unowned_data` (`candidates`, else the unit), `exclude` leaves it in neither list.  Two overrides may not overlap in one section."""
    out = []
    for n, row in enumerate(rows):
        who = "override #%d" % n
        if not isinstance(row, dict):
            raise SystemExit("%s is %s, want an object" % (who, type(row).__name__))
        action = row.get("action")
        if action not in OVERRIDE_ACTIONS:
            raise SystemExit("%s: action %r (attach|defer|exclude)" % (who, action))
        sec = row.get("section")
        if sec not in DATA_SECS:
            raise SystemExit("%s: section %r is not a data section" % (who, sec))
        try:
            a, b = (sc.to_int(row["range"][0]), sc.to_int(row["range"][1])) if "range" in row else (sc.to_int(row["start"]), sc.to_int(row["end"]))
        except (KeyError, TypeError, ValueError, IndexError):
            raise SystemExit("%s: start/end (or range) missing or not addresses" % who)
        if b <= a:
            raise SystemExit("%s: empty range %s..%s" % (who, hx(a), hx(b)))
        unit = None
        if row.get("text_addr") not in (None, ""):
            ta = sc.to_int(row["text_addr"])
            unit = next((u for u in an.cand.units if u.first(".text") == ta), None)
            if unit is None:
                raise SystemExit("%s: no unit of the candidate holds .text at %s" % (who, hx(ta)))
        elif row.get("unit"):
            unit = an.cand_unit(row["unit"])
            if unit is None:
                raise SystemExit("%s: the candidate has no unit %r" % (who, row["unit"]))
        if action == "attach" and unit is None:
            raise SystemExit("%s: an attach needs a unit or a text_addr" % who)
        grade = row.get("grade", "medium")
        if action == "attach" and grade not in ("strong", "medium"):
            raise SystemExit("%s: grade %r (strong|medium; a guess is never applied)" % (who, grade))
        if row.get("signal", "orchestrator") != "orchestrator":
            raise SystemExit("%s: signal must be \"orchestrator\"" % who)
        ev = row.get("evidence") or []
        ev = [ev] if isinstance(ev, dict) else list(ev)
        if action == "attach" and not ev:
            raise SystemExit("%s: an attach needs evidence (tool, command, finding)" % who)
        for e in ev:
            if not (isinstance(e, dict) and all(e.get(x) for x in ("tool", "command", "finding"))):
                raise SystemExit("%s: evidence needs tool, command and finding" % who)
        cands = list(row.get("candidates") or ([unit.name] if unit is not None else []))
        donor = row.get("takes_from") or None
        if donor and (action != "attach" or an.cand_unit(donor) is None):
            raise SystemExit("%s: takes_from %r (an attach naming a unit of the candidate whose bytes it takes)" % (who, donor))
        out.append({"id": n, "action": action, "unit": unit.name if unit is not None else None, "section": sec, "start": a, "end": b, "grade": grade,
                    "takes_from": donor,
                    "signal": "orchestrator", "evidence": ev, "note": row.get("note", ""), "candidates": cands,
                    "reproduce": row.get("reproduce") or "python tools/splits/dataattach.py --overrides %s" % (path or "<overrides file>")})
    by = collections.defaultdict(list)
    for o in out:
        by[o["section"]].append(o)
    for sec, v in by.items():
        v.sort(key=lambda o: o["start"])
        for x, y in zip(v, v[1:]):
            if y["start"] < x["end"]:
                raise SystemExit("override #%d and #%d overlap in %s at %s" % (x["id"], y["id"], sec, hx(y["start"])))
    return out


def takes_from_records(an, window):
    """The direct attach records of the overrides that carry `takes_from`: one row over the whole range (owned bytes included), the donor named."""
    out = []
    for o in an.overrides:
        if o["action"] != "attach" or not o.get("takes_from"):
            continue
        unit = an.cand_unit(o["unit"])
        syms = [x for x in an.ctx.data_syms if x["section"] == o["section"] and o["start"] <= x["addr"] < o["end"] and not (x["type"] == "label" and not x["size"])]
        entry = {"unit": unit.name, "section": o["section"], "range": [hx(o["start"]), hx(o["end"])], "grade": o["grade"], "signal": "orchestrator",
                 "symbols": len(syms), "bytes": o["end"] - o["start"], "signals": {"orchestrator": len(syms)}, "evidence": o["evidence"],
                 "reproduce": o["reproduce"], "takes_from": o["takes_from"],
                 "note": row_note("orchestrator override %d: %s" % (o["id"], ov_finding(o)))}
        if unit.first(".text") is not None:
            entry["text_addr"] = hx(unit.first(".text"))
        out.append({"entry": entry, "inw": in_window(unit, window), "sec": o["section"], "start": o["start"], "end": o["end"], "run": None, "idx": [],
                    "inversions": [], "override": o["id"], "seg": ("ovr", o["id"])})
    return out


def override_outcomes(an, records, unowned):
    """What became of each override: `applied` / `blocked` (an attach whose invariant check failed) / `deferred` / `excluded`, with the symbols it covered."""
    covered = collections.Counter(o["id"] for o in an.ovr.values())
    out = []
    for o in an.overrides:
        recs = [x for x in records if x.get("override") == o["id"]]
        blocked = [x for x in recs if x.get("blocked")]
        if covered[o["id"]] == 0:
            outcome = "no unowned symbol in the range"
        elif o["action"] == "attach":
            outcome = "blocked" if blocked and len(blocked) == len(recs) else "applied" if not blocked else "partly blocked"
        else:
            outcome = {"defer": "deferred", "exclude": "excluded"}[o["action"]]
        row = {"id": o["id"], "action": o["action"], "unit": o["unit"], "section": o["section"], "start": hx(o["start"]), "end": hx(o["end"]),
               "symbols": covered[o["id"]], "outcome": outcome}
        if blocked:
            row["blocked_by"] = ["%s: %s" % (x["blocked"][0], x["blocked"][1][:160]) for x in blocked]
        out.append(row)
    return out


# ---- settling ---------------------------------------------------------------------------------------------------------------------

def public(entry):
    return {k: v for k, v in entry.items() if not k.startswith("_")}


def phase1_inputs(paths):
    """The proposals the analysis runs on: their phase 2 rows are dropped (they are what this tool derives)."""
    out = []
    for p in paths:
        d = sc.load_proposal(p)
        out.append({k: v for k, v in d.items() if k not in PHASE2_KEYS})
    return out


def default_proposals():
    """Phase 1 a..g, `phase1-reconcile.json` and (when it exists) `phase2-folds.json`: the text folds, data-only units and moves phase 2 accepted."""
    files = (sorted(glob.glob(os.path.join(PROPOSAL_DIR, "phase1-[a-g].json"))) + [os.path.join(PROPOSAL_DIR, "phase1-reconcile.json"),
                                                                              os.path.join(PROPOSAL_DIR, "phase2-folds.json")])
    return [f for f in files if os.path.isfile(f)]


def strip_provisional(cand, info):
    """The candidate without the PROVISIONAL by-reader data of the recut units (`splitcheck --data-by-reader`): those symbols become unowned
    runs this tool decides with evidence, and an attachment replaces the provisional range when the proposals are rendered."""
    cut = collections.defaultdict(list)                          # (unit, section) -> [(a, b)]
    for rec in info.get("data_by_reader", []):
        for label, (a, b) in rec["pieces"].items():
            cut[(label.replace(" (remnant)", ""), rec["section"])].append((sc.to_int(a), sc.to_int(b)))
    units = []
    for u in cand.units:
        nu = sc.Unit(u.name, u.attrs, {k: list(v) for k, v in u.ranges.items()})
        for (name, sec), rr in cut.items():
            if name == u.name and sec in nu.ranges:
                nu.ranges[sec] = [(fs, fe, x) for s0, e0, x in nu.ranges[sec] for fs, fe in sc.subtract([(s0, e0)], rr)]
                if not nu.ranges[sec]:
                    del nu.ranges[sec]
        units.append(nu)
    return sc.Splits(list(cand.header), units)


def analysis_stamp(paths, extra=()):
    """The key of a cached analysis: the stat (size, mtime) of the map, `splits.txt`, the DOL, every proposal file and the three tool sources it is
    built by, and the `extra` proposals' content - anything that changes the result changes the key."""
    root = sc.tree_root()
    files = [os.path.join(root, "config", sc.GAME, "splits.txt"), os.path.join(root, "config", sc.GAME, "symbols.txt"),
             sc.find_file(os.path.join("orig", sc.GAME, "sys", "main.dol"), root)] + list(paths) + [os.path.join(HERE, n) for n in ("dataattach.py", "splitcheck.py", "dataorder.py")]
    return stamp_of(files, extra), root


def stamp_of(files, extra=()):
    """A short hash of the stat (path, size, mtime) of `files` and the content of the `extra` proposals."""
    import hashlib
    h = hashlib.sha1()
    for f in files:
        st = os.stat(f)
        h.update(("%s %d %d\n" % (os.path.abspath(f), st.st_size, st.st_mtime_ns)).encode())
    h.update(json.dumps(list(extra), sort_keys=True, default=str).encode())
    return h.hexdigest()[:20]


CACHE = [True]


def load_analysis(paths, extra=(), cache=None):
    """`(Analysis, render info)` of the candidate the proposals `paths` (phase 2 rows dropped) and the `extra` proposals (e.g. the folds) render to.

    Building it decodes the whole `.text` (about 12 s); the result is pickled to `build/tmp/dataattach/` (gitignored) under `analysis_stamp`, so a batch of
    `--explain` calls pays once (0.2 s a call after).  `cache=False` (`--no-cache`) always rebuilds; a cache that does not load is rebuilt."""
    if cache is None:
        cache = CACHE[0]
    if cache:
        import pickle
        key, root = analysis_stamp(paths, extra)
        cdir = os.path.join(root, "build", "tmp", "dataattach")
        cpath = os.path.join(cdir, "analysis-%s.pkl" % key)
        try:
            with open(cpath, "rb") as fh:
                return pickle.load(fh)
        except Exception:                                      # missing, stale format, truncated: rebuild
            pass
        out = load_analysis(paths, extra, cache=False)
        try:
            os.makedirs(cdir, exist_ok=True)
            for old in sorted(glob.glob(os.path.join(cdir, "analysis-*.pkl")), key=os.path.getmtime)[:-3]:      # keep the newest few (generate builds two)
                os.remove(old)
            tmp = cpath + ".tmp%d" % os.getpid()
            with open(tmp, "wb") as fh:
                pickle.dump(out, fh, protocol=pickle.HIGHEST_PROTOCOL)
            os.replace(tmp, cpath)
        except OSError:
            pass
        return out
    splits, symbols, dol = sc.load_ctx()
    props = phase1_inputs(paths) + list(extra)
    cand, info = sc.render(splits, props, dol, symbols)
    if info["issues"]:
        raise SystemExit("the proposals do not render clean: " + "; ".join(info["issues"][:3]))
    base_ctx = sc.Ctx(cand, symbols, dol)                      # the phase 1 candidate: the invariants' baseline
    ctx = sc.Ctx(strip_provisional(cand, info), symbols, dol, base_ctx.sda13, base_ctx.sda2)
    import dataorder as do
    kind = {y.addr: y for y in do.classify_all(sc.dataorder_rows(symbols), dol)}
    an = Analysis(ctx.splits, ctx, kind)
    an.splits, an.symbols, an.dol, an.props, an.base_ctx = splits, symbols, dol, props, base_ctx
    an.provisional = sum(len(r["pieces"]) for r in info.get("data_by_reader", []))
    return an, info


def check_candidate(an, rows, lane, moves=()):
    """Render the proposals plus the live rows (and the `moves`) and run every invariant: `(candidate, info, results, ctx)`."""
    doc = {"phase": 2, "lane": lane, "attach": rows, "unowned_data": [], "moves": list(moves)}
    cand, info = sc.render(an.splits, an.props + [doc], an.dol, an.symbols)
    ctx2 = sc.Ctx(cand, an.symbols, an.dol, an.base_ctx.sda13, an.base_ctx.sda2)
    res = sc.run_checks(ctx2, sc.dataorder_rows(an.symbols))
    return cand, info, res, ctx2


def derive_moves(ctx, moved):
    """`moves` rows for the data-only units the attachments left out of link order: a unit with no text is ordered by its file position alone, so one
    whose range lies by address between two units that precede it in the file goes right behind the in-order unit before it (`splitcheck.
    file_order_offenders`).  Several behind one anchor chain, in address order.  `moved` (names already moved) is extended."""
    rows, last = [], {}
    for sec, name, addr, finding, before in sorted(sc.file_order_offenders(ctx), key=lambda o: (sc.SECTION_ORDER.index(o[0]), o[2])):
        u = ctx.by_name[name]
        if before is None or name in moved or u.first(".text") is not None or u.first(".init") is not None:
            continue
        rows.append({"unit": name, "after": last.get(before, before), "reason": finding})
        last[before] = name
        moved.add(name)
    return rows


#: the data sections an invariant reads, for attributing a failure whose address lies outside every attached range
INV_SECTIONS = {"data-order": (".data",), "vtable": (".data",), "jumptable": (".data", ".rodata"), "pool": (".sdata", ".sdata2"),
                "bss": (".bss", ".sbss", ".sbss2"), "local-static": (".data", ".bss", ".sbss", ".sdata", ".rodata")}


def fail_items(res):
    out = {}
    for unit, recs in res.units.items():
        for inv, rec in recs.items():
            for it in rec["items"]:
                if it["status"] == sc.FAIL:
                    out[(unit, inv, it["finding"])] = it["addr"]
    return out


def settle(an, extra_rows, records, lane, log=print):
    """Block every attachment that makes an invariant fail that the phase 1 candidate does not fail, until none does.

    A new FAIL is attributed to the record whose range holds its address (the unit's own record first).  The blocked data goes to `unowned_data`
    (kind `multi-tu` or `invariant`) with the checker's finding as the evidence: the unit holds more than one TU, or the decided data is not the
    unit's.  Returns `(blocked, unattributed, final results)`."""
    base_res = sc.run_checks(an.base_ctx, sc.dataorder_rows(an.symbols))
    base_units = {(u, i) for (u, i, _f) in fail_items(base_res)}
    unattributed, blocked = [], []
    accepted = {}
    moves, moved = [], set()
    res = None
    for rnd in range(12):
        live = [public(x["entry"]) for x in records if not x.get("blocked")] + extra_rows
        cand, info, res, ctx2 = check_candidate(an, live, lane, moves)
        if info["issues"]:
            raise SystemExit("the attachments do not render clean: " + "; ".join(info["issues"][:3]))
        fresh = derive_moves(ctx2, moved)
        if fresh:                                              # the attachments put a data-only unit out of link order: place it, then check again
            moves += fresh
            log("settle round %d: %d data-only unit(s) moved to their link position" % (rnd, len(fresh)))
            continue
        now = fail_items(res)
        # new = a unit that did not fail the invariant in the phase 1 candidate (a unit that already fails it - an open pooldup question -
        # keeps failing, and the claimed pool then also reports the same duplicate in other words)
        new = sorted(((k, a) for k, a in now.items() if (k[0], k[1]) not in base_units), key=lambda t: (t[0], t[1] or 0))
        todo = []
        unattributed = []
        for (unit, inv, finding), addr in new:
            hit = [x for x in records if not x.get("blocked") and addr is not None and x["start"] <= addr < x["end"]]
            hit.sort(key=lambda x: x["entry"]["unit"] != unit)
            if not hit:
                secs = INV_SECTIONS.get(inv)
                hit = [x for x in records if not x.get("blocked") and x["entry"]["unit"] == unit and (secs is None or x["sec"] in secs)]
            if hit and inv == "pool" and finding.startswith("claimed pool: first use of") and hit[0].get("inversions"):
                accepted[id(hit[0])] = hit[0]               # the recorded two-TU signal of a TU-local pool: an open question, not a reason to defer
            elif hit:
                todo.append((hit[0], inv, finding))
            else:
                unattributed.append((unit, inv, finding))
        log("settle round %d: %d new FAIL item(s), %d attributable, %d not" % (rnd, len(new), len(todo), len(unattributed)))
        if not todo:
            break
        for rec, inv, finding in todo:
            if not rec.get("blocked"):
                rec["blocked"] = (inv, finding)
                blocked.append(rec)
            for x in records:                                  # a seam cut the block in pieces: what follows the blocked one goes with it (a hole in the unit's range fails `order`)
                if x.get("seg") == rec.get("seg") and x["start"] > rec["start"] and not x.get("blocked"):
                    x["blocked"] = (inv, "follows the blocked piece at 0x%08X of the same block: %s" % (rec["start"], finding))
                    blocked.append(x)
    return blocked, unattributed, res, [x for x in accepted.values() if not x.get("blocked")], moves


def blocked_to_unowned(rec):
    e = rec["entry"]
    inv, finding = rec["blocked"]
    kind = "multi-tu" if inv in ("data-order", "vtable", "jumptable", "pool", "bss", "local-static") else "invariant"
    cmd = e["reproduce"]
    return {"section": rec["sec"], "range": [hx(rec["start"]), hx(rec["end"])], "candidates": [e["unit"]], "kind": kind,
            "symbols": e["symbols"], "bytes": rec["end"] - rec["start"], "signals": e["signals"],
            "evidence": [{"tool": "splitcheck", "command": "python tools/splits/splitcheck.py --proposal <phase 1 files> --proposal <this file with the attachment> --only %s --unit %s"
                                                          % (inv, stem_of(e["unit"])), "finding": finding}],
            "reproduce": cmd,
            "note": ("orchestrator override %d blocked: attaching the range to %s makes the %s invariant fail" % (rec["override"], e["unit"], inv))
            if rec.get("override") is not None else
            "link order and evidence put these symbols in %s, but attaching them makes the %s invariant fail: the unit holds more than one TU, "
            "or one of its reads is not its own" % (e["unit"], inv)}


def run_table(an, records, window):
    """One row per run that holds a window unit: the symbols and bytes by fate (window / outside / deferred)."""
    rows = []
    for ri, r in enumerate(an.res):
        run = r["run"]
        if not any(in_window(u, window) for u in run.chain):
            continue
        rows.append({"section": run.sec, "start": run.syms[0]["addr"], "end": sym_end(run.syms[-1]), "symbols": len(run.syms),
                     "bytes": sym_end(run.syms[-1]) - run.syms[0]["addr"], "chain": len(run.chain), "prev": run.prev_owner.name if run.prev_owner else None,
                     "next": run.next_owner.name if run.next_owner else None,
                     "attached_window": sum(x["entry"]["symbols"] for x in records if x["run"] == ri and x["inw"] and not x.get("blocked")),
                     "attached_outside": sum(x["entry"]["symbols"] for x in records if x["run"] == ri and not x["inw"] and not x.get("blocked")),
                     "blocked": sum(x["entry"]["symbols"] for x in records if x["run"] == ri and x.get("blocked")),
                     "segments": len(an.segments(r))})
    return rows


def totals(records, unowned):
    """Counts per section for the window: attached by signal and grade, deferred by kind."""
    st = collections.defaultdict(collections.Counter)
    for x in records:
        if x["inw"] and not x.get("blocked"):
            e, sec = x["entry"], x["sec"]
            for sig, c in e["signals"].items():
                st[sec]["attached:" + sig] += c
            st[sec]["attached_symbols"] += e["symbols"]
            st[sec]["attached_bytes"] += e["bytes"]
            st[sec]["grade_" + e["grade"]] += e["symbols"]
    for u in unowned:
        sec = u["section"]
        st[sec]["deferred_" + u["kind"] + "_symbols"] += u["symbols"]
        st[sec]["deferred_" + u["kind"] + "_bytes"] += u["bytes"]
        st[sec]["deferred_symbols"] += u["symbols"]
        st[sec]["deferred_bytes"] += u["bytes"]
    return {k: dict(v) for k, v in st.items()}


def row_key(e):
    sec = e["section"]
    return (sc.SECTION_ORDER.index(sec) if sec in sc.SECTION_ORDER else 99, sc.to_int(e["range"][0]), e.get("unit") or "")


def proposal_doc(lane, window, attach, unowned, questions=None, moves=None):
    doc = {"phase": 2, "lane": lane, "window": [hx(window[0]), hx(window[1])], "units": [],
           "attach": sorted(attach, key=row_key),
           "unowned_data": sorted(unowned, key=lambda e: (sc.SECTION_ORDER.index(e["section"]) if e["section"] in sc.SECTION_ORDER else 99, sc.to_int(e["range"][0])))}
    if questions:
        doc["open_questions"] = questions
    if moves:
        doc["moves"] = moves
    return doc


# ---- reports ----------------------------------------------------------------------------------------------------------------------

def explain(an, rx, secs):
    """Print the segments (decided / interleave / ambiguous) of the units matching `rx`, or - when `rx` is an address - the run holding it with the
    evidence and the verdict of every symbol."""
    if re.match(r"^(0x)?[0-9A-Fa-f]{6,8}$", rx):
        addr = sc.to_int(rx if rx.lower().startswith("0x") else "0x" + rx)
        for r in an.res:
            run = r["run"]
            if not (run.syms[0]["addr"] <= addr < sym_end(run.syms[-1])):
                continue
            print("run %s %s..%s, %d symbols, previous owner %s, next owner %s, chain of %d units" % (
                run.sec, hx(run.syms[0]["addr"]), hx(sym_end(run.syms[-1])), len(run.syms), run.prev_owner.name if run.prev_owner else None,
                run.next_owner.name if run.next_owner else None, len(run.chain)))
            for i, s in enumerate(run.syms):
                lo, hi = r["adm"][i]
                verdict = (run.chain[lo].name if lo == hi else "%s..%s" % (run.chain[lo].name, run.chain[hi].name)) if lo is not None else "-"
                ev = ",".join(run.chain[k].name.split("/")[-1][:14] for k in sorted(r["strong"][i] or r["derived"][i]))[:30]
                print("%s %5x %-30s %-10s %-30s -> %s%s%s" % (hx(s["addr"]), s["size"] or 0, s["name"][:30], r["label"][i] or r["dlabel"][i] or "free", ev, verdict,
                                                           " [zone]" if r["zone_of"][i] is not None else "", " CONTRADICTION" if i in r["contra"] else ""))
            return 0
        print("%s is in no unowned run (it is in an owned range, or in no data symbol)" % hx(addr))
        return 0
    keep = re.compile(rx)
    shown = False
    for r in an.res:
        run = r["run"]
        if secs and run.sec not in secs:
            continue
        for seg in an.segments(r):
            kk = seg["key"]
            idx = seg["idx"]
            if kk[0] == "unit":
                names = [run.chain[kk[1]].name]
            elif kk[0] == "zone":
                names = [run.chain[x].name for x in range(r["zones"][kk[1]][0], r["zones"][kk[1]][1] + 1)]
            elif kk[0] == "rvp":
                names = list(an.rvp_groups[kk[1]]["units"])
            elif kk[0] == "ovr":
                names = [an.overrides[kk[1]]["unit"]]
            else:
                names = [run.chain[x].name for x in range(kk[1], kk[2] + 1)]
            if not any(keep.search(x) for x in names):
                continue
            a, b = run.syms[idx[0]], run.syms[idx[-1]]
            shown = True
            print("%-7s 0x%08X..0x%08X %4d sym %-9s %s  %s" % (run.sec, a["addr"], sym_end(b), len(idx), kk[0],
                                                                names[0] if len(names) == 1 else "%s..%s (%d)" % (names[0], names[-1], len(names)),
                                                                dict(signal_counts(r, idx))))
    for u in an.cand.units:                                   # a data-only unit is in no chain: its ranges are owned, so list them with who reads each symbol
        if u.first(".text") is not None or not keep.search(u.name):
            continue
        ctx = an.ctx
        for sec in DATA_SECS:
            if secs and sec not in secs:
                continue
            for z0, z1, _a in u.ranges.get(sec, []):
                rows = [x for x in ctx.data_syms if x["section"] == sec and z0 <= x["addr"] < z1 and not (x["type"] == "label" and not x["size"])]
                shown = True
                print("%-7s 0x%08X..0x%08X %4d sym owned    %s (data-only unit, no text)" % (sec, z0, z1, len(rows), u.name))
                for x in rows[:200]:
                    rd = sorted(reader_names(ctx, x))
                    print("        0x%08X %5x %-30s read by %s" % (x["addr"], x["size"] or 0, x["name"][:30], ",".join(n.split("/")[-1] for n in rd[:4]) or "-"))
    if not shown:
        print("no unit matches %r (a unit with no data here, or none of that name)" % rx)
    return 0


def holdout_runs(cand, info, symbols, batch, nb=3):
    """The owned data ranges of the candidate that batch `batch` hides: every `nb`-th range of a section (so a hidden range keeps both neighbours), of a
    unit with text, that is not a provisional by-reader piece and holds a map symbol.  `[(unit name, section, start, end)]`."""
    prov = collections.defaultdict(list)
    for rec in info.get("data_by_reader", []):
        for label, v in rec["pieces"].items():
            prov[(label.replace(" (remnant)", ""), rec["section"])].append((sc.to_int(v[0]), sc.to_int(v[1])))
    out = []
    for sec in DATA_SECS:
        own = sorted((s, e, u) for u in cand.units for s, e, a in u.ranges.get(sec, []) if "rename:" not in a and "common" not in a)
        for k, (s, e, u) in enumerate(own):
            if k % nb != batch or u.first(".text") is None:
                continue
            if any(ps < e and pe > s for ps, pe in prov.get((u.name, sec), [])):
                continue
            if not any(x["section"] == sec and s <= x["addr"] < e for x in symbols):
                continue
            out.append((u.name, sec, s, e))
    return out


def strip_ranges(cand, held):
    byu = collections.defaultdict(list)
    for name, sec, s, e in held:
        byu[(name, sec)].append((s, e))
    units = []
    for u in cand.units:
        nu = sc.Unit(u.name, u.attrs, {k: list(v) for k, v in u.ranges.items()})
        for (name, sec), rr in byu.items():
            if name == u.name and sec in nu.ranges:
                nu.ranges[sec] = [(fs, fe, x) for s0, e0, x in nu.ranges[sec] for fs, fe in sc.subtract([(s0, e0)], rr)]
                if not nu.ranges[sec]:
                    del nu.ranges[sec]
        units.append(nu)
    return sc.Splits(list(cand.header), units)


def holdout(paths, secs=None, nb=3, log=print):
    """Hide a third of the registered data ranges at a time (`holdout_runs`), decide them again as unowned runs between their neighbours with the
    same solver, compare with the registered owner.  -> `({section: Counter(right, wrong, undecided)}, [wrong symbols])`."""
    splits, symbols, dol = sc.load_ctx()
    cand, info = sc.render(splits, phase1_inputs(paths), dol, symbols)
    import dataorder as do
    kind = {y.addr: y for y in do.classify_all(sc.dataorder_rows(symbols), dol)}
    stat = collections.defaultdict(collections.Counter)
    wrong = []
    for b in range(nb):
        held = [h for h in holdout_runs(cand, info, symbols, b, nb) if not secs or h[1] in secs]
        stripped = strip_ranges(cand, held)
        an = Analysis(stripped, sc.Ctx(stripped, symbols, dol), kind)
        dec = an.decisions()
        for (u, sec, s0, e0) in held:
            for x in symbols:
                if x["section"] != sec or not (s0 <= x["addr"] < e0) or (x["type"] == "label" and not x["size"]):
                    continue
                got = dec.get((sec, x["addr"]))
                if got is None:
                    stat[sec]["undecided"] += 1
                elif got == u:
                    stat[sec]["right"] += 1
                else:
                    stat[sec]["wrong"] += 1
                    wrong.append((sec, hx(x["addr"]), x["name"], u, got))
        log("batch %d: %d hidden ranges" % (b, len(held)))
    return stat, wrong


def print_holdout(stat, wrong):
    tot = collections.Counter()
    print("%-8s %8s %7s %7s %10s %9s" % ("section", "decided", "right", "wrong", "undecided", "precision"))
    for sec in DATA_SECS:
        if sec not in stat:
            continue
        c = stat[sec]
        tot.update(c)
        d = c["right"] + c["wrong"]
        print("%-8s %8d %7d %7d %10d %8.2f%%" % (sec, d, c["right"], c["wrong"], c["undecided"], 100.0 * c["right"] / max(1, d)))
    d = tot["right"] + tot["wrong"]
    print("%-8s %8d %7d %7d %10d %8.2f%%" % ("all", d, tot["right"], tot["wrong"], tot["undecided"], 100.0 * tot["right"] / max(1, d)))
    by = collections.Counter((w[3], w[4]) for w in wrong)
    for (truth, got), n in by.most_common(8):
        print("wrong: %3d symbols of %s decided to %s" % (n, truth, got))


def vtable_order(cand, symbols, dol, ctx):
    """Per unit: the direction of its adjacent vtable pairs by first code slot (`dataorder`'s `zigzag` rule says descending inside one TU), and how many
    vtables have that owner function inside the unit's own text.  `[(unit, vtables, Counter, owners_in_unit)]`."""
    import dataorder as do
    syms = do.classify_all(sc.dataorder_rows(symbols), dol)
    out = []
    for u in cand.units:
        for s0, e0, _a in u.ranges.get(".data", []):
            vs = [y for y in syms if s0 <= y.addr < e0 and y.kind == do.VTABLE]
            c = collections.Counter()
            for a, b in zip(vs, vs[1:]):
                if a.owner and b.owner:
                    c["up" if b.owner > a.owner else "down" if b.owner < a.owner else "tie"] += 1
            if sum(c.values()):
                out.append((u.name, len(vs), c, sum(1 for y in vs if y.owner and ctx.text_owner(y.owner) is u)))
    return out


# ---- the generator --------------------------------------------------------------------------------------------------------------

def generate(paths, window, lane, settle_on=True, linker=False, log=print, overrides=None, overrides_path=None):
    """`(doc, stats, blocked, unattributed)`: the window's phase 2 proposal.  `overrides` are the orchestrator rows (`resolve_overrides`) that force a
    decision the engine cannot derive; the settle loop still checks them and blocks one that makes an invariant fail."""
    an, _info = load_analysis(paths)
    all_folds, fquestions = find_folds(an, (0, 0xFFFFFFFF))
    if all_folds:                                              # a folded data-only range is its unit's: the data after it is decided on the folded candidate
        an, _info = load_analysis(paths, [{"phase": 2, "lane": lane, "units": [], "attach": [dict(r) for r in all_folds]}])
    fquestions += sinit_owner_questions(an, {r["takes_from"] for r in all_folds}, window)
    if overrides:
        an.set_overrides(resolve_overrides(an, overrides, overrides_path))
    records, unowned = build_records(an, window)
    records += takes_from_records(an, window)
    frows = [r for r in all_folds if window[0] <= sc.to_int(r["text_addr"]) < window[1]]
    xrows, _xun = extab_rows(an.cand, an.ctx, an.dol, an.symbols, window)
    blocked, unattributed, res, accepted, moves = [], [], None, [], []
    if settle_on:                                              # the fold rows are part of the candidate already (`an.props`); the extab rows are not
        blocked, unattributed, res, accepted, moves = settle(an, [dict(r) for r in xrows], records, lane, log)
    live = [x for x in records if not x.get("blocked")]
    for x in live:                                             # the pool inversions of a decided block: the unit holds two TUs' pool order
        if x["inw"] and x.get("inversions"):
            lit, prev = x["inversions"][0]
            fquestions.append({"unit": x["entry"]["unit"], "section": x["sec"],
                               "question": "the first use of the %s pool of %s goes down at %s (after %s): its literals are TU-local, so the block is the "
                                           "reader's, and the unit holds two TUs' pool order - where does the cut lie?" % (x["sec"], x["entry"]["unit"], lit, prev),
                               "reproduce": x["entry"]["reproduce"]})
    attach = [public(x["entry"]) for x in live if x["inw"]] + [dict(r) for r in frows + xrows]
    for rec in blocked:
        if rec["inw"]:
            unowned.append(blocked_to_unowned(rec))
            # the decision link order and the readers make, kept as a `guess` entry (never applied) next to its deferral
            attach.append(dict(public(rec["entry"]), grade="guess", note="not applied: attaching it fails the %s invariant (%s)" % (rec["blocked"][0], rec["blocked"][1][:160])))
    if linker:
        unowned += linker_rows(an.ctx, an.symbols)
    stats = {"totals": totals(records, unowned), "runs": run_table(an, records, window),
             "blocked": [{"unit": x["entry"]["unit"], "section": x["sec"], "start": hx(x["start"]), "end": hx(x["end"]), "symbols": x["entry"]["symbols"],
                          "invariant": x["blocked"][0], "finding": x["blocked"][1]} for x in blocked],
             "accepted": [{"unit": x["entry"]["unit"], "section": x["sec"], "start": hx(x["start"]), "end": hx(x["end"]), "inversion": x["inversions"][0][0]} for x in accepted],
             "overrides": override_outcomes(an, records, unowned),
             "unattributed": [list(u) for u in unattributed], "summary": res.summary() if res is not None else None}
    return proposal_doc(lane, window, attach, unowned, fquestions, moves), stats, blocked, unattributed


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--proposal", action="append", help="proposal file (repeatable; default: phase 1 a..g + reconcile); a file with `units` (a fold, a data-only unit) joins them")
    ap.add_argument("--window", nargs=2, default=["0x80000000", "0x80700000"], help="rows for the units whose .text STARTS in [LO, HI) (default: all)")
    ap.add_argument("--lane", default="p2")
    ap.add_argument("--overrides", metavar="FILE", help="orchestrator overrides (docs/splits-program.md): ranges to attach, defer or exclude by decision; "
                    "the settle loop still checks them")
    ap.add_argument("--out", help="write the window's phase 2 proposal (attach + unowned_data) here")
    ap.add_argument("--stats", help="write the counts (per section, per run, the settle loop) here (json)")
    ap.add_argument("--no-settle", action="store_true", help="skip the invariant loop (keep every decided attachment)")
    ap.add_argument("--linker", action="store_true", help="also list the linker-generated tables nobody owns (`unowned_data`, kind linker-generated)")
    ap.add_argument("--explain", metavar="UNIT_REGEX|ADDR", help="print the segments of the matching units, or the run holding ADDR with every symbol's evidence")
    ap.add_argument("--section", help="comma list of sections to restrict --explain / --holdout to")
    ap.add_argument("--holdout", action="store_true", help="hide registered data ranges, decide them again, compare with the registered owner")
    ap.add_argument("--vtable-order", action="store_true", help="per unit of the candidate: up/down/tie vtable pairs by first slot")
    ap.add_argument("--no-cache", action="store_true", help="rebuild the analysis instead of loading build/tmp/dataattach/ (keyed by the inputs' stat)")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    CACHE[0] = not args.no_cache
    if args.selftest:
        return selftest()
    window = (sc.to_int(args.window[0]), sc.to_int(args.window[1]))
    paths = args.proposal or default_proposals()
    secs = set(args.section.split(",")) if args.section else None
    if args.holdout:
        stat, wrong = holdout(paths, secs)
        print_holdout(stat, wrong)
        return 0
    if args.vtable_order:
        splits, symbols, dol = sc.load_ctx()
        reconciled = [p for p in (os.path.join(PROPOSAL_DIR, "phase2-reconcile.json"),) if os.path.isfile(p)]
        cand, _info = sc.render(splits, [sc.load_proposal(p) for p in paths + reconciled], dol, symbols)
        ctx = sc.Ctx(cand, symbols, dol)
        tot = collections.Counter()
        for name, nv, c, own in vtable_order(cand, symbols, dol, ctx):
            print("%-44s %3d vtables  up %2d down %2d tie %2d  owners in the unit's text %d" % (name, nv, c["up"], c["down"], c["tie"], own))
            tot.update(c)
        print("pairs: up %d, down %d, tie %d" % (tot["up"], tot["down"], tot["tie"]))
        return 0
    ovr_rows = load_overrides(args.overrides) if args.overrides else None
    if args.explain:
        an, _info = load_analysis(paths)
        if ovr_rows:
            an.set_overrides(resolve_overrides(an, ovr_rows, args.overrides))
        return explain(an, args.explain, secs)
    doc, stats, blocked, unattributed = generate(paths, window, args.lane, not args.no_settle, args.linker, overrides=ovr_rows, overrides_path=args.overrides)
    if args.out:
        with open(args.out, "w", encoding="utf-8", newline="\n") as fh:
            json.dump(doc, fh, indent=1)
            fh.write("\n")
    if args.stats:
        with open(args.stats, "w", encoding="utf-8", newline="\n") as fh:
            json.dump(stats, fh, indent=1)
            fh.write("\n")
    applied = [e for e in doc["attach"] if e["grade"] != "guess"]
    print("window %s..%s: %d attachments (%d symbols, strong %d, medium %d), %d guess (blocked by an invariant), %d unowned_data items"
          % (hx(window[0]), hx(window[1]), len(applied), sum(e.get("symbols", 0) for e in applied), sum(1 for e in applied if e["grade"] == "strong"),
             sum(1 for e in applied if e["grade"] == "medium"), len(doc["attach"]) - len(applied), len(doc["unowned_data"])))
    for u in unattributed:
        print("unattributed new FAIL: %s %s: %s" % u)
    return 1 if unattributed else 0


# ---- selftest ---------------------------------------------------------------------------------------------------------------------

def selftest():
    fails = []
    n = [0]

    def check(name, got, want):
        n[0] += 1
        ok = got == want
        print("%s %s%s" % ("ok  " if ok else "FAIL", name, "" if ok else "  got %r want %r" % (got, want)))
        if not ok:
            fails.append(name)

    # dp: three units, A reads s0, C reads s3, the middle two are free -> the free ones lie anywhere between A and C
    opt, adm = dp_admissible([{0}, set(), set(), {2}], [1000] * 4, 3)
    check("dp: free symbols between two anchors of different units are ambiguous over the interval", (opt, adm), (0, [(0, 0), (0, 2), (0, 2), (2, 2)]))
    opt, adm = dp_admissible([{1}, set(), {1}], [1000] * 3, 3)
    check("dp: free symbols between two anchors of one unit are forced to it", adm[1], (1, 1))
    opt, adm = dp_admissible([set(), {0}, set()], [1000] * 3, 2)
    check("dp: a free symbol before the only anchor at the first unit is forced, after it is open", adm, [(0, 0), (0, 0), (0, 1)])
    opt, adm = dp_admissible([{1}, {1}, {0}, {1}], [1000] * 4, 2)
    check("dp: the optimum violates the single out-of-order row, costing one weight", (opt, adm[2]), (1000, (1, 1)))
    opt, adm = dp_admissible([{1}, {0}, {1}], [1000] * 3, 2)
    check("dp: a tie between two rows is not a contradiction, both stay open", (opt, adm[0], adm[1], adm[2]), (1000, (0, 1), (0, 1), (1, 1)))
    opt, adm = dp_admissible([{0}, {1}, {0}], [1000, 1, 1000], 2)
    check("dp: a derived row (weight 1) does not override two strong rows", (opt, adm[1]), (1, (0, 0)))
    opt, adm = dp_admissible([set(), {1}, set(), set()], [1] * 4, 1)
    check("dp: a single-unit chain decides everything", adm, [(0, 0)] * 4)
    # zones
    strong = [{0}, {0}, {1}, {1}, {0}, {1}, {1}]                 # symbol 4 is read by 0 but lies in 1's span
    opt, adm = dp_admissible(strong, [1000] * 7, 2)
    check("zones: a static contradiction between adjacent units unites them", find_zones(strong, adm, lambda i: i == 4), [(0, 1, [4])])
    check("zones: a lone global contradiction is only noted", find_zones(strong, adm, lambda i: False), [])
    strong3 = [{1}, {1}, {1}, {0}, {1}, {0}, {1}, {0}, {1}, {1}, {1}]
    opt, adm = dp_admissible(strong3, [1000] * 11, 2)
    check("zones: three contradictions of one pair between adjacent units unite them even when global",
          [(a, b) for a, b, _i in find_zones(strong3, adm, lambda i: False)], [(0, 1)])
    far = [{0}, {1}, {2}, {3}, {4}, {0}, {4}]
    opt, adm = dp_admissible(far, [1000] * 7, 5)
    check("zones: a static read more than two data-bearing units away is not united", find_zones(far, adm, lambda i: True), [])
    # end to end on a three-unit world: A, B, C read d0, d3+d4, d6 of a seven-symbol `.data` run nobody owns; d1/d2 and d5 are read by nothing
    T, D0 = 0x80100000, 0x80600000

    def rd(off):
        return [sc._lis(3, D0 >> 16), sc._w(14, 3, 3, off), sc.BLR, sc.NOP]

    words = rd(0) + [sc.NOP] * 4 + rd(0x30) + rd(0x40) + rd(0x60) + [sc.NOP] * 4
    units = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:32\n\n"
             + "".join("%s.cpp:\n\t.text       start:0x%X end:0x%X\n\n" % (nm, T + 0x20 * k, T + 0x20 * (k + 1)) for k, nm in enumerate("abc")))
    syms = ["d%d = .data:0x%X; // type:object size:0x10 scope:global" % (i, D0 + 0x10 * i) for i in range(7)]
    cx, _s = sc._mini(T, words, [("fA", 0, 0x20), ("fB", 0x20, 0x20), ("fC", 0x40, 0x20)], units, [(D0, bytes(0x80))], syms)
    an = Analysis(cx.splits, cx, {})
    recs, unowned = build_records(an, (T, T + 0x100))
    check("end to end: each unit gets the block of the symbols its text reads",
          [(x["entry"]["unit"], x["entry"]["range"], x["entry"]["grade"]) for x in recs],
          [("a.cpp", ["0x%08X" % D0, "0x%08X" % (D0 + 0x10)], "strong"), ("b.cpp", ["0x%08X" % (D0 + 0x30), "0x%08X" % (D0 + 0x50)], "strong"),
           ("c.cpp", ["0x%08X" % (D0 + 0x60), "0x%08X" % (D0 + 0x70)], "strong")])
    check("end to end: symbols nothing reads, between two units' anchors, are deferred with both candidates",
          [(u["range"], u["kind"], u["candidates"]) for u in unowned],
          [(["0x%08X" % (D0 + 0x10), "0x%08X" % (D0 + 0x30)], "unread", ["a.cpp", "b.cpp"]), (["0x%08X" % (D0 + 0x50), "0x%08X" % (D0 + 0x60)], "unread", ["b.cpp", "c.cpp"])])
    check("end to end: a unit outside the window is attached but marked outside", [x["inw"] for x in build_records(an, (T, T + 0x40))[0]], [True, True, False])
    check("end to end: the signals name what decided each block", [x["entry"]["signals"] for x in recs], [{"reader": 1}, {"reader": 2}, {"reader": 1}])
    check("end to end: a row carries the unit's text address, the canonical `section` + `range`, and is accepted by the renderer's linter",
          ([x["entry"]["text_addr"] for x in recs], sc.lint_proposal(proposal_doc("t", (T, T + 0x100), [public(x["entry"]) for x in recs], unowned), cx.splits)),
          (["0x%08X" % T, "0x%08X" % (T + 0x20), "0x%08X" % (T + 0x40)], []))
    doc = proposal_doc("t", (T, T + 0x100), [public(x["entry"]) for x in recs], unowned)
    cand2, info2 = sc.render(cx.splits, [doc], cx.dol, cx.symbols)
    check("the rows render: each unit holds its block, nothing else changes", (info2["issues"], {u.name: [(a, b) for a, b, _x in u.ranges.get(".data", [])] for u in cand2.units}),
          ([], {"a.cpp": [(D0, D0 + 0x10)], "b.cpp": [(D0 + 0x30, D0 + 0x50)], "c.cpp": [(D0 + 0x60, D0 + 0x70)]}))
    check("the output is a pure function of the input: the same rows from a second analysis", json.dumps(doc, sort_keys=False),
          json.dumps(proposal_doc("t", (T, T + 0x100), [public(x["entry"]) for x in build_records(Analysis(cx.splits, cx, {}), (T, T + 0x100))[0]],
                                  build_records(Analysis(cx.splits, cx, {}), (T, T + 0x100))[1]), sort_keys=False))
    # a derived row (a table of code pointers) never outweighs a strong one (a reader): the table between two symbols read by B is B's although its pointer is A's
    wtxt = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:32\n\n" +
            "".join("%s.cpp:\n\t.text       start:0x%X end:0x%X\n\n" % (nm, T + 0x20 * k, T + 0x20 * (k + 1)) for k, nm in enumerate("abc")))
    wsyms = ["d%d = .data:0x%X; // type:object size:0x10 scope:global" % (i, D0 + 0x10 * i) for i in range(3)]
    wblob = bytes(0x10) + struct.pack(">4I", T, 0, 0, 0) + bytes(0x10)          # d1 holds a pointer to the first function of a.cpp
    wx, _w = sc._mini(T, [sc.NOP] * 8 + rd(0) + rd(0x20) + [sc.NOP] * 8, [("fA", 0, 0x20), ("fB", 0x20, 0x20), ("fC", 0x40, 0x20)], wtxt, [(D0, wblob)], wsyms)
    wan = Analysis(wx.splits, wx, {})
    check("evidence: a table of code pointers into a.cpp (derived) between two symbols b.cpp reads (strong) is b.cpp's", wan.decisions().get((".data", D0 + 0x10)), "b.cpp")
    # extab/extabindex entries nobody's range covers go to the unit holding their function; the linker's own tables are nobody's
    etxt = ("Sections:\n\t.text       type:code align:32\n\textab       type:rodata align:32\n\textabindex  type:rodata align:32\n\n"
            "a.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 0x20))
    esyms = ["@eti_a = extabindex:0x80400000; // type:object size:0xC scope:local", "@etb_a = extab:0x80000100; // type:object size:0x8 scope:local",
             "_eti_init_info = extabindex:0x8040000C; // type:object size:0x20 scope:global"]
    ex_, _e = sc._mini(T, [sc.NOP] * 8, [("fA", 0, 0x20)], etxt, [(0x80400000, struct.pack(">3I", T, 0x10, 0x80000100) + bytes(0x20))], esyms)
    erows, _eu = extab_rows(ex_.splits, ex_, ex_.dol, ex_.symbols, (T, T + 0x100))
    check("extab_rows: the entry follows its function's unit (one extabindex row, one extab row, strong); the linker's `_eti_init_info` is not given",
          [(r["unit"], r["section"], r["range"], r["grade"], r["text_addr"]) for r in erows],
          [("a.cpp", "extabindex", ["0x80400000", "0x8040000C"], "strong", "0x%08X" % T), ("a.cpp", "extab", ["0x80000100", "0x80000108"], "strong", "0x%08X" % T)])
    check("linker_rows: the linker's own tables are listed as nobody's, with no candidate",
          [(r["section"], r["range"], r["kind"], r["candidates"]) for r in linker_rows(ex_, ex_.symbols)], [("extabindex", ["0x8040000C", "0x8040002C"], "linker-generated", [])])
    # a range abutting the owner's own range across alignment padding extends over the padding
    ptxt = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:32\n\n"
            "a.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n\n" % (T, T + 0x20, D0 - 0x20, D0 - 0x10))
    psy = ["g_pre = .data:0x%X; // type:object size:0x10 scope:global" % (D0 - 0x20), "g0 = .data:0x%X; // type:object size:0x4 scope:global" % D0]
    pxx, _p = sc._mini(T, rd(0) + [sc.NOP] * 4, [("fA", 0, 0x20)], ptxt, [(D0 - 0x20, bytes(0x30))], psy)
    prec, _pu = build_records(Analysis(pxx.splits, pxx, {}), (T, T + 0x100))
    check("a decided range that abuts the owner's range across alignment padding (0x10 unmapped bytes) takes the padding", [x["entry"]["range"] for x in prec],
          [["0x%08X" % (D0 - 0x10), "0x%08X" % (D0 + 4)]])
    # pool order: inside one owner's block of strings only the longest first-use-ordered stretch is decided, the rest is given back; numeric literals are TU-local
    S2 = 0x80790000
    hi, lo = S2 >> 16, S2 & 0xFFFF
    nl = 6
    use = [[0, 3], [1, 4], [2], [], [], [5]]                         # function f reads the literals use[f]: l3 and l4 are first used BEFORE l2's function
    fn_words, fns = [], []
    for f, ks in enumerate(use):
        w = []
        for k in ks:
            w += [sc._lis(3, hi), sc._w(48, 1, 3, lo + 4 * k)]
        w.append(sc.BLR)
        w += [sc.NOP] * (-len(w) % 8)
        fns.append(("f%d" % f, 4 * len(fn_words), 4 * len(w)))
        fn_words += w
    qk = len(fn_words)
    fn_words += [sc._lis(3, hi), sc._w(48, 1, 3, lo + 12), sc.BLR, sc.NOP, sc.NOP, sc.NOP, sc.NOP, sc.NOP]               # q reads l3 too
    fns.append(("fq", 4 * qk, 0x20))

    def pool_world(sec, kind_attr, with_q):
        ptxt = ("Sections:\n\t.text       type:code align:32\n\t%s     type:rodata align:4\n\n" % sec +
                "p.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 4 * qk) + ("\nq.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T + 4 * qk, T + 4 * qk + 0x20) if with_q else ""))
        psyms = ["l%d = %s:0x%X; // type:object size:0x4 scope:local%s" % (k, sec, S2 + 4 * k, kind_attr) for k in range(nl)]
        c, _ps = sc._mini(T, fn_words[:qk + (8 if with_q else 0)], fns[:(7 if with_q else 6)], ptxt, [(S2, struct.pack(">%df" % nl, *[1.5 + k for k in range(nl)]))], psyms)
        return c

    cp = pool_world(".sdata2", " data:float", False)
    rp, up = build_records(Analysis(cp.splits, cp, {}), (T, T + 0x1000))
    check("pool order: a numeric literal is TU-local - its first use going down cuts nothing back, the block stays whole and the inversion is recorded",
          ([(x["entry"]["range"], x["entry"]["symbols"], (x["inversions"][:1] or [("-", "-")])[0][0]) for x in rp], up), ([(["0x%08X" % S2, "0x%08X" % (S2 + 24)], 6, "l3")], []))
    check("pool order: the recorded inversion is in the row's note (an open question of the unit)", "goes down at l3" in rp[0]["entry"].get("note", ""), True)
    cs = pool_world(".sdata", " data:string", True)
    rs, us = build_records(Analysis(cs.splits, cs, {}), (T, T + 0x1000))
    check("pool order: a block of strings that goes down is cut back to its longest stretch; the rest, read by another unit too, is given back to both readers",
          ([(x["entry"]["range"], x["entry"]["symbols"]) for x in rs], [(u["range"], u["kind"], u["candidates"]) for u in us]),
          ([(["0x%08X" % S2, "0x%08X" % (S2 + 12)], 3)], [(["0x%08X" % (S2 + 12), "0x%08X" % (S2 + 24)], "pool-order", ["p.cpp", "q.cpp"])]))
    recp, blp, accp, _unp, _mvp = _settled(cp, {}, T)
    check("settle: a numeric block whose first use goes down fails the pool invariant by construction; it is accepted (recorded), not blocked",
          ([x["entry"]["range"] for x in recp], blp, [(x["inversions"][:1] or [("-", "-")])[0][0] for x in accp]), ([["0x%08X" % S2, "0x%08X" % (S2 + 24)]], [], ["l3"]))
    cs1 = pool_world(".sdata", " data:string", False)
    rs1, us1 = build_records(Analysis(cs1.splits, cs1, {}), (T, T + 0x1000))
    check("pool order: strings nobody else reads stay the unit's (restored), the inversion recorded for its open question",
          ([(x["entry"]["range"], x["entry"]["symbols"]) for x in rs1], us1, [(x["inversions"][:1] or [("-", "-")])[0][0] for x in rs1]), ([(["0x%08X" % S2, "0x%08X" % (S2 + 24)], 6)], [], ["l3"]))
    # a data-only unit between two anchors of the unit before it is that unit's fragment
    gtxt = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:32\n\n"
            "ua.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n\n" % (T, T + 0x20, D0 - 8, D0) +
            "z.cpp:\n\t.data       start:0x%X end:0x%X\n\n" % (D0 + 8, D0 + 16) +
            "ub.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n" % (T + 0x20, T + 0x40, D0 + 32, D0 + 40))
    gsyms = ["%s = .data:0x%X; // type:object size:0x%X" % (nm, D0 + off, sz) for nm, off, sz in
             (("g_pre", -8, 8), ("g0", 0, 4), ("g1", 4, 4), ("gz", 8, 8), ("x0", 16, 4), ("x1", 20, 4), ("x2", 24, 8), ("g_post", 32, 8))]
    gwords = [sc._lis(3, D0 >> 16), sc._w(32, 0, 3, 0), sc._w(32, 0, 3, 4), sc._w(32, 0, 3, 16), sc._w(32, 0, 3, 20), sc.BLR, sc.NOP, sc.NOP] + [sc.BLR] + [sc.NOP] * 7
    gx, _gs = sc._mini(T, gwords, [("fa", 0, 0x20), ("fb", 0x20, 0x20)], gtxt, [(D0 - 8, bytes(48))], gsyms)
    gan = Analysis(gx.splits, gx, {})
    grows, gq = find_folds(gan, (T, T + 0x100))
    check("find_folds: ua holds g1 before z and x0 after it, so z's range is ua's (takes_from z), strong",
          [(r["unit"], r["section"], r["range"], r["kind"], r["takes_from"], r["grade"]) for r in grows],
          [("ua.cpp", ".data", ["0x%08X" % (D0 + 8), "0x%08X" % (D0 + 16)], "fold-data-only", "z.cpp", "strong")])
    gwords2 = gwords[:8] + [sc._lis(3, D0 >> 16), sc._w(32, 0, 3, 16), sc.BLR, sc.NOP] + [sc.NOP] * 4        # ub reads x0 too: the right flank has two readers
    gx2, _gs2 = sc._mini(T, gwords2, [("fa", 0, 0x20), ("fb", 0x20, 0x20)], gtxt, [(D0 - 8, bytes(48))], gsyms)
    check("find_folds: a right flank that another unit reads too is no anchor - no fold", find_folds(Analysis(gx2.splits, gx2, {}), (T, T + 0x100))[0], [])
    gcand, ginfo = sc.render(gx.splits, [{"phase": 2, "units": [], "attach": [dict(r) for r in grows]}], gx.dol, gx.symbols)
    check("find_folds: the rows render - ua holds one range over z's bytes and z is gone",
          (ginfo["issues"], {u.name: [(a, b) for a, b, _x in u.ranges.get(".data", [])] for u in gcand.units}, ginfo["dropped"]),
          ([], {"ua.cpp": [(D0 - 8, D0), (D0 + 8, D0 + 16)], "ub.cpp": [(D0 + 32, D0 + 40)]}, ["z.cpp"]))
    gan2 = Analysis(gcand, sc.Ctx(gcand, gx.symbols, gx.dol), {})
    check("find_folds: on the folded candidate the data after z is decided with ua as a candidate (x0, x1 are ua's: it reads them; x2 stays open)",
          (gan2.decisions().get((".data", D0 + 16)), gan2.decisions().get((".data", D0 + 20)), (".data", D0 + 24) in gan2.decisions()), ("ua.cpp", "ua.cpp", False))
    check("find_folds: without the fold the same symbols are decided to ub (z stands between: ua is no candidate), x0 and x1 are ub's by default",
          (gan.decisions().get((".data", D0 + 16)), gan.decisions().get((".data", D0 + 20))), ("ub.cpp", "ub.cpp"))
    # hold-out: hide a registered range, decide it from its readers between its neighbours, compare with the truth
    htxt = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:32\n\n" +
            "".join("%s.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n\n" % (nm, T + 0x20 * k, T + 0x20 * (k + 1), D0 + 0x10 * k, D0 + 0x10 * (k + 1))
                    for k, nm in enumerate("abc")))
    hsyms = ["d%d = .data:0x%X; // type:object size:0x10 scope:global" % (i, D0 + 0x10 * i) for i in range(3)]
    hx_, _hs = sc._mini(T, rd(0) + [sc.NOP] * 4 + rd(0x10) + [sc.NOP] * 4 + rd(0x20) + [sc.NOP] * 4, [("fA", 0, 0x20), ("fB", 0x20, 0x20), ("fC", 0x40, 0x20)], htxt, [(D0, bytes(0x40))], hsyms)
    held = holdout_runs(hx_.splits, {}, hx_.symbols, 1)
    stripped = strip_ranges(hx_.splits, held)
    han = Analysis(stripped, sc.Ctx(stripped, hx_.symbols, hx_.dol), {})
    check("holdout: every third range is hidden (batch 1 hides b's), and b's symbol is decided back to b between a's and c's",
          (held, han.decisions()), ([("b.cpp", ".data", D0 + 0x10, D0 + 0x20)], {(".data", D0 + 0x10): "b.cpp"}))
    selftest_phase3(check, T)
    print("\n%d checks, %d failed" % (n[0], len(fails)))
    return 1 if fails else 0


def _settled(c, kind_, T):
    """`settle` over a mini candidate `c`: `(records, blocked, accepted, unattributed, moves)`."""
    a_ = Analysis(c.splits, c, kind_)
    a_.splits, a_.symbols, a_.dol, a_.props, a_.base_ctx = c.splits, c.symbols, c.dol, [], c
    recs, _u = build_records(a_, (T, T + 0x1000))
    bl, un, _res, acc, mv = settle(a_, [], recs, "t", log=lambda *x: None)
    return recs, bl, acc, un, mv


def selftest_phase3(check, T):
    """The checks of the phase 3 review fixes (the sinit definers, false pointer edges, the closure depth, labels, reader-vs-pointer, anchor grades and
    seams, overrides, moves, the analysis cache, explain)."""
    def hex8(a):
        return "0x%08X" % a

    hx8 = hex8

    HDRD = "Sections:\n\t.text       type:code align:32\n\t.ctors      type:rodata align:4\n\t.data       type:data align:8\n\t.bss        type:bss align:8\n\t.sdata      type:data align:8\n\n"

    def utext(*blocks):
        return HDRD + "".join("%s:\n%s\n" % (nm, "".join("\t%-11s start:0x%X end:0x%X\n" % (sec, a, b) for sec, a, b in rr)) for nm, rr in blocks)

    def sy(name, sec, addr, size=4, extra="", scope="global"):
        return "%s = %s:0x%X; // type:object size:0x%X scope:%s%s" % (name, sec, addr, size, scope, extra)

    def fn_reading(*addrs, pad=0x10):
        """words of a function that loads each address (`lis` + `lwz`), then `blr`, padded to `pad` bytes."""
        w = []
        for a in addrs:
            w += [sc._lis(3, (a + 0x8000) >> 16), sc._w(32, 0, 3, a & 0xFFFF)]
        w.append(sc.BLR)
        return w + [sc.NOP] * (pad // 4 - len(w))

    # rule 1: a global a unit's own __sinit constructs is that unit's, whoever else reads it
    CT, B0 = 0x80200000, 0x80650000
    sinit = [sc._lis(3, B0 >> 16), sc._w(14, 3, 3, (B0 + 0x10) & 0xFFFF), sc._b(T + 8, T + 0x10), sc.NOP]               # &g1 in r3; b ctor
    ctor = fn_reading(B0, pad=0x20)                                                                                      # the ctor reads g0
    fb = [sc.BLR] + [sc.NOP] * 3
    fc = fn_reading(B0 + 0x10, B0 + 0x20, pad=0x20)                                                                      # c reads g1 and g2
    sx, _x = sc._mini(T, sinit + ctor + fb + fc, [("sinit", 0, 0x10), ("ctor", 0x10, 0x20), ("fb", 0x30, 0x10), ("fc", 0x40, 0x20)],
                      utext(("a.cpp", [(".text", T, T + 0x30), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x30, T + 0x40)]), ("c.cpp", [(".text", T + 0x40, T + 0x60)])),
                      [(CT, struct.pack(">I", T)), (B0, bytes(0x40))], [sy("g%d" % k, ".bss", B0 + 0x10 * k, 0x10) for k in range(4)])
    sxan = Analysis(sx.splits, sx, {})
    check("rule 1: g1 is constructed by a.cpp's own __sinit although only c.cpp reads it -> a.cpp (sinit beats the foreign reader)",
          (sxan.decisions().get((".bss", B0 + 0x10)), sxan.res[0]["label"][1], sxan.res[0]["label"][2]), ("a.cpp", "sinit", "reader"))

    # rule 2: a word at an address that is not 4-aligned and a holder that is mostly non-pointer data name no neighbour
    H0 = 0x80600000
    big = bytearray(0x500)
    for i in range(0, 0x500, 4):
        big[i:i + 4] = struct.pack(">I", 0x01020304 + i)
    big[8:12] = struct.pack(">I", H0 + 0x700)                                       # one value that happens to point at t1: 1 of 320 words
    small = struct.pack(">IIII", H0 + 0x700, 5, 0, 0)                               # 1 of 2 non-zero words: a real pointer table
    odd = bytearray(0x10)
    odd[1:5] = struct.pack(">I", H0 + 0x700)                                        # a `.4byte` inside a byte array: the word at the odd start of the symbol
    hx_, _h = sc._mini(T, [sc.BLR] * 4, [("f", 0, 0x10)], utext(("a.cpp", [(".text", T, T + 0x10)])),
                       [(H0, bytes(big)), (H0 + 0x600, small), (H0 + 0x700, bytes(0x10)), (H0 + 0x800, bytes(odd))],
                       [sy("h_big", ".data", H0, 0x500), sy("h_small", ".data", H0 + 0x600, 0x10), sy("t1", ".data", H0 + 0x700, 0x10), sy("h_odd", ".data", H0 + 0x801, 8)])
    ed = pointer_edges(hx_)
    check("rule 2: only the real pointer table names t1; the 4-byte data table (1 pointer in 320 words) and the unaligned word name nothing",
          sorted(hex(a) for a in ed.get(H0 + 0x700, ())), [hex(H0 + 0x600)])

    # rule 7: the pointer closure runs to a fixpoint (three hops decide s3), code pointers that all enter one unit are as good as a reader
    P0 = 0x80600000
    blob = struct.pack(">IIII", P0 + 0x20, 0, 0, 0)          # s0 reads... s1 -> s2 -> s3, s0 -> s1
    blob = struct.pack(">IIII", P0 + 0x10, 0, 0, 0) + struct.pack(">IIII", P0 + 0x20, 0, 0, 0) + struct.pack(">IIII", P0 + 0x30, 0, 0, 0) + bytes(0x20)
    cx3, _c3 = sc._mini(T, fn_reading(P0) + fn_reading() + fn_reading(P0 + 0x40, pad=0x20),
                        [("fa", 0, 0x10), ("fb", 0x10, 0x10), ("fc", 0x20, 0x20)],
                        utext(("a.cpp", [(".text", T, T + 0x10)]), ("b.cpp", [(".text", T + 0x10, T + 0x20)]), ("c.cpp", [(".text", T + 0x20, T + 0x40)])),
                        [(P0, blob)], [sy("s%d" % k, ".data", P0 + 0x10 * k, 0x10) for k in range(5)])
    can3 = Analysis(cx3.splits, cx3, {})
    check("rule 7: s0 (read by a) points at s1, s1 at s2, s2 at s3 - three hops - and c reads s4: s1, s2 and s3 are decided to a.cpp",
          [can3.decisions().get((".data", P0 + 0x10 * k)) for k in range(5)], ["a.cpp", "a.cpp", "a.cpp", "a.cpp", "c.cpp"])
    check("rule 7: the closure depth of each symbol (s1 round 1, s2 round 2, s3 round 3)", can3.res[0]["ddepth"], [0, 1, 2, 3, 0])

    # code-pointer tables
    tblob = struct.pack(">4I", T, T + 4, T + 8, 0) + struct.pack(">4I", T, T + 4, 5, 0)                      # pure / impure (a number among the pointers)
    tx, _t = sc._mini(T, fn_reading(P0 + 0x100, pad=0x20), [("fa", 0, 0x20)], utext(("a.cpp", [(".text", T, T + 0x20)])), [(P0 + 0x100, bytes(0x10) + tblob)],
                      [sy("t_anchor", ".data", P0 + 0x100, 0x10), sy("t_pure", ".data", P0 + 0x110, 0x10), sy("t_mixed", ".data", P0 + 0x120, 0x10)])
    tan = Analysis(tx.splits, tx, {})
    check("rule 7: code-pointer tables - every non-zero word enters a.cpp: pure; one number among them: not",
          [(tan.res[0]["dlabel"][i], tan.res[0]["dpure"][i]) for i in range(3)], [("", False), ("code-pointers", True), ("code-pointers", False)])
    check("rule 7: derived_strong follows the purity", [tan.derived_strong(tan.res[0], i, 0) for i in range(3)], [False, True, False])

    # rule 8: file-string is a real file-name string; a plain string literal one unit reads is a string-reader
    fx_, _f = sc._mini(T, fn_reading(P0 + 0x200, P0 + 0x210), [("fa", 0, 0x10)], utext(("a.cpp", [(".text", T, T + 0x10)])),
                       [(P0 + 0x200, b"foo.cpp\0" + bytes(8) + b"hello\0\0\0" + bytes(8))],
                       [sy("s_file", ".data", P0 + 0x200, 8, " data:string"), sy("s_plain", ".data", P0 + 0x210, 8, " data:string")])
    check("rule 8: 'foo.cpp' is a file-string, 'hello' a string-reader", list(Analysis(fx_.splits, fx_, {}).res[0]["label"]), ["file-string", "string-reader"])

    # rule 5: reader against pointer table
    R0 = 0x80600000
    h_blob = struct.pack(">IIII", R0 + 0x10, 0, 0, 0)

    def rvp_world(readers, extra_syms=0):
        """run `.data` u0 (0x10), x (0x10), u2 (0x10) at R0..; c owns a pointer table h at R0+0x100 that points at x; `readers` = [(unit idx, symbol idx)]."""
        fa = fn_reading(*[R0 + 0x10 * k for u, k in readers if u == 0], pad=0x20)
        fb = fn_reading(*[R0 + 0x10 * k for u, k in readers if u == 1], pad=0x20)
        return sc._mini(T, fa + fb + [sc.BLR] + [sc.NOP] * 7, [("fa", 0, 0x20), ("fb", 0x20, 0x20), ("fc", 0x40, 0x20)],
                        utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x20, T + 0x40)]), ("c.cpp", [(".text", T + 0x40, T + 0x60), (".data", R0 + 0x100, R0 + 0x110)])),
                        [(R0, bytes(0x100) + h_blob)], [sy("u0", ".data", R0, 0x10), sy("x", ".data", R0 + 0x10, 0x10), sy("u2", ".data", R0 + 0x20, 0x10),
                                                        sy("h", ".data", R0 + 0x100, 0x10)])[0]

    cr = rvp_world([(0, 0), (0, 1), (1, 2)])
    rr, ur = build_records(Analysis(cr.splits, cr, {}), (T, T + 0x100))
    check("rule 5: x is read by a.cpp but the pointer table in c.cpp names it, and it sits between a.cpp's and b.cpp's data: deferred reader-vs-pointer, both listed",
          ([(x["entry"]["unit"], x["entry"]["range"]) for x in rr], [(u["kind"], u["range"], u["candidates"]) for u in ur]),
          ([("a.cpp", [hex8(R0), hex8(R0 + 0x10)]), ("b.cpp", [hex8(R0 + 0x20), hex8(R0 + 0x30)])], [("reader-vs-pointer", [hex8(R0 + 0x10), hex8(R0 + 0x20)], ["a.cpp", "c.cpp"])]))
    ce = rvp_world([(0, 0), (0, 1), (0, 2)])
    re_, ue = build_records(Analysis(ce.splits, ce, {}), (T, T + 0x100))
    check("rule 5: the same disagreement enclosed by a.cpp's own symbols stays attached (taking it out would split the unit's range), graded medium with both signals",
          ([(x["entry"]["unit"], x["entry"]["range"], x["entry"]["grade"], "reader-vs-pointer" in x["entry"].get("note", "")) for x in re_], ue),
          ([("a.cpp", [hex8(R0), hex8(R0 + 0x30)], "medium", True)], []))
    # an array: four contiguous 8-byte elements one table names, the readers split between a and b
    E0 = 0x80600200
    arr_blob = struct.pack(">IIII", E0, E0 + 8, E0 + 16, E0 + 24)
    fa = fn_reading(E0, E0 + 8, pad=0x20)
    fb = fn_reading(E0 + 16, E0 + 24, pad=0x20)
    ca, _ca = sc._mini(T, fa + fb + [sc.BLR] + [sc.NOP] * 7, [("fa", 0, 0x20), ("fb", 0x20, 0x20), ("fc", 0x40, 0x20)],
                       utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x20, T + 0x40)]), ("c.cpp", [(".text", T + 0x40, T + 0x60), (".data", E0 + 0x100, E0 + 0x110)])),
                       [(E0, bytes(0x20) + bytes(0xE0) + arr_blob)], [sy("e%d" % k, ".data", E0 + 8 * k, 8) for k in range(4)] + [sy("tbl", ".data", E0 + 0x100, 0x10)])
    ra_, ua = build_records(Analysis(ca.splits, ca, {}), (T, T + 0x100))
    check("rule 5: an array (4 contiguous equal-size elements one table names) is one object: its readers split it between a and b, so no row takes a half - one deferral lists both",
          ([x["entry"]["unit"] for x in ra_], [(u["kind"], u["range"], u["symbols"], u["candidates"]) for u in ua]),
          ([], [("reader-vs-pointer", [hex8(E0), hex8(E0 + 0x20)], 4, ["a.cpp", "b.cpp"])]))

    # rule 6: grade by anchors - a block of N symbols with fewer than max(3, N/50) read by its unit is medium
    G0, NG = 0x80600000, 60

    def grade_world(read):
        w = fn_reading(*[G0 + 4 * k for k in read], pad=4 * (len(read) * 2 + 1 + 3) // 4 * 4)
        c, _g = sc._mini(T, w, [("fa", 0, len(w) * 4)], utext(("a.cpp", [(".text", T, T + 4 * len(w)), (".ctors", CT, CT + 4)])), [(G0, bytes(4 * NG))],
                         [sy("v%d" % k, ".data", G0 + 4 * k, 4) for k in range(NG)])
        return build_records(Analysis(c.splits, c, {}), (T, T + 0x1000))[0]

    g2, g3 = grade_world([0, NG - 1]), grade_world([0, NG // 2, NG - 1])
    check("rule 6: 60 symbols of which the unit reads 2 are medium (note says so), 3 are strong",
          ([(x["entry"]["symbols"], x["entry"]["grade"]) for x in g2], "only 2 of 60" in g2[0]["entry"].get("note", ""), [(x["entry"]["symbols"], x["entry"]["grade"]) for x in g3]),
          ([(60, "medium")], True, [(60, "strong")]))

    # rule 6: a strong V->S seam inside a decided block cuts the row at the seam: each piece is graded by its own anchors
    V0 = 0x80600000
    vt1 = struct.pack(">4I", 0, 0, T, 0)
    blob = vt1 + b"abc\0" + b"def\0" + vt1
    words = fn_reading(V0, V0 + 4, V0 + 8, pad=0x20)
    cv, _cv = sc._mini(T, words, [("fa", 0, 0x20)], utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)])), [(V0, blob)],
                       [sy("vt_a", ".data", V0, 0x10), sy("str_x", ".data", V0 + 0x10, 4, " data:string"), sy("str_y", ".data", V0 + 0x14, 4, " data:string"),
                        sy("vt_b", ".data", V0 + 0x18, 0x10)])
    import dataorder as do
    kindv = {y.addr: y for y in do.classify_all(sc.dataorder_rows(cv.symbols), cv.dol)}
    sv, _su = build_records(Analysis(cv.splits, cv, kindv), (T, T + 0x100))
    check("rule 6: the V->S seam at the first string cuts a.cpp's block in two rows (before the seam / from it)",
          [(x["entry"]["range"], x["entry"]["symbols"]) for x in sv], [([hex8(V0), hex8(V0 + 0x10)], 1), ([hex8(V0 + 0x10), hex8(V0 + 0x28)], 3)])
    sv0, _s0 = build_records(Analysis(cv.splits, cv, {}), (T, T + 0x100))
    check("rule 6: without the classification (no seams) the block is one row", [(x["entry"]["range"], x["entry"]["symbols"]) for x in sv0], [([hex8(V0), hex8(V0 + 0x28)], 4)])
    cvi, _cvi = sc._mini(T, words, [("fa", 0, 0x20)], utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)])), [(V0, blob), (CT, struct.pack(">I", T))],
                         [sy("vt_a", ".data", V0, 0x10), sy("str_x", ".data", V0 + 0x10, 4, " data:string"), sy("str_y", ".data", V0 + 0x14, 4, " data:string"),
                          sy("vt_b", ".data", V0 + 0x18, 0x10)])
    kvi = {y.addr: y for y in do.classify_all(sc.dataorder_rows(cvi.symbols), cvi.dol)}
    svi, _sui = build_records(Analysis(cvi.splits, cvi, kvi), (T, T + 0x100))
    ua = cvi.by_name["a.cpp"]
    dsyms = sorted(kvi.values(), key=lambda y: y.addr)
    kept_i, inst_i = sc.seams_inside(cvi, ua, V0, V0 + 0x28, dsyms, do.seams(dsyms), kvi, sc.unit_sinit_closure(cvi, ua))
    check("rule 6: a V->S seam the unit's own sinit instantiates is no data-order edge (the checker keeps none) but still cuts the row for grading",
          ([(x["entry"]["range"], x["entry"]["symbols"]) for x in svi], kept_i, len(inst_i)),
          ([([hex8(V0), hex8(V0 + 0x10)], 1), ([hex8(V0 + 0x10), hex8(V0 + 0x28)], 3)], [], 1))

    # settle: a block cut by two seams - the first blocked piece takes the pieces after it along (a hole in the unit's range would fail `order`)
    V1 = 0x80600200
    blob2 = vt1 + b"abc\0" + vt1 + b"def\0" + vt1
    cv2, _cv2 = sc._mini(T, fn_reading(V1, pad=0x20), [("fa", 0, 0x20)], utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)])), [(V1, blob2)],
                         [sy("vt_a", ".data", V1, 0x10), sy("str_x", ".data", V1 + 0x10, 4, " data:string"), sy("vt_b", ".data", V1 + 0x14, 0x10),
                          sy("str_y", ".data", V1 + 0x24, 4, " data:string"), sy("vt_c", ".data", V1 + 0x28, 0x10)])
    kv2 = {y.addr: y for y in do.classify_all(sc.dataorder_rows(cv2.symbols), cv2.dol)}
    recs2, bl2, _acc2, un2, _mv2 = _settled(cv2, kv2, T)
    check("settle: two V->S seams cut a.cpp's block in three rows; the first seam fails data-order, so that piece and the one after it are blocked, the piece before stays",
          ([x["entry"]["range"][0] for x in recs2], sorted(x["entry"]["range"][0] for x in bl2), [x["blocked"][0] for x in bl2], un2),
          ([hex8(V1), hex8(V1 + 0x10), hex8(V1 + 0x24)], [hex8(V1 + 0x10), hex8(V1 + 0x24)], ["data-order", "data-order"], []))


    # overrides (rule 13): attach / defer / exclude a range the engine cannot derive; the settle loop still checks them; the output is reproducible
    O0 = 0x80600000
    fa = fn_reading(O0, pad=0x20)
    fb = fn_reading(O0 + 0x30, O0 + 0x40, pad=0x20)
    fc = fn_reading(O0 + 0x60, pad=0x20)
    co, _co = sc._mini(T, fa + fb + fc, [("fa", 0, 0x20), ("fb", 0x20, 0x20), ("fc", 0x40, 0x20)],
                       utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x20, T + 0x40)]), ("c.cpp", [(".text", T + 0x40, T + 0x60)])),
                       [(O0, bytes(0x100))], [sy("d%d" % k, ".data", O0 + 0x10 * k, 0x10) for k in range(8)])
    ev = [{"tool": "orchestrator", "command": "python tools/splits/dataattach.py --explain 0x%08X" % (O0 + 0x10), "finding": "d1/d2 hold b's tables (reviewer 4)"}]
    rows = [{"unit": "b.cpp", "section": ".data", "start": hex8(O0 + 0x10), "end": hex8(O0 + 0x30), "action": "attach", "grade": "medium", "signal": "orchestrator", "evidence": ev, "note": "reviewer"},
            {"unit": "c.cpp", "section": ".data", "start": hex8(O0 + 0x50), "end": hex8(O0 + 0x60), "action": "defer", "candidates": ["b.cpp", "c.cpp"], "note": "keep open"},
            {"section": ".data", "start": hex8(O0 + 0x70), "end": hex8(O0 + 0x80), "action": "exclude"}]
    oan = Analysis(co.splits, co, {})
    base_rec, base_un = build_records(oan, (T, T + 0x100))
    oan.set_overrides(resolve_overrides(oan, rows))
    orec, oun = build_records(oan, (T, T + 0x100))
    check("overrides: the attach row is a record with signal orchestrator, its grade and the note; the defer row an unowned row; the exclude row neither",
          ([(x["entry"]["unit"], x["entry"]["range"], x["entry"]["grade"], x["entry"]["signal"]) for x in orec if x.get("override") is not None],
           [(u["kind"], u["range"], u["candidates"]) for u in oun if u["kind"] == "orchestrator"],
           [u["range"] for u in oun if u["range"][0] == hex8(O0 + 0x70)]),
          ([("b.cpp", [hex8(O0 + 0x10), hex8(O0 + 0x30)], "medium", "orchestrator")], [("orchestrator", [hex8(O0 + 0x50), hex8(O0 + 0x60)], ["b.cpp", "c.cpp"])], []))
    check("overrides: without them d1/d2 and d5 are deferred, and d7 is c.cpp's (it ends c.cpp's row, the exclude carves it out)",
          (sorted(u["range"][0] for u in base_un if u["range"][0] in (hex8(O0 + 0x10), hex8(O0 + 0x50))), [x["entry"]["range"][1] for x in base_rec if x["entry"]["unit"] == "c.cpp"],
           [x["entry"]["range"][1] for x in orec if x["entry"]["unit"] == "c.cpp"]), ([hex8(O0 + 0x10), hex8(O0 + 0x50)], [hex8(O0 + 0x80)], [hex8(O0 + 0x70)]))
    check("overrides: the decision is the solver's input for the data after it (decisions() counts an attach)", oan.decisions().get((".data", O0 + 0x10)), "b.cpp")

    def refuses(rows_):
        try:
            resolve_overrides(oan, rows_)
        except SystemExit as e:
            return str(e)
        return None

    check("overrides: a row with an unknown unit, an attach without evidence, a guess grade and overlapping rows are refused",
          [refuses([dict(rows[0], unit="nope.cpp")]) is not None, refuses([dict(rows[0], evidence=[])]) is not None, refuses([dict(rows[0], grade="guess")]) is not None,
           refuses([rows[0], dict(rows[0], start=hex8(O0 + 0x20), end=hex8(O0 + 0x28))]) is not None, refuses(rows) is None], [True, True, True, True, True])
    # a blocked override: a jump table read by a.cpp's dispatch, attached to c.cpp, fails the jumptable invariant, so it is blocked and reported
    J = 0x80600100
    disp = [sc._lis(3, J >> 16), sc._w(14, 3, 3, J & 0xFFFF), 0x5480103A, (31 << 26) | (3 << 16) | (23 << 1), 0x7C0903A6, 0x4E800420, sc.NOP, sc.NOP]
    cj, _cj = sc._mini(T, fn_reading(J - 0x20, pad=0x20) + disp + fn_reading(pad=0x20), [("fa", 0, 0x20), ("fb", 0x20, 0x20), ("fc", 0x40, 0x20)],
                       utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x20, T + 0x40)]), ("c.cpp", [(".text", T + 0x40, T + 0x60)])),
                       [(J - 0x20, bytes(0x20) + struct.pack(">4I", T, T, T, T))], [sy("jumptable_%X" % J, ".data", J, 0x10, scope="local"), sy("dz", ".data", J - 0x20, 0x10)])
    jan = Analysis(cj.splits, cj, {})
    jan.splits, jan.symbols, jan.dol, jan.props, jan.base_ctx = cj.splits, cj.symbols, cj.dol, [], cj
    jrows = [{"unit": "c.cpp", "section": ".data", "start": hex8(J), "end": hex8(J + 0x10), "action": "attach", "grade": "strong", "evidence": ev}]
    jan.set_overrides(resolve_overrides(jan, jrows))
    jrec, _ju = build_records(jan, (T, T + 0x100))
    blocked, unattr, _jr, _acc, _mv = settle(jan, [], jrec, "t", log=lambda *a: None)
    check("overrides: attaching a jump table to the wrong unit is BLOCKED by the invariant loop and reported (not applied)",
          ([(b["entry"]["unit"], b["blocked"][0]) for b in blocked], override_outcomes(jan, jrec, [])[0]["outcome"], unattr), ([("c.cpp", "jumptable")], "blocked", []))
    jan0 = Analysis(cj.splits, cj, {})
    jrec0, _ju0 = build_records(jan0, (T, T + 0x100))
    check("overrides: the same table without the override is b.cpp's (the dispatch reads it)", [(x["entry"]["unit"], x["entry"]["signal"]) for x in jrec0], [("a.cpp", "reader"), ("b.cpp", "jumptable")])

    def regen():
        a_ = Analysis(co.splits, co, {})
        a_.set_overrides(resolve_overrides(a_, rows))
        r_, u_ = build_records(a_, (T, T + 0x100))
        return json.dumps(proposal_doc("t", (T, T + 0x100), [public(x["entry"]) for x in r_], u_), sort_keys=False)

    check("overrides: regeneration with the same overrides is byte-identical", regen(), regen())
    # takes_from: one row over bytes another unit owns (a recut)
    rows_tf = [{"unit": "a.cpp", "section": ".data", "start": hex8(O0 + 0x10), "end": hex8(O0 + 0x30), "action": "attach", "grade": "strong", "evidence": ev, "takes_from": "c.cpp"}]
    tan = Analysis(co.splits, co, {})
    tan.set_overrides(resolve_overrides(tan, rows_tf))
    trec, _tu = build_records(tan, (T, T + 0x100))
    tfr = takes_from_records(tan, (T, T + 0x100))
    check("overrides: a takes_from attach is one direct row over the whole range, naming the donor (and the solver's segments skip it)",
          ([(x["entry"]["unit"], x["entry"]["range"], x["entry"]["takes_from"], x["entry"]["signal"]) for x in tfr], [x for x in trec if x.get("override") is not None]),
          ([("a.cpp", [hex8(O0 + 0x10), hex8(O0 + 0x30)], "c.cpp", "orchestrator")], []))

    # the analysis pickles (the --explain cache) and its key follows the inputs
    import pickle
    check("cache: an analysis survives a pickle round trip", pickle.loads(pickle.dumps(oan)).decisions(), oan.decisions())
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        f1 = os.path.join(td, "a.json")
        with open(f1, "w") as fh:
            fh.write("{}")
        k1 = stamp_of([f1])
        with open(f1, "w") as fh:
            fh.write("{ }")
        check("cache: the key changes with a file's size, with the extra proposals, and not otherwise",
              (k1 != stamp_of([f1]), stamp_of([f1], [{"a": 1}]) != stamp_of([f1]), stamp_of([f1]) == stamp_of([f1])), (True, True, True))

    # moves (phase 3): a data-only unit the attachments left out of link order goes right behind the in-order unit before it, by address
    M2 = 0x80700000
    mu = utext(("z.cpp", [(".data", M2 + 0x30, M2 + 0x40)]), ("a.cpp", [(".text", T, T + 0x10), (".data", M2, M2 + 0x10)]), ("b.cpp", [(".text", T + 0x10, T + 0x20), (".data", M2 + 0x10, M2 + 0x20)]),
               ("c.cpp", [(".text", T + 0x20, T + 0x30), (".data", M2 + 0x20, M2 + 0x30)]))
    cm, _cm = sc._mini(T, [sc.BLR] * 12, [("fa", 0, 0x10), ("fb", 0x10, 0x10), ("fc", 0x20, 0x10)], mu, [(M2, bytes(0x40))], [])
    check("moves: z.cpp (data-only, first in the file, last by address) is moved behind c.cpp; the text units stay", [(m["unit"], m["after"]) for m in derive_moves(cm, set())], [("z.cpp", "c.cpp")])

    # fold: a data-only unit whose symbols one unit's own __sinit constructs is a fragment of that unit's TU (the sinit route; the flank route needs readers)
    ZB = 0x80650000
    zs = [sc._lis(3, ZB >> 16), sc._w(14, 3, 3, ZB & 0xFFFF), sc._b(T + 8, T + 0x10), sc.NOP, sc.BLR, sc.NOP, sc.NOP, sc.NOP]
    cz2, _cz2 = sc._mini(T, zs, [("sinit", 0, 0x10), ("ctor", 0x10, 0x10)], utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("z.cpp", [(".bss", ZB, ZB + 0x10)])),
                         [(CT, struct.pack(">I", T)), (ZB, bytes(0x10))], [sy("zsym", ".bss", ZB, 0x10)])
    frows, _fq = find_folds(Analysis(cz2.splits, cz2, {}), (T, T + 0x100))
    check("fold: z.cpp (data-only) holds zsym, which a.cpp's own __sinit constructs: z is a fragment of a.cpp, strong, takes_from z.cpp",
          [(r["unit"], r["section"], r["takes_from"], r["grade"], r.get("signal")) for r in frows], [("a.cpp", ".bss", "z.cpp", "strong", "sinit")])

    # a range one unit owns although another unit's own __sinit constructs its symbols: an open question naming both (the solver does not move owned data)
    OW = 0x80650100
    ow_words = [sc._lis(3, OW >> 16), sc._w(14, 3, 3, OW & 0xFFFF), sc._b(T + 8, T + 0x10), sc.NOP, sc.BLR, sc.NOP, sc.NOP, sc.NOP] + [sc.BLR] + [sc.NOP] * 7
    cow, _cow = sc._mini(T, ow_words, [("sinit", 0, 0x10), ("ctor", 0x10, 0x10), ("fb", 0x20, 0x20)],
                         utext(("a.cpp", [(".text", T, T + 0x20), (".ctors", CT, CT + 4)]), ("b.cpp", [(".text", T + 0x20, T + 0x40), (".bss", OW, OW + 0x10)])),
                         [(CT, struct.pack(">I", T)), (OW, bytes(0x10))], [sy("ow", ".bss", OW, 0x10)])
    oq = sinit_owner_questions(Analysis(cow.splits, cow, {}), set(), (T, T + 0x100))
    check("owner vs sinit: b.cpp's range holds a symbol a.cpp's own __sinit constructs - one open question names both and the interval",
          [(q["unit"], q["section"], q["candidate_interval"], "a.cpp" in q["question"]) for q in oq], [("b.cpp", ".bss", [hx8(OW), hx8(OW + 0x10)], True)])

    # settle: the data-only unit left out of link order is moved by the loop itself (the moves it returns are the doc's `moves`)
    mu2 = utext(("z.cpp", [(".data", M2 + 0x30, M2 + 0x40)]), ("a.cpp", [(".text", T, T + 0x20), (".data", M2, M2 + 0x10), (".ctors", CT, CT + 4)]),
                ("b.cpp", [(".text", T + 0x20, T + 0x40), (".data", M2 + 0x10, M2 + 0x20)]), ("c.cpp", [(".text", T + 0x40, T + 0x60), (".data", M2 + 0x20, M2 + 0x30)]))
    cm2, _cm2 = sc._mini(T, fn_reading(M2 - 0x10, pad=0x20) + fn_reading(pad=0x20) + fn_reading(pad=0x20), [("fa", 0, 0x20), ("fb", 0x20, 0x20), ("fc", 0x40, 0x20)], mu2,
                         [(M2 - 0x10, bytes(0x50))], [sy("extra", ".data", M2 - 0x10, 0x10)])
    recm, blm, _accm, unm, mvm = _settled(cm2, {}, T)
    check("settle: the data-only unit the file order leaves behind is moved by the loop (not left as an unattributed failure); the attachment stays",
          ([(m["unit"], m["after"]) for m in mvm], [x["entry"]["unit"] for x in recm], blm, unm), ([("z.cpp", "c.cpp")], ["a.cpp"], [], []))

    # explain: a data-only unit lists its ranges and who reads each symbol
    import io, contextlib
    zt = utext(("a.cpp", [(".text", T, T + 0x10), (".ctors", CT, CT + 4)]), ("z.cpp", [(".data", O0, O0 + 0x10)]))
    cz, _z = sc._mini(T, fn_reading(O0, pad=0x10), [("fa", 0, 0x10)], zt, [(O0, bytes(0x20))], [sy("gz", ".data", O0, 0x10)])
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        explain(Analysis(cz.splits, cz, {}), r"z\.cpp", None)
    out = buf.getvalue()
    check("explain: a data-only unit prints its range and the readers of each symbol (it printed nothing)", ("data-only unit" in out, "gz" in out, "read by a.cpp" in out), (True, True, True))


if __name__ == "__main__":
    sys.exit(main())
