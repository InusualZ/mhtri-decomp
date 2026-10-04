"""The `text-cut` invariant: a unit's `.text` starts and ends on a function symbol.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import CODE_SECTIONS, FAIL, PASS, UNKNOWN


def check_text_cut(ctx, res):
    for u in ctx.units:
        for sec in CODE_SECTIONS:
            for s, e, _a in u.ranges.get(sec, []):
                f = ctx.fn_at(s)
                if f is None or f["addr"] != s:
                    if f is None and not any(x["addr"] == s for x in ctx.symbols if False):
                        pass
                    res.add(u.name, "text-cut", FAIL if f is not None else UNKNOWN, s,
                            "%s starts %s" % (sec, ("inside %s (+0x%X)" % (f["name"], s - f["addr"])) if f
                                              else "where no function symbol is"))
                    continue
                last = ctx.fn_at(e - 1)
                if last is not None and last["addr"] + (last["size"] or 0) > e:
                    res.add(u.name, "text-cut", FAIL, e, "%s ends inside %s (+0x%X)" % (sec, last["name"], e - last["addr"]))
                else:
                    res.add(u.name, "text-cut", PASS)
