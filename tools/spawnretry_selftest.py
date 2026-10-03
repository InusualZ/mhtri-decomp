#!/usr/bin/env python3
"""Selftest for `spawnretry`: WinError 5 at launch is retried, anything else is raised at once."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import subprocess

from tools import spawnretry


def _winerr(code):
    exc = PermissionError(13, "Access is denied")
    exc.winerror = code
    return exc


def main() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def flaky(n, exc):
        calls = []

        def init(self, *a, **k):
            calls.append(1)
            if len(calls) <= n:
                raise exc
            return "started"
        return init, calls

    sleeps = []
    init, calls = flaky(2, _winerr(5))
    check("WinError 5 twice, then success: returns the launch's result",
          spawnretry.retrying(init, sleep=sleeps.append)(None), "started")
    check("... after three attempts with a growing backoff", (len(calls), sleeps), (3, [0.1, 0.2]))

    init, calls = flaky(99, _winerr(5))
    try:
        spawnretry.retrying(init, sleep=lambda s: None)(None)
        raised = False
    except PermissionError:
        raised = True
    check("a launch that always gets WinError 5 is raised after ATTEMPTS", (raised, len(calls)),
          (True, spawnretry.ATTEMPTS))

    init, calls = flaky(1, _winerr(2))
    try:
        spawnretry.retrying(init, sleep=lambda s: None)(None)
        other = None
    except PermissionError as exc:
        other = exc.winerror
    check("another PermissionError is raised at once, not retried", (other, len(calls)), (2, 1))

    init, calls = flaky(1, FileNotFoundError("nope"))
    try:
        spawnretry.retrying(init, sleep=lambda s: None)(None)
        other = None
    except FileNotFoundError:
        other = "fnf"
    check("FileNotFoundError is raised at once", (other, len(calls)), ("fnf", 1))

    if os.name == "nt":
        spawnretry.install()
        check("install() leaves Popen wrapped", getattr(subprocess.Popen.__init__, "__wrapped_by_spawnretry__", False),
              True)
        check("... and a second install() is a no-op", spawnretry.install(), False)
    p = subprocess.run([sys.executable, "-c", "print(7)"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    check("a real launch still works through the wrapper", p.stdout.strip(), "7")

    for f in fails:
        print("FAIL " + f)
    print("ok - %d checks" % checks)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
