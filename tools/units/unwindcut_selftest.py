#!/usr/bin/env python3
"""Fixtures-only selftest for tools/units/unwindcut.py.

    python tools/units/unwindcut_selftest.py
    python tools/units/unwindcut.py --selftest

No build, no `ninja`, no repository state: every fixture is a synthetic DOL (built by this file), a
`splits.txt`, and a `symbols.txt` in the system temp, so the contract is pinned:

* the re-cut's arithmetic - the full range re-absorbed from the unclaimed gap, the record split at the
  cut, the three section halves and the sum check;
* the two refusals that make the tool safe - a cut mid-function (no record and no symbol starts there),
  and a record run whose `etab_addr != extab base + 8*i`;
* the `.ctors`/`.dtors` rule - a word whose target leaves with the cut is named for dropping, a word
  whose target stays keeps its claim;
* the paste-ready text itself, so a formatting regression is a failure and not a surprise on a real cut.
"""
from __future__ import annotations

import contextlib
import io
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import unwindcut as uc  # noqa: E402  (imported through the sys.path shim above)

TEXT_BASE = 0x80001000
ETAB_BASE = 0x80010000
ETI_BASE = 0x80020000
CTOR_BASE = 0x80030000
DTOR_BASE = 0x80031000
CUT = 0x80001100
# 8 framed functions in the unit's full range: the first four end exactly at CUT, the last four are the
# released tail. A 9th record opens the next unit's claim, so a run can stop at its `fn_addr >= .text end`.
RECS = [
    (0x80001000, 0x40), (0x80001040, 0x40), (0x80001080, 0x40), (0x800010C0, 0x40),
    (0x80001100, 0x80), (0x80001180, 0x80), (0x80001200, 0x80), (0x80001280, 0x80),
]
SENTINEL = (0x80001400, 0x40)
ALL_RECS = RECS + [SENTINEL]

CHECK_COUNT = 0
FAILURES: list[str] = []


def _check(name: str, got, want) -> None:
    global CHECK_COUNT
    CHECK_COUNT += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


def _contains(name: str, needle: str, haystack: str) -> None:
    global CHECK_COUNT
    CHECK_COUNT += 1
    if needle not in haystack:
        FAILURES.append("%s: %r not found in %r" % (name, needle, haystack))


def build_dol(text_secs, data_secs) -> bytes:
    """A minimal valid DOL: [(address, bytes)] text sections (<=7) and data sections (<=11).

    The header is the retail layout `tudiscover.Dol` reads: offsets at 0x00/0x1C, addresses at
    0x48/0x64, sizes at 0x90/0xAC, BSS/entry at 0xE4/0xE8/0xEC.
    """
    if len(text_secs) > 7 or len(data_secs) > 11:
        raise ValueError("too many sections for a DOL")
    header = bytearray(0x100)
    blobs, offset = [], 0x100
    toff, doff = [], []
    for _addr, data in text_secs:
        offset += (-offset) % 4
        toff.append(offset)
        blobs.append((offset, data))
        offset += len(data)
    for _addr, data in data_secs:
        offset += (-offset) % 4
        doff.append(offset)
        blobs.append((offset, data))
        offset += len(data)

    def pad7(values):
        return tuple(values) + (0,) * (7 - len(values))

    def pad11(values):
        return tuple(values) + (0,) * (11 - len(values))

    struct.pack_into(">7I", header, 0x00, *pad7(toff))
    struct.pack_into(">11I", header, 0x1C, *pad11(doff))
    struct.pack_into(">7I", header, 0x48, *pad7([a for a, _d in text_secs]))
    struct.pack_into(">11I", header, 0x64, *pad11([a for a, _d in data_secs]))
    struct.pack_into(">7I", header, 0x90, *pad7([len(d) for _a, d in text_secs]))
    struct.pack_into(">11I", header, 0xAC, *pad11([len(d) for _a, d in data_secs]))
    struct.pack_into(">III", header, 0xE4, 0x80000000, 0, TEXT_BASE)   # bss address/size, entry point

    body = bytearray(header)
    for off, data in blobs:
        if len(body) < off:
            body.extend(b"\0" * (off - len(body)))
        body.extend(data)
    return bytes(body)


def records(extab_offsets=None, exti_offsets=None) -> bytes:
    """The 12-byte {fn, size, etab} run; the two override lists make a fixture break the invariant."""
    out = bytearray()
    for i, (fn, size) in enumerate(ALL_RECS):
        etab = ETAB_BASE + 8 * i
        if extab_offsets and i < len(extab_offsets) and extab_offsets[i] is not None:
            etab = extab_offsets[i]
        out.extend(struct.pack(">III", fn, size, etab))
    return bytes(out)


