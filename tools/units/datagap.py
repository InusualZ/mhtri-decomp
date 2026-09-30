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
    python tools/units/datagap.py --pool-seams       # literal pools as TU evidence: which units are ONE original TU
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
import poolseams  # noqa: E402  (literal pools as TU evidence: a deferral a pool-sharing group explains)

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


# -- the strict half: sole-owned orphans of a TOUCHED unit (owner, 2026-09-29: "Yes, refuse (strict)") ----------
#
# The add-only row above lets a pre-existing orphan through, reported. The owner's ruling: a batch that touches a
# unit must claim that unit's own data ("no data should be left behind"). "Own" is computed, never a file key:
# exactly ONE registered unit's target object references the address and `callers.py` finds no unsplit reader
# (`sole_owned_pairs`). Every such pair of a batch unit is refused, pre-existing or new, unless it can be shown
# the pair is not claimable on its own. Each deferred pair names its class and its reason in the row's output:
#
#   pool-synth       .sdata/.sdata2 and our object emits more of that section than the unit's claims carry: a
#                    claim is partial against a section the compiler still fills (playbook 23/29/58: the link dies
#                    or the flip fails). Needs our object, so a unit with no built object is never deferred here.
#   ambiguous-owner  the claim would sit beside a claim whose unit's `.text` order does not bracket this unit's
#                    (text-window order): the address is read by a unit that cannot be its TU, or dtk would order
#                    the units in a cycle.
#   span-blocked     the unit already claims that section and the gap between its claim and the run holds another
#                    unit's claim or data another registered unit references: one spanning claim is impossible and
#                    a second separate range is the playbook 53 cycle.
#   isolated-run     .sdata/.sdata2 only: the unit has no claim there and both neighbours of the run are other
#                    owners' data or unowned (auto) - a merged cross-TU literal pool the object cannot reproduce.
#
# Everything else is `refuse`. A run is the maximal stretch of adjacent map rows that are all the unit's own
# sole-owned orphans; runs merge into one block (one `splits.txt` range) while the gap between them holds no
# other owner's data, and one verdict covers the block.

POOL_SECTIONS = (".sdata", ".sdata2")
STRICT_CLASSES = ("pool-synth", "ambiguous-owner", "span-blocked", "isolated-run")
SECTION_ORDER = (".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data", ".bss",
                 ".sdata", ".sbss", ".sdata2", ".sbss2")


def section_rows(symbols: dict) -> dict[str, list[tuple[int, str, int]]]:
    """`{section: [(address, name, size)]}` sorted, one row per address (an alias at the same address is dropped)."""
    out: dict[str, dict[int, tuple]] = {}
    for name, e in symbols.items():
        if e.get("section") in CENSUS_SECTIONS and e.get("type") != "function":
            out.setdefault(e["section"], {}).setdefault(e["address"], (e["address"], name, int(e.get("size") or 0)))
    return {sec: [rows[a] for a in sorted(rows)] for sec, rows in out.items()}


def row_extent(rows: list, index: int) -> int:
    """Byte extent of `rows[index]`: its `size:`, else the distance to the next row, else one word."""
    address, _name, size = rows[index]
    if size > 0:
        return size
    if index + 1 < len(rows) and rows[index + 1][0] > address:
        return rows[index + 1][0] - address
    return 4


def text_starts(ranges: dict) -> dict[str, int]:
    """`{unit: first .text address}` from the claims (a data-only unit has none)."""
    out: dict[str, int] = {}
    for start, _end, unit in ranges.get(".text", []):
        out[unit] = min(start, out.get(unit, start))
    return out


def unit_claims(ranges: dict, unit: str, section: str) -> list[tuple[int, int]]:
    return [(s, e) for s, e, u in ranges.get(section, []) if u == unit]


def sole_owned_pairs(records: list[dict], query=None, units=None) -> tuple[list[dict], dict]:
    """`(pairs, stats)`: the orphan pairs exactly one registered unit's object references and nothing else reads.

    `query(address) -> ({unit: sites}, unsplit_sites)` is `readers_index`'s; with None the callers check is
    skipped (`stats["readers_checked"]` says so). `units` restricts which units are asked (the query is the
    expensive part). Pure given `query`.
    """
    want = set(units) if units is not None else None
    pairs, stats = [], {"readers_checked": query is not None, "shared": 0, "unsplit_reader": 0}
    for g in orphan_groups(records).values():
        if len(g["units"]) != 1:
            stats["shared"] += 1
            continue
        unit = next(iter(g["units"]))
        if want is not None and unit not in want:
            continue
        if query is not None:
            readers, unsplit = query(g["address"])
            if unsplit or not set(readers) <= {unit}:
                stats["unsplit_reader" if unsplit else "shared"] += 1
                continue
        pairs.append({"unit": unit, "name": g["name"], "section": g["section"], "address": g["address"],
                      "size": g["size"], "sites": g["units"][unit]})
    return pairs, stats


def _runs(pairs: list[dict], rows_by_section: dict) -> list[dict]:
    """Group one unit's sole-owned pairs into runs of adjacent map rows: `{unit, section, start, end, pairs}`."""
    import bisect

    out: list[dict] = []
    by_key: dict[tuple, list[dict]] = {}
    for p in pairs:
        by_key.setdefault((p["unit"], p["section"]), []).append(p)
    for (unit, section), plist in sorted(by_key.items()):
        rows = rows_by_section.get(section, [])
        addrs = [r[0] for r in rows]
        plist.sort(key=lambda p: p["address"])
        cur = None
        for p in plist:
            i = bisect.bisect_left(addrs, p["address"])
            known = i < len(rows) and rows[i][0] == p["address"]
            extent = row_extent(rows, i) if known else max(int(p["size"]) or 4, 4)
            end = p["address"] + extent
            if cur is not None and known and cur["last_index"] is not None and i == cur["last_index"] + 1:
                cur["pairs"].append(p)
                cur["end"] = end
                cur["last_index"] = i
            else:
                cur = {"unit": unit, "section": section, "start": p["address"], "end": end, "pairs": [p],
                       "last_index": i if known else None}
                out.append(cur)
    for run in out:
        run.pop("last_index", None)
        run["end"] = (run["end"] + 3) // 4 * 4
    return out


def _foreign_neighbour(rows: list, index: int, unit: str, section: str, ranges: dict, group_units: dict) -> bool:
    """Whether the map row at `index` (or the section's edge) is another owner's data or unowned."""
    if index < 0 or index >= len(rows):
        return True
    verdict = classify_address(section, rows[index][0], ranges, unit)
    if verdict["status"] == "own":
        return False
    if verdict["status"] == "other":
        return True
    return group_units.get((section, rows[index][0])) != {unit}


