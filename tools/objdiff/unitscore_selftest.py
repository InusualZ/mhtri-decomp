#!/usr/bin/env python3
"""Self-test for `tools/objdiff/unitscore.py` - one report read, and the freshness guard that refuses it.

    python tools/objdiff/unitscore_selftest.py
    python tools/objdiff/unitscore.py --selftest        (the same checks)

**Fixtures only.** The tree is a fake repository in the system temp (`configure.py`, `src/demo/unit.cpp`
with a header closure, `build/RMHE08/obj/` so `lib.units.versions()` finds the version, and a hand-written
`report.json`), and every mtime is set with `os.utime`, so the check count and the outcome are identical in
MAIN, in a fresh worktree and in a slot. The fake tree is created outside the repository on purpose: a temp
directory inside it is inside a git worktree, and `lib.repo.repo_root()` would then resolve the real tree
and read the real report instead of the fixture.

The fixture is deliberately **not** a git worktree, and does not need to be. `lib.repo.repo_root()` roots a
run at the *invocation's* tree - its git worktree when there is one, else the `cwd` when the `cwd` is a tree
at all - so a run with `cwd=<fixture>` resolves the fixture even though `git rev-parse` answers nothing. It
used to fall back to the tool's own directory and silently score the real build; the fix is
`lib.repo.repo_root(start=)` / `Unit.resolve(spec, root)` and the `cwd`-that-is-a-tree rule.

The checks that matter are the two incidents the item exists for: a report older than the unit's source must
be **refused** (exit 1, no numbers), and a report older than the unit's **object** must be refused too.
`--force-stale` has to override the refusal while keeping the verdict visible, report mode must issue
**zero** objdiff invocations, and `--measure` exactly one.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)
TOOL = os.path.join(HERE, "unitscore.py")

DAY = 86400.0
ENV = dict(os.environ, PYTHONIOENCODING="utf-8")


def _load():
    """Import the tool by path. It is registered in `sys.modules` first: `@dataclass` looks its own
    module up by name, and a module created but never registered fails inside the decorator."""
    spec = importlib.util.spec_from_file_location("unitscore_under_test", TOOL)
    mod = importlib.util.module_from_spec(spec)
    sys.modules["unitscore_under_test"] = mod
    spec.loader.exec_module(mod)
    return mod


class Fixture:
    """A fake repository: one unit whose report, object and sources have mtimes the test controls."""

    def __init__(self):
        self.dir = tempfile.mkdtemp(prefix="unitscore-fixture-")
        self.build = os.path.join(self.dir, "build", "RMHE08")
        os.makedirs(os.path.join(self.build, "obj", "demo"))
        os.makedirs(os.path.join(self.build, "src", "demo"))
        os.makedirs(os.path.join(self.dir, "src", "demo"))
        os.makedirs(os.path.join(self.dir, "config", "RMHE08"))
        open(os.path.join(self.dir, "configure.py"), "w").write("config.libs = []\n")
        self.src = os.path.join(self.dir, "src", "demo", "unit.cpp")
        open(self.src, "w").write('#include "demo/unit.h"\n#include <string.h>\nint f(void);\n')
        self.header = os.path.join(self.dir, "src", "demo", "unit.h")
        open(self.header, "w").write('#include "types.h"\nvoid g(void);\n')
        self.types = os.path.join(self.dir, "src", "types.h")
        open(self.types, "w").write("typedef int s32;\n")
        self.obj = os.path.join(self.build, "src", "demo", "unit.o")
        self.target = os.path.join(self.build, "obj", "demo", "unit.o")
        open(self.obj, "wb").write(b"\x7fELF")
        open(self.target, "wb").write(b"\x7fELF")
        self.report = os.path.join(self.build, "report.json")
        open(os.path.join(self.dir, "config", "RMHE08", "splits.txt"), "w").write(
            "demo/unit.cpp:\n\t.text       start:0x80001000 end:0x80001100\n")
        self.now = time.time() - 10 * DAY
        self.write_report()
        self.set_times(report=0, obj=-DAY, source=-2 * DAY)

    # -- the fixture's own knobs -------------------------------------------------------------------
    def write_report(self, unit_name="main/demo/unit", measures=None, functions=None):
        """The report the fixture's tree carries: four rows, one of them unscored on purpose."""
        entry = {
            "name": unit_name,
            "measures": measures or {
                "fuzzy_match_percent": 25.0, "total_code": "256", "matched_code": "64",
                "matched_code_percent": 25.0, "total_functions": 4, "matched_functions": 1,
            },
            "sections": [{"name": ".text", "size": "256"}],
            "functions": functions if functions is not None else [
                {"name": "alpha_hundred", "size": "64", "fuzzy_match_percent": 100.0,
                 "metadata": {"virtual_address": "2147487744"}},
                {"name": "beta_half", "size": "64", "fuzzy_match_percent": 50.0,
                 "metadata": {"virtual_address": "2147487808"}},
                {"name": "gamma_twenty", "size": "64", "fuzzy_match_percent": 20.0,
                 "metadata": {"virtual_address": "2147487872"}},
                {"name": "delta_unscored", "size": "64",
                 "metadata": {"virtual_address": "2147487936"}},
            ],
        }
        with open(self.report, "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "measures": {}, "units": [entry]}, fh, indent=2)

    def set_times(self, report=0.0, obj=0.0, source=0.0, header=None):
        """Age each fixture file by a day count, so every comparison has one obvious answer."""
        for path, delta in ((self.report, report), (self.obj, obj), (self.target, obj), (self.src, source),
                            (self.header, source if header is None else header), (self.types, source)):
            stamp = self.now + delta
            os.utime(path, (stamp, stamp))

    def spec(self, us):
        """A `Spec` pointing at the fixture, for the in-process checks (child processes resolve it)."""
        return us.Spec(unit="demo/unit", unit_name="main/demo/unit", obj=self.obj, target=self.target,
                       src=self.src, tree=self.dir, report=self.report, sources=[self.src])

    def run(self, *args):
        """The tool as a real subprocess, from inside the fake tree."""
        return subprocess.run([sys.executable, TOOL, *args], cwd=self.dir, capture_output=True, text=True,
                              encoding="utf-8", errors="replace", env=ENV)

    def cleanup(self):
        shutil.rmtree(self.dir, ignore_errors=True)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.cleanup()
        return False


def check(name, got, want, fails):
    if got == want:
        print("ok    " + name)
        # `fails`, not 0: returning 0 here reset the accumulator, so a single earlier failure was wiped by
        # the next passing check and the selftest printed "ok" and exited 0 with a real failure in its log.
        # (The same latent bug is in symdiff_selftest.py; this file needs the honest count for its new
        # `matched_code` check to be able to fail.)
        return fails
    print("FAIL  %s\n        got:  %r\n        want: %r" % (name, got, want))
    return fails + 1


def contains(name, text, needle, fails):
    if needle in text:
        print("ok    " + name)
        return fails
    print("FAIL  %s\n        missing %r in:\n%s" % (name, needle, text[-700:]))
    return fails + 1


def selftest() -> int:
    fails = 0
    us = _load()

    fx = Fixture()
    try:
        # 1. a fresh report: every row, worst first, exit 0, no objdiff involved
        p = fx.run("demo/unit")
        fails = check("a fresh report scores and exits 0", p.returncode, 0, fails)
        fails = contains("... naming the unit", p.stdout, "== demo/unit", fails)
        fails = contains("... with the freshness verdict", p.stdout, "freshness  current", fails)
        fails = contains("... the report metric in the summary", p.stdout, "report metric", fails)
        order = [p.stdout.index(n) for n in ("delta_unscored", "gamma_twenty", "beta_half",
                                            "alpha_hundred")]
        fails = check("... rows worst-first (an unscored row reads 0 %, not 100 %)",
                      order == sorted(order), True, fails)
        fails = contains("... the summary carries matched_functions", p.stdout, "matched_functions 1/4",
                         fails)
        fails = contains("... and the code ratio", p.stdout, "code 64/256 B", fails)
        fails = contains("... and the registered range", p.stdout, "range      .text 0x80001000-0x80001100",
                         fails)

        # 2. --threshold filters, and says how many it hid
        p = fx.run("demo/unit", "--threshold", "50")
        fails = check("--threshold exits 0", p.returncode, 0, fails)
        fails = check("... hiding the row at the threshold", "beta_half" in p.stdout, False, fails)
        fails = check("... keeping the two under it",
                      ("gamma_twenty" in p.stdout and "delta_unscored" in p.stdout), True, fails)
        fails = contains("... with a filter line", p.stdout, "2 of 4 row(s) below 50 %", fails)

        # 3. the incident itself: report older than the SOURCE is refused, with no numbers at all
        fx.set_times(report=-DAY, obj=-2 * DAY, source=0)
        p = fx.run("demo/unit")
        fails = check("a report older than the unit's source is refused (exit 1)", p.returncode, 1, fails)
        fails = contains("... loudly", p.stdout, "freshness  STALE", fails)
        fails = contains("... naming the source", p.stdout, "predates the newest source", fails)
        fails = check("... and printing no score", "alpha_hundred" in p.stdout, False, fails)
        fails = contains("... pointing at --measure", p.stdout, "--measure", fails)

        # 4. the same guard from the other side: report older than the OBJECT
        fx.set_times(report=-DAY, obj=0, source=-2 * DAY)
        p = fx.run("demo/unit")
        fails = check("a report older than the unit's object is refused too", p.returncode, 1, fails)
        fails = contains("... naming the object", p.stdout, "predates the unit's object", fails)

        # 5. a header edit counts as a source edit (the include closure reaches it)
        fx.set_times(report=0, obj=-DAY, source=-2 * DAY, header=DAY)
        p = fx.run("demo/unit")
        fails = check("a header newer than the report is stale", p.returncode, 1, fails)
        fails = contains("... naming the header", p.stdout, "unit.h", fails)

        # 6. --force-stale overrides, and the verdict stays visible
        fx.set_times(report=-DAY, obj=-2 * DAY, source=0)
        p = fx.run("demo/unit", "--force-stale")
        fails = check("--force-stale scores a stale report (exit 0)", p.returncode, 0, fails)
        fails = contains("... printing the rows", p.stdout, "alpha_hundred", fails)
        fails = contains("... and still saying STALE", p.stdout, "STALE (forced)", fails)

        # 7. --json: the record, with all three mtimes and the verdict
        p = fx.run("demo/unit", "--json", "--force-stale")
        rec = json.loads(p.stdout)
        fails = check("--json carries the report mtime",
                      isinstance(rec["report"]["mtime"], (int, float)), True, fails)
        fails = check("... the object mtime", isinstance(rec["object"]["mtime"], (int, float)), True,
                      fails)
        fails = check("... the newest source mtime",
                      isinstance(rec["sources"]["newest_mtime"], (int, float)), True, fails)
        fails = check("... the verdict, still stale", rec["freshness"]["stale"], True, fails)
        fails = check("... with a reason naming the source",
                      any("source" in r for r in rec["freshness"]["reasons"]), True, fails)
        fails = check("... and forced, not clean", (rec["forced"], rec["refused"]), (True, False), fails)
        fails = check("... the row shape", sorted(rec["rows"][0].keys()),
                      ["address", "match_percent", "matched", "name", "scored", "size"], fails)
        fails = check("... worst first in JSON too", [r["name"] for r in rec["rows"]],
                      ["delta_unscored", "gamma_twenty", "beta_half", "alpha_hundred"], fails)
        fails = check("... measures as ints", (rec["measures"]["matched_functions"],
                                               rec["measures"]["matched_code"],
                                               rec["measures"]["total_code"]), (1, 64, 256), fails)
        fails = check("... and the rows tile the registered range", rec["range"]["ok"], True, fails)
        fails = check("... with a summary line", rec["summary"].startswith("summary: "), True, fails)

        # 8. a refused --json run carries the reasons and no numbers
        fx.set_times(report=-DAY, obj=-2 * DAY, source=0)
        p = fx.run("demo/unit", "--json")
        rec = json.loads(p.stdout)
        fails = check("a refused --json run exits 1", p.returncode, 1, fails)
        fails = check("... says so", rec["refused"], True, fails)
        fails = check("... and carries no rows", rec["rows"], None, fails)
        fails = check("... with non-empty reasons", len(rec["freshness"]["reasons"]) > 0, True, fails)

        # 9. an input error is exit 2 and names the remedy - never a traceback
        p = fx.run("demo/unit", "--report", os.path.join(fx.dir, "nope.json"))
        fails = check("a missing report is exit 2", p.returncode, 2, fails)
        fails = contains("... naming the path", p.stdout + p.stderr, "no report at", fails)
        fails = contains("... and the remedy", p.stdout + p.stderr, "ninja build/RMHE08/report.json", fails)
        fails = check("... with no traceback", "Traceback" in (p.stdout + p.stderr), False, fails)
        fx.write_report(unit_name="main/demo/other")
        p = fx.run("demo/unit", "--json")
        fails = check("a unit absent from the report is exit 2", p.returncode, 2, fails)
        fails = contains("... saying the report predates the registration",
                         json.loads(p.stdout)["error"], "predates the unit's registration", fails)

        # 10. --measure: exactly ONE objdiff invocation, and the report is not read at all
        fx.write_report()
        fx.set_times(report=-2 * DAY, obj=0, source=-DAY)
        calls = []
        real = us._report.score

        def fake(target, base, unit_name=None, tmpdir=None, **kw):
            calls.append((target, base, unit_name))
            return us._report.Report({"units": [{"name": unit_name, "measures": {}, "functions": [
                {"name": "solo", "size": "16", "fuzzy_match_percent": 75.0}]}]})

        try:
            us._report.score = fake
            rec = us.run(fx.spec(us), measure=True)
        finally:
            us._report.score = real
        fails = check("--measure issues exactly one objdiff invocation", len(calls), 1, fails)
        fails = check("... over the unit's own object pair", calls[0][:2] if calls else None,
                      (fx.target, fx.obj), fails)
        fails = check("... reads no report", rec["report"]["used"], False, fails)
        fails = check("... and scores the objects' rows", [r["name"] for r in rec["rows"]], ["solo"],
                      fails)
        fails = check("... current when the object is newer than the source",
                      rec["freshness"]["stale"], False, fails)

        # 11. --measure refuses a stale OBJECT (the false-improvement incident, from the other side)
        fx.set_times(report=-2 * DAY, obj=-2 * DAY, source=0)
        calls.clear()
        try:
            us._report.score = fake
            rec = us.run(fx.spec(us), measure=True)
        finally:
            us._report.score = real
        fails = check("--measure refuses an object older than its source", rec["refused"], True, fails)
        fails = check("... saying why", any("not rebuilt" in r for r in rec["freshness"]["reasons"]),
                      True, fails)
        fails = check("... with no rows", rec["rows"], None, fails)
        rec = None
        try:
            us._report.score = fake
            rec = us.run(fx.spec(us), measure=True, force=True)
        finally:
            us._report.score = real
        fails = check("... unless forced", (rec["refused"], len(rec["rows"] or [])), (False, 1), fails)
        fx.set_times(report=0, obj=-DAY, source=-2 * DAY)

        # 12. report mode makes ZERO objdiff invocations: the primitive is not even reachable
        code = ("import importlib.util,sys,json;"
                "spec=importlib.util.spec_from_file_location('u',%r);"
                "m=importlib.util.module_from_spec(spec);sys.modules['u']=m;spec.loader.exec_module(m);"
                "m._report.score=lambda *a,**k: (_ for _ in ()).throw(AssertionError('objdiff!'));"
                "m._report.objdiff_cli=lambda *a,**k: 'nope-not-a-binary';"
                "r=m.run(m.spec_of('demo/unit'));"
                "print(json.dumps({'rows': len(r['rows'] or []), 'stale': r['freshness']['stale']}))" % TOOL)
        child = subprocess.run([sys.executable, "-c", code], cwd=fx.dir, capture_output=True, text=True,
                               encoding="utf-8", errors="replace", env=ENV)
        fails = check("report mode scores a unit with zero objdiff invocations", child.returncode, 0,
                      fails)
        fails = check("... reading every row out of the report",
                      json.loads(child.stdout) if child.returncode == 0 else child.stderr[-300:],
                      {"rows": 4, "stale": False}, fails)

        # 13. the pure helpers, so a regression names itself instead of showing up as a wrong number
        rows = us.rows_of({"functions": [{"name": "b", "size": "8", "fuzzy_match_percent": 50.0},
                                        {"name": "a", "size": "8"},
                                        {"name": "c", "size": "8", "fuzzy_match_percent": 100.0}]})
        fails = check("rows_of reads an absent percent as 0 %", [r.name for r in rows], ["a", "b", "c"],
                      fails)
        fails = check("... and marks it unscored", [r.scored for r in rows], [False, True, True], fails)
        fails = check("below() filters strictly", [r.name for r in us.below(rows, 50.0)], ["a"], fails)
        fails = check("below(None) is everything", len(us.below(rows, None)), 3, fails)
        # a unit the report gives `total_code` but no `matched_code`: `measures_of` reads it as None (the
        # key is absent, exactly as `fuzzy_match_percent` is), and the summary must read the absent count
        # as 0 instead of multiplying `None` by 100.0. Measured on Network/NetworkSessionManagerPat (one
        # partial function at 93.14 %): `summary_line` raised `TypeError: unsupported operand type(s) for
        # *: 'float' and 'NoneType'` and the whole tool died.
        no_matched = us.measures_of({"measures": {"total_code": 52, "total_functions": 1,
                                                   "matched_functions": 0}})
        try:
            summary = us.summary_line(None, no_matched, rows, len(rows), None)
        except TypeError as exc:                       # the pre-fix shape, named instead of a traceback
            summary = "TypeError: %s" % exc
        fails = check("summary_line: a missing matched_code reads as 0, not a TypeError",
                      "code 0/52 B" in summary, True, fails)
        fails = check("freshness: an equal stamp is current",
                      us.freshness(True, 10.0, 10.0, ("s.c", 10.0), "r.json", "o.o"), [], fails)
        fails = check("freshness: one second older is stale",
                      len(us.freshness(True, 9.0, 10.0, None, "r.json", "o.o")), 1, fails)
        fails = check("freshness: a missing object is stale",
                      len(us.freshness(True, 10.0, None, None, "r.json", "o.o")), 1, fails)
        fails = check("freshness(measure): the report is not consulted",
                      us.freshness(False, 0.0, 10.0, ("s.c", 5.0), "r.json", "o.o"), [], fails)
        fails = check("... but the object is",
                      len(us.freshness(False, 0.0, 1.0, ("s.c", 5.0), "r.json", "o.o")), 1, fails)
        fails = check("split_claims reads the unit's range",
                      us.split_claims(fx.dir, "demo/unit"), {".text": (0x80001000, 0x80001100)}, fails)
        fails = check("split_claims does not leak a neighbour's range",
                      us.split_claims(fx.dir, "demo/other"), {}, fails)
        closure = [os.path.basename(p) for p in us.source_closure(fx.src, fx.dir)]
        fails = check("the include closure is transitive and in-tree", sorted(closure),
                      ["types.h", "unit.cpp", "unit.h"], fails)
        fails = check("... and a system header is not a dependency", "string.h" in closure, False, fails)
        fails = check("a range that does not tile is a warning, not a refusal",
                      us.range_block({".text": (0x80001000, 0x80000800)}, rows)["ok"], False, fails)

        # 13b. --refresh: rebuild the report through ninja, then score it
        calls = []

        def rebuilds(argv, cwd=None, **_kw):
            calls.append((argv, cwd))
            stamp = time.time()
            os.utime(fx.report, (stamp, stamp))
            return subprocess.CompletedProcess(argv, 0, "", "")

        fx.set_times(report=-DAY, obj=-2 * DAY, source=0)
        fails = check("refresh: without it the stale report is refused",
                      us.run(fx.spec(us))["refused"], True, fails)
        rec = us.run(fx.spec(us), refresh_build=True, runner=rebuilds)
        fails = check("refresh: ninja builds the tree's report in the tree",
                      calls, [(["ninja", "build/RMHE08/report.json"], fx.dir)], fails)
        fails = check("refresh: ... and the rebuilt report is scored", (rec["refused"], rec["row_count"]),
                      (False, 4), fails)
        fails = check("refresh: --measure rebuilds only the unit's object",
                      us.refresh_target(fx.spec(us), measure=True), "build/RMHE08/src/demo/unit.o", fails)
        failed = us.run(fx.spec(us), refresh_build=True,
                        runner=lambda argv, **_kw: subprocess.CompletedProcess(argv, 1, "FAILED: x.o\n", ""))
        fails = check("refresh: a failed build is the record's error, with no rows",
                      (failed["error"] is not None and "FAILED: x.o" in failed["error"], failed["rows"]),
                      (True, None), fails)
        p = fx.run("demo/unit", "--refresh", "--report", fx.report)
        fails = check("refresh: --report cannot be rebuilt (usage error)", p.returncode, 2, fails)

        # 15. --baseline: every row against a claim-time report, PAIRED BY ADDRESS (the lanes' cmp.py)
        fx.write_report()
        fx.set_times(report=0, obj=-DAY, source=-2 * DAY)
        base = os.path.join(fx.dir, "before.json")

        def row(name, addr, pct=None):
            r = {"name": name, "size": "64", "metadata": {"virtual_address": str(addr)}}
            if pct is not None:
                r["fuzzy_match_percent"] = pct
            return r
        with open(base, "w", encoding="utf-8") as fh:
            json.dump({"units": [
                {"name": "main/demo/neighbour", "functions": [row("fn_80001000", 0x80001000, 100.0)]},
                {"name": "main/demo/unit", "functions": [
                    row("fn_80001040", 0x80001040, 40.0), row("gamma_twenty", 0x80001080, 30.0),
                    row("delta_unscored", 0x800010C0), row("old_gone", 0x80001100, 10.0)]}]}, fh)
        rec = us.run(fx.spec(us), baseline=base)
        d = rec["baseline"]
        fails = check("baseline: a renamed row pairs by address and its rise is UP",
                      [(r["name"], r["before_name"], r["before"], r["after"]) for r in d["up"]],
                      [("beta_half", "fn_80001040", 40.0, 50.0)], fails)
        fails = check("baseline: a fall is DOWN", [(r["name"], r["before"], r["after"]) for r in d["down"]],
                      [("gamma_twenty", 30.0, 20.0)], fails)
        fails = check("baseline: a row moved in from a neighbour pairs too (renamed, not new)",
                      ([r["name"] for r in d["new"]],
                       sorted((r["before_unit"], r["name"]) for r in d["renamed"])),
                      ([], [("main/demo/neighbour", "alpha_hundred"), ("main/demo/unit", "beta_half")]), fails)
        fails = check("baseline: a row the unit lost is REMOVED", [r["name"] for r in d["removed"]],
                      ["old_gone"], fails)
        p = fx.run("demo/unit", "--baseline", base)
        fails = check("baseline: a DOWN row exits 1", p.returncode, 1, fails)
        fails = contains("... and is printed with both scores", p.stdout, "DOWN", fails)
        p = fx.run("--baseline", base)
        fails = check("baseline with no unit: every unit of the report, exit 1 on the down row",
                      (p.returncode, "1 down" in p.stdout, "1 removed" in p.stdout), (1, True, True), fails)
        with open(base, "w", encoding="utf-8") as fh:
            json.dump({"units": [{"name": "main/demo/unit", "functions": [row("fn_80001080", 0x80001080, 10.0)]}]},
                      fh)
        p = fx.run("--baseline", base, "--json")
        got = json.loads(p.stdout) if p.returncode in (0, 1) else {}
        fails = check("baseline: no down row exits 0, the rest are new", (p.returncode, len(got.get("new", []))),
                      (0, 3), fails)
        p = fx.run("--baseline", os.path.join(fx.dir, "missing.json"))
        fails = check("baseline: an unreadable baseline is exit 2", p.returncode, 2, fails)
    finally:
        fx.cleanup()

    # 14. the CLI's own edges, with no fixture at all
    p = subprocess.run([sys.executable, TOOL], capture_output=True, text=True, encoding="utf-8",
                       errors="replace", cwd=ROOT, env=ENV)
    fails = check("no unit is a usage error (exit 2)", p.returncode, 2, fails)
    p = subprocess.run([sys.executable, TOOL, "no-such-unit-xyz"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace", cwd=ROOT, env=ENV)
    fails = check("an unknown unit is exit 2", p.returncode, 2, fails)
    fails = check("... without a traceback", "Traceback" in (p.stdout + p.stderr), False, fails)

    if fails:
        print("FAIL (%d)" % fails)
        return 1
    print("ok - 89 checks")
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
