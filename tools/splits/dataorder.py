#!/usr/bin/env python3
"""dataorder.py - translation-unit seams read from the *order* of retail `.data`.

MWCC lays out one TU's `.data` in a fixed order (measured with the project's flags, see
`docs/data-order-seams.md`): initialised globals over 8 B in definition order, the strings of out-of-line
functions in first-use order, **vtables in the reverse of class order**, then the strings of **inline** functions
(the "inline tail": in-class bodies and free `inline` functions, one unmerged copy per instance).  So a TU is
`D* S* V* s*`, and the linker concatenates TU fragments.  In retail:

* **V->S** (strong) - two vtable groups with strings between them: the second group is another TU, and the
  boundary lies somewhere in the gap (after any inline tail).  A vtable followed by strings and *no* later vtable
  is `V->tail` (weak): it may be an inline tail of the same TU, which is exactly what the g3d "contradictions"
  were.  **A vtable followed by strings is not by itself a seam.**
* **V->D** (weak) - a vtable followed by ordinary data (a jump table is `.data` too, its place is unmeasured);
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
DOL = uu.resolve_input(os.path.join("orig", GAME, "sys", "main.dol"), ROOT, os.path.isfile)   # MAIN's copy in a fresh worktree

VTABLE, STRING, DATA = "V", "S", "D"
PRINTABLE = set(range(32, 127)) | {9, 10, 13}
#: A `D` symbol this small between a vtable and the next symbol is alignment padding, not a new object.
PAD_MAX = 8
STRONG_KINDS = ("V->S", "zigzag")
SYMBOL_RE = re.compile(r"^(\S+) = (\S+):0x([0-9A-Fa-f]+);(.*)$")
SIZE_RE = re.compile(r"size:0x([0-9A-Fa-f]+)")


class Sym:
    """One `.data` symbol with its size, kind and (for a vtable) owner - the address of its first code slot."""

    __slots__ = ("addr", "size", "name", "kind", "owner", "text")

    def __init__(self, addr, size, name, kind=DATA, owner=None, text=None):
        self.addr, self.size, self.name, self.kind, self.owner, self.text = addr, size, name, kind, owner, text

    def as_dict(self):
        d = {"addr": self.addr, "size": self.size, "name": self.name, "kind": self.kind}
        if self.owner is not None:
            d["owner"] = self.owner
        return d


HEADER_NAME_RE = re.compile(r"^[A-Za-z0-9_./\\-]*\.(?:h|hpp|inl)$")


def is_header_name(text: str | None) -> bool:
    """A bare header file name (`g3d_resnode_ac.h`): the `__FILE__` of an assert in an INLINE function."""
    return bool(text) and bool(HEADER_NAME_RE.match(text.strip()))


SOURCE_NAME_RE = re.compile(r"^[A-Za-z0-9_./\-]*\.(?:c|cc|cpp|cp)$")
#: Most non-header strings that may sit between two header names (or before the first one) of one inline tail:
#: the assert message and the class-name argument of an inline instance (`ResLightSet`, `%s::%s: Object not valid.`).
TAIL_RUN_MAX = 3


def is_source_name(text: str | None) -> bool:
    """A bare source file name (`g3d_anmvis.cpp`): the `__FILE__` of an assert in an OUT-OF-LINE function."""
    return bool(text) and bool(SOURCE_NAME_RE.match(text.strip()))


def inline_tail(gap: list["Sym"]) -> int:
    """How many leading symbols of a `V->S` gap are the first TU's inline tail (0 when nothing says so).

    An inline function's assert leaves a run of unmerged strings after the vtables - message, class name and the
    *header* `__FILE__` (`g3d_resnode_ac.h`), one copy per instance - whereas the next TU's own strings lead with
    its source `__FILE__` (`g3d_anmvis.cpp`) or a message of its own.  The tail is therefore the leading run of
    strings up to and including the LAST header name that comes before the first source-file name, with at
    most `TAIL_RUN_MAX` other strings between two header names.  A gap with no header name (the
    `network_transport` gaps) has tail 0; a non-string symbol or a source-file name ends the run.
    """
    last = run = 0
    for n, s in enumerate(gap):
        if s.kind != STRING or is_source_name(s.text):
            break
        if is_header_name(s.text):
            last, run = n + 1, 0
        else:
            run += 1
            if run > TAIL_RUN_MAX:
                break
    return last


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
        blob = reader.read(addr, size)
        kind, owner = classify(blob, tlo, thi)
        text = blob.rstrip(b"\0").decode("latin-1") if kind == STRING else None
        out.append(Sym(addr, size, name, kind, owner, text))
    return out


def seams(syms: list[Sym]) -> list[dict]:
    """Every seam after a vtable group: `{addr, kind, before, after, ...}` in address order.

    `kind` is
    * `V->S` (strong) - strings between this vtable group and a LATER vtable: another TU starts in the gap.
      `addr` is the earliest the new TU can begin (the first string), `latest` the next vtable, `width` the
      number of symbols in the gap, `tail` how many leading symbols are the first TU's inline tail
      (`inline_tail`: strings up to the last header name before the first source-file name) - the boundary is
      after them;
    * `V->tail` (weak) - strings after the vtable group with no later vtable: an inline tail or another TU;
    * `V->D` (weak) - an object over 8 B that is not a string (a jump table is one too);
    * `zigzag` (strong) - two adjacent vtables, owners going up.
    Padding (a `D` of at most 8 B) is skipped.
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
        row = {"addr": b.addr, "before": a.name, "after": b.name}
        if b.kind == STRING:
            k = j
            while k < len(syms) and syms[k].kind != VTABLE:
                k += 1
            tail = inline_tail(syms[j:k])
            if k < len(syms):
                row.update(kind="V->S", latest=syms[k].addr, width=k - j, tail=tail)
            else:
                row.update(kind="V->tail", width=k - j, tail=tail)
        elif b.kind == DATA:
            row["kind"] = "V->D"
        elif a.owner is not None and b.owner is not None and b.owner > a.owner:
            row["kind"] = "zigzag"
        else:
            continue
        out.append(row)
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


