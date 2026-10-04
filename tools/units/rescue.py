#!/usr/bin/env python3
"""Audit the `refs/rescue/*` safety net and prune only what is provably redundant (`lib.lanes.rescue`).
Spec: docs/tools/spec/rescue.md. CLI: python tools/units/rescue.py audit [--prune] [--json] [--full-diff]
[--ref REF]... [--repo PATH] [--main REF] [--prefix P]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json

from tools.lib import cli
from tools.lib.lanes import rescue as _rescue
from tools.lib.lanes.rescue import (CONFIGURE, DEFAULT_PREFIX, DERIVATION_NONE, DERIVATION_REGISTERED,  # noqa: F401
                                    DERIVATION_TOUCHED, SPLITS, VERDICT_DRIFT, VERDICT_REDUNDANT,
                                    VERDICT_UNKNOWN, VERDICT_UNLANDED, VERDICTS, audit, default_repo)

TOOL = cli.Tool("rescue", "docs/tools/spec/rescue.md", description=(__doc__ or "").splitlines()[0], common=())
classify_ref = _rescue.classify


def render(report: dict) -> str:
    lines: list[str] = []
    for row in report["refs"]:
        units = ", ".join(row["units"]) or "(none)"
        on_main = ", ".join("%s=%s" % (u, "yes" if ok else "NO") for u, ok in row["units_on_main"].items()) or "(n/a)"
        lines.append("%s  %s  [%s]" % (row["ref"], row["date"] or "?", row["verdict"]))
        lines.append("    units: %s  (derivation: %s)" % (units, row["derivation"]))
        if row["units_on_main"]:
            lines.append("    on main: %s" % on_main)
        lines.append("    reason: %s" % row["reason"])
        if row["drift_paths"]:
            lines.append("    drifted paths (%d): %s" % (len(row["drift_paths"]), ", ".join(row["drift_paths"][:8])
                                                        + (" ..." if len(row["drift_paths"]) > 8 else "")))
    lines.append("")
    s = report["summary"]
    lines.append("rescue audit: %d ref(s) - redundant %d, landed-with-drift %d, unlanded %d, unknown %d"
                 % (report["count"], s[VERDICT_REDUNDANT], s[VERDICT_DRIFT], s[VERDICT_UNLANDED], s[VERDICT_UNKNOWN]))
    if report["prune"]:
        lines.append("pruned %d redundant ref(s): %s" % (len(report["deleted"]), ", ".join(report["deleted"]) or "(none)"))
    for row in report["refs"]:
        if row["verdict"] == VERDICT_UNLANDED:
            lines.append("  UNLANDED  %s  %s  unit(s): %s" % (row["ref"], row["date"] or "?", ", ".join(row["units"])))
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    ap = TOOL.parser()
    ap.add_argument("cmd", nargs="?", choices=["audit"], default="audit", help="audit the rescue refs (the only command)")
    ap.add_argument("--prune", action="store_true", help="delete the redundant refs (and only those), printing each deletion")
    ap.add_argument("--json", action="store_true", help="emit the report as JSON")
    ap.add_argument("--full-diff", action="store_true",
                    help="include the complete touched-path patch in each report row (large)")
    ap.add_argument("--ref", action="append", default=[], help="audit only this ref; repeatable")
    ap.add_argument("--repo", default=None, help="the repository to inspect (default: this checkout)")
    ap.add_argument("--main", default="main", help="the branch that represents landed state (default: main)")
    ap.add_argument("--prefix", default=DEFAULT_PREFIX, help="the rescue ref prefix")
    args = ap.parse_args(argv)
    report = audit(args.repo or default_repo(), args.main, args.prefix, refs=args.ref or None, prune=args.prune,
                   full_diff=args.full_diff)
    print(json.dumps(report, indent=2) if args.json else render(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
