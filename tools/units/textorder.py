#!/usr/bin/env python3
"""Compare a unit's `.text` function order with the target's and print the first symbols out of order.
Spec: docs/tools/spec/textorder.md. CLI: textorder.py <unit> [--limit N] [--refs]."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import re

from tools.lib.binary import objdump

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = "RMHE08"
LABEL_RE = re.compile(r"^[0-9a-f]+ <(.+)>:")
CALL_RE = re.compile(r"R_PPC_REL24\s+(\S+)")


def run(*args: str) -> str:
    """`objdump <flags> <object>` (the object is the last argument)."""
    return objdump.run_objdump(objdump.locate(ROOT) or "objdump", list(args[:-1]), args[-1]).stdout


def parse_functions(symtab_text: str) -> list[str]:
    """The `.text` function names of an `objdump -t` listing, in address order."""
    out = []
    for line in symtab_text.splitlines():
        q = line.split()
        if len(q) >= 6 and q[-3] == ".text" and " F " in line:
            out.append((int(q[0], 16), q[-1]))
    return [n for _, n in sorted(out)]


def parse_refs(disasm_text: str) -> tuple[list[str], dict[str, list[str]]]:
    """The functions of an `objdump -dr` listing in order, and the `bl` targets each one relocates."""
    cur = None
    order: list[str] = []
    calls: dict[str, list[str]] = {}
    for line in disasm_text.splitlines():
        m = LABEL_RE.match(line)
        if m:
            cur = m.group(1)
            order.append(cur)
            calls[cur] = []
            continue
        m = CALL_RE.search(line)
        if m and cur:
            calls[cur].append(m.group(1))
    return order, calls


def deferred_tags(order: list[str], calls: dict[str, list[str]]) -> list[tuple[int, str, str, list[str]]]:
    """Per function: (index, tag, name, callees in the object). Tag D when an earlier function first references it
    (a deferred-inline candidate), R otherwise."""
    names = set(order)
    first: dict[str, int] = {}
    for i, f in enumerate(order):
        for r in calls[f]:
            if r in names and r != f:
                first.setdefault(r, i)
    rows = []
    for i, f in enumerate(order):
        seen: list[str] = []
        for r in calls[f]:
            if r in names and r != f and r not in seen:
                seen.append(r)
        rows.append((i, "D" if first.get(f, i) < i else "R", f, seen))
    return rows


def compare_order(target: list[str], ours: list[str]) -> tuple[list[int], list[str], list[str]]:
    """(indexes where the common functions disagree, ours-only names, the common names in our order)."""
    ts = set(target)
    extra = [n for n in ours if n not in ts]
    common = [n for n in ours if n in ts]
    bad = [i for i, (x, y) in enumerate(zip(target, common)) if x != y]
    return bad, extra, common


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare a unit's .text function order with the target's.")
    ap.add_argument("unit", help="unit key, e.g. g3d/g3d_anmchr")
    ap.add_argument("--limit", type=int, default=30, help="first N out-of-order positions to print")
    ap.add_argument("--refs", action="store_true", help="print each target function with the target functions it calls")
    a = ap.parse_args()
    tgt = os.path.join(ROOT, "build", GAME, "obj", a.unit + ".o")
    ours = os.path.join(ROOT, "build", GAME, "src", a.unit + ".o")
    if a.refs:
        order, calls = parse_refs(run("-dr", tgt))
        for i, tag, f, seen in deferred_tags(order, calls):
            print(i, tag, f[:50], "->", [r[:28] for r in seen])
        return 0
    t = parse_functions(run("-t", tgt))
    bad, extra, common = compare_order(t, parse_functions(run("-t", ours)))
    print("target %d, ours %d, out of order %d, missing %d" % (len(t), len(common), len(bad), len(t) - len(common)))
    if extra:
        print("ours only:", [n[:50] for n in extra])
    for i in bad[:a.limit]:
        print(i, t[i][:50], "|", common[i][:50])
    return 1 if bad or len(t) != len(common) else 0


if __name__ == "__main__":
    sys.exit(main())
