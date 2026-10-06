#!/usr/bin/env python3
"""Read and summarise the landing log, `MAIN/.pi/land-log.jsonl` (`lib.lanes.landlog`). Spec: docs/tools/spec/landlog.md.
CLI: python tools/units/landlog.py [summary] [--last N] [--json] | list [--last N] [--json];
tests: tools/tests/units/test_landlog.py."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json

from tools.lib import cli
from tools.lib.lanes import landlog, registry

TOOL = cli.Tool("landlog", "docs/tools/spec/landlog.md", description=(__doc__ or "").splitlines()[0], common=())


def render_summary(s: dict, bad: list[int], path: str) -> str:
    lines = ["landing log %s: %d attempt(s)%s" % (path, s["attempts"], "" if not bad else
                                                 " (%d unreadable line(s): %s)" % (len(bad), ", ".join(map(str, bad[:10]))))]
    if not s["attempts"]:
        return "\n".join(lines)
    o = s["outcomes"]
    lines.append("  landed %d, refused %d, conflict %d, error %d (landed ratio %.0f%%)"
                 % (o["landed"], o["refused"], o["conflict"], o["error"], 100 * (s["landed_ratio"] or 0)))
    lines.append("  wall time %.0f s in total; median landing %s s"
                 % (s["seconds_total"], s["seconds_median_landed"] if s["seconds_median_landed"] is not None else "-"))
    if s["refused_rows"]:
        lines.append("  refused by:")
        lines += ["    %3d  %s" % (n, row) for row, n in s["refused_rows"][:10]]
    allowances = s.get("allowances") or {}
    lines.append("  allowances: %s" % ("; ".join("%s %d attempt(s), %d entr%s" % (cls, v["attempts"], v["entries"],
                                                                              "y" if v["entries"] == 1 else "ies")
                                                for cls, v in allowances.items()) or "none recorded"))
    if s.get("manifests"):
        lines.append("  lane manifests: %d attempt(s) were judged against one (`land --manifest`)" % s["manifests"])
    if s.get("warning_rows"):
        lines.append("  warnings (never a refusal): %s" % "; ".join("%d %s" % (n, row) for row, n in s["warning_rows"]))
    schemas = s.get("schemas") or {}
    if schemas.get(1):
        lines.append("  (%d schema-1 line(s) predate the allowance record and carry none)" % schemas[1])
    if s["conflicted_paths"]:
        lines.append("  conflicted paths:")
        lines += ["    %3d  %s" % (n, p) for p, n in s["conflicted_paths"][:10]]
    return "\n".join(lines)


def render_list(rows: list[dict]) -> str:
    out = []
    for r in rows:
        why = r.get("refused_row") or ", ".join(r.get("conflicts") or []) or (r.get("commit") or "")
        allow = r.get("allow") if isinstance(r.get("allow"), dict) else {}
        if allow:
            why += "  [allow: %s]" % ", ".join(sorted(allow))
        if r.get("manifest"):
            why += "  [manifest: %s]" % r["manifest"]
        out.append("%s  %-8s %6.1fs  %-40s %s" % (r.get("at", "?"), r.get("outcome"), float(r.get("seconds") or 0),
                                                 (r.get("branch") or "")[:40], why))
    return "\n".join(out) or "no landing attempts recorded"


def main(argv: list[str] | None = None) -> int:
    ap = TOOL.parser()
    ap.add_argument("cmd", nargs="?", choices=("summary", "list"), default="summary")
    ap.add_argument("--last", type=int, default=None, help="only the last N attempts")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--main", default=None, help="MAIN's path (default: resolved from the current tree)")
    args = ap.parse_args(argv)
    main_wt = args.main or registry.main_of()
    rows, bad = landlog.read(main_wt)
    if args.last:
        rows = rows[-args.last:]
    if args.cmd == "list":
        print(json.dumps(rows, indent=2) if args.json else render_list(rows))
        return 0
    s = landlog.summary(rows)
    print(json.dumps(dict(s, unreadable_lines=bad), indent=2) if args.json
          else render_summary(s, bad, landlog.log_path(main_wt)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
