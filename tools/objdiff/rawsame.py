#!/usr/bin/env python3
"""Every function a unit scores 100 % on, compared byte for byte with the target (relocated operand bits masked). Spec: docs/tools/spec/rawsame.md.
CLI: python tools/objdiff/rawsame.py <unit>... [--report [R]] [--min-percent P] [--json]."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os
import sys

from tools.lib import rawsame as _rawsame
from tools.lib import repo as _repo
from tools.lib import report as _report
from tools.lib import units as _units

REPORT_REL = os.path.join("build", "RMHE08", "report.json")


def scored_names(unit, root: str, report_path: str | None, minimum: float) -> tuple[list[str] | None, str | None]:
    """The unit's functions at or above `minimum` percent: from the project report when `report_path` is given,
    else from one `objdiff report generate` over the two objects on disk (never stale). `(None, why)` on failure."""
    if report_path:
        try:
            rep = _report.Report.load(report_path)
        except _report.ReportError as exc:
            return None, "cannot read the report: %s" % exc
        entry = rep.unit(unit.report_name)
        if entry is None:
            return None, "%s is not in %s" % (unit.report_name, report_path)
        funcs = entry.get("functions") or []
        return [f["name"] for f in funcs if f.get("name") and _report.score_of(f) >= minimum], None
    entries = _report.score_entries(unit.obj_target, unit.obj_ours, unit.report_name, _repo.session_tmpdir(),
                                    objdiff=_report.objdiff_cli(root), cwd=root)
    if "_error" in entries:
        return None, "objdiff report failed: %s" % entries["_error"][:300]
    return [n for n, e in entries.items() if n and _report.score_of(e) >= minimum], None


def check_unit(spec: str, root: str, report_path: str | None = None, minimum: float = 100.0) -> dict:
    """`{unit, rows, checked, differing, error}` for one unit spec."""
    unit = _units.Unit.resolve(spec, root)
    rec = {"unit": unit.report_name, "checked": 0, "differing": 0, "rows": [], "error": None}
    for path, what in ((unit.obj_ours, "our object (build it)"), (unit.obj_target, "the split target object")):
        if not os.path.exists(path):
            rec["error"] = "%s does not exist: %s" % (what, path)
            return rec
    names, why = scored_names(unit, root, report_path, minimum)
    if names is None:
        rec["error"] = why
        return rec
    rows = _rawsame.compare(unit.obj_target, unit.obj_ours, sorted(names))
    rec["checked"] = len(rows)
    rec["rows"] = [{"name": r.name, "status": r.status, "target_size": r.target_size, "ours_size": r.ours_size,
                    "diffs": [list(d) for d in r.diffs], "line": r.line(unit.report_name)} for r in rows if r.differs]
    rec["differing"] = len(rec["rows"])
    return rec


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("units", nargs="*", help="unit paths, e.g. quest/arenatask")
    ap.add_argument("--report", nargs="?", const=REPORT_REL, metavar="R",
                    help="read the scored rows from this report (default build/RMHE08/report.json) instead of "
                         "scoring the two objects on disk")
    ap.add_argument("--min-percent", type=float, default=100.0, help="judge functions at or above this percent")
    ap.add_argument("--json", action="store_true", help="the records as JSON")
    args = ap.parse_args(argv)
    if not args.units:
        ap.error("a unit is required")
    root = _repo.repo_root()
    report_path = os.path.join(root, args.report) if args.report and not os.path.isabs(args.report) else args.report
    records = [check_unit(u, root, report_path, args.min_percent) for u in args.units]
    if args.json:
        print(json.dumps({"tool": "rawsame", "units": records,
                          "ok": not any(r["differing"] or r["error"] for r in records)}, indent=2))
    else:
        for rec in records:
            if rec["error"]:
                print("%s: cannot compare: %s" % (rec["unit"], rec["error"]))
                continue
            for row in rec["rows"]:
                print(row["line"])
            print("%s: %d function(s) at %g %% or more, %d with raw differences"
                  % (rec["unit"], rec["checked"], args.min_percent, rec["differing"]))
    if any(r["error"] for r in records):
        return 2
    return 1 if any(r["differing"] for r in records) else 0


if __name__ == "__main__":
    sys.exit(main())
