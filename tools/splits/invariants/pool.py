"""The `pool` invariant: one literal pool per TU, read only by its unit, in first-use order.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import bisect
import collections

from tools.splits.invariants.context import FAIL, PASS, UNKNOWN, hx, is_literal
import tools.splits.seams.evidence as _ev


def _witness(sym):
    """Whether a pool literal's bytes prove its identity (`evidence.is_value_witness`: a typed float/double)."""
    return _ev.is_value_witness(sym["section"], sym["size"], sym["kind"], sym["type"])


def pool_literals(ctx, u):
    """(symbol, is_dedupe_witness) for the literals of a unit's `.sdata2` / `.sdata` ranges."""
    out = []
    for sec in (".sdata2", ".sdata"):
        for s, e, _a in u.ranges.get(sec, []):
            i = bisect.bisect_left(ctx._ds_addr, s)
            while i < len(ctx.data_syms) and ctx._ds_addr[i] < e:
                sym = ctx.data_syms[i]
                i += 1
                if sym["section"] != sec:
                    continue
                if sec == ".sdata2" and sym["size"] in (4, 8) and sym["type"] == "object":
                    out.append((sym, _witness(sym)))
                elif sec == ".sdata" and sym["kind"] == "string":
                    out.append((sym, False))
    return out


def _first_use_fn(ctx, site):
    """The start of the function holding `site` (the site itself when the map has no function there)."""
    f = ctx.fn_at(site)
    return f["addr"] if f is not None else site


def pool_first_use(ctx, sym, u, own=None, held=None):
    """`(order key, site)` of the first use of a pool literal by unit `u`, or `None` when `u` does not use it.

    A string a pointer initialiser names (a `.data`/`.rodata`/`.sdata` word of `u`'s own ranges pointing at it: the initialiser of a file-scope
    table) is emitted BEFORE the strings the functions use, so its first use sorts first (`(0, 0)`: initialisers carry no order among
    themselves, like the literals of one function); any other use is ordered by the function that makes it (`(1, function start)`).
    `held(word address)` says whether a table word is `u`'s when the table is not in `u`'s ranges yet (a phase 2 decision)."""
    if sym["section"] == ".sdata":
        mine = held if held is not None else (lambda a: ctx.owner(ctx.section_of(a), a) is u)
        inits = [a for a in ctx.string_init_words().get(sym["addr"], ()) if mine(a)]
        if inits:
            return (0, 0), min(inits)
    if own is None:
        own = [x for x in ctx.literal_readers(sym) if ctx.text_owner(x) is u]
    if not own:
        return None
    first = min(own)
    return (1, _first_use_fn(ctx, first)), first


def check_pool(ctx, res):
    """Idea 94.  Two halves: what a unit's own pool claim holds, and what its text reads wherever the pool lives."""
    ctx.pool_edges = collections.defaultdict(set)              # (unit, other) -> literal addresses
    per_unit = collections.defaultdict(lambda: collections.defaultdict(list))   # unit -> (size, value) -> [literal sym]
    reads = collections.defaultdict(int)
    for i, sym in enumerate(ctx.data_syms):
        if not is_literal(sym):
            continue
        sites = ctx.literal_readers(sym)
        units = collections.OrderedDict()
        for x in sites:
            u = ctx.text_owner(x)
            if u is not None:
                units.setdefault(u.name, u)
        for n in units:
            reads[n] += 1
        if len(units) >= 2:
            names = list(units)
            for n in names:
                others = [o for o in names if o != n]
                for o in others:
                    ctx.pool_edges[(n, o)].add(sym["addr"])
                res.add(n, "pool", FAIL, sym["addr"], "literal %s is also read by %s (one pool per TU: the units are one TU, or one read is a false decode)"
                        % (sym["name"], ", ".join(others[:3])))
        if _witness(sym):
            v = ctx.dol.read(sym["addr"], sym["size"])
            for n in units:
                per_unit[n][(sym["size"], v)].append(sym)
    for n, vals in per_unit.items():
        for (size, v), lits in vals.items():
            if len(lits) > 1 and v is not None:
                res.add(n, "pool", FAIL, lits[1]["addr"], "reads value 0x%s at %s and at %s (one entry per value per TU: two TUs)"
                        % (v.hex(), hx(lits[0]["addr"]), hx(lits[1]["addr"])))
    for u in ctx.units:
        lits = pool_literals(ctx, u)
        if lits:
            own_first = []
            seen = {}
            for sym, witness in lits:
                own = [x for x in ctx.literal_readers(sym) if ctx.text_owner(x) is u]
                use = pool_first_use(ctx, sym, u, own)
                if witness:
                    v = ctx.dol.read(sym["addr"], sym["size"])
                    key = (sym["size"], v)
                    if v is not None and key in seen:
                        res.add(u.name, "pool", FAIL, sym["addr"], "claimed pool holds value 0x%s at %s and again at %s (one entry per value per TU)"
                                % (v.hex(), hx(seen[key]), hx(sym["addr"])))
                    elif v is not None:
                        seen[key] = sym["addr"]
                if use is not None:
                    own_first.append((sym["addr"], use[1], sym["name"], use[0], sym["section"]))
            own_first.sort()
            # order is judged per FUNCTION: the scheduler reorders two loads of one function, so literals first used
            # in the same function carry no order; a literal first used in an EARLIER function than the one before it does.
            # Per SECTION: the `.sdata` strings and the `.sdata2` literals are two pools (the strings sit at lower addresses, so one
            # address-sorted list would call every late string followed by an early float an inversion); the lowest-addressed inversion is reported
            bad = None
            for psec in sorted({t[4] for t in own_first}):
                run = [t for t in own_first if t[4] == psec]
                hit = next(((a, f, n, pf) for (_pa, pf, _pn, pfn, _ps), (a, f, n, fn, _qs) in zip(run, run[1:]) if fn < pfn), None)
                if hit is not None and (bad is None or hit[0] < bad[0]):
                    bad = hit
            if bad:
                res.add(u.name, "pool", FAIL, bad[0], "claimed pool: first use of %s (%s) is in an earlier function than the literal before it (%s): not text order"
                        % (bad[2], hx(bad[1]), hx(bad[3])))
            foreign_only = [sym for sym, _w in lits if ctx.literal_readers(sym) and not any(ctx.text_owner(x) is u for x in ctx.literal_readers(sym))]
            if foreign_only and not any(ctx.text_owner(x) is u for sym, _w in lits for x in ctx.literal_readers(sym)):
                res.add(u.name, "pool", FAIL, foreign_only[0]["addr"], "claimed pool %s is read by no code of this unit (readers: %s)"
                        % (foreign_only[0]["name"], sorted({ctx.text_owner(x).name for x in ctx.literal_readers(foreign_only[0]) if ctx.text_owner(x)})[0]))
        if reads.get(u.name) or lits:
            res.add(u.name, "pool", PASS if reads.get(u.name) or any(ctx.literal_readers(sym) for sym, _w in lits) else UNKNOWN,
                    None, "no literal is read by decoded code")


