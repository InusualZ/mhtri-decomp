#!/usr/bin/env python3
"""The one place a campaign lane's launch command is built (`lib.lanes.launch`). Spec: docs/tools/spec/lanecmd.md.
CLI: python tools/units/lanecmd.py check [CWD] [--main M] (is CWD a claimed slot a lane may be launched in?);
with no subcommand it prints its help (the API is the module)."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os

from tools.lib import cli
from tools.lib.lanes import registry
from tools.lib.lanes.launch import (BIN_ENV, INLINE_LIMIT, MODE_ENV, TOOLS_ENV, cwd_problem,  # noqa: F401
                                    lane_call, lanes_dir, resume_call, stage_task)

TOOL = cli.Tool("lanecmd", "docs/tools/spec/lanecmd.md", description=(__doc__ or "").splitlines()[0], common=())


def main(argv=None) -> int:
    ap = TOOL.parser()
    sub = ap.add_subparsers(dest="cmd")
    ck = sub.add_parser("check", help="exit 0 when CWD is a claimed slot (or a claim's worktree) a lane may be "
                                      "launched in, 1 with the reason and the remedy otherwise")
    ck.add_argument("cwd", nargs="?", default=None, help="the launch directory (default: the current one)")
    ck.add_argument("--main", default=None, help="MAIN (default: resolved from CWD with git)")
    args = ap.parse_args(argv)
    if args.cmd != "check":
        ap.print_help()
        return 0
    cwd = os.path.abspath(args.cwd or os.getcwd())
    main_tree = args.main or registry.main_of(cwd if os.path.isdir(cwd) else None)
    why = cwd_problem(main_tree, cwd)
    if why:
        print("REFUSED launch cwd: %s" % why)
        return 1
    print("ok: %s is a claimed lane tree" % cwd.replace("\\", "/"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
