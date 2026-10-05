#!/usr/bin/env python3
"""Who calls this function / who reads this data: the whole-DOL reference index, keyed on addresses.
Spec: docs/tools/spec/callers.md. CLI: callers.py <address|name> [--json] [--code|--data] [--kind K]
[--pointers] [--limit N] [--rebuild] [--refresh | --no-refresh] | --range LO HI [--step N] [--each] | --stats | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os

from tools.lib import repo as _repo  # resolve_input: MAIN's build/ by path when the tree has none
from tools.lib import refs as _refs  # the one reference index: dump parser, object fallback, cache, the query
from tools.lib.report import rel_path
from tools.lib.repo import VERSION

ROOT = str(pathlib.Path(__file__).resolve().parents[2])
SCHEMA = _refs.SCHEMA
GAME = VERSION
DUMP_TOOL = "python tools/splits/dump_asm.py"
CODE_SECTIONS = _refs.CODE_SECTIONS
KINDS = _refs.KINDS
CODE_KINDS = _refs.CODE_KINDS                       # a control transfer: a caller
DATA_KINDS = _refs.DATA_KINDS                       # the address is used, not entered
ARG_WINDOW = _refs.ARG_WINDOW                       # instructions scanned back for the r3 argument

# --------------------------------------------------------------------------------------------------
# the dump, the object fallback, the index, the map and the query are `lib.refs` (one reference index);
# this tool keeps where the inputs live, the dump's verdict and how the answer is printed
# --------------------------------------------------------------------------------------------------
LOAD_MNEMONICS = _refs.LOAD_MNEMONICS
SCAN_RE = _refs.SCAN_RE
SYM_RE = _refs.SYM_RE
MOD_RE = _refs.MOD_RE
is_scaffolding = _refs.is_scaffolding
file_rank = _refs.file_rank
branch_target = _refs.dump_branch_target
symbol_operand = _refs.symbol_operand
mem_kind = _refs.mem_kind
arg_hint = _refs.arg_hint
parse_dump_file = _refs.parse_dump_file
key_of = _refs.key_of
attach_derived = _refs.attach_derived
object_signature = _refs.object_signature
coalesce = _refs.coalesce
Map = _refs.RefMap
plain_name = _refs.plain_name
find_target = _refs.find_target
fmt_addr = _refs.fmt_addr
query = _refs.query
caller_of = _refs.caller_of
norm_reader = _refs.norm_reader
reader_units = _refs.reader_units
readers_of = _refs.readers_of
range_report = _refs.range_report
all_asm_files = _refs.dump_files      # every `.s` of the dump (`.stamp.json` is not a unit), sorted
cache_of = _refs.dump_cache


def asm_dir_of(root=ROOT):
    """The dump to READ: the tree's own `build/<game>/asm`, else MAIN's by path when the tree has none (a fresh worktree)."""
    return _repo.resolve_input(os.path.join("build", GAME, "asm"), root, _refs.has_dump, honour_env=_repo.is_served(root))


def dump_signature(asm_dir, files, root=None):
    return _refs.dump_signature(asm_dir, files, root)


def build_index(asm_dir, files):
    """-> (index dict, stats dict) over every `.s` (`lib.refs.build_dump_index`)."""
    return _refs.build_dump_index(asm_dir, files, GAME)


def dump_state(asm_dir, files, root=ROOT):
    """-> (state, message, remedy): `missing` is never answered with a count of zero callers.

    The fresh/stale verdict is the dump's own stamp (`lib.refs.DumpStamp`, what `tudiscover` and `dump_asm.py` read).
    """
    if not files:
        return ("missing", "%s holds no `.s` file" % rel(asm_dir, root), DUMP_TOOL)
    stamp = _refs.DumpStamp.for_tree(root, _repo.is_served(root), GAME)
    if os.path.abspath(asm_dir) != os.path.abspath(stamp.asm_dir):
        return ("present", "%d file(s) (not the repository's dump: %s)"
                % (len(files), rel(asm_dir, root)), None)
    try:
        state, msg = stamp.status()
    except OSError as exc:
        # a fresh worktree has no `orig/**`: the dump is there, its stamp just cannot be checked
        return ("present", "%d file(s); the dump's stamp could not be checked (%s)"
                % (len(files), exc), None)
    remedy = DUMP_TOOL if state in ("stale", "missing", "truncated", "unstamped") else None
    return (state, msg, remedy)


#: The dump states that make its answers describe an older map.
STALE_DUMP = ("stale", "truncated", "unstamped")


def choose_dump(root=ROOT, policy=None, out=None, runner=None):
    """-> `(asm_dir, files, stale_note)`: which dump answers (`lib.artifacts`, default policy `warn`).

    A fresh dump answers. A stale one is rebuilt under `auto` (`--refresh`, `FRESH=auto`), refused under `refuse`,
    and otherwise left alone: `stale_note` then carries its stamp's message and the caller answers from the split
    objects - unless the tree has none, when the stale dump answers and says so (the old behaviour)."""
    from tools.lib import artifacts  # noqa: PLC0415
    out = out if out is not None else sys.stderr
    asm_dir = asm_dir_of(root)
    files = all_asm_files(asm_dir)
    if not files:
        return asm_dir, files, None
    state, msg, _remedy = dump_state(asm_dir, files, root)
    if state not in STALE_DUMP:
        return asm_dir, files, None
    chosen = artifacts.resolve_policy(policy, default="warn")
    if chosen == "refuse":
        raise artifacts.StaleArtifact(artifacts.Status("asm-dump", "stale", msg, DUMP_TOOL))
    if chosen == "auto":
        try:
            artifacts.ensure(["asm-dump"], artifacts.Context(root, runner=runner), "auto", out=out)
        except artifacts.RefreshFailed as exc:
            print("WARNING: %s" % exc.code, file=out)
        asm_dir = asm_dir_of(root)
        files = all_asm_files(asm_dir)
        state, msg, _remedy = dump_state(asm_dir, files, root)
        if state not in STALE_DUMP:
            return asm_dir, files, None
    if not all_object_files(root):
        return asm_dir, files, None
    return asm_dir, files, msg


def load_index(root=ROOT, rebuild=False, asm_dir=None, cache=None):
    """-> (index, info) - from the cache when it is the same dump, else built (`lib.refs.load_dump_index`)."""
    return _refs.load_dump_index(root, rebuild, asm_dir or asm_dir_of(root), cache or cache_of(root), game=GAME)


# No dump (it is written only on demand): the split objects under `build/<game>/obj` answer from their
# relocations instead - the same address-keyed graph, coarser kinds, `source: elf`.
def obj_dir_of(root=ROOT):
    """The split objects to READ: the tree's own `build/<game>/obj`, else MAIN's by path when the tree has none."""
    return _repo.resolve_input(os.path.join("build", GAME, "obj"), root, _refs._has_objects,
                               honour_env=_repo.is_served(root))


def all_object_files(root=ROOT):
    """Every split object under `build/<game>/obj` (the target pieces dtk wrote), sorted."""
    return _refs.object_files(obj_dir_of(root))


def build_elf_index(obj_dir, files, cmap):
    """The address-keyed graph from the split objects' relocations (`lib.refs.build_object_index`)."""
    return _refs.build_object_index(obj_dir, files, cmap.symbols, GAME)


