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


CODE_SOURCES = (".text", "extab")


def object_reloc_sources(path: str) -> dict[str, set[str]]:
    """`{name: {section of each relocation that names it}}` for an object's non-bookkeeping relocations.

    `{}` when the object cannot be read. The section is the relocation's `target` (`.text`, `.data`, ...).
    """
    from units import undefrefs as uref  # noqa: PLC0415 - the one relocation reader

    loaded = uref.load_object(path) if os.path.exists(path) else None
    out: dict[str, set[str]] = {}
    for rel in (loaded or {}).get("relocs", ()):
        if rel["symbol"]:
            out.setdefault(rel["symbol"], set()).add(rel["target"] or "")
    return out


def exposing_sections(sources) -> list[str]:
    """The claimed sections that are the ONLY thing referencing a symbol; `[]` when anything else does.

    THE claim-exposed test (owner, 2026-09-30): a name is exposed by a claim when every relocation that
    names it sits in a data section of the unit's own object - a `.text`/`extab` relocation (the unit's code
    reads it) or an unnamed section means the unit needs the word whatever it claims. Pure; `explain_added`
    (the `--base-root` cause) and `claim_exposed_pairs` (the gate's deferral) both read it.
    """
    srcs = {t for t in sources if t}
    if not srcs or any(t.startswith(CODE_SOURCES) for t in sources):
        return []
    return sorted(srcs)


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
            return "%s claims 0x%08X-0x%08X" % (u, s, e)
    rows = ctx["rows"].get(section, [])
    query = ctx.get("query")
    for address, name, _size in rows[bisect.bisect_left(rows, (lo,)):]:
        if address >= hi:
            break
        units = ctx["group_units"].get((section, address))
        if units:
            if units != {unit}:
                return "%s reads %s (0x%08X)" % (",".join(sorted(units - {unit})[:3]), name, address)
            continue
        if query is not None:
            readers, unsplit = query(address)
            if unsplit or not set(readers) <= {unit}:
                foreign = sorted(set(readers) - {unit})
                return "%s reads %s (0x%08X)" % (",".join(foreign[:3]) if foreign else "unsplit code", name, address)
    return None


def judge_blocks(unit: str, section: str, runs: list[dict], ctx: dict) -> list[dict]:
    """The blocks one unit's runs in one section merge into, each with its verdict. Pure given `ctx`.

    Runs (and the unit's existing claims, as anchors) merge into one block while the gap between them holds no
    `_gap_blocker`. The **main** block is the one holding an existing claim - kept even when it carries no pairs,
    because a unit's own claim is what every other range must be reachable from (the `enemy/fn_80147CE0` and
    `quest/quest_entry` bug: dropping it made a far run the "main" one and the plan spanned foreign data) - or,
    with no claim, the block with the most pairs. The main block's verdict is `refuse` unless a class below defers
    it. Every other block is `span-blocked`, and its reason names the foreign reader or claim between it and the
    main block ("blocked by <unit> reads <symbol> (<addr>)"). Only blocks with pairs are returned. Per block, in
    order: `pool-synth`, `ambiguous-owner`, then for the main block `isolated-run`. `ctx` carries `ranges`, `rows`,
    `group_units`, `text_start`, `our_sizes` and `query` (see `strict_report`).
    """
    import bisect

    ranges, rows = ctx["ranges"], ctx["rows"].get(section, [])
    mine = unit_claims(ranges, unit, section)
    claimed = sum(e - s for s, e in mine)
    items = ([{"start": s, "end": e, "pairs": [], "anchor": True} for s, e in mine]
             + [dict(r, anchor=False) for r in runs])
    for item in items:
        item["anchors"] = [(item["start"], item["end"])] if item["anchor"] else []
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
                blocks[-1]["anchors"] += item["anchors"]
                continue
            item["gap_before"] = gap
        blocks.append({"unit": unit, "section": section, "start": item["start"], "end": item["end"],
                       "pairs": list(item["pairs"]), "anchor": item["anchor"], "anchors": list(item["anchors"]),
                       "gap_before": item.get("gap_before")})
    anchored = [b for b in blocks if b["anchor"]]
    by_weight = lambda b: (len(b["pairs"]), b["end"] - b["start"], -b["start"])          # noqa: E731
    main = max(anchored, key=by_weight) if anchored else (max(blocks, key=by_weight) if blocks else None)
    blocks = [b for b in blocks if b["pairs"]]                        # an anchor with no pair is judged, not reported
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
            lo, hi = (main["end"], start) if start >= main["end"] else (end, main["start"])
            why = _gap_blocker(lo, hi, unit, section, ctx) or b.get("gap_before") or "other data"
            b["verdict"], b["cls"] = "deferred", "span-blocked"
            b["blocker"] = why
            b["reason"] = ("a second %s range for the unit: blocked by %s between it and the unit's main range "
                           "(one spanning claim is impossible, two is the playbook 53 cycle)" % (section, why))
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


