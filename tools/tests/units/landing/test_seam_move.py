"""The seam-move credit of the regression row: a `splits.txt` diff that moves functions between units may lower a
unit's average without any function getting worse, and is then no regression; every other shape still refuses."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import copy
import inspect
import io
import json
import unittest.mock as mock

from tools.lib import project as _project
from tools.lib import report, testing
from tools.lib.lanes import landlog
from tools.units.landing import api as L
from tools.units.landing.rows import regression as R

TIER = "fixture"

A, B, C = "main/u/a", "main/u/b", "main/u/c"
MOVE = [(A, B, 0x100, 0x200)]
TOUCHED = {A, B}


def unit(fns: dict, fuzzy: float, matched: int, total: int) -> dict:
    """A snapshot row: `fns` is `{name: (address, percent)}`."""
    return {"fuzzy": fuzzy, "matched_code": str(matched), "total_code": str(total), "all": True,
            "symbols": {n: p for n, (_a, p) in fns.items()}, "addrs": {n: a for n, (a, _p) in fns.items()}}


def before() -> dict:
    return {A: unit({"f1": (0x100, 100.0), "f2": (0x200, 100.0), "f3": (0x300, 0.0)}, 200 / 3, 200, 300),
            B: unit({"g1": (0x1000, 0.0)}, 0.0, 0, 100),
            C: unit({"h1": (0x2000, 50.0)}, 50.0, 50, 100)}


def after() -> dict:
    """f1 (100 %, inside the moved range) went from A to B: A's average fell 66.67 -> 50, nothing got worse."""
    return {A: unit({"f2": (0x200, 100.0), "f3": (0x300, 0.0)}, 50.0, 100, 200),
            B: unit({"f1": (0x100, 100.0), "g1": (0x1000, 0.0)}, 50.0, 100, 200),
            C: unit({"h1": (0x2000, 50.0)}, 50.0, 50, 100)}


def judged(b: dict, a: dict, moves=MOVE, touched=TOUCHED, fn=None):
    """-> (rest, seams, why) of `seam_exempt` over what `regression` refuses."""
    unauth, _ = report.regression(b, a, [])
    return (fn or report.seam_exempt)(b, a, moves, touched, unauth), unauth


def test_the_seam_move_passes(c):
    (rest, seams, why), unauth = judged(before(), after())
    c.check("the fixture really is refused today: A's average fell with no symbol worse",
            [(r[0], r[1]) for r in unauth], [(A, "unit fuzzy")])
    c.check("(a) a seam move that drops A's average is credited", (rest, why), ([], ""))
    c.check("... and names the move and its function count", seams, [{"from": A, "to": B, "functions": 1}])


def test_refusals(c):
    b, a = before(), after()
    a[B]["symbols"]["g1"], b[B]["symbols"]["g1"] = 5.0, 10.0
    (rest, seams, why), unauth = judged(b, a)
    c.check("(b) one function actually worse in B refuses, both rows kept", (rest == unauth, seams), (True, []))
    c.contains("... and says which", why, "got worse")
    (rest, seams, why), unauth = judged(before(), after(), moves=[], touched=set())
    c.check("(c) a unit regression with no splits diff refuses", (rest, seams), (unauth, []))
    (rest, seams, why), unauth = judged(before(), after(), moves=[(A, B, 0x500, 0x600)])
    c.check("(d) moved functions outside the diffed range refuse", (rest, seams), (unauth, []))
    c.contains("... naming the function", why, "0x100")
    (rest, seams, why), unauth = judged(before(), after(), moves=[(A, C, 0x100, 0x200)])
    c.check("a range moved to a different unit than the function went to refuses", (rest, seams), (unauth, []))
    a = after()
    a[B]["matched_code"] = "50"
    (rest, _s, why), unauth = judged(before(), a)
    c.check("(e) matched code of the touched units falling refuses", rest, unauth)
    c.contains("... saying so", why, "matched code")
    a = after()
    a[C].update(fuzzy=40.0)
    a[C]["symbols"]["h1"] = 50.0
    (rest, _s, why), unauth = judged(before(), a)
    c.check("(f) the whole-report fuzzy falling refuses", rest, unauth)
    c.contains("... saying so", why, "whole-report")
    a = after()
    del a[B]["symbols"]["f1"], a[B]["addrs"]["f1"]
    (rest, _s, why), unauth = judged(before(), a)
    c.check("a function that is in no unit afterwards refuses", rest, unauth)
    old = before()
    del old[A]["addrs"]
    (rest, _s, why), unauth = judged(old, after())
    c.check("(g) a base snapshot without addresses credits nothing", rest, unauth)
    c.contains("... and says to re-record", why, "re-record")
    a = after()
    a[A]["fuzzy"], a[A]["symbols"] = 60.0, {"f2": 100.0, "f3": 0.0}
    c.check("a unit that no move names keeps its average row",
            judged(before(), a, moves=[(C, B, 0x100, 0x200)], touched={C, B})[0][0], judged(before(), a)[1])
    a = after()
    a[A]["symbols"]["f2"] = 40.0
    (rest, _s, _why), _u = judged(before(), a)
    c.check("a per-symbol row is never lifted", [r[1] for r in rest], ["f2"])


