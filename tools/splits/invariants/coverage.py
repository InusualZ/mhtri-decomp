"""The `coverage` invariant: every map symbol inside one unit range of its section.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import collections

from tools.splits.invariants.context import FAIL, PASS, SECTION_ORDER, hx


def check_coverage(ctx, res):
    """Every map symbol is inside exactly one unit's range of its section; the unowned runs are summarised."""
    runs = collections.defaultdict(list)                 # section -> [[start, end, n_symbols, last_owned_unit]]
    last_owned = {}
    for s in sorted(ctx.symbols, key=lambda x: (x["section"], x["addr"])):
        sec = s["section"]
        if sec not in SECTION_ORDER or sec not in ctx.sec_index or (s["type"] == "label" and not s["size"]):
            continue
        a, size = s["addr"], s["size"]
        u = ctx.owner(sec, a)
        if u is None:
            r = runs[sec]
            if r and r[-1][3] == last_owned.get(sec):
                r[-1][1] = a + size
                r[-1][2] += 1
            else:
                r.append([a, a + size, 1, last_owned.get(sec)])
            continue
        last_owned[sec] = u.name
        if size and sec != ".bss":
            e = ctx.owner(sec, a + size - 1)
            if e is not u:
                res.add(u.name, "coverage", FAIL, a, "%s %s (0x%X B) straddles the end of the unit's %s range"
                        % (s["type"] or "symbol", s["name"], size, sec))
    for u in ctx.units:
        res.add(u.name, "coverage", PASS)
    ctx.coverage_gaps = {sec: {"runs": len(r), "symbols": sum(x[2] for x in r), "bytes": sum(x[1] - x[0] for x in r),
                               "largest": [{"start": hx(x[0]), "end": hx(x[1]), "symbols": x[2]}
                                           for x in sorted(r, key=lambda x: -(x[1] - x[0]))[:3]]}
                         for sec, r in runs.items()}
