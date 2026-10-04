#!/usr/bin/env python3
"""Per-function prologue frame size and length of a unit, without objdiff. Spec: docs/tools/spec/frame.md.
CLI: python tools/flags/frame.py [-u <unit>] [--flags-extra "<flags>"] [--obj <o>]... [--versions [<v>...]]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os

from tools.lib import repo, units


def scratch(root):
    return os.path.join(root, "build", "tmp", "probe")


def show(label, path, root):
    print("%s (%s)" % (label, os.path.relpath(path, root)))
    for name, size, frame in units.frames(path):
        print("  %-32s size=%-6d frame=%s" % (
            name, size, ("-0x%x" % frame) if frame else "??"))


def compile_variants(unit, extra, versions):
    """Compile the unit once per compiler version into the scratch dir; return [(label, obj)]."""
    head, flags, tail = units.split_command(unit)
    flags = units.override_flags(flags, extra)
    out = []
    for version in versions:
        head_v = units.with_compiler_version(head, version) if version else head
        rc, log, obj = units.run_tokens(head_v + flags + tail, unit.root, scratch_dir=scratch(unit.root))
        if rc != 0:
            print("%s: COMPILE FAILED\n%s" % (version or "ours", units.quiet(log)[:800]))
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

    root = repo.repo_root()
    if args.obj:
        for p in args.obj:
            show("object", p, root)
        return

    unit = units.Unit.resolve(args.unit, root)
    if args.versions is not None and not args.versions:
        head, _, _ = units.split_command(unit)
        print("\n".join(units.available_versions(head)))
        return
    print("unit %s   flags-extra: %s" % (unit.report_name, args.flags_extra or "(none)"))
    if os.path.exists(unit.obj_target):
        show("target", unit.obj_target, root)
    for label, obj in compile_variants(unit, args.flags_extra, args.versions or [None]):
        show(label or "ours", obj, root)


if __name__ == "__main__":
    main()
