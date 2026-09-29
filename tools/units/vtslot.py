#!/usr/bin/env python3
"""The reverse of `vtableaudit --at`: given a function address, find the `.data` table that holds it.

`vtableaudit --at <table>` goes table -> slots. Lanes kept re-deriving the other direction by hand - the
`network_transport` lane named 15 of 19 rows in ten minutes by packing a function's address big-endian and
scanning the DOL's data sections for it, because the hit *is* the table slot and therefore names the class
and the vtable offset. Its evidence is `MAIN/.pi/notes/net-transport-eeda.md`:

    the .data run 0x805F94E0..0x805FA908 is TWO vtables with one log string between them, not five
    interleaved tables: 0x805F9510 = NetworkPeerBuffer, 0x805F95E0 = NetworkPeerSocket.

This is that hand recipe as a tool, for one or more addresses. For each address it reports **every 4-byte
location in the DOL's loaded data sections whose value equals the address** - a vtable slot, a jump-table
entry, an extabindex entry and a plain pointer in a struct all surface the same way - each labelled by the
registered range that contains it (which unit, which section) or as an unregistered band. When the location
sits inside a run of consecutive code pointers, it prints the table's base, the slot index and the byte
offset, and the neighbouring slots' targets with their owning units and map symbols - the adjacency that is
what makes a class name guessable.

The table -> slots half is **not** re-implemented here: the run's slots are enumerated with
`vtableaudit.vtable_slots` (the same function `--at` uses), and the owner of each target comes from
`vtableaudit.owner_at`. The unit spelling goes through `claims.norm_unit`, the one definition every tool
shares. The only new reader is the 4-byte big-endian word scan itself.

What it does, in the order the report reads it:

* **every word hit.** A 4-byte, 4-aligned word in a DOL *data* section whose value equals the address.
  `.text` is not scanned: an address appearing there is an instruction immediate (the `lis`/`addi` pair's
  split halves never form the full aligned word), not a pointer.
* **where the hit is.** The registered split range that covers it, with the unit normalised through
  `claims.norm_unit`, or `unregistered` when no range owns the band (the state the transport unit's `.data`
  is still in - it is a config *request*, not a claim).
* **the run it sits in.** A maximal run of consecutive words that are code pointers (addresses inside the
  DOL's `.text`). One word is an ordinary pointer, not a table; `vtableaudit.MIN_RUN_WORDS` (2) is the same
  threshold `--at` uses. A run is bounded by the DOL data section, by the registered range when the hit is
  in one, and by a hard word cap; a run clipped by the cap says so (`truncated`) instead of guessing.
* **the table base.** A Metrowerks/Itanium vtable is two RTTI words (offset-to-top and typeinfo) followed
  by the function pointers; when those two words immediately precede the run and are zero, the base is
  reported *including* them, which is the base the transport lane labelled its slots from (`networkPeer_init`
  is slot 0x24 of the socket table at 0x805F95E0, `networkPeer_setInfo` slot 0x0C). The map symbol at the
  base (`lbl_805F95E0`) is reported when `symbols.txt` names it.

Read-only by construction: no `ninja`, no compile, no link, no write anywhere. The DOL header, sections and
maps are read through `vtableaudit`'s readers (`dol_segments`/`dol_read`/`dol_text_ranges`) and
`vtableaudit.load_tree`, never a second parser.

    python tools/units/vtslot.py 0x803CDFC0                  # -> socket table 0x805F95E0, slot 0x24
    python tools/units/vtslot.py 0x803CD854 0x803CF7A8       # a slot and a plain function (no hits)
    python tools/units/vtslot.py --json 0x803CD854
    python tools/units/vtslot.py 0x803CD854 0x803CDFC0 0x803CF7A8
    python tools/units/vtslot.py --scan 0x805F9510 0x805FA908 # bounded band, report every pointer to it
    python tools/units/vtslot.py --selftest

`--scan` flips the question to "what points INTO this band": every 4-aligned code pointer whose target
lands in `[start, end)` is reported with the same per-hit labelling, which names a whole table's callers at
once. `--main` points at a tree holding `orig/RMHE08/sys/main.dol` and `config/RMHE08/` (default: this
file's tree).
"""

from __future__ import annotations

