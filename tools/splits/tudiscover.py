#!/usr/bin/env python3
"""Propose a TU boundary around an address from referrer runs, the pool model, data order and __FILE__ anchors.
Spec: docs/tools/spec/tudiscover.md. CLI: tudiscover.py at <addr|symbol> [--json] | stats | cache [--force] | dataorder |
prune [--include-obj --apply] | bench [--seeds N] [--compare F] | --selftest."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import collections
import hashlib
import json
import os
import re
import sys
import time

from tools.lib import project as _project  # the map / splits readers
from tools.lib import refs as _refs  # the dump's per-function graph and its stamp
from tools.lib import repo as _repo
import tools.splits.seams.evidence as ev  # the seam evidence: data order, the pool model, source names

GAME = ev.GAME
has_dump = _refs.has_dump

#: The tree's paths, resolved on first use (never at import) and assignable (`td.ASM_DIR = ...` pins one):
#: `ASM_DIR`/`DOL` are READ from the tree's own, else MAIN's by path (a fresh worktree has no `build/`);
#: `LOCAL_ASM_DIR` is the tree's own dump, the only place a tool WRITES.
_LAZY = ("ROOT", "SYMBOLS", "SPLITS", "LOCAL_ASM_DIR", "ASM_DIR", "DOL", "CACHE", "ASM_STAMP")


def _g(name):
    """The current value of one lazy tree path (an assigned value wins)."""
    g = globals()
    if name not in g:
        root = g.get("ROOT") or _repo.repo_root()
        g.setdefault("ROOT", root)
        g.setdefault("SYMBOLS", os.path.join(root, "config", GAME, "symbols.txt"))
        g.setdefault("SPLITS", os.path.join(root, "config", GAME, "splits.txt"))
        g.setdefault("LOCAL_ASM_DIR", os.path.join(root, "build", GAME, "asm"))
        if "ASM_DIR" not in g:
            g["ASM_DIR"] = _repo.resolve_input(os.path.join("build", GAME, "asm"), root, has_dump, honour_env=True)
        if "DOL" not in g:
            g["DOL"] = _repo.resolve_input(os.path.join("orig", GAME, "sys", "main.dol"), root, os.path.isfile,
                                           honour_env=True)
        g.setdefault("CACHE", os.path.join(root, "build", "tmp", "tudiscover", "graph.json"))
        g.setdefault("ASM_STAMP", os.path.join(g["ASM_DIR"], ".stamp.json"))
    return g[name]


def __getattr__(name):
    if name in _LAZY:
        return _g(name)
    raise AttributeError("module %r has no attribute %r" % (__name__, name))


SCHEMA = 10     # bump on any change to what `build_graph()` stores (the stamp also hashes the code)

# Section order for the printed `splits.txt` block (matches config/RMHE08/splits.txt's header).
SECTION_ORDER = [".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data",
                 ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2"]

TOKEN_RE = _refs.TOKEN_RE
CALL_RE = _refs.CALL_RE
SAVE_RE = _refs.SAVE_RE
REC_RE = _refs.REC_RE
SRCFILE_RE = ev.SOURCE_NAME_RE
ETI_RE = _refs.ETI_RE
FOURBYTE_RE = _refs.FOURBYTE_RE
# The whole-unit extent dtk prints in the header comment of every `.s`, in either spelling the tree
# uses: `# 0xSTART..0xEND | size: 0xN` (current dtk) and the pre-2025 `# 0xSTART - 0xEND`.
HEADER_RANGE_RE = re.compile(r"^#\s+(0x[0-9A-Fa-f]+)\s*(?:\.\.|-)\s*(0x[0-9A-Fa-f]+)\s*(?:\|.*)?$",
                             re.M)
# A function is exactly its `.fn <name>, ...` .. matching `.endfn <name>` span (nothing else).
FN_BLOCK_RE = _refs.FN_BLOCK_RE
# Just the names, for the cheap "does the symbol map know this file" test (`FN_BLOCK_RE.findall`
# returns (name, block) tuples and would compare tuples against the map).
FN_NAME_RE = _refs.FN_NAME_RE
# The address dtk prints in the leading comment of every instruction (`/* ADDR RELOC  bytes */`).
FIRST_INS_RE = _refs.FIRST_INS_RE
# `.obj <label>` blocks that declare where the addresses in their relocations live.
REL_OBJ_RE = _refs.REL_OBJ_RE
REL_OWNER_RE = _refs.REL_OWNER_RE
# Observation kinds by authority: `pool`/`source` pin a real boundary, the other two are weak.
STRONG = ("pool", "source")
#: `--data-order` modes: which `.data` emission-order seams (tools/splits/dataorder.py) become observations.
#: `on` (default) - V->S and zigzag as soft votes (they rank candidates and show as pins, but cannot move a
#: boundary alone); `strong` - V->S joins the strong kinds; `weak` - `on` plus V->D as a weak vote; `off`.
DATA_ORDER_MODES = ("off", "on", "strong", "weak")
DATA_ORDER_DEFAULT = "on"


def strong_kinds(mode, pool_model=None):
    """The observation kinds that may move a boundary on their own, for a `--data-order` / `--pool-model` mode."""
    pool_model = POOL_MODEL if pool_model is None else pool_model
    return STRONG + (("dataorder",) if mode == "strong" else ()) + (("pooldup",) if pool_model == "strong" else ())


#: `--pool-model` (docs/pool-seams.md): MWCC emits one literal pool per TU, one entry per value, so
#: `on` - a pooled literal (an `.sdata2` float/double, an `.sdata` string) read by several functions is one TU
#: (must-link) even when its value is unique, and one value at two pool addresses is a boundary (`pooldup`, soft);
#: `strong` - `pooldup` joins the strong kinds; `off` - only the older rule (a value copied elsewhere + span).
POOL_MODELS = ("off", "on", "strong")
#: a `pooldup` interval wider than this many functions names no boundary worth printing (it still votes in `score_cuts`)
POOL_DEDUPE_NARROW = 64
POOL_MODEL_DEFAULT = "on"
POOL_MODEL = POOL_MODEL_DEFAULT

# Tier 1: how many functions inside one claimed `.text` range may seed their own closure, and how
# many of the resulting distinct intervals the report lists (the rest are counted).
SEED_CAP = 64
INTERVAL_CAP = 4

# Sections whose labels must never be read as TU-shared data: `extab`/`extabindex` are per-function
# unwind table fragments whose `@eti_`/`@etb_` symbols are aliases covering arbitrary addresses in the
# section (real code loads `"@eti_8001FFF8"+0xA`), and `.init` is boot code owned by no game TU.
NO_REF_SECTIONS = _refs.NO_REF_SECTIONS




Dol = ev.Image   # address -> bytes for the retail image (`read` may run past a segment end)


def load_map():
    """(functions, labels): names -> records, straight from the symbol map (`evidence.map_tables`)."""
    return ev.map_tables(_g("SYMBOLS"))


# Duplicate/stale copies of one unit's asm that `asm_files()` resolved (refilled on every call).
ASM_COLLISIONS = []
ASM_RANGE_DUPS = []
ASM_NO_RANGE = []
ASM_SUBRANGE = []


def dedupe_ranges(paths, fns):
    """Drop the stale copy when two parsed files cover the same section range.

    `build/<version>/asm/` keeps a file per naming convention rather than per unit: an older run
    wrote `auto_fn_<ADDR>_text.s` and today's writes `auto_dtor_<ADDR>_text.s`, and both describe the
    same region - the header's `# 0xSTART..0xEND | size:` is the file's leading section range, so the
    range, not the stem, is what makes them copies.  Their stems differ, so the stem rule in
    `asm_files()` cannot see them; the shadowed file only adds its stale `fn_<ADDR>` to the `.fn`
    parse self-check's `outside_map` and costs a full parse.

    The copy whose `.fn` names the symbol map knows is the live one; among equals the newest mtime
    wins.  A group that cannot be decided keeps every file and is reported (never silently).  A file
    whose header cannot be read at all is **kept** and reported rather than dropped: the parse
    self-check must keep seeing a format change, not lose the file that would have shown it.
    """
    ranges, unkeyed = {}, []
    for path in paths:
        try:
            head = open(path, "rb").read(1024).decode("utf-8", "replace")
        except OSError:
            head = ""
        m = HEADER_RANGE_RE.search(head)
        if m:
            ranges[path] = (int(m.group(1), 16), int(m.group(2), 16))
        else:
            unkeyed.append(path)
            ASM_NO_RANGE.append(os.path.relpath(path, _g("ASM_DIR")))
    keep = list(unkeyed)
    by_range = collections.defaultdict(list)
    for path, key in ranges.items():
        by_range[key].append(path)
    for key, group in sorted(by_range.items()):
        if len(group) == 1:
            keep.append(group[0])
            continue
        scored = [(map_fn_hits(path, fns), os.path.getmtime(path), path) for path in group]
        best = max(s[0] for s in scored)
        top = max(s[1] for s in scored if s[0] == best)
        kept = [s[2] for s in scored if s[0] == best and s[1] == top]
        keep.extend(kept)
        ASM_RANGE_DUPS.append({"range": "0x%08X..0x%08X" % key,
                               "kept": [os.path.relpath(p, _g("ASM_DIR")) for p in kept],
                               "dropped": [os.path.relpath(s[2], _g("ASM_DIR"))
                                           for s in scored if s[2] not in kept],
                               "resolved": len(kept) == 1})

    # A second kind of leftover: the range is *contained* in another kept file's range (an older
    # naming pass wrote a sub-range with the pre-2025 header spelling, so the exact-range test above
    # cannot see it). Contained is only stale when the symbol map knows none of its `.fn` names -
    # a file whose functions are live is a real unit that happens to sit inside a bigger region.
    ordered = sorted(keep, key=lambda p: (ranges.get(p, (1 << 32, 0))[0],
                                          -ranges.get(p, (1 << 32, 0))[1]))
    pruned, max_end, max_path = [], -1, None
    for path in ordered:
        if path not in ranges:
            pruned.append(path)                 # headerless: never deduped, never judged
            continue
        start, end = ranges[path]
        if end <= max_end and map_fn_hits(path, fns) == 0:
            ASM_SUBRANGE.append({"range": "0x%08X..0x%08X" % (start, end),
                                 "kept": os.path.relpath(max_path, _g("ASM_DIR")),
                                 "dropped": os.path.relpath(path, _g("ASM_DIR"))})
            continue
        pruned.append(path)
        if end > max_end:
            max_end, max_path = end, path
    return sorted(pruned)


def map_fn_hits(path, fns):
    """How many of a file's `.fn` names the symbol map knows (full read; collision groups only)."""
    if not fns:
        return 0
    try:
        txt = open(path, "r", encoding="utf-8", errors="replace").read()
    except OSError:
        return 0
    return sum(1 for n in FN_NAME_RE.findall(txt) if n in fns)