def load_elf_index(root=ROOT, rebuild=False, cmap=None, cache=None):
    """`load_index`'s fallback: the same index shape, built from the split objects, cached the same way."""
    return _refs.load_object_index(root, rebuild, cmap, cache, game=GAME, obj_dir=obj_dir_of(root))


def load_map(root=ROOT):
    """The map, or an empty one when the tree has no `symbols.txt` (a scratch fixture)."""
    return _refs.RefMap.load(root)


def rel(path, root=ROOT):
    """`path` relative to `root` with forward slashes - a stable string for output and for JSON."""
    return rel_path(path, root)


def _owner_column(rep):
    """-> {site: owner} for the printed table (already resolved in the query, so no second index)."""
    return {r["site"]: (r["caller"] or {}).get("owner", "") for r in rep["references"]}


def print_report(rep, info=None, state=None, msg=None, remedy=None, root=ROOT, asm_dir=None):
    """The human answer. `sorted by site` throughout, one table per kind, the counts never hidden."""
    if rep.get("error"):
        print("== %s: no answer" % rep["query"])
        print()
        print("   %s" % rep["error"])
        if rep["candidates"]:
            print("   candidates:")
            for c in rep["candidates"][:12]:
                print("     %s" % c)
            print("   (%d shown; pass the exact name, or the address)" % min(12, len(rep["candidates"])))
        else:
            print("   try  python tools/symbols/symedit.py find <regex>   (the current map)")
        return 1
    hit = rep["resolved"]
    print("== %s  %s  %s%s  owner %s" % (
        hit["name"], hit["address"], hit["section"] or "?",
        (" size:0x%X" % hit["size"]) if hit["size"] else "", hit["owner"]))
    if hit["name_source"] == "dump":
        print("   (not in the current symbol map - this is the dump's own label)")
    if hit["asm_label"]:
        print("   the dump still prints it as %s (a stale label: the address is what was indexed)"
              % hit["asm_label"])
    if asm_dir is not None:
        print("   dump   %s - %s" % (rel(asm_dir, root), state))
        if msg:
            print("          %s" % msg)
        if remedy:
            print("          remedy: %s" % remedy)
    if info:
        print("   index  %s  %d reference(s) over %d target address(es), %d file(s) - %s" % (
            rel(info["cache"], root), info.get("stats", {}).get("refs", 0),
            info.get("stats", {}).get("targets", 0), info.get("files", 0),
            ("cached" if info.get("cached") else "built in %.1f s" % info["index"]["seconds"])))
    if not rep["references"]:
        print()
        if rep.get("all_kinds"):
            print("   no site matched the filter; this address has %s" % ", ".join(
                "%d %s reference(s)" % (n, k) for k, n in sorted(rep["all_kinds"].items())))
        else:
            print("   no reference to this address in the index.")
        if rep["pointer_hidden"]:
            print("   (%d pointer table entry(ies) - pass --pointers)" % rep["pointer_hidden"])
        for note in rep["notes"]:
            print("   note   %s" % note)
        return 0
    order = [k for k in KINDS if rep["counts"][k]]
    titles = {"call": "call site(s) (bl) - who calls this",
              "branch": "branch site(s) (b to a symbol) - a tail call, or a jump into this",
              "addr": "address-taken site(s) - the address is materialised and passed on",
              "read": "read(s) - the address is loaded from",
              "write": "write(s) - the address is stored to",
              "pointer": "pointer table(s) - a .4byte entry in a data object points here"}
    owners = _owner_column(rep)
    limit = rep.get("limit") or 0
    print()
    for kind in order:
        rows = [r for r in rep["references"] if r["kind"] == kind]
        print("   %d %s" % (len(rows), titles[kind]))
        hdr = "   #  %-10s %-18s %-22s %-28s %s" % ("site", "arg", "in function", "owner", "instruction")
        print(hdr)
        print("   " + "-" * (len(hdr) - 3))
        for i, r in enumerate(rows[:limit] if limit else rows, 1):
            caller = r["caller"] or {}
            print("   %-2d %-10s %-18s %-22s %-28s %s" % (
                i, r["site"], r["arg"] or "-", (caller.get("name") or "?")[:22],
                (owners.get(r["site"]) or (caller.get("name") and "not in the symbol map") or "")[:28],
                r["instruction"]))
        if limit and len(rows) > limit:
            print("   ... (%d more, raise --limit)" % (len(rows) - limit))
        print()
    print("   total  %d site(s) in %d function(s)/object(s)%s" % (
        rep["counts"]["sites"], rep["counts"]["functions"],
        ("  (+%d pointer table entry(ies), see --pointers)" % rep["pointer_hidden"])
        if rep["pointer_hidden"] else ""))
    for note in rep["notes"]:
        print("   note   %s" % note)
    return 0


def print_range(payload, root=ROOT, asm_dir=None, state=None, each=False):
    """The human answer: one row per run, its readers, and the seams between them."""
    rng = payload["range"]
    print("== per-address referrer runs  %s..%s  (step 0x%X, %d address(es), %d run(s))" % (
        fmt_addr(rng["lo"]), fmt_addr(rng["hi"]), rng["step"], rng["addresses"],
        len(payload["runs"])))
    if asm_dir is not None:
        print("   dump   %s - %s" % (rel(asm_dir, root), state))
    print()
    header = "   %-21s %9s  %s" % ("run", "addresses", "readers")
    print(header)
    print("   " + "-" * (len(header) - 3))
    for run in payload["runs"]:
        lo, hi = fmt_addr(run["start"]), fmt_addr(run["end"])
        span = lo if run["start"] == run["end"] else "%s-%s" % (lo, hi)
        readers = ", ".join(run["readers"]) or "(no reader in the index)"
        print("   %-21s %9d  %s" % (span, run["count"], readers))
    print()
    if payload["seams"]:
        print("   seams (%d): %s" % (len(payload["seams"]),
                                    ", ".join(fmt_addr(a) for a in payload["seams"])))
    else:
        print("   seams: none - the reader set is constant across the range")
    if each:
        print()
        print("   %-12s %s" % ("address", "readers"))
        for row in payload["addresses"]:
            print("   %-12s %s" % (fmt_addr(row["address"]),
                                    ", ".join(row["readers"]) or "-"))
    return 0


