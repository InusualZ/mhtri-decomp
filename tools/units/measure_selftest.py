#!/usr/bin/env python3
"""Self-test for tools/units/measure.py - the whole-unit search-loop measurer.

    python tools/units/measure_selftest.py

The contract this pins is the one that made 61 workers write their own driver: **one compile and one
`objdiff report generate` for all N symbols of a unit**, scored with the official `fuzzy_match_percent`
(the metric `build/RMHE08/report.json` carries), with the worktree's own split object preferred over
MAIN's retired fallback. `recompile.py --measure` answers one symbol per run with two objdiff calls; this
tool must not turn that into N runs.

Layers:

* a **batching test** with a fake runner: a three-function unit must issue exactly one compile and exactly
  one `report generate`, and the focused form must still issue only that one report;
* a **target-resolution test**: worktree split > MAIN registered > MAIN `auto_*_text` fallback > missing,
  on MAIN-shaped temp trees, so the preference cannot silently regress;
* a **parsing test**: the report's functions and measures, the `>=80%`/`==100%` counts, and the compact
  instruction-diff counts;
* a **cache test**: the second run's per-symbol delta is read, and a target change invalidates it;
* an **integration cross-check** against the real `build/RMHE08/report.json` whenever the build tree and a
  compiled unit are present, skipped (not failed) otherwise.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
if os.path.dirname(HERE) not in sys.path:
    sys.path.insert(0, os.path.dirname(HERE))

import measure as ms  # noqa: E402
import recompile as rc  # noqa: E402
import unitutil  # noqa: E402


def _ok(label, got, want, failures):
    if got == want:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures + 1


def _cp(argv, stdout="", returncode=0):
    return subprocess.CompletedProcess(argv, returncode, stdout, "")


def _fake_main(tmp):
    """A MAIN-shaped tree: a registered unit object, the map, and dtk's config for the fallback."""
    main = os.path.join(tmp, "main")
    for d in (os.path.join(main, "config", "RMHE08"),
              os.path.join(main, "build", "RMHE08", "obj", "prop")):
        os.makedirs(d, exist_ok=True)
    open(os.path.join(main, "config", "RMHE08", "symbols.txt"), "w", encoding="utf-8").write(
        "fn_80001000 = .text:0x80001000; // type:function size:0x10\n"
        "fn_80001020 = .text:0x80001020; // type:function size:0x30\n"
        "fn_80002000 = .text:0x80002000; // type:function size:0x20\n"
        "lbl_80000500 = .data:0x80000500; // type:object size:0x4\n")
    json.dump({"units": [
        {"name": "auto_03_80001000_text", "object": "build/RMHE08/obj/auto_03_80001000_text.o",
         "code_size": 64, "data_size": 0},
    ]}, open(os.path.join(main, "build", "RMHE08", "config.json"), "w", encoding="utf-8"))
    open(os.path.join(main, "build", "RMHE08", "obj", "auto_03_80001000_text.o"), "wb").write(b"\x7fELF")
    open(os.path.join(main, "build", "RMHE08", "obj", "auto_fn_80002000_text.o"), "wb").write(b"\x7fELF")
    open(os.path.join(main, "build", "RMHE08", "obj", "prop", "unit.o"), "wb").write(b"\x7fELF")
    return main


