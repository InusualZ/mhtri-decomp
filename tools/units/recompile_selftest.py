#!/usr/bin/env python3
"""Self-test for tools/units/recompile.py's `--measure` - the score a worker is allowed to quote.

    python tools/units/recompile_selftest.py

The contract this pins is the one that was silently broken: `--measure` must return the *report* metric
(`report generate`'s `fuzzy_match_percent`), not objdiff-cli's explicit-diff `match_percent`. The two are
different normalisations, and `diff` additionally defaults `functionRelocDiffs` to `data_value` where the
report defaults to `none`, so the old number was lower than the official one and sent workers chasing
regressions that did not exist (`RSOStaticLocateObject`: 99.28205 vs 99.64103).

Two layers:

* a **wire test** with a fake objdiff runner, so the contract (`report generate` on a one-unit project,
  absolute target/base paths, `functionRelocDiffs=none` on the row detail) is checked without a build;
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
        return 0
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


def main() -> int:
    failures = wire_rows()
    failures += integration_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