def _gap_blocker(lo: int, hi: int, unit: str, section: str, ctx: dict) -> str | None:
    """Why the bytes `[lo, hi)` cannot be claimed into `unit`'s one spanning range, or None.

    A blocker is another unit's claim in the gap, or a map row in it that another registered unit references or
    `callers.py` shows an unsplit reader for (data of some other TU: a second range would put it between two
    ranges of this unit - the playbook 53 link-order cycle). An unreferenced row is padding and does not block.
    """
    import bisect

    for s, e, u in ctx["ranges"].get(section, []):
        if u != unit and s < hi and lo < e:
            return "%s's claim 0x%08X-0x%08X" % (u, s, e)
    rows = ctx["rows"].get(section, [])
    query = ctx.get("query")
    for address, name, _size in rows[bisect.bisect_left(rows, (lo,)):]:
        if address >= hi:
            break
        units = ctx["group_units"].get((section, address))
        if units:
            if units != {unit}:
                return "%s (0x%08X), referenced by %s" % (name, address, ",".join(sorted(units)[:3]))
            continue
        if query is not None:
            readers, unsplit = query(address)
            if unsplit or not set(readers) <= {unit}:
                return "%s (0x%08X), read by %s" % (name, address,
                                                    ",".join(sorted(readers)[:3]) if readers else "unsplit code")
    return None


def judge_blocks(unit: str, section: str, runs: list[dict], ctx: dict) -> list[dict]:
    """The blocks one unit's runs in one section merge into, each with its verdict. Pure given `ctx`.

    Runs (and the unit's existing claims, as anchors) merge into one block while the gap between them holds no
    `_gap_blocker`. The block holding an existing claim - or, with none, the block with the most pairs - is the
    **main** one: its verdict is `refuse` unless a class below defers it. Every other block is `span-blocked`:
    claiming it is a second range of the section separated by another owner's data. Per block, in order:
    `pool-synth`, `ambiguous-owner`, then for the main block `isolated-run`. `ctx` carries `ranges`, `rows`,
    `group_units`, `text_start`, `our_sizes` and `query` (see `strict_report`).
    """
    import bisect

    ranges, rows = ctx["ranges"], ctx["rows"].get(section, [])
    mine = unit_claims(ranges, unit, section)
    claimed = sum(e - s for s, e in mine)
    items = ([{"start": s, "end": e, "pairs": [], "anchor": True} for s, e in mine]
             + [dict(r, anchor=False) for r in runs])
    items.sort(key=lambda r: r["start"])
    blocks: list[dict] = []
    for item in items:
        if blocks:
            gap = _gap_blocker(blocks[-1]["end"], item["start"], unit, section, ctx) \
                if item["start"] > blocks[-1]["end"] else None
            if gap is None:
                blocks[-1]["end"] = max(blocks[-1]["end"], item["end"])
                blocks[-1]["pairs"] += item["pairs"]
                blocks[-1]["anchor"] = blocks[-1]["anchor"] or item["anchor"]
                continue
            item["gap_before"] = gap
        blocks.append({"unit": unit, "section": section, "start": item["start"], "end": item["end"],
                       "pairs": list(item["pairs"]), "anchor": item["anchor"], "gap_before": item.get("gap_before")})
    blocks = [b for b in blocks if b["pairs"]]
    anchored = [b for b in blocks if b["anchor"]]
    main = anchored[0] if anchored else (max(blocks, key=lambda b: (len(b["pairs"]), -b["start"])) if blocks else None)
    our = ctx.get("our_sizes", {}).get(unit)
    tstart = ctx["text_start"]
    mine_t = tstart.get(unit)
    for b in blocks:
        start, end = b["start"], b["end"]
        b["verdict"], b["cls"], b["reason"] = "refuse", None, ""
        if section in POOL_SECTIONS and our is not None and our.get(section, 0) > claimed:
            b["verdict"], b["cls"] = "deferred", "pool-synth"
            b["reason"] = ("our object emits %d B of its own %s and the unit claims %d B: a claim of this run "
                           "is a partial pool (playbook 23/29/58)" % (our.get(section, 0), section, claimed))
            continue
        below = [(s, e, u) for s, e, u in ranges.get(section, []) if e <= start and u != unit]
        above = [(s, e, u) for s, e, u in ranges.get(section, []) if s >= end and u != unit]
        prev = max(below, key=lambda r: r[1]) if below else None
        nxt = min(above, key=lambda r: r[0]) if above else None
        if mine_t is not None:
            for other, bad in ((prev, lambda t: t > mine_t), (nxt, lambda t: t < mine_t)):
                if other and other[2] in tstart and bad(tstart[other[2]]):
                    b["verdict"], b["cls"] = "deferred", "ambiguous-owner"
                    b["reason"] = ("0x%08X-0x%08X sits beside %s's claim (.text 0x%08X) but this unit's .text is "
                                   "0x%08X: the text-window order does not bracket it"
                                   % (start, end, other[2], tstart[other[2]], mine_t))
                    break
            if b["verdict"] == "deferred":
                continue
        if b is not main:
            b["verdict"], b["cls"] = "deferred", "span-blocked"
            b["reason"] = ("a second %s range for the unit: %s separates it from the unit's main range (one "
                           "spanning claim is impossible, two is the playbook 53 cycle)"
                           % (section, b.get("gap_before") or "other data"))
            continue
        if b["anchor"]:
            b["reason"] = ("extends the unit's own %s claim (span claim, unreferenced padding included)" % section)
            continue
        if section in POOL_SECTIONS and rows:
            addrs = [r[0] for r in rows]
            i = bisect.bisect_left(addrs, start)
            j = bisect.bisect_left(addrs, end)
            if (_foreign_neighbour(rows, i - 1, unit, section, ranges, ctx["group_units"])
                    and _foreign_neighbour(rows, j, unit, section, ranges, ctx["group_units"])):
                b["verdict"], b["cls"] = "deferred", "isolated-run"
                b["reason"] = ("no claim of the unit in %s and both neighbours of 0x%08X-0x%08X are other owners' "
                               "or unowned data (a merged cross-TU pool)" % (section, start, end))
                continue
        b["reason"] = "a new %s range for the unit" % section
    return _explain_by_pool(unit, blocks, ctx)


#: the deferral classes a pool-sharing group can explain (docs/pool-seams.md): the claim is a partial pool of a TU
#: that the registry has cut into several units
POOL_FOLD_CLASSES = ("pool-synth", "isolated-run", "span-blocked", "ambiguous-owner")


def _explain_by_pool(unit: str, blocks: list[dict], ctx: dict) -> list[dict]:
    """Append the pool-sharing group to a deferred block's reason: a seam blocks the claim, not the tool.

    `ctx["pool"]` is `poolseams.build_groups`' census (absent = no change). The verdict and class never change,
    so this is add-only: a deferral stays a deferral and is only *named* better.
    """
    census = ctx.get("pool")
    group = poolseams.group_of(census, unit) if census else None
    if not group:
        return blocks
    for b in blocks:
        if b["verdict"] == "deferred" and b["cls"] in POOL_FOLD_CLASSES and b["section"] in POOL_SECTIONS:
            b["reason"] += "; %s - the claim is blocked by a seam, not by the tool" % poolseams.fold_line(group, unit)
            b["fold"] = list(group["units"])
    return blocks


