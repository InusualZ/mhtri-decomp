#!/usr/bin/env python3
"""Diff two report.json snapshots - moved rows, added/removed units, the denominators - with the verdict as the exit
status (`lib.report.compare`). Spec: docs/tools/spec/reportdiff.md.
CLI: python tools/units/reportdiff.py <before.json> <after.json> [--json] [--changed-only] [--limit N] [--eps E] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os

from tools.lib import report as _report

MEASURE_ORDER = _report.MEASURE_ORDER
#: The task's short names (`fuzzy 23.229952`) where they differ from the report's own key.
METRIC_LABEL = {"fuzzy_match_percent": "fuzzy"}
REGRESSION_KEYS = _report.REGRESSION_KEYS
DEFAULT_EPS = _report.DEFAULT_EPS
ReportError = _report.ReportError
num = _report.num


def load_report(path: str) -> dict:
    """Read one report (`lib.report.Report.load`), refusing anything that is not a report."""
    return _report.Report.load(path).data


def unit_measures(report: dict) -> dict[str, dict]:
    """`{unit name: measures}` - the unit-level score rows."""
    return _report.Report.coerce(report).unit_measures()


def symbol_measures(report: dict) -> dict[tuple, float]:
    """`{(unit name, symbol name): score}` - the symbol-level rows, with the 0 % rule."""
    return _report.Report.coerce(report).symbol_measures()


def denominators(report: dict) -> dict[str, dict]:
    """`{scope: {measure key: number}}` for the project total and every category."""
    return _report.Report.coerce(report).denominators()


diff_units = _report.diff_units
diff_symbols = _report.diff_symbols
diff_denominators = _report.diff_denominators


#: The whole diff as a JSON-safe payload, plus the verdict the exit status encodes.
build = _report.compare


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
    counts = _report.direction(units["moved"])
    out.append("  %d moved (%d up, %d down); %d added; %d removed"
               % (counts["moved"], counts["up"], counts["down"], len(units["added"]), len(units["removed"])))
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
    counts = _report.direction(symbols["moved"])
    out.append("  %d moved (%d up, %d down); %d added; %d removed"
               % (counts["moved"], counts["up"], counts["down"], len(symbols["added"]), len(symbols["removed"])))
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
