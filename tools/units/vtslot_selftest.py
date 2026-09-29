#!/usr/bin/env python3
"""Self-test for `tools/units/vtslot.py` - the reverse of `vtableaudit --at`.

The two acceptance cases are pinned here as fixtures, because they are the two ways this tool can lie:

* **a vtable slot is found** - the word equals the function's address, it sits in a run of code pointers,
  the two RTTI words before the run give the table base the transport lane numbered its slots from, and the
  neighbouring slots resolve to their owning unit and map symbol;
* **a plain function reports no hits** - not a false positive from an unaligned overlap, not a `.text`
  instruction immediate, not a data pointer mislabelled as a code-pointer run.

The fixtures are a hand-built DOL blob (one `.text` section, one `.data` section) and a hand-written
`splits.txt`/`symbols.txt`; no real `src/`, `build/`, `config/` or DOL is read or written. The end-to-end
CLI check runs the tool against a temp tree and confirms every fixture file is byte-identical afterwards.

    python tools/units/vtslot_selftest.py
    python tools/units/vtslot.py --selftest
"""

from __future__ import annotations

import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vtableaudit as va  # noqa: E402
import vtslot as vs  # noqa: E402

TEXT_ADDR = 0x80004000
DATA_ADDR = 0x80500000

SYMBOLS = """fn_80004000 = .text:0x80004000; // type:function size:0x10
fn_80004010 = .text:0x80004010; // type:function size:0x10
fn_80004020 = .text:0x80004020; // type:function size:0x10
fn_80004030 = .text:0x80004030; // type:function size:0x10
UnitVTable = .data:0x80500000; // type:object size:0x18 scope:local
"""

# `t/unit.cpp` owns the table (0x80500000..0x80500018) and a lone pointer at 0x80500018; everything past
# 0x8050001C is an unregistered band - the state the transport unit's real .data is still in.
SPLITS = """Sections:
\t.text       type:code align:32
\t.data       type:data align:32

t/unit.cpp:
\t.text       start:0x80004000 end:0x80004100
\t.data       start:0x80500000 end:0x8050001C
"""


def fixture_dol() -> bytes:
    """A DOL with a `.text` at 0x80004000 and a `.data` at 0x80500000.

    `.data` holds: two RTTI zeros, a three-word code-pointer run (0x80004000/0x80004010/0x80004020), a
    zero, a lone code pointer (0x80004020), a non-code word, a **big-endian 0x80004010 written at the
    unaligned offset +0x22** (the overlap a naive search must not report), and a pointer to the table base
    at +0x28. `.text` holds 0x80004030 as an instruction immediate: the scan never looks at `.text`, so a
    function that appears only there has no hits.
    """
    header = bytearray(0x100)
    struct.pack_into(">I", header, 0x00, 0x100)          # text[0] file offset
    struct.pack_into(">I", header, 0x1C, 0x200)          # data[0] file offset
    struct.pack_into(">I", header, 0x48, TEXT_ADDR)      # text[0] address
    struct.pack_into(">I", header, 0x64, DATA_ADDR)      # data[0] address
    struct.pack_into(">I", header, 0x90, 0x100)          # text[0] size
    struct.pack_into(">I", header, 0xAC, 0x100)          # data[0] size
    text = bytearray(0x100)
    struct.pack_into(">I", text, 0x80, 0x80004030)       # an address in .text: never a hit
    data = bytearray(0x100)
    struct.pack_into(">I", data, 0x08, 0x80004000)       # run start
    struct.pack_into(">I", data, 0x0C, 0x80004010)       # <- the table slot queried
    struct.pack_into(">I", data, 0x10, 0x80004020)
    struct.pack_into(">I", data, 0x18, 0x80004020)       # lone pointer
    struct.pack_into(">I", data, 0x1C, 0x12345678)
    data[0x20:0x22] = b"\x80\x00"                        # word@0x20 = 0x80008000
    data[0x22:0x26] = b"\x80\x00\x40\x10"                # unaligned word@0x22 = 0x80004010 (a trap)
    data[0x24:0x28] = b"\x40\x10\x00\x00"                # word@0x24 = 0x40100000
    struct.pack_into(">I", data, 0x28, DATA_ADDR)        # a data pointer to the table base
    return bytes(header) + bytes(text) + bytes(data)


