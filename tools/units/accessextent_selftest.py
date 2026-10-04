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
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.units import accessextent


def main() -> int:
    return accessextent.selftest()


if __name__ == "__main__":
    sys.exit(main())
