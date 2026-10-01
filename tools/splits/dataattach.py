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


def code_target_names(ctx, s):
    c = collections.Counter()
    for i in range((s["size"] or 0) // 4):
        w = ctx.dol.word(s["addr"] + 4 * i)
        if w and 0x80004000 <= w < 0x80580000 and ctx.fn_at(w) is not None:
            u = ctx.text_owner(w)
            if u is not None:
                c[u.name] += 1
    return c


def pointer_edges(ctx):
    """`{symbol address: set of neighbour addresses}` over every data symbol: a word of one that points into another."""
    edges = collections.defaultdict(set)
    for s in ctx.data_syms:
        if s["section"] not in PTR_SECS or not s["size"]:
            continue
        for i in range(s["size"] // 4):
            w = ctx.dol.word(s["addr"] + 4 * i)
            if not w or w < 0x80500000:
                continue
            j, t = ctx.data_sym_at(w)
            if t is not None and t is not s and t["section"] in DATA_SECS + (".ctors", ".dtors"):
                edges[s["addr"]].add(t["addr"])
                edges[t["addr"]].add(s["addr"])
    return edges


def strong_label(s, names, vtable):
    """The signal a strong row stands on (docs: reader=1, pool-order=3, vtable-store=4, local-static=5, jumptable=6, file-string=7)."""
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
        return "file-string" if one else "readers"
    return "reader" if one else "readers"


def kind_of(kind, s):
    y = kind.get(s["addr"]) if s["section"] == ".data" else None
    return y.kind if y is not None else ""


class Analysis:
    """The evidence and the monotone assignment of every unowned run of a candidate (`cand`, with `ctx` its decoded reference index)."""

    def __init__(self, cand, ctx, kind):
        self.cand, self.ctx, self.kind = cand, ctx, kind
        self.runs = collect_runs(cand, ctx)
        self.edges = pointer_edges(ctx)
        self.res = []                           # per run: dict
        self._strong_pass()
        self._derived_pass()
        for r in self.res:
            self._weighted(r)

    # evidence ----------------------------------------------------------------------------------------------------------------
    def _strong_pass(self):
        ctx = self.ctx
        for run in self.runs:
            n = len(run.syms)
            sets, names, labels, foreign = [], [], [], []
            for s in run.syms:
                nm = reader_names(ctx, s)
                ks = {run.pos[x] for x in nm if x in run.pos}
                label = ""
                if ks:
                    k = kind_of(self.kind, s)
                    label = strong_label(s, nm, k == "V")
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
                             "adm_strong": adm, "opt_strong": opt, "derived": [set() for _ in range(n)], "dlabel": [""] * n})

    def _derived_pass(self):
        ctx = self.ctx
        decided = {}
        for r in self.res:
            for i, s in enumerate(r["run"].syms):
                lo, hi = r["adm_strong"][i]
                if lo is not None and lo == hi:
                    decided[s["addr"]] = {r["run"].chain[lo].name}
        base = {}                               # address -> unit names, from strong rows
        for r in self.res:
            for i, s in enumerate(r["run"].syms):
                if r["names"][i]:
                    base[s["addr"]] = set(r["names"][i])
        owner_cache = {}

        def ev(addr, prev):
            if addr in base:
                return base[addr]
            if addr in decided:
                return decided[addr]
            if addr in prev:
                return prev[addr]
            if addr not in owner_cache:
                i, t = ctx.data_sym_at(addr)
                u = ctx.owner(t["section"], t["addr"]) if t is not None else None
                owner_cache[addr] = {u.name} if (u is not None and u.first(".text") is not None) else set()
            return owner_cache[addr]

        prev = {}
        cands = []
        for r in self.res:
            for i, s in enumerate(r["run"].syms):
                if not r["strong"][i]:
                    ct = code_target_names(ctx, s)
                    nm = {x for x in ct if x in r["run"].pos}
                    if nm:
                        r["derived"][i] = {r["run"].pos[x] for x in nm}
                        r["dlabel"][i] = "code-pointers"
                        prev[s["addr"]] = nm
                    else:
                        cands.append((r, i))
        for _round in range(2):
            new = {}
            for r, i in cands:
                s = r["run"].syms[i]
                acc = set()
                for a in sorted(self.edges.get(s["addr"], ())):
                    acc |= ev(a, prev)
                acc = {x for x in acc if x in r["run"].pos}
                if acc:
                    new[(id(r), i)] = (r, i, acc)
            for (_k, (r, i, acc)) in new.items():
                s = r["run"].syms[i]
                r["derived"][i] = {r["run"].pos[x] for x in acc}
                r["dlabel"][i] = "pointer-graph"
                prev[s["addr"]] = acc
            cands = [(r, i) for r, i in cands if not r["derived"][i]]

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

    # segments ----------------------------------------------------------------------------------------------------------------
    def segments(self, r):
        """Consecutive symbols of a run with the same fate: decided to a unit, in an interleave zone, or ambiguous over an interval."""
        run = r["run"]
        out = []
        for i, s in enumerate(run.syms):
            lo, hi = r["adm"][i]
            if r["zone_of"][i] is not None:
                key = ("zone", r["zone_of"][i])
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
        """`{(section, address): unit name}` of every symbol the analysis decides."""
        out = {}
        for r in self.res:
            run = r["run"]
            for seg in self.segments(r):
                if seg["key"][0] == "unit":
                    for i in seg["idx"]:
                        out[(run.sec, run.syms[i]["addr"])] = run.chain[seg["key"][1]].name
        return out


