#!/usr/bin/env python3
"""dataseams.py - the `.data` emission-order seams (tools/splits/dataorder.py) as a guard for data-claim tools.

`dataorder.py` classifies every retail `.data` symbol and finds the TU seams (docs/data-order-seams.md): a vtable
followed by a string (`V->S`) or two adjacent vtables whose owners go up (`zigzag`) start a new TU.  Those two are
**strong**; `V->D` is weak (a jump table is `.data` too) and is never used to cut or refuse.

This module is the thin consumer layer the claim tools share - `dataqueue.py` (cut a proposed run), `dataclaim.py`
(warn on a run or a rule-12 claim), `flipcheck.py` and `datagap.py` (name an order-only mismatch).  It classifies
nothing itself: every kind comes from `dataorder`.

    python tools/units/dataseams.py [START END]     # the strong seams (optionally inside one range)
    python tools/units/dataseams.py --selftest
"""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
for _p in (os.path.join(TOOLS, "splits"), TOOLS, HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

STRONG = ("V->S", "zigzag")
_CACHE: dict = {}


def load_strong() -> list[dict]:
    """Every strong `.data` seam of the retail DOL as `{addr, kind, before, after}`, cached.

    Returns `[]` (never raises) when the DOL or the map is not readable: a guard must not break a tool that
    could otherwise run; `load_error()` says why the list is empty.
    """
    if "seams" in _CACHE:
        return _CACHE["seams"]
    try:
        import dataorder as do
        import tudiscover as td
        syms = do.classify_all(do.load_symbols(), td.Dol(do.DOL))
        found = [s for s in do.seams(syms) if s["kind"] in STRONG]
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


def cut_ranges(start: int, end: int, seams: list[dict]) -> list[tuple[int, int]]:
    """`[start, end)` cut at every seam inside it: one `(start, end)` per probable TU."""
    points = [start] + sorted({s["addr"] for s in seams_in(seams, start, end)}) + [end]
    return [(points[i], points[i + 1]) for i in range(len(points) - 1)]


def describe(seams: list[dict]) -> str:
    """`0x805F9570 (V->S), 0x805F9610 (V->S)` - what a warning names."""
    return ", ".join("0x%08X (%s)" % (s["addr"], s["kind"]) for s in sorted(seams, key=lambda s: s["addr"]))


def warning(start: int, end: int, seams: list[dict]) -> str | None:
    """The one-line warning for a range that contains seams, with the suggested cut ranges; None when clean."""
    inside = seams_in(seams, start, end)
    if not inside:
        return None
    return ("spans %d TUs, not one: .data emission-order seams at %s - it can never match as one unit; "
            "suggested cuts: %s" % (len(inside) + 1, describe(inside),
                                    ", ".join("0x%08X-0x%08X" % r for r in cut_ranges(start, end, inside))))


def section_chunks(path: str, section: str) -> list[bytes] | None:
    """The bytes of each symbol defined in `section` of one ELF object, in section-offset order.

    Zero-size symbols are skipped and two symbols at one offset count once (the larger wins). `None` when the
    object or the section is missing or unreadable.
    """
    try:
        from elftools.elf.elffile import ELFFile
        with open(path, "rb") as fh:
            elf = ELFFile(fh)
            sec = elf.get_section_by_name(section)
            symtab = elf.get_section_by_name(".symtab")
            if sec is None or symtab is None:
                return None
            data = sec.data()
            index = next(i for i, s in enumerate(elf.iter_sections()) if s.name == section)
            by_off: dict[int, int] = {}
            for sym in symtab.iter_symbols():
                if sym["st_shndx"] == index and sym["st_size"]:
                    by_off[sym["st_value"]] = max(by_off.get(sym["st_value"], 0), sym["st_size"])
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
        import dataorder as do
        for start, (name, end) in do.unit_data_ranges().items():
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
    return ("%s: order-only: the unit spans several TUs; seams at %s - the target's symbols are the same, "
            "in the order of separate translation units (one TU emits globals, strings, then vtables in "
            "reverse class order), so split the unit at the seams" % (
                section, ", ".join("0x%08X" % s["addr"] for s in sorted(seams, key=lambda s: s["addr"]))))


def multi_tu_message(section: str, start: int, end: int, seams: list[dict]) -> str:
    """The weaker diagnosis: the section differs and the unit's target range spans several TUs."""
    return ("%s: the target range 0x%08X-0x%08X spans %d TUs (emission-order seams at %s) - a unit is one TU, "
            "so this section cannot match until the unit is split at the seams"
            % (section, start, end, len(seams) + 1,
               ", ".join("0x%08X" % s["addr"] for s in sorted(seams, key=lambda s: s["addr"]))))


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
    check("the warning names the seams and the TU count",
          ("0x00001100 (V->S)" in w, "0x00001200 (zigzag)" in w, "spans 3 TUs" in w), (True, True, True))
    check("the warning suggests the cuts", "0x00001000-0x00001100" in w and "0x00001200-0x00001300" in w, True)
    check("order_only: the same symbols reordered", order_only([b"a", b"bb", b"c"], [b"bb", b"a", b"c"]), True)
    check("order_only: identical sequences are not", order_only([b"a", b"bb"], [b"a", b"bb"]), False)
    check("order_only: different bytes are not", order_only([b"a", b"bb"], [b"a", b"bc"]), False)
    check("order_only: a different symbol count is not", order_only([b"a"], [b"a", b"b"]), False)
    check("order_only: no evidence is not", (order_only(None, [b"a"]), order_only([], [])), (False, False))
    check("the order-only line names the seams",
          "seams at 0x00001100, 0x00001200" in order_only_message(".data", sm), True)
    check("... and says so when no seam is known", "no strong .data emission-order seam" in order_only_message(".data", []),
          True)
    check("section_chunks tolerates a missing file", section_chunks("does/not/exist.o", ".data"), None)
    check("a range with no seam gets no note",
          seam_note("u", "no.o", "no2.o", seams=sm, rng=(0x1300, 0x1400)), None)
    multi = seam_note("u", "no.o", "no2.o", seams=sm, rng=(0x1000, 0x1300)) or ""
    check("unreadable objects still name the multi-TU range",
          ("spans 3 TUs" in multi, "0x00001100, 0x00001200" in multi), (True, True))
    g = globals()
    saved = g["section_chunks"]
    try:
        g["section_chunks"] = lambda path, section: {"o.o": [b"S1", b"V1", b"S2"], "t.o": [b"V1", b"S1", b"S2"],
                                                      "same.o": [b"S1", b"V1"], "other.o": [b"XX"]}[path]
        order = seam_note("u", "o.o", "t.o", seams=sm, rng=(0x1000, 0x1300)) or ""
        check("same symbols in another sequence is order-only, with the seams",
              (": order-only: the unit spans several TUs; seams at 0x00001100, 0x00001200" in order), True)
        check("identical objects need no note", seam_note("u", "same.o", "same.o", seams=sm, rng=(0x1000, 0x1300)), None)
        check("different symbols are the multi-TU line, not order-only",
              "order-only" in (seam_note("u", "o.o", "other.o", seams=sm, rng=(0x1000, 0x1300)) or ""), False)
    finally:
        g["section_chunks"] = saved
    if os.path.exists(os.path.join(os.path.dirname(TOOLS), "orig", "RMHE08", "sys", "main.dol")):
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
