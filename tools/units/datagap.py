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

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))   # `tools/`: `from units import`
import dataseams  # noqa: E402  (`.data` emission-order seams: order-only / multi-TU diagnosis)

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


# -- the data census: what each registered unit's TARGET object references that no claim covers ---------------
#
# `compare_sections` above sees a size difference between two objects. It cannot see data a unit's target
# relocates against that **no claim covers** - the address sits in a dtk `auto_*` object, so the unit's bytes
# match, the section sizes agree, and the data is simply left behind (owner, 2026-09-29: "no data should be
# left behind"). Every review of 2026-09-29 found such a gap by hand (em024_ai, network_pat_control,
# menu_message, arenatask). The census asks the question directly, per registered unit:
#
#   own     the object defines the symbol itself (the unit's claim covers it by construction)
#   other   the object references it and ANOTHER unit's claim covers its address (a legitimate cross-unit read)
#   orphan  the object references it and NO claim covers its address (named with its neighbours and section)
#
# The readers are the project's existing ones: `undefrefs.load_object` (the one relocation reader),
# `symedit.entries` (the map rows, with sizes), `splits.txt` (the claims) and `callers.py`'s index (who else
# reads an orphan). A claim is a `splits.txt` range, so "covered" is exactly "some registered block's range
# contains the row's start address".

CENSUS_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")
GAME_DIR = "RMHE08"


