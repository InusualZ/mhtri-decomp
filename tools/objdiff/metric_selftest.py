#!/usr/bin/env python3
"""Self-test for the verification/flag tools' scoring: they must read the **report** metric.

    python tools/objdiff/metric_selftest.py

The contract this pins is the one three tools used to break:

* `objdiff-cli diff` defaults `functionRelocDiffs` to `data_value` while `report generate` defaults to
  `none`, so relocation-only differences were counted as mismatches in their rows (and in the score
  `diff` reports);
* even at the same setting the diff JSON's `match_percent` is a different normalisation from the
  report's `fuzzy_match_percent` (`RSOStaticLocateObject`: 99.38461 vs 99.64103; `pl_skill`
  `fn_80270018`: 99.88039 vs **100.0**).

The official number - the one `build/RMHE08/report.json`, `ledger.py`, `brief.py` and `land.py` read -
comes from `report generate`, which is what `lib.report.score_entries`/`symbol_score` run, and what
`symdiff` (with `-u`), `tryvar` and `mwcc_matrix` now print. Layout:

* **wire** checks with a stub objdiff runner: the exact objdiff invocations (`report generate` on a
  one-unit project with absolute paths; `functionRelocDiffs=none` on `diff`) and the parsing;
* **integration** checks against the real tools and objects whenever a build tree exists, skipped (not
  failed) otherwise: `lib.report.score_entries` == the project report, `symdiff` prints that number,
  `slotmap`'s TARGET/OURS columns come from the right side, `tryvar.match_pcts` and
  `mwcc_matrix.summarize` return it for real objects.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json
import os
import re
import subprocess
import sys
import tempfile

from tools.lib import repo as _repo
from tools.lib import report as _report
from tools.lib import units as _units

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OBJDIFF = os.path.join(ROOT, _report.OBJDIFF_REL)


def objdiff(unit, symbol, out, runner=subprocess.run):
    """`objdiff-cli diff` in project mode for one symbol of `unit` (`lib.report.project_diff`)."""
    return _report.project_diff(ROOT, unit.report_name, symbol, out, OBJDIFF, runner=runner)


def report_functions(target, base, unit_name=None, tmpdir=None, runner=subprocess.run):
    """The official per-function scores of one object pair (`lib.report.score_entries`)."""
    return _report.score_entries(target, base, unit_name, tmpdir or _repo.session_tmpdir(), objdiff=OBJDIFF,
                                 cwd=ROOT, runner=runner)


def report_measure(target, base, symbol, unit_name=None, tmpdir=None, runner=subprocess.run):
    """The official score of one symbol (`lib.report.symbol_score`)."""
    return _report.symbol_score(target, base, symbol, unit_name, tmpdir or _repo.session_tmpdir(), objdiff=OBJDIFF,
                                cwd=ROOT, runner=runner)

SYMDIFF = os.path.join(ROOT, "tools", "objdiff", "symdiff.py")
SLOTMAP = os.path.join(ROOT, "tools", "objdiff", "slotmap.py")


def _completed(argv):
    return subprocess.CompletedProcess(argv, 0, "", "")


def _ok(label, got, want, failures):
    if got == want:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures + 1


def _truthy(label, cond, failures):
    if cond:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}")
    return failures + 1


# --- wire checks (no build) ---------------------------------------------------------------------

def wire_objdiff() -> int:
    """`objdiff()` must classify its rows with the report's relocation default."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, "d.json")
        seen = {}

        def runner(argv, **kwargs):
            seen["argv"] = list(argv)
            json.dump({"left": {"symbols": []}, "right": {"symbols": []}},
                      open(out, "w", encoding="utf-8"))
            return _completed(argv)

        unit = _units.Unit("Lib/file", ".cpp", tmp)
        path, _log = objdiff(unit, "fn_1", out=out, runner=runner)
        failures = _ok("objdiff returns the json path", path, out, failures)
        argv = seen.get("argv") or []
        failures = _truthy("objdiff passes functionRelocDiffs=none",
                           "functionRelocDiffs=none" in argv, failures)
        failures = _truthy("objdiff runs project mode for the unit",
                           argv[:4] == [OBJDIFF, "diff", "-p", "."], failures)
    return failures