def use_local_dump():
    """Point the dump at the tree's OWN `build/<game>/asm` - what the tool that WRITES the dump (`dump_asm.py`) calls first,
    so a fallback to MAIN's dump can never make it write into MAIN."""
    global ASM_DIR, ASM_STAMP
    ASM_DIR = _g("LOCAL_ASM_DIR")
    ASM_STAMP = os.path.join(ASM_DIR, ".stamp.json")


def dump_is_main_fallback():
    """True when the dump is read from another tree's `build/` (MAIN's) because this tree has none."""
    return _stamp().is_main_fallback()


def repo_rel(path):
    """`path` relative to the repo root, or as-is when that is not expressible (Windows drives)."""
    try:
        return os.path.relpath(path, _g("ROOT"))
    except ValueError:
        return path


def _stamp():
    """The dump's stamp (`lib.refs.DumpStamp`) over this module's current paths."""
    return _refs.DumpStamp(_g("ASM_DIR"), _g("SYMBOLS"), _g("SPLITS"), _g("DOL"), GAME, _g("LOCAL_ASM_DIR"), _g("ROOT"),
                           _g("ASM_STAMP"))


def dump_files():
    """Every `.s` in the dump. The directory also holds `.stamp.json`, which is not a unit."""
    return _refs.dump_files(_g("ASM_DIR"))


def dump_stamp():
    """What the dump in `build/<game>/asm/` is a function of: the three files dtk's split reads."""
    return _stamp().current()


def write_asm_stamp(extra=None):
    """Stamp the current dump (called by `tools/splits/dump_asm.py` after a split)."""
    return _stamp().write(extra)


def asm_stamp_status():
    """`(state, message)` for the dump: fresh / missing / unstamped / stale / truncated (`lib.refs.DumpStamp`)."""
    return _stamp().status()


def asm_files(fns=None):
    """Every unit's disassembly - one file per unit, never a stale duplicate.

    `build/<version>/asm/` can hold the same unit twice: a top-level copy (`asm/camellia.s`) and one
    under the unit's lib directory (`asm/Camellia/camellia.s`, the path `configure.py`/`objdiff.json`
    use for a claimed unit).  The copies disagree: the stale top-level `camellia.s` prints its save
    helper as `bl fn_80456DD4` where the canonical copy prints `bl _savegpr_14`, which silently
    zeroes the codegen fingerprint of `camellia_setup128`/`camellia_setup256`.  A top-level copy is
    therefore dropped whenever the same stem also exists in a subdirectory, and every collision is
    recorded in `ASM_COLLISIONS` (reported by `stats`, never silent).

    A second, independent kind of stale copy - one per *section range* rather than per stem, the
    `auto_fn_<ADDR>` files an older dtk run left beside today's `auto_dtor_<ADDR>` - is resolved by
    `dedupe_ranges()`.
    """
    out = []
    for dirpath, _dirs, names in os.walk(_g("ASM_DIR")):
        for n in names:
            if not n.endswith(".s"):
                continue
            if dirpath == _g("ASM_DIR") and n.startswith("auto_") and not n.endswith("_text.s"):
                continue          # per-function scaffolding / data dumps: not a unit's `.text`
            out.append(os.path.join(dirpath, n))
    del ASM_COLLISIONS[:]
    del ASM_RANGE_DUPS[:]
    del ASM_NO_RANGE[:]
    del ASM_SUBRANGE[:]
    by_stem = collections.defaultdict(list)
    for path in sorted(out):
        by_stem[os.path.basename(path)[:-len(".s")]].append(path)
    keep = []
    for stem, paths in sorted(by_stem.items()):
        top = [p for p in paths if os.path.dirname(p) == _g("ASM_DIR")]
        deep = [p for p in paths if os.path.dirname(p) != _g("ASM_DIR")]
        if top and deep:
            keep.extend(deep)          # the configured unit's directory wins, deterministically
            ASM_COLLISIONS.append({"stem": stem, "kept": deep, "dropped": top, "resolved": True})
        else:
            keep.extend(paths)
            if len(paths) > 1:          # two lib directories, one stem: keep both, but say so
                ASM_COLLISIONS.append({"stem": stem, "kept": paths, "dropped": [],
                                       "resolved": False})
    return dedupe_ranges(keep, fns)


rel_owners = _refs.rel_owners   # `data label -> function names it relocates against` (`.rel` lines)


AT_SPELLED_RE = _refs.AT_SPELLED_RE


resolve_name = _refs.resolve_name   # an asm operand token -> a symbol-map name, or None


def parse_fingerprint():
    """A fingerprint of the parsing code itself, for the graph-cache stamp.

    Without it the stamp binds only the inputs (symbols sha1, file count, byte total), so an edit to
    `FN_BLOCK_RE`/`rel_owners` that forgets to bump `SCHEMA` silently reuses a graph built by the old
    parser.  The regex patterns and the source text of the parse functions are both hashed, so *any*
    parse-relevant edit invalidates the cache.
    """
    parts = [FN_BLOCK_RE.pattern, FN_NAME_RE.pattern, REL_OBJ_RE.pattern, REL_OWNER_RE.pattern,
             ETI_RE.pattern, FIRST_INS_RE.pattern, TOKEN_RE.pattern, CALL_RE.pattern, SAVE_RE.pattern,
             REC_RE.pattern, FOURBYTE_RE.pattern, HEADER_RANGE_RE.pattern, AT_SPELLED_RE.pattern]
    try:
        import inspect
        for fn in (asm_files, dedupe_ranges, load_map, ev.map_tables, rel_owners, build_graph, resolve_name,
                   _refs.function_graph):
            parts.append(inspect.getsource(fn))
    except (OSError, TypeError, IOError):
        parts.append("source unavailable")
    return hashlib.sha1("\n".join(parts).encode("utf-8")).hexdigest()


def warn_parse(out):
    """A one-line stderr warning when the graph's own `.fn` self-check is unhappy."""
    c = out.get("fn_check")
    if c and (c["mismatched_count"] or c["nocode_count"] or c["unparsed_fn_lines"]):
        print("# warning: `.fn` parse self-check failed (%d mismatched, %d without code, %d "
              "unparsed) - `stats` has the details"
              % (c["mismatched_count"], c["nocode_count"], c["unparsed_fn_lines"]), file=sys.stderr)


def stale_files(out):
    """The stale split-tree files still on disk, as (absolute, relative) pairs.

    Read from the *cache*, not the module globals: a cache hit skips the walk, so the globals are
    empty then. Existence is checked, so a tree that has been pruned reports none.
    """
    rel = []
    for d in out.get("collisions") or []:
        if d.get("resolved"):
            rel += d.get("dropped") or []
    for d in out.get("range_dups") or []:
        if d.get("resolved"):
            rel += d.get("dropped") or []
    for d in out.get("subrange") or []:
        rel.append(d["dropped"])
    pairs = []
    for r in sorted(set(rel)):
        p = os.path.join(_g("ASM_DIR"), r)
        if os.path.exists(p):
            pairs.append((p, r))
    return pairs


def parse_report(out):
    """The `.fn` self-check and the asm-file collisions stored with a graph, as print lines."""
    c, col = out.get("fn_check"), out.get("collisions") or []
    rd = out.get("range_dups") or []
    lines = []
    if not c:
        lines.append("parse check        not in this cache - rebuild with --force")
    else:
        total = c["matched"] + c["mismatched_count"] + c["nocode_count"]
        lines.append("parse check        %d/%d `.fn` spans start at the map address of their function"
                     "  (%d mismatched, %d with no code, %d `.fn` names outside the map)"
                     % (c["matched"], total, c["mismatched_count"], c["nocode_count"],
                        c["outside_map"]))
        for name, got, want in c["mismatched"][:10]:
            lines.append("  mismatch         %-40s asm 0x%08X != map 0x%08X" % (name, got, want))
        if c["mismatched_count"] > len(c["mismatched"]):
            lines.append("  ...              %d more mismatched"
                         % (c["mismatched_count"] - len(c["mismatched"])))
        for name in c["nocode"][:5]:
            lines.append("  no code          %s" % name)
        if c["unparsed_fn_lines"]:
            lines.append("  UNPARSED         %d `.fn` line(s) did not parse as a span: %s"
                         % (c["unparsed_fn_lines"],
                            ", ".join("%s (%d lines, %d spans)" % tuple(m)
                                      for m in c["parse_misses"][:5])))
    if col:
        live = [d for d in col if any(os.path.exists(os.path.join(_g("ASM_DIR"), x))
                                      for x in (d.get("dropped") or []))]
        lines.append("asm duplicates     %d stem(s) exist in more than one asm directory (the"
                     " deeper, configured-unit copy is kept)%s:"
                     % (len(col), "  (all pruned)" if not live else ""))
        for dup in (live or [])[:10]:
            lines.append("  %-24s keep %s   drop %s%s"
                         % (dup["stem"], ", ".join(dup["kept"]),
                            ", ".join(dup["dropped"]) or "-",
                            "" if dup["resolved"] else "   UNRESOLVED (both kept)"))
    if rd:
        live = [d for d in rd if any(os.path.exists(os.path.join(_g("ASM_DIR"), x))
                                     for x in (d.get("dropped") or []))]
        lines.append("range duplicates   %d section range(s) are covered by more than one parsed"
                     " file (the copy whose `.fn` names the symbol map is kept)%s:"
                     % (len(rd), "  (all pruned)" if not live else ""))
        for dup in (live or [])[:10]:
            lines.append("  %-20s keep %-34s drop %s%s"
                         % (dup["range"], ", ".join(dup["kept"]),
                            ", ".join(dup["dropped"]) or "-",
                            "" if dup["resolved"] else "   UNRESOLVED (all kept)"))
        if len(rd) > 10:
            lines.append("  ...                  %d more (--json/`stats --force` has the rest)"
                         % (len(rd) - 10))
    sub = out.get("subrange") or []
    if sub:
        left = [d for d in sub if os.path.exists(os.path.join(_g("ASM_DIR"), d["dropped"]))]
        lines.append("sub-range copies   %d file(s) sit inside another file's range and name no"
                     " function the map knows%s:"
                     % (len(sub), "  (all pruned)" if not left else "  - `prune --apply` removes them"))
        for d in sub[:10]:
            lines.append("  %-20s keep %-30s drop %s" % (d["range"], d["kept"], d["dropped"]))
    if out.get("no_range"):
        lines.append("header unreadable  %d parsed file(s) have no recognisable `# 0xSTART..0xEND`"
                     " header (kept, not deduped): %s"
                     % (len(out["no_range"]), ", ".join(out["no_range"][:6])))
    return lines