def parse_splits_text(text: str) -> dict[str, list[tuple[int, int, str]]]:
    """`{section: [(start, end, unit)]}` (sorted) from the text of a `splits.txt`; the unit has no extension."""
    import re

    out: dict[str, list[tuple[int, int, str]]] = {}
    unit = None
    for line in text.splitlines():
        if line.startswith("Sections:"):
            continue
        m = re.match(r"^([^\s:][^:]*):\s*$", line)
        if m:
            unit = os.path.splitext(m.group(1))[0]
            continue
        m = re.match(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and unit:
            out.setdefault(m.group(1), []).append((int(m.group(2), 16), int(m.group(3), 16), unit))
    return {k: sorted(v) for k, v in out.items()}


def merged(ranges) -> list[tuple[int, int]]:
    """Sorted `(start, end)` ranges with overlaps and touching neighbours merged (the claimed bytes)."""
    out: list[list[int]] = []
    for start, end, *_ in sorted(ranges):
        if out and start <= out[-1][1]:
            out[-1][1] = max(out[-1][1], end)
        else:
            out.append([start, end])
    return [(a, b) for a, b in out]


def claimed_bytes(ranges: dict) -> dict[str, list[tuple[int, int]]]:
    """`{section: merged (start, end)}` - the snapshot a recut is compared against."""
    return {sec: merged(rows) for sec, rows in ranges.items()}


def shrunk_claims(base: dict, now: dict) -> list[tuple[str, int, int]]:
    """`(section, start, end)` for every byte claimed in `base` and not claimed in `now` (a recut's loss).

    Both arguments are `claimed_bytes`-shaped. Pure; a byte that merely moved to another unit is still claimed.
    """
    out = []
    for section, rows in sorted(base.items()):
        keep = sorted(tuple(r) for r in now.get(section, []))
        for start, end in rows:
            cur = start
            for a, b in keep:
                if b <= cur or a >= end:
                    continue
                if a > cur:
                    out.append((section, cur, a))
                cur = max(cur, b)
                if cur >= end:
                    break
            if cur < end:
                out.append((section, cur, end))
    return out


def classify_address(section: str, address: int, ranges: dict, unit: str) -> dict:
    """`{status, owner, prev, next}` for one data address against the claims.

    `status` is `own` (a range of `unit` contains it), `other` (another unit's does; `owner` names it) or
    `orphan` (none does; `prev`/`next` are the registered neighbours bracketing it in the section, each
    `(unit, start, end)` or None). Pure.
    """
    prev = nxt = None
    for start, end, owner in ranges.get(section, []):
        if start <= address < end:
            return {"status": "own" if owner == unit else "other", "owner": owner, "prev": None, "next": None}
        if end <= address:
            prev = (owner, start, end)
        elif start > address and nxt is None:
            nxt = (owner, start, end)
    return {"status": "orphan", "owner": None, "prev": prev, "next": nxt}


def object_data_refs(path: str) -> dict[str, int]:
    """`{name: relocation sites}` for the symbols the object references and does not define.

    Extab bookkeeping is skipped; the site count is the number of relocations that name the symbol.
    """
    from units import undefrefs as uref  # noqa: PLC0415 - the one relocation reader

    loaded = uref.load_object(path)
    if loaded is None:
        return {}
    defined = loaded.get("defined") or {}
    out: dict[str, int] = {}
    for r in loaded["relocs"]:
        name = r["symbol"]
        if not name or (r["target"] or "").startswith(uref.BOOKKEEPING) or name in defined:
            continue
        out[name] = out.get(name, 0) + 1
    return out


def census_records(unit_refs: dict, symbols: dict, ranges: dict) -> tuple[list[dict], dict]:
    """`(records, stats)` - one record per (unit, referenced data symbol). Pure.

    `unit_refs` is `{unit: {name: sites}}`, `symbols` is `{name: {section, address, size, type}}` (the map)
    and `ranges` the claims. A reference to a name the map does not carry, to a code symbol or to a
    non-data section is not data and is counted in `stats` only.
    """
    records, stats = [], {"unmapped": 0, "code": 0, "other_section": 0}
    for unit in sorted(unit_refs):
        for name, sites in sorted(unit_refs[unit].items()):
            entry = symbols.get(name)
            if entry is None:
                stats["unmapped"] += 1
                continue
            if entry.get("type") == "function":
                stats["code"] += 1
                continue
            if entry["section"] not in CENSUS_SECTIONS:
                stats["other_section"] += 1
                continue
            verdict = classify_address(entry["section"], entry["address"], ranges, unit)
            records.append({"unit": unit, "name": name, "section": entry["section"],
                            "address": entry["address"], "size": int(entry.get("size") or 0),
                            "sites": sites, **verdict})
    return records, stats


def orphan_key(unit: str, section: str, address: int) -> str:
    """The add-only row's key: rename-stable (an address never changes), unit-scoped (a new unit adds it)."""
    return "orphan:%s:%s:%08X" % (unit, section, address)


def orphan_keys(records: list[dict], units: list[str] | None = None) -> dict[str, dict]:
    """`{key: record}` for the orphan records, restricted to `units` when given. Pure."""
    want = set(units) if units is not None else None
    return {orphan_key(r["unit"], r["section"], r["address"]): r
            for r in records if r["status"] == "orphan" and (want is None or r["unit"] in want)}


def orphan_verdict(base_keys, after: dict, shrinks, allowed=()) -> dict:
    """The gate row's decision. Pure.

    `added` is every after-key the base did not have plus every shrunk claim, minus what `allowed` (hex
    addresses, `--allow-orphan`) names; `pre_existing` is the after-keys the base already had (reported,
    never refused); `accepted` lists what an allowance excused. An allowance that matches nothing excuses
    nothing, so the refusal stands.
    """
    sanction = set()
    for token in allowed:
        try:
            sanction.add(int(str(token).strip(), 16))
        except ValueError:
            continue
    base = set(base_keys)
    added, accepted = [], []
    for key in sorted(after):
        if key in base:
            continue
        rec = after[key]
        (accepted if rec["address"] in sanction else added).append(
            "%s references %s 0x%08X (%s, %d site(s)) and no claim covers it"
            % (rec["unit"], rec["name"], rec["address"], rec["section"], rec["sites"]))
    for section, start, end in shrinks:
        text = ("claim shrunk: %s 0x%08X-0x%08X was claimed at the base and no unit claims it now"
                % (section, start, end))
        (accepted if any(start <= a < end for a in sanction) else added).append(text)
    pre = ["%s %s 0x%08X (%s)" % (after[k]["unit"], after[k]["name"], after[k]["address"], after[k]["section"])
           for k in sorted(after) if k in base]
    return {"added": added, "pre_existing": pre, "accepted": accepted}


def load_claims(root: str) -> dict:
    path = os.path.join(root, "config", GAME_DIR, "splits.txt")
    with open(path, encoding="utf-8", errors="replace") as fh:
        return parse_splits_text(fh.read())


def claims_at_ref(root: str, ref: str) -> dict:
    """`claimed_bytes` of the `splits.txt` as committed at `ref` (a what-if base for a branch's own diff)."""
    import subprocess

    text = subprocess.run(["git", "show", "%s:config/%s/splits.txt" % (ref, GAME_DIR)], cwd=root,
                          capture_output=True, text=True, encoding="utf-8", errors="replace", check=True).stdout
    return {sec: [list(r) for r in rows] for sec, rows in claimed_bytes(parse_splits_text(text)).items()}


def load_data_symbols(root: str) -> dict[str, dict]:
    """`{name: {section, address, size, type}}` from `symbols.txt` (a duplicated name is dropped: ambiguous)."""
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "symbols"))
    import symedit  # noqa: PLC0415 - the map proxy; never prints the file

    out: dict[str, dict] = {}
    dup = set()
    for e in symedit.entries(os.path.join(root, "config", GAME_DIR, "symbols.txt")):
        if e["name"] in out:
            dup.add(e["name"])
        out[e["name"]] = e
    for name in dup:
        out.pop(name, None)
    return out