def narrow_gap() -> int:
    """`dataseams.NARROW`, the widest `V->S` gap (in symbols) whose boundary is cut rather than only reported."""
    tools = os.path.dirname(HERE)
    for p in (os.path.join(tools, "units"), tools):
        if p not in sys.path:
            sys.path.insert(0, p)
    import dataseams
    return dataseams.NARROW


def fragments(syms: list[Sym], weak: bool = False) -> list[list[Sym]]:
    """Cut the run at every strong seam (and the weak `V->D`/`V->tail` ones when `weak`): one list per probable TU.

    A `V->S` gap of at most `dataseams.NARROW` symbols is cut at its estimated boundary, the first symbol after
    the row's `tail` inline-tail strings; a wider gap is not cut (a boundary is somewhere in it, and where is
    unknown).  A zigzag and the weak kinds cut at their address.
    """
    at = {s.addr: i for i, s in enumerate(syms)}
    narrow = narrow_gap()
    cuts = set()
    for row in seams(syms):
        if not (weak or row["kind"] in STRONG_KINDS):
            continue
        if row["kind"] == "V->S":
            if row["width"] > narrow:
                continue
            cuts.add(syms[min(at[row["addr"]] + row["tail"], len(syms) - 1)].addr)
        else:
            cuts.add(row["addr"])
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
    check("... a vtable then a string with no later vtable is only a weak V->tail",
          [s["kind"] for s in seams(got)], ["V->tail"])
    check("a bare header name is an inline-tail string", (is_header_name("g3d_resnode_ac.h"),
          is_header_name("particle.h"), is_header_name("%s::%s: Object not valid."), is_header_name("a.c")),
          (True, True, False, False))
    check("... but a message that merely mentions .h is not", is_header_name("see foo.h for details"), False)

    def tsym(name, kind, text=None, owner=None, size=16):
        return Sym(0, size, name, kind, owner, text)
    seq2 = [tsym("v1", VTABLE, owner=0x100), tsym("hdr", STRING, "x_ac.h"), tsym("msg", STRING, "A::f failed"),
            tsym("v2", VTABLE, owner=0x400)]
    for i, x in enumerate(seq2):
        x.addr = 0x1000 + i * 0x40
    got = seams(seq2)
    check("vtable, strings, later vtable: a strong V->S seam", [g["kind"] for g in got], ["V->S"])
    check("... earliest boundary is the first string, latest the next vtable",
          (got[0]["addr"], got[0]["latest"], got[0]["width"]), (0x1040, 0x10C0, 2))
    check("... and the leading header name is counted as an inline tail", got[0]["tail"], 1)
    seq3 = [tsym("v1", VTABLE, owner=0x100), tsym("hdr", STRING, "x_ac.h"), tsym("msg", STRING, "A::f failed")]
    for i, x in enumerate(seq3):
        x.addr = 0x1000 + i * 0x40
    got = seams(seq3)
    check("vtable then an inline-looking tail and no later vtable: weak V->tail",
          [(g["kind"], g["tail"]) for g in got], [("V->tail", 1)])
    check("... which fragments() does not cut", len(fragments(seq3)), 1)

    # inline tails as the retail DOL has them: message + header pairs, unmerged repeats, the next TU's source name
    def run(*spec):
        out = []
        for i, (kind, text) in enumerate(spec):
            out.append(Sym(0x1000 + i * 0x40, 16, "%s%d" % (kind, i), kind, 0x100 if kind == VTABLE else None,
                           text if kind == STRING else None))
        return out

    V, S, D = VTABLE, STRING, DATA
    OBJ = "%s::%s: Object not valid."
    pairs = run((V, None), (S, "NW4R:Failed assertion IsValid()"), (S, "g3d_fog.h"), (S, "NW4R:Failed assertion x"),
                (S, "g3d_fog.h"), (S, "g3d_scnobj.cpp"), (S, "NW4R:Pointer Error: this is not valid"),
                (V, None))
    check("message + header pairs are an inline tail, the source name ends it", seams(pairs)[0]["tail"], 4)
    check("... its fragment cut is the source name, not the first string",
          [[s.name for s in f] for f in fragments(pairs)][1][0], "S5")
    rep = run((V, None), (S, "ResAnmFog"), (S, OBJ), (S, "g3d_resanmfog_ac.h"), (S, "ResAnmLight"), (S, OBJ),
              (S, "g3d_resanmlight_ac.h"), (S, "ResAnmScn"), (S, OBJ), (S, "g3d_resanmscn_ac.h"),
              (S, "g3d_anmshp.cpp"), (V, None))
    check("repeated unmerged literals (name, message, header) count as one tail", seams(rep)[0]["tail"], 9)
    plain = run((V, None), (S, "NetworkPeerMcs::put: buf over"), (S, "NetworkSingleTcp::move: bad"), (V, None))
    check("a gap with no header name has tail 0 (network_transport)", seams(plain)[0]["tail"], 0)
    check("... and its fragment is cut at the first string", [[s.name for s in f] for f in fragments(plain)][1][0], "S1")
    check("a source name before any header ends the run", seams(run((V, None), (S, "ef_cube.cpp"), (S, "msg"),
          (S, "particle.h"), (V, None)))[0]["tail"], 0)
    check("a header after the next TU's source name is not the tail",
          seams(run((V, None), (S, "m"), (S, "a.h"), (S, "x.cpp"), (S, "m"), (S, "b.h"), (V, None)))[0]["tail"], 2)
    check("too many plain strings between headers break the run",
          seams(run((V, None), (S, "a"), (S, "b"), (S, "c"), (S, "d"), (S, "e.h"), (V, None)))[0]["tail"], 0)
    check("a non-string symbol ends the run",
          seams(run((V, None), (S, "m"), (S, "a.h"), (D, None), (S, "m"), (S, "b.h"), (V, None)))[0]["tail"], 2)
    check("is_source_name", (is_source_name("g3d_anmvis.cpp"), is_source_name("a.c"), is_source_name("g3d_fog.h"),
          is_source_name("see x.c for it")), (True, True, False, False))
    wide = run((V, None), (S, "m"), (S, "a.h"), *[(S, "text%d" % i) for i in range(12)], (V, None))
    check("a wide gap has its tail but fragments() does not cut it", (seams(wide)[0]["tail"], seams(wide)[0]["width"],
          len(fragments(wide))), (2, 14, 1))
    narrow_gap_run = run((V, None), (S, "m"), (S, "a.h"), (S, "next"), (V, None))
    check("a narrow gap is cut after its tail", [[s.name for s in f] for f in fragments(narrow_gap_run)],
          [["V0", "S1", "S2"], ["S3", "V4"]])
    check("fragments() reuses dataseams' threshold", narrow_gap(), 8)

    # the real DOL, when the repo has it: the network_transport seams the discovery came from
    if os.path.exists(DOL) and os.path.exists(SYMBOLS):
        out = scan_dol()
        check("real DOL: vtables were found", out["kinds"].get(VTABLE, 0) > 150, True)
        # inside the unit while it was one TU, at a registered unit's start since it was split (Network/NetworkPeerMcs
        # and its neighbours, docs/network-transport-split.md): either way the scan must find all four
        inside = {s["addr"] for s in out["inside_registered_unit"] + out["at_registered_unit_start"]}
        found_nt = [a for a in (0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40) if a in inside]
        check("real DOL: the four network_transport V->S seams are inside a unit or at a unit start",
              found_nt, [0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40])
        check("real DOL: unclaimed .data holds candidate seams", out["in_unclaimed_data"] > 30, True)
        rows_vs = [s for s in out["inside_registered_unit"] + out["unclaimed"] + out["at_registered_unit_start"]
                   if s["kind"] == "V->S"]
        check("real DOL: the g3d/ef inline tails are found (16 V->S rows measured 2026-09-29)",
              sum(1 for s in rows_vs if s["tail"]) >= 12, True)
        check("real DOL: the four network_transport gaps have no tail",
              [s["tail"] for s in rows_vs if s["addr"] in (0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40)],
              [0, 0, 0, 0])

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
