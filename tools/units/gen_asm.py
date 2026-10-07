#!/usr/bin/env python3
"""Emit MWCC `asm void f(void) { nofralloc ... }` source from a unit's TARGET object, one function or all.
Spec: docs/tools/spec/gen_asm.md. CLI: gen_asm.py <unit> [<symbol>...] [--obj PATH] [--list] [-o FILE]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse

from tools.lib import asmgen, repo, units
from tools.lib.binary import objdump


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", help="unit spec (lib/file, with or without the extension)")
    ap.add_argument("symbols", nargs="*", help="functions to emit (default: every function of the object)")
    ap.add_argument("--obj", help="an object file to read instead of the unit's target object")
    ap.add_argument("--list", action="store_true", help="print the function names and exit")
    ap.add_argument("-o", "--out", help="write the source here instead of stdout")
    args = ap.parse_args(argv)

    root = repo.repo_root()
    obj = args.obj or units.Unit.resolve(args.unit, root).obj_target
    tool = objdump.locate(root)
    if tool is None:
        print("gen_asm: no objdump (build/binutils); run `slots.py seed-worktree` or `ninja tools`", file=sys.stderr)
        return 2
    try:
        text = objdump.disassemble(tool, obj, relocs=True, cpu=asmgen.CPU)
    except RuntimeError as exc:
        print("gen_asm: %s" % exc, file=sys.stderr)
        return 2
    if args.list:
        print("\n".join(asmgen.parse(text)))
        return 0
    source, missing = asmgen.generate(text, args.symbols or None)
    for name in missing:
        print("gen_asm: no function %s in %s" % (name, obj), file=sys.stderr)
    if args.out:
        with open(args.out, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(source)
    else:
        sys.stdout.write(source)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
