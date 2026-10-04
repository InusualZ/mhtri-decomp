"""The splitcheck context: splits + map + retail DOL, the decoded text references, the result store.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

import bisect
import collections
import os
import struct

from tools.lib import findings as _findings
from tools.lib import ppc as _ppc
from tools.lib import project as _project
from tools.lib import refs as _refs
from tools.lib import repo as _repo
from tools.lib.binary.dol import Dol as LibDol
import tools.splits.seams.evidence as _ev

PASS, FAIL, UNKNOWN, NA = "PASS", "FAIL", "UNKNOWN", "-"
#: the verdict as a `lib.findings` status (`-`, not applicable, is SKIP)
ROW_STATUS = {PASS: _findings.PASS, FAIL: _findings.FAIL, UNKNOWN: _findings.UNKNOWN, NA: _findings.SKIP}
GAME = _ev.GAME
RANK = {NA: 0, PASS: 1, UNKNOWN: 2, FAIL: 3}
INVARIANTS = ("order", "coverage", "text-cut", "extab", "ctors", "dtors", "pool", "data-order", "vtable",
              "jumptable", "bss", "local-static")
#: how bad a defect of this invariant is, for the top-N list (higher first)
WEIGHT = {"order": 100, "coverage": 95, "extab": 90, "ctors": 85, "dtors": 80, "text-cut": 75, "pool": 60,
          "vtable": 55, "jumptable": 50, "data-order": 45, "bss": 40, "local-static": 58}
SECTION_ORDER = [".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data", ".bss", ".sdata",
                 ".sbss", ".sdata2", ".sbss2"]
CODE_SECTIONS = (".init", ".text")
#: a function that forms this many distinct section starts with `addi @l` is start-up / module-loader code taking
#: `_f_<section>` linker symbols (the RSO loader forms nine, `__init_data` two: those two stay ordinary references)
LINKER_STARTS = 3
#: the `data:` kinds whose equal values prove two pool entries (`evidence.VALUE_KINDS`)
LITERAL_KINDS = _ev.VALUE_KINDS
#: alignment padding a unit may keep after its last function (a 16-byte-aligned function start)
SINIT_SLACK = 0xC
#: an end bound that never binds (the own-slot run is followed past a unit's end)
UNBOUNDED = 0xFFFFFFFF
#: the largest slot function past a unit's end that still reads as an inline virtual the unit emitted
INLINE_SLOT_MAX = 0x40
#: crt chain entries a runtime unit registers for another unit's function
CRT_CHAIN = ("__destroy_global_chain", "__init_cpp_exceptions", "__fini_cpp_exceptions")


def tree_root():
    """The tree the audit reads (`lib.repo.repo_root`: the invocation's tree)."""
    return _repo.repo_root()


def main_root(root=None):
    """The primary checkout (`$MHTRI_MAIN` for the served tree, else the parent of the git common dir), or None."""
    root = root or tree_root()
    return _repo.main_tree(root, honour_env=_repo.is_served(root))


def find_file(rel, root=None):
    """`rel` under the tree, else under the primary checkout (orig/ and build/ are not in a worktree)."""
    root = root or tree_root()
    return _repo.resolve_input(rel, root, os.path.isfile, honour_env=_repo.is_served(root))


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


def find_sda_bases(dol, code_words):
    """`(r13, r2)` from the start-up code's `lis`/`ori|addi` pairs (`lib.ppc.find_sda_bases`; `dol` is unused)."""
    return _ppc.find_sda_bases(code_words)


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

    def rows(self) -> list:
        """One `lib.findings.Row` per (unit, invariant): `<unit>: <invariant>`, the worst verdict, the finding that
        set it as `detail` (with its address), the pass/fail counts as `evidence`."""
        out = []
        for unit, recs in self.units.items():
            for inv in INVARIANTS:
                r = recs.get(inv)
                if r is None:
                    continue
                detail = ("%s %s" % (hx(r["addr"]), r["finding"])).strip() if r["addr"] is not None else r["finding"]
                out.append(_findings.Row("%s: %s" % (unit, inv), ROW_STATUS[r["status"]], detail,
                                         "%d pass, %d fail" % (r["n_pass"], r["n_fail"])))
        return out


def hx(a):
    return "0x%08X" % a if a is not None else "-"


def is_literal(sym):
    """A pool literal (`evidence.is_literal`): an `.sdata2` 4/8-byte object or an `.sdata` string."""
    return _ev.is_literal(sym["section"], sym["size"], sym["kind"], sym["type"])


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
