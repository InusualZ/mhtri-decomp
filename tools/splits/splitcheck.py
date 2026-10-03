#!/usr/bin/env python3
"""Read-only audit of `splits.txt`: twelve invariants per unit against the retail DOL and the map.
Spec: docs/tools/spec/splitcheck.md. CLI: splitcheck.py --baseline [--json F] [--all] [--only INV]
[--unit REGEX] [--intervals] | --readers SEC:START-END | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import bisect
import collections
import json
import os
import re
import struct
import subprocess
import sys

from tools.lib.binary.dol import Dol as LibDol

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
for _p in (TOOLS, HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from tools.lib import project as _project  # noqa: E402  (the one splits / map parser)

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
#: a function that forms this many distinct section starts with `addi @l` is start-up / module-loader code taking `_f_<section>` linker symbols (the
#: RSO loader forms nine, `__init_data` two: those two stay ordinary references - the first symbol of a section can be read for real)
LINKER_STARTS = 3


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
    """This tool's `Splits`/`Unit` view of `lib.project.Splits.parse(text)` (a range's attrs keep their space)."""
    parsed = _project.Splits.parse(text)
    units = []
    for block in parsed.blocks:
        unit = Unit(block.unit, block.attrs)
        for r in block.ranges:
            unit.ranges.setdefault(r.section, []).append((r.start, r.end, (" " + r.attrs) if r.attrs else ""))
        units.append(unit)
    return Splits(list(parsed.header), units)


# ---- symbols.txt and the DOL ------------------------------------------------------------------------------------------

def parse_symbols(lines):
    """Map rows as dicts (`name section addr size type scope kind`); the map is streamed, never printed."""
    out = []
    for ln in lines:
        e = _project.parse_line(ln.rstrip("\n"))
        if e is not None:
            out.append({"name": e.name, "section": e.section, "addr": e.address, "size": e.size, "type": e.type,
                        "scope": e.scope, "kind": e.kind})
    return out


class Dol(LibDol):
    """Address -> bytes of the retail image through the DOL section table (`lib.binary.dol`)."""

    def __init__(self, data):
        super().__init__(data)
        self.secs = [(s.address, s.size, s.offset) for s in self.segments]

    def read(self, addr, n):
        return self.bytes_at(addr, n)


# ---- instruction decoding: who reads which data address (lib.ppc: the one decoder and reference scanner) -------------

from tools.lib.ppc import (LOADS_INT, STORES, FP_MEM, LOAD_OPS, STORE_OPS, UPDATE_OPS, VOLATILE,  # noqa: E402,F401
                           X_ARITH, X_LOADS, X_LOGIC, LIS_WINDOW, written_reg, scan_refs, scan_calls)
from tools.lib import ppc as _ppc  # noqa: E402
from tools.lib import refs as _refs  # noqa: E402


def find_sda_bases(dol, code_words):
    """`(r13, r2)` from the start-up code's `lis`/`ori|addi` pairs (`lib.ppc.find_sda_bases`; `dol` is unused)."""
    return _ppc.find_sda_bases(code_words)
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
        self.store_refs = {}                                  # data symbol index -> [sites that STORE through the address]
        self.pass_refs = {}                                   # data symbol index -> [(site, register, callee)] of the calls that pass its address
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

    def section_of(self, addr):
        """The map section of the data symbol that holds `addr` (`None` when none does)."""
        _i, t = self.data_sym_at(addr)
        return t["section"] if t is not None else None

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
        found = _refs.text_refs(chunks, self.sda13, self.sda2, is_data, self._fn_starts,
                                lambda site, t: self.fn_at(site) is self.fn_at(t))
        raw, rawl, raws, rawp = found.refs, found.loads, found.stores, found.passes
        for site, t, kind in found.fn_edges:
            self._add_fn_ref(site, t, kind)
        ignore = self.linker_operand_sites(raw)
        for t, sites in raw.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.refs.setdefault(i, []).extend(x for x in sites if x not in ignore)
        for t, sites in rawl.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.load_refs.setdefault(i, []).extend(x for x in sites if x not in ignore)
        for t, sites in raws.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.store_refs.setdefault(i, []).extend(x for x in sites if x not in ignore)
        for t, sites in rawp.items():
            i, _s = self.data_sym_at(t)
            if i is not None:
                self.pass_refs.setdefault(i, []).extend(x for x in sites if x[0] not in ignore)
        self.scanned = True

    def linker_operand_sites(self, raw):
        """The sites whose operand is a linker symbol, not a map symbol: `_f_<section>` is the start of a section, so the `lis/addi` that forms one
        lands on the section's first byte.  Module-loader code (`RSOStaticLocateObject`) takes nine in one function (`_f_sbss2`, `_f_sdata2`, `_f_sbss`,
        ...): in a function that forms `LINKER_STARTS` or more distinct section starts with `addi`/`ori` every reference to them is a linker operand,
        not a read of the first symbol of the section (the owner of that symbol is whoever defines it)."""
        edges = set()                                # the start of every data section: the map's first symbol, the first owned range
        for sec in SECTION_ORDER:
            ext = self.section_extent(sec)
            if ext:
                edges.add(ext[0])
        for sec in set(s["section"] for s in self.data_syms):
            edges.add(min(s["addr"] for s in self.data_syms if s["section"] == sec))
        per_fn = collections.defaultdict(dict)
        for t in edges:
            for site in raw.get(t, ()):
                if (self.dol.word(site) >> 16) & 31 in (2, 13):      # a small-data access is `sda21`, never a `@l` operand
                    continue
                f = self.fn_at(site)
                per_fn[f["addr"] if f is not None else None].setdefault(t, []).append(site)
        out = set()
        for fa, by_t in per_fn.items():
            formed = [t for t, sites in by_t.items() if any(self.dol.word(x) >> 26 in (14, 24) for x in sites)]         # an `addi`/`ori @l` forms the address
            if fa is not None and len(formed) >= LINKER_STARTS:
                for sites in by_t.values():
                    out.update(sites)
        return out

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

    def sinit_definers(self):
        """`{data symbol index: {unit name: [site, ...]}}` - the globals a unit's OWN `__sinit` constructs, so the unit defines them.

        The `__sinit` is the function a `.ctors` word of the unit points at, inside the unit's own text.  Within it a global counts when it is
        WRITTEN (a store through its address: `stw r0, off(r13)`, `lis/stw`, a register an `addi` formed), when it is the `this` of a constructor
        call (r3 of a `bl` made while r3 holds its address), or when it is passed to `__register_global_object` (r3 the object, r5 the registration
        node) or `__construct_array` (r3).  Taking an address is not enough: a vtable another unit owns is only the VALUE stored into a word.
        A string literal or a pool constant passed to a call is not constructed (an argument, not an object)."""
        if getattr(self, "_sinit_definers", None) is not None:
            return self._sinit_definers
        out = collections.defaultdict(lambda: collections.defaultdict(list))
        reg_obj = {f["addr"] for f in self.fns if f["name"] == "__register_global_object"}
        spans = []
        for u in self.units:
            for s, e, _a in u.ranges.get(".ctors", []):
                for a in range(s, e - 3, 4):
                    w = self.dol.word(a)
                    f = self.fn_at(w) if w else None
                    if f is not None and f["addr"] == w and self.text_owner(w) is u and f["name"] not in CRT_CHAIN:
                        spans.append((f["addr"], f["addr"] + (f["size"] or 4), u))
        spans.sort(key=lambda t: t[0])
        starts = [t[0] for t in spans]

        def span_of(site):
            k = bisect.bisect_right(starts, site) - 1
            return spans[k] if k >= 0 and site < spans[k][1] else None

        for i, sites in self.store_refs.items():
            for x in sites:
                sp = span_of(x)
                if sp is not None:
                    out[i][sp[2].name].append(x)
        for i, sites in self.pass_refs.items():
            sym = self.data_syms[i]
            if is_literal(sym) or sym["kind"] == "string" or sym["section"] in (".rodata", ".sdata2"):
                continue
            for x, reg, callee in sites:
                sp = span_of(x)
                if sp is None:
                    continue
                if reg == 3 or (reg == 5 and callee in reg_obj):
                    out[i][sp[2].name].append(x)
        self._sinit_definers = {i: {n: sorted(set(v)) for n, v in d.items()} for i, d in out.items()}
        return self._sinit_definers

    def string_init_words(self):
        """`{string address: [word address, ...]}` - the aligned words of `.data`/`.rodata`/`.sdata` that point at a `.sdata` string: the pointer
        initialisers of a file-scope table (`const char* names[] = {"a", "b"}`).  MWCC emits those strings before the strings the functions use."""
        if getattr(self, "_str_inits", None) is None:
            out = collections.defaultdict(list)
            for s in self.data_syms:
                if s["section"] not in (".data", ".rodata", ".sdata") or not s["size"]:
                    continue
                for i in range(s["size"] // 4):
                    a = s["addr"] + 4 * i
                    w = self.dol.word(a)
                    if a % 4 or not w or w < 0x80500000:
                        continue
                    _j, t = self.data_sym_at(w)
                    if t is not None and t is not s and t["section"] == ".sdata" and is_literal(t):
                        out[t["addr"]].append(a)
            self._str_inits = dict(out)
        return self._str_inits

    def definers(self, sym):
        """`{unit name: [site, ...]}` of the units whose own `__sinit` constructs this map row (by identity), `{}` when none does."""
        i, _ = self.data_sym_at(sym["addr"])
        return self.sinit_definers().get(i, {}) if i is not None and self.data_syms[i] is sym else {}

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
    for _sec, name, addr, finding, _before in file_order_offenders(ctx):
        res.add(name, "order", FAIL, addr, finding)
    for u in ctx.units:
        res.add(u.name, "order", PASS)


def longest_nondecreasing(seq):
    """Indices of one longest non-decreasing subsequence of `seq` (patience sorting; the later of two equal-length candidates wins, deterministic)."""
    tails, tail_idx, parent = [], [], [None] * len(seq)
    for i, x in enumerate(seq):
        k = bisect.bisect_right(tails, x)
        if k == len(tails):
            tails.append(x)
            tail_idx.append(i)
        else:
            tails[k], tail_idx[k] = x, i
        parent[i] = tail_idx[k - 1] if k else None
    out = []
    i = tail_idx[-1] if tail_idx else None
    while i is not None:
        out.append(i)
        i = parent[i]
    return out[::-1]


def file_order_offenders(ctx):
    """`[(section, unit name, address, finding, before)]`: the ranges whose unit is out of link order (`before`: the in-order unit whose range precedes it
    by address, `None` at the section start).  The linker places the units of one section in FILE order
    (`dtk` reads `splits.txt` top to bottom; a data-only unit has no text to anchor it, so its file position is the only thing that orders it), so the
    ranges of every section, taken by address, must have non-decreasing file positions.  A longest-non-decreasing-subsequence picks the ranges that
    are in order; each range outside it is out of link order (the fewest ranges that have to move).  A `rename:` range is ordered by the linker script."""
    pos = {u.name: k for k, u in enumerate(ctx.units)}
    out = []
    for sec in sorted(ctx.sec_index):
        seq = []
        for s, e, u in ctx.sec_index[sec]:
            if "rename:" in next((a for x, _y, a in u.ranges[sec] if x == s), "") or "common" in next((a for x, _y, a in u.ranges[sec] if x == s), ""):
                continue
            seq.append((s, e, u))
        keep = set(longest_nondecreasing([pos[u.name] for _s, _e, u in seq]))
        for i, (s, e, u) in enumerate(seq):
            if i in keep:
                continue
            before = next((seq[j] for j in range(i - 1, -1, -1) if j in keep), None)
            after = next((seq[j] for j in range(i + 1, len(seq)) if j in keep), None)
            out.append((sec, u.name, s, "%s range %s..%s is out of link order: by address it lies between %s and %s, but %s is at file position %d (%s at %d, %s at %d)"
                        % (sec, hx(s), hx(e), before[2].name if before else "the section start", after[2].name if after else "the section end", u.name,
                           pos[u.name], before[2].name if before else "-", pos[before[2].name] if before else -1, after[2].name if after else "-",
                           pos[after[2].name] if after else -1), before[2].name if before else None))
    return out


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


def pool_first_use(ctx, sym, u, own=None, held=None):
    """`(order key, site)` of the first use of a pool literal by unit `u`, or `None` when `u` does not use it.

    A string a pointer initialiser names (a `.data`/`.rodata`/`.sdata` word of `u`'s own ranges pointing at it: the initialiser of a file-scope
    table) is emitted BEFORE the strings the functions use, so its first use sorts first (`(0, 0)`: initialisers carry no order among
    themselves, like the literals of one function); any other use is ordered by the function that makes it (`(1, function start)`).
    `held(word address)` says whether a table word is `u`'s when the table is not in `u`'s ranges yet (a phase 2 decision)."""
    if sym["section"] == ".sdata":
        mine = held if held is not None else (lambda a: ctx.owner(ctx.section_of(a), a) is u)
        inits = [a for a in ctx.string_init_words().get(sym["addr"], ()) if mine(a)]
        if inits:
            return (0, 0), min(inits)
    if own is None:
        own = [x for x in ctx.literal_readers(sym) if ctx.text_owner(x) is u]
    if not own:
        return None
    first = min(own)
    return (1, _first_use_fn(ctx, first)), first


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
                use = pool_first_use(ctx, sym, u, own)
                if witness:
                    v = ctx.dol.read(sym["addr"], sym["size"])
                    key = (sym["size"], v)
                    if v is not None and key in seen:
                        res.add(u.name, "pool", FAIL, sym["addr"], "claimed pool holds value 0x%s at %s and again at %s (one entry per value per TU)"
                                % (v.hex(), hx(seen[key]), hx(sym["addr"])))
                    elif v is not None:
                        seen[key] = sym["addr"]
                if use is not None:
                    own_first.append((sym["addr"], use[1], sym["name"], use[0], sym["section"]))
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


def seams_inside(ctx, u, s, e, syms, found, by_addr, closure=None):
    """`(seams, instantiated)`: the strong `.data` seams that cut the range `[s, e)` of unit `u` - the rule `check_data_order` applies, one implementation.

    A V->S row says a boundary lies in `[addr, latest)`: a range that ends inside that gap (`latest >= e`) can have it at its own end, so only one that
    also holds the later vtable group crosses the seam; a zigzag cuts at `addr`.  Two classes whose member functions alternate in the text are one TU
    (a zigzag between them is not an edge), and an instantiated vtable - a deferred constructor of this TU stores it (`closure`, the unit's `__sinit`
    closure) - is no TU edge: the later vtable of a V->S seam, the second of a zigzag pair, or the vtable before either."""
    import dataorder as do
    inside = [x for x in found if x["kind"] in do.STRONG_KINDS and s < x["addr"] < e and (x["kind"] != "V->S" or x["latest"] < e)]
    inside = [x for x in inside if not (x["kind"] == "zigzag" and zigzag_interleaved(ctx, syms, by_addr, x))]
    kept, inst = [], []
    for x in inside:
        if closure and any(_vtable_stored_in(ctx, by_addr.get(a_), closure) for a_ in (x.get("latest") or x["addr"], _vtable_before(syms, x))):
            inst.append({"unit": u.name, "addr": x["addr"], "vtable": x["before"]})
        else:
            kept.append(x)
    return kept, inst


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
            inside, inst = seams_inside(ctx, u, s, e, syms, found, by_addr, unit_sinit_closure(ctx, u))
            ctx.seams_instantiated += inst
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


#: how far past the instruction that forms a jump table's address the dispatch (`lwzx` ... `mtctr` ... `bctr`) may lie
JUMP_WINDOW = 16
BCTR = 0x4E800420


def jumptable_reader_sites(ctx, sym):
    """The sites that READ a jump table: the `addi`/`ori` that forms its address followed, within `JUMP_WINDOW` instructions, by the indexed load
    (`lwzx`/`lwzux` through that register), then `mtctr` and `bctr`.  Any other reference is not a read of the table: a displacement load whose address
    happens to fall inside the symbol (`lhz r0, 8(r3)` through a base that was formed with `lis`) is a struct-field access, not a dispatch."""
    out = []
    for x in ctx.readers(sym):
        w = ctx.dol.word(x)
        if w is None:
            continue
        op = w >> 26
        if op == 14:
            reg = (w >> 21) & 31                              # addi rD, rA, lo
        elif op == 24:
            reg = (w >> 16) & 31                              # ori rA, rS, lo
        else:
            continue
        loaded = False
        for k in range(1, JUMP_WINDOW + 1):
            v = ctx.dol.word(x + 4 * k)
            if v is None:
                break
            if v >> 26 == 31 and (v >> 1) & 0x3FF in (23, 55) and reg in ((v >> 16) & 31, (v >> 11) & 31):
                loaded = True
                continue
            if loaded and v >> 26 == 31 and (v >> 1) & 0x3FF == 467 and (v >> 11) & 0x3FF == 0x120:       # mtctr
                if any(ctx.dol.word(x + 4 * (k + j)) == BCTR for j in range(1, 8)):
                    out.append(x)
                break
    return out


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
        rsites = jumptable_reader_sites(ctx, s) if s["name"].startswith("jumptable_") else ctx.readers(s)
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


def cmd_readers(args):
    splits, symbols, dol = load_ctx(args.splits, args.symbols, args.dol)
    ctx = Ctx(splits, symbols, dol)
    for spec in args.readers:
        for line in readers_report(ctx, spec):
            print(line)
    return 0


def readers_report(ctx, spec):
    """One line per map symbol of `spec`'s section range: its owner unit, then the units whose decoded text reads it
    (`literal_readers` for a pool literal) with the site counts."""
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

    # checker rules for data a unit holds
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
    jt_units += "".join("%s:\n\t.text       start:0x%X end:0x%X\n\n" % (n, T + 0x20 * (k + 1) - 0x10, T + 0x20 * (k + 2) - 0x10) for k, n in enumerate(names8))
    jt_reader = [_lis(3, JT >> 16), _w(14, 3, 3, JT & 0xFFFF), 0x5480103A, (31 << 26) | (3 << 16) | (23 << 1), 0x7C0903A6, 0x4E800420, NOP, NOP]       # the dispatch
    cjt, _s = _mini(T, [NOP] * 4 + jt_reader * 8, [("own", 0, 0x10)] + [("f%d" % k, 0x10 + 0x20 * k, 0x20) for k in range(8)], jt_units, [(JT, bytes(8))],
                    ["jumptable_80500000 = .data:0x%X; // type:object size:0x8 scope:local" % JT])
    check("jumptable: the finding names the foreign reader first by name, not first in a set", run_checks(cjt, None, ["jumptable"]).units["own.cpp"]["jumptable"]["finding"],
          "jumptable_80500000 is read by r0.cpp, not by this unit")

    # --readers: one line per map symbol of a range with its owner and the units that read it
    DS = 0x80300000
    wa = [_lis(3, DS >> 16), _w(48, 1, 3, 0), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn A (0x00..0x20) reads l0
    wb = [_lis(3, DS >> 16), _w(48, 1, 3, 4), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn B (0x20..0x40) reads l1
    r_txt = hdr + "big.cpp:" + chr(10) + "	.text       start:0x%X end:0x%X" % (T, T + 0x40) + chr(10) + "	.sdata2     start:0x%X end:0x%X" % (DS, DS + 8) + chr(10)
    rctx, _rs = _mini(T, wa + wb, [("A", 0, 0x20), ("B", 0x20, 0x20)], r_txt, [(DS, struct.pack(">ff", 1.5, 2.5))],
                      ["l0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % DS,
                       "l1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (DS + 4)])
    check("--readers: owner and decoded readers of every symbol in the range (A reads l0, B reads l1)",
          [" ".join(l.split()) for l in readers_report(rctx, ".sdata2:0x%X-0x%X" % (DS, DS + 8))],
          [".sdata2 0x%08X l0 size 0x4 owner big.cpp readers big.cpp x1" % DS, ".sdata2 0x%08X l1 size 0x4 owner big.cpp readers big.cpp x1" % (DS + 4)])

    # ---- phase 3 review fixes: the decoder, the `__sinit` definers, the pool's initialiser order, the file-order check, the jump-table dispatch ----
    TT = 0x80100000
    HDR = "Sections:\n\t.text       type:code align:32\n\t.ctors      type:rodata align:4\n\t.data       type:data align:8\n\t.sdata      type:data align:8\n\t.sdata2     type:rodata align:4\n\t.sbss2      type:bss align:4\n\n"

    def ADDI(rt, ra, imm):
        return _w(14, rt, ra, imm)

    def STW(rs, ra, d):
        return _w(36, rs, ra, d)

    def LHZ(rt, ra, d):
        return _w(40, rt, ra, d)

    def X31(rt, ra, rb, xo):
        return (31 << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (xo << 1)

    def MR(ra, rs):
        return X31(rs, ra, rs, 444)                              # or rA, rS, rS

    def ADD(rt, ra, rb):
        return X31(rt, ra, rb, 266)

    def LWZX(rt, ra, rb):
        return X31(rt, ra, rb, 23)

    MTCTR_R0, BCTR_W, SLWI_R0_R4_2 = 0x7C0903A6, 0x4E800420, 0x5480103A

    def dsym(name, sec, addr, size=4, extra="", scope="global"):
        return "%s = %s:0x%X; // type:object size:0x%X scope:%s%s" % (name, sec, addr, size, scope, extra)

    def unit_text(*blocks):
        return HDR + "".join("%s:\n%s\n" % (nm, "".join("\t%-11s start:0x%X end:0x%X\n" % (sec, a, b) for sec, a, b in rr)) for nm, rr in blocks)

    # decoder (rule 3): a `lis` in a callee-saved register outlives the 200-instruction window, until the register is written; `mr` copies it
    DD = 0x80580000
    long_fn = [_lis(18, DD >> 16)] + [NOP] * 250 + [ADDI(24, 18, DD & 0xFFFF), BLR]                       # r18 is callee-saved: still live at the addi
    vol_fn = [_lis(5, DD >> 16)] + [NOP] * 250 + [ADDI(6, 5, DD & 0xFFFF), BLR]                           # r5 is volatile: the window ended
    mr_fn = [_lis(5, DD >> 16), MR(18, 5), ADDI(24, 18, DD & 0xFFFF), BLR]                                 # mr r18, r5 copies the lis
    nomr_fn = [_lis(5, DD >> 16), NOP, ADDI(24, 18, DD & 0xFFFF), BLR]                                     # r18 never held it
    dead_fn = [_lis(18, DD >> 16), ADD(18, 4, 5), ADDI(24, 18, DD & 0xFFFF), BLR]                          # add r18, r4, r5 overwrote the lis
    long_ori = [_lis(18, DD >> 16)] + [NOP] * 250 + [_w(24, 18, 24, DD & 0xFFFF), BLR]                      # ori r24, r18, lo: the same window rule
    words, fns, off = [], [], 0
    for nm, w in (("long", long_fn), ("vol", vol_fn), ("mr", mr_fn), ("nomr", nomr_fn), ("dead", dead_fn), ("long_ori", long_ori)):
        w = w + [NOP] * (-len(w) % 4)
        fns.append((nm, off, 4 * len(w)))
        words += w
        off += 4 * len(w)
    cdec, _sd = _mini(TT, words, fns, unit_text(("u.cpp", [(".text", TT, TT + off)])), [(DD, bytes(8))], [dsym("g", ".data", DD, 8)])
    gi, _gs = cdec.data_sym_at(DD)
    fn_of = lambda c, x: c.fn_at(x)["name"]
    check("decode: a lis in a callee-saved register outlives the 200-instruction window; a volatile one does not; mr copies it; an overwrite kills it",
          [fn_of(cdec, x) for x in sorted(cdec.refs.get(gi, []))], ["long", "mr", "long_ori"])

    # decoder (rule 3): a function that forms two section starts (`_f_sbss2`, `_f_sdata2`: start-up / module-loader code) reads none of the first symbols
    SB2, SD2, SD1 = 0x80594000, 0x80596000, 0x80592000
    lk = [_lis(30, SB2 >> 16), _lis(29, SD2 >> 16), _lis(28, SD1 >> 16), ADDI(30, 30, SB2 & 0xFFFF), ADDI(29, 29, SD2 & 0xFFFF), ADDI(28, 28, SD1 & 0xFFFF), BLR, NOP]
    one = [_lis(3, SB2 >> 16), ADDI(3, 3, SB2 & 0xFFFF), BLR, NOP]                                          # one section start: an ordinary reference
    two = [_lis(3, SB2 >> 16), _lis(4, SD2 >> 16), ADDI(3, 3, SB2 & 0xFFFF), ADDI(4, 4, SD2 & 0xFFFF)]       # two: below the threshold, both stay
    clk, _sl = _mini(TT, lk + one + two, [("start", 0, 0x20), ("one", 0x20, 0x10), ("two", 0x30, 0x10)], unit_text(("u.cpp", [(".text", TT, TT + 0x40)])),
                     [(SB2, bytes(8)), (SD2, bytes(8)), (SD1, bytes(8))],
                     [dsym("sb2_first", ".sbss2", SB2, 8), dsym("sd2_first", ".sdata2", SD2, 8), dsym("sd_first", ".sdata", SD1, 8)])
    check("decode: the section starts a module loader forms (three or more: _f_sbss2, _f_sdata2, _f_sdata) are linker operands, not reads; a function with fewer keeps them",
          sorted((s["name"], sorted(fn_of(clk, x) for x in clk.refs.get(i, []))) for i, s in enumerate(clk.data_syms) if s["name"] in ("sb2_first", "sd2_first", "sd_first")),
          [("sb2_first", ["one", "two"]), ("sd2_first", ["two"]), ("sd_first", [])])

    # decoder: stores and the calls that pass an address; the `__sinit` definers (rule 1)
    CT = 0x80200000
    A1, A2, A3, A4, A5, AW, AV = (0x80580000 + 0x20 * k for k in range(7))
    sinit = [_lis(4, A1 >> 16), STW(0, 4, A1 & 0xFFFF),                                                    # 0x00 a store into g1 defines it
             _lis(3, A2 >> 16), ADDI(3, 3, A2 & 0xFFFF), _b(TT + 0x10, TT + 0x80, True),                   # 0x08 ctor(this = &g2)
             _lis(3, A3 >> 16), ADDI(3, 3, A3 & 0xFFFF), _lis(5, A4 >> 16), ADDI(5, 5, A4 & 0xFFFF),       # 0x14 __register_global_object(&g3, dtor, &g4)
             _b(TT + 0x24, TT + 0xA0, True),
             _lis(4, AV >> 16), ADDI(4, 4, AV & 0xFFFF), _lis(5, AW >> 16), STW(4, 5, AW & 0xFFFF),        # 0x28 the vtable is only the VALUE stored into a word
             _lis(3, A5 >> 16), ADDI(3, 3, A5 & 0xFFFF), _b(TT + 0x40, TT + 0x80)]                          # 0x38 lis/addi r3; b ctor (a tail call)
    words = sinit + [NOP] * (0x80 // 4 - len(sinit)) + [BLR] + [NOP] * 7 + [BLR] + [NOP] * 3
    cdf, _s2 = _mini(TT, words, [("sinit_a", 0, 0x80), ("ctor", 0x80, 0x20), ("__register_global_object", 0xA0, 0x10)],
                     unit_text(("a.cpp", [(".text", TT, TT + 0xB0), (".ctors", CT, CT + 4)])), [(CT, struct.pack(">I", TT)), (0x80580000, bytes(0x100))],
                     [dsym("g1", ".bss", A1), dsym("g2", ".bss", A2), dsym("g3", ".bss", A3), dsym("g4", ".bss", A4), dsym("g5", ".bss", A5), dsym("word", ".sbss", AW),
                      dsym("vt", ".data", AV, 0x10)])
    got = {s["name"]: sorted(cdf.definers(s)) for s in cdf.data_syms if s["name"] in ("g1", "g2", "g3", "g4", "g5", "word", "vt")}
    check("sinit definers: a store, a ctor's this, __register_global_object's object and node, a tail-called ctor's this define; a vtable stored as a value does not",
          got, {"g1": ["a.cpp"], "g2": ["a.cpp"], "g3": ["a.cpp"], "g4": ["a.cpp"], "g5": ["a.cpp"], "word": ["a.cpp"], "vt": []})
    check("decode: the passes record the site, the register and the callee (r3 = &g2 into the ctor at +0x80)",
          [(hex(x), r, hex(c)) for x, r, c in cdf.pass_refs[cdf.data_sym_at(A2)[0]]], [(hex(TT + 0x10), 3, hex(TT + 0x80))])

    SS = 0x80790100
    sb = [_lis(3, SS >> 16), ADDI(3, 3, SS & 0xFFFF), _b(TT + 8, TT + 0x10, True), BLR]                      # sinit: bl f(&str) - a string is an argument
    cstr, _cs = _mini(TT, sb + [BLR, NOP, NOP, NOP], [("sb_sinit", 0, 0x10), ("sb_ctor", 0x10, 0x10)],
                      unit_text(("s.cpp", [(".text", TT, TT + 0x20), (".ctors", CT, CT + 4)])), [(CT, struct.pack(">I", TT)), (SS, bytes(8))],
                      [dsym("str", ".sdata", SS, 8, " data:string")])
    check("sinit definers: a string literal passed to a call in a sinit is an argument, not an object the unit constructs",
          [(s["name"], sorted(cstr.definers(s))) for s in cstr.data_syms if s["name"] == "str"], [("str", [])])

    # pool order (rule 4): a string a pointer initialiser of the unit's own table names is used first; the same table in another unit's data is not
    S0, S1, TB = 0x80790000, 0x80790008, 0x80590000
    pf = [_lis(3, S1 >> 16), ADDI(3, 3, S1 & 0xFFFF), BLR, NOP, _lis(3, S0 >> 16), ADDI(3, 3, S0 & 0xFFFF), BLR, NOP]            # f0 reads s1, f1 reads s0: s0 follows s1
    for label, tab, want in (("no table: s0 is first used after s1 (an inversion)", None, FAIL),
                             ("a table of the unit's own data points at s0: s0 is used first", "own", PASS),
                             ("the table is another unit's data: no help", "other", FAIL)):
        own_r = [(".text", TT, TT + 0x20), (".sdata", S0, S0 + 0x10)] + ([(".data", TB, TB + 8)] if tab == "own" else [])
        oth_r = [(".text", TT + 0x20, TT + 0x40)] + ([(".data", TB, TB + 8)] if tab == "other" else [])
        csp, _sp = _mini(TT, pf + [BLR] + [NOP] * 7 + [BLR] + [NOP] * 7, [("f0", 0, 0x10), ("f1", 0x10, 0x10), ("o", 0x20, 0x20)],
                         unit_text(("own.cpp", own_r), ("other.cpp", oth_r)), [(S0, bytes(0x10)), (TB, struct.pack(">II", S0, 0))],
                         [dsym("s0", ".sdata", S0, 8, " data:string"), dsym("s1", ".sdata", S1, 8, " data:string"), dsym("tbl", ".data", TB, 8)])
        check("pool: %s" % label, run_checks(csp, None, ["pool"]).units["own.cpp"]["pool"]["status"], want)

    # file order (rule 10): the ranges of a section, by address, must have non-decreasing file positions - a data-only unit has only its file position
    O2 = 0x80300000
    ou = unit_text(*[("%s.cpp" % nm, [(".sdata2", O2 + 4 * k, O2 + 4 * k + 4)]) for nm, k in (("a", 0), ("c", 1), ("b", 2), ("z", 3))])
    co1, _o1 = _mini(TT, [BLR], [("f", 0, 4)], ou, [], [])
    check("order: units in file order are in link order", file_order_offenders(co1), [])
    ou2 = unit_text(*[("%s.cpp" % nm, [(".sdata2", O2 + 4 * k, O2 + 4 * k + 4)]) for nm, k in (("z", 3), ("a", 0), ("b", 1), ("c", 2))])
    co2, _o2 = _mini(TT, [BLR], [("f", 0, 4)], ou2, [], [])
    check("order: a data-only unit placed first in the file but last by address is out of link order (the fewest ranges that must move)",
          [(o[1], hx(o[2]), o[4]) for o in file_order_offenders(co2)], [("z.cpp", hx(O2 + 12), "c.cpp")])
    check("order: the offender is an `order` FAIL of its own unit and of nobody else",
          sorted((n, r["order"]["status"]) for n, r in run_checks(co2, None, ["order"]).units.items() if "order" in r),
          [("a.cpp", PASS), ("b.cpp", PASS), ("c.cpp", PASS), ("z.cpp", FAIL)])

    # jump tables (rule 11): a reader is the indexed dispatch, not any reference whose address falls inside the symbol
    JT2 = 0x80580100
    own_fn = [_lis(3, JT2 >> 16), ADDI(3, 3, JT2 & 0xFFFF), SLWI_R0_R4_2, LWZX(0, 3, 0), MTCTR_R0, BCTR_W, NOP, NOP]            # the dispatch
    field_fn = [_lis(3, JT2 >> 16), LHZ(0, 3, (JT2 & 0xFFFF) + 8), BLR, NOP]                                                     # lhz r0, 8(r3): a struct field that falls in the symbol
    for label, foreign, want, find in (("a foreign lhz through lis is a field access, the unit's own dispatch is the reader", field_fn, PASS, None),
                                       ("a foreign dispatch is a foreign reader", own_fn, FAIL, "jumptable_80580100 is read by foreign.cpp, not by this unit")):
        cj, _sj = _mini(TT, own_fn + foreign + [NOP] * (0x40 // 4 - len(foreign)), [("own", 0, 0x20), ("f", 0x20, 0x20), ("g", 0x40, 0x20)],
                        unit_text(("own.cpp", [(".text", TT, TT + 0x20), (".data", JT2, JT2 + 0x20)]), ("foreign.cpp", [(".text", TT + 0x20, TT + 0x60)])),
                        [(JT2, struct.pack(">8I", *([TT] * 8)))], [dsym("jumptable_80580100", ".data", JT2, 0x20, scope="local")])
        r = run_checks(cj, None, ["jumptable"]).units["own.cpp"]["jumptable"]
        check("jumptable: %s" % label, (r["status"], r["finding"] if want == FAIL else None), (want, find))
    check("jumptable_reader_sites: only the dispatch counts (the addi that forms the address, followed by lwzx, mtctr, bctr)",
          [hex(x) for x in jumptable_reader_sites(cj, next(s for s in cj.data_syms if s["name"].startswith("jumptable_")))], [hex(TT + 4), hex(TT + 0x24)])

    # ranking
    td = top_defects(ctx, res, 50)
    check("defects are ranked by score", [d["score"] for d in td] == sorted((d["score"] for d in td), reverse=True) and bool(td), True)
    print("\n%d checks, %d failed" % (NCHECKS[0], len(fails)))
    return 1 if fails else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--baseline", action="store_true", help="check the current splits.txt")
    ap.add_argument("--splits"), ap.add_argument("--symbols"), ap.add_argument("--dol")
    ap.add_argument("--outbox", help="lane outbox with seam requests (default: the primary checkout's .pi/outbox)")
    ap.add_argument("--readers", action="append", metavar="SEC:START-END",
                    help="print each map symbol of that data range with its owner and the units that read it (repeatable)")
    ap.add_argument("--only", help="comma list of invariants")
    ap.add_argument("--unit", help="regex: report only the units whose name matches (--baseline)")
    ap.add_argument("--json", help="write the machine-readable report here")
    ap.add_argument("--all", action="store_true", help="list every unit, not only the failing ones")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--intervals", action="store_true",
                    help="with --baseline: print each pool value held at two addresses (read by --unit's units) with the interval a TU starts in")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.readers:
        return cmd_readers(args)
    if args.baseline:
        return cmd_baseline(args)
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