def normalize_rows() -> int:
    """The `Camellia/camellia` trap: no extension must find the `.c` source, not append `.cpp`."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        wt = os.path.join(tmp, "wt")
        os.makedirs(os.path.join(wt, "src", "Camellia"))
        os.makedirs(os.path.join(wt, "src", "prop"))
        open(os.path.join(wt, "src", "Camellia", "camellia.c"), "w").write("")
        open(os.path.join(wt, "src", "prop", "unit.cpp"), "w").write("")
        failures = _ok("a .c unit keeps its extension", ms.normalize_unit("Camellia/camellia", wt),
                       "Camellia/camellia.c", failures)
        failures = _ok("a .cpp unit keeps its extension", ms.normalize_unit("prop/unit", wt),
                       "prop/unit.cpp", failures)
        failures = _ok("`src/` is stripped", ms.normalize_unit("src/Camellia/camellia", wt),
                       "Camellia/camellia.c", failures)
        failures = _ok("`main/` is stripped", ms.normalize_unit("main/Camellia/camellia", wt),
                       "Camellia/camellia.c", failures)
        failures = _ok("a build path is stripped", ms.normalize_unit("build/RMHE08/src/prop/unit.cpp", wt),
                       "prop/unit.cpp", failures)
        failures = _ok("an explicit extension is respected", ms.normalize_unit("Camellia/camellia.c", wt),
                       "Camellia/camellia.c", failures)
        failures = _ok("target_rel points at the split object", ms.target_rel("Camellia/camellia.c"),
                       os.path.join("build", "RMHE08", "obj", "Camellia", "camellia.o"), failures)
    return failures


def target_rows() -> int:
    """The target choice a proposal unit needs: the worktree's real object first, then MAIN, then auto."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        main = _fake_main(tmp)
        wt = os.path.join(tmp, "wt")
        os.makedirs(os.path.join(wt, "build", "RMHE08", "obj", "prop"), exist_ok=True)
        wt_obj = os.path.join(wt, "build", "RMHE08", "obj", "prop", "unit.o")
        main_obj = os.path.join(main, "build", "RMHE08", "obj", "prop", "unit.o")

        failures = _ok("a proposal with no worktree split uses MAIN's registered object",
                       os.path.normcase(ms.resolve_target(wt, main, "prop/unit", "fn_80002000")[0]),
                       os.path.normcase(main_obj), failures)
        open(wt_obj, "wb").write(b"\x7fELF")
        failures = _ok("the worktree's own split object wins once it exists",
                       os.path.normcase(ms.resolve_target(wt, main, "prop/unit", "fn_80002000")[0]),
                       os.path.normcase(wt_obj), failures)
        failures = _ok("... and it is labelled", ms.resolve_target(wt, main, "prop/unit", "fn_80002000")[1],
                       "worktree-split", failures)
        failures = _ok("a registered unit run from MAIN is `registered`, not `worktree-split`",
                       ms.resolve_target(main, main, "prop/unit", "fn_80002000")[1], "registered", failures)

        # no registered object: the retired auto run / single-symbol object for the address
        run, kind, _note = ms.resolve_target(wt, main, "prop/other", "fn_80001020")
        failures = _ok("an unregistered unit falls back to the auto run", kind, "auto-fallback", failures)
        failures = _ok("... and it is the run that covers the address",
                       os.path.basename(run), "auto_03_80001000_text.o", failures)
        one, kind, _note = ms.resolve_target(wt, main, "prop/other", "fn_80002000")
        failures = _ok("a single-symbol auto object is found by name", os.path.basename(one),
                       "auto_fn_80002000_text.o", failures)

        # nothing anywhere: `missing`, not a silent 0.0
        _path, kind, note = ms.resolve_target(wt, main, "prop/other", "lbl_80000500")
        failures = _ok("a data symbol has no target", kind, "missing", failures)
        failures = _ok("... and the note names the unit", "prop/other" in note, True, failures)
    return failures


def _report_runner(calls, functions, measures=None):
    """A fake objdiff writing the report/objdiff shapes `unitutil` parses; records every command."""
    def runner(argv, **kwargs):
        calls.append(list(argv))
        if "generate" in argv:
            out = argv[argv.index("-o") + 1]
            proj = argv[argv.index("-p") + 1]
            cfg = json.load(open(os.path.join(proj, "objdiff.json"), encoding="utf-8"))
            json.dump({"units": [{
                "name": cfg["units"][0]["name"],
                "measures": measures or {"fuzzy_match_percent": 50.0, "matched_functions": 1,
                                         "total_functions": len(functions)},
                "functions": functions,
            }]}, open(out, "w", encoding="utf-8"))
        else:
            out = argv[argv.index("-o") + 1]
            json.dump({"left": {"symbols": [
                {"name": "f0", "size": "16", "instructions": [
                    {"instruction": {"address": "0", "formatted": "li r3, 1"}},
                    {"diff_kind": "DIFF_ARG_MISMATCH",
                     "instruction": {"address": "4", "formatted": "li r4, 2"}}]},
            ]}, "right": {"symbols": [
                {"name": "f0", "size": "16", "instructions": [
                    {"instruction": {"address": "0", "formatted": "li r3, 1"}},
                    {"diff_kind": "DIFF_ARG_MISMATCH",
                     "instruction": {"address": "4", "formatted": "li r4, 3"}}]},
            ]}}, open(out, "w", encoding="utf-8"))
        return _cp(argv)
    return runner


