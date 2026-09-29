#!/usr/bin/env python3
"""dataorder.py - translation-unit seams read from the *order* of retail `.data`.

MWCC lays out one TU's `.data` in a fixed order (measured on a scratch file with the project's flags, see
`docs/data-order-seams.md`): initialised globals over 8 B in definition order, then string literals in first-use
order, then **vtables last, in the reverse of class order**.  The linker concatenates TU fragments, so in retail:

* **V->S / V->D** - a vtable followed by a string (strong) or by ordinary data (weak: a jump table is `.data` too
  and its place in the order is unmeasured) starts a new TU;
* **zigzag** - two *adjacent* vtables whose owners' first code slots go **up** in address are two TUs (inside one
  TU they descend).

This module is the reusable core: classify the DOL's `.data` symbols, list the seams, cut a run into TU
fragments.  It reads only the DOL and `config/RMHE08/symbols.txt`.  Nothing is written.

  dataorder.py scan [--json]                    # every seam in the DOL, with the counts `docs/data-order-seams.md` quotes
  dataorder.py at <address|symbol> [--window N] # the symbols around one address, their kinds and the seams between
  dataorder.py --selftest
"""
from __future__ import annotations

import argparse
import collections
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)

import unitutil as uu  # noqa: E402  (repo root + build layout)

ROOT = uu.ROOT
GAME = "RMHE08"
SYMBOLS = os.path.join(ROOT, "config", GAME, "symbols.txt")
SPLITS = os.path.join(ROOT, "config", GAME, "splits.txt")
DOL = os.path.join(ROOT, "orig", GAME, "sys", "main.dol")

VTABLE, STRING, DATA = "V", "S", "D"
PRINTABLE = set(range(32, 127)) | {9, 10, 13}
#: A `D` symbol this small between a vtable and the next symbol is alignment padding, not a new object.
PAD_MAX = 8
SYMBOL_RE = re.compile(r"^(\S+) = (\S+):0x([0-9A-Fa-f]+);(.*)$")
SIZE_RE = re.compile(r"size:0x([0-9A-Fa-f]+)")


class Sym:
    """One `.data` symbol with its size, kind and (for a vtable) owner - the address of its first code slot."""

    __slots__ = ("addr", "size", "name", "kind", "owner")

    def __init__(self, addr, size, name, kind=DATA, owner=None):
        self.addr, self.size, self.name, self.kind, self.owner = addr, size, name, kind, owner

    def as_dict(self):
        d = {"addr": self.addr, "size": self.size, "name": self.name, "kind": self.kind}
        if self.owner is not None:
            d["owner"] = self.owner
        return d


def load_symbols(path: str = SYMBOLS) -> list[tuple[str, int, int | None, str]]:
    """`(section, address, size-or-None, name)` for every map row."""
    out = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = SYMBOL_RE.match(ln.rstrip("\n"))
            if not m:
                continue
            sz = SIZE_RE.search(m.group(4))
            out.append((m.group(2), int(m.group(3), 16), int(sz.group(1), 16) if sz else None, m.group(1)))
    return out


def text_range(rows) -> tuple[int, int]:
    """The `.init`/`.text` span, from the map: a vtable slot must point into it."""
    text = [r for r in rows if r[0] in (".text", ".init")]
    return min(r[1] for r in text), max(r[1] + (r[2] or 0) for r in text)


def data_symbols(rows) -> list[tuple[int, int, str]]:
    """The `.data` rows as `(address, size, name)`, sizes filled from the next symbol when the map has none."""
    data = sorted((r for r in rows if r[0] == ".data"), key=lambda r: r[1])
    out = []
    for i, (_sec, addr, size, name) in enumerate(data):
        nxt = data[i + 1][1] if i + 1 < len(data) else None
        if not size:
            size = (nxt - addr) if nxt else 4
        if nxt and nxt > addr:
            size = min(size, nxt - addr)
        out.append((addr, size, name))
    return out


