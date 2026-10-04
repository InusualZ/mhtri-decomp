#!/usr/bin/env python3
"""Self-test for tools/units/callees.py - the generated-callee survey.

    python tools/units/callees_selftest.py
    python tools/units/callees.py --selftest        (the same checks)

Fixtures only: a hand-built ELF32-BE object, a map/splits/source tree in a temp directory and fixture
disassembly text - no build tree, no compiler, so the check count is identical in MAIN and a fresh
worktree. `callees.selftest()` holds the checks so this entry point and `callees.py --selftest` cannot
drift.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.units import callees


def main() -> int:
    return callees.selftest()


if __name__ == "__main__":
    sys.exit(main())
