"""Retry a process launch that Windows refuses transiently (`PermissionError: [WinError 5] Access is denied`).

Measured 2026-09-30 while lanes were building: under parallel load `subprocess.run(["git", ...])` inside the
`queue`, `lane` and `land` selftests died in `_winapi.CreateProcess` with `PermissionError [WinError 5]` -
the executable is fine a moment later (a scanner or another process briefly holds it), and the same error was
seen once in `claims`. The test was not wrong, the launch lost a race.

`install()` wraps `subprocess.Popen.__init__` so a WinError 5 from the launch is retried a few times with a
short backoff before it is raised. A genuine refusal (a directory, a file that cannot execute) still raises,
after ~1.5 s. It is a no-op off Windows, idempotent, and is installed by `unitutil` (every tool that
imports it) and by `tools/selftest_site/sitecustomize.py` (every process the selftest runner starts).
"""
from __future__ import annotations

import os
import subprocess
import time

ATTEMPTS = 6
BACKOFF_S = 0.1          # sleeps 0.1, 0.2, 0.3, 0.4, 0.5 between the six attempts

_installed = False


def is_transient(exc: BaseException) -> bool:
    return isinstance(exc, PermissionError) and getattr(exc, "winerror", None) == 5


def retrying(init, sleep=time.sleep, attempts: int = ATTEMPTS, backoff: float = BACKOFF_S):
    """`init` (a `Popen.__init__`-shaped callable) with WinError 5 retried."""
    def wrapper(self, *args, **kwargs):
        for attempt in range(attempts):
            try:
                return init(self, *args, **kwargs)
            except PermissionError as exc:
                if not is_transient(exc) or attempt == attempts - 1:
                    raise
                sleep(backoff * (attempt + 1))
    wrapper.__wrapped_by_spawnretry__ = True
    return wrapper


def install() -> bool:
    """Patch `subprocess.Popen` once; True when this call installed it."""
    global _installed
    if os.name != "nt" or _installed or getattr(subprocess.Popen.__init__, "__wrapped_by_spawnretry__", False):
        return False
    subprocess.Popen.__init__ = retrying(subprocess.Popen.__init__)
    _installed = True
    return True