# ---- pool order, from segments to proposal rows -------------------------------------------------------------------------------

def pool_cutback(ctx, run, idx, unit):
    """A pool is one per TU in first-use order (idea 94): inside one owner's block of `.sdata`/`.sdata2` the first-use function of the literals the
    owner reads must not go down.  Returns `(keep, given_back)`: the symbol indices of the longest in-order stretch (every index when the block is in
    order) and the lists of the indices before and after it."""
    if run.sec not in POOL_SECS or len(idx) < 2:
        return idx, []
    seq = []                                                   # (position in idx, first-use function of the owner's own reads)
    for t, i in enumerate(idx):
        sym = run.syms[i]
        if not sc.is_literal(sym):
            continue
        own = [x for x in ctx.literal_readers(sym) if ctx.text_owner(x) is unit]
        if own:
            seq.append((t, sc._first_use_fn(ctx, min(own))))
    stretches, cur = [], []
    for item in seq:
        if cur and item[1] < cur[-1][1]:
            stretches.append(cur)
            cur = []
        cur.append(item)
    if cur:
        stretches.append(cur)
    if len(stretches) < 2:
        return idx, []
    keep = max(stretches, key=len)                             # the first of equal length
    lo, hi = keep[0][0], keep[-1][0] + 1
    given = [idx[:lo], idx[hi:]]
    return idx[lo:hi], [g for g in given if g]


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


def signal_counts(r, idx, strong_decided):
    c = collections.Counter()
    for i in idx:
        if r["label"][i]:
            c[r["label"][i]] += 1
        elif strong_decided[i]:
            c["forced"] += 1
        else:
            c[r["dlabel"][i] or "forced"] += 1
    return dict(c)


def finding_for(an, r, i, unit_name):
    s = r["run"].syms[i]
    sites = (an.ctx.literal_readers(s) if sc.is_literal(s) else an.ctx.readers(s))
    mine = [x for x in sites if (an.ctx.text_owner(x) is not None and an.ctx.text_owner(x).name == unit_name)]
    if mine:
        return "%s (%s 0x%08X, 0x%X B) is read at 0x%08X in %s" % (s["name"], s["section"], s["addr"], s["size"], min(mine), unit_name)
    ct = code_target_names(an.ctx, s)
    return "%s (%s 0x%08X, 0x%X B): %s" % (s["name"], s["section"], s["addr"], s["size"],
                                          "its branch targets lie in %s" % ", ".join(sorted(ct)) if ct else "no decoded reader")


def explain_cmd(unit, sec):
    return "python tools/splits/dataattach.py --explain %s --section %s" % (stem_of(unit), sec)


