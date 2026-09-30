#!/usr/bin/env python3
"""Decide, mechanically, whether a proposed data run may be claimed - before `splits.txt` is edited.

docs/plan.md 7.8 / §6.1. Claiming a data range is the campaign's riskiest edit class: three of them were
decided by hand and two were "do not claim" (playbook 23 - the RSO string pool cost 1.35 % on
`fn_804DABF0`, `Gecko_ExceptionPPC.cp`'s `.bss fragmentinfo` would have collapsed `matched_data`). Those
three decisions reduce to one sentence: **claim data the object emits; a range our object does not emit
must not be claimed**, because a target section paired against nothing adds target bytes that nothing
reproduces, so the unit's `matched_data` ratio falls.

This is the reader for `tools/units/data-queue.json` (written by `dataqueue.py`, 7.17; read by `brief.py`).
For every proposed run it answers, with evidence, without compiling and without touching a shared file:

1. **the range next to what is claimed today.** The run's section/start/end is compared against every range
   in `config/RMHE08/splits.txt`; an intersection is a **refusal** (`overlap`), not a warning - the queue is
   a snapshot and `attribute.py apply` can have moved since it was written.
2. **the target's bytes.** Read from the split object that currently covers the address
   (`build/RMHE08/obj/**/*.o`; the object's base address comes from `build/RMHE08/config.json`) and
   cross-checked against `orig/RMHE08/sys/main.dol`. The DOL is the fallback when no object covers the
   address. A covering object whose bytes disagree with the DOL is itself a refusal - the DOL already has
   those bytes right.
3. **what our source emits.** Our object for the run's unit (`build/RMHE08/src/<unit>.o`) is read at the
   offset the run would land on: the unit's lowest claimed range in that section, or the run's own start
   when the claim would create the section. Equal for the whole run -> `safe` (+run size `matched_data`);
   absent, short or different -> `lowers-score`, naming the unit, symbol and worst function it affects.
4. **the expected effect** on the ledger's matched bytes, from the unit's own numbers in
   `build/RMHE08/report.json`.

Verdicts: `safe` | `lowers-score` (the symbol/function it would affect is named) | `overlap` (the run
intersects a claimed range) | `unowned` (nothing to claim it into: an `auto/*` placeholder with no
registered source, or the queue's own `never`/`owner-held`/`not claimed` refusal).

    python tools/units/dataclaim.py                   # verdicts over the whole queue + summary
    python tools/units/dataclaim.py --risky 10        # the riskiest runs, with their reason
    python tools/units/dataclaim.py --queue-unit Pl/pl_act   # one unit's proposed runs
    python tools/units/dataclaim.py --json            # machine-readable entries
    python tools/units/dataclaim.py --out FILE        # write the verdicts (atomic)
    python tools/units/dataclaim.py --selftest

    python tools/units/dataclaim.py --unit Pl/pl_act_step [--dry-run] [--json]
        rule 12, the other direction: every data symbol that unit references but does not own,
        with who else reads it, where the declaration actually sits (`declared in:` - the
        address's owner and the declaration's home can disagree), and the exact `splits.txt`
        claim (or named data-only unit) to fix it.
        Read-only - it never writes `splits.txt`; `--dry-run` just says so explicitly.
        It ends with the land gate's STRICT data-claim view of that unit (`datagap.strict_report`: the
        data only that unit references, refusable or deferred with its class) and the exact `splits.txt`
        edit that claims the refusable blocks (lines to ADD in section order inside the unit's block, or the
        spanning range that REPLACES its existing line, with a partial-run note) - a touched unit must claim it.

Read-only by design: no `ninja`, no compile, no link, no write to `splits.txt`. `land.py` owns the batch
that acts on these verdicts.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _path in (os.path.join(ROOT, "tools", "elf"), os.path.dirname(HERE), HERE):
    if _path not in sys.path:
        sys.path.insert(0, _path)

import elfsect  # noqa: E402  (the project's object section reader)
import symbolpreflight as preflight  # noqa: E402
import ledger as ledger_mod  # noqa: E402  (Objects/covering + the report's numbers)
import dataqueue as dq  # noqa: E402  (the queue's own section order is this tool's sort key)
import dataseams  # noqa: E402  (`.data` emission-order seams - a run spanning several TUs)

GAME = "RMHE08"
QUEUE_REL = os.path.join("tools", "units", "data-queue.json")
DOL_REL = os.path.join("orig", GAME, "sys", "main.dol")
REPORT_REL = os.path.join("build", GAME, "report.json")
OBJ_DIR = os.path.join("build", GAME, "obj")
SRC_DIR = os.path.join("build", GAME, "src")

# A section with no file bytes: `.bss` and friends are zero-filled, so "the target's bytes" is its size.
NOBITS_SECTIONS = (".bss", ".sbss", ".sbss2")
VERDICTS = ("safe", "lowers-score", "overlap", "unowned")
HEX_PREFIX = 16

# What the queue's own pre-measurement verdict means, for the `unowned` reason string.
QUEUE_WHY = {
    "never": "linker-generated data (`_rom_copy_info`/`_bss_init_info`); MW ld emits it",
    "owner-held": "the TRK interrupt-vector table, kept unowned by decision",
    "not claimed": "the run leaks across units or a claimed symbol sits inside it",
}


# -- the pure core (everything the selftest exercises) --------------------------------------------------------

def section_key(section: str) -> tuple:
    """The queue's deterministic section order, so a `--limit` here is the queue's own prefix."""
    order = dq.SECTION_ORDER
    return (order.index(section) if section in order else len(order), section)


def order_key(entry: dict) -> tuple:
    return (section_key(entry["section"]), entry["start"], entry["end"], entry["unit"])


def sort_entries(entries: list[dict]) -> list[dict]:
    return sorted(entries, key=order_key)


def intersects(a_start: int, a_end: int, b_start: int, b_end: int) -> bool:
    """Half-open ranges: touching boundaries do not overlap."""
    return a_start < b_end and b_start < a_end


def overlapping(run: dict, claimed: list[dict]) -> list[dict]:
    """Every claimed range in the run's section that the run intersects - a refusal, not a warning."""
    return [r for r in claimed
            if r["section"] == run["section"] and intersects(run["start"], run["end"], r["start"], r["end"])]


def nearest_claim(run: dict, claimed: list[dict]) -> dict | None:
    """The closest claimed range in the same section, so every row shows what it sits next to."""
    same = [r for r in claimed if r["section"] == run["section"]]
    if not same:
        return None

    def distance(r: dict) -> int:
        if intersects(run["start"], run["end"], r["start"], r["end"]):
            return 0
        return min(abs(run["start"] - r["end"]), abs(r["start"] - run["end"]))

    best = min(same, key=distance)
    return {"unit": best["unit"], "section": best["section"], "start": best["start"], "end": best["end"],
            "distance": distance(best)}


def masked_offsets(reloc_addrs, start: int, size: int) -> set:
    """Offsets in `[0, size)` covered by a 4-byte relocation at an absolute address.

    A relocation site holds the *addend* in the object and the resolved address in the linked DOL, so the
    two legitimately differ there: those bytes are not evidence of anything.
    """
    out = set()
    for address in reloc_addrs or ():
        base = address - start
        for i in range(base, base + 4):
            if 0 <= i < size:
                out.add(i)
    return out


def first_byte_diff(a: bytes, b: bytes, masked: set) -> int | None:
    """The first unmasked byte that differs, or None when the compared prefix agrees."""
    limit = min(len(a), len(b))
    for i in range(limit):
        if i not in masked and a[i] != b[i]:
            return i
    return None


def hex_prefix(data: bytes | None, n: int = HEX_PREFIX) -> str | None:
    return data[:n].hex() if data else None


def dol_sections(blob: bytes) -> list[tuple[int, int, int]]:
    """`(address, size, file_offset)` for every DOL section with file bytes - the DOL's own header layout."""
    if len(blob) < 0x100:
        return []
    text_off = struct.unpack_from(">7I", blob, 0x00)
    data_off = struct.unpack_from(">11I", blob, 0x1C)
    text_addr = struct.unpack_from(">7I", blob, 0x48)
    data_addr = struct.unpack_from(">11I", blob, 0x64)
    text_size = struct.unpack_from(">7I", blob, 0x90)
    data_size = struct.unpack_from(">11I", blob, 0xAC)
    out = [(text_addr[i], text_size[i], text_off[i]) for i in range(7) if text_size[i]]
    out += [(data_addr[i], data_size[i], data_off[i]) for i in range(11) if data_size[i]]
    return out


def dol_bytes(blob: bytes, sections: list[tuple[int, int, int]], address: int, length: int) -> bytes | None:
    """The DOL's file bytes at `address`, or None when the address is not backed by file bytes."""
    for start, size, offset in sections:
        if start <= address and address + length <= start + size:
            begin = offset + (address - start)
            return blob[begin:begin + length]
    return None


def effect_text(verdict: str, size: int, gain: int, stats: dict | None) -> str:
    """What the ledger's matched bytes should do - the number the batch is judged by."""
    if verdict in ("overlap", "unowned"):
        return "0 (refused)"
    if not stats:
        if verdict == "safe":
            return "+%d matched_data" % size
        return "0 gained (no measured unit)"
    unit = stats["report_unit"]
    matched, total = stats["matched_data"], stats["total_data"]
    after_total = total + size
    if verdict == "safe":
        return "+%d matched_data (%s: %d/%d -> %d/%d)" % (size, unit, matched, total,
                                                          matched + gain, after_total)
    return "0 gained (%s: matched_data %d/%d -> %d/%d; total_data +%d)" % (
        unit, matched, total, matched + gain, after_total, size)


def _finish(entry: dict, verdict: str, reason: str, affected: dict | None,
            size: int, gain: int, stats: dict | None) -> dict:
    entry["verdict"] = verdict
    # a run that contains a strong `.data` emission-order seam spans several TUs (docs/data-order-seams.md):
    # whatever else is true of it, it can never match as one unit. Warn - the verdict is unchanged.
    warn = dataseams.warning(entry["start"], entry["end"], entry.get("seams") or []) if entry.get("seams") else None
    if warn and verdict != "overlap":
        reason += "; WARNING: this run " + warn
    entry["seam_warning"] = warn
    entry["reason"] = reason
    entry["affected"] = affected
    entry["expected"] = effect_text(verdict, size, gain, stats)
    return entry


