#!/usr/bin/env python3
"""Relocation-level diff of one unit: the target object's relocations against ours, on both sides.

    python tools/objdiff/relocdiff.py g3d/fn_8005AA28
    python tools/objdiff/relocdiff.py --unit Pl/pl_act --unit menu/menu_note
    python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --section .text --rows 0
    python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --json r.json
    python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --check      # exit 1 when a relocation differs
    python tools/objdiff/relocdiff.py Network/network_state --by-owner   # compact, robust to moved code
    python tools/objdiff/relocdiff.py --selftest

**Two pairings.** The default view pairs by section offset, so a function that moved (or one inserted
instruction) makes every later relocation read as target-only + ours-only. `--by-owner` groups the
relocations by the symbol that contains them and aligns each group in order: it prints only the real
differences (a wrong name, type or addend, an inserted or dropped relocation, a symbol on one side only)
and a non-failing `note` when only the offsets moved. Use `--by-owner` on a unit with residual `.text`
differences; use the default view for the both-sides tables.

**Why this tool exists.** objdiff scores a *relocation-name* mismatch as equal - this project runs it
with `functionRelocDiffs=none`, so a `bl`/`lis`/`addi` to the wrong symbol of the same shape reads
100 % - and a byte diff cannot see it either, because the relocated field holds the same value. Three
separate lanes hand-rolled this comparison in throwaway scripts before it was shipped. The case that
matters is a unit whose `.text` is byte-identical to the target's while its relocations are not: this
tree holds 16 of them (e.g. `g3d/fn_8005AA28`, `menu/menu_note`, `Pl/pl_act`, `Pl/fn_80229ECC`), every
one scored 100.00000 % by `unitscore.py`.

**What it prints,** for each section that either side relocates:

* both sides' relocations - section offset, target symbol name, relocation type, addend - merged on the
  offset so the two sides read across;
* the diff, in the four classes a lane acts on:
    (a) relocations **only the target** has;
    (b) relocations **only ours** has;
    (c) the **same offset pointing at a different symbol** (the objdiff-invisible class);
    (d) the **same symbol with a different type or addend**;
* an explicit line when the two sets are **identical** - the answer most of the time, and the thing that
  makes "byte- and relocation-identical" checkable rather than assumed.

The unit's two object paths and their mtimes are named in the header (so a stale read is visible), and a
prebuilt object older than a source in the unit's include closure is flagged with `stale` (the same rule
`tools/objdiff/freshguard.py` enforces for the scorers).

**Reuse, not re-implementation.** The object is read by `tools/units/dossier.py`'s `parse_elf` - the
project's ELF reader, already used by `callees.py` - which carries the RELA addend; the relocation type
names come from the same module; the unit and its paths come from `tools/unitutil.py`; the staleness
arithmetic is `tools/objdiff/freshguard.py`. This tool is the section-relocation *view*; `sectiongap.py`
stays the section size/byte view and pairs relocations by name, while this one pairs by offset and
carries addends. It deliberately does not decode an instruction's field, resolve a symbol in the link,
or judge what `splits.txt` claims.
"""
from __future__ import annotations

import argparse
import json
import os
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))                 # tools/objdiff
TOOLS = os.path.dirname(HERE)                                     # tools/
for _path in (TOOLS, os.path.join(TOOLS, "units"), os.path.join(TOOLS, "elf"), HERE):
    if _path not in sys.path:
        sys.path.insert(0, _path)

import unitutil as uu                                             # noqa: E402
import freshguard                                                 # noqa: E402
from units import dossier                                         # noqa: E402


# --------------------------------------------------------------------------------------------------
# reading and the pure diff rule
# --------------------------------------------------------------------------------------------------

