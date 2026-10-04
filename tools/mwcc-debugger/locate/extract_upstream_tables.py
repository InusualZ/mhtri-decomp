#!/usr/bin/env python3
"""Lift the GC/1.1 and GC/2.6 address tables out of the upstream mwcc_debugger.py as image-relative RVAs.
Spec: docs/tools/spec/mwcc-debugger.md. CLI: extract_upstream_tables.py <upstream/mwcc_debugger.py> [exe]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import ast

from tools.lib.binary.pe import Pe


def image_base(exe: str) -> int:
    return Pe(exe).image_base


def main():
    src = open(sys.argv[1], encoding="utf-8").read()
    base = image_base(sys.argv[2]) if len(sys.argv) > 2 else 0x400000
    tree = ast.parse(src)
    for node in ast.walk(tree):
        if not (isinstance(node, ast.Call) and isinstance(node.func, ast.Name)):
            continue
        if node.func.id != "MwccVersion":
            continue
        kw = {k.arg: k.value for k in node.keywords}
        name = ast.literal_eval(kw["name"])
        print(f"## {name}")
        for field, value in kw.items():
            if field == "name":
                continue
            try:
                val = ast.literal_eval(value)
            except ValueError:
                print(f"    {field} = <unparsed>")
                continue
            if isinstance(val, dict):
                items = ", ".join(f"{k - base:#x}: {v!r}" for k, v in val.items())
                print(f"    {field} = {{{items}}}")
            elif isinstance(val, int) and (field.endswith("_addr") or field.startswith("frame_")):
                print(f"    {field} = {val - base:#x}")
            else:
                print(f"    {field} = {val!r}")


if __name__ == "__main__":
    main()