def splits_plan(text: str, unit: str, blocks: list[dict], our_sizes: dict | None = None, blocker=None) -> dict:
    """The exact `splits.txt` edit that claims `unit`'s refusable blocks, in link-order position. Pure.

    The unit's block in `text` is already where the link order wants it (units are listed in address order), so
    the edit is inside it: each refusable block becomes one line in the section order of the file's `Sections:`
    table, or - when the unit already claims that section - the REPLACEMENT of its line(s) by one spanning
    range (two ranges with a gap between them is the playbook 53 cycle). Returns `{header, edits, result,
    deferred, found}`: `edits` are `{action, section, old, new, start, end, note}`, `result` is the unit's block
    as it would read, `deferred` the blocks the row would not refuse, with their class. Never applied.

    A replacement folds in only the unit's lines that lie INSIDE the block (`judge_blocks` merged them across a
    clean gap); a same-section line elsewhere is left alone, and a block that would need it is `span-blocked`.
    `blocker(section, start, end) -> str | None` is the last line of defence: a span that still covers an address
    another unit reads or claims is turned into a `span-blocked` entry with the reader named and NO edit line.
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
        start, end = block["start"], block["end"]
        same = [r for r in parsed if r["section"] == section and start <= r["start"] and r["end"] <= end]
        elsewhere = [r for r in parsed if r["section"] == section and r not in same
                     and (not same or (r["start"] < end and start < r["end"]))]
        why = None
        if elsewhere:
            why = ("the unit already claims %s 0x%08X-0x%08X outside this block: a second range is the playbook 53 "
                   "cycle, one spanning range would cover what lies between" % (section, elsewhere[0]["start"],
                                                                                 elsewhere[0]["end"]))
        elif blocker is not None:
            why = blocker(section, start, end)
            why = ("blocked by %s" % why) if why else None
        if why:
            deferred.append(dict(block, verdict="deferred", cls="span-blocked", blocker=why,
                                 reason="the claim span 0x%08X-0x%08X cannot be claimed: %s" % (start, end, why)))
            continue
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
        lines.append("  %s %s %s 0x%08X-0x%08X (%d pair(s)): %s"
                     % ("blocked " if b["cls"] == "span-blocked" else "deferred", b["cls"], b["section"], b["start"],
                        b["end"], len(b["pairs"]), b["reason"]))
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
    ctx = report["ctx"]
    return {"plan": splits_plan(text, unit, blocks, ctx["our_sizes"].get(unit), span_blocker(ctx, unit)),
            "report": report, "text": text}


def span_blocker(ctx: dict, unit: str):
    """`blocker(section, start, end)` for `splits_plan`: the first foreign claim or reader in `[start, end)`.

    The unit's own claims inside the span are skipped (they are the unit's); everything else goes through
    `_gap_blocker` (another unit's claim, a row another registered unit references, a row `callers.py` shows a
    foreign or unsplit reader for).
    """
    def blocker(section: str, start: int, end: int):
        cur = start
        for s, e in sorted(unit_claims(ctx["ranges"], unit, section)):
            if e <= cur or s >= end:
                continue
            if s > cur:
                why = _gap_blocker(cur, s, unit, section, ctx)
                if why:
                    return why
            cur = max(cur, e)
        return _gap_blocker(cur, end, unit, section, ctx) if cur < end else None

    return blocker


# -- the fixpoint: claim, re-judge, repeat until the plan is stable (owner ask, 2026-09-30) -----------------------
#
# One plan is judged against the claims as they stand. Applying it moves a claim, which can change the next verdict
# (a span now touches another unit's claim, a neighbour's text-window order changes, a pool the claim completes).
# A lane that applies the first plan and asks again must not be handed a plan that exists only because of the
# first one. `fixpoint_plan` applies each plan to an in-memory copy of `splits.txt` and re-judges until no edit is
# left, or names why it cannot: the blockers of the final state, or the loop.

def _unit_head(lines: list[str], unit: str) -> int | None:
    import re

    for i, line in enumerate(lines):
        m = re.match(r"^([^\s:][^:]*):\s*$", line)
        if m and os.path.splitext(m.group(1))[0] == unit:
            return i
    return None


def _unit_lines(lines: list[str], unit: str) -> list[str]:
    head = _unit_head(lines, unit)
    if head is None:
        return []
    out, j = [], head + 1
    while j < len(lines) and lines[j].strip():
        out.append(lines[j])
        j += 1
    return out


def apply_plan_text(text: str, unit: str, plan: dict) -> str:
    """`text` with the unit's block replaced by the plan's `result` (the `+ `/`  ` markers stripped). Pure."""
    lines = text.splitlines()
    head = _unit_head(lines, unit)
    if head is None or not plan.get("found"):
        return text
    end = head + 1 + len(_unit_lines(lines, unit))
    body = [r[2:] for r in plan["result"]]
    return "\n".join(lines[:head + 1] + body + lines[end:]) + ("\n" if text.endswith("\n") else "")


def fixpoint_plan(root: str, unit: str, max_steps: int = 8, text: str | None = None,
                  unit_refs: dict | None = None, symbols: dict | None = None, query="lazy") -> dict:
    """`{converged, stop, steps, plans, final, blockers, net, text}` for one unit; writes nothing.

    `steps` counts the plans that carried an edit; `plans` is every plan judged (the last has no edit when it
    converged); `blockers` are the final state's blocks that are not claimable, with their reasons; `net` is the
    unit's `splits.txt` block before and after; `converged` is False when the loop hit `max_steps` or returned to
    a state it had already judged (`stop` says which). The inputs default to `root`'s files; a test passes them.
    """
    unit = os.path.splitext(unit.replace("\\", "/"))[0]
    if text is None:
        with open(os.path.join(root, "config", GAME_DIR, "splits.txt"), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    if unit_refs is None:
        unit_refs, _n = census_inputs(root, None, parse_splits_text(text))
    symbols = symbols if symbols is not None else load_data_symbols(root)
    if query == "lazy":
        try:
            query = readers_index(root)
        except Exception:                                                    # noqa: BLE001 - no index: reloc-only
            query = None
    start_text, seen, plans, stop = text, {text}, [], None
    for _ in range(max_steps + 1):
        ranges = parse_splits_text(text)
        records, _stats = census_records(unit_refs, symbols, ranges)
        report = strict_report(root, records, ranges, [unit], query=query, symbols=symbols)
        ctx = report["ctx"]
        blocks = [b for b in report["blocks"] if b["unit"] == unit]
        plan = splits_plan(text, unit, blocks, (ctx["our_sizes"].get(unit)), span_blocker(ctx, unit))
        plans.append({"plan": plan, "blocks": blocks})
        if not plan["edits"]:
            break
        text = apply_plan_text(text, unit, plan)
        if text in seen:
            stop = "the plan returned to a state already judged: no fixpoint"
            break
        seen.add(text)
    else:
        stop = "no fixpoint after %d step(s): every plan still carries an edit" % max_steps
    last = plans[-1]["plan"]
    return {"unit": unit, "converged": stop is None, "stop": stop,
            "steps": sum(1 for p in plans if p["plan"]["edits"]), "plans": plans, "final": last,
            "blockers": list(last["deferred"]), "text": text,
            "net": {"before": _unit_lines(start_text.splitlines(), unit), "after": _unit_lines(text.splitlines(), unit)},
            "readers_checked": query is not None}


def render_fixpoint(fp: dict) -> str:
    """The converged plan, or the exact blocker, as text a lane can act on."""
    unit = fp["unit"]
    lines = [("fixpoint for `%s`: converged after %d claim step(s)" % (unit, fp["steps"])) if fp["converged"]
             else "fixpoint for `%s`: NOT converged - %s" % (unit, fp["stop"])]
    if not fp["readers_checked"]:
        lines.append("  WARNING: callers index unavailable - a foreign reader of an unreferenced gap row is invisible")
    if not fp["final"]["found"]:
        lines.append("  no `%s` block in splits.txt: register the unit first" % unit)
        return "\n".join(lines)
    for k, p in enumerate(fp["plans"], 1):
        if not p["plan"]["edits"]:
            continue
        lines.append("  step %d%s:" % (k, "" if k == 1 else " (exists only because step %d moved the claims)" % (k - 1)))
        for e in p["plan"]["edits"]:
            lines.append("    %s %s 0x%08X-0x%08X (%d pair(s))" % (e["action"].upper(), e["section"], e["start"],
                                                                 e["end"], e["pairs"]))
    before, after = fp["net"]["before"], fp["net"]["after"]
    if before != after:
        lines.append("  net change of the unit's splits.txt block (apply exactly this, once):")
        lines += ["    - " + ln.strip() for ln in before if ln not in after]
        lines += ["    + " + ln.strip() for ln in after if ln not in before]
    else:
        lines.append("  nothing to claim: the unit's block is already at its fixpoint")
    for b in fp["blockers"]:
        lines.append("  %s %s %s 0x%08X-0x%08X (%d pair(s)): %s"
                     % ("blocked " if b["cls"] == "span-blocked" else "deferred", b["cls"], b["section"], b["start"],
                        b["end"], len(b["pairs"]), b["reason"]))
    return "\n".join(lines)


# -- is the census itself current? (the `--base-root` discrepancy, 2026-09-30) ------------------------------------
#
# The census reads `build/RMHE08/obj/<unit>.o`, the TARGET objects `dtk dol split` writes from `splits.txt` +
# `symbols.txt`. An object cut from other claims references other symbols (a range a unit claimed since is no
# longer an external reference), so a base census built from it and a branch census built from current objects
# disagree about which pairs are orphans - and the row reports the difference as pairs the batch "added". That is a
# stale base, not a claims effect. `tree_freshness` decides it by CONTENT: a current object's `.text` and data
# sections are exactly as large as the unit's claims in `splits.txt`. (File times are reported too but decide
# nothing: dtk leaves an unchanged object alone, so a current object can be much older than `splits.txt`.)

FRESH_SECTIONS = (".text",) + CENSUS_SECTIONS


def tree_freshness(root: str, tolerance: float = 2.0, units: "list[str] | set[str] | None" = None) -> dict:
    """`{total, stale, older, newest, worst_age, examples}` for `root`'s target objects against its `splits.txt`.

    `stale` counts the registered units whose object has a `.text`/data section of a different size than the
    unit's claims total; `examples` are the first five `(unit, section, claimed, object)`. `older` counts the
    objects whose file time predates `splits.txt`/`symbols.txt` by more than `tolerance` seconds (information
    only). A tree with no objects or no map files has `total` 0. `units` (extensionless unit names) scopes the
    scan to the batch: a stale object of a unit the batch does not touch is not this row's business.
    """
    want = {os.path.splitext(u)[0] for u in units} if units is not None else None
    cfg = os.path.join(root, "config", GAME_DIR)
    stamps = {}
    for fn in ("splits.txt", "symbols.txt"):
        try:
            stamps[fn] = os.stat(os.path.join(cfg, fn)).st_mtime
        except OSError:
            continue
    out = {"total": 0, "stale": 0, "older": 0, "newest": None, "worst_age": 0.0, "examples": [], "units": {}}
    if not stamps or not os.path.exists(os.path.join(cfg, "splits.txt")):
        return out
    newest = max(stamps, key=stamps.get)
    out["newest"] = newest
    ranges = load_claims(root)
    claimed: dict[str, dict[str, int]] = {}
    for section, rows in ranges.items():
        for start, end, unit in rows:
            claimed.setdefault(unit, {})[section] = claimed.get(unit, {}).get(section, 0) + (end - start)
    for unit in sorted(u for u in claimed if want is None or u in want):
        path = os.path.join(root, "build", GAME_DIR, "obj", unit + ".o")
        try:
            mtime = os.stat(path).st_mtime
            sizes = section_sizes(path)
        except Exception:                                                    # noqa: BLE001 - no/unreadable object
            continue
        out["total"] += 1
        age = stamps[newest] - mtime
        if age > tolerance:
            out["older"] += 1
            out["worst_age"] = max(out["worst_age"], age)
        for sec in FRESH_SECTIONS:
            if sizes.get(sec, 0) != claimed[unit].get(sec, 0):
                out["stale"] += 1
                out["units"][unit] = (sec, claimed[unit].get(sec, 0), sizes.get(sec, 0))
                if len(out["examples"]) < 5:
                    out["examples"].append((unit, sec, claimed[unit].get(sec, 0), sizes.get(sec, 0)))
                break
    return out


def render_freshness(label: str, fr: dict) -> list[str]:
    """The WARNING line for a stale census (none when every object matches its unit's claims)."""
    if not fr["stale"]:
        return []
    return ["WARNING: %s census built from objects older than splits.txt/symbols.txt: %d of %d target object(s) do not "
            "match their unit's claims (e.g. %s; %d object file(s) predate %s, oldest by %.1f h) - they were cut "
            "from different claims, so pairs may differ for that reason alone; re-split (`rm "
            "build/RMHE08/config.json` + ninja) before trusting it"
            % (label, fr["stale"], fr["total"],
               ", ".join("%s %s claimed 0x%X, object 0x%X" % e for e in fr["examples"][:2]), fr["older"],
               fr["newest"], fr["worst_age"] / 3600.0)]


def explain_added(root: str, base_root: str, units: list[str], base_keys, base_ranges: dict) -> list[str]:
    """One line per (unit, cause) for the pairs the tree has and the base does not: WHY the two censuses differ.

    Causes, in the order they are tested: the base tree has no target object for the unit; its object does not
    reference the symbol (the objects differ - `stale` when the base object predates the base's map files);
    the base's claims cover the address (a claims effect); else the snapshot and the base tree disagree.
    """
    names = [os.path.splitext(u)[0] for u in units]
    (records, _stats), _n, _have = census(root, names)
    base_keys = set(base_keys)
    new = [r for k, r in sorted(orphan_keys(records, names).items()) if k not in base_keys]
    if not new:
        return []
    base_stale = tree_freshness(base_root, units=names)["units"]
    cache: dict[str, tuple] = {}
    causes: dict[tuple, list[str]] = {}
    for r in new:
        unit = r["unit"]
        if unit not in cache:
            bo = os.path.join(base_root, "build", GAME_DIR, "obj", unit + ".o")
            ro = os.path.join(root, "build", GAME_DIR, "obj", unit + ".o")
            sources = object_reloc_sources(ro)
            cache[unit] = (object_data_refs(bo) if os.path.exists(bo) else None, _sha(bo), _sha(ro), sources)
        refs, bsha, rsha, sources = cache[unit]
        if refs is None:
            why = "the base tree has no target object for this unit (a unit the batch registers)"
        elif r["name"] not in refs:
            via = exposing_sections(sources.get(r["name"], ()))
            if via:
                why = ("EXPOSED BY THE CLAIM: the base object does not reference it; in the tree only the unit's "
                       "claimed %s reference it (claimed data carries its own relocations - the next plan only exists "
                       "because the claim was made: run `dataclaim.py --unit %s --fixpoint`)" % (", ".join(via), unit))
            else:
                why = ("the base tree's target object does not reference it (objects differ: base %s, tree %s)"
                       % (bsha, rsha))
            if unit in base_stale:
                why += ("; STALE base census: its %s is 0x%X but the base's claims total 0x%X"
                        % (base_stale[unit][0], base_stale[unit][2], base_stale[unit][1]))
        else:
            status = classify_address(r["section"], r["address"], base_ranges, unit)
            if status["status"] != "orphan":
                why = "the base's claims cover it (%s %s): a claims effect" % (status["status"], status["owner"])
            else:
                why = "referenced and unclaimed at the base too: the snapshot's keys come from another census"
        causes.setdefault((unit, why), []).append(r["name"])
    return ["cause: %s: %d pair(s) (%s%s): %s" % (unit, len(ns), ", ".join(ns[:3]), " ..." if len(ns) > 3 else "", why)
            for (unit, why), ns in sorted(causes.items())]


def _sha(path: str) -> str | None:
    import hashlib

    try:
        with open(path, "rb") as fh:
            return hashlib.sha1(fh.read()).hexdigest()[:8]
    except OSError:
        return None


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


def census_inputs(root: str, units: list[str] | None = None, ranges: dict | None = None) -> tuple[dict, int]:
    """`({unit: {name: sites}}, registered)`: what each registered unit's TARGET object in `root` references."""
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
    return unit_refs, len(registered)


def census(root: str, units: list[str] | None = None, ranges: dict | None = None):
    """`(records, stats), registered, read` for the registered units' TARGET objects as they stand in `root`."""
    ranges = ranges if ranges is not None else load_claims(root)
    unit_refs, registered = census_inputs(root, units, ranges)
    return census_records(unit_refs, load_data_symbols(root), ranges), registered, len(unit_refs)


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


# -- folds: the base snapshot is keyed by UNIT NAME, a recut changes the names (2026-09-30) -----------------------
#
# A batch that folds units (deletes `A`, `B` into `C`), shrinks one (function moved to a neighbour) or renames one
# leaves the snapshot's `orphan:<unit>:...` keys, `unit_claims` and `objects` naming units that no longer exist or no
# longer hold those bytes, so the pre-existing pairs of the folded unit read as pairs the absorber ADDED. The map
# below is the unit-name bridge: `{NEW: [OLD, ...]}` - NEW inherits OLD's base pairs and (intersected with what NEW
# holds now) claims. It is derived from evidence, never guessed: a base byte range owned by OLD that a DIFFERENT unit
# claims now (`splits.txt` at the base vs now), plus git's own rename detection over `src/**`. An explicit
# `OLD=NEW` (`--unit-rename`) overrides the derivation for that OLD; `OLD=` (no NEW) says its pairs are simply gone.

def _intersect(a: list, b: list) -> list:
    """Intersection of two sorted-or-not `[[start, end], ...]` range lists, merged."""
    out = []
    for s1, e1 in a:
        for s2, e2 in b:
            lo, hi = max(s1, s2), min(e1, e2)
            if lo < hi:
                out.append((lo, hi))
    return [list(r) for r in merged(out)]


def derive_absorption(base_table: dict, now_table: dict, git_renames: dict | None = None) -> dict[str, list[str]]:
    """`{NEW: sorted [OLD, ...]}` - which unit now holds bytes another unit held at the base. Pure.

    `base_table`/`now_table` are `unit_claim_table`s. OLD donates to NEW when a range OLD owned in a section is (in
    part) owned by a different NEW now: a deleted or renamed unit's whole claim, a shrunk unit's moved part. A
    renamed unit's bytes move wholesale, so the evidence covers the 1:1 rename too. `git_renames` (`{old unit: new
    unit}` from git's rename detection) adds the pairs a pure `git mv` shows.
    """
    out: dict[str, set] = {}
    for section in sorted({s for t in (base_table, now_table) for secs in t.values() for s in secs}):
        base = sorted((lo, hi, unit) for unit, secs in base_table.items() for lo, hi in secs.get(section, ()))
        now = sorted((lo, hi, unit) for unit, secs in now_table.items() for lo, hi in secs.get(section, ()))
        j = 0
        for lo, hi, old in base:
            while j < len(now) and now[j][1] <= lo:
                j += 1
            k = j
            while k < len(now) and now[k][0] < hi:
                if now[k][2] != old:
                    out.setdefault(now[k][2], set()).add(old)
                k += 1
    for old, new in (git_renames or {}).items():
        if old != new and old in base_table:
            out.setdefault(new, set()).add(old)
    return {new: sorted(olds) for new, olds in sorted(out.items())}


def explicit_absorption(absorbs: dict[str, list[str]], explicit: dict[str, str]) -> dict[str, list[str]]:
    """`absorbs` with the explicit `OLD=NEW` pairs applied: each OLD is taken out of every derived absorber and (when
    it names a NEW) put under that one; `OLD=` (empty NEW) leaves it with no absorber at all. Pure."""
    out = {new: [o for o in olds if o not in explicit] for new, olds in absorbs.items()}
    for old, new in explicit.items():
        if new:
            out.setdefault(new, []).append(old)
    return {new: sorted(set(olds)) for new, olds in sorted(out.items()) if olds}


def fold_snapshot(snap: dict, absorbs: dict[str, list[str]], now_table: dict, explicit: dict[str, str] | None = None) -> dict:
    """The base snapshot re-keyed onto today's unit names. Pure; `snap` is left untouched.

    * `keys` - a base pair `(OLD, section, address)` also holds under every NEW that absorbed OLD (several OLDs folding
      into one NEW are a union). An OLD that is no longer registered, or was renamed/dropped explicitly, loses its own
      key: its bytes now belong to its absorbers by claim, and the claimed-bytes check still applies to them.
    * `unit_claims` - NEW's base claims gain the part of each OLD's claims NEW holds now (a fold is not a claim
      change); a vanished OLD's entry is removed.
    * `objects` - a vanished OLD's fingerprint is dropped; NEW keeps only its own (a fold changes its object, and a
      compiled object cannot be unioned: the row then says `object changed`, which is true).
    """
    explicit = explicit or {}
    vanished = {o for o in set(snap.get("unit_claims") or {}) | {k[7:].rsplit(":", 2)[0] for k in snap.get("keys") or []
                                                               if k.startswith("orphan:")}
                if o not in now_table or o in explicit}
    targets: dict[str, set] = {}
    for new, olds in absorbs.items():
        for old in olds:
            targets.setdefault(old, set()).add(new)
    out = dict(snap)
    keys = set()
    for key in snap.get("keys") or []:
        if not key.startswith("orphan:"):
            keys.add(key)
            continue
        unit, section, addr = key[7:].rsplit(":", 2)
        if unit not in vanished:
            keys.add(key)
        for new in targets.get(unit, ()):
            keys.add("orphan:%s:%s:%s" % (new, section, addr))
    out["keys"] = sorted(keys)
    if snap.get("unit_claims") is not None:
        claims = {u: {s: [list(r) for r in rs] for s, rs in secs.items()}
                  for u, secs in snap["unit_claims"].items() if u not in vanished}
        for new, olds in absorbs.items():
            for old in olds:
                for section, rows in (snap["unit_claims"].get(old) or {}).items():
                    moved = _intersect(rows, (now_table.get(new) or {}).get(section, []))
                    if moved:
                        have = claims.setdefault(new, {}).setdefault(section, [])
                        claims[new][section] = [list(r) for r in merged([tuple(r) for r in have + moved])]
        out["unit_claims"] = claims
    if snap.get("objects") is not None:
        out["objects"] = {u: fp for u, fp in snap["objects"].items() if u not in vanished}
    return out


def git_unit_renames(root: str, base_ref: str) -> dict[str, str]:
    """`{old unit: new unit}` for the `src/**` source files git detects as renamed between `base_ref` and the tree. IO.

    Unit names are the path under `src/` without its extension - what `splits.txt` and the snapshot key on.
    """
    import subprocess

    p = subprocess.run(["git", "diff", "--name-status", "-M", base_ref, "--", "src"], cwd=root, capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    out: dict[str, str] = {}
    for line in p.stdout.splitlines():
        parts = line.split("\t")
        if len(parts) == 3 and parts[0].startswith("R") and parts[1].startswith("src/") and parts[2].startswith("src/"):
            out[os.path.splitext(parts[1][4:])[0]] = os.path.splitext(parts[2][4:])[0]
    return out


def render_unit_map(absorbs: dict[str, list[str]], explicit: dict[str, str] | None = None) -> list[str]:
    """The fold map as gate-log lines: what was derived and what an explicit `--unit-rename` overrode."""
    lines = ["data closure: unit map (base pairs follow the bytes): %s" % "; ".join(
        "%s <- %s" % (new, ", ".join(olds)) for new, olds in absorbs.items())] if absorbs else []
    if explicit:
        lines.append("data closure: explicit --unit-rename overrides: %s" % ", ".join(
            "%s=%s" % (o, n) for o, n in sorted(explicit.items())))
    return lines


def claim_exposed_pairs(root: str, new: dict[str, dict], base_unit_claims: dict, now_unit_claims: dict) -> dict[str, dict]:
    """`{orphan_key: {reason, ranges, via, pair}}` for the NEW pairs the batch's own claim exposed. Pure but for IO.

    CLAIM-EXPOSED (owner, 2026-09-30): a pair (unit U, address A) that the base did not have, where every
    relocation of U's target object that names A sits in a data section of U (`exposing_sections` - the
    classifier `explain_added` names "EXPOSED BY THE CLAIM") AND the batch newly claimed or widened U's claim in
    that section (`base_unit_claims` vs `now_unit_claims`, `unit_claim_table`s). The pair exists only because the
    batch claimed the bytes that reference it; a `.text` reference never qualifies. `new` is `{key: record}`.
    """
    sources: dict[str, dict] = {}
    out: dict[str, dict] = {}
    for key, rec in new.items():
        unit = rec["unit"]
        if unit not in sources:
            sources[unit] = object_reloc_sources(os.path.join(root, "build", GAME_DIR, "obj", unit + ".o"))
        via = exposing_sections(sources[unit].get(rec["name"], ()))
        if not via:
            continue
        ranges, wanted = [], True
        for sec in via:
            before = {tuple(r) for r in (base_unit_claims.get(unit) or {}).get(sec, [])}
            now = [tuple(r) for r in (now_unit_claims.get(unit) or {}).get(sec, [])]
            fresh = [r for r in now if r not in before]
            if not fresh:
                wanted = False
                break
            ranges += ["%s 0x%08X-0x%08X" % (sec, lo, hi) for lo, hi in fresh]
        if not wanted:
            continue
        out[key] = {"reason": "exposed by the batch's own claim of %s; %s's claimed %s reference it"
                              % (", ".join(ranges), unit, ", ".join(via)),
                    "ranges": ranges, "via": via, "pair": rec}
    return out


def tree_claim_exposed(root: str, records: list[dict], symbols: dict | None = None) -> list[dict]:
    """The backlog view of claim-exposed data: `[{unit, section, start, end, pairs}]`, one run per adjacent stretch.

    A tree has no batch, so the test is the tree half of `claim_exposed_pairs`: an ORPHAN pair of a unit whose
    every relocation in its target object sits in one of its own data sections (`exposing_sections`) - the claim
    carries the reference, nothing else. The pair may also be sole-owned (the strict report refuses it); the
    backlog then owes it as THIS item and not as the `sole-owned` one (`backlog.dataclaim_counts` leaves it out).
    """
    symbols = symbols if symbols is not None else load_data_symbols(root)
    sources: dict[str, dict] = {}
    pairs = []
    for _key, rec in sorted(orphan_keys(records).items()):
        unit = rec["unit"]
        if unit not in sources:
            sources[unit] = object_reloc_sources(os.path.join(root, "build", GAME_DIR, "obj", unit + ".o"))
        if exposing_sections(sources[unit].get(rec["name"], ())):
            pairs.append({"unit": unit, "name": rec["name"], "section": rec["section"],
                          "address": rec["address"], "size": rec["size"], "sites": rec["sites"]})
    return _runs(pairs, section_rows(symbols))


def batch_orphans(root: str, units: list[str], base_snapshot: dict | None, allowed=(), strict=True,
                  query="lazy", touched=None, unit_map=None, git_renames=None) -> dict:
    """The gate row's whole decision for the batch units against the recorded base (see `orphan_verdict`).

    `touched` (`{unit: {touched, reasons}}`) overrides `touch_verdicts` - a test's or a caller's own judgement.
    `unit_map` is the explicit `{OLD: NEW or ""}` (`--unit-rename`); `git_renames` git's `{old: new}` detection. The
    snapshot is re-keyed onto today's unit names first (`derive_absorption`/`fold_snapshot`), and the derived map is
    returned as `verdict["unit_map_lines"]` for the gate log.
    """
    ranges = load_claims(root)
    names = [os.path.splitext(u)[0] for u in units]
    (records, _stats), _n, _have = census(root, None, ranges=ranges)
    after = orphan_keys(records, names)
    snap = base_snapshot or {}
    unit_map_lines: list[str] = []
    if snap.get("unit_claims") is not None:
        now_table = unit_claim_table(ranges)
        absorbs = derive_absorption(snap["unit_claims"], now_table, git_renames)
        explicit = {os.path.splitext(o)[0]: os.path.splitext(n)[0] if n else "" for o, n in (unit_map or {}).items()}
        absorbs = explicit_absorption(absorbs, explicit)
        snap = fold_snapshot(snap, absorbs, now_table, explicit)
        unit_map_lines = render_unit_map(absorbs, explicit)
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
    # CLAIM-EXPOSED (owner, 2026-09-30): a new pair only the batch's own newly claimed data references is
    # deferred too - claiming data exposes the pairs its relocations name, without end - reported, never refused,
    # and it earns no allowance. Needs the base's per-unit claims: without them nothing is exposed.
    exposed: dict[str, dict] = {}
    if snap.get("unit_claims") is not None:
        new_pairs = {k: v for k, v in after.items() if k not in base_keys and k not in added_deferred}
        exposed = claim_exposed_pairs(root, new_pairs, snap["unit_claims"], unit_claim_table(ranges))
    refusable = {k: v for k, v in after.items() if k not in added_deferred and k not in exposed}
    verdict = orphan_verdict(snap.get("keys") or [], refusable, shrinks, allowed)
    verdict["added_deferred"] = [pair_line(c["pair"], "deferred %s, a new pair, not refused: %s" % (c["cls"], c["reason"]))
                                 for c in added_deferred.values()]
    verdict["claim_exposed"] = [pair_line(c["pair"], "deferred claim-exposed, a new pair, not refused: %s" % c["reason"])
                                for c in exposed.values()]
    verdict["claim_exposed_pairs"] = [{"unit": c["pair"]["unit"], "name": c["pair"]["name"],
                                       "section": c["pair"]["section"], "address": c["pair"]["address"],
                                       "reason": c["reason"]} for c in exposed.values()]
    verdict["have_base"] = "keys" in snap and "claims" in snap
    verdict["unit_map_lines"] = unit_map_lines
    verdict["base_keys"] = list(snap.get("keys") or [])       # the FOLDED keys: what `explain_added` must compare
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
                                   if k not in base_keys and k not in exposed and r["address"] in sanction}
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


def build_fixture_object(path: str, refs: list[str], defined: list[str] = (), fill: int = 0,
                         data_refs: list[str] = ()) -> None:
    """Write a minimal ELF32-BE object: `.text` with one `R_PPC_ADDR16_HA` relocation per name in `refs`
    (undefined symbols), a `.data` with one relocation per name in `data_refs` (claimed data carrying its own
    references) and each name in `defined` defined there; `fill` is the `.text` byte value (a body edit).
    Enough for `parse_elf`."""
    import struct

    names = list(refs) + [n for n in data_refs if n not in refs] + list(defined)
    undefined = names[:len(names) - len(list(defined))]
    strtab = b"\0"
    offs = {}
    for n in names:
        offs[n] = len(strtab)
        strtab += n.encode() + b"\0"
    syms = b"\0" * 16
    for n in undefined:
        syms += struct.pack(">IIIBBH", offs[n], 0, 0, 0x10, 0, 0)            # global, undefined
    for n in defined:
        syms += struct.pack(">IIIBBH", offs[n], 0, 4, 0x11, 0, 2)            # global object in section 2
    index = {n: i + 1 for i, n in enumerate(names)}
    rela = b"".join(struct.pack(">IIi", 4 * i, (index[n] << 8) | 6, 0) for i, n in enumerate(refs))
    rela_data = b"".join(struct.pack(">IIi", 4 * i, (index[n] << 8) | 6, 0) for i, n in enumerate(data_refs))
    shstr = b"\0.text\0.data\0.rela.text\0.rela.data\0.symtab\0.strtab\0.shstrtab\0"

    def name_off(s):
        return shstr.index(b"\0" + s.encode() + b"\0") + 1

    bodies = [b"", bytes([fill]) * max(4, 4 * len(refs)), b"\0" * max(4, 4 * len(data_refs)), rela, rela_data,
              syms, strtab, shstr]
    heads = [(0, 0, 0, 0), (name_off(".text"), 1, 0, 0), (name_off(".data"), 1, 0, 0),
             (name_off(".rela.text"), 4, 5, 1), (name_off(".rela.data"), 4, 5, 2),
             (name_off(".symtab"), 2, 6, 1), (name_off(".strtab"), 3, 0, 0), (name_off(".shstrtab"), 3, 0, 0)]
    entsize = {3: 12, 4: 12, 5: 16}
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
    hdr = ident + struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 0x34, 0, 0, 40, len(heads), 7)
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
        # A/a claims no .rodata here: a run beyond another unit's claim is a SECOND range for a unit that has one
        # (span-blocked), so the "unclaimed word is refused" shape needs a unit with no claim in the section
        splits = (splits.replace("A/a.cpp:\n\t.rodata start:0x80500000 end:0x80500010\n", "A/a.cpp:\n")
                  .replace("start:0x80500010 end:0x80500020", "start:0x80500000 end:0x80500020"))
        ranges = parse_splits_text(splits)
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


