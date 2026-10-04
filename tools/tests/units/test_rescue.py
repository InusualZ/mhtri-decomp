"""rescue: synthetic rescue refs in a throwaway repository - all four verdicts, the registered and touched-path
derivations, a read-only audit, `--prune` deleting only `redundant`, `--ref` restricting, and the CLI's text."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import testing
from tools.units import rescue

TIER = "fixture"


def _registration(units):
    return "config.libs = [\n%s]\n" % "".join('    Object(NonMatching, "%s.cpp"),\n' % u for u in units)


def _splits(units):
    out, addr = ["Sections:\n\t.text       type:code align:32\n"], 0x80000100
    for u in units:
        out.append("%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n" % (u, addr, addr + 0x100))
        addr += 0x100
    return "\n".join(out) + "\n"


def _apply_unit(g, unit, src=None):
    """Add `unit` to configure.py + splits.txt + a source file, committed by the caller."""
    conf = (g.root / rescue.CONFIGURE).read_text(encoding="utf-8").replace(
        "\n]", '\n    Object(NonMatching, "%s.cpp"),\n]' % unit)
    spl = (g.root / rescue.SPLITS).read_text(encoding="utf-8") + \
        "\n%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n" % (unit, 0x80001000, 0x80001100)
    return {rescue.CONFIGURE: conf, rescue.SPLITS: spl,
            "src/%s.cpp" % unit: src if src is not None else "int %s(void) { return 0; }\n" % unit.replace("/", "_")}


def _refs(g):
    return set(g.git("for-each-ref", "--format=%(refname)", "refs/rescue/").split())


def _base():
    g = testing.GitFixture().init()
    base = g.commit({rescue.CONFIGURE: _registration(["mainunit"]), rescue.SPLITS: _splits(["mainunit"]),
                     "src/mainunit.cpp": "int mainunit(void) { return 0; }\n"}, "base")
    return g, base


def test_four_verdicts_and_prune(c):
    g, base = _base()
    repo = str(g.root)
    g.branch("b-red", checkout=True)
    red_tip = g.commit({"src/mainunit.cpp": "int mainunit(void) { return 7; }\n"}, "red")
    g.checkout("main")
    g.commit({"src/mainunit.cpp": "int mainunit(void) { return 7; }\n"}, "land red")
    g.git("update-ref", "refs/rescue/red", red_tip)
    g.branch("b-drift", base, checkout=True)
    drift_tip = g.commit(_apply_unit(g, "drift", "int drift(void) { return 1; }\n"), "drift v1")
    g.checkout("main")
    g.commit(_apply_unit(g, "drift", "int drift(void) { return 2; }\n"), "land drift v2")
    g.git("update-ref", "refs/rescue/drift", drift_tip)
    g.branch("b-unl", base, checkout=True)
    g.git("update-ref", "refs/rescue/unl", g.commit(_apply_unit(g, "unl"), "unl"))
    g.git("checkout", "-q", "--orphan", "orphan")
    g.git("rm", "-rq", "--cached", ".")
    g.git("update-ref", "refs/rescue/orphan", g.commit({"tools/scratch.txt": "nothing here\n"}, "orphan root"))
    g.git("checkout", "-qf", "main")
    g.branch("b-tools", base, checkout=True)
    g.git("update-ref", "refs/rescue/tools-only", g.commit({"tools/scratch.txt": "tool change\n"}, "tools only"))
    g.git("checkout", "-qf", "main")

    before = _refs(g)
    report = rescue.audit(repo, "main", refs=None, prune=False)
    c.check("read-only audit leaves every ref in place", _refs(g), before)
    by = {r["ref"]: r for r in report["refs"]}
    red = by["refs/rescue/red"]
    c.check("redundant: verdict, touched-path units, on main, no drift",
            (red["verdict"], red["units"], red["derivation"], red["units_on_main"], red["drift_paths"],
             red["touched_paths"]),
            (rescue.VERDICT_REDUNDANT, ["mainunit"], rescue.DERIVATION_TOUCHED, {"mainunit": True}, [],
             ["src/mainunit.cpp"]))
    dr = by["refs/rescue/drift"]
    c.check("drift: verdict, registered derivation, the differing path",
            (dr["verdict"], dr["units"], dr["derivation"], dr["units_on_main"], dr["drift_paths"]),
            (rescue.VERDICT_DRIFT, ["drift"], rescue.DERIVATION_REGISTERED, {"drift": True}, ["src/drift.cpp"]))
    c.check("unlanded: verdict, not on main", (by["refs/rescue/unl"]["verdict"], by["refs/rescue/unl"]["units_on_main"]),
            (rescue.VERDICT_UNLANDED, {"unl": False}))
    c.check("unknown: no merge-base", by["refs/rescue/orphan"]["verdict"], rescue.VERDICT_UNKNOWN)
    c.check("unknown: nothing parseable", by["refs/rescue/tools-only"]["verdict"], rescue.VERDICT_UNKNOWN)
    c.check("summary counts all four verdicts", {k: report["summary"][k] for k in rescue.VERDICTS},
            {rescue.VERDICT_REDUNDANT: 1, rescue.VERDICT_DRIFT: 1, rescue.VERDICT_UNLANDED: 1, rescue.VERDICT_UNKNOWN: 2})
    text = rescue.render(report)
    c.contains("the text names the unlanded ref", text, "UNLANDED  refs/rescue/unl")
    c.contains("the text carries the summary", text, "redundant 1, landed-with-drift 1, unlanded 1, unknown 2")

    pruned = rescue.audit(repo, "main", prune=True)
    c.check("prune: exactly the redundant ref was deleted", pruned["deleted"], ["refs/rescue/red"])
    c.check("prune: every other verdict survives", _refs(g),
            {"refs/rescue/drift", "refs/rescue/unl", "refs/rescue/orphan", "refs/rescue/tools-only"})
    c.check("--ref audits one ref", [r["ref"] for r in rescue.audit(repo, "main", refs=["refs/rescue/unl"])["refs"]],
            ["refs/rescue/unl"])
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = rescue.main(["audit", "--repo", repo, "--json", "--ref", "refs/rescue/drift"])
    c.check("the CLI's --json is the report", (rc, json.loads(out.getvalue())["count"]), (0, 1))


def test_registered_redundant(c):
    g, _base_sha = _base()
    g.branch("b-reg", checkout=True)
    reg_tip = g.commit(_apply_unit(g, "reg"), "reg")
    g.checkout("main")
    g.commit(_apply_unit(g, "reg"), "land reg")
    g.git("update-ref", "refs/rescue/reg", reg_tip)
    row = {r["ref"]: r for r in rescue.audit(str(g.root), "main")["refs"]}["refs/rescue/reg"]
    c.check("registered redundant: verdict, unit, no drift", (row["verdict"], row["units"], row["drift_paths"]),
            (rescue.VERDICT_REDUNDANT, ["reg"], []))
    c.check("registered redundant: prune deletes it", rescue.audit(str(g.root), "main", prune=True)["deleted"],
            ["refs/rescue/reg"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
