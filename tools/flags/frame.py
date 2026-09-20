#!/usr/bin/env python3
"""Per-function prologue frame size and length of a unit, without objdiff.

Compiles the unit with the exact ninja command line (optionally with `--flags-extra` overrides) into a
scratch object, decodes each function's prologue `stwu r1,-N(r1)` and prints it next to the target
object's frame, so "one extra 4-byte local" differences are visible immediately.

Usage:
    python tools/flags/frame.py                          # the only unit in the repo
    python tools/flags/frame.py -u <unit>                # any unit
    python tools/flags/frame.py -u <unit> --flags-extra "-O3 -inline noauto"
    python tools/flags/frame.py --obj build/<version>/obj/<Lib>/<file>.o   # read an existing object
    python tools/flags/frame.py --versions 1.3 1.5       # try several compilers
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu

SCRATCH = os.path.join(uu.ROOT, "build", "tmp", "probe")


def show(label, path):
    print("%s (%s)" % (label, os.path.relpath(path, uu.ROOT)))
    for name, size, frame in uu.frames(path):
        print("  %-32s size=%-6d frame=%s" % (
            name, size, ("-0x%x" % frame) if frame else "??"))


def compile_variants(unit, extra, versions):
    """Compile the unit once per compiler version into the scratch dir; return [(label, obj)]."""
    head, flags, tail = uu.split_flags(uu.compile_command(unit))
    flags = uu.override_flags(flags, extra)
    out = []
    for version in versions:
        head_v = uu.with_compiler_version(head, version) if version else head
        rc, log, obj = uu.run_compile(head_v + flags + tail, scratch_dir=SCRATCH)
        if rc != 0:
            print("%s: COMPILE FAILED\n%s" % (version or "ours", uu.quiet(log)[:800]))
            continue
        out.append((version, obj))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--obj", action="append", default=[],
                    help="read this object instead of compiling (repeatable)")
    ap.add_argument("--versions", nargs="*", default=None,
                    help="compiler versions to try (no value: list the installed ones)")
    args = ap.parse_args()

    if args.obj:
        for p in args.obj:
            show("object", p)
        return

    unit = uu.resolve_unit(args.unit)
    if args.versions is not None and not args.versions:
        head, _, _ = uu.split_flags(uu.compile_command(unit))
        print("\n".join(uu.available_versions(head)))
        return
    print("unit %s   flags-extra: %s" % (unit.name, args.flags_extra or "(none)"))
    if os.path.exists(unit.target):
        show("target", unit.target)
    for label, obj in compile_variants(unit, args.flags_extra, args.versions or [None]):
        show(label or "ours", obj)


if __name__ == "__main__":
    main()
