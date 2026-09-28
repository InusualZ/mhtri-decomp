#!/usr/bin/env python3
"""Self-test for `symdiff.py`'s scratch handling: one unique temp directory per invocation, cleaned up.

    python tools/objdiff/symdiff_selftest.py

The incident this closes: the shared `build/tmp/unitutil/unitutil_report.json` raised
`PermissionError [WinError 5]` while another process held it, twice, costing a measurement round
(2026-09-28). `session_tmpdir()` gives each invocation its own directory; `retry_transient` covers the
residual one-shot lock `os.remove`/`open` can still raise.
"""
from __future__ import annotations

import contextlib
import importlib.util
import io
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SYMDIFF = os.path.join(ROOT, "tools", "objdiff", "symdiff.py")


def _load():
    spec = importlib.util.spec_from_file_location("symdiff_under_test", SYMDIFF)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class _FakeUnit:
    target = "target.o"
    obj = "base.o"
    name = "mod/unit"


def check(name, got, want, fails):
    if got == want:
        print("ok    " + name)
        return 0
    print("FAIL  %s\n        got:  %r\n        want: %r" % (name, got, want))
    return fails + 1


def main() -> int:
    fails = 0
    symdiff = _load()

    # 1. the scratch directory is unique, exists, and is NOT the shared build/tmp/unitutil path
    shared = os.path.normpath(os.path.join(ROOT, "build", "tmp", "unitutil"))
    d1 = symdiff.session_tmpdir()
    fails = check("a scratch directory exists", os.path.isdir(d1), True, fails)
    fails = check("... with the symdiff- prefix", os.path.basename(d1).startswith("symdiff-"), True, fails)
    fails = check("... and it is not the shared build/tmp/unitutil",
                  os.path.normpath(d1) != shared, True, fails)
    fails = check("... one directory per invocation", symdiff.session_tmpdir(), d1, fails)

    # 2. uniqueness across invocations: a second process gets a different directory (the collision the
    #    shared path caused).  Run a child so the two `_TMPDIR`s are genuinely independent.
    code = ("import importlib.util,sys;"
            "spec=importlib.util.spec_from_file_location('s',%r);"
            "m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);"
            "print(m.session_tmpdir())" % SYMDIFF)
    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    child = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True,
                           cwd=ROOT, env=env)
    d2 = child.stdout.strip().splitlines()[-1] if child.returncode == 0 else ""
    fails = check("a second invocation gets a different directory",
                  bool(d2) and os.path.normpath(d2) != os.path.normpath(d1), True, fails)
    fails = check("... and the child's directory is cleaned up on exit", os.path.exists(d2), False, fails)
    if fails:
        print("        (child rc=%s stderr=%s)" % (child.returncode, child.stderr[:200]))

    # 3. the report/diff calls are handed the unique directory, never the shared default
    import unitutil as uu
    seen = {}
    real_funcs, real_measure = uu.report_functions, uu.report_measure

    def fake_funcs(target, base, unit_name=None, tmpdir=None, runner=subprocess.run):
        seen["funcs_tmpdir"] = tmpdir
        return {"fn": {"name": "fn", "size": 4, "fuzzy_match_percent": 100.0}}

    def fake_measure(target, base, symbol, unit_name=None, tmpdir=None, runner=subprocess.run):
        seen["measure_tmpdir"] = tmpdir
        return {"symbol": symbol, "match_percent": 100.0}

    try:
        uu.report_functions, uu.report_measure = fake_funcs, fake_measure
        symdiff.official_match(_FakeUnit, "fn")
        fails = check("official_match uses the unique scratch directory",
                      seen.get("measure_tmpdir"), d1, fails)
        with contextlib.redirect_stdout(io.StringIO()):
            symdiff.list_symbols(_FakeUnit)
        fails = check("list_symbols uses the unique scratch directory",
                      seen.get("funcs_tmpdir"), d1, fails)
    finally:
        uu.report_functions, uu.report_measure = real_funcs, real_measure

    # 4. the transient Windows sharing violation is retried, then given up on
    tries = {"n": 0}

    def flaky():
        tries["n"] += 1
        if tries["n"] < 3:
            raise PermissionError(5)
        return "ok"

    fails = check("retry_transient rides out a transient lock", symdiff.retry_transient(flaky), "ok", fails)
    fails = check("... after exactly the retries it needed", tries["n"], 3, fails)

    def always_locked():
        raise PermissionError(5)

    try:
        symdiff.retry_transient(always_locked, attempts=2)
        fails = check("retry_transient gives up with a PermissionError after its attempts", "raised", None,
                      fails)
    except PermissionError:
        fails = check("retry_transient gives up with a PermissionError after its attempts", "raised",
                      "raised", fails)

    def fine():
        return "ok"

    fails = check("retry_transient returns at once on success", symdiff.retry_transient(fine), "ok", fails)

    if fails:
        print("FAIL (%d)" % fails)
        return 1
    print("ok - 12 checks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
