#!/usr/bin/env python3
"""Propose the translation-unit (TU) boundary around an address, offline.

Why this works.  The retail `main.dol` is a link of objects, and a linker concatenates each object's
sections in link order.  So every data section is a *concatenation of per-TU fragments in the same
order as `.text`* - measured here as Spearman 1.000 between a `.sdata2` constant's address and the
lowest address of the `.text` functions that reference it (0.990 for `.sdata`, 0.985 for `.rodata`).
A **private** data symbol (a file-scope static, or a constant MWCC pooled per TU) therefore pins all
of its referrers into one TU, and a discontinuity between two sections' referrer runs pins a
boundary.  This tool collects those observations from the already-split DOL's own disassembly
(`build/<version>/asm/`, one `.s` per unit) and scores every candidate function boundary.

Observations, in decreasing authority:

* **must-link** (a boundary here is impossible) - from a private label referenced from both sides, a
  `scope:local` function and its callers, or a `__FILE__` assert string naming a `.c`/`.cpp` file.
* **pool run jump** - two adjacent labels of one section whose referrer sets are disjoint and
  ordered; the boundary lies between the last referrer of the first run and the first of the second.
* **codegen fingerprint** (soft) - a `_savegpr_*`/`stmw` change or a record-form presence change
  between two neighbouring functions is a per-TU flag change (playbook idea 21).
* **alignment gap** (soft) - a >4 byte gap; weak in this binary, where `.text` is one run with gaps
  of only 4/8/12 bytes.

What is deliberately *not* used: dtk's `auto_*` units (they are per-function build scaffolding, not
TU evidence), naive `lbl_` sharing (`.sbss` 69.8 % and `.bss` 60 % of labels are shared by several
functions and are ordinary cross-TU globals), and constant values (a repeated float is not a
boundary marker - dtk labels every pool word).

Usage (addresses in hex, or a symbol name):

    tudiscover.py at <address|symbol> [--window 40] [--json] [--splits]
                                      [--unit src/<Lib>/<file>.c] [--max-funcs 400]
    tudiscover.py stats                 # cache + coverage + observation counts
    tudiscover.py cache [--force]       # (re)build build/tmp/tudiscover/graph.json

Nothing is written outside `build/tmp/`: the `splits.txt` block is printed, never applied.
"""
import argparse
import collections
import hashlib
import importlib.util
import json
import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu  # noqa: E402  (repo root + build layout)

ROOT = uu.ROOT
GAME = "RMHE08"
SYMBOLS = os.path.join(ROOT, "config", GAME, "symbols.txt")
SPLITS = os.path.join(ROOT, "config", GAME, "splits.txt")
ASM_DIR = os.path.join(ROOT, "build", GAME, "asm")
DOL = os.path.join(ROOT, "orig", GAME, "sys", "main.dol")
CACHE = os.path.join(ROOT, "build", "tmp", "tudiscover", "graph.json")
SCHEMA = 5

# Section order for the printed `splits.txt` block (matches config/RMHE08/splits.txt's header).
SECTION_ORDER = [".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data",
                 ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2"]

TOKEN_RE = re.compile(r"[A-Za-z_@][A-Za-z0-9_@$.]*")
CALL_RE = re.compile(r"\bbl (\S+)")
SAVE_RE = re.compile(r"\b(?:_savegpr_|_restgpr_|stmw|lmw)\S*")
REC_RE = re.compile(r"\b(?:rlwinm|and|or|add|subf|subfc|neg|cntlzw|slw|srw|andc|xor|extsb|extsh)\.")
SRCFILE_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_./\\-]*\.(?:c|cpp|cc|cxx|cp)$")
ETI_RE = re.compile(r'\.obj "@eti_([0-9A-Fa-f]{8})".*?\.endobj', re.S)
FOURBYTE_RE = re.compile(r"\.4byte\s+(\S+)")
# Observation kinds by authority: `pool`/`source` pin a real boundary, the other two are weak.
STRONG = ("pool", "source")

# Sections whose labels must never be read as TU-shared data: `extab`/`extabindex` are per-function
# unwind table fragments whose `@eti_`/`@etb_` symbols are aliases covering arbitrary addresses in the
# section (real code loads `"@eti_8001FFF8"+0xA`), and `.init` is boot code owned by no game TU.
NO_REF_SECTIONS = ("extab", "extabindex", ".init")