def read_relocs(path: str) -> tuple[dict[str, list[tuple[int, str, int, int]]] | None, str | None]:
    """`{section: [(offset, symbol, type, addend), ...]}` for one object, or `(None, why)`.

    `None` (not `{}`) distinguishes "unreadable/not an object" from "an object with no relocations", so
    a missing build artefact is reported as unbuilt rather than as a falsely-clean unit.
    """
    try:
        with open(path, "rb") as fh:
            blob = fh.read()
    except OSError as exc:
        return None, "cannot read %s: %s" % (path, exc)
    try:
        _sections, _symbols, relocs = dossier.parse_elf(blob)
    except ValueError as exc:
        return None, "%s is not an ELF object: %s" % (path, exc)
    out: dict[str, list[tuple[int, str, int, int]]] = {}
    for r in relocs:
        section = r.get("target") or r.get("section") or "?"
        out.setdefault(section, []).append((r["offset"], r.get("symbol") or "", r["type"],
                                            r.get("addend", 0)))
    for rows in out.values():
        rows.sort()
    return out, None


def type_name(typ: int) -> str:
    """`R_PPC_REL24` for a relocation kind this project emits, `R_PPC_<n>` otherwise."""
    return dossier.RELOC_TYPES.get(typ, "R_PPC_%d" % typ)


def diff_relocs(ours: list, target: list) -> dict:
    """The four-class diff of two relocation lists, pure so the selftest drives it with tuples.

    Identity first (offset + symbol + type + addend, as a multiset), so an unchanged relocation is
    never reported; what is left is paired **by offset**, which is what makes class (c) - the same
    offset pointing at a different symbol - a single fact rather than two unrelated add/remove lines.
    """
    o, t = Counter(map(tuple, ours)), Counter(map(tuple, target))
    identical = sum((o & t).values())
    o_by, t_by = {}, {}
    for off, sym, typ, add in (o - t).elements():
        o_by.setdefault(off, []).append((sym, typ, add))
    for off, sym, typ, add in (t - o).elements():
        t_by.setdefault(off, []).append((sym, typ, add))
    only_target, only_ours, diffsym, diffattr = [], [], [], []
    for off in sorted(set(o_by) | set(t_by)):
        mine, theirs = o_by.get(off, []), t_by.get(off, [])
        while mine and theirs:
            a, b = mine.pop(0), theirs.pop(0)
            if a[0] != b[0]:
                diffsym.append((off, a, b))              # (c) same offset, different symbol
            else:
                diffattr.append((off, a, b))             # (d) same symbol, different type/addend
        only_ours.extend((off,) + x for x in mine)
        only_target.extend((off,) + x for x in theirs)
    return {"ours": len(ours), "target": len(target), "matched": identical,
            "only_target": only_target, "only_ours": only_ours,
            "different_symbol": diffsym, "different_attr": diffattr,
            "identical": not (only_target or only_ours or diffsym or diffattr)}


# --------------------------------------------------------------------------------------------------
# the record: one unit, both objects
# --------------------------------------------------------------------------------------------------

def unit_record(spec: str, sections: list[str] | None = None) -> dict:
    """Resolve the unit, read both objects' relocations, and diff every section they relocate."""
    unit = uu.resolve_unit(spec)
    rec = {"unit": unit.name, "stem": spec, "ours": unit.obj, "target": unit.target,
           "sections": [], "error": None, "stale": []}
    if not os.path.exists(unit.obj):
        rec["error"] = "our object does not exist (%s) - build it (`ninja %s`)" % (unit.obj, spec)
        return rec
    if not os.path.exists(unit.target):
        rec["error"] = ("the split target object does not exist (%s) - split the unit first "
                        "(`python configure.py && ninja`)" % unit.target)
        return rec
    src = unit.src if os.path.isabs(unit.src) else os.path.join(uu.ROOT, unit.src)
    rec["stale"] = freshguard.unit_reasons(src, unit.obj, uu.ROOT,
                                           rel=lambda p: freshguard.rel_path(p, uu.ROOT))[0]
    ours, err = read_relocs(unit.obj)
    if err:
        rec["error"] = err
        return rec
    target, err = read_relocs(unit.target)
    if err:
        rec["error"] = err
        return rec
    names = sorted(set(ours) | set(target))
    if sections:
        names = [n for n in names if n in set(sections)]
    for name in names:
        rec["sections"].append({"section": name, "diff": diff_relocs(ours.get(name, []),
                                                                     target.get(name, [])),
                                "ours": sorted(ours.get(name, [])),
                                "target": sorted(target.get(name, []))})
    return rec


