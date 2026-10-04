#!/usr/bin/env python3
"""Per-unit data-section gap target vs ours, the data-closure census and the gate's strict/span/fold verdicts.
Spec: docs/tools/spec/datagap.md. CLI: datagap.py [--flip-blockers] [--unit U] [--census] [--mode M] [--json F]
[--pool-seams] [--touched-by REF] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import sys

from tools.lib import objcompare
from tools.units import dataclosure  # the data closure: census, strict/span/fold verdicts, the gate's rows
from tools.units import dataseams  # `.data` emission-order seams: order-only / multi-TU diagnosis
from tools.units import poolseams  # literal pools as TU evidence (`--pool-seams`)
# The closure names this CLI uses, and the ones its importers still spell `datagap.<name>` (land, backlog, dataclaim,
# stylelint, poolseams; re-exported until they import `dataclosure` themselves - WP4 for land).
from tools.units.dataclosure import (  # noqa: F401
    GAME_DIR, STRICT_CLASSES, batch_orphans, build_fixture_object, census, census_report, claims_at_ref,
    derive_absorption, explain_added, fixpoint_plan, git_unit_renames, load_claims, load_data_symbols, orphan_key,
    orphan_verdict, readers_index, render_census, render_fixpoint, render_freshness, render_plan, render_strict,
    render_touch, snapshot_orphans, splits_at_ref, strict_report, tree_claim_exposed, tree_freshness, unit_claim_table,
    unit_plan)

META_SECTIONS = objcompare.META_SECTIONS
# The sections that make a unit's *data* wrong rather than its code unfinished (with their relocation sections).
DATA_SECTIONS = objcompare.FLIP_DATA_SECTIONS
section_sizes = objcompare.section_sizes        # {section: size}, empty sections omitted
compare_sections = objcompare.size_gaps         # (ours_extra, target_extra), metadata excluded unless all_sections


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


def seam_notes(pairs) -> list[tuple[str, str]]:
    """`(unit, note)` for every scanned unit whose `.data` differs and whose target range spans several TUs.

    `dataseams.seam_note` decides (docs/data-order-seams.md): `order-only` when the two objects hold the same
    symbols in a different sequence - a size comparison cannot see that - else the multi-TU line. Units whose
    range holds no strong seam, or whose objects agree, are silent.
    """
    out = []
    for row in pairs:
        if not (os.path.exists(row["obj"]) and os.path.exists(row["src"])):
            continue
        # the report's unit name carries a `main/` prefix; splits.txt names the unit by its source path
        stem = os.path.splitext(row["obj"].replace("\\", "/").split("/obj/", 1)[-1])[0]
        note = dataseams.seam_note(stem, row["src"], row["obj"])
        if note:
            out.append((row["unit"], note))
    return out


def summary_lines(listed: int, data_rows: int, scanned: int, mode: str,
                  flip_blockers: bool, min_fuzzy: float) -> list[str]:
    """The closing lines, honest about the **direction** a zero is a zero *of*.

    The default `--mode ours-extra` lists only the ours-extra direction, so `0 unit(s) listed` means "no
    unit has ours-extra bytes", never "no unit has a data object" - a lane read it as the latter once.
    The zero case therefore names the direction that was **not** scanned and how to scan it.
    """
    what = ("data-only gap at or above %g %% code" % min_fuzzy) if flip_blockers else ("%s gap" % mode)
    lines = ["%d unit(s) listed, %d of them with a real %s, out of %d scanned"
             % (listed, data_rows, what, scanned)]
    if listed == 0:
        if flip_blockers:
            lines.append("no unit among the %d scanned has a data-only gap at or above %g %% code "
                         "(code not matching, or no data difference) - not \"no data object\""
                         % (scanned, min_fuzzy))
        else:
            other = {"ours-extra": "target-extra", "target-extra": "ours-extra"}.get(mode, "both")
            lines.append("no unit among the %d scanned has %s bytes - this is \"no %s\", NOT \"no data "
                         "object\". The %s direction was not scanned; use --mode both (or --mode %s) "
                         "to look there." % (scanned, mode, mode, other, other))
    return lines


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

    # the summary must not read a zero in one direction as "no data object"
    ok = summary_lines(0, 0, 3, "ours-extra", False, 99.0)
    eq(len(ok), 2, "a zero in one direction adds an honest second line")
    eq("0 unit(s) listed" in ok[0], True, "... while keeping the count line")
    eq("target-extra" in ok[1] and "no data object" in ok[1], True,
       "... and names the direction that was not scanned")
    eq(summary_lines(2, 2, 5, "ours-extra", False, 99.0), [summary_lines(2, 2, 5, "ours-extra",
                                                                        False, 99.0)[0]],
       "a non-zero listing is one line")
    flip = summary_lines(0, 0, 3, "ours-extra", True, 99.0)
    eq(len(flip), 2, "a flip-blocker zero is explained too")
    eq("data-only gap" in flip[1], True, "... as a data-only gap, not a missing object")

    # pure-function property: sorted, never both lists for one section
    for name, t, o in [(".data", 0, 120), (".data", 120, 0), (".data", 1, 1)]:
        e, m = compare_sections({name: t}, {name: o})
        eq(len(e) + len(m) <= 1, True, f"one direction only for {t}/{o}")

    # emission-order seams: a unit whose sizes agree can still be order-only, and the size scan is blind to it
    eq(compare_sections({".data": 4108}, {".data": 4108}), ([], []), "equal sizes are silent to the size scan")
    real = dataseams.seam_note
    try:
        dataseams.seam_note = lambda unit, src, obj, **kw: (
            ".data: order-only: the unit spans several TUs; seams: at 0x00001100" if unit == "A/a" else None)
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            def touch(rel):
                path = os.path.join(tmp, rel)
                os.makedirs(os.path.dirname(path), exist_ok=True)
                open(path, "wb").close()
                return path

            pairs = [{"unit": "main/A/a", "obj": touch("build/RMHE08/obj/A/a.o"),
                      "src": touch("build/RMHE08/src/A/a.o"), "fuzzy": 0.0},
                     {"unit": "main/B/b", "obj": touch("build/RMHE08/obj/B/b.o"),
                      "src": touch("build/RMHE08/src/B/b.o"), "fuzzy": 0.0},
                     {"unit": "main/C/c", "obj": touch("build/RMHE08/obj/C/c.o"),
                      "src": os.path.join(tmp, "no", "such.o"), "fuzzy": 0.0}]
            got = seam_notes(pairs)
        eq([u for u, _n in got], ["main/A/a"], "only a unit with a note is listed; a missing object is skipped")
        eq("order-only: the unit spans several TUs; seams: at" in got[0][1], True,
           "the note is the order-only diagnosis")
    finally:
        dataseams.seam_note = real

    # the data closure (the census, the strict/span/fold verdicts, the gate rows) lives in dataclosure.py
    dataclosure.selftest(eq)

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
    ap.add_argument("--census", action="store_true",
                    help="the data census: per registered unit, the data its TARGET object references, "
                         "classified own / other-unit / orphan (no claim covers it); --unit narrows it")
    ap.add_argument("--top", type=int, default=40, help="orphans listed by --census (ranked by readers)")
    ap.add_argument("--pool-seams", action="store_true",
                    help="the literal-pool census (tools/units/poolseams.py): groups of registered units that read "
                         "one pooled literal = one original TU (a fold candidate); --unit names one unit's group, "
                         "--json writes it, --top caps the groups shown")
    ap.add_argument("--row", metavar="UNITS",
                    help="what the land gate's data-closure row says for these comma-separated units in --root, "
                         "judged against --base-root's census (pairs) and --base-ref's claims (default: "
                         "--base-root's); exit 1 when it would refuse")
    ap.add_argument("--base-root", default=None, help="--row: the base tree (default: --root)")
    ap.add_argument("--base-ref", default=None, help="--row: revision whose splits.txt is the claimed-bytes base")
    ap.add_argument("--allow-orphan", action="append", default=[], metavar="ADDR", help="--row: an allowance")
    ap.add_argument("--unit-rename", action="append", default=[], metavar="OLD=NEW",
                    help="--row: override the derived fold map for OLD (several OLDs may share one NEW; `OLD=` means "
                         "its base pairs are gone with the unit)")
    ap.add_argument("--touched-by", action="store_true",
                    help="--row: only list, per unit, whether the batch really touches it and why (registered / "
                         "claims changed / object changed) against the base")
    ap.add_argument("--write-snapshot", metavar="FILE", help="write --root's base snapshot (what record-base stores)")
    ap.add_argument("--base-snapshot", metavar="FILE", help="--row: a snapshot written by --write-snapshot")
    ap.add_argument("--no-readers", action="store_true", help="--census: skip the callers.py reader index")
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                    help="--census: the tree to read (default: this tool's tree)")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if args.write_snapshot:
        with open(args.write_snapshot, "w", encoding="utf-8") as fh:
            json.dump(snapshot_orphans(args.root, [os.path.splitext(u.strip())[0] for u in args.row.split(",")]
                                       if args.row else None), fh)
        print(f"wrote {args.write_snapshot}")
        return 0
    if args.row:
        base_root = args.base_root or args.root
        if args.base_snapshot:
            with open(args.base_snapshot, encoding="utf-8") as fh:
                snap = json.load(fh)
        else:
            snap = snapshot_orphans(base_root, [os.path.splitext(u.strip())[0] for u in args.row.split(",") if u.strip()])
        if args.base_ref:
            snap["claims"] = claims_at_ref(args.root, args.base_ref)
            snap["unit_claims"] = unit_claim_table(splits_at_ref(args.root, args.base_ref))
        units = [u.strip() for u in args.row.split(",") if u.strip()]
        explicit = {}
        for item in args.unit_rename:
            old, _eq, new = item.partition("=")
            if old.strip():
                explicit[old.strip().strip("/")] = new.strip().strip("/")
        renames = git_unit_renames(args.root, args.base_ref) if args.base_ref else None
        verdict = batch_orphans(args.root, units, snap, args.allow_orphan, unit_map=explicit, git_renames=renames)
        for line in verdict["unit_map_lines"]:
            print(line)
        for label, tree in (("base", base_root), ("tree", args.root)):
            if label == "tree" and os.path.abspath(tree) == os.path.abspath(base_root):
                continue
            for line in render_freshness(label, tree_freshness(tree, units=units)):
                print(line)
        if args.touched_by:
            for unit in units:
                print(render_touch(os.path.splitext(unit)[0], verdict["touch"].get(os.path.splitext(unit)[0],
                                   {"touched": True, "reasons": ["not a batch unit"]})))
            return 0
        for unit in units:
            name = os.path.splitext(unit)[0]
            print(render_touch(name, verdict["touch"].get(name, {"touched": True, "reasons": ["unjudged"]})))
        for line in verdict["added_deferred"][:6]:
            print("deferred (new pair) " + line)
        if len(verdict["added_deferred"]) > 6:
            print("deferred (new pair) ... %d more" % (len(verdict["added_deferred"]) - 6))
        for line in verdict["claim_exposed"][:6]:
            print("deferred (claim-exposed) " + line)
        if len(verdict["claim_exposed"]) > 6:
            print("deferred (claim-exposed) ... %d more" % (len(verdict["claim_exposed"]) - 6))
        for line in verdict["added"]:
            print("REFUSED  " + line)
        if verdict["added"] and os.path.abspath(base_root) != os.path.abspath(args.root):
            base_ranges = splits_at_ref(args.root, args.base_ref) if args.base_ref else load_claims(base_root)
            for line in explain_added(args.root, base_root, units, verdict["base_keys"], base_ranges):
                print(line)
        for line in verdict["sole_owned"]:
            print("REFUSED  sole-owned: " + line)
        for line in verdict["accepted"] + verdict["strict"]["accepted"]:
            print("ALLOWED  " + line)
        if verdict["unmatched_allowances"]:
            print("UNMATCHED allowance(s) excuse nothing: " + ", ".join(verdict["unmatched_allowances"]))
        for cls, lines in sorted(verdict["strict"]["deferred"].items()):
            print("deferred %-15s %4d pair(s)" % (cls, len(lines)))
            for line in lines[:3]:
                print("    " + line)
        per_unit: dict = {}
        for block in verdict["strict_blocks"]:
            row = per_unit.setdefault(block["unit"], {"refuse": 0, "deferred": 0})
            row["refuse" if block["verdict"] == "refuse" else "deferred"] += len(block["pairs"])
        for unit, row in sorted(per_unit.items()):
            print("  %-34s refusable pairs %3d, deferred pairs %3d" % (unit, row["refuse"], row["deferred"]))
        for unit, count in sorted(verdict["untouched_pairs"].items()):
            print("  %-34s not touched: %3d sole-owned pair(s) reported, not demanded" % (unit, count))
        print("pre-existing (reported, not refused): %d; claim-exposed (deferred, not refused): %d"
              % (len(verdict["pre_existing"]), len(verdict["claim_exposed"])))
        failed = bool(verdict["added"] or verdict["sole_owned"])
        print("row: %s" % ("FAIL, %d added + %d sole-owned pair(s) unclaimed" % (len(verdict["added"]),
                                                                                  len(verdict["sole_owned"]))
                           if failed else "PASS"))
        return 1 if failed else 0
    if args.pool_seams:
        pool_census = poolseams.load_census(args.root, True)
        if args.json:
            with open(args.json, "w", encoding="utf-8") as fh:
                json.dump(pool_census, fh, indent=1, default=list)
            print(f"wrote {args.json}")
        if args.unit:
            for unit in args.unit:
                group = poolseams.group_of(pool_census, unit)
                print("\n".join(poolseams.render_group(group)) if group else f"{unit}: no pool-sharing group")
            return 0
        print(poolseams.render_census(pool_census, min(args.top, 25)))
        return 0
    if args.census:
        ranges = load_claims(args.root)
        (records, stats), registered, read = census(args.root, args.unit or None, ranges=ranges)
        query = None if args.no_readers else readers_index(args.root)
        rep = census_report(records, stats, query)
        if args.json:
            with open(args.json, "w", encoding="utf-8") as fh:
                json.dump({"registered": registered, "read": read, **rep}, fh, indent=1)
            print(f"wrote {args.json}")
        print(render_census(rep, args.top, (registered, read)))
        if args.unit:
            # the units' own sole-owned data, judged as the strict row judges it (the census above reads only
            # the requested units' objects, so the other units' references are not seen here: for the pair's
            # sharing the row re-reads every registered object)
            (all_records, _s), _n, _h = census(args.root, None, ranges=ranges)
            names = [os.path.splitext(u)[0] for u in args.unit]
            print("")
            print(render_strict(strict_report(args.root, all_records, ranges, names, query=None if args.no_readers else (query or "lazy"))))
        return 0
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
            # `.text` is not a data gap: a NonMatching unit whose functions overrun reports it too, and that
            # is a size residual (progress), not the actionable data defect the rest of this tool is about.
            data_extra = [e for e in extra if e[0] != ".text"]
            text_extra = [e for e in extra if e[0] == ".text"]
            if data_extra:
                bits.append("ours-extra " + ", ".join(f"{s} {o}B (target {t}B)" for s, o, t in data_extra))
            if text_extra:
                bits.append("size residual, not a data gap " +
                            ", ".join(f"{s} {o}B (target {t}B)" for s, o, t in text_extra))
        if missing:
            bits.append("target-extra " + ", ".join(f"{s} {t}B (ours {o}B)" for s, t, o in missing))
        print(f"{name}: " + "; ".join(bits))
    notes = seam_notes(pairs)
    for name, note in notes:
        print(f"{name}: {note}")
    data_rows = sum(1 for _n, e, m in rows if m or any(s != ".text" for s, _o, _t in e))
    for line in [""] + summary_lines(len(rows), data_rows, len(pairs), args.mode,
                                      args.flip_blockers, args.min_fuzzy):
        print(line)

    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump([{"unit": n, "ours_extra": e, "target_extra": m} for n, e, m in rows]
                      + [{"unit": n, "seam_note": t} for n, t in notes], fh, indent=1)
        print(f"wrote {args.json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
