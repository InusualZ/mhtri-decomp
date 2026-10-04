#!/usr/bin/env python3
"""Every symbol of one unit from one report read, refusing a stale report. Spec: docs/tools/spec/unitscore.md.
CLI: python tools/objdiff/unitscore.py <unit> [--measure] [--threshold P] [--report R] [--json] [--force-stale] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import sys
from dataclasses import dataclass, field

from tools.lib import project as _project
from tools.lib import repo as _repo
from tools.lib import report as _report
from tools.lib import units as _units
from tools.lib.report import (mtime, stamp, stamp_json, freshness, source_closure,
                              newest, rel_path as _rel)

REPORT_REL = os.path.join("build", "RMHE08", "report.json")


# --------------------------------------------------------------------------------------------------
# the record: one unit, one report read
# --------------------------------------------------------------------------------------------------

@dataclass
class Spec:
    """Everything a run needs about the unit, resolved once."""
    unit: str                                    # stem: `quest/arenatask`
    unit_name: str                               # report unit name: `main/quest/arenatask`
    obj: str                                     # this tree's built object
    target: str                                  # this tree's split target object
    src: str                                     # the unit's source file
    tree: str                                    # the tree the run reads
    report: str                                  # the project report path
    sources: list[str] = field(default_factory=list)


@dataclass
class Row:
    """One symbol of the report, with the reading conventions applied."""
    name: str
    size: int
    address: int | None
    percent: float                               # a missing `fuzzy_match_percent` key is 0.0, not 100.0
    scored: bool                                 # False when the report carried no score for the row


def spec_of(unit_spec: str, report: str | None = None, tree: str | None = None) -> Spec:
    """Resolve a unit spec (`lib.units.Unit.resolve`), plus its include closure and the tree's report path.

    `tree` names the tree to read (default: the invocation's, `lib.repo.repo_root()`), so the unit's own
    paths land under it - a fixture that is not a git worktree resolves there, never in the real tree.
    """
    root = tree or _repo.repo_root()
    unit = _units.Unit.resolve(unit_spec, root)
    src = unit.source if os.path.isabs(unit.source) else os.path.join(root, unit.source)
    return Spec(unit=unit.key, unit_name=unit.report_name, obj=unit.obj_ours, target=unit.obj_target,
                src=src, tree=root, report=os.path.abspath(report) if report else os.path.join(root, REPORT_REL),
                sources=source_closure(src, root))


def rows_of(entry: dict) -> list[Row]:
    """Every symbol the report entry carries, worst first (ties by name) - the one ordering this tool has.

    The percent is the report's own `fuzzy_match_percent`; a row the report left unscored (the key is
    absent) is **0 %, not 100 %** - `lib.report.score_of`.
    """
    rows: list[Row] = []
    for fn in (entry.get("functions") or []):
        name = fn.get("name")
        if not name:
            continue
        try:
            size = int(fn.get("size"))
        except (TypeError, ValueError):
            size = 0
        addr = (fn.get("metadata") or {}).get("virtual_address")
        try:
            address = int(addr) if addr is not None else None
        except (TypeError, ValueError):
            address = None
        rows.append(Row(name=name, size=size, address=address, percent=_report.score_of(fn),
                        scored=_report.is_scored(fn)))
    rows.sort(key=lambda r: (r.percent, r.name))
    return rows


def below(rows: list[Row], threshold: float | None) -> list[Row]:
    """The rows to print: everything, or only those strictly under `threshold`."""
    return rows if threshold is None else [r for r in rows if r.percent < threshold]


def measures_of(entry: dict) -> dict:
    """The unit's own report measures, with the three the campaign quotes as ints."""
    raw = entry.get("measures") or {}
    out: dict = {"fuzzy_match_percent": raw.get("fuzzy_match_percent")}
    for key in ("total_code", "matched_code", "total_data", "matched_data",
                "total_functions", "matched_functions"):
        try:
            out[key] = int(raw.get(key))
        except (TypeError, ValueError):
            out[key] = None
    for key in ("matched_code_percent", "matched_functions_percent", "matched_data_percent"):
        value = raw.get(key)
        out[key] = float(value) if isinstance(value, (int, float)) else None
    return out


def split_claims(tree: str, unit: str) -> dict[str, tuple[int, int]]:
    """`{section: (start, end)}` from `splits.txt` for one unit stem - the unit's registered ranges."""
    path = os.path.join(tree, "config", "RMHE08", "splits.txt")
    try:
        splits = _project.Splits.read(path)
    except OSError:
        return {}
    out: dict[str, tuple[int, int]] = {}
    want = _units.stem(unit)
    for block in splits.blocks:
        if _units.stem(block.unit) == want:
            for r in block.ranges:
                out[r.section] = (r.start, r.end)
    return out


