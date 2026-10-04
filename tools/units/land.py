"""The land gate: one command that runs the batch checklist and refuses to let a bad batch through.
Spec: docs/tools/spec/land.md. CLI: land.py record-base|verify|land [--branch B]|resolve --branch B|integrate ARGS."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.units.landing.api import *  # noqa: E402,F401,F403 - the names other tools import from `land`
from tools.units.landing.api import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