def census(root: str, units: list[str] | None = None, ranges: dict | None = None):
    """`(records, stats), registered, read` for the registered units' TARGET objects as they stand in `root`."""
    ranges = ranges if ranges is not None else load_claims(root)
    registered = sorted({u for rows in ranges.values() for _s, _e, u in rows})
    if units is not None:
        want = {os.path.splitext(u)[0] for u in units}
        registered = [u for u in registered if u in want]
    unit_refs = {}
    for unit in registered:
        path = os.path.join(root, "build", GAME_DIR, "obj", unit + ".o")
        if os.path.exists(path):
            unit_refs[unit] = object_data_refs(path)
    return census_records(unit_refs, load_data_symbols(root), ranges), len(registered), len(unit_refs)


def snapshot_orphans(root: str) -> dict:
    """The base snapshot `land.record_base` stores: every pre-existing orphan pair and the claimed bytes."""
    ranges = load_claims(root)
    (records, _stats), _n, _have = census(root, ranges=ranges)
    return {"keys": sorted(orphan_keys(records)),
            "claims": {sec: [list(r) for r in rows] for sec, rows in claimed_bytes(ranges).items()}}


def batch_orphans(root: str, units: list[str], base_snapshot: dict | None, allowed=()) -> dict:
    """The gate row's whole decision for the batch units against the recorded base (see `orphan_verdict`)."""
    ranges = load_claims(root)
    names = [os.path.splitext(u)[0] for u in units]
    (records, _stats), _n, _have = census(root, None, ranges=ranges)
    after = orphan_keys(records, names)
    snap = base_snapshot or {}
    base_claims = {sec: [tuple(r) for r in rows] for sec, rows in (snap.get("claims") or {}).items()}
    shrinks = shrunk_claims(base_claims, claimed_bytes(ranges)) if base_claims else []
    verdict = orphan_verdict(snap.get("keys") or [], after, shrinks, allowed)
    verdict["have_base"] = "keys" in snap and "claims" in snap
    # Reported, never refused: a pre-existing orphan no OTHER registered unit's object references is data this
    # unit alone needs - the batch that works on the unit is the natural place to close it.
    referencing = {(g["section"], g["address"]): len(g["units"]) for g in orphan_groups(records).values()}
    base_keys = set(snap.get("keys") or [])
    verdict["sole_owned_debt"] = ["%s %s 0x%08X (%s)" % (r["unit"], r["name"], r["address"], r["section"])
                                  for k, r in sorted(after.items())
                                  if k in base_keys and referencing[(r["section"], r["address"])] == 1]
    return verdict