def range_block(claims: dict[str, tuple[int, int]], rows: list[Row]) -> dict | None:
    """The unit's registered `.text` range against the report's rows - a free target-object drift check.

    The report's functions come from the split target object, which `splits.txt` *describes*: if the two
    disagree, the tree's split is older than the registration (the "no unit's split target object moved
    under the batch" class) and the scores are being read off an object the map no longer describes. It is
    a warning here, never a refusal - the freshness guard owns what is fatal.
    """
    claim = claims.get(".text")
    if not claim:
        return None
    start, end = claim
    have = [r for r in rows if r.address is not None]
    rows_bytes = sum(r.size for r in have)
    warnings: list[str] = []
    if sum(r.size for r in rows) != rows_bytes:
        warnings.append("some report rows carry no address")
    if rows_bytes != end - start:
        warnings.append("the report's %d row(s) cover %d B and splits.txt claims %d B"
                        % (len(rows), rows_bytes, end - start))
    if have:
        lo = min(r.address for r in have)
        hi = max(r.address + r.size for r in have)
        if lo != start or hi != end:
            warnings.append("the report's rows span 0x%08X-0x%08X and splits.txt claims "
                            "0x%08X-0x%08X" % (lo, hi, start, end))
    return {"section": ".text", "start": start, "end": end, "size": end - start,
            "rows_bytes": rows_bytes, "ok": not warnings, "warnings": warnings}


