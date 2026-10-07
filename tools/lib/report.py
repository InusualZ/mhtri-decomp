"""Scores: the objdiff report, the one metric, freshness, and one regression rule.
Spec: docs/tools/spec/lib-report.md. CLI: none (library)."""
from __future__ import annotations

import json
import os
import re
import subprocess
import time
from dataclasses import dataclass, field
from typing import Any, Callable

from tools.lib import cscan
from tools.lib.repo import header_root

#: The per-function score key of report version 2; a function entry **without** it scores 0 %, not 100 %.
SCORE_KEY = "fuzzy_match_percent"
#: The oldest objdiff that reads the one-unit project this module writes.
MIN_PROJECT_VERSION = "2.0.0-beta.5"
#: The one-unit project directory and its report, inside the caller's scratch directory (callers read the
#: report back by this name).
PROJECT_DIR = "unitutil_project"
REPORT_FILE = "unitutil_report.json"
#: objdiff-cli under a tree.
OBJDIFF_REL = os.path.join("build", "tools", "objdiff-cli.exe")
#: The smallest score movement that counts as a move.
DEFAULT_EPS = 1e-9
#: The tolerance of `arithmetic_check`: a float unit percent reproduces up to its report rounding.
ARITH_TOL = 0.01

#: The report `measures` keys in reading order: the score, code, data, functions, "complete", unit counts.
MEASURE_ORDER = (
    "fuzzy_match_percent",
    "matched_code", "matched_code_percent", "total_code",
    "matched_data", "matched_data_percent", "total_data",
    "matched_functions", "matched_functions_percent", "total_functions",
    "complete_code", "complete_code_percent",
    "complete_data", "complete_data_percent",
    "complete_units", "total_units",
)
#: The measures whose fall is a regression at project scope (a `total_*` moving is a re-tiling).
REGRESSION_KEYS = frozenset((
    "fuzzy_match_percent", "matched_code", "matched_code_percent",
    "matched_data", "matched_data_percent",
    "matched_functions", "matched_functions_percent",
    "complete_code", "complete_code_percent",
    "complete_data", "complete_data_percent",
    "complete_units",
))


class ReportError(Exception):
    """A path or a run that cannot serve as a report (missing, unreadable, not a report, objdiff failed)."""


# --------------------------------------------------------------------------------------------------
# numbers and the 0 % rule
# --------------------------------------------------------------------------------------------------

