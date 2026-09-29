#!/usr/bin/env python3
"""Self-test for tools/units/accessextent.py - how far anything reads a data block.

    python tools/units/accessextent_selftest.py
    python tools/units/accessextent.py --selftest      (the same checks)

`accessextent.selftest()` holds the checks, so this entry point and the tool's flag cannot drift. The
synthetic half is instruction sequences only - no dump, no ELF and no objdump - and the real Q_UserData
acceptance run is made only when this tree has `build/<game>/main.elf` and an objdump, so the check count
is identical in MAIN and in a fresh worktree.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
for _path in (os.path.join(ROOT, "tools"), os.path.join(ROOT, "tools", "units")):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from units import accessextent  # noqa: E402


def main() -> int:
    return accessextent.selftest()


if __name__ == "__main__":
    sys.exit(main())