def measure_report(spec: Spec, report_path: str) -> tuple[dict | None, str | None]:
    """The report's unit entry for this unit, or `(None, why)`. One file read, zero objdiff calls."""
    if not os.path.exists(report_path):
        default = _rel(os.path.join(spec.tree, REPORT_REL), spec.tree)
        return None, ("no report at %s - build one with `rm -f %s && ninja %s` (one `objdiff report "
                      "generate`, ~40 s), or score the objects on disk with `--measure`"
                      % (report_path, default, default))
    try:
        data = json.load(open(report_path, encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return None, "cannot read %s: %s" % (report_path, exc)
    entry = _report.Report.coerce(data).unit(_units.report_name(spec.unit))
    if entry is None:
        for name in (spec.unit_name, spec.unit):
            for u in (data.get("units") or []):
                if u.get("name") == name:
                    entry = u
                    break
            if entry is not None:
                break
    if entry is None:
        return None, ("%s is not in %s - the report predates the unit's registration; regenerate it "
                      "(`rm -f %s && ninja %s`)" % (spec.unit, report_path, _rel(report_path, spec.tree),
                                                    _rel(report_path, spec.tree)))
    return entry, None


def score_by_measure(spec: Spec, tmpdir: str | None = None) -> tuple[dict | None, str | None]:
    """The unit's symbols scored from the objects on disk with exactly one `objdiff report generate`.

    This is `symdiff.py -u <unit>`'s primitive (`lib.report.score` in the process's unique scratch
    directory, with `lib.report.retry_transient`), so the numbers are the same code path as the project
    report - and a missing target object is an error naming the path, never a table of 0 %.
    """
    if not os.path.exists(spec.obj):
        return None, ("the unit's object does not exist (%s): build it first (`ninja %s`), or use "
                      "`python tools/units/measure.py %s`, which resolves the target and compiles"
                      % (spec.obj, _rel(spec.obj, spec.tree), spec.unit))
    if not os.path.exists(spec.target):
        return None, ("the split target object does not exist (%s): the registration has not been split "
                      "in this tree yet (`python configure.py && ninja`) - `python tools/units/measure.py "
                      "%s` resolves a proposal's target object and compiles in one step"
                      % (spec.target, spec.unit))
    scratch = tmpdir or _repo.session_tmpdir()
    objdiff = _report.objdiff_cli(spec.tree)
    try:
        rep = _report.retry_transient(
            lambda: _report.score(spec.target, spec.obj, spec.unit_name, scratch, objdiff=objdiff, cwd=spec.tree))
    except _report.ReportError as exc:
        return None, ("one `report generate` over %s / %s failed: %s" % (spec.target, spec.obj, exc))
    except OSError as exc:
        return None, ("cannot run objdiff (%s): %s - `python tools/units/measure.py %s` finds the "
                      "tree's own binary" % (objdiff, exc, spec.unit))
    unit = _report.first_unit(rep)
    functions = [dict(e) for e in (unit.get("functions") or []) if e.get("name")]
    return {"name": spec.unit_name, "functions": functions, "measures": unit.get("measures") or {}}, None


# --------------------------------------------------------------------------------------------------
# the run
# --------------------------------------------------------------------------------------------------

def summary_line(spec: Spec, measures: dict, rows: list[Row], shown: int,
                 threshold: float | None) -> str:
    """One line carrying what a lane reports: the counts, the code ratio and the report metric."""
    matched = sum(1 for r in rows if r.percent >= 100.0)
    unscored = sum(1 for r in rows if not r.scored)
    total_code = measures.get("total_code")
    # `matched_code` can be absent (None) while `total_code` is present: the report gives a unit with one
    # partial function `total_code` but no `matched_code` (measured on Network/NetworkSessionManagerPat,
    # one row at 93.14 %), and `None * 100.0` is a TypeError. The absent count reads as 0, exactly the way
    # an absent `fuzzy_match_percent` reads as 0 % in `rows_of`.
    matched_code = measures.get("matched_code") or 0
    parts = ["%d symbol(s): %d at 100.00 %%, %d open" % (len(rows), matched, len(rows) - matched)]
    if unscored:
        parts.append("%d row(s) carry no score in the report - an absent `fuzzy_match_percent` is 0 %%, "
                     "not 100 %%" % unscored)
    if total_code is not None:
        parts.append("code %s/%d B (%s %%)"
                     % (matched_code, total_code,
                        "%.5f" % (100.0 * matched_code / total_code) if total_code else "0"))
    mf, tf = measures.get("matched_functions"), measures.get("total_functions")
    if mf is not None and tf is not None:
        parts.append("matched_functions %d/%d" % (mf, tf))
    if measures.get("fuzzy_match_percent") is not None:
        parts.append("report metric %.5f %%" % measures["fuzzy_match_percent"])
    if threshold is not None:
        parts.append("--threshold %g: %d of %d row(s) shown" % (threshold, shown, len(rows)))
    return "summary: " + "; ".join(parts)


def run(spec: Spec, *, report: str | None = None, threshold: float | None = None, force: bool = False,
        measure: bool = False, tmpdir: str | None = None) -> dict:
    """Score the unit and build the record. `refused` is the freshness guard's verdict, not an error."""
    report_path = os.path.abspath(report) if report else spec.report
    obj_mtime = mtime(spec.obj)
    newest_source = newest(spec.sources)
    mtimes = {"report": mtime(report_path), "object": obj_mtime, "source": newest_source}

    entry: dict | None = None
    error: str | None = None
    if measure:
        entry, error = score_by_measure(spec, tmpdir=tmpdir)
    else:
        entry, error = measure_report(spec, report_path)

    record: dict = {
        "unit": spec.unit,
        "unit_name": spec.unit_name,
        "tree": spec.tree,
        "report": {"path": report_path, "used": not measure,
                   "mtime": stamp_json(mtimes["report"]), "mtime_iso": stamp(mtimes["report"])},
        "object": {"path": spec.obj, "exists": obj_mtime is not None,
                   "mtime": stamp_json(obj_mtime), "mtime_iso": stamp(obj_mtime)},
        "sources": {"count": len(spec.sources),
                    "newest_path": newest_source[0] if newest_source else None,
                    "newest_mtime": stamp_json(newest_source[1]) if newest_source else None,
                    "newest_mtime_iso": stamp(newest_source[1]) if newest_source else None,
                    "paths": spec.sources},
        "mode": "measure" if measure else "report",
        "threshold": threshold,
        "rows": None,
        "row_count": 0,
        "shown": 0,
        "measures": {},
        "range": None,
        "refused": False,
        "error": error,
        "forced": bool(force),
    }
    # the guard is only meaningful when there is something to print: a missing report is an error, and an
    # error is exit 2 whatever the mtimes say.
    reasons = [] if error else freshness(not measure, mtimes["report"], obj_mtime, newest_source,
                                         report_path, spec.obj,
                                         rel=lambda p: _rel(p, spec.tree))
    record["freshness"] = {"stale": bool(reasons), "reasons": reasons}
    if error:
        record["summary"] = "cannot score %s: %s" % (spec.unit, error)
        return record

    rows = rows_of(entry)
    measures = measures_of(entry)
    shown_rows = below(rows, threshold)
    record.update({
        "rows": [{"name": r.name, "size": r.size, "address": r.address, "match_percent": r.percent,
                  "scored": r.scored, "matched": r.percent >= 100.0} for r in shown_rows],
        "row_count": len(rows),
        "shown": len(shown_rows),
        "measures": measures,
        "range": range_block(split_claims(spec.tree, spec.unit), rows),
        "refused": bool(reasons) and not force,
        "summary": summary_line(spec, measures, rows, len(shown_rows), threshold),
    })
    if record["refused"]:
        record["rows"] = None
        record["shown"] = 0
    return record


def render(record: dict) -> None:
    """The human report: the three mtimes and the verdict first, then the table, then the summary."""
    tree = record["tree"]
    rel = lambda p: (os.path.relpath(p, tree).replace("\\", "/") if p else "-")  # noqa: E731

    def row(label: str, path: str | None, when: str, extra: str = "") -> None:
        print("%-10s %-46s %s%s" % (label, rel(path) if path else "-", when, extra))

    print("== %s  (report unit %s, mode %s)" % (record["unit"], record["unit_name"], record["mode"]))
    if record["report"]["used"]:
        row("report", record["report"]["path"], record["report"]["mtime_iso"])
    else:
        print("%-10s %s" % ("report", "-  (not read: --measure scores the objects on disk)"))
    row("object", record["object"]["path"], record["object"]["mtime_iso"])
    row("source", record["sources"]["newest_path"], record["sources"]["newest_mtime_iso"],
        "   (+%d file(s) in the include closure)" % max(0, record["sources"]["count"] - 1))
    fresh = record["freshness"]
    if fresh["stale"]:
        print("freshness  STALE%s" % (" (forced)" if record["forced"] else ""))
        for reason in fresh["reasons"]:
            print("             - " + reason)
    elif record["mode"] == "measure":
        print("freshness  current - the object is newer than every source under the unit")
    else:
        print("freshness  current - the report is newer than the object and every source under the unit")
    if record["error"]:
        print("error      %s" % record["error"])
        print("hint       `python tools/units/measure.py %s` compiles the unit and scores every symbol "
              "in one report." % record["unit"])
        return
    if record["range"]:
        rng = record["range"]
        note = ("rows tile the range" if rng["ok"] else "; ".join(rng["warnings"]))
        print("range      %s 0x%08X-0x%08X (%d B) - %s" % (rng["section"], rng["start"], rng["end"],
                                                           rng["size"], note))
    if record["refused"]:
        print("refused    a stale report would print numbers that are not this build's; nothing shown.")
        print("           --force-stale scores it anyway (the verdict stays in the output), or")
        print("           --measure scores the objects on disk with one `objdiff report generate`.")
        return
    threshold = record["threshold"]
    if threshold is not None:
        print("filter     %d of %d row(s) below %g %%" % (record["shown"], record["row_count"], threshold))
    print("%-50s %8s %11s  %-10s %s" % ("symbol", "size B", "match %", "address", ""))
    for r in record["rows"] or []:
        flag = "" if r["matched"] else "  <- open"
        addr = "0x%08X" % r["address"] if r["address"] is not None else "-"
        print("%-50s %8d %11.5f  %-10s%s" % (r["name"], r["size"], r["match_percent"], addr, flag))
    print(record["summary"])


def cli(argv: list[str] | None = None) -> int:
    """Parse the command line, run the score, print it, and return the exit status."""
    ap = argparse.ArgumentParser(
        prog="unitscore.py",
        description="Every symbol of one unit from one report read - refusing a stale report by default.")
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. quest/arenatask")
    ap.add_argument("--report", help="the objdiff report to read (default build/RMHE08/report.json)")
    ap.add_argument("--threshold", type=float,
                    help="print only the rows strictly below this percentage")
    ap.add_argument("--json", action="store_true", help="the whole record, mtimes and verdict included")
    ap.add_argument("--measure", action="store_true",
                    help="score the objects on disk with one `objdiff report generate` instead of "
                         "reading the project report")
    ap.add_argument("--force-stale", action="store_true",
                    help="print the numbers even when the report (or object) is older than the unit's "
                         "sources - the STALE verdict stays in the output")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        from tools.objdiff import unitscore_selftest
        return unitscore_selftest.selftest()
    if not args.unit:
        ap.error("a unit is required (or --selftest)")

    try:
        spec = spec_of(args.unit, report=args.report)
    except SystemExit as exc:
        print(str(exc), file=sys.stderr)
        return 2
    record = run(spec, report=args.report, threshold=args.threshold, force=args.force_stale,
                 measure=args.measure)
    if args.json:
        print(json.dumps(record, indent=2))
    else:
        render(record)
    if record["error"]:
        return 2
    return 1 if record["refused"] else 0


def main() -> int:
    return cli()


if __name__ == "__main__":
    sys.exit(main())
