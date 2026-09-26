#!/usr/bin/env python3
"""Self-test for tools/units/verifyunit.py - the independent per-symbol verifier and registration gate.

    python tools/units/verifyunit_selftest.py

Each check is pinned against a fixture that must **refuse** (or, for the happy path, must pass), so the
failure the check exists for is reproducible rather than described:

* a unit registered in name only (a source with no `Object(...)` line, no `splits.txt` block, and no
  object target in the build graph) must refuse - and one registered on all three axes must pass;
* a per-symbol score that is not reproducible from a fresh `report generate`, and a 100 % claim whose
  bytes are not identical, must refuse;
* a function with no `fuzzy_match_percent` key must be read as 0 %, and the unit arithmetic that proves
  it must refuse when the two readings disagree;
* a split target object that moved for a unit the batch does not name must refuse (a neighbour the
  `splits.txt` change re-ranged), while the batch's own unit may move.

Two layers need the real build tree (a pair of objects and `objdiff-cli`) and are skipped, not failed,
without it: the live cross-check of a registered unit, and the doctored-report fixture that must refuse.
"""
from __future__ import annotations

import json
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
if os.path.dirname(HERE) not in sys.path:
    sys.path.insert(0, os.path.dirname(HERE))

import verifyunit as vu  # noqa: E402


def _ok(label, got, want, failures):
    if got == want:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures + 1


def name_rows() -> int:
    """The three spellings of one unit collapse to one key, and objects/report names derive from it."""
    failures = 0
    failures = _ok("a source path becomes a stem", vu.unit_stem("src/hud/fn_80334568.cpp"),
                   "hud/fn_80334568", failures)
    failures = _ok("the report spelling becomes the same stem", vu.unit_stem("main/hud/fn_80334568"),
                   "hud/fn_80334568", failures)
    failures = _ok("a build object becomes the same stem",
                   vu.unit_stem("build/RMHE08/src/hud/fn_80334568.o"), "hud/fn_80334568", failures)
    failures = _ok("a .c unit keeps its stem", vu.unit_stem("Camellia/camellia.c"), "Camellia/camellia",
                   failures)
    failures = _ok("the candidate object path", vu.src_object_rel("hud/fn_80334568"),
                   "build/RMHE08/src/hud/fn_80334568.o".replace("/", os.sep), failures)
    failures = _ok("the target object path", vu.target_object_rel("hud/fn_80334568"),
                   "build/RMHE08/obj/hud/fn_80334568.o".replace("/", os.sep), failures)
    failures = _ok("the report names it main/<stem>", vu.report_unit_name("hud/fn_80334568"),
                   "main/hud/fn_80334568", failures)
    return failures


def registration_rows() -> int:
    """A unit is in the build only when configure.py, splits.txt and the graph all carry it."""
    failures = 0
    unit = "hud/fn_80334568"
    conf = 'config.libs = [\n    Object(NonMatching, "hud/fn_80334568.cpp"),\n]\n'
    spl = "Sections:\nhud/fn_80334568.cpp:\n\t\t.text start:0x80334568 end:0x80338808\n"
    ninja = "build build\\RMHE08\\src\\hud\\fn_80334568.o: mwcc_sjis\n  cflags = ...\n"
    failures = _ok("a fully registered unit has no problem",
                   vu.registration_problems([unit], conf, spl, ninja), [], failures)
    # the incident: the source exists, the registration does not
    source_only = "build build\\RMHE08\\main.elf: link build\\RMHE08\\src\\main.o\n"
    problems = vu.registration_problems([unit], "config.libs = []\n", "Sections:\n", source_only)
    failures = _ok("a unit registered in name only refuses", problems != [], True, failures)
    failures = _ok("... and all three axes are named", len(problems), 3, failures)
    failures = _ok("... the configure.py axis", any("Object(" in p for p in problems), True, failures)
    failures = _ok("... the splits.txt axis", any("splits.txt" in p for p in problems), True, failures)
    failures = _ok("... the build-graph axis", any("build graph" in p for p in problems), True, failures)
    # a half-registration: the Object line is there but the split is not
    failures = _ok("an Object line without a splits block still refuses",
                   vu.registration_problems([unit], conf, "Sections:\n", ninja) != [], True, failures)
    # the Object line and the block are there but configure.py was never re-run
    failures = _ok("a unit missing from a stale build.ninja still refuses",
                   vu.registration_problems([unit], conf, spl, source_only) != [], True, failures)
    failures = _ok("no units means no work", vu.registration_problems([], conf, spl, ninja), [],
                   failures)
    # target extraction normalises separators and ignores phony/order-only tokens
    targets = vu.build_ninja_targets("build a\\b.o: rule\nbuild c.o d.o: rule | e.o\n")
    failures = _ok("backslash targets are normalised", "a/b.o" in targets, True, failures)
    failures = _ok("several outputs on one line are all read", {"c.o", "d.o"} <= targets, True,
                   failures)
    failures = _ok("order-only inputs are not targets", "e.o" in targets, False, failures)
    return failures


