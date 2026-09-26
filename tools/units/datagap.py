#!/usr/bin/env python3
"""Per-unit data-section gap between a unit's target object and ours.

A registered unit whose code matches can still differ in *data*: our source defines a constant, a table or a
string pool the original translation unit did not have (MWCC pools its own copy of a floating-point constant,
a string literal lands in `.data`, an array is emitted where retail referenced a map symbol). objdiff's unit
score hides this - the extra section is simply not counted - so a unit can read 100 % fuzzy and still not be
the target object.

This tool compares the two objects' section sizes directly:

    build/RMHE08/obj/<path>.o   the target, split out of the DOL
    build/RMHE08/src/<path>.o   ours, compiled from src/

and reports, per unit:

    ours-extra    ours has bytes in a section the target does not have at all (the usual case: our source
                  defines pooled data the original referenced from elsewhere)
    target-extra  the target has bytes ours does not (a range we failed to claim, or data we dropped)

Usage:
    python tools/units/datagap.py --flip-blockers    # the actionable list: matched code, data-only gap
    python tools/units/datagap.py                    # every unit with data we did not mean to emit
    python tools/units/datagap.py --mode both        # both directions
    python tools/units/datagap.py --unit Pl/fn_8026FFBC
    python tools/units/datagap.py --json out.json    # machine-readable
    python tools/units/datagap.py --selftest

`--flip-blockers` is the list to work: units whose **code already matches** (`fuzzy_match_percent` at or above
`--min-fuzzy`, default 99) and whose only remaining defect is `ours-extra` bytes in a data section. A raw
`ours-extra` listing is noisier than it looks - a unit with unwritten bodies reports `.text` ours-extra too,
which is progress, not a defect. `--mode` selects the direction to report (default `ours-extra`, the direction
that blocks a flip) and `--flip-blockers` narrows it to the data sections. Sections
that only carry metadata (`.comment`, the string/symbol tables, `.note.split`) are ignored unless
`--all-sections` is given; everything else - `.text`, `.data`, `.sdata`, `.sdata2`, `.bss`, `.sbss`,
`.rodata`, `extab`, `extabindex`, `.ctors`, `.dtors` and the `.rela*` sections - is compared.

The finding this tool exists for (2026-09-26): the data gap on the ten units that read `.text` 100 % was
**ours-extra**, not target-extra - e.g. `Pl/fn_8026FFBC` emits an 8-byte `.sdata2` double the target does not
have, and `ef/eft002` / `ef/fn_800FD864` emit 120 / 548 bytes of `.data`. The fix is therefore playbook 29's
rule - reference the map's symbol (`extern`), never define it - not a `splits.txt` claim (there is no range to
claim: the target object has no such section).
"""

from __future__ import annotations

import argparse
import json
import os
import sys

META_SECTIONS = {".comment", ".note.split", ".shstrtab", ".strtab", ".symtab", ".dynsym", ".dynstr"}

# The sections that make a unit's *data* wrong rather than its code unfinished.
DATA_SECTIONS = {".data", ".sdata", ".sdata2", ".rodata", ".bss", ".sbss", ".ctors", ".dtors",
                 ".rela.data", ".rela.sdata", ".rela.sdata2", ".rela.rodata", ".rela.bss",
                 ".rela.ctors", ".rela.dtors"}

# What "carries bytes that matter" means, instead of an allow-list: everything but the metadata tables above.


def section_sizes(path: str) -> dict[str, int]:
    """Section name -> byte size for one ELF object (empty sections are omitted)."""
    from elftools.elf.elffile import ELFFile

    with open(path, "rb") as fh:
        elf = ELFFile(fh)
        return {s.name: s.data_size for s in elf.iter_sections() if s.data_size}


def compare_sections(target: dict[str, int], ours: dict[str, int], all_sections: bool = False):
    """Return (ours_extra, target_extra) as sorted lists of (section, ours_size, target_size).

    Pure function so `--selftest` can exercise it without ELF files.
    """
    names = set(target) | set(ours)
    if not all_sections:
        names -= META_SECTIONS
    extra = []
    missing = []
    for name in sorted(names):
        t, o = target.get(name, 0), ours.get(name, 0)
        if o > t:
            extra.append((name, o, t))
        elif t > o:
            missing.append((name, t, o))
    return extra, missing


def units_from_report(report_path: str):
    """One row per built unit in the objdiff report: name, target object, our object, unit fuzzy percent."""
    with open(report_path, encoding="utf-8") as fh:
        report = json.load(fh)
    out = []
    for unit in report.get("units", []):
        src = (unit.get("metadata") or {}).get("source_path")
        if not src:
            continue
        stem = os.path.splitext(src[len("src/") :] if src.startswith("src/") else src)[0]
        fuzzy = (unit.get("measures") or {}).get("fuzzy_match_percent") or 0.0
        out.append({"unit": unit["name"], "obj": f"build/RMHE08/obj/{stem}.o",
                    "src": f"build/RMHE08/src/{stem}.o", "fuzzy": float(fuzzy)})
    return out


