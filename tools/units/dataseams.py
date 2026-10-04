#!/usr/bin/env python3
"""The .data emission-order seams (dataorder) as cut points, warnings and order-only notes for the claim tools.
Spec: docs/tools/spec/dataseams.md. CLI: dataseams.py [START END] | --selftest."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import sys

from tools.lib import repo as _repo
from tools.lib.binary.elf import Elf as LibElf
import tools.splits.seams.evidence as ev

STRONG = ev.STRONG_KINDS
#: A V->S gap with at most this many symbols is cut (at the end of its tail); a wider one only warns.
NARROW = ev.NARROW
_CACHE: dict = {}


def _paths() -> dict:
    """The tree's map, splits and retail DOL (the DOL MAIN's by path in a fresh worktree), resolved on first use."""
    if "paths" not in _CACHE:
        root = _repo.repo_root()
        _CACHE["paths"] = {"root": root, "symbols": os.path.join(root, "config", ev.GAME, "symbols.txt"),
                           "splits": os.path.join(root, "config", ev.GAME, "splits.txt"),
                           "dol": _repo.resolve_input(os.path.join("orig", ev.GAME, "sys", "main.dol"), root,
                                                      os.path.isfile, honour_env=True)}
    return _CACHE["paths"]


def load_strong() -> list[dict]:
    """Every strong `.data` seam of the retail DOL as `{addr, kind, before, after}` (a `V->S` row also carries
    `latest`, `width`, `tail` and `cut` - the first symbol after the tail strings), cached.

    Returns `[]` (never raises) when the DOL or the map is not readable: a guard must not break a tool that
    could otherwise run; `load_error()` says why the list is empty.
    """
    if "seams" in _CACHE:
        return _CACHE["seams"]
    try:
        paths = _paths()
        found = ev.strong_seams(ev.retail_symbols(paths["symbols"], paths["dol"]))
        _CACHE["error"] = None
    except Exception as exc:  # noqa: BLE001 - see the docstring
        found = []
        _CACHE["error"] = "%s: %s" % (type(exc).__name__, exc)
    _CACHE["seams"] = found
    return found


def load_error() -> str | None:
    load_strong()
    return _CACHE.get("error")


def seams_in(seams: list[dict], start: int, end: int) -> list[dict]:
    """The seams strictly inside `(start, end)`: a seam AT `start` is the run's own TU boundary, not a span."""
    return [s for s in seams if start < s["addr"] < end]


def is_gap(seam: dict) -> bool:
    """A `V->S` row that carries its gap `[addr, latest)` (rows from `dataorder.seams`)."""
    return seam.get("kind") == "V->S" and seam.get("latest") is not None


def cut_point(seam: dict) -> int | None:
    """Where a run is cut for this seam, or None when the position is too uncertain to cut.

    A `zigzag` (and a legacy row with no gap) cuts at `addr`; a narrow `V->S` gap cuts at `cut`, the first symbol
    after its inline-tail strings; a wide gap has no cut point - it only warns.
    """
    if not is_gap(seam):
        return seam["addr"]
    if seam.get("width", 0) <= NARROW:
        return seam.get("cut", seam["addr"])
    return None


def cut_ranges(start: int, end: int, seams: list[dict]) -> list[tuple[int, int]]:
    """`[start, end)` cut at every cuttable seam inside it: one `(start, end)` per probable TU."""
    cuts = {cut_point(s) for s in seams_in(seams, start, end)} - {None}
    points = [start] + sorted(c for c in cuts if start < c < end) + [end]
    return [(points[i], points[i + 1]) for i in range(len(points) - 1)]


def cut_addresses(seams: list[dict] | None) -> set[int]:
    """The addresses `dataqueue` cuts a run at: every seam's `cut_point`, wide gaps excluded."""
    return {c for c in (cut_point(s) for s in (seams or ())) if c is not None}


def where(seam: dict) -> str:
    """`at 0x..` for a seam with a known position, `a boundary in [a, b)` for a gap."""
    if is_gap(seam):
        return "a boundary in [0x%08X, 0x%08X)" % (seam["addr"], seam["latest"])
    return "at 0x%08X" % seam["addr"]


