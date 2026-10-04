"""The `order` invariant: one range per section, no overlap, no link-order cycle, file order.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import bisect
import collections

from tools.splits.invariants.context import FAIL, PASS, UNKNOWN, hx


def check_order(ctx, res):
    """One range per section, no overlap, no cycle in the union of the per-section orders."""
    for u in ctx.units:
        for sec, rr in u.ranges.items():
            plain = sorted((s, e) for s, e, a in rr if "rename:" not in a and "common" not in a)
            if len(plain) > 1:
                gaps = [(pe, s) for (_ps, pe), (s, _e) in zip(plain, plain[1:]) if s != pe]
                if gaps:
                    res.add(u.name, "order", FAIL, plain[1][0], "%s has %d ranges with a gap at %s (dtk takes one)" % (sec, len(plain), hx(gaps[0][0])))
                else:
                    res.add(u.name, "order", UNKNOWN, plain[1][0], "%s has %d abutting ranges (one range in effect)" % (sec, len(plain)))
            for s, e, _a in rr:
                if e <= s:
                    res.add(u.name, "order", FAIL, s, "%s range is empty or reversed (%s..%s)" % (sec, hx(s), hx(e)))
    edges = collections.defaultdict(set)
    where = {}
    for sec, v in ctx.sec_index.items():
        prev = None
        for s, e, u in v:
            if "rename:" in next((a for x, y, a in u.ranges[sec] if x == s), ""):
                continue                     # `.ctors$10` / `.dtors$15`: the linker script orders these, not the unit order
            if prev is not None:
                ps, pe, pu = prev
                if s < pe and pu is not u:
                    res.add(pu.name, "order", FAIL, s, "%s overlaps %s at %s..%s" % (sec, u.name, hx(s), hx(min(e, pe))))
                    res.add(u.name, "order", FAIL, s, "%s overlaps %s at %s..%s" % (sec, pu.name, hx(s), hx(min(e, pe))))
                if pu is not u:
                    edges[pu.name].add(u.name)
                    where[(pu.name, u.name)] = (sec, s)
            prev = (s, e, u) if prev is None or e > prev[1] else prev
    for comp in _sccs(edges):
        if len(comp) < 2:
            continue
        cs = set(comp)
        for name in comp:
            ev = [(where[(a, b)], a, b) for a in comp for b in edges[a] if b in cs and (a, b) in where]
            (sec, addr), a, b = min(ev, key=lambda t: t[0][1])
            res.add(name, "order", FAIL, addr, "link-order cycle of %d units (%s before %s at %s %s)"
                    % (len(comp), a, b, sec, hx(addr)), {"cycle": sorted(comp)[:8]})
    for _sec, name, addr, finding, _before in file_order_offenders(ctx):
        res.add(name, "order", FAIL, addr, finding)
    for u in ctx.units:
        res.add(u.name, "order", PASS)


def longest_nondecreasing(seq):
    """Indices of one longest non-decreasing subsequence of `seq` (patience sorting; the later of two equal-length candidates wins, deterministic)."""
    tails, tail_idx, parent = [], [], [None] * len(seq)
    for i, x in enumerate(seq):
        k = bisect.bisect_right(tails, x)
        if k == len(tails):
            tails.append(x)
            tail_idx.append(i)
        else:
            tails[k], tail_idx[k] = x, i
        parent[i] = tail_idx[k - 1] if k else None
    out = []
    i = tail_idx[-1] if tail_idx else None
    while i is not None:
        out.append(i)
        i = parent[i]
    return out[::-1]


def file_order_offenders(ctx):
    """`[(section, unit name, address, finding, before)]`: the ranges whose unit is out of link order (`before`: the in-order unit whose range precedes it
    by address, `None` at the section start).  The linker places the units of one section in FILE order
    (`dtk` reads `splits.txt` top to bottom; a data-only unit has no text to anchor it, so its file position is the only thing that orders it), so the
    ranges of every section, taken by address, must have non-decreasing file positions.  A longest-non-decreasing-subsequence picks the ranges that
    are in order; each range outside it is out of link order (the fewest ranges that have to move).  A `rename:` range is ordered by the linker script."""
    pos = {u.name: k for k, u in enumerate(ctx.units)}
    out = []
    for sec in sorted(ctx.sec_index):
        seq = []
        for s, e, u in ctx.sec_index[sec]:
            if "rename:" in next((a for x, _y, a in u.ranges[sec] if x == s), "") or "common" in next((a for x, _y, a in u.ranges[sec] if x == s), ""):
                continue
            seq.append((s, e, u))
        keep = set(longest_nondecreasing([pos[u.name] for _s, _e, u in seq]))
        for i, (s, e, u) in enumerate(seq):
            if i in keep:
                continue
            before = next((seq[j] for j in range(i - 1, -1, -1) if j in keep), None)
            after = next((seq[j] for j in range(i + 1, len(seq)) if j in keep), None)
            out.append((sec, u.name, s, "%s range %s..%s is out of link order: by address it lies between %s and %s, but %s is at file position %d (%s at %d, %s at %d)"
                        % (sec, hx(s), hx(e), before[2].name if before else "the section start", after[2].name if after else "the section end", u.name,
                           pos[u.name], before[2].name if before else "-", pos[before[2].name] if before else -1, after[2].name if after else "-",
                           pos[after[2].name] if after else -1), before[2].name if before else None))
    return out


def _sccs(edges):
    """Strongly connected components (iterative Tarjan)."""
    index, low, on, stack, out = {}, {}, set(), [], []
    counter = [0]
    nodes = set(edges) | {b for v in edges.values() for b in v}
    for root in sorted(nodes):
        if root in index:
            continue
        work = [(root, iter(sorted(edges.get(root, ()))))]
        index[root] = low[root] = counter[0]
        counter[0] += 1
        stack.append(root)
        on.add(root)
        while work:
            node, it = work[-1]
            adv = False
            for nxt in it:
                if nxt not in index:
                    index[nxt] = low[nxt] = counter[0]
                    counter[0] += 1
                    stack.append(nxt)
                    on.add(nxt)
                    work.append((nxt, iter(sorted(edges.get(nxt, ())))))
                    adv = True
                    break
                if nxt in on:
                    low[node] = min(low[node], index[nxt])
            if adv:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[node])
            if low[node] == index[node]:
                comp = []
                while True:
                    w = stack.pop()
                    on.discard(w)
                    comp.append(w)
                    if w == node:
                        break
                out.append(comp)
    return out