def registration_check_rows() -> int:
    """`registration_check` reads a tree (not just texts) and refuses the source-only fixture."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = []\n")
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write("Sections:\n")
        open(os.path.join(tmp, "build.ninja"), "w").write("")
        ok, _detail = vu.registration_check(tmp, ["hud/fn_80334568"])
        failures = _ok("registration_check refuses a source-only unit", ok, False, failures)
        # now register it on all three axes
        open(os.path.join(tmp, "configure.py"), "w").write(
            'Object(NonMatching, "hud/fn_80334568.cpp")\n')
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "Sections:\nhud/fn_80334568.cpp:\n\t\t.text start:0x1 end:0x2\n")
        open(os.path.join(tmp, "build.ninja"), "w").write(
            "build build\\RMHE08\\src\\hud\\fn_80334568.o: mwcc_sjis\n")
        ok, detail = vu.registration_check(tmp, ["hud/fn_80334568"])
        failures = _ok("registration_check passes the fully registered unit", ok, True, failures)
        failures = _ok("... and says so", "registered" in detail, True, failures)
    return failures


def drift_rows() -> int:
    """A target object that moved for a non-batch unit refuses; the batch's own unit may move."""
    failures = 0
    before = {"ours": "aaaa", "neighbour": "bbbb", "gone": "cccc"}
    after = {"ours": "XXXX", "neighbour": "YYYY", "new": "dddd"}
    problems = vu.target_drift_problems(before, after, ["ours"])
    failures = _ok("the batch's own unit may change", any(p.startswith("ours") for p in problems),
                   False, failures)
    failures = _ok("a re-ranged neighbour refuses", any(p.startswith("neighbour") for p in problems),
                   True, failures)
    failures = _ok("... and the message says it was re-ranged",
                   any("re-ranged a neighbour" in p for p in problems), True, failures)
    failures = _ok("a target object that disappeared refuses",
                   any("disappeared" in p for p in problems), True, failures)
    failures = _ok("a target object that appeared for a non-batch unit refuses",
                   any(p.startswith("new") for p in problems), True, failures)
    failures = _ok("an unchanged tree has no drift",
                   vu.target_drift_problems(before, before, []), [], failures)
    failures = _ok("a newly registered batch unit may appear",
                   vu.target_drift_problems({"a": None}, {"a": "hash"}, ["a"]), [], failures)
    return failures


