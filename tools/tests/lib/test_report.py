"""lib.report: the 0 % rule, the arithmetic identity, the comparisons, the one regression rule, the scoring wire, freshness."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import subprocess
import tempfile
from pathlib import Path

from tools.lib import report, testing

TIER = "fixture"


def _rep(units: dict, measures: dict | None = None, categories: dict | None = None) -> dict:
    """A report dict: `{unit: (fuzzy, total_code, {fn: (size, pct-or-None)})}`."""
    out = []
    for name, (fuzzy, total, fns) in units.items():
        funcs = []
        for fn, (size, pct) in fns.items():
            row = {"name": fn, "size": str(size)}
            if pct is not None:
                row["fuzzy_match_percent"] = pct
            funcs.append(row)
        out.append({"name": name, "measures": {"fuzzy_match_percent": fuzzy, "total_code": str(total)},
                    "functions": funcs})
    d = {"units": out}
    if measures is not None:
        d["measures"] = measures
    if categories is not None:
        d["categories"] = [{"id": k, "measures": v} for k, v in categories.items()]
    return d


def test_numbers_and_the_zero_rule(c):
    c.check("a string count is an int", report.num("24544"), 24544)
    c.check("a float stays", report.num(12.5), 12.5)
    c.check("an empty string is None", report.num(""), None)
    c.check("a bool is not a number", report.num(True), None)
    c.check("an absent key scores 0, not 100", report.score_of({"name": "f"}), 0.0)
    c.check("... and is unscored", (report.is_scored({"name": "f"}), report.entry_score({"name": "f"})), (False, None))
    c.check("a present key is its value", report.score_of({"fuzzy_match_percent": 42.5}), 42.5)
    c.check("a missing entry scores 0", report.score_of(None), 0.0)
    r = report.Report(_rep({"main/a": (25.0, 200, {"f": (100, 50.0), "g": (100, None)})}))
    c.check("functions() applies the rule", r.functions("main/a"), {"f": 50.0, "g": 0.0})
    c.check("scores() is every unit", r.scores(), {"main/a": {"f": 50.0, "g": 0.0}})
    c.check("symbol_measures() keys (unit, symbol)", r.symbol_measures(), {("main/a", "f"): 50.0, ("main/a", "g"): 0.0})
    c.check("an unknown unit has no functions", (r.unit("main/x"), r.functions("main/x")), (None, {}))


def test_arithmetic_identity(c):
    good = report.Report(_rep({"main/a": (25.0, 200, {"f": (100, 50.0), "g": (100, None)})}))
    ok, detail = good.arithmetic_check("main/a")
    c.check("the identity holds with the absent key read as 0", ok, True)
    c.contains("... and says so", detail, "25.00000")
    read_as_100 = report.Report(_rep({"main/a": (75.0, 200, {"f": (100, 50.0), "g": (100, None)})}))
    ok, detail = read_as_100.arithmetic_check("main/a")
    c.check("a unit percent that needs the absent key at 100 fails", ok, False)
    c.contains("... naming the rule", detail, "reads as 0%")
    c.check("no total_code passes with the reason", report.arithmetic_check({}, {}), (True, "no total_code to check"))


def test_load_refuses_a_non_report(c):
    with tempfile.TemporaryDirectory() as tmp:
        c.raises("a missing path", report.ReportError, report.Report.load, os.path.join(tmp, "nope.json"))
        bad = Path(tmp, "bad.json")
        bad.write_text("{not json", encoding="utf-8")
        c.raises("unreadable JSON", report.ReportError, report.Report.load, bad)
        non = Path(tmp, "changes.json")
        non.write_text(json.dumps({"from": {}, "to": {}}), encoding="utf-8")
        c.raises("not a report", report.ReportError, report.Report.load, non)
        good = Path(tmp, "r.json")
        good.write_text(json.dumps(_rep({"main/a": (1.0, 4, {})})), encoding="utf-8")
        c.check("a report loads with its path", report.Report.load(good).path, str(good))
        c.check("read() of a missing file is the empty report",
                report.read(os.path.join(tmp, "nope.json")).units(), [])


def test_comparisons(c):
    before = _rep({"main/a": (50.0, 100, {"fa": (50, 100.0), "fb": (50, 40.0)}),
                   "main/b": (80.0, 100, {"fc": (100, 90.0)}), "main/gone": (10.0, 1, {})},
                  measures={"fuzzy_match_percent": 40.0, "matched_data": "10", "total_data": "50"},
                  categories={"game": {"matched_data": "10"}})
    after = _rep({"main/a": (50.0, 100, {"fa": (50, None), "fb": (50, 60.0)}),
                  "main/b": (70.0, 100, {"fc": (100, 80.0)}), "main/new": (99.0, 1, {})},
                 measures={"fuzzy_match_percent": 41.0, "matched_data": "9", "total_data": "50"},
                 categories={"game": {"matched_data": "9"}})
    units = report.diff_units(before, after)
    c.check("unit rows: only the moved one", [(r["unit"], r["delta"]) for r in units["moved"]], [("main/b", -10.0)])
    c.check("units added/removed", (units["added"], units["removed"]), (["main/new"], ["main/gone"]))
    syms = report.diff_symbols(report.Report(before), report.Report(after))
    c.check("symbol rows worst first; a symbol losing its score is a drop to 0",
            [(r["symbol"], r["delta"]) for r in syms["moved"]], [("fa", -100.0), ("fc", -10.0), ("fb", 20.0)])
    dens = {(r["scope"], r["metric"]): r for r in report.diff_denominators(before, after)}
    c.check("a project numerator falling is a regression", dens[("project", "matched_data")]["regressed"], True)
    c.check("a denominator is never one", dens[("project", "total_data")]["regressed"], False)
    c.check("a category fall is reported, not a verdict",
            (dens[("game", "matched_data")]["fell"], dens[("game", "matched_data")]["regressed"]), (True, False))

    rows = report.moved({"a": 50.0, "b": 100.0, "c": 5.0, "d": None, "e": 1.0},
                        {"a": 45.0, "b": 100.0, "c": 6.0, "d": 7.0, "f": 2.0})
    c.check("moved: rows both sides score, that moved, in key order",
            [(r["key"], r["delta"]) for r in rows], [("a", -5.0), ("c", 1.0)])
    c.check("direction counts up and down", report.direction(rows), {"moved": 2, "up": 1, "down": 1})
    c.check("drops are the rows that fell", [r["key"] for r in report.drops(rows)], ["a"])
    c.check("a move inside eps is no move", report.moved({"a": 1.0}, {"a": 1.0 + 1e-12}), [])

    def fn(name, addr, pct):
        return {"name": name, "size": "4", "fuzzy_match_percent": pct, "metadata": {"virtual_address": str(addr)}}
    b = report.address_rows({"units": [{"name": "main/x", "functions": [fn("fn_10", 16, 40.0), fn("g", 32, 9.0)]},
                                       {"name": "main/y", "functions": [fn("h", 48, 5.0)]}]})
    a = report.address_rows({"units": [{"name": "main/z", "functions": [fn("f", 16, 50.0), fn("k", 64, 1.0)]},
                                       {"name": "main/y", "functions": [fn("h", 48, 4.0)]}]})
    d = report.diff_by_address(b, a)
    c.check("diff_by_address: a renamed row moved to another unit pairs by its address",
            [(r["before_unit"], r["before_name"], r["unit"], r["name"], r["delta"]) for r in d["up"]],
            [("main/x", "fn_10", "main/z", "f", 10.0)])
    c.check("... a fall is down, an unpaired row new or removed",
            ([r["name"] for r in d["down"]], [r["name"] for r in d["new"]], [r["name"] for r in d["removed"]],
             d["paired"], len(d["renamed"])), (["h"], ["k"], ["g"], 2, 1))

    payload = report.compare(before, after, before_path="b.json", after_path="a.json")
    c.check("compare: every kind of drop is named",
            sorted((d["kind"], d["name"]) for d in payload["drops"]),
            [("denominator", "project/matched_data"), ("symbol", "main/a/fa"), ("symbol", "main/b/fc"),
             ("unit", "main/b")])
    c.check("... and exits 1, comparable over the common units",
            (payload["exit"], payload["regressed"], payload["comparable"]["common_units"]), (1, True, 2))
    c.check("compare: identical reports exit 0", report.compare(after, after)["exit"], 0)
    c.check("compare: nothing in common exits 2",
            report.compare(_rep({"x": (1.0, 1, {})}), _rep({"y": (1.0, 1, {})}))["exit"], 2)
    c.check("compare: a gain is not a drop", report.compare(_rep({"u": (1.0, 1, {"f": (4, 10.0)})}),
                                                            _rep({"u": (2.0, 1, {"f": (4, 20.0)})}))["exit"], 0)


def test_snapshot_and_regression(c):
    r1 = _rep({"main/u": (50.0, 300, {"f": (100, 100.0), "g": (100, 40.0), "h": (100, None)}),
               "main/auto_01_80000000_text": (0.0, 4, {"x": (4, 10.0)})})
    snap = report.snapshot(r1)
    c.check("the snapshot lists every symbol, an unscored one at 0.0, and marks the row",
            (snap["main/u"]["symbols"], snap["main/u"]["all"]), ({"f": 100.0, "g": 40.0, "h": 0.0}, True))
    c.check("an older report's match_percent is read",
            report.snapshot({"units": [{"name": "u", "functions": [{"name": "f", "match_percent": 5.0}]}]})["u"]["symbols"],
            {"f": 5.0})
    c.check("an empty report snapshots to nothing", report.snapshot(None), {})

    def snapped(fns, fuzzy=50.0, matched=100):
        return {"main/u": {"fuzzy": fuzzy, "matched_code": matched, "symbols": fns}}

    base = snapped({"g": 40.0, "h": 0.0})
    c.check("identical is no regression", report.regression(base, base), ([], []))
    c.check("a symbol that fell is a drop, named",
            report.regression(base, snapped({"g": 30.0, "h": 0.0}))[0], [("main/u", "g", 40.0, 30.0)])
    c.check("a symbol gone from `after` reached 100", report.regression(base, snapped({"h": 0.0}, fuzzy=60.0)), ([], []))
    c.check("a new weak symbol is growth, not a drop (even with the average down)",
            report.regression(base, snapped({"g": 40.0, "h": 0.0, "n": 1.0}, fuzzy=30.0)), ([], []))
    c.check("the average speaks when nothing grew and no symbol fell",
            report.regression(base, snapped({"g": 40.0, "h": 0.0}, fuzzy=45.0))[0], [("main/u", "unit fuzzy", 50.0, 45.0)])
    c.check("more matched bytes is growth", report.unit_grew(base["main/u"], snapped({"g": 40.0, "h": 0.0}, matched=200)["main/u"]), True)
    c.check("... read as numbers: the report stores matched_code as a string",
            report.unit_grew(snapped({"g": 40.0}, matched="100")["main/u"], snapped({"g": 40.0}, matched="200")["main/u"]),
            True)
    full = snapped({"f": 100.0, "g": 40.0, "h": 0.0})
    full["main/u"]["all"] = True
    c.check("a symbol at 100 % that fell is a drop (the full snapshot sees it)",
            report.regression(full, snapped({"f": 90.0, "g": 40.0, "h": 0.0}))[0], [("main/u", "f", 100.0, 90.0)])
    c.check("... and so is one that lost its score (100 -> unscored = 0.0)",
            report.regression(full, snapped({"f": 0.0, "g": 40.0, "h": 0.0}))[0], [("main/u", "f", 100.0, 0.0)])
    c.check("a rename of a 100 % symbol is not growth (unit_grew reads the sub-100 % set)",
            report.unit_grew(full["main/u"], snapped({"f2": 100.0, "g": 40.0, "h": 0.0})["main/u"]), False)
    c.check("a pre-WP4 base (sub-100 only) against a full snapshot: a symbol at 100 % after is no drop or growth",
            report.regression(base, snapped({"f": 100.0, "g": 40.0, "h": 0.0})), ([], []))
    c.check("an allowed unit's drop is authorised",
            report.regression(base, snapped({"g": 30.0, "h": 0.0}), ["main/u"]), ([], [("main/u", "g", 40.0, 30.0)]))
    auto = {"main/auto_01_80000000_text": {"fuzzy": 9.0, "matched_code": 1, "symbols": {"x": 10.0}}}
    c.check("an auto_* scaffold is bookkeeping",
            report.regression(auto, {"main/auto_01_80000000_text": {"fuzzy": 1.0, "matched_code": 0, "symbols": {"x": 1.0}}}),
            ([], []))


def _runner(seen: dict, functions: list[dict], rc: int = 0):
    def run(argv, **kwargs):
        seen["argv"], seen["kwargs"] = list(argv), kwargs
        if rc:
            return subprocess.CompletedProcess(argv, rc, "", "boom")
        if "report" in argv:
            proj = argv[argv.index("-p") + 1]
            seen["config"] = json.loads(Path(proj, "objdiff.json").read_text(encoding="utf-8"))
            Path(argv[argv.index("-o") + 1]).write_text(json.dumps(
                {"units": [{"name": seen["config"]["units"][0]["name"], "measures": {"total_code": "52"},
                            "functions": functions}]}), encoding="utf-8")
        else:
            Path(argv[argv.index("-o") + 1]).write_text(json.dumps(
                {"left": {"symbols": [{"name": "fn_1", "size": "40", "match_percent": 99.0}]},
                 "right": {"symbols": [{"name": "fn_1", "size": "44", "match_percent": 98.0}]}}), encoding="utf-8")
        return subprocess.CompletedProcess(argv, 0, "", "")
    return run


def test_scoring_wire(c):
    with tempfile.TemporaryDirectory() as tmp:
        target, base = os.path.join(tmp, "t.o"), os.path.join(tmp, "b.o")
        seen: dict = {}
        fns = [{"name": "fn_1", "size": "40", "fuzzy_match_percent": 100.0}, {"name": "fn_2", "size": "12"}]
        rep = report.score(target, base, "main/Lib/file", tmp, objdiff="objdiff-cli", runner=_runner(seen, fns))
        c.check("one `report generate` is the scoring path", seen["argv"][:3], ["objdiff-cli", "report", "generate"])
        unit = seen["config"]["units"][0]
        c.check("a one-unit project with absolute paths and the pinned version",
                (len(seen["config"]["units"]), unit["target_path"], unit["base_path"], seen["config"]["min_version"]),
                (1, os.path.abspath(target), os.path.abspath(base), report.MIN_PROJECT_VERSION))
        c.check("the report is left where callers read it", rep.path, os.path.join(tmp, report.REPORT_FILE))
        c.check("the first unit's functions", report.score_entries(target, base, "u", tmp, objdiff="o",
                                                                   runner=_runner(seen, fns))["fn_2"]["size"], "12")
        c.check("symbol_score is the report number", report.symbol_score(target, base, "fn_1", "u", tmp, objdiff="o",
                                                                         runner=_runner(seen, fns))["match_percent"], 100.0)
        c.check("an unscored symbol keeps None (not a made-up 0 or 100)",
                report.symbol_score(target, base, "fn_2", "u", tmp, objdiff="o", runner=_runner(seen, fns))["match_percent"],
                None)
        c.expect("an unknown symbol is an error, not 0.0",
                 "error" in report.symbol_score(target, base, "gone", "u", tmp, objdiff="o", runner=_runner(seen, fns)))
        c.raises("a failed run raises", report.ReportError, report.score, target, base, "u", tmp, objdiff="o",
                 runner=_runner(seen, fns, rc=1))
        c.contains("... and score_entries returns it as `_error`",
                   report.score_entries(target, base, "u", tmp, objdiff="o", runner=_runner(seen, fns, rc=1))["_error"],
                   "objdiff report generate failed")
        out = os.path.join(tmp, "pd", "diff.json")
        path, _log = report.project_diff(tmp, "main/Lib/file", "fn_1", out, "objdiff-cli", runner=_runner(seen, fns))
        c.check("project_diff: `diff -p . -u <unit> <symbol>` in the tree, rows with the report's relocation setting",
                (path, seen["argv"][:7], seen["kwargs"]["cwd"], "functionRelocDiffs=none" in seen["argv"]),
                (out, ["objdiff-cli", "diff", "-p", ".", "-u", "main/Lib/file", "fn_1"], tmp, True))
        c.check("... and a failed diff has no path", report.project_diff(tmp, "u", "fn_1", out, "o",
                                                                       runner=_runner(seen, fns, rc=1))[0], None)
        tries = {"n": 0}

        def flaky():
            tries["n"] += 1
            if tries["n"] < 3:
                raise PermissionError(5)
            return "ok"

        c.check("retry_transient rides out a transient lock", (report.retry_transient(flaky), tries["n"]), ("ok", 3))

        def locked():
            raise PermissionError(5)

        c.raises("... and gives up after its attempts", PermissionError, report.retry_transient, locked, 2)
        rows = report.diff_rows(target, base, "fn_1", "objdiff-cli", tmp, runner=_runner(seen, fns))
        c.expect("diff rows use the report's relocation setting", "functionRelocDiffs=none" in seen["argv"])
        c.check("diff rows expose the positional value as diff_match_percent, sizes per side",
                (rows["diff_match_percent"], rows["target_size"], rows["candidate_size"], rows["paired"]),
                (98.0, "40", "44", True))
        Path(tmp, "a", "build", "tools").mkdir(parents=True)
        Path(tmp, "b", "build", "tools").mkdir(parents=True)
        Path(tmp, "b", report.OBJDIFF_REL).write_bytes(b"")
        c.check("objdiff_cli falls back to MAIN's binary", report.objdiff_cli(Path(tmp, "a"), Path(tmp, "b")),
                os.path.join(str(Path(tmp, "b")), report.OBJDIFF_REL))
        c.check("... and names the tree's own path when neither has it", report.objdiff_cli(Path(tmp, "a")),
                os.path.join(str(Path(tmp, "a")), report.OBJDIFF_REL))


def _touch(path: Path, t: float, text: str = "") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    os.utime(path, (t, t))


def test_freshness(c):
    with tempfile.TemporaryDirectory() as tmp:
        root, base = Path(tmp), 1_700_000_000.0
        src = root / "src" / "Dir" / "u.cpp"
        _touch(root / "include" / "deep.h", base + 10)
        _touch(root / "include" / "Dir" / "u.h", base + 10, '#include "deep.h"\n')
        _touch(src, base + 10, '#include "Dir/u.h"\n#include <string.h>\n// #include "gone.h"\n')
        closure = {os.path.relpath(p, root).replace("\\", "/") for p in report.source_closure(str(src), str(root))}
        c.check("the closure follows include/ transitively, never a system or commented header",
                closure, {"src/Dir/u.cpp", "include/Dir/u.h", "include/deep.h"})
        order_root = root / "order"
        _touch(order_root / "include" / "a.h", base, '#include "deep.h"\n')
        _touch(order_root / "include" / "deep.h", base)
        _touch(order_root / "include" / "z.h", base, '/*\n#include "gone.h"\n*/\n')
        _touch(order_root / "include" / "gone.h", base)   # it exists: only the comment keeps it out
        _touch(order_root / "src" / "o.cpp", base, '#include "a.h"\n#include "z.h"\n#include "a.h"\n')
        c.check("the closure is in the preprocessor's order: depth first, each file once, a block-commented include none",
                [os.path.relpath(p, order_root).replace("\\", "/")
                 for p in report.source_closure(str(order_root / "src" / "o.cpp"), str(order_root))],
                ["src/o.cpp", "include/a.h", "include/deep.h", "include/z.h"])
        obj, rep = root / "build" / "u.o", root / "build" / "report.json"
        _touch(obj, base + 10)
        c.check("an equal stamp is current (strict <)", report.unit_reasons(str(src), str(obj), str(root))[0], [])
        _touch(root / "include" / "deep.h", base + 20)
        reasons, newest = report.unit_reasons(str(src), str(obj), str(root))
        c.check("a header edit dates the object, naming the header",
                (len(reasons), os.path.basename(newest[0])), (1, "deep.h"))
        _touch(rep, base + 15)
        c.check("report mode: older than the newest source is one reason",
                len(report.report_reasons(str(rep), str(obj), str(src), str(root))[0]), 1)
        _touch(rep, base + 5)
        c.check("... older than the object too is two",
                len(report.Freshness.report_reasons(str(rep), str(obj), str(src), str(root))[0]), 2)
        c.check("a missing object is a reason", len(report.freshness(False, None, None, ("s", 1.0), "r", "o")), 1)
        c.check("rel_path is tree-relative with forward slashes", report.rel_path(str(root / "a" / "b"), str(root)), "a/b")


def test_freshness_modes(c):
    """`freshness` in both modes and `newest`/`mtime`, on stamps alone (no files needed but one)."""
    base = 1_000_000.0
    src = ("u.cpp", base + 20)
    c.check("mtime of a missing file is None", report.mtime(os.path.join(tempfile.gettempdir(), "nope-xyz-wp6")), None)
    c.check("newest of nothing is None", report.newest([]), None)
    with tempfile.TemporaryDirectory() as tmp:
        a, b = Path(tmp) / "a.h", Path(tmp) / "b.h"
        _touch(a, base)
        _touch(b, base + 10)
        c.check("newest finds the most recent input and its stamp",
                (os.path.basename(report.newest([str(a), str(b)])[0]), report.newest([str(a), str(b)])[1]),
                ("b.h", base + 10))
    c.check("report mode: a report newer than the object and every source is current",
            report.freshness(True, base + 100, base + 5, src, "r.json", "o.o"), [])
    c.check("report mode: older than the newest source is stale",
            len(report.freshness(True, base + 15, base + 5, src, "r.json", "o.o")), 1)
    c.check("report mode: older than the object is stale", len(report.freshness(True, base + 3, base + 5, None, "r", "o")), 1)
    c.check("report mode: a missing report is stale", len(report.freshness(True, None, base + 5, None, "r", "o")), 1)
    c.check("object mode: an object newer than every source is current",
            report.freshness(False, None, base + 50, src, "r", "o"), [])
    c.check("object mode: a source newer than the object is stale",
            len(report.freshness(False, None, base + 10, src, "r", "o")), 1)
    c.check("object mode: an equal whole-second stamp is current", report.freshness(False, None, base + 20, src, "r", "o"),
            [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