def _mutated(c, label, old, new, scenario, expect_rest_empty=True):
    src = inspect.getsource(report.seam_exempt)
    c.check("mutation %s: the text to change is there" % label, src.count(old), 1)
    scope = dict(vars(report))
    exec(src.replace(old, new), scope)
    b, a, moves, touched = scenario()
    (rest, seams, _why), _unauth = judged(b, a, moves, touched, fn=scope["seam_exempt"])
    c.check("mutation %s: the loosened rule lets the bad batch through" % label,
            ([r for r in rest if r[0] == A], bool(seams)), ([], True))


def test_mutations(c):
    worse = lambda: (lambda b, a: (b, a, MOVE, TOUCHED))(*_b_worse())
    _mutated(c, "1 (function worse)", "if worse:", "if False:", worse)
    _mutated(c, "2 (inside the range)", "inside(a, m[2], m[3])", "True", lambda: (before(), after(), [(A, B, 0x500, 0x600)], TOUCHED))
    _mutated(c, "2 (the target unit)", "m[1] == u_new and ", "", lambda: (before(), after(), [(A, C, 0x100, 0x200)], TOUCHED))
    _mutated(c, "3 (matched code)", "if sums[1] < sums[0] - eps:", "if False:", lambda: (before(), _matched_down(), MOVE, TOUCHED))
    _mutated(c, "3 (whole fuzzy)", "if wb is not None and wa is not None and wa < wb - 1e-3:", "if False:",
             lambda: (before(), _whole_down(), MOVE, TOUCHED))


def _b_worse():
    b, a = before(), after()
    b[B]["symbols"]["g1"], a[B]["symbols"]["g1"] = 10.0, 5.0
    return b, a


def _matched_down():
    a = after()
    a[B]["matched_code"] = "50"
    return a


def _whole_down():
    a = after()
    a[C]["fuzzy"] = 40.0
    return a


def test_splits_moves(c):
    t = lambda *rows: "Sections:\n\t.text type:code\n\n" + "".join("%s:\n\t.text start:0x%X end:0x%X\n\n" % r for r in rows)
    sp = _project.Splits.parse
    before_t = t(("g/a.cpp", 0x100, 0x400), ("g/b.cpp", 0x400, 0x500))
    after_t = t(("g/a.cpp", 0x100, 0x200), ("g/b.cpp", 0x200, 0x500))
    c.check("the tail of A handed to B is one move", _project.splits.range_moves(sp(before_t), sp(after_t)),
            [("g/a.cpp", "g/b.cpp", 0x200, 0x400)])
    c.check("... touching both", _project.splits.touched_units(sp(before_t), sp(after_t)), {"g/a.cpp", "g/b.cpp"})
    c.check("a new claim over unowned bytes is no move",
            _project.splits.range_moves(sp(t(("g/a.cpp", 0x100, 0x200))), sp(t(("g/a.cpp", 0x100, 0x300)))), [])
    c.check("identical splits touch nothing", _project.splits.touched_units(sp(before_t), sp(before_t)), set())
    c.check("moves read under the report's unit names",
            R.seam_moves_of(before_t, after_t), ([("main/g/a", "main/g/b", 0x200, 0x400)], {"main/g/a", "main/g/b"}))


def _row(moves, b_report, a_report, allow=()):
    b = L.Batch(main=".", units=["u/a"], unit_units=["u/a"], base="x", recorded={"report": b_report},
                allow_regression=list(allow))
    with mock.patch.object(R, "report_snapshot", lambda main: a_report), \
            mock.patch.object(R, "batch_seam_moves", lambda batch: moves), contextlib.redirect_stdout(io.StringIO()):
        R.regression_check_rows(b)
    return b, next(r for r in b.checks if r.name == "no symbol or unit regressed")


def test_the_row_and_the_log(c):
    b, row = _row((MOVE, TOUCHED), before(), after())
    c.check("the row passes and says why in the log", (row.status, row.evidence),
            ("PASS", "seam move: %s -> %s, 1 functions, none worse" % (A, B)))
    c.check("the batch records the move for the landing log", b.seam_moves, ["%s -> %s (1 functions)" % (A, B)])
    b, row = _row(([], set()), before(), after())
    c.check("the same batch with no splits move refuses as today", (row.status, b.seam_moves), ("FAIL", []))
    ok = landlog.Attempt("worker/x", "landed", 1.0, seam_moves=tuple(b.seam_moves or ["u -> v (2 functions)"]))
    c.check("the land log carries `seam_moves`", json.loads(ok.to_json())["seam_moves"], ["u -> v (2 functions)"])
    c.check("... and omits the key when there is none", "seam_moves" in json.loads(landlog.Attempt("w/x", "landed", 1.0).to_json()), False)
    snap = report.snapshot({"units": [{"name": "main/u", "measures": {"fuzzy_match_percent": 1.0, "total_code": "4"},
                                        "functions": [{"name": "f", "size": "4",
                                                       "metadata": {"virtual_address": "2148025904"}}]}]})
    c.check("the snapshot records addresses and total code",
            (snap["main/u"]["addrs"], snap["main/u"]["total_code"]), ({"f": 2148025904}, "4"))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