import argparse
import json
import os
import statistics
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _p in (os.path.dirname(HERE), HERE, os.path.join(ROOT, "tools", "elf")):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import claims  # noqa: E402  (norm_unit - the one spelling rule every tool shares)
import vtableaudit as va  # noqa: E402  (dol_segments/dol_read/dol_text_ranges/load_tree, vtable_slots)

GAME = "RMHE08"
DOL_REL = os.path.join("orig", GAME, "sys", "main.dol")

# A table's function-pointer run cannot reasonably exceed this; a run clipped by it is reported as
# `truncated` rather than silently extended. `vtableaudit.vtable_slots` has its own, smaller cap.
MAX_RUN_WORDS = 512

# A Metrowerks/Itanium vtable's two RTTI words before the first function pointer.
RTTI_WORDS = 2

# The sections a map symbol can live in, most likely first - `map_symbol` only ever uses this to look for
# an EXACT address match, so the order is cosmetic.
LOOKUP_SECTIONS = (".data", ".rodata", ".sdata", ".sdata2", ".text", ".init",
                   "extab", "extabindex", ".ctors", ".dtors")


# --------------------------------------------------------------------------------------------------
# the pure core (everything the selftest drives from a fixture blob)
# --------------------------------------------------------------------------------------------------
def data_segments(blob: bytes) -> dict:
    """`{name: (start, size, file_offset)}` for the DOL's **data** sections, in header order.

    The DOL header declares seven text and eleven data sections (`vtableaudit.dol_segments`); the text
    ones are dropped by testing each against `vtableaudit.dol_text_ranges`, so no second header parser is
    written. `name` is `data0`.. in header order - the DOL has no section names of its own.
    """
    text = va.dol_text_ranges(blob)
    out, index = {}, 0
    for start, size, offset in va.dol_segments(blob):
        if va.in_ranges(text, start):
            continue
        out["data%d" % index] = (start, size, offset)
        index += 1
    return out


def find_word_hits(blob: bytes, address: int) -> list:
    """`[(segment_name, location)]` for every 4-aligned data word whose value equals `address`.

    The whole search is `bytes.find` over each data section - the exact move the transport lane made by
    hand. An unaligned occurrence is not a 4-byte location and is skipped (a little-endian or offset
    overlap is not a pointer word).
    """
    packed = struct.pack(">I", address & 0xFFFFFFFF)
    hits = []
    for name, (start, size, offset) in data_segments(blob).items():
        data = blob[offset:offset + size]
        i = data.find(packed)
        while i != -1:
            if (start + i) % 4 == 0:
                hits.append((name, start + i))
            i = data.find(packed, i + 1)
    return hits


def scan_band_hits(blob: bytes, start: int, end: int) -> list:
    """`[(segment_name, location, target)]` for every 4-aligned code pointer into `[start, end)`.

    The inverse question of `find_word_hits`: not "where is this exact value stored" but "every stored
    word that points anywhere into this band". Read from the DOL, so each pointer is already resolved.
    """
    hits = []
    for name, (seg_start, size, offset) in data_segments(blob).items():
        data = blob[offset:offset + size]
        for i in range(0, len(data) - 3, 4):
            loc = seg_start + i
            value = struct.unpack_from(">I", data, i)[0]
            if start <= value < end:
                hits.append((name, loc, value))
    return hits


def containing_range(splits: dict, address: int):
    """`(unit, range_dict)` for the registered range covering `address`, or `(None, None)`.

    The range is the one `splits.txt` declares, so `range_dict` carries the split's own `section`
    (`extabindex`, `.data`, ...) - the label the hit is reported under.
    """
    for unit, ranges in splits.items():
        for r in ranges:
            if r["start"] <= address < r["end"]:
                return unit, r
    return None, None


def map_symbol(tree: dict, address: int, preferred: str | None = None) -> str | None:
    """The `symbols.txt` name at `address`, or None - an **exact** match, never the nearest symbol.

    A nearest-symbol fallback would attach a neighbouring string's label to a table base, which is the
    kind of guess this tool exists to avoid. The scan looks for an exact row in any section first (a table
    base in an unregistered band is still named `.data` in the map); `preferred` narrows it through
    `vtableaudit.symbol_at` when the caller knows the section, and the result is accepted only if it is
    that same exact row.
    """
    symbols = tree["symbols"]
    for name, row in symbols.items():
        if row[1] == address:
            return name
    if preferred:
        got = va.symbol_at(symbols, preferred, address)
        if got and got in symbols and symbols[got][1] == address:
            return got
    return None