def classify(run: dict, claimed: list[dict], target: dict | None, ours: dict | None,
             symbol_at, stats: dict | None) -> dict:
    """One run's verdict. Pure: every input is already-read data, so the selftest needs no build tree.

    `target` is `{source, data (exactly the run's bytes, or None), size, nobits, dol_mismatch}`.
    `ours` is `{source, base, offset, size, data, nobits}` for our object's section, or None.
    """
    size = run["end"] - run["start"]
    entry = {
        "unit": run["unit"],
        "section": run["section"],
        "start": run["start"],
        "end": run["end"],
        "size": size,
        "queue_verdict": run.get("queue_verdict"),
        "leak": run.get("leak"),
        "density": run.get("density"),
        "claimed": nearest_claim(run, claimed),
        "target_source": target.get("source") if target else None,
        "our_source": ours.get("source") if ours else None,
        "target_relocs": len(target.get("reloc_addrs") or ()) if target else None,
        "our_relocs": len(ours.get("reloc_addrs") or ()) if ours else None,
        "target_hex": None,
        "our_hex": None,
        "equal_prefix": None,
        "seams": list(run.get("seams") or []),
    }

    hits = overlapping(run, claimed)
    if hits:
        hit = min(hits, key=lambda r: r["start"])
        return _finish(entry, "overlap",
                       "0x%X-0x%X intersects %s's claimed %s 0x%X-0x%X"
                       % (run["start"], run["end"], hit["unit"], hit["section"], hit["start"], hit["end"]),
                       {"unit": hit["unit"], "symbol": None, "function": None, "function_basis": None},
                       size, 0, stats)

    queue_verdict = run.get("queue_verdict")
    if queue_verdict in ("never", "owner-held", "not claimed"):
        detail = QUEUE_WHY.get(queue_verdict, "")
        if queue_verdict == "not claimed":
            detail += " (leak=%s, density=%s)" % (run.get("leak"), run.get("density"))
        return _finish(entry, "unowned",
                       "the queue refuses it (`%s`): %s" % (queue_verdict, detail),
                       None, size, 0, stats)

    if target is None:
        return _finish(entry, "unowned",
                       "no split object covers 0x%X in %s and the DOL has no bytes there"
                       % (run["start"], run["section"]), None, size, 0, stats)

    entry["target_hex"] = hex_prefix(target.get("data"))

    if target.get("dol_mismatch"):
        return _finish(entry, "lowers-score",
                       "the split object's bytes differ from the DOL for this range - the DOL already has "
                       "these bytes right", {"unit": run["unit"], "symbol": symbol_at(run["section"], run["start"]),
                                             "function": None}, size, 0, stats)

    if ours is None:
        if not run.get("registered"):
            return _finish(entry, "unowned",
                           "the queue names no registered owner (`%s`) - register the containing region first"
                           % run["unit"], None, size, 0, stats)
        if not run.get("our_object_exists"):
            return _finish(entry, "lowers-score",
                           "our object %s is not built, so nothing can emit this run"
                           % run.get("our_object"),
                           _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                           size, 0, stats)
        return _finish(entry, "lowers-score",
                       "our object %s emits no %s section, so the claim pairs a target section against "
                       "nothing" % (run.get("our_object"), run["section"]),
                       _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                       size, 0, stats)

    offset = ours["offset"]
    if offset < 0:
        return _finish(entry, "lowers-score",
                       "our %s section is placed at 0x%X, above this run, so it emits none of it"
                       % (run["section"], ours["base"]),
                       _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                       size, 0, stats)

    coverage = min(ours["size"] - offset, size)
    entry["our_hex"] = hex_prefix(ours["data"][offset:offset + size] if ours.get("data") else None)

    if target.get("nobits") or ours.get("nobits"):
        if coverage < size:
            return _finish(entry, "lowers-score",
                           "our object emits %d of %d zero-filled %s bytes"
                           % (max(coverage, 0), size, run["section"]),
                           _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                           size, 0, stats)
        return _finish(entry, "safe",
                       "our object emits the whole %d-byte zero-filled %s section" % (size, run["section"]),
                       None, size, size, stats)

    if not target.get("data") or not ours.get("data"):
        return _finish(entry, "lowers-score",
                       "the target has %d bytes here but our %s section is empty"
                       % (size, run["section"]),
                       _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                       size, 0, stats)

    mine = ours["data"][offset:offset + size]
    masked = masked_offsets(target.get("reloc_addrs"), run["start"], size)
    diff = first_byte_diff(target["data"], mine, masked)
    entry["equal_prefix"] = diff if diff is not None else len(mine)
    if diff is not None:
        first_diff = run["start"] + diff
        return _finish(entry, "lowers-score",
                       "byte %d differs at 0x%X (target %02X, ours %02X), symbol `%s`"
                       % (diff, first_diff, target["data"][diff], mine[diff],
                          symbol_at(run["section"], first_diff)),
                       _affected(run["unit"], symbol_at(run["section"], first_diff), stats),
                       size, 0, stats)

    if len(mine) < size:
        matched = "the first %d bytes match" % len(mine) if len(mine) else "none of it"
        return _finish(entry, "lowers-score",
                       "our object emits only %d of %d bytes (%s); the remaining %d pair against nothing"
                       % (len(mine), size, matched, size - len(mine)),
                       _affected(run["unit"], symbol_at(run["section"], run["start"] + len(mine)), stats),
                       size, 0, stats)

    target_relocs = set(target.get("reloc_addrs") or ())
    our_relocs = set(ours.get("reloc_addrs") or ())
    if target_relocs != our_relocs:
        return _finish(entry, "lowers-score",
                       "the bytes are equal but the relocation sites differ (target %d, ours %d)"
                       % (len(target_relocs), len(our_relocs)),
                       _affected(run["unit"], symbol_at(run["section"], run["start"]), stats),
                       size, 0, stats)

    return _finish(entry, "safe",
                   "our object emits all %d %s bytes identically" % (size, run["section"]),
                   None, size, size, stats)


def _worst(stats: dict | None) -> str | None:
    """The unit's worst-scoring function - playbook 23's regression was on a function, so name one."""
    if not stats or stats.get("worst_function") is None:
        return None
    return "%s (%.2f%%)" % (stats["worst_function"], stats["worst_percent"])


# The *exact* function a refusal affects is the one whose code references the run, which only `tudiscover`'s
# referrer graph knows - and that cache is stale today, so the unit's worst score stands in and says so.
FUNCTION_BASIS = "unit's worst score (referrer graph unavailable)"


def _affected(unit: str, symbol: str | None, stats: dict | None) -> dict:
    return {"unit": unit, "symbol": symbol, "function": _worst(stats), "function_basis": FUNCTION_BASIS}


def render(entries: list[dict]) -> str:
    """Sorted keys and a stable order, so the same repository state renders the same bytes."""
    return json.dumps(entries, indent=1, sort_keys=True) + "\n"


def write_atomic(path: str, text: str) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)) or ".", exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    os.replace(tmp, path)


def summary(entries: list[dict], queue_path: str) -> str:
    """The one screen an operator reads: the verdict counts, and where the risk is."""
    if not entries:
        return "no runs - the queue is empty or absent"
    verdicts: dict[str, int] = {}
    sections: dict[str, int] = {}
    for entry in entries:
        verdicts[entry["verdict"]] = verdicts.get(entry["verdict"], 0) + 1
        sections[entry["section"]] = sections.get(entry["section"], 0) + 1
    total = sum(entry.get("size", 0) for entry in entries)
    risky = [e for e in entries if e.get("verdict") != "safe"]
    risky.sort(key=lambda e: (VERDICTS.index(e.get("verdict", "unowned")), -e.get("size", 0), e["start"]))
    lines = ["%d run(s), %d bytes -> %s" % (len(entries), total,
                                            ", ".join("%s %d" % (v, verdicts.get(v, 0)) for v in VERDICTS)),
             "  sections: " + ", ".join("%s %d" % (s, n) for s, n in
                                        sorted(sections.items(), key=lambda kv: section_key(kv[0]))),
             "  claimable: %d safe; refused: %d lowers-score, %d overlap, %d unowned"
             % (verdicts.get("safe", 0), verdicts.get("lowers-score", 0), verdicts.get("overlap", 0),
                verdicts.get("unowned", 0)),
             "  -> " + queue_path]
    spanning = [e for e in entries if e.get("seam_warning")]
    if spanning:
        lines.insert(3, "  multi-TU: %d run(s) contain a strong .data emission-order seam (V->S gap/zigzag) and "
                        "span at least 2 TUs, so can never match as one unit (%d of them `safe`)"
                     % (len(spanning), sum(1 for e in spanning if e["verdict"] == "safe")))
    for entry in risky[:5]:
        lines.append("  %-8s %s 0x%08X-0x%08X (%d B, %s): %s"
                     % (entry.get("verdict"), entry.get("section"), entry["start"], entry["end"],
                        entry.get("size", 0), entry.get("unit"), entry.get("reason")))
    return "\n".join(lines)


# -- the impure edges: read the repository --------------------------------------------------------------------

def read_object_sections(path: str) -> dict | None:
    """`{section: {type, size, data}}` for one object; a NOBITS section carries no file bytes."""
    try:
        _, headers = elfsect.sections(path)
    except (OSError, AssertionError, struct.error, ValueError):
        return None
    out = {}
    for header in headers:
        name = header["name"]
        if not name:
            continue
        data = b"" if header["typ"] == 8 else header["data"]
        out[name] = {"type": header["typ"], "size": header["size"], "data": data}
    return out


def covering_object(objects, section: str, address: int) -> tuple[str | None, int | None]:
    """The split object covering an address, and its base address (config.json's ranges)."""
    path = objects.covering(section, address)
    if not path:
        return None, None
    for start, _end, candidate in objects.ranges.get(section, ()):
        if candidate == path:
            return path, start
    return path, None


