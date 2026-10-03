#!/usr/bin/env python3
"""Re-cut one unit at a function boundary: print the exact splits.txt lines for both halves, or refuse.
Spec: docs/tools/spec/unwindcut.md. CLI: unwindcut.py <unit> <cut-addr> | --selftest."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os
import re
import struct
import sys

from tools.lib.binary.dol import Dol as LibDol, DolError
from tools.lib.binary.elf import Elf as LibElf

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)

SECTIONS_DIR = os.path.join("config", "RMHE08")
DEFAULT_SPLITS = os.path.join(ROOT, SECTIONS_DIR, "splits.txt")
DEFAULT_SYMBOLS = os.path.join(ROOT, SECTIONS_DIR, "symbols.txt")
DEFAULT_DOL = os.path.join(ROOT, "orig", "RMHE08", "sys", "main.dol")
VERSION = "RMHE08"

# The 12-byte `{fn_addr, fn_size, etab_addr}` record, big-endian like the rest of the image.
RECORD = struct.Struct(">III")
EXTAB_ENTRY = 8
EXTABINDEX_ENTRY = 12
# A cap so a corrupt/misaligned read cannot walk the whole data section. The widest real unit is tens of
# thousands of frames, not millions.
MAX_RECORDS = 1 << 16

SPLIT_BLOCK_RE = re.compile(r"^(?P<unit>\S+):\s*$")
SPLIT_LINE_RE = re.compile(
    r"^\s*(?P<section>[.\w]+)\s+start:(?P<start>0x[0-9a-fA-F]+)\s+end:(?P<end>0x[0-9a-fA-F]+)"
    r"(?:\s+rename:(?P<rename>\S+))?\s*$"
)

# The fragment sections a re-cut has to move 1:1 with the `.text` range.
FRAGMENTS = ("extab", "extabindex")
CTOR_SECTIONS = (".ctors", ".dtors")

KNOWN_SECTIONS = (".text", "extab", "extabindex", ".ctors", ".dtors", ".rodata", ".data", ".bss",
                  ".sdata", ".sbss", ".sdata2", ".sbss2", ".init")


class Refusal(Exception):
    """A condition the tool will not guess through - the message is the report."""


# --------------------------------------------------------------------------------------------------
# inputs


class Dol(LibDol):
    """Address -> bytes for the retail image, through its section table (`lib.binary.dol`)."""

    def __init__(self, path: str):
        with open(path, "rb") as handle:
            data = handle.read()
        try:
            super().__init__(data, path)
        except DolError:
            raise Refusal("%s is too short to be a DOL" % path) from None
        self.secs = [(s.address, s.size, s.offset) for s in self.segments]

    def read(self, addr: int, n: int) -> bytes | None:
        """The n bytes at `addr`, or None when they do not lie wholly inside one section."""
        return self.bytes_at(addr, n)


def read_splits(path: str) -> "dict[str, dict]":
    """{unit key: {section: (start, end)}} in file order - the parser `symbolpreflight.py` uses."""
    blocks: dict[str, dict] = {}
    current: str | None = None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line[:1] not in ("", " ", "\t") and line.rstrip().endswith(":"):
                key = line.strip()[:-1]
                if key != "Sections":
                    current = key
                    blocks.setdefault(current, {})
                else:
                    current = None
                continue
            match = SPLIT_LINE_RE.match(line)
            if match and current:
                name = match.group("section")
                blocks[current][name] = (int(match.group("start"), 16), int(match.group("end"), 16))
    return blocks


def read_functions(path: str):
    """The `.text` function rows of `symbols.txt`, address-sorted: [(addr, size, name)]."""
    try:
        if os.path.join(TOOLS, "symbols") not in sys.path:
            sys.path.insert(0, os.path.join(TOOLS, "symbols"))
        import symedit  # noqa: E402  (same shim as symbolpreflight.py)
    except ImportError:
        return []
    if not os.path.exists(path):
        return []
    rows = []
    for entry in symedit.entries(path):
        if entry.get("section") == ".text" and entry.get("type") == "function":
            rows.append((entry["address"], entry.get("size") or 0, entry["name"]))
    rows.sort()
    return rows


def resolve_unit(blocks: "dict[str, dict]", spec: str) -> str:
    """The `splits.txt` block key for a unit spec like `menu/menu_message` or `menu/menu_message.cpp`."""
    s = spec.replace("\\", "/").strip()
    for prefix in ("build/", "src/"):
        if s.startswith(prefix):
            s = s[len(prefix):]
    if s.endswith(".o"):
        s = s[:-2]
    for candidate in (s, s + ".cpp", s + ".c", s + ".cp", s + ".cc"):
        if candidate in blocks:
            return candidate
    stem = os.path.splitext(s.rsplit("/", 1)[-1])[0]
    matches = [key for key in blocks if os.path.splitext(key.rsplit("/", 1)[-1])[0] == stem]
    if len(matches) == 1:
        return matches[0]
    hint = ", ".join(sorted(matches)[:3]) if matches else "none"
    raise Refusal("no `splits.txt` block for %r (matches by file stem: %s)" % (spec, hint))


# --------------------------------------------------------------------------------------------------
# the analysis


def next_start(blocks: "dict[str, dict]", section: str, after: int, unit: str):
    """The smallest start of `section` strictly at/after `after` claimed by a *different* unit."""
    best = None
    for key, ranges in blocks.items():
        if key == unit or section not in ranges:
            continue
        start = ranges[section][0]
        if start >= after and (best is None or start < best):
            best = start
    return best


def claim_span(blocks: "dict[str, dict]", section: str):
    """(min start, max end) over every claim of `section` - the section's doorstep in `.ctors` scans."""
    starts, ends = [], []
    for ranges in blocks.values():
        if section in ranges:
            starts.append(ranges[section][0])
            ends.append(ranges[section][1])
    return (min(starts), max(ends)) if starts else None


