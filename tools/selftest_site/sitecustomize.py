"""Loaded by every Python process the selftest runner starts (it prepends this directory to PYTHONPATH).

Installs the spawn retry (`tools.lib.proc.install_spawn_retry`) so a selftest's `subprocess.run(["git", ...])`
survives the transient `PermissionError [WinError 5]` Windows raises at process launch under load, including in
selftests that never import `unitutil`. Never raises: a broken hook must not fail a test.
"""
import os
import sys

try:
    _ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sys.path.insert(0, _ROOT)
    from tools.lib import proc as _proc
    _proc.install_spawn_retry()
    sys.path.remove(_ROOT)
except Exception:  # noqa: BLE001
    pass
