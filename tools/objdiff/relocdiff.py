#!/usr/bin/env python3
"""Relocation-level diff of one unit: the target object's relocations against ours, on both sides.
Spec: docs/tools/spec/relocdiff.md. CLI: relocdiff.py <unit>... [--unit U] [--section S] [--rows N | --all] [--json F]
[--check] [--by-owner] | --callees [unit...] [--json F|-] | --selftest."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os
import sys

from tools.lib import repo as _repo
from tools.lib import units as _units
from tools.lib import objcompare
from tools.lib import report as _report

read_relocs = objcompare.reloc_rows          # `({section: [(offset, symbol, type, addend)]}, None)` or `(None, why)`
owner_groups = objcompare.owner_groups


def type_name(typ: int) -> str:
    """`R_PPC_REL24` for a relocation kind this view names, `R_PPC_<n>` otherwise (`objcompare.legacy_reloc_name`)."""
    return objcompare.legacy_reloc_name(typ)


def diff_relocs(ours: list, target: list) -> dict:
    """The four-class diff of two relocation lists (`objcompare.reloc_classes`, target first)."""
    return objcompare.reloc_classes(target, ours)


def compare_by_owner(ours, target) -> tuple[int, int, list[str]]:
    """`(matching, total, lines)` aligned in order within each owning symbol (`objcompare.by_owner`)."""
    return objcompare.by_owner(target, ours)


# --------------------------------------------------------------------------------------------------
# the record: one unit, both objects
# --------------------------------------------------------------------------------------------------

def unit_record(spec: str, sections: list[str] | None = None) -> dict:
    """Resolve the unit, read both objects' relocations, and diff every section they relocate."""
    unit = _units.Unit.resolve(spec, _repo.repo_root())
    rec = {"unit": unit.report_name, "stem": spec, "ours": unit.obj_ours, "target": unit.obj_target,
           "sections": [], "error": None, "stale": []}
    if not os.path.exists(unit.obj_ours):
        rec["error"] = "our object does not exist (%s) - build it (`ninja %s`)" % (unit.obj_ours, spec)
        return rec
    if not os.path.exists(unit.obj_target):
        rec["error"] = ("the split target object does not exist (%s) - split the unit first "
                        "(`python configure.py && ninja`)" % unit.obj_target)
        return rec
    src = unit.source
    rec["stale"] = _report.unit_reasons(src, unit.obj_ours, _repo.repo_root(),
                                           rel=lambda p: _report.rel_path(p, _repo.repo_root()))[0]
    ours, err = read_relocs(unit.obj_ours)
    if err:
        rec["error"] = err
        return rec
    target, err = read_relocs(unit.obj_target)
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


def run_by_owner(units: list[str]) -> int:
    """`--by-owner`: differences only, then one `N/N relocations match` line per unit. 0 clean, 1 differs, 2 error."""
    status = 0
    for spec in units:
        unit = _units.Unit.resolve(spec, _repo.repo_root())
        missing = [p for p in (unit.obj_ours, unit.obj_target) if not os.path.exists(p)]
        if missing:
            print("%s: object missing (%s) - build/split it first" % (spec, missing[0]))
            status = max(status, 2)
            continue
        try:
            with open(unit.obj_ours, "rb") as fo, open(unit.obj_target, "rb") as ft:
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


def callee_record(spec: str, root: str | None = None) -> dict:
    """`{unit, functions: lib.objcompare.callee_diffs(...), error}` for one unit (`--callees`)."""
    unit = _units.Unit.resolve(spec, root or _repo.repo_root())
    rec = {"unit": unit.report_name, "stem": spec, "functions": [], "error": None}
    missing = [p for p in (unit.obj_ours, unit.obj_target) if not os.path.exists(p)]
    if missing:
        rec["error"] = "object missing (%s) - build/split it first" % missing[0]
        return rec
    try:
        rec["functions"] = objcompare.callee_diffs(unit.obj_target, unit.obj_ours)
    except (ValueError, OSError) as exc:
        rec["error"] = "unreadable object (%s)" % exc
    return rec


def callee_line(fn: dict) -> str:
    """One function's differences on one line: `kind: ours X vs target Y @+0xNN; ...`."""
    parts = []
    for d in fn["diffs"]:
        if d["kind"] == "extra":
            parts.append("extra: ours %s @+0x%x" % (d["ours"], d["offset"]))
        elif d["kind"] == "missing":
            parts.append("missing: target %s @+0x%x" % (d["target"], d["offset"]))
        else:
            parts.append("%s: ours %s vs target %s @+0x%x" % (d["kind"], d["ours"], d["target"], d["offset"]))
    return "%s  %s" % (fn["function"], "; ".join(parts))


def registered_with_objects(root: str) -> list[str]:
    """Every registered unit (`configure.py`) whose compiled and split objects both exist, extensionless."""
    from tools.lib import artifacts  # noqa: PLC0415 - only the whole-tree sweep needs the registry
    out = []
    for stem in sorted(artifacts.registered_stems(root) or ()):
        if all(os.path.exists(os.path.join(root, "build", _repo.VERSION, side, *(stem + ".o").split("/")))
               for side in ("src", "obj")):
            out.append(stem)
    return out


def run_callees(units: list[str], json_path: str | None = None) -> int:
    """`--callees`: per written function, the relocation symbol names that differ (wrong callee, mangling,
    linkage), one line per function. 0 none, 1 some, 2 an object missing or unreadable."""
    root = _repo.repo_root()
    specs = units or registered_with_objects(root)
    recs = [callee_record(spec, root) for spec in specs]
    status = 0
    for rec in recs:
        if rec["error"]:
            print("%s: %s" % (rec["stem"], rec["error"]))
            status = 2
            continue
        for fn in rec["functions"]:
            print("%s  %s" % (rec["stem"], callee_line(fn)))
            status = max(status, 1)
    n = sum(len(r["functions"]) for r in recs)
    print("relocdiff --callees: %d function(s) with a symbol-name difference in %d of %d unit(s)"
          % (n, sum(1 for r in recs if r["functions"]), len(recs)))
    if json_path:
        payload = json.dumps(recs, indent=1)
        if json_path == "-":
            print(payload)
        else:
            with open(json_path, "w", encoding="utf-8") as fh:
                fh.write(payload)
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
    rel = lambda p: _report.rel_path(p, _repo.repo_root())               # noqa: E731
    print("== %s" % rec["unit"])
    print("   ours    %-54s %s" % (rel(rec["ours"]), _report.stamp(_report.mtime(rec["ours"]))))
    print("   target  %-54s %s" % (rel(rec["target"]),
                                   _report.stamp(_report.mtime(rec["target"]))))
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
    ap.add_argument("--callees", action="store_true",
                    help="per written function, the relocation symbol names that differ (wrong callee, mangling, "
                         "linkage), one line per function; no unit = every registered unit with both objects; "
                         "--json F (or -) writes the record")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    units = list(args.units) + list(args.specs)
    if args.callees:
        return run_callees(units, args.json)
    if not units:
        ap.error("a unit is required (or --selftest)")
    if args.all and args.rows != 60:
        ap.error("--all and --rows are mutually exclusive")
    if args.by_owner:
        return run_by_owner(units)
    return _run(args, units)


if __name__ == "__main__":
    sys.exit(main())
