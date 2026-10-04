"""The `vtable` invariant: a vtable sits in the unit that holds one of its slots or stores it.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import FAIL, PASS, UNKNOWN


def check_vtable(ctx, res):
    syms = getattr(ctx, "data_order", None)
    if syms is None:
        return
    for y in syms:
        if y.kind != "V":
            continue
        u = ctx.owner(".data", y.addr)
        if u is None:
            continue
        slots = [ctx.dol.word(y.addr + o) for o in range(8, y.size - 3, 4)]
        slots = [w for w in slots if w]
        in_unit = [w for w in slots if ctx.text_owner(w) is u]
        if in_unit:
            res.add(u.name, "vtable", PASS)
            continue
        ds = next((s for s in ctx.data_syms if s["addr"] == y.addr and s["section"] == ".data"), None)
        stores = [x for x in (ctx.readers(ds) if ds else []) if ctx.text_owner(x) is u]
        if stores:
            res.add(u.name, "vtable", PASS)
            continue
        owners = sorted({ctx.text_owner(w).name for w in slots if ctx.text_owner(w)})
        if owners:
            res.add(u.name, "vtable", FAIL, y.addr, "vtable %s has no slot in this unit; its slots are in %s" % (y.name, ", ".join(owners[:3])))
        else:
            res.add(u.name, "vtable", UNKNOWN, y.addr, "vtable %s: no slot or constructor store is owned by any unit" % y.name)
