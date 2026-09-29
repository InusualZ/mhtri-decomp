#!/usr/bin/env python3
"""Diff two `build/<game>/report.json` snapshots: which rows moved, which units appeared, and the
category denominators - the instrument two lanes hand-wrote and neither kept.

    python tools/units/reportdiff.py <before.json> <after.json> [--json] [--changed-only] [--limit N]
    python tools/units/reportdiff.py --selftest

**The question this answers, and why it needs a tool.** Every data/declaration batch has to prove it
did not move codegen ("the whole-project report is unchanged row for row"), and the two lanes that
pinned the `Pl` `.sdata2` pool wrote the same JSON diff by hand - twice - from a scratch script that no
one else could run. Their acceptance criteria were exactly the ones an ad-hoc diff gets wrong: *no row
may drop* (a unit or symbol whose `fuzzy_match_percent` fell is a regression, even if the totals rose)
and *any row that moves at all* must be named (the change is either explained by the batch or it is a
surprise). This tool is that diff, with the exit status as the verdict.

**What it prints.**

* **unit rows whose score moved**, with before -> after (worst first), keyed on the unit's
  `fuzzy_match_percent`;
* **symbol rows whose score moved**, the same way, keyed on `(unit, symbol)` - the report's `functions`
  rows, which is the level a lane can act on;
* **units added and removed** (a claim re-tiles the `auto_*` units, so the set always churns - the
  names are the evidence, not a count);
* **the category denominators** - the project totals and every report `category` (`game`, `sdk`,
  `auto`), as a before/after table with deltas: `fuzzy`, `matched_code`, `matched_data`, `total_units`,
  `total_code`, `total_data`, `matched_functions`, `complete_*` and the percent columns.

**Exit status is the verdict** (the house codes):

* `0` nothing regressed - no unit or symbol row dropped, no numerator denominator fell;
* `1` a row dropped - the drops are listed even when the aggregate improved;
* `2` nothing comparable was found - a path that is missing, unreadable, not a report, or a pair with
  no unit and no denominator in common. A traceback is never the answer.

**Direction matters, so it is explicit.** `<before.json>` is the baseline, `<after.json>` the candidate;
swapping the arguments swaps every delta and the verdict. Both files are ordinary `report.json`s (dtk's
`ninja build/<game>/report.json`), so `git show` of a committed report, a copy of an earlier build, or
MAIN's own report all work unchanged.
"""
from __future__ import annotations

import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

#: The report `measures` keys, in the order a reader wants them: the score, then code, data, functions,
#: the "complete" family, and the unit counts last.
MEASURE_ORDER = (
    "fuzzy_match_percent",
    "matched_code", "matched_code_percent",
    "total_code",
    "matched_data", "matched_data_percent",
    "total_data",
    "matched_functions", "matched_functions_percent",
    "total_functions",
    "complete_code", "complete_code_percent",
    "complete_data", "complete_data_percent",
    "complete_units",
    "total_units",
)
#: The task's short names (`fuzzy 23.229952`) where they differ from the report's own key.
METRIC_LABEL = {"fuzzy_match_percent": "fuzzy"}
#: A *fall* in one of these is a regression **at project scope**; a `total_*` rising or falling is a
#: re-tiling, not a loss, and a `category` denominator moves with the bucketing rather than with a byte,
#: so category rows are reported (and marked) but never set the exit status on their own.
REGRESSION_KEYS = frozenset((
    "fuzzy_match_percent", "matched_code", "matched_code_percent",
    "matched_data", "matched_data_percent",
    "matched_functions", "matched_functions_percent",
    "complete_code", "complete_code_percent",
    "complete_data", "complete_data_percent",
    "complete_units",
))
DEFAULT_EPS = 1e-9


class ReportError(Exception):
    """A path that cannot serve as one side of the diff (missing, unreadable, or not a report)."""