def snapshot_rows() -> int:
    """`target_object_snapshot` scopes to splits.txt units and sees a mutated object."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj"))
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "Sections:\na/a.cpp:\n\t\t.text start:0x1 end:0x2\nb/b.cpp:\n\t\t.text start:0x2 end:0x3\n")
        for rel in ("a/a.o", "b/b.o"):
            path = os.path.join(tmp, "build", "RMHE08", "obj", *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as fh:
                fh.write(b"data-" + rel.encode())
        before = vu.target_object_snapshot(tmp)
        failures = _ok("every splits.txt unit is snapshotted", sorted(before), ["a/a", "b/b"], failures)
        failures = _ok("... including a unit with no object yet",
                       vu.target_object_snapshot(tmp).get("a/a") is not None, True, failures)
        with open(os.path.join(tmp, "build", "RMHE08", "obj", "a", "a.o"), "wb") as fh:
            fh.write(b"mutated")
        after = vu.target_object_snapshot(tmp)
        failures = _ok("a mutated object is seen as drift by an outsider",
                       vu.target_drift_problems(before, after, []) != [], True, failures)
        failures = _ok("... and tolerated for the unit the batch names",
                       vu.target_drift_problems(before, after, ["a/a"]), [], failures)
    return failures


def arithmetic_rows() -> int:
    """The `fuzzy_match_percent`-absent trap: absent is 0 %, and the identity must reproduce."""
    failures = 0
    functions = {"a": {"name": "a", "size": "100", "fuzzy_match_percent": 100.0},
                 "b": {"name": "b", "size": "100"}}          # b has no key
    ok, detail = vu.arithmetic_crosscheck({"total_code": 200, "fuzzy_match_percent": 50.0}, functions)
    failures = _ok("an absent key is 0%, so the unit arithmetic reproduces", ok, True, failures)
    failures = _ok("... and the check reports the computed number", "50.00000" in detail, True,
                   failures)
    # the wrong reading (absent = 100) would give 100; a report that says 100 is the liar
    ok, detail = vu.arithmetic_crosscheck({"total_code": 200, "fuzzy_match_percent": 100.0}, functions)
    failures = _ok("a unit fuzzy that only reproduces if absent=100 refuses", ok, False, failures)
    failures = _ok("... and the detail names the trap", "0%" in detail and "100%" in detail, True,
                   failures)
    failures = _ok("a subset of listed partials still reproduces",
                   vu.arithmetic_crosscheck({"total_code": 100, "fuzzy_match_percent": 100.0},
                                            {"a": {"size": "100", "fuzzy_match_percent": 100.0}})[0],
                   True, failures)
    return failures


def symbol_rows() -> int:
    """The report, the fresh report and the raw bytes are cross-checked against each other."""
    failures = 0
    raw_ok = {"a": {"target_size": 16, "candidate_size": 16, "in_target": True, "in_candidate": True,
                    "identical": True}}
    failures = _ok("a 100% symbol that is byte-identical passes",
                   vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                      {"a": {"fuzzy_match_percent": 100.0}}, raw_ok), ([], []),
                   failures)
    # report says 100, the bytes say otherwise
    raw_bad = {"a": dict(raw_ok["a"], identical=False)}
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 100.0}}, raw_bad)
    failures = _ok("a 100% claim with differing bytes refuses",
                   any("not identical" in p for p in hard), True, failures)
    raw_sized = {"a": dict(raw_ok["a"], candidate_size=32, identical=False)}
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 100.0}}, raw_sized)
    failures = _ok("a 100% claim with differing sizes refuses",
                   any("sizes differ" in p for p in hard), True, failures)
    # the fresh report disagrees with the committed one
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}}, {}, raw_ok)
    failures = _ok("a symbol the fresh report does not pair refuses",
                   any("does not pair" in p for p in hard), True, failures)
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 50.0}}, raw_ok)
    failures = _ok("a score that is not reproducible refuses",
                   any("not reproducible" in p for p in hard), True, failures)
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"name": "a", "size": "16"}}, raw_ok)
    failures = _ok("one report scoring a symbol the other reads as 0% refuses",
                   any("reads it as 0%" in p for p in hard), True, failures)
    hard, soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 50.0}},
                                    {"a": {"fuzzy_match_percent": 50.0}}, raw_ok)
    failures = _ok("identical bytes scored below 100 is an advisory, not a refusal", (hard, soft != []),
                   ([], True), failures)
    return failures


def size_gap_rows() -> int:
    """A symbol present on both sides that objdiff declines to pair is named."""
    failures = 0
    raw = {"x": {"target_size": 16, "candidate_size": 100, "in_target": True, "in_candidate": True,
                 "identical": False}}
    problems = vu.size_gap_problems({}, raw)
    failures = _ok("a >50% size gap is detected", len(problems), 1, failures)
    failures = _ok("... and says objdiff declines the pair", "declines the pair" in problems[0], True,
                   failures)
    failures = _ok("... and reads as untouched", "untouched" in problems[0], True, failures)
    failures = _ok("a similar size is not flagged",
                   vu.size_gap_problems({}, {"x": dict(raw["x"], candidate_size=20)}), [], failures)
    failures = _ok("an unwritten symbol (absent from ours) is honest 0%, not the trap",
                   vu.size_gap_problems({}, {"x": dict(raw["x"], in_candidate=False,
                                                       candidate_size=None)}), [], failures)
    failures = _ok("a paired symbol with a score is not flagged",
                   vu.size_gap_problems({"x": {"fuzzy_match_percent": 40.0}}, raw), [], failures)
    return failures


# --------------------------------------------------------------------------------------------------
# layers that need the real build tree (skipped, not failed, without it)
# --------------------------------------------------------------------------------------------------

def _real_objects():
    tgt = os.path.join(ROOT, "build", "RMHE08", "obj", "hud", "fn_80334568.o")
    cand = os.path.join(ROOT, "build", "RMHE08", "src", "hud", "fn_80334568.o")
    objdiff = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
    return tgt, cand, objdiff


def live_rows() -> int:
    """A registered unit, re-measured from its objects, reproduces the report. Skipped without a tree."""
    tgt, cand, objdiff = _real_objects()
    report = os.path.join(ROOT, "build", "RMHE08", "report.json")
    if not (os.path.exists(tgt) and os.path.exists(cand) and os.path.exists(objdiff)
            and os.path.exists(report)):
        print("skip  live cross-check (no compiled unit / report / objdiff)")
        return 0
    failures = 0
    ok, detail, advisories = vu.verify_units(ROOT, ["hud/fn_80334568"], objdiff=objdiff)
    failures = _ok("verify_units accepts a real registered unit", ok, True, failures)
    failures = _ok("... and reports the objects it re-measured", "re-measured" in detail, True, failures)
    for line in advisories:
        print("note  advisory: " + line)
    ok, detail = vu.registration_check(ROOT, ["hud/fn_80334568"])
    failures = _ok("the real tree registers hud/fn_80334568", ok, True, failures)
    failures = _ok("... and the registration check refuses a unit the tree lacks",
                   vu.registration_check(ROOT, ["hud/fn_00000000"])[0], False, failures)
    return failures


def doctored_report_rows() -> int:
    """A report doctored to disagree with a real `report generate` must refuse. Skipped without a tree."""
    tgt, cand, objdiff = _real_objects()
    report = os.path.join(ROOT, "build", "RMHE08", "report.json")
    if not (os.path.exists(tgt) and os.path.exists(cand) and os.path.exists(objdiff)
            and os.path.exists(report)):
        print("skip  doctored-report fixture (no compiled unit / report / objdiff)")
        return 0
    failures = 0
    real = None
    for entry in json.load(open(report, encoding="utf-8")).get("units") or []:
        if entry.get("name") == "main/hud/fn_80334568":
            real = entry
            break
    if real is None:
        print("skip  doctored-report fixture (hud/fn_80334568 not in the report)")
        return 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj", "hud"))
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "src", "hud"))
        shutil.copyfile(tgt, os.path.join(tmp, "build", "RMHE08", "obj", "hud", "fn_80334568.o"))
        shutil.copyfile(cand, os.path.join(tmp, "build", "RMHE08", "src", "hud", "fn_80334568.o"))
        doctored = json.loads(json.dumps(real))
        # flip the first 100% symbol's score so the committed report can no longer be reproduced
        victim = next((f for f in doctored["functions"]
                       if isinstance(f.get("fuzzy_match_percent"), (int, float))
                       and f["fuzzy_match_percent"] >= 100.0), None)
        if victim is None:
            print("skip  doctored-report fixture (no 100% symbol to doctor)")
            return 0
        victim["fuzzy_match_percent"] = 13.0
        json.dump({"units": [doctored]},
                  open(os.path.join(tmp, "build", "RMHE08", "report.json"), "w"))
        ok, detail, _adv = vu.verify_units(tmp, ["hud/fn_80334568"], objdiff=objdiff)
        failures = _ok("a report that disagrees with a fresh measurement refuses", ok, False, failures)
        failures = _ok("... and names the symbol", victim["name"] in detail, True, failures)
    return failures


def main() -> int:
    failures = name_rows()
    failures += registration_rows()
    failures += registration_check_rows()
    failures += drift_rows()
    failures += snapshot_rows()
    failures += arithmetic_rows()
    failures += symbol_rows()
    failures += size_gap_rows()
    failures += live_rows()
    failures += doctored_report_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
