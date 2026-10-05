"""The regression rows: no symbol or unit fell against the base's report snapshot (`lib.report.regression`).
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os

from tools.lib import report as _report
from tools.units.landing.base import report_snapshot
from tools.units.landing.common import Batch, KIND_BOOKKEEPING


def regression_rows(changes_json: str) -> list[tuple[str, str, float, float]]:
    """(unit, measure, before, after) for every measure that went down, from a report_changes.json."""
    if not os.path.exists(changes_json):
        return []
    data = json.loads(open(changes_json, encoding="utf-8").read())
    rows = []

    def walk(node):
        if isinstance(node, dict):
            name = node.get("name") or ""
            for key, value in (node.get("measures") or {}).items():
                if isinstance(value, dict) and "old" in value and "new" in value:
                    old, new = value["old"], value["new"]
                    if isinstance(old, (int, float)) and isinstance(new, (int, float)) and new < old - 1e-9:
                        rows.append((name, key, old, new))
            for key in ("units", "children", "categories"):
                for child in (node.get(key) or []):
                    walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(data)
    return [r for r in rows if "auto_" not in r[0]]


unit_grew = _report.unit_grew


def report_regressions(before: dict, after: dict, allow: list[str]) -> tuple[list[tuple], list[tuple]]:
    """-> (unauthorised, authorised) regressions as (unit, what, before, after): the one rule,
    `lib.report.regression` (per symbol; the unit average only for a unit that did not grow)."""
    return _report.regression(before, after, allow)


# --- the rows -------------------------------------------------------------------------------------------------

def regression_check_rows(b: Batch) -> None:
    """18. no symbol or unit regressed against the base snapshot (`lib.report.regression`); every
    `--allow-regression` was needed (a WARNING since 2026-10-05, never a refusal); the base carried a report snapshot
    (BOOKKEEPING)."""
    before_report = b.recorded.get("report") or {}
    after_report = report_snapshot(b.main)
    unauthorised, authorised = report_regressions(before_report, after_report, b.allow_regression)
    for row in authorised:
        print("note: regression ALLOWED by --allow-regression: %s %s %.2f -> %.2f" % row)
    b.check("no symbol or unit regressed", not unauthorised,
            "; ".join("%s %s %.2f -> %.2f" % r for r in unauthorised[:5]),
            info=("%d authorised regression(s)" % len(authorised)) if authorised else "")
    used = {a for a in b.allow_regression if any(a in row[0] for row in authorised)}
    stale_allow = [a for a in b.allow_regression if a not in used]
    b.warn("every --allow-regression was actually needed",
           ["stale allowance %s (no regression of it was authorised)" % a for a in stale_allow],
           remedy="drop the stale --allow-regression flag(s) next time")
    if not before_report:
        b.check("the batch base carries a report snapshot", False,
                "record-base did not snapshot report.json (rebuild it and re-record the base)",
                kind=KIND_BOOKKEEPING,
                remedy="run `ninja build/RMHE08/report.json` then `python tools/units/land.py record-base`")
