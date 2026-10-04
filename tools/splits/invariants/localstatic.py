"""The `local-static` invariant: a `scope:local` data object is read by one unit only.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import collections

from tools.splits.invariants.context import FAIL, PASS, hx, is_literal


def check_local_static(ctx, res):
    """A `scope:local` data object is private to one TU: its decoded readers must lie in one unit.  A reader in two units is a
    FAIL for both (the units are one TU, or one read is a false decode - stated in the finding, like the pool check's)."""
    for sym in ctx.data_syms:
        if sym["scope"] != "local" or sym["type"] != "object" or is_literal(sym) or sym["section"] == ".sdata2":
            continue
        if sym["name"].startswith("jumptable_"):
            continue                                       # a switch table: the `jumptable` invariant owns it (its base register is reused across units)
        units = collections.OrderedDict()
        for x in ctx.readers(sym):
            u = ctx.text_owner(x)
            if u is not None:
                units.setdefault(u.name, (u, x))
        if len(units) < 2:
            for n in units:
                res.add(n, "local-static", PASS)
            continue
        names = list(units)
        for n in names:
            others = [o for o in names if o != n]
            res.add(n, "local-static", FAIL, sym["addr"],
                    "local %s (%s %s) is read at %s by this unit and at %s by %s (a static is one TU's: the units are one TU, or one read is a false decode)"
                    % (sym["name"], sym["section"], hx(sym["addr"]), hx(units[n][1]), hx(units[others[0]][1]), ", ".join(others[:3])),
                    {"symbol": sym["name"], "units": names})
