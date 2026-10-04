#!/usr/bin/env python3
"""Pair-gap report: the symbols a unit's object defines against the ones its target object has.
Spec: docs/tools/spec/pairgap.md. CLI: pairgap.py [<unit>...] [-u U] [--mode M] [--sections S] [--threshold P]
[--summary] [--json F] [--check] | --selftest."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import contextlib
import json
import os
import sys
from dataclasses import dataclass, field

from tools.lib import repo as _repo
from tools.lib import objcompare

# objdiff's report metric omits the key when nothing matched; the project reads that as 0 %, never 100 %.
DEFAULT_THRESHOLD = 50.0                # percent
DEFAULT_REPORT = os.path.join("build", "RMHE08", "report.json")

META_SECTIONS = objcompare.META_SECTIONS
DATA_SECTIONS = objcompare.DATA_SECTIONS
CLASS_ORDER = objcompare.CLASS_ORDER
MODE_CLASSES = objcompare.MODE_CLASSES
Sym = objcompare.SymbolSize             # one defined symbol: name, size, section
section_kind = objcompare.section_kind
wanted_kinds = objcompare.wanted_kinds
read_symbols = objcompare.defined_symbols
size_delta = objcompare.size_delta


@dataclass
class Row:
    unit: str = ""
    cls: str = "size-gap"
    name: str = ""
    target_size: int = 0
    ours_size: int = 0
    section: str = ""
    delta: float = 0.0            # 0.0 = equal, 1.0 = one side absent (or 0-size)
    listed: bool = False          # does `report.json` list this symbol for this unit?
    score: float | None = None    # the report's fuzzy_match_percent; None == 0 % (or not listed)

    @property
    def ratio(self) -> float:
        lo, hi = min(self.target_size, self.ours_size), max(self.target_size, self.ours_size)
        return (hi / lo) if lo else float("inf")

    @property
    def reads_as_zero(self) -> bool:
        """True when the report lists this symbol with no `fuzzy_match_percent` key.

        That is the row the project reads as 0 % (a fully matched row *keeps* its key at 100.0), so it is
        the shape this tool exists to tell apart from an unwritten body.
        """
        return self.listed and self.score is None


def compare(target: dict[str, Sym], ours: dict[str, Sym], threshold: float = DEFAULT_THRESHOLD,
            mode: str = "all") -> list[Row]:
    """The rows of the requested classes between the target's symbols and ours (`objcompare.symbols`)."""
    return [Row(cls=g.cls, name=g.name, target_size=g.target_size, ours_size=g.ours_size, section=g.section,
                delta=g.delta) for g in objcompare.symbols(target, ours, threshold, mode)]


# --------------------------------------------------------------------------------------------------
# units
# --------------------------------------------------------------------------------------------------

def _norm(path: str) -> str:
    """Forward slashes for any path that goes into output - `build/RMHE08/obj/...` reads the same
    everywhere, and the JSON is diffed between hosts."""
    return path.replace("\\", "/")


@dataclass
class UnitRef:
    name: str                 # objdiff unit name, e.g. "main/g3d/fn_80075DCC"
    source: str = ""          # absolute source path, when the report knows it
    target: str = ""
    ours: str = ""


@dataclass
class Scan:
    """Everything one invocation found: the rows, the units, and what it had to skip."""
    root: str = ""
    rows: list[Row] = field(default_factory=list)
    units: list[UnitRef] = field(default_factory=list)
    skipped: list[tuple[str, str]] = field(default_factory=list)
    report: str = ""
    report_stale: str = ""
    threshold: float = DEFAULT_THRESHOLD
    mode: str = "all"
    sections: str = "code"


def units_from_report(report_path: str, root: str) -> list[UnitRef]:
    """One UnitRef per non-auto unit the report knows, in report order."""
    with open(report_path, encoding="utf-8") as fh:
        report = json.load(fh)
    out: list[UnitRef] = []
    for unit in report.get("units") or []:
        md = unit.get("metadata") or {}
        src = md.get("source_path")
        if not src or md.get("auto_generated"):
            continue
        stem = os.path.splitext(src[4:] if src.startswith("src/") else src)[0]
        out.append(UnitRef(name=unit["name"], source=_norm(os.path.join(root, src)),
                           target=_norm(os.path.join(root, "build", "RMHE08", "obj", stem + ".o")),
                           ours=_norm(os.path.join(root, "build", "RMHE08", "src", stem + ".o"))))
    return out