def readers_index(root: str):
    """`query(address) -> (registered {unit: sites}, unsplit_sites)` from `callers.py`'s index, or None."""
    from units import callers as callers_mod  # noqa: PLC0415

    cmap = callers_mod.load_map(root)
    asm_dir = callers_mod.asm_dir_of(root)
    if callers_mod.all_asm_files(asm_dir):
        index, _info = callers_mod.load_index(root=root, asm_dir=asm_dir)
    else:
        index, _info = callers_mod.load_elf_index(root=root, cmap=cmap)
    if index is None:
        return None

    def query(address: int):
        units: dict[str, int] = {}
        unsplit = 0
        rep = callers_mod.query("0x%08X" % address, index, cmap, limit=0)
        for ref in rep.get("references", ()):
            name = callers_mod.norm_reader((ref.get("caller") or {}).get("owner"))
            if name:
                units[name] = units.get(name, 0) + 1
            else:
                unsplit += 1
        return units, unsplit

    return query


def orphan_groups(records: list[dict]) -> dict[tuple, dict]:
    """Group the orphan records by `(section, address)`: who references it, its size, its neighbours. Pure."""
    groups: dict[tuple, dict] = {}
    for r in records:
        if r["status"] != "orphan":
            continue
        g = groups.setdefault((r["section"], r["address"]),
                              {"name": r["name"], "section": r["section"], "address": r["address"],
                               "size": r["size"], "units": {}, "prev": r["prev"], "next": r["next"]})
        g["units"][r["unit"]] = r["sites"]
    return groups


def census_report(records: list[dict], stats: dict, query=None) -> dict:
    """Totals per section plus the orphan list ranked by readers. Pure given `query`."""
    per: dict[str, dict] = {}
    for r in records:
        row = per.setdefault(r["section"], {"refs": 0, "own": 0, "other": 0, "orphan": 0,
                                            "orphan_objects": 0, "clearly_owned": 0})
        row["refs"] += 1
        row[r["status"]] += 1
    ranked = []
    for (section, address), g in orphan_groups(records).items():
        per[section]["orphan_objects"] += 1
        reloc_units = set(g["units"])
        readers, unsplit = ({}, 0) if query is None else query(address)
        total = (sum(readers.values()) + unsplit) if query is not None else sum(g["units"].values())
        clearly = len(reloc_units) == 1 and unsplit == 0 and set(readers) <= reloc_units
        if clearly:
            per[section]["clearly_owned"] += 1
        ranked.append({**g, "readers": total, "unsplit_readers": unsplit,
                       "reader_units": sorted(readers) if query is not None else sorted(reloc_units),
                       "clearly_owned": clearly})
    ranked.sort(key=lambda g: (-g["readers"], g["section"], g["address"]))
    return {"sections": per, "orphans": ranked, "stats": stats}


