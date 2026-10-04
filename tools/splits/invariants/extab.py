"""The `extab` invariant: an `extabindex` entry, its function and its `extab` record in one unit.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import FAIL, PASS, hx


def check_extab(ctx, res):
    ext = ctx.section_extent("extabindex")
    if not ext:
        return
    for a in range(ext[0], ext[1] - 11, 12):
        fn = ctx.dol.word(a)
        size = ctx.dol.word(a + 4)
        ex = ctx.dol.word(a + 8)
        if fn is None or size is None or ex is None:
            continue
        ue = ctx.owner("extabindex", a)
        uf = ctx.text_owner(fn)
        ux = ctx.owner("extab", ex)
        if ue is None:
            continue
        if uf is None or ux is None or uf is not ue or ux is not ue or (uf and fn + size > uf.ranges.get(".text", [(0, 0, "")])[-1][1] + 0 and ctx.text_owner(fn + size - 1) is not uf):
            why = []
            if uf is not ue:
                why.append("function %s is in %s" % (hx(fn), uf.name if uf else "no unit"))
            if ux is not ue:
                why.append("extab %s is in %s" % (hx(ex), ux.name if ux else "no unit"))
            if not why:
                why.append("function %s..%s runs past the unit's text" % (hx(fn), hx(fn + size)))
            res.add(ue.name, "extab", FAIL, a, "entry %s: %s" % (hx(a), "; ".join(why)))
            for other in (uf, ux):
                if other is not None and other is not ue:
                    res.add(other.name, "extab", FAIL, a, "entry %s in %s points at this unit's %s"
                            % (hx(a), ue.name, "text" if other is uf else "extab"))
        else:
            res.add(ue.name, "extab", PASS)
    for u in ctx.units:
        if u.ranges.get("extabindex") or u.ranges.get("extab"):
            res.add(u.name, "extab", PASS)