def pool_intervals(ctx, keep=None):
    """`[line]`: for each value held at two or more pool addresses and read from one unit, the pair of consecutive copies, the last
    read of the first and the first read of the second, and the function starts between them (a TU starts in `(fn(last), fn(first)]`).
    `keep(unit_name)` filters by reader unit."""
    groups = collections.defaultdict(list)
    for sym in ctx.data_syms:
        if _witness(sym):
            sites = sorted(ctx.literal_readers(sym))
            if sites:
                groups[(sym["size"], ctx.dol.read(sym["addr"], sym["size"]))].append((sym, sites))
    out = []
    for (size, v), lits in sorted(groups.items(), key=lambda kv: kv[1][0][0]["addr"]):
        lits.sort(key=lambda t: t[0]["addr"])
        for (a, sa), (b, sb) in zip(lits, lits[1:]):
            owners = {ctx.text_owner(x).name for x in sa + sb if ctx.text_owner(x)} or {"unowned"}
            if keep is not None and not any(keep(n) for n in owners):
                continue
            ua = {ctx.text_owner(x) for x in sa if ctx.text_owner(x)}
            ub = {ctx.text_owner(x) for x in sb if ctx.text_owner(x)}
            split_note = ""
            if ua and ub and not (ua & ub):
                split_note = " [already two units: %s | %s]" % (sorted(x.name for x in ua)[0], sorted(x.name for x in ub)[0])
            last, first = sa[-1], sb[0]
            fl, ff = _first_use_fn(ctx, last), _first_use_fn(ctx, first)
            n = bisect.bisect_right(ctx._fn_starts, ff) - bisect.bisect_right(ctx._fn_starts, fl)
            out.append("pooldup value 0x%s: %s %s (last read %s) and %s %s (first read %s)%s" % (
                (v or b"").hex(), a["name"], hx(a["addr"]), hx(last), b["name"], hx(b["addr"]), hx(first),
                (("; a TU starts in (%s, %s]: %d function starts" % (hx(fl), hx(ff), n)) if fl < ff else "; the reads interleave (no interval)") + split_note))
    return out


def pool_groups(ctx):
    """Units chained by a literal two of them read: the `docs/pool-seams.md` groups, recomputed from the decode
    (`evidence.components`, the union-find `poolseams` groups the census with)."""
    out = []
    for members in _ev.components(getattr(ctx, "pool_edges", {})):
        us = sorted(members, key=lambda n: ctx.by_name[n].lo() if n in ctx.by_name else 0)
        out.append({"units": us, "lo": hx(ctx.by_name[us[0]].lo()) if us[0] in ctx.by_name else "-",
                    "literals": sum(len(v) for k, v in ctx.pool_edges.items() if k[0] in members and k[1] in members)})
    out.sort(key=lambda g: -len(g["units"]))
    return out