def describe(seams: list[dict]) -> str:
    """`0x805F9570 (V->S), 0x805F9610 (zigzag)` - what a warning names; a gap adds its `[addr, latest)`."""
    return ", ".join("0x%08X (%s)" % (s["addr"], s["kind"] + (", boundary in [0x%08X, 0x%08X)" % (s["addr"], s["latest"])
                                                              if is_gap(s) else ""))
                     for s in sorted(seams, key=lambda s: s["addr"]))


def warning(start: int, end: int, seams: list[dict]) -> str | None:
    """The one-line warning for a range that contains seams, with the suggested cut ranges; None when clean.

    Each seam is at least one more TU (a gap holds one boundary somewhere in it), so the count is a lower bound.
    Only narrow gaps and zigzags give cut ranges; a wide gap says where a boundary lies and nothing more.
    """
    inside = seams_in(seams, start, end)
    if not inside:
        return None
    cuts = cut_ranges(start, end, inside)
    if len(cuts) > 1:
        cut_text = "suggested cuts: " + ", ".join("0x%08X-0x%08X" % r for r in cuts)
    else:
        cut_text = "no cut suggested: the boundary position is uncertain (wide gap)"
    wide = [s for s in inside if cut_point(s) is None]
    if wide and len(cuts) > 1:
        cut_text += "; a boundary also lies in " + ", ".join(
            "[0x%08X, 0x%08X)" % (s["addr"], s["latest"]) for s in wide)
    return ("spans at least %d TUs, not one: .data emission-order seams %s - it can never match as one unit; %s"
            % (len(inside) + 1, describe(inside), cut_text))


def section_chunks(path: str, section: str) -> list[bytes] | None:
    """The bytes of each symbol defined in `section` of one ELF object, in section-offset order.

    Zero-size symbols are skipped and two symbols at one offset count once (the larger wins). `None` when the
    object or the section is missing or unreadable.
    """
    try:
        elf = LibElf.read(path)
        named = [s for s in elf.sections if s.name == section]
        if not named or elf.section(".symtab") is None:
            return None
        # a repeated section name (dtk can write two `.data` pieces): the bytes of the last, the symbols of
        # the first - exactly what the pyelftools reader this replaced returned (its name map keeps the last)
        sec = named[-1]
        data = b"\0" * sec.size if sec.is_nobits else sec.raw
        by_off: dict[int, int] = {}
        for sym in elf.symbols:
            if sym.shndx == named[0].index and sym.size:
                by_off[sym.value] = max(by_off.get(sym.value, 0), sym.size)
    except Exception:  # noqa: BLE001 - an unreadable object is "no evidence", never a crash
        return None
    return [data[off:off + size] for off, size in sorted(by_off.items())]


def order_only(ours: list[bytes] | None, target: list[bytes] | None) -> bool:
    """True when the two symbol lists hold the same bytes - the same multiset of symbols - in a different sequence."""
    if not ours or not target or len(ours) != len(target):
        return False
    return sorted(ours) == sorted(target) and ours != target


def unit_data_range(unit: str, section: str = ".data") -> tuple[int, int] | None:
    """The `splits.txt` `(start, end)` of `section` for a unit named with or without its extension."""
    if section != ".data":
        return None
    try:
        for start, (name, end) in ev.section_ranges(_paths()["splits"], ".data").items():
            if os.path.splitext(name)[0] == os.path.splitext(unit)[0]:
                return start, end
    except Exception:  # noqa: BLE001
        return None
    return None


def order_only_message(section: str, seams: list[dict]) -> str:
    """The diagnosis line: same symbols, different order, the unit spans several TUs, and where the seams are."""
    if not seams:
        return ("%s: order-only: the same symbols in a different sequence, but no strong .data emission-order "
                "seam is known inside the unit's range - one TU may still lay them out differently" % section)
    return ("%s: order-only: the unit spans several TUs; seams: %s - the target's symbols are the same, "
            "in the order of separate translation units (one TU emits globals, strings, then vtables in "
            "reverse class order), so split the unit at the seams" % (
                section, ", ".join(where(s) for s in sorted(seams, key=lambda s: s["addr"]))))


