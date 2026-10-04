"""The `data-order` invariant: no strong V->S or zigzag seam strictly inside a unit's `.data`.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import FAIL, PASS, hx
from tools.splits.invariants.ctors import unit_sinit_closure
import tools.splits.seams.evidence as _ev


def _vtable_before(syms, seam):
    """The vtable symbol address of a `V->S` seam's leading group (its `before` name), or None."""
    for y in syms:
        if y.name == seam.get("before"):
            return y.addr
    return None


def _vtable_stored_in(ctx, vt, fns):
    """Whether a function in `fns` references the `.data` vtable `vt` (a `dataorder.Sym`): its constructor is one of them."""
    if vt is None:
        return False
    i, sym = ctx.data_sym_at(vt.addr)
    if sym is None or sym["addr"] != vt.addr:
        return False
    return any((ctx.fn_at(x) or {}).get("addr") in fns for x in ctx.refs.get(i, []))


def zigzag_interleaved(ctx, syms, by_addr, seam):
    """Whether the two vtables of a zigzag pair have member functions that alternate in the text (each class's own slots - the ones the
    other table does not share - span past the other's).  A TU is one contiguous text range, so no cut separates two classes whose
    members interleave: the pair is one TU, and the `owner goes up` reading of the first slots is not an edge."""
    a = by_addr.get(_vtable_before(syms, seam))
    b = by_addr.get(seam["addr"])
    if a is None or b is None:
        return False
    fs = set(ctx._fn_starts)

    def slots(y):
        return {w for w in (ctx.dol.word(y.addr + o) for o in range(8, y.size - 3, 4)) if w in fs}

    sa, sb = slots(a), slots(b)
    own_a, own_b = sa - sb, sb - sa
    return bool(own_a and own_b and min(own_a) < max(own_b) and min(own_b) < max(own_a))


def seams_inside(ctx, u, s, e, syms, found, by_addr, closure=None):
    """`(seams, instantiated)`: the strong `.data` seams that cut the range `[s, e)` of unit `u` - the rule `check_data_order` applies, one implementation.

    A V->S row says a boundary lies in `[addr, latest)`: a range that ends inside that gap (`latest >= e`) can have it at its own end, so only one that
    also holds the later vtable group crosses the seam; a zigzag cuts at `addr`.  Two classes whose member functions alternate in the text are one TU
    (a zigzag between them is not an edge), and an instantiated vtable - a deferred constructor of this TU stores it (`closure`, the unit's `__sinit`
    closure) - is no TU edge: the later vtable of a V->S seam, the second of a zigzag pair, or the vtable before either."""
    inside = [x for x in found if x["kind"] in _ev.STRONG_KINDS and s < x["addr"] < e and (x["kind"] != "V->S" or x["latest"] < e)]
    inside = [x for x in inside if not (x["kind"] == "zigzag" and zigzag_interleaved(ctx, syms, by_addr, x))]
    kept, inst = [], []
    for x in inside:
        if closure and any(_vtable_stored_in(ctx, by_addr.get(a_), closure) for a_ in (x.get("latest") or x["addr"], _vtable_before(syms, x))):
            inst.append({"unit": u.name, "addr": x["addr"], "vtable": x["before"]})
        else:
            kept.append(x)
    return kept, inst


def check_data_order(ctx, res, rows):
    reader = ctx.dol
    syms = _ev.classify_all(rows, reader)
    ctx.data_order = syms
    found = _ev.seams(syms)
    by_addr = {y.addr: y for y in syms}
    ctx.seams_instantiated = []
    for u in ctx.units:
        for s, e, _a in u.ranges.get(".data", []):
            inside, inst = seams_inside(ctx, u, s, e, syms, found, by_addr, unit_sinit_closure(ctx, u))
            ctx.seams_instantiated += inst
            has_v = any(s <= y.addr < e and y.kind == _ev.VTABLE for y in syms)
            if inside:
                x = inside[0]
                res.add(u.name, "data-order", FAIL, x["addr"], "strong %s seam at %s inside the unit's .data (a boundary in [%s, %s)): at least %d TUs"
                        % (x["kind"], hx(x["addr"]), hx(x["addr"]), hx(x.get("latest", x["addr"])), len(inside) + 1))
            elif has_v:
                res.add(u.name, "data-order", PASS)