def build_records(an, window):
    """The proposal entries of every run as records: `(records, unowned)`.

    A record is `{"entry": attach row, "inw": the owner starts in the window, "sec", "start", "end", "run", "idx"}`; `unowned` holds the window's
    deferrals (`ambiguous` over an interval of units, `interleave` of two units' alternating data, `pool-order` given back, `unread`)."""
    records, unowned = [], []
    for ri, r in enumerate(an.res):
        run = r["run"]
        sec = run.sec
        sd = [(lo is not None and lo == hi) for lo, hi in r["adm_strong"]]
        sd_same = [sd[i] and r["adm_strong"][i][0] == r["adm"][i][0] for i in range(len(run.syms))]
        contra = set(r["contra"])
        pending = []                                           # (segment, candidate names, kind, owner) given back by the pool order, or open
        for seg in an.segments(r):
            kk = seg["key"]
            if kk[0] == "unit":
                k = kk[1]
                unit = run.chain[k]
                keep, given = pool_cutback(an.ctx, run, seg["idx"], unit)
                for g in given:
                    nb = [run.chain[x].name for x in (k - 1, k, k + 1) if 0 <= x < len(run.chain)]
                    pending.append(({"key": ("amb", k, k), "idx": g}, nb, "pool-order", unit.name))
                idx = keep
                nsym = len(idx)
                counts = signal_counts(r, idx, sd_same)
                weak = [i for i in idx if not sd_same[i]]
                touched = [i for i in idx if i in contra]
                grade = "strong" if not weak and not touched else "medium"
                start, end = tight_range(an, r, seg, k, idx)
                anchors = [i for i in idx if r["label"][i]]
                cmd = explain_cmd(unit.name, sec)
                evid = []
                for i in ([anchors[0], anchors[-1]] if len(anchors) > 1 else anchors):
                    evid.append({"tool": "callers", "command": "python tools/units/callers.py 0x%08X" % run.syms[i]["addr"],
                                 "finding": finding_for(an, r, i, unit.name)})
                n_read = len(anchors)
                n_forced = sum(1 for i in idx if not r["label"][i] and sd_same[i])
                n_derived = nsym - n_read - n_forced
                dl = ", ".join(sorted({r["dlabel"][i] for i in idx if r["dlabel"][i]}))
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
                if touched:
                    entry["note"] = ("%d symbol(s) are read by another unit than the one link order puts them in (a global read where it is not "
                                     "defined): %s" % (len(touched), ", ".join("%s read by %s" % (run.syms[i]["name"], "/".join(sorted(r["names"][i])))
                                                                                for i in touched[:4])))
                records.append({"entry": entry, "inw": in_window(unit, window), "sec": sec, "start": start, "end": end, "run": ri, "idx": idx})
            else:
                pending.append((seg, None, None, None))
        for seg, cnames, forced_kind, owner in pending:
            idx = seg["idx"]
            nsym = len(idx)
            kk = seg["key"]
            if forced_kind == "pool-order":
                cv, kindname = None, "pool-order"
                cands = cnames
            elif kk[0] == "zone":
                a, b, _ix = r["zones"][kk[1]]
                cv, kindname = list(range(a, b + 1)), "interleave"
                cands = [run.chain[x].name for x in cv]
            else:
                cv, kindname = list(range(kk[1], kk[2] + 1)), "ambiguous"
                cands = [run.chain[x].name for x in cv]
            if cv is not None and not any(in_window(run.chain[x], window) for x in cv):
                continue
            if cv is None and not any(in_window(u, window) for u in run.chain if u.name in cands):
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
            cmd = explain_cmd(cands[0], sec)
            ud = {"section": sec, "range": [hx(start), hx(end)], "kind": kindname, "symbols": nsym, "bytes": end - start,
                  "candidates": cands if len(cands) <= 8 else cands[:4] + ["... (%d units)" % len(cands)] + cands[-3:],
                  "evidence": [{"tool": "dataattach", "command": cmd, "finding": unowned_finding(an, r, idx, kindname, cands)}],
                  "reproduce": cmd}
            if kindname == "interleave":
                ud["signals"] = signal_counts(r, idx, sd_same)
            elif kindname == "pool-order":
                ud["note"] = ("the pool of %s would go down in first-use order here (a pool is one per TU: two TUs' pools in one block); only its in-order "
                              "stretch is decided, the rest is open between it and its neighbours" % owner)
            else:
                dn = sorted({(run.chain[k].name, r["dlabel"][i]) for i in idx for k in r["derived"][i] if not kk[1] <= k <= kk[2]})
                if dn:
                    ud["note"] = ("derived evidence names %s, outside the interval: the monotone link order puts the symbol before data that a unit "
                                  "inside the interval reads (that unit may be the head of the named one's TU)" % ", ".join("%s (%s)" % t for t in dn[:3]))
            unowned.append(ud)
    return records, unowned