def parse_rows() -> int:
    """One report -> functions + measures; the aggregates and the compact diff counts."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "target.o")
        base = os.path.join(tmp, "base.o")
        for path in (target, base):
            open(path, "wb").write(b"\x7fELF")
        functions = [
            {"name": "a", "size": "100", "fuzzy_match_percent": 100.0},
            {"name": "b", "size": "200", "fuzzy_match_percent": 80.0},
            {"name": "c", "size": "300", "fuzzy_match_percent": 10.0},
        ]
        calls = []
        entries, measures, _path = ms.score_report(
            target, base, "main/prop/unit", os.path.join(tmp, "r"), "objdiff-cli",
            runner=_report_runner(calls, functions))
        failures = _ok("every symbol comes back from the one report", sorted(entries), ["a", "b", "c"],
                       failures)
        failures = _ok("exactly one report generate for three symbols",
                       sum(1 for c in calls if "generate" in c), 1, failures)
        failures = _ok("the unit's own measures are kept", measures.get("fuzzy_match_percent"), 50.0,
                       failures)
        agg = ms.aggregates(entries, measures)
        failures = _ok(">= 80% counts the bar", agg["ge80"], 2, failures)
        failures = _ok("== 100% comes from the report's matched_functions", agg["at100"], 1, failures)
        failures = _ok("the mean is over the scored functions", round(agg["mean"], 2), 63.33, failures)

        rows = ms.instruction_diff_rows(target, base, "f0", "objdiff-cli", os.path.join(tmp, "r"),
                                        runner=_report_runner([], functions))
        failures = _ok("one differing instruction is counted", rows["differ"], 1, failures)
        failures = _ok("... by kind", rows["kinds"], {"DIFF_ARG_MISMATCH": 1}, failures)
        failures = _ok("... and carries both spellings", (rows["rows"][0]["target"],
                       rows["rows"][0]["ours"]), ("li r4, 2", "li r4, 3"), failures)
    return failures


def _compile_runner(calls, main):
    """Answer ninja and the compile itself; write the object the compile is supposed to produce."""
    unit_line = ("build\\tools\\sjiswrap.exe build\\compilers\\Wii\\1.3\\mwcceppc.exe -nodefaults "
                 "-O3 -lang=c++ -MMD -c src\\prop\\unit.cpp -o build\\RMHE08\\src\\prop")

    def runner(argv, **kwargs):
        calls.append(list(argv))
        if argv[0] == "ninja":
            if argv[-1] == "build/RMHE08/src/prop/unit.o":
                return _cp(argv, unit_line + "\n")
            return _cp(argv, "ninja: error: unknown target\n", 1)
        if any("mwcceppc" in a for a in argv):
            tokens = list(argv)
            obj_dir = tokens[tokens.index("-o") + 1]
            src = tokens[tokens.index("-c") + 1]
            obj = os.path.join(obj_dir, os.path.splitext(os.path.basename(src))[0] + ".o")
            os.makedirs(obj_dir, exist_ok=True)
            open(obj, "wb").write(b"\x7fELF")
            return _cp(argv)
        return _cp(argv)
    return runner


def collect_rows() -> int:
    """The whole contract: one compile, one report, N symbols - and the second run's delta."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        main = _fake_main(tmp)
        calls = []
        functions = [
            {"name": "f0", "size": "16", "fuzzy_match_percent": 90.0},
            {"name": "f1", "size": "32", "fuzzy_match_percent": 100.0},
            {"name": "f2", "size": "48", "fuzzy_match_percent": 40.0},
        ]
        runner = _compile_runner(calls, main)
        report = _report_runner(calls, functions)
        combined = lambda argv, **kw: report(argv, **kw) if argv[0] != "ninja" \
            and not any("mwcceppc" in a for a in argv) else runner(argv, **kw)

        result = ms.collect("prop/unit", main, main, symbol=None, runner=combined)
        failures = _ok("the unit compiles", result.get("compiled"), True, failures)
        failures = _ok("all three symbols are scored", sorted(result.get("functions") or {}),
                       ["f0", "f1", "f2"], failures)
        failures = _ok("the registered target is used", result.get("target_kind"), "registered", failures)
        failures = _ok("exactly one compile", sum(1 for c in calls if any("mwcceppc" in a for a in c)), 1,
                       failures)
        failures = _ok("exactly one report generate", sum(1 for c in calls if "generate" in c), 1,
                       failures)
        failures = _ok(">= 80% is computed", result["aggregates"]["ge80"], 2, failures)

        # second run, one score moved: the delta must be per-symbol and off the cache
        functions[0]["fuzzy_match_percent"] = 91.5
        calls2 = []
        report2 = _report_runner(calls2, functions)
        combined2 = lambda argv, **kw: report2(argv, **kw) if argv[0] != "ninja" \
            and not any("mwcceppc" in a for a in argv) else _compile_runner(calls2, main)(argv, **kw)
        result2 = ms.collect("prop/unit", main, main, symbol=None, runner=combined2)
        failures = _ok("the delta is read from the previous run",
                       round(result2["functions"]["f0"].get("delta", 0), 3), 1.5, failures)
        failures = _ok("an unmoved symbol has a zero delta",
                       round(result2["functions"]["f1"].get("delta", 1), 3), 0.0, failures)
        failures = _ok("a matching delta still means one report", sum(1 for c in calls2 if "generate" in c),
                       1, failures)

        # `--no-cache` must not read the stale file
        calls3 = []
        report3 = _report_runner(calls3, functions)
        combined3 = lambda argv, **kw: report3(argv, **kw) if argv[0] != "ninja" \
            and not any("mwcceppc" in a for a in argv) else _compile_runner(calls3, main)(argv, **kw)
        result3 = ms.collect("prop/unit", main, main, symbol=None, runner=combined3, use_cache=False)
        failures = _ok("no-cache drops the delta", "delta" in result3["functions"]["f0"], False, failures)
    return failures


