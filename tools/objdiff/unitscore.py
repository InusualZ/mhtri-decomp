#!/usr/bin/env python3
"""Every symbol of one unit, from **one** report read - and a freshness verdict that refuses a stale one.

    python tools/objdiff/unitscore.py <unit>                    # every symbol, worst first (0 objdiff calls)
    python tools/objdiff/unitscore.py <unit> --measure          # score the objects on disk (exactly 1 call)
    python tools/objdiff/unitscore.py <unit> --threshold 99.9   # only the rows below the threshold
    python tools/objdiff/unitscore.py <unit> --json             # the whole record, mtimes included
    python tools/objdiff/unitscore.py <unit> --force-stale      # score anyway, STALE stays in the output
    python tools/objdiff/unitscore.py --selftest

`<unit>` is the path from the repository root (`quest/arenatask`, `Pl/pl_act`); the extension may be
omitted, and `src/`/`main/`/`build/RMHE08/...` prefixes are accepted.

**Why this tool exists.** Scoring a unit one symbol at a time costs one objdiff invocation per symbol, and
a 70-row unit is 70 of them; lanes kept hand-writing the same driver (`build/tmp/unitreport.py`,
`build/scratch/score.py`, `.pi/scratch/score.py`) and the re-inventions produced *wrong* numbers - one
scored a stale object twice and reported two false improvements. A report over the tree already carries
every symbol of every unit; the only thing missing was a reader that does not lie about its freshness.

**The hazard this is built around.** `build/RMHE08/report.json` is an **order-only** target of `all_source`
in `build.ninja`: after a source edit, `ninja build/RMHE08/report.json` prints "no work to do" and the file
still holds the PREVIOUS build's scores. That cost one lane three iterations that looked like "all new
functions score 0 %". So every run measures and prints three mtimes - the report's, the unit's object's and
the newest source under the unit - and **refuses to print numbers** (exit 1) when the report predates
either of the other two. `--force-stale` overrides the refusal; the STALE verdict and its reasons stay in
the output and in the JSON (`"freshness"`), so a forced run is never mistakable for a clean one.

`--measure` is the escape hatch for exactly that case: instead of the project report it scores the unit's
already-built object pair with **one** `objdiff report generate` (the same primitive `symdiff.py -u <unit>`
and `measure.py` use, ~0.2 s for a 19-symbol unit). The guard then applies to the object: a source newer
than the object means the object was never rebuilt, and that is refused the same way (that is the incident
above, seen from the other side). Neither mode ever issues N calls: report mode issues zero.

**Reuse, not re-implementation.** The unit and its paths come from `tools/unitutil.py` (`resolve_unit`,
`ROOT`); the report entry's identity and scoring conventions come from `tools/units/verifyunit.py`
(`report_unit`, `unit_stem`, and its "a function with no `fuzzy_match_percent` key is 0 %, not 100 %"
reading); the one-report score comes from `unitutil.report_functions` through `tools/objdiff/symdiff.py`'s
scratch/retry helpers. There is one implementation of each, and this file holds none of them.

**Exit status is the answer**: 0 the numbers are printable (current, or explicitly forced), 1 the
freshness guard refused, 2 a usage or input error (no report, an unreadable report, the unit missing from
it) - a traceback is never the answer.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import time
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))                 # tools/objdiff
TOOLS = os.path.dirname(HERE)                                     # tools/
for _path in (TOOLS, os.path.join(TOOLS, "units"), HERE):
    if _path not in sys.path:
        sys.path.insert(0, _path)

import unitutil as uu                                             # noqa: E402
import symdiff                                                    # noqa: E402
from units import verifyunit as vu                                # noqa: E402

REPORT_REL = os.path.join("build", "RMHE08", "report.json")
# `\t.text       start:0x804459E4 end:0x80448404` - the splits.txt claim shape (land.py/flipcheck.py's).
SPLIT_ROW_RE = re.compile(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)")
# a one-line `#include "x.h"` / `#include <x.h>`; comments are stripped first so a commented-out one is not
# counted as a dependency.
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
MAX_INCLUDE_DEPTH = 12


# --------------------------------------------------------------------------------------------------
# mtimes and the freshness verdict
# --------------------------------------------------------------------------------------------------

def mtime(path: str) -> float | None:
    """The file's mtime, or None when it does not exist (never an exception)."""
    try:
        return os.path.getmtime(path)
    except OSError:
        return None


