#!/usr/bin/env python3
"""The one union rule (code hunks union, prose hunks take the superset) - a shim over `tools/units/merge/union.py`.
Spec: docs/tools/spec/unionprose.md. CLI: unionprose.py --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import tempfile

from tools.lib import cli
import tools.units.merge.unionprose as _impl

TOOL = cli.Tool("unionprose", "docs/tools/spec/unionprose.md", tests="tools/tests/units/test_unionprose.py")


def __getattr__(name: str):
    """Every name the rule had (`union_markers`, `prose_superset`, `hunk_class`, ...) lives in `merge.unionprose`."""
    return getattr(_impl, name)


def selftest() -> int:
    return TOOL.selftest(cwd=tempfile.gettempdir())


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("--selftest", action="store_true", help="run tools/tests/units/test_unionprose.py")
    ap.parse_args(argv)
    return selftest()


if __name__ == "__main__":
    sys.exit(main())
