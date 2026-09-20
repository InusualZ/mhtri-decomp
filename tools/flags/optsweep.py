#!/usr/bin/env python3
"""Sweep `-opt` sub-options for a unit and print the resulting frame sizes.

For every candidate sub-option it recompiles the unit with `-opt <current>,<sub>` (the unit's own `-opt`
value is kept as the base) into a scratch object, and prints `.text` size plus each function's prologue
frame next to the target object's frame. This is the fast filter for "does this option change stack
allocation at all?" - confirm anything interesting with `mwcc_matrix.py`.

Usage:
    python tools/flags/optsweep.py                                  # all candidates, the only unit
    python tools/flags/optsweep.py -u <unit>                        # any unit
    python tools/flags/optsweep.py -u <unit> lifetimes nodeadstore  # only these candidates
    python tools/flags/optsweep.py -u <unit> --flags-extra "-O3 -inline noauto"
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu

SCRATCH = os.path.join(uu.ROOT, "build", "tmp", "probe")

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

    unit = uu.resolve_unit(args.unit)
    head, flags, tail = uu.split_flags(uu.compile_command(unit))
    base = uu.override_flags(flags, args.flags_extra)
    opt = current_opt(base)
    subs = args.subs or CANDIDATES
    print("unit %s   -opt base: %r" % (unit.name, opt))

    target = {n: f for n, _, f in uu.frames(unit.target)} if os.path.exists(unit.target) else {}
    if target:
        print("  %-24s %s" % ("TARGET", " ".join("%s=%s" % (n, fmt(f)) for n, f in target.items())))

    for sub in subs:
        cand = ("%s,%s" % (opt, sub)) if opt else sub
        rc, log, obj = uu.run_compile(head + uu.override_flags(base, "-opt " + cand) + tail,
                                      scratch_dir=SCRATCH)
        if rc != 0:
            print("  %-24s COMPILE FAILED %s" % (sub, uu.quiet(log).strip().splitlines()[-1:]))
            continue
        ours = uu.frames(obj)
        same = target and all(f == target.get(n) for n, _, f in ours)
        print("  %-24s %s%s" % (sub, " ".join("%s=%s" % (n, fmt(f)) for n, _, f in ours),
                                "   <== same frames as target" if same else ""))


def fmt(f):
    return ("-0x%x" % f) if f else "?"


if __name__ == "__main__":
    main()