def num(value):
    """A measure as a number, or None. The report stores counts as strings and percents as floats."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return value
    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None
        try:
            return int(text)
        except ValueError:
            try:
                return float(text)
            except ValueError:
                return None
    return None


def load_report(path: str) -> dict:
    """Read one report, refusing anything that is not a report so `nothing comparable` is honest."""
    if not os.path.exists(path):
        raise ReportError("%s does not exist" % path)
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
    except (OSError, ValueError) as exc:
        raise ReportError("%s is not readable JSON (%s)" % (path, exc))
    if not isinstance(data, dict) or ("units" not in data and "measures" not in data):
        raise ReportError("%s is not a report (no `units` and no `measures`)" % path)
    return data


def unit_measures(report: dict) -> dict[str, dict]:
    """`{unit name: measures}` - the unit-level score rows."""
    out: dict[str, dict] = {}
    for unit in report.get("units") or []:
        name = unit.get("name")
        if name:
            out[name] = unit.get("measures") or {}
    return out


def symbol_measures(report: dict) -> dict[tuple, float | None]:
    """`{(unit name, symbol name): fuzzy_match_percent}` - the symbol-level score rows."""
    out: dict[tuple, float | None] = {}
    for unit in report.get("units") or []:
        name = unit.get("name")
        if not name:
            continue
        for fn in unit.get("functions") or []:
            symbol = fn.get("name")
            if symbol:
                out[(name, symbol)] = num(fn.get("fuzzy_match_percent"))
    return out


def denominators(report: dict) -> dict[str, dict]:
    """`{scope: {measure key: number}}` for the project total and every category - the denominator rows."""
    out: dict[str, dict] = {"project": {k: num(v) for k, v in (report.get("measures") or {}).items()}}
    for category in report.get("categories") or []:
        scope = category.get("id") or category.get("name")
        if scope:
            out[str(scope)] = {k: num(v) for k, v in (category.get("measures") or {}).items()}
    return out


def _moved_rows(before: dict, after: dict, eps: float) -> list[dict]:
    """`(key, before, after)` for every key both sides carry whose value moved past `eps`."""
    rows = []
    for key in sorted(set(before) & set(after)):
        a, b = before[key], after[key]
        if a is None or b is None:
            continue
        delta = b - a
        if abs(delta) > eps:
            rows.append({"key": key, "before": a, "after": b, "delta": delta})
    return rows


def diff_units(before: dict, after: dict, eps: float = DEFAULT_EPS) -> dict:
    """Unit-level rows: moved (worst first), added, removed."""
    ub, ua = unit_measures(before), unit_measures(after)
    moved = []
    for row in _moved_rows({k: num(v.get("fuzzy_match_percent")) for k, v in ub.items()},
                           {k: num(v.get("fuzzy_match_percent")) for k, v in ua.items()}, eps):
        moved.append({"unit": row["key"], "before": row["before"], "after": row["after"],
                      "delta": row["delta"]})
    moved.sort(key=lambda r: (r["delta"], r["unit"]))
    return {"moved": moved,
            "added": sorted(set(ua) - set(ub)),
            "removed": sorted(set(ub) - set(ua))}


def diff_symbols(before: dict, after: dict, eps: float = DEFAULT_EPS) -> dict:
    """Symbol-level rows: moved (worst first), plus how many symbols appeared and vanished."""
    sb, sa = symbol_measures(before), symbol_measures(after)
    moved = []
    for row in _moved_rows(sb, sa, eps):
        unit, symbol = row["key"]
        moved.append({"unit": unit, "symbol": symbol, "before": row["before"],
                      "after": row["after"], "delta": row["delta"]})
    moved.sort(key=lambda r: (r["delta"], r["unit"], r["symbol"]))
    return {"moved": moved,
            "added": sorted(set(sa) - set(sb)),
            "removed": sorted(set(sb) - set(sa))}


def diff_denominators(before: dict, after: dict, eps: float = DEFAULT_EPS) -> list[dict]:
    """The project total and every category, measure by measure, with the delta and a regression flag."""
    db, da = denominators(before), denominators(after)
    scopes = ["project"] + sorted((set(db) | set(da)) - {"project"})
    rows = []
    for scope in scopes:
        mb, ma = db.get(scope, {}), da.get(scope, {})
        keys = [k for k in MEASURE_ORDER if k in mb or k in ma]
        keys += sorted((set(mb) | set(ma)) - set(MEASURE_ORDER))
        for key in keys:
            a, b = mb.get(key), ma.get(key)
            delta = None if (a is None or b is None) else b - a
            fell = delta is not None and delta < -eps
            rows.append({"scope": scope, "metric": key, "before": a, "after": b, "delta": delta,
                         "moved": delta is not None and abs(delta) > eps,
                         "fell": fell,
                         "regressed": bool(fell and scope == "project" and key in REGRESSION_KEYS)})
    return rows


def build(before: dict, after: dict, eps: float = DEFAULT_EPS,
          before_path: str = "?", after_path: str = "?") -> dict:
    """The whole diff as a JSON-safe payload, plus the verdict the exit status encodes."""
    units = diff_units(before, after, eps)
    symbols = diff_symbols(before, after, eps)
    dens = diff_denominators(before, after, eps)
    drops = ([{"kind": "unit", "name": r["unit"], "before": r["before"], "after": r["after"],
               "delta": r["delta"]} for r in units["moved"] if r["delta"] < -eps]
             + [{"kind": "symbol", "name": "%s/%s" % (r["unit"], r["symbol"]), "before": r["before"],
                 "after": r["after"], "delta": r["delta"]} for r in symbols["moved"]
                if r["delta"] < -eps]
             + [{"kind": "denominator", "name": "%s/%s" % (r["scope"], r["metric"]),
                 "before": r["before"], "after": r["after"], "delta": r["delta"]}
                for r in dens if r["regressed"]])
    common_units = len(set(unit_measures(before)) & set(unit_measures(after)))
    shared_measures = len(set(denominators(before).get("project", {}))
                          & set(denominators(after).get("project", {})))
    comparable = common_units or shared_measures
    return {
        "before": before_path, "after": after_path, "eps": eps,
        "units": units, "symbols": symbols, "denominators": dens,
        "drops": drops, "regressed": bool(drops),
        "comparable": {"common_units": common_units, "shared_measures": shared_measures,
                       "ok": bool(comparable)},
        "exit": 1 if drops else (0 if comparable else 2),
    }


# --------------------------------------------------------------------------------------------------
# the human answer
# --------------------------------------------------------------------------------------------------
def _fmt(value) -> str:
    if value is None:
        return "-"
    if isinstance(value, int):
        return str(value)
    return "%.9g" % value


def _fmt_delta(value) -> str:
    if value is None:
        return "-"
    if value == 0:
        return "0"
    return ("%d" % value) if isinstance(value, int) else ("%+.9g" % value)


def _table(header: tuple, rows: list[list[str]]) -> str:
    widths = [len(h) for h in header]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))
    fmt = "  " + "  ".join("%%-%ds" % w for w in widths)
    return "\n".join([fmt % header] + [fmt % tuple(r) for r in rows])


def _grouped(moved: list[dict], limit: int) -> list[dict]:
    """Every drop, plus the biggest gains up to `limit` - a cap on the listing, never on the count."""
    drops = [r for r in moved if r["delta"] < 0]
    if limit and len(moved) > limit:
        return drops + [r for r in moved if r["delta"] >= 0][:max(0, limit - len(drops))]
    return moved


def render(payload: dict, limit: int = 20, changed_only: bool = False) -> str:
    """The one screen: moved rows, added/removed units, and the denominator table."""
    out = ["reportdiff: %s -> %s" % (payload["before"], payload["after"]), ""]
    units, symbols = payload["units"], payload["symbols"]

    out.append("== unit rows whose score moved (fuzzy_match_percent, worst first) ==")
    shown = _grouped(units["moved"], limit)
    if not units["moved"]:
        out.append("  (none - every common unit is identical)")
    else:
        rows = [["", ("! " if r["delta"] < 0 else "  ") + r["unit"], _fmt(r["before"]),
                 _fmt(r["after"]), _fmt_delta(r["delta"])] for r in shown]
        out.append(_table(("", "UNIT", "BEFORE", "AFTER", "DELTA"), rows))
        if len(shown) < len(units["moved"]):
            out.append("  ... (%d more, raise --limit; the count below is exact)"
                       % (len(units["moved"]) - len(shown)))
    up = sum(1 for r in units["moved"] if r["delta"] > 0)
    down = sum(1 for r in units["moved"] if r["delta"] < 0)
    out.append("  %d moved (%d up, %d down); %d added; %d removed"
               % (len(units["moved"]), up, down, len(units["added"]), len(units["removed"])))
    out.append("")

    out.append("== symbol rows whose score moved (worst first) ==")
    shown = _grouped(symbols["moved"], limit)
    if not symbols["moved"]:
        out.append("  (none - every common symbol is identical)")
    else:
        rows = [["", ("! " if r["delta"] < 0 else "  ") + r["unit"], r["symbol"], _fmt(r["before"]),
                 _fmt(r["after"]), _fmt_delta(r["delta"])] for r in shown]
        out.append(_table(("", "UNIT", "SYMBOL", "BEFORE", "AFTER", "DELTA"), rows))
        if len(shown) < len(symbols["moved"]):
            out.append("  ... (%d more, raise --limit)" % (len(symbols["moved"]) - len(shown)))
    up = sum(1 for r in symbols["moved"] if r["delta"] > 0)
    down = sum(1 for r in symbols["moved"] if r["delta"] < 0)
    out.append("  %d moved (%d up, %d down); %d added; %d removed"
               % (len(symbols["moved"]), up, down, len(symbols["added"]), len(symbols["removed"])))
    out.append("")

    out.append("== units added (%d) ==" % len(units["added"]))
    for name in units["added"][:limit] if limit else units["added"]:
        out.append("  + %s" % name)
    if limit and len(units["added"]) > limit:
        out.append("  ... (%d more, raise --limit)" % (len(units["added"]) - limit))
    out.append("")

    out.append("== units removed (%d) ==" % len(units["removed"]))
    for name in units["removed"][:limit] if limit else units["removed"]:
        out.append("  - %s" % name)
    if limit and len(units["removed"]) > limit:
        out.append("  ... (%d more, raise --limit)" % (len(units["removed"]) - limit))
    out.append("")

    out.append("== category denominators (before -> after) ==")
    rows = []
    for row in payload["denominators"]:
        if changed_only and not row["moved"]:
            continue
        rows.append(["", ("! " if row["regressed"] else ("~ " if row["fell"] else "  ")) + row["scope"],
                     METRIC_LABEL.get(row["metric"], row["metric"]), _fmt(row["before"]),
                     _fmt(row["after"]), _fmt_delta(row["delta"])])
    moved = sum(1 for r in payload["denominators"] if r["moved"])
    if not rows:
        out.append("  (none - every denominator is identical)")
    else:
        out.append(_table(("", "SCOPE", "METRIC", "BEFORE", "AFTER", "DELTA"), rows))
    out.append("  %d of %d denominator row(s) moved; `!` is a project-metric regression, `~` is a "
               "denominator that fell with the bucketing (a row, not a verdict)"
               % (moved, len(payload["denominators"])))
    out.append("")

    out.append("== verdict ==")
    out.append("  comparable: %d common unit(s), %d shared project measure(s)"
               % (payload["comparable"]["common_units"], payload["comparable"]["shared_measures"]))
    if payload["exit"] == 2:
        out.append("  NOTHING COMPARABLE (exit 2)")
    elif payload["regressed"]:
        out.append("  %d row(s) dropped (exit 1):" % len(payload["drops"]))
        for drop in payload["drops"][:limit] if limit else payload["drops"]:
            out.append("    ! %-8s %-44s %s -> %s (%s)"
                       % (drop["kind"], drop["name"], _fmt(drop["before"]), _fmt(drop["after"]),
                          _fmt_delta(drop["delta"])))
        if limit and len(payload["drops"]) > limit:
            out.append("    ... (%d more)" % (len(payload["drops"]) - limit))
    else:
        out.append("  NO REGRESSION (exit 0)")
    return "\n".join(line.rstrip() for line in out)


# --------------------------------------------------------------------------------------------------
# self-test: fixtures only (two tiny reports in a temp dir), so the check count is identical anywhere
# --------------------------------------------------------------------------------------------------
def _fixture(units: dict, measures: dict | None = None, categories: dict | None = None) -> dict:
    """A minimal report: `{name: (fuzzy, {"symbol": pct})}`."""
    out_units = []
    for name, (fuzzy, functions) in units.items():
        entry = {"name": name, "measures": {"fuzzy_match_percent": fuzzy},
                 "functions": [{"name": s, "fuzzy_match_percent": p} for s, p in functions.items()]}
        out_units.append(entry)
    report = {"version": 2, "units": out_units}
    if measures is not None:
        report["measures"] = measures
    if categories is not None:
        report["categories"] = [{"id": cid, "name": cid, "measures": m}
                                for cid, m in categories.items()]
    return report


def selftest() -> int:
    """Fixture checks for every comparison, the renderer and all three exit codes."""
    import io
    import contextlib
    import tempfile

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def check_in(name, needle, hay):
        nonlocal checks
        checks += 1
        if needle not in hay:
            fails.append("%s: %r is not in the output:\n%s" % (name, needle, hay[:2000]))

    base = _fixture(
        {"main/a": (50.0, {"fa": 100.0, "fb": 40.0}),
         "main/b": (80.0, {"fc": 90.0}),
         "main/gone": (10.0, {})},
        measures={"fuzzy_match_percent": 40.0, "matched_code": "100", "total_code": "200",
                  "matched_data": "10", "total_data": "50", "matched_functions": 3,
                  "total_functions": 4, "complete_code": "20", "complete_units": 1,
                  "total_units": 3},
        categories={"game": {"fuzzy_match_percent": 40.0, "matched_data": "10", "total_units": 3}})
    after = _fixture(
        {"main/a": (50.0, {"fa": 100.0, "fb": 60.0}),          # fb up, unit unchanged
         "main/b": (70.0, {"fc": 80.0}),                        # unit and symbol down
         "main/new": (99.0, {})},                               # a new unit
        measures={"fuzzy_match_percent": 41.0, "matched_code": "120", "total_code": "200",
                  "matched_data": "9", "total_data": "50", "matched_functions": 3,
                  "total_functions": 4, "complete_code": "20", "complete_units": 1,
                  "total_units": 3},
        categories={"game": {"fuzzy_match_percent": 41.0, "matched_data": "9", "total_units": 3}})

    # -- number reading: counts are strings, percents are floats --------------------------------
    check("num: a string count", num("24544"), 24544)
    check("num: a float percent", num(12.5), 12.5)
    check("num: an empty string is None", num(""), None)
    check("num: a bool is not a number", num(True), None)

    # -- unit rows -----------------------------------------------------------------------------
    units = diff_units(base, after)
    check("unit: only the moved row", [(r["unit"], r["before"], r["after"]) for r in units["moved"]],
          [("main/b", 80.0, 70.0)])
    check("unit: the delta", units["moved"][0]["delta"], -10.0)
    check("unit: added", units["added"], ["main/new"])
    check("unit: removed", units["removed"], ["main/gone"])
    check("unit: an unchanged unit is not a row", all(r["unit"] != "main/a" for r in units["moved"]),
          True)

    # -- symbol rows ---------------------------------------------------------------------------
    symbols = diff_symbols(base, after)
    check("symbol: every moved symbol, worst first",
          [(r["unit"], r["symbol"], r["delta"]) for r in symbols["moved"]],
          [("main/b", "fc", -10.0), ("main/a", "fb", 20.0)])
    check("symbol: added", symbols["added"], [])
    check("symbol: removed", symbols["removed"], [])

    # -- denominators --------------------------------------------------------------------------
    dens = diff_denominators(base, after)
    find = {(r["scope"], r["metric"]): r for r in dens}
    check("denominator: the project fuzzy moved up", find[("project", "fuzzy_match_percent")]["delta"],
          1.0)
    check("denominator: matched_code is read as a number",
          find[("project", "matched_code")]["delta"], 20)
    check("denominator: matched_data falling is a regression",
          find[("project", "matched_data")]["regressed"], True)
    check("denominator: total_data is a denominator, never a regression",
          find[("project", "total_data")]["regressed"], False)
    check("denominator: the `total_units` denominator is present",
          ("project", "total_units") in find, True)
    check("denominator: `complete_units` is present", ("project", "complete_units") in find, True)
    check("denominator: the category scope is present", ("game", "matched_data") in find, True)
    check("denominator: a category's fall is reported, not a verdict",
          (find[("game", "matched_data")]["regressed"], find[("game", "matched_data")]["fell"]),
          (False, True))

    # -- the payload and the verdict -----------------------------------------------------------
    payload = build(base, after, before_path="b.json", after_path="a.json")
    check("build: it reads as regressed", payload["regressed"], True)
    check("build: exit 1 on a drop", payload["exit"], 1)
    check("build: the drops name every kind",
          sorted({d["kind"] for d in payload["drops"]}), ["denominator", "symbol", "unit"])
    check("build: a unit drop is named", any(d["name"] == "main/b" and d["kind"] == "unit"
                                             for d in payload["drops"]), True)
    check("build: comparable counts the common units", payload["comparable"]["common_units"], 2)

    text = render(payload, limit=0)
    check_in("render: the unit table is headed", "UNIT", text)
    check_in("render: the unit before -> after", "80", text)
    check_in("render: the added unit is listed", "+ main/new", text)
    check_in("render: the removed unit is listed", "- main/gone", text)
    check_in("render: the denominator table is headed", "SCOPE", text)
    check_in("render: the short `fuzzy` label is used", "fuzzy", text)
    check_in("render: the drop is called out", "! main/b", text)
    check_in("render: the verdict is a regression", "dropped (exit 1)", text)

    # -- exit 0: identical reports -------------------------------------------------------------
    payload = build(base, base)
    check("build: identical reports are not regressed", payload["regressed"], False)
    check("build: identical reports exit 0", payload["exit"], 0)
    check("build: identical reports have no moved rows", payload["units"]["moved"], [])
    check_in("render: a clean pair says NO REGRESSION", "NO REGRESSION",
             render(payload, limit=0))

    # -- exit 0: an improvement (a row up, nothing down) ---------------------------------------
    payload = build(after, base)                    # reversed: the gain becomes a drop
    check("build: reversed direction turns a symbol gain into a drop",
          any(d["name"] == "main/a/fb" and d["kind"] == "symbol" for d in payload["drops"]), True)
    improving = _fixture({"main/a": (50.0, {"fb": 40.0})},
                         measures={"matched_data": "8"},
                         categories={"game": {"matched_data": "8"}})
    payload = build(improving, _fixture({"main/a": (50.0, {"fb": 60.0})},
                                        measures={"matched_data": "10"},
                                        categories={"game": {"matched_data": "10"}}))
    check("build: an all-up diff exits 0", payload["exit"], 0)

    # -- exit 2: nothing comparable ------------------------------------------------------------
    empty = _fixture({})
    payload = build(base, empty)
    check("build: no common unit and no shared measure is exit 2", payload["exit"], 2)
    check_in("render: exit 2 says NOTHING COMPARABLE", "NOTHING COMPARABLE",
             render(payload, limit=0))
    no_units = {"measures": {"fuzzy_match_percent": 1.0}}          # a report with no `units` rows
    check("build: a measures-only report still compares denominators",
          build(no_units, no_units)["exit"], 0)
    check("build: two disjoint reports are not comparable",
          build(_fixture({"x": (1.0, {})}), _fixture({"y": (1.0, {})}))["exit"], 2)

    # -- load_report refuses a non-report ------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        missing = os.path.join(tmp, "nope.json")
        try:
            load_report(missing)
            raised = None
        except ReportError as exc:
            raised = str(exc)
        check("load: a missing path is a ReportError", raised is not None and "does not exist" in raised,
              True)
        bad = os.path.join(tmp, "bad.json")
        with open(bad, "w", encoding="utf-8") as fh:
            fh.write("{not json")
        try:
            load_report(bad)
            raised = None
        except ReportError as exc:
            raised = str(exc)
        check("load: unreadable JSON is a ReportError",
              raised is not None and "not readable" in raised, True)
        non = os.path.join(tmp, "changes.json")
        with open(non, "w", encoding="utf-8") as fh:
            json.dump({"from": {}, "to": {}}, fh)
        try:
            load_report(non)
            raised = None
        except ReportError as exc:
            raised = str(exc)
        check("load: a non-report is a ReportError",
              raised is not None and "not a report" in raised, True)

        # -- the CLI end to end -----------------------------------------------------------------
        b = os.path.join(tmp, "before.json")
        a = os.path.join(tmp, "after.json")
        with open(b, "w", encoding="utf-8") as fh:
            json.dump(base, fh)
        with open(a, "w", encoding="utf-8") as fh:
            json.dump(after, fh)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main([b, a])
        check("cli: a regressed pair exits 1", rc, 1)
        check_in("cli: the report names the drop", "main/b", buf.getvalue())
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main([b, b])
        check("cli: identical snapshots exit 0", rc, 0)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(buf):
            rc = main([b, non])
        check("cli: a non-report exits 2", rc, 2)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main([b, a, "--json"])
        payload = json.loads(buf.getvalue())
        check("cli: --json parses", rc, 1)
        check("cli: --json carries the exit", payload["exit"], 1)
        check("cli: --json carries the drops", len(payload["drops"]) >= 1, True)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main([b, a, "--changed-only"])
        text = buf.getvalue()
        check("cli: --changed-only exits 1", rc, 1)
        check_in("cli: --changed-only still prints the moved denominator", "matched_data", text)

    if fails:
        print("FAIL (%d)" % len(fails))
        for failure in fails:
            print("  " + failure)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog="Exit status is the verdict: 0 nothing regressed, 1 a row "
                                        "dropped, 2 nothing comparable.")
    ap.add_argument("before", nargs="?", help="the baseline report.json")
    ap.add_argument("after", nargs="?", help="the candidate report.json")
    ap.add_argument("--json", action="store_true", help="machine-readable payload on stdout")
    ap.add_argument("--changed-only", action="store_true",
                    help="the denominator table lists only the rows that moved")
    ap.add_argument("--limit", type=int, default=20,
                    help="max rows listed per section (0 = all); counts are never capped")
    ap.add_argument("--eps", type=float, default=DEFAULT_EPS,
                    help="the smallest score movement that counts as a move (default %g)" % DEFAULT_EPS)
    ap.add_argument("--selftest", action="store_true", help="run the fixture checks and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.before or not args.after:
        ap.error("two report paths are required (before and after)")
    try:
        before = load_report(args.before)
        after = load_report(args.after)
    except ReportError as exc:
        print("reportdiff: %s" % exc, file=sys.stderr)
        return 2
    payload = build(before, after, eps=args.eps, before_path=args.before, after_path=args.after)
    if args.json:
        sys.stdout.write(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    else:
        sys.stdout.write(render(payload, limit=args.limit, changed_only=args.changed_only) + "\n")
    return payload["exit"]


if __name__ == "__main__":
    sys.exit(main())
