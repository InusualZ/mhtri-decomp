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


#: The new-unit name row's two scenarios (2026-10-05): a unit registered under a generated stem, and a unit renamed
#: from one (`--unit-rename Net/fn_80001100=Net/net_new`).
GEN_UNIT = "Net/fn_80001100"
CONF_GEN = CONF_NEW.replace("Net/net_new.cpp", "Net/fn_80001100.cpp")
SPLITS_GEN = SPLITS_NEW.replace("Net/net_new.cpp", "Net/fn_80001100.cpp")


class Scenario:
    def __init__(self, name, kind, units, dry_run=False, check_outbox=True, record_base=True, lint=(0, ""),
                 after=None, drift=False, allow_regression=(), no_selftests=False, post_fail=False, variant=None,
                 unit_renames=(), manifest=None):
        self.name, self.kind, self.units, self.dry_run = name, kind, list(units), dry_run
        # `gen`: the batch registers GEN_UNIT; `rename`: the base registers GEN_UNIT and the batch renames it to UNIT
        self.variant, self.unit_renames = variant, list(unit_renames)
        self.check_outbox, self.record_base, self.lint, self.drift = check_outbox, record_base, lint, drift
        self.after = after or {"main/Net/net_old": {"fn_net_old": 90.0}, "main/Net/net_new": {"net_new_step": 50.0}}
        self.allow_regression, self.no_selftests = list(allow_regression), no_selftests
        # every object row's reader refuses: registration, references, rule 10, data closure, the re-measure
        self.post_fail = post_fail
        # the lane manifest written under the `manifest` key of `.pi/outbox/<MANIFEST_SLUG>.json` and named by slug
        self.manifest = manifest


#: The style-lint row run for real (rule 15, 2026-10-05): the stylelint subprocess is not stubbed but run from this
#: repository against the fixture, so the row's verdict is the lint's own on the batch's comment text.
REAL_LINT = "real"
REAL_LINT_PATH = os.path.join(str(next(p for p in pathlib.Path(__file__).resolve().parents
                                       if (p / "tools" / "__init__.py").is_file())), "tools", "units", "stylelint.py")
#: The worker's unit text per rule-15 variant: a stale path new to the file (refused), and only advisory markers (passes).
R15_UNIT_TEXT = {
    "r15-stale": "/* the step table, see proposal/80001100_net.cpp */\nint net_new_step(void) { return 1; }\n",
    "r15-advisory": "/* 2026-10-05: the network lane's wave 2 (50 % fuzzy match) */\n"
                    "int net_new_step(void) { return 1; }\n",
}
#: The lane-manifest scenarios (2026-10-06): the unit batch changes configure.py, splits.txt, symbols.txt, its own
#: source and the hot header `src/Net/net.h`. Inside its manifest it passes; with that header read-only it refuses.
MANIFEST_SLUG = "lane-net"
MANIFEST_OWNS = {"lane": "lane-net", "base": "HEAD", "owns": ["src/Net/", "configure.py", "config/RMHE08/*.txt"],
                 "read_only": [], "units": [UNIT]}
MANIFEST_HOT = dict(MANIFEST_OWNS, read_only=["src/Net/net.h"])
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
    Scenario("new-unit-generated-name-refusal", "unit", [GEN_UNIT], check_outbox=False, variant="gen"),
    Scenario("new-unit-renamed-from-generated-name", "unit", [UNIT], check_outbox=False, dry_run=True,
             variant="rename", unit_renames=["%s=%s" % (GEN_UNIT, UNIT)]),
    # the 2026-10-05 header move: a batch of renames out of the retired include/ root lands; a new path there refuses
    Scenario("layout-move", "layout", ["configure.py"], check_outbox=False),
    Scenario("layout-retired-include-refusal", "layout", ["configure.py"], check_outbox=False, variant="add"),
    # rule 15 through the real lint: a stale path new to a changed file refuses the row; advisory markers pass it
    Scenario("lint-rule15-stale-path-refusal", "unit", [UNIT], dry_run=True, lint=REAL_LINT, variant="r15-stale"),
    Scenario("lint-rule15-advisory-pass", "unit", [UNIT], dry_run=True, lint=REAL_LINT, variant="r15-advisory"),
    # the lane manifest: a batch inside its `owns` passes the row; one touching a `read_only` hot header refuses it
    Scenario("manifest-inside-pass", "unit", [UNIT], dry_run=True, manifest=MANIFEST_OWNS),
    Scenario("manifest-read-only-hot-header-refusal", "unit", [UNIT], dry_run=True, manifest=MANIFEST_HOT),
]