def reloc_addrs(sections: dict | None, section: str, base: int, start: int, end: int) -> list[int]:
    """Absolute addresses of the relocations inside `[start, end)` of `section`.

    dtk splits relocations into `.rela.<section>` and MWCC emits the same shape, so both sides of the
    comparison carry them. A relocation site is the one place object bytes and DOL bytes legitimately
    differ (the object holds the addend, the linked DOL the resolved address).
    """
    header = (sections or {}).get(".rela" + section)
    if not header or not header.get("data"):
        return []
    out, data = [], header["data"]
    for i in range(0, len(data) - 11, 12):
        offset, = struct.unpack_from(">I", data, i)
        address = base + offset
        if start <= address < end:
            out.append(address)
    return sorted(out)


def unit_stats(report: dict, unit: str) -> dict | None:
    name = ledger_mod.report_name(unit)
    for record in report.get("units", ()):
        if record.get("name") != name:
            continue
        measures = record.get("measures") or {}
        functions = [(f.get("name"), f.get("fuzzy_match_percent")) for f in record.get("functions", ())]
        functions = [(n, p) for n, p in functions if isinstance(p, (int, float))]
        worst = min(functions, key=lambda pair: pair[1]) if functions else (None, None)
        return {"report_unit": name,
                "matched_data": int(measures.get("matched_data") or 0),
                "total_data": int(measures.get("total_data") or 0),
                "worst_function": worst[0], "worst_percent": worst[1]}
    return None


def symbol_lookup(by_section: dict):
    """`(section, address) -> the nearest symbol at or below the address` - the name a refusal affects."""
    index = {section: sorted(entries, key=lambda e: e["address"]) for section, entries in by_section.items()}

    def at(section: str, address: int) -> str | None:
        entries = index.get(section)
        if not entries:
            return None
        lo, hi = 0, len(entries)
        while lo < hi:
            mid = (lo + hi) // 2
            if entries[mid]["address"] <= address:
                lo = mid + 1
            else:
                hi = mid
        return entries[lo - 1]["name"] if lo else None

    return at


def load_queue(path: str) -> list[dict]:
    """`dataqueue.py` writes a bare list; `brief.py` also accepts `{"entries": [...]}` - take both."""
    if not os.path.exists(path):
        return []
    data = json.loads(open(path, encoding="utf-8").read())
    return list(data if isinstance(data, list) else data.get("entries", []))


def analyze(root: str = ROOT, queue_path: str | None = None) -> list[dict]:
    """Classify every proposed run in the queue against the repository as it stands now."""
    queue_path = queue_path or os.path.join(root, QUEUE_REL)
    runs = load_queue(queue_path)

    splits = preflight.load_splits()
    claimed = [{"unit": block["unit"], "section": rng["section"], "start": rng["start"], "end": rng["end"]}
               for block in splits for rng in block["ranges"]]
    unit_ranges: dict[str, dict[str, list[tuple[int, int]]]] = {}
    for block in splits:
        bare = os.path.splitext(block["unit"])[0]
        for rng in block["ranges"]:
            unit_ranges.setdefault(bare, {}).setdefault(rng["section"], []).append((rng["start"], rng["end"]))

    by_section = preflight.load_symbols()[1]
    symbol_at = symbol_lookup(by_section)
    objects = ledger_mod.Objects()
    report = ledger_mod.read_json(os.path.join(root, REPORT_REL)) or {}

    dol_path = os.path.join(root, DOL_REL)
    dol = open(dol_path, "rb").read() if os.path.exists(dol_path) else b""
    dol_map = dol_sections(dol)

    target_cache: dict[str, dict | None] = {}
    strong_seams = dataseams.load_strong()

    def sections_of(rel_path: str) -> dict | None:
        if rel_path not in target_cache:
            target_cache[rel_path] = read_object_sections(os.path.join(root, rel_path))
        return target_cache[rel_path]

    entries = []
    for run in runs:
        unit = run.get("unit") or ""
        section, start, end = run.get("section") or "", int(run.get("start") or 0), int(run.get("end") or 0)
        size = end - start
        bare = os.path.splitext(unit)[0]
        registered = bare in unit_ranges
        our_rel = os.path.join(SRC_DIR, bare + ".o")

        enriched = {"unit": unit, "section": section, "start": start, "end": end,
                    "queue_verdict": run.get("verdict"), "leak": run.get("leak"),
                    "density": run.get("density"), "registered": registered,
                    "our_object": our_rel, "our_object_exists": os.path.exists(os.path.join(root, our_rel)),
                    "seams": dataseams.seams_in(strong_seams, start, end) if section == ".data" else []}

        # 2. the target's bytes: the covering split object, cross-checked against the DOL
        obj_path, obj_base = covering_object(objects, section, start)
        target = None
        if obj_path is not None and obj_base is not None:
            rel_obj = os.path.relpath(obj_path, root)
            sections = sections_of(rel_obj)
            header = (sections or {}).get(section)
            offset = start - obj_base
            if header is not None and 0 <= offset and offset + size <= header["size"]:
                if header["type"] == 8 or section in NOBITS_SECTIONS:
                    target = {"source": rel_obj, "data": None, "size": size,
                              "nobits": True, "dol_mismatch": False, "reloc_addrs": []}
                else:
                    data = header["data"][offset:offset + size]
                    relocs = reloc_addrs(sections, section, obj_base, start, end)
                    mismatch = False
                    expected = dol_bytes(dol, dol_map, start, size) if dol else None
                    if expected is not None:
                        masked = masked_offsets(relocs, start, size)
                        mismatch = first_byte_diff(data, expected, masked) is not None
                    target = {"source": rel_obj, "data": data, "size": size, "nobits": False,
                              "dol_mismatch": mismatch, "reloc_addrs": relocs}
        if target is None and section in NOBITS_SECTIONS:
            # A zero-filled section has no file bytes anywhere: its target is its size alone.
            target = {"source": "size-only (NOBITS)", "data": None, "size": size, "nobits": True,
                      "dol_mismatch": False, "reloc_addrs": []}
        if target is None:
            expected = dol_bytes(dol, dol_map, start, size) if dol else None
            if expected is not None:
                target = {"source": DOL_REL, "data": expected, "size": size, "nobits": False,
                          "dol_mismatch": False, "reloc_addrs": []}

        # 3. what our source emits
        ours = None
        ranges = unit_ranges.get(bare, {}).get(section)
        our_base = min(r[0] for r in ranges) if ranges else start
        if registered and enriched["our_object_exists"]:
            our_sections = sections_of(our_rel)
            header = (our_sections or {}).get(section)
            if header is not None:
                ours = {"source": our_rel, "base": our_base, "offset": start - our_base,
                        "size": header["size"], "data": header["data"],
                        "nobits": header["type"] == 8 or section in NOBITS_SECTIONS,
                        "reloc_addrs": reloc_addrs(our_sections, section, our_base, start, end)}

        entries.append(classify(enriched, claimed, target, ours, symbol_at, unit_stats(report, unit)))

    return sort_entries(entries)


# ==================================================================================================
# rule 12: the data a unit references but does not own, and the one-line claim that fixes it
# ==================================================================================================
#
# `docs/plan.md` 6.5 rule 12 refuses an `extern` of data no registered `splits.txt` range covers, and
# unlike rule 7 there is no rename remedy - the only remedies are ownership changes, which lanes were
# deriving by hand. This mode lists, for one unit, every data symbol its object references and does not
# own, with the census (who else reads it) and the exact `splits.txt` text to paste. Three shapes:
#
#   1. ordinary     the range is free: claim it into the unit being worked. A claim covers the WHOLE
#                   map symbol extent and its `end:` is 4-aligned (a partial `.sdata`/`.sdata2` claim
#                   cannot be linked).
#   2. span         the unit already owns a run of that section: claim the contiguous span, gap
#                   included, or dtk inserts an `auto_*_data` unit inside the range and the split dies
#                   with a link-order cycle (playbook 53).
#   3. named owner  a pool/table several units read: register a named data-only unit - a splits.txt
#                   range, a source file that defines nothing, and (optional) a symbols.txt name. For a
#                   `NonMatching` unit the original bytes stay in the DOL (the binary is untouched)
#                   while the range gains an owner, and the consumers declare into its header (rule 2).
#                   Strictly better than the anonymous `auto_XX_data` unit dtk would otherwise create;
#                   one owner per pool. The `symbols.txt:` row is OPTIONAL and OVERLAPPING: the pool's
#                   words are already named and own those addresses, so one object symbol over the
#                   span would sit on top of them. The accepted `Pl/pl_frame_data` precedent (and its
#                   `Pl/pl_act_data` follow-on) registered the `splits.txt` range alone and the link
#                   worked; the output says so instead of leaving a lane to decide.
#
# Read-only by design: it never writes `splits.txt` (or anything else). `--dry-run` is accepted and only
# asserts that; the reference listing is the same either way.

CLAIM_ALIGN = 4
DATA_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")
POOL_EXPAND_LIMIT = 512          # symbols around the seed we are willing to walk (a sanity cap)
POOL_SPAN_LIMIT = 0x4000         # bytes: a pool this wide is a different question (the auto run)
SHARERS_SHOWN = 12
REMEDIES = ("claim-into-unit", "span-claim", "named-owner-unit", "owner-header")


def align_up(value: int, alignment: int = CLAIM_ALIGN) -> int:
    return (value + alignment - 1) // alignment * alignment


def section_stem(section: str) -> str:
    return section.lstrip(".").replace(".", "_")


def extent_of(entry: dict, following: dict | None) -> tuple[int, str]:
    """`(extent, source)` for one map row: its own `size:`, else the distance to the next row.

    `symbols.txt` rows carry `size:` for real data (a float is 4, a table its length); a label row may
    not, and then the row's extent is the distance to the next row in the section. `default` is the
    last resort and is named so a reader knows it was a floor, never a measurement.
    """
    size = int(entry.get("size") or 0)
    if size > 0:
        return size, "map"
    if following is not None and following["address"] > entry["address"]:
        return following["address"] - entry["address"], "next"
    return CLAIM_ALIGN, "default"