def word_at(blob: bytes, address: int, segments: dict):
    """The 4-byte big-endian word at `address`, or None when the DOL does not carry it."""
    for _name, (start, size, offset) in segments.items():
        if start <= address < start + size:
            i = offset + address - start
            if i + 4 <= len(blob):
                return struct.unpack_from(">I", blob, i)[0]
            return None
    return None


def run_bounds(tree: dict, blob: bytes, location: int) -> dict | None:
    """The maximal code-pointer run containing `location`, or None when it cannot be bounded.

    Bounded by the DOL data section, by the registered range when the hit is in one, and by
    `MAX_RUN_WORDS`. `truncated` is set when the cap (not a bound) stopped the walk, and `bounded_by` says
    which; a caller must not read a truncated run as the whole table. `None` means "no data section covers
    the location" - the scan cannot be bounded at all.
    """
    segments = data_segments(blob)
    seg = next(((n, s) for n, s in segments.items() if s[0] <= location < s[0] + s[1]), None)
    if seg is None:
        return None
    seg_name, (seg_start, seg_size, _off) = seg
    lo_bound, hi_bound = seg_start, seg_start + seg_size
    bounded_by = "segment"
    unit, rng = containing_range(tree["splits"], location)
    if rng is not None:
        lo_bound = max(lo_bound, rng["start"])
        hi_bound = min(hi_bound, rng["end"])
        bounded_by = "range"
    # the window is the run's search space: centred on the hit, a radius of MAX_RUN_WORDS
    win_lo = max(lo_bound, location - 4 * MAX_RUN_WORDS)
    win_lo -= (win_lo - seg_start) % 4
    win_hi = min(hi_bound, location + 4 * MAX_RUN_WORDS)
    flags, n = [], (win_hi - win_lo) // 4
    for i in range(n):
        value = word_at(blob, win_lo + 4 * i, segments)
        flags.append(value is not None and va.in_ranges(tree["text_ranges"], value))
    for first, words in va.find_runs(flags, min_words=1):
        run_start = win_lo + 4 * first
        if run_start <= location < run_start + 4 * words:
            clipped_lo = first == 0 and win_lo > lo_bound
            clipped_hi = first + words == len(flags) and win_hi < hi_bound
            return {"start": run_start, "words": words, "segment": seg_name,
                    "truncated": bool(clipped_lo or clipped_hi),
                    "bounded_by": "max" if (clipped_lo or clipped_hi) else bounded_by}
    return None


def rtti_base(tree: dict, blob: bytes, run: dict, location: int) -> int:
    """The table base including the two RTTI words when they immediately precede the run, else the run base.

    The transport lane numbered its slots from the vtable base (`0x805F95E0`), not from the first function
    pointer (`0x805F95E8`); `networkPeer_init` is slot 0x24 only under the former. The two words are used
    only when they read as zero and sit inside the same bounds as the run, so an adjacent run's pointers
    are never mistaken for RTTI.
    """
    if run["truncated"] or run["start"] - 8 < 0:
        return run["start"]
    unit, rng = containing_range(tree["splits"], location)
    if rng is not None and run["start"] - 8 < rng["start"]:
        return run["start"]
    for i in range(RTTI_WORDS):
        if word_at(blob, run["start"] - 4 * (i + 1), data_segments(blob)) != 0:
            return run["start"]
    return run["start"] - 8


def locate(tree: dict, blob: bytes, address: int) -> list:
    """Every data location holding `address`, each enriched with its band and (when it is a run) its table.

    `tree` needs only the keys `vtableaudit.load_tree` produces (`splits`, `symbols`, `text_ranges`); the
    slots of a run are enumerated by `vtableaudit.vtable_slots` - the same call `--at` makes - so the
    table -> slots direction lives in exactly one place.
    """
    is_code = va.in_ranges(tree["text_ranges"], address)
    return [enrich_hit(tree, blob, address, is_code, segment, location)
            for segment, location in find_word_hits(blob, address)]


