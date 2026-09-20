#!/usr/bin/env python3
"""r1-relative stack-slot map for one symbol of an objdiff-cli file diff.

Usage:
    python tools/objdiff/slotmap.py -u <unit> <symbol> [--map] [--slot 0xc8] [--around 700,730]
    python tools/objdiff/slotmap.py <diff.json> <symbol> [--map] [--slot 0xc8] [--around 700,730]

Invocation (file mode): `objdiff-cli diff -1 <ours> -2 <target> <symbol> --format json -o f.json`,
i.e. **left = the -1 object (ours)**, right = the -2 object (target); pass -1/-2 the other way round
and the TARGET/OURS labels in the output swap.  Because the two instruction streams are identical
except for the r1 offsets, the offset mapping is recovered by index-aligned majority vote.
"""
import json
import os
import re
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
        raise SystemExit("usage: slotmap.py -u <unit> <symbol> [--map]"
                         " | slotmap.py <diff.json> <symbol> [--map]")
    return a[0], a[1]

OFF = re.compile(r"(-?0x[0-9a-f]+)\(r1\)")
LOAD = re.compile(r"^(lwz|lbz|lhz|lha|lfs|lfd|lwzu|lmw)\b")
STORE = re.compile(r"^(stw|stb|sth|stfs|stfd|stwu|stmw)\b")


def load(path):
    """Symbol entry of both sides, handling both objdiff JSON layouts.

    objdiff-cli v3.6.1 (the version pinned in configure.py) emits a flat top-level
    `symbols[]` array and only when a symbol argument is passed; older builds nested
    the entries under `sections[].symbols[]` with a `symbol` object.
    """
    d = json.load(open(path))
    out = {}
    for side in ("left", "right"):  # left = ours (-1), right = target (-2)
        syms = d[side].get("symbols")
        if syms:
            cands = [{"symbol": {"name": e.get("name"), "size": e.get("size")},
                      "instructions": e.get("instructions") or []} for e in syms]
        else:
            cands = [e for s in d[side]["sections"] for e in (s.get("symbols") or [])]
        for e in cands:
            if e["symbol"]["name"] == sys.argv[2]:
                out[side] = e
    return out


def offsets(e):
    """[(index, offset, 'r'|'w')] for every r1-relative memory access."""
    res = []
    for i, ins in enumerate(e.get("instructions") or []):
        f = (ins.get("instruction") or {}).get("formatted")
        if not f:
            continue
        m = OFF.search(f)
        if not m:
            continue
        mnem = f.split()[0]
        kind = "w" if STORE.match(mnem) else ("r" if LOAD.match(mnem) else "a")
        res.append((i, int(m.group(1), 16), kind))
    return res


def main():
    cli()
    d = load(sys.argv[1])
    o, t = offsets(d["left"]), offsets(d["right"])   # left = ours, right = target
    print("target accesses %d, ours %d, target insns %d, ours %d"
          % (len(t), len(o), len(d["left"]["instructions"]), len(d["right"]["instructions"])))
    votes = {}
    for (i1, a1, k1), (i2, a2, k2) in zip(t, o):
        if i1 != i2 or k1 != k2:
            print("  !! misaligned at idx %d/%d" % (i1, i2))
            continue
        votes.setdefault(a1, {}).setdefault(a2, 0)
        votes[a1][a2] += 1
    if "--map" in sys.argv:
        print("\n== offset map (target -> ours, votes)")
        for a in sorted(votes):
            best = sorted(votes[a].items(), key=lambda x: -x[1])
            extra = " " + str(best[1:]) if len(best) > 1 else ""
            print("  0x%-5x -> 0x%-5x (%d)%s" % (a, best[0][0], best[0][1], extra))
    if "--slot" in sys.argv:
        want = {int(x, 16) for x in sys.argv[sys.argv.index("--slot") + 1].split(",")}
        for label, seq in (("TARGET", t), ("OURS", o)):
            print("\n== %s slot access timeline" % label)
            for a in sorted(want):
                acc = [(i, k) for i, b, k in seq if b == a]
                if not acc:
                    print("  0x%-5x  (unused)" % a)
                    continue
                runs, cur = [], [acc[0]]
                for it in acc[1:]:
                    if it[0] - cur[-1][0] <= 3 and it[1] == cur[-1][1]:
                        cur.append(it)
                    else:
                        runs.append(cur)
                        cur = [it]
                runs.append(cur)
                desc = ", ".join("%s%d-%d" % (c[0][1], c[0][0], c[-1][0]) for c in runs)
                print("  0x%-5x  %d accesses: %s" % (a, len(acc), desc))
    if "--around" in sys.argv:
        lo, hi = (int(x) for x in sys.argv[sys.argv.index("--around") + 1].split(","))
        print("%4s  %-46s | %s" % ("idx", "TARGET", "OURS"))
        for i in range(lo, hi):
            ti = (d["right"]["instructions"][i].get("instruction") or {}).get("formatted")
            oi = (d["left"]["instructions"][i].get("instruction") or {}).get("formatted")
            mark = "  " if ti == oi else "->"
            print("%s %4d  %-46s | %s" % (mark, i, ti, oi))


main()
