"""The `bss` invariant: a local `.bss`/`.sbss` object is read by the unit that holds it.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import bisect

from tools.splits.invariants.context import FAIL, PASS, UNKNOWN


def check_bss(ctx, res):
    """A local `.bss`/`.sbss` object is read by the unit that holds it; a unit whose range no decoded code touches is UNKNOWN."""
    for u in ctx.units:
        own = foreign_global = 0
        for sec in (".bss", ".sbss", ".sbss2"):
            for s0, e0, _a in u.ranges.get(sec, []):
                i = bisect.bisect_left(ctx._ds_addr, s0)
                while i < len(ctx.data_syms) and ctx._ds_addr[i] < e0:
                    sym = ctx.data_syms[i]
                    i += 1
                    if sym["section"] != sec:
                        continue
                    ous = {ctx.text_owner(x) for x in ctx.readers(sym) if ctx.text_owner(x)}
                    if not ous:
                        continue
                    if u in ous:
                        own += 1
                    elif sym["scope"] == "local":
                        res.add(u.name, "bss", FAIL, sym["addr"], "local %s is read only by %s" % (sym["name"], sorted(o.name for o in ous)[0]))
                    else:
                        foreign_global += 1
        if u.ranges.get(".bss") or u.ranges.get(".sbss") or u.ranges.get(".sbss2"):
            res.add(u.name, "bss", PASS if own else UNKNOWN, None,
                    "" if own else "no object of the range is read by this unit's decoded code (%d read only elsewhere)" % foreign_global)