def read_records(dol: Dol, eti_start: int, etab_start: int, text_start: int, text_end: int):
    """The `extabindex` records over `[text_start, text_end)`, verified.

    Returns (records, refusals). A record is `{index, fn, size, etab}`. Stops at the first
    `fn_addr >= text_end` (the records are in function-address order), so the run is the unit's full
    range, not its claim.
    """
    records = []
    refusals: list[str] = []
    addr = eti_start
    while len(records) < MAX_RECORDS:
        raw = dol.read(addr, EXTABINDEX_ENTRY)
        if raw is None or len(raw) < EXTABINDEX_ENTRY:
            refusals.append("extabindex run ends at 0x%X before the .text end 0x%X "
                            "(the section table does not cover it)" % (addr, text_end))
            break
        fn, size, etab = RECORD.unpack(raw)
        if fn >= text_end:
            break
        records.append({"index": len(records), "fn": fn, "size": size, "etab": etab,
                        "etab_addr": addr})
        addr += EXTABINDEX_ENTRY
    else:
        refusals.append("more than %d records before 0x%X - refusing to walk further"
                        % (MAX_RECORDS, text_end))

    for rec in records:
        want = etab_start + EXTAB_ENTRY * rec["index"]
        if rec["etab"] != want:
            refusals.append("record %d (fn 0x%08X): etab_addr 0x%08X != extab base 0x%08X + 8*i = "
                            "0x%08X - the 1:1 extab invariant does not hold"
                            % (rec["index"], rec["fn"], rec["etab"], etab_start, want))
    for i in range(1, len(records)):
        if records[i]["fn"] <= records[i - 1]["fn"]:
            refusals.append("records %d/%d are not in function-address order (0x%08X then 0x%08X)"
                            % (i - 1, i, records[i - 1]["fn"], records[i]["fn"]))
    return records, refusals


def containing_function(functions, addr: int):
    for start, size, name in functions:
        if start <= addr < start + max(size, 1):
            return {"start": start, "size": size, "name": name}
    return None


def function_at(functions, addr: int):
    for start, size, name in functions:
        if start == addr:
            return {"start": start, "size": size, "name": name}
    return None


