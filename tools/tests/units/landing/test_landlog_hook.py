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
                    release_claims=True, problems=None, branch=None, no_selftests=False, warnings=None):
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


ALL_ALLOW = {"regression": ["Net/a", "Net/b"], "rule10": ["run:.data:805FB0F8", "ref:src/x.cpp:3:T"],
             "rule12": ["DWCi_natProbeStatus"], "orphan": ["0x8079B83C"], "no_outbox": True, "no_selftests": True,
             "unit_renames": ["Net/old=Net/new"]}


def body_gate():
    """A `verify` stand-in that writes the REAL commit body (`gate.message_body`) from the arguments it was given."""
    def fake_verify(main, units, base, dry_run, no_build, allow_regression=None, check_outbox=True,
                    release_claims=True, problems=None, branch=None, no_selftests=False, warnings=None):
        b = L.Batch(main=main, units=units, unit_units=units, base="0" * 40, recorded={}, branch=branch,
                    check_outbox=check_outbox, no_selftests=no_selftests, allow_regression=list(allow_regression or []))
        b.subject = "land: batch"
        L.write_land_message(main, L.message_body(b, {}, {}, True))
        return 0
    return fake_verify


def set_all_allowances():
    L.set_allow_rule10(ALL_ALLOW["rule10"] + [" "])
    L.set_allow_rule12(ALL_ALLOW["rule12"])
    L.set_allow_orphan(ALL_ALLOW["orphan"])
    L.set_unit_renames(ALL_ALLOW["unit_renames"])


def clear_allowances():
    L.set_allow_rule10([])
    L.set_allow_rule12([])
    L.set_allow_orphan([])
    L.set_unit_renames([])


def test_every_allowance_is_recorded(c):
    fx = fixture()
    set_all_allowances()
    try:
        with mock.patch.object(L.gate, "verify", body_gate()), contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()):
            code = L.land(str(fx.root), ["Pl/pl_act"], None, no_build=True, check_outbox=False, release_claims=False,
                          subject="x", allow_regression=["Net/a", "Net/b", "Net/a"], no_selftests=True)
    finally:
        clear_allowances()
    rows, _ = landlog.read(str(fx.root))
    c.check("a landing with every allowance lands", (code, rows[-1]["outcome"]), (0, "landed"))
    c.check("... its log line is schema 2 and carries every allowance class, duplicates dropped",
            (rows[-1]["schema"], rows[-1]["allow"]), (2, ALL_ALLOW))
    body = fx.git("log", "-1", "--format=%B")
    allow_lines = [line for line in body.splitlines() if line.startswith("allow: ")]
    c.check("... and its commit body has one `allow:` line per class, in the class order",
            allow_lines, ["allow: regression Net/a, Net/b", "allow: rule10 run:.data:805FB0F8, ref:src/x.cpp:3:T",
                          "allow: rule12 DWCi_natProbeStatus", "allow: orphan 0x8079B83C", "allow: no_outbox",
                          "allow: no_selftests", "allow: unit_renames Net/old=Net/new"])
    c.check("... the old `authorised regressions` suffix is gone (one record, not two)",
            "authorised regressions" in body, False)
    code = run_land(fx, gate(1, ["rule 10 (vtable ownership) adds no violation [GATE]: 1 added (remedy: x)"]))
    rows, _ = landlog.read(str(fx.root))
    c.check("a refused landing records its allowance too (here only `--no-outbox`)", (code, rows[-1]["allow"]), (1, {"no_outbox": True}))


def test_manifest_reaches_the_log_and_the_body(c):
    """`--manifest` (2026-10-06): the landing log line carries the manifest id and the commit body a `manifest:` line;
    a landing without one carries neither."""
    fx = fixture()
    L.set_manifest("lane-net")
    try:
        with mock.patch.object(L.gate, "verify", body_gate()), contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()):
            code = L.land(str(fx.root), ["Pl/pl_act"], None, no_build=True, check_outbox=False, release_claims=False,
                          subject="x")
    finally:
        L.set_manifest(None)
    rows, _ = landlog.read(str(fx.root))
    body = fx.git("log", "-1", "--format=%B")
    c.check("a landing judged against a manifest logs its id and writes it in the body",
            (code, rows[-1].get("manifest"), "manifest: lane-net" in body.splitlines()), (0, "lane-net", True))
    (fx.root / "src" / "batch.c").write_text("the second batch\n", encoding="utf-8")
    run_land(fx, body_gate())
    rows, _ = landlog.read(str(fx.root))
    c.check("... and a landing without one carries no `manifest` key", "manifest" in rows[-1], False)


def test_warnings_reach_the_log_and_the_body(c):
    fx = fixture()

    def warning_gate(main, units, base, dry_run, no_build, allow_regression=None, check_outbox=True,
                     release_claims=True, problems=None, branch=None, no_selftests=False, warnings=None):
        b = L.Batch(main=main, units=units, unit_units=units, base="0" * 40, recorded={})
        with contextlib.redirect_stdout(io.StringIO()):
            b.warn("every unit's outbox validates", ["Pl/pl_act: no outbox at x"])
        b.subject = "land: batch"
        L.write_land_message(main, L.message_body(b, {}, {}, True))
        if warnings is not None:
            warnings.extend(b.warnings)
        return 0
    with mock.patch.object(L.gate, "verify", warning_gate), contextlib.redirect_stdout(io.StringIO()), \
            contextlib.redirect_stderr(io.StringIO()):
        code = L.land(str(fx.root), ["Pl/pl_act"], None, no_build=True, check_outbox=True, release_claims=False,
                      subject="x")
    rows, _ = landlog.read(str(fx.root))
    c.check("a landing with a warning lands", (code, rows[-1]["outcome"]), (0, "landed"))
    c.check("... its log line carries the warning", rows[-1]["warnings"],
            ["every unit's outbox validates: Pl/pl_act: no outbox at x"])
    c.check("... and its commit body one `warning:` line",
            [l for l in fx.git("log", "-1", "--format=%B").splitlines() if l.startswith("warning: ")],
            ["warning: every unit's outbox validates: Pl/pl_act: no outbox at x"])
    c.check("... and the summary counts it by row", landlog.summary(rows)["warning_rows"],
            [("every unit's outbox validates", 1)])


def test_schema_1_lines_still_read(c):
    fx = fixture()
    path = landlog.log_path(str(fx.root))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write('{"at":"2026-10-04T12:00:00","branch":"worker/a","commit":"abc","conflicts":[],"outcome":"landed",'
                 '"refused_row":null,"schema":1,"seconds":10.0,"units":["X"]}\n')
    landlog.append(str(fx.root), landlog.Attempt("worker/b", "landed", 5.0, allow={"rule10": ["run:.data:1", "r:2"]}))
    rows, bad = landlog.read(str(fx.root))
    s = landlog.summary(rows)
    c.check("a schema-1 and a schema-2 line read side by side", (len(rows), bad, s["schemas"]), (2, [], {1: 1, 2: 1}))
    c.check("... the summary counts the allowances (attempts and entries)", s["allowances"],
            {"rule10": {"attempts": 1, "entries": 2}})
    try:
        landlog.Attempt("worker/c", "landed", 1.0, allow={"rule7": ["x"]})
        c.check("an unknown allowance class is refused", "accepted", "refused")
    except ValueError:
        c.check("an unknown allowance class is refused", "refused", "refused")


def test_land_has_one_rule10_path(c):
    import inspect
    c.check("`flow.land` takes no `allow_rule10` (the CLI sets `state` - one path)",
            "allow_rule10" in inspect.signature(L.land).parameters, False)


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
