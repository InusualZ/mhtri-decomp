#!/usr/bin/env python3
"""Refuse a union of a conflicted apply unless every conflict is a disjoint addition; undo the apply on refusal.
Spec: docs/tools/spec/unionguard.md. CLI: unionguard.py --branch B [--base REF] [--no-cleanup] [--cwd D] [path ...] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import tempfile

from tools.lib import cli
import tools.units.merge.unionguard as _impl

TOOL = cli.Tool("unionguard", "docs/tools/spec/unionguard.md", tests="tools/tests/units/test_unionguard.py")


def __getattr__(name: str):
    """Every name `land.py` calls (`unmerged`, `rename_sets`, `classify`, `cleanup_applied`) lives in `merge.unionguard`."""
    return getattr(_impl, name)


def selftest() -> int:
    return TOOL.selftest(cwd=tempfile.gettempdir())


def main(argv: list[str] | None = None) -> int:
    return _impl.main(argv, selftest=selftest, description=(__doc__ or "").splitlines()[0])


if __name__ == "__main__":
    sys.exit(main())