def _stem_of(spec: str) -> str:
    s = spec.replace("\\", "/").strip()
    for pre in ("build/RMHE08/obj/", "build/RMHE08/src/", "build/", "src/"):
        if s.startswith(pre):
            s = s[len(pre):]
    if s.startswith("main/"):
        s = s[5:]
    s = os.path.splitext(s)[0]
    if "/" in s and s.split("/")[0] in ("obj", "src"):
        s = s.split("/", 1)[1]
    return s


def resolve_specs(specs, known, root) -> tuple[list[UnitRef], list[str]]:
    """Unit specs -> UnitRefs, plus the specs that matched nothing.

    A spec is accepted in every spelling `lib.units.Unit.resolve` accepts, and is matched against the
    report's unit list *first* (so `-u g3d/fn_80075DCC`, `main/g3d/fn_80075DCC` and `src/g3d/...` all
    land on one unit).  A spec the report does not know is still resolved from its path, which is how a
    brand-new registration is inspected before the report is regenerated.
    """
    by_name = {u.name: u for u in known}
    by_stem = {u.name[len("main/"):]: u for u in known}
    out, bad = [], []
    for spec in specs:
        if spec in by_name:
            out.append(by_name[spec])
            continue
        stem = _stem_of(spec)
        if stem in by_stem:
            out.append(by_stem[stem])
            continue
        hits = [u for u in known if u.name == "main/" + stem or u.name.endswith("/" + stem)]
        if len(hits) == 1:
            out.append(hits[0])
            continue
        if len(hits) > 1:
            bad.append("%s (ambiguous: %s)" % (spec, ", ".join(h.name for h in hits)))
            continue
        obj = _norm(os.path.join(root, "build", "RMHE08", "obj", stem + ".o"))
        src_obj = _norm(os.path.join(root, "build", "RMHE08", "src", stem + ".o"))
        if os.path.exists(obj) or os.path.exists(src_obj):
            out.append(UnitRef(name="main/" + stem, target=obj, ours=src_obj))
            continue
        bad.append(spec)
    return out, bad


# --------------------------------------------------------------------------------------------------
# the report cross-check (a cross-check, never the measurement)
# --------------------------------------------------------------------------------------------------

def report_scores(report_path: str) -> dict[str, dict[str, float | None]]:
    """{unit name: {symbol name: fuzzy_match_percent or None}} - None == listed at 0 %."""
    with open(report_path, encoding="utf-8") as fh:
        report = json.load(fh)
    out: dict[str, dict[str, float | None]] = {}
    for unit in report.get("units") or []:
        fns = unit.get("functions")
        if fns is None:
            continue
        out[unit["name"]] = {f.get("name"): f.get("fuzzy_match_percent") for f in fns}
    return out


def annotate(row: Row, scores: dict[str, float | None] | None) -> None:
    """Fill a row's report half in place; no report (or no entry) leaves `listed` False."""
    if scores is None:
        return
    if row.name in scores:
        row.listed = True
        row.score = scores[row.name]


def staleness_note(report_path: str, inputs: list[str]) -> str:
    """Which build the report is, or a warning that it predates what is being compared."""
    try:
        r_mtime = os.path.getmtime(report_path)
    except OSError:
        return ""
    newer = [p for p in inputs if os.path.exists(p) and os.path.getmtime(p) > r_mtime]
    if not newer:
        return ""
    return ("report predates %d of the compared file(s) (newest: %s) - its scores are from an earlier "
            "build; regenerate with `ninja build/RMHE08/report.json` after deleting it"
            % (len(newer), os.path.relpath(newer[0]).replace("\\", "/")))