def load_symedit():
    path = os.path.join(ROOT, "tools", "symbols", "symedit.py")
    spec = importlib.util.spec_from_file_location("symedit", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Dol:
    """Address -> bytes for the retail image (traced through the DOL's section table)."""

    def __init__(self, path):
        self.data = open(path, "rb").read()
        h = self.data
        toff = struct.unpack(">7I", h[0x00:0x1C])
        doff = struct.unpack(">11I", h[0x1C:0x48])
        taddr = struct.unpack(">7I", h[0x48:0x64])
        daddr = struct.unpack(">11I", h[0x64:0x90])
        tsize = struct.unpack(">7I", h[0x90:0xAC])
        dsize = struct.unpack(">11I", h[0xAC:0xD8])
        self.secs = [(a, s, o) for a, s, o in zip(taddr, tsize, toff) if s]
        self.secs += [(a, s, o) for a, s, o in zip(daddr, dsize, doff) if s]

    def read(self, addr, n):
        for a, s, o in self.secs:
            if a <= addr < a + s:
                return self.data[o + (addr - a):o + (addr - a) + n]
        return None

    def cstr(self, addr, limit=256):
        raw = self.read(addr, limit) or b""
        return raw.split(b"\0", 1)[0]


def load_map():
    """(functions, labels): names -> records, straight from the symbol map (never printed)."""
    se = load_symedit()
    fns, labels = {}, {}
    for e in se.entries(SYMBOLS):
        if e["section"] == ".text" and e["type"] == "function":
            fns[e["name"]] = {"addr": e["address"], "size": e["size"],
                              "scope": "local" if "scope:local" in e["line"] else ""}
        elif e["section"] != ".text":
            # `(?<!\w)` matters: `.sdata:` and `.rodata:` also contain the substring `data:`.
            kind = re.search(r"(?<!\w)data:(\S+)", e["line"])
            labels[e["name"]] = {"section": e["section"], "addr": e["address"],
                                 "size": e["size"], "kind": kind.group(1) if kind else "",
                                 "local": "scope:local" in e["line"]}
    return fns, labels


def asm_files():
    """Every unit's disassembly: `*_text.s` at top level, plus the claimed units' files."""
    out = []
    for dirpath, _dirs, names in os.walk(ASM_DIR):
        for n in names:
            if not n.endswith(".s"):
                continue
            if dirpath == ASM_DIR and n.startswith("auto_") and not n.endswith("_text.s"):
                continue          # data-section scaffolding, no `.fn` blocks
            out.append(os.path.join(dirpath, n))
    return sorted(out)


def build_graph(fns, labels, force=False):
    """Per-function data references, calls and codegen fingerprint, from the disassembly."""
    files = asm_files()
    stamp = {"schema": SCHEMA, "symbols": hashlib.sha1(open(SYMBOLS, "rb").read()).hexdigest(),
             "files": len(files), "bytes": sum(os.path.getsize(f) for f in files)}
    if not force and os.path.exists(CACHE):
        try:
            cached = json.load(open(CACHE, encoding="utf-8"))
            if cached.get("stamp") == stamp:
                print("# graph cache: build/tmp/tudiscover/graph.json (%d files)" % stamp["files"],
                      file=sys.stderr)
                return cached
        except (ValueError, OSError):
            pass
    t0 = time.time()
    datanames = {n for n, l in labels.items() if l["section"] not in NO_REF_SECTIONS}
    graph = {}
    extab = {}
    for path in files:
        txt = open(path, "r", encoding="utf-8", errors="replace").read()
        for m in ETI_RE.finditer(txt):
            ops = FOURBYTE_RE.findall(m.group(0))
            if ops and ops[0] in fns:
                etb = int(ops[1], 16) if len(ops) > 1 and ops[1].startswith("0x") else None
                extab[ops[0]] = [int(m.group(1), 16), etb]
        for block in txt.split("\n.fn ")[1:]:
            cut = block.find(",")
            name = block[:cut if 0 <= cut < 80 else block.find("\n")].strip()
            if name not in fns:
                continue
            refs, calls = set(), set()
            for tok in TOKEN_RE.findall(block):
                base = tok.split("@", 1)[0]
                if base in datanames:
                    refs.add(base)
                elif tok in datanames:
                    refs.add(tok)
            for callee in CALL_RE.findall(block):
                base = callee.split("@", 1)[0]
                if base in fns:
                    calls.add(base)
            rec = len(REC_RE.findall(block))
            graph[name] = {"refs": sorted(refs), "calls": sorted(calls),
                           "fp": [1 if SAVE_RE.search(block) else 0, rec]}
    out = {"stamp": stamp, "files": len(files), "funcs": graph, "extab": extab}
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    json.dump(out, open(CACHE, "w", encoding="utf-8"))
    print("# graph: %d functions (%d with extab) from %d files in %.1fs -> build/tmp/tudiscover/graph.json"
          % (len(graph), len(extab), len(files), time.time() - t0), file=sys.stderr)
    return out


def duplicate_values(labels, dol):
    """Names whose byte pattern also occurs at another label of the same section.

    MWCC emits pooled constants as a *private copy per TU*, so a value with several copies proves
    the per-TU pooling and makes each copy a strong privacy candidate.  A label holding the only
    copy of its value is usually an ordinary cross-TU global (`globals` below showed exactly that:
    an 8-byte `.sdata` object referenced from two far-apart clusters).
    """
    seen = collections.defaultdict(list)
    for name, lab in labels.items():
        n = lab["size"] if lab["size"] in (1, 2, 4, 8) else 4
        raw = dol.read(lab["addr"], n)
        if raw and len(raw) == n:
            seen[(lab["section"], raw)].append(name)
    return {name for names in seen.values() if len(names) > 1 for name in names}


def classify(labels, refs_of, ordered, addr, size, span_max, dup):
    """private / ambiguous per label, with the reason kept for the report.

    `span_max` is in *bytes* of .text: every referrer of a private pooled constant is inside the one
    TU that owns it, so the referrer span is bounded by that TU's size.  A label whose referrers sit
    megabytes apart (`lbl_80794380`, an 8-byte `.sdata` object called from two distant clusters) is an
    ordinary cross-TU global however duplicated its value is.
    """
    out = {}
    for name, lab in labels.items():
        users = refs_of.get(name)
        if not users:
            continue
        first, last = ordered[users[0]], ordered[users[-1]]
        span = addr[users[-1]] + size[users[-1]] - addr[users[0]]
        if lab["local"]:
            out[name] = ("private", "scope:local")
        elif lab["section"] in (".sdata2", ".sdata") and name in dup and span <= span_max:
            out[name] = ("private", "%s pool, value copied elsewhere, span %d B" % (lab["section"], span))
        elif len(users) > 1:
            out[name] = ("ambiguous", "%s, span %d B, no scope%s"
                         % (lab["section"], span, "" if name in dup else ", unique value"))
    return out


def source_file_label(dol, labels, name):
    """A `.rodata` string label holding a bare source-file name, e.g. `ef_util.cpp`."""
    lab = labels[name]
    if lab["kind"] != "string":
        return None
    text = dol.cstr(lab["addr"], min(max(lab["size"], 4), 256)).decode("latin-1", "replace")
    return text.strip() if SRCFILE_RE.match(text.strip()) else None


def analyse(fns, labels, graph, dol, span_max):
    """Ordered function list + every observation, in index space (cut i = boundary before f[i])."""
    ordered = sorted(fns, key=lambda n: fns[n]["addr"])
    idx = {n: i for i, n in enumerate(ordered)}
    addr, size = [fns[n]["addr"] for n in ordered], [fns[n]["size"] for n in ordered]

    refs_of = collections.defaultdict(list)          # data name -> sorted function indices
    callers_of = collections.defaultdict(list)       # function name -> sorted caller indices
    for name, rec in graph["funcs"].items():
        if name not in idx:
            continue
        i = idx[name]
        for ref in rec["refs"]:
            refs_of[ref].append(i)
        for callee in rec["calls"]:
            callers_of[callee].append(i)
    for d in (refs_of, callers_of):
        for k in d:
            d[k] = sorted(set(d[k]))

    cls = classify(labels, refs_of, ordered, addr, size, span_max, duplicate_values(labels, dol))
    must_link, soft = [], []

    def link(lo, hi, why):
        if hi - lo >= 1:
            must_link.append((lo, hi, why))

    for name, (kind, why) in cls.items():
        if kind == "private":
            u = refs_of[name]
            link(u[0], u[-1], "%s [%s]" % (name, why))
    for name, users in callers_of.items():
        if name in fns and fns[name]["scope"] == "local":
            u = sorted(set(users) | {idx[name]})
            link(u[0], u[-1], "%s is scope:local, %d callers" % (name, len(users)))

    # Single-caller helpers: a function called only from one neighbourhood usually sits next to its
    # callers in the source, so the TU holding the callers holds it too.  Soft, and only kept while
    # the implied interval is short enough for the vote to matter.
    for name, users in callers_of.items():
        if name not in idx or not users:
            continue
        k = idx[name]
        lo_i, hi_i = min(min(users), k), max(max(users), k)
        if hi_i - lo_i <= 64:
            soft.append((lo_i, hi_i, 0.35, "call",
                         "%s called only from this range (%d callers)" % (name, len(users))))

    # Source-file assert strings: one `.c`/`.cpp` per TU - must-link inside, boundary between.
    file_of = {}
    for name in labels:
        src = source_file_label(dol, labels, name)
        if src and name in refs_of:
            file_of[name] = src
            u = refs_of[name]
            link(u[0], u[-1], '%s = "%s"' % (name, src))
    groups = sorted(((refs_of[n][0], refs_of[n][-1], s) for n, s in file_of.items()),
                    key=lambda t: t[0])
    for (a_lo, a_hi, a_src), (b_lo, b_hi, b_src) in zip(groups, groups[1:]):
        if a_src != b_src and a_hi <= b_lo:
            soft.append((a_hi, b_lo, 1.0, "source",
                         "source file change %s -> %s" % (a_src, b_src)))

    # Pool runs: adjacent *private* labels of one section whose referrer sets are disjoint and
    # ordered pin the boundary between the last referrer of the first run and the first of the
    # second (both labels belong to one TU fragment each, so the cut lies between them).
    by_section = collections.defaultdict(list)
    for name, lab in labels.items():
        if name in refs_of and cls.get(name, ("",))[0] == "private":
            by_section[lab["section"]].append((lab["addr"], name))
    for section, items in by_section.items():
        items.sort()
        for (_, n1), (_, n2) in zip(items, items[1:]):
            u, v = refs_of[n1], refs_of[n2]
            if u[-1] < v[0]:
                soft.append((u[-1], v[0], 1.0, "pool",
                             "%s run jump %s -> %s" % (section, n1, n2)))

    # Codegen fingerprint and alignment gaps between neighbours (weak, dense in this binary).
    for i in range(1, len(ordered)):
        prev, cur = graph["funcs"].get(ordered[i - 1]), graph["funcs"].get(ordered[i])
        if prev and cur:
            w = 0.6 if prev["fp"][0] != cur["fp"][0] else 0.0
            if (prev["fp"][1] > 0) != (cur["fp"][1] > 0):
                w += 0.35
            if w:
                soft.append((i - 1, i - 1, w, "codegen", "codegen fingerprint change"))
        gap = addr[i] - (addr[i - 1] + size[i - 1])
        if gap > 4:
            soft.append((i - 1, i - 1, 0.25 if gap < 16 else 0.5, "gap",
                         "alignment gap 0x%X" % gap))

    return {"ordered": ordered, "idx": idx, "addr": addr, "size": size, "refs_of": refs_of,
            "cls": cls, "must_link": must_link, "soft": soft}


def expand(an, seed, max_funcs):
    """Interval closure of the must-link anchors around `seed` (a cut inside an anchor is illegal)."""
    lo, hi = seed, seed + 1
    touched, grew = [], True
    while grew:
        grew = False
        for a, b, why in an["must_link"]:
            if b + 1 <= lo or a >= hi:
                continue                      # no overlap with [lo, hi]
            nlo, nhi = min(lo, a), max(hi, b + 1)
            if (nlo, nhi) != (lo, hi):
                lo, hi = nlo, nhi
                touched.append((a, b, why))
                grew = True
        if hi - lo > max_funcs:
            return lo, hi, touched, "range exceeded --max-funcs (%d)" % max_funcs
    return lo, hi, touched, None


def score_cuts(an, lo, hi, window):
    """Score candidate boundaries near the must-link closure.

    An observation is an interval of cuts it *admits*, so a wide one carries little information:
    each observation spreads its weight uniformly over its interval.  A candidate's score is the
    share of the local evidence that lands on it, and the narrow observations that land there
    ("pins") are what actually name a boundary.
    """
    sides = {"left": range(max(0, lo - window), lo + 1),
             "right": range(hi, min(len(an["ordered"]), hi + window))}
    out = {}
    for side, rng in sides.items():
        cand = {c: {"side": side, "cut": c, "support": 0.0, "pins": [], "veto": None}
                for c in rng}
        avail = 0.0
        for olo, ohi, w, kind, why in an["soft"]:
            width = ohi - olo + 1
            a = max(olo, rng.start)
            b = min(ohi, rng.stop - 1)
            if a > b:
                continue
            per = w / width
            avail += per * (b - a + 1)
            for c in range(a, b + 1):
                cand[c]["support"] += per
                if width <= 4:
                    cand[c]["pins"].append((kind, why))
        for a, b, why in an["must_link"]:
            for c in rng:
                if a <= c <= b and cand[c]["veto"] is None:
                    cand[c]["veto"] = why
        for d in cand.values():
            d["support"] = round(d["support"], 3)
            d["share"] = round(d["support"] / avail, 3) if avail else 0.0
            d["strong"] = [p for p in d["pins"] if p[0] in STRONG]
            d["rank"] = (len(d["strong"]), len(d["pins"]), d["share"])
        out.update(cand)
    return out


def short(why):
    """One clause of a reason, short enough for a table cell."""
    why = why.replace("source file change ", "")
    why = re.sub(r"^(\S+) run jump ", r"\1: ", why)
    return re.sub(r"\s*\(.*\)$", "", why)


def data_runs(an, labels, lo, hi):
    """The data a TU covering functions [lo, hi) plausibly owns: one contiguous run per section."""
    inside = set(an["ordered"][lo:hi])
    out = {}
    for name, lab in labels.items():
        if lab["section"] in (".init", "extab", "extabindex"):
            continue                      # boot code and per-function unwind tables: see extab_runs
        users = an["refs_of"].get(name)
        if not users:
            continue
        fns_here = [an["ordered"][i] for i in users]
        here = [f for f in fns_here if f in inside]
        if not here:
            continue
        out.setdefault(lab["section"], []).append((lab["addr"], lab["size"], name,
                                                  len(here) != len(fns_here)))
    runs = {}
    for section, items in out.items():
        items.sort()
        start = items[0][0]
        end = max(a + s for a, s, _, _ in items)
        own = {n for _, _, n, _ in items}
        # a linker fragment is contiguous: every label inside the run belongs to the same TU
        filler = [n for n, lab in labels.items()
                  if lab["section"] == section and start <= lab["addr"] < end and n not in own]
        runs[section] = {"start": start, "end": end, "labels": len(items), "filler": len(filler),
                         "density": round(len(items) / (len(items) + len(filler)), 2),
                         "leak": sum(1 for _, _, _, leaks in items if leaks),
                         "names": sorted(own)}
    return runs


def extab_runs(an, labels, graph, lo, hi):
    """`extab`/`extabindex` ranges for a function range, from each function's own unwind entries.

    These are the fragments `splits.txt` needs and that are easy to forget: they follow the
    functions one for one, so a unit's range is `min..max` over the entries of the functions it
    owns (the entry address is the `@eti_`/`@etb_` symbol's own address).
    """
    got = {}
    for name in an["ordered"][lo:hi]:
        pair = graph.get("extab", {}).get(name)
        if not pair:
            continue
        for section, addr in (("extabindex", pair[0]), ("extab", pair[1])):
            label = labels.get("@et%s_%08X" % ("b" if section == "extab" else "i", addr or 0))
            if addr and label:
                got.setdefault(section, []).append((label["addr"], label["size"]))
    return {s: {"start": min(a for a, _ in v), "end": max(a + sz for a, sz in v),
                "labels": len(v), "filler": 0, "leak": 0}
            for s, v in got.items()}


def report(args):
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=False)
    dol = Dol(DOL)
    an = analyse(fns, labels, graph, dol, args.span_max)
    ordered, idx = an["ordered"], an["idx"]

    if args.at in idx:
        seed = idx[args.at]
        target = args.at
    else:
        addr = int(args.at, 0) if not re.fullmatch(r"[0-9a-fA-F]+", args.at) else int(args.at, 16)
        hit = [i for i, n in enumerate(ordered)
               if fns[n]["addr"] <= addr < fns[n]["addr"] + max(fns[n]["size"], 4)]
        if not hit:
            print("no function contains 0x%X - run `stats` to check the map/asm coverage" % addr,
                  file=sys.stderr)
            return 1
        seed, target = hit[0], ordered[hit[0]]

    lo, hi, touched, guard = expand(an, seed, args.max_funcs)
    if guard:
        widest = max(touched, key=lambda t: t[1] - t[0]) if touched else None
        print("warning: %s; widest anchor: %s" % (guard, widest[2] if widest else "?"),
              file=sys.stderr)
    cands = score_cuts(an, lo, hi, args.window)

    def pick(side):
        """Extend the closure only on strong evidence - weak signals cannot move a boundary."""
        pool = [c for c in cands.values()
                if c["side"] == side and not c["veto"] and c["strong"]]
        if not pool:
            return None
        return max(pool, key=lambda c: (c["rank"],
                                        -abs(c["cut"] - (lo if side == "left" else hi))), )

    ranked = {side: sorted((c for c in cands.values() if c["side"] == side and not c["veto"]),
                           key=lambda c: c["rank"], reverse=True)[:args.top]
              for side in ("left", "right")}
    left, right = pick("left"), pick("right")
    sug_lo = left["cut"] if left and left["cut"] < lo else lo
    sug_hi = right["cut"] if right and right["cut"] > hi else hi
    runs = data_runs(an, labels, sug_lo, sug_hi)
    runs.update(extab_runs(an, labels, graph, sug_lo, sug_hi))
    candidates = list(ordered[lo:hi])
    srcs = sorted({s for n in labels if (s := source_file_label(dol, labels, n))
                   and n in an["refs_of"] and sug_lo <= an["refs_of"][n][0] < sug_hi})

    result = {
        "target": target, "target_address": fns[target]["addr"], "seed_index": seed,
        "must_link_range": [lo, hi], "suggested_range": [sug_lo, sug_hi],
        "match": [{"name": n, "address": fns[n]["addr"], "size": fns[n]["size"]}
                  for n in candidates],
        "text": [an["addr"][sug_lo], an["addr"][sug_hi - 1] + an["size"][sug_hi - 1]],
        "functions": sug_hi - sug_lo,
        "source_files": srcs,
        "anchor_count": len(touched),
        "widest_anchor": max((t[1] - t[0], t[2]) for t in touched)[1] if touched else None,
        "boundaries": {"left": _clean(left), "right": _clean(right),
                       "candidates": {s: [_clean(c) for c in v] for s, v in ranked.items()}},
        "runs": runs,
    }
    if args.json:
        print(json.dumps(result, indent=2))
        return 0
    print_human(result, ordered, fns, sug_lo, sug_hi)
    if args.splits:
        print_splits(result, args.unit)
    return 0