def strict_report(root: str, records: list[dict], ranges: dict, units: list[str] | None = None,
                  query="lazy", symbols: dict | None = None) -> dict:
    """The strict row's whole evidence for `units` (all registered units when None): blocks with their verdicts.

    Returns `{blocks, stats, ctx}`; each block is `{unit, section, start, end, pairs, verdict, cls, reason}`.
    """
    symbols = symbols if symbols is not None else load_data_symbols(root)
    if query == "lazy":
        try:
            query = readers_index(root)
        except Exception:                                                    # noqa: BLE001 - no index: reloc-only
            query = None
    pairs, stats = sole_owned_pairs(records, query, units)
    ctx = {"ranges": ranges, "rows": section_rows(symbols), "query": query,
           "group_units": {k: set(g["units"]) for k, g in orphan_groups(records).items()},
           "text_start": text_starts(ranges), "our_sizes": {},
           # the pool-sharing census over the SAME records/claims (no second reader pass)
           "pool": poolseams.build_groups(records, ranges, poolseams.literal_table(symbols))}
    for unit in sorted({p["unit"] for p in pairs}):
        path = os.path.join(root, "build", GAME_DIR, "src", unit + ".o")
        try:
            ctx["our_sizes"][unit] = section_sizes(path) if os.path.exists(path) else None
        except Exception:                                                    # noqa: BLE001
            ctx["our_sizes"][unit] = None
    blocks: list[dict] = []
    grouped: dict[tuple, list[dict]] = {}
    for run in _runs(pairs, ctx["rows"]):
        grouped.setdefault((run["unit"], run["section"]), []).append(run)
    for (unit, section), runs in sorted(grouped.items()):
        blocks += judge_blocks(unit, section, runs, ctx)
    stats["pairs"] = len(pairs)
    return {"blocks": blocks, "stats": stats, "ctx": ctx}


def strict_counts(report: dict) -> dict:
    """`{cls-or-refuse: pairs}` over a strict report's runs (a run counts its pair count)."""
    out: dict[str, int] = {}
    for run in report["blocks"]:
        key = run["cls"] if run["verdict"] == "deferred" else "refuse"
        out[key] = out.get(key, 0) + len(run["pairs"])
    return out


def render_strict(report: dict, names_shown: int = 4) -> str:
    """One screen: the counts, then every block with its verdict, pair count, reason and first names."""
    counts = strict_counts(report)
    stats = report["stats"]
    lines = ["strict data claim: %d sole-owned pair(s) in %d block(s) - refusable %d, %s%s"
             % (stats.get("pairs", 0), len(report["blocks"]), counts.get("refuse", 0),
                ", ".join("%s %d" % (c, counts.get(c, 0)) for c in STRICT_CLASSES),
                "" if stats.get("readers_checked") else "  (callers index unavailable: references only)")]
    for block in sorted(report["blocks"], key=lambda b: (b["unit"], SECTION_ORDER.index(b["section"])
                                                         if b["section"] in SECTION_ORDER else 99, b["start"])):
        tag = "REFUSE  " if block["verdict"] == "refuse" else "deferred %s" % block["cls"]
        names = ", ".join(p["name"] for p in block["pairs"][:names_shown])
        lines.append("  %-22s %-28s %-8s 0x%08X-0x%08X  %3d pair(s): %s%s"
                     % (tag, block["unit"], block["section"], block["start"], block["end"], len(block["pairs"]),
                        names, " ..." if len(block["pairs"]) > names_shown else ""))
        lines.append("      " + block["reason"])
    return "\n".join(lines)


def classify_pairs(report: dict) -> dict[str, dict]:
    """`{orphan_key: {verdict, cls, reason, pair}}` for every pair of a strict report's blocks. Pure.

    The ONE classification both halves of the data-closure row read: the strict half refuses a `refuse` pair of a
    touched unit, the add-only half defers a NEW pair whose class is deferred instead of refusing it.
    """
    return {orphan_key(p["unit"], p["section"], p["address"]):
            {"verdict": run["verdict"], "cls": run["cls"], "reason": run["reason"], "pair": p}
            for run in report["blocks"] for p in run["pairs"]}


def pair_line(p: dict, reason: str) -> str:
    return "%s %s 0x%08X (%s, %d site(s)): %s" % (p["unit"], p["name"], p["address"], p["section"], p["sites"], reason)


def strict_verdict(report: dict, skip_keys=(), allowed=()) -> dict:
    """The strict row's decision. Pure.

    `refused` is one line per refusable pair not already in `skip_keys` (the add-only row refuses those itself)
    and not excused by `allowed` (hex addresses; an address inside the pair's object excuses it); `accepted` is
    what an allowance excused; `deferred` is `{cls: [line]}`, reported and never refused.
    """
    sanction = set()
    for token in allowed:
        try:
            sanction.add(int(str(token).strip(), 16))
        except ValueError:
            continue
    refused, accepted, deferred, used = [], [], {}, set()
    for key, c in classify_pairs(report).items():
        p = c["pair"]
        line = pair_line(p, c["reason"])
        if c["verdict"] == "deferred":
            deferred.setdefault(c["cls"], []).append(line)
            continue
        if key in skip_keys:
            continue
        hit = [a for a in sanction if p["address"] <= a < p["address"] + max(int(p["size"]), 1)]
        if hit:
            used.update(hit)
            accepted.append(line)
        else:
            refused.append(line)
    return {"refused": refused, "accepted": accepted, "deferred": deferred, "used": sorted(used),
            "unmatched_allowances": sorted("0x%08X" % a for a in sanction - used)}


def splits_plan(text: str, unit: str, blocks: list[dict], our_sizes: dict | None = None) -> dict:
    """The exact `splits.txt` edit that claims `unit`'s refusable blocks, in link-order position. Pure.

    The unit's block in `text` is already where the link order wants it (units are listed in address order), so
    the edit is inside it: each refusable block becomes one line in the section order of the file's `Sections:`
    table, or - when the unit already claims that section - the REPLACEMENT of its line(s) by one spanning
    range (two ranges with a gap between them is the playbook 53 cycle). Returns `{header, edits, result,
    deferred, found}`: `edits` are `{action, section, old, new, start, end, note}`, `result` is the unit's block
    as it would read, `deferred` the blocks the row would not refuse, with their class. Never applied.
    """
    import re

    lines = text.splitlines()
    head = None
    for i, line in enumerate(lines):
        m = re.match(r"^([^\s:][^:]*):\s*$", line)
        if m and os.path.splitext(m.group(1))[0] == unit:
            head = i
            break
    if head is None:
        return {"found": False, "header": None, "edits": [], "result": [], "deferred": []}
    j = head + 1
    body = []
    while j < len(lines) and lines[j].strip():
        body.append(lines[j])
        j += 1
    parsed = []
    for line in body:
        m = re.match(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)(.*)$", line)
        parsed.append({"section": m.group(1), "start": int(m.group(2), 16), "end": int(m.group(3), 16),
                       "tail": m.group(4), "line": line, "new": False} if m else
                      {"section": None, "line": line, "new": False})
    edits, deferred = [], []
    for block in sorted(blocks, key=lambda b: (SECTION_ORDER.index(b["section"]) if b["section"] in SECTION_ORDER
                                               else 99, b["start"])):
        if block["verdict"] != "refuse":
            deferred.append(block)
            continue
        section = block["section"]
        same = [r for r in parsed if r["section"] == section]
        start, end = block["start"], block["end"]
        for r in same:
            start, end = min(start, r["start"]), max(end, r["end"])
        new_line = splits_line(section, start, end)
        ours = None if our_sizes is None else our_sizes.get(section, 0)
        total = end - start
        note = ("our object emits %d B of %s and the claim totals %d B" % (ours, section, total)
                if ours is not None else "no built object for the unit: the claim is unmeasured")
        if ours is not None and ours != total:
            if section in POOL_SECTIONS:
                note += ("; PARTIAL RUN: the section pairs %d B of ours against %d B of the target - a claim the "
                         "source does not define or declare in full lowers the score and can fail the link "
                         "(playbook 23/29/58)" % (ours, total))
            else:
                note += ("; partial until the source defines the %d B the claim adds (rule 12: claim it, then "
                         "reconstruct it to byte-match)" % (total - ours))
        if same:
            edits.append({"action": "replace", "section": section, "old": [r["line"] for r in same],
                          "new": new_line, "start": start, "end": end, "note": note,
                          "pairs": len(block["pairs"])})
            first = parsed.index(same[0])
            parsed[first] = {"section": section, "start": start, "end": end, "tail": same[0]["tail"],
                             "line": new_line, "new": True}
            for r in same[1:]:
                parsed.remove(r)
        else:
            edits.append({"action": "add", "section": section, "old": [], "new": new_line, "start": start,
                          "end": end, "note": note, "pairs": len(block["pairs"])})
            at = len(parsed)
            for k, r in enumerate(parsed):
                if r["section"] in SECTION_ORDER and SECTION_ORDER.index(r["section"]) > SECTION_ORDER.index(section):
                    at = k
                    break
            parsed.insert(at, {"section": section, "start": start, "end": end, "tail": "", "line": new_line,
                               "new": True})
    return {"found": True, "header": lines[head], "edits": edits, "deferred": deferred,
            "result": [("+ " if r["new"] else "  ") + r["line"] for r in parsed]}


