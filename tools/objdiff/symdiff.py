#!/usr/bin/env python3
"""Side-by-side instruction diff for one symbol of a unit.

Usage:
    python tools/objdiff/symdiff.py -u <unit> <symbol> [n] [--all]        # runs objdiff for you
    python tools/objdiff/symdiff.py <diff.json> <symbol> [n] [--all]      # reuse an existing diff

Left  = the -1 object, Right = the -2 object (file mode), or target/base in project mode
(`-p . -u <unit>`, where left = target and right = our build).

objdiff's per-symbol `match_percent` is positional: one inserted/deleted
instruction shifts every later instruction, so a single early divergence can
report ~0 %. Read the first divergence, not the percentage.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu


def cli():
    """(diff json path, symbol), from either `-u <unit> <symbol>` or `<diff.json> <symbol>`."""
    a = sys.argv[1:]
    for flag in ("-u", "--unit"):
        if flag in a:
            i = a.index(flag)
            rest = a[:i] + a[i + 2:]
            unit = uu.resolve_unit(a[i + 1])
            symbol = rest[0]
            path, log = uu.objdiff(unit, symbol)
            if not path:
                raise SystemExit("objdiff failed: " + log)
            sys.argv = [sys.argv[0], path, symbol] + rest[1:]
            return path, symbol
    if len(a) < 2:
        raise SystemExit("usage: symdiff.py -u <unit> <symbol> [n] [--all]"
                         " | symdiff.py <diff.json> <symbol> [n] [--all]")
    return a[0], a[1]


def norm(d, side):
    """Sections dict with symbols normalized to the old nested shape.

    objdiff-cli v3.6.1 (pinned in configure.py) emits a flat top-level `symbols[]`
    array and only when a symbol argument was passed; older builds nested the
    entries under `sections[].symbols[]` with a `symbol` object.
    """
    syms = d[side].get("symbols")
    if not syms:
        return {s["name"]: s for s in d[side]["sections"]}
    out = {s["name"]: dict(s) for s in d[side]["sections"]}
    out.setdefault(".text", {"name": ".text"})["symbols"] = [
        {"symbol": {"name": e.get("name"), "size": e.get("size")},
         "instructions": e.get("instructions") or [],
         "match_percent": e.get("match_percent")} for e in syms]
    return out


def load(path):
    d = json.load(open(path))
    return norm(d, "left"), norm(d, "right")


def find(section, name):
    for e in section.get("symbols", []):
        if e["symbol"].get("name") == name:
            return e
    return None


def fmt(ins):
    if ins is None:
        return "--"
    i = ins.get("instruction")
    return i["formatted"] if i else "<" + str(ins.get("diff_kind", "?")) + ">"


def kind(ins):
    if ins is None:
        return "MISSING"
    if "instruction" not in ins:
        return "PH"
    k = ins.get("diff_kind")
    if k == "DIFF_ARG_MISMATCH":
        return "ARG"  # opcode matches, only an argument differs
    if k in (None, "DIFF_NONE"):
        return "=="
    return k.replace("DIFF_", "")


def main():
    path, name = cli()
    n = int(sys.argv[3]) if len(sys.argv) > 3 and sys.argv[3].isdigit() else 30
    show_all = "--all" in sys.argv
    left, right = load(path)
    for sec in (".text", ".rodata", ".data"):
        if sec not in left:
            continue
        le = find(left[sec], name)
        if not le:
            continue
        re_ = find(right.get(sec, {}), name)
        li = le.get("instructions") or []
        ri = (re_.get("instructions") or []) if re_ else []
        print(f"== {sec} {name}: target {le['symbol'].get('size')} B / {len(li)} ins, "
              f"ours {re_['symbol'].get('size') if re_ else '?'} B / {len(ri)} ins, "
              f"match {le.get('match_percent')}")
        print(f"{'i':>5} {'T':>4} {'kind':<12} {'target':<40} | {'ours':<40} ours-kind")
        limit = max(len(li), len(ri)) if show_all else min(n, max(len(li), len(ri)))
        for i in range(limit):
            l = li[i] if i < len(li) else None
            r = ri[i] if i < len(ri) else None
            print(f"{i:>5} {kind(l):>4} {kind(l):<12} {fmt(l):<40} | {fmt(r):<40} {kind(r)}")
        return
    print(f"symbol {name!r} not found")


if __name__ == "__main__":
    main()
