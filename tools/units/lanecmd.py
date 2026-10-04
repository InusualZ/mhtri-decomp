#!/usr/bin/env python3
"""The one place a campaign lane's launch command is built (`lib.lanes.launch`). Spec: docs/tools/spec/lanecmd.md.
CLI: python tools/units/lanecmd.py (prints its help; the API is the module)."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import cli
from tools.lib.lanes.launch import (BIN_ENV, INLINE_LIMIT, MODE_ENV, TOOLS_ENV, lane_call, lanes_dir,  # noqa: F401
                                    resume_call, stage_task)

TOOL = cli.Tool("lanecmd", "docs/tools/spec/lanecmd.md", description=(__doc__ or "").splitlines()[0], common=())


def main(argv=None) -> int:
    ap = TOOL.parser()
    ap.parse_args(argv)
    ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