def scan(rows, mode="ours-extra", all_sections=False, min_fuzzy=0.0):
    """Yield (unit, extra, missing) for the units that have a gap in the requested direction.

    `min_fuzzy` above 0 restricts the scan to units whose code already matches (a flip blocker): there the only
    thing left to report is the data, so only data sections are returned.
    """
    for row in rows:
        name, obj_path, src_path = row["unit"], row["obj"], row["src"]
        if not (os.path.exists(obj_path) and os.path.exists(src_path)):
            print(f"skip {name}: object missing ({obj_path if not os.path.exists(obj_path) else src_path})",
                  file=sys.stderr)
            continue
        extra, missing = compare_sections(section_sizes(obj_path), section_sizes(src_path), all_sections)
        if min_fuzzy > 0:
            if row["fuzzy"] < min_fuzzy:
                continue
            extra = [e for e in extra if e[0] in DATA_SECTIONS]
            missing = [m for m in missing if m[0] in DATA_SECTIONS or m[0] in (".ctors", ".dtors")]
            if not (extra or missing):
                continue
        elif mode == "ours-extra" and not extra:
            continue
        elif mode == "target-extra" and not missing:
            continue
        elif not (extra or missing):
            continue
        yield name, extra, missing


def selftest() -> int:
    checks = 0

    def eq(got, want, what):
        nonlocal checks
        checks += 1
        assert got == want, f"{what}: got {got!r}, want {want!r}"

    target = {".text": 92, ".comment": 124}
    ours = {".text": 92, ".comment": 132, ".sdata2": 8}
    extra, missing = compare_sections(target, ours)
    eq(extra, [(".sdata2", 8, 0)], "ours-extra finds the pooled double")
    eq(missing, [], "ours-extra does not invent a target section")
    eq(compare_sections(target, ours, all_sections=True)[0].count((".comment", 132, 124)), 1,
       "--all-sections reports metadata churn")

    # The reverse direction: a target range we have not claimed.
    t2 = {".text": 100, ".data": 64}
    o2 = {".text": 100}
    extra2, missing2 = compare_sections(t2, o2)
    eq(extra2, [], "target-extra has nothing ours-extra")
    eq(missing2, [(".data", 64, 0)], "target-extra finds the unclaimed range")

    # Equal sizes are not a gap, and a smaller section of ours is a target-extra.
    eq(compare_sections({".text": 10, ".data": 8}, {".text": 10, ".data": 4}), ([], [(".data", 8, 4)]),
       "smaller ours is target-extra")
    eq(compare_sections({".text": 10}, {".text": 10}), ([], []), "identical sections are silent")

    # pure-function property: sorted, never both lists for one section
    for name, t, o in [(".data", 0, 120), (".data", 120, 0), (".data", 1, 1)]:
        e, m = compare_sections({name: t}, {name: o})
        eq(len(e) + len(m) <= 1, True, f"one direction only for {t}/{o}")

    print(f"datagap selftest: {checks} checks OK")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", default="build/RMHE08/report.json")
    ap.add_argument("--unit", action="append", default=[], help="only these units (substring match on the name)")
    ap.add_argument("--mode", choices=["ours-extra", "target-extra", "both"], default="ours-extra")
    ap.add_argument("--all-sections", action="store_true", help="include .comment / .strtab / .symtab churn")
    ap.add_argument("--flip-blockers", action="store_true",
                    help="only units whose code matches and whose gap is data-only (see --min-fuzzy)")
    ap.add_argument("--min-fuzzy", type=float, default=99.0, help="code-match floor for --flip-blockers")
    ap.add_argument("--json", help="write the full scan to this file")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if not os.path.exists(args.report):
        print(f"no report at {args.report} - run `ninja build/RMHE08/report.json` first", file=sys.stderr)
        return 2

    pairs = units_from_report(args.report)
    if args.unit:
        pairs = [p for p in pairs if any(u in p["unit"] for u in args.unit)]
    rows = list(scan(pairs, args.mode, args.all_sections,
                     min_fuzzy=args.min_fuzzy if args.flip_blockers else 0.0))

    for name, extra, missing in rows:
        bits = []
        if extra:
            bits.append("ours-extra " + ", ".join(f"{s} {o}B (target {t}B)" for s, o, t in extra))
        if missing:
            bits.append("target-extra " + ", ".join(f"{s} {t}B (ours {o}B)" for s, t, o in missing))
        print(f"{name}: " + "; ".join(bits))
    what = f"data-only gap at or above {args.min_fuzzy:g} % code" if args.flip_blockers else f"{args.mode} gap"
    print(f"\n{len(rows)} unit(s) with a {what}, out of {len(pairs)} scanned")

    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump([{"unit": n, "ours_extra": e, "target_extra": m} for n, e, m in rows], fh, indent=1)
        print(f"wrote {args.json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