def fixture_tree(blob: bytes | None = None) -> dict:
    """The `vtableaudit.load_tree`-shaped dict the pure functions need, from the fixture text."""
    blob = fixture_dol() if blob is None else blob
    return {"splits": va.parse_splits(SPLITS), "symbols": va.parse_symbols(SYMBOLS),
            "text_ranges": va.dol_text_ranges(blob), "blob": blob, "text_source": "dol"}


def make_tree(root: str) -> str:
    paths = {
        "splits": os.path.join(root, "config", "RMHE08", "splits.txt"),
        "symbols": os.path.join(root, "config", "RMHE08", "symbols.txt"),
        "dol": os.path.join(root, "orig", "RMHE08", "sys", "main.dol"),
    }
    for p in paths.values():
        os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(paths["splits"], "w", encoding="utf-8") as fh:
        fh.write(SPLITS)
    with open(paths["symbols"], "w", encoding="utf-8") as fh:
        fh.write(SYMBOLS)
    with open(paths["dol"], "wb") as fh:
        fh.write(fixture_dol())
    return root


def tree_digest(root: str) -> dict:
    out = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(dirnames)
        for fn in sorted(filenames):
            p = os.path.join(dirpath, fn)
            with open(p, "rb") as fh:
                out[os.path.relpath(p, root).replace("\\", "/")] = hashlib.sha1(fh.read()).hexdigest()
    return out


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    blob = fixture_dol()
    tree = fixture_tree(blob)

    # -- the word scan --------------------------------------------------------------------------
    segments = vs.data_segments(blob)
    check("only the DOL's data sections are scanned", list(segments), ["data0"])
    check("the data section's base, size and file offset",
          segments["data0"], (DATA_ADDR, 0x100, 0x200))
    check("a code word is found at its 4-aligned slot",
          vs.find_word_hits(blob, 0x80004010), [("data0", DATA_ADDR + 0x0C)])
    check("an unaligned overlap is NOT a 4-byte location",
          vs.find_word_hits(blob, 0x80004010), [("data0", DATA_ADDR + 0x0C)])
    check("a value only in .text is never a hit",
          vs.find_word_hits(blob, 0x80004030), [])
    check("a data value is a hit (the caller labels it, not the scan)",
          vs.find_word_hits(blob, DATA_ADDR), [("data0", DATA_ADDR + 0x28)])
    check("an address stored nowhere has no hits",
          vs.find_word_hits(blob, 0x80009999), [])

    # -- the band --------------------------------------------------------------------------------
    check("a registered range is returned with its unit and section",
          vs.containing_range(tree["splits"], DATA_ADDR + 0x0C),
          ("t/unit.cpp", {"section": ".data", "start": DATA_ADDR, "end": DATA_ADDR + 0x1C,
                          "object": ".data"}))
    check("a band no range owns is None",
          vs.containing_range(tree["splits"], DATA_ADDR + 0x28), (None, None))

    # -- the map symbol --------------------------------------------------------------------------
    check("an exact map symbol is resolved", vs.map_symbol(tree, DATA_ADDR), "UnitVTable")
    check("an address the map does not name is None", vs.map_symbol(tree, DATA_ADDR + 0x40), None)

    # -- the run and the table base --------------------------------------------------------------
    run = vs.run_bounds(tree, blob, DATA_ADDR + 0x0C)
    check("the run containing the slot is the three-word block",
          (run["start"], run["words"], run["bounded_by"], run["truncated"]),
          (DATA_ADDR + 0x08, 3, "range", False))
    check("a lone code pointer is a one-word run", vs.run_bounds(tree, blob, DATA_ADDR + 0x18)["words"], 1)
    check("a non-code location still has a (one-word) flags run at itself",
          vs.run_bounds(tree, blob, DATA_ADDR + 0x1C), None)
    check("the two RTTI zeros are read as the base",
          vs.rtti_base(tree, blob, run, DATA_ADDR + 0x0C), DATA_ADDR)
    no_rtti = {"start": DATA_ADDR + 0x08, "words": 3, "segment": "data0",
               "truncated": False, "bounded_by": "range"}
    check("no zeros before the run means the run base is the base",
          vs.rtti_base(tree, {"splits": {}, "text_ranges": tree["text_ranges"]}, no_rtti, DATA_ADDR + 0x08),
          DATA_ADDR + 0x08)

    # -- locate: the slot ------------------------------------------------------------------------
    hits = vs.locate(tree, blob, 0x80004010)
    check("the slot has exactly one hit", len(hits), 1)
    hit = hits[0]
    check("the hit is registered to the owning unit",
          (hit["band"], hit["unit"], hit["section"]), ("registered", "t/unit", ".data"))
    check("the hit is a table", hit["kind"], "table")
    check("the table base includes the two RTTI words",
          (hit["table"]["base"], hit["table"]["with_rtti"]), (DATA_ADDR, True))
    check("the table base is named from symbols.txt", hit["table"]["symbol"], "UnitVTable")
    check("the slot index and byte offset are measured from the base",
          (hit["table"]["slot_index"], hit["table"]["byte_offset"]), (3, 0x0C))
    check("the run is reported under the RTTI base",
          (hit["table"]["run_base"], hit["table"]["run_words"],
           hit["table"]["bounded_by"], hit["table"]["truncated"]),
          (DATA_ADDR + 0x08, 3, "range", False))
    check("the neighbouring slots resolve to owners and symbols",
          [(s["index"], s["target"], s["owner"], s["symbol"]) for s in hit["table"]["slots"]],
          [(0, 0, None, None), (1, 0, None, None),
           (2, 0x80004000, "t/unit", "fn_80004000"),
           (3, 0x80004010, "t/unit", "fn_80004010"),
           (4, 0x80004020, "t/unit", "fn_80004020")])
    check("the queried slot is in the neighbour list",
          [s["index"] for s in hit["table"]["slots"] if s["address"] == hit["location"]], [3])

    # -- locate: the two acceptance negatives ----------------------------------------------------
    check("a plain function with no data reference reports no hits",
          vs.locate(tree, blob, 0x80004030), [])
    check("a lone pointer is not promoted to a table",
          [h["kind"] for h in vs.locate(tree, blob, 0x80004020)],
          ["table", "code-pointer"])
    check("... and the lone one carries no table",
          vs.locate(tree, blob, 0x80004020)[1]["table"], None)
    check("a data pointer is labelled as one, not as a table",
          [h["kind"] for h in vs.locate(tree, blob, DATA_ADDR)], ["data-pointer"])
    check("a data pointer names no table either",
          vs.locate(tree, blob, DATA_ADDR)[0]["table"], None)
    check("a data pointer in an unregistered band says so",
          vs.locate(tree, blob, DATA_ADDR)[0]["band"], "unregistered")

    # -- a run clipped by the search radius is marked, never guessed ------------------------------
    big_header = bytearray(0x100)
    struct.pack_into(">I", big_header, 0x00, 0x100)
    struct.pack_into(">I", big_header, 0x1C, 0x200)
    struct.pack_into(">I", big_header, 0x48, TEXT_ADDR)
    struct.pack_into(">I", big_header, 0x64, DATA_ADDR)
    struct.pack_into(">I", big_header, 0x90, 0x100)
    struct.pack_into(">I", big_header, 0xAC, 0x4000)
    big = bytes(big_header) + b"\x00" * 0x100 + struct.pack(">I", 0x80004000) * 0x1000
    clipped = vs.run_bounds({"splits": {}, "text_ranges": va.dol_text_ranges(big), "blob": big},
                            big, DATA_ADDR + 0x2000)
    check("a run the search radius clips says `truncated`", clipped["truncated"], True)
    check("... and names the cap as the bound", clipped["bounded_by"], "max")

    # -- a band scan -----------------------------------------------------------------------------
    wall = vs.scan_band_hits(blob, 0x80004000, 0x80004030)
    check("the band scan finds every stored pointer into it, and only 4-aligned ones",
          [(l, t) for _s, l, t in wall],
          [(DATA_ADDR + 0x08, 0x80004000), (DATA_ADDR + 0x0C, 0x80004010),
           (DATA_ADDR + 0x10, 0x80004020), (DATA_ADDR + 0x18, 0x80004020)])

    # -- end to end, over the fixture tree -------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        make_tree(tmp)
        before = tree_digest(tmp)
        res = vs.scan(tmp, [0x80004010, 0x80004030])
        check("scan: the DOL was read", res["dol_present"], True)
        check("scan: two addresses", [r["address"] for r in res["addresses"]],
              [0x80004010, 0x80004030])
        check("scan: the slot is found with its table",
              [(h["table"]["base"], h["table"]["slot_index"], h["table"]["byte_offset"])
               for h in res["addresses"][0]["hits"]], [(DATA_ADDR, 3, 0x0C)])
        check("scan: the plain function has no hits", res["addresses"][1]["hits"], [])
        check("scan: the first address is cold", res["addresses"][0]["cold"], True)
        check("scan: a warm median exists for two addresses",
              isinstance(res["warm_scan_s"], float), True)
        check("scan: the raw scan is timed apart from the enrichment",
              ("raw_s" in res["addresses"][0] and "enrich_s" in res["addresses"][0]), True)
        check("every fixture file is byte-identical afterwards", tree_digest(tmp), before)

        # the CLI: text and JSON, and the exit status of a no-hit address is still 0
        tool = os.path.join(os.path.dirname(os.path.abspath(__file__)), "vtslot.py")
        p = subprocess.run([sys.executable, tool, "--main", tmp, "0x80004010"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("the CLI prints the table base, the slot and the offset",
              (p.returncode, "TABLE 0x80500000" in p.stdout and "slot 3 (byte +0xC)" in p.stdout),
              (0, True))
        check("the CLI names a neighbouring slot's owner",
              ("t/unit" in p.stdout and "fn_80004000" in p.stdout), True)
        p = subprocess.run([sys.executable, tool, "--main", tmp, "0x80004030"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("the CLI reports no hits for a plain function, and exits 0",
              (p.returncode, "no 4-byte data word" in p.stdout), (0, True))
        p = subprocess.run([sys.executable, tool, "--main", tmp, "--json", "0x80004010"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        payload = json.loads(p.stdout)
        check("--json is parseable and carries the table",
              (p.returncode, payload["addresses"][0]["hits"][0]["table"]["base"]), (0, DATA_ADDR))
        p = subprocess.run([sys.executable, tool, "--main", tmp, "--scan",
                            "0x80004000", "0x80004030"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("--scan reports the four pointers into the band",
              (p.returncode, p.stdout.count("-> ")), (0, 4))
        check("the CLI did not write into the tree", tree_digest(tmp), before)

        # a tree with no DOL is refused, per address, not guessed
        os.remove(os.path.join(tmp, "orig", "RMHE08", "sys", "main.dol"))
        res2 = vs.scan(tmp, [0x80004010])
        check("no DOL: the scan reports it cannot scan", res2["dol_present"], False)
        check("no DOL: the per-address note says so",
              "no DOL" in res2["addresses"][0]["note"], True)
        p = subprocess.run([sys.executable, tool, "--main", tmp, "0x80004010"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("no DOL on the CLI exits 2", p.returncode, 2)

    print("vtslot selftest: %d checks, %d failed" % (checks, len(fails)))
    for f in fails:
        print("  FAIL %s" % f)
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(selftest())
