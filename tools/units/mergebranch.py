#!/usr/bin/env python3
"""Bring `main` into a held lane branch, resolve the conflicts by class, prove the result, commit it.
Spec: docs/tools/spec/mergebranch.md. CLI: mergebranch.py resolve [--branch B] [--dry-run] [--json] | status."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import cli
import tools.units.merge.mergebranch as _impl

TOOL = cli.Tool("mergebranch", "docs/tools/spec/mergebranch.md")


def __getattr__(name: str):
    """Every name the module had (`resolve`, `addadd_choice`, `union_markers`, ...) lives in `merge.mergebranch`."""
    return getattr(_impl, name)


def main(argv: list[str] | None = None) -> int:
    return _impl.main(argv, description=(__doc__ or "").splitlines()[0])


if __name__ == "__main__":
    sys.exit(main())
