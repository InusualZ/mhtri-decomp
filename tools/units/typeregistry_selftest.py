#!/usr/bin/env python3
"""Self-test for tools/units/typeregistry.py - the shared type/helper registry.

    python tools/units/typeregistry_selftest.py

The checks run against a temp fixture tree, so no repository state, no map and no compiler are touched.
What they pin: comment/string stripping that preserves length and line numbers, the declarator extractor
for every shape the tree carries (named/anonymous aggregates, forward and function-pointer and array
typedefs, `using`, macros, `inline` helpers, `extern` prototypes), the field-type fingerprint, the
`<digits><that many chars>` mangled-name peeling, shared-header scanning, the two debt kinds plus the
rule-2 repeated prototype, the vendor exception, the relevance ranking the brief renders, and that the
report text names the header and the copying unit.

`typeregistry.selftest()` holds the checks so `typeregistry.py --selftest` and this entry point cannot drift.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools"))

from units import typeregistry  # noqa: E402


def main() -> int:
    return typeregistry.selftest()


if __name__ == "__main__":
    sys.exit(main())