def git(root, *args):
    return testing.GitFixture(root).git(*args)


#: The long-path batch (2026-10-05: the 687-path header move crashed the commit step on the Windows command line):
#: BULK_N headers move out of the retired `include/` root by a plain rename, so the batch is BULK_N deletions plus
#: BULK_N new files - 3,000 paths, about 115 KB, three times the 32,767-character `CreateProcess` limit. The kind is
#: not in SCENARIOS (the golden is unchanged): `test_long_paths.py` lands it through the whole flow.
BULK_N = 1500
BULK_OLD = ["include/Bulk/bulk_header_number_%04d.h" % i for i in range(BULK_N)]
BULK_NEW = [p.replace("include/", "src/", 1) for p in BULK_OLD]


def build_fixture(root: str, sc: Scenario) -> None:
    fx = testing.GitFixture(root).init()
    if sc.kind == "bulk":
        os.makedirs(os.path.join(root, "include", "Bulk"))
        for rel in BULK_OLD:
            with open(os.path.join(root, *rel.split("/")), "w", encoding="utf-8", newline="\n") as fh:
                fh.write("int %s;\n" % os.path.basename(rel)[:-2])
        git(root, "add", "--", "include")
    files = {"configure.py": CONF, "config/RMHE08/splits.txt": SPLITS, "config/RMHE08/symbols.txt": SYMBOLS,
             "src/Net/net_old.cpp": "int net_old(void) { return 0; }\n",
             "src/Net/net.h": "#ifndef NET_H\n#define NET_H\nint net_old(void);\n#endif\n",
             "tools/units/stylelint.py": "", "tools/selftest.py": "", "tools/git/commitlint.py": "",
             "tools/units/fixture_tool.py": "X = 1\n", "CLAUDE.md": "agents\n",
             ".gitignore": ".pi/\nbuild/\nbuild.ninja\n"}
    if sc.variant == "rename":
        files.update({"configure.py": CONF_GEN, "config/RMHE08/splits.txt": SPLITS_GEN,
                      "src/Net/fn_80001100.cpp": "int net_new_step(void) { return 1; }\n"})
    if sc.kind == "layout":
        files["include/Net/old.h"] = "#ifndef OLD_H\n#define OLD_H\nint net_old(void);\n#endif\n"
    base = fx.commit(files, "base")
    if sc.kind == "unit":
        unit = sc.units[0]
        fx.branch(claims.branch_for(unit), checkout=True)
        fx.commit({"src/%s.cpp" % unit: "int net_new_step(void) { return 1; }\n"}, "the worker's own work")
        fx.checkout("main")
        if sc.variant == "rename":
            git(root, "rm", "-q", "src/Net/fn_80001100.cpp")
        conf, spl = (CONF_GEN, SPLITS_GEN) if sc.variant == "gen" else (CONF_NEW, SPLITS_NEW)
        edits = {"configure.py": conf, "config/RMHE08/splits.txt": spl,
                 "config/RMHE08/symbols.txt": SYMBOLS_NEW,
                 "src/%s.cpp" % unit: "int net_new_step(void) { return 1; }\n",
                 "src/Net/net.h": "#ifndef NET_H\n#define NET_H\nint net_old(void);\nint net_new_step(void);\n"
                                      "#endif\n"}
        if sc.variant in R15_UNIT_TEXT:
            # the real lint judges these: only the unit's comment differs, and no foreign declaration rides along
            edits["src/%s.cpp" % unit] = R15_UNIT_TEXT[sc.variant]
            del edits["src/Net/net.h"]
        for rel, text in edits.items():
            p = os.path.join(root, *rel.split("/"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
        os.makedirs(os.path.join(root, ".pi", "outbox"), exist_ok=True)
        with open(claims.outbox_path(root, UNIT), "w", encoding="utf-8") as fh:
            json.dump(OUTBOX, fh)
        if sc.manifest is not None:
            with open(os.path.join(root, ".pi", "outbox", MANIFEST_SLUG + ".json"), "w", encoding="utf-8") as fh:
                json.dump({"manifest": sc.manifest}, fh)
    elif sc.kind == "layout":
        if sc.variant == "add":
            with open(os.path.join(root, "include", "Net", "new.h"), "w", encoding="utf-8") as fh:
                fh.write("int net_new(void);\n")
        else:
            git(root, "mv", "include/Net/old.h", "src/Net/old.h")
    elif sc.kind == "bulk":
        os.makedirs(os.path.join(root, "src", "Bulk"))
        for old, new in zip(BULK_OLD, BULK_NEW):
            os.replace(os.path.join(root, *old.split("/")), os.path.join(root, *new.split("/")))
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


def capture(verify, sc: Scenario, mods: dict | None = None, root: str | None = None) -> dict:
    """Run `verify` on a fresh fixture for `sc` with the boundary stubs; -> `{exit, rows: [[name, status, kind]]}`.
    `root` is the (empty) directory to build the fixture in; a fresh temp directory by default."""
    root = root or tempfile.mkdtemp(prefix="gate-golden-")
    build_fixture(root, sc)
    real_run = proc.run
    seen = {"rows": None}

    def fake_run(args, cwd=None, **kw):
        a = [str(x) for x in args]
        tool = os.path.basename(a[1]) if len(a) > 1 and a[0] == sys.executable else ""
        if a[0] == "git":
            return real_run(args, cwd=cwd, **kw)
        if tool == "stylelint.py":
            if sc.lint == REAL_LINT:
                return real_run([sys.executable, REAL_LINT_PATH] + a[2:], cwd=cwd, **kw)
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
    from tools.units.landing import state as _state
    with contextlib.ExitStack() as stack:
        for p in patches:
            stack.enter_context(p)
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
        _state.set_unit_renames(sc.unit_renames)
        _state.set_manifest(MANIFEST_SLUG if sc.manifest is not None else None)
        try:
            code = verify(root, sc.units, None, dry_run=sc.dry_run, no_build=False,
                          allow_regression=sc.allow_regression, check_outbox=sc.check_outbox, release_claims=True,
                          no_selftests=sc.no_selftests)
        finally:
            _state.set_unit_renames([])
            _state.set_manifest(None)
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


def test_post_build_refusal_names_its_row(c):
    """A post-build refusal reaches `problems` (what `land` reads): before 2026-10-05 only the pre-build rows did, so
    `land` refused a red build naming no row and the landing log recorded `nothing to stage`."""
    from tools.units.landing import api, flow
    sc = next(s for s in SCENARIOS if s.name == "unit-object-rows-refuse")
    problems: list = []
    got = capture(lambda *a, **k: api.verify(*a, problems=problems, **k), sc)
    c.check("the post-build scenario refuses", got["exit"], 1)
    c.check("... and its failing rows reach `problems`, the first one first",
            flow.failing_row(problems), "every batch unit is registered (configure.py + splits.txt + build graph)")
    c.check("... every FAIL row of the table is named once",
            len(problems), sum(1 for r in got["rows"] if r[1] == "FAIL"))
    _action, why = api.land_decision(False, [], problems, api.kinds_from_failures(problems))
    c.check("... so the refusal line names the row instead of a bare 'the gate failed'",
            "every batch unit is registered" in why, True)


def test_bookkeeping_rows_warn(c):
    """The outbox row and the stale `--allow-regression` row are WARNINGS (2026-10-05): their findings reach the
    `warnings` out-parameter, their rows PASS, and the verdict comes from the other rows alone."""
    from tools.units.landing import api
    got = {}
    for name in ("two-units-outbox-refusal", "two-units-neighbour-moved-regressed", "unit"):
        sc = next(s for s in SCENARIOS if s.name == name)
        warned: list = []
        res = capture(lambda *a, **k: api.verify(*a, warnings=warned, **k), sc)
        got[name] = (res["exit"], sorted({w.split(":", 1)[0] for w in warned}))
    c.check("the missing outbox is a warning; the branch-commits row still refuses that batch",
            got["two-units-outbox-refusal"], (1, ["every unit's outbox validates"]))
    c.check("the stale --allow-regression is a warning; the regression and drift rows still refuse",
            got["two-units-neighbour-moved-regressed"], (1, ["every --allow-regression was actually needed"]))
    c.check("a clean unit batch warns nothing", got["unit"], (0, []))


def test_new_unit_name_row(c):
    """The new-unit name row (2026-10-05) refuses a unit registered under a generated stem before the build, credits a
    declared rename from one, and is absent from a batch that registers named units (every older scenario)."""
    from tools.units.landing import api
    from tools.units.landing.rows.objects import NEW_UNIT_ROW
    gen = capture(api.verify, next(s for s in SCENARIOS if s.name == "new-unit-generated-name-refusal"))
    c.check("a generated new unit refuses (exit 1)", gen["exit"], 1)
    c.check("... on the new-unit row, before the build", ([r[1] for r in gen["rows"] if r[0] == NEW_UNIT_ROW],
                                                          any(r[0] == "ninja (exit 0)" for r in gen["rows"])),
            (["FAIL"], False))
    ren = capture(api.verify, next(s for s in SCENARIOS if s.name == "new-unit-renamed-from-generated-name"))
    c.check("a rename from a generated stem to a named one is credited (PASS)",
            [r[1] for r in ren["rows"] if r[0] == NEW_UNIT_ROW], ["PASS"])
    named = capture(api.verify, next(s for s in SCENARIOS if s.name == "unit-dry-run"))
    c.check("a batch registering a named unit carries no new-unit row", any(r[0] == NEW_UNIT_ROW
                                                                            for r in named["rows"]), False)


def test_new_unit_name_problems(c):
    """The row's pure decision: refused / credited, by the generated components a new registration spells."""
    from tools.units.landing.rows.objects import new_unit_name_problems
    base = {"Net/net_old", "Net/fn_80001100", "auto/fn_80002000"}
    c.check("a new named unit is neither refused nor credited",
            new_unit_name_problems(base, base | {"Net/net_new"}), ([], []))
    c.check("a new generated stem is refused", new_unit_name_problems(base, base | {"Net/fn_80009999"})[0],
            ["Net/fn_80009999: generated name `fn_80009999`"])
    c.check("an address-named stem is refused", new_unit_name_problems(base, base | {"menu/Panel805482CC"})[0],
            ["menu/Panel805482CC: generated name `Panel805482CC`"])
    c.check("a generated directory is refused", new_unit_name_problems(base, base | {"fn_8004CAD8/vec"})[0],
            ["fn_8004CAD8/vec: generated name `fn_8004CAD8`"])
    c.check("a GUESS name is allowed (the check is the pattern only)",
            new_unit_name_problems(base, base | {"enemy/em_guess_spawner"}), ([], []))
    c.check("a rename to a named stem is credited",
            new_unit_name_problems(base - {"Net/fn_80001100"}, base - {"Net/fn_80001100"} | {"Net/net_new"},
                                   {"Net/fn_80001100": ["Net/net_new"]}),
            ([], ["Net/net_new (renamed from Net/fn_80001100)"]))
    c.check("a move that keeps the generated stem is credited",
            new_unit_name_problems(base, base | {"Net/fn_80002000"}, {"auto/fn_80002000": ["Net/fn_80002000"]}),
            ([], ["Net/fn_80002000 (keeps fn_80002000 from auto/fn_80002000)"]))
    c.check("a rename to ANOTHER generated stem is refused",
            new_unit_name_problems(base, base | {"Net/fn_80003000"}, {"Net/fn_80001100": ["Net/fn_80003000"]})[0],
            ["Net/fn_80003000: generated name `fn_80003000`"])


def test_rule15_style_lint_row(c):
    """Rule 15 through the real lint (2026-10-05): a stale path new to a changed file refuses the style-lint row, and
    the refusal names rule 15 and the file; advisory markers alone (a date, a lane, a percentage) leave it passing."""
    from tools.units.landing import api
    style = "style lint (§6.5) adds no violation"
    got = {}
    for name in ("lint-rule15-stale-path-refusal", "lint-rule15-advisory-pass"):
        problems: list = []
        res = capture(lambda *a, **k: api.verify(*a, problems=problems, **k), next(s for s in SCENARIOS if s.name == name))
        got[name] = (res["exit"], [r[1] for r in res["rows"] if r[0] == style], [str(p) for p in problems])
    exit_code, status, problems = got["lint-rule15-stale-path-refusal"]
    c.check("a new stale path refuses the style-lint row (exit 1)", (exit_code, status), (1, ["FAIL"]))
    c.check("... and the refusal is rule 15's, on the unit's file",
            any("rule 15" in p and "src/Net/net_new.cpp" in p for p in problems), True)
    c.check("advisory markers alone pass the row", got["lint-rule15-advisory-pass"][:2], (0, ["PASS"]))


def test_manifest_row(c):
    """The lane-manifest row (2026-10-06): inside its manifest the batch passes; a read-only hot header refuses it with
    the header named and the remedy; no `--manifest` means no row (every older scenario)."""
    from tools.units.landing import api
    from tools.units.landing.rows.manifest import ROW
    got = {}
    for name in ("manifest-inside-pass", "manifest-read-only-hot-header-refusal", "unit-dry-run"):
        problems: list = []
        res = capture(lambda *a, **k: api.verify(*a, problems=problems, **k), next(s for s in SCENARIOS if s.name == name))
        got[name] = (res["exit"], [r[1] for r in res["rows"] if r[0] == ROW], problems)
    c.check("a batch inside its manifest passes the row", got["manifest-inside-pass"][:2], (0, ["PASS"]))
    exit_code, status, problems = got["manifest-read-only-hot-header-refusal"]
    c.check("a batch touching a read-only hot header refuses (exit 1, FAIL)", (exit_code, status), (1, ["FAIL"]))
    c.check("... and the refusal names the header, why, and the request remedy",
            [("src/Net/net.h (read-only `src/Net/net.h`)" in p, "integrator request" in p) for p in problems],
            [(True, True)])
    c.check("no --manifest, no row", got["unit-dry-run"][1], [])


def test_manifest_decision(c):
    """The row's pure decision and loader: globs, directories, read-only over owns, the outbox key, malformed input."""
    import tempfile as _tf
    from tools.units.landing.rows import manifest as m
    c.check("a directory entry covers everything below it, with or without the slash",
            [m.matches("src/Net/a/b.cpp", "src/Net/"), m.matches("src/Net/a.cpp", "src/Net"),
             m.matches("src/Network/a.cpp", "src/Net")], [True, True, False])
    c.check("a glob is fnmatch (`*` crosses `/`); an exact path is itself",
            [m.matches("config/RMHE08/splits.txt", "config/RMHE08/*.txt"), m.matches("configure.py", "configure.py"),
             m.matches("configure.pyc", "configure.py")], [True, True, False])
    man = {"lane": "l", "owns": ["src/Net/", "configure.py"], "read_only": ["src/Net/net.h"]}
    c.check("outside_manifest: read-only wins over owns, an unowned path is named, an owned one passes",
            m.outside_manifest(["src/Net/net.h", "src/pl/pl.h", "src/Net/x.cpp", "configure.py"], man),
            [("src/Net/net.h", "read-only `src/Net/net.h`"), ("src/pl/pl.h", "not in `owns`")])
    c.check("a well-formed manifest has no shape problem", m.shape_problems(man), [])
    c.check("an empty owns, a missing lane and a non-list read_only are each named",
            len(m.shape_problems({"owns": [], "read_only": "src/"})), 3)
    root = _tf.mkdtemp(prefix="manifest-load-")
    os.makedirs(os.path.join(root, ".pi", "outbox"))
    with open(os.path.join(root, ".pi", "outbox", "lane-a.json"), "w", encoding="utf-8") as fh:
        json.dump({"unit": "x", "manifest": man}, fh)
    with open(os.path.join(root, "bare.json"), "w", encoding="utf-8") as fh:
        json.dump(man, fh)
    c.check("a slug resolves to its outbox's `manifest` key; `worker/<slug>` too; a bare file is the manifest",
            [m.load(root, "lane-a")[0], m.load(root, "worker/lane-a")[0], m.load(root, "bare.json")[0]], [man] * 3)
    c.check("a missing manifest is reported, never a crash", (m.load(root, "nope")[0], "no manifest" in m.load(root, "nope")[1]),
            (None, True))


def test_golden_is_not_vacuous(c):
    from tools.tests.units.landing.gate_golden import GOLDEN as golden
    statuses = {r[1] for g in golden.values() for r in g["rows"]}
    c.check("the golden holds both PASS and FAIL rows", statuses >= {"PASS", "FAIL"}, True)
    c.check("... and a full run (the post-build rows) for the unit batch",
            any(r[0] == "ok was recreated by THIS run" for r in golden["unit"]["rows"]), True)
    c.check("... and both exit codes", {g["exit"] for g in golden.values()}, {0, 1})


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
