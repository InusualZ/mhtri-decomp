#!/usr/bin/env python3
"""The landing path's union of a registration conflict and its invariant assertions (`check_union`), for `land.py`.
Spec: docs/tools/spec/unionresolve.md. CLI: unionresolve.py PATH... (union in place) | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import tempfile

from tools.lib import cli
import tools.units.merge.unionresolve as _impl

TOOL = cli.Tool("unionresolve", "docs/tools/spec/unionresolve.md", tests="tools/tests/units/test_unionresolve.py")


def __getattr__(name: str):
    """Every name `land.py`/`rescue.py` call (`union_text_full`, `check_union`, `object_names`, `split_units`,
    `UNION_SCOPE`) lives in `merge.unionresolve`."""
    return getattr(_impl, name)


def selftest() -> int:
    return TOOL.selftest(cwd=tempfile.gettempdir())


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("paths", nargs="*", help="conflicted files to union in place")
    ap.add_argument("--selftest", action="store_true", help="run tools/tests/units/test_unionresolve.py")
    args = ap.parse_args(argv)
    if args.selftest or not args.paths:
        return selftest()
    for path in args.paths:
        print("    %-32s union-resolved %d" % (path, _impl.union_file(path)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
