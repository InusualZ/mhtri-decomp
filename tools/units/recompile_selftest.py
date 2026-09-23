#!/usr/bin/env python3
"""Self-test for tools/units/recompile.py's `--measure` - the score a worker is allowed to quote.

    python tools/units/recompile_selftest.py

The contract this pins is the one that was silently broken: `--measure` must return the *report* metric
(`report generate`'s `fuzzy_match_percent`), not objdiff-cli's explicit-diff `match_percent`. The two are
different normalisations, and `diff` additionally defaults `functionRelocDiffs` to `data_value` where the
report defaults to `none`, so the old number was lower than the official one and sent workers chasing
regressions that did not exist (`RSOStaticLocateObject`: 99.28205 vs 99.64103).

Three layers:

* a **wire test** with a fake objdiff runner, so the contract (`report generate` on a one-unit project,
  absolute target/base paths, `functionRelocDiffs=none` on the row detail) is checked without a build;
* an **include-order test** - the other silent lie: MWCC searches `-i` in the order given and MAIN's paths
  are relative to MAIN, so a worktree edit to an existing shared header was shadowed. It resolves a header
  through the command line `rewrite` actually returns, the way the compiler would;
* an **integration cross-check** against the real `report generate` for the whole project whenever the
  build tree is present, skipped (not failed) otherwise - that is the assertion that the two numbers agree
  for real symbols.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import recompile as rc  # noqa: E402


def _completed(argv):
    return subprocess.CompletedProcess(argv, 0, "", "")


def _ok(label, got, want, failures):
    if got == want:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures + 1


def wire_rows() -> int:
    """The `--measure` return value and the commands it issues, with objdiff replaced by a stub.

    The stub validates the invocation *and* writes the two JSON shapes objdiff-cli would write, so the
    parsing half is exercised too.
    """
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "target.o")
        base = os.path.join(tmp, "base.o")
        for path in (target, base):
            with open(path, "wb") as fh:
                fh.write(b"\x7fELF")

        seen = {}

        def runner(argv, **kwargs):
            seen.setdefault("commands", []).append(list(argv))
            if "generate" in argv:
                proj = argv[argv.index("-p") + 1]
                cfg = json.load(open(os.path.join(proj, "objdiff.json"), encoding="utf-8"))
                seen["config"] = cfg
                out = argv[argv.index("-o") + 1]
                json.dump({"units": [{"name": cfg["units"][0]["name"], "functions": [
                    {"name": "LocateObject", "size": "888", "fuzzy_match_percent": 42.5}]}]},
                    open(out, "w", encoding="utf-8"))
            else:
                out = argv[argv.index("-o") + 1]
                json.dump({"left": {"symbols": [{"name": "LocateObject", "size": "888",
                                                 "match_percent": 1.0}]},
                           "right": {"symbols": [{"name": "LocateObject", "size": "888",
                                                  "match_percent": 1.0}]}},
                          open(out, "w", encoding="utf-8"))
            return _completed(argv)

        m = rc.measure(target, base, "LocateObject", "objdiff-cli", os.path.join(tmp, "t"),
                       unit="main/RSO/runtime", runner=runner)

        failures = _ok("score is the report metric", m.get("match_percent"), 42.5, failures)
        failures = _ok("positional metric kept separately", m.get("diff_match_percent"), 1.0, failures)
        failures = _ok("pairing from the row detail", m.get("paired"), True, failures)

        cfg = seen.get("config") or {}
        failures = _ok("one-unit project written", len(cfg.get("units") or []), 1, failures)
        failures = _ok("project version present", cfg.get("min_version"), rc.MIN_PROJECT_VERSION,
                       failures)
        unit = (cfg.get("units") or [{}])[0]
        failures = _ok("target_path absolute", unit.get("target_path"), os.path.abspath(target), failures)
        failures = _ok("base_path absolute", unit.get("base_path"), os.path.abspath(base), failures)
        failures = _ok("unit name carried through", unit.get("name"), "main/RSO/runtime", failures)

        report_cmd = next((c for c in seen.get("commands", []) if "generate" in c), [])
        diff_cmd = next((c for c in seen.get("commands", []) if "generate" not in c), [])
        failures = _ok("report generate is used for the score",
                       report_cmd[:3], ["objdiff-cli", "report", "generate"], failures)
        failures = _ok("row detail forces the report's reloc default",
                       "functionRelocDiffs=none" in diff_cmd, True, failures)

        # A symbol the report does not know must be reported as such, never as a number.
        def missing_runner(argv, **kwargs):
            out = argv[argv.index("-o") + 1]
            if "generate" in argv:
                json.dump({"units": [{"name": "u", "functions": []}]}, open(out, "w", encoding="utf-8"))
            else:
                json.dump({"left": {"symbols": []}, "right": {"symbols": []}},
                          open(out, "w", encoding="utf-8"))
            return _completed(argv)

        miss = rc.measure(target, base, "NotThere", "objdiff-cli", os.path.join(tmp, "t"),
                          unit="u", runner=missing_runner)
        failures = _ok("unknown symbol is an error", "error" in miss, True, failures)
    return failures


def _pick_unit(root: str):
    """A real objdiff unit with target and base objects present and at least two scored functions.

    The two-symbol requirement is the point of the cross-check, so a unit with a single function is not a
    candidate; `unitutil.frames` reads the target ELF directly, which is cheaper than a report round.
    """
    import unitutil

    path = os.path.join(root, "objdiff.json")
    if not os.path.exists(path):
        return None
    data = json.load(open(path, encoding="utf-8"))
    for unit in data.get("units") or []:
        target = os.path.join(root, unit.get("target_path") or "")
        base = os.path.join(root, unit.get("base_path") or "")
        if not (os.path.exists(target) and os.path.exists(base)):
            continue
        try:
            names = unitutil.function_names(target)
        except Exception:
            continue
        if len(names) >= 2:
            return {"name": unit.get("name"), "target": target, "base": base}
    return None


def integration_rows() -> int:
    """`measure()` must equal the real project report, symbol for symbol. Skipped without a build."""
    objdiff = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
    unit = _pick_unit(ROOT)
    if not os.path.exists(objdiff) or unit is None:
        print("skip  integration cross-check (no build tree)")
        return 0

    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        full = os.path.join(tmp, "full.json")
        p = subprocess.run([objdiff, "report", "generate", "-p", ROOT, "-o", full],
                           cwd=ROOT, capture_output=True, text=True, errors="replace")
        if p.returncode != 0 or not os.path.exists(full):
            print("skip  integration cross-check (report generate failed: %s)" % (p.stderr or p.stdout)[:120])
            return 0
        data = json.load(open(full, encoding="utf-8"))
        official = {}
        for u in data.get("units") or []:
            if u.get("name") == unit["name"]:
                for fn in u.get("functions") or []:
                    official[fn.get("name")] = fn.get("fuzzy_match_percent")

        symbols = [n for n in official if isinstance(official[n], (int, float))][:2]
        if len(symbols) < 2:
            print("skip  integration cross-check (unit has fewer than two scored functions)")
            return 0
        for sym in symbols:
            m = rc.measure(unit["target"], unit["base"], sym, objdiff, tmp, unit=unit["name"])
            failures = _ok(f"{unit['name']} {sym}: measure == report", m.get("match_percent"),
                           official[sym], failures)
    return failures


def _search_path(tokens):
    """The `-i` directories in the order MWCC searches them - read off the command line, not a helper."""
    return [tokens[i + 1] for i, t in enumerate(tokens) if t == "-i" and i + 1 < len(tokens)]


def _which(tokens, cwd, name):
    """The file the compiler would read for `#include <name>`: first hit on the search path.

    A relative entry is resolved against the compile's cwd, which `recompile.compile_unit` always sets to
    MAIN - that is exactly why MAIN's relative `-i include` shadowed the worktree's copy.
    """
    for d in _search_path(tokens):
        cand = d if os.path.isabs(d) else os.path.join(cwd, d)
        if os.path.exists(os.path.join(cand, name)):
            return os.path.abspath(os.path.join(cand, name))
    return None


def _legacy_rewrite(tokens, unit, main, wt):
    """`rewrite` as it was before the fix: the worktree's include dirs appended, i.e. after MAIN's."""
    out, obj_dir, i = [], None, 0
    src = os.path.join(wt, "src", *rc.unit_source(unit).split("/"))
    while i < len(tokens):
        tok = tokens[i]
        if tok == "-o":
            out.append(tok)
            i += 1
            obj_dir = os.path.join(wt, tokens[i].lstrip("./\\"))
            out.append(obj_dir)
        elif tok == "-c":
            out.append(tok)
            i += 1
            out.append(src)
        else:
            out.append(tok)
        i += 1
    inc = []
    for p in (os.path.join(wt, "build", "RMHE08", "include"), os.path.join(wt, "include")):
        if os.path.isdir(p):
            inc += ["-i", p]
    first_compile = next(k for k, t in enumerate(out) if t == "-c")
    return out[:first_compile] + inc + out[first_compile:], obj_dir


def include_order_rows() -> int:
    """The worktree's headers must be the ones compiled, even when MAIN carries the same path.

    The failure this pins is silent: a worker edits an existing shared header, MAIN's copy wins the search,
    the compile succeeds, and the measurement is of MAIN's source. Measured on `auto/800FF8D4_fn_800FF8D4`
    with a deliberately different `include/nw4r/math.h` in the worktree: the old ordering read MAIN's
    header and scored `fn_8010140C` 100.0 %, the fixed one read the worktree's and scored 99.57143 %.
    """
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "main")
        wt = os.path.join(tmp, "wt")
        for path in (os.path.join(main, "include", "nw4r"), os.path.join(main, "build", "RMHE08", "include"),
                     os.path.join(wt, "include", "nw4r"), os.path.join(wt, "src", "probe")):
            os.makedirs(path, exist_ok=True)
        for path, text in ((os.path.join(main, "include", "shared.h"), "MAIN\n"),
                           (os.path.join(main, "include", "nw4r", "math.h"), "MAIN\n"),
                           (os.path.join(main, "build", "RMHE08", "include", "generated.h"), "gen\n"),
                           (os.path.join(wt, "include", "shared.h"), "WORKTREE\n"),
                           (os.path.join(wt, "include", "nw4r", "math.h"), "WORKTREE\n"),
                           (os.path.join(wt, "src", "probe", "probe.cpp"), "int f() { return 1; }\n")):
            open(path, "w", encoding="utf-8").write(text)

        tokens = ["sjiswrap.exe", "mwcceppc.exe", "-nodefaults", "-i", "include", "-i",
                  "build/RMHE08/include", "-O3", "-MMD", "-c", "src/probe/probe.cpp",
                  "-o", "build/RMHE08/src/probe"]
        cmd, obj = rc.rewrite(tokens, "probe/probe", main, wt)

        wt_inc = os.path.abspath(os.path.join(wt, "include"))
        failures = _ok("worktree include is searched first", _search_path(cmd)[0], wt_inc, failures)
        failures = _ok("a shadowed header resolves to the worktree's copy",
                       _which(cmd, main, "shared.h"), os.path.join(wt_inc, "shared.h"), failures)
        failures = _ok("a header only the worktree has still resolves",
                       _which(cmd, main, os.path.join("nw4r", "math.h")),
                       os.path.join(wt_inc, "nw4r", "math.h"), failures)
        failures = _ok("MAIN's generated include stays reachable",
                       _which(cmd, main, "generated.h"),
                       os.path.join(main, "build", "RMHE08", "include", "generated.h"), failures)
        failures = _ok("no relative MAIN spelling survives the rewrite",
                       [d for d in _search_path(cmd) if not os.path.isabs(d)], [], failures)
        failures = _ok("no duplicate search entries", len(_search_path(cmd)), 2, failures)
        failures = _ok("source is the worktree's",
                       cmd[cmd.index("-c") + 1], os.path.join(wt, "src", "probe", "probe.cpp"), failures)
        failures = _ok("object goes to the worktree's build dir",
                       os.path.normcase(os.path.normpath(obj)),
                       os.path.normcase(os.path.normpath(
                           os.path.join(wt, "build", "RMHE08", "src", "probe", "probe.o"))), failures)

        # A command line with no `-i` at all: the worktree's include is still added, ahead of the flags.
        bare = ["sjiswrap.exe", "mwcceppc.exe", "-O3", "-c", "src/probe/probe.cpp", "-o",
                "build/RMHE08/src/probe"]
        cmd4, _obj4 = rc.rewrite(bare, "probe/probe", main, wt)
        failures = _ok("an include-less command line gains the worktree's include",
                       _search_path(cmd4), [wt_inc], failures)
        failures = _ok("and it is inserted ahead of the flags", cmd4[2:4], ["-i", wt_inc], failures)

        # The regression this test exists for: the legacy ordering resolves the header to MAIN's copy.
        legacy, _obj_dir = _legacy_rewrite(tokens, "probe/probe", main, wt)
        failures = _ok("the old ordering read MAIN's header (the bug this pins)",
                       _which(legacy, main, "shared.h"),
                       os.path.join(main, "include", "shared.h"), failures)
        failures = _ok("the old ordering put the worktree's include last",
                       os.path.normcase(os.path.abspath(_search_path(legacy)[-1])), os.path.normcase(wt_inc),
                       failures)

        # A worktree that has generated headers of its own: its `build/RMHE08/include` outranks MAIN's.
        os.makedirs(os.path.join(wt, "build", "RMHE08", "include"), exist_ok=True)
        open(os.path.join(wt, "build", "RMHE08", "include", "generated.h"), "w",
             encoding="utf-8").write("worktree\n")
        cmd2, _obj2 = rc.rewrite(tokens, "probe/probe", main, wt)
        failures = _ok("the worktree's generated include outranks MAIN's",
                       _which(cmd2, main, "generated.h"),
                       os.path.join(wt, "build", "RMHE08", "include", "generated.h"), failures)

        # A worktree with no `include/` of its own: MAIN's directories are kept, in MAIN's own order.
        plain = os.path.join(tmp, "plain")
        os.makedirs(os.path.join(plain, "src", "probe"), exist_ok=True)
        cmd3, _obj3 = rc.rewrite(tokens, "probe/probe", main, plain)
        failures = _ok("a bare worktree keeps MAIN's search path",
                       _which(cmd3, main, "shared.h"),
                       os.path.join(main, "include", "shared.h"), failures)
        failures = _ok("a bare worktree keeps MAIN's order",
                       _search_path(cmd3),
                       [os.path.join(main, "include"),
                        os.path.join(main, "build", "RMHE08", "include")], failures)
    return failures


def main() -> int:
    failures = wire_rows()
    failures += include_order_rows()
    failures += integration_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
