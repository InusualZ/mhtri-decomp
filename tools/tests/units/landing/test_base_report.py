"""record-base rebuilds report.json before it snapshots it (a stale MAIN report is not the base's scores), and the
gate refuses (BOOKKEEPING) a base whose rebuild failed or changed the tree; the snapshot holds every symbol."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import subprocess
import unittest.mock as mock

from tools.lib import testing
from tools.units.landing import api as L

TIER = "fixture"
ROW = "the batch base's report.json was rebuilt at record-base"


def report(score):
    return {"units": [{"name": "main/u", "measures": {"fuzzy_match_percent": score, "matched_code": "10"},
                       "functions": [{"name": "f", "size": "16", "fuzzy_match_percent": score},
                                     {"name": "g", "size": "16", "fuzzy_match_percent": 100.0}]}]}


def fixture(stale_score=40.0):
    fx = testing.GitFixture().init()
    fx.commit({"src/u.c": "int f(void) { return 0; }\n", ".gitignore": ".pi/\nbuild/\nbuild.ninja\n"}, "base")
    os.makedirs(fx.root / "build" / "RMHE08")
    (fx.root / "build.ninja").write_text("", encoding="utf-8")
    (fx.root / "build" / "RMHE08" / "report.json").write_text(json.dumps(report(stale_score)), encoding="utf-8")
    return fx


def ninja(rc=0, fresh_score=90.0, dirty=None):
    """A `run` stand-in: `ninja build/RMHE08/report.json` writes the fresh report (or fails, or dirties the tree)."""
    real = L.base.run

    def fake(args, cwd):
        if args[:2] == ["ninja", "build/RMHE08/report.json"]:
            if rc == 0:
                with open(os.path.join(cwd, "build", "RMHE08", "report.json"), "w", encoding="utf-8") as fh:
                    json.dump(report(fresh_score), fh)
            if dirty:
                with open(os.path.join(cwd, dirty), "w", encoding="utf-8") as fh:
                    fh.write("touched by the build\n")
            return subprocess.CompletedProcess(args, rc, "", "FAILED: build/RMHE08/src/u.o\n" if rc else "")
        if args and args[0] == "ninja" or "ledger.py" in " ".join(map(str, args)):
            return subprocess.CompletedProcess(args, 1, "", "")
        return real(args, cwd)
    return fake


def record(fx, fake):
    with mock.patch.object(L.base, "run", fake), contextlib.redirect_stderr(io.StringIO()):
        return L.record_base(str(fx.root))


def dry_run_problems(fx):
    problems = []
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        L.verify(str(fx.root), ["CLAUDE.md"], None, dry_run=True, no_build=True, check_outbox=False, problems=problems)
    return [p.split(" [")[0] + " [" + p.split(" [")[1].split("]")[0] + "]" for p in problems]


def test_rebuilt_before_the_snapshot(c):
    fx = fixture(stale_score=40.0)
    data = record(fx, ninja(0, fresh_score=90.0))
    c.check("the base snapshot holds the REBUILT report's scores, not the stale file's",
            data["report"]["main/u"]["symbols"], {"f": 90.0, "g": 100.0})
    c.check("... and records the rebuild", data["report_build"]["returncode"], 0)
    c.check("a clean rebuild adds no row", [p for p in dry_run_problems(fx) if p.startswith(ROW)], [])


def test_failed_or_dirtying_rebuild_refuses(c):
    fx = fixture()
    data = record(fx, ninja(1))
    c.check("a failed rebuild is recorded with its reason",
            (data["report_build"]["returncode"], "FAILED" in data["report_build"]["detail"]), (1, True))
    c.check("... and the gate refuses as BOOKKEEPING", [p for p in dry_run_problems(fx) if p.startswith(ROW)],
            [ROW + " [BOOKKEEPING]"])
    fx2 = fixture()
    data = record(fx2, ninja(0, dirty="src/u.c"))
    c.check("a rebuild that changed the tree is named", data["report_build"].get("dirtied"), ["src/u.c"])
    c.check("... and refuses too", [p for p in dry_run_problems(fx2) if p.startswith(ROW)], [ROW + " [BOOKKEEPING]"])


def test_a_pre_wp4_base_has_no_row(c):
    fx = fixture()
    os.makedirs(fx.root / ".pi", exist_ok=True)
    (fx.root / ".pi" / "land-base.json").write_text(json.dumps({"base": fx.head(), "report": {}}), encoding="utf-8")
    c.check("a base with no `report_build` key (recorded by the old gate) adds no row",
            [p for p in dry_run_problems(fx) if p.startswith(ROW)], [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