SPLITS = """Sections:
\t.init       type:code align:4
\textab       type:rodata align:32
\textabindex  type:rodata align:32
\t.text       type:code align:32
\t.ctors      type:rodata align:16
\t.dtors      type:rodata align:32

menu/menu_message.cpp:
\t.text       start:0x80001000 end:0x80001100
\textab       start:0x80010000 end:0x80010020
\textabindex  start:0x80020000 end:0x80020030
\t.ctors      start:0x80030000 end:0x80030008
\t.dtors      start:0x80031000 end:0x80031004

stage/stg_w.cpp:
\t.text       start:0x80001400 end:0x80001800
\textab       start:0x80010040 end:0x80010080
\textabindex  start:0x80020060 end:0x800200C0
"""


def symbols(prev_size=0x40) -> str:
    rows = []
    for i, (fn, size) in enumerate(ALL_RECS):
        real = prev_size if i == 3 else size     # i == 3 is the function that ends exactly at the cut
        rows.append("fn_%08X = .text:0x%08X; // type:function size:0x%X" % (fn, fn, real))
    return "\n".join(rows) + "\n"


def fixture(root: str, splits=SPLITS, symbols_text=None, extab_offsets=None, dol=None):
    """Write one fixture's files under `root` (a caller-owned temp dir) and return its three paths.

    Everything a run reads or writes lives under `root`, which the caller owns through a
    `tempfile.TemporaryDirectory`; nothing here resolves a path against the repository, so a selftest
    run cannot move the tree (the guard `tools/selftest.py` and `land.py` enforce).
    """
    cfg = os.path.join(root, "config", "RMHE08")
    os.makedirs(cfg, exist_ok=True)
    with open(os.path.join(cfg, "splits.txt"), "w", encoding="utf-8") as fh:
        fh.write(splits)
    with open(os.path.join(cfg, "symbols.txt"), "w", encoding="utf-8") as fh:
        fh.write(symbols_text if symbols_text is not None else symbols())
    img = dol if dol is not None else build_dol(
        [(TEXT_BASE, b"\x60\0\0\0" * (0x800 // 4))],
        [(ETAB_BASE, b"\0" * 0x80),
         (ETI_BASE, records(extab_offsets=extab_offsets)),
         (CTOR_BASE, struct.pack(">II", 0x80001040, 0x80001200)),
         (DTOR_BASE, struct.pack(">I", 0x80001000))])
    path = os.path.join(root, "main.dol")
    with open(path, "wb") as fh:
        fh.write(img)
    return path, os.path.join(cfg, "splits.txt"), os.path.join(cfg, "symbols.txt")


def selftest() -> int:
    # One caller-owned temp root for every fixture: `TemporaryDirectory` removes it on the way out, and
    # nothing below ever resolves a path against the repository - a selftest run must not move the tree.
    with tempfile.TemporaryDirectory(prefix="unwindcut-selftest-") as tmp:
        return _selftest(tmp)


def _selftest(tmp: str) -> int:
    # ------------------------------------------------------------------ the re-cut, the real shape
    dol, splits, syms = fixture(os.path.join(tmp, "real"))
    result = uc.analyse("menu/menu_message", CUT, splits, dol, syms)

    _check("the cut is the 5th record (index 4)", result["cut_index"], 4)
    _check("the full record run is 8", result["records"], 8)
    _check("the cut function is named", result["cut_function"]["name"], "fn_80001100")
    _check("the claim is the kept half's start", result["text"]["kept"], (0x80001000, CUT))
    _check("the released tail ends at the next claim", result["text"]["tail"], (CUT, 0x80001400))
    _check("the unclaimed gap was absorbed", result["absorbed"][".text"], (CUT, 0x80001400))
    _check("extab is split at 8 B * 4", result["extab"]["kept"], (ETAB_BASE, ETAB_BASE + 0x20))
    _check("extab tail", result["extab"]["tail"], (ETAB_BASE + 0x20, ETAB_BASE + 0x40))
    _check("extabindex is split at 12 B * 4", result["extabindex"]["kept"], (ETI_BASE, ETI_BASE + 0x30))
    _check("extabindex tail", result["extabindex"]["tail"], (ETI_BASE + 0x30, ETI_BASE + 0x60))
    _check("kept rows", result["text"]["rows_kept"], 4)
    _check("tail rows", result["text"]["rows_tail"], 4)
    _check("total rows", result["text"]["rows_total"], 8)

    _check(".text halves sum", result["sums"][".text"],
           {"kept": 0x100, "tail": 0x300, "original": 0x400})
    _check("extab halves sum", result["sums"]["extab"],
           {"kept": 0x20, "tail": 0x20, "original": 0x40})
    _check("extabindex halves sum", result["sums"]["extabindex"],
           {"kept": 0x30, "tail": 0x30, "original": 0x60})
    _check("record halve sum", result["sums"]["records"], {"kept": 4, "tail": 4, "original": 8})

    leaves = [(w["addr"], w["target"]) for w in result["ctors"] if w["leaves"]]
    keeps = [(w["addr"], w["target"]) for w in result["ctors"] if not w["leaves"]]
    _check("the ctor word into the tail is flagged", leaves, [(0x80030004, 0x80001200)])
    _check("the ctor word that stays is not", keeps,
           [(0x80030000, 0x80001040), (0x80031000, 0x80001000)])
    _check("the .ctors kept span", result["kept_ctor_spans"][".ctors"], (0x80030000, 0x80030004))
    _check("the .dtors kept span", result["kept_ctor_spans"][".dtors"], (0x80031000, 0x80031004))
    _check("with no split object the cross-check is skipped, not guessed",
           result["object"]["present"], False)

    # ------------------------------------------------------------------ the paste-ready text
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        uc.print_report(result)
    report = buf.getvalue()
    _contains("the kept .text line is paste-ready",
              "\t.text       start:0x80001000 end:0x80001100", report)
    _contains("the released .text line is paste-ready",
              "\t.text       start:0x80001100 end:0x80001400", report)
    _contains("the kept extab line", "\textab       start:0x80010000 end:0x80010020", report)
    _contains("the released extabindex line",
              "\textabindex  start:0x80020030 end:0x80020060", report)
    _contains("the DROP line names the word", "DROP .ctors word at 0x80030004", report)
    _contains("the report carries the rule", "un-claiming a .text range obliges dropping", report)
    _contains("the sum check is printed", "sum check", report)
    _contains("the assignment is printed", "records", report)

    # ------------------------------------------------------------------ refusal: cut mid-function
    for cut, why in ((0x80001020, "mid-function (inside fn_80001000)"),
                     (0x80001140, "mid-function in the released tail")):
        try:
            uc.analyse("menu/menu_message", cut, splits, dol, syms)
            _check("a %s cut is refused" % why, "no refusal", "Refusal")
        except uc.Refusal as exc:
            _check("a %s cut is refused" % why, "not a function boundary" in str(exc), True)
    try:
        uc.analyse("menu/menu_message", 0x80001020, splits, dol, syms)
    except uc.Refusal as exc:
        _contains("the mid-function refusal names the containing function", "fn_80001000", str(exc))

    # a boundary that the records accept but the symbol sizes contradict is refused too
    _, splits_short, syms_short = fixture(os.path.join(tmp, "short"), symbols_text=symbols(prev_size=0x30))
    try:
        uc.analyse("menu/menu_message", CUT, splits_short, dol, syms_short)
        _check("a cut the symbol sizes contradict is refused", "no refusal", "Refusal")
    except uc.Refusal as exc:
        _contains("the size refusal explains itself", "ends at 0x800010F0", str(exc))

    # ------------------------------------------------------------------ refusal: the invariant
    bad = fixture(os.path.join(tmp, "invariant"), extab_offsets=[None, None, ETAB_BASE + 8 * 2 + 4])
    try:
        uc.analyse("menu/menu_message", CUT, bad[1], bad[0], bad[2])
        _check("a broken etab invariant is refused", "no refusal", "Refusal")
    except uc.Refusal as exc:
        _contains("the invariant refusal names the record", "record 2", str(exc))
        _contains("... and says the invariant does not hold", "does not hold", str(exc))

    # ------------------------------------------------------------------ refusal: the gaps disagree
    disjoint = SPLITS.replace("extabindex  start:0x80020060 end:0x800200C0",
                              "extabindex  start:0x80020030 end:0x8002005C")
    _, splits_bad, syms_bad = fixture(os.path.join(tmp, "disjoint"), splits=disjoint)
    try:
        uc.analyse("menu/menu_message", CUT, splits_bad, dol, syms_bad)
        _check("a tail whose unwind gaps disagree is refused", "no refusal", "Refusal")
    except uc.Refusal as exc:
        _contains("the gap refusal says what disagrees", "disagrees with the unwind gaps", str(exc))

    # ------------------------------------------------------------------ refusal: unknown unit
    try:
        uc.analyse("menu/nope", CUT, splits, dol, syms)
        _check("an unknown unit is refused", "no refusal", "Refusal")
    except uc.Refusal as exc:
        _contains("the unknown-unit refusal says so", "no `splits.txt` block", str(exc))

    # ------------------------------------------------------------------ the split object is optional
    _check("a missing object path is fine (it is only a cross-check)",
           uc.analyse("menu/menu_message", CUT, splits, dol, syms, object_path=None)["records"], 8)

    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECK_COUNT)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