def classify(blob: bytes | None, tlo: int, thi: int) -> tuple[str, int | None]:
    """`(kind, owner)` of one symbol's bytes.

    A **vtable** is at least three words, a leading `0, 0` header (the game builds `-RTTI off`) and every other
    word a code pointer or zero, with at least one pointer.  A jump table has no header, so it is *data*.  A
    **string** is printable, NUL-terminated (trailing padding allowed) and at least two characters.
    """
    if not blob:
        return DATA, None
    if len(blob) >= 12 and len(blob) % 4 == 0:
        w = [int.from_bytes(blob[i:i + 4], "big") for i in range(0, len(blob), 4)]
        if w[0] == 0 and w[1] == 0 and all(x == 0 or tlo <= x < thi for x in w[2:]):
            ptr = [x for x in w[2:] if x]
            if ptr:
                return VTABLE, ptr[0]
    s = blob.rstrip(b"\0")
    if len(s) >= 2 and all(c in PRINTABLE for c in s) and sum(1 for c in s if c >= 32) >= 2:
        return STRING, None
    return DATA, None


def classify_all(rows, reader) -> list[Sym]:
    """Every `.data` symbol classified, in address order.  `reader.read(addr, n)` returns the retail bytes."""
    tlo, thi = text_range(rows)
    out = []
    for addr, size, name in data_symbols(rows):
        kind, owner = classify(reader.read(addr, size), tlo, thi)
        out.append(Sym(addr, size, name, kind, owner))
    return out


def seams(syms: list[Sym]) -> list[dict]:
    """Every seam between neighbouring symbols: `{addr, kind, before, after}` in address order.

    `kind` is `V->S` (strong), `V->D` (weak: an object over 8 B that is not a string) or `zigzag` (two adjacent
    vtables, owners going up).  Padding (a `D` of at most 8 B) between the two is skipped.  `addr` is where the
    new TU's data begins.
    """
    out = []
    for i, a in enumerate(syms):
        if a.kind != VTABLE:
            continue
        j = i + 1
        while j < len(syms) and syms[j].kind == DATA and syms[j].size <= PAD_MAX:
            j += 1
        if j >= len(syms):
            continue
        b = syms[j]
        if b.kind == STRING:
            kind = "V->S"
        elif b.kind == DATA:
            kind = "V->D"
        elif a.owner is not None and b.owner is not None and b.owner > a.owner:
            kind = "zigzag"
        else:
            continue
        out.append({"addr": b.addr, "kind": kind, "before": a.name, "after": b.name})
    return out


def zigzag_pairs(syms: list[Sym]) -> collections.Counter:
    """How many adjacent vtable pairs go `up` (a seam), `down` (the same TU) or `tie` (equal owners: no evidence)."""
    c = collections.Counter()
    for i, a in enumerate(syms):
        if a.kind != VTABLE:
            continue
        j = i + 1
        while j < len(syms) and syms[j].kind == DATA and syms[j].size <= PAD_MAX:
            j += 1
        if j < len(syms) and syms[j].kind == VTABLE and a.owner and syms[j].owner:
            c["up" if syms[j].owner > a.owner else ("down" if syms[j].owner < a.owner else "tie")] += 1
    return c


def fragments(syms: list[Sym], weak: bool = False) -> list[list[Sym]]:
    """Cut the run at every strong seam (and the weak `V->D` ones when `weak`): one list per probable TU."""
    cuts = {s["addr"] for s in seams(syms) if weak or s["kind"] != "V->D"}
    out, cur = [], []
    for s in syms:
        if s.addr in cuts and cur:
            out.append(cur)
            cur = []
        cur.append(s)
    if cur:
        out.append(cur)
    return out


def unit_data_ranges(path: str = SPLITS) -> dict[int, tuple[str, int]]:
    """`start -> (unit, end)` for every registered unit `.data` range."""
    out, cur = {}, None
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            if ln and not ln[0].isspace() and ln.rstrip().endswith(":"):
                cur = ln.strip()[:-1]
                continue
            m = re.match(r"\s+\.data\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)", ln)
            if m and cur:
                out[int(m.group(1), 16)] = (cur, int(m.group(2), 16))
    return out


def unit_of(ranges: dict, addr: int):
    for start, (unit, end) in ranges.items():
        if start <= addr < end:
            return unit, start, end
    return None


def scan_dol() -> dict:
    """The whole-DOL numbers: classification counts, seams by kind, where they fall (registered / unclaimed)."""
    import tudiscover as td
    rows = load_symbols()
    syms = classify_all(rows, td.Dol(DOL))
    ranges = unit_data_ranges()
    found = seams(syms)
    inside, unclaimed, at_start = [], [], []
    for s in found:
        if s["addr"] in ranges:
            at_start.append(s)
        elif unit_of(ranges, s["addr"]):
            inside.append(dict(s, unit=unit_of(ranges, s["addr"])[0]))
        else:
            unclaimed.append(s)
    return {"symbols": len(syms), "kinds": dict(collections.Counter(s.kind for s in syms)),
            "seams_by_kind": dict(collections.Counter(s["kind"] for s in found)),
            "zigzag_pairs": dict(zigzag_pairs(syms)),
            "inside_registered_unit": inside, "at_registered_unit_start": at_start,
            "in_unclaimed_data": len(unclaimed), "unclaimed": unclaimed}