def analyse(unit_spec: str, cut: int, splits_path: str = DEFAULT_SPLITS,
            dol_path: str = DEFAULT_DOL, symbols_path: str = DEFAULT_SYMBOLS,
            object_path: str | None = None) -> dict:
    """The whole computation, as data. Raises `Refusal` when it will not guess."""
    blocks = read_splits(splits_path)
    unit = resolve_unit(blocks, unit_spec)
    claimed = blocks[unit]
    if ".text" not in claimed:
        raise Refusal("%s claims no `.text` range in %s" % (unit, splits_path))

    ts, te = claimed[".text"]

    # --- the full range: the claim, plus the immediately-following unclaimed gap ------------------
    absorbed: dict[str, tuple[int, int]] = {}
    full_end = te
    nxt = next_start(blocks, ".text", te, unit)
    if nxt is not None and nxt > te:
        full_end = nxt
        absorbed[".text"] = (te, nxt)
    text_end = full_end
    if not (ts <= cut < text_end):
        raise Refusal("the cut 0x%X is not inside %s's full .text range 0x%X-0x%X"
                      % (cut, unit, ts, text_end))

    extab = claimed.get("extab")
    extabindex = claimed.get("extabindex")
    records: list[dict] = []
    kept_records = 0
    warnings: list[str] = []

    if extabindex and extab:
        eti_start, eti_end = extabindex
        etab_start, etab_end = extab
        records, refusals = read_records(dol_from(dol_path), eti_start, etab_start, ts, text_end)
        if refusals:
            raise Refusal("; ".join(refusals))
        if not records:
            raise Refusal("%s claims extabindex 0x%X-0x%X but the DOL carries no record there"
                          % (unit, eti_start, eti_end))

        # the claim covers a prefix k of the records; extab/extabindex ends must agree with 8/12 B each
        kept_records = sum(1 for rec in records if rec["fn"] < te)
        problems = []
        if eti_end != eti_start + EXTABINDEX_ENTRY * kept_records:
            problems.append("claimed extabindex end 0x%08X != start + 12*%d = 0x%08X"
                            % (eti_end, kept_records, eti_start + EXTABINDEX_ENTRY * kept_records))
        if etab_end != etab_start + EXTAB_ENTRY * kept_records:
            problems.append("claimed extab end 0x%08X != start + 8*%d = 0x%08X"
                            % (etab_end, kept_records, etab_start + EXTAB_ENTRY * kept_records))
        if kept_records and records[kept_records - 1]["fn"] + max(records[kept_records - 1]["size"], 4) != te:
            # the claim may legitimately end at a gap, but a non-boundary end is worth naming
            warnings.append("the claimed .text end 0x%08X is not the end of record %d (fn 0x%08X + %d B)"
                            % (te, kept_records - 1, records[kept_records - 1]["fn"],
                               records[kept_records - 1]["size"]))
        if problems:
            raise Refusal("; ".join(problems))

        # --- absorbed gaps must agree across the three sections -----------------------------------
        if absorbed.get(".text"):
            n = len(records)
            gap_start, gap_end = absorbed[".text"]
            implied_extab = (etab_start + EXTAB_ENTRY * n)
            implied_eti = (eti_start + EXTABINDEX_ENTRY * n)
            n_extab = next_start(blocks, "extab", etab_end, unit)
            n_eti = next_start(blocks, "extabindex", eti_end, unit)
            problems = []
            if n_extab is not None and n_extab != implied_extab:
                problems.append("the .text gap implies %d records (extab would end 0x%08X) but the next "
                                "extab claim starts 0x%08X" % (n, implied_extab, n_extab))
            if n_eti is not None and n_eti != implied_eti:
                problems.append("the .text gap implies %d records (extabindex would end 0x%08X) but the "
                                "next extabindex claim starts 0x%08X" % (n, implied_eti, n_eti))
            if problems:
                raise Refusal("the unclaimed tail 0x%08X-0x%08X disagrees with the unwind gaps: %s"
                              % (gap_start, gap_end, "; ".join(problems)))
            if n_extab is not None:
                absorbed["extab"] = (etab_end, n_extab)
            if n_eti is not None:
                absorbed["extabindex"] = (eti_end, n_eti)

    # --- boundary 1: the cut must be a record's fn_addr --------------------------------------------
    functions = read_functions(symbols_path)
    inside = containing_function(functions, cut) if functions else None
    cut_record = next((rec for rec in records if rec["fn"] == cut), None)
    if records and cut_record is None:
        prev = max((r for r in records if r["fn"] < cut), key=lambda r: r["fn"], default=None)
        nxt_rec = min((r for r in records if r["fn"] > cut), key=lambda r: r["fn"], default=None)
        raise Refusal("0x%08X is not a function boundary in %s: no extabindex record starts there%s "
                      "(nearest framed functions: %s before, %s after)"
                      % (cut, unit,
                         " (it is inside %s, 0x%X-0x%X)"
                         % (inside["name"], inside["start"], inside["start"] + inside["size"])
                         if inside else "",
                         "0x%08X" % prev["fn"] if prev else "none",
                         "0x%08X" % nxt_rec["fn"] if nxt_rec else "none"))

    # --- boundary 2: symbols.txt must also see a function start, with the previous one ending there
    cut_fn = function_at(functions, cut)
    if functions:
        if cut_fn is None:
            raise Refusal("0x%08X is not a function boundary in %s: no .text function symbol starts "
                          "there%s - a cut mid-function would corrupt the split"
                          % (cut, unit,
                             " (it is inside %s, 0x%X-0x%X)"
                             % (inside["name"], inside["start"], inside["start"] + inside["size"])
                             if inside else ""))
        prev_fn = None
        for start, size, name in functions:
            if start < cut:
                prev_fn = {"start": start, "size": size, "name": name}
            else:
                break
        if prev_fn and prev_fn["start"] + max(prev_fn["size"], 4) != cut:
            raise Refusal("0x%08X is not a function boundary in %s: %s (0x%X) ends at 0x%X, not there"
                          % (cut, unit, prev_fn["name"], prev_fn["start"],
                             prev_fn["start"] + prev_fn["size"]))
        if cut_fn["size"] and cut + cut_fn["size"] > text_end and cut + cut_fn["size"] > full_end:
            warnings.append("the cut function %s runs past the full .text end 0x%08X"
                            % (cut_fn["name"], full_end))

    # --- the halves --------------------------------------------------------------------------------
    cut_index = cut_record["index"] if cut_record else kept_records
    total_records = len(records) if records else None
    extab_half, eti_half = None, None
    if records:
        etab_start = claimed["extab"][0] if extab else None
        eti_start = claimed["extabindex"][0] if extabindex else None
        if etab_start is not None:
            extab_full_end = etab_start + EXTAB_ENTRY * total_records
            extab_half = {"kept": (etab_start, etab_start + EXTAB_ENTRY * cut_index),
                          "tail": (etab_start + EXTAB_ENTRY * cut_index, extab_full_end)}
        if eti_start is not None:
            eti_full_end = eti_start + EXTABINDEX_ENTRY * total_records
            eti_half = {"kept": (eti_start, eti_start + EXTABINDEX_ENTRY * cut_index),
                        "tail": (eti_start + EXTABINDEX_ENTRY * cut_index, eti_full_end)}

    def rows(lo: int, hi: int) -> int:
        return sum(1 for a, _s, _n in functions if lo <= a < hi)

    text_kept = (ts, cut)
    text_tail = (cut, text_end)
    sums = {
        ".text": {"kept": text_kept[1] - text_kept[0], "tail": text_tail[1] - text_tail[0],
                  "original": text_end - ts},
    }
    if extab_half:
        sums["extab"] = {"kept": extab_half["kept"][1] - extab_half["kept"][0],
                         "tail": extab_half["tail"][1] - extab_half["tail"][0],
                         "original": extab_half["tail"][1] - extab_half["kept"][0]}
    if eti_half:
        sums["extabindex"] = {"kept": eti_half["kept"][1] - eti_half["kept"][0],
                              "tail": eti_half["tail"][1] - eti_half["tail"][0],
                              "original": eti_half["tail"][1] - eti_half["kept"][0]}
    sums["records"] = {"kept": cut_index, "tail": (total_records or 0) - cut_index,
                       "original": total_records or 0}

    # --- .ctors / .dtors words whose target leaves with the cut ------------------------------------
    dol = dol_from(dol_path)
    name_by_addr = {}
    for start, _size, name in functions:
        name_by_addr.setdefault(start, name)
    ctors = []
    kept_ctor_spans: dict[str, tuple[int, int]] = {}
    for section in CTOR_SECTIONS:
        span = claim_span(blocks, section)
        if not span:
            continue
        raw = dol.read(span[0], span[1] - span[0])
        if raw is None:
            continue
        for off in range(0, len(raw) - 3, 4):
            target = struct.unpack(">I", raw[off:off + 4])[0]
            if ts <= target < text_end:
                ctors.append({"section": section, "addr": span[0] + off, "target": target,
                              "leaves": target >= cut, "name": name_by_addr.get(target, "")})
        kept = sorted(w["addr"] for w in ctors if w["section"] == section and not w["leaves"])
        if kept:
            kept_ctor_spans[section] = (kept[0], kept[-1] + 4)

    result = {
        "unit": unit,
        "cut": cut,
        "cut_index": cut_index,
        "cut_function": cut_fn,
        "text": {"start": ts, "end": text_end, "claimed_end": te,
                 "kept": text_kept, "tail": text_tail,
                 "rows_kept": rows(ts, cut), "rows_tail": rows(cut, text_end),
                 "rows_claimed": rows(ts, te), "rows_total": rows(ts, text_end)},
        "extab": None if not extab_half else {"start": extab_half["kept"][0],
                                              "kept": extab_half["kept"], "tail": extab_half["tail"]},
        "extabindex": None if not eti_half else {"start": eti_half["kept"][0],
                                                 "kept": eti_half["kept"], "tail": eti_half["tail"]},
        "records": total_records,
        "absorbed": absorbed,
        "sums": sums,
        "ctors": ctors,
        "kept_ctor_spans": kept_ctor_spans,
        "warnings": warnings,
        "symbols_seen": len(functions),
    }
    # The split object cross-check: it only covers the claimed prefix, so it verifies the kept half.
    if object_path is None:
        stem = unit.rsplit(".", 1)[0] if unit.rsplit(".", 1)[-1] in ("c", "cpp", "cp", "cc") else unit
        object_path = os.path.join(tree_root(splits_path), "build", VERSION, "obj", stem + ".o")
    result["object"] = object_cross_check(object_path, text_kept[1] - text_kept[0], cut_index)
    result["warnings"].extend("split object disagrees with the kept half: %s" % p
                              for p in result["object"]["problems"])
    return result