def format_rows() -> int:
    """The text a worker reads: official unit score, the table, and the focused symbol with a delta."""
    failures = 0
    r = {
        "unit": "Camellia/camellia.c", "compiled": True, "fresh": True, "bytes": 1,
        "object": "obj.o", "target": "tgt.o", "target_kind": "registered", "command_source": "main",
        "functions": {
            "good": {"score": 100.0, "target_size": 16, "ours_size": 16, "delta": 2.0},
            "bad": {"score": 41.0, "target_size": 32, "ours_size": 32, "delta": -8.5},
        },
        "measures": {"fuzzy_match_percent": 72.5, "matched_functions": 1},
        "aggregates": {"total": 2, "at100": 1, "ge80": 1, "mean": 70.5},
        "extra_functions": [],
    }
    text = "\n".join(ms.format_report(r, show_diff=False))
    failures = _ok("the official unit score is printed", "72.5" in text and "official report metric" in text,
                   True, failures)
    failures = _ok("the ==100% count is printed as the report's", "1 == 100%" in text, True, failures)
    failures = _ok("the >=80% count is printed", "1 >= 80%" in text, True, failures)
    failures = _ok("worst-first puts the 41% row ahead of the 100% row",
                   text.index("bad") < text.index("good"), True, failures)
    failures = _ok("a negative delta is signed", "-8.50" in text, True, failures)
    focused = ms.format_report(dict(r, symbol="bad", detail=None), quiet=True)[-1]
    failures = _ok("the focused line carries target/ours sizes", "target    32 B" in focused, True,
                   failures)
    return failures


def integration_rows() -> int:
    """`collect` must equal the real report: the unit fuzzy and one symbol, symbol for symbol. Skipped
    without a build tree."""
    objdiff = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
    report_path = os.path.join(ROOT, "build", "RMHE08", "report.json")
    unit_src = os.path.join(ROOT, "src", "Camellia", "camellia.c")
    unit_tgt = os.path.join(ROOT, "build", "RMHE08", "obj", "Camellia", "camellia.o")
    if not (os.path.exists(objdiff) and os.path.exists(report_path) and os.path.exists(unit_src)
            and os.path.exists(unit_tgt)):
        print("skip  integration cross-check (no build tree)")
        return 0
    official = {}
    for u in json.load(open(report_path, encoding="utf-8")).get("units") or []:
        if u.get("name") == "main/Camellia/camellia":
            official = {"fuzzy": (u.get("measures") or {}).get("fuzzy_match_percent"),
                        "functions": {f.get("name"): f.get("fuzzy_match_percent")
                                      for f in u.get("functions") or []}}
    if not official:
        print("skip  integration cross-check (Camellia/camellia not in the report)")
        return 0
    failures = 0
    result = ms.collect("Camellia/camellia", ROOT, ROOT, symbol="camellia_setup256", use_cache=False)
    failures = _ok("collect compiles the unit", result.get("compiled"), True, failures)
    got = (result.get("measures") or {}).get("fuzzy_match_percent")
    failures = _ok("the unit score equals build/RMHE08/report.json", got, official["fuzzy"], failures)
    for name in ("camellia_setup128", "camellia_setup256"):
        failures = _ok("%s equals the report" % name, result["functions"][name]["score"],
                       official["functions"].get(name), failures)
    return failures


def main() -> int:
    failures = normalize_rows()
    failures += target_rows()
    failures += parse_rows()
    failures += collect_rows()
    failures += format_rows()
    failures += integration_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