def cmd_scan(args) -> int:
    out = scan_dol()
    if args.json:
        print(json.dumps(out, indent=1))
        return 0
    print(".data symbols classified: %d  %s" % (out["symbols"], out["kinds"]))
    print("seams by kind: %s   adjacent vtable pairs: %s" % (out["seams_by_kind"], out["zigzag_pairs"]))
    print("at a registered unit .data start: %d" % len(out["at_registered_unit_start"]))
    print("inside a registered unit (candidate seams the unit hides): %d" % len(out["inside_registered_unit"]))
    for s in out["inside_registered_unit"]:
        print("  0x%08X  %-7s %s  (%s -> %s)" % (s["addr"], s["kind"], s["unit"], s["before"], s["after"]))
    print("in unclaimed .data (candidate cuts for proposals): %d" % out["in_unclaimed_data"])
    return 0


def resolve_address(token: str, rows) -> int:
    if re.match(r"^(0x)?[0-9A-Fa-f]{8}$", token):
        return int(token, 16)
    for _sec, addr, _sz, name in rows:
        if name == token:
            return addr
    raise SystemExit("dataorder: %r is neither an address nor a symbol in the map" % token)


def cmd_at(args) -> int:
    import tudiscover as td
    rows = load_symbols()
    addr = resolve_address(args.target, rows)
    syms = classify_all(rows, td.Dol(DOL))
    idx = min(range(len(syms)), key=lambda i: abs(syms[i].addr - addr))
    lo, hi = max(0, idx - args.window), min(len(syms), idx + args.window + 1)
    cut = {s["addr"]: s["kind"] for s in seams(syms)}
    ranges = unit_data_ranges()
    for s in syms[lo:hi]:
        u = unit_of(ranges, s.addr)
        print("%s0x%08X %5X %s %-40s %s%s" % (
            ">> " if s is syms[idx] else "   ", s.addr, s.size, s.kind, s.name[:40],
            u[0] if u else "(unclaimed)", "   <-- %s seam" % cut[s.addr] if s.addr in cut else ""))
    return 0


# --- selftest ------------------------------------------------------------------------------------

