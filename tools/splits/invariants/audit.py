"""The baseline audit: every invariant over a context, the boundary checks, the ranking and the report.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import collections
import json
import os
import re

from tools.splits.invariants import (bss, coverage, ctors, data_order, extab, jumptable,
                                     localstatic, order, pool, textcut, vtable)
from tools.splits.invariants.context import (FAIL, INVARIANTS, NA, PASS, RANK, UNKNOWN, WEIGHT,
                                             Ctx, Results, dataorder_rows, hx, is_literal)
# the package's API for the CLI (`splitcheck.py` imports this module only)
from tools.splits.invariants.context import load_ctx, main_root, tree_root  # noqa: F401
from tools.splits.invariants.ctors import ctors_detail  # noqa: F401
from tools.splits.invariants.pool import pool_groups, pool_intervals  # noqa: F401


def run_checks(ctx, rows_for_dataorder=None, only=None):
    res = Results()
    want = set(only or INVARIANTS)
    if "order" in want:
        order.check_order(ctx, res)
    if "coverage" in want:
        coverage.check_coverage(ctx, res)
    if "text-cut" in want:
        textcut.check_text_cut(ctx, res)
    if "extab" in want:
        extab.check_extab(ctx, res)
    if want & {"ctors", "dtors"}:
        ctors.check_ctors(ctx, res)
    pool.check_pool(ctx, res)             # always: the boundary checks read its pool edges
    if want & {"data-order", "vtable"} and rows_for_dataorder is not None:
        data_order.check_data_order(ctx, res, rows_for_dataorder)
        vtable.check_vtable(ctx, res)
    if "jumptable" in want:
        jumptable.check_jumptable(ctx, res)
    if "bss" in want:
        bss.check_bss(ctx, res)
    if "local-static" in want:
        localstatic.check_local_static(ctx, res)
    if only:
        for name in list(res.units):
            for inv in list(res.units[name]):
                if inv not in want:
                    del res.units[name][inv]
    return res


def boundaries(ctx, res):
    """One record per adjacent pair of text units: the function start and the pool literals read across it."""
    tu = sorted((u for u in ctx.units if u.ranges.get(".text")), key=lambda u: u.first(".text"))
    out = []
    for a, b in zip(tu, tu[1:]):
        addr = b.first(".text")
        rec = {"addr": addr, "left": a.name, "right": b.name, "checks": {}}
        f = ctx.fn_at(addr)
        rec["checks"]["fn-start"] = PASS if f and f["addr"] == addr else FAIL
        shared = sorted(ctx.pool_edges.get((a.name, b.name), set()) | ctx.pool_edges.get((b.name, a.name), set())) \
            if getattr(ctx, "pool_edges", None) else []
        rec["checks"]["pool-shared"] = FAIL if shared else PASS
        if shared:
            rec["pool_shared"] = [hx(x) for x in shared[:6]]
        out.append(rec)
    return out


def top_defects(ctx, res, limit=10):
    items = []
    for name, recs in res.units.items():
        for inv, r in recs.items():
            if r["status"] != FAIL:
                continue
            u = ctx.by_name.get(name)
            size = u.size(".text") if u else 0
            items.append({"unit": name, "invariant": inv, "addr": r["addr"], "finding": r["finding"],
                          "fails": r["n_fail"], "score": WEIGHT.get(inv, 10) * 1000 + min(r["n_fail"], 99) * 10 + size // 0x1000})
    items.sort(key=lambda d: -d["score"])
    return items[:limit]


def seam_requests(ctx, outbox):
    """Every `{"kind": "seam"}` request in the lane outbox, with the unit that owns its address."""
    out = []
    if not outbox or not os.path.isdir(outbox):
        return out

    def num(v):
        if isinstance(v, int):
            return v
        try:
            return int(str(v), 16) if str(v).lower().startswith("0x") else int(v)
        except (TypeError, ValueError):
            return None

    def walk(o, f):
        if isinstance(o, dict):
            if o.get("kind") == "seam":
                addr = next((num(o[k]) for k in ("new", "addr", "start", "old") if k in o and num(o[k]) is not None), None)
                u = ctx.text_owner(addr) if addr is not None else None
                out.append({"file": f, "addr": hx(addr) if addr is not None else "-", "unit": u.name if u else None,
                            "section": o.get("section", ".text"), "evidence": str(o.get("evidence", ""))[:200]})
            for v in o.values():
                walk(v, f)
        elif isinstance(o, list):
            for v in o:
                walk(v, f)

    for fn in sorted(os.listdir(outbox)):
        if fn.endswith(".json"):
            try:
                with open(os.path.join(outbox, fn), encoding="utf-8") as fh:
                    walk(json.load(fh), fn)
            except (OSError, ValueError):
                pass
    return out


def report_json(ctx, res, bounds, extra=None):
    return {"units": res.units, "coverage_gaps": getattr(ctx, "coverage_gaps", {}), "summary": res.summary(), "boundaries": bounds,
            "top_defects": top_defects(ctx, res, 10), **(extra or {})}


def print_table(ctx, res, show_all=False, limit=40):
    summ = res.summary()
    print("%-11s %6s %6s %8s %6s" % ("invariant", "PASS", "FAIL", "UNKNOWN", "-"))
    for inv in INVARIANTS:
        s = summ[inv]
        print("%-11s %6d %6d %8d %6d" % (inv, s[PASS], s[FAIL], s[UNKNOWN], s[NA] + len(ctx.units) - sum(s.values())))
    rows = []
    for u in ctx.units:
        recs = res.units.get(u.name, {})
        worst = max((RANK[r["status"]] for r in recs.values()), default=0)
        if show_all or worst >= RANK[FAIL]:
            rows.append((u.name, recs))
    print("\nunits with a FAIL: %d of %d%s" % (sum(1 for n, r in rows if any(x["status"] == FAIL for x in r.values())), len(ctx.units),
                                             "" if show_all else " (--all lists every unit)"))
    print("%-44s %s" % ("unit", " ".join("%-4s" % i[:4] for i in INVARIANTS)))
    for name, recs in rows[:limit if not show_all else None]:
        cells = []
        for inv in INVARIANTS:
            r = recs.get(inv)
            cells.append("%-4s" % ({PASS: "ok", FAIL: "FAIL", UNKNOWN: "?", NA: "-"}[r["status"]] if r else "-"))
        print("%-44s %s" % (name[:44], " ".join(cells)))
    if len(rows) > limit and not show_all:
        print("... %d more" % (len(rows) - limit))


def print_defects(items):
    print("\ntop defects")
    for i, d in enumerate(items, 1):
        print("%2d. [%s] %s @ %s  %s" % (i, d["invariant"], d["unit"], hx(d["addr"]), d["finding"][:150]))


def analyse(splits, symbols, dol, only=None, outbox=None, sda=(None, None)):
    ctx = Ctx(splits, symbols, dol, sda[0], sda[1])
    res = run_checks(ctx, dataorder_rows(symbols), only)
    bounds = boundaries(ctx, res)
    return ctx, res, bounds


def parse_readers_spec(spec):
    """`SECTION:START-END` (hex addresses) -> `(section, start, end)`."""
    m = re.match(r"^(\.?\w+):(0x[0-9A-Fa-f]+)-(0x[0-9A-Fa-f]+)$", spec or "")
    if not m:
        raise ValueError("--readers wants SECTION:0xSTART-0xEND, got %r" % (spec,))
    return m.group(1), int(m.group(2), 16), int(m.group(3), 16)


def readers_report(ctx, spec):
    """One line per map symbol of `spec`'s section range: its owner unit, then the units whose decoded text reads it
    (`literal_readers` for a pool literal) with the site counts."""
    sec, lo, hi = parse_readers_spec(spec)
    lines = []
    for sym in ctx.data_syms:
        if sym["section"] != sec or not (lo <= sym["addr"] < hi):
            continue
        sites = ctx.literal_readers(sym) if is_literal(sym) else ctx.readers(sym)
        by = collections.Counter((ctx.text_owner(x).name if ctx.text_owner(x) else "?") for x in sites)
        own = ctx.owner(sec, sym["addr"])
        lines.append("%s %s %-26s size 0x%X owner %-34s readers %s" % (
            sec, hx(sym["addr"]), sym["name"], sym["size"] or 0, own.name if own else "(unowned)",
            ", ".join("%s x%d" % (u, n) for u, n in sorted(by.items())) or "none"))
    return lines
