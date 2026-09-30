"""Loaded by every Python process the selftest runner starts (it prepends this directory to PYTHONPATH).

Installs `tools/spawnretry.py` so a selftest's `subprocess.run(["git", ...])` survives the transient
`PermissionError [WinError 5]` Windows raises at process launch under load, including in selftests that never
import `unitutil`. Never raises: a broken hook must not fail a test.
"""
import os
import sys

try:
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    import spawnretry
    spawnretry.install()
    sys.path.remove(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
except Exception:  # noqa: BLE001
    pass
