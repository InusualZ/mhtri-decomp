#!/usr/bin/env python3
"""Self-test for tools/units/unionguard.py - the guard `.pi/bin/applybranch.sh` runs before it unions.

    python tools/units/unionguard_selftest.py
    python tools/units/unionguard.py --selftest

The guard exists because a plain ours-then-theirs union is only safe for **disjoint additions**; on a
delete, a rename, or both sides editing the same region it silently writes a tree that cannot build
(measured 2026-09-25: `configure.py` registered both halves of a rename; ten shared g3d headers unioned into
"illegal function overloading").  The four cases below are the contract: the first three must be refused,
the disjoint union must still succeed.  Every case builds a real git repo and a real unmerged index - no
repository state, no build, no mocks.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import unionguard  # noqa: E402  (imported through the sys.path shim above)


def main() -> int:
    print("unionguard self-test")
    return unionguard.selftest()


if __name__ == "__main__":
    sys.exit(main())
