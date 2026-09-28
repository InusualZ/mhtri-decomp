#!/usr/bin/env python3
"""Self-test for `symdiff.py`: one unique temp directory per invocation, and the stale-object refusal.

    python tools/objdiff/symdiff_selftest.py

The first incident this closes: the shared `build/tmp/unitutil/unitutil_report.json` raised
`PermissionError [WinError 5]` while another process held it, twice, costing a measurement round
(2026-09-28). `session_tmpdir()` gives each invocation its own directory; `retry_transient` covers the
residual one-shot lock `os.remove`/`open` can still raise.

The second: `-u <unit>` scores the unit's **prebuilt** object, and the stale-object incident is that a
lane read it twice without the source having been rebuilt. `stale_reasons` (through
`tools/objdiff/freshguard.py`) must name the newer file, cover a header in the include closure, and
`main()` must refuse (exit 1, no score printed) rather than report a build that no longer exists.
"""
from __future__ import annotations

import contextlib
import importlib.util
import io
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SYMDIFF = os.path.join(ROOT, "tools", "objdiff", "symdiff.py")
CHECKS = 0


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
    global CHECKS
    CHECKS += 1
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
    child = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, encoding="utf-8", errors="replace",
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

    # 5. the freshness guard: `-u <unit>` scores a **prebuilt** object, and refuses when a source under
    #    the unit is newer than it (the stale-object incident). Fixture tree, no repository state.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src", "demo"))
        src = os.path.join(tmp, "src", "demo", "unit.cpp")
        hdr = os.path.join(tmp, "include", "demo")
        os.makedirs(hdr)
        header = os.path.join(hdr, "unit.h")
        obj = os.path.join(tmp, "build", "RMHE08", "src", "demo", "unit.o")
        os.makedirs(os.path.dirname(obj))
        with open(src, "w", encoding="utf-8") as fh:
            fh.write('#include "demo/unit.h"\nint fn(void) { return 0; }\n')
        with open(header, "w", encoding="utf-8") as fh:
            fh.write("\n")
        with open(obj, "wb") as fh:
            fh.write(b"\x7fELF")

        class _Fixture:
            name = "demo/unit"

        fx = _Fixture()
        fx.src = src
        fx.obj = obj
        base = 1_000_000.0
        os.utime(header, (base, base))
        os.utime(src, (base, base))
        os.utime(obj, (base + 10, base + 10))          # object newer than every source: current
        fails = check("a fresh object is not stale", symdiff.stale_reasons(fx, root=tmp), [], fails)
        os.utime(src, (base + 20, base + 20))          # the source moved, the object did not
        reasons = symdiff.stale_reasons(fx, root=tmp)
        fails = check("a source newer than the object is stale", bool(reasons), True, fails)
        fails = check("... naming the newer source",
                      bool(reasons) and "unit.cpp" in reasons[0], True, fails)
        # a **header** in the include closure dates the object too (the unit's inputs, not its .cpp)
        os.utime(src, (base, base))
        os.utime(header, (base + 30, base + 30))
        reasons = symdiff.stale_reasons(fx, root=tmp)
        fails = check("a header newer than the object is stale",
                      bool(reasons) and "unit.h" in reasons[0], True, fails)

        # and `main()` refuses loudly (exit 1, nothing on stdout) instead of printing stale numbers
        real_cli, real_root = symdiff.cli, uu.ROOT
        try:
            symdiff.cli = lambda: (None, None, fx)
            uu.ROOT = tmp                              # `main()` resolves the unit's tree from uu.ROOT
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc = symdiff.main()
            fails = check("main() refuses a stale object", rc, 1, fails)
            fails = check("... printing no score", out.getvalue().strip(), "", fails)
        finally:
            symdiff.cli, uu.ROOT = real_cli, real_root

    if fails:
        print("FAIL (%d)" % fails)
        return 1
    print("ok - %d checks" % CHECKS)
    return 0


if __name__ == "__main__":
    sys.exit(main())