def stamp(t: float | None) -> str:
    """A local timestamp for a printed report, `-` for a missing file."""
    return "-" if t is None else time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(t))


def stamp_json(t: float | None):
    """The same instant machine-readably: seconds since the epoch, or None."""
    return None if t is None else round(t, 3)


def freshness(use_report: bool, report_mtime: float | None, object_mtime: float | None,
              newest_source: tuple[str, float] | None, report_path: str, object_path: str,
              rel=lambda p: p) -> list[str]:
    """The reasons the numbers would be stale, worst first - `[]` means the run is current.

    Strict `<` on purpose: report.json is written in the same whole second as the last object ninja
    rebuilt (measured: report 07:21:39 / newest object 07:21:39 in a settled tree), so an equal stamp is
    current and a report one second older than any input is not. `rel` shortens the paths for the printed
    reason; the JSON carries the absolute ones in its own fields.
    """
    reasons: list[str] = []
    if object_mtime is None:
        reasons.append("the unit's object does not exist (%s): nothing was built for it in this tree, so "
                       "the report's rows cannot be this tree's" % rel(object_path))
    if use_report:
        if report_mtime is None:
            reasons.append("the report does not exist (%s)" % rel(report_path))
        else:
            if object_mtime is not None and report_mtime < object_mtime:
                reasons.append(
                    "the report (%s, %s) predates the unit's object (%s, %s) - it was written before the "
                    "last build of this unit" % (rel(report_path), stamp(report_mtime), rel(object_path),
                                                 stamp(object_mtime)))
            if newest_source is not None and report_mtime < newest_source[1]:
                reasons.append(
                    "the report (%s, %s) predates the newest source under the unit (%s, %s) - it holds "
                    "the PREVIOUS build's scores (report.json is an order-only target of all_source)"
                    % (rel(report_path), stamp(report_mtime), rel(newest_source[0]),
                       stamp(newest_source[1])))
    else:
        if object_mtime is not None and newest_source is not None and object_mtime < newest_source[1]:
            reasons.append(
                "the object (%s, %s) predates the newest source under the unit (%s, %s) - it was not "
                "rebuilt after the edit" % (rel(object_path), stamp(object_mtime), rel(newest_source[0]),
                                            stamp(newest_source[1])))
    return reasons


# --------------------------------------------------------------------------------------------------
# the unit's sources: the file plus its include closure, so a header edit dates the object too
# --------------------------------------------------------------------------------------------------

def includes_of(path: str) -> list[str]:
    """The `#include` targets named in a source file, comments removed so a commented one is not read."""
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return []
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return INCLUDE_RE.findall(text)


def resolve_include(name: str, from_dir: str, root: str) -> str | None:
    """Where MWCC would find `name` from `from_dir`: beside the includer, then `include/`, then `src/`.

    Only paths inside `root` are returned: a system header (`<string.h>`) is not a source under the
    unit and must not date it.
    """
    name = name.replace("\\", "/")
    for base in (from_dir, os.path.join(root, "include"), os.path.join(root, "src")):
        cand = os.path.normpath(os.path.join(base, *name.split("/")))
        if os.path.isfile(cand) and _inside(cand, root):
            return cand
    return None


def _rel(path: str, tree: str) -> str:
    """A path as it is printed: relative to the tree, with forward slashes on every host."""
    try:
        out = os.path.relpath(path, tree)
    except ValueError:                                 # different drives
        out = path
    return out.replace("\\", "/")


def _inside(path: str, root: str) -> bool:
    """`/a/b` inside `/a` - the check `os.path.commonpath` needs to be asked, not assumed."""
    try:
        return os.path.commonpath([os.path.normcase(os.path.abspath(path)),
                                   os.path.normcase(os.path.abspath(root))]) == \
            os.path.normcase(os.path.abspath(root))
    except ValueError:                                    # different drives
        return False