def scan(units, root, scores, threshold, mode, scope, all_sections=False) -> tuple[Scan, list[str]]:
    """Read every unit's two objects; -> (Scan, the files it read, for the staleness cross-check)."""
    kinds = wanted_kinds(scope, all_sections)
    out = Scan(root=root, threshold=threshold, mode=mode, sections=scope)
    inputs: list[str] = []
    for unit in units:
        if not os.path.exists(unit.target):
            out.skipped.append((unit.name, "no target object (%s)" % _rel(root, unit.target)))
            continue
        if not os.path.exists(unit.ours):
            out.skipped.append((unit.name, "our object not built (%s)" % _rel(root, unit.ours)))
            continue
        inputs += [unit.target, unit.ours, unit.source] if unit.source else [unit.target, unit.ours]
        rows = compare(read_symbols(unit.target, kinds), read_symbols(unit.ours, kinds),
                       threshold=threshold, mode=mode)
        if not rows:
            out.units.append(unit)
            continue
        for row in rows:
            row.unit = unit.name
            annotate(row, (scores or {}).get(unit.name))
        out.rows += rows
        out.units.append(unit)
    # the rows the report shows as 0 % (no `fuzzy_match_percent` key) lead their class: they are the ones
    # a lane reads as "unwritten", which is exactly what this tool exists to correct.
    out.rows.sort(key=lambda r: (CLASS_ORDER.index(r.cls), 0 if r.reads_as_zero else 1,
                                 -max(r.target_size, r.ours_size), r.unit, r.name))
    return out, inputs


def _rel(root: str, path: str) -> str:
    try:
        return os.path.relpath(path, root).replace("\\", "/")
    except ValueError:
        return path


# --------------------------------------------------------------------------------------------------
# printing
# --------------------------------------------------------------------------------------------------

def fmt_delta(delta: float) -> str:
    return "%.1f%%" % (delta * 100.0)


def fmt_score(row: Row, with_report: bool) -> str:
    if not with_report:
        return "-"
    if not row.listed:
        return "not listed"
    if row.score is None:
        return "0 %*"
    return "%.2f %%" % row.score


def print_rows(rows: list[Row], with_report: bool, limit: int = 0, header: bool = True) -> int:
    """Print the table; returns how many rows were suppressed by `--limit`."""
    if not rows:
        return 0
    shown = rows if limit <= 0 else rows[:limit]
    if header:
        print("%-9s %-40s %-40s %9s %9s %8s %-10s %s"
              % ("class", "unit", "symbol", "target", "ours", "size d", "section", "report"))
    for r in shown:
        print("%-9s %-40s %-40s %8d B %8d B %8s %-10s %s"
              % (r.cls, _clip(r.unit, 40), _clip(r.name, 40), r.target_size, r.ours_size,
                 fmt_delta(r.delta), r.section, fmt_score(r, with_report)))
    return len(rows) - len(shown)


def _clip(s: str, n: int) -> str:
    return s if len(s) <= n else s[: n - 3] + "..."


def print_summary(scanres: Scan, with_report: bool) -> None:
    counts = {c: 0 for c in CLASS_ORDER}
    zero = {c: 0 for c in CLASS_ORDER}
    per_unit: dict[str, dict[str, int]] = {}
    for r in scanres.rows:
        counts[r.cls] += 1
        if r.reads_as_zero:
            zero[r.cls] += 1
        per_unit.setdefault(r.unit, {c: 0 for c in CLASS_ORDER})[r.cls] += 1
    print("units scanned      : %d" % len(scanres.units))
    if scanres.skipped:
        print("units skipped      : %d" % len(scanres.skipped))
    label = {"size-gap": "size-gap rows      ", "missing": "missing-in-ours    ",
             "extra": "extra-in-ours      "}
    for cls in MODE_CLASSES[scanres.mode]:
        extra = ""
        if with_report:
            extra = (("  (%d of them read as 0 %% in the report)" % zero[cls]) if cls == "size-gap"
                     else ("  (%d at 0 %%)" % zero[cls]))
        print("%s: %d%s" % (label[cls], counts[cls], extra))
    if per_unit and "size-gap" in MODE_CLASSES[scanres.mode]:
        worst = sorted(per_unit.items(), key=lambda kv: (-kv[1]["size-gap"], kv[0]))
        hottest = [u for u, c in worst if c["size-gap"]]
        if hottest:
            print("units with a size gap: %d, the worst being %s"
                  % (len(hottest), ", ".join("%s(%d)" % (u, per_unit[u]["size-gap"])
                                             for u in hottest[:5])))
    return None


