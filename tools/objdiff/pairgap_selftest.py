#!/usr/bin/env python3
"""Standalone entry point for `pairgap.py`'s selftest.

    python tools/objdiff/pairgap_selftest.py

`tools/objdiff/pairgap.py --selftest` is the same set of checks; this half exists so the discovery in
`tools/selftest.py` sees a standalone `*_selftest.py` beside the tool and collapses the two into one
entry (it detects the delegation below and runs the tool, never both).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import pairgap  # noqa: E402

if __name__ == "__main__":
    sys.exit(pairgap.selftest())
