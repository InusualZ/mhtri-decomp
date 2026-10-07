"""The regression rows: no symbol or unit fell against the base's report snapshot (`lib.report.regression`).
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os

from tools.lib import project as _project
from tools.lib import report as _report
from tools.units.landing.base import report_snapshot
from tools.units.landing.common import Batch, KIND_BOOKKEEPING, run


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


def seam_moves_of(before_text: str, after_text: str) -> tuple[list[tuple[str, str, int, int]], set[str]]:
    """(`.text` moves, touched units) of a `splits.txt` diff, under the report's unit names (`main/<stem>`)."""
    before, after = _project.Splits.parse(before_text), _project.Splits.parse(after_text)
    name = lambda unit: "main/" + _project.splits.stem(unit)
    return ([(name(a), name(b), lo, hi) for a, b, lo, hi in _project.splits.range_moves(before, after)],
            {name(u) for u in _project.splits.touched_units(before, after)})


def batch_seam_moves(b: Batch) -> tuple[list[tuple[str, str, int, int]], set[str]]:
    """`seam_moves_of` the batch base's `splits.txt` and the working tree's; ([], set()) when either is unreadable."""
    if not b.base:
        return [], set()
    shown = run(["git", "show", "%s:config/RMHE08/splits.txt" % b.base], b.main)
    try:
        with open(os.path.join(b.main, "config", "RMHE08", "splits.txt"), encoding="utf-8", errors="replace") as fh:
            now = fh.read()
    except OSError:
        return [], set()
    return seam_moves_of(shown.stdout or "", now) if shown.returncode == 0 else ([], set())


# --- the rows -------------------------------------------------------------------------------------------------

def regression_check_rows(b: Batch) -> None:
    """18. no symbol or unit regressed against the base snapshot (`lib.report.regression`); every
    `--allow-regression` was needed (a WARNING since 2026-10-05, never a refusal); the base carried a report snapshot
    (BOOKKEEPING)."""
    before_report = b.recorded.get("report") or {}
    after_report = report_snapshot(b.main)
    unauthorised, authorised = report_regressions(before_report, after_report, b.allow_regression)
    seams, why_not = [], ""
    if unauthorised:
        moves, touched = batch_seam_moves(b)
        unauthorised, seams, why_not = _report.seam_exempt(before_report, after_report, moves, touched, unauthorised)
        for seam in seams:
            b.seam_moves.append("%s -> %s (%d functions)" % (seam["from"], seam["to"], seam["functions"]))
        if why_not:
            print("note: seam move not credited: " + why_not)
    for row in authorised:
        print("note: regression ALLOWED by --allow-regression: %s %s %.2f -> %.2f" % row)
    b.check("no symbol or unit regressed", not unauthorised,
            "; ".join("%s %s %.2f -> %.2f" % r for r in unauthorised[:5]),
            info="; ".join(filter(None, [
                ("%d authorised regression(s)" % len(authorised)) if authorised else "",
                "; ".join("seam move: %s -> %s, %d functions, none worse" % (s["from"], s["to"], s["functions"])
                          for s in seams)])))
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