def claim_span(address: int, extent: int) -> tuple[int, int, bool]:
    """The `(start, end, rounded)` a `splits.txt` claim needs: start at the row, `end:` 4-aligned."""
    end = align_up(address + extent)
    return address, end, end != address + extent


def splits_range_line(section: str, start: int, end: int, indent: str = "\t") -> str:
    return "%s%s start:0x%08X end:0x%08X" % (indent, section, start, end)


def splits_unit_block(unit: str, section: str, start: int, end: int) -> str:
    return "%s.cpp:\n%s" % (unit, splits_range_line(section, start, end))


def symbols_row(name: str, section: str, address: int, size: int, type_: str = "object") -> str:
    return "%s = %s:0x%08X; // type:%s size:0x%X" % (name, section, address, type_, size)


def _adjacent(prev: dict, nxt: dict) -> bool:
    """Whether two map rows touch (the previous row's declared extent reaches the next row)."""
    return prev["address"] + int(prev.get("size") or 0) == nxt["address"]


def pool_components(rows: list[dict], readers_of) -> list[dict]:
    """Partition the section's unowned rows into reader-connected, address-contiguous pools.

    One left-to-right pass: a row joins the current pool while it touches the previous row and its
    readers overlap the pool's (an unread row always joins - an unnamed blob between two unowned
    constants is still the same pool). A reader-disjoint neighbour starts a new pool, which is what
    separates the `Pl` pool from the `enemy` pool inside one enormous unclaimed run. The partition
    does not depend on which symbol asked, so every symbol of one pool is recommended the same owner.
    Two caps bound a pathological walk (`POOL_EXPAND_LIMIT` symbols, `POOL_SPAN_LIMIT` bytes).
    """
    comps: list[dict] = []
    cur = None
    for row in rows:
        extra = set(readers_of(row["address"]))
        joins = (cur is not None and _adjacent(cur["last"], row)
                 and (not cur["readers"] or not extra or bool(extra & cur["readers"]))
                 and cur["count"] < POOL_EXPAND_LIMIT
                 and row["address"] - cur["lo"]["address"] <= POOL_SPAN_LIMIT)
        if joins:
            cur["rows"].append(row)
            cur["readers"] |= extra
            cur["count"] += 1
            cur["last"] = row
            cur["hi"] = row
        else:
            cur = {"lo": row, "hi": row, "last": row, "readers": set(extra),
                   "count": 1, "rows": [row]}
            comps.append(cur)
    return comps


def section_pools(rows: list[dict], readers_of) -> dict[int, dict]:
    """`{row address: pool}` for every unowned row of one section (one partition, reused)."""
    out: dict[int, dict] = {}
    for comp in pool_components(rows, readers_of):
        pool = {"start": comp["lo"]["address"],
                "end": comp["hi"]["address"] + int(comp["hi"].get("size") or CLAIM_ALIGN),
                "count": comp["count"], "readers": comp["readers"]}
        for row in comp["rows"]:
            out[row["address"]] = pool
    return out


def pool_around(entry: dict, rows: list[dict], readers_of) -> dict:
    """The reader-connected pool containing `entry`, or a one-symbol pool when the row is not there."""
    pool = section_pools(rows, readers_of).get(entry["address"])
    if pool is not None:
        return pool
    return {"start": entry["address"], "end": entry["address"] + CLAIM_ALIGN,
            "count": 1, "readers": set(readers_of(entry["address"]))}


def recommend(rec: dict, unit: str, unit_ranges: list[tuple[int, int]]) -> dict:
    """The remedy for one referenced-but-not-owned symbol, and the text to paste.

    `rec` carries `name/section/address/extent/owner/sharers/pool`. `unit_ranges` is the unit's own
    registered `(start, end)` runs in this section, so a second run (playbook 53) is visible.
    """
    section, address, extent = rec["section"], rec["address"], rec["extent"]
    start, end, rounded = claim_span(address, extent)
    round_note = ("the map extent ends at 0x%X (not 4-aligned): the claim's `end:` rounds up to 0x%X"
                  % (address + extent, end)) if rounded else ""
    owner = rec.get("owner")
    if owner:
        return {"remedy": "owner-header", "claim_start": None, "claim_end": None,
                "splits": None, "symbols": None,
                "note": "`%s` is owned by `%s`: declare it in that unit's header and #include it "
                        "(rule 2) - rule 12 does not fire and there is nothing to claim"
                        % (rec["name"], owner)}
    sharers = rec.get("sharers") or []
    if sharers:
        module = unit.split("/", 1)[0] if "/" in unit else unit
        pool = rec["pool"]
        pool_name = "%s/%s_pool" % (module or section_stem(section), section_stem(section))
        pstart, pend = pool["start"], align_up(pool["end"])
        # The pool's rows already OWN these addresses: a unit-level `symbols.txt` row would place ONE
        # object symbol over the whole span, on top of the word rows that are already named there. The
        # accepted `Pl/pl_frame_data` registration (408 B, and its `Pl/pl_act_data` follow-on) added the
        # `splits.txt` range and NO row, and the split linked - so the row is optional, and a lane must
        # not spend time deciding whether it is required. `overlap` is how many named rows it would
        # overlap.
        overlap = int(pool.get("count") or 0)
        note = ("the pool (%d unowned symbol(s), 0x%X-0x%X) is read by %d unit(s) besides `%s` (%s): "
                "register a named data-only unit owning the whole pool - a `splits.txt` range, a "
                "source file that defines nothing, and (optional, see below) a `symbols.txt` name. A "
                "`NonMatching` unit keeps the original bytes in the binary (the DOL is untouched) and "
                "the consumers declare into its header (rule 2). One owner per pool: a partial "
                "`.sdata2` claim cannot be linked."
                % (pool["count"], pool["start"], pool["end"], len(sharers), unit,
                   ", ".join(sharers[:SHARERS_SHOWN])))
        note += (" The `symbols.txt:` row is OPTIONAL and OVERLAPPING: the pool's %d row(s) at "
                 "0x%X-0x%X already own those addresses, so one object symbol would span them - the "
                 "accepted `Pl/pl_frame_data` precedent registered the unit with the `splits.txt` "
                 "range alone; omit it when the words are already named."
                 % (overlap, pool["start"], pool["end"]))
        return {"remedy": "named-owner-unit", "claim_start": pstart, "claim_end": pend,
                "splits": splits_unit_block(pool_name, section, pstart, pend),
                "symbols": symbols_row(pool_name, section, pstart, pend - pstart),
                "symbols_optional": True, "symbols_overlap_rows": overlap, "note": note}
    below = [r for r in unit_ranges if r[1] <= address]
    if below:
        rs, re_ = max(below, key=lambda r: r[1])
        s, e = min(rs, start), align_up(max(re_, end))
        note = ("`%s` already owns %s 0x%X-0x%X and this run is separate from it: claim the "
                "contiguous span, gap included, or dtk inserts an `auto_*_data` unit inside the "
                "range and `dtk dol split` dies with a link-order cycle (playbook 53)"
                % (unit, section, rs, re_))
        return {"remedy": "span-claim", "claim_start": s, "claim_end": e,
                "splits": splits_range_line(section, s, e), "symbols": None,
                "note": (round_note + "; " + note) if rounded else note}
    note = "the range is free: claim it into `%s`, covering the whole map symbol extent" % unit
    return {"remedy": "claim-into-unit", "claim_start": start, "claim_end": end,
            "splits": splits_range_line(section, start, end), "symbols": None,
            "note": (round_note + "; " + note) if rounded else note}


def norm_reader(label: str | None) -> str | None:
    """A census owner label -> a unit name, or None for the labels that are not units.

    The one implementation is `callers.norm_reader` (the tool that builds the census); this re-export is
    what `dataclaim`'s sharer lists and its selftest read through, so the two tools cannot disagree
    about who a label names.
    """
    try:
        from units import callers as callers_mod  # noqa: PLC0415
    except ImportError:                           # pragma: no cover - direct script execution
        import callers as callers_mod             # type: ignore
    return callers_mod.norm_reader(label)


def declaration_locations(root: str, names) -> dict[str, list[str]]:
    """`{name: [repo-relative declaration paths]}` for `names`, read from `include/` and `src/`.

    A declaration is a site `stylelint.declaration_sites` reads (the `extern` keyword or a plain function
    prototype) - the same reading rules 2 and 12 use. The remedy is derived from the address's *owner*;
    this says where the declaration actually sits, so an `owner-header` remedy beside `declared in:
    include/unsplit/<band>.h` shows the gap in one line. An empty list renders as `not declared`.
    """
    from units import stylelint as sl  # noqa: PLC0415 - the one declaration reader
    want = set(names)
    out: dict[str, list[str]] = {n: [] for n in want}
    if not want:
        return out
    for top in (sl.HEADERS, sl.SRC):
        base = os.path.join(root, top)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
            for fn in sorted(filenames):
                if not fn.endswith(sl.SUFFIXES):
                    continue
                path = os.path.join(dirpath, fn)
                try:
                    text = sl.read_text(path)
                except OSError:
                    continue
                if not any(name in text for name in want):    # a cheap pre-filter before the parse
                    continue
                rel = os.path.relpath(path, root).replace(os.sep, "/")
                for name, _pos, _line in sl.declaration_sites(sl.Source(path, rel, text)):
                    if name in want and rel not in out[name]:
                        out[name].append(rel)
    return out


