#!/usr/bin/env python3
"""Read-only audit of `splits.txt`: twelve invariants per unit against the retail DOL and the map.
Spec: docs/tools/spec/splitcheck.md. CLI: splitcheck.py --baseline [--json F] [--all] [--only INV]
[--unit REGEX] [--intervals] | --readers SEC:START-END."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import collections
import json
import os
import re
import sys

from tools.lib import cli as _cli
from tools.splits.invariants.audit import (FAIL, INVARIANTS, Ctx, analyse, ctors_detail, hx, load_ctx, main_root,
                                           pool_groups, pool_intervals, print_defects, print_table, readers_report,
                                           report_json, seam_requests, top_defects, tree_root)

TOOL = _cli.Tool("splitcheck", "docs/tools/spec/splitcheck.md", description=__doc__, common=())


def cmd_baseline(args):
    splits, symbols, dol = load_ctx(args.splits, args.symbols, args.dol)
    ctx, res, bounds = analyse(splits, symbols, dol, args.only.split(",") if args.only else None)
    if args.unit:
        keep = re.compile(args.unit)
        ctx.units = [u for u in ctx.units if keep.search(u.name)]
        res.units = collections.OrderedDict((k, v) for k, v in res.units.items() if keep.search(k))
        bounds = [b for b in bounds if keep.search(b["left"]) or keep.search(b["right"])]
    outbox = args.outbox or os.path.join(main_root() or tree_root(), ".pi", "outbox")
    seams = seam_requests(ctx, outbox)
    only_set = set(args.only.split(",")) if args.only else None
    if args.unit and (only_set is None or only_set & {"ctors", "dtors"}):
        for u in ctx.units:
            for sec, inv in ((".ctors", "ctors"), (".dtors", "dtors")):
                if u.ranges.get(sec) and (only_set is None or inv in only_set):
                    for line in ctors_detail(ctx, u, sec):
                        print("detail %s: %s" % (u.name, line))
    if args.intervals:
        keep = re.compile(args.unit).search if args.unit else None
        for line in pool_intervals(ctx, keep):
            print(line)
    groups = pool_groups(ctx)
    print("baseline: %d units, %d map symbols, r13=%s r2=%s" % (len(ctx.units), len(symbols), hx(ctx.sda13), hx(ctx.sda2)))
    print_table(ctx, res, args.all, args.limit)
    print_defects(top_defects(ctx, res, 10))
    print("\none worst defect per failing invariant")
    allf = top_defects(ctx, res, 100000)
    for inv in INVARIANTS:
        d = next((x for x in allf if x["invariant"] == inv), None)
        if d:
            print("  [%s] %d units fail; e.g. %s @ %s  %s" % (inv, sum(1 for x in allf if x["invariant"] == inv), d["unit"], hx(d["addr"]), d["finding"][:120]))
    bad_b = [b for b in bounds if FAIL in b["checks"].values()]
    print("\nboundaries: %d text cuts, %d with a failing check (fn-start %d, pool-shared %d)"
          % (len(bounds), len(bad_b), sum(1 for b in bounds if b["checks"]["fn-start"] == FAIL),
             sum(1 for b in bounds if b["checks"]["pool-shared"] == FAIL)))
    print("suspected: %d seam requests in the outbox (%d with an owning unit), %d pool groups (largest %s)"
          % (len(seams), sum(1 for s in seams if s["unit"]), len(groups), len(groups[0]["units"]) if groups else 0))
    gaps = getattr(ctx, "coverage_gaps", {})
    print("unowned (no unit range): " + ", ".join("%s %d syms/%d runs" % (k, v["symbols"], v["runs"]) for k, v in sorted(gaps.items())))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(report_json(ctx, res, bounds, {"seam_requests": seams, "pool_groups": groups,
                                                     "sda": {"r13": ctx.sda13, "r2": ctx.sda2},
                                                     "rows": [r.to_dict() for r in res.rows()]}), fh, indent=1)
    return 0


def cmd_readers(args):
    splits, symbols, dol = load_ctx(args.splits, args.symbols, args.dol)
    ctx = Ctx(splits, symbols, dol)
    for spec in args.readers:
        for line in readers_report(ctx, spec):
            print(line)
    return 0


def main(argv=None):
    ap = TOOL.parser(formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--baseline", action="store_true", help="check the current splits.txt")
    ap.add_argument("--splits"), ap.add_argument("--symbols"), ap.add_argument("--dol")
    ap.add_argument("--outbox", help="lane outbox with seam requests (default: the primary checkout's .pi/outbox)")
    ap.add_argument("--readers", action="append", metavar="SEC:START-END",
                    help="print each map symbol of that data range with its owner and the units that read it (repeatable)")
    ap.add_argument("--only", help="comma list of invariants")
    ap.add_argument("--unit", help="regex: report only the units whose name matches (--baseline)")
    ap.add_argument("--json", help="write the machine-readable report here")
    ap.add_argument("--all", action="store_true", help="list every unit, not only the failing ones")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--intervals", action="store_true",
                    help="with --baseline: print each pool value held at two addresses (read by --unit's units) with the interval a TU starts in")
    args = ap.parse_args(argv)
    if args.readers:
        return cmd_readers(args)
    if args.baseline:
        return cmd_baseline(args)
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