# --------------------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("units", nargs="*", help="unit specs; default: every unit the report knows")
    ap.add_argument("-u", "--unit", action="append", default=[], dest="unit_flags",
                    help="the same as a positional unit spec (repeatable)")
    ap.add_argument("--match", action="append", default=[],
                    help="only units whose name contains this substring (repeatable)")
    ap.add_argument("--threshold", default=str(DEFAULT_THRESHOLD),
                    help="size difference, in percent, above which a pair is a gap (default 50)")
    ap.add_argument("--mode", choices=sorted(MODE_CLASSES), default="all")
    ap.add_argument("--sections", choices=("code", "data", "all"), default="code",
                    help="which sections to compare (default code = .text/.init)")
    ap.add_argument("--all-sections", action="store_true",
                    help="with --sections all, include .comment/.strtab/.symtab/.rela* too")
    ap.add_argument("--report", default=DEFAULT_REPORT, help="report.json to cross-check against")
    ap.add_argument("--no-report", action="store_true", help="skip the report cross-check entirely")
    ap.add_argument("--json", help="write the scan as JSON here ('-' = stdout)")
    ap.add_argument("--limit", type=int, default=0, help="print at most N rows per class (0 = all)")
    ap.add_argument("--summary", action="store_true", help="counts only, no rows")
    ap.add_argument("--check", action="store_true",
                    help="exit 1 when a row of the requested class exists")
    ap.add_argument("--root", help="the tree to read (default: the caller's git worktree)")
    ap.add_argument("--selftest", action="store_true")
    return ap


def parse_threshold(text: str) -> float:
    s = str(text).strip().rstrip("%").strip()
    try:
        value = float(s)
    except ValueError:
        raise SystemExit("--threshold must be a percentage, e.g. 50 or 50%% (got %r)" % text)
    if value < 0:
        raise SystemExit("--threshold must not be negative (got %r)" % text)
    return value


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    if args.selftest:
        return selftest()

    root = os.path.abspath(args.root) if args.root else _repo.repo_root()
    threshold = parse_threshold(args.threshold)
    specs = list(args.units) + list(args.unit_flags)

    report_path = args.report if os.path.isabs(args.report) else os.path.join(root, args.report)
    with_report = not args.no_report and os.path.exists(report_path)

    known: list[UnitRef] = []
    scores: dict[str, dict[str, float | None]] | None = None
    if os.path.exists(report_path):
        known = units_from_report(report_path, root)
        if with_report:
            scores = report_scores(report_path)

    if specs:
        units, bad = resolve_specs(specs, known, root)
        for spec in bad:
            print("no unit for %r" % spec, file=sys.stderr)
        if not units:
            print("no unit resolved - `--json -` lists nothing; units the report knows start with %s"
                  % (known[0].name if known else "main/"), file=sys.stderr)
            return 2
    else:
        units = known
        if not units:
            raise SystemExit("no report at %s - run `ninja build/RMHE08/report.json` first, or name a "
                             "unit explicitly" % _rel(root, report_path))

    for sub in args.match:
        units = [u for u in units if sub.lower() in u.name.lower()]

    scanres, inputs = scan(units, root, scores, threshold, args.mode, args.sections, args.all_sections)
    scanres.report = _rel(root, report_path) if os.path.exists(report_path) else ""
    if with_report:
        scanres.report_stale = staleness_note(report_path, inputs)

    for name, why in scanres.skipped:
        print("skip %s: %s" % (name, why), file=sys.stderr)

    # with `--json -` the JSON owns stdout, so the human table goes to stderr and both stay usable
    stream = contextlib.redirect_stdout(sys.stderr) if args.json == "-" else contextlib.nullcontext()
    with stream:
        print("== pairgap: tree %s, report %s  (threshold %.0f%%, mode %s, sections %s)"
              % (_rel(root, root),
                 _rel(root, report_path) if os.path.exists(report_path) else "none",
                 threshold, args.mode, args.sections))
        if scanres.report_stale:
            print("!! " + scanres.report_stale)
        print_report(scanres, with_report, args.limit, args.summary)

    if args.json:
        payload = _json_payload(scanres, with_report, threshold)
        text = json.dumps(payload, indent=1)
        if args.json == "-":
            print(text)
        else:
            with open(args.json, "w", encoding="utf-8") as fh:
                fh.write(text)
            print("wrote %s" % args.json)

    if args.check and any(r.cls in MODE_CLASSES[args.mode] for r in scanres.rows):
        return 1
    return 0


