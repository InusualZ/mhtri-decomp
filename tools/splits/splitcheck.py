#!/usr/bin/env python3
"""splitcheck.py - a read-only checker for a `splits.txt`: does every unit and every boundary satisfy what we know?

    python tools/splits/splitcheck.py --baseline [--json F] [--all] [--only INV[,INV]] [--unit REGEX] [--intervals]
    python tools/splits/splitcheck.py --proposal F [--proposal G ...] [--emit-splits OUT] [--json F]
    python tools/splits/splitcheck.py --selftest

`--baseline` checks the repository's current `config/RMHE08/splits.txt` and prints the audit list (phase 5 of the
splits program, `docs/splits-program.md`): per invariant PASS/FAIL/UNKNOWN counts, the top defects, the suspected
seams (the seam requests in `.pi/outbox/*.json`, the pool groups of `docs/pool-seams.md`).  `--proposal` renders a
proposal file (`.pi/splits/phase<N>-<band>.json`, format in `docs/splits-program.md`) into a candidate `splits.txt`
(strong + medium cuts applied, `guess` cuts merged, never emitted), lints the proposal, checks the candidate and
prints the delta against the baseline.  Nothing is written except `--json` / `--emit-splits`.

A phase 2 proposal (`docs/splits-program.md`, "Phase 2 files") may have no `units` and carries `attach` rows (data ranges given to a unit of the candidate; the
legacy names `data_attach` and `attachments` load as aliases with a warning), `unowned_data` rows (the deferrals), `moves`, and units without `.text` placed by
`after`.  `--readers SECTION:START-END` prints each map symbol of a data range with its owner in the candidate and the units whose decoded text reads it.

`--baseline --unit REGEX` also prints, for the matching units, one `detail` line per `.ctors`/`.dtors` word (the function, its end, the
closure end, the end with the unit's own vtable slots, the unit end) and, with `--intervals`, one `pooldup` line per pool value held
at two addresses (both addresses, the last read of the first, the first read of the second, the interval a TU starts in): the numbers
a proposal's `reproduce` row quotes.  `--proposal` also reports every proposed cut, a `guess` included, that a `scope:local`
data object is read across (a static is one TU's) and exits 1 on one.

Invariants (one verdict per unit per invariant; PASS / FAIL / UNKNOWN, `-` = not applicable; evidence = an address):

  order       link order: no overlapping ranges, one range per section, no cycle between the units' section orders
              (`.bss`/`.sbss`/data sequences against the text sequence are the same graph)
  coverage    every map symbol sits inside exactly one unit's range of its section (no gap under a symbol, no straddle)
  text-cut    a unit's `.text` starts and ends on a function symbol
  extab       every `extabindex` entry is owned by the unit that owns its function and its `extab` record
  ctors/dtors the `.ctors`/`.dtors` word points at a function of the unit; a unit has one `.ctors` word (`__sinit`), and the
              TU ends at the end of the closure of the sinit's local callees and address-taken functions (MWCC emits the
              deferred constructors, registered destructors and `b ctor` thunks after it), not at the sinit's own end
  pool        idea 94: the `.sdata2` float/double and `.sdata` string pool of a unit is read only (by a load) by that unit,
              runs in first-use order (per function), and holds each value once
  data-order  docs/data-order-seams.md: no strong V->S / zigzag seam strictly inside the unit's `.data`
  vtable      a vtable sits in the unit whose text holds one of its slots or stores it
  jumptable   a jump table sits in the unit that reads it and branches into
  bss         a local `.bss`/`.sbss` object is read by the unit that holds it
  local-static a `scope:local` data object (not a pool literal) is read by the text of one unit only: a static is private to its
              TU, so a local read from both sides of a boundary says the boundary is not a TU edge

The text references (pool first-use, jump table and bss readers) are decoded from the retail `.text`: a `lis` + `addi`/
`ori`/load pair, or an r13/r2 small-data access.  That is heuristic evidence (a register reused across a branch can
fool it) and is stated as such in every finding that depends on it.
"""
from __future__ import annotations