def render_census(rep: dict, top: int, units_total: tuple[int, int]) -> str:
    lines = ["data census: %d registered unit(s), %d target object(s) read" % units_total,
             "%-9s %7s %7s %7s %7s %9s %9s" % ("section", "refs", "own", "other", "orphan", "orph.obj",
                                               "clearly")]
    totals = dict.fromkeys(("refs", "own", "other", "orphan", "orphan_objects", "clearly_owned"), 0)
    for section in CENSUS_SECTIONS:
        row = rep["sections"].get(section)
        if not row:
            continue
        for key in totals:
            totals[key] += row[key]
        lines.append("%-9s %7d %7d %7d %7d %9d %9d" % (section, row["refs"], row["own"], row["other"],
                                                      row["orphan"], row["orphan_objects"],
                                                      row["clearly_owned"]))
    lines.append("%-9s %7d %7d %7d %7d %9d %9d" % ("total", totals["refs"], totals["own"], totals["other"],
                                                  totals["orphan"], totals["orphan_objects"],
                                                  totals["clearly_owned"]))
    lines.append("skipped references: %s" % ", ".join("%s %d" % kv for kv in sorted(rep["stats"].items())))
    lines.append("")
    lines.append("orphans ranked by readers (clearly owned = one referencing registered unit, no other reader):")

    def side(n):
        return "-" if not n else "%s[0x%08X-0x%08X]" % n

    for g in rep["orphans"][:top]:
        lines.append("  %-7s 0x%08X %-28s size 0x%X readers %d%s  units %s" % (
            g["section"], g["address"], g["name"], g["size"], g["readers"],
            " (+%d unsplit)" % g["unsplit_readers"] if g["unsplit_readers"] else "",
            ",".join(g["reader_units"][:4])))
        lines.append("      between %s and %s%s" % (side(g["prev"]), side(g["next"]),
                                                  "   CLEARLY OWNED" if g["clearly_owned"] else ""))
    if len(rep["orphans"]) > top:
        lines.append("  ... %d more (raise --top or use --json)" % (len(rep["orphans"]) - top))
    return "\n".join(lines)


def build_fixture_object(path: str, refs: list[str], defined: list[str] = ()) -> None:
    """Write a minimal ELF32-BE object: `.text` with one `R_PPC_ADDR16_HA` relocation per name in `refs`
    (undefined symbols) and an empty `.data` defining each name in `defined`. Enough for `parse_elf`."""
    import struct

    names = list(refs) + list(defined)
    strtab = b"\0"
    offs = {}
    for n in names:
        offs[n] = len(strtab)
        strtab += n.encode() + b"\0"
    syms = b"\0" * 16
    for n in refs:
        syms += struct.pack(">IIIBBH", offs[n], 0, 0, 0x10, 0, 0)            # global, undefined
    for n in defined:
        syms += struct.pack(">IIIBBH", offs[n], 0, 4, 0x11, 0, 2)            # global object in section 2
    index = {n: i + 1 for i, n in enumerate(names)}
    rela = b"".join(struct.pack(">IIi", 4 * i, (index[n] << 8) | 6, 0) for i, n in enumerate(refs))
    shstr = b"\0.text\0.data\0.rela.text\0.symtab\0.strtab\0.shstrtab\0"

    def name_off(s):
        return shstr.index(b"\0" + s.encode() + b"\0") + 1

    bodies = [b"", b"\0" * max(4, 4 * len(refs)), b"\0" * 4, rela, syms, strtab, shstr]
    heads = [(0, 0, 0, 0), (name_off(".text"), 1, 0, 0), (name_off(".data"), 1, 0, 0),
             (name_off(".rela.text"), 4, 4, 1), (name_off(".symtab"), 2, 5, 1),
             (name_off(".strtab"), 3, 0, 0), (name_off(".shstrtab"), 3, 0, 0)]
    entsize = {3: 12, 4: 16}
    blob = b"\0" * 0x34
    offsets = []
    for body in bodies:
        offsets.append(len(blob))
        blob += body
    shoff = len(blob)
    for i, (nm, typ, link, info) in enumerate(heads):
        blob += struct.pack(">IIIIIIIIII", nm, typ, 0, 0, offsets[i], len(bodies[i]), link, info, 4,
                            entsize.get(i, 0))
    ident = b"\x7fELF\x01\x02\x01" + b"\0" * 9
    hdr = ident + struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 0x34, 0, 0, 40, len(heads), 6)
    with open(path, "wb") as fh:
        fh.write(hdr + blob[0x34:])


