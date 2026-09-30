#!/usr/bin/env python3
"""Dispatcher for the repository's matching tools, so the skill has one stable entry point.

The tools themselves live in the repository (`tools/`), not in this skill: they are versioned with the
code they compile and they are already unit-agnostic. This wrapper only forwards to them, which keeps the
skill free of forks that could drift.

Usage:
    python scripts/mt.py units                       # which units can be worked on
    python scripts/mt.py info   [-u <unit>]          # resolved paths + the real ninja command line
    python scripts/mt.py frames [-u <unit>] [--flags-extra "..."] [--versions ...]
    python scripts/mt.py matrix [-u <unit>] [--flags-extra "..."] [versions...]
    python scripts/mt.py sweep  [-u <unit>] [subs...]
    python scripts/mt.py variants [-u <unit>] [--variants f.py] [names...]
    python scripts/mt.py shapes [-u <unit>] [-f <function>] [--scan N] [--gens ...] [--depth N]
    python scripts/mt.py diff   [-u <unit>] <symbol> [n] [--all]        (symdiff: side by side)
    python scripts/mt.py slots  [-u <unit>] <symbol> [--map] [--slot 0x64]
    python scripts/mt.py sections [-u <unit>]                           (elfsect: section table)
    python scripts/mt.py dwarf  <obj> <function>                        (dwarfmap: var -> slot)
    python scripts/mt.py ideas  find <words>|show N|where N|new ...|check [--demos]|demo-check (the playbook's ideas)

Anything after the subcommand is passed through unchanged, so `--help` works per tool.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL = os.path.dirname(HERE)

TOOLS = {
    "units":    "tools/unitutil.py",
    "info":     "tools/unitutil.py",
    "frames":   "tools/flags/frame.py",
    "matrix":   "tools/flags/mwcc_matrix.py",
    "sweep":    "tools/flags/optsweep.py",
    "variants": "tools/flags/tryvar.py",
    "shapes":   "tools/flags/shapesearch.py",
    "diff":     "tools/objdiff/symdiff.py",
    "slots":    "tools/objdiff/slotmap.py",
    "sections": "tools/elf/elfsect.py",
    "dwarf":    "tools/elf/dwarfmap.py",
    "ideas":    "tools/agents/ideas.py",
}


def repo_root(start):
    d = os.path.abspath(start)
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s - this skill expects a "
                             "decomp-toolkit project" % start)
        d = parent


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0
    cmd, rest = argv[0], argv[1:]
    if cmd not in TOOLS:
        print("unknown command %r\n\n%s" % (cmd, __doc__.strip()), file=sys.stderr)
        return 2
    root = repo_root(SKILL)
    tool = os.path.join(root, TOOLS[cmd])
    if not os.path.exists(tool):
        print("missing %s - the repository's tools/ directory is the source of truth for these helpers"
              % os.path.relpath(tool, root), file=sys.stderr)
        return 2
    return subprocess.call([sys.executable, tool] + rest, cwd=root)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