def selftest_exposed(eq) -> None:
    """The claim-exposed deferral (owner, 2026-09-30): the data a batch claims carries its own relocations."""
    import tempfile

    eq(exposing_sections({".data"}), [".data"], "a word only claimed .data references is exposed by the claim")
    eq(exposing_sections({".data", ".text"}), [], "... a .text relocation (the unit's code) is never exposed")
    eq(exposing_sections({".data", "extab"}), [], "... nor is an extab one")
    eq(exposing_sections({""}), [], "... nor a relocation of no known section")
    eq(exposing_sections(()), [], "... nor a name nothing references")

    text_only = ("Sections:\n\t.text type:code align:32\n\nA/a.cpp:\n\t.text start:0x80010000 end:0x80010100\n")
    with_data = text_only + "\t.data start:0x805E0000 end:0x805E0010\n"
    symbols = ("a1 = .data:0x805E0000; // type:object size:0x4\n"
               "w_bss = .bss:0x806A0000; // type:object size:0x4\n"
               "w_bss2 = .bss:0x806A0004; // type:object size:0x4\n")
    with tempfile.TemporaryDirectory() as tmp:
        def put(rel, text):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def objs(refs=(), data=(), fill=0):
            for kind in ("src", "obj"):
                path = os.path.join(tmp, "build", "RMHE08", kind, "A", "a.o")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                build_fixture_object(path, list(refs), fill=fill, data_refs=list(data))

        def row(snap, **kw):
            return batch_orphans(tmp, ["A/a"], snap, query=None, **kw)

        put("config/RMHE08/symbols.txt", symbols)
        put("config/RMHE08/splits.txt", text_only)
        objs()
        snap = snapshot_orphans(tmp, ["A/a"])

        # the batch claims .data; its relocations name an unclaimed .bss word
        put("config/RMHE08/splits.txt", with_data)
        objs(data=["w_bss"], fill=1)
        v = row(snap)
        eq((v["added"], v["sole_owned"]), ([], []), "claimed .data referencing an unclaimed .bss word: not refused")
        eq(len(v["claim_exposed"]), 1, "... it is deferred claim-exposed")
        eq("claim-exposed" in v["claim_exposed"][0] and "exposed by the batch's own claim of .data 0x805E0000-0x805E0010"
           in v["claim_exposed"][0], True, "... with the claimed range in the reason")
        eq([(p["unit"], p["name"]) for p in v["claim_exposed_pairs"]], [("A/a", "w_bss")], "... and as a structured pair")
        eq(v["accepted"], [], "... and it earns no allowance")
        v = row(snap, allowed=["0x806A0000"])
        eq(v["unmatched_allowances"], ["0x806A0000"], "... an --allow-orphan for it is unmatched (nothing to excuse)")

        # the same word read by the unit's own code: refused
        objs(refs=["w_bss"], data=[], fill=1)
        v = row(snap)
        eq((len(v["added"]), v["claim_exposed"]), (1, []), "the same word referenced by .text is still refused")
        objs(refs=["w_bss"], data=["w_bss"], fill=1)
        v = row(snap)
        eq((len(v["added"]), v["claim_exposed"]), (1, []), "... and so is one both .text and the claimed .data reference")

        # one exposed word and one code-read word: only the second refuses
        objs(refs=["w_bss2"], data=["w_bss"], fill=1)
        v = row(snap)
        eq((len(v["added"]), len(v["claim_exposed"])), (1, 1), "a mixed batch defers the exposed word, refuses the other")
        eq("w_bss2" in v["added"][0], True, "... and the refused one is the code-read word")

        # the claim did not change: the reference was already there, so a new pair is not exposed by the batch
        put("config/RMHE08/splits.txt", with_data)
        objs(fill=0)
        snap_d = snapshot_orphans(tmp, ["A/a"])
        objs(data=["w_bss"], fill=1)
        v = row(snap_d)
        eq((len(v["added"]), v["claim_exposed"]), (1, []), "an unchanged claim exposes nothing: a new pair refuses")

        # a pair that existed at the base keeps its behaviour (pre-existing, reported)
        objs(data=["w_bss"], fill=1)
        snap_p = snapshot_orphans(tmp, ["A/a"])
        put("config/RMHE08/splits.txt", text_only)
        snap_p["unit_claims"] = unit_claim_table(parse_splits_text(text_only))
        put("config/RMHE08/splits.txt", with_data)
        v = row(snap_p)
        eq((v["added"], v["claim_exposed"], len(v["pre_existing"])), ([], [], 1),
           "a pair that existed at the base is pre-existing, not claim-exposed")

        # a base with no per-unit claims cannot say what the batch claimed: nothing is deferred
        nobase = dict(snap)
        nobase.pop("unit_claims")
        objs(data=["w_bss"], fill=1)
        eq((len(row(nobase)["added"]), row(nobase)["claim_exposed"]), (1, []), "no base claims: refused, never guessed")

        # the tree view the backlog reads
        objs(refs=["w_bss"], data=["w_bss2"], fill=1)
        ranges = load_claims(tmp)
        (records, _stats), _n, _have = census(tmp, None, ranges=ranges)
        runs = tree_claim_exposed(tmp, records)
        eq([(r["unit"], r["section"], r["start"], len(r["pairs"])) for r in runs], [("A/a", ".bss", 0x806A0004, 1)],
           "the tree view lists the orphan only claimed data references, not the code-read one")