class _FakeDol:
    def __init__(self, blobs):
        self.blobs = blobs

    def read(self, addr, n):
        for a, b in self.blobs.items():
            if a <= addr < a + len(b):
                return b[addr - a:addr - a + n]
        return None


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def word(x):
        return x.to_bytes(4, "big")

    tlo, thi = 0x80010000, 0x80020000
    vt = lambda *ptrs: word(0) + word(0) + b"".join(word(p) for p in ptrs)   # noqa: E731
    check("a 0,0-headed pointer table is a vtable", classify(vt(0x80010100, 0x80010200), tlo, thi),
          (VTABLE, 0x80010100))
    check("... a jump table (no header) is data", classify(word(0x80010100) + word(0x80010200) + word(0x80010300),
                                                          tlo, thi)[0], DATA)
    check("... a header with no code pointer is data", classify(vt(0), tlo, thi)[0], DATA)
    check("... a pointer outside .text is data", classify(vt(0x80030000), tlo, thi)[0], DATA)
    check("a printable NUL-terminated blob is a string", classify(b"hello world\n\0\0\0", tlo, thi)[0], STRING)
    check("... one character is not", classify(b"a\0\0\0", tlo, thi)[0], DATA)
    check("... binary bytes are data", classify(b"\x01\x02\x03\x04\x05", tlo, thi)[0], DATA)
    check("nothing to read is data", classify(None, tlo, thi), (DATA, None))

    def seq(spec):
        return [Sym(0x1000 + i * 0x40, size, name, kind, owner) for i, (name, kind, size, owner) in enumerate(spec)]

    # one TU (strings then two vtables, owners DOWN), then a second TU (string, vtable)
    one_tu = seq([("s1", STRING, 16, None), ("s2", STRING, 16, None), ("vC", VTABLE, 16, 0x300),
                  ("vB", VTABLE, 16, 0x200), ("vA", VTABLE, 16, 0x100)])
    check("strings then descending vtables is one TU: no seam", seams(one_tu), [])
    check("... one fragment", len(fragments(one_tu)), 1)
    two_tu = one_tu + seq([("t1", STRING, 16, None), ("wA", VTABLE, 16, 0x400)])
    for i, s in enumerate(two_tu):
        s.addr = 0x1000 + i * 0x40
    got = seams(two_tu)
    check("a string after a vtable is a V->S seam at the string", [(s["kind"], s["after"]) for s in got],
          [("V->S", "t1")])
    check("... two fragments, cut before the string", [[s.name for s in f] for f in fragments(two_tu)],
          [["s1", "s2", "vC", "vB", "vA"], ["t1", "wA"]])
    up = seq([("vA", VTABLE, 16, 0x100), ("vB", VTABLE, 16, 0x200)])
    check("two adjacent vtables going UP are a zigzag seam", [s["kind"] for s in seams(up)], ["zigzag"])
    check("... counted as up", dict(zigzag_pairs(up)), {"up": 1})
    down = seq([("vB", VTABLE, 16, 0x200), ("vA", VTABLE, 16, 0x100)])
    check("two adjacent vtables going DOWN are the same TU", (seams(down), dict(zigzag_pairs(down))),
          ([], {"down": 1}))
    padded = seq([("vB", VTABLE, 16, 0x200), ("pad", DATA, 4, None), ("vA", VTABLE, 16, 0x100)])
    check("alignment padding between vtables is skipped", (seams(padded), dict(zigzag_pairs(padded))),
          ([], {"down": 1}))
    weak = seq([("vA", VTABLE, 16, 0x100), ("tbl", DATA, 64, None)])
    check("a vtable followed by other data is a weak V->D seam", [s["kind"] for s in seams(weak)], ["V->D"])
    check("... not cut by default, cut on request", (len(fragments(weak)), len(fragments(weak, weak=True))), (1, 2))
    tie = seq([("vA", VTABLE, 16, 0x100), ("vB", VTABLE, 16, 0x100)])
    check("two adjacent vtables with the same owner are a tie, not a seam", (seams(tie), dict(zigzag_pairs(tie))),
          ([], {"tie": 1}))
    check("a vtable at the end of the run has no seam", seams(seq([("vA", VTABLE, 16, 0x100)])), [])

    # map plumbing
    rows = [(".text", 0x80010000, 0x100, "f"), (".data", 0x100, 0x10, "a"), (".data", 0x110, None, "b"),
            (".data", 0x140, 8, "c")]
    check("text_range comes from the map", text_range(rows), (0x80010000, 0x80010100))
    check("data_symbols fills a missing size from the next symbol",
          data_symbols(rows), [(0x100, 0x10, "a"), (0x110, 0x30, "b"), (0x140, 8, "c")])
    fake = _FakeDol({0x100: vt(0x80010010) + b"", 0x110: b"a string here\0\0\0"})
    rows2 = [(".text", 0x80010000, 0x100, "f"), (".data", 0x100, 12, "v"), (".data", 0x110, 16, "s")]
    got = classify_all(rows2, fake)
    check("classify_all reads the DOL through the map", [(s.name, s.kind) for s in got],
          [("v", VTABLE), ("s", STRING)])
    check("... and finds the V->S seam", [s["kind"] for s in seams(got)], ["V->S"])

    # the real DOL, when the repo has it: the network_transport seams the discovery came from
    if os.path.exists(DOL) and os.path.exists(SYMBOLS):
        out = scan_dol()
        check("real DOL: vtables were found", out["kinds"].get(VTABLE, 0) > 150, True)
        inside = {s["addr"] for s in out["inside_registered_unit"]}
        found_nt = [a for a in (0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40) if a in inside]
        check("real DOL: the four network_transport V->S seams are inside the unit",
              found_nt, [0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40])
        check("real DOL: unclaimed .data holds candidate seams", out["in_unclaimed_data"] > 30, True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    s = sub.add_parser("scan", help="every seam in the DOL's .data")
    s.add_argument("--json", action="store_true")
    a = sub.add_parser("at", help="the symbols around one address, with their kinds and seams")
    a.add_argument("target", help="an address (8 hex digits) or a map symbol")
    a.add_argument("--window", type=int, default=8)
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.cmd == "scan":
        return cmd_scan(args)
    if args.cmd == "at":
        return cmd_at(args)
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