def _clean(cand):
    if not cand:
        return None
    return {"cut": cand["cut"], "support": cand["support"], "share": cand["share"],
            "strong": [{"kind": k, "why": w} for k, w in cand["strong"]],
            "pins": [{"kind": k, "why": w} for k, w in cand["pins"][:6]], "veto": cand["veto"]}


def print_human(res, ordered, fns, sug_lo, sug_hi):
    lo, hi = res["must_link_range"]
    print("target        %s (%s)" % (res["target"], ordered[res["seed_index"]]))
    print()
    print("MATCH SET     %d functions, certainly one TU:  0x%08X..0x%08X"
          % (hi - lo, fns[ordered[lo]]["addr"],
             fns[ordered[hi - 1]]["addr"] + fns[ordered[hi - 1]]["size"]))
    names = ["%s (0x%08X)" % (n, fns[n]["addr"]) for n in ordered[lo:hi]]
    shown = 16 if len(names) > 24 else len(names)
    for i in range(0, shown, 2):
        print("              %s" % "   ".join("%-34s" % n for n in names[i:i + 2]).rstrip())
    if shown < len(names):
        print("              ... and %d more (--json for the full list)" % (len(names) - shown))
    print("              match these together; the exact extent settles as they match (see below).")
    t0, t1 = res["text"]
    print("extended      %d functions if the boundary evidence is taken: 0x%08X..0x%08X (%d B)%s"
          % (res["functions"], t0, t1, t1 - t0,
             "  (none: the set above is the whole answer)" if (sug_lo, sug_hi) == (lo, hi) else ""))
    if res["source_files"]:
        print("source file   %s" % ", ".join(res["source_files"]))
    print("anchors       %d must-link anchors inside the range%s"
          % (res["anchor_count"], (", widest: %s" % res["widest_anchor"]) if res["widest_anchor"] else ""))
    print()
    for side in ("left", "right"):
        print("%s boundary (cut = first function after it):" % side.capitalize())
        got = res["boundaries"]["candidates"][side]
        if not got:
            print("  no candidate in the window")
        for i, c in enumerate(got):
            addr = fns[ordered[c["cut"]]]["addr"] if c["cut"] < len(ordered) else 0
            pins = [p["why"] for p in (c["strong"] or c["pins"])]
            print("  %s cut %5d 0x%08X  %s  share %.3f  %s"
                  % ("*" if i == 0 else " ", c["cut"], addr,
                     ("strong x%d" % len(c["strong"])) if c["strong"] else "weak",
                     c["share"], "; ".join(short(p) for p in pins[:2]) or "-"))
        print("    %s" % ("strong evidence moves this boundary" if res["boundaries"][side]
                          else "only weak signals here: the closure edge is the best estimate"))
    print()
    print("data the range would own (contiguous run per section; `dens` = share of labels inside the"
          " run that the range actually references, `leak` = also referenced from outside):")
    for section in sorted(res["runs"], key=SECTION_ORDER.index):
        r = res["runs"][section]
        print("  %-10s 0x%08X..0x%08X  %3d labels  dens %.2f  leak %s"
              % (section, r["start"], r["end"], r["labels"], r.get("density", 1.0),
                 r["leak"] if r["leak"] else "0"))
    if not res["runs"]:
        print("  (none - this range owns no labelled data at all: the boundary is unconstrained)")
    leak = sum(r["leak"] for r in res["runs"].values())
    if leak:
        print("  ^ %d label(s) inside these runs are also referenced from outside the range: either\n"
              "    a genuine cross-TU global lives there, or the range is too wide - see docs." % leak)
    print()
    print("next: write the MATCH SET (or the extended range) as one unit and measure it - a symbol in\n"
          "      the set that will not match under the unit's flags is where the real boundary is")