import argparse
import bisect
import collections
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
for _p in (TOOLS, HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

GAME = "RMHE08"
PASS, FAIL, UNKNOWN, NA = "PASS", "FAIL", "UNKNOWN", "-"
RANK = {NA: 0, PASS: 1, UNKNOWN: 2, FAIL: 3}
INVARIANTS = ("order", "coverage", "text-cut", "extab", "ctors", "dtors", "pool", "data-order", "vtable",
              "jumptable", "bss", "local-static")
#: how bad a defect of this invariant is, for the top-N list (higher first)
WEIGHT = {"order": 100, "coverage": 95, "extab": 90, "ctors": 85, "dtors": 80, "text-cut": 75, "pool": 60,
          "vtable": 55, "jumptable": 50, "data-order": 45, "bss": 40, "local-static": 58}
SECTION_ORDER = [".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data", ".bss", ".sdata",
                 ".sbss", ".sdata2", ".sbss2"]
CODE_SECTIONS = (".init", ".text")
SYMBOL_RE = re.compile(r"^(\S+) = (\S+):0x([0-9A-Fa-f]+);(.*)$")
RANGE_RE = re.compile(r"^\s+(\S+)\s+start:0x([0-9A-Fa-f]+)\s+end:0x([0-9A-Fa-f]+)(.*)$")
UNIT_RE = re.compile(r"^(\S.*?):(?:\s+(.*))?$")
GRADES = ("strong", "medium", "guess")
#: a window (in instructions) a `lis` value stays live for the reference decoder
LIS_WINDOW = 200


# ---- locating the inputs ---------------------------------------------------------------------------------------------

def tree_root():
    import unitutil as uu
    return uu.ROOT


def main_root(root=None):
    """The primary checkout (`$MHTRI_MAIN`, else the parent of the git common dir), or None outside git."""
    import unitutil as uu
    return uu.main_tree(root or tree_root())


def find_file(rel, root=None):
    """`rel` under the tree, else under the primary checkout (orig/ and build/ are not in a worktree)."""
    import unitutil as uu
    return uu.resolve_input(rel, root or tree_root(), os.path.isfile)


# ---- splits.txt ------------------------------------------------------------------------------------------------------

class Unit:
    """One `splits.txt` entry: name, raw file attributes, and `section -> [(start, end, raw_attrs)]`."""

    def __init__(self, name, attrs="", ranges=None):
        self.name = name
        self.attrs = attrs or ""
        self.ranges = ranges if ranges is not None else {}

    def rs(self, sec):
        return [(s, e) for s, e, _a in self.ranges.get(sec, [])]

    def first(self, sec):
        r = self.ranges.get(sec)
        return r[0][0] if r else None

    def size(self, sec):
        return sum(e - s for s, e, _a in self.ranges.get(sec, []))

    def lo(self):
        """The unit's anchor address: its first `.text` start, else its lowest range start."""
        t = self.first(".text")
        if t is not None:
            return t
        starts = [s for rr in self.ranges.values() for s, _e, _a in rr]
        return min(starts) if starts else 0


class Splits:
    def __init__(self, header, units):
        self.header = header          # the raw lines up to and including the last `Sections:` row
        self.units = units            # in file order

    def by_name(self):
        return {u.name: u for u in self.units}


def parse_splits(text):
    header, units, cur = [], [], None
    in_header = True
    for ln in text.splitlines():
        if in_header:
            if ln.startswith("Sections:") or (header and ln[:1].isspace() and ln.strip()):
                header.append(ln)
                continue
            in_header = False
        if not ln.strip():
            continue
        m = RANGE_RE.match(ln)
        if m and cur is not None:
            cur.ranges.setdefault(m.group(1), []).append((int(m.group(2), 16), int(m.group(3), 16),
                                                          m.group(4).rstrip()))
            continue
        m = UNIT_RE.match(ln)
        if m and not ln[0].isspace():
            cur = Unit(m.group(1), m.group(2) or "")
            units.append(cur)
    return Splits(header, units)


def render_splits(sp):
    out = list(sp.header) + [""]
    order = {s: i for i, s in enumerate(SECTION_ORDER)}
    for u in sp.units:
        out.append("%s:%s" % (u.name, (" " + u.attrs) if u.attrs else ""))
        for sec in sorted(u.ranges, key=lambda s: order.get(s, 99)):
            for s, e, a in sorted(u.ranges[sec]):
                out.append("\t%-11s start:0x%08X end:0x%08X%s" % (sec, s, e, (" " + a.strip()) if a.strip() else ""))
        out.append("")
    return "\n".join(out)


# ---- symbols.txt and the DOL ------------------------------------------------------------------------------------------

def parse_symbols(lines):
    """Map rows as dicts (`name section addr size type scope kind`); the map is streamed, never printed."""
    out = []
    for ln in lines:
        m = SYMBOL_RE.match(ln.rstrip("\n"))
        if not m:
            continue
        rest = m.group(4)
        sz = re.search(r"size:0x([0-9A-Fa-f]+)", rest)
        ty = re.search(r"type:(\w+)", rest)
        sc = re.search(r"scope:(\w+)", rest)
        kd = re.search(r"(?<![\w.])data:(\S+)", rest)
        out.append({"name": m.group(1), "section": m.group(2), "addr": int(m.group(3), 16),
                    "size": int(sz.group(1), 16) if sz else 0, "type": ty.group(1) if ty else "",
                    "scope": sc.group(1) if sc else "", "kind": kd.group(1) if kd else ""})
    return out


class Dol:
    """Address -> bytes of the retail image through the DOL section table."""

    def __init__(self, data):
        self.data = data
        h = data
        toff = struct.unpack(">7I", h[0x00:0x1C])
        doff = struct.unpack(">11I", h[0x1C:0x48])
        taddr = struct.unpack(">7I", h[0x48:0x64])
        daddr = struct.unpack(">11I", h[0x64:0x90])
        tsize = struct.unpack(">7I", h[0x90:0xAC])
        dsize = struct.unpack(">11I", h[0xAC:0xD8])
        self.secs = [(a, s, o) for a, s, o in zip(taddr + daddr, tsize + dsize, toff + doff) if s]

    def read(self, addr, n):
        for a, s, o in self.secs:
            if a <= addr and addr + n <= a + s:
                return self.data[o + (addr - a):o + (addr - a) + n]
        return None

    def word(self, addr):
        b = self.read(addr, 4)
        return struct.unpack(">I", b)[0] if b else None


# ---- instruction decoding: who reads which data address -----------------------------------------------------------------

LOADS_INT = (32, 33, 34, 35, 40, 41, 42, 43)          # write rD
STORES = (36, 37, 38, 39, 44, 45, 47)
FP_MEM = (48, 49, 50, 51, 52, 53, 54, 55)


def find_sda_bases(dol, code_words):
    """`(r13, r2)` values: the `lis rN / ori|addi rN` pair in the start-up code (`__init_registers`)."""
    out = {13: None, 2: None}
    lis = {}
    for addr, w in code_words:
        op = w >> 26
        rd, ra = (w >> 21) & 31, (w >> 16) & 31
        if op == 15 and ra == 0 and rd in out:
            lis[rd] = (w & 0xFFFF) << 16
        elif op == 24 and rd in lis and ra == rd and out[rd] is None:         # ori rA,rS,UI
            out[rd] = lis[rd] | (w & 0xFFFF)
        elif op == 14 and ra in lis and rd == ra and out[rd] is None:         # addi rD,rA,SIMM
            s = w & 0xFFFF
            out[rd] = (lis[rd] + (s - 0x10000 if s & 0x8000 else s)) & 0xFFFFFFFF
    return out[13], out[2]


#: opcodes that READ memory through `rA + d` (lwz/lbz/lhz/lha/lfs/lfd, their update forms, lmw, psq_l/psq_lu)
LOAD_OPS = (32, 33, 34, 35, 40, 41, 42, 43, 46, 48, 49, 50, 51, 56, 57)
#: the update-form accesses (lwzu/lbzu/lhzu/lhau/stwu/stbu/sthu/lfsu/lfdu/stfsu/stfdu): they write rA back
UPDATE_OPS = (33, 35, 37, 39, 41, 43, 45, 49, 51, 53, 55)
#: registers a call clobbers (r0, r3..r12): an address formed there does not survive a `bl`
VOLATILE = frozenset([0] + list(range(3, 13)))


def scan_refs(code, start, sda13, sda2, is_data, fn_starts=(), loads=None):
    """`{target_address: [site, ...]}` for every absolute/small-data address a function materialises or accesses.

    `code` is the big-endian bytes of a code range starting at `start`; `is_data(addr)` says whether a computed
    address is worth recording; a `lis` value is forgotten at a function start (`fn_starts`).  When `loads` is a
    dict, it receives `{address: [site, ...]}` for the accesses that READ the address: a load through r13/r2, a
    `lis` + load, and a load through a register an `addi`/`ori` formed the address into.  The `addi`/`ori` that
    only forms the address of an entry is a reference (it is in the result) and is NOT a read of the literal.
    """
    refs = collections.defaultdict(list)
    n = len(code) // 4
    words = struct.unpack(">%dI" % n, code[:n * 4])
    fs = set(fn_starts)
    lis = {}
    areg = {}                                          # register -> (address formed by addi/ori, index)
    for i, w in enumerate(words):
        site = start + i * 4
        if site in fs:
            lis.clear()
            areg.clear()
        op = w >> 26
        if op == 18 or op == 19:                       # b / bl / blr / bctr / bctrl / bclr...: control leaves
            if w & 1:                                  # a call clobbers the volatile registers
                for r in VOLATILE:
                    areg.pop(r, None)
                    lis.pop(r, None)
            elif op == 18 or (w >> 21) & 31 == 20:     # an unconditional jump or return ends the path
                areg.clear()
            continue
        if op == 15:                                   # addis rD,rA,SIMM  (lis when rA == 0)
            rd, ra = (w >> 21) & 31, (w >> 16) & 31
            areg.pop(rd, None)
            if ra == 0:
                lis[rd] = ((w & 0xFFFF) << 16, i)
            else:
                lis.pop(rd, None)
            continue
        if op == 24:                                   # ori rA,rS,UI : base is rS, result goes to rA
            rs, ra = (w >> 21) & 31, (w >> 16) & 31
            areg.pop(ra, None)
            if rs in lis and i - lis[rs][1] <= LIS_WINDOW:
                t = lis[rs][0] | (w & 0xFFFF)
                if is_data(t):
                    refs[t].append(site)
                areg[ra] = (t, i)
            if ra != rs:
                lis.pop(ra, None)
            continue
        if op == 14 or 32 <= op <= 57:
            rt, ra = (w >> 21) & 31, (w >> 16) & 31
            if op in (56, 57):                         # psq_l / psq_lu: 12-bit displacement
                s = w & 0xFFF
                simm = s - 0x1000 if s & 0x800 else s
            else:
                s = w & 0xFFFF
                simm = s - 0x10000 if s & 0x8000 else s
            t = None
            if ra == 13 and sda13 is not None:
                t = (sda13 + simm) & 0xFFFFFFFF
            elif ra == 2 and sda2 is not None:
                t = (sda2 + simm) & 0xFFFFFFFF
            elif ra != 0 and ra in lis and i - lis[ra][1] <= LIS_WINDOW:
                t = (lis[ra][0] + simm) & 0xFFFFFFFF
            if t is not None and is_data(t):
                refs[t].append(site)
                if loads is not None and op in LOAD_OPS:
                    loads.setdefault(t, []).append(site)
            elif op != 14 and ra in areg and loads is not None and op in LOAD_OPS:
                ta = (areg[ra][0] + simm) & 0xFFFFFFFF
                if is_data(ta):
                    loads.setdefault(ta, []).append(site)
            if op in UPDATE_OPS and ra in lis and t is not None:
                lis[ra] = (t, i)                         # the update form leaves rA = the effective address
            if op == 14:
                if rt != ra:
                    areg.pop(rt, None)
                if t is not None and is_data(t):
                    areg[rt] = (t, i)
                else:
                    areg.pop(rt, None)
            elif op in LOADS_INT:
                areg.pop(rt, None)
            writes = op == 14 or op in LOADS_INT
            if writes:
                lis.pop(rt, None)
            elif op == 46:                               # lmw rD: rD..r31 written
                for r in range(rt, 32):
                    lis.pop(r, None)
                    areg.pop(r, None)
    return refs


def scan_calls(code, start, fn_starts):
    """`[(site, target), ...]` for every `b`/`bl` whose target is a function start outside the function holding the site."""
    out = []
    n = len(code) // 4
    words = struct.unpack(">%dI" % n, code[:n * 4])
    fs = sorted(set(fn_starts))
    fset = set(fs)
    for i, w in enumerate(words):
        if w >> 26 != 18 or w & 2:                     # b/bl with AA = 0
            continue
        site = start + i * 4
        li = w & 0x03FFFFFC
        if li & 0x02000000:
            li -= 0x04000000
        t = (site + li) & 0xFFFFFFFF
        if t in fset:
            k = bisect.bisect_right(fs, site) - 1
            if k < 0 or fs[k] != t:                    # a branch to the function's own start is a loop, not a call
                out.append((site, t))
    return out


# ---- the context: everything the invariants read --------------------------------------------------------------------------

class Ctx:
    """A splits file + the map + the retail image, with the indices every invariant uses."""

    def __init__(self, splits, symbols, dol, sda13=None, sda2=None, scan=True):
        self.splits, self.symbols, self.dol = splits, symbols, dol
        self.units = splits.units
        self.by_name = splits.by_name()
        self.sec_index = collections.defaultdict(list)        # section -> sorted [(start, end, unit)]
        for u in self.units:
            for sec, rr in u.ranges.items():
                for s, e, _a in rr:
                    self.sec_index[sec].append((s, e, u))
        for v in self.sec_index.values():
            v.sort(key=lambda t: (t[0], t[1]))
        self._starts = {sec: [t[0] for t in v] for sec, v in self.sec_index.items()}
        self.fns = sorted((s for s in symbols if s["type"] == "function" and s["section"] in CODE_SECTIONS),
                          key=lambda s: s["addr"])
        self._fn_starts = [s["addr"] for s in self.fns]
        self.data_syms = sorted((s for s in symbols if s["section"] not in CODE_SECTIONS
                                 and s["section"] in SECTION_ORDER), key=lambda s: (s["addr"], s["section"]))
        self._ds_addr = [s["addr"] for s in self.data_syms]
        self.sda13, self.sda2 = sda13, sda2
        self.refs = {}                                        # data symbol index -> [sites]
        self.load_refs = {}                                   # data symbol index -> [sites that LOAD through the address]
        self.fn_refs = collections.defaultdict(list)          # function start -> [(site, "call" | "addr")]
        self.fn_out = collections.defaultdict(set)            # function start -> {function starts it calls / takes}
        self.scanned = False
        if scan:
            self.scan()

    # lookups
    def owner(self, sec, addr):
        v = self.sec_index.get(sec)
        if not v:
            return None
        i = bisect.bisect_right(self._starts[sec], addr) - 1
        for j in range(i, max(i - 8, -1), -1):
            s, e, u = v[j]
            if s <= addr < e:
                return u
        return None

    def text_owner(self, addr):
        return self.owner(".text", addr) or self.owner(".init", addr)

    def fn_at(self, addr):
        i = bisect.bisect_right(self._fn_starts, addr) - 1
        if i < 0:
            return None
        f = self.fns[i]
        end = f["addr"] + (f["size"] or 4)
        return f if f["addr"] <= addr < end else None

    def data_sym_at(self, addr):
        i = bisect.bisect_right(self._ds_addr, addr) - 1
        if i < 0:
            return None, None
        s = self.data_syms[i]
        end = s["addr"] + (s["size"] or 4)
        if s["addr"] <= addr < end:
            return i, s
        return None, None

    def section_extent(self, sec):
        v = self.sec_index.get(sec)
        return (min(t[0] for t in v), max(t[1] for t in v)) if v else None

    # the text reference index
    def scan(self):
        data_lo = None
        for sec in (".ctors", ".dtors", ".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2"):
            ext = self.section_extent(sec)
            if ext:
                data_lo = ext[0] if data_lo is None else min(data_lo, ext[0])
        hi = max((e for sec in self.sec_index for _s, e, _u in self.sec_index[sec]), default=0)
        # the map's own data extent counts too: an unowned `.sdata2` pool past the last owned range is still read by text
        hi = max([hi] + [s["addr"] + (s["size"] or 4) for s in self.data_syms])
        if data_lo is None and self.data_syms:
            data_lo = min(s["addr"] for s in self.data_syms)

        def is_data(t):
            return data_lo is not None and data_lo <= t < hi

        ranges = []
        for sec in CODE_SECTIONS:
            ext = self.section_extent(sec)
            if ext:
                ranges.append(ext)
        chunks = [(lo, self.read_span(lo, hi2)) for lo, hi2 in ranges]
        if self.sda13 is None and self.sda2 is None:
            init = next((f for f in self.fns if f["name"] == "__init_registers"), None)
            if init:
                b = self.dol.read(init["addr"], init["size"] or 0x90)
                if b:
                    ws = [(init["addr"] + i * 4, w) for i, w in enumerate(struct.unpack(">%dI" % (len(b) // 4), b))]
                    self.sda13, self.sda2 = find_sda_bases(self.dol, ws)
        raw = collections.defaultdict(list)
        rawl = {}
        fnset = set(self._fn_starts)
        for lo, blob in chunks:
            if not blob:
                continue
            for t, sites in scan_refs(blob, lo, self.sda13, self.sda2, is_data, self._fn_starts, rawl).items():
                raw[t].extend(sites)
            for site, t in scan_calls(blob, lo, self._fn_starts):
                self._add_fn_ref(site, t, "call")
            for t, sites in scan_refs(blob, lo, None, None, fnset.__contains__, self._fn_starts).items():
                for site in sites:
                    if self.fn_at(site) is not self.fn_at(t):
                        self._add_fn_ref(site, t, "addr")
        for t, sites in raw.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.refs.setdefault(i, []).extend(sites)
        for t, sites in rawl.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.load_refs.setdefault(i, []).extend(sites)
        self.scanned = True

    def _add_fn_ref(self, site, target, kind):
        self.fn_refs[target].append((site, kind))
        f = self.fn_at(site)
        if f is not None:
            self.fn_out[f["addr"]].add(target)

    def read_span(self, lo, hi):
        """Bytes of `[lo, hi)`, reading across DOL section borders (a hole is zero-filled)."""
        out = bytearray()
        a = lo
        while a < hi:
            nxt = None
            for s, size, _o in self.dol.secs:
                if s <= a < s + size:
                    nxt = s + size
                    break
            if nxt is None:
                nxt = min([s for s, _z, _o in self.dol.secs if s > a] + [hi])
                out += b"\0" * (min(nxt, hi) - a)
            else:
                n = min(nxt, hi) - a
                out += self.dol.read(a, n) or b"\0" * n
            a = min(nxt, hi)
        return bytes(out)

    def readers(self, sym):
        """The sites (text addresses) that read this map row, by identity."""
        i, _ = self.data_sym_at(sym["addr"])
        return self.refs.get(i, []) if i is not None and self.data_syms[i] is sym else []

    def literal_readers(self, sym):
        """The sites that read a pool literal.  A numeric `.sdata2` literal is read only by a LOAD (a `lis`/`addi` that
        passes or compares its address does not read the value); a `.sdata` string is used by its address, so any reference."""
        if sym["section"] == ".sdata2" and sym["size"] in (4, 8) and sym["type"] == "object":
            i, _ = self.data_sym_at(sym["addr"])
            return self.load_refs.get(i, []) if i is not None and self.data_syms[i] is sym else []
        return self.readers(sym)

    # the function reference index (callers-style, from the decoder)
    def refs_to_fn(self, addr, lo=None, hi=None):
        """`[(site, kind)]` of the code that calls or takes the address of the function at `addr`, within `[lo, hi)` when given."""
        return [(s, k) for s, k in self.fn_refs.get(addr, []) if (lo is None or s >= lo) and (hi is None or s < hi)]

    def sinit_closure(self, sinit_addr, end):
        """`(functions, L)`: the closure of the functions the `__sinit` at `sinit_addr` calls or takes the address of, among
        those after it and before `end` (MWCC emits the deferred constructors, registered destructors and `lis/addi/b ctor`
        thunks there), and `L`, the end of the last of them (the sinit's own end when it has none)."""
        f0 = self.fn_at(sinit_addr)
        if f0 is None:
            return set(), sinit_addr
        seen = {f0["addr"]}
        work = [f0["addr"]]
        while work:
            a = work.pop()
            for t in self.fn_out.get(a, ()):
                if f0["addr"] < t < end and t not in seen:
                    seen.add(t)
                    work.append(t)
        return seen, max(f["addr"] + (f["size"] or 0) for f in (self.fn_at(a) for a in seen))

    def fn_slot_words(self):
        """`{function start: [.data word addresses holding it]}` - the virtual-table slots (read lazily, once)."""
        if getattr(self, "_slot_words", None) is None:
            self._slot_words = collections.defaultdict(list)
            rows = [s for s in self.data_syms if s["section"] == ".data"]          # the map's extent: unowned `.data` counts too
            ext = (min(s["addr"] for s in rows), max(s["addr"] + (s["size"] or 4) for s in rows)) if rows else None
            if ext:
                blob = self.read_span(ext[0], ext[1])
                fs = set(self._fn_starts)
                for i, w in enumerate(struct.unpack(">%dI" % (len(blob) // 4), blob[:len(blob) // 4 * 4])):
                    if w in fs:
                        self._slot_words[w].append(ext[0] + i * 4)
        return self._slot_words

    def extend_over_own_vtable_slots(self, limit, lo, end, unit_end=None):
        """The end of the run of functions at `limit` that are slots of a vtable a function of the unit text `[lo, limit)` stores
        (inline virtual functions a TU emits after its `__sinit`: the table's constructor is in the unit, nothing calls them).
        Returns `limit` unchanged when the function at `limit` is not such a slot.  Past `unit_end` (the unit's real end, `end` being
        unbounded) the run must open with a small slot (an inline stub): a large function is a non-inline member another TU defines
        (a `lis/addi/stw vtable` constructor says nothing about where the member is), while a run that has opened with a stub goes on."""
        stub = False
        while limit + SINIT_SLACK < end:
            k = bisect.bisect_left(self._fn_starts, limit)
            if k >= len(self.fns):
                break
            g = self.fns[k]
            if g["addr"] - limit > SINIT_SLACK or g["addr"] >= end:
                break
            own = False
            for wa in self.fn_slot_words().get(g["addr"], ()):
                i, _sym = self.data_sym_at(wa)
                if i is None:
                    continue
                beyond = unit_end is not None and g["addr"] >= unit_end
                if beyond and not stub and (g["size"] or 0) > INLINE_SLOT_MAX:
                    continue
                if any(lo <= site < limit for site in self.refs.get(i, ())):
                    own = True
                if own:
                    break
            if not own:
                break
            stub = stub or (g["addr"] >= (unit_end if unit_end is not None else end))
            # the slot and what it calls after itself; a slot past the unit end brings only itself (its callees may be anyone's)
            bound = end if unit_end is None or g["addr"] < unit_end else g["addr"] + 1
            limit = max(limit, self.sinit_closure(g["addr"], bound)[1])
        return limit


# ---- the result store ---------------------------------------------------------------------------------------------------

class Results:
    def __init__(self):
        self.units = collections.OrderedDict()
        self.extra = []                      # defects not attached to a unit (unowned symbols, ...)

    def add(self, unit, inv, status, addr=None, finding="", item=None):
        rec = self.units.setdefault(unit, {}).setdefault(inv, {"status": NA, "addr": None, "finding": "",
                                                               "items": [], "n_fail": 0, "n_pass": 0})
        if status == FAIL:
            rec["n_fail"] += 1
        elif status == PASS:
            rec["n_pass"] += 1
        if RANK[status] > RANK[rec["status"]]:
            rec["status"] = status
            rec["addr"] = addr
            rec["finding"] = finding
        if status in (FAIL, UNKNOWN) and len(rec["items"]) < 12:
            rec["items"].append({"status": status, "addr": addr, "finding": finding, **(item or {})})

    def summary(self):
        out = {inv: {PASS: 0, FAIL: 0, UNKNOWN: 0, NA: 0} for inv in INVARIANTS}
        for recs in self.units.values():
            for inv, r in recs.items():
                out[inv][r["status"]] += 1
        return out


def hx(a):
    return "0x%08X" % a if a is not None else "-"


# ---- the invariants -----------------------------------------------------------------------------------------------------

def check_order(ctx, res):
    """One range per section, no overlap, no cycle in the union of the per-section orders."""
    for u in ctx.units:
        for sec, rr in u.ranges.items():
            plain = sorted((s, e) for s, e, a in rr if "rename:" not in a and "common" not in a)
            if len(plain) > 1:
                gaps = [(pe, s) for (_ps, pe), (s, _e) in zip(plain, plain[1:]) if s != pe]
                if gaps:
                    res.add(u.name, "order", FAIL, plain[1][0], "%s has %d ranges with a gap at %s (dtk takes one)" % (sec, len(plain), hx(gaps[0][0])))
                else:
                    res.add(u.name, "order", UNKNOWN, plain[1][0], "%s has %d abutting ranges (one range in effect)" % (sec, len(plain)))
            for s, e, _a in rr:
                if e <= s:
                    res.add(u.name, "order", FAIL, s, "%s range is empty or reversed (%s..%s)" % (sec, hx(s), hx(e)))
    edges = collections.defaultdict(set)
    where = {}
    for sec, v in ctx.sec_index.items():
        prev = None
        for s, e, u in v:
            if "rename:" in next((a for x, y, a in u.ranges[sec] if x == s), ""):
                continue                     # `.ctors$10` / `.dtors$15`: the linker script orders these, not the unit order
            if prev is not None:
                ps, pe, pu = prev
                if s < pe and pu is not u:
                    res.add(pu.name, "order", FAIL, s, "%s overlaps %s at %s..%s" % (sec, u.name, hx(s), hx(min(e, pe))))
                    res.add(u.name, "order", FAIL, s, "%s overlaps %s at %s..%s" % (sec, pu.name, hx(s), hx(min(e, pe))))
                if pu is not u:
                    edges[pu.name].add(u.name)
                    where[(pu.name, u.name)] = (sec, s)
            prev = (s, e, u) if prev is None or e > prev[1] else prev
    for comp in _sccs(edges):
        if len(comp) < 2:
            continue
        cs = set(comp)
        for name in comp:
            ev = [(where[(a, b)], a, b) for a in comp for b in edges[a] if b in cs and (a, b) in where]
            (sec, addr), a, b = min(ev, key=lambda t: t[0][1])
            res.add(name, "order", FAIL, addr, "link-order cycle of %d units (%s before %s at %s %s)"
                    % (len(comp), a, b, sec, hx(addr)), {"cycle": sorted(comp)[:8]})
    for u in ctx.units:
        res.add(u.name, "order", PASS)


def _sccs(edges):
    """Strongly connected components (iterative Tarjan)."""
    index, low, on, stack, out = {}, {}, set(), [], []
    counter = [0]
    nodes = set(edges) | {b for v in edges.values() for b in v}
    for root in sorted(nodes):
        if root in index:
            continue
        work = [(root, iter(sorted(edges.get(root, ()))))]
        index[root] = low[root] = counter[0]
        counter[0] += 1
        stack.append(root)
        on.add(root)
        while work:
            node, it = work[-1]
            adv = False
            for nxt in it:
                if nxt not in index:
                    index[nxt] = low[nxt] = counter[0]
                    counter[0] += 1
                    stack.append(nxt)
                    on.add(nxt)
                    work.append((nxt, iter(sorted(edges.get(nxt, ())))))
                    adv = True
                    break
                if nxt in on:
                    low[node] = min(low[node], index[nxt])
            if adv:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[node])
            if low[node] == index[node]:
                comp = []
                while True:
                    w = stack.pop()
                    on.discard(w)
                    comp.append(w)
                    if w == node:
                        break
                out.append(comp)
    return out


def check_coverage(ctx, res):
    """Every map symbol is inside exactly one unit's range of its section; the unowned runs are summarised."""
    runs = collections.defaultdict(list)                 # section -> [[start, end, n_symbols, last_owned_unit]]
    last_owned = {}
    for s in sorted(ctx.symbols, key=lambda x: (x["section"], x["addr"])):
        sec = s["section"]
        if sec not in SECTION_ORDER or sec not in ctx.sec_index or (s["type"] == "label" and not s["size"]):
            continue
        a, size = s["addr"], s["size"]
        u = ctx.owner(sec, a)
        if u is None:
            r = runs[sec]
            if r and r[-1][3] == last_owned.get(sec):
                r[-1][1] = a + size
                r[-1][2] += 1
            else:
                r.append([a, a + size, 1, last_owned.get(sec)])
            continue
        last_owned[sec] = u.name
        if size and sec != ".bss":
            e = ctx.owner(sec, a + size - 1)
            if e is not u:
                res.add(u.name, "coverage", FAIL, a, "%s %s (0x%X B) straddles the end of the unit's %s range"
                        % (s["type"] or "symbol", s["name"], size, sec))
    for u in ctx.units:
        res.add(u.name, "coverage", PASS)
    ctx.coverage_gaps = {sec: {"runs": len(r), "symbols": sum(x[2] for x in r), "bytes": sum(x[1] - x[0] for x in r),
                               "largest": [{"start": hx(x[0]), "end": hx(x[1]), "symbols": x[2]}
                                           for x in sorted(r, key=lambda x: -(x[1] - x[0]))[:3]]}
                         for sec, r in runs.items()}


def check_text_cut(ctx, res):
    for u in ctx.units:
        for sec in CODE_SECTIONS:
            for s, e, _a in u.ranges.get(sec, []):
                f = ctx.fn_at(s)
                if f is None or f["addr"] != s:
                    if f is None and not any(x["addr"] == s for x in ctx.symbols if False):
                        pass
                    res.add(u.name, "text-cut", FAIL if f is not None else UNKNOWN, s,
                            "%s starts %s" % (sec, ("inside %s (+0x%X)" % (f["name"], s - f["addr"])) if f
                                              else "where no function symbol is"))
                    continue
                last = ctx.fn_at(e - 1)
                if last is not None and last["addr"] + (last["size"] or 0) > e:
                    res.add(u.name, "text-cut", FAIL, e, "%s ends inside %s (+0x%X)" % (sec, last["name"], e - last["addr"]))
                else:
                    res.add(u.name, "text-cut", PASS)


def check_extab(ctx, res):
    ext = ctx.section_extent("extabindex")
    if not ext:
        return
    for a in range(ext[0], ext[1] - 11, 12):
        fn = ctx.dol.word(a)
        size = ctx.dol.word(a + 4)
        ex = ctx.dol.word(a + 8)
        if fn is None or size is None or ex is None:
            continue
        ue = ctx.owner("extabindex", a)
        uf = ctx.text_owner(fn)
        ux = ctx.owner("extab", ex)
        if ue is None:
            continue
        if uf is None or ux is None or uf is not ue or ux is not ue or (uf and fn + size > uf.ranges.get(".text", [(0, 0, "")])[-1][1] + 0 and ctx.text_owner(fn + size - 1) is not uf):
            why = []
            if uf is not ue:
                why.append("function %s is in %s" % (hx(fn), uf.name if uf else "no unit"))
            if ux is not ue:
                why.append("extab %s is in %s" % (hx(ex), ux.name if ux else "no unit"))
            if not why:
                why.append("function %s..%s runs past the unit's text" % (hx(fn), hx(fn + size)))
            res.add(ue.name, "extab", FAIL, a, "entry %s: %s" % (hx(a), "; ".join(why)))
            for other in (uf, ux):
                if other is not None and other is not ue:
                    res.add(other.name, "extab", FAIL, a, "entry %s in %s points at this unit's %s"
                            % (hx(a), ue.name, "text" if other is uf else "extab"))
        else:
            res.add(ue.name, "extab", PASS)
    for u in ctx.units:
        if u.ranges.get("extabindex") or u.ranges.get("extab"):
            res.add(u.name, "extab", PASS)


def check_ctors(ctx, res):
    for sec, inv in ((".ctors", "ctors"), (".dtors", "dtors")):
        for u in ctx.units:
            words = []
            for s, e, _a in u.ranges.get(sec, []):
                words += [(a, ctx.dol.word(a)) for a in range(s, e - 3, 4)]
            if not words:
                continue
            tr = u.ranges.get(".text", [])
            if sec == ".ctors" and len(words) > 1:
                res.add(u.name, inv, FAIL, words[1][0], "%d .ctors words: one TU has one __sinit, so this is %d or more TUs"
                        % (len(words), len(words)))
            for a, w in words:
                if not w:
                    res.add(u.name, inv, UNKNOWN, a, "zero word")
                    continue
                f = ctx.fn_at(w)
                if f is None:
                    res.add(u.name, inv, FAIL, a, "word %s is not a function" % hx(w))
                    continue
                tu = ctx.text_owner(w)
                if f["name"] in CRT_CHAIN:
                    res.add(u.name, inv, PASS)
                elif tu is not u:
                    res.add(u.name, inv, FAIL, a, "word %s is %s, which is in %s" % (hx(w), f["name"],
                                                                                     tu.name if tu else "no unit"))
                elif sec == ".ctors":
                    end = tr[-1][1] if tr else 0
                    sinit_end = f["addr"] + (f["size"] or 0)
                    if sinit_end > end:
                        res.add(u.name, inv, FAIL, a, "%s runs past the unit's text (text ends %s, it ends %s)"
                                % (f["name"], hx(end), hx(sinit_end)), {"cut_at": sinit_end})
                        continue
                    _fns, limit = ctx.sinit_closure(f["addr"], end)
                    # the own-slot run is followed past the unit end too: a cut placed over a unit's own deferred slots is early
                    limit = ctx.extend_over_own_vtable_slots(limit, tr[0][0] if tr else 0, UNBOUNDED, end)
                    if limit > end + SINIT_SLACK:
                        res.add(u.name, inv, FAIL, a, "%s's closure (sinit + its local callees + the slots of the vtables it stores) ends %s, past the unit text end %s: "
                                "the TU boundary is at %s, not %s" % (f["name"], hx(limit), hx(end), hx(limit), hx(end)), {"cut_at": limit})
                        continue
                    if limit + SINIT_SLACK >= end:
                        res.add(u.name, inv, PASS)
                        continue
                    g = ctx.fn_at(limit)
                    lo = tr[0][0] if tr else 0
                    before = ctx.refs_to_fn(limit, lo, limit) if g is not None and g["addr"] == limit else []
                    if before:
                        site, kind = before[0]
                        res.add(u.name, inv, UNKNOWN, a, "%s's closure (sinit + its local callees) ends %s, before the unit end %s, but %s at %s is %s from %s, "
                                "inside the unit: the boundary at %s is not confirmed" % (f["name"], hx(limit), hx(end), g["name"], hx(limit),
                                                                                         "called" if kind == "call" else "address-taken", hx(site), hx(limit)),
                                {"closure_end": limit})
                    else:
                        res.add(u.name, inv, FAIL, a, "%s's closure (sinit + its local callees) ends %s, unit text ends %s: a TU boundary is at %s"
                                % (f["name"], hx(limit), hx(end), hx(limit)), {"cut_at": limit})
                else:
                    res.add(u.name, inv, PASS)


LITERAL_KINDS = ("float", "double")
#: alignment padding a unit may keep after its last function (a 16-byte-aligned function start)
SINIT_SLACK = 0xC
#: an end bound that never binds (the own-slot run is followed past a unit's end)
UNBOUNDED = 0xFFFFFFFF
#: the largest slot function past a unit's end that still reads as an inline virtual the unit emitted (a stub, a trivial accessor)
INLINE_SLOT_MAX = 0x40
#: crt chain entries a runtime unit registers for another unit's function
CRT_CHAIN = ("__destroy_global_chain", "__init_cpp_exceptions", "__fini_cpp_exceptions")


def pool_literals(ctx, u):
    """(symbol, is_dedupe_witness) for the literals of a unit's `.sdata2` / `.sdata` ranges."""
    out = []
    for sec in (".sdata2", ".sdata"):
        for s, e, _a in u.ranges.get(sec, []):
            i = bisect.bisect_left(ctx._ds_addr, s)
            while i < len(ctx.data_syms) and ctx._ds_addr[i] < e:
                sym = ctx.data_syms[i]
                i += 1
                if sym["section"] != sec:
                    continue
                if sec == ".sdata2" and sym["size"] in (4, 8) and sym["type"] == "object":
                    out.append((sym, sym["kind"] in LITERAL_KINDS))
                elif sec == ".sdata" and sym["kind"] == "string":
                    out.append((sym, False))
    return out


def is_literal(sym):
    return ((sym["section"] == ".sdata2" and sym["size"] in (4, 8) and sym["type"] == "object")
            or (sym["section"] == ".sdata" and sym["kind"] == "string"))


def _first_use_fn(ctx, site):
    """The start of the function holding `site` (the site itself when the map has no function there)."""
    f = ctx.fn_at(site)
    return f["addr"] if f is not None else site


def check_pool(ctx, res):
    """Idea 94.  Two halves: what a unit's own pool claim holds, and what its text reads wherever the pool lives."""
    ctx.pool_edges = collections.defaultdict(set)              # (unit, other) -> literal addresses
    per_unit = collections.defaultdict(lambda: collections.defaultdict(list))   # unit -> (size, value) -> [literal sym]
    reads = collections.defaultdict(int)
    for i, sym in enumerate(ctx.data_syms):
        if not is_literal(sym):
            continue
        sites = ctx.literal_readers(sym)
        units = collections.OrderedDict()
        for x in sites:
            u = ctx.text_owner(x)
            if u is not None:
                units.setdefault(u.name, u)
        for n in units:
            reads[n] += 1
        if len(units) >= 2:
            names = list(units)
            for n in names:
                others = [o for o in names if o != n]
                for o in others:
                    ctx.pool_edges[(n, o)].add(sym["addr"])
                res.add(n, "pool", FAIL, sym["addr"], "literal %s is also read by %s (one pool per TU: the units are one TU, or one read is a false decode)"
                        % (sym["name"], ", ".join(others[:3])))
        if sym["kind"] in LITERAL_KINDS:
            v = ctx.dol.read(sym["addr"], sym["size"])
            for n in units:
                per_unit[n][(sym["size"], v)].append(sym)
    for n, vals in per_unit.items():
        for (size, v), lits in vals.items():
            if len(lits) > 1 and v is not None:
                res.add(n, "pool", FAIL, lits[1]["addr"], "reads value 0x%s at %s and at %s (one entry per value per TU: two TUs)"
                        % (v.hex(), hx(lits[0]["addr"]), hx(lits[1]["addr"])))
    for u in ctx.units:
        lits = pool_literals(ctx, u)
        if lits:
            own_first = []
            seen = {}
            for sym, witness in lits:
                own = [x for x in ctx.literal_readers(sym) if ctx.text_owner(x) is u]
                if witness:
                    v = ctx.dol.read(sym["addr"], sym["size"])
                    key = (sym["size"], v)
                    if v is not None and key in seen:
                        res.add(u.name, "pool", FAIL, sym["addr"], "claimed pool holds value 0x%s at %s and again at %s (one entry per value per TU)"
                                % (v.hex(), hx(seen[key]), hx(sym["addr"])))
                    elif v is not None:
                        seen[key] = sym["addr"]
                if own:
                    first = min(own)
                    own_first.append((sym["addr"], first, sym["name"], _first_use_fn(ctx, first), sym["section"]))
            own_first.sort()
            # order is judged per FUNCTION: the scheduler reorders two loads of one function, so literals first used
            # in the same function carry no order; a literal first used in an EARLIER function than the one before it does.
            # Per SECTION: the `.sdata` strings and the `.sdata2` literals are two pools (the strings sit at lower addresses, so one
            # address-sorted list would call every late string followed by an early float an inversion); the lowest-addressed inversion is reported
            bad = None
            for psec in sorted({t[4] for t in own_first}):
                run = [t for t in own_first if t[4] == psec]
                hit = next(((a, f, n, pf) for (_pa, pf, _pn, pfn, _ps), (a, f, n, fn, _qs) in zip(run, run[1:]) if fn < pfn), None)
                if hit is not None and (bad is None or hit[0] < bad[0]):
                    bad = hit
            if bad:
                res.add(u.name, "pool", FAIL, bad[0], "claimed pool: first use of %s (%s) is in an earlier function than the literal before it (%s): not text order"
                        % (bad[2], hx(bad[1]), hx(bad[3])))
            foreign_only = [sym for sym, _w in lits if ctx.literal_readers(sym) and not any(ctx.text_owner(x) is u for x in ctx.literal_readers(sym))]
            if foreign_only and not any(ctx.text_owner(x) is u for sym, _w in lits for x in ctx.literal_readers(sym)):
                res.add(u.name, "pool", FAIL, foreign_only[0]["addr"], "claimed pool %s is read by no code of this unit (readers: %s)"
                        % (foreign_only[0]["name"], sorted({ctx.text_owner(x).name for x in ctx.literal_readers(foreign_only[0]) if ctx.text_owner(x)})[0]))
        if reads.get(u.name) or lits:
            res.add(u.name, "pool", PASS if reads.get(u.name) or any(ctx.literal_readers(sym) for sym, _w in lits) else UNKNOWN,
                    None, "no literal is read by decoded code")


def unit_sinit_closure(ctx, u):
    """The function starts of the unit's `__sinit`s and everything they call or take the address of after them (the deferred
    constructors this TU instantiates); empty when the unit has no `.ctors` word."""
    out = set()
    tr = u.ranges.get(".text", [])
    end = tr[-1][1] if tr else 0
    for s, e, _a in u.ranges.get(".ctors", []):
        for a in range(s, e - 3, 4):
            w = ctx.dol.word(a)
            f = ctx.fn_at(w) if w else None
            if f is not None and ctx.text_owner(w) is u:
                out |= ctx.sinit_closure(f["addr"], end)[0]
    return out


def _vtable_before(syms, seam):
    """The vtable symbol address of a `V->S` seam's leading group (its `before` name), or None."""
    for y in syms:
        if y.name == seam.get("before"):
            return y.addr
    return None


def _vtable_stored_in(ctx, vt, fns):
    """Whether a function in `fns` references the `.data` vtable `vt` (a `dataorder.Sym`): its constructor is one of them."""
    if vt is None:
        return False
    i, sym = ctx.data_sym_at(vt.addr)
    if sym is None or sym["addr"] != vt.addr:
        return False
    return any((ctx.fn_at(x) or {}).get("addr") in fns for x in ctx.refs.get(i, []))


def zigzag_interleaved(ctx, syms, by_addr, seam):
    """Whether the two vtables of a zigzag pair have member functions that alternate in the text (each class's own slots - the ones the
    other table does not share - span past the other's).  A TU is one contiguous text range, so no cut separates two classes whose
    members interleave: the pair is one TU, and the `owner goes up` reading of the first slots is not an edge."""
    a = by_addr.get(_vtable_before(syms, seam))
    b = by_addr.get(seam["addr"])
    if a is None or b is None:
        return False
    fs = set(ctx._fn_starts)

    def slots(y):
        return {w for w in (ctx.dol.word(y.addr + o) for o in range(8, y.size - 3, 4)) if w in fs}

    sa, sb = slots(a), slots(b)
    own_a, own_b = sa - sb, sb - sa
    return bool(own_a and own_b and min(own_a) < max(own_b) and min(own_b) < max(own_a))


def check_data_order(ctx, res, rows):
    import dataorder as do
    reader = ctx.dol
    syms = do.classify_all(rows, reader)
    ctx.data_order = syms
    found = do.seams(syms)
    by_addr = {y.addr: y for y in syms}
    ctx.seams_instantiated = []
    for u in ctx.units:
        for s, e, _a in u.ranges.get(".data", []):
            # a V->S row says a boundary lies in [addr, latest): a unit that ends inside that gap (latest >= e) can have it at its own end,
            # so only a unit that also holds the later vtable group crosses the seam; a zigzag cuts at addr
            inside = [x for x in found if x["kind"] in do.STRONG_KINDS and s < x["addr"] < e and (x["kind"] != "V->S" or x["latest"] < e)]
            # two classes whose member functions alternate in the text are one TU: a zigzag between them is not an edge
            inside = [x for x in inside if not (x["kind"] == "zigzag" and zigzag_interleaved(ctx, syms, by_addr, x))]
            if inside:
                closure = unit_sinit_closure(ctx, u)
                kept = []
                for x in inside:
                    # an instantiated vtable (a deferred constructor of this TU stores it) is not a TU edge: the later vtable of a V->S seam,
                    # the second of a zigzag pair, or the vtable before either
                    if closure and any(_vtable_stored_in(ctx, by_addr.get(a_), closure) for a_ in (x.get("latest") or x["addr"], _vtable_before(syms, x))):
                        ctx.seams_instantiated.append({"unit": u.name, "addr": x["addr"], "vtable": x["before"]})
                    else:
                        kept.append(x)
                inside = kept
            has_v = any(s <= y.addr < e and y.kind == do.VTABLE for y in syms)
            if inside:
                x = inside[0]
                res.add(u.name, "data-order", FAIL, x["addr"], "strong %s seam at %s inside the unit's .data (a boundary in [%s, %s)): at least %d TUs"
                        % (x["kind"], hx(x["addr"]), hx(x["addr"]), hx(x.get("latest", x["addr"])), len(inside) + 1))
            elif has_v:
                res.add(u.name, "data-order", PASS)


def check_vtable(ctx, res):
    syms = getattr(ctx, "data_order", None)
    if syms is None:
        return
    for y in syms:
        if y.kind != "V":
            continue
        u = ctx.owner(".data", y.addr)
        if u is None:
            continue
        slots = [ctx.dol.word(y.addr + o) for o in range(8, y.size - 3, 4)]
        slots = [w for w in slots if w]
        in_unit = [w for w in slots if ctx.text_owner(w) is u]
        if in_unit:
            res.add(u.name, "vtable", PASS)
            continue
        ds = next((s for s in ctx.data_syms if s["addr"] == y.addr and s["section"] == ".data"), None)
        stores = [x for x in (ctx.readers(ds) if ds else []) if ctx.text_owner(x) is u]
        if stores:
            res.add(u.name, "vtable", PASS)
            continue
        owners = sorted({ctx.text_owner(w).name for w in slots if ctx.text_owner(w)})
        if owners:
            res.add(u.name, "vtable", FAIL, y.addr, "vtable %s has no slot in this unit; its slots are in %s" % (y.name, ", ".join(owners[:3])))
        else:
            res.add(u.name, "vtable", UNKNOWN, y.addr, "vtable %s: no slot or constructor store is owned by any unit" % y.name)


def check_jumptable(ctx, res):
    for s in ctx.symbols:
        if not s["name"].startswith("jumptable_") or s["section"] not in (".data", ".rodata"):
            continue
        u = ctx.owner(s["section"], s["addr"])
        if u is None:
            continue
        targets = [ctx.dol.word(s["addr"] + o) for o in range(0, s["size"] - 3, 4)]
        targets = [t for t in targets if t]
        tu = {ctx.text_owner(t) for t in targets if ctx.fn_at(t)}
        rsites = ctx.readers(s)
        ru = {ctx.text_owner(x) for x in rsites if ctx.text_owner(x)}
        bad = sorted((x for x in ru if x is not u), key=lambda x: x.name)           # a set of units: name order, so the finding is the same on every run
        if bad:
            res.add(u.name, "jumptable", FAIL, s["addr"], "%s is read by %s, not by this unit" % (s["name"], bad[0].name))
        elif tu and tu != {u}:
            other = sorted(x.name if x else "no unit" for x in tu - {u})
            res.add(u.name, "jumptable", FAIL, s["addr"], "%s branches into %s" % (s["name"], ", ".join(other[:3])))
        elif ru or tu:
            res.add(u.name, "jumptable", PASS)
        else:
            res.add(u.name, "jumptable", UNKNOWN, s["addr"], "%s: no decoded reader and no code target" % s["name"])


def check_bss(ctx, res):
    """A local `.bss`/`.sbss` object is read by the unit that holds it; a unit whose range no decoded code touches is UNKNOWN."""
    for u in ctx.units:
        own = foreign_global = 0
        for sec in (".bss", ".sbss", ".sbss2"):
            for s0, e0, _a in u.ranges.get(sec, []):
                i = bisect.bisect_left(ctx._ds_addr, s0)
                while i < len(ctx.data_syms) and ctx._ds_addr[i] < e0:
                    sym = ctx.data_syms[i]
                    i += 1
                    if sym["section"] != sec:
                        continue
                    ous = {ctx.text_owner(x) for x in ctx.readers(sym) if ctx.text_owner(x)}
                    if not ous:
                        continue
                    if u in ous:
                        own += 1
                    elif sym["scope"] == "local":
                        res.add(u.name, "bss", FAIL, sym["addr"], "local %s is read only by %s" % (sym["name"], sorted(o.name for o in ous)[0]))
                    else:
                        foreign_global += 1
        if u.ranges.get(".bss") or u.ranges.get(".sbss") or u.ranges.get(".sbss2"):
            res.add(u.name, "bss", PASS if own else UNKNOWN, None,
                    "" if own else "no object of the range is read by this unit's decoded code (%d read only elsewhere)" % foreign_global)


def check_local_static(ctx, res):
    """A `scope:local` data object is private to one TU: its decoded readers must lie in one unit.  A reader in two units is a
    FAIL for both (the units are one TU, or one read is a false decode - stated in the finding, like the pool check's)."""
    for sym in ctx.data_syms:
        if sym["scope"] != "local" or sym["type"] != "object" or is_literal(sym) or sym["section"] == ".sdata2":
            continue
        if sym["name"].startswith("jumptable_"):
            continue                                       # a switch table: the `jumptable` invariant owns it (its base register is reused across units)
        units = collections.OrderedDict()
        for x in ctx.readers(sym):
            u = ctx.text_owner(x)
            if u is not None:
                units.setdefault(u.name, (u, x))
        if len(units) < 2:
            for n in units:
                res.add(n, "local-static", PASS)
            continue
        names = list(units)
        for n in names:
            others = [o for o in names if o != n]
            res.add(n, "local-static", FAIL, sym["addr"],
                    "local %s (%s %s) is read at %s by this unit and at %s by %s (a static is one TU's: the units are one TU, or one read is a false decode)"
                    % (sym["name"], sym["section"], hx(sym["addr"]), hx(units[n][1]), hx(units[others[0]][1]), ", ".join(others[:3])),
                    {"symbol": sym["name"], "units": names})


def local_static_crossings(ctx, cuts):
    """`[(cut, grade, unit, symbol, lo_reader, hi_reader)]`: a proposed cut (every grade, a guess included - a guess is merged away
    and so never reaches the candidate's `local-static` check) that a `scope:local` data object is read across, one decoded reader
    below the cut and one at or above it: a static is one TU's, so a TU start cannot sit between its readers."""
    spans = []
    for sym in ctx.data_syms:
        if sym["scope"] != "local" or sym["type"] != "object" or is_literal(sym) or sym["section"] == ".sdata2" \
                or sym["name"].startswith("jumptable_"):
            continue
        sites = sorted(ctx.readers(sym))                    # unowned text counts too: a proposal cuts most of it
        if len(sites) >= 2:
            spans.append((sites[0], sites[-1], sym, sites))
    out = []
    for addr, grade, unit in sorted(cuts):
        for lo, hi, sym, sites in spans:
            if lo < addr <= hi:
                below = max(x for x in sites if x < addr)
                above = min(x for x in sites if x >= addr)
                out.append((addr, grade, unit, sym["name"], below, above))
    return out


def ctors_detail(ctx, u, sec=".ctors"):
    """One line per `.ctors`/`.dtors` word of `u`: the word, the function it names, that function's own end, the closure end the
    ctors check derives (sinit + local callees + own vtable slots) and the unit's text end - the numbers behind a ctors finding."""
    out = []
    tr = u.ranges.get(".text", [])
    end = tr[-1][1] if tr else 0
    for s0, e0, _a in u.ranges.get(sec, []):
        for a in range(s0, e0 - 3, 4):
            w = ctx.dol.word(a)
            f = ctx.fn_at(w) if w else None
            if f is None:
                out.append("%s word %s = %s: not a function" % (sec, hx(a), hx(w or 0)))
                continue
            if sec != ".ctors" or f["name"] in CRT_CHAIN or ctx.text_owner(w) is not u:
                out.append("%s word %s = %s (%s, ends %s)%s" % (sec, hx(a), hx(w), f["name"], hx(f["addr"] + (f["size"] or 0)),
                                                                 "" if ctx.text_owner(w) is u else " in %s" % (ctx.text_owner(w).name if ctx.text_owner(w) else "no unit")))
                continue
            _fns, limit = ctx.sinit_closure(f["addr"], end)
            ext = ctx.extend_over_own_vtable_slots(limit, tr[0][0] if tr else 0, UNBOUNDED, end)
            out.append("%s word %s = %s (%s, ends %s): closure ends %s, with own vtable slots %s; unit text ends %s"
                       % (sec, hx(a), hx(w), f["name"], hx(f["addr"] + (f["size"] or 0)), hx(limit), hx(ext), hx(end)))
    return out


def pool_intervals(ctx, keep=None):
    """`[line]`: for each value held at two or more pool addresses and read from one unit, the pair of consecutive copies, the last
    read of the first and the first read of the second, and the function starts between them (a TU starts in `(fn(last), fn(first)]`).
    `keep(unit_name)` filters by reader unit."""
    groups = collections.defaultdict(list)
    for sym in ctx.data_syms:
        if sym["kind"] in LITERAL_KINDS and sym["section"] == ".sdata2" and sym["size"] in (4, 8) and sym["type"] == "object":
            sites = sorted(ctx.literal_readers(sym))
            if sites:
                groups[(sym["size"], ctx.dol.read(sym["addr"], sym["size"]))].append((sym, sites))
    out = []
    for (size, v), lits in sorted(groups.items(), key=lambda kv: kv[1][0][0]["addr"]):
        lits.sort(key=lambda t: t[0]["addr"])
        for (a, sa), (b, sb) in zip(lits, lits[1:]):
            owners = {ctx.text_owner(x).name for x in sa + sb if ctx.text_owner(x)} or {"unowned"}
            if keep is not None and not any(keep(n) for n in owners):
                continue
            ua = {ctx.text_owner(x) for x in sa if ctx.text_owner(x)}
            ub = {ctx.text_owner(x) for x in sb if ctx.text_owner(x)}
            split_note = ""
            if ua and ub and not (ua & ub):
                split_note = " [already two units: %s | %s]" % (sorted(x.name for x in ua)[0], sorted(x.name for x in ub)[0])
            last, first = sa[-1], sb[0]
            fl, ff = _first_use_fn(ctx, last), _first_use_fn(ctx, first)
            n = bisect.bisect_right(ctx._fn_starts, ff) - bisect.bisect_right(ctx._fn_starts, fl)
            out.append("pooldup value 0x%s: %s %s (last read %s) and %s %s (first read %s)%s" % (
                (v or b"").hex(), a["name"], hx(a["addr"]), hx(last), b["name"], hx(b["addr"]), hx(first),
                (("; a TU starts in (%s, %s]: %d function starts" % (hx(fl), hx(ff), n)) if fl < ff else "; the reads interleave (no interval)") + split_note))
    return out


def boundaries(ctx, res):
    """One record per adjacent pair of text units: the function start and the pool literals read across it."""
    tu = sorted((u for u in ctx.units if u.ranges.get(".text")), key=lambda u: u.first(".text"))
    out = []
    for a, b in zip(tu, tu[1:]):
        addr = b.first(".text")
        rec = {"addr": addr, "left": a.name, "right": b.name, "checks": {}}
        f = ctx.fn_at(addr)
        rec["checks"]["fn-start"] = PASS if f and f["addr"] == addr else FAIL
        shared = sorted(ctx.pool_edges.get((a.name, b.name), set()) | ctx.pool_edges.get((b.name, a.name), set())) \
            if getattr(ctx, "pool_edges", None) else []
        rec["checks"]["pool-shared"] = FAIL if shared else PASS
        if shared:
            rec["pool_shared"] = [hx(x) for x in shared[:6]]
        out.append(rec)
    return out


def run_checks(ctx, rows_for_dataorder=None, only=None):
    res = Results()
    want = set(only or INVARIANTS)
    if "order" in want:
        check_order(ctx, res)
    if "coverage" in want:
        check_coverage(ctx, res)
    if "text-cut" in want:
        check_text_cut(ctx, res)
    if "extab" in want:
        check_extab(ctx, res)
    if want & {"ctors", "dtors"}:
        check_ctors(ctx, res)
    if "pool" in want or True:
        check_pool(ctx, res)
    if want & {"data-order", "vtable"} and rows_for_dataorder is not None:
        check_data_order(ctx, res, rows_for_dataorder)
        check_vtable(ctx, res)
    if "jumptable" in want:
        check_jumptable(ctx, res)
    if "bss" in want:
        check_bss(ctx, res)
    if "local-static" in want:
        check_local_static(ctx, res)
    if only:
        for name in list(res.units):
            for inv in list(res.units[name]):
                if inv not in want:
                    del res.units[name][inv]
    return res


# ---- the audit list -------------------------------------------------------------------------------------------------------

def top_defects(ctx, res, limit=10):
    items = []
    for name, recs in res.units.items():
        for inv, r in recs.items():
            if r["status"] != FAIL:
                continue
            u = ctx.by_name.get(name)
            size = u.size(".text") if u else 0
            items.append({"unit": name, "invariant": inv, "addr": r["addr"], "finding": r["finding"],
                          "fails": r["n_fail"], "score": WEIGHT.get(inv, 10) * 1000 + min(r["n_fail"], 99) * 10 + size // 0x1000})
    items.sort(key=lambda d: -d["score"])
    return items[:limit]


def pool_groups(ctx):
    """Units chained by a literal two of them read: the `docs/pool-seams.md` groups, recomputed from the decode."""
    parent = {}

    def find(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for (a, b) in getattr(ctx, "pool_edges", {}):
        parent[find(a)] = find(b)
    groups = collections.defaultdict(set)
    for (a, b) in getattr(ctx, "pool_edges", {}):
        groups[find(a)].update((a, b))
    out = []
    for members in groups.values():
        us = sorted(members, key=lambda n: ctx.by_name[n].lo() if n in ctx.by_name else 0)
        out.append({"units": us, "lo": hx(ctx.by_name[us[0]].lo()) if us[0] in ctx.by_name else "-",
                    "literals": sum(len(v) for k, v in ctx.pool_edges.items() if k[0] in members and k[1] in members)})
    out.sort(key=lambda g: -len(g["units"]))
    return out


def seam_requests(ctx, outbox):
    """Every `{"kind": "seam"}` request in the lane outbox, with the unit that owns its address."""
    out = []
    if not outbox or not os.path.isdir(outbox):
        return out

    def num(v):
        if isinstance(v, int):
            return v
        try:
            return int(str(v), 16) if str(v).lower().startswith("0x") else int(v)
        except (TypeError, ValueError):
            return None

    def walk(o, f):
        if isinstance(o, dict):
            if o.get("kind") == "seam":
                addr = next((num(o[k]) for k in ("new", "addr", "start", "old") if k in o and num(o[k]) is not None), None)
                u = ctx.text_owner(addr) if addr is not None else None
                out.append({"file": f, "addr": hx(addr) if addr is not None else "-", "unit": u.name if u else None,
                            "section": o.get("section", ".text"), "evidence": str(o.get("evidence", ""))[:200]})
            for v in o.values():
                walk(v, f)
        elif isinstance(o, list):
            for v in o:
                walk(v, f)

    for fn in sorted(os.listdir(outbox)):
        if fn.endswith(".json"):
            try:
                with open(os.path.join(outbox, fn), encoding="utf-8") as fh:
                    walk(json.load(fh), fn)
            except (OSError, ValueError):
                pass
    return out


# ---- proposals ------------------------------------------------------------------------------------------------------------

def to_int(v):
    if isinstance(v, int):
        return v
    return int(str(v), 16) if str(v).lower().startswith("0x") else int(v)


def load_proposal(path):
    with open(path, encoding="utf-8") as fh:
        p = json.load(fh)
    if isinstance(p, dict):
        p["_file"] = os.path.basename(path)          # only for messages: a `_`-prefixed key is not a field
    return p


#: the proposal file's fields: name -> accepted types.  An unknown key is a lint WARNING, a wrong type an ERROR.
TOP_FIELDS = {"phase": (int, str), "band": (str,), "text_range": (list,), "range": (list,), "units": (list,), "open_questions": (list,),
              "supersedes": (list,), "attach": (list,), "unowned_data": (list,), "moves": (list,), "lane": (str,), "window": (list,)}
#: phase 2: the canonical name of a list is `attach`; the names the first six lanes used are accepted on load (a lint warning names the file)
ATTACH_ALIASES = {"data_attach": "attach", "attachments": "attach"}
UNIT_FIELDS = {"derived_name": (str,), "module": (str,), "ranges": (dict,), "cuts": (list,), "open_questions": (list,),
               "removes_cuts": (list,), "absorbs": (list,), "replaces_tail_of": (str, list), "after": (str,)}
CUT_FIELDS = {"addr": (int, str), "section": (str,), "grade": (str,), "evidence": (list,), "reproduce": (str,), "kind": (str,),
              "keep_registered_edge": (bool,)}
#: phase 2 `attach` row: ONE data range (or a few, of one section) given to a unit of the candidate.  The unit is named (`unit`: a
#: candidate unit name, a proposal unit's `derived_name`, or a name a guess merge / `absorbs` folded away) or addressed by the `.text`
#: address it holds (`text_addr`: survives renames, which a merged unit does).  The range is `section` + `range` [start, end] or
#: `ranges` ([[start, end], ...] with `section`, or `{section: [[start, end], ...]}`) or `section` + `start` + `end`.
ATTACH_FIELDS = {"unit": (str,), "text_addr": (int, str), "section": (str,), "range": (list,), "ranges": (list, dict), "start": (int, str),
                 "end": (int, str), "grade": (str,), "evidence": (list,), "reproduce": (str,), "kind": (str,), "signal": (str, int),
                 "signals": (list, dict), "symbols": (int,), "bytes": (int,), "note": (str,), "takes_from": (str,), "provisional": (bool,),
                 "edges": (dict,)}
#: phase 2 `unowned_data` row: a run nobody could be given (a deferral): the candidate owners and why, never applied
UNOWNED_FIELDS = {"section": (str,), "range": (list,), "start": (int, str), "end": (int, str), "symbols": (int,), "bytes": (int,),
                  "candidates": (list,), "kind": (str,), "reason": (str,), "why": (dict, str), "class": (str,), "candidate_count": (int,),
                  "evidence": (list, str), "reproduce": (str,), "note": (str,), "signals": (dict,), "clusters": (list,), "readers": (dict, list),
                  "provisional": (bool,), "bounds": (dict,), "evidence_units": (list,), "cut_interval": (list,)}
MOVE_FIELDS = {"unit": (str,), "after": (str,), "reason": (str,), "evidence": (list, str), "reproduce": (str,)}


def canonical_proposal(p):
    """The proposal with its phase 2 list under the canonical `attach` (a legacy name is moved, not copied); `p` itself is left alone.
    Returns `(proposal, errors, warnings)`: both names in one file is an error, a legacy name a warning that names the file."""
    errs, warns = [], []
    if not isinstance(p, dict):
        return p, errs, warns
    for old, new in ATTACH_ALIASES.items():
        if old in p:
            if new in p:
                errs.append("proposal %s: has both `%s` and `%s`" % (p.get("_file", "?"), old, new))
                continue
            p = {(new if k == old else k): v for k, v in p.items()}
            warns.append("proposal %s: field `%s` is the legacy name of `%s`" % (p.get("_file", "?"), old, new))
    return p, errs, warns


def norm_units(proposal):
    """The proposal's units with integer ranges and cuts, in text order."""
    out = []
    for pu in proposal.get("units", []):
        rng = {sec: sorted((to_int(a), to_int(b)) for a, b in rr) for sec, rr in (pu.get("ranges") or {}).items()}
        cuts = []
        for c in pu.get("cuts", []):
            cuts.append(dict(c, addr=to_int(c["addr"])))
        out.append({"name": "%s/%s.cpp" % (pu["module"], pu["derived_name"]), "derived_name": pu["derived_name"],
                    "module": pu["module"], "ranges": rng, "cuts": cuts, "open_questions": pu.get("open_questions", []),
                    "removes_cuts": pu.get("removes_cuts", []), "absorbs": pu.get("absorbs", []),
                    "replaces_tail_of": pu.get("replaces_tail_of"), "after": pu.get("after"), "merged": []})
    out.sort(key=lambda u: (u["ranges"].get(".text") or [(min(a for rr in u["ranges"].values() for a, _b in rr), 0)])[0][0]
             if u["ranges"] else 0)
    return out


def _check_fields(obj, spec, who, errors, warnings):
    """Unknown key -> warning; wrong type -> error."""
    for k, v in obj.items():
        if k.startswith("_"):
            continue
        if k not in spec:
            warnings.append("%s: unknown field %r" % (who, k))
        elif not isinstance(v, spec[k]) or (bool not in spec[k] and isinstance(v, bool)):
            errors.append("%s: field %r is %s, want %s" % (who, k, type(v).__name__, "|".join(t.__name__ for t in spec[k])))


def _lst(v):
    return v if isinstance(v, list) else []


def _rng(a, b):
    return (to_int(a), to_int(b))


def _row_spans(row):
    """`{section: [(start, end), ...]}` of an `attach` row in any of its accepted shapes (`ranges` as a dict or a list, `range`,
    `start`/`end`); raises KeyError/TypeError/ValueError when the row has none or a malformed one."""
    if isinstance(row.get("ranges"), dict):
        return {sec: [_rng(a, b) for a, b in rr] for sec, rr in row["ranges"].items()}
    if isinstance(row.get("ranges"), list):
        return {row["section"]: [_rng(a, b) for a, b in row["ranges"]]}
    if "range" in row:
        a, b = row["range"]
        return {row["section"]: [_rng(a, b)]}
    return {row["section"]: [_rng(row["start"], row["end"])]}


def attach_entries(p):
    """The well-formed `attach` rows of a canonical proposal, one per (row, section):
    `{"row", "unit", "text_addr", "section", "ranges": [(a, b)], "grade", "takes_from"}`.  A malformed row is skipped (the linter names it)."""
    out = []
    for row in _lst(p.get("attach")):
        if not isinstance(row, dict):
            continue
        try:
            spans = _row_spans(row)
            ta = to_int(row["text_addr"]) if row.get("text_addr") not in (None, "") else None
        except (KeyError, TypeError, ValueError):
            continue
        for sec, rr in spans.items():
            out.append({"row": row, "unit": row.get("unit"), "text_addr": ta, "section": sec, "ranges": rr, "grade": row.get("grade"),
                        "takes_from": row.get("takes_from")})
    return out


def unowned_entries(p):
    """The well-formed `unowned_data` rows: `{"row", "section", "a", "b", "candidates": [names], "provisional"}` (`range` or `start`/`end`;
    a candidate is a name or an object with a `unit`)."""
    out = []
    for row in _lst(p.get("unowned_data")):
        if not isinstance(row, dict):
            continue
        try:
            a, b = _rng(*row["range"]) if "range" in row else _rng(row["start"], row["end"])
        except (KeyError, TypeError, ValueError):
            continue
        cands = [c["unit"] if isinstance(c, dict) and "unit" in c else c for c in _lst(row.get("candidates"))]
        out.append({"row": row, "section": row.get("section"), "a": a, "b": b, "candidates": [c for c in cands if isinstance(c, str)],
                    "provisional": row.get("provisional") is True})
    return out


def _lint_phase2(p, issues, warns):
    """Shape of the phase 2 lists: `attach` (a unit or `text_addr`, a data section, a non-empty range, a grade, a reproduce command, evidence for
    strong/medium; two non-guess rows may not overlap), `unowned_data` (section, range, candidates, a stated reason) and `moves`."""
    seen = collections.defaultdict(list)
    for k, row in enumerate(_lst(p.get("attach"))):
        if not isinstance(row, dict):
            issues.append("attach[%d] is %s, want an object" % (k, type(row).__name__))
            continue
        who = "attach %s" % (row.get("unit") or row.get("text_addr") or "#%d" % k)
        _check_fields(row, ATTACH_FIELDS, who, issues, warns)
        if not (row.get("unit") or row.get("text_addr")):
            issues.append("%s: missing unit or text_addr" % who)
        if row.get("grade") not in GRADES:
            issues.append("%s: grade %r (strong|medium|guess)" % (who, row.get("grade")))
        if not row.get("reproduce"):
            issues.append("%s: no reproduce command" % who)
        if row.get("grade") in ("strong", "medium") and not row.get("evidence"):
            issues.append("%s: %s attach has no evidence" % (who, row.get("grade")))
        for ev in _lst(row.get("evidence")):
            if not (isinstance(ev, dict) and all(ev.get(x) for x in ("tool", "command", "finding"))):
                issues.append("%s: evidence needs tool, command and finding" % who)
        if row.get("text_addr") not in (None, ""):
            try:
                to_int(row["text_addr"])
            except (TypeError, ValueError):
                issues.append("%s: text_addr is not an address" % who)
        try:
            spans = _row_spans(row)
        except (KeyError, TypeError, ValueError):
            issues.append("%s: the range is missing or not [start, end]" % who)
            continue
        if not spans:
            issues.append("%s: missing ranges" % who)
        for sec, rr in spans.items():
            if sec not in SECTION_ORDER or sec in CODE_SECTIONS:
                issues.append("%s: %s is not a data section" % (who, sec))
                continue
            for a, b in rr:
                if b <= a:
                    issues.append("%s: %s range %s..%s is empty" % (who, sec, hx(a), hx(b)))
                elif row.get("grade") != "guess":
                    seen[sec].append((a, b, who))
    for sec, v in seen.items():
        v.sort()
        for (a, b, w), (c, d, x) in zip(v, v[1:]):
            if c < b:
                issues.append("%s and %s overlap in %s at %s" % (w, x, sec, hx(c)))
    for k, row in enumerate(_lst(p.get("unowned_data"))):
        who = "unowned_data[%d]" % k
        if not isinstance(row, dict):
            issues.append("%s is %s, want an object" % (who, type(row).__name__))
            continue
        _check_fields(row, UNOWNED_FIELDS, who, issues, warns)
        if row.get("section") not in SECTION_ORDER:
            issues.append("%s: section %r" % (who, row.get("section")))
        try:
            a, b = _rng(*row["range"]) if "range" in row else _rng(row["start"], row["end"])
            if b <= a:
                issues.append("%s: range %s..%s is empty" % (who, hx(a), hx(b)))
        except (KeyError, TypeError, ValueError):
            issues.append("%s: the range is missing or not [start, end]" % who)
        if not isinstance(row.get("candidates"), list):
            issues.append("%s: candidates must list the units that could own it" % who)
        elif not row["candidates"] and not (row.get("note") or row.get("why") or row.get("reason")):
            issues.append("%s: no candidate owners and no reason saying why (a linker-generated range)" % who)
        if not any(row.get(x) for x in ("reason", "why", "evidence", "note")):
            issues.append("%s: needs a reason (reason, why, evidence or note)" % who)
    for k, mv in enumerate(_lst(p.get("moves"))):
        if not (isinstance(mv, dict) and isinstance(mv.get("unit"), str) and isinstance(mv.get("after"), str)):
            issues.append("moves[%d] needs a unit and an after (strings)" % k)
            continue
        _check_fields(mv, MOVE_FIELDS, "moves[%d]" % k, issues, warns)


def lint_proposal_full(proposal, base=None):
    """`(errors, warnings)` of a proposal file.  With `base` (the baseline `Splits`) the fields that name baseline state are
    checked against it: `keep_registered_edge` must sit on a registered range start or end (error), `removes_cuts`, `absorbs` and
    `replaces_tail_of` must name one (warning)."""
    issues, warns = [], []
    proposal, aerr, awarn = canonical_proposal(proposal)
    issues += aerr
    warns += awarn
    if not isinstance(proposal, dict):
        return ["no units"], warns
    phase2 = any(proposal.get(k) for k in ("attach", "unowned_data", "moves"))
    if not isinstance(proposal.get("units", [] if phase2 else None), list) or not (proposal.get("units") or phase2):
        return ["no units"], warns
    proposal = dict(proposal, units=proposal.get("units") or [])
    _check_fields(proposal, TOP_FIELDS, "proposal", issues, warns)
    _lint_phase2(proposal, issues, warns)
    for q in _lst(proposal.get("open_questions")):
        if not isinstance(q, (str, dict)):
            issues.append("proposal: an open_questions entry is %s, want str|dict" % type(q).__name__)
    for s in _lst(proposal.get("supersedes")):
        if not (isinstance(s, dict) and all(isinstance(s.get(k), str) and s.get(k) for k in ("band", "unit", "reason"))):
            issues.append("proposal: a supersedes entry needs band, unit and reason (strings): %r" % (s,))
    edges = collections.defaultdict(set)
    ends = collections.defaultdict(set)      # registered range ends: the edge of a registered range against an unowned run
    names = set()
    if base is not None:
        names = {u.name for u in base.units}
        for u in base.units:
            for sec, rr in u.ranges.items():
                for a, b, _x in rr:
                    edges[sec].add(a)
                    ends[sec].add(b)
    seen = collections.defaultdict(list)
    for pu in proposal["units"]:
        if not isinstance(pu, dict):
            issues.append("a unit is %s, want an object" % type(pu).__name__)
            continue
        who = pu.get("derived_name", "?")
        _check_fields(pu, UNIT_FIELDS, who, issues, warns)
        for k in ("derived_name", "module", "ranges"):
            if not pu.get(k):
                issues.append("%s: missing %s" % (who, k))
        if not re.match(r"^[A-Za-z0-9_]+$", str(pu.get("derived_name", ""))):
            issues.append("%s: derived_name must be a plain identifier" % who)
        for q in _lst(pu.get("open_questions")):
            if not isinstance(q, (str, dict)):
                issues.append("%s: an open_questions entry is %s, want str|dict" % (who, type(q).__name__))
        for nm in _lst(pu.get("absorbs")):
            if not isinstance(nm, str):
                issues.append("%s: absorbs entry %r is not a unit name" % (who, nm))
            elif base is not None and nm.split()[0:1] != [] and nm.split()[0] not in names:
                warns.append("%s: absorbs %s, which is not a baseline unit" % (who, nm))
        rt = pu.get("replaces_tail_of")
        for nm in ([rt] if isinstance(rt, str) else rt if isinstance(rt, list) else []):
            if not isinstance(nm, str):
                issues.append("%s: replaces_tail_of entry %r is not a unit name" % (who, nm))
            elif base is not None and nm.split()[0:1] != [] and nm.split()[0] not in names:
                warns.append("%s: replaces_tail_of %s is not a baseline unit" % (who, nm))
        try:
            ranges = {sec: [(to_int(a), to_int(b)) for a, b in rr] for sec, rr in (pu.get("ranges") or {}).items()}
        except (TypeError, ValueError):
            issues.append("%s: a range is not [start, end]" % who)
            continue
        for sec, rr in ranges.items():
            if sec not in SECTION_ORDER:
                issues.append("%s: unknown section %s" % (who, sec))
            for a, b in rr:
                if b <= a:
                    issues.append("%s: %s range %s..%s is empty" % (who, sec, hx(a), hx(b)))
                seen[sec].append((a, b, who))
        starts = {(sec, a) for sec, rr in ranges.items() for a, _b in rr}
        cut_keys = set()
        for rc in _lst(pu.get("removes_cuts")):
            if not isinstance(rc, dict):
                issues.append("%s: a removes_cuts entry is %s, want an object" % (who, type(rc).__name__))
                continue
            _check_fields(rc, CUT_FIELDS, "%s removes_cuts" % who, issues, warns)
            try:
                ra, rsec = to_int(rc["addr"]), rc.get("section", ".text")
            except (KeyError, TypeError, ValueError):
                issues.append("%s: a removes_cuts entry has no numeric addr" % who)
                continue
            if base is not None and ra not in edges.get(rsec, ()) and ra not in ends.get(rsec, ()):
                warns.append("%s: removes_cuts %s %s is not a registered cut" % (who, rsec, hx(ra)))
        for c in _lst(pu.get("cuts")):
            if not isinstance(c, dict):
                issues.append("%s: a cut is %s, want an object" % (who, type(c).__name__))
                continue
            _check_fields(c, CUT_FIELDS, "%s cut %s" % (who, c.get("addr")), issues, warns)
            keep = c.get("keep_registered_edge") is True
            if keep:
                if "grade" in c and c["grade"] not in GRADES:
                    issues.append("%s: cut %s has grade %r (strong|medium|guess)" % (who, c.get("addr"), c.get("grade")))
            elif c.get("grade") not in GRADES:
                issues.append("%s: cut %s has grade %r (strong|medium|guess)" % (who, c.get("addr"), c.get("grade")))
            if not c.get("reproduce"):
                issues.append("%s: cut %s has no reproduce command" % (who, c.get("addr")))
            if not keep and c.get("grade") in ("strong", "medium") and not c.get("evidence"):
                issues.append("%s: %s cut %s has no evidence" % (who, c.get("grade"), c.get("addr")))
            for ev in _lst(c.get("evidence")):
                if not (isinstance(ev, dict) and all(ev.get(k) for k in ("tool", "command", "finding"))):
                    issues.append("%s: cut %s evidence needs tool, command and finding" % (who, c.get("addr")))
            try:
                key = (c.get("section"), to_int(c["addr"]))
                cut_keys.add(key)
                if keep and base is not None and key[1] not in edges.get(key[0], ()) and key[1] not in ends.get(key[0], ()):
                    issues.append("%s: cut %s %s has keep_registered_edge but no registered unit starts or ends there" % (who, key[0], hx(key[1])))
            except (KeyError, TypeError, ValueError):
                issues.append("%s: cut without a numeric addr" % who)
        for sec, a in sorted(starts):
            if sec == ".text" and (sec, a) not in cut_keys and pu is not proposal["units"][0]:
                issues.append("%s: .text starts at %s with no cut recording why" % (who, hx(a)))
        for sec, a in cut_keys:
            if (sec, a) not in starts:
                issues.append("%s: cut %s %s is not the start of one of the unit's ranges" % (who, sec, hx(a)))
    for sec, v in seen.items():
        v.sort()
        for (a, b, w), (c, d, x) in zip(v, v[1:]):
            if c < b:
                issues.append("%s and %s overlap in %s at %s" % (w, x, sec, hx(c)))
    return issues, warns


def lint_proposal(proposal, base=None):
    """Structural problems of a proposal file: a list of strings (empty = well-formed); warnings are `lint_proposal_full`'s."""
    return lint_proposal_full(proposal, base)[0]


def coalesce(rr):
    out = []
    for a, b in sorted(rr):
        if out and a <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], b))
        else:
            out.append((a, b))
    return out


def merge_guess(units, base=None, info=None):
    """Fold every unit whose left edge carries a `guess` cut into the unit that TOUCHES that edge - a proposal unit with a
    range ending at the cut, else a baseline (registered) unit that does; the merged cut is recorded.  A guess cut with no
    touching unit is a lint error (the unit stays) - a `keep_registered_edge` cut is never merged.

    `info` (optional) receives `issues` and `base_merges` (`{"unit", "into", "cut"}`: the unit's ranges move to the baseline unit)."""
    info = info if info is not None else {}
    info.setdefault("issues", [])
    info.setdefault("base_merges", [])
    out = []
    bm_end = {}                      # (section, end) of a unit folded into a registered unit -> that registered unit's name
    for u in units:
        guess = [c for c in u["cuts"] if c.get("grade") == "guess" and not c.get("keep_registered_edge")]
        if not guess:
            out.append(u)
            continue
        c0 = guess[0]
        sec, addr = c0.get("section", ".text"), c0["addr"]
        at = next((i for i in range(len(out) - 1, -1, -1) if any(e == addr for _s, e in out[i]["ranges"].get(sec, []))), None)
        rec = [{"candidate_cut": hx(c["addr"]), "section": c.get("section"), "grade": "guess"} for c in guess]
        if at is not None:
            a = out[at]
            big, small = (a, u) if sum(e - s for s, e in a["ranges"].get(".text", [])) >= sum(e - s for s, e in u["ranges"].get(".text", [])) else (u, a)
            merged = {"name": big["name"], "derived_name": big["derived_name"], "module": big["module"],
                      "ranges": {}, "cuts": [c for c in a["cuts"] + u["cuts"] if c.get("grade") != "guess"],
                      "open_questions": a["open_questions"] + u["open_questions"],
                      "removes_cuts": a.get("removes_cuts", []) + u.get("removes_cuts", []),
                      "absorbs": a.get("absorbs", []) + u.get("absorbs", []), "replaces_tail_of": big.get("replaces_tail_of"),
                      "after": big.get("after"), "merged": a["merged"] + u["merged"] + [dict(r, absorbed=small["name"]) for r in rec]}
            for s2 in set(a["ranges"]) | set(u["ranges"]):
                merged["ranges"][s2] = coalesce(a["ranges"].get(s2, []) + u["ranges"].get(s2, []))
            out[at] = merged
            continue
        bn = next((bu for bu in (base.units if base is not None else []) if any(e == addr for _s, e, _x in bu.ranges.get(sec, []))), None)
        into = bn.name if bn is not None else bm_end.get((sec, addr))
        if into is not None:
            # a unit folded into a registered unit extends it: the next guess cut touching ITS end folds there too
            for s2, rr2 in u["ranges"].items():
                for _a2, e2 in rr2:
                    bm_end[(s2, e2)] = into
            info["base_merges"].append({"unit": u, "into": into, "cut": c0, "merged": [dict(r, absorbed=u["name"], into=into) for r in rec]})
            continue
        info["issues"].append("%s: guess cut %s %s has no adjacent unit to merge into (no proposal or registered unit ends there)"
                              % (u["derived_name"], sec, hx(addr)))
        out.append(u)
    return out


def subtract(rr, cuts):
    """The parts of the ranges `rr` that lie outside every `(a, b)` of `cuts`."""
    cuts = coalesce(cuts)
    out = []
    for s, e in rr:
        cur = s
        for a, b in cuts:
            if b <= cur or a >= e:
                continue
            if a > cur:
                out.append((cur, a))
            cur = max(cur, b)
        if cur < e:
            out.append((cur, e))
    return out


def derive_attached(units, base, dol, symbols):
    """Fill the sections a text cut determines by itself: `extabindex`/`extab` (by the function of every entry) and
    `.ctors`/`.dtors` (by the function each word points at).  A unit that lists a section keeps its own range."""
    tv = sorted((a, b, i) for i, u in enumerate(units) for a, b in u["ranges"].get(".text", []))
    starts = [t[0] for t in tv]

    def unit_of_text(addr):
        k = bisect.bisect_right(starts, addr) - 1
        return tv[k][2] if k >= 0 and tv[k][0] <= addr < tv[k][1] else None

    def extent(sec):
        rr = [(s0, e0) for bu in base.units for s0, e0, _a in bu.ranges.get(sec, [])]
        return (min(r[0] for r in rr), max(r[1] for r in rr)) if rr else None

    etb = {x["addr"]: x["size"] for x in symbols if x["section"] == "extab"}
    got = collections.defaultdict(lambda: collections.defaultdict(list))
    ext = extent("extabindex")
    if ext:
        for a in range(ext[0], ext[1] - 11, 12):
            fn, ex = dol.word(a), dol.word(a + 8)
            i = unit_of_text(fn) if fn else None
            if i is not None:
                got[i]["extabindex"].append((a, a + 12))
                got[i]["extab"].append((ex, ex + etb.get(ex, 8)))
    for sec in (".ctors", ".dtors"):
        ext = extent(sec)
        if ext:
            for a in range(ext[0], ext[1] - 3, 4):
                w = dol.word(a)
                i = unit_of_text(w) if w else None
                if i is not None:
                    got[i][sec].append((a, a + 4))
    derived = []
    for i, secs in got.items():
        for sec, rr in secs.items():
            if sec not in units[i]["ranges"]:
                units[i]["ranges"][sec] = coalesce(rr)
                derived.append("%s %s" % (units[i]["name"], sec))
    return derived


DATA_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")


def assign_data_by_reader(movers, base, dol, symbols):
    """PROVISIONAL phase-1 default for the DATA of a recut registered unit (phase 2 replaces it with evidence).

    A baseline unit whose text is split among `movers` (proposal units, plus its own remnant) keeps its data run in one
    piece, which leaves data-only remnants.  Per data run, per section, the symbols are assigned to the pieces in text order
    by a monotone DP: a symbol costs one per piece that reads it other than the piece it is put in (readers decoded from
    the retail text, pool literals by loads only); ties keep a symbol with the earlier piece.  A section one of the
    movers already lists over that run is left as written.  Each mover's `ranges` gets its share; the remnant keeps the rest.
    Returns one record per (unit, section) assigned."""
    ctx = Ctx(base, symbols, dol)
    text = sorted((a, b, i) for i, u in enumerate(movers) for a, b in u["ranges"].get(".text", []))
    tstarts = [t[0] for t in text]
    out = []

    def mover_at(site):
        k = bisect.bisect_right(tstarts, site) - 1
        return text[k][2] if k >= 0 and text[k][0] <= site < text[k][1] else None

    for bu in base.units:
        trs = [(a, b) for a, b, _x in bu.ranges.get(".text", [])]
        if not trs:
            continue
        lo, hi = trs[0][0], trs[-1][1]
        touching = sorted({i for a, b, i in text if a < hi and b > lo}, key=lambda i: min(a for a, _b, j in text if j == i and a < hi and _b > lo))
        if not touching:
            continue
        claimed_text = coalesce([(max(a, lo), min(b, hi)) for a, b, i in text if a < hi and b > lo])
        remnant = subtract(trs, claimed_text)
        pieces = [("m", i, max(lo, min(a for a, _b, j in text if j == i and a < hi and _b > lo))) for i in touching]
        if remnant:
            pieces.append(("r", None, remnant[0][0]))
        pieces.sort(key=lambda t: t[2])
        pidx = {(kind, i): k for k, (kind, i, _st) in enumerate(pieces)}

        def piece_of(site):
            if not (lo <= site < hi):
                return None
            i = mover_at(site)
            return pidx.get(("m", i)) if i is not None else pidx.get(("r", None))

        for sec in DATA_SECTIONS:
            for s0, e0, attr in bu.ranges.get(sec, []):
                if "rename:" in attr or "common" in attr:
                    continue
                if any(s1 < e0 and e1 > s0 for u in movers for s1, e1 in u["ranges"].get(sec, [])):
                    continue
                syms = [x for x in ctx.data_syms if x["section"] == sec and s0 <= x["addr"] < e0]
                if not syms:
                    continue
                K = len(pieces)
                costs = []
                for sym in syms:
                    rd = ctx.literal_readers(sym) if is_literal(sym) else ctx.readers(sym)
                    who = {piece_of(x) for x in rd} - {None}
                    costs.append([len(who) - (1 if k in who else 0) for k in range(K)])
                INF = (10 ** 9, 0)
                dp = [[INF] * K for _ in syms]
                back = [[0] * K for _ in syms]
                for i in range(len(syms)):
                    for k in range(K):
                        if i == 0:
                            dp[0][k] = (costs[0][k], k)
                            continue
                        best, bk = INF, 0
                        for k2 in range(k + 1):
                            if dp[i - 1][k2] < best:
                                best, bk = dp[i - 1][k2], k2
                        dp[i][k] = (best[0] + costs[i][k], best[1] + k)
                        back[i][k] = bk
                k = min(range(K), key=lambda kk: dp[-1][kk])
                total = dp[-1][k][0]
                assign = [0] * len(syms)
                for i in range(len(syms) - 1, -1, -1):
                    assign[i] = k
                    k = back[i][k]
                runs = []
                for i, pk in enumerate(assign):
                    if runs and runs[-1][0] == pk:
                        runs[-1][2] = i + 1
                    else:
                        runs.append([pk, i, i + 1])
                shares = {}
                for n, (pk, i0, i1) in enumerate(runs):
                    a = s0 if n == 0 else syms[i0]["addr"]
                    b = e0 if n == len(runs) - 1 else syms[runs[n + 1][1]]["addr"]
                    shares[pk] = (a, b)
                rec = {"unit": bu.name, "section": sec, "misplaced": total, "pieces": {}}
                for pk, (a, b) in shares.items():
                    kind, i, _st = pieces[pk]
                    if kind == "m":
                        movers[i]["ranges"][sec] = coalesce(movers[i]["ranges"].get(sec, []) + [(a, b)])
                        rec["pieces"][movers[i]["name"]] = [hx(a), hx(b)]
                    else:
                        rec["pieces"][bu.name + " (remnant)"] = [hx(a), hx(b)]
                out.append(rec)
    return out


def _extend(nu, sec, a, b):
    """Add `[a, b)` to a rendered unit's `sec` ranges, growing an abutting plain fragment instead of adding a second range."""
    frags = nu.ranges.setdefault(sec, [])
    for k, (s0, e0, at) in enumerate(frags):
        if "rename:" in at or "common" in at:
            continue
        if e0 == a:
            frags[k] = (s0, b, at)
            return
        if s0 == b:
            frags[k] = (a, e0, at)
            return
    frags.append((a, b, ""))
    frags.sort()


def _cut_range(unit, sec, a, b):
    """Remove `[a, b)` from `unit`'s `sec` fragments (a fragment is split or trimmed; its attributes are kept)."""
    out = []
    for s, e, at in unit.ranges.get(sec, []):
        if e <= a or s >= b:
            out.append((s, e, at))
            continue
        if s < a:
            out.append((s, a, at))
        if e > b:
            out.append((b, e, at))
    if out:
        unit.ranges[sec] = out
    else:
        unit.ranges.pop(sec, None)


def name_resolver(final, pre_merge, units, base_merges):
    """`resolve(name)`: the name of the candidate unit a phase 2 row means by `name` - the name itself when the candidate has it, else a
    proposal unit's `derived_name`, the unit a `guess` cut folded it into, or the proposal unit that `absorbs` it."""
    alias = {}
    for u in pre_merge:
        alias.setdefault(u["derived_name"], u["name"])
    for u in units:
        for r in u["merged"]:
            if r.get("absorbed") and r["absorbed"] != u["name"]:
                alias[r["absorbed"]] = u["name"]
    for bm in base_merges:
        alias[bm["unit"]["name"]] = bm["into"]
    for u in units:
        for nm in _lst(u.get("absorbs")):
            if isinstance(nm, str) and nm.split() and nm.split()[0] != u["name"]:
                alias.setdefault(nm.split()[0], u["name"])           # a trailing comment after the name is fine

    def resolve(name):
        present = {x.name for x in final}
        seen = set()
        while name not in present and name in alias and name not in seen:
            seen.add(name)
            name = alias[name]
        return name

    return resolve


def apply_attach(final, proposals, info, resolve):
    """Phase 2: give every non-`guess` `attach` range to its unit.  The rule, one implementation of what the six lanes each had:

    * the target is the unit `unit` names (`resolve`: a name, a `derived_name`, a name a guess fold or an `absorbs` replaced) or the unit whose
      `.text`/`.init` holds `text_addr` (a mismatch with `unit` is a warning); a unit the candidate lacks is a lint line;
    * a range may take unowned bytes, bytes the target holds already (a restatement), bytes of a PROVISIONAL by-reader piece (the evidence
      replaces the default) and bytes of the unit its row names in `takes_from` (a fold: that unit, left with no range, leaves the
      candidate); a byte any other unit keeps - a registered unit nothing recut, a proposal unit's own listed range - is a lint line and
      the range is not applied;
    * two applied rows never share a byte (the second is a lint line); `guess` rows are counted, never applied;
    * `unowned_data` rows are checked against the result: a deferred range a unit owns is a lint line unless the row says `provisional`."""
    by_name = {u.name: u for u in final}
    prov = collections.defaultdict(list)
    for r in info.get("data_by_reader", []):
        for label, v in r["pieces"].items():
            nm = label[:-len(" (remnant)")] if label.endswith(" (remnant)") else label
            prov[(nm, r["section"])].append((to_int(v[0]), to_int(v[1])))
    entries = []
    for p in proposals:
        for e in attach_entries(p):
            if e["grade"] == "guess":
                info["attach_skipped"].append({"unit": e["unit"], "section": e["section"], "ranges": e["ranges"], "grade": "guess"})
            else:
                entries.append(e)
        info["unowned_data"] += [x for x in _lst(p.get("unowned_data")) if isinstance(x, dict)]
    entries.sort(key=lambda e: (SECTION_ORDER.index(e["section"]) if e["section"] in SECTION_ORDER else 99, min(a for a, _b in e["ranges"]),
                                e["unit"] or ""))
    seen = collections.defaultdict(list)
    touched, emptied = set(), set()
    text_starts = sorted((a, b, u) for u in final for sec in CODE_SECTIONS for a, b, _x in u.ranges.get(sec, []))
    for e in entries:
        sec, who = e["section"], e["unit"] or hx(e["text_addr"] or 0)
        tgt = None
        if e["text_addr"] is not None:
            tgt = next((u for a, b, u in text_starts if a <= e["text_addr"] < b), None)
            if tgt is None:
                info["issues"].append("attach %s %s: text_addr %s is in no unit of the candidate" % (who, sec, hx(e["text_addr"])))
                continue
            if e["unit"] and resolve(e["unit"]) != tgt.name:
                info["warnings"].append("attach %s %s: text_addr %s is in %s" % (e["unit"], sec, hx(e["text_addr"]), tgt.name))
        else:
            tgt = by_name.get(resolve(e["unit"] or ""))
            if tgt is None or tgt not in final:
                info["issues"].append("attach: unit %s is not in the candidate (%s %s..%s)" % (who, sec, hx(e["ranges"][0][0]), hx(e["ranges"][0][1])))
                continue
        donor = resolve(e["takes_from"]) if e["takes_from"] else None
        for a, b in e["ranges"]:
            lab = "attach %s %s %s..%s" % (tgt.name, sec, hx(a), hx(b))
            clash = [x for x in seen[sec] if x[0] < b and x[1] > a]
            if clash:
                info["issues"].append("%s overlaps another attach row (%s)" % (lab, clash[0][2]))
                continue
            bad = None
            for u in final:
                for s, en, _at in u.ranges.get(sec, []):
                    lo, hi = max(s, a), min(en, b)
                    if lo >= hi or u is tgt or u.name == donor:
                        continue
                    if not any(ps <= lo and hi <= pe for ps, pe in prov.get((u.name, sec), [])):
                        bad = (u.name, lo, hi)
            if bad:
                info["issues"].append("%s takes %s..%s from %s, whose range no cut recut (only an unowned run, a provisional by-reader range or a "
                                      "`takes_from` unit may be taken)" % (lab, hx(bad[1]), hx(bad[2]), bad[0]))
                continue
            seen[sec].append((a, b, lab))
            for u in final:
                if u is not tgt and any(s < b and en > a for s, en, _x in u.ranges.get(sec, [])):
                    _cut_range(u, sec, a, b)
                    emptied.add(u.name)
            had = sum(max(0, min(en, b) - max(s, a)) for s, en, _x in tgt.ranges.get(sec, []))
            if had < b - a:
                _cut_range(tgt, sec, a, b)
                _extend(tgt, sec, a, b)
            touched.add((tgt.name, sec))
            info["attach"].append({"unit": tgt.name, "section": sec, "start": a, "end": b, "grade": e["grade"]})
            if tgt.name not in info["attach_units"]:
                info["attach_units"].append(tgt.name)
    for u in final:                                          # abutting plain fragments of a unit that took a range are one range
        for sec in list(u.ranges):
            if (u.name, sec) not in touched:
                continue
            merged = []
            for s, e, at in sorted(u.ranges[sec]):
                if merged and merged[-1][1] == s and not at and not merged[-1][2]:
                    merged[-1] = (merged[-1][0], e, at)
                else:
                    merged.append((s, e, at))
            u.ranges[sec] = merged
    for u in list(final):                                    # a unit whose every range was taken is gone
        if u.name in emptied and not u.ranges:
            final.remove(u)
            info["dropped"].append(u.name)
    info["attach_units"] = [n for n in info["attach_units"] if any(u.name == n for u in final)]
    for p in proposals:
        for ud in unowned_entries(p):
            if ud["provisional"]:
                continue
            for u in final:
                for s, e, _at in u.ranges.get(ud["section"], []):
                    lo, hi = max(s, ud["a"]), min(e, ud["b"])
                    if lo < hi and not any(ps <= lo and hi <= pe for ps, pe in prov.get((u.name, ud["section"]), [])):
                        info["issues"].append("unowned_data %s %s..%s is listed as deferred but %s owns %s..%s"
                                              % (ud["section"], hx(ud["a"]), hx(ud["b"]), u.name, hx(max(s, ud["a"])), hx(min(e, ud["b"]))))
                        break


def render(base, proposals, dol=None, symbols=None, data_by_reader=True):
    """`(candidate Splits, info)`: the baseline with every proposal's units cut in (guess cuts merged).

    With `dol` and `symbols` the sections a text cut determines (extab, extabindex, ctors, dtors) are derived and, unless
    `data_by_reader` is False, the data of a recut registered unit is assigned by reader (`assign_data_by_reader`).
    """
    info = {"issues": [], "warnings": [], "merged": [], "units": [], "derived": [], "data_by_reader": [], "cuts": {}, "superseded": [],
            "attach": [], "attach_skipped": [], "attach_units": [], "unowned_data": [], "dropped": [], "moved": []}
    units = []
    drop = {}                       # (band, derived_name) -> the superseding entry; filled from every proposal's `supersedes`
    raw = proposals
    proposals = [canonical_proposal(p)[0] for p in proposals]
    for p in proposals:
        for s in _lst(p.get("supersedes")):
            if isinstance(s, dict) and s.get("band") and s.get("unit"):
                drop[(s["band"], s["unit"])] = s
    for p in raw:
        errs, warns = lint_proposal_full(p, base)
        info["issues"] += errs
        info["warnings"] += warns
        kept_units = [pu for pu in p.get("units", []) if not (isinstance(pu, dict) and (p.get("band"), pu.get("derived_name")) in drop)]
        for pu in p.get("units", []):
            if isinstance(pu, dict) and (p.get("band"), pu.get("derived_name")) in drop:
                info["superseded"].append("band %s unit %s" % (p.get("band"), pu.get("derived_name")))
                drop[(p.get("band"), pu.get("derived_name"))]["_used"] = True
        units += norm_units(dict(p, units=kept_units))
    for (band, name), s in drop.items():
        if not s.get("_used"):
            info["warnings"].append("supersedes: band %s has no unit %s" % (band, name))
    cc = collections.Counter()
    for u in units:
        for c in u["cuts"]:
            cc["keep_registered_edge" if c.get("keep_registered_edge") else c.get("grade", "?")] += 1
    info["cuts"] = dict(cc)
    info["proposal_cuts"] = [(c["addr"], "guess" if c.get("keep_registered_edge") is not True and c.get("grade") == "guess" else c.get("grade", "?"), u["name"])
                             for u in units for c in u["cuts"] if c.get("keep_registered_edge") is not True and c.get("section", ".text") == ".text"]
    units.sort(key=lambda u: (u["ranges"].get(".text") or [(0, 0)])[0][0])
    mg = {"issues": info["issues"], "base_merges": []}
    pre_merge = list(units)
    units = merge_guess(units, base, mg)
    base_merges = mg["base_merges"]
    movers = units + [bm["unit"] for bm in base_merges]
    if dol is not None and symbols is not None:
        info["derived"] = derive_attached(movers, base, dol, symbols)
        if data_by_reader:
            info["data_by_reader"] = assign_data_by_reader(movers, base, dol, symbols)
    for u in units:
        info["merged"] += u["merged"]
        info["units"].append(u["name"])
    for bm in base_merges:
        info["merged"] += bm["merged"]
    claimed = collections.defaultdict(list)
    for u in movers:
        for sec, rr in u["ranges"].items():
            claimed[sec] += rr
    into = collections.defaultdict(list)
    for bm in base_merges:
        into[bm["into"]].append(bm["unit"])
    kept = []                       # [(anchor, Unit)]: a unit's anchor is its original text start (data-only: its predecessor's)
    anchor = 0
    for bu in base.units:
        if bu.first(".text") is not None:
            anchor = bu.first(".text")
        nu = Unit(bu.name, bu.attrs)
        for sec, rr in bu.ranges.items():
            frags = []
            for s, e, a in rr:
                for fs, fe in subtract([(s, e)], claimed.get(sec, [])):
                    frags.append((fs, fe, a))
            touched = [(fs, fe) for fs, fe, _a in frags] != [(x, y) for x, y, _z in rr]
            plain = sorted((fs, fe) for fs, fe, a in frags if "rename:" not in a and "common" not in a)
            holey = any(b != c for (_x, b), (c, _y) in zip(plain, plain[1:]))
            if touched and len(frags) > 1 and holey:
                info["issues"].append("baseline unit %s is left with %d %s fragments (%s): the proposal punches a hole"
                                      % (bu.name, len(frags), sec, ", ".join("%s..%s" % (hx(a), hx(b)) for a, b, _x in frags)))
            if frags:
                nu.ranges[sec] = frags
        for mu in into.get(bu.name, []):
            for sec, rr in mu["ranges"].items():
                for a, b in rr:
                    _extend(nu, sec, a, b)
        if nu.ranges:
            kept.append((anchor, nu))
    late = []                       # data-only proposal units with an `after` anchor: placed once every unit is in
    for u in units:
        nu = Unit(u["name"], "", {sec: [(a, b, "") for a, b in rr] for sec, rr in u["ranges"].items()})
        at = nu.first(".text")
        if at is None:
            if not u.get("after"):
                info["warnings"].append("%s: a unit without .text has no `after` anchor: it is placed at the end of the candidate" % u["name"])
            late.append((u, nu))
            continue
        pos = len(kept)
        for i, (anc, _k) in enumerate(kept):
            if anc >= at:
                pos = i
                break
        kept.insert(pos, (at, nu))
    names = collections.Counter(k.name for _a, k in kept)
    for n, c in names.items():
        if c > 1:
            info["issues"].append("unit name %s is used %d times in the candidate (a proposal unit reuses a baseline unit's name)" % (n, c))
    final = [k for _a, k in kept]
    resolve = name_resolver(final, pre_merge, units, base_merges)
    placed = collections.Counter()
    for u, nu in late:              # units after one anchor keep the file order; one without an anchor goes last
        if not u.get("after"):
            final.append(nu)
            continue
        anchor_name = resolve(u["after"])
        k = next((i for i, x in enumerate(final) if x.name == anchor_name), None)
        if k is None:
            info["issues"].append("%s: after %s, which is not a unit of the candidate" % (u["name"], u["after"]))
            final.append(nu)
            continue
        final.insert(k + 1 + placed[anchor_name], nu)
        placed[anchor_name] += 1
    for p in proposals:
        for mv in _lst(p.get("moves")):
            if not (isinstance(mv, dict) and isinstance(mv.get("unit"), str) and isinstance(mv.get("after"), str)):
                continue
            who, anc = resolve(mv["unit"]), resolve(mv["after"])
            mu = next((x for x in final if x.name == who), None)
            if mu is None or not any(x.name == anc for x in final):
                info["issues"].append("moves: %s after %s names a unit the candidate does not have" % (mv["unit"], mv["after"]))
                continue
            final.remove(mu)
            final.insert(next(i for i, x in enumerate(final) if x.name == anc) + 1, mu)
            info["moved"].append("%s after %s" % (mu.name, anc))
    apply_attach(final, proposals, info, resolve)
    return Splits(list(base.header), final), info


# ---- output -----------------------------------------------------------------------------------------------------------------

def report_json(ctx, res, bounds, extra=None):
    return {"units": res.units, "coverage_gaps": getattr(ctx, "coverage_gaps", {}), "summary": res.summary(), "boundaries": bounds,
            "top_defects": top_defects(ctx, res, 10), **(extra or {})}


def print_table(ctx, res, show_all=False, limit=40):
    summ = res.summary()
    print("%-11s %6s %6s %8s %6s" % ("invariant", "PASS", "FAIL", "UNKNOWN", "-"))
    for inv in INVARIANTS:
        s = summ[inv]
        print("%-11s %6d %6d %8d %6d" % (inv, s[PASS], s[FAIL], s[UNKNOWN], s[NA] + len(ctx.units) - sum(s.values())))
    rows = []
    for u in ctx.units:
        recs = res.units.get(u.name, {})
        worst = max((RANK[r["status"]] for r in recs.values()), default=0)
        if show_all or worst >= RANK[FAIL]:
            rows.append((u.name, recs))
    print("\nunits with a FAIL: %d of %d%s" % (sum(1 for n, r in rows if any(x["status"] == FAIL for x in r.values())), len(ctx.units),
                                             "" if show_all else " (--all lists every unit)"))
    print("%-44s %s" % ("unit", " ".join("%-4s" % i[:4] for i in INVARIANTS)))
    for name, recs in rows[:limit if not show_all else None]:
        cells = []
        for inv in INVARIANTS:
            r = recs.get(inv)
            cells.append("%-4s" % ({PASS: "ok", FAIL: "FAIL", UNKNOWN: "?", NA: "-"}[r["status"]] if r else "-"))
        print("%-44s %s" % (name[:44], " ".join(cells)))
    if len(rows) > limit and not show_all:
        print("... %d more" % (len(rows) - limit))


def print_defects(items):
    print("\ntop defects")
    for i, d in enumerate(items, 1):
        print("%2d. [%s] %s @ %s  %s" % (i, d["invariant"], d["unit"], hx(d["addr"]), d["finding"][:150]))


# ---- entry points -------------------------------------------------------------------------------------------------------------

def load_ctx(splits_path=None, symbols_path=None, dol_path=None, scan=True):
    root = tree_root()
    splits_path = splits_path or os.path.join(root, "config", GAME, "splits.txt")
    symbols_path = symbols_path or os.path.join(root, "config", GAME, "symbols.txt")
    dol_path = dol_path or find_file(os.path.join("orig", GAME, "sys", "main.dol"), root)
    with open(splits_path, encoding="utf-8", errors="replace") as fh:
        splits = parse_splits(fh.read())
    with open(symbols_path, encoding="utf-8", errors="replace") as fh:
        symbols = parse_symbols(fh)
    with open(dol_path, "rb") as fh:
        dol = Dol(fh.read())
    return splits, symbols, dol


def dataorder_rows(symbols):
    return [(s["section"], s["addr"], s["size"] or None, s["name"]) for s in symbols]


def analyse(splits, symbols, dol, only=None, outbox=None, sda=(None, None)):
    ctx = Ctx(splits, symbols, dol, sda[0], sda[1])
    res = run_checks(ctx, dataorder_rows(symbols), only)
    bounds = boundaries(ctx, res)
    return ctx, res, bounds


def cmd_baseline(args):
    splits, symbols, dol = load_ctx(args.splits, args.symbols, args.dol)
    ctx, res, bounds = analyse(splits, symbols, dol, args.only.split(",") if args.only else None)
    if args.unit:
        keep = re.compile(args.unit)
        ctx.units = [u for u in ctx.units if keep.search(u.name)]
        res.units = collections.OrderedDict((k, v) for k, v in res.units.items() if keep.search(k))
        bounds = [b for b in bounds if keep.search(b["left"]) or keep.search(b["right"])]
    outbox = args.outbox or os.path.join(main_root() or tree_root(), ".pi", "outbox")
    seams = seam_requests(ctx, outbox)
    only_set = set(args.only.split(",")) if args.only else None
    if args.unit and (only_set is None or only_set & {"ctors", "dtors"}):
        for u in ctx.units:
            for sec, inv in ((".ctors", "ctors"), (".dtors", "dtors")):
                if u.ranges.get(sec) and (only_set is None or inv in only_set):
                    for line in ctors_detail(ctx, u, sec):
                        print("detail %s: %s" % (u.name, line))
    if args.intervals:
        keep = re.compile(args.unit).search if args.unit else None
        for line in pool_intervals(ctx, keep):
            print(line)
    groups = pool_groups(ctx)
    print("baseline: %d units, %d map symbols, r13=%s r2=%s" % (len(ctx.units), len(symbols), hx(ctx.sda13), hx(ctx.sda2)))
    print_table(ctx, res, args.all, args.limit)
    print_defects(top_defects(ctx, res, 10))
    print("\none worst defect per failing invariant")
    allf = top_defects(ctx, res, 100000)
    for inv in INVARIANTS:
        d = next((x for x in allf if x["invariant"] == inv), None)
        if d:
            print("  [%s] %d units fail; e.g. %s @ %s  %s" % (inv, sum(1 for x in allf if x["invariant"] == inv), d["unit"], hx(d["addr"]), d["finding"][:120]))
    bad_b = [b for b in bounds if FAIL in b["checks"].values()]
    print("\nboundaries: %d text cuts, %d with a failing check (fn-start %d, pool-shared %d)"
          % (len(bounds), len(bad_b), sum(1 for b in bounds if b["checks"]["fn-start"] == FAIL),
             sum(1 for b in bounds if b["checks"]["pool-shared"] == FAIL)))
    print("suspected: %d seam requests in the outbox (%d with an owning unit), %d pool groups (largest %s)"
          % (len(seams), sum(1 for s in seams if s["unit"]), len(groups), len(groups[0]["units"]) if groups else 0))
    gaps = getattr(ctx, "coverage_gaps", {})
    print("unowned (no unit range): " + ", ".join("%s %d syms/%d runs" % (k, v["symbols"], v["runs"]) for k, v in sorted(gaps.items())))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(report_json(ctx, res, bounds, {"seam_requests": seams, "pool_groups": groups,
                                                     "sda": {"r13": ctx.sda13, "r2": ctx.sda2}}), fh, indent=1)
    return 0


def parse_readers_spec(spec):
    """`SECTION:START-END` (hex addresses) -> `(section, start, end)`."""
    m = re.match(r"^(\.?\w+):(0x[0-9A-Fa-f]+)-(0x[0-9A-Fa-f]+)$", spec or "")
    if not m:
        raise ValueError("--readers wants SECTION:0xSTART-0xEND, got %r" % (spec,))
    return m.group(1), int(m.group(2), 16), int(m.group(3), 16)


def readers_report(ctx, spec):
    """One line per map symbol of `spec`'s section range: the owner unit of the candidate, then the units whose decoded text reads it
    (`literal_readers` for a pool literal) with the site counts.  The evidence an `attach` row quotes."""
    sec, lo, hi = parse_readers_spec(spec)
    lines = []
    for sym in ctx.data_syms:
        if sym["section"] != sec or not (lo <= sym["addr"] < hi):
            continue
        sites = ctx.literal_readers(sym) if is_literal(sym) else ctx.readers(sym)
        by = collections.Counter((ctx.text_owner(x).name if ctx.text_owner(x) else "?") for x in sites)
        own = ctx.owner(sec, sym["addr"])
        lines.append("%s %s %-26s size 0x%X owner %-34s readers %s" % (
            sec, hx(sym["addr"]), sym["name"], sym["size"] or 0, own.name if own else "(unowned)",
            ", ".join("%s x%d" % (u, n) for u, n in sorted(by.items())) or "none"))
    return lines


def cmd_proposal(args):
    splits, symbols, dol = load_ctx(args.splits, args.symbols, args.dol)
    proposals = [load_proposal(p) for p in args.proposal]
    cand, info = render(splits, proposals, dol, symbols, getattr(args, "data_by_reader", True))
    for i in info["issues"]:
        print("lint: " + i)
    for i in info["warnings"]:
        print("warn: " + i)
    for s in info["superseded"]:
        print("superseded (dropped before rendering): " + s)
    if info["derived"]:
        print("derived %d attached ranges (extab/extabindex/ctors/dtors) from the text cuts" % len(info["derived"]))
    for m in info["merged"]:
        print("merged guess cut %s (%s): %s absorbed%s" % (m["candidate_cut"], m["section"], m["absorbed"],
                                                          (" into registered %s" % m["into"]) if m.get("into") else ""))
    if info["data_by_reader"]:
        print("data-by-reader (PROVISIONAL phase-1 default, phase 2 replaces it with evidence): %d data runs of recut units assigned"
              % len(info["data_by_reader"]))
        for r in info["data_by_reader"]:
            print("  %-34s %-8s %s%s" % (r["unit"], r["section"], ", ".join("%s %s..%s" % (k, v[0], v[1]) for k, v in r["pieces"].items()),
                                         ("  (%d read from another piece)" % r["misplaced"]) if r["misplaced"] else ""))
    if info["cuts"]:
        print("cuts: " + ", ".join("%s %d" % (k, info["cuts"][k]) for k in ("strong", "medium", "guess", "keep_registered_edge") if k in info["cuts"])
              + "  (keep_registered_edge is not counted as proven; guess is merged, never emitted)")
    if info["attach"] or info["attach_skipped"] or info["unowned_data"]:
        gc = collections.Counter(r["grade"] for r in info["attach"])
        by_sec = collections.Counter()
        for r in info["attach"]:
            by_sec[r["section"]] += r["end"] - r["start"]
        print("attach (phase 2): %d ranges applied (strong %d, medium %d; %s), %d guess ranges not applied; unowned_data: %d rows; %d units dropped, %d moved"
              % (len(info["attach"]), gc["strong"], gc["medium"], ", ".join("%s 0x%X B" % kv for kv in sorted(by_sec.items())) or "-",
                 len(info["attach_skipped"]), len(info["unowned_data"]), len(info["dropped"]), len(info["moved"])))
    for x in info["dropped"]:
        print("dropped (every range taken by an attach): " + x)
    for x in info["moved"]:
        print("moved: " + x)
    if args.emit_splits:
        with open(args.emit_splits, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(render_splits(cand))
    only = args.only.split(",") if args.only else None
    bctx, bres, _bb = analyse(splits, symbols, dol, only, sda=(None, None))
    crossed = local_static_crossings(bctx, info.get("proposal_cuts", []))
    print("\ncuts crossed by a local static: %d" % len(crossed))
    for addr, grade, unit, name, below, above in crossed:
        print("  [%s] cut %s (%s): local %s is read at %s below it and at %s above it" % (grade, hx(addr), unit, name, hx(below), hx(above)))
    cctx, cres, cb = analyse(cand, symbols, dol, only, sda=(bctx.sda13, bctx.sda2))
    bs, cs = bres.summary(), cres.summary()
    if args.unit:
        keep = re.compile(args.unit).search
        for u in cctx.units:
            for sec in (".ctors", ".dtors"):
                if keep(u.name) and u.ranges.get(sec):
                    for line in ctors_detail(cctx, u, sec):
                        print("detail %s: %s" % (u.name, line))
        if args.intervals:
            for line in pool_intervals(cctx, keep):
                print(line)
    print("\ncandidate: %d units (baseline %d); proposal units: %s" % (len(cctx.units), len(bctx.units), ", ".join(info["units"][:12])))
    print("%-11s %10s %10s" % ("invariant", "FAIL base", "FAIL cand"))
    for inv in INVARIANTS:
        print("%-11s %10d %10d" % (inv, bs[inv][FAIL], cs[inv][FAIL]))
    if getattr(args, "readers", None):
        for spec in args.readers:
            for line in readers_report(cctx, spec):
                print(line)
    attached = [n for n in info["attach_units"] if n not in info["units"] and n in cres.units]
    mine = set(info["units"]) | set(attached)         # a unit an `attach` gives data to is the proposal's: its FAILs are listed, not "new"
    print("\nproposal units (and units an `attach` gives data to):")
    for n in info["units"] + attached:
        recs = cres.units.get(n, {})
        bad = {k: r for k, r in recs.items() if r["status"] == FAIL}
        unk = [k for k, r in recs.items() if r["status"] == UNKNOWN]
        print("  %-40s %s%s" % (n, "all checked PASS" if not bad else "FAIL " + ", ".join("%s@%s" % (k, hx(r["addr"])) for k, r in bad.items()),
                                ("  unknown: " + ", ".join(unk)) if unk else ""))
    new = []
    for name, recs in cres.units.items():
        for inv, r in recs.items():
            if r["status"] == FAIL and name not in mine and bres.units.get(name, {}).get(inv, {}).get("status") != FAIL:
                new.append((name, inv, r))
    print("\nnew failures outside the proposal units: %d" % len(new))
    for name, inv, r in new[:10]:
        print("  [%s] %s @ %s  %s" % (inv, name, hx(r["addr"]), r["finding"][:140]))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump({"lint": info["issues"], "merged": info["merged"], "candidate": report_json(cctx, cres, cb),
                       "local_static_crossings": [{"cut": hx(a), "grade": g, "unit": u, "symbol": n, "below": hx(b), "above": hx(c)}
                                                  for a, g, u, n, b, c in crossed],
                       "baseline_summary": bs, "new_failures": [{"unit": n, "invariant": i, "addr": r["addr"],
                                                                  "finding": r["finding"]} for n, i, r in new]}, fh, indent=1)
    return 1 if info["issues"] or new or crossed or any(r["status"] == FAIL for n in mine for r in cres.units.get(n, {}).values()) else 0


# ---- selftest -----------------------------------------------------------------------------------------------------------------

def _w(op, rt, ra, imm):
    return (op << 26) | (rt << 21) | (ra << 16) | (imm & 0xFFFF)


def _lis(rd, hi):
    return _w(15, rd, 0, hi)


def _b(site, target, link=False):
    """`b` / `bl` from `site` to `target`."""
    return (18 << 26) | ((target - site) & 0x03FFFFFC) | (1 if link else 0)


BLR = 0x4E800020
NOP = 0x60000000


def _mini(T, words, fns, units, extra_blobs=(), extra_syms=()):
    """A one-text-blob world: `words` at `T`, `fns` = [(name, offset, size)], `units` = splits text; returns `(Ctx, symbols)`."""
    code = struct.pack(">%dI" % len(words), *words)
    dol = _make_dol([(T, code)] + list(extra_blobs))
    syms = parse_symbols(["%s = .text:0x%X; // type:function size:0x%X scope:global" % (n, T + o, z) for n, o, z in fns] + list(extra_syms))
    return Ctx(parse_splits(units), syms, dol, 0x80500000, 0x80600000), syms


def _make_dol(blobs):
    """A minimal DOL image: `blobs` = [(address, bytes)] as text0.. / data0.. sections."""
    head = bytearray(0x100)
    toff, taddr, tsize = [0] * 7, [0] * 7, [0] * 7
    body = bytearray()
    for i, (a, b) in enumerate(blobs[:7]):
        toff[i], taddr[i], tsize[i] = 0x100 + len(body), a, len(b)
        body += b
    head[0x00:0x1C] = struct.pack(">7I", *toff)
    head[0x48:0x64] = struct.pack(">7I", *taddr)
    head[0x90:0xAC] = struct.pack(">7I", *tsize)
    return Dol(bytes(head) + bytes(body))


NCHECKS = [0]


def selftest():
    NCHECKS[0] = 0
    fails = []

    def check(name, got, want):
        NCHECKS[0] += 1
        ok = got == want
        print("%s %s%s" % ("ok  " if ok else "FAIL", name, "" if ok else "  got %r want %r" % (got, want)))
        if not ok:
            fails.append(name)

    # -- fixture: two units, text 0x80100000.., sdata2 pool at 0x80300000, ctors at 0x80200000
    T0, S2 = 0x80100000, 0x80300000
    a_fn1 = [_lis(3, 0x8030), _w(48, 1, 3, 0), _w(48, 2, 3, 4), 0x4E800020]                 # A.fn1: reads lit0, lit1
    a_sinit = [_lis(4, 0x8030), _w(48, 1, 4, 4), 0x4E800020, 0x60000000]                      # A.__sinit: reads lit1 (first use later)
    b_fn = [_lis(3, 0x8030), _w(48, 1, 3, 8), _w(48, 2, 3, 4), 0x4E800020]                   # B.fn: reads lit2, lit1 (shared!)
    code = struct.pack(">16I", *(a_fn1 + a_sinit + b_fn + [0x60000000] * 4))
    pool = struct.pack(">fff", 1.5, 2.5, 3.5)
    ctors = struct.pack(">II", T0 + 0x10, T0 + 0x20)                                          # A -> its sinit, B -> fn in B? (wrong: mid-unit)
    eti = struct.pack(">3I", T0, 0x10, 0x80000100) + struct.pack(">3I", T0 + 0x20, 0x10, 0x80000108)
    dol = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti)])
    syms = parse_symbols([
        "A_fn1 = .text:0x%X; // type:function size:0x10 scope:global" % T0,
        "A_sinit = .text:0x%X; // type:function size:0x10 scope:local" % (T0 + 0x10),
        "B_fn = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x20),
        "B_pad = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x30),
        "lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2,
        "lit1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 4),
        "lit2 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 8),
        "ct = .ctors:0x80200000; // type:object size:0x8 scope:local",
        "@eti_a = extabindex:0x80400000; // type:object size:0xC scope:local",
        "@eti_b = extabindex:0x8040000C; // type:object size:0xC scope:local",
        "@etb_a = extab:0x80000100; // type:object size:0x8 scope:local",
        "@etb_b = extab:0x80000108; // type:object size:0x8 scope:local",
    ])
    sp_text = """Sections:
\t.text       type:code align:32
\textab       type:rodata align:32
\textabindex  type:rodata align:32
\t.ctors      type:rodata align:16
\t.sdata2     type:rodata align:4

u_a.cpp:
\textab       start:0x80000100 end:0x80000108
\textabindex  start:0x80400000 end:0x8040000C
\t.text       start:0x80100000 end:0x80100020
\t.ctors      start:0x80200000 end:0x80200008
\t.sdata2     start:0x80300000 end:0x80300004

u_b.cpp: comment:0
\textab       start:0x80000108 end:0x80000110
\textabindex  start:0x8040000C end:0x80400018
\t.text       start:0x80100020 end:0x80100038
\t.sdata2     start:0x80300004 end:0x8030000C
"""
    sp = parse_splits(sp_text)
    check("parse units", [u.name for u in sp.units], ["u_a.cpp", "u_b.cpp"])
    check("parse attrs kept", sp.units[1].attrs, "comment:0")
    check("render round-trips ranges", [(u.name, u.ranges) for u in parse_splits(render_splits(sp)).units],
          [(u.name, u.ranges) for u in sp.units])
    ctx = Ctx(sp, syms, dol, 0x80500000, 0x80600000)
    i0, _ = ctx.data_sym_at(S2)
    check("ref decode: lis + lfs", sorted(ctx.refs.get(i0, [])), [T0 + 4])
    i1, s1 = ctx.data_sym_at(S2 + 4)
    check("ref decode: lit1 read by both units", sorted(ctx.refs[i1]), [T0 + 8, T0 + 0x14, T0 + 0x28])
    res = run_checks(ctx, None)
    check("order pass", res.units["u_a.cpp"]["order"]["status"], PASS)
    check("pool: A's literal lit1 (in B's range) is read by A and B -> B fails", res.units["u_b.cpp"]["pool"]["status"], FAIL)
    check("pool: A reads lit1 too, so the shared literal fails both", res.units["u_a.cpp"]["pool"]["status"], FAIL)
    check("pool edges both ways", sorted(ctx.pool_edges), [("u_a.cpp", "u_b.cpp"), ("u_b.cpp", "u_a.cpp")])
    check("ctors: A's last function is __sinit", res.units["u_a.cpp"]["ctors"]["status"], FAIL)   # 2 words -> multi-TU
    check("ctors: 2 words finding", "2 .ctors words" in res.units["u_a.cpp"]["ctors"]["finding"], True)
    check("ctors: second word targets B", any("u_b.cpp" in i["finding"] for i in res.units["u_a.cpp"]["ctors"]["items"]), True)
    check("extab pass for A and B", (res.units["u_a.cpp"]["extab"]["status"], res.units["u_b.cpp"]["extab"]["status"]), (PASS, PASS))
    eti_bad = struct.pack(">3I", T0, 0x10, 0x80000100) + struct.pack(">3I", T0 + 0x10, 0x10, 0x80000108)
    dol_bad = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti_bad)])
    rx = run_checks(Ctx(sp, syms, dol_bad, None, None, scan=False), None, ["extab"])
    check("extab FAIL for B (function is A's) and A is named", (rx.units["u_b.cpp"]["extab"]["status"], rx.units["u_a.cpp"]["extab"]["status"]), (FAIL, FAIL))
    check("text-cut pass", res.units["u_a.cpp"]["text-cut"]["status"], PASS)
    # order: overlap + cycle
    bad = parse_splits(sp_text.replace("start:0x80100020 end:0x80100038", "start:0x8010001C end:0x80100038"))
    cb = Ctx(bad, syms, dol, 0x80500000, 0x80600000, scan=False)
    rb = run_checks(cb, None, ["order", "text-cut"])
    check("overlap is an order FAIL", rb.units["u_b.cpp"]["order"]["status"], FAIL)
    check("text cut inside a function", rb.units["u_b.cpp"]["text-cut"]["status"], FAIL)
    cyc = parse_splits(sp_text + "\nu_c.cpp:\n\t.text       start:0x80100038 end:0x80100040\n\t.sdata2     start:0x80300000 end:0x80300000\n")
    cyc.units[0].ranges[".ctors"] = [(0x80200010, 0x80200014, "")]
    cyc.units[2].ranges[".ctors"] = [(0x80200000, 0x80200004, "")]
    rc = run_checks(Ctx(cyc, syms, dol, None, None, scan=False), None, ["order"])
    check("cycle between units is an order FAIL", rc.units["u_c.cpp"]["order"]["status"], FAIL)
    # coverage
    sp2 = parse_splits(sp_text.replace("end:0x8030000C", "end:0x80300008"))
    c2 = Ctx(sp2, syms, dol, None, None, scan=False)
    run_checks(c2, None, ["coverage"])
    check("an uncovered symbol is reported", c2.coverage_gaps[".sdata2"]["symbols"], 1)
    # proposal render / merge / lint
    p = {"phase": 1, "band": "x", "units": [
        {"derived_name": "one", "module": "m", "ranges": {".text": [["0x80100000", "0x80100010"]]},
         "cuts": [], "open_questions": []},
        {"derived_name": "two", "module": "m", "ranges": {".text": [["0x80100010", "0x80100020"]]},
         "cuts": [{"addr": "0x80100010", "section": ".text", "grade": "guess", "evidence": [], "reproduce": "tudiscover.py at 0x80100010"}]},
        {"derived_name": "three", "module": "m", "ranges": {".text": [["0x80100020", "0x80100038"]]},
         "cuts": [{"addr": "0x80100020", "section": ".text", "grade": "strong",
                   "evidence": [{"tool": "t", "command": "c", "finding": "f"}], "reproduce": "r"}]}]}
    check("lint accepts a well-formed proposal", lint_proposal(p), [])
    cand, info = render(sp, [p])
    check("guess cut merged", [m["candidate_cut"] for m in info["merged"]], ["0x80100010"])
    check("candidate units", [(u.name, u.ranges[".text"][0][:2]) for u in cand.units if u.name.startswith("m/")],
          [("m/one.cpp", (0x80100000, 0x80100020)), ("m/three.cpp", (0x80100020, 0x80100038))])
    check("proposal units sit before the baseline units they cut", [u.name for u in cand.units],
          ["m/one.cpp", "u_a.cpp", "m/three.cpp", "u_b.cpp"])
    q = json.loads(json.dumps(p))
    q["units"][2]["cuts"][0]["grade"] = "weak"
    q["units"][1]["derived_name"] = "bad name"
    check("lint reports bad grade and name", sorted(i.split(":")[0] for i in lint_proposal(q) if "grade" in i or "identifier" in i),
          ["bad name", "three"])
    # sinit-not-last: the word targets A_fn1 (ends at the middle of A): boundary evidence
    ct2 = struct.pack(">II", T0 + 0x00, 0)
    dol_s = _make_dol([(T0, code), (0x80200000, ct2), (S2, pool), (0x80400000, eti)])
    rs = run_checks(Ctx(sp, syms, dol_s, None, None, scan=False), None, ["ctors"])
    cuts = [i.get("cut_at") for i in rs.units["u_a.cpp"]["ctors"]["items"] if i.get("cut_at")]
    check("ctors: not-last sinit names the cut", (rs.units["u_a.cpp"]["ctors"]["status"], cuts), (FAIL, [T0 + 0x10]))
    # dtors: the crt chain entry is exempt
    sp_d = parse_splits(sp_text.replace("	.ctors      start:0x80200000 end:0x80200008", "	.dtors      start:0x80200000 end:0x80200004"))
    syms_d = syms + parse_symbols(["__destroy_global_chain = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x30)])
    dol_d = _make_dol([(T0, code), (0x80200000, struct.pack(">I", T0 + 0x30)), (S2, pool), (0x80400000, eti)])
    rd = run_checks(Ctx(sp_d, syms_d, dol_d, None, None, scan=False), None, ["dtors"])
    check("dtors: __destroy_global_chain entry passes", rd.units["u_a.cpp"]["dtors"]["status"], PASS)
    # bss: a local object read only by the other unit
    syms_b = syms + parse_symbols(["bssvar = .bss:0x80700000; // type:object size:0x4 scope:local"])
    sp_b = parse_splits(sp_text.replace("	.sdata2     start:0x80300000 end:0x80300004", "	.sdata2     start:0x80300000 end:0x80300004\n\t.bss        start:0x80700000 end:0x80700004", 1))
    cb2 = Ctx(sp_b, syms_b, dol, 0x80500000, 0x80600000, scan=False)
    cb2.refs[cb2.data_sym_at(0x80700000)[0]] = [T0 + 0x24]
    rb2 = run_checks(cb2, None, ["bss"])
    check("bss: local object read only by another unit fails", rb2.units["u_a.cpp"]["bss"]["status"], FAIL)
    # data-order + vtable
    D0 = 0x80600000
    blob = struct.pack(">3I", 0, 0, T0) + b"hello\x00\x00\x00" + struct.pack(">3I", 0, 0, T0 + 0x20)
    dol_v = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti), (D0, blob)])
    syms_v = syms + parse_symbols(["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0,
                                   "str_h = .data:0x%X; // type:object size:0x8 scope:local data:string" % (D0 + 12),
                                   "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 20)])
    sp_v = parse_splits(sp_text.replace("	.sdata2     start:0x80300000 end:0x80300004",
                                        "	.sdata2     start:0x80300000 end:0x80300004\n\t.data       start:0x%X end:0x%X" % (D0, D0 + 32), 1))
    cv = Ctx(sp_v, syms_v, dol_v, None, None, scan=False)
    rv = run_checks(cv, dataorder_rows(syms_v), ["data-order", "vtable"])
    check("data-order: a V->S seam inside one unit's .data fails", rv.units["u_a.cpp"]["data-order"]["status"], FAIL)
    check("vtable: B's slot is in unit B, so A's .data holding it fails", rv.units["u_a.cpp"]["vtable"]["status"], FAIL)
    # derived attached ranges + name clash + hole
    der = render(sp, [p], dol, syms)[1]["derived"]
    check("derive: extab/extabindex/ctors follow the text cut", sorted(d.split(" ", 1)[1] for d in der if d.startswith("m/one")),
          [".ctors", "extab", "extabindex"])
    p_clash = json.loads(json.dumps(p))
    p_clash["units"][0]["derived_name"] = "u_a"
    p_clash["units"][0]["module"] = "."
    p_clash["units"][0]["ranges"] = {".text": [["0x80100000", "0x80100010"]]}
    hole = {"units": [{"derived_name": "h", "module": "m", "ranges": {".text": [["0x80100008", "0x80100010"]]}, "cuts": []}]}
    check("render reports a hole punched in a baseline unit", any("punches a hole" in i for i in render(sp, [hole])[1]["issues"]), True)
    # jump-table ownership
    jt = struct.pack(">II", T0 + 0x04, T0 + 0x24)
    dol3 = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti), (0x80500000, jt)])
    syms3 = syms + parse_symbols(["jumptable_80500000 = .data:0x80500000; // type:object size:0x8 scope:local"])
    sp3 = parse_splits(sp_text.replace("\t.sdata2     start:0x80300000 end:0x80300004",
                                       "\t.sdata2     start:0x80300000 end:0x80300004\n\t.data       start:0x80500000 end:0x80500008"))
    r3 = run_checks(Ctx(sp3, syms3, dol3, None, None, scan=False), None, ["jumptable"])
    check("jump table branching into another unit fails", r3.units["u_a.cpp"]["jumptable"]["status"], FAIL)
    # gap 1: a `.ctors` word is not a cut at the end of its __sinit - the closure of its local callees ends the TU
    T = 0x80100000
    hdr = "Sections:\n\t.text       type:code align:32\n\t.ctors      type:rodata align:16\n\t.sdata2     type:rodata align:4\n\n"
    ctor_blob = (0x80200000, struct.pack(">I", T + 0x10))

    def ctors_of(end, words, fns):
        c, _s = _mini(T, words, fns, hdr + "u.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n" % (T, T + end),
                      [ctor_blob])
        r = run_checks(c, None, ["ctors"]).units["u.cpp"]["ctors"]
        return r["status"], [i.get("cut_at") for i in r["items"] if i.get("cut_at")]

    # F0 0x00..0x10, sinit = `b ctor` 0x10..0x14, ctor 0x14..0x24, g 0x24..0x34
    thunk = [NOP, NOP, NOP, BLR, _b(T + 0x10, T + 0x14), BLR, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, BLR]
    fn_thunk = [("F0", 0, 0x10), ("sinit", 0x10, 4), ("ctor", 0x14, 0x10), ("g", 0x24, 0x10)]
    thunk += [NOP] * 4
    check("ctors closure: a `b ctor` sinit whose ctor ends at the unit end passes", ctors_of(0x24, thunk, fn_thunk), (PASS, []))
    check("ctors closure: a unit that goes on after the ctor is cut at L, the function after the closure", ctors_of(0x34, thunk, fn_thunk), (FAIL, [T + 0x24]))
    called = list(thunk)
    called[1] = _b(T + 4, T + 0x24, True)                       # F0 calls g: g belongs to the TU before the sinit
    check("ctors closure: the function at L called from before L is not confirmed", ctors_of(0x34, called, fn_thunk), (UNKNOWN, []))
    # sinit 0x10..0x20 takes the address of the dtor at 0x20 (lis/addi), then `after` 0x28..0x38
    addr = [NOP, NOP, NOP, BLR, _lis(3, T >> 16), _w(14, 3, 3, 0x20), BLR, NOP, BLR, NOP, NOP, BLR, NOP, NOP, NOP, BLR, NOP, NOP, NOP, BLR]
    fn_addr = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("dtor", 0x20, 0x8), ("after", 0x28, 0x10)]
    check("ctors closure: an address-taken dtor after the sinit is in the closure", ctors_of(0x28, addr, fn_addr), (PASS, []))
    check("ctors closure: the tail after an address-taken dtor is cut at L", ctors_of(0x38, addr, fn_addr), (FAIL, [T + 0x28]))
    plain = [NOP, NOP, NOP, BLR, BLR, NOP, NOP, NOP, NOP, NOP, NOP, BLR, NOP, NOP, NOP, NOP]
    check("ctors closure: a sinit with no local callee cuts at its own end", ctors_of(0x34, plain, [("F0", 0, 0x10), ("sinit", 0x10, 0x8), ("h", 0x18, 0x10)]),
          (FAIL, [T + 0x18]))

    # gap 1b: inline virtual functions emitted after the sinit are the unit's own: slots of a vtable the unit stores
    DV = 0x80600000
    vt_hi, vt_lo = DV >> 16, DV & 0xFFFF
    slots_code = [NOP, NOP, NOP, BLR, _lis(3, vt_hi), _w(14, 3, 3, vt_lo), BLR, NOP, BLR, NOP, BLR, NOP]   # F0, sinit (stores __vt__V), slot1, slot2
    slot_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 8), ("slot2", 0x28, 8)]
    vt_blob = struct.pack(">4I", 0, 0, T + 0x20, T + 0x28)
    vt_sym = ["__vt__V = .data:0x%X; // type:object size:0x10 scope:global" % DV]
    vt_units = (hdr + "\t.data       type:rodata align:8\n\nu.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n"
                "\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, DV, DV + 0x10))

    def slot_run(words, cut_to=0x30, fns=None):
        c, _s = _mini(T, words, fns or slot_fns, vt_units.replace("end:0x%X\n\t.ctors" % (T + 0x30), "end:0x%X\n\t.ctors" % (T + cut_to)),
                      [ctor_blob, (DV, vt_blob)], vt_sym)
        r = run_checks(c, None, ["ctors"]).units["u.cpp"]["ctors"]
        return r["status"], [i.get("cut_at") for i in r["items"] if i.get("cut_at")]

    check("ctors closure: the slots of a vtable the unit stores, after the sinit, are the unit's own", slot_run(slots_code), (PASS, []))
    no_store = list(slots_code)
    no_store[4], no_store[5] = NOP, NOP
    check("ctors closure: slots of a vtable the unit never stores are not (cut at the first slot)", slot_run(no_store), (FAIL, [T + 0x20]))
    check("ctors closure: a unit that goes on past the slot run is cut after the run's end", slot_run(slots_code + [NOP] * 4, 0x40), (FAIL, [T + 0x30]))
    # a unit that ENDS inside its own slot run is early: the closure overshoots the unit end (was a PASS)
    check("ctors closure: a cut placed over the unit's own slots (unit ends at the first slot) is flagged at the run's end",
          slot_run(slots_code, 0x20), (FAIL, [T + 0x30]))
    check("ctors closure: an overshoot inside the alignment slack is not flagged (unit ends 0x28, run ends 0x30)", slot_run(slots_code, 0x28), (PASS, []))
    check("ctors closure: a unit that ends at its sinit, over slots of a vtable it never stores, is not an overshoot", slot_run(no_store, 0x20), (PASS, []))
    big_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 0x100), ("slot2", 0x120, 8)]
    check("ctors closure: a large function after the end is another TU's member, not an own inline slot",
          slot_run(slots_code + [NOP] * 0x40, 0x20, big_fns), (PASS, []))
    mixed_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 8), ("slot2", 0x28, 0x100)]
    vt_blob2 = struct.pack(">4I", 0, 0, T + 0x20, T + 0x28)
    check("ctors closure: a run that opened with a stub goes on over a large slot (the cut is at the run's end)",
          slot_run(slots_code + [NOP] * 0x40, 0x20, mixed_fns), (FAIL, [T + 0x128]))

    # gap 2: a lis/addi that only forms a pool address is not a read of the literal; a load through it is
    S2 = 0x80300000
    hi, lo = S2 >> 16, S2 & 0xFFFF
    x_fn = [_lis(3, hi), _w(14, 3, 3, lo), _b(T + 8, T + 0x40, True), BLR]                          # X: passes the address of lit0 to a call
    y_fn = [_lis(3, hi), _w(48, 1, 3, lo), BLR, NOP]                                                # Y: lis + lfs lit0
    z_fn = [_lis(3, hi), _w(14, 3, 3, lo), _w(48, 1, 3, 0), BLR]                                    # Z: lis + addi + lfs 0(r3)
    w_fn = [_lis(3, hi), _w(14, 3, 3, lo), _b(T + 0x38, T + 0x40, True), _w(48, 1, 3, 0)]            # W: address, a call, then lfs 0(r3): r3 is gone
    u_txt = hdr + "".join("%s.cpp:\n\t.text       start:0x%X end:0x%X\n%s" % (n, T + o, T + o + 0x10, ("\t.sdata2     start:0x%X end:0x%X\n" % (S2, S2 + 4)) if n == "y" else "")
                          for n, o in (("x", 0), ("y", 0x10), ("z", 0x20), ("w", 0x30)))
    cg, _s = _mini(T, x_fn + y_fn + z_fn + w_fn, [("x", 0, 0x10), ("y", 0x10, 0x10), ("z", 0x20, 0x10), ("w", 0x30, 0x10)], u_txt,
                   [(S2, struct.pack(">f", 1.5))], ["lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2])
    lit = cg.data_sym_at(S2)[1]
    check("pool decode: every lis/addi/load stays a reference", sorted(cg.readers(lit)), [T + 4, T + 0x14, T + 0x24, T + 0x34])
    check("pool decode: only loads read the literal (Y's lis+lfs, Z's addi+lfs; not X's address, not W's stale r3)",
          sorted(cg.literal_readers(lit)), [T + 0x14, T + 0x28])
    rg = run_checks(cg, None, ["pool"])
    check("pool decode: a unit that only forms the address is not a second reader of the literal",
          rg.units.get("x.cpp", {}).get("pool", {}).get("status", NA), NA)

    # gap 2b: a literal in an UNOWNED `.sdata2` pool past the last owned range is still decoded (the map's data extent counts)
    un_txt = hdr + "y.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 0x10)
    cu, _s = _mini(T, y_fn + [NOP] * 4, [("y", 0, 0x10)], un_txt, [(S2, struct.pack(">f", 1.5))],
                   ["lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2])
    lit_u = cu.data_sym_at(S2)[1]
    check("pool decode: a literal of an unowned pool beyond every owned range is read by the text that loads it",
          (cu.owner(".sdata2", S2), sorted(cu.literal_readers(lit_u))), (None, [T + 4]))

    # gap 3: pool first-use order is judged per function (scheduling reorders two loads of one function)
    lits2 = struct.pack(">ff", 1.5, 2.5)
    s2_syms = ["l0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2,
               "l1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 4)]
    pool_txt = hdr + "p.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n" % (T, T + 0x20, S2, S2 + 8)
    one_fn = [_lis(3, hi), _w(48, 2, 3, 4), _w(48, 1, 3, 0), BLR, BLR, NOP, NOP, NOP]              # l1 loaded before l0, in ONE function
    two_fn = [_lis(3, hi), _w(48, 2, 3, 4), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 0), BLR, NOP]      # f0 uses l1, the later f1 uses l0
    for label, words, want in (("the same function", one_fn, PASS), ("an earlier function", two_fn, FAIL)):
        cp, _s = _mini(T, words, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], pool_txt, [(S2, lits2)], s2_syms)
        rp = run_checks(cp, None, ["pool"]).units["p.cpp"]["pool"]
        check("pool order: l1 first used before l0 in %s -> %s" % (label, want), rp["status"], want)

    # gap 7: a V->S seam whose vtable's constructor is in the closure of the unit's own sinit is not a seam
    D0 = 0x80600000
    dhdr = hdr + "\t.data       type:rodata align:8\n\n"
    vblob = struct.pack(">3I", 0, 0, T) + b"hello\x00\x00\x00" + struct.pack(">3I", 0, 0, T + 4)
    vsyms = ["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0,
             "str_h = .data:0x%X; // type:object size:0x8 scope:local data:string" % (D0 + 12),
             "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 20)]
    vunits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, D0 + 32)
    vhi, vlo = (D0 + 20) >> 16, (D0 + 20) & 0xFFFF
    store = [_lis(3, vhi), _w(14, 3, 3, vlo), BLR, NOP]                                               # a ctor storing the address of __vt__B
    fns_v = [("F0", 0, 0x10), ("sinit", 0x10, 4), ("ctor", 0x20, 0x10)]
    for label, sinit_word, want in (("the sinit calls a ctor that stores __vt__B", _b(T + 0x10, T + 0x20), PASS),
                                    ("the store is in a function the sinit never reaches", BLR, FAIL)):
        words = [NOP, NOP, NOP, BLR, sinit_word, NOP, NOP, NOP] + store + [NOP] * 4
        cv2, sv = _mini(T, words, fns_v, vunits, [(0x80200000, struct.pack(">I", T + 0x10)), (D0, vblob)], vsyms)
        rv2 = run_checks(cv2, dataorder_rows(sv), ["data-order"])
        check("data-order: %s" % label, rv2.units["v.cpp"]["data-order"]["status"], want)

    # local-static: a scope:local object is one TU's; read by two units it fails both, and no proposed cut may sit between its readers
    LS = 0x80700000
    ls_hdr = "Sections:\n\t.text       type:code align:32\n\t.bss        type:bss align:8\n\n"
    ls_units = ls_hdr + "u1.cpp:\n\t.text       start:0x%X end:0x%X\n\t.bss        start:0x%X end:0x%X\n\nu2.cpp:\n\t.text       start:0x%X end:0x%X\n" \
        % (T, T + 0x10, LS, LS + 8, T + 0x10, T + 0x20)
    rd_fn = [_lis(3, LS >> 16), _w(14, 3, 3, LS & 0xFFFF), BLR, NOP]                               # forms the address of the object
    ls_syms = ["stat = .bss:0x%X; // type:object size:0x4 scope:local" % LS, "stat2 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 4)]
    cl, _s = _mini(T, rd_fn + rd_fn + rd_fn + rd_fn, [("a", 0, 0x10), ("b", 0x10, 0x10), ("c", 0x20, 0x10), ("d", 0x30, 0x10)], ls_units, (), ls_syms)
    rl = run_checks(cl, None, ["local-static"])
    check("local-static: a local object read from two units fails both", (rl.units["u1.cpp"]["local-static"]["status"],
                                                                           rl.units["u2.cpp"]["local-static"]["status"]), (FAIL, FAIL))
    cg2, _s = _mini(T, rd_fn + rd_fn + rd_fn + rd_fn, [("a", 0, 0x10), ("b", 0x10, 0x10), ("c", 0x20, 0x10), ("d", 0x30, 0x10)], ls_units, (),
                    [ls_syms[0].replace("scope:local", "scope:global"), ls_syms[1]])
    rg2 = run_checks(cg2, None, ["local-static"])
    check("local-static: a global object read from two units is not a finding", rg2.units.get("u1.cpp", {}).get("local-static", {}).get("status", NA), NA)
    check("local-static crossing: a cut between two readers of a local object is reported",
          [(c[0], c[3]) for c in local_static_crossings(cl, [(T + 0x10, "guess", "u2"), (T + 0x04, "guess", "x"), (T + 0x40, "guess", "y")])],
          [(T + 0x10, "stat")])
    # the update-form store leaves rA = the effective address: `stwu r0, lo(r3)` then `stw r0, 4(r3)` reads/writes stat2, not a stale lis
    upd = [_lis(3, LS >> 16), _w(37, 0, 3, 0x10), _w(36, 0, 3, 4), BLR]
    cu2, _s = _mini(T, upd + [NOP] * 12, [("a", 0, 0x10), ("b", 0x10, 0x30)], ls_units, (),
                    ls_syms + ["stat3 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 0x10),
                               "stat4 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 0x14)])
    check("decode: an update-form store moves rA, so the next displacement is relative to the updated address (not to the lis value)",
          [sorted(cu2.readers(cu2.data_sym_at(a)[1])) for a in (LS + 4, LS + 0x10, LS + 0x14)], [[], [T + 4], [T + 8]])
    cl2, _s = _mini(T, [_lis(3, LS >> 16), _b(T + 4, T + 0x30, True), _w(36, 0, 3, 0), BLR] + [NOP] * 12, [("a", 0, 0x10), ("b", 0x10, 0x30)], ls_units, (), ls_syms)
    check("decode: a `lis` register does not survive a call", cl2.readers(cl2.data_sym_at(LS)[1]), [])

    # reproduce rows: the per-word closure line and the pool interval line print the numbers a finding quotes
    cdet, _s = _mini(T, slots_code, slot_fns, vt_units.replace("end:0x%X\n\t.ctors" % (T + 0x30), "end:0x%X\n\t.ctors" % (T + 0x20)),
                     [ctor_blob, (DV, vt_blob)], vt_sym)
    check("ctors_detail: names the word, the closure end, the own-slot end and the unit end",
          ctors_detail(cdet, cdet.by_name["u.cpp"]),
          [".ctors word 0x80200000 = 0x%X (sinit, ends 0x%X): closure ends 0x%X, with own vtable slots 0x%X; unit text ends 0x%X"
           % (T + 0x10, T + 0x20, T + 0x20, T + 0x30, T + 0x20)])
    eq_pool = struct.pack(">ff", 1.5, 1.5)
    pairs_fn = [_lis(3, hi), _w(48, 1, 3, 0), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 4), BLR, NOP]
    cpi, _s = _mini(T, pairs_fn, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], pool_txt, [(S2, eq_pool)], s2_syms)
    check("pool_intervals: one value at two pool addresses prints both reads and the interval a TU starts in",
          pool_intervals(cpi), ["pooldup value 0x3fc00000: l0 0x%X (last read 0x%X) and l1 0x%X (first read 0x%X); a TU starts in (0x%X, 0x%X]: 1 function starts"
                                % (S2, T + 4, S2 + 4, T + 0x14, T, T + 0x10)])

    two_txt = hdr + "p1.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n\np2.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 0x10, S2, S2 + 8, T + 0x10, T + 0x20)
    c2u, _s = _mini(T, pairs_fn, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], two_txt, [(S2, eq_pool)], s2_syms)
    check("pool_intervals: a pair read by two units says so, and --unit filters by either reader",
          ([l.endswith("[already two units: p1.cpp | p2.cpp]") for l in pool_intervals(c2u)], len(pool_intervals(c2u, lambda n: n == "p2.cpp")), len(pool_intervals(c2u, lambda n: n == "zzz"))),
          ([True], 1, 0))

    # gap 4a: a guess cut merges with the unit that TOUCHES it (proposal or registered); none touching is a lint error
    def prop(*units, **more):
        return dict({"phase": 1, "band": "t", "units": list(units)}, **more)

    def pu(name, a, b, *cuts, **more):
        return dict({"derived_name": name, "module": "m", "ranges": {".text": [["0x%X" % a, "0x%X" % b]]}, "cuts": list(cuts)}, **more)

    def cut(addr, grade, **more):
        return dict({"addr": "0x%X" % addr, "section": ".text", "grade": grade, "reproduce": "r",
                     "evidence": [{"tool": "t", "command": "c", "finding": "f"}]}, **more)

    cand4, info4 = render(sp, [prop(pu("first", T, T + 0x10), pu("late", T + 0x10, T + 0x20, cut(T + 0x10, "guess")))])
    check("guess cut touching the previous proposal unit merges there", (info4["issues"], [m.get("into") for m in info4["merged"]]), ([], [None]))
    cand4, info4 = render(sp, [prop(pu("first", T, T + 0x10), pu("late", T + 0x28, T + 0x38, cut(T + 0x28, "guess")))])
    check("a guess cut that touches no proposal unit and no registered unit end is refused",
          [i for i in info4["issues"] if "no adjacent unit" in i] != [], True)
    check("... and the unit stays (it is not folded into a non-adjacent neighbour)", "m/late.cpp" in [u.name for u in cand4.units], True)
    cand4, info4 = render(sp, [prop(pu("tail", T + 0x20, T + 0x38, cut(T + 0x20, "guess")))])
    text4 = {u.name: u.ranges.get(".text") for u in cand4.units}
    check("a guess cut on the first unit merges into the registered unit whose range ends there", (info4["issues"], [m.get("into") for m in info4["merged"]],
                                                                                                 text4.get("u_a.cpp"), "m/tail.cpp" in text4),
          ([], ["u_a.cpp"], [(T, T + 0x38, "")], False))

    # reconcile 1: a guess cut that touches the end of a unit already folded into a registered unit folds into that registered unit
    chain = prop(pu("tail", T + 0x20, T + 0x30, cut(T + 0x20, "guess")), pu("tail2", T + 0x30, T + 0x38, cut(T + 0x30, "guess")))
    c_ch, i_ch = render(sp, [chain])
    text_ch = {u.name: u.ranges.get(".text") for u in c_ch.units}
    check("a guess chain through a registered unit folds every link into it (was: 'no adjacent unit' for the second)",
          (i_ch["issues"], [m.get("into") for m in i_ch["merged"]], text_ch.get("u_a.cpp"), [n for n in text_ch if n.startswith("m/")]),
          ([], ["u_a.cpp", "u_a.cpp"], [(T, T + 0x38, "")], []))
    # reconcile 2: `supersedes` drops the named band's unit before rendering (and warns when no such unit exists)
    band_f = dict(prop(pu("dup", T, T + 0x20)), band="f")
    band_g = dict(prop(pu("dup", T, T + 0x28)), band="g", supersedes=[{"band": "f", "unit": "dup", "reason": "g owns the longer unit"}])
    c_sup, i_sup = render(sp, [band_f, band_g])
    check("supersedes: only the superseding band's unit survives, no name clash", ([i for i in i_sup["issues"]], i_sup["superseded"],
                                                                                  [(u.name, u.ranges[".text"][0][:2]) for u in c_sup.units if u.name.startswith("m/")]),
          ([], ["band f unit dup"], [("m/dup.cpp", (T, T + 0x28))]))
    band_g["supersedes"] = [{"band": "f", "unit": "nope", "reason": "r"}, {"band": "f", "unit": "dup"}]
    check("supersedes: a missing unit warns, an entry without a reason is an error",
          ([w for w in render(sp, [band_f, band_g])[1]["warnings"] if w.startswith("supersedes")],
           any("supersedes entry needs" in i for i in lint_proposal(band_g, sp))),
          (["supersedes: band f has no unit nope"], True))
    # gap 4b/4c: first-class fields, validated; keep_registered_edge is grade-neutral and never merged
    full = prop(pu("a1", T, T + 0x20, removes_cuts=[cut(T + 0x20, "strong")], absorbs=["u_b.cpp"], replaces_tail_of="u_a.cpp", kind="x"),
                open_questions=[{"unit": "u_a.cpp", "question": "q"}, "plain"])
    full["units"][0]["cuts"] = []
    e_full, w_full = lint_proposal_full(full, sp)
    check("lint: a proposal using every documented field is clean except the unknown `kind` on a unit", (e_full, w_full),
          ([], ["a1: unknown field 'kind'"]))
    bad = prop(pu("b1", T, T + 0x20, removes_cuts=[{"addr": "0x80100999", "section": ".text"}], absorbs=["nope.cpp"], replaces_tail_of=["nope.cpp"]),
               open_questions="text")
    bad["units"][0]["open_questions"] = 5
    e_bad, w_bad = lint_proposal_full(bad, sp)
    check("lint: wrong types are errors (open_questions)", sorted(i for i in e_bad if "open_questions" in i), ["b1: field 'open_questions' is int, want list", "proposal: field 'open_questions' is str, want list"])
    check("lint: names that are not baseline state are warnings", sorted(w_bad), ["b1: absorbs nope.cpp, which is not a baseline unit", "b1: removes_cuts .text 0x80100999 is not a registered cut",
                                                                                   "b1: replaces_tail_of nope.cpp is not a baseline unit"])
    edge_end = prop(pu("e1", T, T + 0x20, removes_cuts=[{"addr": "0x%X" % (T + 0x38), "section": ".text"}]))
    check("lint: a removes_cuts at the END of a registered range (against an unowned run) is a registered edge, no warning",
          [w for w in lint_proposal_full(edge_end, sp)[1] if "removes_cuts" in w], [])
    keep = cut(T + 0x20, "guess", keep_registered_edge=True, evidence=[])
    c5, i5 = render(sp, [prop(pu("m1", T + 0x20, T + 0x38, keep))])
    check("keep_registered_edge: a guess cut is not merged and is counted apart", (i5["issues"], i5["merged"], i5["cuts"], "m/m1.cpp" in [u.name for u in c5.units]),
          ([], [], {"keep_registered_edge": 1}, True))
    off = cut(T + 0x28, "guess", keep_registered_edge=True)
    check("keep_registered_edge at a place no registered unit starts is an error", any("keep_registered_edge" in i for i in render(sp, [prop(pu("m2", T + 0x28, T + 0x38, off))])[1]["issues"]), True)
    keep_end = cut(T + 0x38, "guess", keep_registered_edge=True, evidence=[])
    c6, i6 = render(sp, [prop(pu("m4", T + 0x38, T + 0x48, keep_end))])
    check("keep_registered_edge at the END of a registered unit (an unowned run's start) is allowed and the unit is not folded into it",
          (i6["issues"], i6["merged"], "m/m4.cpp" in [u.name for u in c6.units]), ([], [], True))
    nograde = {"addr": "0x%X" % (T + 0x20), "section": ".text", "keep_registered_edge": True, "reproduce": "r"}
    check("keep_registered_edge needs no grade and no evidence", lint_proposal(prop(pu("m3", T + 0x20, T + 0x38, nograde)), sp), [])
    check("a cut without a grade and without the flag is still an error", bool(lint_proposal(prop(pu("m3", T + 0x20, T + 0x38, dict(nograde, keep_registered_edge=False))), sp)), True)

    # gap 5: the data of a recut registered unit goes to the pieces by reader (provisional default), and can be switched off
    DS = 0x80300000
    wa = [_lis(3, DS >> 16), _w(48, 1, 3, 0), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn A (0x00..0x20) reads l0
    wb = [_lis(3, DS >> 16), _w(48, 1, 3, 4), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn B (0x20..0x40) reads l1
    d_txt = hdr + "big.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n" % (T, T + 0x40, DS, DS + 8)
    d_ctx, d_syms = _mini(T, wa + wb, [("A", 0, 0x20), ("B", 0x20, 0x20)], d_txt, [(DS, struct.pack(">ff", 1.5, 2.5))],
                          ["l0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % DS,
                           "l1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (DS + 4)])
    d_dol = _make_dol([(T, struct.pack(">16I", *(wa + wb))), (DS, struct.pack(">ff", 1.5, 2.5))])
    d_base = parse_splits(d_txt)
    d_prop = prop(pu("bpiece", T + 0x20, T + 0x40, cut(T + 0x20, "strong")))
    c_on, i_on = render(d_base, [d_prop], d_dol, d_syms)
    c_off, i_off = render(d_base, [d_prop], d_dol, d_syms, data_by_reader=False)
    sd = lambda c: {u.name: [(a, b) for a, b, _x in u.ranges.get(".sdata2", [])] for u in c.units}
    check("data-by-reader: the literal B reads moves to B's unit, A's stays", sd(c_on), {"big.cpp": [(DS, DS + 4)], "m/bpiece.cpp": [(DS + 4, DS + 8)]})
    check("data-by-reader: it names the unit and run it assigned", [(r["unit"], r["section"], r["misplaced"]) for r in i_on["data_by_reader"]], [("big.cpp", ".sdata2", 0)])
    check("data-by-reader off: the old behaviour, the data stays with the recut unit", sd(c_off), {"big.cpp": [(DS, DS + 8)], "m/bpiece.cpp": []})
    explicit = json.loads(json.dumps(d_prop))
    explicit["units"][0]["ranges"][".sdata2"] = [["0x%X" % DS, "0x%X" % (DS + 8)]]
    c_ex, _i = render(d_base, [explicit], d_dol, d_syms)
    check("data-by-reader leaves a section the proposal lists itself", sd(c_ex)["m/bpiece.cpp"], [(DS, DS + 8)])

    # phase 2 format: the canonical `attach` list (the legacy names load as aliases), rendered by one rule set
    def arow(unit, a, b, grade="strong", sec=".sdata2", **more):
        return dict({"unit": unit, "section": sec, "range": ["0x%X" % a, "0x%X" % b], "grade": grade, "reproduce": "r",
                     "evidence": [{"tool": "t", "command": "c", "finding": "f"}]}, **more)

    def p2(*rows, **more):
        return dict({"phase": 2, "lane": "t", "units": [], "attach": list(rows)}, **more)

    UN = DS + 0x100                                           # unowned `.sdata2` bytes of the d_base fixture
    shapes = p2(arow("big.cpp", UN, UN + 4),
                {"unit": "big.cpp", "ranges": {".sdata2": [["0x%X" % (UN + 4), "0x%X" % (UN + 8)]]}, "grade": "strong", "reproduce": "r",
                 "evidence": [{"tool": "t", "command": "c", "finding": "f"}]},
                {"unit": "big.cpp", "section": ".sdata2", "ranges": [["0x%X" % (UN + 8), "0x%X" % (UN + 12)]], "grade": "strong", "reproduce": "r",
                 "evidence": [{"tool": "t", "command": "c", "finding": "f"}]},
                {"unit": "big.cpp", "section": ".sdata2", "start": "0x%X" % (UN + 12), "end": "0x%X" % (UN + 16), "grade": "strong", "reproduce": "r",
                 "evidence": [{"tool": "t", "command": "c", "finding": "f"}]})
    c_sh, i_sh = render(d_base, [shapes], d_dol, d_syms)
    check("attach: a row may carry `range`, `ranges` as a dict or as a list, or `start`/`end`; four abutting rows are one range",
          (i_sh["issues"], i_sh["warnings"], sd(c_sh)["big.cpp"], len(i_sh["attach"])), ([], [], [(DS, DS + 8), (UN, UN + 16)], 4))
    legacy = {k: v for k, v in p2(arow("big.cpp", UN, UN + 4)).items() if k != "attach"}
    legacy["data_attach"] = [arow("big.cpp", UN, UN + 4)]
    legacy["_file"] = "phase2-x.json"
    legacy2 = dict(legacy)
    del legacy2["data_attach"]
    legacy2["attachments"] = [arow("big.cpp", UN, UN + 4)]
    c_lg, i_lg = render(d_base, [legacy], d_dol, d_syms)
    c_l2, i_l2 = render(d_base, [legacy2], d_dol, d_syms)
    c_cn, i_cn = render(d_base, [p2(arow("big.cpp", UN, UN + 4))], d_dol, d_syms)
    check("attach: `data_attach` and `attachments` load as aliases of `attach` (the same candidate) and a warning names the file",
          (sd(c_lg) == sd(c_l2) == sd(c_cn), sd(c_cn)["big.cpp"], i_lg["warnings"], i_l2["warnings"]),
          (True, [(DS, DS + 8), (UN, UN + 4)], ["proposal phase2-x.json: field `data_attach` is the legacy name of `attach`"],
           ["proposal phase2-x.json: field `attachments` is the legacy name of `attach`"]))
    check("attach: the canonical name carries no alias warning", i_cn["warnings"], [])
    both = dict(legacy, attach=[arow("big.cpp", UN + 4, UN + 8)])
    check("attach: a file with both the legacy and the canonical name is a lint error",
          [i for i in render(d_base, [both], d_dol, d_syms)[1]["issues"] if "has both" in i], ["proposal phase2-x.json: has both `data_attach` and `attach`"])
    c_gs, i_gs = render(d_base, [p2(arow("big.cpp", UN, UN + 4, "guess"))], d_dol, d_syms)
    check("attach: a guess row is counted and never applied", (sd(c_gs)["big.cpp"], len(i_gs["attach_skipped"]), i_gs["attach"], i_gs["issues"]), ([(DS, DS + 8)], 1, [], []))
    c_ta, i_ta = render(d_base, [d_prop, p2(dict(arow("big.cpp", UN, UN + 4), text_addr="0x%X" % (T + 0x30)))], d_dol, d_syms)
    check("attach: `text_addr` addresses the unit holding that `.text` (the name is only checked: a mismatch warns)",
          (sd(c_ta)["m/bpiece.cpp"], i_ta["warnings"]), ([(DS + 4, DS + 8), (UN, UN + 4)], ["attach big.cpp .sdata2: text_addr 0x%X is in m/bpiece.cpp" % (T + 0x30)]))
    c_tn, i_tn = render(d_base, [p2(dict(arow("big.cpp", UN, UN + 4), text_addr="0x80999999"))], d_dol, d_syms)
    check("attach: a `text_addr` inside no unit is a lint line", [i for i in i_tn["issues"] if "is in no unit" in i] != [], True)
    c_nu, i_nu = render(d_base, [p2(arow("nope.cpp", UN, UN + 4))], d_dol, d_syms)
    check("attach: a unit the candidate lacks is a lint line", [i for i in i_nu["issues"] if "unit nope.cpp is not in the candidate" in i] != [], True)
    # what a range may take: unowned bytes, its own bytes, a provisional by-reader piece, a `takes_from` unit's bytes - nothing else
    c_pv, i_pv = render(d_base, [d_prop, p2(arow("big.cpp", DS, DS + 8))], d_dol, d_syms)
    check("attach: a row takes back a PROVISIONAL by-reader piece (the evidence replaces the default)", (i_pv["issues"], sd(c_pv)),
          ([], {"big.cpp": [(DS, DS + 8)], "m/bpiece.cpp": []}))
    c_np, i_np = render(d_base, [d_prop, p2(arow("m/bpiece.cpp", DS, DS + 4))], d_dol, d_syms, data_by_reader=False)
    check("attach: a registered unit's own range that no cut recut is not taken (lint line naming the donor); nothing changes",
          ([i for i in i_np["issues"] if "from big.cpp" in i] != [], sd(c_np)), (True, {"big.cpp": [(DS, DS + 8)], "m/bpiece.cpp": []}))
    c_tk, i_tk = render(d_base, [d_prop, p2(arow("m/bpiece.cpp", DS, DS + 4, takes_from="big.cpp"))], d_dol, d_syms, data_by_reader=False)
    check("attach: `takes_from` names the unit the row may take from", (i_tk["issues"], sd(c_tk)), ([], {"big.cpp": [(DS + 4, DS + 8)], "m/bpiece.cpp": [(DS, DS + 4)]}))
    c_rs, i_rs = render(d_base, [p2(arow("big.cpp", DS, DS + 8))], d_dol, d_syms)
    check("attach: a row restating the target's own range changes nothing and is no lint line", (i_rs["issues"], sd(c_rs)["big.cpp"]), ([], [(DS, DS + 8)]))
    c_ov, i_ov = render(d_base, [p2(arow("big.cpp", UN, UN + 8), arow("big.cpp", UN + 4, UN + 12))], d_dol, d_syms)
    check("attach: two rows over the same bytes are a lint line (one applied)",
          (any("overlap in .sdata2" in i or "overlaps another attach row" in i for i in i_ov["issues"]), sd(c_ov)["big.cpp"]), (True, [(DS, DS + 8), (UN, UN + 8)]))
    f_txt = hdr + ("u.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n\nz.cpp:\n\t.sdata2     start:0x%X end:0x%X\n"
                   % (T, T + 0x40, DS, DS + 8, DS + 0x10, DS + 0x18))
    f_base = parse_splits(f_txt)
    c_fd, i_fd = render(f_base, [p2(arow("u.cpp", DS + 0x10, DS + 0x18, takes_from="z.cpp"))], d_dol, d_syms)
    check("attach: a fold takes a data-only unit's whole range and the unit leaves the candidate",
          (i_fd["issues"], i_fd["dropped"], [u.name for u in c_fd.units], sd(c_fd)["u.cpp"]), ([], ["z.cpp"], ["u.cpp"], [(DS, DS + 8), (DS + 0x10, DS + 0x18)]))
    c_nf, i_nf = render(f_base, [p2(arow("u.cpp", DS + 0x10, DS + 0x18))], d_dol, d_syms)
    check("attach: ... and without `takes_from` the same row is refused", ([i for i in i_nf["issues"] if "from z.cpp" in i] != [], i_nf["dropped"]), (True, []))
    # a name a proposal unit absorbs, a unit a guess merge folded away, and a derived name all resolve to the surviving unit
    c_ab, i_ab = render(d_base, [prop(pu("big2", T, T + 0x40, cut(T, "strong"), absorbs=["big.cpp # trailing comment"])), p2(arow("big.cpp", UN, UN + 4))], d_dol, d_syms)
    check("attach: a row for a baseline unit a proposal unit `absorbs` lands on the absorber",
          (i_ab["issues"], [(u.name, [(a, b) for a, b, _x in u.ranges.get(".sdata2", [])]) for u in c_ab.units if u.ranges.get(".sdata2")]),
          ([], [("m/big2.cpp", [(DS, DS + 8), (UN, UN + 4)])]))
    fold = prop(pu("first", T, T + 0x10), pu("late", T + 0x10, T + 0x20, cut(T + 0x10, "guess")))
    c_gf, i_gf = render(sp, [fold, p2(arow("m/late.cpp", S2 + 0x100, S2 + 0x108), arow("late", S2 + 0x108, S2 + 0x10C))])
    check("attach: a row for a unit a guess cut folded away (by name or derived_name) lands on the unit that absorbed it",
          (i_gf["issues"], [(a, b) for u in c_gf.units if u.name == "m/first.cpp" for a, b, _x in u.ranges.get(".sdata2", [])]), ([], [(S2 + 0x100, S2 + 0x10C)]))
    # unowned_data: checked against the candidate
    un = {"section": ".sdata2", "range": ["0x%X" % UN, "0x%X" % (UN + 4)], "candidates": ["big.cpp"], "reason": "r"}
    check("unowned_data: a deferral a unit owns is a lint line, unless the row says `provisional`",
          ([i for i in render(d_base, [p2(arow("big.cpp", UN, UN + 4), unowned_data=[un])], d_dol, d_syms)[1]["issues"] if "listed as deferred" in i] != [],
           [i for i in render(d_base, [p2(arow("big.cpp", UN, UN + 4), unowned_data=[dict(un, provisional=True)])], d_dol, d_syms)[1]["issues"]]), (True, []))
    prov_row = {"section": ".sdata2", "range": ["0x%X" % (DS + 4), "0x%X" % (DS + 8)], "candidates": ["m/bpiece.cpp", "big.cpp"], "reason": "r"}
    check("unowned_data: a range the by-reader default holds is not 'owned' (the deferral says the default stays)",
          [i for i in render(d_base, [d_prop, p2(unowned_data=[prov_row])], d_dol, d_syms)[1]["issues"]], [])
    check("unowned_data: `candidates` may be empty only with a reason; the range may be `start`/`end`",
          (lint_proposal(p2(unowned_data=[{"section": ".init", "start": "0x10", "end": "0x20", "candidates": []}]), d_base),
           lint_proposal(p2(unowned_data=[{"section": ".init", "start": "0x10", "end": "0x20", "candidates": [], "reason": "linker table"}]), d_base)),
          (["unowned_data[0]: no candidate owners and no reason saying why (a linker-generated range)", "unowned_data[0]: needs a reason (reason, why, evidence or note)"], []))
    # lint of the rows
    bad_rows = p2(dict(arow("big.cpp", UN, UN + 4), reproduce=""), arow("big.cpp", UN, UN + 4, sec=".text"), arow("big.cpp", UN + 8, UN + 8),
                  arow("big.cpp", UN + 8, UN + 12, "medium", evidence=[]), arow("big.cpp", UN + 8, UN + 12, "maybe"), "x", {"section": ".sdata2", "range": ["0x1", "0x2"], "grade": "strong"},
                  {"unit": "big.cpp", "section": ".sdata2", "grade": "strong", "reproduce": "r"})
    e_bad = lint_proposal(bad_rows, d_base)
    check("lint: no reproduce, a code section, an empty range, a medium row without evidence, a bad grade, a non-object, no unit and no range are each an error",
          [any(k in i for i in e_bad) for k in ("no reproduce command", ".text is not a data section", "is empty", "medium attach has no evidence", "grade 'maybe'",
                                                "want an object", "missing unit or text_addr", "range is missing")], [True] * 8)
    check("lint: `extab`/`extabindex` rows are attachments too (a unit's exception tables past the registered range)",
          lint_proposal(p2(arow("big.cpp", 0x8003F18C, 0x8003F198, sec="extabindex")), d_base), [])
    check("lint: a file with units, attach, unowned_data or moves is a proposal; one with none of them is 'no units'",
          (lint_proposal({"phase": 2}, d_base), lint_proposal(p2(), d_base), lint_proposal(p2(arow("big.cpp", UN, UN + 4)), d_base)), (["no units"], ["no units"], []))
    c_au, i_au = render(d_base, [d_prop, p2(arow("big.cpp", UN, UN + 4), arow("m/bpiece.cpp", UN + 8, UN + 12))], d_dol, d_syms)
    check("attach: the units given data are recorded (cmd_proposal lists them with the proposal units, not as new failures)", i_au["attach_units"], ["big.cpp", "m/bpiece.cpp"])
    # a unit without `.text` has a place: `after` anchors it, else it is the last unit (and warns)
    dunit = lambda **more: pu("dtab", 0, 0, **more)
    dunit_ = lambda **more: dict(dunit(**more), ranges={".sdata2": [["0x%X" % UN, "0x%X" % (UN + 8)]]})
    order = lambda c: [u.name for u in c.units]
    c_da, i_da = render(d_base, [prop(pu("bpiece", T + 0x20, T + 0x40, cut(T + 0x20, "strong")), dunit_(after="big.cpp"))], d_dol, d_syms)
    c_dn, i_dn = render(d_base, [prop(pu("bpiece", T + 0x20, T + 0x40, cut(T + 0x20, "strong")), dunit_())], d_dol, d_syms)
    check("data-only unit: `after` puts it right behind the named unit; without it the unit is last and a warning says so",
          (order(c_da), i_da["issues"], i_da["warnings"], order(c_dn), [w for w in i_dn["warnings"] if "no `after` anchor" in w] != []),
          (["big.cpp", "m/dtab.cpp", "m/bpiece.cpp"], [], [], ["big.cpp", "m/bpiece.cpp", "m/dtab.cpp"], True))
    c_db, i_db = render(d_base, [prop(dunit_(after="m/nope.cpp"))], d_dol, d_syms)
    check("data-only unit: an `after` naming no unit is a lint line", [i for i in i_db["issues"] if "which is not a unit of the candidate" in i] != [], True)
    c_d2, _i = render(d_base, [prop(dunit_(after="big.cpp"), dict(dunit_(after="big.cpp"), derived_name="dtab2", ranges={".sdata2": [["0x%X" % (UN + 8), "0x%X" % (UN + 12)]]}))], d_dol, d_syms)
    check("data-only unit: several units behind one anchor keep the file order", order(c_d2), ["big.cpp", "m/dtab.cpp", "m/dtab2.cpp"])
    c_mv, i_mv = render(d_base, [d_prop, p2(moves=[{"unit": "big.cpp", "after": "m/bpiece.cpp"}])], d_dol, d_syms)
    check("moves: a registered unit is placed behind another (the candidate order is the link order)", (order(c_mv), i_mv["moved"], i_mv["issues"]),
          (["m/bpiece.cpp", "big.cpp"], ["big.cpp after m/bpiece.cpp"], []))
    check("moves: a name the candidate lacks is a lint line", [i for i in render(d_base, [p2(moves=[{"unit": "big.cpp", "after": "nope.cpp"}])], d_dol, d_syms)[1]["issues"]
                                                                if "moves:" in i] != [], True)

    # checker rules the phase 2 attachments exposed
    # pool: a unit claiming `.sdata` strings and `.sdata2` literals has two pools; first-use order is judged inside each, never across
    SD = 0x80200100
    two_pools = hdr + "p.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata      start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n" % (T, T + 0x20, SD, SD + 8, S2, S2 + 4)
    words_sd = [_lis(3, hi), _w(48, 1, 3, 0), BLR, NOP, _lis(3, SD >> 16), _w(14, 3, 3, SD & 0xFFFF), BLR, NOP]      # f0 loads the literal, the later f1 takes the string's address
    cps, _s = _mini(T, words_sd, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], two_pools, [(SD, b"hello\0\0\0"), (S2, struct.pack(">f", 1.5))],
                    ["str0 = .sdata:0x%X; // type:object size:0x8 scope:local data:string" % SD, s2_syms[0]])
    check("pool order: a string of `.sdata` first used after a `.sdata2` literal that sits above it is two pools, not one inverted pool",
          run_checks(cps, None, ["pool"]).units["p.cpp"]["pool"]["status"], PASS)
    both_inv = two_pools.replace("end:0x%X\n" % (S2 + 4), "end:0x%X\n" % (S2 + 8))
    words_inv = [_lis(3, hi), _w(48, 2, 3, 4), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 0), BLR, NOP]                         # `.sdata2`: l1 first used before l0
    cpi, _s = _mini(T, words_inv, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], both_inv, [(SD, b"hello\0\0\0"), (S2, lits2)],
                    ["str0 = .sdata:0x%X; // type:object size:0x8 scope:local data:string" % SD] + s2_syms)
    check("pool order: an inversion inside one section is still a FAIL, reported at the literal that breaks the order", (lambda r: (r["status"], "first use of l1" in r["finding"]))(
          run_checks(cpi, None, ["pool"]).units["p.cpp"]["pool"]), (FAIL, True))
    # data-order: a strong V->S row asserts a boundary in [first string, next vtable): a unit ending inside the gap can end at it
    for label, dend, want in (("the unit ends where the second vtable group starts", D0 + 20, PASS), ("the unit ends inside the string gap", D0 + 16, PASS),
                              ("the unit holds the second vtable group too", D0 + 32, FAIL)):
        vu3 = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, dend)
        cv3, sv3 = _mini(T, [NOP] * 12, fns_v, vu3, [(D0, vblob)], vsyms)
        check("data-order V->S: %s -> %s" % (label, want), run_checks(cv3, dataorder_rows(sv3), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # data-order: a zigzag pair (adjacent vtables whose first slots go up) whose vtable the unit's own sinit closure stores is instantiated, not a seam
    zblob = struct.pack(">3I", 0, 0, T) + struct.pack(">3I", 0, 0, T + 4)
    zsyms = ["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0, "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 12)]
    zunits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, D0 + 24)
    zstore = [_lis(3, (D0 + 12) >> 16), _w(14, 3, 3, (D0 + 12) & 0xFFFF), BLR, NOP]                    # a ctor storing the address of __vt__B
    for label, sinit_word, want in (("the sinit calls a ctor that stores __vt__B", _b(T + 0x10, T + 0x20), PASS),
                                    ("the store is in a function the sinit never reaches", BLR, FAIL)):
        czz, szz = _mini(T, [NOP, NOP, NOP, BLR, sinit_word, NOP, NOP, NOP] + zstore + [NOP] * 4, fns_v, zunits, [(0x80200000, struct.pack(">I", T + 0x10)), (D0, zblob)], zsyms)
        check("data-order zigzag: %s -> %s" % (label, want), run_checks(czz, dataorder_rows(szz), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # data-order: a zigzag pair whose classes' member functions alternate in the text is one TU (no text cut separates them); an ordered pair is a seam
    ffns = [("g%d" % i, 0x10 * i, 0x10) for i in range(6)]
    funits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x60, D0, D0 + 32)
    fsyms = ["__vt__A = .data:0x%X; // type:object size:0x10 scope:global" % D0, "__vt__B = .data:0x%X; // type:object size:0x10 scope:global" % (D0 + 16)]
    for label, aslots, bslots, want in (("alternating members", (T + 0x10, T + 0x30), (T + 0x20, T + 0x40), PASS),
                                        ("A's members all before B's", (T + 0x00, T + 0x10), (T + 0x20, T + 0x30), FAIL)):
        cfz, sfz = _mini(T, [NOP] * 24, ffns, funits, [(D0, struct.pack(">4I", 0, 0, *aslots) + struct.pack(">4I", 0, 0, *bslots))], fsyms)
        check("data-order zigzag: %s -> %s" % (label, want), run_checks(cfz, dataorder_rows(sfz), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # jumptable: the foreign reader a finding names is the first by name (a set of units has no order of its own)
    JT = 0x80500000
    jt_units = "Sections:\n\t.text       type:code align:32\n\t.data       type:data align:8\n\nown.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n\n" % (T, T + 0x10, JT, JT + 8)
    names8 = ["r%d.cpp" % k for k in (5, 2, 7, 0, 3, 6, 1, 4)]
    jt_units += "".join("%s:\n\t.text       start:0x%X end:0x%X\n\n" % (n, T + 0x10 * (k + 1), T + 0x10 * (k + 2)) for k, n in enumerate(names8))
    jt_reader = [_lis(3, JT >> 16), _w(14, 3, 3, JT & 0xFFFF), BLR, NOP]
    cjt, _s = _mini(T, [NOP] * 4 + jt_reader * 8, [("own", 0, 0x10)] + [("f%d" % k, 0x10 * (k + 1), 0x10) for k in range(8)], jt_units, [(JT, bytes(8))],
                    ["jumptable_80500000 = .data:0x%X; // type:object size:0x8 scope:local" % JT])
    check("jumptable: the finding names the foreign reader first by name, not first in a set", run_checks(cjt, None, ["jumptable"]).units["own.cpp"]["jumptable"]["finding"],
          "jumptable_80500000 is read by r0.cpp, not by this unit")

    # --readers: one line per map symbol of a range with its candidate owner and the units that read it
    rctx = Ctx(c_sh, d_syms, d_dol, 0x80500000, 0x80600000)
    check("--readers: owner and decoded readers of every symbol in the range (A reads l0, B reads l1)",
          [" ".join(l.split()) for l in readers_report(rctx, ".sdata2:0x%X-0x%X" % (DS, DS + 8))],
          [".sdata2 0x%08X l0 size 0x4 owner big.cpp readers big.cpp x1" % DS, ".sdata2 0x%08X l1 size 0x4 owner big.cpp readers big.cpp x1" % (DS + 4)])

    # ranking
    td = top_defects(ctx, res, 50)
    check("defects are ranked by score", [d["score"] for d in td] == sorted((d["score"] for d in td), reverse=True) and bool(td), True)
    print("\n%d checks, %d failed" % (NCHECKS[0], len(fails)))
    return 1 if fails else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--baseline", action="store_true", help="check the current splits.txt")
    ap.add_argument("--proposal", action="append", help="proposal file (repeatable); render + check the candidate")
    ap.add_argument("--emit-splits", help="with --proposal: write the candidate splits.txt here")
    ap.add_argument("--data-by-reader", action=argparse.BooleanOptionalAction, default=True,
                    help="with --proposal: assign the data of a recut registered unit to the pieces by reader (PROVISIONAL phase-1 default; default on)")
    ap.add_argument("--splits"), ap.add_argument("--symbols"), ap.add_argument("--dol")
    ap.add_argument("--outbox", help="lane outbox with seam requests (default: the primary checkout's .pi/outbox)")
    ap.add_argument("--readers", action="append", metavar="SEC:START-END",
                    help="with --proposal: print each map symbol of that data range with its candidate owner and the units that read it (repeatable)")
    ap.add_argument("--only", help="comma list of invariants")
    ap.add_argument("--unit", help="regex: report only the units whose name matches (--baseline); with --proposal, the `detail`/`pooldup` lines of the candidate's matching units")
    ap.add_argument("--json", help="write the machine-readable report here")
    ap.add_argument("--all", action="store_true", help="list every unit, not only the failing ones")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--intervals", action="store_true",
                    help="with --baseline: print each pool value held at two addresses (read by --unit's units) with the interval a TU starts in")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.proposal:
        return cmd_proposal(args)
    if args.baseline:
        return cmd_baseline(args)
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