def dol_from(path: str) -> Dol:
    """A standalone DOL reader; kept out of `analyse`'s signature so fixtures can hand one in."""
    return _DOL_CACHE.setdefault(path, Dol(path))


_DOL_CACHE: dict[str, Dol] = {}


def tree_root(splits_path: str) -> str:
    """The repository a `config/<game>/splits.txt` lives in (for deriving the split object's path)."""
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(splits_path))))


def object_sizes(path: str) -> dict:
    """{section: byte size} for the split object, or {} when it is missing/unreadable."""
    if not path or not os.path.exists(path):
        return {}
    try:
        return {s.name: s.size for s in LibElf.read(path).sections}
    except Exception:  # an unreadable object is a missing object, never a refusal
        return {}


def object_cross_check(object_path: str, text_bytes: int, kept_records: int) -> dict:
    """Compare the split object's section sizes with the kept half the tool computed.

    This is the one thing that *is* read from `build/RMHE08/obj/<unit>.o`: the object only covers the
    claimed prefix (the released tail has no object), so its `extab`/`extabindex`/`.text` sizes say
    whether the claimed half is exactly `8*k`/`12*k`/`.text bytes` - a warning when they disagree, never
    a guess (a stale object is not a reason to refuse a re-cut).
    """
    sizes = object_sizes(object_path)
    if not sizes:
        return {"path": object_path, "present": False, "problems": []}
    want = {".text": text_bytes, "extab": EXTAB_ENTRY * kept_records,
            "extabindex": EXTABINDEX_ENTRY * kept_records}
    problems = ["%s: object %d B, the kept half is %d B" % (name, sizes.get(name, -1), size)
                for name, size in want.items() if name in sizes and sizes[name] != size]
    return {"path": object_path, "present": True, "problems": problems}