def owner_groups(blob: bytes) -> dict:
    """`{(section, owner): [(offset_in_owner, type, symbol, addend), ...]}` in offset order.

    The owner is the defined symbol containing the relocated offset; a relocation with none (extabindex,
    padding) groups under `(section, None)` at its absolute offset."""
    _sections, symbols, relocs = dossier.parse_elf(blob)
    table: dict[str, list[dict]] = {}
    for s in symbols:
        if s["name"] and s["section"] and s["type"] in (0, 1, 2):
            table.setdefault(s["section"], []).append(s)
    for lst in table.values():
        lst.sort(key=lambda s: (s["value"], -s["size"]))
    out: dict = {}
    for r in relocs:
        own = None
        for s in table.get(r["target"], ()):
            if s["value"] > r["offset"]:
                break
            if r["offset"] < s["value"] + max(s["size"], 1):
                own = s
        key = (r["target"], own["name"] if own else None)
        off = r["offset"] - own["value"] if own else r["offset"]
        out.setdefault(key, []).append((off, r["type_name"], r["symbol"] or "<section-symbol>", r["addend"]))
    for lst in out.values():
        lst.sort()
    return out


def compare_by_owner(ours: bytes, target: bytes) -> tuple[int, int, list[str]]:
    """`(matching, total, lines)`: relocations aligned in ORDER within each owning symbol.

    A function that moved, or an instruction that slid a few bytes, is not a difference (the offset-paired
    view above drowns in those); only a changed type/name/addend or an inserted/dropped relocation is. Same
    names at different offsets is a `note` line, which never fails. A symbol present on one side only is
    one line. Relocations against a section symbol have no name and compare as `<section-symbol>`."""
    import difflib
    a, b = owner_groups(ours), owner_groups(target)
    lines: list[str] = []
    notes: list[str] = []
    matched = 0
    for key in sorted(set(a) | set(b), key=lambda k: (k[0] or "", k[1] or "")):
        sec, own = key
        x, y = a.get(key, []), b.get(key, [])
        if own and not x:
            lines.append("only in target  %s %s: absent from ours (%d relocation(s))" % (sec, own, len(y)))
            continue
        if own and not y:
            lines.append("only in ours    %s %s: absent from the target (%d relocation(s))" % (sec, own, len(x)))
            continue
        where = "%s %s" % (sec, own) if own else sec
        sm = difflib.SequenceMatcher(None, [i[1:] for i in x], [i[1:] for i in y], autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == "equal":
                matched += i2 - i1
                if [i[0] for i in x[i1:i2]] != [i[0] for i in y[j1:j2]]:
                    notes.append("note            %s: %d relocation(s) at different offsets, same names"
                                 % (where, i2 - i1))
                continue
            for k in range(max(i2 - i1, j2 - j1)):
                xi = x[i1 + k] if i1 + k < i2 else None
                yi = y[j1 + k] if j1 + k < j2 else None
                at = "+0x%x" % (xi or yi)[0]
                if xi and yi:
                    what = []
                    if xi[1] != yi[1]:
                        what.append("type %s vs %s" % (xi[1], yi[1]))
                    if xi[2] != yi[2]:
                        what.append("symbol %s vs %s" % (xi[2], yi[2]))
                    if xi[3] != yi[3]:
                        what.append("addend %+d vs %+d" % (xi[3], yi[3]))
                    lines.append("differs         %s%s: %s  (ours vs target)" % (where, at, "; ".join(what)))
                elif xi:
                    lines.append("extra in ours   %s%s: ours has %s %s%+d" % (where, at, xi[1], xi[2], xi[3]))
                else:
                    lines.append("missing in ours %s%s: target has %s %s%+d" % (where, at, yi[1], yi[2], yi[3]))
    total = max(sum(map(len, a.values())), sum(map(len, b.values())))
    return matched, total, lines + notes


def run_by_owner(units: list[str]) -> int:
    """`--by-owner`: differences only, then one `N/N relocations match` line per unit. 0 clean, 1 differs, 2 error."""
    status = 0
    for spec in units:
        unit = uu.resolve_unit(spec)
        missing = [p for p in (unit.obj, unit.target) if not os.path.exists(p)]
        if missing:
            print("%s: object missing (%s) - build/split it first" % (spec, missing[0]))
            status = max(status, 2)
            continue
        try:
            with open(unit.obj, "rb") as fo, open(unit.target, "rb") as ft:
                matched, total, lines = compare_by_owner(fo.read(), ft.read())
        except (ValueError, OSError) as exc:
            print("%s: unreadable object (%s)" % (spec, exc))
            status = max(status, 2)
            continue
        for line in lines:
            print("  " + line)
        print("%s: %d/%d relocations match" % (spec, matched, total))
        if any(not ln.startswith("note") for ln in lines):
            status = max(status, 1)
    return status


def unit_identical(rec: dict) -> bool:
    """True when every compared section's relocation sets are identical (an error is not identical)."""
    return not rec["error"] and all(s["diff"]["identical"] for s in rec["sections"])


# --------------------------------------------------------------------------------------------------
# rendering
# --------------------------------------------------------------------------------------------------

def _addend(a: int) -> str:
    return "%+d" % a if a else "0"


def _reloc(row: tuple[int, str, int, int]) -> str:
    off, sym, typ, add = row
    return "+0x%-4X  %-20s  %-14s  %s" % (off, sym, type_name(typ), _addend(add))


def _dump(rows: list, limit: int | None) -> list[str]:
    """The two side block is printed merged on the offset; `limit` caps the row count per side."""
    shown = rows if limit is None else rows[:limit]
    lines = ["    " + _reloc(r) for r in shown]
    if limit is not None and len(rows) > limit:
        lines.append("    ... %d more relocation(s) (%d total; --all prints them)"
                     % (len(rows) - limit, len(rows)))
    return lines


def _diff_lines(d: dict, indent: str = "  ") -> list[str]:
    """The four classes, each named, and the identical line when there is nothing to report."""
    out: list[str] = []
    if d["only_target"]:
        out.append("%s(a) %d relocation(s) ONLY the target has:" % (indent, len(d["only_target"])))
        out += [indent + "    " + _reloc(r) for r in d["only_target"]]
    if d["only_ours"]:
        out.append("%s(b) %d relocation(s) ONLY ours has:" % (indent, len(d["only_ours"])))
        out += [indent + "    " + _reloc(r) for r in d["only_ours"]]
    if d["different_symbol"]:
        out.append("%s(c) %d offset(s) pointing at a DIFFERENT SYMBOL:" % (indent,
                                                                          len(d["different_symbol"])))
        for off, a, b in d["different_symbol"]:
            out.append("%s    +0x%X  ours `%s` (%s) vs target `%s` (%s)"
                       % (indent, off, a[0], type_name(a[1]), b[0], type_name(b[1])))
    if d["different_attr"]:
        out.append("%s(d) %d same symbol, DIFFERENT TYPE or ADDEND:" % (indent,
                                                                        len(d["different_attr"])))
        for off, a, b in d["different_attr"]:
            out.append("%s    +0x%X  `%s`  ours %s %s vs target %s %s"
                       % (indent, off, a[0], type_name(a[1]), _addend(a[2]), type_name(b[1]),
                          _addend(b[2])))
    if d["identical"]:
        out.append("%srelocation sets IDENTICAL - %d relocation(s), the same offsets, symbols, types "
                   "and addends" % (indent, d["ours"]))
    return out


def render(rec: dict, rows: int | None = 60) -> None:
    """The human report: the unit's paths and mtimes, then each section's relocations and its diff."""
    rel = lambda p: freshguard.rel_path(p, uu.ROOT)               # noqa: E731
    print("== %s" % rec["unit"])
    print("   ours    %-54s %s" % (rel(rec["ours"]), freshguard.stamp(freshguard.mtime(rec["ours"]))))
    print("   target  %-54s %s" % (rel(rec["target"]),
                                   freshguard.stamp(freshguard.mtime(rec["target"]))))
    for reason in rec["stale"]:
        print("   stale   " + reason)
    if rec["error"]:
        print("   error   " + rec["error"])
        return
    if not rec["sections"]:
        print("   no section of either object carries a relocation")
        return
    for s in rec["sections"]:
        d = s["diff"]
        print()
        print("section %s  - ours %d relocation(s), target %d"
              % (s["section"], d["ours"], d["target"]))
        print("  our relocations:")
        print("\n".join(_dump(s["ours"], rows)) or "    (none)")
        print("  target relocations:")
        print("\n".join(_dump(s["target"], rows)) or "    (none)")
        print("  diff:")
        print("\n".join(_diff_lines(d)))


def summarize(recs: list[dict]) -> None:
    """One honest line per unit - the answer a lane quotes, identical or not."""
    print()
    for rec in recs:
        if rec["error"]:
            print("%-32s ERROR  %s" % (rec["unit"], rec["error"]))
            continue
        total = sum(1 for s in rec["sections"] if not s["diff"]["identical"])
        if total == 0:
            n = sum(s["diff"]["ours"] for s in rec["sections"])
            print("%-32s relocation-identical over %d section(s), %d relocation(s)"
                  % (rec["unit"], len(rec["sections"]), n))
        else:
            print("%-32s RELOCATION MISMATCH in %d of %d section(s): %s"
                  % (rec["unit"], total, len(rec["sections"]),
                     ", ".join("%s (%d)" % (s["section"], _count(s["diff"]))
                               for s in rec["sections"] if not s["diff"]["identical"])))


def _count(d: dict) -> int:
    return (len(d["only_target"]) + len(d["only_ours"]) + len(d["different_symbol"])
            + len(d["different_attr"]))


def selftest() -> int:
    import relocdiff_selftest
    return relocdiff_selftest.selftest()


def _run(args, units: list[str]) -> int:
    """Resolve every requested unit, diff it, render it, and return the exit status."""
    recs = [unit_record(spec, args.section or None) for spec in units]
    for rec in recs:
        render(rec, None if args.all else args.rows)
    summarize(recs)
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(recs, fh, indent=1)
        print("wrote %s" % args.json)
    if any(rec["error"] for rec in recs):
        return 2
    if args.check and not all(unit_identical(rec) for rec in recs):
        return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("specs", nargs="*", metavar="unit",
                    help="unit spec(s) from the repository root, e.g. g3d/fn_8005AA28")
    ap.add_argument("--unit", action="append", default=[], dest="units",
                    help="the same, as a flag (repeatable)")
    ap.add_argument("--section", action="append", default=[],
                    help="only these sections (repeatable; default every section either side relocates)")
    ap.add_argument("--rows", type=int, default=60,
                    help="cap the relocations printed per side (0 = none, default 60)")
    ap.add_argument("--all", action="store_true", help="print every relocation, uncapped")
    ap.add_argument("--json", help="write the whole record to this file")
    ap.add_argument("--check", action="store_true",
                    help="exit 1 when any compared section's relocation sets differ (a gate)")
    ap.add_argument("--by-owner", action="store_true",
                    help="compact mode: align relocations in order within each owning symbol, print only "
                         "differences and `N/N relocations match` per unit (exit 1 on a difference); a "
                         "moved function or slid instruction is not a difference")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    units = list(args.units) + list(args.specs)
    if not units:
        ap.error("a unit is required (or --selftest)")
    if args.all and args.rows != 60:
        ap.error("--all and --rows are mutually exclusive")
    if args.by_owner:
        return run_by_owner(units)
    return _run(args, units)


if __name__ == "__main__":
    sys.exit(main())
