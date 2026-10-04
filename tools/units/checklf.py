#!/usr/bin/env python3
"""Report a working-tree file whose line endings differ from its blob - a shim over `edit.py check --blob` (retired).
Spec: docs/tools/spec/checklf.md. CLI: checklf.py [PATH...] [--rev REV] [--repo DIR] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import tempfile

from tools.agents import edit
from tools.lib import cli

TOOL = cli.Tool("checklf", "docs/tools/spec/checklf.md",tests="tools/tests/units/test_checklf.py")

# the old public names, each the one implementation in `edit.py`
blob_of = edit.blob_of
endings = edit.endings
check_path = edit.check_path
status_paths = edit.status_paths


def selftest() -> int:
    return TOOL.selftest(cwd=tempfile.gettempdir())


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("paths", nargs="*", help="files to check (default: every path `git status` names)")
    ap.add_argument("--rev", default=":", help="the blob to compare against: `:` the index (default), or `HEAD`")
    ap.add_argument("--repo", default=None, help="the repository to run git in (default: this tree)")
    ap.add_argument("--selftest", action="store_true", help="run tools/tests/units/test_checklf.py")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    repo = args.repo or os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    return edit.check_blobs(repo, args.paths, args.rev)


if __name__ == "__main__":
    sys.exit(main())
