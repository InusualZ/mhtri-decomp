"""The `ctors`/`dtors` invariants: one `__sinit` per unit, the TU ends at the sinit closure.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import (CRT_CHAIN, FAIL, PASS, SINIT_SLACK, UNBOUNDED,
                                            UNKNOWN, hx)


def check_ctors(ctx, res):
    for sec, inv in ((".ctors", "ctors"), (".dtors", "dtors")):
        for u in ctx.units:
            words = []
            for s, e, _a in u.ranges.get(sec, []):
                words += [(a, ctx.dol.word(a)) for a in range(s, e - 3, 4)]
            if not words:
                continue
            tr = u.ranges.get(".text", [])
            if sec == ".ctors" and len(words) > 1:
                res.add(u.name, inv, FAIL, words[1][0], "%d .ctors words: one TU has one __sinit, so this is %d or more TUs"
                        % (len(words), len(words)))
            for a, w in words:
                if not w:
                    res.add(u.name, inv, UNKNOWN, a, "zero word")
                    continue
                f = ctx.fn_at(w)
                if f is None:
                    res.add(u.name, inv, FAIL, a, "word %s is not a function" % hx(w))
                    continue
                tu = ctx.text_owner(w)
                if f["name"] in CRT_CHAIN:
                    res.add(u.name, inv, PASS)
                elif tu is not u:
                    res.add(u.name, inv, FAIL, a, "word %s is %s, which is in %s" % (hx(w), f["name"],
                                                                                     tu.name if tu else "no unit"))
                elif sec == ".ctors":
                    end = tr[-1][1] if tr else 0
                    sinit_end = f["addr"] + (f["size"] or 0)
                    if sinit_end > end:
                        res.add(u.name, inv, FAIL, a, "%s runs past the unit's text (text ends %s, it ends %s)"
                                % (f["name"], hx(end), hx(sinit_end)), {"cut_at": sinit_end})
                        continue
                    _fns, limit = ctx.sinit_closure(f["addr"], end)
                    # the own-slot run is followed past the unit end too: a cut placed over a unit's own deferred slots is early
                    limit = ctx.extend_over_own_vtable_slots(limit, tr[0][0] if tr else 0, UNBOUNDED, end)
                    if limit > end + SINIT_SLACK:
                        res.add(u.name, inv, FAIL, a, "%s's closure (sinit + its local callees + the slots of the vtables it stores) ends %s, past the unit text end %s: "
                                "the TU boundary is at %s, not %s" % (f["name"], hx(limit), hx(end), hx(limit), hx(end)), {"cut_at": limit})
                        continue
                    if limit + SINIT_SLACK >= end:
                        res.add(u.name, inv, PASS)
                        continue
                    g = ctx.fn_at(limit)
                    lo = tr[0][0] if tr else 0
                    before = ctx.refs_to_fn(limit, lo, limit) if g is not None and g["addr"] == limit else []
                    if before:
                        site, kind = before[0]
                        res.add(u.name, inv, UNKNOWN, a, "%s's closure (sinit + its local callees) ends %s, before the unit end %s, but %s at %s is %s from %s, "
                                "inside the unit: the boundary at %s is not confirmed" % (f["name"], hx(limit), hx(end), g["name"], hx(limit),
                                                                                         "called" if kind == "call" else "address-taken", hx(site), hx(limit)),
                                {"closure_end": limit})
                    else:
                        res.add(u.name, inv, FAIL, a, "%s's closure (sinit + its local callees) ends %s, unit text ends %s: a TU boundary is at %s"
                                % (f["name"], hx(limit), hx(end), hx(limit)), {"cut_at": limit})
                else:
                    res.add(u.name, inv, PASS)


def unit_sinit_closure(ctx, u):
    """The function starts of the unit's `__sinit`s and everything they call or take the address of after them (the deferred
    constructors this TU instantiates); empty when the unit has no `.ctors` word."""
    out = set()
    tr = u.ranges.get(".text", [])
    end = tr[-1][1] if tr else 0
    for s, e, _a in u.ranges.get(".ctors", []):
        for a in range(s, e - 3, 4):
            w = ctx.dol.word(a)
            f = ctx.fn_at(w) if w else None
            if f is not None and ctx.text_owner(w) is u:
                out |= ctx.sinit_closure(f["addr"], end)[0]
    return out


def ctors_detail(ctx, u, sec=".ctors"):
    """One line per `.ctors`/`.dtors` word of `u`: the word, the function it names, that function's own end, the closure end the
    ctors check derives (sinit + local callees + own vtable slots) and the unit's text end - the numbers behind a ctors finding."""
    out = []
    tr = u.ranges.get(".text", [])
    end = tr[-1][1] if tr else 0
    for s0, e0, _a in u.ranges.get(sec, []):
        for a in range(s0, e0 - 3, 4):
            w = ctx.dol.word(a)
            f = ctx.fn_at(w) if w else None
            if f is None:
                out.append("%s word %s = %s: not a function" % (sec, hx(a), hx(w or 0)))
                continue
            if sec != ".ctors" or f["name"] in CRT_CHAIN or ctx.text_owner(w) is not u:
                out.append("%s word %s = %s (%s, ends %s)%s" % (sec, hx(a), hx(w), f["name"], hx(f["addr"] + (f["size"] or 0)),
                                                                 "" if ctx.text_owner(w) is u else " in %s" % (ctx.text_owner(w).name if ctx.text_owner(w) else "no unit")))
                continue
            _fns, limit = ctx.sinit_closure(f["addr"], end)
            ext = ctx.extend_over_own_vtable_slots(limit, tr[0][0] if tr else 0, UNBOUNDED, end)
            out.append("%s word %s = %s (%s, ends %s): closure ends %s, with own vtable slots %s; unit text ends %s"
                       % (sec, hx(a), hx(w), f["name"], hx(f["addr"] + (f["size"] or 0)), hx(limit), hx(ext), hx(end)))
    return out