def unowned_finding(an, r, idx, kindname, cands):
    run = r["run"]
    if kindname == "interleave":
        zone = r["zone_of"][idx[0]]
        cn = [i for i in r["contra"] if r["zone_of"][i] == zone]
        sample = ", ".join("%s read by %s" % (run.syms[i]["name"], "/".join(sorted(r["names"][i]))) for i in cn[:3])
        return ("the data of %s alternates (%d symbol(s) are read by a unit other than the one their position implies: %s): one TU, or one reads "
                "the other's data" % (" / ".join(stem_of(c) for c in cands[:4]), len(cn), sample))
    if kindname == "pool-order":
        return "%d pool symbol(s) first used in an earlier function than the literal before them inside one owner's block: not first-use order" % len(idx)
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


def load_analysis(paths, extra=()):
    """`(Analysis, render info)` of the candidate the proposals `paths` (phase 2 rows dropped) and the `extra` proposals (e.g. the folds) render to."""
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


def check_candidate(an, rows, lane):
    """Render the proposals plus the live rows and run every invariant: `(candidate, info, results)`."""
    doc = {"phase": 2, "lane": lane, "attach": rows, "unowned_data": []}
    cand, info = sc.render(an.splits, an.props + [doc], an.dol, an.symbols)
    ctx2 = sc.Ctx(cand, an.symbols, an.dol, an.base_ctx.sda13, an.base_ctx.sda2)
    res = sc.run_checks(ctx2, sc.dataorder_rows(an.symbols))
    return cand, info, res


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
    res = None
    for rnd in range(12):
        live = [public(x["entry"]) for x in records if not x.get("blocked")] + extra_rows
        cand, info, res = check_candidate(an, live, lane)
        if info["issues"]:
            raise SystemExit("the attachments do not render clean: " + "; ".join(info["issues"][:3]))
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
            if hit:
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
    return blocked, unattributed, res


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
            "note": "link order and evidence put these symbols in %s, but attaching them makes the %s invariant fail: the unit holds more than one TU, "
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


def proposal_doc(lane, window, attach, unowned, questions=None):
    doc = {"phase": 2, "lane": lane, "window": [hx(window[0]), hx(window[1])], "units": [],
           "attach": sorted(attach, key=row_key),
           "unowned_data": sorted(unowned, key=lambda e: (sc.SECTION_ORDER.index(e["section"]) if e["section"] in sc.SECTION_ORDER else 99, sc.to_int(e["range"][0])))}
    if questions:
        doc["open_questions"] = questions
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
    for r in an.res:
        run = r["run"]
        if secs and run.sec not in secs:
            continue
        for seg in an.segments(r):
            kk = seg["key"]
            idx = seg["idx"]
            names = ([run.chain[kk[1]].name] if kk[0] == "unit" else [run.chain[x].name for x in range(*((r["zones"][kk[1]][0], r["zones"][kk[1]][1] + 1) if kk[0] == "zone" else (kk[1], kk[2] + 1)))])
            if not any(keep.search(x) for x in names):
                continue
            a, b = run.syms[idx[0]], run.syms[idx[-1]]
            sd = [(lo is not None and lo == hi) for lo, hi in r["adm_strong"]]
            print("%-7s 0x%08X..0x%08X %4d sym %-9s %s  %s" % (run.sec, a["addr"], sym_end(b), len(idx), kk[0],
                                                                names[0] if len(names) == 1 else "%s..%s (%d)" % (names[0], names[-1], len(names)),
                                                                dict(signal_counts(r, idx, [sd[i] and r["adm_strong"][i][0] == r["adm"][i][0] for i in range(len(run.syms))]))))
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