def reference_record(unit: str, entry: dict, extent: int, extent_source: str, owner: str | None,
                     pool: dict | None, unit_ranges: list[tuple[int, int]], obj_rel: str | None,
                     declarations: dict | None = None, seams: list[dict] | None = None) -> dict:
    """One referenced symbol's row: the facts plus `recommend`'s decision, JSON-safe throughout.

    `seams` (strong `.data` emission-order seams) is checked against the claim `recommend` proposes: a claim that
    contains one spans several TUs, and the record says so (`seam_warning`, also appended to `note`).
    """
    start, end, rounded = claim_span(entry["address"], extent)
    sharers = sorted(set(pool["readers"]) - {unit}) if pool else []
    rec = {
        "name": entry["name"], "section": entry["section"], "address": entry["address"],
        "extent": extent, "extent_source": extent_source,
        "claim_start": start, "claim_end": end, "not_4_aligned": rounded,
        "owner": owner, "declared_in": list((declarations or {}).get(entry["name"], [])),
        "object": obj_rel, "shared": bool(sharers), "sharers": sharers,
        "pool": ({"start": pool["start"], "end": pool["end"], "count": pool["count"],
                   "readers": sorted(pool["readers"])} if pool else None),
        "remedy": None, "splits": None, "symbols": None, "note": "",
    }
    rec.update(recommend(rec, unit, unit_ranges))
    rec["seam_warning"] = None
    if seams and rec["section"] == ".data" and rec.get("claim_start") is not None:
        warn = dataseams.warning(rec["claim_start"], rec["claim_end"], seams)
        if warn:
            rec["seam_warning"] = "the proposed claim " + warn
            rec["note"] = (rec["note"] + "; WARNING: " + rec["seam_warning"]) if rec["note"] else rec["seam_warning"]
    return rec


def _remedy_counts(records: list[dict]) -> dict:
    counts = {key: 0 for key in REMEDIES}
    for rec in records:
        counts[rec["remedy"]] = counts.get(rec["remedy"], 0) + 1
    return counts


def _next_row(rows: list[dict], address: int) -> dict | None:
    for row in rows:
        if row["address"] > address:
            return row
    return None


def unit_ranges(splits: list[dict], unit: str) -> dict[str, list[tuple[int, int]]]:
    """The unit's own registered `(start, end)` per section, keyed by the split's bare unit name."""
    out: dict[str, list[tuple[int, int]]] = {}
    for block in splits:
        if os.path.splitext(block["unit"])[0] != unit:
            continue
        for rng in block["ranges"]:
            out.setdefault(rng["section"], []).append((rng["start"], rng["end"]))
    return out


def unit_data_references(root: str, unit: str) -> tuple[list[str], str | None]:
    """`(names, object)` for the data symbols the unit's object references. The target object first
    (what retail's TU needs), its source object as the fallback (a unit not split yet)."""
    rel = os.path.splitext(unit)[0] + ".o"
    try:
        from units import undefrefs as uref  # noqa: PLC0415 - one ELF relocation reader
    except ImportError:                       # pragma: no cover - direct script execution
        import undefrefs as uref              # type: ignore
    for base in (OBJ_DIR, SRC_DIR):
        path = os.path.join(root, base, rel)
        if not os.path.exists(path):
            continue
        loaded = uref.load_object(path)
        if loaded is not None:
            names = sorted(set(loaded["refs"]) - set(loaded.get("defined") or {}))
            return names, os.path.join(base, rel).replace("\\", "/")
    return [], None


def census_readers(root: str):
    """`(readers_of, info)` from `callers.py`'s census - reused, never a second graph.

    `readers_of(address) -> {unit: sites}` normalises the census's owner labels to unit names so a
    sharer list joins the unit vocabulary everything else uses. The census itself is `callers.py`'s
    (the asm dump when present, the split objects' relocations otherwise); this only reads it.
    """
    try:
        from units import callers as callers_mod  # noqa: PLC0415
    except ImportError:                           # pragma: no cover - direct script execution
        import callers as callers_mod             # type: ignore
    cmap = callers_mod.load_map(root)
    asm_dir = callers_mod.asm_dir_of(root)
    files = callers_mod.all_asm_files(asm_dir)
    if files:
        index, info = callers_mod.load_index(root=root, asm_dir=asm_dir)
        source = "asm"
    else:
        index, info = callers_mod.load_elf_index(root=root, cmap=cmap)
        source = "elf"
    if index is None:
        return None, {"source": source, "state": "missing", "reason": info.get("reason")}
    # One census, one implementation: `callers.readers_of` wraps the query this tool would otherwise
    # rebuild, so the sharer list here and `callers.py --range`'s runs are the same reader sets.
    return callers_mod.readers_of(index, cmap), {"source": source, "state": "present",
                                                 "files": info.get("files"),
                                                 "cached": info.get("cached"),
                                                 "reason": info.get("reason")}


def reference_report(root: str, unit: str) -> dict:
    """Classify every data symbol `unit` references and does not own, with a remedy per symbol."""
    from units import stylelint as sl  # noqa: PLC0415 - the one ownership index
    ownership = sl.load_ownership(root)
    if ownership is None:
        raise RuntimeError("no config/RMHE08/symbols.txt or splits.txt - nothing to decide against")
    readers_of, census_info = census_readers(root)
    if readers_of is None:
        readers_of = lambda _address: {}  # noqa: E731 - sharing is unchecked, the claim still is
    by_name, by_section, _by_address = preflight.load_symbols()
    splits = preflight.load_splits()
    ranges = unit_ranges(splits, unit)
    ref_names, obj_rel = unit_data_references(root, unit)
    unowned_cache: dict[str, list[dict]] = {}
    declarations = declaration_locations(root, ref_names)
    records, seen = [], set()
    for name in ref_names:
        entry = by_name.get(name)
        if entry is None:
            continue
        section = entry["section"]
        if section not in DATA_SECTIONS or entry.get("type") == "function":
            continue
        resolved = ownership.resolve(name)
        if resolved is None or resolved["kind"] == "dup":
            continue
        if resolved["kind"] == "owned" and resolved["unit"] == unit:
            continue
        key = (section, entry["address"])
        if key in seen:
            continue
        seen.add(key)
        rows = by_section.get(section, [])
        extent, extent_source = extent_of(entry, _next_row(rows, entry["address"]))
        owner = resolved["unit"] if resolved["kind"] == "owned" else None
        pool = None
        if owner is None:
            if section not in unowned_cache:
                unowned_cache[section] = section_pools(
                    [row for row in rows if _is_unsplit(ownership, row["name"])], readers_of)
            pool = unowned_cache[section].get(entry["address"])
        records.append(reference_record(unit, entry, extent, extent_source, owner, pool,
                                        ranges.get(section, []), obj_rel, declarations,
                                        dataseams.load_strong()))
    records.sort(key=lambda x: (section_key(x["section"]), x["address"]))
    return {"unit": unit, "object": obj_rel, "census": census_info, "entries": records,
            "summary": _remedy_counts(records)}


def _is_unsplit(ownership, name: str) -> bool:
    resolved = ownership.resolve(name)
    return resolved is not None and resolved["kind"] == "unsplit"


def render_references(payload: dict) -> str:
    """The one screen an operator reads: every referenced-but-not-owned symbol, its sharers, its fix."""
    unit, records = payload["unit"], payload["entries"]
    lines = ["unit %s -> %d referenced data symbol(s) it does not own  (object %s)"
             % (unit, len(records), payload.get("object") or "-")]
    for rec in records:
        pool = rec.get("pool")
        shared = ("shared by " + ", ".join(rec["sharers"])) if rec["sharers"] else \
            "private to this unit"
        lines.append("  %-8s 0x%08X  %-28s extent 0x%-5X owner %s  %s"
                     % (rec["section"], rec["address"], rec["name"], rec["extent"],
                        rec["owner"] or "unowned", shared))
        if rec["remedy"] == "named-owner-unit" and pool:
            lines.append("      pool 0x%08X-0x%08X (%d symbol(s), %d reader(s))"
                         % (pool["start"], pool["end"], pool["count"], len(pool["readers"])))
        lines.append("      remedy: %s" % rec["remedy"])
        lines.append("      declared in: %s" % (", ".join(rec.get("declared_in") or [])
                                                   or "not declared"))
        if rec.get("splits"):
            lines.append("      splits.txt: %s" % rec["splits"].replace("\n", "\n                 "))
        if rec.get("symbols"):
            lines.append("      symbols.txt: %s" % rec["symbols"])
            if rec.get("symbols_optional"):
                lines.append("                   OPTIONAL and OVERLAPPING - the pool's %d row(s) at "
                             "0x%08X-0x%08X already own those addresses; the `Pl/pl_frame_data` "
                             "precedent registered with the `splits.txt` range alone - omit this "
                             "line when the words are already named"
                             % (rec.get("symbols_overlap_rows") or 0,
                                (pool or {}).get("start"), (pool or {}).get("end")))
        lines.append("      %s" % rec["note"])
    lines.append("  summary: " + ", ".join("%s %d" % (key, payload["summary"].get(key, 0))
                                            for key in REMEDIES))
    return "\n".join(lines)


def reference_cli(unit: str, as_json: bool = False, dry_run: bool = False) -> int:
    unit = os.path.splitext(unit.replace("\\", "/"))[0]
    if dry_run:
        print("dataclaim: --dry-run - read-only; `splits.txt` is never written", file=sys.stderr)
    try:
        payload = reference_report(ROOT, unit)
    except RuntimeError as exc:
        print("dataclaim: %s" % exc, file=sys.stderr)
        return 2
    if payload.get("object") is None:
        print("dataclaim: no built object for `%s` (build/RMHE08/obj/<unit>.o or src/<unit>.o)"
              % unit, file=sys.stderr)
        return 2
    plan_info = None
    try:
        from units import datagap as datagap_mod  # noqa: PLC0415 - the strict row's own reading
        plan_info = datagap_mod.unit_plan(ROOT, unit)
    except Exception as exc:                        # noqa: BLE001 - the plan is additive; the references stand alone
        print("dataclaim: strict claim plan unavailable (%s)" % exc, file=sys.stderr)
    if as_json:
        if plan_info is not None:
            plan = plan_info["plan"]
            payload = dict(payload, strict_plan={
                "found": plan["found"], "header": plan["header"], "result": plan["result"],
                "edits": plan["edits"],
                "deferred": [{k: v for k, v in b.items() if k != "pairs"} | {"pairs": len(b["pairs"])}
                             for b in plan["deferred"]]})
        sys.stdout.write(json.dumps(payload, indent=1, sort_keys=True) + "\n")
    else:
        sys.stdout.write(render_references(payload) + "\n")
        if plan_info is not None:
            sys.stdout.write("\n" + datagap_mod.render_strict(
                {"blocks": [b for b in plan_info["report"]["blocks"] if b["unit"] == unit],
                 "stats": plan_info["report"]["stats"]}) + "\n\n")
            sys.stdout.write(datagap_mod.render_plan(plan_info["plan"], unit) + "\n")
    return 0


