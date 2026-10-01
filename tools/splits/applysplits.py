#!/usr/bin/env python3
"""applysplits.py - phase 4 of the splits program: write the reconciled candidate into `splits.txt` window by window, with a work manifest.

    python tools/splits/applysplits.py plan     --window W [--json OUT] [--check] [--dtk]
    python tools/splits/applysplits.py apply    --window W
    python tools/splits/applysplits.py manifest --window W [--json OUT] [--md OUT]
    python tools/splits/applysplits.py verify   --window W [--base REF] [--no-build] [--before-report F] [--allow-unregistered]
    python tools/splits/applysplits.py all      [--out-dir docs/splits/phase4] [--dtk]        # plan + manifest of every window, read-only
    python tools/splits/applysplits.py freeze                                              # once: docs/splits/phase4/baseline-splits.txt
    python tools/splits/applysplits.py --selftest

`W` is a window letter (`a`..`fg`, the six `.text` windows of docs/splits/phase4/README.md) or an address range `0xLO..0xHI`.

The candidate is the one `docs/splits-program.md` defines (phase 1 a..g + reconcile + folds + `phase2-reconcile.json`, `splitcheck.render`), its
`main/<name>` units renamed to root units.  Against the current tree's `config/RMHE08/splits.txt` the units split into IDENTICAL ones (same name,
ranges and attributes: untouched) and CHANGED ones; a changed old unit and a changed candidate unit are linked when they share a name or overlap in any
section, and a connected set is one COMPONENT: the old blocks it replaces and the candidate blocks that replace them (a new unit, a recut of a registered
unit, a fold of N registered units, a data gain, a data-only unit).  A component is assigned to the window holding the lowest `.text` start among its
units (a data-only unit takes the window of the unit before it in file order); a component whose units lie in more than one window, or a unit whose
`.text` crosses a window edge, is REPORTED as a straddler.

* `plan`     the edit: per component the old blocks replaced and the new blocks in candidate file order (every section, `rename:`/`common` attributes
             kept).  `--check` writes the edited file to a scratch path and runs splitcheck's invariants on it (no failure the base and the candidate
             both lack); `--dtk` runs `dtk dol split --no-update` on it into a scratch dir (the real acceptance test: no cycle, overlap or
             `ends within symbol`).
* `apply`    writes the edited `splits.txt` in the current tree and nothing else (refuses when the file does not round-trip through the renderer, which
             would churn untouched blocks).  Applying the six windows in any order gives exactly the candidate.
* `manifest` the lane's work list for the window (kinds, absorbed sources with their `configure.py` rows and functions, headers and includers, renames,
             `--unit-rename` pairs for the landing gate, Matching demotions, language evidence, data to define) as JSON and/or Markdown.
* `verify`   after a lane's work: splits.txt equals the plan, every unit of the window has a registered `Object(...)` and a source (or neither with
             `--allow-unregistered`), every folded unit's source is gone and unregistered, every function appears once in the merged file, no name
             collides, no demoted unit is still `Matching`, the window's units compile, `ninja build/RMHE08/ok` is green and no function regressed.

`docs/splits/phase4/names.json` (`{"names": {"<candidate name>": "<final name>"}}`) records the names a lane chose for placeholder units; plan, apply,
manifest and verify all read it, so the plan stays equal to the tree.  Nothing here writes outside the tree's `config/RMHE08/splits.txt` (`apply`),
`build/tmp/applysplits/` (scratch) and the paths named by `--json`/`--md`/`--out-dir`.
"""
from __future__ import annotations