def generate(paths, window, lane, settle_on=True, linker=False, log=print):
    """`(doc, stats, blocked, unattributed)`: the window's phase 2 proposal."""
    an, _info = load_analysis(paths)
    all_folds, fquestions = find_folds(an, (0, 0xFFFFFFFF))
    if all_folds:                                              # a folded data-only range is its unit's: the data after it is decided on the folded candidate
        an, _info = load_analysis(paths, [{"phase": 2, "lane": lane, "units": [], "attach": [dict(r) for r in all_folds]}])
    records, unowned = build_records(an, window)
    frows = [r for r in all_folds if window[0] <= sc.to_int(r["text_addr"]) < window[1]]
    xrows, _xun = extab_rows(an.cand, an.ctx, an.dol, an.symbols, window)
    blocked, unattributed, res = [], [], None
    if settle_on:                                              # the fold rows are part of the candidate already (`an.props`); the extab rows are not
        blocked, unattributed, res = settle(an, [dict(r) for r in xrows], records, lane, log)
    live = [x for x in records if not x.get("blocked")]
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
             "unattributed": [list(u) for u in unattributed], "summary": res.summary() if res is not None else None}
    return proposal_doc(lane, window, attach, unowned, fquestions), stats, blocked, unattributed


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--proposal", action="append", help="proposal file (repeatable; default: phase 1 a..g + reconcile); a file with `units` (a fold, a data-only unit) joins them")
    ap.add_argument("--window", nargs=2, default=["0x80000000", "0x80700000"], help="rows for the units whose .text STARTS in [LO, HI) (default: all)")
    ap.add_argument("--lane", default="p2")
    ap.add_argument("--out", help="write the window's phase 2 proposal (attach + unowned_data) here")
    ap.add_argument("--stats", help="write the counts (per section, per run, the settle loop) here (json)")
    ap.add_argument("--no-settle", action="store_true", help="skip the invariant loop (keep every decided attachment)")
    ap.add_argument("--linker", action="store_true", help="also list the linker-generated tables nobody owns (`unowned_data`, kind linker-generated)")
    ap.add_argument("--explain", metavar="UNIT_REGEX|ADDR", help="print the segments of the matching units, or the run holding ADDR with every symbol's evidence")
    ap.add_argument("--section", help="comma list of sections to restrict --explain / --holdout to")
    ap.add_argument("--holdout", action="store_true", help="hide registered data ranges, decide them again, compare with the registered owner")
    ap.add_argument("--vtable-order", action="store_true", help="per unit of the candidate: up/down/tie vtable pairs by first slot")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
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
    if args.explain:
        an, _info = load_analysis(paths)
        return explain(an, args.explain, secs)
    doc, stats, blocked, unattributed = generate(paths, window, args.lane, not args.no_settle, args.linker)
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
    # pool order: inside one owner's block only the longest first-use-ordered stretch stays decided
    S2 = 0x80300000
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
    ptxt = ("Sections:\n\t.text       type:code align:32\n\t.sdata2     type:rodata align:4\n\n"
            "p.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 4 * len(fn_words)))
    psyms = ["l%d = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (k, S2 + 4 * k) for k in range(nl)]
    cp, _ps = sc._mini(T, fn_words, fns, ptxt, [(S2, struct.pack(">%df" % nl, *[1.5 + k for k in range(nl)]))], psyms)
    anp = Analysis(cp.splits, cp, {})
    rp, up = build_records(anp, (T, T + 0x1000))
    check("pool order: the block that goes down in first-use order is cut back to its longest stretch, the rest is given back",
          ([(x["entry"]["range"], x["entry"]["symbols"]) for x in rp], [(u["range"], u["kind"]) for u in up]),
          ([(["0x%08X" % S2, "0x%08X" % (S2 + 12)], 3)], [(["0x%08X" % (S2 + 12), "0x%08X" % (S2 + 24)], "pool-order")]))
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
    print("\n%d checks, %d failed" % (n[0], len(fails)))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
