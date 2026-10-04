#!/usr/bin/env python3
"""Per-section gap of a unit's two objects: sizes, first differing byte, count and the relocation list.
Spec: docs/tools/spec/sectiongap.md. CLI: sectiongap.py --unit U [--all-sections] | --selftest."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import sys

from tools.lib import repo as _repo
from tools.lib import units as _units  # unit spec -> build/RMHE08/{src,obj} paths
from tools.lib import objcompare
from tools.units import poolseams  # literal pools as TU evidence: which differing pools are a partial pool

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
META_SECTIONS = objcompare.META_SECTIONS
MAX_RELOC_DIFFS = objcompare.MAX_RELOC_DIFFS
MAX_RELOC_OFFSETS = objcompare.MAX_RELOC_OFFSETS
read_object = objcompare.object_sections
reloc_reasons = objcompare.reloc_reasons
section_reasons = objcompare.section_reasons


def compare_objects(ours: dict, target: dict) -> list:
    """`[{"section", "ours", "target", "why"}]` per differing section, in the target's order (`objcompare.sections`)."""
    return [gap.to_dict() for gap in objcompare.sections(target, ours)]


def _size(n: int) -> str:
    return "0x%X (%d B)" % (n, n)


POOL_SECTIONS = (".sdata2", ".sdata")


def add_pool_notes(rows: list, note: str | None) -> list:
    """Append the pool-sharing explanation to a differing `.sdata2`/`.sdata` row (`note` is `poolseams`' fold line).

    A literal pool that differs in a unit that shares pooled literals with other registered units is a partial pool
    of ONE original TU (docs/pool-seams.md): the difference is the seam, not the source.  Other sections and a unit
    with no group are untouched.
    """
    if not note:
        return rows
    for row in rows:
        if row["section"] in POOL_SECTIONS:
            row["why"] += ("; pool-shared: our object's %s is a partial pool of a TU that spans several registered "
                           "units (%s)" % (row["section"], note))
    return rows


def selftest() -> int:
    """Delegates to `sectiongap_selftest.py` (fixtures only, no build and no repository state)."""
    from tools.units import sectiongap_selftest
    return sectiongap_selftest.selftest()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append", default=[],
                    help="unit spec, e.g. Network/NetworkWiiMediator (repeatable)")
    ap.add_argument("--all-sections", action="store_true",
                    help="also compare .comment/.symtab/.strtab/.shstrtab bookkeeping")
    ap.add_argument("--selftest", action="store_true", help="run the fixtures-only selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.unit:
        ap.error("--unit is required (this tool compares one unit's two objects at a time)")

    diffs = units = clean = 0
    printed_header = False
    for spec in args.unit:
        try:
            unit = _units.Unit.resolve(spec, _repo.repo_root())
        except SystemExit as exc:
            print("sectiongap: cannot resolve %r: %s" % (spec, exc), file=sys.stderr)
            continue
        units += 1
        missing = [p for p in (unit.obj_ours, unit.obj_target) if not os.path.exists(p)]
        if missing:
            print("sectiongap: %s: object missing (%s)" % (unit.report_name, ", ".join(missing)), file=sys.stderr)
            continue
        rows = compare_objects(read_object(unit.obj_ours, args.all_sections),
                               read_object(unit.obj_target, args.all_sections))
        if any(r["section"] in POOL_SECTIONS for r in rows):
            rows = add_pool_notes(rows, poolseams.note_for_unit(MAIN, unit.report_name))
        if not rows:
            clean += 1
            print("%s: clean - every compared section has the same size, the same bytes and the same "
                  "relocations" % unit.report_name)
            continue
        if not printed_header:
            print("unit  section  ours  target  why")
            printed_header = True
        diffs += len(rows)
        for row in rows:
            print("%s  %s  ours %s  target %s  %s"
                  % (unit.report_name, row["section"], _size(row["ours"]), _size(row["target"]), row["why"]))

    print("\n%d section difference(s) over %d unit(s); %d clean" % (diffs, units, clean))
    return 0    # a reporting tool: the output is the verdict, the exit status is not


if __name__ == "__main__":
    sys.exit(main())