def enrich_hit(tree: dict, blob: bytes, address: int, is_code: bool,
               segment: str, location: int) -> dict:
    """Label one word hit and, when it is in a code-pointer run, add the table's base, slot and neighbours."""
    unit, rng = containing_range(tree["splits"], location)
    rec = {
        "location": location,
        "segment": segment,
        "registered": rng is not None,
        "band": "registered" if rng is not None else "unregistered",
        "unit": claims.norm_unit(unit) if unit else None,
        "section": rng["section"] if rng is not None else None,
        "range": ({"unit": claims.norm_unit(unit), "section": rng["section"],
                   "start": rng["start"], "end": rng["end"]} if rng is not None else None),
        "kind": None,
        "run": None,
        "table": None,
        "pointer": None,
    }
    if not is_code:
        rec["kind"] = "data-pointer"
        rec["pointer"] = {"target": address}
        return rec
    bounds = run_bounds(tree, blob, location)
    if bounds is None:
        rec["kind"] = "code-pointer-unbounded"
        rec["pointer"] = {"target": address}
        return rec
    if bounds["words"] < va.MIN_RUN_WORDS:
        rec["kind"] = "code-pointer"
        rec["run"] = dict(bounds, byte_offset=location - bounds["start"],
                          slot_index=(location - bounds["start"]) // 4)
        rec["pointer"] = {"target": address,
                          "symbol": map_symbol(tree, address, preferred=".text")}
        return rec
    slots = va.vtable_slots(tree, bounds["start"])
    if not slots or not any(s["address"] == location for s in slots):
        # the run is real but `--at`'s own walk disagrees (it caps at the table's registered range or
        # 512 words) - report the run, do not invent slots around the disagreement
        rec["kind"] = "code-pointer-run-unenumerated"
        rec["run"] = dict(bounds, byte_offset=location - bounds["start"],
                          slot_index=(location - bounds["start"]) // 4)
        rec["pointer"] = {"target": address}
        return rec
    base = rtti_base(tree, blob, bounds, location)
    header = []
    if base != bounds["start"]:
        for i in range(RTTI_WORDS):
            header.append({"index": i, "address": base + 4 * i, "target": 0,
                           "owner": None, "symbol": None, "rtti": True})
    rows = header + [
        {"index": (s["address"] - base) // 4, "address": s["address"], "target": s["target"],
         "owner": claims.norm_unit(s["owner"]) if s["owner"] else None,
         "symbol": s["symbol"], "rtti": False}
        for s in slots]
    rec["kind"] = "table"
    rec["run"] = dict(bounds, byte_offset=location - bounds["start"],
                      slot_index=(location - bounds["start"]) // 4)
    rec["table"] = {
        "base": base,
        "with_rtti": base != bounds["start"],
        "symbol": map_symbol(tree, base, preferred=rec["section"] or ".data"),
        "run_base": bounds["start"],
        "run_words": bounds["words"],
        "truncated": bounds["truncated"],
        "bounded_by": bounds["bounded_by"],
        "slot_index": (location - base) // 4,
        "byte_offset": location - base,
        "slots": rows,
    }
    return rec


# --------------------------------------------------------------------------------------------------
# the impure edge: read the tree, scan, time it
# --------------------------------------------------------------------------------------------------
def scan(main: str, addresses: list) -> dict:
    """Load the tree once and locate every address, reporting per-address and cold/warm scan times.

    `cold` is a fresh invocation's first address: the DOL read and the map parse happen once, before any
    scan, and that total is `cold_s`. `warm` is the median of the later addresses in the same process,
    whose maps are already parsed - the cost a re-query pays. The **raw scan** (`raw_s`, the big-endian
    `bytes.find` the transport lane hand-rolled) and the **enrichment** (`enrich_s`, the run/table walk
    through `vtableaudit.vtable_slots`) are timed apart, because the raw scan is the cheap half and the
    report should show it.
    """
    main = os.path.abspath(main)
    t0 = time.perf_counter()
    tree = va.load_tree(main)
    load_s = time.perf_counter() - t0
    blob = tree.get("blob")
    dol_path = os.path.join(main, DOL_REL)
    out = {"root": main, "dol": dol_path.replace("\\", "/"),
           "dol_present": bool(blob), "text_source": tree["text_source"],
           "load_s": round(load_s, 6), "addresses": [], "band": None}
    per, raw = [], []
    for index, address in enumerate(addresses):
        is_code = va.in_ranges(tree["text_ranges"], address)
        t = time.perf_counter()
        found = find_word_hits(blob, address) if blob else []
        raw_s = time.perf_counter() - t
        t = time.perf_counter()
        hits = [enrich_hit(tree, blob, address, is_code, segment, location)
                for segment, location in found]
        enrich_s = time.perf_counter() - t
        elapsed = raw_s + enrich_s
        per.append(elapsed)
        raw.append(raw_s)
        row = {"address": address,
               "is_code": is_code,
               "symbol": map_symbol(tree, address, preferred=".text"),
               "hits": hits,
               "raw_s": round(raw_s, 6),
               "enrich_s": round(enrich_s, 6),
               "scan_s": round(elapsed, 6),
               "cold": index == 0}
        if not blob:
            row["note"] = "no DOL at %s: the loaded image cannot be scanned" % dol_path
        else:
            row["note"] = None
        out["addresses"].append(row)
    out["raw_cold_s"] = round(raw[0], 6) if raw else None
    out["raw_warm_s"] = round(statistics.median(raw[1:]), 6) if len(raw) > 1 else None
    out["cold_scan_s"] = round(per[0], 6) if per else None
    out["warm_scan_s"] = round(statistics.median(per[1:]), 6) if len(per) > 1 else None
    out["cold_s"] = round(load_s + (per[0] if per else 0.0), 6)
    out["warm_s"] = out["warm_scan_s"]
    out["total_scan_s"] = round(sum(per), 6)
    return out


def scan_band(main: str, start: int, end: int) -> dict:
    """Every stored code pointer into `[start, end)`, grouped by the location's band and table."""
    main = os.path.abspath(main)
    t0 = time.perf_counter()
    tree = va.load_tree(main)
    load_s = time.perf_counter() - t0
    blob = tree.get("blob")
    out = {"root": main, "dol": os.path.join(main, DOL_REL).replace("\\", "/"),
           "dol_present": bool(blob), "band": [start, end], "load_s": round(load_s, 6),
           "scan_s": None, "hits": []}
    if not blob:
        out["note"] = "no DOL at %s: the loaded image cannot be scanned" % out["dol"]
        return out
    t = time.perf_counter()
    for segment, location, target in scan_band_hits(blob, start, end):
        unit, rng = containing_range(tree["splits"], location)
        owner = va.owner_at(tree, target, sections=va.CODE_SECTIONS)
        out["hits"].append({
            "location": location, "target": target, "segment": segment,
            "registered": rng is not None,
            "band": "registered" if rng is not None else "unregistered",
            "unit": claims.norm_unit(unit) if unit else None,
            "section": rng["section"] if rng is not None else None,
            "target_symbol": map_symbol(tree, target, preferred=".text"),
            "target_owner": claims.norm_unit(owner) if owner else None,
        })
    out["scan_s"] = round(time.perf_counter() - t, 6)
    return out


# --------------------------------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------------------------------
def render(res: dict, out=sys.stdout) -> None:
    if res.get("band") is not None:
        b0, b1 = res["band"]
        print("vtslot --scan 0x%08X..0x%08X: %d pointer(s) into the band (load %.3fs, scan %.3fs)"
              % (b0, b1, len(res["hits"]), res["load_s"], res["scan_s"] or 0), file=out)
        if not res["dol_present"]:
            print("  %s" % res["note"], file=out)
            return
        for h in res["hits"]:
            print("  [%s] 0x%08X %-6s 0x%08X -> %-28s %s (%s)"
                  % (h["band"], h["location"], h["segment"], h["target"],
                     h["target_owner"] or "(unowned)", h["target_symbol"] or "", h["unit"] or "-"),
                  file=out)
        return

    print("vtslot: reverse vtableaudit --at - %d address(es), %s"
          % (len(res["addresses"]), res["dol"] if res["dol_present"] else "NO DOL"), file=out)
    print("  load %.3fs; cold (first address) %.3fs; warm (median of the rest) %s; raw scan cold %s / "
          "warm %s"
          % (res["load_s"], res["cold_scan_s"] or 0,
             ("%.3fs" % res["warm_scan_s"]) if res["warm_scan_s"] is not None else "-",
             ("%.3fs" % res["raw_cold_s"]) if res["raw_cold_s"] is not None else "-",
             ("%.3fs" % res["raw_warm_s"]) if res["raw_warm_s"] is not None else "-"), file=out)
    for row in res["addresses"]:
        label = row["symbol"] or ""
        code = "code" if row["is_code"] else "not .text"
        print("0x%08X  %-30s  %s  (%d hit(s), raw %.3fs + enrich %.3fs = %.3fs%s)"
              % (row["address"], label, code, len(row["hits"]), row["raw_s"], row["enrich_s"],
                 row["scan_s"], " cold" if row["cold"] else ""), file=out)
        if row["note"]:
            print("    %s" % row["note"], file=out)
        if not row["hits"]:
            print("    no 4-byte data word in the loaded image holds this address", file=out)
            continue
        for hit in row["hits"]:
            where = ("%s %s" % (hit["unit"], hit["section"])) if hit["registered"] else "unregistered band"
            print("    [%s] 0x%08X %-6s  %s"
                  % (hit["band"], hit["location"], hit["segment"], where), file=out)
            _render_hit(hit, out=out)


def _render_hit(hit: dict, out=sys.stdout) -> None:
    if hit["kind"] == "data-pointer":
        print("        a data pointer (the queried address is not in .text) - no table", file=out)
    elif hit["kind"] == "code-pointer":
        sym = (hit["pointer"] or {}).get("symbol") or ""
        print("        a single code pointer, not a table (a run needs %d words)%s"
              % (va.MIN_RUN_WORDS, ("  (%s)" % sym) if sym else ""), file=out)
    elif hit["kind"] == "code-pointer-unbounded":
        print("        unbounded: no DOL data section covers this location - cannot say what table holds "
              "it", file=out)
    elif hit["kind"] == "code-pointer-run-unenumerated":
        print("        a code-pointer run, but vtableaudit.vtable_slots did not enumerate it (bounded "
              "differently) - run at 0x%08X, %d words"
              % (hit["run"]["start"], hit["run"]["words"]), file=out)
    else:
        t = hit["table"]
        print("        TABLE 0x%08X%s%s  slot %d (byte +0x%X)  [run 0x%08X, %d words, bounded by %s%s]"
              % (t["base"], (" (%s)" % t["symbol"]) if t["symbol"] else "",
                 "  incl. rtti" if t["with_rtti"] else "",
                 t["slot_index"], t["byte_offset"], t["run_base"], t["run_words"],
                 t["bounded_by"], ", TRUNCATED" if t["truncated"] else ""), file=out)
        for s in t["slots"]:
            mark = "  <== HIT" if s["address"] == hit["location"] else ""
            if s["rtti"]:
                print("          +0x%02X  0x%08X  %-10s (rtti)%s"
                      % (4 * s["index"], s["address"], "0", mark), file=out)
            else:
                print("          +0x%02X  0x%08X  -> 0x%08X  %-28s %-30s %s"
                      % (4 * s["index"], s["address"], s["target"], s["owner"] or "(unowned)",
                         s["symbol"] or "", mark), file=out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("addresses", nargs="*", metavar="ADDR",
                    help="one or more function addresses (`0x803CDFC0` or `2147549120`)")
    ap.add_argument("--main", default=None, help="tree holding orig/ and config/ (default: this file's)")
    ap.add_argument("--scan", nargs=2, metavar=("START", "END"), default=None,
                    help="every stored code pointer into [START, END) instead of one address")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        import vtslot_selftest
        return vtslot_selftest.selftest()
    main_tree = args.main or ROOT
    if args.scan is not None:
        try:
            start, end = int(args.scan[0], 0), int(args.scan[1], 0)
        except ValueError:
            print("vtslot: --scan wants two addresses (`0x805F9510 0x805FA908`)", file=sys.stderr)
            return 2
        res = scan_band(main_tree, start, end)
        if args.json:
            print(json.dumps(res, indent=2))
        else:
            render(res)
        return 0 if res["dol_present"] else 2
    addresses = []
    for raw in args.addresses:
        try:
            addresses.append(int(raw, 0))
        except ValueError:
            print("vtslot: %r is not an address (`0x803CDFC0` or `2147549120`)" % raw, file=sys.stderr)
            return 2
    if not addresses:
        print("vtslot: give at least one address, or --scan START END", file=sys.stderr)
        return 2
    res = scan(main_tree, addresses)
    if args.json:
        print(json.dumps(res, indent=2))
    else:
        render(res)
    if not res["dol_present"]:
        return 2
    # a hit is the answer; a plain function legitimately has none, so "no hits" is still exit 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