# --------------------------------------------------------------------------------------------------
# the report


def hexrange(pair) -> str:
    return "0x%X-0x%X" % pair


def tail_unit_name(result: dict) -> str:
    """A placeholder key for the released half, from the cut function's stem (rename it later)."""
    unit = result["unit"]
    directory = unit.rsplit("/", 1)[0] if "/" in unit else ""
    fn = (result["cut_function"] or {}).get("name") or "cut"
    stem = re.sub(r"__.*$", "", fn) or ("fn_%08X" % result["cut"])
    return "%s/%s.cpp" % (directory, stem) if directory else "%s.cpp" % stem


def print_report(result: dict) -> None:
    unit, cut = result["unit"], result["cut"]
    text = result["text"]
    print("unwindcut: %s  cut 0x%X" % (unit, cut))
    print()
    print("  unit            %s" % unit)
    print("  claimed .text   %s  (%d B, %d rows)"
          % (hexrange((text["start"], text["claimed_end"])),
             text["claimed_end"] - text["start"], text["rows_claimed"]))
    absorbed = result["absorbed"].get(".text")
    if absorbed:
        # the absorbed gap is [claimed end, full end), so its row count is the rows beyond the claim
        print("  unclaimed tail  %s  (%d B, %d rows)   <- re-absorbed for the re-cut"
              % (hexrange(absorbed), absorbed[1] - absorbed[0],
                 text["rows_total"] - text["rows_claimed"]))
    if result["records"] is not None:
        print("  boundary        0x%X is frame %d/%d%s"
              % (cut, result["cut_index"], result["records"],
                 ": %s (+0x%X)" % (result["cut_function"]["name"], result["cut_function"]["size"])
                 if result["cut_function"] else ""))
        print("  invariant       etab_addr == 0x%X + 8*i over all %d records; extabindex 1:1 at 12 B"
              % (result["extab"]["start"], result["records"]))
    for warning in result["warnings"]:
        print("  warning         %s" % warning)
    obj = result.get("object") or {}
    if obj.get("present") and not obj.get("problems"):
        try:
            where = os.path.relpath(obj["path"], ROOT).replace("\\", "/")
        except ValueError:
            where = obj["path"]
        print("  split object     %s agrees with the kept half (extab 8*%d, extabindex 12*%d)"
              % (where, result["cut_index"], result["cut_index"]))
    print()

    tail = tail_unit_name(result)
    print("--- keep: %s ---" % unit)
    print("\t.text       start:0x%X end:0x%X" % text["kept"])
    print("        # %d B, %d .text rows" % (text["kept"][1] - text["kept"][0], text["rows_kept"]))
    if result["extab"]:
        print("\textab       start:0x%X end:0x%X" % result["extab"]["kept"])
        print("        # %d B, %d records" % (result["extab"]["kept"][1] - result["extab"]["kept"][0],
                                              result["sums"]["records"]["kept"]))
        print("\textabindex  start:0x%X end:0x%X" % result["extabindex"]["kept"])
        print("        # %d B, %d records"
              % (result["extabindex"]["kept"][1] - result["extabindex"]["kept"][0],
                 result["sums"]["records"]["kept"]))
    for section in CTOR_SECTIONS:
        if section in result["kept_ctor_spans"]:
            print("\t%-11s start:0x%X end:0x%X" % (section, *result["kept_ctor_spans"][section]))
    print()
    print("--- release: %s ---" % tail)
    print("\t.text       start:0x%X end:0x%X" % text["tail"])
    print("        # %d B, %d .text rows" % (text["tail"][1] - text["tail"][0], text["rows_tail"]))
    if result["extab"]:
        print("\textab       start:0x%X end:0x%X" % result["extab"]["tail"])
        print("        # %d B, %d records" % (result["extab"]["tail"][1] - result["extab"]["tail"][0],
                                              result["sums"]["records"]["tail"]))
        print("\textabindex  start:0x%X end:0x%X" % result["extabindex"]["tail"])
        print("        # %d B, %d records"
              % (result["extabindex"]["tail"][1] - result["extabindex"]["tail"][0],
                 result["sums"]["records"]["tail"]))
    if result["ctors"] and not result["kept_ctor_spans"]:
        print("        # no .ctors/.dtors word survives - dtk re-derives the claim from the tail")
    print()

    print("--- sum check ---")
    for section in (".text", "extab", "extabindex"):
        if section not in result["sums"]:
            continue
        s = result["sums"][section]
        verdict = "OK" if s["kept"] + s["tail"] == s["original"] else "MISMATCH"
        print("  %-11s %5d + %5d = %5d   %s" % (section, s["kept"], s["tail"], s["original"], verdict))
    r = result["sums"]["records"]
    print("  %-11s %5d + %5d = %5d   %s"
          % ("records", r["kept"], r["tail"], r["original"],
             "OK" if r["kept"] + r["tail"] == r["original"] else "MISMATCH"))
    print()

    print("--- .ctors/.dtors ---")
    if not result["ctors"]:
        print("  none of this unit's .ctors/.dtors words target its range - nothing to drop")
    for word in result["ctors"]:
        tag = "DROP" if word["leaves"] else "keep"
        print("  %-4s %s word at 0x%X -> %s0x%X%s"
              % (tag, word["section"], word["addr"],
                 (word["name"] + " ") if word["name"] else "", word["target"],
                 " - target leaves with the cut" if word["leaves"] else " - target stays"))
    dropped = [w for w in result["ctors"] if w["leaves"]]
    if dropped:
        regions = []
        for word in sorted(dropped, key=lambda w: (w["section"], w["addr"])):
            if regions and regions[-1][0] == word["section"] and regions[-1][2] == word["addr"]:
                regions[-1][2] = word["addr"] + 4
            else:
                regions.append([word["section"], word["addr"], word["addr"] + 4])
        print("  drop the claim covering %s"
              % ", ".join("%s 0x%X-0x%X" % (s, a, b) for s, a, b in regions))
    if dropped:
        print("  rule: un-claiming a .text range obliges dropping every .ctors/.dtors word whose target")
        print("        leaves with it; dtk re-derives the claim from the tail (docs/pipeline.md 9.4.1).")