def build_graph(fns, labels, force=False):
    """Per-function data references, calls and codegen fingerprint, from the disassembly."""
    files = asm_files(fns)
    if not files:
        # Measured 2026-09-27: a lane's first `tudiscover at` call printed "0 functions" and read as a
        # tool bug. The build leaves `build/<game>/asm/` off (`write_asm: false`), so the dump is a
        # separate step; name it instead of reporting an empty map as if it were a real answer.
        raise SystemExit(
            "no disassembly under %s - the asm dump is missing.\n"
            "  `tudiscover` reads build/%s/asm/, which the normal build does NOT write (config.yml sets\n"
            "  `write_asm: false`); it is a separate one-command, ~8 s step. Make it, then re-run:\n"
            "      python tools/splits/dump_asm.py\n"
            "  Reporting 0 functions is the missing dump, not an empty map."
            % (repo_rel(_g("ASM_DIR")), GAME))
    stamp = {"schema": SCHEMA, "symbols": hashlib.sha1(open(_g("SYMBOLS"), "rb").read()).hexdigest(),
             "files": len(files), "bytes": sum(os.path.getsize(f) for f in files),
             "parse": parse_fingerprint()}
    if not force and os.path.exists(_g("CACHE")):
        try:
            cached = json.load(open(_g("CACHE"), encoding="utf-8"))
            if cached.get("stamp") == stamp:
                print("# graph cache: build/tmp/tudiscover/graph.json (%d files)" % stamp["files"],
                      file=sys.stderr)
                warn_parse(cached)
                return cached
        except (ValueError, OSError):
            pass
    t0 = time.time()
    parsed = _refs.function_graph(files, fns, labels, _g("ASM_DIR"))
    graph, extab = parsed["funcs"], parsed["extab"]
    out = {"stamp": stamp, "files": len(files), "funcs": graph, "extab": extab,
           "owners": parsed["owners"],
           "collisions": [{k: [os.path.relpath(p, _g("ASM_DIR")) for p in v] if k in ("kept", "dropped")
                           else v for k, v in c.items()} for c in ASM_COLLISIONS],
           "range_dups": list(ASM_RANGE_DUPS),
           "subrange": list(ASM_SUBRANGE),
           "no_range": list(ASM_NO_RANGE),
           "fn_check": parsed["fn_check"]}
    os.makedirs(os.path.dirname(_g("CACHE")), exist_ok=True)
    json.dump(out, open(_g("CACHE"), "w", encoding="utf-8"))
    print("# graph: %d functions (%d with extab) from %d files in %.1fs -> build/tmp/tudiscover/graph.json"
          % (len(graph), len(extab), len(files), time.time() - t0), file=sys.stderr)
    warn_parse(out)
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


def is_pool_literal(lab):
    """An object MWCC pools per TU (`evidence.is_literal`): an `.sdata2` 4/8-byte object or an `.sdata` string literal."""
    return ev.is_literal(lab["section"], lab["size"], lab["kind"], lab.get("type", "object"))


