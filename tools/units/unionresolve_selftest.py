#!/usr/bin/env python3
"""Self-test for tools/units/unionresolve.py - the append-union resolver `land.py resolve` uses.

    python tools/units/unionresolve_selftest.py
    python tools/units/unionresolve.py --selftest

Pure text: no git, no repository state, no build.  The cases pin the union (`ours` then `theirs`, the
`--diff3` base section dropped, a clean file passed through) and each of the four invariant assertions
`check_union` makes, because a wrong union breaks *silently* and the assertions are the only thing that
sees it.  The end-to-end git fixture, including the unsafe-union refusal, lives in `land.py --selftest`.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import unionresolve  # noqa: E402


if __name__ == "__main__":
    sys.exit(unionresolve.selftest())