def _no_dump(asm_dir, root, as_json, query=None):
    """The missing-dump answer, printed by both the human and the `--json` path; returns exit 2."""
    rel_asm = rel(asm_dir, root)
    if as_json:
        print(json.dumps({"error": "no asm dump", "query": query or "",
                          "dump": {"state": "missing", "asm_dir": rel_asm,
                                   "message": "%s holds no `.s` file" % rel_asm,
                                   "remedy": DUMP_TOOL}}, indent=2))
        return 2
    print("== no asm dump")
    print("   %s holds no `.s` file, so there is no caller data to read." % rel_asm)
    print("   This is not '0 callers' - the dump is the input. Build it first:")
    print("     %s   (a full `dol split` with the asm, ~3.5 s; or pass --refresh)" % DUMP_TOOL)
    return 2


def stats_report(index, info, cmap, state, msg, remedy, asm_dir, root=ROOT, as_json=False):
    """`--stats`: what the index holds, how it was obtained, and what it left unresolved."""
    out = {"dump": {"state": state, "message": msg, "remedy": remedy, "files": info["files"],
                    "asm_dir": rel(asm_dir, root)},
           "index": {"path": rel(info["cache"], root), "cached": info["cached"],
                     "reason": info["reason"], "rebuilt": info["rebuilt"],
                     "built": index["built_utc"], "seconds": index["seconds"],
                     "bytes": info.get("cache_bytes"), "signature": info.get("signature"),
                     "stats": info.get("stats", {})},
           "map": {"available": cmap.available(), "symbols": len(cmap.symbols)}}
    if as_json:
        print(json.dumps(out, indent=2, default=str))
        return 0
    print("== the caller index")
    print("   dump   %s - %s" % (rel(asm_dir, root), state))
    if msg:
        print("          %s" % msg)
    if remedy:
        print("          remedy: %s" % remedy)
    print("   index  %s  %d reference(s) over %d target address(es), %d label(s)" % (
        rel(info["cache"], root), out["index"]["stats"].get("refs", 0),
        out["index"]["stats"].get("targets", 0), len(index["labels"])))
    print("          %d file(s), %.1f MB, built in %.1f s at %s - %s" % (
        index["files"], index["bytes"] / 1e6, index["seconds"], index["built_utc"],
        "from the cache" if info["cached"] else "just rebuilt (%s)" % info["reason"]))
    st = out["index"]["stats"]
    print("          %d duplicate site(s) dropped (a stale copy of a unit's asm), %d label "
          "conflict(s), %d printed label(s) disagreeing with the displacement" % (
              st.get("duplicates", 0), len(st.get("label_conflicts", [])), st.get("label_mismatch", 0)))
    if st.get("unresolved"):
        print("          %d name(s) are in neither the dump's label table nor the map (retried per "
              "query through the map)" % st["unresolved"])
    for n, a in st.get("biggest", []):
        print("          most-referenced: %s (%d reference(s))" % (a, n))
    print("   map    %d symbol(s) from %s (%s)" % (
        len(cmap.symbols), os.path.join("config", GAME, "symbols.txt"),
        "available" if cmap.available() else "MISSING - names come from the dump"))
    return 0


