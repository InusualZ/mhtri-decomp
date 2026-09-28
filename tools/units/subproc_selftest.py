#!/usr/bin/env python3
"""`subproc.py`'s selftest, run standalone: `python tools/units/subproc_selftest.py`.

The tool keeps its checks in `subproc.selftest()` so `tools/selftest.py` discovers them like every other
tool's (this is the wrapper half of the pair), and so the module that documents the decode rule is also the
module that proves the scanner enforcing it works.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from units import subproc  # noqa: E402


if __name__ == "__main__":
    raise SystemExit(subproc.selftest())