# --------------------------------------------------------------------------------------------------
# entry points


def selftest() -> int:
    """Delegates to `unwindcut_selftest.py` (fixtures only - no build, no repository state)."""
    import unwindcut_selftest
    return unwindcut_selftest.selftest()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", nargs="?", help="unit spec, e.g. menu/menu_message")
    ap.add_argument("cut", nargs="?", help="the cut address, e.g. 0x802AA764")
    ap.add_argument("--splits", default=DEFAULT_SPLITS, help="splits.txt (read-only, never written)")
    ap.add_argument("--dol", default=DEFAULT_DOL, help="the retail image the unwind records are read from")
    ap.add_argument("--symbols", default=DEFAULT_SYMBOLS, help="symbols.txt (function-boundary oracle)")
    ap.add_argument("--object", default=None, help="the split object, for the claimed-half cross-check")
    ap.add_argument("--json", action="store_true", help="machine-readable result instead of the report")
    ap.add_argument("--selftest", action="store_true", help="run the fixtures-only selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.unit or args.cut is None:
        ap.error("a unit and a cut address are required, e.g. menu/menu_message 0x802AA764")
    try:
        cut = int(args.cut, 0)
    except ValueError:
        ap.error("cannot read the cut address %r" % args.cut)

    try:
        result = analyse(args.unit, cut, args.splits, args.dol, args.symbols, args.object)
    except Refusal as exc:
        print("unwindcut: REFUSED - %s" % exc, file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print_report(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
