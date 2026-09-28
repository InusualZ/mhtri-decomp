#!/usr/bin/env python3
"""Self-test for tools/units/callers.py - the whole-DOL caller index.

    python tools/units/callers_selftest.py
    python tools/units/callers.py --selftest        (the same checks)

Fixtures only: a three-file fake asm dump (`build/<game>/asm`, including a stale top-level copy of one
function), a fake `symbols.txt`/`splits.txt` and a temp cache - no build tree, no DOL, and no dump on
disk, so the check count is identical in MAIN and in a fresh worktree. `callers.selftest()` holds the
checks, so this entry point and `callers.py --selftest` cannot drift.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
for _path in (os.path.join(ROOT, "tools"), os.path.join(ROOT, "tools", "units")):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from units import callers  # noqa: E402


def main() -> int:
    return callers.selftest()


if __name__ == "__main__":
    sys.exit(main())
