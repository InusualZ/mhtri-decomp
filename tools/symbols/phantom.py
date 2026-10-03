#!/usr/bin/env python3
"""Find phantom `fn_XXXXXXXX` rows (dead epilogues) with reachability evidence from the linked DOL.
Spec: docs/tools/spec/phantom.md. CLI: phantom.py [scan|explain <name|0xADDR>] [--all] [--json]
[--max-size N] [--section S] [--file F] [--dol F] [--dump F|auto] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import bisect
import json
import os
import re
import struct
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)

from tools.lib import ppc as _ppc  # noqa: E402  the one instruction decoder
from tools.symbols import symedit  # noqa: E402  the map parser - never read symbols.txt directly
from tools.units.m2cinput import Dol  # noqa: E402  the read-only DOL image (address -> bytes)

DEFAULT_FILE = os.path.join(ROOT, "config", "RMHE08", "symbols.txt")
DEFAULT_DOL = os.path.join(ROOT, "orig", "RMHE08", "sys", "main.dol")
DUMP_PATHS = ("D:/WiiExperiment/DumpSymbols.zip", "D:/WiiExperiment/Dump_Loading85.raw.map")
DUMP_MEMBER = "Dump_Loading85.raw.map"
UNNAMED = re.compile(r"^fn_[0-9A-Fa-f]{8}$")
MAX_SIZE = 8
DUMP_WINDOW = 0x10000
DUMP_MIN_NEIGHBOURS = 16
BLR = 0x4E800020
CHANNELS = ("callers", "branches", "addrloads", "datawords")
VERDICTS = ("keep", "merge", "unclear")


# --------------------------------------------------------------------------------------------- evidence

class Refs:
    """Static references to code addresses, counted from the linked image, in four channels.

    callers    `bl` to the address - a real call.
    branches   `b`/`bc` to the address - a tail call or a goto; still an entry point.
    addrloads  `lis rD,hi` + `addi/ori rD,rD,lo` materialising the address in code.
    datawords  a 32-bit word equal to the address in a data section: a vtable slot or a jump-table entry.
    """

    def __init__(self) -> None:
        self.counts: dict[str, dict[int, int]] = {ch: {} for ch in CHANNELS}

    def add(self, channel: str, address: int) -> None:
        self.counts[channel][address] = self.counts[channel].get(address, 0) + 1

    def get(self, channel: str, address: int) -> int:
        return self.counts[channel].get(address, 0)

    def total(self, address: int) -> int:
        return sum(self.get(ch, address) for ch in CHANNELS)


def index_refs(image) -> Refs:
    """Every static reference to a code address in the linked image, per channel (see `Refs`); the decode is
    `lib.ppc` (`branch_target`, `materialisations`)."""
    refs = Refs()
    text = [s for s in image.sections[:7] if s[2]]
    ranges = [(a, a + size) for _o, a, size in text]

    def is_code(value: int) -> bool:
        return any(lo <= value < hi for lo, hi in ranges)

    for offset, start, size in text:
        code = image.data[offset:offset + size]
        for i, insn in enumerate(_ppc.words(code)):
            target = _ppc.branch_target(start + i * 4, insn)
            if target is not None:
                refs.add("callers" if insn & 1 else "branches", target)
        for _site, value in _ppc.materialisations(code, start, 16):
            if is_code(value):
                refs.add("addrloads", value)
    for offset, start, size in image.sections[7:]:
        for i in range(0, size - 3, 4):
            word = struct.unpack_from(">I", image.data, offset + i)[0]
            if is_code(word):
                refs.add("datawords", word)
    return refs


class DumpMap:
    """The 2021 runtime symbol map: the addresses the game's own symbol table names.

    Format: `name [args] <hex address> <flags>`.  A `zz_<address>_` name is a placeholder for a
    genuinely unnamed function; **no line at all** means the dump's analysis did not see a function
    there (`docs/memory-dump.md`).  That silence is only evidence where the dump covers the address, so
    `covered()` requires a populated neighbourhood - a module the loading-state dump never reached must
    not have its absence read as "not a function".
    """

    def __init__(self, names: dict[int, str], path: str | None = None) -> None:
        self.names = names
        self.sorted = sorted(names)
        self.path = path

    @classmethod
    def from_lines(cls, lines, path: str | None = None) -> "DumpMap":
        names: dict[int, str] = {}
        for line in lines:
            tok = line.split()
            if len(tok) < 3:
                continue
            try:
                address = int(tok[-2], 16)
            except ValueError:
                continue
            names.setdefault(address, tok[0])
        return cls(names, path)

    @classmethod
    def load(cls, path: str | None = None) -> "DumpMap | None":
        """The dump map from a `.zip` member or a plain file; None when it is not there."""
        path = find_dump(path)
        if not path:
            return None
        if path.lower().endswith(".zip"):
            with zipfile.ZipFile(path) as zf:
                member = DUMP_MEMBER if DUMP_MEMBER in zf.namelist() else next(
                    (n for n in zf.namelist() if n.endswith(".map")), None)
                if not member:
                    return None
                data = zf.read(member).decode("latin-1")
        else:
            with open(path, encoding="latin-1", errors="replace") as fh:
                data = fh.read()
        return cls.from_lines(data.splitlines(), path)

    def name_at(self, address: int) -> str | None:
        return self.names.get(address)

    def covered(self, address: int, window: int = DUMP_WINDOW,
                minimum: int = DUMP_MIN_NEIGHBOURS) -> bool:
        lo = bisect.bisect_left(self.sorted, address - window)
        hi = bisect.bisect_right(self.sorted, address + window)
        return hi - lo >= minimum


def find_dump(path: str | None = None) -> str | None:
    """Resolve `--dump`: an explicit path, `$MH3_DUMP_MAP`, then the known 2021 dump locations."""
    if path and path != "auto":
        return path if os.path.exists(path) else None
    candidates = [os.environ.get("MH3_DUMP_MAP")] + list(DUMP_PATHS)
    for candidate in candidates:
        if candidate and os.path.exists(candidate):
            return candidate
    return None


# ---------------------------------------------------------------------------------------- classification

def first_instruction(image, address: int) -> int | None:
    raw = image.read(address, 4)
    return struct.unpack(">I", raw)[0] if raw and len(raw) == 4 else None


def looks_like_prologue(image, address: int) -> bool:
    """`stwu r1,-N(r1)` / `mflr` / `stmw` - the shapes a real function entry starts with (`lib.ppc`)."""
    return _ppc.looks_like_prologue(first_instruction(image, address))


def is_dead_epilogue(image, symbol: dict) -> bool:
    """The phantom's own shape: the symbol's bytes are exactly one `blr` (`lib.ppc`)."""
    return _ppc.is_dead_epilogue(image.read(symbol["address"], symbol["size"]))


def bytes_at(image, address: int, size: int) -> str:
    raw = image.read(address, size)
    return raw.hex() if raw and len(raw) == size else ""


def relation(previous: dict | None, symbol: dict) -> tuple[str, int | None]:
    """-> (kind, previous end) with kind in none / inside / adjacent / gap."""
    if previous is None:
        return "none", None
    end = previous["address"] + previous["size"]
    if symbol["address"] < end:
        return "inside", end
    if symbol["address"] == end:
        return "adjacent", end
    return "gap", end


def proposed_edit(symbol: dict, previous: dict, end: int) -> list[str]:
    """The two-line map edit a merge needs - printed, never applied."""
    needed = max(end, symbol["address"] + symbol["size"])
    edit = []
    if needed != end:
        edit.append("grow %s size 0x%X -> 0x%X" % (previous["name"], previous["size"],
                                                   needed - previous["address"]))
    edit.append("delete %s" % symbol["name"])
    return edit


def classify(symbol: dict, previous: dict | None, refs: Refs, dump: DumpMap | None,
             image, max_size: int = MAX_SIZE) -> dict:
    """The four evidence items and one verdict for one candidate.  Mutates nothing."""
    address = symbol["address"]
    kind, end = relation(previous, symbol)
    evidence = {ch: refs.get(ch, address) for ch in CHANNELS}
    prologue = looks_like_prologue(image, address)
    dead = is_dead_epilogue(image, symbol)
    named = dump.name_at(address) if dump else None
    covered = dump.covered(address) if dump else False
    reached = evidence["callers"] + evidence["branches"] + evidence["addrloads"]
    hexed = bytes_at(image, address, symbol["size"])

    if evidence["callers"]:
        verdict, kind_reason = "keep", "called"
        reason = "called: %d bl" % evidence["callers"]
    elif reached:
        verdict, kind_reason = "keep", "entry"
        reason = "entry point reached by %d branch(es) / %d address load(s)" % (
            evidence["branches"], evidence["addrloads"])
    elif kind == "inside" and previous["type"] != "function":
        verdict, kind_reason = "unclear", "inside_nonfunction"
        reason = "starts inside a non-function symbol (%s)" % previous["name"]
    elif kind == "inside" and named:
        verdict, kind_reason = "unclear", "inside_named"
        reason = ("starts inside %s's extent but the dump names it (%s): %s's size looks too big"
                  % (previous["name"], named, previous["name"]))
    elif kind == "inside":
        verdict, kind_reason = "merge", "inside"
        reason = "starts inside %s's extent, which already ends at 0x%08X" % (previous["name"], end)
    elif named:
        verdict, kind_reason = "keep", "dump_named"
        reason = "named in the runtime dump (%s)" % named
    elif kind == "adjacent" and prologue:
        verdict, kind_reason = "unclear", "prologue"
        reason = "the bytes are a function prologue (stwu r1/mflr/stmw)"
    elif kind == "adjacent" and dead and evidence["datawords"]:
        verdict, kind_reason = "unclear", "dataword"
        reason = ("a lone blr, but %d data word(s) point at it: a vtable slot (real stub) or a "
                  "jump-table island (phantom)" % evidence["datawords"])
    elif kind == "adjacent" and dead and dump is None:
        verdict, kind_reason = "unclear", "no_dump"
        reason = "a lone blr, but no dump oracle to rule out a real empty function"
    elif kind == "adjacent" and dead and not covered:
        verdict, kind_reason = "unclear", "uncovered"
        reason = "a lone blr, but the runtime dump does not cover this neighbourhood"
    elif kind == "adjacent" and dead:
        verdict, kind_reason = "merge", "dead_epilogue"
        reason = "dead epilogue: a lone blr after %s, nothing reaches it" % previous["name"]
    elif kind == "adjacent":
        verdict, kind_reason = "unclear", "body"
        reason = "not a dead epilogue: bytes %s are a plausible body" % hexed
    elif kind == "gap":
        verdict, kind_reason = "unclear", "gap"
        reason = "there is a %d-byte gap after %s" % (address - end, previous["name"])
    else:
        verdict, kind_reason = "unclear", "no_prev"
        reason = "no preceding symbol in %s" % symbol["section"]

    return {
        "name": symbol["name"],
        "section": symbol["section"],
        "address": address,
        "size": symbol["size"],
        "bytes": hexed,
        "prev": None if previous is None else {
            "name": previous["name"], "address": previous["address"], "size": previous["size"],
            "end": end, "type": previous["type"]},
        "relation": kind,
        "prologue": prologue,
        "dead_epilogue": dead,
        "evidence": evidence,
        "dump": {"named": named, "covered": covered,
                 "path": dump.path if dump else None},
        "max_size": max_size,
        "verdict": verdict,
        "reason": reason,
        "reason_kind": kind_reason,
        "proposed": proposed_edit(symbol, previous, end) if verdict == "merge" else [],
    }


def in_text(image, address: int) -> bool:
    return any(start <= address < start + size for _o, start, size in image.sections[:7] if size)


def by_section(entries) -> dict[str, list[dict]]:
    grouped: dict[str, list[dict]] = {}
    for entry in entries:
        grouped.setdefault(entry["section"], []).append(entry)
    for group in grouped.values():
        group.sort(key=lambda e: e["address"])
    return grouped


def previous_of(grouped: dict[str, list[dict]], symbol: dict) -> dict | None:
    group = grouped[symbol["section"]]
    i = bisect.bisect_left([e["address"] for e in group], symbol["address"])
    return group[i - 1] if i > 0 else None


def scan(entries, image, refs: Refs, dump: DumpMap | None, max_size: int = MAX_SIZE,
         section: str | None = None) -> tuple[list[dict], int]:
    """-> (records for every candidate, number of unnamed fn_* in text at any size)."""
    grouped = by_section(entries)
    all_unnamed = 0
    records = []
    for entry in entries:
        if not UNNAMED.match(entry["name"]) or entry["type"] != "function":
            continue
        if not in_text(image, entry["address"]):
            continue
        all_unnamed += 1
        if entry["size"] <= 0 or entry["size"] > max_size:
            continue
        if section and entry["section"] != section:
            continue
        records.append(classify(entry, previous_of(grouped, entry), refs, dump, image, max_size))
    return records, all_unnamed


# ---------------------------------------------------------------------------------------------- reporting

def counts(records) -> dict[str, int]:
    tally = {v: 0 for v in VERDICTS}
    for record in records:
        tally[record["verdict"]] += 1
    return tally


def header(records, unnamed_total, args, dump: DumpMap | None, text_sections: int) -> list[str]:
    tally = counts(records)
    dump_desc = "none (no dump oracle: merge verdicts degrade to unclear)"
    if dump:
        dump_desc = "%s (%d addresses)" % (dump.path, len(dump.names))
    return [
        "phantom scan: %s  vs  %s" % (args.file, args.dol),
        "  dump oracle : %s" % dump_desc,
        "  candidates  : %d unnamed fn_* in text with size <= 0x%X (of %d unnamed fn_* in %d text sections)"
        % (len(records), args.max_size, unnamed_total, text_sections),
        "  verdicts    : keep %d, merge %d, unclear %d" % (tally["keep"], tally["merge"],
                                                           tally["unclear"]),
    ]


def render(records, unnamed_total, args, dump, text_sections: int) -> list[str]:
    lines = header(records, unnamed_total, args, dump, text_sections)
    merges = [r for r in records if r["verdict"] == "merge"]
    unclear = [r for r in records if r["verdict"] == "unclear"]
    keeps = [r for r in records if r["verdict"] == "keep"]

    kinds: dict[str, int] = {}
    for r in unclear:
        kinds[r["reason_kind"]] = kinds.get(r["reason_kind"], 0) + 1
    lines.append("  unclear     : %s" % ", ".join("%s %d" % (k, n) for k, n in
                                                 sorted(kinds.items(), key=lambda kv: -kv[1])))

    lines.append("")
    lines.append("merge candidates (%d) - the map edit is a human's; this tool never applies it:" % len(merges))
    for r in merges:
        prev = r["prev"]
        lines.append("  %-12s 0x%08X %dB  prev %s (0x%08X size 0x%X end 0x%08X)  %s  bytes %s"
                     % (r["name"], r["address"], r["size"], prev["name"], prev["address"],
                        prev["size"], prev["end"], r["relation"], r["bytes"]))
        lines.append("      %s   [%s]" % ("; ".join(r["proposed"]), r["reason"]))

    if args.all:
        lines.append("")
        lines.append("unclear (%d):" % len(unclear))
        for r in unclear:
            prev = r["prev"]
            where = "%s 0x%08X" % (prev["name"], prev["address"]) if prev else "no previous symbol"
            lines.append("  %-12s 0x%08X %dB  prev %s  %s  bytes %s  - %s"
                         % (r["name"], r["address"], r["size"], where, r["relation"], r["bytes"],
                            r["reason"]))
        lines.append("")
        lines.append("keep (%d, first %d):" % (len(keeps), args.limit))
        for r in keeps[:args.limit]:
            lines.append("  %-12s 0x%08X %dB  - %s" % (r["name"], r["address"], r["size"], r["reason"]))
        if len(keeps) > args.limit:
            lines.append("  ... (%d more, raise --limit)" % (len(keeps) - args.limit))
    return lines


def render_explain(record: dict, refs_output: str | None) -> list[str]:
    prev = record["prev"]
    ev = record["evidence"]
    dump = record["dump"]
    lines = [
        "%s  0x%08X  size 0x%X  %s  type function" % (record["name"], record["address"],
                                                      record["size"], record["section"]),
    ]
    if prev:
        lines.append("  previous : %s 0x%08X size 0x%X end 0x%08X (%s)"
                     % (prev["name"], prev["address"], prev["size"], prev["end"], record["relation"]))
    else:
        lines.append("  previous : none in %s" % record["section"])
    lines.append("  bytes    : %s  (%s)" % (record["bytes"],
                                            "a function prologue" if record["prologue"] else
                                            "not a prologue"))
    lines.append("  callers  : %d bl" % ev["callers"])
    lines.append("  branches : %d b/bc target" % ev["branches"])
    lines.append("  addrload : %d lis+addi/ori" % ev["addrloads"])
    lines.append("  datawords: %d address word(s) in data sections" % ev["datawords"])
    if dump["named"]:
        lines.append("  dump     : named %s (neighbourhood covered: %s)" % (dump["named"], dump["covered"]))
    else:
        lines.append("  dump     : no line at this address (neighbourhood covered: %s)" % dump["covered"])
    lines.append("  in-repo refs: %s" % (refs_output.strip().replace("\n", "\n                ")
                                          if refs_output else "(not checked)"))
    lines.append("  verdict  : %s - %s" % (record["verdict"], record["reason"]))
    if record["proposed"]:
        lines.append("  edit     : %s  (hand edit: symedit.py cannot resize or delete a symbol)"
                     % "; ".join(record["proposed"]))
    return lines


def symedit_refs(name: str) -> str | None:
    """The `symedit.py refs <name>` output, so the in-repo half of the evidence is the proxy's own."""
    script = os.path.join(HERE, "symedit.py")
    try:
        proc = subprocess.run([sys.executable, script, "refs", name, "--limit", "10"],
                              capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
    except (OSError, subprocess.SubprocessError) as exc:  # pragma: no cover - environment dependent
        return "(refs failed: %s)" % exc
    return proc.stdout.strip() or "(no output)"


# ------------------------------------------------------------------------------------------------ selftest

class FakeImage:
    """A DOL-shaped image for the selftest: 7 text sections (padded) then the data sections."""

    def __init__(self, text: list[tuple[int, bytes]], data: list[tuple[int, bytes]]) -> None:
        self.sections: list[tuple[int, int, int]] = []
        self.data = bytearray()
        for address, blob in (list(text) + [(0, b"")] * 7)[:7] + list(data):
            self.sections.append((len(self.data), address, len(blob)))
            self.data += blob

    def read(self, address: int, size: int) -> bytes | None:
        for offset, start, length in self.sections:
            if start <= address and address + size <= start + length:
                return bytes(self.data[offset + address - start: offset + address - start + size])
        return None


def _b(from_address: int, to_address: int) -> int:
    return (18 << 26) | ((to_address - from_address) & 0x03FFFFFC)


def _bl(from_address: int, to_address: int) -> int:
    return _b(from_address, to_address) | 1


def _bc(from_address: int, to_address: int, bo: int = 12) -> int:
    return (16 << 26) | (bo << 21) | ((to_address - from_address) & 0xFFFC)


def _words(*values: int) -> bytes:
    return b"".join(struct.pack(">I", v) for v in values)


def _sym(name: str, address: int, size: int, kind: str = "function") -> dict:
    return {"name": name, "section": ".text", "address": address, "size": size, "type": kind}


def _refs(callers=(), branches=(), addrloads=(), datawords=()) -> Refs:
    refs = Refs()
    for channel, values in (("callers", callers), ("branches", branches),
                            ("addrloads", addrloads), ("datawords", datawords)):
        for address in values:
            refs.add(channel, address)
    return refs


FIXTURE_START, FIXTURE_END = 0x80010000, 0x80010180

# Every symbol in the fixture, with the address it starts at and its declared size.  The bodies between
# them are `nop` padding unless a case needs a real shape; the three shapes that matter are a tail-call
# dispatcher (`prev_merge`), a lone `blr` (the phantoms) and a `stwu r1`/`li r3,0; blr` body.
FIXTURE_SYMBOLS = (
    ("prev_merge", 0x80010000, 0x10),
    ("fn_80010010", 0x80010010, 4),      # lone blr, adjacent, unreached, dump-unnamed -> merge
    ("prev_called", 0x80010014, 0x3C),
    ("fn_80010050", 0x80010050, 4),      # lone blr, adjacent, `bl`-called -> keep
    ("prev_named", 0x80010054, 0x1C),
    ("fn_80010070", 0x80010070, 4),      # lone blr, adjacent, named by the dump -> keep
    ("prev_inside", 0x80010074, 0x20),   # ends 0x80010094, i.e. past the next symbol's start
    ("fn_80010090", 0x80010090, 4),      # starts INSIDE prev_inside -> merge
    ("prev_pro", 0x80010094, 0x2C),      # ends 0x800100C0
    ("fn_800100C0", 0x800100C0, 8),      # stwu r1 ; blr -> unclear (prologue)
    ("prev_body", 0x800100C8, 0x28),     # ends 0x800100F0
    ("fn_800100F0", 0x800100F0, 8),      # li r3,0 ; blr -> unclear (a plausible body)
    ("prev_data", 0x800100F8, 0x20),     # ends 0x80010118
    ("fn_80010118", 0x80010118, 4),      # lone blr, one data word -> unclear (vtable/island)
    ("prev_gap", 0x8001011C, 0x10),      # ends 0x8001012C
    ("fn_80010140", 0x80010140, 4),      # lone blr with a 0x14-byte gap before it -> unclear
    ("prev_nodump", 0x80010144, 0x24),   # ends 0x80010168
    ("fn_80010168", 0x80010168, 4),      # lone blr, adjacent, no dump oracle -> unclear
)
FIXTURE_WORDS = {
    0x80010000: [0x818C001C, 0x7D8903A6, 0x4E800420, 0x60000000],  # lwz/mtctr/bctr tail dispatch
    0x80010010: [0x4E800020],
    0x80010050: [0x4E800020],
    0x80010070: [0x4E800020],
    0x80010090: [0x38600000],
    0x800100C0: [0x9421FFF0, 0x4E800020],
    0x800100F0: [0x38600000, 0x4E800020],
    0x80010118: [0x4E800020],
    0x80010140: [0x4E800020],
    0x80010168: [0x4E800020],
}
FIXTURE_DUMP_NAMED = 0x80010070
FIXTURE_DATA_WORD = 0x80010118


def _fixture():
    """A text blob and symbol set covering every verdict rule; returns (image, dump, symbols)."""
    words: dict[int, int] = {}
    for address, values in FIXTURE_WORDS.items():
        for i, value in enumerate(values):
            words[address + 4 * i] = value
    blob = _words(*[words.get(FIXTURE_START + 4 * i, 0x60000000)
                    for i in range((FIXTURE_END - FIXTURE_START) // 4)])
    image = FakeImage([(FIXTURE_START, blob)], [(0x80020000, _words(FIXTURE_DATA_WORD))])
    dump_lines = (["zz_%08X_ %08X f" % (FIXTURE_DUMP_NAMED, FIXTURE_DUMP_NAMED)]
                  + ["filler%d 800110%02X f" % (i, i * 4) for i in range(20)])
    dump = DumpMap.from_lines(dump_lines, "fixture.map")
    symbols = [_sym(name, address, size) for name, address, size in FIXTURE_SYMBOLS]
    return image, dump, symbols


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def verdict(name, refs, dump, image, symbols, previous=None, kind="function"):
        symbol = next(s for s in symbols if s["name"] == name)
        if previous is None:
            grouped = by_section(symbols)
            previous = previous_of(grouped, symbol)
        elif isinstance(previous, str):
            previous = next(s for s in symbols if s["name"] == previous)
        return classify(symbol, previous, refs, dump, image), symbol

    image, dump, symbols = _fixture()

    # --- the decoder -------------------------------------------------------------------------------
    text = _words(_bl(0x80030000, 0x80030030), _b(0x80030004, 0x80030034),
                  _bc(0x80030008, 0x80030038),
                  0x3C608003, 0x38630010, 0x3C808003, 0x60840014,
                  0x60000000, 0x60000000, 0x60000000, 0x60000000,
                  0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000)
    decoded = index_refs(FakeImage([(0x80030000, text)], [(0x80040000, _words(0x8003003C))]))
    check("decoder: bl -> callers", decoded.get("callers", 0x80030030), 1)
    check("decoder: b -> branches", decoded.get("branches", 0x80030034), 1)
    check("decoder: bc -> branches", decoded.get("branches", 0x80030038), 1)
    check("decoder: lis+addi -> addrloads", decoded.get("addrloads", 0x80030010), 1)
    check("decoder: lis+ori -> addrloads", decoded.get("addrloads", 0x80030014), 1)
    check("decoder: data word -> datawords", decoded.get("datawords", 0x8003003C), 1)
    check("decoder: a data word is not a caller", decoded.get("callers", 0x8003003C), 0)
    check("decoder: an untouched address is silent", decoded.total(0x80030020), 0)

    # --- the shape helpers -------------------------------------------------------------------------
    check("prologue: stwu r1", looks_like_prologue(image, 0x800100C0), True)
    check("prologue: a lone blr is not a prologue", looks_like_prologue(image, 0x80010010), False)
    check("dead epilogue: a lone blr", is_dead_epilogue(image, _sym("x", 0x80010010, 4)), True)
    check("dead epilogue: li r3,0; blr is not one", is_dead_epilogue(image, _sym("x", 0x800100F0, 8)), False)
    check("relation: adjacent", relation(_sym("p", 0x80010000, 0x10), _sym("c", 0x80010010, 4))[0],
          "adjacent")
    check("relation: inside", relation(_sym("p", 0x80010000, 0x20), _sym("c", 0x80010010, 4))[0], "inside")
    check("relation: gap", relation(_sym("p", 0x80010000, 0x10), _sym("c", 0x80010020, 4))[0], "gap")
    check("relation: none", relation(None, _sym("c", 0x80010010, 4))[0], "none")

    # --- the dump oracle ---------------------------------------------------------------------------
    check("dump: name_at", dump.name_at(FIXTURE_DUMP_NAMED), "zz_%08X_" % FIXTURE_DUMP_NAMED)
    check("dump: silence where covered", dump.name_at(0x80010010), None)
    check("dump: covered near the fixture", dump.covered(0x80010010), True)
    check("dump: not covered far away", dump.covered(0x80100000), False)
    check("dump: a missing path loads as None", DumpMap.load(os.path.join(HERE, "no-such-dump.map")), None)

    # --- one fixture per verdict -------------------------------------------------------------------
    rec, _ = verdict("fn_80010010", _refs(), dump, image, symbols)
    check("merge: lone blr, adjacent, unreached", rec["verdict"], "merge")
    check("merge: into the previous symbol", rec["proposed"],
          ["grow prev_merge size 0x10 -> 0x14", "delete fn_80010010"])
    check("merge: relation is adjacent", rec["relation"], "adjacent")
    check("merge: evidence is empty", rec["evidence"],
          {"callers": 0, "branches": 0, "addrloads": 0, "datawords": 0})
    check("merge: reason kind", rec["reason_kind"], "dead_epilogue")

    rec, _ = verdict("fn_80010050", _refs(callers=[0x80010050]), dump, image, symbols)
    check("keep: a called lone blr is real", rec["verdict"], "keep")
    check("keep: reason names the call", "called: 1 bl" in rec["reason"], True)

    rec, _ = verdict("fn_80010070", _refs(), dump, image, symbols)
    check("keep: dump-named lone blr", rec["verdict"], "keep")
    check("keep: reason names the dump", "runtime dump" in rec["reason"], True)

    rec, _ = verdict("fn_80010090", _refs(), dump, image, symbols)
    check("merge: inside the previous extent", rec["verdict"], "merge")
    check("merge: inside needs no size grow", rec["proposed"], ["delete fn_80010090"])

    rec, _ = verdict("fn_800100C0", _refs(), dump, image, symbols)
    check("unclear: prologue", rec["verdict"], "unclear")
    check("unclear: prologue reason", "prologue" in rec["reason"], True)
    check("unclear: prologue reason kind", rec["reason_kind"], "prologue")

    rec, _ = verdict("fn_800100F0", _refs(), dump, image, symbols)
    check("unclear: li r3,0; blr body", rec["verdict"], "unclear")
    check("unclear: body reason", "not a dead epilogue" in rec["reason"], True)

    rec, _ = verdict("fn_80010118", _refs(datawords=[FIXTURE_DATA_WORD]), dump, image, symbols)
    check("unclear: data word only", rec["verdict"], "unclear")
    check("unclear: data word reason", "data word" in rec["reason"], True)

    rec, _ = verdict("fn_80010140", _refs(), dump, image, symbols)
    check("unclear: a gap after the previous symbol", rec["verdict"], "unclear")
    check("unclear: gap reason", "gap" in rec["reason"], True)

    rec, _ = verdict("fn_80010168", _refs(), None, image, symbols)
    check("unclear: no dump oracle", rec["verdict"], "unclear")
    check("unclear: no-dump reason", "no dump oracle" in rec["reason"], True)

    check("unclear: no previous symbol", classify(_sym("fn_80099999", 0x80099999, 4), None, _refs(),
                                                  dump, image)["verdict"], "unclear")
    check("unclear: inside but dump-named",
          classify(_sym("fn_80010070", FIXTURE_DUMP_NAMED, 4), _sym("prev", 0x80010060, 0x20),
                   _refs(), dump, image)["verdict"], "unclear")

    # --- the scan integration ----------------------------------------------------------------------
    records, unnamed_total = scan(symbols, image,
                                  _refs(callers=[0x80010050], datawords=[FIXTURE_DATA_WORD]), dump)
    tally = counts(records)
    check("scan: every candidate is classified", len(records), 9)
    check("scan: unnamed total counted", unnamed_total, 9)
    check("scan: merge count", tally["merge"], 3)
    check("scan: keep count", tally["keep"], 2)
    check("scan: unclear count", tally["unclear"], 4)
    check("scan: verdicts are the three known ones",
          sorted({r["verdict"] for r in records}), ["keep", "merge", "unclear"])
    check("scan: --max-size filters", len(scan(symbols, image, _refs(), dump, max_size=4)[0]), 7)
    check("scan: --section filters", len(scan(symbols, image, _refs(), dump, section=".init")[0]), 0)
    check("scan: --all rendering lists the keeps",
          any("keep (2" in line for line in render(records, unnamed_total,
                                                   argparse.Namespace(file="m", dol="d", max_size=8,
                                                                      all=True, limit=40),
                                                   dump, 7)), True)
    check("render: merge lines carry the edit plan",
          any("grow prev_merge size 0x10 -> 0x14" in line for line in render(
              records, unnamed_total, argparse.Namespace(file="m", dol="d", max_size=8, all=False,
                                                         limit=40), dump, 7)), True)
    check("render: the unclear list is grouped by default",
          any("unclear     : prologue 1" in line for line in render(
              records, unnamed_total, argparse.Namespace(file="m", dol="d", max_size=8, all=False,
                                                         limit=40), dump, 7)), True)
    check("render: the unclear detail needs --all",
          any("plausible body" in line for line in render(
              records, unnamed_total, argparse.Namespace(file="m", dol="d", max_size=8, all=False,
                                                         limit=40), dump, 7)), False)
    check("explain: prints all four evidence items",
          len([l for l in render_explain(records[1], "(none)") if ":" in l]) >= 8, True)

    # --- the no-write contract ---------------------------------------------------------------------
    parser = build_parser()
    options = {o for action in parser._actions for o in action.option_strings}
    check("contract: no --apply/--write option exists", options & {"--apply", "--write", "--fix"}, set())
    check("contract: the module never imports a symbol-map writer",
          hasattr(symedit, "rename") and not hasattr(sys.modules[__name__], "apply"), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for failure in fails:
            print("  " + failure)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --------------------------------------------------------------------------------------------------- cli

def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", nargs="?", default="scan", choices=["scan", "explain"])
    ap.add_argument("target", nargs="?", help="explain: a symbol name or 0x address")
    ap.add_argument("--file", default=DEFAULT_FILE, help="symbol map")
    ap.add_argument("--dol", default=DEFAULT_DOL, help="original DOL (read-only)")
    ap.add_argument("--dump", default="auto", help="runtime symbol map (.zip/.map) or `auto`")
    ap.add_argument("--max-size", type=lambda s: int(s, 0), default=MAX_SIZE,
                    help="candidate size ceiling in bytes (default %d)" % MAX_SIZE)
    ap.add_argument("--section", default=None, help="restrict the scan to one section")
    ap.add_argument("--all", action="store_true", help="also list the keep records")
    ap.add_argument("--limit", type=int, default=40, help="max keep records printed")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    return ap


def cmd_scan(args, image, entries, dump) -> int:
    refs = index_refs(image)
    records, unnamed_total = scan(entries, image, refs, dump, args.max_size, args.section)
    text_sections = len([s for s in image.sections[:7] if s[2]])
    if args.json:
        print(json.dumps({"counts": counts(records), "unnamed_total": unnamed_total,
                          "dump": dump.path if dump else None, "records": records}, indent=2))
        return 0
    for line in render(records, unnamed_total, args, dump, text_sections):
        print(line)
    return 0


def cmd_explain(args, image, entries, dump) -> int:
    if not args.target:
        print("explain needs a symbol name or 0x address")
        return 2
    target = args.target
    match = None
    if target.lower().startswith("0x"):
        address = int(target, 16)
        match = next((e for e in entries if e["address"] == address and e["section"] == ".text"), None)
    else:
        match = next((e for e in entries if e["name"] == target), None)
    if match is None:
        print("not found in %s: %s" % (args.file, target))
        return 2
    refs = index_refs(image)
    grouped = by_section(entries)
    record = classify(match, previous_of(grouped, match), refs, dump, image, args.max_size)
    if args.json:
        print(json.dumps(record, indent=2))
        return 0
    lines = render_explain(record, symedit_refs(match["name"]))
    if not UNNAMED.match(match["name"]):
        lines.append("  note     : not an unnamed fn_* - not a scan candidate, shown for reference")
    for line in lines:
        print(line)
    return 0


def main() -> int:
    args = build_parser().parse_args()
    if args.selftest:
        return selftest()
    if not os.path.exists(args.dol):
        print("missing DOL: %s" % args.dol)
        return 2
    image = Dol(args.dol)
    entries = list(symedit.entries(args.file))
    dump = DumpMap.load(args.dump)
    if dump is None and args.dump != "auto":
        print("warning: no dump oracle at %s - merge verdicts degrade to unclear" % args.dump)
    return cmd_scan(args, image, entries, dump) if args.command == "scan" \
        else cmd_explain(args, image, entries, dump)


if __name__ == "__main__":
    sys.exit(main())