def num(value: Any) -> int | float | None:
    """A measure as a number, or None: the report stores counts as strings and percents as floats."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, (int, float)):
        return value
    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None
        try:
            return int(text)
        except ValueError:
            try:
                return float(text)
            except ValueError:
                return None
    return None


def is_scored(entry: dict | None) -> bool:
    """Whether a function entry carries a numeric score at all."""
    return bool(entry) and isinstance(entry.get(SCORE_KEY), (int, float)) and not isinstance(entry.get(SCORE_KEY), bool)


def score_of(entry: dict | None) -> float:
    """A function entry's score with the campaign's rule: no numeric `fuzzy_match_percent` is 0.0, never 100."""
    return float(entry[SCORE_KEY]) if is_scored(entry) else 0.0


def entry_score(entry: dict | None) -> float | None:
    """The score when the entry carries one, else None ("unscored" kept distinct from 0 for display)."""
    return float(entry[SCORE_KEY]) if is_scored(entry) else None


def arithmetic_check(measures: dict, entries: dict[str, dict] | list[dict],
                     tol: float = ARITH_TOL) -> tuple[bool, str]:
    """`sum(size * score / 100) / total_code == fuzzy_match_percent` with the 0 % rule.

    With an absent key read as 0 the identity holds; read as 100 it does not, so a mismatch means one of
    the two readings is wrong. A unit without `total_code` or a numeric percent passes with the reason.
    """
    try:
        total = int(measures.get("total_code"))
    except (TypeError, ValueError, AttributeError):
        return True, "no total_code to check"
    reported = measures.get(SCORE_KEY)
    if not isinstance(reported, (int, float)):
        return True, "no unit fuzzy_match_percent to check"
    matched = 0.0
    for fn in (entries.values() if isinstance(entries, dict) else entries):
        try:
            size = int(fn.get("size"))
        except (TypeError, ValueError):
            continue
        matched += size * score_of(fn) / 100.0
    computed = (100.0 * matched / total) if total else 0.0
    if abs(computed - reported) <= tol:
        return True, "sum(check) %.5f == report %.5f" % (computed, reported)
    return False, ("per-symbol sum gives %.5f but the unit reports %.5f - a function with no "
                   "fuzzy_match_percent key reads as 0%%, not 100%%" % (computed, reported))


# --------------------------------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Report:
    """A parsed objdiff report (`report generate`'s JSON); `path` is where it was read from, if anywhere."""
    data: dict = field(default_factory=dict)
    path: str | None = None

    @classmethod
    def load(cls, path: str | os.PathLike) -> "Report":
        """Read one report; `ReportError` for a missing, unreadable or non-report file."""
        path = os.fspath(path)
        if not os.path.exists(path):
            raise ReportError("%s does not exist" % path)
        try:
            with open(path, "r", encoding="utf-8") as fh:
                data = json.load(fh)
        except (OSError, ValueError) as exc:
            raise ReportError("%s is not readable JSON (%s)" % (path, exc))
        if not isinstance(data, dict) or ("units" not in data and "measures" not in data):
            raise ReportError("%s is not a report (no `units` and no `measures`)" % path)
        return cls(data, path)

    @classmethod
    def coerce(cls, report: "Report | dict | None") -> "Report":
        """A `Report` from a `Report`, a raw report dict, or None (the empty report)."""
        if isinstance(report, Report):
            return report
        return cls(report or {})

    def units(self) -> list[dict]:
        """The unit rows that carry a name."""
        return [u for u in (self.data.get("units") or []) if u.get("name")]

    def unit(self, name: str) -> dict | None:
        """The unit row named `name` exactly, or None."""
        return next((u for u in self.units() if u.get("name") == name), None)

    def entries(self, name: str) -> dict[str, dict]:
        """`{function name: entry}` of a unit (empty for an unknown unit)."""
        return {f["name"]: f for f in ((self.unit(name) or {}).get("functions") or []) if f.get("name")}

    def functions(self, name: str) -> dict[str, float]:
        """`{function name: score}` of a unit, with the 0 % rule."""
        return {n: score_of(e) for n, e in self.entries(name).items()}

    def scores(self) -> dict[str, dict[str, float]]:
        """`{unit: {function: score}}` for every unit, with the 0 % rule."""
        return {u["name"]: {f["name"]: score_of(f) for f in (u.get("functions") or ()) if f.get("name")}
                for u in self.units()}

    def measures(self, name: str | None = None) -> dict:
        """A unit's `measures` (the project's own with no name)."""
        if name is None:
            return self.data.get("measures") or {}
        return (self.unit(name) or {}).get("measures") or {}

    def unit_measures(self) -> dict[str, dict]:
        """`{unit name: measures}` - the unit-level score rows."""
        return {u["name"]: u.get("measures") or {} for u in self.units()}

    def symbol_measures(self) -> dict[tuple[str, str], float]:
        """`{(unit, symbol): score}` - the symbol-level score rows, with the 0 % rule."""
        out: dict[tuple[str, str], float] = {}
        for u in self.units():
            for f in u.get("functions") or []:
                if f.get("name"):
                    out[(u["name"], f["name"])] = score_of(f)
        return out

    def denominators(self) -> dict[str, dict]:
        """`{scope: {measure: number}}` for the project total and every category."""
        out: dict[str, dict] = {"project": {k: num(v) for k, v in (self.data.get("measures") or {}).items()}}
        for category in self.data.get("categories") or []:
            scope = category.get("id") or category.get("name")
            if scope:
                out[str(scope)] = {k: num(v) for k, v in (category.get("measures") or {}).items()}
        return out

    def arithmetic_check(self, name: str, tol: float = ARITH_TOL) -> tuple[bool, str]:
        """`arithmetic_check` over one unit's measures and functions."""
        return arithmetic_check(self.measures(name), self.entries(name), tol)


def read(path: str | os.PathLike, default: dict | None = None) -> Report:
    """A report from `path`, or `Report(default)` when the file does not exist (a bad file still raises)."""
    path = os.fspath(path)
    if not os.path.exists(path):
        return Report(default if default is not None else {"units": [], "measures": {}}, None)
    with open(path, "r", encoding="utf-8") as fh:
        return Report(json.load(fh), path)


# --------------------------------------------------------------------------------------------------
# comparisons: moved rows, and the one regression rule
# --------------------------------------------------------------------------------------------------

def moved(before: dict, after: dict, eps: float = DEFAULT_EPS) -> list[dict]:
    """`[{key, before, after, delta}]` for every key both maps score (no None) whose score moved by more than
    `eps`, in key order - the one moved-row rule `diff_*`, `compare` and `measure` read."""
    rows = []
    for key in sorted(set(before) & set(after)):
        a, b = before[key], after[key]
        if a is None or b is None:
            continue
        delta = b - a
        if abs(delta) > eps:
            rows.append({"key": key, "before": a, "after": b, "delta": delta})
    return rows


_moved = moved


def direction(rows: list[dict]) -> dict:
    """`{moved, up, down}` of moved rows (a row with `delta` < 0 is down - a fall)."""
    return {"moved": len(rows), "up": sum(1 for r in rows if r["delta"] > 0),
            "down": sum(1 for r in rows if r["delta"] < 0)}


def drops(rows: list[dict], eps: float = DEFAULT_EPS) -> list[dict]:
    """The moved rows that fell by more than `eps`: every one is a drop, whatever the aggregate did."""
    return [r for r in rows if r["delta"] < -eps]


def diff_units(before: "Report | dict", after: "Report | dict", eps: float = DEFAULT_EPS) -> dict:
    """Unit rows whose `fuzzy_match_percent` moved (worst first), plus the units added and removed."""
    ub, ua = Report.coerce(before).unit_measures(), Report.coerce(after).unit_measures()
    moved = [{"unit": r["key"], "before": r["before"], "after": r["after"], "delta": r["delta"]}
             for r in _moved({k: num(v.get(SCORE_KEY)) for k, v in ub.items()},
                             {k: num(v.get(SCORE_KEY)) for k, v in ua.items()}, eps)]
    moved.sort(key=lambda r: (r["delta"], r["unit"]))
    return {"moved": moved, "added": sorted(set(ua) - set(ub)), "removed": sorted(set(ub) - set(ua))}


def diff_symbols(before: "Report | dict", after: "Report | dict", eps: float = DEFAULT_EPS) -> dict:
    """Symbol rows whose score moved (worst first, the 0 % rule), plus the symbols added and removed."""
    sb, sa = Report.coerce(before).symbol_measures(), Report.coerce(after).symbol_measures()
    moved = []
    for r in _moved(sb, sa, eps):
        unit, symbol = r["key"]
        moved.append({"unit": unit, "symbol": symbol, "before": r["before"], "after": r["after"],
                      "delta": r["delta"]})
    moved.sort(key=lambda r: (r["delta"], r["unit"], r["symbol"]))
    return {"moved": moved, "added": sorted(set(sa) - set(sb)), "removed": sorted(set(sb) - set(sa))}


def address_rows(report: "Report | dict", unit: str | None = None) -> list[dict]:
    """`[{unit, name, address, size, score}]` for every function row (of `unit` when given), with the 0 % rule.

    `address` is the row's `metadata.virtual_address` (the DOL address, which a rename or a re-home keeps), or
    None when the report carries none."""
    rows = []
    for u in Report.coerce(report).units():
        if unit is not None and u["name"] != unit:
            continue
        for f in u.get("functions") or []:
            if not f.get("name"):
                continue
            addr = num((f.get("metadata") or {}).get("virtual_address"))
            rows.append({"unit": u["name"], "name": f["name"], "address": None if addr is None else int(addr),
                         "size": int(num(f.get("size")) or 0), "score": score_of(f)})
    return rows


def address_key(row: dict) -> tuple:
    """The pairing key of an `address_rows` row: its address, or `(unit, name)` for a row with none."""
    return ("a", row["address"]) if row["address"] is not None else ("n", row["unit"], row["name"])


def diff_by_address(before: list[dict], after: list[dict], eps: float = DEFAULT_EPS) -> dict:
    """Two `address_rows` lists paired by address: `{up, down, new, removed, renamed, paired}`.

    A row renamed or moved to another unit between the two still pairs (the address is the identity); `up`/`down`
    are the moved rows (`moved`'s rule) with both names, `new`/`removed` the rows only one side has, `renamed` the
    paired rows whose name or unit changed. The first row of a repeated key wins."""
    bk: dict = {}
    ak: dict = {}
    for r in before:
        bk.setdefault(address_key(r), r)
    for r in after:
        ak.setdefault(address_key(r), r)
    out: dict = {"up": [], "down": [], "new": [], "removed": [], "renamed": [], "paired": len(set(bk) & set(ak))}
    for m in moved({k: r["score"] for k, r in bk.items()}, {k: r["score"] for k, r in ak.items()}, eps):
        b, a = bk[m["key"]], ak[m["key"]]
        row = {"unit": a["unit"], "name": a["name"], "address": a["address"], "size": a["size"],
               "before_unit": b["unit"], "before_name": b["name"],
               "before": m["before"], "after": m["after"], "delta": m["delta"]}
        out["up" if m["delta"] > 0 else "down"].append(row)
    out["down"].sort(key=lambda r: (r["delta"], r["unit"], r["name"]))
    out["up"].sort(key=lambda r: (-r["delta"], r["unit"], r["name"]))
    out["new"] = sorted((ak[k] for k in set(ak) - set(bk)), key=lambda r: (r["unit"], r["name"]))
    out["removed"] = sorted((bk[k] for k in set(bk) - set(ak)), key=lambda r: (r["unit"], r["name"]))
    out["renamed"] = sorted(({"address": ak[k]["address"], "before_unit": bk[k]["unit"], "before_name": bk[k]["name"],
                              "unit": ak[k]["unit"], "name": ak[k]["name"]}
                             for k in set(ak) & set(bk)
                             if (ak[k]["unit"], ak[k]["name"]) != (bk[k]["unit"], bk[k]["name"])),
                            key=lambda r: (r["unit"], r["name"]))
    return out


def diff_denominators(before: "Report | dict", after: "Report | dict", eps: float = DEFAULT_EPS) -> list[dict]:
    """The project total and every category, measure by measure, with the delta and a regression flag."""
    db, da = Report.coerce(before).denominators(), Report.coerce(after).denominators()
    rows = []
    for scope in ["project"] + sorted((set(db) | set(da)) - {"project"}):
        mb, ma = db.get(scope, {}), da.get(scope, {})
        keys = [k for k in MEASURE_ORDER if k in mb or k in ma]
        keys += sorted((set(mb) | set(ma)) - set(MEASURE_ORDER))
        for key in keys:
            a, b = mb.get(key), ma.get(key)
            delta = None if (a is None or b is None) else b - a
            fell = delta is not None and delta < -eps
            rows.append({"scope": scope, "metric": key, "before": a, "after": b, "delta": delta,
                         "moved": delta is not None and abs(delta) > eps, "fell": fell,
                         "regressed": bool(fell and scope == "project" and key in REGRESSION_KEYS)})
    return rows


def compare(before: "Report | dict", after: "Report | dict", eps: float = DEFAULT_EPS,
            before_path: str = "?", after_path: str = "?") -> dict:
    """Two reports compared row for row, with the "every moved row that fell" verdict as `exit`.

    A unit or symbol row that dropped, or a project numerator (`REGRESSION_KEYS`) that fell, is a drop even
    when the aggregate rose; `exit` is 1 on a drop, 0 when comparable and clean, 2 when the pair shares no
    unit and no project measure. JSON-safe.
    """
    units = diff_units(before, after, eps)
    symbols = diff_symbols(before, after, eps)
    dens = diff_denominators(before, after, eps)
    found = ([{"kind": "unit", "name": r["unit"], "before": r["before"], "after": r["after"],
               "delta": r["delta"]} for r in drops(units["moved"], eps)]
             + [{"kind": "symbol", "name": "%s/%s" % (r["unit"], r["symbol"]), "before": r["before"],
                 "after": r["after"], "delta": r["delta"]} for r in drops(symbols["moved"], eps)]
             + [{"kind": "denominator", "name": "%s/%s" % (r["scope"], r["metric"]),
                 "before": r["before"], "after": r["after"], "delta": r["delta"]}
                for r in dens if r["regressed"]])
    rb, ra = Report.coerce(before), Report.coerce(after)
    common_units = len(set(rb.unit_measures()) & set(ra.unit_measures()))
    shared_measures = len(set(rb.denominators().get("project", {})) & set(ra.denominators().get("project", {})))
    comparable = common_units or shared_measures
    return {
        "before": before_path, "after": after_path, "eps": eps,
        "units": units, "symbols": symbols, "denominators": dens,
        "drops": found, "regressed": bool(found),
        "comparable": {"common_units": common_units, "shared_measures": shared_measures,
                       "ok": bool(comparable)},
        "exit": 1 if found else (0 if comparable else 2),
    }


def snapshot(report: "Report | dict | None") -> dict:
    """`{unit: {"fuzzy", "matched_code", "total_code", "symbols": {name: score}, "addrs": {name: address}, "all": True}}`
    - what a batch is judged against (`addrs` and `total_code` let `seam_exempt` pair functions across a re-home).

    Every function is listed under the 0 % rule (an unscored one at 0.0), so a symbol at 100 % that falls - or loses
    its score - is visible to `regression`; `"all"` marks the row (a snapshot written before WP4 listed only the
    sub-100 % symbols and has no marker). An older report's `match_percent` is read when `fuzzy_match_percent` is
    absent.
    """
    out = {}
    for unit in Report.coerce(report).units():
        measures = unit.get("measures") or {}
        symbols, addrs = {}, {}
        for fn in unit.get("functions") or []:
            pct = fn.get(SCORE_KEY, fn.get("match_percent"))
            pct = float(pct) if isinstance(pct, (int, float)) and not isinstance(pct, bool) else 0.0
            if fn.get("name"):
                symbols[fn["name"]] = round(pct, 4)
                addr = num((fn.get("metadata") or {}).get("virtual_address"))
                if addr is not None:
                    addrs[fn["name"]] = int(addr)
        if measures or symbols:
            out[unit["name"]] = {"fuzzy": measures.get(SCORE_KEY), "matched_code": measures.get("matched_code"),
                                 "total_code": measures.get("total_code"), "symbols": symbols, "addrs": addrs,
                                 "all": True}
    return out


def _below_100(row: dict) -> dict:
    """A snapshot row's sub-100 % symbols - the set `unit_grew` reads for either snapshot format."""
    return {s: v for s, v in (row.get("symbols") or {}).items() if not isinstance(v, (int, float)) or v < 100.0}


def unit_grew(prior: dict, after: dict, eps: float = DEFAULT_EPS) -> bool:
    """A snapshot row only gained: a sub-100 % symbol it did not hold, or more matched bytes.

    The symbol half reads the sub-100 % symbols of either format (a full snapshot's 100 % rows would count a rename
    as growth); the bytes half reads `matched_code` as a number (the report stores it as a string)."""
    prior_syms, after_syms = _below_100(prior), _below_100(after)
    if len(after_syms) > len(prior_syms) or any(s not in prior_syms for s in after_syms):
        return True
    bm, am = num(prior.get("matched_code")), num(after.get("matched_code"))
    return bm is not None and am is not None and am > bm + eps


def regression(before: dict, after: dict, allow: list[str] | tuple = (),
               eps: float = DEFAULT_EPS) -> tuple[list[tuple], list[tuple]]:
    """The one regression rule over two `snapshot`s -> (unauthorised, authorised) as (unit, what, before, after).

    A symbol the previous snapshot held whose score fell is a drop, whatever the aggregate did - from 100 % too, and
    to 0.0 when it lost its score (a full snapshot holds every symbol); a symbol it did not hold is new, never a
    drop; a symbol gone from `after` is not judged by name (it reached 100 % under a pre-WP4 snapshot, or it was
    renamed or moved). The unit average speaks only when no symbol does and the unit did not grow (`unit_grew`).
    `auto_*` scaffold units (outside `/auto/`) are bookkeeping. `allow` names units whose drops an explicit rule
    authorised (substring match).
    """
    unauthorised, authorised = [], []
    for unit, after_vals in after.items():
        prior = before.get(unit)
        if not prior or "auto_" in unit and "/auto/" not in unit:
            continue
        rows = []
        for sym, bpct in (prior.get("symbols") or {}).items():
            apct = (after_vals.get("symbols") or {}).get(sym)
            if apct is None:
                continue
            if isinstance(apct, (int, float)) and apct < bpct - eps:
                rows.append((unit, sym, bpct, apct))
        if not rows and not unit_grew(prior, after_vals, eps):
            bf, af = prior.get("fuzzy"), after_vals.get("fuzzy")
            if isinstance(bf, (int, float)) and isinstance(af, (int, float)) and af < bf - eps:
                rows.append((unit, "unit fuzzy", bf, af))
        (authorised if any(a in unit for a in allow) else unauthorised).extend(rows)
    return unauthorised, authorised


def whole_fuzzy(snap: dict) -> float | None:
    """The code-weighted fuzzy of a `snapshot` (its units' `total_code` as weights); None when it holds no weight."""
    total = score = 0.0
    for row in snap.values():
        w, f = num(row.get("total_code")), row.get("fuzzy")
        if w and isinstance(f, (int, float)):
            total, score = total + w, score + w * f
    return score / total if total else None


def seam_exempt(before: dict, after: dict, moves: list[tuple[str, str, int, int]], touched: set[str],
                unauthorised: list[tuple], eps: float = DEFAULT_EPS) -> tuple[list[tuple], list[dict], str]:
    """-> (still unauthorised, seam moves, why not) for a batch whose `splits.txt` moved `.text` between units.

    `moves` are `(from_unit, to_unit, start, end)` over snapshot unit names, `touched` every unit the splits diff
    changed. A *unit average* row of a unit a move names is a pure seam effect, and is lifted, only when ALL hold:
    (1) every function present in both snapshots, paired by address, scores at least what it did; (2) every
    function that left a unit lies inside a move *from* that unit and is held by the move's target afterwards, and
    every function a unit gained from another lies inside a move to it; (3) the matched code summed over the
    touched units and the whole-report fuzzy did not fall; (4) a lifted unit really lost or gained a function. A
    per-symbol row is never lifted. Either snapshot lacking addresses (taken before they were recorded) lifts
    nothing: `why not` says so. Each seam move is `{"from", "to", "functions"}`."""
    avg = [r for r in unauthorised if r[1] == "unit fuzzy"]
    if not moves or not avg:
        return list(unauthorised), [], ""
    names = {u for m in moves for u in m[:2]}

    def addrs(snap: dict, unit: str) -> dict[int, float] | None:
        row = snap.get(unit) or {}
        a, sym = row.get("addrs"), row.get("symbols") or {}
        if a is None:
            return None if row else {}
        return {int(addr): sym.get(name, 0.0) for name, addr in a.items()}
    old = {u: addrs(before, u) for u in touched | names}
    new = {u: addrs(after, u) for u in touched | names}
    if any(v is None for v in (*old.values(), *new.values())):
        return list(unauthorised), [], "a snapshot carries no function addresses (re-record the base)"
    every_old = {a: (u, p) for u, fn in ((u, addrs(before, u)) for u in before) if fn for a, p in fn.items()}
    every_new = {a: (u, p) for u, fn in ((u, addrs(after, u)) for u in after) if fn for a, p in fn.items()}
    worse = [a for a, (_u, p) in every_old.items() if a in every_new and every_new[a][1] < p - eps]
    if worse:
        return list(unauthorised), [], "function at 0x%X got worse" % worse[0]
    inside = lambda a, lo, hi: lo <= a < hi
    gone = [a for a in every_old if a not in every_new]
    if gone:
        return list(unauthorised), [], "function at 0x%X is in no unit after the batch" % gone[0]
    moved: dict[tuple[str, str], set[int]] = {}
    for a, (u_old, _p) in every_old.items():
        u_new = every_new[a][0]
        if u_new == u_old:
            continue
        hit = next((m for m in moves if m[0] == u_old and m[1] == u_new and inside(a, m[2], m[3])), None)
        if hit is None:
            return list(unauthorised), [], "function at 0x%X moved %s -> %s outside the splits diff" % (a, u_old, u_new)
        moved.setdefault((u_old, u_new), set()).add(a)
    sums = [sum(num((snap.get(u) or {}).get("matched_code")) or 0 for u in touched | names) for snap in (before, after)]
    if sums[1] < sums[0] - eps:
        return list(unauthorised), [], "matched code of the touched units fell %d -> %d" % tuple(sums)
    wb, wa = whole_fuzzy(before), whole_fuzzy(after)
    if wb is not None and wa is not None and wa < wb - 1e-3:
        return list(unauthorised), [], "whole-report fuzzy fell %.4f -> %.4f" % (wb, wa)
    involved = {u for pair in moved for u in pair}
    lifted = [r for r in avg if r[0] in involved]
    rest = [r for r in unauthorised if r not in lifted]
    seams = [{"from": a, "to": b, "functions": len(fns)} for (a, b), fns in sorted(moved.items())]
    return rest, (seams if lifted else []), ""


# --------------------------------------------------------------------------------------------------
# scoring: `objdiff report generate` on a one-unit project, and the diagnostic diff rows
# --------------------------------------------------------------------------------------------------

def objdiff_cli(root: str | os.PathLike, main: str | os.PathLike | None = None) -> str:
    """objdiff-cli of `root`, else of `main`; `root`'s path when neither has it (so the error names it)."""
    for tree in (root, main):
        if tree:
            cand = os.path.join(os.fspath(tree), OBJDIFF_REL)
            if os.path.exists(cand):
                return cand
    return os.path.join(os.fspath(root), OBJDIFF_REL)


def write_project(target: str, base: str, unit_name: str | None, tmpdir: str) -> str:
    """Write a one-unit objdiff project for (`target`, `base`) under `tmpdir`; return its directory.

    Both paths are absolutised: objdiff joins a relative path to the project directory, and on Windows only
    a backslash-rooted path counts as absolute.
    """
    proj = os.path.join(tmpdir, PROJECT_DIR)
    os.makedirs(proj, exist_ok=True)
    with open(os.path.join(proj, "objdiff.json"), "w", encoding="utf-8") as fh:
        json.dump({"min_version": MIN_PROJECT_VERSION,
                   "units": [{"name": unit_name or "measure", "target_path": os.path.abspath(target),
                              "base_path": os.path.abspath(base)}]}, fh, indent=2)
    return proj


def score(target: str, base: str, unit_name: str | None = None, tmpdir: str | None = None, *,
          objdiff: str, cwd: str | None = None, runner: Callable = subprocess.run) -> Report:
    """The official metric for one object pair: `objdiff report generate` on a one-unit project.

    The report is left at `<tmpdir>/unitutil_report.json` (`Report.path`); `tmpdir` defaults to this
    process's `lib.repo.session_tmpdir()`. A failed run raises `ReportError` with objdiff's output.
    """
    if tmpdir is None:
        from tools.lib.repo import session_tmpdir
        tmpdir = session_tmpdir()
    os.makedirs(tmpdir, exist_ok=True)
    proj = write_project(target, base, unit_name, tmpdir)
    out = os.path.join(tmpdir, REPORT_FILE)
    if os.path.exists(out):
        os.remove(out)
    p = runner([objdiff, "report", "generate", "-p", proj, "-o", out],
               cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        raise ReportError("objdiff report generate failed: " + (p.stdout or "") + (p.stderr or ""))
    with open(out, encoding="utf-8") as fh:
        return Report(json.load(fh), out)


def first_unit(report: Report) -> dict:
    """The one unit of a one-unit report (an empty dict when it has none)."""
    units = report.data.get("units") or []
    return units[0] if units else {}


def score_entries(target: str, base: str, unit_name: str | None = None, tmpdir: str | None = None, *,
                  objdiff: str, cwd: str | None = None, runner: Callable = subprocess.run) -> dict:
    """`{function: report entry}` of `score`, or `{"_error": text}` (never a 0.0 score)."""
    try:
        rep = score(target, base, unit_name, tmpdir, objdiff=objdiff, cwd=cwd, runner=runner)
    except ReportError as exc:
        return {"_error": str(exc)}
    return {f.get("name"): f for f in (first_unit(rep).get("functions") or [])}


def symbol_score(target: str, base: str, symbol: str, unit_name: str | None = None, tmpdir: str | None = None,
                 *, objdiff: str, cwd: str | None = None, runner: Callable = subprocess.run) -> dict:
    """The official score of one symbol: `{symbol, match_percent, target_size, report_json}` or `{symbol, error}`.

    `match_percent` is the report's `fuzzy_match_percent` as it stands (None when objdiff left it unscored),
    so a consumer can tell "unscored" from 0 % and the positional diff value is never exposed here.
    """
    if tmpdir is None:
        from tools.lib.repo import session_tmpdir
        tmpdir = session_tmpdir()
    entries = score_entries(target, base, unit_name, tmpdir, objdiff=objdiff, cwd=cwd, runner=runner)
    if "_error" in entries:
        return {"symbol": symbol, "error": entries["_error"]}
    fn = entries.get(symbol)
    if fn is None:
        return {"symbol": symbol, "error": "symbol is not in the target object (renamed? not in this unit?)"}
    return {"symbol": symbol, "match_percent": fn.get(SCORE_KEY), "target_size": fn.get("size"),
            "report_json": os.path.join(tmpdir, REPORT_FILE)}


def project_diff(root: str, unit_name: str, symbol: str, out: str, objdiff: str,
                 runner: Callable = subprocess.run) -> tuple[str | None, str]:
    """`objdiff diff -p . -u <unit_name> <symbol>` in `root` (the tree's own `objdiff.json`; left = target,
    right = ours) -> (json path or None, output). Rows only: `-c functionRelocDiffs=none` is the report's
    classification, and the JSON's positional `match_percent` is never the score."""
    os.makedirs(os.path.dirname(out), exist_ok=True)
    p = runner([objdiff, "diff", "-p", ".", "-u", unit_name, symbol,
                "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
               cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return (out if p.returncode == 0 else None), (p.stdout or "") + (p.stderr or "")


def retry_transient(fn: Callable, attempts: int = 4):
    """`fn()`, retrying a transient Windows sharing violation (`PermissionError`, WinError 5) with backoff.

    A scoring run writes its project and report under its own scratch directory; this covers the one-shot
    lock an antivirus or indexer can still hold on a file the run just created.
    """
    delay = 0.05
    last: PermissionError | None = None
    for i in range(attempts):
        try:
            return fn()
        except PermissionError as exc:
            last = exc
            if i + 1 < attempts:
                time.sleep(delay)
                delay *= 2
    raise (last if last else PermissionError("transient file lock"))


def diff_rows(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
              runner: Callable = subprocess.run) -> dict:
    """Instruction-level diff rows for one symbol (`objdiff diff -1 -2`), **never** a score.

    `-c functionRelocDiffs=none` matches `report generate`'s classification; the JSON's `match_percent` is
    exposed as `diff_match_percent`, a different normalisation that must not be quoted as the score.
    """
    out = os.path.join(tmpdir, "recompile_%s.json" % re.sub(r"\W", "_", symbol))
    os.makedirs(tmpdir, exist_ok=True)
    p = runner([objdiff, "diff", "-1", target, "-2", base, symbol,
                "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
               capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return {"symbol": symbol, "error": (p.stdout or "") + (p.stderr or "")}
    with open(out, encoding="utf-8") as fh:
        data = json.load(fh)
    if isinstance(data, dict) and "left" in data:
        sides = (data.get("left") or {}, data.get("right") or {})
    elif isinstance(data, dict) and "symbols" in data:
        sides = (data, data)
    else:
        return {"symbol": symbol, "error": "unrecognised objdiff output"}

    def entry(side):
        return next((s for s in side.get("symbols") or [] if s.get("name") == symbol), None)

    tgt, cand = entry(sides[0]), entry(sides[1])
    if tgt is None and cand is None:
        return {"symbol": symbol, "error": "symbol is in neither object (renamed? unpaired?)"}
    return {"symbol": symbol, "diff_match_percent": (cand or tgt or {}).get("match_percent"),
            "target_size": (tgt or {}).get("size"), "candidate_size": (cand or {}).get("size"),
            "paired": tgt is not None and cand is not None, "json": out}


# --------------------------------------------------------------------------------------------------
# freshness: is a prebuilt object (or the report) older than the sources it describes?
# --------------------------------------------------------------------------------------------------

def mtime(path: str) -> float | None:
    """The file's mtime, or None when it does not exist."""
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
    """The reasons the numbers would be stale, worst first - `[]` means current.

    Strict `<`: an equal stamp is current. In report mode the report is checked against the object and the
    newest source; otherwise the object is checked against the newest source. `rel` shortens printed paths.
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


def _uncommented(path: str) -> str:
    """A source file's text with its comments removed (`lib.cscan.remove_comments`): a commented-out `#include` is
    not a dependency."""
    with open(path, encoding="utf-8", errors="replace") as fh:
        return cscan.remove_comments(fh.read())


def includes_of(path: str) -> list[str]:
    """The `#include` targets (quoted and angle) named in a source file, comments removed; [] when unreadable."""
    try:
        return cscan.includes(_uncommented(path))
    except OSError:
        return []


def rel_path(path: str, tree: str) -> str:
    """A path as printed: relative to the tree, forward slashes."""
    try:
        out = os.path.relpath(path, tree)
    except ValueError:
        out = path
    return out.replace("\\", "/")


def _inside(path: str, root: str) -> bool:
    try:
        r = os.path.normcase(os.path.abspath(root))
        return os.path.commonpath([os.path.normcase(os.path.abspath(path)), r]) == r
    except ValueError:
        return False


def resolve_include(name: str, from_dir: str, root: str) -> str | None:
    """Where MWCC finds `name` from `from_dir`: beside the includer, then the include root (`lib.repo.header_root`),
    then `src/`; in-tree only."""
    name = name.replace("\\", "/")
    for base in (from_dir, os.path.join(root, header_root(root)), os.path.join(root, "src")):
        cand = os.path.normpath(os.path.join(base, *name.split("/")))
        if os.path.isfile(cand) and _inside(cand, root):
            return cand
    return None


def source_closure(src: str, root: str) -> list[str]:
    """The unit's source file and every in-tree header it reaches, transitively, each once - in the preprocessor's
    order (`lib.cscan.include_closure`: depth first, an include expanded where it stands)."""
    return cscan.include_closure(os.path.abspath(src),
                                 lambda name, includer: resolve_include(name, os.path.dirname(includer), root),
                                 read=_uncommented, angle=True)


def newest(paths: list[str]) -> tuple[str, float] | None:
    """The `(path, mtime)` of the most recently modified existing path, or None."""
    best: tuple[str, float] | None = None
    for p in paths:
        t = mtime(p)
        if t is not None and (best is None or t > best[1]):
            best = (p, t)
    return best


def unit_reasons(src: str, obj: str, root: str, rel=lambda p: p) -> tuple[list[str], tuple[str, float] | None]:
    """The stale reasons for a unit's prebuilt object against its include closure, and the newest source."""
    newest_source = newest(source_closure(src, root))
    return freshness(False, None, mtime(obj), newest_source, "", obj, rel=rel), newest_source


def report_reasons(report: str, obj: str, src: str, root: str,
                   rel=lambda p: p) -> tuple[list[str], tuple[str, float] | None]:
    """The stale reasons for a report's rows of one unit (report vs object vs include closure)."""
    newest_source = newest(source_closure(src, root))
    return freshness(True, mtime(report), mtime(obj), newest_source, report, obj, rel=rel), newest_source


class Freshness:
    """The staleness rule as one namespace: `Freshness.unit_reasons(...)`, `Freshness.report_reasons(...)`."""
    unit_reasons = staticmethod(unit_reasons)
    report_reasons = staticmethod(report_reasons)
    source_closure = staticmethod(source_closure)