def claimed_units():
    """`splits.txt` as ground truth: {unit: {section: (start, end)}} for every claimed range."""
    out, cur = {}, None
    for line in open(SPLITS, encoding="utf-8", errors="replace"):
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":"):
            cur = line.strip()[:-1]
            out[cur] = {}
            continue
        m = re.match(r"\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            out[cur][m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return {u: s for u, s in out.items() if ".text" in s}


def tier_labels(an, fns, labels):
    """Tier 1: how the tool does on the units `splits.txt` already claims."""
    rows = []
    for unit, secs in sorted(claimed_units().items()):
        start, end = secs[".text"]
        hit = [i for i, n in enumerate(an["ordered"]) if fns[n]["addr"] >= start]
        if not hit:
            continue
        lo, hi, _, guard = expand(an, hit[0], 400)
        got = (an["addr"][lo], an["addr"][hi - 1] + an["size"][hi - 1])
        runs = data_runs(an, labels, lo, hi)
        rows.append({"unit": unit, "claimed": [start, end], "predicted": list(got),
                     "function_delta": (hi - lo) - len([n for n in an["ordered"]
                                                        if start <= fns[n]["addr"] < end]),
                     "start_off": got[0] - start, "end_off": got[1] - end,
                     "guard": bool(guard), "runs": {k: [v["start"], v["end"]] for k, v in runs.items()}})
    return rows


def tier_sweep(an, labels, seeds, seed):
    """Tier 2: closure behaviour over random seeds (over-merging canary, run quality)."""
    import random
    rnd = random.Random(seed)
    picks = rnd.sample(range(len(an["ordered"])), min(seeds, len(an["ordered"])))
    sizes, guard, clean, dens = [], 0, 0, []
    for s in picks:
        lo, hi, _, g = expand(an, s, 400)
        sizes.append(hi - lo)
        guard += 1 if g else 0
        if hi - lo <= 60:
            runs = data_runs(an, labels, lo, hi)
            if runs and hi - lo >= 2:      # a singleton's data is usually shared with its real
                clean += 1 if not any(r["leak"] for r in runs.values()) else 0   # TU-mates
                dens += [r["density"] for r in runs.values()]
    sizes.sort()
    n = len(sizes)
    return {"seeds": n, "median": sizes[n // 2], "p75": sizes[int(n * 0.75)],
            "p90": sizes[int(n * 0.90)], "max": sizes[-1], "guard_hits": guard,
            "singleton_share": round(sum(1 for s in sizes if s == 1) / n, 2),
            "leak_free_share": round(clean / max(1, sum(1 for s in sizes if 2 <= s <= 60)), 2),
            "mean_density": round(sum(dens) / len(dens), 2) if dens else 0.0}


def tier_consistency(an, seeds, seed):
    """Tier 3: distinct closures that partially overlap = the anchor set contradicts itself."""
    import random
    rnd = random.Random(seed + 1)
    picks = rnd.sample(range(len(an["ordered"])), min(seeds, len(an["ordered"])))
    spans = sorted({expand(an, s, 400)[:2] for s in picks})
    merged, partial = [], 0
    for lo, hi in spans:
        while merged and lo < merged[-1][1] < hi:
            partial += 1
            lo = min(lo, merged[-1][0])
            merged.pop()
        merged.append((lo, hi))
    return {"distinct_closures": len(spans), "partial_overlaps": partial}


def cmd_bench(args):
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=args.force)
    an = analyse(fns, labels, graph, Dol(DOL), args.span_max)
    score = {"labels": tier_labels(an, fns, labels), "sweep": tier_sweep(an, labels, args.seeds, args.seed),
             "consistency": tier_consistency(an, args.seeds, args.seed),
             "span_max": args.span_max, "seeds": args.seeds, "seed": args.seed}
    if args.json:
        print(json.dumps(score, indent=2))
    else:
        print("tier 1  known units (splits.txt as truth)")
        for r in score["labels"]:
            print("  %-46s delta %+d fn  start %+d B  end %+d B%s"
                  % (r["unit"], r["function_delta"], r["start_off"], r["end_off"],
                     "  GUARD" if r["guard"] else ""))
        s = score["sweep"]
        print("tier 2  closure over %d random seeds" % s["seeds"])
        print("  median %d  p75 %d  p90 %d  max %d  guard %d (singletons %.0f%%)"
              % (s["median"], s["p75"], s["p90"], s["max"], s["guard_hits"],
                 100 * s["singleton_share"]))
        print("  leak-free data runs %.0f%%  mean density %.2f"
              % (100 * s["leak_free_share"], s["mean_density"]))
        c = score["consistency"]
        print("tier 3  consistency: %d distinct closures, %d partially overlapping pairs"
              % (c["distinct_closures"], c["partial_overlaps"]))
    if args.save:
        path = args.save if os.path.isabs(args.save) else os.path.join(ROOT, args.save)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        json.dump(score, open(path, "w", encoding="utf-8"))
        print("# saved to %s" % os.path.relpath(path, ROOT), file=sys.stderr)
    if args.compare:
        old = json.load(open(args.compare if os.path.isabs(args.compare)
                             else os.path.join(ROOT, args.compare), encoding="utf-8"))
        print("# vs %s" % args.compare, file=sys.stderr)
        for key in ("median", "p75", "p90", "max", "guard_hits", "singleton_share",
                    "leak_free_share", "mean_density"):
            now, was = score["sweep"][key], old["sweep"][key]
            better_up = key in ("leak_free_share", "mean_density")
            flag = "" if (now >= was if better_up else now <= was) else "  <-- worse"
            print("  %-16s %s -> %s%s" % (key, was, now, flag))
        print("  %-16s %s -> %s" % ("partial_overlaps", old["consistency"]["partial_overlaps"],
                                     score["consistency"]["partial_overlaps"]))
    return 0


def print_splits(res, unit):
    unit = unit or "src/<Lib>/<file>.c"
    t0, t1 = res["text"]
    print("\n# proposal only, and `.text` first.  Measure before *and* after every data line (playbook"
          "\n# idea 23): defining a range can make dtk drop the target's R_PPC_NONE pool relocations.")
    print("\n%s:" % unit)
    print("\t%-10s start:0x%08X end:0x%08X" % (".text", t0, t1))
    for section in sorted(res["runs"], key=SECTION_ORDER.index):
        r = res["runs"][section]
        if r["leak"] or r.get("density", 1.0) < 0.5:
            print("\t# NOT claimed - %s 0x%08X..0x%08X looks unreliable (leak %d, density %.2f);"
                  % (section, r["start"], r["end"], r["leak"], r.get("density", 1.0)))
            print("\t#   claim the individual symbols, or verify each one by measurement")
            continue
        print("\t%-10s start:0x%08X end:0x%08X" % (section, r["start"], r["end"]))


def cmd_stats(args):
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=args.force)
    an = analyse(fns, labels, graph, Dol(DOL), args.span_max)
    covered = set(graph["funcs"])
    print("functions in map   %d" % len(fns))
    print("functions in asm   %d  (map entries without a `.fn` block: %d)"
          % (len(covered), len(set(fns) - covered)))
    kinds = collections.Counter(v[0] for v in an["cls"].values())
    print("labels classified  %s" % dict(kinds))
    print("must-link anchors  %d   soft observations %d" % (len(an["must_link"]), len(an["soft"])))
    print("soft by kind       %s" % dict(collections.Counter(o[3] for o in an["soft"])))
    print("candidate cuts     %d function boundaries in .text" % len(an["ordered"]))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)

    a = sub.add_parser("at", help="propose the TU boundary around an address or symbol")
    a.add_argument("at")
    a.add_argument("--window", type=int, default=40, help="candidate cuts to search each side")
    a.add_argument("--top", type=int, default=3, help="candidates to list per side")
    a.add_argument("--span-max", type=int, default=0x4000,
                   help="referrer span (bytes) above which a pool label counts as cross-TU")
    a.add_argument("--max-funcs", type=int, default=400, help="give up past this range size")
    a.add_argument("--unit", default=None, help="unit path for the printed splits block")
    a.add_argument("--json", action="store_true")
    a.add_argument("--splits", action="store_true", help="also print the splits.txt block")
    a.set_defaults(func=report)

    b = sub.add_parser("stats", help="cache, coverage and observation counts")
    b.add_argument("--span-max", type=int, default=0x4000)
    b.add_argument("--force", action="store_true", help="rebuild the graph cache")
    b.set_defaults(func=cmd_stats)

    c = sub.add_parser("cache", help="(re)build the graph cache only")
    c.add_argument("--force", action="store_true")
    c.add_argument("--span-max", type=int, default=0x4000)
    c.set_defaults(func=lambda a: cmd_stats(a) or 0)

    d = sub.add_parser("bench", help="scorecard used to iterate on this tool")
    d.add_argument("--seeds", type=int, default=400)
    d.add_argument("--seed", type=int, default=7)
    d.add_argument("--span-max", type=int, default=0x4000)
    d.add_argument("--force", action="store_true", help="rebuild the graph cache")
    d.add_argument("--json", action="store_true")
    d.add_argument("--save", default=None, help="write the scorecard here (e.g. build/tmp/tudiscover/baseline.json)")
    d.add_argument("--compare", default=None, help="diff the sweep against a saved scorecard")
    d.set_defaults(func=cmd_bench)

    args = ap.parse_args()
    sys.exit(args.func(args) or 0)


if __name__ == "__main__":
    main()