def main(argv=None, root=ROOT):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("query", nargs="?", help="an address (0x803AD47C) or a symbol name (quest_init)")
    ap.add_argument("--range", nargs=2, metavar=("LO", "HI"),
                    help="the per-address referrer runs across LO..HI (both inclusive), stepping "
                         "--step - the `.sdata2`/`.data` seam evidence, in one load")
    ap.add_argument("--step", type=lambda s: int(s, 16), default=4,
                    help="the address stride for --range (hex accepted; default 4)")
    ap.add_argument("--each", action="store_true",
                    help="with --range, list every address as well as the runs")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--code", action="store_true", help="only caller sites (bl / branch)")
    ap.add_argument("--data", action="store_true",
                    help="only address references (read/write/addr/pointer)")
    ap.add_argument("--kind", help="an explicit filter: %s" % ",".join(KINDS))
    ap.add_argument("--pointers", action="store_true", help="list .4byte pointer-table entries too")
    ap.add_argument("--limit", type=int, default=40, help="max sites listed per kind (0 = all)")
    ap.add_argument("--rebuild", action="store_true", help="rebuild the index even if the cache is valid")
    fresh = ap.add_mutually_exclusive_group()
    fresh.add_argument("--refresh", dest="fresh", action="store_const", const="auto",
                       help="rebuild a stale asm dump first (~3.5 s, lib.artifacts; FRESH=auto)")
    fresh.add_argument("--no-refresh", dest="fresh", action="store_const", const="warn",
                       help="never rebuild the dump: a stale one is answered from the split objects (the default)")
    ap.add_argument("--stats", action="store_true", help="report the index's state and exit")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.code and args.data:
        ap.error("--code and --data are exclusive")
    if args.range and (args.stats or args.query):
        ap.error("--range takes no query and no --stats (it is its own report)")
    if not args.query and not args.stats and not args.range:
        ap.error("an address or a symbol name is required (or --range, or --stats, or --selftest)")
    lo = hi = None
    if args.range:
        try:
            lo, hi = (int(args.range[0], 16), int(args.range[1], 16))
        except ValueError:
            ap.error("both --range edges must be hex addresses (0x80799F98)")
        if hi < lo:
            ap.error("--range HI is below LO (0x%X < 0x%X)" % (hi, lo))
        if args.step <= 0:
            ap.error("--step must be positive")
    kinds = None
    if args.code:
        kinds = list(CODE_KINDS)
    if args.data:
        kinds = list(DATA_KINDS)
    if args.kind:
        kinds = [k.strip() for k in args.kind.split(",") if k.strip()]
        bad = [k for k in kinds if k not in KINDS]
        if bad:
            ap.error("unknown kind(s): %s (known: %s)" % (", ".join(bad), ", ".join(KINDS)))
    asm_dir, files, stale_note = choose_dump(root, args.fresh)
    cmap = load_map(root)
    if files and stale_note is None:
        index, info = load_index(root=root, rebuild=args.rebuild, asm_dir=asm_dir)
        if index is None:
            return _no_dump(asm_dir, root, args.json, args.query)
        state, msg, remedy = dump_state(asm_dir, files, root)
    else:
        # No dump (it is written only on demand), or a stale one the caller did not want refreshed: answer from
        # the split objects' relocations instead of refusing. `--stats` and `--json` carry the same `info` shape,
        # so the reader can see which graph answered (`source: elf`).
        index, info = load_elf_index(root=root, rebuild=args.rebuild, cmap=cmap)
        if index is None:
            return _no_dump(asm_dir, root, args.json, args.query)
        obj_dir = obj_dir_of(root)
        if stale_note is None:
            state, msg = "elf fallback", (
                "no asm dump under %s - the graph is the %d split object(s)' relocations (coarser kinds, "
                "exact sites); run %s for the instruction-level dump" % (rel(asm_dir, root), info["files"],
                                                                        DUMP_TOOL))
        else:
            state, msg = "elf fallback", (
                "the asm dump under %s is stale (%s) - the graph is the %d split object(s)' relocations "
                "instead (coarser kinds, exact sites); --refresh (or FRESH=auto) rebuilds the dump first "
                "(~3.5 s)" % (rel(asm_dir, root), stale_note, info["files"]))
        remedy, asm_dir = None, obj_dir
    if args.stats:
        return stats_report(index, info, cmap, state, msg, remedy, asm_dir, root, args.json)
    if args.range:
        payload = range_report(index, cmap, lo, hi, args.step)
        if args.json:
            payload["dump"] = {"state": state, "message": msg, "remedy": remedy,
                               "asm_dir": rel(asm_dir, root), "files": info.get("files", len(files)),
                               "source": info.get("source", "asm")}
            payload["index"] = {"path": rel(info["cache"], root), "cached": info["cached"],
                                "reason": info["reason"], "built": index["built_utc"],
                                "seconds": index["seconds"],
                                "refs": info.get("stats", {}).get("refs", 0)}
            print(json.dumps(payload, indent=2))
            return 0
        return print_range(payload, root=root, asm_dir=asm_dir, state=state, each=args.each)
    rep = query(args.query, index, cmap, kinds=kinds, limit=args.limit, pointers=args.pointers)
    if args.json:
        if rep.get("error"):
            print(json.dumps({"error": rep["error"], "query": args.query,
                              "candidates": rep["candidates"],
                              "dump": {"state": state, "message": msg, "remedy": remedy}}, indent=2))
            return 1
        out = dict(rep)
        out["dump"] = {"state": state, "message": msg, "remedy": remedy,
                       "asm_dir": rel(asm_dir, root), "files": info.get("files", len(files)),
                       "source": info.get("source", "asm")}
        out["index"] = {"path": rel(info["cache"], root), "cached": info["cached"],
                        "reason": info["reason"], "built": index["built_utc"],
                        "seconds": index["seconds"], "refs": info.get("stats", {}).get("refs", 0),
                        "targets": info.get("stats", {}).get("targets", 0)}
        print(json.dumps(out, indent=2))
        return 0
    return print_report(rep, info=info, state=state, msg=msg, remedy=remedy, root=root,
                        asm_dir=asm_dir)


# --------------------------------------------------------------------------------------------------
# self-test: fixtures only (a fake asm dump + a fake map in a temp dir), so the run is identical in
# MAIN, in a fresh worktree, and with no dump on disk at all.
# --------------------------------------------------------------------------------------------------
def _fixture_dump():
    """A three-file fake dump: a unit file, a stale top-level copy, and a data file.

    The encodings are real, so `branch_target` decodes them: 0x80001008 + 0x278 == 0x80001280. The
    orientation is the real one - the unit file (`menu/multi_result.s`) prints the *current* label, the
    top-level `auto_*` scaffolding copy prints the stale `fn_<ADDR>` - so `file_rank` has a real job.
    """
    unit = "\n".join([
        '.include "macros.inc"',
        '.file "menu/multi_result.cpp"',
        '',
        '# 0x80001000..0x80001120 | size: 0x120',
        '.text',
        '.balign 4',
        '',
        '# .text:0x0 | 0x80001000 | size: 0x1C',
        '# caller(unsigned char)',
        '.fn caller, global',
        '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
        '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
        '/* 80001008 00000008  48 00 02 79 */\tbl quest_init__FUc',
        '/* 8000100C 0000000C  90 7F 00 DC */\tstw r3, 0xdc(r31)',
        '/* 80001010 00000010  38 60 00 01 */\tli r3, 0x1',
        '/* 80001014 00000014  48 00 02 6D */\tbl quest_init__FUc',
        '/* 80001018 00000018  4E 80 00 20 */\tblr',
        '.endfn caller',
        '',
        '# .text:0x60 | 0x80001060 | size: 0x18',
        '.fn with_data, global',
        '/* 80001060 00000060  3C 60 80 50 */\tlis r3, lbl_80500000@ha',
        '/* 80001064 00000064  38 63 00 00 */\taddi r3, r3, lbl_80500000@l',
        '/* 80001068 00000068  80 A0 00 00 */\tlwz r5, lbl_80500020@sda21(r0)',
        '/* 8000106C 0000006C  90 C0 00 00 */\tstw r6, lbl_80500020@sda21(r0)',
        '/* 80001070 00000070  4E 80 00 20 */\tblr',
        '.endfn with_data',
        '',
        '# .text:0xA0 | 0x800010A0 | size: 0x14',
        '.fn taker, global',
        '/* 800010A0 000000A0  3C 60 80 00 */\tlis r3, quest_init__FUc@ha',
        '/* 800010A4 000000A4  38 63 12 80 */\taddi r3, r3, quest_init__FUc@l',
        '/* 800010A8 000000A8  4B FF FF 59 */\tbl caller',
        '/* 800010AC 000000AC  48 00 00 0D */\tbl fn_800010B8',
        '/* 800010B0 000000B0  4E 80 00 20 */\tblr',
        '.endfn taker',
        '',
        '# .text:0xB8 | 0x800010B8 | size: 0x8',
        '.fn fn_800010B8, global',
        '/* 800010B8 000000B8  4E 80 00 20 */\tblr',
        '.endfn fn_800010B8',
        '',
    ]) + "\n"
    stale = "\n".join([
        '.include "macros.inc"',
        '.file "auto_fn_80001000_text"',
        '',
        '# 0x80001000..0x80001280 | size: 0x280',
        '.text',
        '.balign 4',
        '',
        '# .text:0x0 | 0x80001000 | size: 0x1C',
        '.fn fn_80001000, global',
        '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
        '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
        '/* 80001008 00000008  48 00 02 79 */\tbl fn_80001280',       # the same site, stale label
        '/* 8000100C 0000000C  90 7F 00 DC */\tstw r3, 0xdc(r31)',
        '/* 80001010 00000010  38 60 00 01 */\tli r3, 0x1',
        '/* 80001014 00000014  48 00 02 6D */\tbl fn_80001280',
        '/* 80001018 00000018  4E 80 00 20 */\tblr',
        '.endfn fn_80001000',
        '',
        '# .text:0x200 | 0x80001200 | size: 0x10',
        '.fn fn_80001200, global',                       # a site only the stale copy dumps
        '/* 80001200 00000200  38 60 00 00 */\tli r3, 0x0',
        '/* 80001204 00000204  48 00 00 7D */\tbl quest_init__FUc',
        '/* 80001208 00000208  4E 80 00 20 */\tblr',
        '.endfn fn_80001200',
        '',
        '# .text:0x280 | 0x80001280 | size: 0x40',                     # the stale label of the target
        '.fn fn_80001280, global',
        '/* 80001280 00000280  4E 80 00 20 */\tblr',
        '.endfn fn_80001280',
        '',
    ]) + "\n"
    data = "\n".join([
        '.include "macros.inc"',
        '.file "auto_07_80500000_data"',
        '',
        '# 0x80500000..0x80500020 | size: 0x20',
        '.data',
        '.balign 8',
        '',
        '# .data:0x0 | 0x80500000 | size: 0x10',
        '.obj lbl_80500000, global',
        '\t.4byte quest_init__FUc',
        '\t.4byte 0x00000000',
        '\t.4byte fn_80001000',
        '\t.4byte 0x00000000',
        '.endobj lbl_80500000',
        '',
        '# .data:0x10 | 0x80500010 | size: 0x4',
        '.obj lbl_80500010, global',
        '\t.4byte lbl_80500000',
        '.endobj lbl_80500010',
        '',
    ]) + "\n"
    return {"menu/multi_result.s": unit, "auto_fn_80001000_text.s": stale,
            "auto_07_80500000_data.s": data}