def selftest_census(eq) -> None:
    """The census and the gate row's decision: pure cores first, then one real tree of fixture objects."""
    import tempfile

    splits = ("Sections:\n\t.rodata type:rodata align:64\n\nA/a.cpp:\n\t.rodata start:0x80500000 end:0x80500010\n"
              "\t.sdata2 start:0x80600000 end:0x80600008\n\nB/b.cpp:\n\t.rodata start:0x80500010 end:0x80500020\n")
    ranges = parse_splits_text(splits)
    eq(ranges[".rodata"], [(0x80500000, 0x80500010, "A/a"), (0x80500010, 0x80500020, "B/b")], "splits parse")
    eq(claimed_bytes(ranges)[".rodata"], [(0x80500000, 0x80500020)], "touching claims merge")
    eq(classify_address(".rodata", 0x80500004, ranges, "A/a")["status"], "own", "a claimed word is own")
    eq(classify_address(".rodata", 0x80500014, ranges, "A/a")["owner"], "B/b", "another unit's claim is other")
    far = classify_address(".rodata", 0x80500040, ranges, "A/a")
    eq((far["status"], far["prev"][0], far["next"]), ("orphan", "B/b", None), "an unclaimed word is an orphan")
    eq(shrunk_claims(claimed_bytes(ranges), claimed_bytes(ranges)), [], "an unchanged claim set shrinks nothing")
    cut = {".rodata": [(0x80500000, 0x80500008), (0x8050000C, 0x80500020)], ".sdata2": [(0x80600000, 0x80600008)]}
    eq(shrunk_claims(claimed_bytes(ranges), cut), [(".rodata", 0x80500008, 0x8050000C)],
       "a recut that drops four claimed bytes is reported exactly")
    eq(shrunk_claims({".rodata": [(0, 8)]}, {".rodata": [(0, 4)], ".sdata2": [(0, 8)]}),
       [(".rodata", 4, 8)], "a claim that only moved sections is still a loss in its own section")

    symbols = {"w_claimed": {"section": ".rodata", "address": 0x80500004, "size": 4, "type": "object"},
               "w_orphan": {"section": ".rodata", "address": 0x80500040, "size": 4, "type": "object"},
               "f": {"section": ".text", "address": 0x80010000, "size": 8, "type": "function"}}
    records, stats = census_records({"A/a": {"w_claimed": 1, "w_orphan": 2, "f": 1, "nope": 1}}, symbols, ranges)
    eq([(r["name"], r["status"]) for r in records], [("w_claimed", "own"), ("w_orphan", "orphan")],
       "claimed and unclaimed words are classified, code and unmapped names are not data")
    eq((stats["code"], stats["unmapped"]), (1, 1), "skipped references are counted, not hidden")
    rep = census_report(records, stats)
    eq((rep["sections"][".rodata"]["orphan_objects"], rep["sections"][".rodata"]["clearly_owned"]), (1, 1),
       "the orphan referenced by one registered unit is clearly owned")
    eq(census_report(records, stats, lambda a: ({"A/a": 1}, 1))["orphans"][0]["clearly_owned"], False,
       "an unsplit reader makes it not clearly owned")

    after = orphan_keys(records)
    key = orphan_key("A/a", ".rodata", 0x80500040)
    eq(list(after), [key], "one orphan key, unit-scoped")
    v = orphan_verdict([], after, [])
    eq(len(v["added"]), 1, "a new orphan is refused")
    eq(orphan_verdict([key], after, []), {"added": [], "pre_existing": ["A/a w_orphan 0x80500040 (.rodata)"],
                                          "accepted": []}, "a pre-existing orphan is reported, never refused")
    eq(len(orphan_verdict([], after, [], ["0x80500040"])["added"]), 0, "an allowance that names it excuses it")
    eq(len(orphan_verdict([], after, [], ["0x80500044"])["added"]), 1, "an allowance that matches nothing refuses")
    eq(len(orphan_verdict([key], {}, [(".rodata", 0x80500008, 0x8050000C)])["added"]), 1,
       "a shrunk claim is refused")

    with tempfile.TemporaryDirectory() as tmp:
        def write(rel, text=None):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            if text is not None:
                with open(path, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)
            return path

        syms = ("w_claimed = .rodata:0x80500004; // type:object size:0x4\n"
                "w_orphan = .rodata:0x80500040; // type:object size:0x4\n")
        write("config/RMHE08/symbols.txt", syms)
        write("config/RMHE08/splits.txt", splits)
        build_fixture_object(write("build/RMHE08/obj/A/a.o"), ["w_orphan"])
        build_fixture_object(write("build/RMHE08/obj/B/b.o"), ["w_claimed"])
        snap = {"keys": [], "claims": {s: [list(r) for r in rows] for s, rows in claimed_bytes(ranges).items()}}
        got = batch_orphans(tmp, ["A/a"], snap)
        eq(len(got["added"]), 1, "end to end: a unit referencing an unclaimed .rodata word is refused")
        eq(batch_orphans(tmp, ["B/b"], snap)["added"], [], "end to end: the same word claimed passes")
        with_pre = dict(snap, keys=sorted(snapshot_orphans(tmp)["keys"]))
        pre = batch_orphans(tmp, ["A/a"], with_pre)
        eq((pre["added"], len(pre["pre_existing"])), ([], 1), "end to end: a pre-existing orphan is reported")
        eq(len(pre["sole_owned_debt"]), 1, "... and flagged as data only this unit's object references")
        write("config/RMHE08/splits.txt", splits.replace("0x80500020", "0x80500018"))
        shrunk = batch_orphans(tmp, ["B/b"], snap)
        eq(len(shrunk["added"]), 1, "end to end: a recut that leaves a claimed byte unclaimed is refused")
        eq(batch_orphans(tmp, ["B/b"], {})["have_base"], False, "a base with no snapshot is flagged")


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

    selftest_census(eq)

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
    ap.add_argument("--row", metavar="UNITS",
                    help="what the land gate's data-closure row says for these comma-separated units in --root, "
                         "judged against --base-root's census (pairs) and --base-ref's claims (default: "
                         "--base-root's); exit 1 when it would refuse")
    ap.add_argument("--base-root", default=None, help="--row: the base tree (default: --root)")
    ap.add_argument("--base-ref", default=None, help="--row: revision whose splits.txt is the claimed-bytes base")
    ap.add_argument("--allow-orphan", action="append", default=[], metavar="ADDR", help="--row: an allowance")
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
            json.dump(snapshot_orphans(args.root), fh)
        print(f"wrote {args.write_snapshot}")
        return 0
    if args.row:
        base_root = args.base_root or args.root
        if args.base_snapshot:
            with open(args.base_snapshot, encoding="utf-8") as fh:
                snap = json.load(fh)
        else:
            snap = snapshot_orphans(base_root)
        if args.base_ref:
            snap["claims"] = claims_at_ref(args.root, args.base_ref)
        units = [u.strip() for u in args.row.split(",") if u.strip()]
        verdict = batch_orphans(args.root, units, snap, args.allow_orphan)
        for line in verdict["added"]:
            print("REFUSED  " + line)
        for line in verdict["accepted"]:
            print("ALLOWED  " + line)
        print("pre-existing (reported, not refused): %d, of which %d referenced by no other registered unit"
              % (len(verdict["pre_existing"]), len(verdict["sole_owned_debt"])))
        for line in verdict["pre_existing"][:10]:
            print("  " + line)
        print("row: %s" % ("FAIL, %d added" % len(verdict["added"]) if verdict["added"] else "PASS"))
        return 1 if verdict["added"] else 0
    if args.census:
        (records, stats), registered, read = census(args.root, args.unit or None)
        query = None if args.no_readers else readers_index(args.root)
        rep = census_report(records, stats, query)
        if args.json:
            with open(args.json, "w", encoding="utf-8") as fh:
                json.dump({"registered": registered, "read": read, **rep}, fh, indent=1)
            print(f"wrote {args.json}")
        print(render_census(rep, args.top, (registered, read)))
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
