#!/usr/bin/env python3
"""Lint `src/` and `include/` against the type and naming discipline of docs/plan.md section 6.5 (roadmap 7.21).
Spec: docs/tools/spec/stylelint.md. CLI: stylelint.py [--budget [--headers]] [--findings [--path G] [--rule N]]
[--diff REF | --ref BRANCH] [--list-added] [--json] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import cli as _cli
import tools.units.stylelint_rules.api as _api
from tools.units.stylelint_rules.api import *  # noqa: F401,F403 - every name the lint had (land, backlog, methodize, ...)

# The rules, the comparison and the reports live in `tools/units/stylelint_rules/` (one module per rule); this file
# is the entry point the gate, the profiles and `tools/selftest.py` call, and the one import surface.
TOOL = _cli.Tool("stylelint", "docs/tools/spec/stylelint.md", tests=lambda: _api.main(["--selftest"]))


def __getattr__(name: str):
    """A name the facade does not star-export still resolves through it."""
    return getattr(_api, name)


def selftest() -> int:
    """The lint's selftest (`tools/units/stylelint_rules/selftest.py`)."""
    return TOOL.selftest()


if __name__ == "__main__":
    sys.exit(main())