def _fixture_map():
    return "\n".join([
        "quest_init__FUc = .text:0x80001280; // type:function size:0x40 scope:global",
        "caller = .text:0x80001000; // type:function size:0x1C scope:global",
        "with_data = .text:0x80001060; // type:function size:0x18 scope:global",
        "taker = .text:0x800010A0; // type:function size:0x14 scope:global",
        "fn_800010B8 = .text:0x800010B8; // type:function size:0x8 scope:global",
        "lbl_80500000 = .data:0x80500000; // type:object size:0x10 scope:global",
        "lbl_80500020 = .data:0x80500020; // type:object size:0x4 scope:global",
        ""]) + "\n"


def _fixture_splits():
    return ("Sections:\n\t.text       type:code align:4\n\n"
            "menu/multi_result.cpp:\n\t.text       start:0x80001000 end:0x80001100\n")


def _fixture_root(tmp):
    """The fake tree: the dump under `build/<game>/asm`, the map under `config/<game>/`."""
    os.makedirs(os.path.join(tmp, "config", GAME), exist_ok=True)
    asm = asm_dir_of(tmp)
    with open(os.path.join(tmp, "config", GAME, "symbols.txt"), "w", encoding="utf-8",
              newline="") as fh:
        fh.write(_fixture_map())
    with open(os.path.join(tmp, "config", GAME, "splits.txt"), "w", encoding="utf-8", newline="") as fh:
        fh.write(_fixture_splits())
    for name, text in _fixture_dump().items():
        path = os.path.join(asm, *name.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="") as fh:
            fh.write(text)
    return asm