def splits_line(section: str, start: int, end: int) -> str:
    return "\t%-11s start:0x%08X end:0x%08X" % (section, start, end)


def render_plan(plan: dict, unit: str) -> str:
    """The plan as text a lane can paste: per-edit action lines, then the unit's resulting block."""
    if not plan["found"]:
        return "no `%s` block in splits.txt: register the unit first" % unit
    lines = ["splits.txt edit for `%s` (NOT applied; force a re-split after it - `rm build/RMHE08/config.json` - and "
             "measure the unit before and after, playbook 23):" % unit]
    if not plan["edits"]:
        lines.append("  nothing to add: no refusable sole-owned data block")
    for e in plan["edits"]:
        if e["action"] == "replace":
            lines.append("  REPLACE %s" % " + ".join(x.strip() for x in e["old"]))
            lines.append("     WITH %s   (%d pair(s))" % (e["new"].strip(), e["pairs"]))
        else:
            lines.append("  ADD     %s   (%d pair(s), in section order inside the unit's block)"
                         % (e["new"].strip(), e["pairs"]))
        lines.append("          %s" % e["note"])
    for b in plan["deferred"]:
        lines.append("  deferred %s %s 0x%08X-0x%08X (%d pair(s)): %s"
                     % (b["cls"], b["section"], b["start"], b["end"], len(b["pairs"]), b["reason"]))
    lines.append("resulting block (+ = new):")
    lines.append("  " + plan["header"])
    lines += ["  " + r for r in plan["result"]]
    return "\n".join(lines)


