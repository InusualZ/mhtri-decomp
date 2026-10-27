#!/usr/bin/env python3
"""Build a one-function translation unit for mwcc_debugger.py: the file's preamble and declarations, the target
function's definition, and every other function reduced to a declaration (or dropped). Line numbers are preserved;
the logic is lib.cextract. Spec: docs/tools/spec/mwcc-debugger.md. CLI: extract.py SOURCE FUNCTION [-o OUT] [--json]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
from pathlib import Path

from tools.lib.cextract import extract


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("source")
    ap.add_argument("function", help="the dump target, plain or mangled")
    ap.add_argument("-o", "--output", help="write the TU here (default: print it)")
    ap.add_argument("--json", action="store_true", help="print what happened to each definition as JSON")
    args = ap.parse_args(argv)
    res = extract(Path(args.source).read_text(encoding="utf-8"), args.function)
    if args.output:
        Path(args.output).write_text(res.text, encoding="utf-8", newline="")
    elif not args.json:
        sys.stdout.write(res.text)
    if args.json:
        print(json.dumps(res.to_dict(), indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