def selftest():
    """Fixture-driven checks for every reader, every query path and every exit; returns an exit code."""
    import contextlib
    import io
    import shutil
    import tempfile

    from tools.lib.binary.build import ElfBuilder

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s\n      got  %r\n      want %r" % (name, got, want))

    def check_in(name, needle, hay):
        nonlocal checks
        checks += 1
        if needle not in hay:
            fails.append("%s\n      %r is not in the output:\n%s" % (name, needle, hay[:3000]))

    def run(rep, info=None, root=None, asm=None, state="present", msg="fixture"):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = print_report(rep, info=info, state=state, msg=msg, remedy=None, root=root,
                                asm_dir=asm)
        return code, buf.getvalue()

    # --- the decoder the address graph rests on ---------------------------------------------------
    check("branch: a bl decodes its own displacement", branch_target(0x80001008, "48 00 02 79"),
          0x80001280)
    check("branch: a b decodes too", branch_target(0x800010AC, "48 00 00 0D"), 0x800010B8)
    check("branch: a negative displacement wraps", branch_target(0x800010A8, "4B FF FF 59"),
          0x80001000)
    check("branch: a non-branch encoding is refused", branch_target(0x80001000, "94 21 FF F0"), None)
    check("branch: an absolute branch is refused", branch_target(0x80001000, "48 00 00 02"), None)
    check("operand: a register is not a symbol", symbol_operand("r3, 0x0"), None)
    check("operand: a local label is not a symbol", symbol_operand(".L_800010A4"), None)
    check("operand: a name is", symbol_operand("quest_init__FUc"), "quest_init__FUc")
    check("operand: a quoted name is unwrapped", symbol_operand('"@eti_80038FAC"'), "@eti_80038FAC")
    check("mem: a load reads", mem_kind("lwz"), "read")
    check("mem: a store writes", mem_kind("stw"), "write")
    check("mem: li only materialises the address", mem_kind("li"), "addr")
    check("mem: lis only materialises the address", mem_kind("lis"), "addr")
    check("mem: addi only materialises the address", mem_kind("addi"), "addr")
    check("arg: li r3 before the call is the argument", arg_hint([("li", "r3, 0x1")], False), "0x1")
    check("arg: a lis/addi address is the argument",
          arg_hint([("lis", "r3, lbl_1@ha"), ("addi", "r3, r3, lbl_1@l")], False), "&lbl_1")
    check("arg: a preceding call leaves r3 live-in", arg_hint([("bl", "x")], False), "0 (r3 live-in)")
    check("arg: nothing writes r3", arg_hint([("li", "r4, 0x1")], False), "0")
    check("arg: a truncated window is not called zero", arg_hint([("li", "r4, 0x1")], True), "0?")
    check("arg: an unmodelled writer is named, not guessed",
          arg_hint([("lwz", "r3, 0x10(r31)")], False), "? (lwz)")
    check("rank: a top-level auto file is the scaffolding",
          is_scaffolding(os.path.join("x", "auto_fn_1_text.s"), "x"), True)
    check("rank: a unit directory file is not",
          is_scaffolding(os.path.join("x", "menu", "multi_result.s"), "x"), False)

    # --- the whole pipeline against the fixture tree ----------------------------------------------
    tmp = tempfile.mkdtemp(prefix="callers-fixture-")
    try:
        asm = _fixture_root(tmp)
        cache = cache_of(tmp)
        index, info = load_index(root=tmp, rebuild=True, asm_dir=asm, cache=cache)
        check("index: a build from scratch reports the rebuild", info["rebuilt"], True)
        check("index: every fixture file is read", index["files"], 3)
        check("index: the cache is written", os.path.exists(cache), True)
        check("index: a data label's address and size come from the dump's header",
              index["labels"]["lbl_80500000"], [0x80500000, 0x10, ".data"])
        check("index: a function's size comes from its .text block",
              index["labels"]["fn_80001280"][:2], [0x80001280, 0x40])
        check("index: the data sites and the pointer entry share one target key",
              sorted(row[1] for row in index["refs"]["0x80500000"]), ["addr", "addr", "pointer"])
        check("index: a duplicate site is dropped", index["stats"]["duplicates"], 2)
        check("index: a name only the map can resolve is kept by name",
              [row[1] for row in index["refs"]["quest_init__FUc"]], ["addr", "addr", "pointer"])
        cmap = load_map(tmp)
        check("map: the fixture map is read", len(cmap.symbols), 7)

        # a query for the *current* name: the dump prints the stale label at two of the four sites
        rep = query("quest_init", index, cmap)
        check("query: the plain name offers the mangled one",
              rep["candidates"], ["quest_init__FUc"])
        rep = query("quest_init__FUc", index, cmap)
        check("query: the target is the current map row", rep["resolved"]["address"], "0x80001280")
        check("query: the target's kind is code", rep["resolved"]["kind"], "code")
        check("query: every call site is found, from both copies of the dump",
              [r["site"] for r in rep["references"] if r["kind"] == "call"],
              ["0x80001008", "0x80001014", "0x80001204"])
        check("query: a caller the map does not know is named from the dump",
              [r["caller"]["name"] for r in rep["references"] if r["site"] == "0x80001204"],
              ["fn_80001200"])
        check("query: the address-taken site is found",
              [r["site"] for r in rep["references"] if r["kind"] == "addr"], ["0x800010A0"])
        check("query: a duplicated site is reported once",
              sum(1 for r in rep["references"] if r["site"] == "0x80001008"), 1)
        check("query: the canonical copy's text won",
              [r["instruction"] for r in rep["references"] if r["site"] == "0x80001008"],
              ["bl quest_init__FUc"])
        check("query: the caller is named from the map, not the dump's .fn",
              [r["caller"]["name"] for r in rep["references"] if r["site"] == "0x80001008"], ["caller"])
        check("query: the caller's owner comes from splits.txt",
              [r["caller"]["owner"] for r in rep["references"] if r["site"] == "0x80001008"],
              ["menu/multi_result.cpp (no source yet)"])
        check("query: the argument is inferred", rep["references"][0]["arg"], "0x0")
        check("query: the second call's argument follows the compiled code",
              [r["arg"] for r in rep["references"] if r["site"] == "0x80001014"], ["0x1"])
        check("query: a caller's argument can be an address",
              [r["arg"] for r in query("caller", index, cmap)["references"]], ["&quest_init__FUc"])
        rep_ptr = query("quest_init__FUc", index, cmap, pointers=True)
        check("query: a pointer entry only the map can name is found per query",
              [r["site"] for r in rep_ptr["references"] if r["kind"] == "pointer"], ["0x80500000"])
        check("query: and the retry says so",
              any("resolved through the current map" in n for n in rep_ptr["notes"]), True)
        check("query: the dump's stale label for the target is reported",
              rep["resolved"]["asm_label"], "fn_80001280")
        check("text: a stale dump label is respelled by the current map at its address (the call, @ha/@l)",
              [_refs.current_text(t, index, cmap) for t in ("bl fn_80001280", "lis r3, fn_80001280@ha",
                                                           "bl fn_80001200", "li r3, 0x0")],
              ["bl quest_init__FUc", "lis r3, quest_init__FUc@ha", "bl fn_80001200", "li r3, 0x0"])
        code, out = run(rep, info=info, root=tmp, asm=asm)
        check("report: it exits 0", code, 0)
        check_in("report: the stale label is called out", "stale label", out)
        check_in("report: the caller's owner is listed", "menu/multi_result.cpp", out)
        check_in("report: the counts are the header of each table", "3 call site(s) (bl)", out)

        # a query by address: the current name, never the dump's stale label
        rep = query("0x80001280", index, cmap)
        check("query by address: the name is the map's", rep["resolved"]["name"], "quest_init__FUc")
        check("query by address: the size is the dump's", rep["resolved"]["size"], 0x40)
        check("query by address: how=address", rep["how"], "address")

        # a query by the dump's own stale name: answered through the dump's label table, with a note
        rep = query("fn_80001280", index, cmap)
        check("query by a stale name: it resolves through the dump",
              rep["resolved"]["address"], "0x80001280")
        check("query by a stale name: how=dump", rep["how"], "dump")
        check("query by a stale name: the note names the current symbol",
              any("quest_init__FUc" in n for n in rep["notes"]), True)
        code, out = run(rep, root=tmp, asm=asm)
        check_in("query by a stale name: the answer is the same three calls", "3 call site(s)", out)
        # data: a read, a write and an address-taken pair (coalesced into one site)
        rep = query("lbl_80500000", index, cmap)
        check("data: the kind is data", rep["resolved"]["kind"], "data")
        check("data: the size is the object's", rep["resolved"]["size"], 0x10)
        check("data: the lis/addi pair is one site", rep["counts"]["addr"], 1)
        check("data: the coalesced site keeps both instructions",
              "lis r3, lbl_80500000@ha" in rep["references"][0]["instruction"], True)
        check("data: the pointer entry is hidden by default", rep["counts"]["pointer"], 0)
        check("data: the hidden pointer count is reported", rep["pointer_hidden"], 1)
        rep = query("lbl_80500000", index, cmap, pointers=True)
        check("data: --pointers lists the table", rep["counts"]["pointer"], 1)
        check("data: the pointer site is the containing object",
              [r["site"] for r in rep["references"] if r["kind"] == "pointer"], ["0x80500010"])
        check("data: the pointer's referrer is the object that holds it",
              [r["caller"]["name"] for r in rep["references"] if r["kind"] == "pointer"],
              ["lbl_80500010"])
        rep = query("lbl_80500020", index, cmap)
        check("data: a load is a read", rep["counts"]["read"], 1)
        check("data: a store is a write", rep["counts"]["write"], 1)
        check("data: the two sda21 accesses are two sites", rep["counts"]["sites"], 2)
        code, out = run(rep, root=tmp, asm=asm)
        check_in("data: the report names the reads", "read(s)", out)

        # --- the census and `--range`: the per-address referrer runs (the .sdata2/.data seam) --------
        check("reader: a source label is normalised to its unit", norm_reader("Pl/x.cpp"), "Pl/x")
        check("reader: an unsplit address is not a unit", norm_reader("unsplit address"), None)
        check("reader: a no-source label keeps the unit", norm_reader("Pl/x.cpp (no source yet)"),
              "Pl/x")
        census = readers_of(index, cmap)
        check("census: a data word's readers are its referencing units", census(0x80500000),
              {"menu/multi_result": 1})
        check("census: both sda21 accesses count", census(0x80500020), {"menu/multi_result": 2})
        check("census: an address nobody references has no reader", census(0x80500004), {})
        check("census: the same address is answered from the cache",
              census(0x80500000) is census(0x80500000), True)
        rng = range_report(index, cmap, 0x80500000, 0x8050000C, 4)
        check("range: four addresses are sampled", rng["range"]["addresses"], 4)
        check("range: the run boundaries", [(r["start"], r["end"], r["readers"])
                                             for r in rng["runs"]],
              [(0x80500000, 0x80500000, ["menu/multi_result"]),
               (0x80500004, 0x8050000C, [])])
        check("range: the reader-set change is the seam", rng["seams"], [0x80500004])
        check("range: a single-run range has no seam",
              range_report(index, cmap, 0x80500004, 0x8050000C, 4)["seams"], [])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = print_range(rng, root=tmp, asm_dir=asm, state="present")
        out = buf.getvalue()
        check("range: print_range exits 0", rc, 0)
        check_in("range: the runs are headed", "per-address referrer runs", out)
        check_in("range: the reader is listed", "menu/multi_result", out)
        check_in("range: the seam is named", "seams (1): 0x80500004", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--range", "0x80500000", "0x8050000C"], root=tmp)
        out = buf.getvalue()
        check("range: the CLI answers", rc, 0)
        check_in("range: the CLI prints the run span", "0x80500004-0x8050000C", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--range", "0x80500000", "0x8050000C", "--json", "--each"], root=tmp)
        payload = json.loads(buf.getvalue())
        check("range: --json parses", rc, 0)
        check("range: --json carries the runs", len(payload["runs"]), 2)
        check("range: --json carries every address", len(payload["addresses"]), 4)

        # filters
        rep = query("quest_init__FUc", index, cmap, kinds=list(CODE_KINDS))
        check("filter: --code keeps every caller", rep["counts"]["call"], 3)
        check("filter: --code drops the address-taken site", rep["counts"]["addr"], 0)
        rep = query("quest_init__FUc", index, cmap, kinds=list(DATA_KINDS))
        check("filter: --data drops the callers", rep["counts"]["call"], 0)
        check("filter: --data keeps the address-taken site", rep["counts"]["addr"], 1)
        check("find: an address query is recognised", find_target("0x80001280", index, cmap)[2],
              "address")
        check("find: a bare 8-digit address is recognised", find_target("80001280", index, cmap)[0],
              0x80001280)
        check("find: a substring query is offered as candidates",
              find_target("quest_init", index, cmap)[2], "plain name")
        check("find: a missing name has no candidate",
              find_target("no_such_symbol_anywhere", index, cmap)[1], [])
        rep = query("no_such_symbol_anywhere", index, cmap)
        check("query: a missing name is an error with no references",
              (bool(rep["error"]), rep["references"]), (True, []))
        code, out = run(rep, root=tmp, asm=asm)
        check("query: a missing name exits 1", code, 1)
        check_in("query: a missing name points at symedit", "symedit.py find", out)

        # the cache: the same dump -> no rebuild; a symbols.txt edit -> still no rebuild (address-keyed);
        # a dump edit -> rebuild
        _i2, info2 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a second load is a hit", info2["cached"], True)
        with open(os.path.join(tmp, "config", GAME, "symbols.txt"), "a", encoding="utf-8") as fh:
            fh.write("renamed_later = .text:0x80001280; // type:function size:0x40 scope:global\n")
        index3, info3 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a symbols.txt edit does not rebuild the graph", info3["cached"], True)
        check("cache: the address answers to the map's current name",
              query("quest_init__FUc", index3, load_map(tmp), kinds=list(CODE_KINDS))["counts"]["call"],
              3)
        with open(os.path.join(asm, "menu", "multi_result.s"), "a", encoding="utf-8") as fh:
            fh.write("\n")
        _i4, info4 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a dump edit rebuilds", (info4["cached"], info4["rebuilt"]), (False, True))
        check("cache: the rebuild says why", info4["reason"], "the dump's content changed since the index was built")
        index5, info5 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: the rebuild is cached again", info5["cached"], True)
        check("cache: a forced rebuild ignores the cache",
              load_index(root=tmp, asm_dir=asm, cache=cache, rebuild=True)[1]["rebuilt"], True)
        check("cache: a fresh build reproduces the same targets",
              sorted(index5["refs"]) == sorted(index["refs"]), True)
        check("cache: a fresh build reproduces the stats",
              (info5["stats"]["refs"], info5["stats"]["duplicates"]),
              (index["stats"]["refs"], index["stats"]["duplicates"]))
        # a cache that cannot be trusted is rebuilt, never used: a half-written file, or another schema
        with open(cache, "w", encoding="utf-8") as fh:
            fh.write('{"schema": 1, "signature": ')
        _i, info_bad = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a truncated file is rebuilt", (info_bad["cached"], info_bad["rebuilt"]),
              (False, True))
        check("cache: and the reason names it", info_bad["reason"].startswith("unreadable cache"), True)
        with open(cache, "w", encoding="utf-8") as fh:
            json.dump({"schema": SCHEMA - 1, "signature": info_bad["signature"], "files": 3}, fh)
        _i, info_schema = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: another schema is rebuilt", info_schema["cached"], False)
        check("cache: and the reason names the schema",
              info_schema["reason"].startswith("cache schema"), True)
        with open(cache, "w", encoding="utf-8") as fh:
            json.dump({"schema": SCHEMA, "signature": "not-this-dump", "files": 3}, fh)
        _i, info_sig = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: another dump's signature is rebuilt",
              (info_sig["cached"], info_sig["reason"]),
              (False, "the dump's content changed since the index was built"))

        # a missing dump is never answered with a count of zero callers
        empty = os.path.join(tmp, "empty-tree")
        os.makedirs(os.path.join(empty, "build", GAME, "asm"), exist_ok=True)
        state_e, msg_e, remedy_e = dump_state(asm_dir_of(empty), [], empty)
        check("missing dump: the state says so", state_e, "missing")
        check("missing dump: the remedy is dump_asm.py", DUMP_TOOL, remedy_e)
        check("missing dump: the message names no count", "0 caller" in msg_e, False)
        _ie, info_e = load_index(root=empty, cache=os.path.join(empty, "no.json"))
        check("missing dump: load_index returns nothing", _ie, None)
        check("missing dump: the reason is 'no dump'", info_e["reason"], "no dump")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init"], root=empty)
        out = buf.getvalue()
        check("missing dump: the exit is 2", rc, 2)
        check_in("missing dump: the remedy is printed", DUMP_TOOL, out)
        check_in("missing dump: it refuses to say 0 callers", "not '0 callers'", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init", "--json"], root=empty)
        check("missing dump: --json carries the error", '"error": "no asm dump"' in buf.getvalue(),
              True)

        # the CLI end to end on the fixture tree (as a human runs it)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init__FUc", "--code", "--limit", "2"], root=tmp)
        out = buf.getvalue()
        check("cli: --code --limit exits 0", rc, 0)
        check_in("cli: the header names the target", "== quest_init__FUc  0x80001280", out)
        check_in("cli: the limit is stated", "1 more, raise --limit", out)
        check_in("cli: the dump's state is printed", "present", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init__FUc", "--data", "--json"], root=tmp)
        payload = json.loads(buf.getvalue())
        check("cli: --json parses", rc, 0)
        check("cli: --json reports the dump's state", payload["dump"]["state"], "present")
        check("cli: --json reports the index as cached",
              payload["index"]["cached"], True)
        check("cli: --json keeps the address-taken site",
              [r["site"] for r in payload["references"]], ["0x800010A0"])
        check("cli: --json names the caller", payload["references"][0]["caller"]["name"], "taker")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--stats"], root=tmp)
        out = buf.getvalue()
        check("cli: --stats exits 0", rc, 0)
        check_in("cli: --stats counts the targets", "target address(es)", out)
        check_in("cli: --stats names the index", "graph.json", out)

        # --- the no-dump fallback: the same answer from the split objects' relocations ----------------
        # `build/<game>/asm` is written only on demand, so a built-but-not-dumped tree has no dump and
        # `callers.py` used to exit 2. The fallback builds the graph from `build/<game>/obj/**/*.o`'s
        # relocations (`dossier.parse_elf`), which is what the question actually needs: a `bl` is a
        # relocation to the callee. This tree is set up with objects and NO asm dir.
        elf_tree = os.path.join(tmp, "elf-tree")
        os.makedirs(os.path.join(elf_tree, "config", GAME), exist_ok=True)
        os.makedirs(os.path.join(elf_tree, "build", GAME, "obj", "probe"), exist_ok=True)
        with open(os.path.join(elf_tree, "config", GAME, "symbols.txt"), "w", encoding="utf-8",
                  newline="") as fh:
            fh.write("caller = .text:0x80001000; // type:function size:0x20 scope:global\n"
                     "callee = .text:0x80002000; // type:function size:0x10 scope:global\n"
                     "gData = .data:0x80500000; // type:object size:0x10 scope:global\n")
        with open(os.path.join(elf_tree, "config", GAME, "splits.txt"), "w", encoding="utf-8",
                  newline="") as fh:
            fh.write("probe/unit.c:\n\t.text       start:0x80001000 end:0x80001020\n")
        elf = (ElfBuilder().section(".text", b"\x48\x00\x00\x01" * 8)
               .symbol("caller", ".text", 0, 0x20, type="func").symbol("callee").symbol("gData", type="func")
               .reloc(".text", 0x08, "callee", 10).reloc(".text", 0x0C, "gData", 6).build())
        with open(os.path.join(elf_tree, "build", GAME, "obj", "probe", "unit.o"), "wb") as fh:
            fh.write(elf)
        elf_cmap = load_map(elf_tree)
        elf_index, elf_info = load_elf_index(root=elf_tree, rebuild=True, cmap=elf_cmap,
                                             cache=os.path.join(elf_tree, "cache.json"))
        check("elf: no dump but objects builds a graph", elf_info["source"], "elf")
        check("elf: the call relocation lands on the callee's address",
              any(r[1] == "call" for r in elf_index["refs"].get("0x80002000", [])), True)
        check("elf: the call site is the anchored absolute address",
              [r[0] for r in elf_index["refs"].get("0x80002000", [])], [0x80001008])
        check("elf: the data relocation is an address reference",
              [r[1] for r in elf_index["refs"].get("0x80500000", [])], ["addr"])
        elf_rep = query("callee", elf_index, elf_cmap)
        check("elf: query resolves the callee through the map",
              elf_rep["resolved"]["address"], "0x80002000")
        check("elf: query reports the call", elf_rep["counts"]["call"], 1)
        check("elf: the caller is named from its own anchor",
              [r["caller"]["name"] for r in elf_rep["references"]], ["caller"])
        check("elf: the reference text names the target (no dump instruction text)",
              [r["instruction"] for r in elf_rep["references"]], ["bl callee"])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["callee", "--code"], root=elf_tree)
        out = buf.getvalue()
        check("elf: the CLI answers without a dump", rc, 0)
        check_in("elf: the CLI says which graph answered", "elf fallback", out)
        check_in("elf: the call is listed", "bl callee", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--stats"], root=elf_tree)
        check("elf: --stats exits 0", rc, 0)
        check_in("elf: --stats names the objects", "elf-graph.json", buf.getvalue())
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(main())