import argparse
import bisect
import collections
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
for _p in (os.path.join(TOOLS, "units"), TOOLS, HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import splitcheck as sc  # noqa: E402

GAME = "RMHE08"
#: the six `.text` windows of phase 4 (docs/splits/phase4/README.md); the last one is open-ended and also holds the units that have no `.text`
WINDOWS = (("a", 0x80000000, 0x800E0000), ("b", 0x800E0000, 0x801C0000), ("c", 0x801C0000, 0x802A0000),
           ("d", 0x802A0000, 0x80380000), ("e", 0x80380000, 0x80460000), ("fg", 0x80460000, None))
CODE = (".init", ".text")
DATA = (".rodata", ".data", ".sdata", ".sdata2", ".bss", ".sbss", ".sbss2")
PHASE4_DIR = os.path.join("docs", "splits", "phase4")
NAMES_FILE = os.path.join(PHASE4_DIR, "names.json")
#: the splits.txt the proposals were written against: the candidate is always rendered over THIS file, never over the current one (a proposal unit that reuses the
#: name of a unit an earlier window already wrote would be refused as a name used twice, and the candidate would change under the lane's feet)
BASELINE_FILE = os.path.join(PHASE4_DIR, "baseline-splits.txt")
SRC_EXT = (".cpp", ".cp", ".c")
MANGLED_RE = re.compile(r"[A-Za-z0-9_]__(?:Q?\d|F|C\w)")
PLACEHOLDER_RE = re.compile(r"^fn_([0-9A-Fa-f]{8})$")
SUFFIX_NAMES = ("_base", "_head", "_tail", "_merged")
RUN_NAME_RE = re.compile(r"^eft\d+_fx$")


def hx(a):
    return "0x%08X" % a


# ---- windows ---------------------------------------------------------------------------------------------------------------

def parse_window(spec, windows=WINDOWS):
    """`a`..`fg` -> `(windows, index)`; `0xLO..0xHI` -> three windows (below, the range, above) and the index of the range, so a component is chosen by the same rule."""
    spec = (spec or "").strip()
    for i, (n, _lo, _hi) in enumerate(windows):
        if n == spec:
            return windows, i
    m = re.match(r"^(0x[0-9A-Fa-f]+)\.\.(0x[0-9A-Fa-f]+)$", spec)
    if m:
        lo, hi = int(m.group(1), 16), int(m.group(2), 16)
        return (("below", 0, lo), ("%s..%s" % (m.group(1), m.group(2)), lo, hi), ("above", hi, None)), 1
    raise ValueError("window %r: want one of %s or 0xLO..0xHI" % (spec, ", ".join(n for n, _a, _b in windows)))


def window_of(addr, windows):
    for i, (_n, lo, hi) in enumerate(windows):
        if addr >= lo and (hi is None or addr < hi):
            return i
    return 0 if addr < windows[0][1] else len(windows) - 1


def unit_windows(units, windows):
    """The window index of each unit of a file-ordered list: its first `.text` start, else the window of the unit before it (a data-only or `.init`-only unit)."""
    out, cur = [], 0
    for u in units:
        t = u.first(".text")
        if t is not None:
            cur = window_of(t, windows)
        out.append(cur)
    return out


# ---- units ------------------------------------------------------------------------------------------------------------------

def unit_sig(u):
    return (u.attrs.strip(), tuple((sec, tuple(sorted((a, b, x.strip()) for a, b, x in rr))) for sec, rr in sorted(u.ranges.items())))


def copy_unit(u, name=None):
    return sc.Unit(name or u.name, u.attrs, {s: list(rr) for s, rr in u.ranges.items()})


def clone_splits(sp):
    return sc.Splits(list(sp.header), [copy_unit(u) for u in sp.units])


def code_ranges(u):
    return sorted(r for sec in CODE for r in u.rs(sec))


def has_code(u):
    return bool(code_ranges(u))


def overlap_bytes(r1, r2):
    """Total bytes shared by two sorted lists of `(a, b)` ranges."""
    tot = 0
    for a, b in r1:
        for c, d in r2:
            lo, hi = max(a, c), min(b, d)
            if hi > lo:
                tot += hi - lo
    return tot


def subtract_ranges(rr, cuts):
    out = []
    for a, b in rr:
        cur = a
        for c, d in sorted(cuts):
            if d <= cur or c >= b:
                continue
            if c > cur:
                out.append((cur, c))
            cur = max(cur, d)
        if cur < b:
            out.append((cur, b))
    return out


def ext_of(name):
    for e in SRC_EXT:
        if name.endswith(e):
            return e
    return ""


def stem_of(name):
    e = ext_of(name)
    return name[:-len(e)] if e else name


def map_candidate(cand, names=None):
    """The candidate with the names a lane chose applied, and `main/<name>` units made root units."""
    names = names or {}
    units = []
    for u in cand.units:
        n = names.get(u.name, u.name)
        if n.startswith("main/"):
            n = n[len("main/"):]
        units.append(copy_unit(u, n))
    return sc.Splits(list(cand.header), units)


def align_candidate(cand, symbols, absorb_blocked=False):
    """`(Splits, fixes, blocked)`: the candidate with every data range boundary that is off a 4-byte boundary moved onto one.

    dtk refuses a split that starts off the alignment (`Invalid alignment for split: auto_07_8058AF41_data` for the gap after a range that ends there,
    `... homebutton/fn_80556FC4.cpp .bss 8:0x8079082A` for a range that starts there; a 2-aligned start such as 0x80673C0A is refused too).  The registered splits.txt
    has no such boundary (180 gaps, all aligned) and the candidate has 23 gaps and 4 more starts: its ranges end where the last symbol ends and begin where the
    first one begins.  An END before a gap moves up over the padding; a START moves down over the padding of the gap before it, or (abutting) takes the padding
    from the range before it.  Neither moves over a map symbol; what cannot move is `blocked` (an unowned or foreign unaligned symbol begins right there).
    `fixes` and `blocked` are `(unit, section, old boundary, new boundary or the blocking address)`."""
    sym_start = collections.defaultdict(list)
    sym_cover = collections.defaultdict(list)
    for s in symbols:
        sym_start[s["section"]].append(s["addr"])
        sym_cover[s["section"]].append((s["addr"], s["addr"] + max(s["size"], 1)))
    for v in sym_start.values():
        v.sort()

    def starts_in(sec, a, b):
        k = bisect.bisect_left(sym_start[sec], a)
        return sym_start[sec][k] if k < len(sym_start[sec]) and sym_start[sec][k] < b else None

    def covered(sec, a, b):
        return any(x < b and y > a and x < a for x, y in sym_cover[sec] if x < b and y > a)

    by = collections.defaultdict(list)
    for u in cand.units:
        for sec, rr in u.ranges.items():
            if sec in CODE or sec in (".ctors", ".dtors", "extab", "extabindex"):
                continue
            for i, (a, b, x) in enumerate(rr):
                if "rename:" not in x and "common" not in x:
                    by[sec].append([a, b, u, i])
    fixes, blocked = [], []

    def write(sec, r):
        u, i = r[2], r[3]
        old = u.ranges[sec][i]
        u.ranges[sec][i] = (r[0], r[1], old[2])

    for sec, lst in by.items():
        for _round in range(3):
            lst.sort(key=lambda r: (r[0], r[1]))
            changed = False
            for k, r in enumerate(lst):
                prev = lst[k - 1] if k else None
                nxt = lst[k + 1] if k + 1 < len(lst) else None
                if r[0] % 4:                                         # an unaligned start
                    fl = r[0] & ~3
                    if prev is not None and prev[1] > r[0]:
                        continue
                    if prev is not None and prev[1] == r[0]:           # abutting: the padding comes out of the range before
                        if prev[0] < fl and not covered(sec, fl, r[0]) and starts_in(sec, fl, r[0]) is None:
                            fixes.append((r[2].name, sec, r[0], fl))
                            prev[1] = r[0] = fl
                            write(sec, prev)
                            write(sec, r)
                            changed = True
                    elif (prev is None or fl >= prev[1]) and starts_in(sec, fl, r[0]) is None and not covered(sec, fl, r[0]):
                        fixes.append((r[2].name, sec, r[0], fl))
                        r[0] = fl
                        write(sec, r)
                        changed = True
                if r[1] % 4 and (nxt is None or nxt[0] > r[1]):      # an unaligned end in front of a gap
                    ce = (r[1] + 3) & ~3
                    if (nxt is None or ce <= nxt[0]) and starts_in(sec, r[1], ce) is None:
                        fixes.append((r[2].name, sec, r[1], ce))
                        r[1] = ce
                        write(sec, r)
                        changed = True
            if not changed:
                break
        lst.sort(key=lambda r: (r[0], r[1]))
        for k, r in enumerate(lst):
            nxt = lst[k + 1] if k + 1 < len(lst) else None
            if r[1] % 4 and (nxt is None or nxt[0] > r[1]):
                hit = starts_in(sec, r[1], (r[1] + 3) & ~3)
                if absorb_blocked and hit is not None:
                    # DIAGNOSTIC ONLY (`--absorb-blocked`): give the unowned symbol to the range before it so dtk can be run past this error
                    end = max(x["addr"] + x["size"] for x in symbols if x["section"] == sec and x["addr"] == hit)
                    ce = (end + 3) & ~3
                    if nxt is None or ce <= nxt[0]:
                        fixes.append((r[2].name, sec, r[1], ce))
                        r[1] = ce
                        write(sec, r)
                        continue
                blocked.append((r[2].name, sec, r[1], hit if hit is not None else (nxt[0] if nxt else 0)))
            if r[0] % 4:
                if absorb_blocked:
                    # DIAGNOSTIC ONLY: the range takes the unowned symbol(s) in front of it, down to an aligned start
                    prev = lst[k - 1] if k else None
                    cur = r[0]
                    for _i in range(4):
                        if cur % 4 == 0:
                            break
                        cov = [x for x in symbols if x["section"] == sec and x["addr"] < cur <= x["addr"] + max(x["size"], 1) - 0 and x["addr"] + x["size"] >= cur]
                        cur = min([x["addr"] for x in cov] + [cur - 1])
                    if cur % 4 == 0 and (prev is None or cur >= prev[1]):
                        fixes.append((r[2].name, sec, r[0], cur))
                        r[0] = cur
                        write(sec, r)
                        continue
                blocked.append((r[2].name, sec, r[0], r[0]))
    return cand, sorted(fixes), sorted(set(blocked))


# ---- the diff and its components --------------------------------------------------------------------------------------------

class Component:
    def __init__(self, idx, old, new, window, windows_seen, crossings):
        self.idx = idx
        self.old = old                  # old unit names, in file order
        self.new = new                  # candidate unit names, in candidate file order
        self.window = window            # window index
        self.windows_seen = windows_seen
        self.crossings = crossings      # [(unit name, edge address)]

    def key(self):
        """A label that does not change when other components are applied: the first candidate unit, else the first old one."""
        return (self.new or self.old)[0]


def lis_indices(seq):
    return set(sc.longest_nondecreasing(seq))


def diff_units(old, new, windows=WINDOWS):
    """`(identical names, moved names, components)`; see the module docstring.  `moved`: identical units whose file order the candidate changes."""
    om, nm = old.by_name(), new.by_name()
    if len(om) != len(old.units):
        raise ValueError("the current splits.txt names a unit twice")
    if len(nm) != len(new.units):
        raise ValueError("the candidate names a unit twice")
    same = {n for n in nm if n in om and unit_sig(om[n]) == unit_sig(nm[n])}
    npos = {u.name: i for i, u in enumerate(new.units)}
    in_old_order = [u.name for u in old.units if u.name in same]
    keep = lis_indices([npos[n] for n in in_old_order])
    moved = {n for i, n in enumerate(in_old_order) if i not in keep}
    ident = same - moved
    changed_old = [u.name for u in old.units if u.name not in ident]
    changed_new = [u.name for u in new.units if u.name not in ident]
    parent = {}

    def find(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        parent[find(a)] = find(b)

    for n in changed_old:
        find(("o", n))
    for n in changed_new:
        find(("n", n))
        if n in om:
            union(("o", n), ("n", n))
    index = collections.defaultdict(list)        # section -> sorted [(start, end, new name)]
    for n in changed_new:
        for sec, rr in nm[n].ranges.items():
            for a, b, _x in rr:
                index[sec].append((a, b, n))
    starts = {}
    for sec in index:
        index[sec].sort()
        starts[sec] = [r[0] for r in index[sec]]
    for n in changed_old:
        for sec, rr in om[n].ranges.items():
            lst = index.get(sec)
            if not lst:
                continue
            for a, b, _x in rr:
                k = max(0, bisect.bisect_right(starts[sec], a) - 2)
                while k < len(lst) and lst[k][0] < b:
                    if lst[k][1] > a:
                        union(("o", n), ("n", lst[k][2]))
                    k += 1
    groups = collections.defaultdict(lambda: ([], []))
    for n in changed_old:
        groups[find(("o", n))][0].append(n)
    for n in changed_new:
        groups[find(("n", n))][1].append(n)
    ow, nw = unit_windows(old.units, windows), unit_windows(new.units, windows)
    owin = {u.name: ow[i] for i, u in enumerate(old.units)}
    nwin = {u.name: nw[i] for i, u in enumerate(new.units)}
    edges = [hi for _n, _lo, hi in windows if hi is not None]
    comps = []
    for _root, (olds, news) in groups.items():
        wins = sorted({owin[n] for n in olds} | {nwin[n] for n in news})
        cross = []
        for src, names in ((om, olds), (nm, news)):
            for n in names:
                cr = code_ranges(src[n])
                if cr:
                    lo, hi = cr[0][0], cr[-1][1]
                    cross += [(n, e) for e in edges if lo < e < hi]
        cross = sorted(set(cross))
        anchor = min([npos[n] for n in news] + [10 ** 9])
        comps.append((wins[0], anchor, olds, news, wins, cross))
    comps.sort(key=lambda c: (c[0], c[1], c[2][:1]))
    out = [Component(i, c[2], c[3], c[0], c[4], c[5]) for i, c in enumerate(comps)]
    return ident, moved, out


def apply_components(old, new, comps):
    """The splits with `comps` applied: each component's old blocks removed, its candidate blocks inserted right after the nearest candidate predecessor
    that is present (position 0 when none is).  The relative order of the units already present is never disturbed, so the order of the result is the
    candidate's once every component is applied, whatever order they came in."""
    nm = new.by_name()
    order = [u.name for u in new.units]
    out = [copy_unit(u) for u in old.units]
    for comp in sorted(comps, key=lambda c: c.idx):
        gone = set(comp.old)
        out = [u for u in out if u.name not in gone]
        for n in sorted(comp.new, key=order.index):
            present = {u.name: i for i, u in enumerate(out)}
            pos = 0
            for j in range(order.index(n) - 1, -1, -1):
                if order[j] in present:
                    pos = present[order[j]] + 1
                    break
            out.insert(pos, copy_unit(nm[n]))
    return sc.Splits(list(old.header), out)


# ---- configure.py -------------------------------------------------------------------------------------------------------------

def parse_configure(text):
    """`path -> {state, line, text, lib, count}` for every `Object(<State>, "<path>", ...)` call (balanced parentheses, so a multi-line call is one row)."""
    libs = [(m.start(), m.group(1)) for m in re.finditer(r'"lib"\s*:\s*"([^"]+)"', text)]
    lib_pos = [p for p, _n in libs]
    out = {}
    for m in re.finditer(r"\bObject\(", text):
        i, depth, quote = m.end(), 1, None
        while i < len(text) and depth:
            ch = text[i]
            if quote:
                if ch == "\\":
                    i += 1
                elif ch == quote:
                    quote = None
            elif ch in "\"'":
                quote = ch
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            i += 1
        body = text[m.start():i]
        mm = re.match(r"Object\(\s*(\w+)\s*,\s*\"([^\"]+)\"", body)
        if not mm:
            continue
        k = bisect.bisect_right(lib_pos, m.start()) - 1
        path = mm.group(2)
        if path in out:
            out[path]["count"] += 1
            continue
        out[path] = {"state": mm.group(1), "line": text.count("\n", 0, m.start()) + 1,
                     "text": re.sub(r"\s+", " ", body).strip(), "lib": libs[k][1] if k >= 0 else "", "count": 1}
    return out


# ---- the sources ----------------------------------------------------------------------------------------------------------------

class Tree:
    """Read access to a tree's sources and includes (the working tree, or a fixture directory)."""

    def __init__(self, root):
        self.root = root
        self._inc = None

    def path(self, rel):
        return os.path.join(self.root, *rel.split("/"))

    def read(self, rel):
        try:
            with open(self.path(rel), encoding="utf-8", errors="replace") as fh:
                return fh.read()
        except OSError:
            return None

    def exists(self, rel):
        return os.path.isfile(self.path(rel))

    def includers(self):
        """`header path -> [files that include it]` over `src/` and `include/` (an include resolves beside the including file, under `include/`, or under `src/`)."""
        if self._inc is not None:
            return self._inc
        inc = collections.defaultdict(list)
        files = []
        for top in ("src", "include"):
            for dp, _dn, fn in os.walk(self.path(top)):
                for f in fn:
                    if f.endswith((".c", ".cpp", ".cp", ".h", ".hpp")):
                        files.append(os.path.relpath(os.path.join(dp, f), self.root).replace("\\", "/"))
        for f in files:
            txt = self.read(f) or ""
            for m in re.finditer(r'^\s*#\s*include\s+["<]([^">]+)[">]', txt, re.M):
                s = m.group(1)
                for base in (os.path.dirname(f), "include", "src", ""):
                    cand = os.path.normpath(os.path.join(base, s)).replace("\\", "/")
                    if self.exists(cand):
                        inc[cand].append(f)
                        break
        self._inc = inc
        return inc

    def headers_of(self, unit_name):
        """The headers a unit's source owns by name (`include/<dir>/<stem>.h`, `src/<dir>/<stem>.h`, `include/<stem>.h`) with the files that include them."""
        stem = stem_of(unit_name)
        base = os.path.basename(stem)
        found = []
        for cand in ("include/%s.h" % stem, "src/%s.h" % stem, "include/%s.h" % base):
            if cand not in found and self.exists(cand):
                found.append(cand)
        inc = self.includers()
        return [{"path": h, "included_by": sorted(inc.get(h, []))} for h in found]


def function_probe(name):
    """The identifiers a source may define a map function under: the symbol, its unmangled base, and `Class::Class`/`Class::~Class` for a ctor/dtor."""
    out = [name]
    m = re.match(r"^__(ct|dt)__(\d+)(\w+?)F", name)
    if m:
        cls = m.group(3)[:int(m.group(2))]
        out += [cls + "::" + ("~" if m.group(1) == "dt" else "") + cls]
    else:
        base = re.sub(r"(?<=\w)__(?:Q?\d|F|C)\w*$", "", name)
        if base != name and base:
            out.append(base)
        m = re.match(r"^(\w+?)__(\d+)(\w+?)(?:F|C)", name)
        if m:
            cls = m.group(3)[:int(m.group(2))]
            out.append(cls + "::" + m.group(1))
    return out


def definition_lines(text, name):
    """`(definitions, first mention)`: the 1-based lines of `text` that DEFINE one of the spellings of `name`, and the first line that merely calls or declares it.
    A definition starts in column 0 and its parameter list, wherever it ends, is followed by `{` (or `:` of a constructor's initialiser list), not `;` or `,`."""
    defs, mention = [], 0
    for spelling in function_probe(name):
        for m in re.finditer(r"(?<![\w:])%s\s*\(" % re.escape(spelling), text):
            line = text.count(chr(10), 0, m.start()) + 1
            start = text.rfind(chr(10), 0, m.start()) + 1
            if text[start:start + 1] in (" ", chr(9), "#", "/", "*", ""):
                mention = mention or line
                continue
            i, depth = m.end(), 1
            while i < len(text) and depth and i - m.end() < 3000:
                depth += (text[i] == "(") - (text[i] == ")")
                i += 1
            tail = re.match(r"\s*(?:const\b\s*)?(\S)", text[i:i + 80])
            if tail and tail.group(1) in "{:":
                if line not in defs:
                    defs.append(line)
            else:
                mention = mention or line
    return sorted(defs), mention


def find_definition(text, name):
    """`(defined, line)`: whether `text` defines `name` (see `definition_lines`), with the first definition's line, else the first mention's; `(None, 0)` without a text."""
    if text is None:
        return None, 0
    defs, mention = definition_lines(text, name)
    return (True, defs[0]) if defs else (False, mention)


# ---- the inputs ---------------------------------------------------------------------------------------------------------------------

class Inputs:
    def __init__(self, old, new, symbols, configure_text, tree, names=None, windows=WINDOWS, cand_issues=(), fixes=(), blocked=(), new_raw=None):
        self.old, self.new = old, new
        self.new_raw = new_raw if new_raw is not None else new       # the candidate before the alignment normalisation
        self.fixes, self.blocked = list(fixes), list(blocked)
        self.symbols = symbols
        self.configure_text = configure_text
        self.conf = parse_configure(configure_text)
        self.tree = tree
        self.names = names or {}
        self.windows = windows
        self.cand_issues = list(cand_issues)
        self.om, self.nm = old.by_name(), new.by_name()
        self._funcs = None

    def diag(self):
        """The same inputs with the candidate's dtk blockers given an owner (`align_candidate(absorb_blocked=True)`): a DIAGNOSTIC to see what dtk says past them."""
        new, fixes, blocked = align_candidate(clone_splits(self.new_raw), self.symbols, True)
        return Inputs(self.old, new, self.symbols, self.configure_text, self.tree, self.names, self.windows, self.cand_issues, fixes, blocked, self.new_raw)

    def functions(self):
        """Function symbols in code sections, address-sorted: `[(addr, size, name)]` and the parallel address list."""
        if self._funcs is None:
            rows = sorted((s["addr"], s["size"], s["name"]) for s in self.symbols if s["type"] == "function" and s["section"] in CODE)
            self._funcs = (rows, [r[0] for r in rows])
        return self._funcs

    def functions_in(self, ranges):
        rows, addrs = self.functions()
        out = []
        for a, b in ranges:
            k = bisect.bisect_left(addrs, a)
            while k < len(rows) and rows[k][0] < b:
                out.append(rows[k])
                k += 1
        return sorted(set(out))


def load_inputs(root=None, splits_text=None, configure_text=None, proposals=None, names=None, windows=WINDOWS, tree=None, absorb_blocked=False):
    root = root or sc.tree_root()
    import dataattach as da
    paths = proposals or da.default_proposals() + [os.path.join(da.PROPOSAL_DIR, "phase2-reconcile.json")]
    with open(os.path.join(root, "config", GAME, "splits.txt"), encoding="utf-8", errors="replace") as fh:
        cur_text = fh.read()
    _s, symbols, dol = sc.load_ctx(os.path.join(root, "config", GAME, "splits.txt"))
    old = sc.parse_splits(splits_text if splits_text is not None else cur_text)
    frozen_path = os.path.join(root, BASELINE_FILE)
    if os.path.isfile(frozen_path):
        with open(frozen_path, encoding="utf-8", errors="replace") as fh:
            frozen = sc.parse_splits(fh.read())
    else:
        frozen = old
    cand, info = sc.render(frozen, [sc.load_proposal(p) for p in paths], dol, symbols)
    if names is None:
        names = load_names(root)
    if configure_text is None:
        with open(os.path.join(root, "configure.py"), encoding="utf-8", errors="replace") as fh:
            configure_text = fh.read()
    raw = map_candidate(cand, names)
    new, fixes, blocked = align_candidate(clone_splits(raw), symbols, absorb_blocked)
    return Inputs(old, new, symbols, configure_text, tree or Tree(root), names, windows, info["issues"], fixes, blocked, raw), dol


def resolve_window(inp, spec):
    """The window index for `spec` (a letter or `0xLO..0xHI`); an address range replaces the windows of `inp` with that one."""
    inp.windows, widx = parse_window(spec, inp.windows)
    return widx


def load_names(root):
    p = os.path.join(root, NAMES_FILE)
    if not os.path.isfile(p):
        return {}
    with open(p, encoding="utf-8") as fh:
        return dict(json.load(fh).get("names", {}))


# ---- plan ---------------------------------------------------------------------------------------------------------------------------

def components_of(inp):
    ident, moved, comps = diff_units(inp.old, inp.new, inp.windows)
    return ident, moved, comps


def plan_for(inp, widx):
    """`(comps of the window, the edited Splits)`."""
    _ident, _moved, comps = components_of(inp)
    mine = [c for c in comps if c.window == widx]
    return mine, apply_components(inp.old, inp.new, mine)


def describe_unit(u):
    return {"name": u.name, "attrs": u.attrs, "ranges": {s: [[hx(a), hx(b)] + ([x.strip()] if x.strip() else []) for a, b, x in sorted(rr)]
                                                         for s, rr in sorted(u.ranges.items())}}


def plan_json(inp, widx, comps, edited):
    win = inp.windows[widx]
    return {"window": win[0], "range": [hx(win[1]), hx(win[2]) if win[2] is not None else None],
            "components": [{"id": c.idx, "replaces": [describe_unit(inp.om[n]) for n in c.old], "with": [describe_unit(inp.nm[n]) for n in c.new],
                            "windows": [inp.windows[w][0] for w in c.windows_seen],
                            "straddles": [{"unit": n, "edge": hx(e)} for n, e in c.crossings]} for c in comps],
            "units_before": len(inp.old.units), "units_after": len(edited.units)}


def plan_text(inp, widx, comps):
    win = inp.windows[widx]
    lines = ["window %s: %s .. %s, %d component(s)" % (win[0], hx(win[1]), hx(win[2]) if win[2] is not None else "end", len(comps))]
    same = [c for c in comps if c.old == c.new and len(c.new) == 1]
    lines.append("  %d unit(s) keep their name and change only in ranges: %s" % (len(same), ", ".join(c.new[0] for c in same[:6]) + (" ..." if len(same) > 6 else "")))
    for c in comps:
        if c in same:
            continue
        lines.append("  component %s%s" % (c.key(), "  STRADDLES windows " + "+".join(inp.windows[w][0] for w in c.windows_seen) if len(c.windows_seen) > 1 else ""))
        for n in c.old:
            lines.append("    - %s" % n)
        for n in c.new:
            lines.append("    + %s" % n)
        for n, e in c.crossings:
            lines.append("    ! %s: .text crosses the window edge %s" % (n, hx(e)))
    return "\n".join(lines)


def write_scratch(root, text, tag):
    d = os.path.join(root, "build", "tmp", "applysplits")
    os.makedirs(d, exist_ok=True)
    p = os.path.join(d, "splits-%s.txt" % re.sub(r"[^\w.]+", "_", tag))
    with open(p, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    return p


class Analysis:
    """splitcheck's invariants on the current file and on the candidate, computed once (about 3 s each), for `check` to compare an edited file with."""

    def __init__(self, inp, dol):
        self.inp, self.dol = inp, dol
        self.bctx, self.bres, _ = sc.analyse(inp.old, inp.symbols, dol, sda=(None, None))
        self.cctx, self.cres, _ = sc.analyse(inp.new, inp.symbols, dol, sda=(self.bctx.sda13, self.bctx.sda2))

    def check(self, edited):
        """`(rows, new failures)`: a row is `(invariant, FAIL current, FAIL edited, FAIL candidate)`; a new failure is a (unit, invariant) the edited file fails
        and neither the current file nor the candidate fails."""
        _ectx, eres, _ = sc.analyse(edited, self.inp.symbols, self.dol, sda=(self.bctx.sda13, self.bctx.sda2))
        bs, es, cs = self.bres.summary(), eres.summary(), self.cres.summary()
        rows = [(inv, bs[inv][sc.FAIL], es[inv][sc.FAIL], cs[inv][sc.FAIL]) for inv in sc.INVARIANTS]
        new = []
        for name, recs in eres.units.items():
            for inv, r in recs.items():
                if r["status"] == sc.FAIL and self.bres.units.get(name, {}).get(inv, {}).get("status") != sc.FAIL \
                        and self.cres.units.get(name, {}).get(inv, {}).get("status") != sc.FAIL:
                    new.append((name, inv, sc.hx(r["addr"]), r["finding"][:160]))
        return rows, new


def check_edited(inp, edited, symbols, dol):
    """`(rows, new failures)` of `edited` against the current file and the candidate (see `Analysis.check`)."""
    return Analysis(inp, dol).check(edited)


NL = chr(10)
DTK_BAD = re.compile(r"cycle|overlap|ends within|error|panic|failed", re.I)


def rewrite_config(cfg, root, paths):
    """`config.yml` text with the `object:`/`selfile:`/`symbols:`/`splits:` lines pointed at `paths`, each relative to `root` (the cwd of the run): dtk reads the `C:` of a
    drive letter as an archive member separator, so an absolute Windows path is refused (`C not found`)."""
    for key, val in paths.items():
        rel = os.path.relpath(val, root).replace(chr(92), "/")
        cfg = re.sub(r"(?m)^%s:.*$" % key, lambda _m, k=key, v=rel: "%s: %s" % (k, v), cfg)
    return cfg


def dtk_split(root, edited_text, tag, dtk=None, symbols_path=None):
    """Run `dtk dol split --no-update` on `edited_text` into a scratch dir: `(ok, seconds, tail of output)`.  A config copy points the splits at the scratch file;
    the DOL and the selfile are read from the tree, else from MAIN, by path."""
    import time
    import unitutil as uu
    cfg_src = os.path.join(root, "config", GAME, "config.yml")
    with open(cfg_src, encoding="utf-8", newline="") as fh:
        cfg = fh.read()
    scratch = os.path.join(root, "build", "tmp", "applysplits", "dtk-" + re.sub(r"[^\w.]+", "_", tag))
    os.makedirs(scratch, exist_ok=True)
    splits_p = os.path.join(scratch, "splits.txt")
    with open(splits_p, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(edited_text)
    sym_p = symbols_path or os.path.join(root, "config", GAME, "symbols.txt")
    dol_p = uu.resolve_input("orig/%s/sys/main.dol" % GAME, root)
    sel_p = uu.resolve_input("orig/%s/files/mh3.sel" % GAME, root)
    cfg = rewrite_config(cfg, root, {"object": dol_p, "selfile": sel_p, "symbols": sym_p, "splits": splits_p})
    cfg_p = os.path.join(scratch, "config.yml")
    with open(cfg_p, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(cfg)
    exe = dtk or uu.resolve_input(os.path.join("build", "tools", "dtk.exe" if os.name == "nt" else "dtk"), root)
    out_dir = os.path.join(scratch, "out")
    t0 = time.time()
    p = subprocess.run([exe, "dol", "split", "--no-update", cfg_p, out_dir], capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=root)
    dt = time.time() - t0
    out = (p.stdout or "") + (p.stderr or "")
    bad = [ln for ln in out.splitlines() if DTK_BAD.search(ln)]
    ok = p.returncode == 0 and not bad
    lines = [ln.strip() for ln in re.sub(r"\x1b\[[0-9;]*m", "", out).splitlines() if ln.strip()]
    return ok, dt, "\n".join([] if ok else lines[-3:])


UNSPLIT_RE = re.compile(r"Unsplit data in (\S+) from (\S+) \d+:0x([0-9A-Fa-f]+) to next split \d+:0x([0-9A-Fa-f]+)")


def dtk_probe(root, splits, tag, max_iter=60):
    """DIAGNOSTIC: run dtk on `splits` (a Splits), and for every `Unsplit data in <sec> from <unit> .. to next split ..` it refuses, give the gap to the unit before it and
    run again.  `(accepted, patches, last error)`; each patch is `(section, unit, gap start, gap end)`: a gap dtk will not turn into an `auto_*` unit, which only an
    owner for those bytes (a decision, not a rule) removes.  Any other refusal stops the probe."""
    patches = []
    sp = sc.Splits(list(splits.header), [copy_unit(u) for u in splits.units])
    tail = ""
    for _i in range(max_iter):
        ok, _dt, tail = dtk_split(root, sc.render_splits(sp), tag)
        if ok:
            return True, patches, ""
        m = UNSPLIT_RE.search(tail.replace("\n", " "))
        if not m:
            return False, patches, tail
        sec, unit, a, b = m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4), 16)
        u = sp.by_name().get(unit)
        rr = u.ranges.get(sec, []) if u else []
        hit = next((i for i, (x, y, z) in enumerate(rr) if y == a), None)
        if hit is None:
            return False, patches, tail
        rr[hit] = (rr[hit][0], b, rr[hit][2])
        patches.append((sec, unit, hx(a), hx(b)))
    return False, patches, tail


# ---- the manifest -------------------------------------------------------------------------------------------------------------------

def placeholder_findings(inp, c_names):
    """Candidate-unit names a reviewer must look at: a generated `fn_<addr>` stem (and whether the address is the unit's own `.text` start), the name-run remnants,
    the suffixed names a collision forced, a name an existing registered source already uses."""
    out = []
    taken = {u.name for u in inp.old.units}
    stems = {stem_of(u.name) for u in inp.new.units}
    for n in c_names:
        u = inp.nm[n]
        st = os.path.basename(stem_of(n))
        t = u.first(".text")
        m = PLACEHOLDER_RE.match(st)
        if m:
            wrong = t is not None and int(m.group(1), 16) != t
            if not wrong and n in taken:
                continue
            out.append({"unit": n, "kind": "placeholder-wrong-address" if wrong else "placeholder",
                        "note": ("stem names %s but the unit's .text starts at %s: rename to fn_%08X" % (m.group(1).upper(), hx(t), t)) if wrong else
                        "generated fn_<addr> stem: name the unit from what it holds before it has bodies"})
        if RUN_NAME_RE.match(st):
            out.append({"unit": n, "kind": "name-run-remnant", "note": "named from a run of effect names its parent held; the tail holds other TUs' names"})
        for suf in SUFFIX_NAMES:
            if st.endswith(suf) and (os.path.join(os.path.dirname(stem_of(n)), st[:-len(suf)])).replace("\\", "/") in stems:
                out.append({"unit": n, "kind": "suffix-collision", "note": "`%s` was added because the unit it is cut from keeps the plain name" % suf})
        if n not in taken and inp.tree.exists("src/" + n):
            out.append({"unit": n, "kind": "source-collision", "note": "src/%s already exists and belongs to no unit this component replaces" % n})
        if n not in taken:
            for e in SRC_EXT:
                other = stem_of(n) + e
                if other != n and other in taken:
                    out.append({"unit": n, "kind": "stem-collision", "note": "same stem as registered %s" % other})
    return out


def language_evidence(inp, cand_unit, absorbed_old):
    """The extension the unit's code asks for: C++ mangled function names in its `.text`, else the registered sources it absorbs."""
    funcs = inp.functions_in(code_ranges(cand_unit))
    mangled = [f[2] for f in funcs if MANGLED_RE.search(f[2])]
    exts = sorted({ext_of(o) for o in absorbed_old if ext_of(o)})
    if mangled or ".cpp" in exts or ".cp" in exts:
        rec = ".cpp"
    elif exts == [".c"]:
        rec = ".c"
    else:
        rec = ext_of(cand_unit.name) or ".cpp"
    mixed = len(exts) > 1
    return {"extensions_absorbed": exts, "mangled_functions": len(mangled), "mangled_examples": mangled[:3], "functions": len(funcs),
            "recommended": rec, "name_extension": ext_of(cand_unit.name), "mixed_c_cpp": mixed,
            "differs": bool(exts) and rec.replace(".cpp", ".cp") != ext_of(cand_unit.name).replace(".cpp", ".cp")}


def gained_data(inp, old_units, cand_unit):
    """Per data section: the bytes `cand_unit` holds that none of `old_units` held, with the map symbols in them."""
    out = {}
    syms = collections.defaultdict(list)
    for s in inp.symbols:
        if s["section"] in DATA:
            syms[s["section"]].append(s)
    for sec in DATA:
        mine = sorted(cand_unit.rs(sec))
        theirs = sorted(r for o in old_units for r in o.rs(sec))
        gain = subtract_ranges(mine, theirs)
        lost = subtract_ranges(theirs, mine)
        if gain or lost:
            names = [s["name"] for s in syms[sec] if any(a <= s["addr"] < b for a, b in gain)]
            out[sec] = {"gained_bytes": sum(b - a for a, b in gain), "lost_bytes": sum(b - a for a, b in lost),
                        "gained_symbols": len(names), "examples": names[:6]}
    return out


def old_record(inp, name):
    u = inp.om[name]
    conf = inp.conf.get(name)
    return {"name": name, "ranges": describe_unit(u)["ranges"], "source": "src/" + name, "source_exists": inp.tree.exists("src/" + name),
            "configure": ({"line": conf["line"], "state": conf["state"], "lib": conf["lib"], "text": conf["text"]} if conf else None),
            "registered": conf is not None, "matching": bool(conf and conf["state"] == "Matching"),
            "headers": inp.tree.headers_of(name)}


def build_component(inp, comp):
    """The manifest record of one component."""
    olds = [inp.om[n] for n in comp.old]
    cands = [inp.nm[n] for n in comp.new]
    new_names = set(comp.new)
    old_recs = {n: old_record(inp, n) for n in comp.old}
    cand_recs = []
    pairs, survivors = [], []
    for c in cands:
        c_code = code_ranges(c)
        over_text = []
        for o in olds:
            oc = code_ranges(o)
            if oc and overlap_bytes(oc, c_code):
                over_text.append(o)
        textless_hits = [o for o in olds if not has_code(o) and any(overlap_bytes(o.rs(s), c.rs(s)) for s in DATA)]
        donors = [o for o in olds if has_code(o) and o not in over_text and any(overlap_bytes(o.rs(s), c.rs(s)) for s in DATA)]
        new_unowned = sum(b - a for a, b in subtract_ranges(c_code, [r for o in olds for r in code_ranges(o)]))
        if not c_code:
            kind = "data-only"
        elif not over_text:
            kind = "new unit"
        elif len(over_text) == 1:
            o = over_text[0]
            same = sorted(o.rs(".text")) == sorted(c.rs(".text")) and sorted(o.rs(".init")) == sorted(c.rs(".init"))
            kind = "data gain" if o.name == c.name and same else "recut registered unit"
        else:
            kind = "fold of %d registered units" % len(over_text)
        absorbed_names = [o.name for o in over_text]
        funcs_by_src = []
        for o in over_text:
            share = [(max(a, x), min(b, y)) for a, b in code_ranges(o) for x, y in c_code if min(b, y) > max(a, x)]
            fl = inp.functions_in(share)
            text = inp.tree.read("src/" + o.name)
            rows = []
            for addr, size, fname in fl:
                found, line = find_definition(text, fname)
                rows.append({"name": fname, "addr": hx(addr), "size": size, "in_source": found, "line": line})
            whole = overlap_bytes(code_ranges(o), c_code) == sum(b - a for a, b in code_ranges(o))
            funcs_by_src.append({"source": "src/" + o.name, "unit": o.name, "covers": "whole" if whole else "part", "functions": rows,
                                 "in_source": sum(1 for r in rows if r["in_source"]), "source_exists": text is not None})
        recs = {
            "name": c.name, "source": "src/" + c.name, "module": os.path.dirname(c.name) or "(root)", "kind": kind,
            "code": [[hx(a), hx(b)] for a, b in c_code], "sections": {s: sum(b - a for a, b, _z in rr) for s, rr in c.ranges.items()},
            "absorbs": absorbed_names, "absorbs_data_only": [o.name for o in textless_hits], "data_donors": [o.name for o in donors],
            "merge_into": next((x["source"] for x in funcs_by_src if x["unit"] == c.name), funcs_by_src[0]["source"] if funcs_by_src else None),
            "new_unowned_text_bytes": new_unowned, "functions_by_source": funcs_by_src,
            "language": language_evidence(inp, c, absorbed_names + [o.name for o in textless_hits]),
            "gained_data": gained_data(inp, over_text + textless_hits + donors, c), "name_review": placeholder_findings(inp, [c.name]),
            "registered_now": c.name in inp.conf, "source_exists_now": inp.tree.exists("src/" + c.name),
        }
        cand_recs.append(recs)
        # --unit-rename: every old unit that feeds this candidate under another name
        for o in over_text + textless_hits:
            if o.name != c.name:
                pairs.append((o.name, c.name))
    for o in olds:
        if o.name in new_names:
            survivors.append(o.name)
        if not any(p[0] == o.name for p in pairs) and o.name not in new_names:
            pairs.append((o.name, ""))
    ren = ["%s=%s" % (stem_of(a), stem_of(b)) if b else "%s=" % stem_of(a) for a, b in sorted(set(pairs))]
    # Matching demotions: a Matching old unit that folds away or whose text changes
    demote = []
    gain = []
    for o in olds:
        rec = old_recs[o.name]
        if not rec["matching"]:
            continue
        if o.name not in new_names:
            into = sorted(c["name"] for c in cand_recs if o.name in c["absorbs"] or o.name in c["absorbs_data_only"])
            demote.append({"unit": o.name, "configure": rec["configure"], "into": into,
                           "reason": "folded into %s" % (", ".join(into) or "no unit") + ": a Matching unit that is no longer a TU of the candidate"})
            continue
        c = inp.nm[o.name]
        if sorted(c.rs(".text")) != sorted(o.rs(".text")) or sorted(c.rs(".init")) != sorted(o.rs(".init")):
            demote.append({"unit": o.name, "configure": rec["configure"], "into": [o.name],
                           "reason": "its .text range changes (%s -> %s)" % ("/".join(hx(a) + ".." + hx(b) for a, b in sorted(o.rs(".text"))[:2]),
                                                                           "/".join(hx(a) + ".." + hx(b) for a, b in sorted(c.rs(".text"))[:2]))})
            continue
        gd = gained_data(inp, [o], c)
        if gd:
            gain.append({"unit": o.name, "configure": rec["configure"], "sections": gd})
    return {"id": comp.key(), "window": inp.windows[comp.window][0], "windows_seen": [inp.windows[w][0] for w in comp.windows_seen],
            "straddles": [{"unit": n, "edge": hx(e)} for n, e in comp.crossings], "old": [old_recs[n] for n in comp.old], "candidates": cand_recs,
            "unit_renames": ren, "survivors": sorted(survivors), "matching_demotions": demote, "matching_data_gain": gain}


def symbol_review(inp, comps):
    """Map rows a reviewer should look at for the window's candidate units: a function named after another unit's stem that lies in this one."""
    owner = []
    for c in comps:
        for n in c.new:
            for a, b in code_ranges(inp.nm[n]):
                owner.append((a, b, n))
    owner.sort()
    starts = [o[0] for o in owner]
    stems = {n: os.path.basename(stem_of(n)) for _a, _b, n in owner}
    out = []
    for addr, size, fname in inp.functions()[0]:
        k = bisect.bisect_right(starts, addr) - 1
        if k < 0 or addr >= owner[k][1]:
            continue
        holder = owner[k][2]
        for n, st in sorted(stems.items(), key=lambda kv: -len(kv[1])):
            if n != holder and len(st) >= 10 and not PLACEHOLDER_RE.match(st) and fname.lower().startswith(st.lower()) \
                    and not fname.lower().startswith(stems[holder].lower()):
                out.append({"symbol": fname, "addr": hx(addr), "lies_in": holder, "named_after": n,
                            "note": "named after %s but lies in %s (rule 7: rename the row, or move the edge)" % (n, holder)})
                break
    return out


def estimate_size(comps_rec):
    funcs = sum(len(s["functions"]) for c in comps_rec for cand in c["candidates"] if cand["kind"] != "data gain" for s in cand["functions_by_source"])
    units = sum(len(c["candidates"]) for c in comps_rec)
    srcs = len({o["name"] for c in comps_rec for o in c["old"] if o["source_exists"]})
    heavy = sum(1 for c in comps_rec for cand in c["candidates"] if cand["kind"] not in ("data gain",))
    score = funcs + 6 * heavy + srcs
    label = "small" if score < 150 else "medium" if score < 600 else "large" if score < 1500 else "very large"
    return {"functions_moved": funcs, "units_in_window": units, "units_needing_work": heavy, "registered_sources_touched": srcs,
            "score": score, "label": label, "note": "heuristic: functions in the sources a unit takes over (a data gain moves none) + 6 x units that are not a data gain + registered sources touched; < 150 small, < 600 medium, < 1500 large"}


def build_manifest(inp, widx, checks=None):
    ident, moved, comps = components_of(inp)
    mine = [c for c in comps if c.window == widx]
    recs = [build_component(inp, c) for c in mine]
    win = inp.windows[widx]
    kinds = collections.Counter(c["kind"].split(" of ")[0] if c["kind"].startswith("fold") else c["kind"] for r in recs for c in r["candidates"])
    problems = []
    for r in recs:
        if r["straddles"]:
            problems.append({"kind": "unit crosses a window edge", "detail": "; ".join("%s at %s" % (s["unit"], s["edge"]) for s in r["straddles"]), "component": r["id"]})
        if len(r["windows_seen"]) > 1:
            problems.append({"kind": "component spans windows", "detail": "component %s has units in %s: applied with window %s" % (r["id"], "+".join(r["windows_seen"]), r["window"]),
                             "component": r["id"]})
        for o in r["old"]:
            if o["registered"] and not o["source_exists"]:
                problems.append({"kind": "registered unit without a source", "detail": o["name"], "component": r["id"]})
            if not o["registered"]:
                problems.append({"kind": "splits.txt unit with no configure.py row", "detail": o["name"], "component": r["id"]})
        for c in r["candidates"]:
            lang = c["language"]
            if lang["differs"]:
                problems.append({"kind": "language", "detail": "%s is named %s but the evidence says %s (%d mangled functions, absorbs %s)" % (
                    c["name"], lang["name_extension"], lang["recommended"], lang["mangled_functions"], ", ".join(lang["extensions_absorbed"]) or "-"), "component": r["id"]})
            elif lang["mixed_c_cpp"]:
                problems.append({"kind": "language", "detail": "%s merges .c and .cpp sources; takes %s" % (c["name"], lang["recommended"]), "component": r["id"]})
    for n in sorted(moved):
        if any(n in c.old for c in mine):
            problems.append({"kind": "identical unit moved in file order", "detail": n, "component": None})
    problems += [{"kind": "candidate lint", "detail": x, "component": None} for x in inp.cand_issues]
    win_new = {n for c in mine for n in c.new}
    for sec, unit, a, b in ((checks or {}).get("probe") or {}).get("patches", []):
        if unit in win_new:
            problems.append({"kind": "dtk refuses an unowned gap (`Unsplit data`)", "detail": "%s %s..%s after %s: nothing in the candidate owns these bytes and dtk will not make "
                             "them an auto unit; give them to a unit" % (sec, a, b, unit), "component": None})
    fixes = [[f[0], f[1], hx(f[2]), hx(f[3])] for f in inp.fixes if f[0] in win_new]
    for b in inp.blocked:
        if b[0] in win_new:
            problems.append({"kind": "dtk cannot split here: a range boundary is off the 4-byte alignment at an unowned symbol",
                             "detail": "%s %s boundary %s (symbol at %s): decide which unit owns the symbol" % (b[0], b[1], hx(b[2]), hx(b[3])), "component": None})
    renames = [x for r in recs for c in r["candidates"] for x in c["name_review"]]
    sym_rows = symbol_review(inp, mine)
    demos = [d for r in recs for d in r["matching_demotions"]]
    gains = [d for r in recs for d in r["matching_data_gain"]]
    rename_pairs = sorted({p for r in recs for p in r["unit_renames"]})
    absorbed_registered = sorted({o["name"] for r in recs for o in r["old"] if o["registered"]})
    land_units = sorted({stem_of(c["name"]) for r in recs for c in r["candidates"] if c["kind"] != "data gain"})
    summary = {
        "components": len(recs), "units_replaced": sum(len(r["old"]) for r in recs), "units_in_candidate": sum(len(r["candidates"]) for r in recs),
        "kinds": dict(kinds), "registered_sources_touched": len([o for r in recs for o in r["old"] if o["source_exists"]]),
        "registered_units_removed": len([o for r in recs for o in r["old"] if o["name"] not in {c["name"] for rr in recs for c in rr["candidates"]}]),
        "matching_demotions": len(demos), "matching_data_gain": len(gains), "unit_renames": len(rename_pairs), "name_reviews": len(renames),
        "symbol_reviews": len(sym_rows), "problems": len(problems), "alignment_fixes": len(fixes), "size": estimate_size(recs),
    }
    return {"window": win[0], "range": [hx(win[1]), hx(win[2]) if win[2] is not None else None], "summary": summary, "components": recs,
            "unit_renames": rename_pairs, "name_reviews": renames, "symbol_reviews": sym_rows, "matching_demotions": demos, "matching_data_gain": gains,
            "problems": problems, "checks": checks or {}, "registered_units_absorbed": absorbed_registered,
            "alignment_fixes": fixes, "land_units": land_units}


def md_manifest(man):
    s = man["summary"]
    L = []
    w = man["window"]
    L.append("# Phase 4 manifest, window %s (%s .. %s)" % (w, man["range"][0], man["range"][1] or "end"))
    L.append("")
    L.append("Generated by `python tools/splits/applysplits.py manifest --window %s` against the tree it was cut from; the plan and the work list are state-dependent "
             "(a component already applied no longer appears), regenerate rather than edit." % w)
    L.append("")
    L.append("## Summary")
    L.append("")
    L.append("| | |")
    L.append("| --- | ---: |")
    L.append("| components | %d |" % s["components"])
    L.append("| old unit blocks replaced | %d |" % s["units_replaced"])
    L.append("| candidate unit blocks written | %d |" % s["units_in_candidate"])
    for k, v in sorted(s["kinds"].items()):
        L.append("| &nbsp;&nbsp;%s | %d |" % (k, v))
    L.append("| registered sources touched | %d |" % s["registered_sources_touched"])
    L.append("| registered units removed (folded away) | %d |" % s["registered_units_removed"])
    L.append("| Matching demotions | %d |" % s["matching_demotions"])
    L.append("| Matching units that gain data (define or demote) | %d |" % s["matching_data_gain"])
    L.append("| `--unit-rename` pairs | %d |" % s["unit_renames"])
    L.append("| unit names to review | %d |" % s["name_reviews"])
    L.append("| symbols.txt rows to review | %d |" % s["symbol_reviews"])
    sz = s["size"]
    L.append("| estimated lane size | %s (score %d: %d functions to move, %d units needing work) |" % (sz["label"], sz["score"], sz["functions_moved"], sz["units_needing_work"]))
    L.append("| candidate ranges extended over alignment padding (dtk needs aligned splits) | %d |" % s["alignment_fixes"])
    L.append("| problems the plan cannot express | %d |" % s["problems"])
    L.append("")
    chk = man.get("checks") or {}
    if chk:
        L.append("## Checks on the edited splits.txt")
        L.append("")
        L.append("`alone` = this window applied to the current file, nothing else; `cumulative` = windows a..%s applied in order (the landing order); `probe` = the diagnostic run "
                 "with the candidate's dtk blockers given an owner (see README)." % w)
        L.append("")
        for key in ("alone", "cumulative"):
            c = chk.get(key)
            if not c:
                continue
            sc_ = c.get("splitcheck")
            if sc_:
                bad = [(inv, b, e, cc) for inv, b, e, cc in sc_["rows"] if e != cc or e != b]
                L.append("* **%s, splitcheck**: new failures (failing in neither the current file nor the candidate) %d; invariants that differ from both: %s" % (
                    key, len(sc_["new"]), ", ".join("%s %d/%d/%d" % x for x in bad) or "none"))
                for n in sc_["new"][:8]:
                    L.append("  * `%s` [%s] %s %s" % (n[0], n[1], n[2], n[3]))
            d = c.get("dtk")
            if d:
                L.append("* **%s, `dtk dol split --no-update`**: %s in %.1f s%s" % (key, "ACCEPTED" if d["ok"] else "REFUSED", d["seconds"], "" if d["ok"] else ": " + d["tail"].replace("\n", " | ")))
        p = chk.get("probe")
        if p:
            L.append("* **probe, dtk with the candidate's blockers given an owner**: %s%s" % ("ACCEPTED" if p["accepted"] else "REFUSED: " + p["tail"].replace("\n", " | "),
                                                                                     ("; %d `.sdata2`-style gap(s) given to the unit before: %s" % (len(p["patches"]), ", ".join("%s %s %s..%s" % (x[0], x[1], x[2], x[3]) for x in p["patches"]))) if p["patches"] else ""))
        L.append("")
    data_gain_only = [(r, c) for r in man["components"] for c in r["candidates"] if c["kind"] == "data gain"]
    heavy = [r for r in man["components"] if any(c["kind"] != "data gain" for c in r["candidates"]) or len(r["old"]) != len([1 for c in r["candidates"] if c["name"] in {o["name"] for o in r["old"]}])]
    L.append("## Components that need source work (%d)" % len(heavy))
    L.append("")
    for r in heavy:
        L.append("### component %s: %s" % (r["id"], ", ".join("`%s`" % c["name"] for c in r["candidates"]) or "(no candidate unit: the old blocks are dropped)"))
        L.append("")
        if r["straddles"] or len(r["windows_seen"]) > 1:
            L.append("* **straddles**: units in windows %s%s" % ("+".join(r["windows_seen"]), "; " + ", ".join("%s crosses %s" % (x["unit"], x["edge"]) for x in r["straddles"]) if r["straddles"] else ""))
        for c in r["candidates"]:
            lang = c["language"]
            L.append("* **`%s`** - %s; code %s; sections %s" % (c["name"], c["kind"], ", ".join("%s..%s" % tuple(x) for x in c["code"][:3]) or "none",
                                                                  ", ".join("%s 0x%X" % kv for kv in sorted(c["sections"].items())) or "-"))
            L.append("  * file `%s` as `%s` (language: %d mangled functions%s; absorbs %s%s)%s" % (
                c["source"], lang["recommended"], lang["mangled_functions"], ", name says " + lang["name_extension"] if lang["differs"] else "",
                ", ".join(lang["extensions_absorbed"]) or "no registered source", ", C and C++ mixed" if lang["mixed_c_cpp"] else "",
                "; start from `%s`" % c["merge_into"] if c["merge_into"] and c["merge_into"] != c["source"] else ""))
            for fs in c["functions_by_source"]:
                rows = fs["functions"]
                span = "%s..%s" % (rows[0]["addr"], rows[-1]["addr"]) if rows else "-"
                L.append("  * from `%s` (%s): %d functions %s, %d found in the source%s" % (
                    fs["source"], fs["covers"], len(rows), span, fs["in_source"], "" if fs["source_exists"] else " (no source file)"))
            if c["absorbs_data_only"]:
                L.append("  * data-only units absorbed: %s" % ", ".join("`%s`" % x for x in c["absorbs_data_only"]))
            if c["data_donors"]:
                L.append("  * data taken from: %s" % ", ".join("`%s`" % x for x in c["data_donors"]))
            if c["new_unowned_text_bytes"]:
                L.append("  * %d bytes of `.text` no registered unit held" % c["new_unowned_text_bytes"])
            if c["gained_data"]:
                L.append("  * data the source must define or leave extern: " + "; ".join(
                    "%s +0x%X (%d symbols%s)" % (sec, g["gained_bytes"], g["gained_symbols"], ", -0x%X" % g["lost_bytes"] if g["lost_bytes"] else "") for sec, g in sorted(c["gained_data"].items())))
        for o in r["old"]:
            cfg = o["configure"]
            L.append("* old `%s`: %s%s; headers %s" % (o["name"], ("`%s` (configure.py:%d, %s)" % (cfg["text"], cfg["line"], cfg["state"])) if cfg else "no configure.py row",
                                                       "" if o["source_exists"] else ", source missing",
                                                       ", ".join("`%s` (included by %d)" % (h["path"], len(h["included_by"])) for h in o["headers"]) or "none"))
        if r["unit_renames"]:
            L.append("* `--unit-rename %s`%s" % (" --unit-rename ".join(r["unit_renames"]), "; survivors: " + ", ".join(r["survivors"]) if r["survivors"] else ""))
        L.append("")
    L.append("## Data gains of registered units (%d)" % len(data_gain_only))
    L.append("")
    L.append("| unit | state | gained data | action |")
    L.append("| --- | --- | --- | --- |")
    for r, c in data_gain_only:
        o = next((x for x in r["old"] if x["name"] == c["name"]), None)
        L.append("| `%s` | %s | %s | %s |" % (c["name"], o["configure"]["state"] if o and o["configure"] else "unregistered",
                                              ", ".join("%s +0x%X" % (sec, g["gained_bytes"]) for sec, g in sorted(c["gained_data"].items())) or "extab/ctors only",
                                              "define the data in the source and re-measure, or demote" if o and o["matching"] else "none (NonMatching)"))
    L.append("")
    L.append("## Matching demotions (%d)" % len(man["matching_demotions"]))
    L.append("")
    for d in man["matching_demotions"]:
        L.append("* `%s` (%s): %s -> flip the merged unit to `NonMatching`: %s" % (d["unit"], d["configure"]["text"] if d["configure"] else "?", d["reason"], ", ".join("`%s`" % x for x in d["into"])))
    L.append("")
    L.append("## Matching units that gain data (%d): define the data in the source and re-measure (playbook 23/29), or demote" % len(man["matching_data_gain"]))
    L.append("")
    for d in man["matching_data_gain"]:
        L.append("* `%s`: %s" % (d["unit"], "; ".join("%s +0x%X (%d symbols)" % (sec, g["gained_bytes"], g["gained_symbols"]) for sec, g in sorted(d["sections"].items()))))
    L.append("")
    L.append("## Names and map rows to review")
    L.append("")
    for x in man["name_reviews"]:
        L.append("* `%s` [%s]: %s" % (x["unit"], x["kind"], x["note"]))
    for x in man["symbol_reviews"]:
        L.append("* map `%s` %s: %s" % (x["symbol"], x["addr"], x["note"]))
    L.append("")
    L.append("## Landing: `--units` and `--unit-rename`")
    L.append("")
    L.append("```")
    L.append("--units " + ",".join(man["land_units"]))
    for p in man["unit_renames"]:
        L.append("--unit-rename " + p)
    L.append("```")
    L.append("")
    L.append("## Problems the plan cannot express (%d)" % len(man["problems"]))
    L.append("")
    for p in man["problems"]:
        L.append("* %s: %s" % (p["kind"], p["detail"]))
    L.append("")
    return "\n".join(L)


# ---- verify ----------------------------------------------------------------------------------------------------------------------------

class GitBase:
    """The base a lane was cut from, read through `git show`: splits, configure.py and old sources."""

    def __init__(self, root, ref):
        self.root, self.ref = root, ref

    def _git(self, *args):
        p = subprocess.run(["git", *args], capture_output=True, text=True, cwd=self.root, encoding="utf-8", errors="replace")
        return p.stdout if p.returncode == 0 else None

    def splits_text(self):
        return self._git("show", "%s:config/%s/splits.txt" % (self.ref, GAME))

    def configure_text(self):
        return self._git("show", "%s:configure.py" % self.ref)

    def read(self, rel):
        return self._git("show", "%s:%s" % (self.ref, rel))

    def exists(self, rel):
        return self.read(rel) is not None

    def headers_of(self, unit_name):
        return []


class DirBase:
    """A base kept in a directory (a fixture, or a checkout): the same interface."""

    def __init__(self, root):
        self.t = Tree(root)
        self.root = root

    def splits_text(self):
        return self.t.read("config/%s/splits.txt" % GAME)

    def configure_text(self):
        return self.t.read("configure.py")

    def read(self, rel):
        return self.t.read(rel)

    def exists(self, rel):
        return self.t.exists(rel)

    def headers_of(self, unit_name):
        return self.t.headers_of(unit_name)


def count_definitions(text, name):
    """How many times `text` defines `name` (any spelling of it)."""
    return len(definition_lines(text or "", name)[0])


def verify_window(inp_base, widx, tree, cur_splits_text, cur_configure_text, base, no_build=False, allow_unregistered=False, before_report=None,
                  run=None, ninja="ninja", allow_regression=()):
    """The `verify` rows: `[(row, ok, detail)]`.  `inp_base` is the plan's inputs built from the BASE (splits, configure, sources of the commit the lane was cut from);
    `tree` is the lane's tree and `cur_*` its splits.txt / configure.py."""
    rows = []

    def add(name, ok, detail):
        rows.append((name, ok, detail))

    ident, moved, comps = components_of(inp_base)
    mine = [c for c in comps if c.window == widx]
    expected = apply_components(inp_base.old, inp_base.new, mine)
    exp_text = sc.render_splits(expected)
    cur = sc.parse_splits(cur_splits_text)
    ok = cur_splits_text == exp_text or sc.render_splits(cur) == exp_text
    if ok:
        add("splits.txt equals the plan", True, "%d units, %d component(s) of window %s applied" % (len(expected.units), len(mine), inp_base.windows[widx][0]))
    else:
        en, cn = [u.name for u in expected.units], [u.name for u in cur.units]
        miss, extra = [n for n in en if n not in cn], [n for n in cn if n not in en]
        wrong = [n for n in en if n in cn and unit_sig(expected.by_name()[n]) != unit_sig(cur.by_name()[n])]
        add("splits.txt equals the plan", False, "missing %s; unexpected %s; ranges differ %s; order %s" % (
            miss[:4], extra[:4], wrong[:4], "same" if [n for n in en if n in cn] == [n for n in cn if n in en] else "differs"))
    win_cands = [n for c in mine for n in c.new]
    conf = parse_configure(cur_configure_text)
    prob = []
    for n in win_cands:
        reg, src = n in conf, tree.exists("src/" + n)
        if reg and not src:
            prob.append("%s: registered without a source" % n)
        elif src and not reg:
            prob.append("%s: has a source but no Object(...) row" % n)
        elif not reg and not src and not allow_unregistered:
            prob.append("%s: neither registered nor has a source (a stub with a header comment + a NonMatching Object is required)" % n)
    add("every unit of the window is registered iff it has a source", not prob, "; ".join(prob[:6]) or "%d units checked" % len(win_cands))
    keep = {n for c in mine for n in c.new}
    gone = [(n, c.idx) for c in mine for n in c.old if n not in keep]
    prob = []
    for n, _i in gone:
        if tree.exists("src/" + n):
            prob.append("%s: source still present" % n)
        if n in conf:
            prob.append("%s: still has an Object row" % n)
    add("every folded registered unit's source is gone and unregistered", not prob, "; ".join(prob[:6]) or "%d folded unit(s)" % len(gone))
    prob, checked = [], 0
    comp_recs = {c.idx: build_component(inp_base, c) for c in mine}
    for c in mine:
        for n in c.new:
            comp_rec = comp_recs[c.idx]
            cand = next(x for x in comp_rec["candidates"] if x["name"] == n)
            text = tree.read("src/" + n)
            if text is None:
                continue
            for fs in cand["functions_by_source"]:
                for f in fs["functions"]:
                    if not f["in_source"]:
                        continue
                    checked += 1
                    k = count_definitions(text, f["name"])
                    if k != 1:
                        prob.append("%s: %s defined %d times" % (n, f["name"], k))
    add("every function of a folded source appears exactly once in the merged file", not prob, "; ".join(prob[:6]) or "%d function(s) checked" % checked)
    stems = collections.Counter(stem_of(p) for p in conf)
    dup = sorted(s for s, k in stems.items() if k > 1)
    paths = collections.Counter(p["count"] for p in conf.values() if p["count"] > 1)
    names_dup = [u.name for u in cur.units if [x.name for x in cur.units].count(u.name) > 1]
    add("no unit name collides", not (dup or paths or names_dup), "stems used twice: %s; Object rows repeated: %d; splits names repeated: %s" % (dup[:4], sum(paths.values()), names_dup[:4]))
    demoted = [d["unit"] for c in mine for d in comp_recs[c.idx]["matching_demotions"]]
    targets = sorted({t for c in mine for d in comp_recs[c.idx]["matching_demotions"] for t in d["into"]})
    bad = [t for t in targets if t in conf and conf[t]["state"] == "Matching"]
    add("configure.py has no Matching on a demoted unit", not bad, "%d demoted unit(s) -> %d target(s)%s" % (len(demoted), len(targets), "; still Matching: %s" % bad[:5] if bad else ""))
    if no_build:
        add("the window's units compile / ok green / no regression", True, "skipped (--no-build)")
        return rows
    run = run or (lambda cmd: subprocess.run(cmd, capture_output=True, text=True, cwd=tree.root, encoding="utf-8", errors="replace"))
    import verifyunit as vu
    objs = [vu.src_object_rel(n).replace("\\", "/") for n in win_cands if n in conf]
    if objs:
        p = run([ninja, "-k", "0"] + objs)
        out = (p.stdout or "") + (p.stderr or "")
        failed = [ln for ln in out.splitlines() if ln.startswith("FAILED")]
        add("the window's units compile", p.returncode == 0 and not failed, "%d object(s); %d failed %s" % (len(objs), len(failed), failed[:3]))
    p = run([ninja, "build/RMHE08/ok"])
    out = (p.stdout or "") + (p.stderr or "")
    failed = [ln for ln in out.splitlines() if ln.startswith("FAILED")]
    add("ninja build/RMHE08/ok is green", p.returncode == 0 and not failed, "exit %d, %d FAILED" % (p.returncode, len(failed)))
    add("no function scores lower than before the window", *regression_row(tree, before_report, run, ninja, allow_regression))
    return rows


def function_scores(report):
    """`name -> percent` for every function of a `report.json` (a function without `fuzzy_match_percent` is 0 %)."""
    out = {}
    for u in report.get("units", []):
        for f in u.get("functions") or []:
            if f.get("name"):
                out[f["name"]] = float(f.get("fuzzy_match_percent", f.get("match_percent", 0.0)) or 0.0)
    return out


def regression_rows(before, after, allow=()):
    """Functions that scored before and score lower (or are gone) after: `[(name, before, after)]`."""
    out = []
    for n, b in before.items():
        if any(a in n for a in allow):
            continue
        a = after.get(n)
        if b > 0 and (a is None or a < b - 1e-9):
            out.append((n, b, a))
    return out


def regression_row(tree, before_path, run, ninja, allow):
    if not before_path or not os.path.isfile(before_path):
        return False, "no --before-report (save build/RMHE08/report.json before the window edit; README step 1)"
    p = run([ninja, "build/RMHE08/report.json"])
    rp = os.path.join(tree.root, "build", "RMHE08", "report.json")
    if p.returncode != 0 or not os.path.isfile(rp):
        return False, "report.json did not build"
    with open(before_path, encoding="utf-8") as fh:
        b = function_scores(json.load(fh))
    with open(rp, encoding="utf-8") as fh:
        a = function_scores(json.load(fh))
    rows = regression_rows(b, a, allow)
    return not rows, "%d function(s) compared; %d lower: %s" % (len(b), len(rows), ", ".join("%s %.1f->%s" % (n, x, "gone" if y is None else "%.1f" % y) for n, x, y in rows[:5]))


# ---- commands ------------------------------------------------------------------------------------------------------------------------------

def cmd_plan(args, inp=None, dol=None):
    root = sc.tree_root()
    if inp is None:
        inp, dol = load_inputs(root)
    widx = resolve_window(inp, args.window)
    comps, edited = plan_for(inp, widx)
    print(plan_text(inp, widx, comps))
    text = sc.render_splits(edited)
    scratch = write_scratch(root, text, inp.windows[widx][0])
    print("edited splits.txt: %s (%d units; the file is %d bytes, current %d)" % (scratch, len(edited.units), len(text), len(sc.render_splits(inp.old))))
    result = {"plan": plan_json(inp, widx, comps, edited)}
    if args.check:
        rows, new = check_edited(inp, edited, inp.symbols, dol)
        print("%-11s %10s %10s %10s" % ("invariant", "FAIL cur", "FAIL edit", "FAIL cand"))
        for inv, b, e, c in rows:
            print("%-11s %10d %10d %10d" % (inv, b, e, c))
        print("new failures (neither current nor candidate fails them): %d" % len(new))
        for n in new[:10]:
            print("  [%s] %s @ %s  %s" % (n[1], n[0], n[2], n[3]))
        result["splitcheck"] = {"rows": rows, "new": new}
    if args.dtk:
        ok, dt, tail = dtk_split(root, text, inp.windows[widx][0])
        print("dtk dol split --no-update: %s in %.1f s" % ("ACCEPTED" if ok else "REFUSED", dt))
        if not ok:
            print(tail)
        result["dtk"] = {"ok": ok, "seconds": dt, "tail": tail}
        if not ok and args.probe:
            dinp = inp.diag()
            _c, dedited = plan_for(dinp, widx)
            acc, patches, ptail = dtk_probe(root, dedited, inp.windows[widx][0] + "-probe")
            print("probe (the candidate's dtk blockers given an owner): %s%s" % ("ACCEPTED" if acc else "REFUSED " + ptail.replace(NL, " | "),
                                                                                   ("; gaps given to the unit before: %s" % patches) if patches else ""))
            result["probe"] = {"accepted": acc, "patches": patches, "tail": ptail}
    if args.json:
        with open(args.json, "w", encoding="utf-8", newline="\n") as fh:
            json.dump(result, fh, indent=1)
    bad = (args.check and result["splitcheck"]["new"]) or (args.dtk and not result["dtk"]["ok"])
    return 1 if bad else 0


def cmd_freeze(args):
    root = sc.tree_root()
    dst = os.path.join(root, BASELINE_FILE)
    if os.path.isfile(dst) and not args.force:
        print("refused: %s exists (the candidate is rendered over it; `--force` only for a deliberate re-baseline)" % BASELINE_FILE)
        return 2
    with open(os.path.join(root, "config", GAME, "splits.txt"), encoding="utf-8", newline="") as fh:
        text = fh.read()
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8", newline=NL) as fh:
        fh.write(text)
    print("wrote %s (%d bytes)" % (BASELINE_FILE, len(text)))
    return 0


def cmd_apply(args):
    root = sc.tree_root()
    inp, _dol = load_inputs(root)
    widx = resolve_window(inp, args.window)
    path = os.path.join(root, "config", GAME, "splits.txt")
    with open(path, encoding="utf-8", newline="") as fh:
        cur = fh.read()
    if sc.render_splits(sc.parse_splits(cur)) != cur:
        print("refused: the current splits.txt does not round-trip through the renderer (apply would churn untouched blocks); normalise it first")
        return 2
    comps, edited = plan_for(inp, widx)
    if not comps:
        print("window %s: nothing to apply (the tree already holds the candidate for it)" % inp.windows[widx][0])
        return 0
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(sc.render_splits(edited))
    print("window %s: %d component(s) applied, %d -> %d units" % (inp.windows[widx][0], len(comps), len(inp.old.units), len(edited.units)))
    return 0


def cmd_manifest(args, inp=None, dol=None, checks=None):
    root = sc.tree_root()
    if inp is None:
        inp, dol = load_inputs(root)
    widx = resolve_window(inp, args.window)
    man = build_manifest(inp, widx, checks)
    if args.json:
        with open(args.json, "w", encoding="utf-8", newline="\n") as fh:
            json.dump(man, fh, indent=1)
    if args.md:
        with open(args.md, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(md_manifest(man))
    if not args.json and not args.md:
        print(md_manifest(man))
    return 0


def cmd_all(args):
    root = sc.tree_root()
    inp, dol = load_inputs(root)
    out_dir = os.path.join(root, args.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    an = Analysis(inp, dol)
    diag = inp.diag() if args.dtk else None
    cum, dcum = inp.old, (diag.old if diag else None)
    for widx, (name, _lo, _hi) in enumerate(inp.windows):
        comps, edited = plan_for(inp, widx)
        rows, new = an.check(edited)
        checks = {"alone": {"splitcheck": {"rows": rows, "new": new}}}
        cum = apply_components(cum, inp.new, [c for c in diff_units(cum, inp.new, inp.windows)[2] if c.window == widx])
        crows, cnew = an.check(cum)
        checks["cumulative"] = {"splitcheck": {"rows": crows, "new": cnew}}
        if args.dtk:
            ok, dt, tail = dtk_split(root, sc.render_splits(edited), name)
            checks["alone"]["dtk"] = {"ok": ok, "seconds": dt, "tail": tail}
            _c, dedited = plan_for(diag, widx)
            acc, patches, ptail = dtk_probe(root, dedited, name + "-probe")
            checks["probe"] = {"accepted": acc, "patches": patches, "tail": ptail}
            dcum = apply_components(dcum, diag.new, [c for c in diff_units(dcum, diag.new, diag.windows)[2] if c.window == widx])
            acc2, patches2, ptail2 = dtk_probe(root, dcum, name + "-cum")
            checks["cumulative"]["dtk_probe"] = {"accepted": acc2, "patches": patches2, "tail": ptail2}
        man = build_manifest(inp, widx, checks)
        with open(os.path.join(out_dir, "manifest-%s.json" % name), "w", encoding="utf-8", newline=NL) as fh:
            json.dump(man, fh, separators=(",", ":"))
        with open(os.path.join(out_dir, "manifest-%s.md" % name), "w", encoding="utf-8", newline=NL) as fh:
            fh.write(md_manifest(man))
        s = man["summary"]
        al = checks["alone"].get("dtk")
        print("window %-3s components %3d  old %3d -> new %3d  kinds %s  sources %3d  demotions %2d  renames %2d  size %s  problems %2d  new-failures alone %d cumulative %d  dtk alone %s probe %s cumulative-probe %s" % (
            name, s["components"], s["units_replaced"], s["units_in_candidate"], s["kinds"], s["registered_sources_touched"], s["matching_demotions"], s["unit_renames"],
            s["size"]["label"], s["problems"], len(new), len(cnew), "-" if al is None else ("ok" if al["ok"] else "REFUSED"),
            "-" if "probe" not in checks else ("ok" if checks["probe"]["accepted"] else "REFUSED"),
            "-" if "probe" not in checks else ("ok" if checks["cumulative"]["dtk_probe"]["accepted"] else "REFUSED")))
    full = sc.render_splits(cum) == sc.render_splits(inp.new)
    print("all six windows applied in turn == the candidate: %s" % full)
    import matchinggain as mg
    rows, absent = mg.gains(mg.matching_units(inp.configure_text), inp.om, inp.nm)
    gain_units = sorted({r[0] for r in rows})
    gone_units = sorted(a[0] for a in absent if a[1] == "candidate")
    mine_gain, mine_demote, mine_gone = set(), set(), set()
    for widx in range(len(inp.windows)):
        man = json.load(open(os.path.join(out_dir, "manifest-%s.json" % inp.windows[widx][0]), encoding="utf-8"))
        mine_gain |= {d["unit"] for d in man["matching_data_gain"]}
        mine_demote |= {d["unit"] for d in man["matching_demotions"]}
    print("matchinggain.py: %d Matching units gain data %s, %d absent under their name" % (len(gain_units), "", len(gone_units)))
    print("manifests: gain %d (same set: %s), demoted %d (the absent ones are all demoted: %s; recut Matching units demoted besides: %s)" % (
        len(mine_gain), mine_gain == set(gain_units), len(mine_demote), set(gone_units) <= mine_demote, sorted(mine_demote - set(gone_units))))
    return 0 if full else 1


def cmd_verify(args):
    root = sc.tree_root()
    ref = args.base
    if not ref:
        p = subprocess.run(["git", "merge-base", "HEAD", "main"], capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=root)
        ref = p.stdout.strip() if p.returncode == 0 else "main"
    base = GitBase(root, ref)
    inp, _dol = load_inputs(root, splits_text=base.splits_text(), configure_text=base.configure_text(), tree=base)
    widx = resolve_window(inp, args.window)
    with open(os.path.join(root, "config", GAME, "splits.txt"), encoding="utf-8", newline="") as fh:
        cur_splits = fh.read()
    with open(os.path.join(root, "configure.py"), encoding="utf-8") as fh:
        cur_conf = fh.read()
    rows = verify_window(inp, widx, Tree(root), cur_splits, cur_conf, base, args.no_build, args.allow_unregistered, args.before_report,
                         allow_regression=args.allow_regression)
    bad = 0
    for name, ok, detail in rows:
        print("%s  %s - %s" % ("PASS" if ok else "FAIL", name, detail))
        bad += 0 if ok else 1
    print("base %s: %d row(s), %d failed" % (ref, len(rows), bad))
    return 1 if bad else 0


# ---- selftest ------------------------------------------------------------------------------------------------------------------------------

def selftest():
    import shutil
    fails = []

    count = [0]

    def check(what, got, want):
        count[0] += 1
        if got != want:
            fails.append("%s: got %r, want %r" % (what, got, want))

    def U(name, **secs):
        return sc.Unit(name, "", {s: [(a, b, "") for a, b in (rr if isinstance(rr, list) else [rr])] for s, rr in secs.items()})

    def put(root, rel, body):
        os.makedirs(os.path.dirname(os.path.join(root, *rel.split("/"))), exist_ok=True)
        with open(os.path.join(root, *rel.split("/")), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(body)

    W = (("a", 0x1000, 0x2000), ("b", 0x2000, 0x3000), ("c", 0x3000, None))
    header = ["Sections:", "\t.text       type:code align:32"]
    old = sc.Splits(header, [
        U("a1.c", **{".text": (0x1000, 0x1100), ".data": (0x8000, 0x8010)}),
        U("a2.cpp", **{".text": (0x1100, 0x1200)}),
        U("a3.cpp", **{".text": (0x1200, 0x1300), ".data": (0x8010, 0x8020)}),
        U("b1.cpp", **{".text": (0x2000, 0x2100)}),
        U("d_only.cpp", **{".sdata": (0x9000, 0x9010)}),
        U("b2.cpp", **{".text": (0x2100, 0x2200)}),
        U("c1.cpp", **{".text": (0x3000, 0x3100)}),
    ])
    new = sc.Splits(header, [
        U("a1.c", **{".text": (0x1000, 0x1100), ".data": (0x8000, 0x8010)}),                # identical
        U("a23.cpp", **{".text": (0x1100, 0x1300), ".data": (0x8010, 0x8020)}),             # fold of a2 + a3
        U("a4.cpp", **{".text": (0x1300, 0x1400)}),                                         # new unit in unowned text
        U("b1.cpp", **{".text": (0x2000, 0x2100), ".sdata2": (0xA000, 0xA008)}),            # data gain
        U("d_only.cpp", **{".sdata": (0x9000, 0x9010)}),                                    # identical, data-only
        U("b2.cpp", **{".text": (0x2100, 0x2180)}),                                         # recut: loses a tail
        U("b3.cpp", **{".text": (0x2180, 0x2200)}),                                         # the tail, a new unit
        U("c1.cpp", **{".text": (0x3000, 0x3100)}),                                         # identical
    ])
    ident, moved, comps = diff_units(old, new, W)
    check("identical units", sorted(ident), ["a1.c", "c1.cpp", "d_only.cpp"])
    check("nothing moved", sorted(moved), [])
    check("components by window and anchor", [(c.window, c.old, c.new) for c in comps],
          [(0, ["a2.cpp", "a3.cpp"], ["a23.cpp"]), (0, [], ["a4.cpp"]), (1, ["b1.cpp"], ["b1.cpp"]), (1, ["b2.cpp"], ["b2.cpp", "b3.cpp"])])
    # applying every window in turn is the candidate, whatever the order of the windows
    for order in ((0, 1, 2), (2, 1, 0), (1, 0, 2)):
        cur = old
        for w in order:
            _i, _m, cs = diff_units(cur, new, W)
            cur = apply_components(cur, new, [c for c in cs if c.window == w])
        check("windows %s applied in turn give the candidate" % (order,), sc.render_splits(cur), sc.render_splits(new))
    # one window leaves the other windows' blocks alone
    _i, _m, cs = diff_units(old, new, W)
    only_a = apply_components(old, new, [c for c in cs if c.window == 0])
    check("window a alone: its fold and its new unit in, b untouched", [u.name for u in only_a.units], ["a1.c", "a23.cpp", "a4.cpp", "b1.cpp", "d_only.cpp", "b2.cpp", "c1.cpp"])
    check("window a alone: the b1 block is the old one", sc.Splits([], [only_a.by_name()["b1.cpp"]]).units[0].size(".sdata2"), 0)
    # a straddler: a unit whose .text crosses the edge at 0x2000 is reported, and the component sits in the window of its first start
    old2 = sc.Splits(header, [U("s.cpp", **{".text": (0x1F00, 0x2100)}), U("t.cpp", **{".text": (0x2100, 0x2200)})])
    new2 = sc.Splits(header, [U("s.cpp", **{".text": (0x1F00, 0x2200)})])
    _i, _m, cs = diff_units(old2, new2, W)
    check("a fold over a window edge: one component, window a, both windows seen, the crossing named", (len(cs), cs[0].window, cs[0].windows_seen, cs[0].crossings),
          (1, 0, [0, 1], [("s.cpp", 0x2000)]))
    # an identical unit the candidate puts elsewhere is a move
    old3 = sc.Splits(header, [U("x.cpp", **{".text": (0x1000, 0x1100)}), U("y.cpp", **{".text": (0x1100, 0x1200)}), U("z.cpp", **{".sdata": (0x9000, 0x9010)})])
    new3 = sc.Splits(header, [U("x.cpp", **{".text": (0x1000, 0x1100)}), U("z.cpp", **{".sdata": (0x9000, 0x9010)}), U("y.cpp", **{".text": (0x1100, 0x1200)})])
    _i, mv, cs = diff_units(old3, new3, W)
    check("a reordered identical unit is a one-unit component", (len(mv), [(c.old, c.new) for c in cs]), (1, [([next(iter(mv))], [next(iter(mv))])]))
    check("moving it gives the candidate order", [u.name for u in apply_components(old3, new3, cs).units], ["x.cpp", "z.cpp", "y.cpp"])
    # candidate names: `main/` units are root units, a lane's chosen names win
    m = map_candidate(sc.Splits(header, [U("main/draw.cpp", **{".text": (0x1000, 0x1100)}), U("ef/fn_1.cpp", **{".text": (0x1100, 0x1200)})]), {"ef/fn_1.cpp": "ef/real.cpp"})
    check("main/ is dropped, chosen names applied", [u.name for u in m.units], ["draw.cpp", "ef/real.cpp"])
    # splits.txt text round trip
    txt = sc.render_splits(old)
    check("render/parse round trip", sc.render_splits(sc.parse_splits(txt)), txt)
    # parse_window
    check("window letters and ranges", (parse_window("b", W)[1], parse_window("0x1000..0x2000", W)[0][1][1:], parse_window("0x1000..0x2000", W)[1]), (1, (0x1000, 0x2000), 1))
    cw = parse_window("0x2000..0x3000", W)[0]
    _i, _m, cs = diff_units(old, new, cw)
    check("an address-range window selects the same components as the letter that spans it", [c.new for c in cs if c.window == 1], [c.new for c in diff_units(old, new, W)[2] if c.window == 1])
    check("window_of", [window_of(a, W) for a in (0x1000, 0x1FFF, 0x2000, 0x9000, 0x10)], [0, 0, 1, 2, 0])
    # the configure.py parser: balanced calls, multi-line, state, lib
    conf = parse_configure('x = [{"lib": "ef", "objects": [Object(Matching, "ef/a.cpp"),\n Object(NonMatching, "ef/b.cpp",\n   cflags=[*c, "-x(y"]), Object(Matching, "ef/a.cpp")]}, {"lib": "hud",\n "objects": [Object(NonMatching, "hud/c.c")]}]')
    check("configure rows: state, line, lib, repeats", [(p, r["state"], r["line"], r["lib"], r["count"]) for p, r in conf.items()],
          [("ef/a.cpp", "Matching", 1, "ef", 2), ("ef/b.cpp", "NonMatching", 2, "ef", 1), ("hud/c.c", "NonMatching", 4, "hud", 1)])
    check("a multi-line Object keeps its whole call", conf["ef/b.cpp"]["text"].endswith('"-x(y"])'), True)
    # function probing
    check("unmangled and ctor spellings", (function_probe("foo__5ClassFv")[-2:], function_probe("__ct__5ClassFv")[-1]), (["foo", "Class::foo"], "Class::Class"))
    check("a definition is told from a call and a prototype", (find_definition("void foo(int a);\nvoid bar() {\n    foo(1);\n}\nint foo(int a) {\n}\n", "foo"),
                                                              find_definition("void bar() {\n    foo(1);\n}\n", "foo")), ((True, 5), (False, 2)))
    check("definitions counted once each", count_definitions("int foo(int a) {\n    foo(2);\n}\n// foo(\nint foo(int);\n", "foo"), 1)
    # language evidence
    inp = Inputs(old, new, [{"name": "bar__Fv", "section": ".text", "addr": 0x1100, "size": 8, "type": "function", "scope": "", "kind": ""},
                            {"name": "plain", "section": ".text", "addr": 0x2000, "size": 8, "type": "function", "scope": "", "kind": ""}],
                 'Object(Matching, "a2.cpp")', Tree(os.getcwd()), None, W)
    check("mangled function -> .cpp", language_evidence(inp, new.by_name()["a23.cpp"], ["a2.cpp"])["recommended"], ".cpp")
    check("only .c absorbed and nothing mangled -> .c, flagged when the name says .cpp", (language_evidence(inp, new.by_name()["b1.cpp"], ["x.c"])["recommended"],
                                                                                           language_evidence(inp, new.by_name()["b1.cpp"], ["x.c"])["differs"]), (".c", True))
    # the manifest of a fixture tree: kinds, renames, demotions, data gain
    tmp = tempfile.mkdtemp(prefix="applysplits-")
    try:
        for rel, body in (("src/a2.cpp", "void bar() {\n}\n"), ("src/a3.cpp", "void baz() {\n}\n"), ("src/b1.cpp", "void qux() {\n}\n"), ("src/a1.c", "void one() {\n}\n"),
                          ("include/a2.h", "extern int x;\n"), ("src/other.cpp", '#include "a2.h"\n')):
            os.makedirs(os.path.dirname(os.path.join(tmp, *rel.split("/"))), exist_ok=True)
            with open(os.path.join(tmp, *rel.split("/")), "w", encoding="utf-8", newline="\n") as fh:
                fh.write(body)
        syms = [{"name": n, "section": ".text", "addr": a, "size": 0x10, "type": "function", "scope": "", "kind": ""}
                for n, a in (("bar", 0x1100), ("baz", 0x1210), ("qux", 0x2010), ("zed__Fv", 0x2110), ("tailfn", 0x2190))]
        syms += [{"name": "d1", "section": ".sdata2", "addr": 0xA000, "size": 8, "type": "object", "scope": "", "kind": ""}]
        cfg = 'Object(Matching, "a1.c"), Object(NonMatching, "a2.cpp"), Object(Matching, "a3.cpp"), Object(Matching, "b1.cpp"), Object(NonMatching, "b2.cpp")'
        inp = Inputs(old, new, syms, cfg, Tree(tmp), None, W)
        man = build_manifest(inp, 0)
        comp_fold = next(c for c in man["components"] if c["candidates"][0]["name"] == "a23.cpp")
        cand = comp_fold["candidates"][0]
        check("the fold: kind, absorbed sources in text order, one function each", (cand["kind"], cand["absorbs"], [len(f["functions"]) for f in cand["functions_by_source"]]),
              ("fold of 2 registered units", ["a2.cpp", "a3.cpp"], [1, 1]))
        check("the fold: functions are found in their sources", [f["in_source"] for f in cand["functions_by_source"]], [1, 1])
        check("the fold: unit-rename pairs, one NEW for both OLDs", comp_fold["unit_renames"], ["a2=a23", "a3=a23"])
        check("the fold: a Matching a3 is demoted into the merged unit, a2 is not Matching", [(d["unit"], d["into"]) for d in comp_fold["matching_demotions"]], [("a3.cpp", ["a23.cpp"])])
        check("the fold: the header of a2 and its includer", [(h["path"], h["included_by"]) for h in comp_fold["old"][0]["headers"]], [("include/a2.h", ["src/other.cpp"])])
        newu = next(c for c in man["components"] if c["candidates"][0]["name"] == "a4.cpp")["candidates"][0]
        check("a unit in unowned text is a new unit", (newu["kind"], newu["new_unowned_text_bytes"]), ("new unit", 0x100))
        mm = build_manifest(inp, 1)
        gain = next(c for c in mm["components"] if c["candidates"][0]["name"] == "b1.cpp")
        check("a data gain of a Matching unit is listed as such and is not a demotion", (gain["candidates"][0]["kind"], [g["unit"] for g in gain["matching_data_gain"]],
                                                                                     gain["matching_demotions"], gain["candidates"][0]["gained_data"][".sdata2"]["gained_bytes"]),
              ("data gain", ["b1.cpp"], [], 8))
        recut = next(c for c in mm["components"] if c["candidates"][0]["name"] == "b2.cpp")
        check("a recut registered unit keeps its name and gives the tail to a new unit", ([c["kind"] for c in recut["candidates"]], recut["survivors"], recut["unit_renames"]),
              (["recut registered unit", "recut registered unit"], ["b2.cpp"], ["b2=b3"]))
        check("the tail unit takes tailfn from the recut source", [(f["unit"], [x["name"] for x in f["functions"]]) for f in recut["candidates"][1]["functions_by_source"]], [("b2.cpp", ["tailfn"])])
        check("the manifest counts", (man["summary"]["components"], man["summary"]["kinds"]), (2, {"fold": 1, "new unit": 1}))
        # verify, on a fixture: a tree that did the fold wrongly fails, the right one passes
        edited = sc.render_splits(apply_components(old, new, [c for c in diff_units(old, new, W)[2] if c.window == 0]))
        good_cfg = 'Object(Matching, "a1.c"), Object(NonMatching, "a23.cpp"), Object(NonMatching, "a4.cpp"), Object(Matching, "b1.cpp"), Object(NonMatching, "b2.cpp")'
        t2 = tempfile.mkdtemp(prefix="applysplits-v-")
        try:
            for rel, body in (("src/a23.cpp", "void bar() {\n}\nvoid baz() {\n}\n"), ("src/a4.cpp", "/* header */\n"), ("src/a1.c", "void one() {\n}\n")):
                put(t2, rel, body)
            base_inp = Inputs(old, new, syms, cfg, DirBase(tmp), None, W)
            rows = dict((r[0], r[1]) for r in verify_window(base_inp, 0, Tree(t2), edited, good_cfg, DirBase(tmp), no_build=True))
            check("verify: a correct window passes every row", [k for k, v in rows.items() if not v], [])
            bad_cfg = good_cfg.replace('NonMatching, "a23.cpp"', 'Matching, "a23.cpp"') + ', Object(NonMatching, "a2.cpp")'
            rows = dict((r[0], r[1]) for r in verify_window(base_inp, 0, Tree(t2), edited, bad_cfg, DirBase(tmp), no_build=True))
            check("verify: a Matching merged unit and a stale a2 row are caught", sorted(k for k, v in rows.items() if not v),
                  ["configure.py has no Matching on a demoted unit", "every folded registered unit's source is gone and unregistered"])
            with open(os.path.join(t2, "src", "a23.cpp"), "w", encoding="utf-8", newline="\n") as fh:
                fh.write("void bar() {\n}\nvoid bar() {\n}\nvoid baz() {\n}\n")
            rows = dict((r[0], r[1]) for r in verify_window(base_inp, 0, Tree(t2), edited, good_cfg, DirBase(tmp), no_build=True))
            check("verify: a function defined twice in the merged file is caught", [k for k, v in rows.items() if not v], ["every function of a folded source appears exactly once in the merged file"])
            os.remove(os.path.join(t2, "src", "a4.cpp"))
            rows = dict((r[0], r[1]) for r in verify_window(base_inp, 0, Tree(t2), edited, good_cfg, DirBase(tmp), no_build=True))
            check("verify: a registered unit without a source is caught", "every unit of the window is registered iff it has a source" in [k for k, v in rows.items() if not v], True)
            rows = dict((r[0], r[1]) for r in verify_window(base_inp, 0, Tree(t2), edited.replace("a23.cpp", "zz.cpp"), good_cfg, DirBase(tmp), no_build=True))
            check("verify: a splits.txt that is not the plan is caught", rows["splits.txt equals the plan"], False)
        finally:
            shutil.rmtree(t2, ignore_errors=True)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    # alignment normalisation: dtk refuses a split that starts off the 4-byte alignment
    def sym(name, sec, addr, size):
        return {"name": name, "section": sec, "addr": addr, "size": size, "type": "object", "scope": "", "kind": ""}

    al_syms = [sym("s0", ".data", 0x100, 0x11), sym("s1", ".data", 0x114, 4), sym("s2", ".data", 0x200, 4), sym("b0", ".bss", 0x300, 0x0A), sym("b1", ".bss", 0x30A, 0x20),
               sym("c0", ".data", 0x400, 0x0D), sym("c1", ".data", 0x40D, 0x13), sym("c2", ".data", 0x420, 4)]
    al = sc.Splits(header, [U("p.cpp", **{".data": (0x100, 0x111)}), U("q.cpp", **{".data": (0x200, 0x204)}), U("r.cpp", **{".bss": (0x300, 0x30A)}),
                            U("t.cpp", **{".data": (0x400, 0x40D)}), U("v.cpp", **{".data": (0x40D, 0x424)})])
    got, fixes, blocked = align_candidate(clone_splits(al), al_syms)
    check("an end before a gap moves up over the padding", got.by_name()["p.cpp"].rs(".data"), [(0x100, 0x114)])
    check("an end in front of an unowned symbol that starts there is blocked, not extended", (got.by_name()["r.cpp"].rs(".bss"), [b[:3] for b in blocked if b[0] == "r.cpp"]),
          ([(0x300, 0x30A)], [("r.cpp", ".bss", 0x30A)]))
    check("abutting unaligned boundary: the range before gives up the padding only when it holds no symbol across it", (got.by_name()["t.cpp"].rs(".data"), got.by_name()["v.cpp"].rs(".data")),
          ([(0x400, 0x40D)], [(0x40D, 0x424)]))
    check("fixes name the unit, section, old and new boundary", fixes, [("p.cpp", ".data", 0x111, 0x114), ("v.cpp", ".data", 0x424, 0x424)][:1])
    al2 = sc.Splits(header, [U("a.cpp", **{".data": (0x100, 0x110)}), U("b.cpp", **{".data": (0x122, 0x130)})])
    got2, fixes2, blocked2 = align_candidate(clone_splits(al2), [sym("x", ".data", 0x110, 0x10), sym("y", ".data", 0x120, 2), sym("z", ".data", 0x122, 8)])
    check("an unaligned start after a gap moves down over padding that holds no symbol start; here the symbol y starts in it, so it stays blocked", ([b[:3] for b in blocked2], got2.by_name()["b.cpp"].rs(".data")),
          ([("b.cpp", ".data", 0x122)], [(0x122, 0x130)]))
    got3, _f3, blocked3 = align_candidate(clone_splits(al2), [sym("x", ".data", 0x110, 0x10), sym("z", ".data", 0x122, 8)])
    check("... and without that symbol the start moves down to the aligned address", (got3.by_name()["b.cpp"].rs(".data"), blocked3), ([(0x120, 0x130)], []))
    # dtk's refusals and its config
    m = UNSPLIT_RE.search("Caused by: Unsplit data in .sdata2 from Network/NetworkSessionManagerPat.cpp 11:0x8079C78C to next split 11:0x8079C790")
    check("the `Unsplit data` refusal is parsed", m and (m.group(1), m.group(2), m.group(3), m.group(4)), (".sdata2", "Network/NetworkSessionManagerPat.cpp", "8079C78C", "8079C790"))
    cfg = rewrite_config("object: orig/x.dol\nsplits: config/s.txt\nname: main\n", os.path.join(os.getcwd(), "a", "b"), {"splits": os.path.join(os.getcwd(), "a", "s2.txt"), "object": os.path.join(os.getcwd(), "orig", "x.dol")})
    check("config paths are written relative to the run's cwd, other lines kept", cfg, "object: ../../orig/x.dol\nsplits: ../s2.txt\nname: main\n")
    # names a reviewer must look at
    inp3 = Inputs(sc.Splits(header, [U("m/fn_00001100.cpp", **{".text": (0x1100, 0x1200)}), U("m/base.cpp", **{".text": (0x1200, 0x1300)})]),
                  sc.Splits(header, [U("m/fn_00001100.cpp", **{".text": (0x1180, 0x1200)}), U("m/base.cpp", **{".text": (0x1200, 0x1280)}),
                                     U("m/base_base.cpp", **{".text": (0x1280, 0x1300)}), U("m/eft004_fx.cpp", **{".text": (0x1300, 0x1400)}),
                                     U("m/fn_00001400.cpp", **{".text": (0x1400, 0x1500)}), U("m/fn_00001500.cpp", **{".text": (0x1550, 0x1600)})]),
                  [], "", Tree(os.getcwd()), None, W)
    kinds = sorted((x["unit"], x["kind"]) for x in placeholder_findings(inp3, [u.name for u in inp3.new.units]))
    check("placeholder reviews: a stem at the wrong address, a new fn_ stem, a suffix collision and a name-run remnant; an unchanged right-address name is not reviewed", kinds,
          [("m/base_base.cpp", "suffix-collision"), ("m/eft004_fx.cpp", "name-run-remnant"), ("m/fn_00001100.cpp", "placeholder-wrong-address"),
           ("m/fn_00001400.cpp", "placeholder"), ("m/fn_00001500.cpp", "placeholder-wrong-address")])
    # regression rows
    check("a function that scored and now scores lower, or is gone, is a regression", regression_rows({"f": 100.0, "g": 50.0, "h": 0.0, "k": 80.0}, {"f": 99.0, "g": 50.0, "k": 90.0}),
          [("f", 100.0, 99.0)])
    check("a gone function that scored is a regression", regression_rows({"f": 100.0}, {}), [("f", 100.0, None)])
    check("function scores read a missing key as 0", function_scores({"units": [{"functions": [{"name": "a"}, {"name": "b", "fuzzy_match_percent": 75.0}]}]}), {"a": 0.0, "b": 75.0})
    for f in fails:
        print("FAIL " + f)
    print("applysplits selftest: %s - %d checks (%d failure(s))" % ("FAIL" if fails else "PASS", count[0], len(fails)))
    return 1 if fails else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    for name in ("plan", "apply", "manifest", "verify"):
        p = sub.add_parser(name)
        p.add_argument("--window", required=True)
        if name in ("plan", "manifest"):
            p.add_argument("--json")
        if name == "plan":
            p.add_argument("--check", action="store_true")
            p.add_argument("--dtk", action="store_true")
            p.add_argument("--probe", action="store_true", help="with --dtk: when dtk refuses, run again with the candidate's blockers given an owner (diagnostic)")
        if name == "manifest":
            p.add_argument("--md")
        if name == "verify":
            p.add_argument("--base")
            p.add_argument("--no-build", action="store_true")
            p.add_argument("--before-report")
            p.add_argument("--allow-unregistered", action="store_true")
            p.add_argument("--allow-regression", action="append", default=[])
    p = sub.add_parser("freeze")
    p.add_argument("--force", action="store_true")
    p = sub.add_parser("all")
    p.add_argument("--out-dir", default=PHASE4_DIR)
    p.add_argument("--dtk", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.cmd == "plan":
        return cmd_plan(args)
    if args.cmd == "apply":
        return cmd_apply(args)
    if args.cmd == "manifest":
        return cmd_manifest(args)
    if args.cmd == "verify":
        return cmd_verify(args)
    if args.cmd == "all":
        return cmd_all(args)
    if args.cmd == "freeze":
        return cmd_freeze(args)
    ap.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