def print_report(scanres: Scan, with_report: bool, limit: int, summary_only: bool) -> int:
    """The human half; -> how many rows `--limit` suppressed."""
    if summary_only:
        print_summary(scanres, with_report)
        return 0
    per_class: dict[str, list[Row]] = {c: [] for c in CLASS_ORDER}
    for r in scanres.rows:
        per_class[r.cls].append(r)
    suppressed, printed_header = 0, False
    for cls in MODE_CLASSES[scanres.mode]:
        rows = per_class[cls]
        if not rows:
            continue
        print("\n-- %s (%d)%s" % (cls, len(rows),
                                  " - defined on both sides, sizes apart" if cls == "size-gap"
                                  else (" - the target has it, our object does not"
                                        if cls == "missing"
                                        else " - our object has it, the target does not")))
        suppressed += print_rows(rows, with_report, limit, header=not printed_header)
        printed_header = True
    print_summary(scanres, with_report)
    if suppressed:
        print("(%d further row(s) suppressed by --limit; use --limit 0 for all)" % suppressed)
    if scanres.rows and "size-gap" in MODE_CLASSES[scanres.mode]:
        print("\nthe size-gap rows are the ones no other tool names: objdiff pairs them by name, and "
              "neither its report nor `symdiff.py -u <unit>` ever prints name bytes-to-name bytes")
    return suppressed


def _json_payload(scanres: Scan, with_report: bool, threshold: float) -> dict:
    return {
        "root": scanres.root,
        "report": scanres.report,
        "report_used": with_report,
        "report_stale": scanres.report_stale,
        "threshold_percent": threshold,
        "mode": scanres.mode,
        "sections": scanres.sections,
        "units": [{"name": u.name, "target": u.target, "ours": u.ours} for u in scanres.units],
        "skipped": [{"unit": n, "why": w} for n, w in scanres.skipped],
        "rows": [{"unit": r.unit, "class": r.cls, "symbol": r.name, "target_size": r.target_size,
                  "ours_size": r.ours_size, "delta_percent": round(r.delta * 100.0, 4),
                  "ratio": (None if r.ratio == float("inf") else round(r.ratio, 4)),
                  "section": r.section, "report_listed": r.listed,
                  "report_score": r.score, "reads_as_zero": r.reads_as_zero}
                 for r in scanres.rows],
    }


# --------------------------------------------------------------------------------------------------
# selftest - fixtures only, so it runs in a tree with no build
# --------------------------------------------------------------------------------------------------

