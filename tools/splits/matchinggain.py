#!/usr/bin/env python3
"""matchinggain.py - the `Object(Matching, ...)` units whose data the splits candidate changes.

    python tools/splits/matchinggain.py [--proposal F ...] [--configure configure.py] [--json]
    python tools/splits/matchinggain.py --selftest

A unit registered `Matching` links its own object over the bytes `splits.txt` gives it, so a candidate that gives the unit MORE data
than the registered `splits.txt` does (or less) changes what that object has to define: playbook 23/29 (a source declares its
pool `extern` and never defines it, `flipcheck` refuses a data section that differs). The phase 4 action per unit is "define the
data in the source and re-measure, or demote the unit to NonMatching".

The tool reads the `Object(Matching, "<path>")` rows of `configure.py`, renders the candidate the way `splitcheck --proposal` does
(`dataattach.default_proposals()` + `phase2-reconcile.json`, or the `--proposal` files) and prints, per Matching unit and data
section, the registered bytes, the candidate bytes and the difference; a Matching unit the candidate no longer has under that name
(folded, renamed) is listed as such.  It is read-only.
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))

import splitcheck as sc

DATA_SECTIONS = (".rodata", ".data", ".sdata", ".sdata2", ".bss", ".sbss", ".sbss2")
MATCHING_RE = re.compile(r'Object\(\s*Matching\s*,\s*"([^"]+)"')


def matching_units(text):
    """The paths of every `Object(Matching, "<path>")` row, in file order, once."""
    seen, out = set(), []
    for m in MATCHING_RE.finditer(text):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            out.append(m.group(1))
    return out


def holder(units, addr):
    """The name of the unit whose `.text` holds `addr`, else ""."""
    if addr is None:
        return ""
    for u in units.values():
        for a, b in u.rs(".text"):
            if a <= addr < b:
                return u.name
    return ""


def gains(matching, base_units, cand_units):
    """`(rows, absent)`: a row is `(unit, section, base_bytes, cand_bytes)` for each data section whose size differs; `absent` lists the Matching
    units with no baseline entry or no candidate entry under their name (`(unit, 'baseline'|'candidate')`)."""
    rows, absent = [], []
    for name in matching:
        b, c = base_units.get(name), cand_units.get(name)
        if b is None:
            absent.append((name, "baseline", ""))
            continue
        if c is None:
            absent.append((name, "candidate", holder(cand_units, b.first(".text"))))
            continue
        for sec in DATA_SECTIONS:
            nb, nc = b.size(sec), c.size(sec)
            if nb != nc:
                rows.append((name, sec, nb, nc))
    return rows, absent


def render_candidate(paths):
    splits, symbols, dol = sc.load_ctx()
    cand, _info = sc.render(splits, [sc.load_proposal(p) for p in paths], dol, symbols)
    return splits, cand


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--proposal", action="append", help="proposal file (repeatable); default: phase 1 a..g, reconcile, folds, phase2-reconcile")
    ap.add_argument("--configure", default=None, help="configure.py to read (default: the tree's)")
    ap.add_argument("--json", action="store_true", help="print the rows as JSON")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    import dataattach as da
    paths = args.proposal or da.default_proposals() + [os.path.join(da.PROPOSAL_DIR, "phase2-reconcile.json")]
    conf = args.configure or os.path.join(sc.tree_root(), "configure.py")
    with open(conf, encoding="utf-8") as fh:
        matching = matching_units(fh.read())
    splits, cand = render_candidate(paths)
    rows, absent = gains(matching, splits.by_name(), cand.by_name())
    if args.json:
        print(json.dumps({"matching": len(matching), "rows": [list(r) for r in rows], "absent": [list(a) for a in absent]}, indent=1))
        return 0
    print("%d Matching units; %d with a data section the candidate changes, %d absent under their name" % (
        len(matching), len({r[0] for r in rows}), len(absent)))
    for name, sec, nb, nc in rows:
        print("  %-52s %-8s 0x%X -> 0x%X (%+#x)" % (name, sec, nb, nc, nc - nb))
    for name, where, by in absent:
        print("  %-52s absent from the %s%s" % (name, where, (" (its text is in %s)" % by) if by else ""))
    return 0


def selftest():
    fails = []

    def check(what, got, want):
        if got != want:
            fails.append("%s: got %r, want %r" % (what, got, want))

    text = 'a = [Object(Matching, "x/a.c"), Object(NonMatching, "x/n.c"),\n  Object( Matching , "x/b.cpp"), Object(Matching, "x/a.c")]'
    check("parser: Matching rows only, once, in file order", matching_units(text), ["x/a.c", "x/b.cpp"])

    def unit(name, **secs):
        return sc.Unit(name, "", {s: [(a, b, "")] for s, (a, b) in secs.items()})

    base = {"x/a.c": unit("x/a.c", **{".data": (0x100, 0x128)}), "x/b.cpp": unit("x/b.cpp", **{".sdata": (0x10, 0x18)}),
            "x/n.c": unit("x/n.c", **{".data": (0x300, 0x310)})}
    cand = {"x/a.c": unit("x/a.c", **{".data": (0x100, 0x150), ".sdata2": (0x40, 0x44)}), "x/n.c": unit("x/n.c", **{".data": (0x300, 0x3F0)}),
            "x/big.c": unit("x/big.c", **{".text": (0x1000, 0x2000)})}
    base["x/b.cpp"].ranges[".text"] = [(0x1100, 0x1200, "")]
    rows, absent = gains(["x/a.c", "x/b.cpp", "x/gone.c"], base, cand)
    check("a Matching unit that gains .data and .sdata2 is a row each; NonMatching n.c is not looked at", rows,
          [("x/a.c", ".data", 0x28, 0x50), ("x/a.c", ".sdata2", 0, 4)])
    check("a Matching unit missing from the candidate / the baseline is listed apart, with the unit that swallowed it", absent,
          [("x/b.cpp", "candidate", "x/big.c"), ("x/gone.c", "baseline", "")])
    same = gains(["x/a.c"], {"x/a.c": unit("x/a.c", **{".data": (0, 8)})}, {"x/a.c": unit("x/a.c", **{".data": (0x20, 0x28)})})
    check("equal sizes at different addresses are no gain", same, ([], []))
    for f in fails:
        print("FAIL " + f)
    print("matchinggain selftest: %s (%d failure(s))" % ("FAIL" if fails else "PASS", len(fails)))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
