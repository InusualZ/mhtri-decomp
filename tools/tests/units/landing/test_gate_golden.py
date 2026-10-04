"""The gate's table, pinned: on fixture batches (a no-op, a unit batch shaped like d97750f09, a two-unit batch with a
moved neighbour and a regression like 042956de2, a tools batch like a5d6f87fc, a lint refusal, an unrecorded base)
`verify` produces exactly the rows - names, statuses, kinds, order - and exit codes in `gate-golden.json`.

The golden was recorded from the monolithic `land.py` before the WP4 split (`capture_all(<old verify>)`); the
subprocesses and the heavy tool readers are stubbed at their module boundary, the git calls are real."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import subprocess
import sys
import tempfile
import unittest.mock as mock

import tools.git.prepcommit as prepcommit
import tools.units.claims as claims
import tools.units.dataclosure as dataclosure
import tools.units.undefrefs as undefrefs
import tools.units.verifyunit as verifyunit
import tools.units.vtableaudit as vtableaudit
from tools.lib import findings, proc, testing

TIER = "fixture"

UNIT = "Net/net_new"
BRANCH = claims.branch_for(UNIT)
SPLITS = ("Sections:\n\t.text       type:code align:32\n\n"
          "Net/net_old.cpp:\n\t.text       start:0x80001000 end:0x80001100\n")
SPLITS_NEW = SPLITS + "\nNet/net_new.cpp:\n\t.text       start:0x80001100 end:0x80001200\n"
CONF = 'config.libs = [\n    Object(NonMatching, "Net/net_old.cpp"),\n]\n'
CONF_NEW = 'config.libs = [\n    Object(NonMatching, "Net/net_old.cpp"),\n    Object(NonMatching, "Net/net_new.cpp"),\n]\n'
SYMBOLS = ("fn_net_old = .text:0x80001000; // type:function size:0x100\n"
           "fn_80001100 = .text:0x80001100; // type:function size:0x100\n")
SYMBOLS_NEW = SYMBOLS.replace("fn_80001100", "net_new_step")
OUTBOX = {"unit": UNIT, "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 50.0,
          "symbols": [{"name": "net_new_step", "percent": 50.0}], "residual": "none",
          "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}
LEDGER = {"totals": {"claimed_functions": 10, "closed": 4, "partial": 2, "unclaimed": 1, "matched_functions": 30,
                     "matched_code": 1000, "total_code": 9000, "fuzzy_match_percent": 50.0}}


def report(units: dict) -> dict:
    """An objdiff report: `{unit: {symbol: score or None}}` -> the JSON shape (`None` = unscored)."""
    out = []
    for name, syms in units.items():
        fns = [dict({"name": s, "size": "256"}, **({} if v is None else {"fuzzy_match_percent": v}))
               for s, v in syms.items()]
        scored = [v or 0.0 for v in syms.values()]
        out.append({"name": name, "measures": {"fuzzy_match_percent": sum(scored) / max(len(scored), 1),
                                                "matched_code": "512", "total_code": str(256 * len(syms))},
                    "functions": fns})
    return {"units": out}


class Scenario:
    def __init__(self, name, kind, units, dry_run=False, check_outbox=True, record_base=True, lint=(0, ""),
                 after=None, drift=False, allow_regression=(), no_selftests=False, post_fail=False):
        self.name, self.kind, self.units, self.dry_run = name, kind, list(units), dry_run
        self.check_outbox, self.record_base, self.lint, self.drift = check_outbox, record_base, lint, drift
        self.after = after or {"main/Net/net_old": {"fn_net_old": 90.0}, "main/Net/net_new": {"net_new_step": 50.0}}
        self.allow_regression, self.no_selftests = list(allow_regression), no_selftests
        # every object row's reader refuses: registration, references, rule 10, data closure, the re-measure
        self.post_fail = post_fail


LINT_REFUSAL = (1, json.dumps({"added": [{"rule": 2, "file": "src/Net/net_new.cpp", "added": 1, "before": 0,
                                          "after": 1}],
                               "detail": [{"rule": 2, "file": "src/Net/net_new.cpp", "line": 3, "token": "x"}]}))
SCENARIOS = [
    Scenario("noop-dry-run", "noop", ["CLAUDE.md"], dry_run=True, check_outbox=False),
    Scenario("noop", "noop", ["CLAUDE.md"], check_outbox=False),
    Scenario("unit-dry-run", "unit", [UNIT], dry_run=True),
    Scenario("unit", "unit", [UNIT]),
    Scenario("unit-no-selftests", "unit", [UNIT], no_selftests=True),
    Scenario("two-units-outbox-refusal", "unit", [UNIT, "Net/net_old"]),
    Scenario("two-units-neighbour-moved-regressed", "unit", [UNIT, "Net/net_old"], drift=True, check_outbox=False,
             after={"main/Net/net_old": {"fn_net_old": 70.0}, "main/Net/net_new": {"net_new_step": 50.0}},
             allow_regression=["Net/zzz"]),
    Scenario("tools", "tools", ["tools/units/fixture_tool.py"]),
    Scenario("lint-refusal", "unit", [UNIT], lint=LINT_REFUSAL),
    Scenario("no-base-dry-run", "unit", [UNIT], dry_run=True, record_base=False),
    Scenario("unit-object-rows-refuse", "unit", [UNIT], post_fail=True),
]


def git(root, *args):
    return testing.GitFixture(root).git(*args)


def build_fixture(root: str, sc: Scenario) -> None:
    fx = testing.GitFixture(root).init()
    files = {"configure.py": CONF, "config/RMHE08/splits.txt": SPLITS, "config/RMHE08/symbols.txt": SYMBOLS,
             "src/Net/net_old.cpp": "int net_old(void) { return 0; }\n",
             "include/Net/net.h": "#ifndef NET_H\n#define NET_H\nint net_old(void);\n#endif\n",
             "tools/units/stylelint.py": "", "tools/selftest.py": "", "tools/git/commitlint.py": "",
             "tools/units/fixture_tool.py": "X = 1\n", "CLAUDE.md": "agents\n",
             ".gitignore": ".pi/\nbuild/\nbuild.ninja\n"}
    base = fx.commit(files, "base")
    if sc.kind == "unit":
        fx.branch(BRANCH, checkout=True)
        fx.commit({"src/Net/net_new.cpp": "int net_new_step(void) { return 1; }\n"}, "the worker's own work")
        fx.checkout("main")
        edits = {"configure.py": CONF_NEW, "config/RMHE08/splits.txt": SPLITS_NEW,
                 "config/RMHE08/symbols.txt": SYMBOLS_NEW,
                 "src/Net/net_new.cpp": "int net_new_step(void) { return 1; }\n",
                 "include/Net/net.h": "#ifndef NET_H\n#define NET_H\nint net_old(void);\nint net_new_step(void);\n"
                                      "#endif\n"}
        for rel, text in edits.items():
            p = os.path.join(root, *rel.split("/"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
        os.makedirs(os.path.join(root, ".pi", "outbox"), exist_ok=True)
        with open(claims.outbox_path(root, UNIT), "w", encoding="utf-8") as fh:
            json.dump(OUTBOX, fh)
    elif sc.kind == "tools":
        with open(os.path.join(root, "tools", "units", "fixture_tool.py"), "w", encoding="utf-8") as fh:
            fh.write("X = 2\n")
    os.makedirs(os.path.join(root, "build", "RMHE08"), exist_ok=True)
    open(os.path.join(root, "build.ninja"), "w").close()
    with open(os.path.join(root, "build", "RMHE08", "report.json"), "w", encoding="utf-8") as fh:
        json.dump(report(sc.after), fh)
    if sc.record_base:
        # the base snapshot `record-base` would write, by hand (it would compile the base objects)
        snap = {"main/Net/net_old": {"fuzzy": 90.0, "matched_code": 512, "symbols": {"fn_net_old": 90.0}}}
        data = {"base": base, "recorded_at": "2026-01-01T00:00:00", "subject": "base",
                "ledger": {"covered": 10, "closed": 4, "partial": 2, "matched": 30, "bytes": 1000},
                "report": snap, "dirty_at_base": [], "undefrefs": {}, "orphans": {"pairs": []}}
        os.makedirs(os.path.join(root, ".pi"), exist_ok=True)
        with open(os.path.join(root, ".pi", "land-base.json"), "w", encoding="utf-8") as fh:
            json.dump(data, fh)


def orphans_stub(*_a, **_k):
    return {"unit_map_lines": [], "accepted": [], "strict": {"accepted": [], "deferred": {}},
            "unmatched_allowances": [], "touch": {}, "added_deferred": [], "claim_exposed": [], "have_base": True,
            "added": [], "pre_existing": [], "strict_counts": {}, "sole_owned": [], "untouched_pairs": {}}


def capture(verify, sc: Scenario, mods: dict | None = None) -> dict:
    """Run `verify` on a fresh fixture for `sc` with the boundary stubs; -> `{exit, rows: [[name, status, kind]]}`."""
    root = tempfile.mkdtemp(prefix="gate-golden-")
    build_fixture(root, sc)
    real_run = proc.run
    seen = {"rows": None}

    def fake_run(args, cwd=None, **kw):
        a = [str(x) for x in args]
        tool = os.path.basename(a[1]) if len(a) > 1 and a[0] == sys.executable else ""
        if a[0] == "git":
            return real_run(args, cwd=cwd, **kw)
        if tool == "stylelint.py":
            return subprocess.CompletedProcess(a, sc.lint[0], sc.lint[1], "")
        if tool == "selftest.py":
            return subprocess.CompletedProcess(a, 0, json.dumps({"failures": [], "stale_parks": [],
                                                                 "tree_clean": True}), "")
        if tool == "ledger.py":
            return subprocess.CompletedProcess(a, 0, json.dumps(LEDGER), "")
        if tool in ("commitlint.py", "configure.py", "flipcheck.py"):
            return subprocess.CompletedProcess(a, 0, "", "")
        if a[0] == "ninja":
            if "build/RMHE08/ok" in a:
                with open(os.path.join(root, "build", "RMHE08", "ok"), "w") as fh:
                    fh.write("ok\n")
            return subprocess.CompletedProcess(a, 0, "", "")
        return real_run(args, cwd=cwd, **kw)

    def record(fn):
        def wrapped(rows, *a, **k):
            seen["rows"] = [[r.name, r.status, r.kind] for r in findings.rows_of(rows)]
            return fn(rows, *a, **k)
        return wrapped

    snapshots = iter([{"Net/net_old": "a", "Net/nbr": "b"},
                      {"Net/net_old": "a", "Net/nbr": "c" if sc.drift else "b"}])
    bad = sc.post_fail
    rule10 = iter([{}, {"ref:src/Net/net_new.cpp:3:NetVTable": {"unit": UNIT, "kind": "ref", "where": "x"}}
                   if bad else {}])
    orphans = dict(orphans_stub(), **({"added": ["Net/net_new .data 0x80500000"], "have_base": False,
                                       "sole_owned": ["Net/net_new .rodata 0x80500100"]} if bad else {}))
    m = dict(MODULES, **(mods or {}))
    patches = [
        mock.patch.object(proc, "run", fake_run),
        mock.patch.object(findings, "render_table", record(findings.render_table)),
        mock.patch.object(findings, "render_lines", record(findings.render_lines)),
        mock.patch.object(m["prepcommit"], "ground_truth_error", lambda *a, **k: []),
        mock.patch.object(m["verifyunit"], "target_object_snapshot", lambda main: next(snapshots)),
        mock.patch.object(m["verifyunit"], "registration_check", lambda main, units: (not bad, "no splits block")),
        mock.patch.object(m["verifyunit"], "verify_units", lambda main, units: (not bad, "not reproducible", [])),
        mock.patch.object(m["undefrefs"], "check_units",
                          lambda *a, **k: {"problems": ["adds fn_x"] if bad else [], "pre_existing": [],
                                           "missing": [UNIT] if bad else []}),
        mock.patch.object(m["dataclosure"], "batch_orphans", lambda *a, **k: orphans),
        mock.patch.object(m["vtableaudit"], "sweep", lambda main, text_ref=None: {}),
        mock.patch.object(m["vtableaudit"], "rename_map", lambda main, ref: {}),
        mock.patch.object(m["vtableaudit"], "violation_rows", lambda sweep, rename=None: next(rule10)),
        mock.patch.object(m["claims"], "release", lambda unit, main, force=False, dry_run=False, **k:
                          {"complete": True, "steps": [], "branch": claims.branch_for(unit)}),
    ]
    with contextlib.ExitStack() as stack:
        for p in patches:
            stack.enter_context(p)
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
        code = verify(root, sc.units, None, dry_run=sc.dry_run, no_build=False,
                      allow_regression=sc.allow_regression, check_outbox=sc.check_outbox, release_claims=True,
                      no_selftests=sc.no_selftests)
    return {"exit": code, "rows": seen["rows"]}


#: The modules whose readers are stubbed, by role (the recording of the golden passed the monolith's own copies).
MODULES = {"prepcommit": prepcommit, "verifyunit": verifyunit, "undefrefs": undefrefs, "dataclosure": dataclosure,
           "vtableaudit": vtableaudit, "claims": claims}


def capture_all(verify, mods: dict | None = None) -> dict:
    return {sc.name: capture(verify, sc, mods) for sc in SCENARIOS}


def test_golden(c):
    from tools.units.landing import api
    from tools.tests.units.landing.gate_golden import GOLDEN as golden
    got = capture_all(api.verify)
    c.check("the same scenarios", sorted(got), sorted(golden))
    for name in golden:
        c.check("%s: exit code" % name, got.get(name, {}).get("exit"), golden[name]["exit"])
        c.check("%s: rows (name, status, kind, order)" % name, got.get(name, {}).get("rows"), golden[name]["rows"])


def test_golden_is_not_vacuous(c):
    from tools.tests.units.landing.gate_golden import GOLDEN as golden
    statuses = {r[1] for g in golden.values() for r in g["rows"]}
    c.check("the golden holds both PASS and FAIL rows", statuses >= {"PASS", "FAIL"}, True)
    c.check("... and a full run (the post-build rows) for the unit batch",
            any(r[0] == "ok was recreated by THIS run" for r in golden["unit"]["rows"]), True)
    c.check("... and both exit codes", {g["exit"] for g in golden.values()}, {0, 1})


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