def _elf32(sections, symbols) -> bytes:
    """A minimal ELF32 big-endian relocatable object holding `sections` and `symbols`.

    Enough for `lib.binary.elf.Elf`: one section-header table, a `SHT_SYMTAB`, its `.strtab` and a
    `.shstrtab`.  `sections` is `[(name, bytes)]`; `symbols` is `[(name, value, size, st_info, section
    name)]`.  It emits no code - only the symbol/layout records this tool compares - which is the point:
    the comparison is a function of the *symbol tables*, so the fixture is the smallest object that
    exercises the real reader.
    """
    import struct

    sec_names = [name for name, _data in sections]
    shstr, offs = bytearray(), {}
    for n in [""] + sec_names + [".symtab", ".strtab", ".shstrtab"]:
        offs[n] = len(shstr)
        shstr += n.encode() + b"\0"

    strtab, sym_offs = bytearray(b"\0"), {"": 0}
    for sym in symbols:
        sym_offs[sym[0]] = len(strtab)
        strtab += sym[0].encode() + b"\0"

    # section index: 0 = null, 1..N = the data sections, then symtab, strtab, shstrtab
    sec_index = {name: i + 1 for i, name in enumerate(sec_names)}
    body, data_offs = bytearray(), {}
    for nm, data in sections:
        data_offs[nm] = (len(body), len(data))
        body += data
    symtab_off = len(body)
    symtab = bytearray(struct.pack(">IIIBBH", 0, 0, 0, 0, 0, 0))
    for nm, value, size, stype, sec in symbols:
        symtab += struct.pack(">IIIBBH", sym_offs[nm], value, size, (stype & 0xF) << 4, 0,
                              sec_index[sec])
    body += symtab
    strtab_off = len(body)
    body += strtab
    shstr_off = len(body)
    body += shstr

    _, strtab_index, shstrtab_index = (1 + len(sections) + i for i in range(3))
    shnum = 1 + len(sections) + 3
    ehsize = 52                                             # section offsets are FILE offsets

    def sh(name, typ, off, size, link=0, info=0, align=1, entsize=0):
        return struct.pack(">IIIIIIIIII", offs[name], typ, 0, 0, ehsize + off, size, link, info,
                           align, entsize)

    hdr = bytearray(b"\0" * 40)                              # the null section
    for nm, _data in sections:
        o, sz = data_offs[nm]
        hdr += sh(nm, 1, o, sz)                              # SHT_PROGBITS
    hdr += sh(".symtab", 2, symtab_off, len(symtab), link=strtab_index, info=1, align=4, entsize=16)
    hdr += sh(".strtab", 3, strtab_off, len(strtab))
    hdr += sh(".shstrtab", 3, shstr_off, len(shstr))
    assert len(hdr) == 40 * shnum

    ehdr = bytearray(b"\x7fELF\x01\x02\x01" + b"\0" * 9)   # 32-bit, big-endian, version 1
    ehdr += struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, ehsize + len(body), 0, ehsize, 0, 0, 40,
                        shnum, shstrtab_index)
    assert len(ehdr) == ehsize
    return bytes(ehdr) + bytes(body) + bytes(hdr)


def _fixture(dirpath):
    """Two objects carrying every shape the selftest must name, and the two it must not."""
    target = [
        ("fn_big", 0, 100, 2, ".text"),          # ours is 40 B -> 60 % gap, found
        ("fn_edge", 100, 100, 2, ".text"),       # ours is 52 B -> 48 % gap, NOT found
        ("fn_same", 200, 64, 2, ".text"),        # identical size, NOT found
        ("fn_gone", 264, 32, 2, ".text"),        # absent from ours -> missing
        ("fn_gap2", 296, 200, 2, ".text"),       # a bigger gap the report does *not* read as 0 %
    ]
    ours = [
        ("fn_big", 0, 40, 2, ".text"),
        ("fn_edge", 40, 52, 2, ".text"),
        ("fn_same", 92, 64, 2, ".text"),
        ("fn_extra", 156, 12, 2, ".text"),       # absent from the target -> extra
        ("fn_gap2", 168, 30, 2, ".text"),
    ]
    t = os.path.join(dirpath, "target.o")
    o = os.path.join(dirpath, "ours.o")
    with open(t, "wb") as fh:
        fh.write(_elf32([(".text", bytes(496))], target))
    with open(o, "wb") as fh:
        fh.write(_elf32([(".text", bytes(198))], ours))
    return t, o