def source_closure(src: str, root: str) -> list[str]:
    """The unit's source file and every in-tree header it reaches, transitively (deduplicated).

    A header is part of the unit's inputs - the object is rebuilt when it changes - so a stale verdict
    that ignored headers would miss "the lane edited `include/unsplit/Pl.h` and never ran ninja".
    """
    out: list[str] = []
    seen: set[str] = set()
    todo: list[tuple[str, int]] = [(os.path.abspath(src), 0)]
    while todo:
        path, depth = todo.pop(0)
        key = os.path.normcase(os.path.abspath(path))
        if key in seen:
            continue
        seen.add(key)
        out.append(path)
        if depth >= MAX_INCLUDE_DEPTH:
            continue
        for name in includes_of(path):
            hit = resolve_include(name, os.path.dirname(path), root)
            if hit and os.path.normcase(os.path.abspath(hit)) not in seen:
                todo.append((hit, depth + 1))
    return out


def newest(paths: list[str]) -> tuple[str, float] | None:
    """The `(path, mtime)` of the most recently modified of `paths`, or None for an empty list."""
    best: tuple[str, float] | None = None
    for p in paths:
        t = mtime(p)
        if t is None:
            continue
        if best is None or t > best[1]:
            best = (p, t)
    return best


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
    """Resolve a unit spec through `unitutil`, plus its include closure and the tree's report path.

    `tree` names the tree to read and is passed straight to `unitutil.resolve_unit(root=...)`, so the
    unit's own paths land under it - a fixture that is not a git worktree resolves there instead of
    silently reading `unitutil.ROOT` (the real tree).
    """
    root = tree or uu.ROOT
    unit = uu.resolve_unit(unit_spec, root=root)
    src = unit.src if os.path.isabs(unit.src) else os.path.join(root, unit.src)
    return Spec(unit=vu.unit_stem(unit.name), unit_name=unit.name, obj=unit.obj, target=unit.target,
                src=src, tree=root, report=os.path.abspath(report) if report else os.path.join(root, REPORT_REL),
                sources=source_closure(src, root))


def rows_of(entry: dict) -> list[Row]:
    """Every symbol the report entry carries, worst first (ties by name) - the one ordering this tool has.

    The percent is the report's own `fuzzy_match_percent`; a row the report left unscored (the key is
    absent) is **0 %, not 100 %** - `verifyunit._score`'s reading, reused here.
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
        pct = fn.get("fuzzy_match_percent")
        rows.append(Row(name=name, size=size, address=address,
                        percent=float(pct) if isinstance(pct, (int, float)) else 0.0,
                        scored=isinstance(pct, (int, float))))
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
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return {}
    out: dict[str, tuple[int, int]] = {}
    want = vu.unit_stem(unit)
    current: str | None = None
    for line in text.splitlines():
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":"):
            current = vu.unit_stem(line.strip()[:-1])
            continue
        if current != want:
            continue
        m = SPLIT_ROW_RE.match(line)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
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
    entry = vu.report_unit(data, spec.unit)
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

    This is `symdiff.py -u <unit>`'s primitive (`unitutil.report_functions` through symdiff's unique
    scratch directory and its transient-lock retry), so the numbers are the same code path as the project
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
    scratch = tmpdir or symdiff.session_tmpdir()
    try:
        entries = symdiff.retry_transient(
            lambda: uu.report_functions(spec.target, spec.obj, unit_name=spec.unit_name, tmpdir=scratch))
    except OSError as exc:
        return None, ("cannot run objdiff (%s): %s - `python tools/units/measure.py %s` finds the "
                      "tree's own binary" % (uu.OBJDIFF, exc, spec.unit))
    if "_error" in entries:
        return None, ("one `report generate` over %s / %s failed: %s"
                      % (spec.target, spec.obj, entries["_error"]))
    functions = [dict(e) for e in entries.values()]
    return {"name": spec.unit_name, "functions": functions,
            "measures": _measures_from_one_unit_report(scratch)}, None


def _measures_from_one_unit_report(tmpdir: str) -> dict:
    """The unit measures out of the one-unit report `unitutil.report_functions` just wrote.

    A file read, not an invocation: `report_functions` returns only the functions, and `--measure` is asked
    for the same `matched_functions`/`matched_code`/`total_code` the project report carries. The path is
    the one `unitutil` documents as its output (`<tmpdir>/unitutil_report.json`); an unreadable or
    unparsable file leaves the measures empty rather than failing a run that has its rows already.
    """
    try:
        data = json.load(open(os.path.join(tmpdir, "unitutil_report.json"), encoding="utf-8"))
        return ((data.get("units") or [{}])[0] or {}).get("measures") or {}
    except (OSError, ValueError, IndexError):
        return {}


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
        import unitscore_selftest
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