def wire_report() -> int:
    """The report-scoring primitive: one-unit project, absolute paths, official metric parsed."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "target.o")
        base = os.path.join(tmp, "base.o")
        for p in (target, base):
            open(p, "wb").write(b"\x7fELF")
        seen = {}

        def runner(argv, **kwargs):
            seen["argv"] = list(argv)
            proj = argv[argv.index("-p") + 1]
            cfg = json.load(open(os.path.join(proj, "objdiff.json"), encoding="utf-8"))
            seen["config"] = cfg
            out = argv[argv.index("-o") + 1]
            json.dump({"units": [{"name": cfg["units"][0]["name"], "functions": [
                {"name": "fn_1", "size": "40", "fuzzy_match_percent": 100.0},
                {"name": "fn_2", "size": "12", "fuzzy_match_percent": 42.5}]}]},
                open(out, "w", encoding="utf-8"))
            return _completed(argv)

        fns = report_functions(target, base, unit_name="main/Lib/file", tmpdir=tmp, runner=runner)
        failures = _in_order(fns, ["fn_1", "fn_2"], failures)
        failures = _ok("report metric parsed", fns["fn_2"]["fuzzy_match_percent"], 42.5, failures)
        failures = _ok("target size carried", fns["fn_1"]["size"], "40", failures)
        cfg = seen.get("config") or {}
        failures = _ok("one-unit project", len(cfg.get("units") or []), 1, failures)
        failures = _ok("project version", cfg.get("min_version"), _report.MIN_PROJECT_VERSION, failures)
        unit = (cfg.get("units") or [{}])[0]
        failures = _ok("target_path absolute", unit.get("target_path"), os.path.abspath(target), failures)
        failures = _ok("base_path absolute", unit.get("base_path"), os.path.abspath(base), failures)
        failures = _ok("unit name carried", unit.get("name"), "main/Lib/file", failures)
        argv = seen.get("argv") or []
        failures = _truthy("report generate is the scoring path",
                           argv[:3] == [OBJDIFF, "report", "generate"], failures)

        m = report_measure(target, base, "fn_1", unit_name="main/Lib/file", tmpdir=tmp, runner=runner)
        failures = _ok("report_measure official number", m.get("match_percent"), 100.0, failures)
        miss = report_measure(target, base, "gone", unit_name="main/Lib/file", tmpdir=tmp, runner=runner)
        failures = _truthy("unknown symbol is an error, not a 0.0 score", "error" in miss, failures)

        def broken(argv, **kwargs):
            return subprocess.CompletedProcess(argv, 1, "", "boom")

        err = report_functions(target, base, tmpdir=tmp, runner=broken)
        failures = _truthy("a failed report is an error, not an empty score",
                           "_error" in err, failures)
    return failures


def wire_tmpdir() -> int:
    """The default scratch must be per-invocation, not the shared `build/tmp/unitutil`.

    `tryvar`/`slotmap`/`recompile`'s single-symbol path call `report_functions`/`report_measure` with no
    `tmpdir`, so their project and `unitutil_report.json` all landed in one shared directory. Two
    concurrent invocations raced on that file - the loser read the other run's report (or raised
    `PermissionError [WinError 5]` while it was held), the same collision `symdiff.py` was fixed for with
    a per-invocation directory. This pins the fix at the function that owns the default.
    """
    failures = 0
    shared = os.path.join(ROOT, "build", "tmp", "unitutil")
    first = _repo.session_tmpdir()
    failures = _truthy("the default scratch exists", os.path.isdir(first), failures)
    failures = _truthy("it is not the shared build/tmp/unitutil",
                       os.path.normcase(os.path.abspath(first))
                       != os.path.normcase(os.path.abspath(shared)), failures)
    failures = _ok("one directory per process keeps report_measure's path correct",
                   _repo.session_tmpdir(), first, failures)

    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "target.o")
        base = os.path.join(tmp, "base.o")
        for p in (target, base):
            open(p, "wb").write(b"\x7fELF")

        def runner(argv, **kwargs):
            out = argv[argv.index("-o") + 1]
            json.dump({"units": [{"name": "u", "functions": [
                {"name": "fn_1", "size": "4", "fuzzy_match_percent": 100.0}]}]},
                open(out, "w", encoding="utf-8"))
            return _completed(argv)

        m = report_measure(target, base, "fn_1", unit_name="u", runner=runner)
        failures = _ok("report_measure official number with the default dir",
                       m.get("match_percent"), 100.0, failures)
        failures = _ok("and its report_json is in that dir",
                       os.path.normcase(os.path.abspath(os.path.dirname(m.get("report_json") or ""))),
                       os.path.normcase(os.path.abspath(first)), failures)

    return failures


def _in_order(mapping, names, failures):
    got = [n for n in mapping if n != "_error"]
    if got == names:
        print("ok    functions are keyed by name")
        return failures
    print(f"FAIL  functions are keyed by name\n        got:  {got!r}\n        want: {names!r}")
    return failures + 1


# --- integration checks (need a build tree) -----------------------------------------------------

def _have(path):
    return os.path.exists(path)


def _pick_unit():
    """A real unit with target+base objects and at least one scored function."""
    for spec in ("RSO/runtime", "Pl/pl_skill", "main/pl_act", "Camellia/camellia"):
        try:
            unit = _units.Unit.resolve(spec, ROOT)
        except SystemExit:
            continue
        if _have(unit.obj_target) and _have(unit.obj_ours):
            names = _units.function_names(unit.obj_target)
            if names:
                return unit
    for unit in _units.Unit.list(ROOT):
        if _have(unit.obj_target) and _have(unit.obj_ours) and _units.function_names(unit.obj_target):
            return unit
    return None


def _project_report():
    """The freshly generated whole-project report, or None."""
    out = os.path.join(ROOT, "build", "tmp", "metric_selftest_report.json")
    p = subprocess.run([OBJDIFF, "report", "generate", "-p", ROOT, "-o", out],
                       cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return None
    data = json.load(open(out, encoding="utf-8"))
    return {u.get("name"): {f.get("name"): f.get("fuzzy_match_percent")
                            for f in u.get("functions") or []}
            for u in data.get("units") or []}


def integration(unit, official_all) -> int:
    failures = 0
    official = official_all.get(unit.report_name)
    if not official:
        print("skip  integration (the project report has no functions for %s)" % unit.report_name)
        return failures

    # 1. the shared primitive equals the project report for every function of the unit
    fns = report_functions(unit.obj_target, unit.obj_ours, unit.report_name, os.path.join(ROOT, "build", "tmp"))
    if "_error" in fns:
        print("skip  integration (" + fns["_error"][:120] + ")")
        return failures
    mism = [(n, (fns.get(n) or {}).get("fuzzy_match_percent"), official[n])
            for n in official if isinstance(official[n], (int, float))
            and (fns.get(n) or {}).get("fuzzy_match_percent") != official[n]]
    failures = _truthy("report_functions == project report (%d functions of %s)" % (len(official), unit.report_name),
                       not mism, failures)
    if mism:
        print("        first mismatches: %r" % (mism[:3],))

    sym = next((n for n in official if isinstance(official[n], (int, float))), None)
    if sym is None:
        return failures

    # 2. symdiff prints the official number (and says so)
    p = subprocess.run([sys.executable, SYMDIFF, "-u", unit.report_name, sym, "1"],
                       cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    head = (p.stdout or "").splitlines()[0] if p.stdout else ""
    if p.returncode == 1 and "freshness  STALE" in (p.stderr or ""):
        # the object predates its sources here (a fresh worktree's checkout): symdiff refuses by design
        print("skip  symdiff header (the unit's object is older than its sources in this tree)")
    else:
        failures = _truthy("symdiff prints the report metric for %s" % sym,
                           ("match %s" % official[sym]) in head and "report metric" in head, failures)
        if not (("match %s" % official[sym]) in head and "report metric" in head):
            print("        header: %s" % head)

    # 3. slotmap labels the sides correctly (project mode: left = target)
    diff_json = os.path.join(ROOT, "build", "tmp", "metric_selftest_slotmap.json")
    path, log = objdiff(unit, sym, out=diff_json)
    if not path:
        print("skip  slotmap orientation (objdiff failed: %s)" % log[:120])
        return failures
    data = json.load(open(diff_json, encoding="utf-8"))
    left = _instr(data["left"].get("symbols") or [], sym)
    right = _instr(data["right"].get("symbols") or [], sym)
    if not left or not right:
        print("skip  slotmap orientation (no instruction rows)")
        return failures
    for index in (0, len(left) - 1):
        rows = _slotmap_rows(diff_json, sym, index)
        if index not in rows:
            failures = _truthy("slotmap --around prints row %d" % index, False, failures)
            continue
        tgt_col, ours_col = rows[index]
        failures = _ok("slotmap TARGET row %d is the target" % index, tgt_col, left[index].strip(), failures)
        failures = _ok("slotmap OURS row %d is our build" % index, ours_col, right[index].strip(), failures)
    # and the `-u` path (which runs objdiff itself) agrees with it
    rows = _slotmap_rows(None, sym, 0, unit=unit)
    if 0 in rows:
        failures = _ok("slotmap -u TARGET row 0 is the target", rows[0][0], left[0].strip(), failures)
    return failures


def _instr(symbols, name):
    for e in symbols:
        if e.get("name") == name:
            return [(i.get("instruction") or {}).get("formatted") for i in e.get("instructions") or []]
    return []


def _slotmap_rows(diff_json, symbol, index, unit=None):
    """{index: (target, ours)} from `slotmap --around index,index+1`, in either invocation mode."""
    cmd = [sys.executable, SLOTMAP]
    if unit is not None:
        cmd += ["-u", unit.report_name, symbol]
    else:
        cmd += [diff_json, symbol]
    cmd += ["--around", "%d,%d" % (index, index + 1)]
    p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    rows = {}
    for line in (p.stdout or "").splitlines():
        if "|" not in line:
            continue
        left, right = line.split("|", 1)
        parts = left.split()
        if parts and parts[0] in ("idx", "->"):
            parts = parts[1:]
        if not parts or not parts[0].isdigit():
            continue
        rows[int(parts[0])] = (" ".join(parts[1:]).strip(), right.strip())
    return rows


def integration_flag_tools(unit, official_all) -> int:
    """`tryvar.match_pcts` and `mwcc_matrix.summarize` must return the official per-function number."""
    failures = 0
    official = official_all.get(unit.report_name) or {}
    scored = {n: v for n, v in official.items() if isinstance(v, (int, float))}
    if not scored:
        print("skip  flag-tool integration (no scored functions for %s)" % unit.report_name)
        return failures

    from tools.flags import mwcc_matrix, tryvar
    from tools.lib import units as lib_units

    pcts = tryvar.match_pcts(unit.obj_ours, unit.obj_target)
    if not pcts:
        print("skip  tryvar integration (no report result)")
        return failures
    mism = [(n, pcts[n][0], round(scored[n], 2)) for n in scored
            if n in pcts and abs(pcts[n][0] - round(scored[n], 2)) > 1e-9]
    failures = _truthy("tryvar.match_pcts == project report (%d functions)" % len(pcts), not mism, failures)
    if mism:
        print("        first mismatches: %r" % (mism[:3],))

    path, err = mwcc_matrix.diff_unit(lib_units.Unit.resolve(unit.report_name, ROOT), "metric_selftest",
                                      _units.function_names(unit.obj_target)[0])
    if not path:
        print("skip  mwcc_matrix integration (%s)" % err[:120])
        return failures
    rows = mwcc_matrix.summarize(path, scored)
    mism = [(r[0], r[3], round(scored[r[0]], 2)) for r in rows
            if r[0] in scored and abs(r[3] - round(scored[r[0]], 2)) > 1e-9]
    failures = _truthy("mwcc_matrix.summarize == project report (%d rows)" % len(rows), not mism, failures)
    if mism:
        print("        first mismatches: %r" % (mism[:3],))
    return failures


def regression_metric_gap() -> int:
    """The documented bug, pinned: `RSOStaticLocateObject` is 99.38 positionally, 99.64 officially.

    Also pins the reloc-default half: `pl_skill`'s `fn_80270018` reads 99.88 % with `data_value` rows and
    100.0 % officially. Both are skipped when the unit is not built.
    """
    failures = 0
    try:
        rso = _units.Unit.resolve("RSO/runtime", ROOT)
    except SystemExit:
        rso = None
    if rso is not None and _have(rso.obj_target) and _have(rso.obj_ours):
        json_out = os.path.join(ROOT, "build", "tmp", "metric_selftest_pos.json")
        objdiff(rso, "RSOStaticLocateObject", out=json_out)
        data = json.load(open(json_out, encoding="utf-8"))
        pos = None
        for e in data["left"].get("symbols") or []:
            if e.get("name") == "RSOStaticLocateObject":
                pos = e.get("match_percent")
        m = report_measure(rso.obj_target, rso.obj_ours, "RSOStaticLocateObject", rso.report_name)
        failures = _truthy("normalisation gap is real (%s positional vs %s official)"
                           % (pos, m.get("match_percent")),
                           isinstance(pos, (int, float)) and m.get("match_percent") == 99.64103
                           and pos == 99.38461, failures)
    else:
        print("skip  regression pin (RSO/runtime is not built)")

    try:
        skill = _units.Unit.resolve("Pl/pl_skill", ROOT)
    except SystemExit:
        skill = None
    if skill is not None and _have(skill.obj_target) and _have(skill.obj_ours):
        m = report_measure(skill.obj_target, skill.obj_ours, "fn_80270018", skill.report_name)
        failures = _truthy("reloc-only residual is 100 %% officially (pl_skill fn_80270018 = %s)"
                           % m.get("match_percent"), m.get("match_percent") == 100.0, failures)
    else:
        print("skip  regression pin (Pl/pl_skill is not built)")
    return failures


def main() -> int:
    failures = wire_objdiff()
    failures += wire_report()
    failures += wire_tmpdir()

    unit = _pick_unit()
    official_all = _project_report()
    if unit is None or official_all is None:
        print("skip  integration checks (no build tree / no project report)")
    else:
        failures += integration(unit, official_all)
        failures += integration_flag_tools(unit, official_all)
    failures += regression_metric_gap()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
