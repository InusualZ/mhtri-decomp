"""The landing log hook: every `land` / `land --branch` attempt appends one `.pi/land-log.jsonl` line - landed with
its commit, refused with the guard or gate row that refused, a conflict with its paths, an exception as `error`."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os
import unittest.mock as mock

from tools.lib import testing
from tools.lib.lanes import landlog
from tools.units.landing import api as L

TIER = "fixture"


def fixture() -> testing.GitFixture:
    fx = testing.GitFixture().init()
    fx.commit({"src/batch.c": "base\n", ".gitignore": ".pi/\n"}, "base")
    (fx.root / "src" / "batch.c").write_text("the batch\n", encoding="utf-8")
    return fx


def gate(code=0, failed=()):
    """A `verify` stand-in: exit `code`, the failed rows `failed` in its `problems` out-parameter."""
    def fake_verify(main, units, base, dry_run, no_build, allow_regression=None, check_outbox=True,
                    release_claims=True, problems=None, branch=None, no_selftests=False):
        if problems is not None:
            problems.extend(failed)
        L.write_land_message(main, "land: batch\n\nledger: (fixture)\n")
        return code
    return fake_verify


def run_land(fx, verify, **kw):
    with mock.patch.object(L.gate, "verify", verify), contextlib.redirect_stdout(io.StringIO()), \
            contextlib.redirect_stderr(io.StringIO()):
        return L.land(str(fx.root), ["Pl/pl_act"], None, no_build=True, check_outbox=False, release_claims=False,
                      subject="x", **kw)


def test_landed_and_refused(c):
    fx = fixture()
    code = run_land(fx, gate(1, ["style lint (§6.5) adds no violation [GATE]: +1 rule 2 (remedy: x)"]))
    rows, bad = landlog.read(str(fx.root))
    c.check("a refused landing exits 1 and logs one line", (code, len(rows), bad), (1, 1, []))
    c.check("... refused, naming the gate row and the units",
            (rows[0]["outcome"], rows[0]["refused_row"], rows[0]["units"], rows[0]["commit"]),
            ("refused", "style lint (§6.5) adds no violation", ["Pl/pl_act"], None))
    code = run_land(fx, gate(0))
    rows, _ = landlog.read(str(fx.root))
    c.check("a landing logs a second line: landed, with the commit it made",
            (code, len(rows), rows[1]["outcome"], rows[1]["commit"], rows[1]["refused_row"]),
            (0, 2, "landed", fx.git("rev-parse", "--short", "HEAD").strip(), None))
    c.check("... and its wall time", isinstance(rows[1]["seconds"], (int, float)), True)


def test_guard_refusal_and_error(c):
    fx = fixture()
    with contextlib.redirect_stdout(io.StringIO()):
        code = L.land(str(fx.root), ["Pl/pl_act"], None, no_build=True, subject="  ")
    rows, _ = landlog.read(str(fx.root))
    c.check("a guard refusal before the gate is logged with the guard's name",
            (code, rows[-1]["outcome"], rows[-1]["refused_row"]), (1, "refused", "--message"))

    def boom(*a, **k):
        raise RuntimeError("the gate crashed")

    try:
        run_land(fx, boom)
    except RuntimeError:
        pass
    rows, _ = landlog.read(str(fx.root))
    c.check("an exception is logged as an error and still raised", rows[-1]["outcome"], "error")


def test_branch_conflict(c):
    fx = testing.GitFixture().init()
    fx.commit({"include/shared.h": "int shared = 0;\n", ".gitignore": ".pi/\n"}, "base")
    fx.branch("worker/x", checkout=True)
    fx.commit({"include/shared.h": "int shared = 1;\n"}, "branch")
    fx.checkout("main")
    fx.commit({"include/shared.h": "int shared = 2;\n"}, "main")
    with mock.patch.object(L.gate, "verify", gate(0)), contextlib.redirect_stdout(io.StringIO()), \
            contextlib.redirect_stderr(io.StringIO()):
        code = L.land_branch(str(fx.root), "worker/x", units=["shared"], no_build=True, check_outbox=False,
                             release_claims=False)
    rows, _ = landlog.read(str(fx.root))
    c.check("a branch whose apply conflicts outside the union scope logs a conflict with its paths",
            (code, len(rows), rows[-1]["outcome"], rows[-1]["conflicts"], rows[-1]["branch"],
             rows[-1]["refused_row"]),
            (1, 1, "conflict", ["include/shared.h"], "worker/x", "apply"))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
