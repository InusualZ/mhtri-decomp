#!/usr/bin/env python3
"""Retired: folded into `undefrefs.py --census --linkage` (docs/tools/retired.md); this shim forwards until WP6.
Spec: docs/tools/spec/undefrefs.md. CLI: relocaudit.py [--main M] [--unit U] [--no-decls] [--json]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import subprocess
import sys

UNDEFREFS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "undefrefs.py")


def forwarded(argv: list[str]) -> list[str]:
    """The `undefrefs.py` arguments for a relocaudit command line: `--census --linkage` plus the same flags."""
    return ["--census", "--linkage"] + list(argv)


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    return subprocess.call([sys.executable, UNDEFREFS] + forwarded(argv))


if __name__ == "__main__":
    raise SystemExit(main())