def selftest() -> int:
    import tempfile

    checks = 0
    fails = 0

    def ok(what, got, want):
        nonlocal checks, fails
        checks += 1
        if got == want:
            print("ok    %s" % what)
        else:
            fails += 1
            print("FAIL  %s\n        got:  %r\n        want: %r" % (what, got, want))

    def truth(what, cond):
        ok(what, bool(cond), True)

    # --- pure: the arithmetic ---------------------------------------------------------------
    ok("size_delta of equal sizes is 0", size_delta(100, 100), 0.0)
    ok("size_delta is symmetric", size_delta(100, 40), size_delta(40, 100))
    ok("size_delta of 100 vs 40 is 60 %", round(size_delta(100, 40), 6), 0.6)
    ok("size_delta with a zero side is 1.0", size_delta(0, 100), 1.0)
    ok("size_delta of two zeros is 0", size_delta(0, 0), 0.0)

    # --- pure: the classes, on dicts --------------------------------------------------------
    T = {"fn_big": Sym("fn_big", 100, ".text"), "fn_edge": Sym("fn_edge", 100, ".text"),
         "fn_same": Sym("fn_same", 64, ".text"), "fn_gone": Sym("fn_gone", 32, ".text")}
    O = {"fn_big": Sym("fn_big", 40, ".text"), "fn_edge": Sym("fn_edge", 52, ".text"),
         "fn_same": Sym("fn_same", 64, ".text"), "fn_extra": Sym("fn_extra", 12, ".text")}

    rows = compare(T, O, threshold=50.0)
    ok("a >50 % gap is found", [r.name for r in rows if r.cls == "size-gap"], ["fn_big"])
    ok("a 48 % gap just under the threshold is NOT found",
       "fn_edge" in [r.name for r in rows if r.cls == "size-gap"], False)
    ok("a missing-in-ours symbol is found", [r.name for r in rows if r.cls == "missing"], ["fn_gone"])
    ok("an extra-in-ours symbol is found", [r.name for r in rows if r.cls == "extra"], ["fn_extra"])
    ok("an equal size is in no class", [r.name for r in rows if r.name == "fn_same"], [])
    ok("the gap row carries both sizes and the section",
       [(r.target_size, r.ours_size, r.section) for r in rows if r.cls == "size-gap"],
       [(100, 40, ".text")])
    ok("the gap row's delta is 60 %",
       round([r.delta for r in rows if r.name == "fn_big"][0], 6), 0.6)
    ok("the ratio is printed beside the delta",
       round([r for r in rows if r.name == "fn_big"][0].ratio, 4), 2.5)
    ok("size-gap rows sort before missing and extra",
       [r.cls for r in rows], ["size-gap", "missing", "extra"])

    # --- the threshold flag is honoured -----------------------------------------------------
    ok("--threshold 60 drops the 60 % gap (strictly greater)",
       [r.name for r in compare(T, O, threshold=60.0) if r.cls == "size-gap"], [])
    ok("--threshold 59 keeps the 60 % gap",
       [r.name for r in compare(T, O, threshold=59.0) if r.cls == "size-gap"], ["fn_big"])
    ok("--threshold 45 finds the 48 % gap as well",
       [r.name for r in compare(T, O, threshold=45.0) if r.cls == "size-gap"], ["fn_big", "fn_edge"])
    ok("--threshold 0 reports every unequal pair",
       sorted(r.name for r in compare(T, O, threshold=0.0) if r.cls == "size-gap"),
       ["fn_big", "fn_edge"])
    ok("the flag parses a bare number and a percent sign",
       (parse_threshold("50"), parse_threshold("50%"), parse_threshold(" 12.5 ")), (50.0, 50.0, 12.5))

    # --- the mode flag is honoured ----------------------------------------------------------
    ok("--mode gap keeps only size gaps",
       sorted({r.cls for r in compare(T, O, mode="gap")}), ["size-gap"])
    ok("--mode missing keeps only the target-only class",
       [r.name for r in compare(T, O, mode="missing")], ["fn_gone"])
    ok("--mode extra keeps only our-only class",
       [r.name for r in compare(T, O, mode="extra")], ["fn_extra"])

    # --- the report cross-check -------------------------------------------------------------
    r0 = Row(name="fn_gone", listed=True, score=None)
    truth("a listed symbol with no key reads as 0 %", r0.reads_as_zero)
    ok("a listed symbol with a score is not a 0 % read",
       Row(name="x", listed=True, score=0.65).reads_as_zero, False)
    ok("a symbol the report does not list is not a 0 % read",
       Row(name="x", listed=False, score=None).reads_as_zero, False)
    ok("the score column marks a key-less 0 %", fmt_score(Row(listed=True, score=None), True), "0 %*")
    ok("the score column names a symbol the report omits",
       fmt_score(Row(listed=False), True), "not listed")
    ok("--no-report prints no score", fmt_score(Row(listed=True, score=99.0), False), "-")

    # --- end to end, through the real ELF reader --------------------------------------------
    with tempfile.TemporaryDirectory(prefix="pairgap-") as d:
        t, o = _fixture(d)
        ts = read_symbols(t, wanted_kinds("code"))
        os_ = read_symbols(o, wanted_kinds("code"))
        ok("the fixture reads back through the ELF reader",
           sorted(ts), ["fn_big", "fn_edge", "fn_gap2", "fn_gone", "fn_same"])
        ok("the fixture's sizes read back", [ts["fn_big"].size, os_["fn_big"].size], [100, 40])
        ok("the fixture's section reads back", ts["fn_big"].section, ".text")
        rows = compare(ts, os_, threshold=50.0)
        ok("a >50 % gap is found in a real object",
           [r.name for r in rows if r.cls == "size-gap"], ["fn_gap2", "fn_big"])
        ok("a missing-in-ours symbol is found in a real object",
           [r.name for r in rows if r.cls == "missing"], ["fn_gone"])
        ok("an extra-in-ours symbol is found in a real object",
           [r.name for r in rows if r.cls == "extra"], ["fn_extra"])
        ok("the gap row names the section it lives in",
           [r.section for r in rows if r.cls == "size-gap"], [".text", ".text"])
        # the whole scan path, with the fixture objects standing in for the build tree
        root = os.path.dirname(d)
        u = UnitRef(name="main/fixture", target=t, ours=o)
        res, inputs = scan([u], root,
                           {"main/fixture": {"fn_big": None, "fn_gap2": 1.5, "fn_gone": 0.65}},
                           50.0, "all", "code")
        ok("scan() returns one row per class", sorted({r.cls for r in res.rows}),
           ["extra", "missing", "size-gap"])
        ok("scan() leads a class with the rows the report reads as 0 %",
           [r.name for r in res.rows if r.cls == "size-gap"], ["fn_big", "fn_gap2"])
        ok("scan() attributes every row to the unit", {r.unit for r in res.rows}, {"main/fixture"})
        ok("scan() fills the report score from the cross-check",
           [r.score for r in res.rows if r.name == "fn_gap2"], [1.5])
        ok("scan() marks a key-less report row as a 0 % read",
           [[r.reads_as_zero for r in res.rows if r.name == n] for n in ("fn_big", "fn_gone")],
           [[True], [False]])
        ok("scan() lists the compared files as staleness inputs",
           len(inputs), 2)
        res2, _ = scan([UnitRef(name="main/none", target=os.path.join(d, "nope.o"),
                                ours=os.path.join(d, "nope2.o"))], root, None, 50.0, "all", "code")
        ok("scan() skips a unit whose target object is absent",
           [why.split(" (")[0] for _n, why in res2.skipped], ["no target object"])
        ok("scan() skips a unit whose object we have not built",
           [why.split(" (")[0] for _n, why in scan([UnitRef(name="m", target=t,
                                                           ours=os.path.join(d, "nope.o"))],
                                                   root, None, 50.0, "all", "code")[0].skipped],
           ["our object not built"])

    # --- the section scope -------------------------------------------------------------------
    ok("--sections code ignores .comment", read_scope_names(".comment", "code"), [])
    ok("--sections all ignores .comment by default", read_scope_names(".comment", "all"), [])
    ok("--sections all --all-sections keeps .comment", read_scope_names(".comment", "all", True), ["x"])
    ok("--sections data ignores .text", read_scope_names(".text", "data"), [])
    ok("--sections data keeps .sdata2", read_scope_names(".sdata2", "data"), ["x"])
    ok("extab is not code and not data", section_kind("extab"), "other")
    ok("a .rela section is metadata", section_kind(".rela.text"), "meta")
    ok("a .text.N sub-section is code", section_kind(".text.hot"), "code")

    print("\npairgap selftest: %d checks, %d failed" % (checks, fails))
    return 1 if fails else 0


def read_scope_names(section: str, scope: str, all_sections: bool = False) -> list[str]:
    """Selftest helper: the symbol names a scope keeps, for a one-section fake object."""
    import tempfile
    with tempfile.TemporaryDirectory(prefix="pairgap-scope-") as d:
        p = os.path.join(d, "s.o")
        with open(p, "wb") as fh:
            fh.write(_elf32([(section, bytes(8))], [("x", 0, 8, 2, section)]))
        return sorted(read_symbols(p, wanted_kinds(scope, all_sections)))


if __name__ == "__main__":
    sys.exit(main())
