#!/usr/bin/env python3
"""Classify retail `.data` symbols and list the TU seams MWCC's emission order implies (V->S, zigzag).
Spec: docs/tools/spec/dataorder.md. CLI: dataorder.py scan [--json] | at <address|symbol> [--window N] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import collections
import json
import os
import re

from tools.lib import repo as _repo
import tools.splits.seams.evidence as ev
# the reusable core lives in the seams evidence module; these names are this tool's public API
from tools.splits.seams.evidence import (VTABLE, STRING, DATA, PRINTABLE, PAD_MAX, STRONG_KINDS, TAIL_RUN_MAX,  # noqa: F401
                                         HEADER_NAME_RE, SOURCE_NAME_RE, Sym, is_header_name, is_source_name,
                                         inline_tail, text_range, data_symbols, classify, classify_all, seams,
                                         zigzag_pairs, fragments)

GAME = ev.GAME
_PATHS: dict = {}


def _paths() -> dict:
    """The tree's map, splits and retail DOL, resolved on first use (MAIN's DOL by path in a fresh worktree)."""
    if not _PATHS:
        root = _repo.repo_root()
        _PATHS.update(ROOT=root, SYMBOLS=os.path.join(root, "config", GAME, "symbols.txt"),
                      SPLITS=os.path.join(root, "config", GAME, "splits.txt"),
                      DOL=_repo.resolve_input(os.path.join("orig", GAME, "sys", "main.dol"), root, os.path.isfile,
                                              honour_env=True))
    return _PATHS


def __getattr__(name):
    if name in ("ROOT", "SYMBOLS", "SPLITS", "DOL"):
        return _paths()[name]
    raise AttributeError("module %r has no attribute %r" % (__name__, name))


def load_symbols(path: str | None = None) -> list[tuple[str, int, int | None, str]]:
    """`(section, address, size-or-None, name)` for every map row."""
    return ev.map_rows(path or _paths()["SYMBOLS"])


def narrow_gap() -> int:
    """The widest `V->S` gap (in symbols) whose boundary is cut rather than only reported (`evidence.NARROW`)."""
    return ev.NARROW


def unit_data_ranges(path: str | None = None) -> dict[int, tuple[str, int]]:
    """`start -> (unit, end)` for every registered unit `.data` range."""
    return ev.section_ranges(path or _paths()["SPLITS"], ".data")


unit_of = ev.range_of


def scan_dol() -> dict:
    """The whole-DOL numbers: classification counts, seams by kind, where they fall (registered / unclaimed)."""
    rows = load_symbols()
    syms = classify_all(rows, ev.Image(_paths()["DOL"]))
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
    rows = load_symbols()
    addr = resolve_address(args.target, rows)
    syms = classify_all(rows, ev.Image(_paths()["DOL"]))
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
    check("fragments() reuses the seams threshold (dataseams cuts with it too)", narrow_gap(), 8)

    # the real DOL, when the repo has it: the network_transport seams the discovery came from
    if os.path.exists(_paths()["DOL"]) and os.path.exists(_paths()["SYMBOLS"]):
        out = scan_dol()
        check("real DOL: vtables were found", out["kinds"].get(VTABLE, 0) > 150, True)
        # inside the unit while it was one TU, at a registered unit's start since it was split (Network/NetworkPeerMcs
        # and its neighbours, docs/network-transport-split.md): either way the scan must find all four
        inside = {s["addr"] for s in out["inside_registered_unit"] + out["at_registered_unit_start"]}
        found_nt = [a for a in (0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40) if a in inside]
        check("real DOL: the four network_transport V->S seams are inside a unit or at a unit start",
              found_nt, [0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40])
        # the seams stay in the DOL whoever owns the data: once the splits program claimed the unowned runs the
        # "unclaimed" bucket empties, so count every bucket
        check("real DOL: .data holds candidate seams",
              out["in_unclaimed_data"] + len(out["inside_registered_unit"]) + len(out["at_registered_unit_start"]) > 30, True)
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