def selftest_fold(eq) -> None:
    """Folds, deleted units and renames: the base snapshot follows the bytes, not the unit name (2026-09-30)."""
    import tempfile

    base_t = {"A/a": {".text": [[0x100, 0x200]]}, "A/b": {".text": [[0x200, 0x300]]}, "A/k": {".text": [[0x300, 0x400]]},
              "A/r": {".text": [[0x500, 0x600]]}}
    now_t = {"A/c": {".text": [[0x100, 0x300]]}, "A/k": {".text": [[0x300, 0x380]]}, "A/n": {".text": [[0x380, 0x400]]},
             "A/r2": {".text": [[0x500, 0x600]]}}
    got = derive_absorption(base_t, now_t)
    eq(got, {"A/c": ["A/a", "A/b"], "A/n": ["A/k"], "A/r2": ["A/r"]},
       "a fold (two units into one), a shrunk unit's moved part and a 1:1 rename are all derived from the ranges")
    eq(derive_absorption(base_t, base_t), {}, "an unchanged claim table derives nothing")
    eq(derive_absorption({"A/a": {}}, {"A/z": {}}, {"A/a": "A/z"}), {"A/z": ["A/a"]}, "git's rename pairs are added")
    eq(explicit_absorption(got, {"A/b": "A/x", "A/r": ""}),
       {"A/c": ["A/a"], "A/n": ["A/k"], "A/x": ["A/b"]},
       "an explicit OLD=NEW moves OLD to that NEW; OLD= leaves it with no absorber")

    snap = {"keys": ["orphan:A/a:.data:80798244", "orphan:A/b:.data:80798250", "orphan:A/k:.data:80798254",
                     "orphan:A/r:.data:80798258"],
            "claims": {}, "unit_claims": base_t, "objects": {"A/a": {"body": "1"}, "A/c": {"body": "2"}}}
    f = fold_snapshot(snap, got, now_t)
    eq(f["keys"], ["orphan:A/c:.data:80798244", "orphan:A/c:.data:80798250", "orphan:A/k:.data:80798254",
                   "orphan:A/n:.data:80798254", "orphan:A/r2:.data:80798258"],
       "pairs of both folded units merge under the absorber, the deleted units' own keys go, a survivor keeps its own")
    eq(f["unit_claims"]["A/c"], {".text": [[0x100, 0x300]]}, "the absorber's base claims are the union of what it took")
    eq("A/a" in f["unit_claims"] or "A/b" in f["unit_claims"], False, "a deleted unit's claims are dropped")
    eq(f["unit_claims"]["A/k"], base_t["A/k"], "a survivor's own base claims are untouched")
    eq(sorted(f["objects"]), ["A/c"], "a deleted unit's object fingerprint is dropped, the absorber keeps its own")
    eq(sorted(snap["keys"])[0], "orphan:A/a:.data:80798244", "the recorded snapshot is left untouched")
    eq(fold_snapshot(snap, {}, now_t, {"A/a": ""})["keys"].count("orphan:A/a:.data:80798244"), 0,
       "OLD= drops the pairs with the unit")

    splits_base = ("Sections:\n\t.text type:code align:32\n\nA/a.cpp:\n\t.text start:0x80010000 end:0x80010100\n\n"
                   "A/b.cpp:\n\t.text start:0x80010100 end:0x80010200\n")
    splits_now = "Sections:\n\t.text type:code align:32\n\nA/c.cpp:\n\t.text start:0x80010000 end:0x80010200\n"
    symbols = ("w1 = .bss:0x806A0000; // type:object size:0x4\nw2 = .bss:0x806A0004; // type:object size:0x4\n"
               "w3 = .bss:0x806A0008; // type:object size:0x4\n")
    with tempfile.TemporaryDirectory() as tmp:
        def put(rel, text):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def obj(unit, refs):
            for kind in ("src", "obj"):
                path = os.path.join(tmp, "build", "RMHE08", kind, unit + ".o")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                build_fixture_object(path, refs)

        put("config/RMHE08/symbols.txt", symbols)
        put("config/RMHE08/splits.txt", splits_base)
        obj("A/a", ["w1"])
        obj("A/b", ["w2"])
        base = snapshot_orphans(tmp, ["A/a", "A/b"])
        put("config/RMHE08/splits.txt", splits_now)
        for kind in ("src", "obj"):
            for gone in ("a", "b"):
                os.remove(os.path.join(tmp, "build", "RMHE08", kind, "A", gone + ".o"))
        obj("A/c", ["w1", "w2"])
        unmapped = batch_orphans(tmp, ["A/c"], {k: v for k, v in base.items() if k != "unit_claims"}, query=None)
        eq(len(unmapped["added"]), 2, "without the base claims the fold cannot be derived: both pre-existing pairs read as added")
        v = batch_orphans(tmp, ["A/c"], base, query=None)
        eq(v["added"], [], "a fold of two units: their pre-existing pairs are merged under the absorber, no false additions")
        eq(len(v["pre_existing"]), 2, "... and they are reported as pre-existing")
        eq(v["unit_map_lines"], ["data closure: unit map (base pairs follow the bytes): A/c <- A/a, A/b"],
           "... and the derived map is printed for the gate log")
        obj("A/c", ["w1", "w2", "w3"])
        v = batch_orphans(tmp, ["A/c"], base, query=None)
        eq(len(v["added"]), 1, "a genuinely new pair in the absorber still refuses")
        obj("A/c", ["w1", "w2"])
        v = batch_orphans(tmp, ["A/c"], base, query=None, unit_map={"A/b": ""})
        eq(len(v["added"]), 1, "an explicit OLD= override takes that unit's pair out of the merge: its pair is added")
        v = batch_orphans(tmp, ["A/c"], base, query=None, unit_map={"A/a": "A/c", "A/b": "A/c"})
        eq(v["added"], [], "explicit OLD=NEW pairs sharing one NEW merge rather than overwrite")

    # the freshness scan is scoped to the batch's units
    with tempfile.TemporaryDirectory() as tmp:
        eq(tree_freshness(tmp, units=["A/c"])["total"], 0, "a tree with no map files has nothing to be stale for the batch")