def unit_plan(root: str, unit: str) -> dict:
    """`{plan, report}` for one unit as it stands in `root` (census over every registered object)."""
    unit = os.path.splitext(unit.replace("\\", "/"))[0]
    ranges = load_claims(root)
    (records, _stats), _n, _have = census(root, None, ranges=ranges)
    report = strict_report(root, records, ranges, [unit])
    with open(os.path.join(root, "config", GAME_DIR, "splits.txt"), encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    blocks = [b for b in report["blocks"] if b["unit"] == unit]
    return {"plan": splits_plan(text, unit, blocks, report["ctx"]["our_sizes"].get(unit)), "report": report}


def load_claims(root: str) -> dict:
    path = os.path.join(root, "config", GAME_DIR, "splits.txt")
    with open(path, encoding="utf-8", errors="replace") as fh:
        return parse_splits_text(fh.read())


def splits_at_ref(root: str, ref: str) -> dict:
    """The parsed `splits.txt` as committed at `ref` (a what-if base for a branch's own diff)."""
    import subprocess

    text = subprocess.run(["git", "show", "%s:config/%s/splits.txt" % (ref, GAME_DIR)], cwd=root,
                          capture_output=True, text=True, encoding="utf-8", errors="replace", check=True).stdout
    return parse_splits_text(text)


def claims_at_ref(root: str, ref: str) -> dict:
    """`claimed_bytes` of the `splits.txt` as committed at `ref` (a what-if base for a branch's own diff)."""
    return {sec: [list(r) for r in rows] for sec, rows in claimed_bytes(splits_at_ref(root, ref)).items()}


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


# -- which batch units the STRICT row judges: "touched" means a REAL change (owner, 2026-09-29: "Only real changes") --
#
# A unit is TOUCHED for the strict row only when the batch
#   (a) registers it (it has claims now and none at the base),
#   (b) recuts it: its merged `splits.txt` claims differ from the base's in any section, or
#   (c) changes its COMPILED object `build/RMHE08/src/<unit>.o` against the base's (`object_fingerprint`): any
#       section's bytes or flags, any defined symbol's section/offset/size/binding/type, any relocation's
#       (section, offset, type, addend) or a relocation to a defined symbol whose own section/offset moved, or an
#       EXTERNAL relocation whose target changed. An external target is the same when the two maps resolve the two
#       names to the same ADDRESS (a rename sweep) or when the name is spelled the same (a map row renamed into
#       an unresolved name the source already used; a name the map lacks resolves by its `linkage_stem`, the one
#       symbol under two linkages `undefrefs` already equates); it differs only when both resolve and the addresses differ,
#       or the spellings differ and one does not resolve. A pure rename sweep and an edit that compiles to the
#       same object touch nothing.
# A unit with no compiled object falls back to (a)/(b); a unit whose base snapshot carries no claims record (an old
# `record-base`) is conservatively touched, reason "no base record"; a unit with an object and no base object is
# touched ("no base object to compare"). A source file edited is NOT, by itself, a touch.

UNSTABLE_SECTIONS = (".symtab", ".strtab", ".shstrtab", ".comment")


def object_fingerprint(path: str, symbols: dict | None = None) -> dict | None:
    """`{body, ext}` of a compiled object, or None when it cannot be read. See the TOUCHED rule above.

    `body` hashes everything that is not an external target (sections, defined symbols, relocations to defined
    symbols by the target's section/offset); `ext` is `{"section|offset|type|addend": [name, "SECTION:ADDR"|None]}`
    for the external relocations, the address being what `symbols` (`{name: {section, address}}`, the tree's own
    map) says. JSON-ready: this is what `record-base` stores per batch unit.
    """
    import hashlib

    from units import dossier as dossier_mod  # noqa: PLC0415 - the one ELF reader
    from units import undefrefs as uref  # noqa: PLC0415 - `linkage_stem`: one symbol under two linkages

    try:
        with open(path, "rb") as fh:
            blob = fh.read()
        sections, syms, relocs = dossier_mod.parse_elf(blob)
    except (OSError, ValueError):
        return None
    symbols = symbols or {}
    h = hashlib.sha1()
    for sec in sections:
        if sec["name"] in UNSTABLE_SECTIONS or sec["typ"] == 4 or not sec["name"]:
            continue
        h.update(("S|%s|%d|%d|%d|%d|" % (sec["name"], sec["typ"], sec["flags"], sec["align"], sec["size"])).encode())
        h.update(sec["data"] if sec["typ"] != 8 else b"")
    by_name = {s["name"]: s for s in syms if s["name"] and s["shndx"]}
    rows = ["D|%s|%d|%d|%d|%d" % (s["section"], s["value"], s["size"], s["bind"], s["type"])
            for s in syms if s["name"] and s["shndx"]]
    ext: dict[str, list] = {}
    for r in relocs:
        name = r["symbol"] or ""
        own = by_name.get(name)
        if own is not None:
            rows.append("R|%s|%d|%d|%d|L|%s|%d" % (r["target"], r["offset"], r["type"], r["addend"], own["section"],
                                                   own["value"]))
            continue
        rows.append("R|%s|%d|%d|%d|X" % (r["target"], r["offset"], r["type"], r["addend"]))
        entry = symbols.get(name) or symbols.get(uref.linkage_stem(name))
        ext["%s|%d|%d|%d" % (r["target"], r["offset"], r["type"], r["addend"])] = \
            [name, "%s:%d" % (entry["section"], entry["address"]) if entry else None]
    for row in sorted(rows):
        h.update(row.encode() + b"\n")
    return {"body": h.hexdigest(), "ext": ext}


def fingerprints_equal(a: dict, b: dict) -> bool:
    """Whether two `object_fingerprint`s are the same object under the TOUCHED rule above. Pure."""
    if a.get("body") != b.get("body") or set(a.get("ext") or {}) != set(b.get("ext") or {}):
        return False
    for key, (name_a, addr_a) in (a.get("ext") or {}).items():
        name_b, addr_b = b["ext"][key]
        if addr_a is not None and addr_b is not None:
            if addr_a != addr_b:
                return False
        elif name_a != name_b:
            return False
    return True


def unit_claim_table(ranges: dict) -> dict[str, dict[str, list[list[int]]]]:
    """`{unit: {section: merged [[start, end], ...]}}` from parsed claims - the per-unit base a recut is judged on."""
    per: dict[str, dict[str, list]] = {}
    for section, rows in ranges.items():
        for start, end, unit in rows:
            per.setdefault(unit, {}).setdefault(section, []).append((start, end))
    return {u: {sec: [list(r) for r in merged(rs)] for sec, rs in secs.items()} for u, secs in per.items()}


def object_path(root: str, unit: str) -> str:
    return os.path.join(root, "build", GAME_DIR, "src", unit + ".o")


def touch_verdicts(units: list[str], base: dict, now_claims: dict, now_objects: dict) -> dict[str, dict]:
    """`{unit: {touched, reasons}}` under the TOUCHED rule above. Pure.

    `base` is the recorded snapshot (`unit_claims`, `objects`), `now_claims` a `unit_claim_table`, `now_objects`
    `{unit: fingerprint or None}`. `reasons` is ordered registered / claims changed / object changed; empty means
    not touched. A snapshot with no `unit_claims` cannot tell, so every unit is touched ("no base record").
    """
    out = {}
    base_claims = base.get("unit_claims")
    base_objects = base.get("objects")
    for unit in units:
        if base_claims is None:
            out[unit] = {"touched": True, "reasons": ["no base record of the claims (old record-base): judged touched"]}
            continue
        reasons = []
        was, now = base_claims.get(unit) or {}, now_claims.get(unit) or {}
        if now and not was:
            reasons.append("registered")
        elif was != now:
            secs = sorted({sec for sec in set(was) | set(now) if was.get(sec) != now.get(sec)})
            reasons.append("claims changed (%s)" % ", ".join(secs))
        cur = now_objects.get(unit)
        if cur is not None:
            if base_objects is None or unit not in base_objects:
                reasons.append("object changed (no base object to compare)")
            elif not fingerprints_equal(base_objects[unit], cur):
                reasons.append("object changed")
        out[unit] = {"touched": bool(reasons), "reasons": reasons}
    return out


def render_touch(unit: str, verdict: dict) -> str:
    if verdict["touched"]:
        return "%-34s TOUCHED: %s" % (unit, "; ".join(verdict["reasons"]))
    return "%-34s not touched (claims and compiled object unchanged: rename-only / no real change)" % unit


def snapshot_orphans(root: str, units: list[str] | None = None) -> dict:
    """The base snapshot `land.record_base` stores: every pre-existing orphan pair, the claimed bytes, each unit's
    claims and the compiled-object fingerprint of `units` (none when None: the manual flow names no batch)."""
    ranges = load_claims(root)
    (records, _stats), _n, _have = census(root, ranges=ranges)
    table = unit_claim_table(ranges)
    symbols = load_data_symbols(root)
    names = [os.path.splitext(u)[0] for u in units] if units is not None else []
    objects = {}
    for unit in names:
        if os.path.exists(object_path(root, unit)):
            fp = object_fingerprint(object_path(root, unit), symbols)
            if fp is not None:
                objects[unit] = fp
    return {"keys": sorted(orphan_keys(records)),
            "claims": {sec: [list(r) for r in rows] for sec, rows in claimed_bytes(ranges).items()},
            "unit_claims": table, "objects": objects}


def batch_orphans(root: str, units: list[str], base_snapshot: dict | None, allowed=(), strict=True,
                  query="lazy", touched=None) -> dict:
    """The gate row's whole decision for the batch units against the recorded base (see `orphan_verdict`).

    `touched` (`{unit: {touched, reasons}}`) overrides `touch_verdicts` - a test's or a caller's own judgement.
    """
    ranges = load_claims(root)
    names = [os.path.splitext(u)[0] for u in units]
    (records, _stats), _n, _have = census(root, None, ranges=ranges)
    after = orphan_keys(records, names)
    snap = base_snapshot or {}
    base_claims = {sec: [tuple(r) for r in rows] for sec, rows in (snap.get("claims") or {}).items()}
    shrinks = shrunk_claims(base_claims, claimed_bytes(ranges)) if base_claims else []
    base_keys = set(snap.get("keys") or [])
    # STRICT (owner, 2026-09-29): a unit the batch REALLY changes must claim the data only it references - the
    # pre-existing pairs too. `strict_report` decides per block (refuse, or deferred with a named class) and
    # `classify_pairs` is the one classification both halves read: the add-only half below defers a NEW pair whose
    # block is deferred (isolated-run / pool-synth / span-blocked / ambiguous-owner) instead of refusing it.
    report = strict_report(root, records, ranges, names, query=query) if strict else None
    added_deferred: dict[str, dict] = {}
    if report is not None:
        classes = classify_pairs(report)
        added_deferred = {k: classes[k] for k in after if k not in base_keys and k in classes
                          and classes[k]["verdict"] == "deferred"}
    refusable = {k: v for k, v in after.items() if k not in added_deferred}
    verdict = orphan_verdict(snap.get("keys") or [], refusable, shrinks, allowed)
    verdict["added_deferred"] = [pair_line(c["pair"], "deferred %s, a new pair, not refused: %s" % (c["cls"], c["reason"]))
                                 for c in added_deferred.values()]
    verdict["have_base"] = "keys" in snap and "claims" in snap
    if report is not None:
        symbols = load_data_symbols(root)
        now_objects = {}
        for unit in names:
            if os.path.exists(object_path(root, unit)):
                now_objects[unit] = object_fingerprint(object_path(root, unit), symbols)
        touch = touched if touched is not None else touch_verdicts(names, snap, unit_claim_table(ranges), now_objects)

        def is_touched(unit):
            return touch.get(unit, {"touched": True})["touched"]

        strict = dict(report, blocks=[b for b in report["blocks"] if is_touched(b["unit"])])
        untouched_pairs: dict[str, int] = {}
        for b in report["blocks"]:
            if not is_touched(b["unit"]):
                untouched_pairs[b["unit"]] = untouched_pairs.get(b["unit"], 0) + len(b["pairs"])
        sv = strict_verdict(strict, skip_keys=set(after) - base_keys, allowed=allowed)
        verdict["strict"] = sv
        verdict["touch"] = touch
        verdict["untouched_pairs"] = untouched_pairs
        verdict["strict_counts"] = strict_counts(strict)
        verdict["strict_blocks"] = strict["blocks"]
        verdict["strict_stats"] = strict["stats"]
        verdict["sole_owned"] = sv["refused"]
        sanction = set()
        for token in allowed:
            try:
                sanction.add(int(str(token).strip(), 16))
            except ValueError:
                continue
        used = set(sv["used"]) | {r["address"] for k, r in after.items()
                                   if k not in base_keys and r["address"] in sanction}
        used |= {a for a in sanction for _s, lo, hi in shrinks if lo <= a < hi}
        verdict["unmatched_allowances"] = sorted("0x%08X" % a for a in sanction - used)
        verdict["sole_owned_debt"] = sv["refused"] + sv["accepted"] + [ln for v in sv["deferred"].values() for ln in v]
    else:
        verdict["strict"] = {"refused": [], "accepted": [], "deferred": {}, "used": [], "unmatched_allowances": []}
        verdict["touch"], verdict["untouched_pairs"] = {}, {}
        verdict["strict_counts"], verdict["strict_blocks"], verdict["strict_stats"] = {}, [], {}
        verdict["unmatched_allowances"] = []
        verdict["sole_owned"], verdict["sole_owned_debt"] = [], []
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


def build_fixture_object(path: str, refs: list[str], defined: list[str] = (), fill: int = 0) -> None:
    """Write a minimal ELF32-BE object: `.text` with one `R_PPC_ADDR16_HA` relocation per name in `refs`
    (undefined symbols) and an empty `.data` defining each name in `defined`; `fill` is the `.text` byte value (a body edit).
    Enough for `parse_elf`."""
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

    bodies = [b"", bytes([fill]) * max(4, 4 * len(refs)), b"\0" * 4, rela, syms, strtab, shstr]
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


def selftest_strict(eq) -> None:
    """The strict half: classification classes, the row's verdict, allowance plumbing, the `splits.txt` plan."""
    import tempfile

    splits = ("Sections:\n\t.text type:code align:32\n\n"
              "A/a.cpp:\n\t.text start:0x80010000 end:0x80010100\n\t.data start:0x805E0000 end:0x805E0010\n\n"
              "B/b.cpp:\n\t.text start:0x80010100 end:0x80010200\n\t.data start:0x805E0100 end:0x805E0110\n\n"
              "C/c.cpp:\n\t.text start:0x80010200 end:0x80010300\n\t.rodata start:0x80500000 end:0x80500010\n")
    ranges = parse_splits_text(splits)

    def sym(section, address, size=4):
        return {"section": section, "address": address, "size": size, "type": "object"}

    symbols = {"a1": sym(".data", 0x805E0000), "p1": sym(".data", 0x805E0010), "q": sym(".data", 0x805E0020),
               "x": sym(".data", 0x805E0040), "y": sym(".data", 0x805E0060),
               "ro": sym(".rodata", 0x80500010),
               "f0": sym(".sdata2", 0x8078FFFC), "s1": sym(".sdata2", 0x80790000), "f1": sym(".sdata2", 0x80790004)}
    refs = {"A/a": {"p1": 1, "q": 2, "y": 1, "s1": 3, "ro": 1}, "B/b": {"f0": 1, "f1": 1}, "C/c": {"x": 1}}
    records, _stats = census_records(refs, symbols, ranges)
    with tempfile.TemporaryDirectory() as tmp:
        rep = strict_report(tmp, records, ranges, ["A/a"], query=None, symbols=symbols)
        by = {(b["section"], b["start"]): b for b in rep["blocks"]}
        data_main = by[(".data", 0x805E0000)]
        eq((data_main["verdict"], data_main["cls"], [p["name"] for p in data_main["pairs"]]),
           ("refuse", None, ["p1", "q"]), "a run beside the unit's own claim merges into it and is refused")
        eq((data_main["start"], data_main["end"]), (0x805E0000, 0x805E0024), "the block is the span claim")
        far = by[(".data", 0x805E0060)]
        eq((far["verdict"], far["cls"]), ("deferred", "span-blocked"),
           "a second range separated by another unit's data is span-blocked")
        eq("x (0x805E0040)" in far["reason"], True, "... and the reason names the blocker")
        eq(by[(".sdata2", 0x80790000)]["cls"], "isolated-run",
           "a literal with both neighbours other owners' and no claim is an isolated run")
        eq(by[(".rodata", 0x80500010)]["cls"], "ambiguous-owner",
           "data beside a claim whose unit's .text is later is ambiguous by text-window order")
        eq(strict_counts(rep), {"refuse": 2, "span-blocked": 1, "isolated-run": 1, "ambiguous-owner": 1},
           "counts are per pair and per class")

        # (b) our object emits its own pool: pool-synth wins over the neighbour test
        ctx = dict(rep["ctx"], our_sizes={"A/a": {".sdata2": 8}})
        run = {"unit": "A/a", "section": ".sdata2", "start": 0x80790000, "end": 0x80790004,
               "pairs": [{"unit": "A/a", "name": "s1", "section": ".sdata2", "address": 0x80790000, "size": 4,
                          "sites": 3}]}
        got = judge_blocks("A/a", ".sdata2", [run], ctx)
        eq((got[0]["verdict"], got[0]["cls"]), ("deferred", "pool-synth"),
           "a pool section our object fills beyond the claim is deferred as pool-synth")
        ctx0 = dict(rep["ctx"], our_sizes={"A/a": {".sdata2": 0}})
        eq(judge_blocks("A/a", ".sdata2", [run], ctx0)[0]["cls"], "isolated-run", "no own pool: the next test decides")
        eq(judge_blocks("A/a", ".sdata2", [run], dict(ctx0, our_sizes={"A/a": None}))[0]["cls"], "isolated-run",
           "a unit with no built object is never deferred as pool-synth")
        # a pool-sharing group names the seam behind a deferral (add-only: verdict and class are unchanged)
        pool = poolseams.build_groups(
            census_records({"A/a": {"s1": 1}, "B/b": {"s1": 1, "f1": 1}}, symbols, ranges)[0], ranges,
            poolseams.literal_table(symbols))
        named = judge_blocks("A/a", ".sdata2", [run], dict(ctx0, pool=pool))[0]
        eq((named["verdict"], named["cls"]), ("deferred", "isolated-run"), "a pool group leaves the verdict alone")
        eq("candidate fold: A/a with B/b" in named["reason"] and "blocked by a seam, not by the tool" in named["reason"],
           True, "... and names the fold in the reason")
        eq(named["fold"], ["A/a", "B/b"], "... and lists the units")
        eq("candidate fold" in judge_blocks("A/a", ".sdata2", [run], ctx0)[0]["reason"], False,
           "no pool census: no fold note")
        # a non-pool section never defers on our object's emission
        drun = dict(run, section=".data", start=0x805E0010, end=0x805E0014,
                    pairs=[dict(run["pairs"][0], section=".data", address=0x805E0010)])
        eq(judge_blocks("A/a", ".data", [drun], ctx)[0]["verdict"], "refuse", ".data is never pool-synth")

        # the row's decision: pre-existing pairs are refused, an allowance excuses one, deferred pairs are reported
        ver = strict_verdict(rep)
        eq((len(ver["refused"]), sorted(ver["deferred"])), (2, ["ambiguous-owner", "isolated-run", "span-blocked"]),
           "refusable pairs are refused; each deferred class is reported with its pairs")
        eq(all("adjacent" in ln or "span claim" in ln for ln in ver["refused"]), True, "every refusal carries its reason")
        eq(len(strict_verdict(rep, allowed=["0x805E0010"])["refused"]), 1, "an allowance on the pair's address excuses it")
        eq(len(strict_verdict(rep, allowed=["0x805E0012"])["refused"]), 1,
           "... so does an address inside the pair's object")
        eq(strict_verdict(rep, allowed=["0x805E0FFF"])["unmatched_allowances"], ["0x805E0FFF"],
           "an allowance that matches nothing is reported and excuses nothing")
        eq(len(strict_verdict(rep, allowed=["0x805E0FFF"])["refused"]), 2, "... the refusals stand")
        eq(len(strict_verdict(rep, skip_keys={orphan_key("A/a", ".data", 0x805E0010)})["refused"]), 1,
           "a pair the add-only half already refused is not refused twice")

        # sharing: a second registered reader, or an unsplit reader, makes it not sole-owned
        shared_refs = dict(refs, **{"B/b": {"p1": 1}})
        shared_records, _s = census_records(shared_refs, symbols, ranges)
        pairs, stats = sole_owned_pairs(shared_records, None, ["A/a"])
        eq("p1" in [p["name"] for p in pairs], False, "a word a second unit's object references is shared, not refused")
        pairs, stats = sole_owned_pairs(records, lambda a: ({"A/a": 1}, 1 if a == 0x805E0010 else 0), ["A/a"])
        eq("p1" in [p["name"] for p in pairs], False, "an unsplit reader keeps the word out of the strict set")
        eq(stats["unsplit_reader"], 1, "... and is counted")
        pairs, _s = sole_owned_pairs(records, lambda a: ({"A/a": 1, "D/d": 1} if a == 0x805E0020 else {}, 0), ["A/a"])
        eq("q" in [p["name"] for p in pairs], False, "another registered reader found by callers keeps it out too")
        pairs, _s = sole_owned_pairs(records, None, ["C/c"])
        eq([p["name"] for p in pairs], ["x"], "units restricts whose pairs are judged")

        # the end-to-end row: a touched unit is refused for its PRE-EXISTING sole-owned orphan, an untouched one is not
        import json as _json
        os.makedirs(os.path.join(tmp, "config", "RMHE08"), exist_ok=True)
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj", "A"), exist_ok=True)
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj", "B"), exist_ok=True)
        with open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(splits)
        with open(os.path.join(tmp, "config", "RMHE08", "symbols.txt"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write("a1 = .data:0x805E0000; // type:object size:0x4\np1 = .data:0x805E0010; // type:object size:0x4\n")
        build_fixture_object(os.path.join(tmp, "build", "RMHE08", "obj", "A", "a.o"), ["p1"])
        build_fixture_object(os.path.join(tmp, "build", "RMHE08", "obj", "B", "b.o"), ["a1"])
        snap = snapshot_orphans(tmp)
        eq(snap["keys"], [orphan_key("A/a", ".data", 0x805E0010)], "the fixture's one pre-existing orphan is in the base")
        touched = batch_orphans(tmp, ["A/a"], snap, query=None,
                                touched={"A/a": {"touched": True, "reasons": ["registered"]}})
        eq((touched["added"], len(touched["sole_owned"])), ([], 1),
           "a TOUCHED unit with a pre-existing sole-owned orphan is refused (not merely reported)")
        eq(touched["strict_counts"], {"refuse": 1}, "... with the counts alongside")
        untouched = batch_orphans(tmp, ["B/b"], snap, query=None)
        eq((untouched["added"], untouched["sole_owned"]), ([], []),
           "the same orphan with its unit UNTOUCHED is not refused")
        allowed = batch_orphans(tmp, ["A/a"], snap, ["0x805E0010"], query=None,
                                touched={"A/a": {"touched": True, "reasons": ["registered"]}})
        eq((allowed["sole_owned"], len(allowed["strict"]["accepted"]), allowed["unmatched_allowances"]), ([], 1, []),
           "--allow-orphan <addr> excuses it, recorded as accepted")
        wrong = batch_orphans(tmp, ["A/a"], snap, ["0x805E0044"], query=None,
                              touched={"A/a": {"touched": True, "reasons": ["registered"]}})
        eq((len(wrong["sole_owned"]), wrong["unmatched_allowances"]), (1, ["0x805E0044"]),
           "an allowance that matches nothing keeps the refusal and is listed as unmatched")
        off = batch_orphans(tmp, ["A/a"], snap, query=None, strict=False)
        eq(off["sole_owned"], [], "strict=False is the old add-only row")
        _ = _json

        # the migration aid: the exact edit, in section order, replacing the unit's own line when it has one
        plan = splits_plan(splits, "A/a", [data_main, far])
        eq([(e["action"], e["section"], e["new"].strip()) for e in plan["edits"]],
           [("replace", ".data", ".data       start:0x805E0000 end:0x805E0024")],
           "the plan replaces the unit's .data line with the spanning range")
        eq([b["cls"] for b in plan["deferred"]], ["span-blocked"], "... and lists the deferred block with its class")
        eq(plan["result"][-1].strip().startswith("+ .data") or plan["result"][-1].startswith("+ "), True,
           "the resulting block marks the new line")
        ro = {"unit": "C/c", "section": ".sdata", "start": 0x80793000, "end": 0x80793008, "verdict": "refuse",
              "cls": None, "pairs": [1, 2], "reason": "x"}
        plan2 = splits_plan(splits, "C/c", [ro], {".sdata": 0})
        eq(plan2["edits"][0]["action"], "add", "a section the unit does not claim is an ADD")
        eq([r.split()[-3] if r.startswith("+") else "" for r in plan2["result"]].count(".sdata"), 1,
           "... placed in the result")
        eq("partial until the source defines" in plan2["edits"][0]["note"] or "PARTIAL RUN" in plan2["edits"][0]["note"],
           True, "... with a partial-run note when our object emits less than the claim")
        eq(splits_plan(splits, "Z/z", [])["found"], False, "an unregistered unit has no block")
        eq("NOT applied" in render_plan(plan, "A/a"), True, "the rendering says it does not apply the edit")


def selftest_touch(eq) -> None:
    """The TOUCHED rule and the add-only half's deferral classes, end to end over fixture trees."""
    import tempfile

    same = {"body": "h", "ext": {"k": ["foo", ".text:1"]}}
    eq(fingerprints_equal(same, {"body": "h", "ext": {"k": ["bar", ".text:1"]}}), True,
       "an external relocation renamed to the same address is the same object")
    eq(fingerprints_equal(same, {"body": "h", "ext": {"k": ["foo", ".text:2"]}}), False,
       "... one that resolves to another address is not")
    eq(fingerprints_equal({"body": "h", "ext": {"k": ["foo__Fv", None]}},
                          {"body": "h", "ext": {"k": ["foo__Fv", ".text:1"]}}),
       True, "an unresolved name the map later resolves, spelled the same, is the same object")
    eq(fingerprints_equal({"body": "h", "ext": {"k": ["foo__Fv", None]}},
                          {"body": "h", "ext": {"k": ["baz", ".text:1"]}}),
       False, "an unresolved name replaced by another spelling is judged changed")
    eq(fingerprints_equal(same, dict(same, body="g")), False, "a different body is a different object")

    base = {"unit_claims": {"A/a": {".data": [[1, 5]]}}, "objects": {"A/a": same}}
    now_claims = {"A/a": {".data": [[1, 5]]}, "C/c": {".data": [[8, 9]]}}
    got = touch_verdicts(["A/a", "C/c"], base, now_claims, {"A/a": same})
    eq((got["A/a"]["touched"], got["A/a"]["reasons"]), (False, []), "same claims and object: not touched")
    eq(got["C/c"]["reasons"], ["registered"], "a unit with claims now and none at the base is registered")
    eq(touch_verdicts(["A/a"], base, {"A/a": {".data": [[1, 9]]}}, {})["A/a"]["reasons"], ["claims changed (.data)"],
       "a recut is named with its section; no object falls back to the claims")
    eq(touch_verdicts(["A/a"], {}, now_claims, {})["A/a"]["touched"], True, "an old base with no record is touched")
    eq(touch_verdicts(["A/a"], base, now_claims, {"A/a": dict(same, body="g")})["A/a"]["reasons"], ["object changed"],
       "a changed body is an object change")

    splits = ("Sections:\n\t.text type:code align:32\n\nA/a.cpp:\n\t.text start:0x80010000 end:0x80010100\n"
              "\t.data start:0x805E0000 end:0x805E0010\n")
    symbols = ("a1 = .data:0x805E0000; // type:object size:0x4\np1 = .data:0x805E0010; // type:object size:0x4\n"
               "ro = .rodata:0x80500040; // type:object size:0x4\ns1 = .sdata:0x80792300; // type:object size:0x4\n")
    with tempfile.TemporaryDirectory() as tmp:
        def put(rel, text):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def objs(refs, fill=0, target=None):
            for kind, names in (("src", refs), ("obj", target if target is not None else refs)):
                path = os.path.join(tmp, "build", "RMHE08", kind, "A", "a.o")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                build_fixture_object(path, names, fill=fill)

        def row(snap, **kw):
            return batch_orphans(tmp, ["A/a"], snap, query=None, **kw)

        put("config/RMHE08/splits.txt", splits)
        put("config/RMHE08/symbols.txt", symbols)
        objs(["p1"])
        snap = snapshot_orphans(tmp, ["A/a"])
        eq(sorted(snap["objects"]), ["A/a"], "the snapshot fingerprints the named units' compiled objects")
        eq(sorted(snap["unit_claims"]["A/a"]), [".data", ".text"], "... and records each unit's claims")
        v = row(snap)
        eq((v["touch"]["A/a"]["touched"], v["sole_owned"], v["untouched_pairs"]), (False, [], {"A/a": 1}),
           "an unchanged unit is not touched: its sole-owned pair is reported, not demanded")
        eq("rename-only" in render_touch("A/a", v["touch"]["A/a"]), True, "... and the row says why")

        # rename-only: the map row and the object's relocation name change, the address does not
        put("config/RMHE08/symbols.txt", symbols.replace("p1 =", "p1_renamed ="))
        objs(["p1_renamed"])
        v = row(snap)
        eq((v["touch"]["A/a"]["touched"], v["sole_owned"]), (False, []), "a pure rename sweep does not touch the unit")

        # a body edit that changes bytes touches it, and the pair is demanded
        put("config/RMHE08/symbols.txt", symbols)
        objs(["p1"], fill=1)
        v = row(snap)
        eq((v["touch"]["A/a"]["reasons"], len(v["sole_owned"])), (["object changed"], 1),
           "an edit that changes the compiled bytes touches the unit")
        objs(["p1"])
        eq(row(snap)["touch"]["A/a"]["touched"], False, "a source edit that compiles to the same object does not")

        # a claim change touches it
        put("config/RMHE08/splits.txt", splits.replace("0x805E0010", "0x805E0008"))
        v = row(snap)
        eq((v["touch"]["A/a"]["reasons"], len(v["sole_owned"])), (["claims changed (.data)"], 1),
           "a splits.txt claim change touches the unit")
        put("config/RMHE08/splits.txt", splits)

        # registration: a unit that has claims now and none in the base
        put("config/RMHE08/splits.txt", splits + "\nB/b.cpp:\n\t.text start:0x80010100 end:0x80010200\n")
        bobj = os.path.join(tmp, "build", "RMHE08", "obj", "B", "b.o")
        os.makedirs(os.path.dirname(bobj), exist_ok=True)
        build_fixture_object(bobj, ["p1"])
        v = batch_orphans(tmp, ["B/b"], snap, query=None)
        eq(v["touch"]["B/b"]["reasons"], ["registered"], "a newly registered unit is touched (registered)")
        put("config/RMHE08/splits.txt", splits)
        os.remove(bobj)

        # DEFECT 1: a widened claim re-splits the target object; its new pairs are classified, not refused
        put("config/RMHE08/splits.txt", splits.replace("0x805E0010", "0x805E0014"))
        objs(["p1"], target=["p1", "s1", "ro"])
        v = row(snap)
        eq(len(v["added_deferred"]), 1, "a NEW .sdata pair whose class is deferred (isolated-run) is reported deferred")
        eq("isolated-run" in v["added_deferred"][0], True, "... with its class and reason")
        eq(len(v["added"]), 1, "a NEW unclaimed .rodata word (refusable) still refuses")
        eq("ro" in v["added"][0], True, "... and it is that word")
        eq(row(snap, strict=False)["added_deferred"], [], "strict=False has no classification: the old add-only row")


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
    selftest_strict(eq)
    selftest_touch(eq)

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
        verdict = batch_orphans(args.root, units, snap, args.allow_orphan)
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
        for line in verdict["added"]:
            print("REFUSED  " + line)
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
        print("pre-existing (reported, not refused): %d" % len(verdict["pre_existing"]))
        failed = bool(verdict["added"] or verdict["sole_owned"])
        print("row: %s" % ("FAIL, %d added + %d sole-owned pair(s) unclaimed" % (len(verdict["added"]),
                                                                                  len(verdict["sole_owned"]))
                           if failed else "PASS"))
        return 1 if failed else 0
    if args.pool_seams:
        census = poolseams.load_census(args.root, True)
        if args.json:
            with open(args.json, "w", encoding="utf-8") as fh:
                json.dump(census, fh, indent=1, default=list)
            print(f"wrote {args.json}")
        if args.unit:
            for unit in args.unit:
                group = poolseams.group_of(census, unit)
                print("\n".join(poolseams.render_group(group)) if group else f"{unit}: no pool-sharing group")
            return 0
        print(poolseams.render_census(census, min(args.top, 25)))
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
