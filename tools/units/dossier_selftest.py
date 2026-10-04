#!/usr/bin/env python3
"""Self-test for tools/units/dossier.py - the binary dossier.

    python tools/units/dossier_selftest.py
    python tools/units/dossier.py --selftest          (the same checks)

The pure readers run against synthetic data (a hand-built `.text` with three asserts, a dense jump
table, a two-literal pool, a one-section DOL, a fixture dump zip), so they need no build tree and no
compiler. When `build/RMHE08/obj/auto/800CCFB0_fn_800CCFB0.o` exists - the `ef_line.cpp` unit whose
`__FILE__`/panic/`Panic` traces motivated the tool - the whole pipeline is checked against it, and the
`brief.py` integration is checked by rendering that brief and looking for the dossier block.

`dossier.selftest()` holds the checks so this entry point and `dossier.py --selftest` cannot drift.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.units import dossier


def main() -> int:
    return dossier.selftest()


if __name__ == "__main__":
    sys.exit(main())
