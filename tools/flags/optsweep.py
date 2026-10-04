#!/usr/bin/env python3
"""Sweep `-opt` sub-options for a unit and print the resulting frame sizes. Spec: docs/tools/spec/optsweep.md.
CLI: python tools/flags/optsweep.py [-u <unit>] [<sub>...] [--flags-extra "<flags>"]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os

from tools.lib import repo, units

# The compiler's own `-opt` keyword list (see `mwcceppc.exe -help`); anything not listed there is
# silently ignored by MWCC, so it cannot be the answer.
CANDIDATES = ["level=0", "level=1", "level=2", "level=3", "level=4", "size", "space", "speed",
              "nospeed", "nospace", "peephole", "nopeephole", "cse", "nocse", "commonsubs",
              "nocommonsubs", "deadcode", "nodeadcode", "deadstore", "nodeadstore", "dead",
              "nodead", "lifetimes", "nolifetimes", "loop", "noloop", "loopinvariants",
              "noloopinvariants", "propagation", "nopropagation", "strength", "nostrength",
              "off", "on", "all", "full"]


def current_opt(flags):
    """The unit's current `-opt` value (the base the sweep builds on)."""
    for i, t in enumerate(flags):
        if t == "-opt" and i + 1 < len(flags):
            return flags[i + 1]
    return ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("subs", nargs="*", help="candidates to test (default: all known keywords)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    args = ap.parse_args()

    unit = units.Unit.resolve(args.unit, repo.repo_root())
    scratch = os.path.join(unit.root, "build", "tmp", "probe")
    head, flags, tail = units.split_command(unit)
    base = units.override_flags(flags, args.flags_extra)
    opt = current_opt(base)
    subs = args.subs or CANDIDATES
    print("unit %s   -opt base: %r" % (unit.report_name, opt))

    target = {n: f for n, _, f in units.frames(unit.obj_target)} if os.path.exists(unit.obj_target) else {}
    if target:
        print("  %-24s %s" % ("TARGET", " ".join("%s=%s" % (n, fmt(f)) for n, f in target.items())))

    for sub in subs:
        cand = ("%s,%s" % (opt, sub)) if opt else sub
        rc, log, obj = units.run_tokens(head + units.override_flags(base, "-opt " + cand) + tail, unit.root,
                                        scratch_dir=scratch)
        if rc != 0:
            print("  %-24s COMPILE FAILED %s" % (sub, units.quiet(log).strip().splitlines()[-1:]))
            continue
        ours = units.frames(obj)
        same = target and all(f == target.get(n) for n, _, f in ours)
        print("  %-24s %s%s" % (sub, " ".join("%s=%s" % (n, fmt(f)) for n, _, f in ours),
                                "   <== same frames as target" if same else ""))


def fmt(f):
    return ("-0x%x" % f) if f else "?"


if __name__ == "__main__":
    main()