def selftest_span(eq) -> None:
    """A claim span never covers another unit's read; the anchor survives; the fixpoint; the census freshness."""
    import tempfile

    splits = ("Sections:\n\t.text type:code align:32\n\n"
              "A/a.cpp:\n\t.text start:0x80010000 end:0x80010100\n\t.data start:0x805E1000 end:0x805E1010\n\n"
              "B/b.cpp:\n\t.text start:0x80010100 end:0x80010200\n\n"
              "C/c.cpp:\n\t.text start:0x80010200 end:0x80010300\n\t.data start:0x805E2000 end:0x805E2010\n")
    ranges = parse_splits_text(splits)

    def sym(section, address, size=4):
        return {"section": section, "address": address, "size": size, "type": "object"}

    # the 0x805A1368 shape: A/a's own claim carries no pair; its sole-owned word sits far below it with a word
    # another unit reads in between (and a second word of its own below that one)
    symbols = {"a1": sym(".data", 0x805E1000), "far": sym(".data", 0x805E0000), "theirs": sym(".data", 0x805E0100),
               "mid": sym(".data", 0x805E0F00), "near": sym(".data", 0x805E1010)}
    refs = {"A/a": {"far": 1}, "B/b": {"theirs": 1}}
    records, _s = census_records(refs, symbols, ranges)
    with tempfile.TemporaryDirectory() as tmp:
        rep = strict_report(tmp, records, ranges, ["A/a"], query=None, symbols=symbols)
        blocks = [b for b in rep["blocks"] if b["unit"] == "A/a"]
        eq([(b["section"], b["start"], b["verdict"], b["cls"]) for b in blocks],
           [(".data", 0x805E0000, "deferred", "span-blocked")],
           "a word below the unit's own claim with another unit's read between is span-blocked, never main")
        eq("blocked by B/b reads theirs (0x805E0100)" in blocks[0]["reason"], True,
           "... and the reason names the foreign reader and the address")
        plan = splits_plan(splits, "A/a", blocks, None, span_blocker(rep["ctx"], "A/a"))
        eq(plan["edits"], [], "NO plan line is produced for it")
        text = render_plan(plan, "A/a")
        eq(("REPLACE" in text, "ADD " in text, "blocked  span-blocked" in text), (False, False, True),
           "... the rendering says blocked, not REPLACE")
        eq([r.strip() for r in plan["result"]], [ln.strip() for ln in _unit_lines(splits.splitlines(), "A/a")],
           "... and the unit's block is unchanged")

        # the quest_entry shape: the anchor has no pairs and the only pair is past a foreign claim
        refs2 = {"A/a": {"near": 1}, "C/c": {"a1": 1}}
        records2, _s = census_records(refs2, symbols, ranges)
        rep2 = strict_report(tmp, records2, ranges, ["A/a"], query=None, symbols=symbols)
        b2 = [b for b in rep2["blocks"] if b["unit"] == "A/a"]
        eq([(b["start"], b["verdict"]) for b in b2], [(0x805E1000, "refuse")],
           "a pair adjacent to the unit's claim merges into the anchor block and stays refusable")
        eq((b2[0]["start"], b2[0]["end"], len(b2[0]["anchors"])), (0x805E1000, 0x805E1014, 1),
           "... the block is the span and records the anchor it merged")
        p2 = splits_plan(splits, "A/a", b2, None, span_blocker(rep2["ctx"], "A/a"))
        eq([(e["action"], e["start"], e["end"]) for e in p2["edits"]], [("replace", 0x805E1000, 0x805E1014)],
           "... and the replacement spans the claim and the word exactly")

        # an anchor with no pair keeps every other block a SECOND range, even when several pairs are far away
        symbols3 = dict(symbols, far2=sym(".data", 0x805E0010), far3=sym(".data", 0x805E0014))
        refs3 = {"A/a": {"far": 1, "far2": 1, "far3": 1}, "B/b": {"theirs": 1}}
        records3, _s = census_records(refs3, symbols3, ranges)
        rep3 = strict_report(tmp, records3, ranges, ["A/a"], query=None, symbols=symbols3)
        eq(sorted((b["start"], len(b["pairs"]), b["verdict"], b["cls"]) for b in rep3["blocks"] if b["unit"] == "A/a"),
           [(0x805E0000, 3, "deferred", "span-blocked")],
           "with the anchor as main, a far block of three pairs still does not become main")

        # a foreign claim in the gap is named too
        ranges4 = parse_splits_text(splits + "\nD/d.cpp:\n\t.text start:0x80010300 end:0x80010400\n"
                                    "\t.data start:0x805E0800 end:0x805E0810\n")
        records4, _s = census_records({"A/a": {"far": 1}}, symbols, ranges4)
        rep4 = strict_report(tmp, records4, ranges4, ["A/a"], query=None, symbols=symbols)
        eq("blocked by D/d claims 0x805E0800-0x805E0810" in rep4["blocks"][0]["reason"], True,
           "another unit's claim between the ranges is named as the blocker")

        # a reader only callers.py knows (an unsplit function) blocks the gap as well
        rep5 = strict_report(tmp, records4, ranges, ["A/a"], query=lambda a: (({}, 1) if a == 0x805E0F00 else ({}, 0)),
                             symbols=symbols)
        eq("blocked by unsplit code reads mid (0x805E0F00)" in rep5["blocks"][0]["reason"], True,
           "an unsplit reader found by callers is named as the blocker")

    # the plan's own guard: a refusable block whose span still covers a foreign read is blocked, with no edit
    ok_block = {"unit": "A/a", "section": ".data", "start": 0x805E1000, "end": 0x805E1014, "verdict": "refuse",
                "cls": None, "pairs": [1], "reason": "x", "anchors": [(0x805E1000, 0x805E1010)]}
    guard = splits_plan(splits, "A/a", [ok_block], None, lambda sec, lo, hi: "B/b reads theirs (0x805E1012)")
    eq((guard["edits"], [b["cls"] for b in guard["deferred"]]), ([], ["span-blocked"]),
       "the plan guard turns a span over a foreign read into a blocked entry")
    eq("blocked by B/b reads theirs" in guard["deferred"][0]["reason"], True, "... naming the reader")
    clean = splits_plan(splits, "A/a", [ok_block], None, lambda sec, lo, hi: None)
    eq(len(clean["edits"]), 1, "a clean guard lets the edit through")
    # a same-section line of the unit outside the block is never folded into the span
    two = splits.replace("\t.data start:0x805E1000 end:0x805E1010\n",
                         "\t.data start:0x805E1000 end:0x805E1010\n\t.data start:0x805E1800 end:0x805E1810\n")
    far_block = dict(ok_block, start=0x805E1010, end=0x805E1014, anchors=[(0x805E1000, 0x805E1010)])
    p_two = splits_plan(two, "A/a", [dict(far_block, start=0x805E1000, end=0x805E1014)], None)
    eq([(e["start"], e["end"]) for e in p_two["edits"]], [(0x805E1000, 0x805E1014)],
       "a replacement folds in only the lines inside the block: the unit's other .data line stays as it is")
    eq(any("0x805E1800" in r for r in p_two["result"]), True, "... and is still in the resulting block")
    p_none = splits_plan(two, "A/a", [dict(ok_block, start=0x805E0F00, end=0x805E0F04, anchors=[])], None)
    eq(([e for e in p_none["edits"]], p_none["deferred"][0]["cls"]), ([], "span-blocked"),
       "a block that would be a third/second range beside the unit's existing ones is blocked, not added")

    # the fixpoint: apply, re-judge, stop when stable
    refs_f = {"A/a": {"near": 1, "far": 1}, "B/b": {"theirs": 1}}
    with tempfile.TemporaryDirectory() as tmp:
        fp = fixpoint_plan(tmp, "A/a", text=splits, unit_refs=refs_f, symbols=symbols, query=None)
        eq((fp["converged"], fp["steps"]), (True, 1), "one claim step reaches the fixpoint")
        eq(fp["net"]["after"][-1].strip(), ".data       start:0x805E1000 end:0x805E1014", "the net edit is the span claim")
        eq([b["cls"] for b in fp["blockers"]], ["span-blocked"], "the far word is reported blocked at the fixpoint")
        eq("blocked by B/b reads theirs (0x805E0100)" in fp["blockers"][0]["reason"], True, "... with the reader named")
        again = fixpoint_plan(tmp, "A/a", text=fp["text"], unit_refs=refs_f, symbols=symbols, query=None)
        eq((again["converged"], again["steps"], again["net"]["before"] == again["net"]["after"]), (True, 0, True),
           "asking again after applying the converged plan yields no plan: it is stable")
        eq("converged after 1 claim step(s)" in render_fixpoint(fp) and "+ .data" in render_fixpoint(fp), True,
           "the rendering carries the converged plan")
        eq("already at its fixpoint" in render_fixpoint(again), True, "... or says nothing is left")
        eq(fixpoint_plan(tmp, "Z/z", text=splits, unit_refs=refs_f, symbols=symbols, query=None)["final"]["found"],
           False, "an unregistered unit has no block to fix")

        # a plan that never settles is reported with its reason, not looped on
        real = globals()["splits_plan"]
        flip = [0]

        def unstable(text, unit, blocks, our_sizes=None, blocker=None):
            flip[0] += 1
            end = 0x805E1020 if flip[0] % 2 else 0x805E1010
            line = "\t.data       start:0x805E1000 end:0x%08X" % end
            return {"found": True, "header": "A/a.cpp:", "deferred": [], "result": ["+ " + line],
                    "edits": [{"action": "replace", "section": ".data", "start": 0x805E1000, "end": end, "pairs": 1}]}

        globals()["splits_plan"] = unstable
        try:
            bad = fixpoint_plan(tmp, "A/a", text=splits, unit_refs=refs_f, symbols=symbols, query=None)
        finally:
            globals()["splits_plan"] = real
        eq((bad["converged"], "no fixpoint" in bad["stop"]), (False, True), "an oscillating plan is reported, not looped")
        eq("NOT converged" in render_fixpoint(bad), True, "... and the rendering names it")

    # the census freshness: a current object's sections are as large as its unit's claims
    with tempfile.TemporaryDirectory() as tmp:
        def put(rel, text):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        fsplits = ("Sections:\n\t.text type:code align:32\n\nA/a.cpp:\n\t.text start:0x80010000 end:0x80010004\n"
                   "\t.data start:0x805E0000 end:0x805E0004\n")
        put("config/RMHE08/splits.txt", fsplits)
        put("config/RMHE08/symbols.txt", "p1 = .data:0x805E0010; // type:object size:0x4\n")
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj", "A"), exist_ok=True)
        build_fixture_object(os.path.join(tmp, "build", "RMHE08", "obj", "A", "a.o"), ["p1"])
        fr = tree_freshness(tmp)
        eq((fr["total"], fr["stale"]), (1, 0), "an object whose sections equal the claims is current")
        eq(render_freshness("base", fr), [], "... and no warning is printed")
        put("config/RMHE08/splits.txt", fsplits.replace("0x805E0004", "0x805E0010"))
        fr = tree_freshness(tmp)
        eq((fr["stale"], fr["examples"][0][:3]), (1, ("A/a", ".data", 0x10)), "a claim the object does not carry is stale")
        warn = render_freshness("base", fr)
        eq(len(warn) == 1 and "base census built from objects older than splits.txt/symbols.txt" in warn[0], True,
           "... with the WARNING naming the unit, section and sizes")
        eq(tree_freshness(os.path.join(tmp, "nowhere"))["total"], 0, "a tree with no map files has nothing to be stale")

    # the --base-root discrepancy: which cause made a pair appear only in the tree's census
    with tempfile.TemporaryDirectory() as tmp:
        def put2(root, rel, text):
            path = os.path.join(root, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        esplits = ("Sections:\n\t.text type:code align:32\n\nA/a.cpp:\n\t.text start:0x80010000 end:0x80010004\n")
        esyms = "p1 = .data:0x805E0010; // type:object size:0x4\n"
        base, tree = os.path.join(tmp, "base"), os.path.join(tmp, "tree")
        for root in (base, tree):
            put2(root, "config/RMHE08/splits.txt", esplits)
            put2(root, "config/RMHE08/symbols.txt", esyms)
        os.makedirs(os.path.join(base, "build", "RMHE08", "obj", "A"), exist_ok=True)
        build_fixture_object(os.path.join(base, "build", "RMHE08", "obj", "A", "a.o"), [])
        os.makedirs(os.path.join(tree, "build", "RMHE08", "obj", "A"), exist_ok=True)
        build_fixture_object(os.path.join(tree, "build", "RMHE08", "obj", "A", "a.o"), ["p1"])
        lines = explain_added(tree, base, ["A/a"], [], parse_splits_text(esplits))
        eq(len(lines), 1, "one cause line per (unit, cause)")
        eq("does not reference it" in lines[0] and "EXPOSED" not in lines[0], True,
           "a pair the base object never referenced is reported as differing objects")
        from units import undefrefs as uref

        real_load = uref.load_object
        uref.load_object = lambda path: ({"relocs": [{"symbol": "p1", "target": ".data", "offset": 0}], "defined": {},
                                          "refs": {"p1"}} if path.startswith(tree) else real_load(path))
        try:
            lines = explain_added(tree, base, ["A/a"], [], parse_splits_text(esplits))
        finally:
            uref.load_object = real_load
        eq("EXPOSED BY THE CLAIM" in lines[0] and "--fixpoint" in lines[0], True,
           "a pair only the unit's claimed .data references is named as exposed by the claim")
        eq(explain_added(tree, base, ["A/a"], [orphan_key("A/a", ".data", 0x805E0010)], parse_splits_text(esplits)), [],
           "a pair the base already has needs no explanation")


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
    selftest_exposed(eq)
    selftest_fold(eq)
    selftest_span(eq)

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
