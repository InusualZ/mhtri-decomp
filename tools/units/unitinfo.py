#!/usr/bin/env python3
"""List the units with source, or show what a unit spec resolves to and its exact ninja compile command.
Spec: docs/tools/spec/unitinfo.md. CLI: unitinfo.py [-u <unit>] [<unit>] [--root DIR]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import subprocess

from tools.lib import proc, repo, units


def list_lines(root: str) -> list[str]:
    """The unit listing: a count line, then one `name  source` line per unit with source."""
    found = units.Unit.list(root)
    return ["%d unit(s) with source in this repo:" % len(found)] + [
        "  %-32s %s" % (u.report_name, os.path.relpath(u.source, root)) for u in found]


def info_lines(spec: str, root: str, runner=subprocess.run) -> list[str]:
    """What `spec` resolves to in `root`: the unit name, its three paths, and the split compile command."""
    unit = units.Unit.resolve(spec, root)
    head, flags, tail = units.split_command(unit, runner=runner)
    return ["unit     " + unit.report_name,
            "src      " + os.path.relpath(unit.source, root),
            "obj      " + os.path.relpath(unit.obj_ours, root),
            "target   " + os.path.relpath(unit.obj_target, root),
            "compiler " + " ".join(head),
            "flags    " + " ".join(flags),
            "tail     " + " ".join(tail)]


def main(argv: list[str] | None = None, runner=subprocess.run) -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("spec", nargs="?", help="a unit spec (any spelling lib.units accepts)")
    ap.add_argument("-u", "--unit", dest="unit", help="the unit spec (same as the positional)")
    ap.add_argument("--root", default=None, help="the tree to read (default: the caller's tree)")
    args = ap.parse_args(argv)
    proc.install_spawn_retry()  # a launch Windows refuses transiently (WinError 5) is retried
    root = os.path.abspath(args.root) if args.root else repo.repo_root()
    spec = args.unit or args.spec
    lines = list_lines(root) if spec is None else info_lines(spec, root, runner)
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