def classify(labels, refs_of, ordered, addr, size, span_max, dup, pool=False):
    """private / ambiguous per label, with the reason kept for the report.

    `span_max` is in *bytes* of .text: every referrer of a private pooled constant is inside the one
    TU that owns it, so the referrer span is bounded by that TU's size.  A label whose referrers sit
    megabytes apart (`natNegMessageMagic`, an 8-byte `.sdata` object called from two distant clusters)
    is an ordinary cross-TU global however duplicated its value is.
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
        elif pool and is_pool_literal(lab) and len(users) > 1 and span <= span_max:
            # one pool per TU: a literal several functions read lives in one TU's pool whatever its value
            out[name] = ("private", "%s pool literal read by %d functions, span %d B"
                         % (lab["section"], len(users), span))
        elif len(users) > 1:
            out[name] = ("ambiguous", "%s, span %d B, no scope%s"
                         % (lab["section"], span, "" if name in dup else ", unique value"))
    return out


def pool_dedupe_cuts(labels, refs_of, dol):
    """`(observations, records)` for one `.sdata2` value held at two pool addresses (the compiler would reuse the first).

    Copies of a value (same section, same bytes) sorted by address belong to different TUs, in text order, so the
    boundary lies between the last referrer of one copy and the first of the next: cut indices
    `(last+1 .. first)`.  Referrers that interleave contradict the rule (the copies are cross-TU globals or the
    referrer graph is wrong): recorded as `overlap`, no observation.  Needs the DOL (`dol` None -> nothing).
    """
    if dol is None:
        return [], []
    seen = collections.defaultdict(list)
    for name, lab in labels.items():
        if name not in refs_of or not ev.is_value_witness(lab["section"], lab["size"], lab["kind"],
                                                         lab.get("type", "object")):
            continue          # only a typed `.sdata2` float/double proves a value (`evidence.is_value_witness`)
        raw = dol.read(lab["addr"], lab["size"])
        if raw:
            seen[(lab["section"], bytes(raw))].append((lab["addr"], name))
    soft, records = [], []
    for (section, raw), copies in sorted(seen.items(), key=lambda kv: min(a for a, _n in kv[1])):
        copies.sort()
        for (_a1, n1), (_a2, n2) in zip(copies, copies[1:]):
            r1, r2 = refs_of[n1], refs_of[n2]
            rec = {"section": section, "first": n1, "second": n2, "value": raw.hex()}
            if r1[-1] >= r2[0]:
                records.append({**rec, "status": "overlap"})
                continue
            records.append({**rec, "status": "pinned", "lo": r1[-1] + 1, "hi": r2[0]})
            soft.append((r1[-1] + 1, r2[0], 1.0, "pooldup",
                         "%s value copy %s -> %s" % (section, n1, n2)))
    return soft, records


def source_file_label(dol, labels, name):
    """A `.rodata` string label holding a bare source-file name, e.g. `ef_util.cpp`."""
    lab = labels[name]
    if lab["kind"] != "string":
        return None
    text = dol.cstr(lab["addr"], min(max(lab["size"], 4), 256)).decode("latin-1", "replace")
    return text.strip() if SRCFILE_RE.match(text.strip()) else None


def data_order_records(addr, size, refs_of, dol, mode, syms=None):
    """Map every `.data` emission-order seam to a `.text` interval (see the module docstring).

    Returns `(soft, records)`.  `soft` are `(lo, hi, weight, kind, why)` observations; `records` has one
    dict per seam with its status: `pinned` (a strict interval), `overlap` (the two sides' referrers
    interleave: the rule does not hold for this seam) or `no-referrers` (nothing to place it by).
    """
    do = ev
    if syms is None:
        syms = ev.classify_all(ev.map_rows(_g("SYMBOLS")), dol)
    # V->tail is no evidence at all (a vtable followed by strings and no later vtable may be an inline tail),
    # so only V->S, zigzag and (on request) V->D become observations.
    found = [f for f in do.seams(syms)
             if f["kind"] in ("V->S", "zigzag") or (mode == "weak" and f["kind"] == "V->D")]
    by_addr = {s.addr: i for i, s in enumerate(syms)}
    cut_at = {f["addr"] for f in found}
    cut_list = sorted(by_addr[a] for a in cut_at)

    def fn_index_of(a):
        lo, hi = 0, len(addr)
        while lo < hi:
            mid = (lo + hi) // 2
            if addr[mid] <= a:
                lo = mid + 1
            else:
                hi = mid
        i = lo - 1
        return i if i >= 0 and a < addr[i] + max(size[i], 4) else None

    def referrers(run):
        got = set()
        for s in run:
            got.update(refs_of.get(s.name, ()))
        if not got:
            for s in run:
                if s.kind == do.VTABLE and s.owner is not None and fn_index_of(s.owner) is not None:
                    got.add(fn_index_of(s.owner))
        return sorted(got)

    kind_of = {"V->S": ("dataorder", 1.0), "zigzag": ("dataorder-zz", 0.5), "V->D": ("dataorder-weak", 0.25)}
    soft, records = [], []
    for f in found:
        j = by_addr[f["addr"]]
        after = [syms[j]]
        gap = f["kind"] == "V->S"
        if gap:
            # A V->S row is a GAP [addr, latest): the first `tail` strings are the previous TU's inline tail and
            # the boundary lies somewhere after them, before the next vtable group (which is the new TU's).
            j2 = by_addr[f["latest"]]
            tail_syms = syms[j:j + f.get("tail", 0)]
            after = [syms[j2]]
            k = j2 + 1
            while k < len(syms) and syms[k].kind == do.VTABLE and syms[k].addr not in cut_at:
                after.append(syms[k])
                k += 1
        else:
            tail_syms = []
            k = j + 1
            while k < len(syms) and syms[k].kind == syms[j].kind and syms[k].addr not in cut_at:
                after.append(syms[k])
                k += 1
        # the run of vtables that ends the previous fragment (padding before the seam is skipped)
        k = j - 1
        while k >= 0 and syms[k].kind == do.DATA and syms[k].size <= do.PAD_MAX:
            k -= 1
        before = []
        while k >= 0 and syms[k].kind == do.VTABLE:
            before.append(syms[k])
            if syms[k].addr in cut_at:
                break
            k -= 1
        u, v = referrers(before), referrers(after)
        if tail_syms:
            u = sorted(set(u) | set(referrers(tail_syms)))
        if not gap:
            # The whole fragments on either side pin tighter than the vtable runs, but a global shared with
            # another TU breaks them: use them when they stay ordered, else keep the narrow pair.  (A V->S gap
            # gets no such refinement: the strings past the tail may belong to either side.)
            ci = cut_list.index(j)
            wu, wv = referrers(syms[(cut_list[ci - 1] if ci else 0):j]), \
                referrers(syms[j:(cut_list[ci + 1] if ci + 1 < len(cut_list) else len(syms))])
            if wu and wv and wu[-1] < wv[0]:
                u, v = wu, wv
        rec = {"addr": f["addr"], "seam": f["kind"], "before": f["before"], "after": f["after"]}
        if gap:
            rec.update(latest=f["latest"], width=f["width"], tail=f.get("tail", 0))
        if not u or not v:
            rec["status"] = "no-referrers"
        elif u[-1] < v[0]:
            kind, w = kind_of[f["kind"]]
            rec.update(status="pinned", lo=u[-1], hi=v[0])
            if gap:
                what = "V->S gap [0x%08X, 0x%08X) %s -> %s" % (f["addr"], f["latest"], f["before"],
                                                              syms[by_addr[f["latest"]]].name)
            else:
                what = "%s seam %s -> %s" % (f["kind"], f["before"], f["after"])
            soft.append((u[-1], v[0], w, kind, what))
        else:
            rec.update(status="overlap", u_last=u[-1], v_first=v[0])
        records.append(rec)
    return soft, records


def analyse(fns, labels, graph, dol, span_max, source_span_max=0x8000, data_order="off", do_syms=None,
            pool_model=None):
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

    # `.rel` ownership: which functions' addresses a data object's relocations point at.  Kept out
    # of `refs_of` on purpose - ownership is not a reference and must not link functions.
    owners_of = {}
    for name, own in graph.get("owners", {}).items():
        got = tuple(n for n in own if n in idx)
        if got:
            owners_of[name] = got

    pool_model = POOL_MODEL if pool_model is None else pool_model
    cls = classify(labels, refs_of, ordered, addr, size, span_max, duplicate_values(labels, dol),
                   pool=pool_model != "off")
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
    # Anchored per *name*, never per label: 5 names carry 3-5 copies of their own string (MWCC does
    # not pool a `__FILE__` literal once per TU here), and anchoring each copy nests intervals inside
    # each other.  A name is trusted only while its whole span holds no function referencing a
    # *different* file name - the direct fingerprint of a literal shared with a second TU (0 of 1804
    # functions do today) - and while the span stays within `--source-span-max`, the same size sanity
    # check `classify()` applies to a pooled constant.  A rejected name keeps its soft source-change
    # vote, so a mis-attributed name misvotes a boundary instead of forbidding one.
    file_of = {}
    for name in labels:
        src = source_file_label(dol, labels, name)
        if src and name in refs_of:
            file_of[name] = src
    by_name = collections.defaultdict(list)
    for name, src in file_of.items():
        by_name[src].append(name)
    src_of = collections.defaultdict(set)        # function index -> file names it references
    for name, src in file_of.items():
        for i in refs_of[name]:
            src_of[i].add(src)
    source_names = []
    for src, names_ in sorted(by_name.items(),
                              key=lambda kv: (min(refs_of[n][0] for n in kv[1]), kv[0])):
        lo, hi = min(refs_of[n][0] for n in names_), max(refs_of[n][-1] for n in names_)
        users = set()
        for n in names_:
            users.update(refs_of[n])
        span = addr[hi] + size[hi] - addr[lo]
        other = sorted({s for i in range(lo, hi + 1) for s in src_of.get(i, ()) if s != src})
        gaps = [i for i in range(lo, hi + 1) if i not in users]
        if other:
            reject = "contaminating referrer: %s" % ", ".join(other[:3])
        elif span > source_span_max:
            reject = "span %d B > --source-span-max 0x%X" % (span, source_span_max)
        else:
            reject = None
        source_names.append({"src": src, "labels": sorted(names_), "lo": lo, "hi": hi,
                             "refs": len(users), "copies": len(names_), "cuts": hi - lo,
                             "bytes": span, "gaps": len(gaps),
                             "gap_bytes": sum(size[i] for i in gaps),
                             "start": addr[lo], "end": addr[hi] + size[hi], "reject": reject})
        if reject is None:
            link(lo, hi, '"%s" (%d label(s), %d refs)' % (src, len(names_), len(users)))

    # The soft vote is unchanged: between adjacent labels' first referrers, so a rejected name still
    # votes - a literal spanning two TUs then misvotes a boundary, it does not merge them.
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

    # Pool dedupe: one value at two pool addresses is two TUs (MWCC keeps one entry per value per TU).
    pool_records = []
    if pool_model != "off":
        more, pool_records = pool_dedupe_cuts(labels, refs_of, dol)
        soft.extend(more)

    # Data emission order: a vtable followed by a string (or two ascending vtables) is a TU seam.
    order_records = []
    if data_order != "off":
        more, order_records = data_order_records(addr, size, refs_of, dol, data_order, do_syms)
        soft.extend(more)

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
            "cls": cls, "must_link": must_link, "soft": soft, "owners_of": owners_of,
            "source_names": source_names, "order_records": order_records, "pool_records": pool_records,
            "pool_model": pool_model, "strong_kinds": strong_kinds(data_order, pool_model)}


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
        # A cut index is `boundary before function c`, so an anchor (a, b) - "functions a..b are one
        # TU" - forbids cuts a < c <= b, NOT c == a: a cut before a does not separate a from b. Using
        # `<=` on the left wrongly vetoed the closure's own start, which pushed the suggested boundary
        # one function out (the 51-vs-50 seam on the Pl units, and LocateObject on RSO/runtime).
        for a, b, why in an["must_link"]:
            for c in rng:
                if a < c <= b and cand[c]["veto"] is None:
                    cand[c]["veto"] = why
        for d in cand.values():
            d["support"] = round(d["support"], 3)
            d["share"] = round(d["support"] / avail, 3) if avail else 0.0
            d["strong"] = [p for p in d["pins"] if p[0] in an.get("strong_kinds", STRONG)]
            d["rank"] = (len(d["strong"]), len(d["pins"]), d["share"])
        out.update(cand)
    return out


def short(why):
    """One clause of a reason, short enough for a table cell."""
    why = why.replace("source file change ", "")
    why = re.sub(r"^(\S+) run jump ", r"\1: ", why)
    return re.sub(r"\s*\(.*\)$", "", why)


def data_runs(an, labels, lo, hi):
    """The data a TU covering functions [lo, hi) plausibly owns: one contiguous run per section.

    A label counts either because a `.text` function in the range references it, or because its
    object's `.rel` lines name one (see `rel_owners`) - a jump table no instruction loads by name
    still belongs to the function whose addresses it holds.
    """
    inside = set(an["ordered"][lo:hi])
    out = {}
    for name, lab in labels.items():
        if lab["section"] in (".init", "extab", "extabindex"):
            continue                      # boot code and per-function unwind tables: see extab_runs
        users = an["refs_of"].get(name) or []
        fns_here = [an["ordered"][i] for i in users]
        here = [f for f in fns_here if f in inside]
        owned = bool([f for f in an["owners_of"].get(name, ()) if f in inside])
        if not here and not owned:
            continue
        out.setdefault(lab["section"], []).append((lab["addr"], lab["size"], name,
                                                  len(here) != len(fns_here), owned and not here))
    runs = {}
    for section, items in out.items():
        items.sort()
        start = items[0][0]
        end = max(a + s for a, s, _, _, _ in items)
        own = {n for _, _, n, _, _ in items}
        # a linker fragment is contiguous: every label inside the run belongs to the same TU
        filler = [n for n, lab in labels.items()
                  if lab["section"] == section and start <= lab["addr"] < end and n not in own]
        runs[section] = {"start": start, "end": end, "labels": len(items), "filler": len(filler),
                         "density": round(len(items) / (len(items) + len(filler)), 2),
                         "leak": sum(1 for _, _, _, leaks, _ in items if leaks),
                         "owned": sum(1 for _, _, _, _, only in items if only),
                         "owned_names": sorted(n for _, _, n, _, only in items if only),
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
    dol = Dol(_g("DOL"))
    an = analyse(fns, labels, graph, dol, args.span_max, args.source_span_max, args.data_order)
    ordered, idx = an["ordered"], an["idx"]

    if args.at in idx:
        seed = idx[args.at]
        target = args.at
    else:
        addr = int(args.at, 0) if not re.fullmatch(r"[0-9a-fA-F]+", args.at) else int(args.at, 16)
        hit = [i for i, n in enumerate(ordered)
               if fns[n]["addr"] <= addr < fns[n]["addr"] + max(fns[n]["size"], 4)]
        # A `.data` address that is a data-order seam names the `.text` interval it pins: seed at its
        # far end (the first referrer of the fragment the seam starts).
        seam = [r for r in an["order_records"] if r["addr"] == addr and r["status"] == "pinned"]
        if not hit and seam:
            hit = [seam[0]["hi"]]
            print("# 0x%08X is a %s seam in `.data`: the new TU's first code lies in .text 0x%08X..0x%08X"
                  % (addr, seam[0]["seam"], an["addr"][seam[0]["lo"]], an["addr"][seam[0]["hi"]]),
                  file=sys.stderr)
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
    srcs = sorted({r["src"] for r in an["source_names"]
                   if sug_lo <= r["start"] and r["start"] < sug_hi})

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
        "data_order": [{"addr": r["addr"], "seam": r["seam"], "before": r["before"], "after": r["after"],
                        "text": [an["addr"][r["lo"]], an["addr"][r["hi"]]], "functions": r["hi"] - r["lo"]}
                       for r in an["order_records"]
                       if r["status"] == "pinned" and r["hi"] >= sug_lo - args.window
                       and r["lo"] <= sug_hi + args.window],
        "pool_dedupe": sorted(
            ({"first": r["first"], "second": r["second"], "value": r["value"],
              "text": [an["addr"][r["lo"]], an["addr"][r["hi"]]], "functions": r["hi"] - r["lo"] + 1}
             for r in an["pool_records"]
             if r["status"] == "pinned" and r["hi"] >= sug_lo - args.window and r["lo"] <= sug_hi + args.window
             and r["hi"] - r["lo"] + 1 <= POOL_DEDUPE_NARROW),
            key=lambda r: (r["functions"], r["text"][0]))[:12],
        "pool_model": an["pool_model"],
        "pool_units": pool_fold_info(an["addr"][lo], an["addr"][hi - 1] + an["size"][hi - 1],
                                     an["addr"][sug_lo], an["addr"][sug_hi - 1] + an["size"][sug_hi - 1]),
    }
    if args.json:
        print(json.dumps(result, indent=2))
        return 0
    print_human(result, ordered, fns, sug_lo, sug_hi)
    if args.splits:
        # Hand off nothing that may have been built from a stale parse: the duplicates in the split
        # tree are the one input the tool can be silently wrong about (a stale copy won the parse for
        # camellia_setup128/256 once). `prune` is idempotent and only removes files dtk never rewrites.
        stale = stale_files(graph)
        if stale and not args.allow_stale:
            print("\n# HANDOFF BLOCKED: %d stale split-tree file(s) shadow current ones, so this block"
                  % len(stale))
            print("# could come from a stale parse. Remove them first, then hand off:")
            print("#   python tools/splits/tudiscover.py prune --apply")
            for _p, r in stale[:5]:
                print("#   %s" % r)
            if len(stale) > 5:
                print("#   ... and %d more (`prune` lists them all)" % (len(stale) - 5))
            print("# (read-only inspection of this proposal: add --allow-stale)")
            return 2
        print_splits(result, args.unit)
    return 0


def _clean(cand):
    if not cand:
        return None
    return {"cut": cand["cut"], "support": cand["support"], "share": cand["share"],
            "strong": [{"kind": k, "why": w} for k, w in cand["strong"]],
            "pins": [{"kind": k, "why": w} for k, w in cand["pins"][:6]], "veto": cand["veto"]}


def pool_fold_info(match_lo, match_hi, ext_lo, ext_hi):
    """The registered units a proposed range overlaps, and the pool-sharing groups they belong to.

    `scope` is `match` when the MATCH SET itself spans several registered units, `extended` when only the
    extended range does.  `{}` whenever the registry, the objects or the census are unreadable (never raises).
    """
    try:
        claims = claimed_units()
        hit = lambda lo, hi: sorted((c[".text"][0], u) for u, c in claims.items()  # noqa: E731
                                     if c[".text"][0] < hi and c[".text"][1] > lo)
        units, scope = [u for _a, u in hit(match_lo, match_hi)], "match"
        if len(units) < 2:
            units, scope = [u for _a, u in hit(ext_lo, ext_hi)], "extended"
        units = [os.path.splitext(u)[0] for u in units]
        out = {"units": units, "scope": scope, "groups": []}
        if units:
            from tools.units import poolseams  # noqa: PLC0415 - the pool census: evidence, not a dependency
            census = poolseams.load_census(_g("ROOT"))
            seen = set()
            for u in units:
                g = poolseams.group_of(census, u)
                if g and id(g) not in seen:
                    seen.add(id(g))
                    out["groups"].append(poolseams.fold_line(g))
        return out
    except Exception:  # noqa: BLE001 - evidence, not a dependency
        return {}


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
    if res.get("data_order"):
        print("`.data` emission-order seams pinning a TU start near here (the new TU's first code lies in the"
              " interval; a narrow one names the boundary):")
        for r in res["data_order"][:6]:
            print("  0x%08X %-6s .text 0x%08X..0x%08X (%d fn)  %s -> %s"
                  % (r["addr"], r["seam"], r["text"][0], r["text"][1], r["functions"], r["before"], r["after"]))
        print()
    pu = res.get("pool_units") or {}
    if pu.get("groups") or len(pu.get("units") or ()) > 1:
        print("pool model    the %s overlaps %d registered unit(s): %s" % (
            "MATCH SET" if pu["scope"] == "match" else "extended range", len(pu["units"]), ", ".join(pu["units"][:6])
            + (" ..." if len(pu["units"]) > 6 else "")))
        for line in pu.get("groups", [])[:3]:
            print("              %s" % line)
        if pu.get("groups"):
            print("              a pooled literal read by two registered units is one TU's pool entry (MWCC: one pool per TU,\n"
                  "              `mwld` does not merge pools): the claim is blocked by a seam, not by the tool - fold them.")
        print()
    if res.get("pool_dedupe"):
        print("pool dedupe   one value held at two pool addresses is two TUs (the compiler reuses a pooled value inside one TU):")
        for r in res["pool_dedupe"][:6]:
            print("  value %-16s %-14s -> %-14s new TU's first code in .text 0x%08X..0x%08X (%d fn)"
                  % (r["value"], r["first"], r["second"], r["text"][0], r["text"][1], r["functions"]))
        print()
    print("data the range would own (contiguous run per section; `dens` = share of labels inside the"
          " run that the range actually references, `leak` = also referenced from outside, `own` ="
          " claimed by the object's `.rel` lines with no reference at all):")
    for section in sorted(res["runs"], key=SECTION_ORDER.index):
        r = res["runs"][section]
        line = ("  %-10s 0x%08X..0x%08X  %3d labels  dens %.2f  leak %s"
                % (section, r["start"], r["end"], r["labels"], r.get("density", 1.0),
                   r["leak"] if r["leak"] else "0"))
        if r.get("owned"):
            line += "  own %d" % r["owned"]
        print(line)
        if r.get("owned_names"):
            names = r["owned_names"]
            print("             own: %s%s" % (", ".join(names[:6]),
                                              " (+%d more)" % (len(names) - 6) if len(names) > 6 else ""))
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
    out = {b.unit: {r.section: (r.start, r.end) for r in b.ranges} for b in _project.Splits.read(_g("SPLITS")).blocks}
    return {u: s for u, s in out.items() if ".text" in s}


def sampled(seq, cap):
    """At most `cap` evenly spaced items of `seq` (deterministic), plus whether it was truncated."""
    if len(seq) <= cap:
        return list(seq), False
    if cap <= 1:
        return list(seq[:cap]), True
    step = (len(seq) - 1) / (cap - 1)
    out = []
    for i in range(cap):
        j = int(i * step + 0.5)
        if not out or seq[j] != out[-1]:
            out.append(seq[j])
    return out, True


def tier_labels(an, fns, labels, seeds_per_unit=SEED_CAP, max_funcs=400):
    """Tier 1: every map function inside a claimed `.text` range seeds its own closure.

    One seed is one observation, not the answer: seeding only the range's first function made all
    three claimed units look like "start exact, end short", which was an artifact of that seed.  A
    unit is `consistent` only when all of its seeds close to the same interval, `disagrees` when
    they differ, and `no evidence` when every seed closes to itself - the range has no must-link
    anchor at all, so it says nothing about the boundary either way.
    """
    rows, classes = [], collections.Counter()
    for unit, secs in sorted(claimed_units().items()):
        start, end = secs[".text"]
        inside = [i for i, n in enumerate(an["ordered"]) if start <= fns[n]["addr"] < end]
        seeds, capped = sampled(inside, seeds_per_unit)
        closures = []
        for s in seeds:
            lo, hi, _touched, guard = expand(an, s, max_funcs)
            closures.append((lo, hi, bool(guard)))
        groups = collections.OrderedDict()
        for lo, hi, guard in sorted(closures, key=lambda c: c[:2]):
            rec = groups.setdefault((lo, hi), {
                "range": [lo, hi], "functions": hi - lo, "seeds": 0, "guard": False,
                "text": [an["addr"][lo], an["addr"][hi - 1] + an["size"][hi - 1]]})
            rec["seeds"] += 1
            rec["guard"] = rec["guard"] or guard
        # `no evidence` first: a unit whose every seed closes to itself has no evidence even when
        # the seeds disagree wildly (they are just different singletons).
        anchored = [c for c in closures if c[1] - c[0] > 1]
        klass = ("no evidence" if not anchored else
                 "consistent" if len(groups) == 1 else "disagrees")
        classes[klass] += 1
        # Worst case against the claimed range, in functions: dropped at each edge, pulled in from
        # outside it.  A claimed range holding no map function uses its insertion point.
        if inside:
            c_lo, c_hi = inside[0], inside[-1] + 1
        else:
            c_lo = c_hi = sum(1 for i, n in enumerate(an["ordered"]) if fns[n]["addr"] < start)
        worst = [0, 0, 0, 0]
        for lo, hi, _guard in closures:
            worst[0] = max(worst[0], max(0, min(lo, c_hi) - c_lo))
            worst[1] = max(worst[1], max(0, c_hi - max(hi, c_lo)))
            worst[2] = max(worst[2], max(0, c_lo - lo))
            worst[3] = max(worst[3], max(0, hi - c_hi))
        row = {"unit": unit, "claimed": [start, end], "claimed_functions": len(inside),
               "seeds": len(seeds), "seed_cap": seeds_per_unit, "seed_cap_binds": capped,
               "distinct_intervals": len(groups), "class": klass,
               "intervals": list(groups.values())[:INTERVAL_CAP],
               "intervals_omitted": max(0, len(groups) - INTERVAL_CAP),
               "worst_missing_start": worst[0], "worst_missing_end": worst[1],
               "worst_extra_start": worst[2], "worst_extra_end": worst[3],
               "guard": any(g for _lo, _hi, g in closures)}
        if closures:                       # the first seed is still the old single-seed number
            lo, hi = closures[0][0], closures[0][1]
            got = (an["addr"][lo], an["addr"][hi - 1] + an["size"][hi - 1])
            runs = data_runs(an, labels, lo, hi)
            row.update({"predicted": list(got), "function_delta": (hi - lo) - len(inside),
                        "start_off": got[0] - start, "end_off": got[1] - end,
                        "runs": {k: [v["start"], v["end"]] for k, v in runs.items()}})
        rows.append(row)
    return rows, {"units": len(rows), "seed_cap": seeds_per_unit,
                  "capped_units": [r["unit"] for r in rows if r["seed_cap_binds"]],
                  "classes": {k: classes[k] for k in ("consistent", "disagrees", "no evidence")},
                  "units_with_evidence": sum(1 for r in rows if r["class"] != "no evidence")}


def tier_sweep(an, labels, seeds, seed, max_funcs=400):
    """Tier 2: closure behaviour over random seeds (over-merging canary, run quality)."""
    import random
    rnd = random.Random(seed)
    picks = rnd.sample(range(len(an["ordered"])), min(seeds, len(an["ordered"])))
    sizes, guard, clean, dens = [], 0, 0, []
    for s in picks:
        lo, hi, _, g = expand(an, s, max_funcs)
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


def tier_consistency(an, seeds, seed, max_funcs=400):
    """Tier 3: distinct closures that partially overlap = the anchor set contradicts itself."""
    import random
    rnd = random.Random(seed + 1)
    picks = rnd.sample(range(len(an["ordered"])), min(seeds, len(an["ordered"])))
    spans = sorted({expand(an, s, max_funcs)[:2] for s in picks})
    merged, partial = [], 0
    for lo, hi in spans:
        while merged and lo < merged[-1][1] < hi:
            partial += 1
            lo = min(lo, merged[-1][0])
            merged.pop()
        merged.append((lo, hi))
    return {"distinct_closures": len(spans), "partial_overlaps": partial}


def tier_pins(an, claimed=None):
    """Tier 4: how well the *narrow* strong observations (the pins `score_cuts` counts) name a real boundary.

    `splits.txt` is the truth, and it only knows boundaries between registered units: a pinned cut at a
    claimed unit's `.text` start or end is a `hit`, one strictly inside a claimed unit's `.text` is a `miss`
    (a unit can be several TUs, so a miss is a candidate defect of the rule *or* a hidden seam), and one in
    unclaimed `.text` is `unknown` and counts for neither.  `precision` = hit / (hit + miss); `recall` = the
    share of claimed unit starts that carry a strong pin at exactly that cut.  Reported per observation kind.
    """
    claimed = claimed if claimed is not None else claimed_units()
    addr = an["addr"]
    edges, inner = set(), []
    for secs in claimed.values():
        t0, t1 = secs[".text"]
        edges.update((t0, t1))
        inner.append((t0, t1))
    inner.sort()
    starts = sorted(a for a, _b in inner)

    def truth(a):
        if a in edges:
            return "hit"
        for t0, t1 in inner:
            if t0 < a < t1:
                return "miss"
            if t0 > a:
                break
        return "unknown"

    per, cuts_of = {}, {}
    for olo, ohi, _w, kind, _why in an["soft"]:
        if kind not in an.get("strong_kinds", STRONG) or ohi - olo + 1 > 4:
            continue
        for c in range(olo, ohi + 1):
            cuts_of.setdefault(kind, set()).add(c)
    all_cuts = set()
    for kind, cs in sorted(cuts_of.items()):
        cnt = collections.Counter(truth(addr[c]) for c in cs)
        per[kind] = {"cuts": len(cs), "hit": cnt["hit"], "miss": cnt["miss"], "unknown": cnt["unknown"],
                     "precision": round(cnt["hit"] / max(1, cnt["hit"] + cnt["miss"]), 3)}
        all_cuts |= cs
    cnt = collections.Counter(truth(addr[c]) for c in all_cuts)
    hit_addr = {addr[c] for c in all_cuts}
    return {"per_kind": per, "hit": cnt["hit"], "miss": cnt["miss"], "unknown": cnt["unknown"],
            "precision": round(cnt["hit"] / max(1, cnt["hit"] + cnt["miss"]), 3),
            "claimed_starts": len(starts), "starts_pinned": sum(1 for a in starts if a in hit_addr),
            "recall": round(sum(1 for a in starts if a in hit_addr) / max(1, len(starts)), 3)}


def cmd_bench(args):
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=args.force)
    an = analyse(fns, labels, graph, Dol(_g("DOL")), args.span_max, args.source_span_max, args.data_order)
    rows, classes = tier_labels(an, fns, labels, args.seeds_per_unit, args.max_funcs)
    score = {"labels": rows, "label_classes": classes,
             "sweep": tier_sweep(an, labels, args.seeds, args.seed, args.max_funcs),
             "consistency": tier_consistency(an, args.seeds, args.seed, args.max_funcs),
             "pins": tier_pins(an), "data_order": args.data_order,
             "span_max": args.span_max, "source_span_max": args.source_span_max,
             "seeds": args.seeds, "seed": args.seed}
    if args.json:
        print(json.dumps(score, indent=2))
    else:
        print("tier 1  claimed units (splits.txt as truth): every map function in the range seeds"
              " a closure; one seed is one observation, not the answer")
        for r in score["labels"]:
            print("  %-46s %2d seeds  %2d intervals  %s%s"
                  % (r["unit"], r["seeds"], r["distinct_intervals"], r["class"].upper(),
                     "  (seed cap)" if r["seed_cap_binds"] else ""))
            if "predicted" in r:
                print("      %d claimed fn; first-seed closure delta %+d fn  start %+d B  end %+d B"
                      % (r["claimed_functions"], r["function_delta"], r["start_off"], r["end_off"]))
            else:
                print("      %d claimed fn; the range contains no map function at all"
                      % r["claimed_functions"])
            print("      worst case %d fn missing at the start, %d at the end (%+d/%+d outside)"
                  % (r["worst_missing_start"], r["worst_missing_end"],
                     r["worst_extra_start"], r["worst_extra_end"]))
            for iv in r["intervals"]:
                print("      interval 0x%08X..0x%08X  %4d fn  %d seed(s)%s"
                      % (iv["text"][0], iv["text"][1], iv["functions"], iv["seeds"],
                         "  GUARD" if iv["guard"] else ""))
            if r["intervals_omitted"]:
                print("      ... and %d more distinct interval(s) (--json for the full list)"
                      % r["intervals_omitted"])
        lc = score["label_classes"]
        print("  classes  consistent %d   disagrees %d   no evidence %d"
              % (lc["classes"]["consistent"], lc["classes"]["disagrees"],
                 lc["classes"]["no evidence"]))
        print("  %d of %d claimed units have any closure evidence at all%s"
              % (lc["units_with_evidence"], lc["units"],
                 "  (seed cap: %s)" % ", ".join(lc["capped_units"]) if lc["capped_units"] else ""))
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
        pn = score["pins"]
        print("tier 4  strong pins vs splits.txt (--data-order %s): precision %.3f (hit %d, miss %d, unknown %d),"
              " recall %.3f (%d of %d claimed unit starts pinned)"
              % (args.data_order, pn["precision"], pn["hit"], pn["miss"], pn["unknown"], pn["recall"],
                 pn["starts_pinned"], pn["claimed_starts"]))
        for kind, row in pn["per_kind"].items():
            print("        %-10s %4d cuts  hit %3d  miss %3d  unknown %4d  precision %.3f"
                  % (kind, row["cuts"], row["hit"], row["miss"], row["unknown"], row["precision"]))
    if args.save:
        path = args.save if os.path.isabs(args.save) else os.path.join(_g("ROOT"), args.save)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        json.dump(score, open(path, "w", encoding="utf-8"))
        print("# saved to %s" % os.path.relpath(path, _g("ROOT")), file=sys.stderr)
    if args.compare:
        old = json.load(open(args.compare if os.path.isabs(args.compare)
                             else os.path.join(_g("ROOT"), args.compare), encoding="utf-8"))
        print("# vs %s" % args.compare, file=sys.stderr)
        for key in ("median", "p75", "p90", "max", "guard_hits", "singleton_share",
                    "leak_free_share", "mean_density"):
            now, was = score["sweep"][key], old["sweep"][key]
            better_up = key in ("leak_free_share", "mean_density")
            flag = "" if (now >= was if better_up else now <= was) else "  <-- worse"
            print("  %-16s %s -> %s%s" % (key, was, now, flag))
        print("  %-16s %s -> %s" % ("partial_overlaps", old["consistency"]["partial_overlaps"],
                                     score["consistency"]["partial_overlaps"]))
        if "pins" in old:
            for key in ("precision", "recall"):
                now, was = score["pins"][key], old["pins"][key]
                print("  pins %-11s %s -> %s%s" % (key, was, now, "" if now >= was else "  <-- worse"))
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


def source_anchors(an, top=10):
    """The `__FILE__`-style anchors, widest `.text` span first, with the guard's verdict.

    One source file per TU makes each distinct name a must-link anchor, but the literal could also
    have been *shared* (linker string pooling) - then its referrers straddle several TUs and the
    anchor over-merges them.  `copies` is the first discriminator: MWCC emits one copy of a literal
    per TU that uses it (measured on this image as 16 copies of the generic `NW4R:Failed assertion
    0` string, while a `.cpp` name has one), so several copies of one name are several emitters, not
    a merged literal.  The guards are the second: a function inside the span referencing a
    *different* name, and a span past `--source-span-max`.  The rows come from `analyse()`, so the
    table and the anchors cannot disagree.
    """
    rows = sorted(an["source_names"], key=lambda r: (-r["bytes"], r["src"]))
    return rows[:top], rows


def cmd_stats(args):
    state, msg = asm_stamp_status()
    print("asm dump           %s" % msg)
    if state != "fresh":
        print("WARNING: the asm dump is %s - every number below describes that dump, not the current"
              " map; run `python tools/splits/dump_asm.py` first" % state, file=sys.stderr)
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=args.force)
    dol = Dol(_g("DOL"))
    an = analyse(fns, labels, graph, dol, args.span_max, args.source_span_max, args.data_order)
    covered = set(graph["funcs"])
    print("functions in map   %d" % len(fns))
    print("functions in asm   %d  (map entries with no `.fn` block in a parsed file: %d)"
          % (len(covered), len(set(fns) - covered)))
    for line in parse_report(graph):
        print(line)
    kinds = collections.Counter(v[0] for v in an["cls"].values())
    print("labels classified  %s" % dict(kinds))
    owners = graph.get("owners", {})
    print("labels owned       %d via `.rel` lines (%d with more than one owner)"
          % (sum(1 for v in owners.values() if v), sum(1 for v in owners.values() if len(v) > 1)))
    print("must-link anchors  %d   soft observations %d" % (len(an["must_link"]), len(an["soft"])))
    print("soft by kind       %s" % dict(collections.Counter(o[3] for o in an["soft"])))
    print("candidate cuts     %d function boundaries in .text" % len(an["ordered"]))
    rows, all_rows = source_anchors(an)
    rejected = [r for r in all_rows if r["reject"]]
    print("source anchors     %d `.c`/`.cpp` file names are referenced by .text functions; %d"
          " anchored, %d rejected by the guard (--source-span-max 0x%X); widest %d by span:"
          % (len(all_rows), len(all_rows) - len(rejected), len(rejected), args.source_span_max,
             len(rows)))
    print("  %-34s %4s %4s %5s %8s %5s %8s  %s"
          % ("source file", "lbls", "refs", "cuts", "span B", "gaps", "gap B", "verdict"))
    for r in rows:
        print("  %-34s %4d %4d %5d %8d %5d %8d  %s"
              % (r["src"], r["copies"], r["refs"], r["cuts"], r["bytes"], r["gaps"],
                 r["gap_bytes"], r["reject"] or ("contiguous" if not r["gaps"] else
                                                  "accepted, %d gap fn" % r["gaps"])))
    for r in [r for r in rejected if r not in rows]:
        print("  rejected           %-34s %4d %4d %5d %8d   %s"
              % (r["src"], r["copies"], r["refs"], r["cuts"], r["bytes"], r["reject"]))
    print("  lbls = string labels carrying that name in the image (MWCC pools a literal per TU, so"
          " several copies = several emitters, not one merged literal)")
    print("  gaps = functions inside the span that do not reference it (a TU's assert-free"
          " functions); anchor count is one per accepted name, not per label")
    return 0


def data_order_stats(an, fns, claimed=None):
    """How the `.data` emission-order seams sit against the other evidence (read-only, no ground truth claimed).

    Per seam: `pinned` / `overlap` / `no-referrers`, then for a pinned one whether the cuts its interval admits
    (`lo+1..hi`) are *all* vetoed by a must-link (`contradicted`, and `source` when a `__FILE__` anchor does it),
    whether a registered unit's `.text` start falls in the interval (`at_unit_start`: independent agreement) or
    the interval lies wholly inside one registered unit (`inside_unit`: the unit hides a seam, or the rule is
    wrong there).
    """
    addr = an["addr"]
    claimed = claimed if claimed is not None else claimed_units()
    starts = {secs[".text"][0] for secs in claimed.values()}
    texts = sorted((secs[".text"][0], secs[".text"][1], unit) for unit, secs in claimed.items())
    out = {"seams": len(an["order_records"]), "by_status": collections.Counter(), "by_seam": collections.Counter(),
           "contradicted": [], "source_contradicted": [], "at_unit_start": [], "inside_unit": [], "rows": []}
    for r in an["order_records"]:
        out["by_status"][r["status"]] += 1
        out["by_seam"][(r["seam"], r["status"])] += 1
        row = dict(r)
        if r["status"] == "pinned":
            cuts = range(r["lo"] + 1, r["hi"] + 1)
            veto = {c: [w for a, b, w in an["must_link"] if a < c <= b] for c in cuts}
            row["cuts"] = [addr[c] for c in cuts]
            if cuts and all(veto[c] for c in cuts):
                row["contradicted"] = sorted({w for c in cuts for w in veto[c]})[:3]
                out["contradicted"].append(row)
                if all(any(w.startswith('"') for w in veto[c]) for c in cuts):
                    out["source_contradicted"].append(row)
            row["at_unit_start"] = any(addr[c] in starts for c in cuts)
            if row["at_unit_start"]:
                out["at_unit_start"].append(row)
            t0, t1 = addr[r["lo"]], addr[r["hi"]]
            for a, b, unit in texts:
                if a < t0 and t1 < b:
                    row["inside_unit"] = unit
                    out["inside_unit"].append(row)
                    break
        elif r["status"] == "overlap":
            # the two sides' referrers interleave: a `__FILE__` anchor covering both ends says the source
            # file spans the seam, i.e. the rule is contradicted by the strongest must-link there is
            lo, hi = sorted((r["u_last"], r["v_first"]))
            cover = [w for a, b, w in an["must_link"] if w.startswith('"') and a <= lo and hi <= b]
            if cover:
                row["contradicted"] = cover[:3]
                out["contradicted"].append(row)
                out["source_contradicted"].append(row)
        out["rows"].append(row)
    return out


def cmd_dataorder(args):
    """List every `.data` emission-order seam with its `.text` interval and how it sits against the other evidence."""
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=False)
    an = analyse(fns, labels, graph, Dol(_g("DOL")), args.span_max, args.source_span_max,
                 "weak" if args.weak else args.data_order if args.data_order != "off" else "on")
    st = data_order_stats(an, fns)
    ranges = ev.section_ranges(_g("SPLITS"), ".data")
    print("seams %d   status %s" % (st["seams"], dict(st["by_status"])))
    print("by seam kind/status %s" % {"%s/%s" % k: v for k, v in sorted(st["by_seam"].items())})
    print("pinned intervals agreeing with a registered unit's .text start: %d" % len(st["at_unit_start"]))
    print("pinned intervals wholly inside one registered unit (a hidden seam, or the rule is wrong): %d"
          % len(st["inside_unit"]))
    print("contradicted by a must-link: %d (by a `__FILE__` anchor: %d)"
          % (len(st["contradicted"]), len(st["source_contradicted"])))
    if args.json:
        print(json.dumps({k: v for k, v in st.items() if k != "rows"}, indent=1, default=str))
        return 0
    for r in st["rows"]:
        if args.addr and r["addr"] != args.addr:
            continue
        u = ev.range_of(ranges, r["addr"])
        line = "0x%08X %-6s %-12s" % (r["addr"], r["seam"], r["status"])
        if r["status"] == "pinned":
            line += " .text 0x%08X..0x%08X" % (an["addr"][r["lo"]], an["addr"][r["hi"]])
            line += " %s%s%s" % ("[unit start] " if r.get("at_unit_start") else "",
                                 "[INSIDE %s] " % r["inside_unit"] if r.get("inside_unit") else "",
                                 "[CONTRADICTED: %s]" % "; ".join(r["contradicted"]) if r.get("contradicted") else "")
        if r["status"] == "overlap" and r.get("contradicted"):
            line += " [CONTRADICTED by %s]" % "; ".join(r["contradicted"])
        if r.get("latest"):
            line += " gap [0x%08X,0x%08X) width %d tail %d" % (r["addr"], r["latest"], r["width"], r["tail"])
        line += "  (%s -> %s)%s" % (r["before"], r["after"], "  in %s" % u[0] if u else "")
        if args.all or args.addr or r.get("contradicted") or u or r["status"] == "overlap":
            print(line)
    return 0


def selftest():
    """Fixtures only: the data-order observation on a synthetic map, no asm dump and no DOL."""
    do = ev
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # ten 0x10-byte functions f0..f9; TU1 = f0..f4, TU2 = f5..f9
    addr = [0x80010000 + 0x10 * i for i in range(10)]
    size = [0x10] * 10

    # TU1: strings s1, vtables vB, vA (owners down), inline tail `x_ac.h`; TU2: string s2, vtable vC
    def sym(a, sz, name, kind, owner=None, text=None):
        return do.Sym(a, sz, name, kind, owner, text)

    syms = [sym(0x100, 16, "s1", do.STRING), sym(0x110, 16, "vB", do.VTABLE, addr[3]),
            sym(0x120, 16, "vA", do.VTABLE, addr[1]), sym(0x130, 16, "hdr", do.STRING, None, "x_ac.h"),
            sym(0x140, 16, "s2", do.STRING, None, "a message"), sym(0x150, 16, "vC", do.VTABLE, addr[7])]
    refs = {"s1": [0, 2], "vB": [3], "vA": [1], "hdr": [2], "s2": [6], "vC": [5, 7]}
    soft, rec = data_order_records(addr, size, refs, None, "on", syms)
    check("V->S gap becomes one pinned observation: last vtable/tail referrer .. next vtable's first referrer",
          [(o[0], o[1], o[3]) for o in soft], [(3, 5, "dataorder")])
    check("... the record carries the gap (first string, latest = next vtable, width, tail)",
          [(r["addr"], r["seam"], r["status"], r["latest"], r["width"], r["tail"]) for r in rec],
          [(0x130, "V->S", "pinned", 0x150, 2, 1)])
    check("... the strings past the tail (TU2's, or an undetected tail) do not move the interval",
          [(o[0], o[1]) for o in data_order_records(addr, size, dict(refs, s2=[9]), None, "on", syms)[0]], [(3, 5)])
    check("... a tail string referenced later in .text widens the interval's lower end",
          [(o[0], o[1]) for o in data_order_records(addr, size, dict(refs, hdr=[4]), None, "on", syms)[0]], [(4, 5)])
    check("dataorder is soft by default and strong only with --data-order strong; zigzag/weak never",
          [(m, [k for k in ("dataorder", "dataorder-zz", "dataorder-weak") if k in strong_kinds(m)])
           for m in ("off", "on", "strong", "weak")],
          [("off", []), ("on", []), ("strong", ["dataorder"]), ("weak", [])])

    # interleaved referrers: the rule does not hold, the seam is recorded as overlap and adds no observation
    soft, rec = data_order_records(addr, size, dict(refs, vC=[2, 7]), None, "on", syms)
    check("interleaved referrers: overlap, no observation", (soft, [r["status"] for r in rec]), ([], ["overlap"]))

    # nothing references the vtable runs: their owner functions stand in
    soft, rec = data_order_records(addr, size, {}, None, "on", syms)
    check("no referrer: the vtables' owner functions stand in (and the tail has none)",
          [(o[0], o[1]) for o in soft], [(3, 7)])
    bare = [sym(x.addr, x.size, x.name, x.kind, None, x.text) for x in syms]
    soft, rec = data_order_records(addr, size, {}, None, "on", bare)
    check("nothing references either side: no-referrers", (soft, [r["status"] for r in rec]), ([], ["no-referrers"]))

    # V->tail (a vtable, strings, and no later vtable) is no evidence: it is not an observation and has no record
    tl = syms[:5]
    soft, rec = data_order_records(addr, size, refs, None, "weak", tl)
    check("V->tail is no evidence, even with --data-order weak", (soft, rec), ([], []))

    # zigzag: two vtables whose owners go up
    zz = [sym(0x100, 16, "vA", do.VTABLE, addr[1]), sym(0x110, 16, "vB", do.VTABLE, addr[7])]
    soft, rec = data_order_records(addr, size, {"vA": [1], "vB": [6]}, None, "on", zz)
    check("zigzag is a soft `dataorder-zz` observation", [(o[2], o[3]) for o in soft], [(0.5, "dataorder-zz")])

    # V->D is off by default and weak when asked for
    vd = [sym(0x100, 16, "vA", do.VTABLE, addr[1]), sym(0x110, 64, "tbl", do.DATA)]
    soft, rec = data_order_records(addr, size, {"vA": [1], "tbl": [6]}, None, "on", vd)
    check("V->D is off by default", (soft, rec), ([], []))
    soft, rec = data_order_records(addr, size, {"vA": [1], "tbl": [6]}, None, "weak", vd)
    check("V->D is a weak vote with --data-order weak", [(o[2], o[3]) for o in soft], [(0.25, "dataorder-weak")])

    # the observation reaches the scoring: a strong pin names the cut, and a must-link vetoes it
    an = {"soft": [(3, 6, 1.0, "dataorder", "V->S seam")], "must_link": [], "ordered": list("abcdefghij"),
          "addr": addr, "size": size, "strong_kinds": strong_kinds("on")}
    cands = score_cuts(an, 5, 6, 4)
    check("soft by default: a dataorder pin is listed but is not strong",
          ([c for c in cands.values() if c["strong"]], any(c["pins"] for c in cands.values())), ([], True))
    an["strong_kinds"] = strong_kinds("strong")
    cands = score_cuts(an, 5, 6, 4)
    pool = [c for c in cands.values() if c["strong"] and not c["veto"]]
    check("--data-order strong: a dataorder observation is a strong pin", len(pool) > 0, True)
    an["must_link"] = [(2, 6, '"file.c" (1 label(s), 2 refs)')]
    cands = score_cuts(an, 5, 6, 4)
    check("... but a must-link anchor across it vetoes every cut", [c for c in cands.values() if c["strong"] and not c["veto"]], [])

    an = {"order_records": [{"addr": 0x130, "seam": "V->S", "before": "vB", "after": "s2", "status": "pinned",
                             "lo": 3, "hi": 6},
                            {"addr": 0x140, "seam": "V->S", "before": "x", "after": "y", "status": "pinned",
                             "lo": 1, "hi": 2},
                            {"addr": 0x150, "seam": "V->S", "before": "p", "after": "q", "status": "overlap",
                             "u_last": 6, "v_first": 3}],
          "must_link": [(3, 6, '"file.c" (1 label(s), 2 refs)'), (1, 2, "sym [private]")], "addr": addr}
    st = data_order_stats(an, None, {"u/a": {".text": (addr[0], addr[3])}, "u/b": {".text": (addr[5], addr[9])}})
    check("stats: a source anchor across the interval (or across interleaved referrers) is a source contradiction",
          [r["addr"] for r in st["source_contradicted"]], [0x130, 0x150])
    check("... and any must-link is a contradiction", [r["addr"] for r in st["contradicted"]], [0x130, 0x140, 0x150])
    check("... an interval holding a registered unit start agrees with it",
          [r["addr"] for r in st["at_unit_start"]], [0x130])
    check("... status counts", dict(st["by_status"]), {"pinned": 2, "overlap": 1})

    # pool model (docs/pool-seams.md): a shared pooled literal is one TU even with a unique value, and one value at
    # two pool addresses is a boundary between the referrers of the two copies
    class FakeDol:
        def __init__(self, words):
            self.words = words

        def read(self, a, n):
            return self.words.get(a)

        def cstr(self, a, limit=256):
            return b""

    def lab(section, a, size, kind="float"):
        return {"section": section, "addr": a, "size": size, "kind": kind, "local": False}

    plabels = {"u1": lab(".sdata2", 0x8000, 4), "u2": lab(".sdata2", 0x8004, 4), "c1": lab(".sdata2", 0x8008, 4),
               "c2": lab(".sdata2", 0x800C, 4), "tbl": lab(".sdata2", 0x8010, 0x20, ""),
               "str": lab(".sdata", 0x9000, 3, "string"), "var": lab(".sdata", 0x9008, 4, "4byte")}
    prefs = {"u1": [0, 1], "u2": [2], "c1": [0, 1], "c2": [3, 4], "tbl": [0, 5], "str": [0, 1], "var": [0, 1]}
    porder = ["f%d" % i for i in range(6)]
    paddr = [0x1000 + 0x100 * i for i in range(6)]
    psize = [0x100] * 6
    check("is_pool_literal: sdata2 4/8 bytes and sdata strings only",
          [n for n, l in plabels.items() if is_pool_literal(l)], ["u1", "u2", "c1", "c2", "str"])
    off = classify(plabels, prefs, porder, paddr, psize, 0x4000, set(), pool=False)
    on = classify(plabels, prefs, porder, paddr, psize, 0x4000, set(), pool=True)
    check("old rule: a unique-valued shared literal is only ambiguous", off["u1"][0], "ambiguous")
    check("pool model: a shared pooled literal is private even with a unique value",
          sorted(n for n, (k, _w) in on.items() if k == "private"), ["c1", "c2", "str", "u1"])
    check("... a table and a non-string sdata object stay ambiguous", (on["tbl"][0], on["var"][0]),
          ("ambiguous", "ambiguous"))
    check("... one reader is no link", "u2" in on, False)
    wide = classify(plabels, prefs, porder, paddr, psize, 0x100, set(), pool=True)
    check("... the span guard still keeps a far-apart reader pair out", wide["c2"][0], "ambiguous")
    one = bytes.fromhex("3f800000")
    dol = FakeDol({0x8000: one, 0x8004: bytes.fromhex("40000000"), 0x8008: bytes.fromhex("40000000"),
                   0x800C: bytes.fromhex("40000000")})
    obs, rec = pool_dedupe_cuts(plabels, prefs, dol)
    check("dedupe: copies of 2.0f at 0x8004/0x8008/0x800C chain pairwise; u2/c1 referrers interleave -> overlap, "
          "c1/c2 do not -> a boundary",
          ([(o[0], o[1], o[3]) for o in obs], [r["status"] for r in rec]), ([(2, 3, "pooldup")], ["overlap", "pinned"]))
    obs2, rec2 = pool_dedupe_cuts(plabels, {"u2": [0], "c1": [1], "c2": [3], "u1": [2]}, dol)
    check("dedupe: disjoint ordered referrers pin the cut between them (last+1 .. first)",
          [(o[0], o[1], o[3]) for o in obs2], [(1, 1, "pooldup"), (2, 3, "pooldup")])
    check("dedupe: no DOL, no observation", pool_dedupe_cuts(plabels, prefs, None), ([], []))
    check("pooldup is strong only with --pool-model strong",
          ["pooldup" in strong_kinds("on", m) for m in POOL_MODELS], [False, False, True])
    fold = pool_fold_info(0, 1, 0, 1)
    check("pool_fold_info never raises on an unreadable registry", isinstance(fold, dict), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def cmd_prune(args):
    """Remove the split-tree duplicates the tool already refuses to parse. Idempotent.

    Safe by construction: only files this tool has proven stale are touched (a shadowed same-range
    copy, or a sub-range copy whose `.fn` names are all unknown to the symbol map), `build/` is
    gitignored build output, and dtk never rewrites them - deleting cannot change the DOL or the
    linked build. `--apply` is required; the default is a dry run.
    """
    fns, labels = load_map()
    graph = build_graph(fns, labels, force=args.force)
    asm = stale_files(graph)
    print("stale asm files    %d (%.1f MB) - copies `dedupe_ranges()` already drops"
          % (len(asm), sum(os.path.getsize(p) for p, _ in asm) / 1e6))
    for _p, rel in asm[:args.limit]:
        print("  %s" % rel)
    if len(asm) > args.limit:
        print("  ... and %d more" % (len(asm) - args.limit))
    npath = os.path.join(_g("ROOT"), "build.ninja")
    ninja = open(npath, encoding="utf-8", errors="replace").read() if os.path.exists(npath) else ""
    obj = []
    if args.include_obj:
        for _p, rel in asm:
            cand = os.path.join(os.path.dirname(_g("ASM_DIR")), "obj", rel[:-2] + ".o")
            # A path guard, not a basename guard: `obj/camellia.o` shares its basename with the
            # canonical `obj/Camellia/camellia.o`, and build.ninja only ever names the latter.
            if os.path.exists(cand) and os.path.relpath(cand, _g("ROOT")).replace(os.sep, "/") not in ninja:
                obj.append(cand)
        print("stale obj files    %d (the same units; none is referenced by build.ninja)" % len(obj))
        for c in obj[:args.limit]:
            print("  %s" % os.path.relpath(c, _g("ROOT")))
    if not args.apply:
        print("\ndry run: nothing deleted. Add --apply to remove them.")
        return 0
    removed = 0
    for path in [p for p, _ in asm] + obj:
        try:
            os.remove(path)
            removed += 1
        except OSError as exc:
            print("  could not remove %s: %s" % (path, exc), file=sys.stderr)
    print("\nremoved %d file(s). `stats` should now report none; hand off the block afterwards." % removed)
    return 0


def main():
    if "--selftest" in sys.argv[1:]:
        sys.exit(selftest())
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--selftest", action="store_true", help="fixture checks, no asm dump needed")
    sub = ap.add_subparsers(dest="cmd", required=True)

    def data_order_arg(sp):
        sp.add_argument("--data-order", choices=DATA_ORDER_MODES, default=DATA_ORDER_DEFAULT,
                        help="`.data` emission-order seams (V->S = a boundary in a gap, zigzag; soft): off / on (default) / "
                             "weak (adds V->D as a weak vote)")

    def pool_model_arg(sp):
        sp.add_argument("--pool-model", choices=POOL_MODELS, default=POOL_MODEL_DEFAULT,
                        help="literal pools as TU evidence (docs/pool-seams.md): on (default) = a shared pooled literal "
                             "is one TU + a value held at two pool addresses is a soft boundary; strong = that boundary "
                             "may move a cut; off = the older rule")

    a = sub.add_parser("at", help="propose the TU boundary around an address or symbol")
    a.add_argument("at")
    a.add_argument("--window", type=int, default=40, help="candidate cuts to search each side")
    a.add_argument("--top", type=int, default=3, help="candidates to list per side")
    a.add_argument("--span-max", type=int, default=0x4000,
                   help="referrer span (bytes) above which a pool label counts as cross-TU")
    a.add_argument("--source-span-max", type=int, default=0x8000,
                   help="referrer span (bytes) above which a `__FILE__` name stops being anchored")
    a.add_argument("--max-funcs", type=int, default=400, help="give up past this range size")
    a.add_argument("--unit", default=None, help="unit path for the printed splits block")
    a.add_argument("--json", action="store_true")
    a.add_argument("--splits", action="store_true", help="also print the splits.txt block")
    a.add_argument("--allow-stale", action="store_true",
                   help="print the splits block even when stale split-tree files remain (read-only)")
    data_order_arg(a)
    pool_model_arg(a)
    a.set_defaults(func=report)

    b = sub.add_parser("stats", help="cache, coverage and observation counts")
    b.add_argument("--span-max", type=int, default=0x4000)
    b.add_argument("--source-span-max", type=int, default=0x8000)
    b.add_argument("--force", action="store_true", help="rebuild the graph cache")
    data_order_arg(b)
    pool_model_arg(b)
    b.set_defaults(func=cmd_stats)

    c = sub.add_parser("cache", help="(re)build the graph cache only")
    c.add_argument("--force", action="store_true")
    c.add_argument("--span-max", type=int, default=0x4000)
    c.add_argument("--source-span-max", type=int, default=0x8000)
    data_order_arg(c)
    pool_model_arg(c)
    c.set_defaults(func=lambda a: cmd_stats(a) or 0)

    d = sub.add_parser("bench", help="scorecard used to iterate on this tool")
    d.add_argument("--seeds", type=int, default=400)
    d.add_argument("--seed", type=int, default=7)
    d.add_argument("--seeds-per-unit", type=int, default=SEED_CAP,
                   help="tier 1: functions per claimed unit allowed to seed a closure")
    d.add_argument("--max-funcs", type=int, default=400, help="give up past this range size")
    d.add_argument("--span-max", type=int, default=0x4000)
    d.add_argument("--source-span-max", type=int, default=0x8000)
    d.add_argument("--force", action="store_true", help="rebuild the graph cache")
    d.add_argument("--json", action="store_true")
    d.add_argument("--save", default=None, help="write the scorecard here (e.g. build/tmp/tudiscover/baseline.json)")
    d.add_argument("--compare", default=None, help="diff the sweep against a saved scorecard")
    data_order_arg(d)
    pool_model_arg(d)
    d.set_defaults(func=cmd_bench)

    g = sub.add_parser("dataorder", help="the `.data` emission-order seams as `.text` intervals")
    g.add_argument("--span-max", type=int, default=0x4000)
    g.add_argument("--source-span-max", type=int, default=0x8000)
    g.add_argument("--data-order", choices=DATA_ORDER_MODES, default="on")
    g.add_argument("--weak", action="store_true", help="include the weak V->D seams")
    g.add_argument("--all", action="store_true", help="list every seam, not just the notable ones")
    g.add_argument("--addr", type=lambda x: int(x, 16), default=None, help="one seam, by its .data address")
    g.add_argument("--json", action="store_true")
    g.set_defaults(func=cmd_dataorder)

    e = sub.add_parser("prune", help="remove the stale split-tree duplicates (dry run unless --apply)")
    e.add_argument("--apply", action="store_true", help="actually delete (default: dry run)")
    e.add_argument("--include-obj", action="store_true",
                   help="also remove the matching stale objects under build/<version>/obj")
    e.add_argument("--limit", type=int, default=20, help="paths to list")
    e.add_argument("--force", action="store_true", help="rebuild the graph cache")
    e.set_defaults(func=cmd_prune)

    args = ap.parse_args()
    global POOL_MODEL
    POOL_MODEL = getattr(args, "pool_model", POOL_MODEL_DEFAULT)
    sys.exit(args.func(args) or 0)


if __name__ == "__main__":
    main()