# -- selftest -------------------------------------------------------------------------------------------------

def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    claimed = [
        {"unit": "Pl/pl_act", "section": ".sdata2", "start": 0x8000, "end": 0x8010},
        {"unit": "RSO/runtime", "section": ".data", "start": 0x9000, "end": 0x9008},
    ]
    names = {0x1000: "gAct", 0x1004: "gAct2", 0x1008: "gAct3", 0x1010: "gRso"}

    def symbol_at(section, address):
        best = None
        for addr, name in names.items():
            if addr <= address:
                best = name
        return best

    def run(**kw):
        base = {"unit": "Pl/pl_act", "section": ".data", "start": 0x1000, "end": 0x1008,
                "queue_verdict": "proposed", "registered": True, "our_object": "build/RMHE08/src/Pl/pl_act.o",
                "our_object_exists": True}
        base.update(kw)
        return base

    def target(data, source="build/RMHE08/obj/auto_07_00001000_data.o", nobits=False, mismatch=False,
               reloc_addrs=()):
        return {"source": source, "data": data, "size": len(data) if data is not None else 8,
                "nobits": nobits, "dol_mismatch": mismatch, "reloc_addrs": list(reloc_addrs)}

    def ours(base, data, size=None, nobits=False, at=0x1000, reloc_addrs=(),
             source="build/RMHE08/src/Pl/pl_act.o"):
        return {"source": source, "base": base, "offset": at - base,
                "size": size if size is not None else len(data or b""), "data": data, "nobits": nobits,
                "reloc_addrs": list(reloc_addrs)}

    stats = {"report_unit": "main/Pl/pl_act", "matched_data": 40, "total_data": 1300,
             "worst_function": "fn_1", "worst_percent": 93.5}

    # --- the range checks
    check("touching ranges do not overlap", intersects(0, 8, 8, 16), False)
    check("a straddling range overlaps", intersects(4, 12, 8, 16), True)
    check("a contained range overlaps", intersects(9, 10, 8, 16), True)

    overlap = classify(run(section=".data", start=0x9004, end=0x900C), claimed,
                       target(b"\x00" * 8), ours(0x9004, b"\x00" * 8), symbol_at, stats)
    check("an overlap is refused", overlap["verdict"], "overlap")
    check("an overlap names the owning unit", overlap["affected"]["unit"], "RSO/runtime")
    check("an overlap expects nothing", overlap["expected"], "0 (refused)")
    check("an overlap says what it intersects", "intersects RSO/runtime's claimed .data 0x9000-0x9008" in overlap["reason"], True)
    adjacent = classify(run(section=".data", start=0x8FF8, end=0x9000), claimed,
                        target(b"\x00" * 8), ours(0x8FF8, b"\x00" * 8, at=0x8FF8), symbol_at, stats)
    check("an adjacent run is not an overlap", adjacent["verdict"], "safe")

    # --- safe
    good = classify(run(), claimed, target(b"ABCDEFGH"), ours(0x1000, b"ABCDEFGH"), symbol_at, stats)
    check("equal bytes are safe", good["verdict"], "safe")
    check("safe reports the whole run as equal", good["equal_prefix"], 8)
    check("safe expects +size matched_data", good["expected"].startswith("+8 matched_data"), True)
    check("safe quotes the unit's numbers", "40/1300 -> 48/1308" in good["expected"], True)
    check("safe affects nothing", good["affected"], None)
    check("safe carries the target bytes", good["target_hex"], b"ABCDEFGH".hex())
    check("safe carries our bytes", good["our_hex"], b"ABCDEFGH".hex())

    # --- lowers-score: a differing byte
    diff = classify(run(), claimed, target(b"ABCDXFGH"), ours(0x1000, b"ABCDEFGH"), symbol_at, stats)
    check("a differing byte lowers the score", diff["verdict"], "lowers-score")
    check("the first differing byte is recorded", diff["equal_prefix"], 4)
    check("the difference names the symbol at the offset", diff["affected"]["symbol"], "gAct2")
    check("the reason quotes both bytes", "(target 58, ours 45)" in diff["reason"], True)
    check("a differing byte names the worst function", diff["affected"]["function"], "fn_1 (93.50%)")
    check("the function's basis is recorded", diff["affected"]["function_basis"], FUNCTION_BASIS)
    check("lowers-score expects no gain", diff["expected"].startswith("0 gained"), True)

    # --- lowers-score: our object emits nothing / too little
    none_reg = classify(run(), claimed, target(b"ABCDEFGH"), None, symbol_at, stats)
    check("a registered unit with no section lowers the score", none_reg["verdict"], "lowers-score")
    check("the reason says a section pairs against nothing", "pairs a target section against nothing" in none_reg["reason"], True)
    not_built = classify(run(our_object_exists=False), claimed, target(b"ABCDEFGH"), None, symbol_at, stats)
    check("an unbuilt object lowers the score", not_built["verdict"], "lowers-score")
    check("the reason names the missing object", "is not built" in not_built["reason"], True)
    short = classify(run(), claimed, target(b"ABCDEFGH"), ours(0x1000, b"ABCD", size=4), symbol_at, stats)
    check("a short section lowers the score", short["verdict"], "lowers-score")
    check("the reason says how many bytes pair", "only 4 of 8 bytes" in short["reason"], True)
    below = classify(run(start=0x2000, end=0x2008), claimed, target(b"ABCDEFGH"),
                     ours(0x3000, b"ABCDEFGH"), symbol_at, stats)
    check("a section placed above the run lowers the score", below["verdict"], "lowers-score")
    check("the reason says it is above the run", "above this run" in below["reason"], True)

    # --- unowned
    unreg = classify(run(unit="auto/00001000_data", registered=False, our_object_exists=False), claimed,
                     target(b"ABCDEFGH"), None, symbol_at, stats)
    check("an auto placeholder is unowned", unreg["verdict"], "unowned")
    check("the reason says to register the region", "register the containing region first" in unreg["reason"], True)
    no_target = classify(run(), claimed, None, ours(0x1000, b"ABCDEFGH"), symbol_at, stats)
    check("no target bytes are unowned", no_target["verdict"], "unowned")
    check("the reason says the DOL has no bytes", "the DOL has no bytes there" in no_target["reason"], True)
    for queue_verdict in ("never", "owner-held", "not claimed"):
        refused = classify(run(queue_verdict=queue_verdict), claimed, target(b"ABCDEFGH"),
                           ours(0x1000, b"ABCDEFGH"), symbol_at, stats)
        check("the queue's `%s` is unowned" % queue_verdict, refused["verdict"], "unowned")
        check("the queue's `%s` says why" % queue_verdict, ("`%s`" % queue_verdict) in refused["reason"], True)

    # --- a target that disagrees with the DOL
    mismatch = classify(run(), claimed, target(b"ABCDEFGH", mismatch=True), ours(0x1000, b"ABCDEFGH"),
                        symbol_at, stats)
    check("a target disagreeing with the DOL is refused", mismatch["verdict"], "lowers-score")
    check("the reason says the DOL already has it right", "the DOL already has these bytes right" in mismatch["reason"], True)

    # --- NOBITS: `.bss` has sizes, not bytes
    bss_safe = classify(run(section=".bss", start=0x1000, end=0x1008), claimed,
                        target(None, nobits=True), ours(0x1000, b"", size=8, nobits=True), symbol_at, stats)
    check("a covered zero-filled section is safe", bss_safe["verdict"], "safe")
    check("a zero-filled safe run expects +size", bss_safe["expected"].startswith("+8 matched_data"), True)
    bss_short = classify(run(section=".bss", start=0x1000, end=0x1008), claimed,
                         target(None, nobits=True), ours(0x1000, b"", size=4, nobits=True), symbol_at, stats)
    check("a short zero-filled section lowers the score", bss_short["verdict"], "lowers-score")
    check("the reason counts the emitted bytes", "emits 4 of 8 zero-filled .bss bytes" in bss_short["reason"], True)
    bss_none = classify(run(section=".bss", start=0x1000, end=0x1008), claimed,
                        target(None, nobits=True), None, symbol_at, stats)
    check("a missing zero-filled section lowers the score", bss_none["verdict"], "lowers-score")

    # --- relocation sites are not evidence (the object holds the addend, the DOL the address)
    check("a masked offset set covers four bytes", sorted(masked_offsets([0x1002], 0x1000, 8)), [2, 3, 4, 5])
    check("a masked offset outside the run is dropped", sorted(masked_offsets([0x2000], 0x1000, 8)), [])
    check("a masked byte is not a difference",
          first_byte_diff(b"ABCDEFGH", b"ABCDZZGH", {4, 5}), None)
    check("an unmasked byte is a difference",
          first_byte_diff(b"ABCDEFGH", b"ABCDZZGH", {6, 7}), 4)
    masked_ok = classify(run(), claimed, target(b"ABCDZZGH", reloc_addrs=[0x1004]),
                         ours(0x1000, b"ABCDEFGH", reloc_addrs=[0x1004]), symbol_at, stats)
    check("a relocation-covered difference is safe", masked_ok["verdict"], "safe")
    check("the entry records the target's relocation count", masked_ok["target_relocs"], 1)
    reloc_diff = classify(run(), claimed, target(b"ABCDEFGH", reloc_addrs=[0x1004]),
                          ours(0x1000, b"ABCDEFGH", reloc_addrs=[0x1005]), symbol_at, stats)
    check("differing relocation sites lower the score", reloc_diff["verdict"], "lowers-score")
    check("the reason says the relocation sites differ", "relocation sites differ" in reloc_diff["reason"], True)

    # --- the DOL reader
    blob = bytearray(0x120)
    struct.pack_into(">7I", blob, 0x00, 0x100, 0, 0, 0, 0, 0, 0)
    struct.pack_into(">11I", blob, 0x1C, 0x108, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    struct.pack_into(">7I", blob, 0x48, 0x80004000, 0, 0, 0, 0, 0, 0)
    struct.pack_into(">11I", blob, 0x64, 0x80500000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    struct.pack_into(">7I", blob, 0x90, 8, 0, 0, 0, 0, 0, 0)
    struct.pack_into(">11I", blob, 0xAC, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    blob[0x100:0x108] = b"TEXTDATA"
    blob[0x108:0x110] = b"DATADATA"
    sections = dol_sections(bytes(blob))
    check("the DOL reader finds both sections", sections, [(0x80004000, 8, 0x100), (0x80500000, 8, 0x108)])
    check("a DOL text read", dol_bytes(bytes(blob), sections, 0x80004000, 8), b"TEXTDATA")
    check("a DOL data read at an offset", dol_bytes(bytes(blob), sections, 0x80500004, 4), b"DATA")
    check("an unbacked DOL address reads None", dol_bytes(bytes(blob), sections, 0x80700000, 4), None)
    check("a read past a section reads None", dol_bytes(bytes(blob), sections, 0x80004004, 8), None)
    check("a short blob has no sections", dol_sections(b"\x00" * 16), [])

    # --- the neighbours, the ordering and idempotency
    check("a nearest claim is reported with its distance",
          nearest_claim(run(section=".sdata2", start=0x8020, end=0x8028), claimed),
          {"unit": "Pl/pl_act", "section": ".sdata2", "start": 0x8000, "end": 0x8010, "distance": 0x10})
    check("a touching claim is distance zero",
          nearest_claim(run(section=".sdata2", start=0x8010, end=0x8018), claimed)["distance"], 0)
    check("no claim in a section reports None", nearest_claim(run(section=".rodata"), claimed), None)
    entries = [
        {"unit": "b", "section": ".sdata2", "start": 0x200, "end": 0x204, "size": 4, "verdict": "safe"},
        {"unit": "a", "section": ".data", "start": 0x300, "end": 0x304, "size": 4, "verdict": "unowned"},
        {"unit": "c", "section": ".data", "start": 0x100, "end": 0x108, "size": 8, "verdict": "overlap"},
    ]
    ordered = sort_entries(entries)
    check("the order is section order then address", [e["unit"] for e in ordered], ["c", "a", "b"])
    check("the order is stable", [e["unit"] for e in sort_entries(ordered)], ["c", "a", "b"])
    check("rendering twice is byte-identical", render(ordered), render(sort_entries(ordered)))
    check("the render is valid JSON", json.loads(render(ordered))[0]["unit"], "c")
    check("the section order matches the queue's", section_key(".data") < section_key(".sdata2"), True)
    check("an unknown section sorts last", section_key(".zzz")[0], len(dq.SECTION_ORDER))
    check("every verdict is from the vocabulary", set(VERDICTS), {"safe", "lowers-score", "overlap", "unowned"})
    check("a summary counts the verdicts", "safe 1" in summary(ordered, "q.json"), True)
    check("an empty summary says so", summary([], "q.json"), "no runs - the queue is empty or absent")
    check("hex_prefix truncates", hex_prefix(bytes(range(32))), bytes(range(16)).hex())
    check("hex_prefix of nothing is None", hex_prefix(None), None)

    # --- rule 12: referenced-but-unowned data, the sharing census, and the three claim shapes -----
    rows = [
        {"name": "a", "section": ".sdata", "address": 0x1000, "size": 0x4, "type": "object"},
        {"name": "b", "section": ".sdata", "address": 0x1004, "size": 0x2, "type": "object"},
        {"name": "c", "section": ".data", "address": 0x2000, "size": 0x8, "type": "object"},
        {"name": "d", "section": ".data", "address": 0x2008, "size": 0x0, "type": "object"},
        {"name": "e", "section": ".data", "address": 0x2010, "size": 0x4, "type": "object"},
    ]
    pool_readers = {0x1000: {"U": 1}, 0x1004: {"U": 1, "V": 2}, 0x2000: {"U": 1},
                    0x2008: {}, 0x2010: {"U": 1}}

    def rof(address):
        return pool_readers.get(address, {})

    check("extent: the map `size:` is used first", extent_of(rows[0], rows[1]), (4, "map"))
    check("extent: a size-0 row takes the distance to the next", extent_of(rows[3], rows[4]), (8, "next"))
    check("extent: a trailing size-0 row falls back",
          extent_of({"name": "z", "address": 0x9000, "size": 0}, None), (4, "default"))
    check("claim: a not-4-aligned extent rounds the end up",
          claim_span(0x1004, 0x2), (0x1004, 0x1008, True))
    check("claim: a 4-aligned extent is not rounded", claim_span(0x2000, 0x8), (0x2000, 0x2008, False))
    check("splits: the range line is the paste shape",
          splits_range_line(".sdata", 0x1000, 0x1008), "\t.sdata start:0x00001000 end:0x00001008")
    check("splits: a unit block names the file",
          splits_unit_block("Pl/sdata_pool", ".sdata", 0x1000, 0x1008),
          "Pl/sdata_pool.cpp:\n\t.sdata start:0x00001000 end:0x00001008")
    check("symbols: the pool row carries address and size",
          symbols_row("Pl/sdata_pool", ".sdata", 0x1000, 0x8),
          "Pl/sdata_pool = .sdata:0x00001000; // type:object size:0x8")

    pool = pool_around(rows[0], rows[:2], rof)
    check("pool: contiguous symbols merge",
          (pool["start"], pool["end"], pool["count"]), (0x1000, 0x1006, 2))
    check("pool: the readers are the union", sorted(pool["readers"]), ["U", "V"])
    lone = pool_around(rows[2], rows[2:], rof)
    check("pool: an unread neighbour is still included",
          (lone["start"], lone["count"]), (0x2000, 2))
    disjoint = pool_around(
        {"name": "x", "address": 0x3000, "size": 0x4},
        [{"name": "x", "address": 0x3000, "size": 0x4},
         {"name": "y", "address": 0x3004, "size": 0x4}],
        lambda a: {0x3000: {"U": 1}, 0x3004: {"Z": 1}}.get(a, {}))
    check("pool: a reader-disjoint neighbour ends the pool",
          (disjoint["start"], disjoint["count"]), (0x3000, 1))

    def rec(name, section, address, extent, owner=None, sharers=None, pool=None):
        return {"name": name, "section": section, "address": address, "extent": extent,
                "owner": owner, "sharers": sharers or [], "pool": pool}

    plain_pool = {"start": 0x1000, "end": 0x1006, "count": 2, "readers": ["Pl/pl_act", "V"]}
    shared = recommend(rec("a", ".sdata", 0x1000, 0x4, sharers=["V"], pool=plain_pool),
                       "Pl/pl_act", [])
    check("remedy: a shared symbol gets a named data-only unit", shared["remedy"], "named-owner-unit")
    check("remedy: the named unit is <module>/<section>_pool",
          shared["splits"], "Pl/sdata_pool.cpp:\n\t.sdata start:0x00001000 end:0x00001008")
    check("remedy: the named unit gets a symbols.txt row",
          shared["symbols"].startswith("Pl/sdata_pool = .sdata:0x00001000;"), True)
    check("remedy: the named unit explains NonMatching and the DOL",
          ("NonMatching" in shared["note"] and "DOL" in shared["note"]), True)
    check("remedy: the symbols.txt row is marked optional", shared["symbols_optional"], True)
    check("remedy: and counts the rows it would overlap", shared["symbols_overlap_rows"], 2)
    check("remedy: the note says OPTIONAL and names the precedent",
          ("OPTIONAL" in shared["note"] and "Pl/pl_frame_data" in shared["note"]), True)

    # the exact pool the two Pl lanes hand-wrote a diff for: 17 word rows, one 0x44 object over them
    sdata_pool = {"start": 0x80799F98, "end": 0x80799FDC, "count": 17,
                  "readers": ["Pl/fn_80258FCC", "Pl/fn_8025F088"]}
    sdata = recommend(rec("pl_float_neg250", ".sdata2", 0x80799F98, 0x4,
                          sharers=["Pl/fn_80258FCC"], pool=sdata_pool), "Pl/fn_8025F088", [])
    check("remedy: the pool lane's row is produced exactly", sdata["symbols"],
          "Pl/sdata2_pool = .sdata2:0x80799F98; // type:object size:0x44")
    check("remedy: it is marked optional", sdata["symbols_optional"], True)
    check("remedy: overlapping the 17 word rows", sdata["symbols_overlap_rows"], 17)

    private = recommend(rec("c", ".data", 0x2000, 0x8), "Pl/pl_act", [])
    check("remedy: a private symbol is claimed into the unit", private["remedy"], "claim-into-unit")
    check("remedy: the claim covers the whole extent", private["splits"],
          "\t.data start:0x00002000 end:0x00002008")

    span = recommend(rec("c", ".data", 0x2000, 0x8), "Pl/pl_act", [(0x1F00, 0x1FF0)])
    check("remedy: a second run of one section becomes a span claim", span["remedy"], "span-claim")
    check("remedy: the span starts at the unit's own run", span["claim_start"], 0x1F00)
    check("remedy: the span includes the gap and the symbol", span["claim_end"], 0x2008)
    check("remedy: the span names playbook 53's cycle", "link-order cycle" in span["note"], True)

    header = recommend(rec("t", ".data", 0x2000, 0x8, owner="Other/unit"), "Pl/pl_act", [])
    check("remedy: data another unit owns is rule 2's, not a claim", header["remedy"], "owner-header")
    check("remedy: rule 2's remedy makes no claim", header["splits"], None)
    check("remedy: rule 2's remedy names the owner", "`Other/unit`" in header["note"], True)

    rounded = recommend(rec("b", ".sdata", 0x1004, 0x2), "Pl/pl_act", [])
    check("remedy: a not-4-aligned extent still claims the whole symbol", rounded["claim_end"], 0x1008)
    check("remedy: and it says the end was rounded", "not 4-aligned" in rounded["note"], True)

    check("readers: a source label is normalised to its unit",
          norm_reader("Pl/pl_act_step.cpp"), "Pl/pl_act_step")
    check("readers: a C source label too", norm_reader("Pl/fn_8010D1A8.c"), "Pl/fn_8010D1A8")
    check("readers: an unsplit-address label is not a unit", norm_reader("unsplit address"), None)
    check("readers: an unsplit-module label is not a unit", norm_reader("unsplit (ef)"), None)
    check("readers: a no-source-yet label keeps the unit name",
          norm_reader("Pl/x.cpp (no source yet)"), "Pl/x")

    splits_fix = [{"unit": "Pl/pl_act.cpp",
                   "ranges": [{"section": ".data", "start": 0x1F00, "end": 0x1FF0}]}]
    check("unit_ranges: keyed by the bare unit name",
          unit_ranges(splits_fix, "Pl/pl_act"), {".data": [(0x1F00, 0x1FF0)]})
    check("unit_ranges: another unit is not this one's", unit_ranges(splits_fix, "Pl/other"), {})

    payload = {"unit": "Pl/pl_act", "object": "build/RMHE08/obj/Pl/pl_act.o",
               "census": {"source": "elf"},
               "entries": [dict(rec("c", ".data", 0x2000, 0x8), **private)],
               "summary": _remedy_counts([dict(rec("c", ".data", 0x2000, 0x8), **private)])}
    check("render: the unit header counts the symbols",
          render_references(payload).splitlines()[0].startswith("unit Pl/pl_act -> 1 "), True)
    check("render: the remedy is printed", "remedy: claim-into-unit" in render_references(payload), True)
    check("render: the summary counts the remedies",
          "claim-into-unit 1" in render_references(payload), True)

    # the optional/overlapping `symbols.txt:` line, rendered for the pool the lanes hand-diffed
    sdata_home = dict(rec("pl_float_neg250", ".sdata2", 0x80799F98, 0x4,
                          sharers=["Pl/fn_80258FCC"], pool=sdata_pool), **sdata)
    sdata_payload = {"unit": "Pl/fn_8025F088", "object": "build/RMHE08/obj/Pl/fn_8025F088.o",
                     "census": {"source": "asm"}, "entries": [sdata_home],
                     "summary": _remedy_counts([sdata_home])}
    sdata_text = render_references(sdata_payload)
    check("render: the optional symbols.txt line is printed",
          "symbols.txt: Pl/sdata2_pool = .sdata2:0x80799F98;" in sdata_text, True)
    check("render: and it is called OPTIONAL and OVERLAPPING",
          "OPTIONAL and OVERLAPPING" in sdata_text, True)
    check("render: naming how many rows it overlaps", "17 row(s)" in sdata_text, True)
    check("render: and telling the reader it can be omitted",
          "omit this" in sdata_text and "pl_frame_data" in sdata_text, True)

    # --- `declared in:` - the address's owner vs the declaration's actual home ----------------------
    with tempfile.TemporaryDirectory() as tmp:
        for rel, body in (("include/unsplit/mod.h", "extern const u16 t;\n"),
                          ("include/mod/owner.h", "extern const u16 t;\n"),
                          ("src/other/c.c", "void f(void) {}\n")):
            path = os.path.join(tmp, *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(body)
        loc = declaration_locations(tmp, ["t", "absent"])
        check("declared in: every declaration site is found, sorted", sorted(loc["t"]),
              ["include/mod/owner.h", "include/unsplit/mod.h"])
        check("declared in: a name with no declaration is empty", loc["absent"], [])
        check("declared in: a non-declaration is not a home", declaration_locations(tmp, ["f"])["f"], [])

    home = dict(rec("t", ".data", 0x2000, 0x8, owner="Other/unit"), **header)
    home["declared_in"] = ["include/unsplit/mod.h"]
    home_payload = {"unit": "Pl/pl_act", "object": "build/RMHE08/obj/Pl/pl_act.o",
                    "census": {"source": "elf"}, "entries": [home],
                    "summary": _remedy_counts([home])}
    check("render: the declaration home is printed beside the remedy",
          "declared in: include/unsplit/mod.h" in render_references(home_payload), True)
    nowhere = dict(home)
    nowhere["declared_in"] = []
    nowhere_payload = dict(home_payload, entries=[nowhere])
    check("render: no declaration site reads not declared",
          "declared in: not declared" in render_references(nowhere_payload), True)
    built = reference_record("Pl/pl_act", rec("t", ".data", 0x2000, 0x8, owner="Other/unit"),
                             8, "map", "Other/unit", None, [], None, {"t": ["include/unsplit/mod.h"]})
    check("reference_record carries the declaration home", built["declared_in"], ["include/unsplit/mod.h"])
    check("reference_record keys a name it did not see as not declared",
          reference_record("Pl/pl_act", rec("z", ".data", 0x3000, 4), 4, "map", None, None, [], None,
                           {})["declared_in"], [])

    # --- emission-order seams: a run/claim that spans several TUs is warned about, never silently safe
    strong = [{"addr": 0x1004, "kind": "V->S"}]
    spanning = classify(run(seams=dataseams.seams_in(strong, 0x1000, 0x1008)), claimed,
                        target(b"ABCDEFGH"), ours(0x1000, b"ABCDEFGH"), symbol_at, stats)
    check("a spanning run keeps its verdict", spanning["verdict"], "safe")
    check("... but names the seam and the cut in its reason",
          ("WARNING" in spanning["reason"], "0x00001004 (V->S)" in spanning["reason"],
           "0x00001004-0x00001008" in spanning["reason"]), (True, True, True))
    check("... and records the seams", spanning["seams"], strong)
    check("a run with no seam carries no warning", (good["seam_warning"], "WARNING" in good["reason"]), (None, False))
    over = classify(run(section=".data", start=0x9004, end=0x900C, seams=strong), claimed,
                    target(b"\x00" * 8), ours(0x9000, b"\x00" * 8, at=0x9000), symbol_at, stats)
    check("an overlap refusal stays the refusal, not a multi-TU note", "WARNING" in over["reason"], False)
    check("summary counts the multi-TU runs", "multi-TU: 1 run(s)" in summary([spanning, good], "q.json"), True)
    claim_rec = reference_record("Pl/pl_act", rec("t", ".data", 0x1000, 0x8), 8, "map", None, None, [], None,
                                 {}, strong)
    check("a rule-12 claim over a seam warns", ("the proposed claim spans at least 2 TUs" in (claim_rec["seam_warning"] or ""),
                                                "WARNING" in claim_rec["note"]), (True, True))
    gap = [{"addr": 0x1004, "kind": "V->S", "latest": 0x1400, "width": 60, "tail": 0, "cut": 0x1004}]
    gap_rec = reference_record("Pl/pl_act", rec("t", ".data", 0x1000, 0x8), 8, "map", None, None, [], None,
                               {}, gap)
    check("a rule-12 claim over a wide gap warns that a boundary lies in [addr, latest), suggesting no cut",
          ("boundary in [0x00001004, 0x00001400)" in (gap_rec["seam_warning"] or ""),
           "no cut suggested" in (gap_rec["seam_warning"] or "")), (True, True))
    check("a rule-12 claim clear of seams does not",
          reference_record("Pl/pl_act", rec("t", ".data", 0x2000, 0x8), 8, "map", None, None, [], None,
                           {}, strong)["seam_warning"], None)

    if fails:
        print("FAIL (%d)" % len(fails))
        for failure in fails:
            print("  " + failure)
        return 1
    print("ok - %d checks" % checks)
    return 0


# -- CLI ------------------------------------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--json", action="store_true", help="print the classified entries on stdout")
    ap.add_argument("--risky", type=int, default=0, metavar="N", help="print the N riskiest runs")
    ap.add_argument("--unit", default=None, metavar="U",
                    help="rule 12: the data symbols unit U references but does not own, with a "
                         "recommended remedy and the exact splits.txt text to paste (read-only; "
                         "splits.txt is never written)")
    ap.add_argument("--dry-run", action="store_true", dest="dry_run",
                    help="assert the reference listing is read-only (the default and only mode)")
    ap.add_argument("--queue-unit", default=None, metavar="U",
                    help="only this unit's proposed runs (the data queue)")
    ap.add_argument("--limit", type=int, default=0, help="only the first N runs (the queue's own order)")
    ap.add_argument("--out", default=None, metavar="FILE", help="write the entries here (atomic)")
    ap.add_argument("--queue", default=None, metavar="FILE", help="the queue to read")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    if args.unit:
        return reference_cli(args.unit, as_json=args.json, dry_run=args.dry_run)

    entries = analyze(queue_path=args.queue)
    if args.queue_unit:
        entries = [e for e in entries if e["unit"] == args.queue_unit]
    if args.limit:
        entries = entries[:args.limit]

    if args.out:
        write_atomic(args.out, render(entries))
        print("wrote %s (%d entries)" % (args.out, len(entries)))

    if args.json:
        sys.stdout.write(render(entries))
        return 0

    print(summary(entries, args.queue or os.path.join(ROOT, QUEUE_REL)))
    if args.risky:
        risky = [e for e in entries if e["verdict"] != "safe"]
        risky.sort(key=lambda e: (VERDICTS.index(e["verdict"]), -e["size"], e["start"]))
        print("\ntop %d riskiest:" % min(args.risky, len(risky)))
        for entry in risky[:args.risky]:
            affected = entry.get("affected") or {}
            print("  %-13s %s 0x%08X-0x%08X (%6d B) %s" % (entry["verdict"], entry["section"],
                                                           entry["start"], entry["end"], entry["size"],
                                                           entry["unit"]))
            print("      %s" % entry["reason"])
            if affected.get("symbol") or affected.get("function"):
                print("      affects: symbol=%s worst-function=%s (%s)"
                      % (affected.get("symbol"), affected.get("function"),
                         affected.get("function_basis")))
            print("      expected: %s" % entry["expected"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