def multi_tu_message(section: str, start: int, end: int, seams: list[dict]) -> str:
    """The weaker diagnosis: the section differs and the unit's target range spans several TUs."""
    return ("%s: the target range 0x%08X-0x%08X spans at least %d TUs (emission-order seams: %s) - a unit is one "
            "TU, so this section cannot match until the unit is split at the seams"
            % (section, start, end, len(seams) + 1,
               ", ".join(where(s) for s in sorted(seams, key=lambda s: s["addr"]))))


def seam_note(unit: str, ours_path: str, target_path: str, section: str = ".data",
              seams: list[dict] | None = None, rng: tuple[int, int] | None = None) -> str | None:
    """What to say about a unit's differing `.data`, or None when the seams have nothing to add.

    `order-only` when the two objects hold the same symbols in a different sequence, else the multi-TU line -
    both only when the unit's target range actually contains a strong seam. A unit whose range holds no seam,
    or whose objects agree symbol for symbol, gets nothing: this is never a reason to refuse anything.
    """
    rng = rng or unit_data_range(unit, section)
    if rng is None:
        return None
    inside = seams_in(load_strong() if seams is None else seams, *rng)
    if not inside:
        return None
    ours, target = section_chunks(ours_path, section), section_chunks(target_path, section)
    if ours is not None and ours == target:
        return None
    if order_only(ours, target):
        return order_only_message(section, inside)
    return multi_tu_message(section, rng[0], rng[1], inside)


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    sm = [{"addr": 0x1100, "kind": "V->S"}, {"addr": 0x1200, "kind": "zigzag"}]
    check("a seam at the start is not inside", seams_in(sm, 0x1100, 0x1300), [sm[1]])
    check("a seam at the end is not inside", seams_in(sm, 0x1000, 0x1200), [sm[0]])
    check("cut ranges tile the run", cut_ranges(0x1000, 0x1300, sm),
          [(0x1000, 0x1100), (0x1100, 0x1200), (0x1200, 0x1300)])
    check("a clean range is not cut", cut_ranges(0x1300, 0x1400, sm), [(0x1300, 0x1400)])
    check("a clean range has no warning", warning(0x1300, 0x1400, sm), None)
    w = warning(0x1000, 0x1300, sm) or ""
    check("the warning names the seams and the TU lower bound",
          ("0x00001100 (V->S)" in w, "0x00001200 (zigzag)" in w, "spans at least 3 TUs" in w), (True, True, True))
    check("the warning suggests the cuts", "0x00001000-0x00001100" in w and "0x00001200-0x00001300" in w, True)
    check("order_only: the same symbols reordered", order_only([b"a", b"bb", b"c"], [b"bb", b"a", b"c"]), True)
    check("order_only: identical sequences are not", order_only([b"a", b"bb"], [b"a", b"bb"]), False)
    check("order_only: different bytes are not", order_only([b"a", b"bb"], [b"a", b"bc"]), False)
    check("order_only: a different symbol count is not", order_only([b"a"], [b"a", b"b"]), False)
    check("order_only: no evidence is not", (order_only(None, [b"a"]), order_only([], [])), (False, False))
    check("the order-only line names the seams",
          "seams: at 0x00001100, at 0x00001200" in order_only_message(".data", sm), True)
    check("... and says so when no seam is known", "no strong .data emission-order seam" in order_only_message(".data", []),
          True)
    check("section_chunks tolerates a missing file", section_chunks("does/not/exist.o", ".data"), None)
    check("a range with no seam gets no note",
          seam_note("u", "no.o", "no2.o", seams=sm, rng=(0x1300, 0x1400)), None)
    multi = seam_note("u", "no.o", "no2.o", seams=sm, rng=(0x1000, 0x1300)) or ""
    check("unreadable objects still name the multi-TU range",
          ("spans at least 3 TUs" in multi, "at 0x00001100, at 0x00001200" in multi), (True, True))
    # gaps: a narrow V->S gap cuts at the end of its inline tail, a wide one only warns, V->tail is never given here
    narrow = {"addr": 0x1100, "kind": "V->S", "latest": 0x1180, "width": 3, "tail": 1, "cut": 0x1120}
    wide = {"addr": 0x1200, "kind": "V->S", "latest": 0x1800, "width": 40, "tail": 0, "cut": 0x1200}
    zz = {"addr": 0x1900, "kind": "zigzag"}
    gaps = [narrow, wide, zz]
    check("cut_point: narrow gap = end of tail, wide gap = none, zigzag = its address",
          [cut_point(x) for x in gaps], [0x1120, None, 0x1900])
    check("cut_point: a legacy V->S row without a gap cuts at its address", cut_point(sm[0]), 0x1100)
    check("cut_addresses drops the wide gap", cut_addresses(gaps), {0x1120, 0x1900})
    check("cut_ranges cut at the tail's end, never at the first string or inside a wide gap",
          cut_ranges(0x1000, 0x2000, gaps), [(0x1000, 0x1120), (0x1120, 0x1900), (0x1900, 0x2000)])
    gw = warning(0x1000, 0x2000, gaps) or ""
    check("the gap warning says a boundary lies in [addr, latest), with a lower bound",
          ("boundary in [0x00001100, 0x00001180)" in gw, "boundary in [0x00001200, 0x00001800)" in gw,
           "spans at least 4 TUs" in gw), (True, True, True))
    check("... and lists the wide gap as uncertain, not cut",
          ("a boundary also lies in [0x00001200, 0x00001800)" in gw, "0x00001200-" in gw), (True, False))
    only_wide = warning(0x1000, 0x2000, [wide]) or ""
    check("a range with only a wide gap warns and suggests no cut",
          ("no cut suggested" in only_wide, "suggested cuts:" in only_wide), (True, False))
    check("the multi-TU/order-only lines say 'a boundary in [a, b)' for a gap",
          "a boundary in [0x00001200, 0x00001800)" in multi_tu_message(".data", 0x1000, 0x2000, [wide]), True)
    g = globals()
    saved = g["section_chunks"]
    try:
        g["section_chunks"] = lambda path, section: {"o.o": [b"S1", b"V1", b"S2"], "t.o": [b"V1", b"S1", b"S2"],
                                                      "same.o": [b"S1", b"V1"], "other.o": [b"XX"]}[path]
        order = seam_note("u", "o.o", "t.o", seams=sm, rng=(0x1000, 0x1300)) or ""
        check("same symbols in another sequence is order-only, with the seams",
              (": order-only: the unit spans several TUs; seams: at 0x00001100, at 0x00001200" in order), True)
        check("identical objects need no note", seam_note("u", "same.o", "same.o", seams=sm, rng=(0x1000, 0x1300)), None)
        check("different symbols are the multi-TU line, not order-only",
              "order-only" in (seam_note("u", "o.o", "other.o", seams=sm, rng=(0x1000, 0x1300)) or ""), False)
    finally:
        g["section_chunks"] = saved
    if os.path.exists(os.path.join(_paths()["root"], "orig", "RMHE08", "sys", "main.dol")):
        real = seams_in(load_strong(), 0x805F94E0, 0x805FA4EC)
        check("real DOL: the four network_transport V->S seams are in the unit's run",
              [s["addr"] for s in real if s["kind"] == "V->S"], [0x805F9570, 0x805F9610, 0x805F9958, 0x805F9A40])
        check("real DOL: the load did not fail", load_error(), None)
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("range", nargs="*", help="START END (hex): only the seams inside this range")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    seams = load_strong()
    if load_error():
        print("dataseams: %s" % load_error(), file=sys.stderr)
        return 2
    if len(args.range) == 2:
        seams = seams_in(seams, int(args.range[0], 16), int(args.range[1], 16))
    for s in seams:
        print("0x%08X  %-7s %s -> %s" % (s["addr"], s["kind"], s["before"], s["after"]))
    print("%d strong seam(s)" % len(seams))
    return 0


if __name__ == "__main__":
    sys.exit(main())
