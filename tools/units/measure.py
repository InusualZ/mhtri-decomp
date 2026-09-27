#!/usr/bin/env python3
"""Score a whole unit in one compile and one `objdiff report generate` - the search-loop measurer.

`tools/units/recompile.py --measure <symbol>` answers one symbol per run, which is the right shape for the
*proof* step and the wrong shape for a search: a worker who wants to know whether a shape helped has to run
it once per symbol of the unit, and each run recompiles the source and issues **two** objdiff calls (the
report for the score, the positional diff for the rows). Workers hit that and each hand-wrote the same
driver - `build/tmp/mp.py`, `build/tmp/measure_all.py`, `build/scratch/*.py`, `.pi/scratch/score.py` - a
compile plus **one** report over the unit's own target object, printed as a per-symbol table. This is that
driver, shipped.

    python tools/units/measure.py <unit> [symbol] [--json] [-q]

What it does, in order, and the only place it differs from `recompile.py`:

* takes the unit's **real** command line from the same construction `recompile.py` uses
  (`recompile.unit_tokens` -> MAIN's ninja, the worktree's ninja, then a same-lib sibling) and compiles
  it into this tree's `build/RMHE08/src/...` through `recompile.compile_unit` (fresh-object assertion
  included). Nothing about the command line is re-derived here;
* resolves the **target** object the way a worker needs it: the worktree's own split object first (a
  proposal whose registration has landed on the branch but not on MAIN), then MAIN's registered object,
  then MAIN's retired `auto_*_text` object for the symbol's address (`recompile.proposal_target`);
* scores **every** symbol with one `report generate` over a one-unit project - the official
  `fuzzy_match_percent`, the same number `build/RMHE08/report.json` carries - and prints the unit's own
  official measures (``fuzzy_match_percent``, ``matched_functions``) beside a per-symbol table;
* remembers the previous run's scores in `build/tmp/measure/<unit>.scores.json` and prints the **delta**
  per symbol, which is what tells a worker "did this shape work" without babysitting a spreadsheet;
* `--baseline <report.json>` (or `--against-main`) makes that delta compare against a **saved** project
  report or **MAIN's** `build/RMHE08/report.json` instead of the tool's own last run - the shape the
  hand-written scorers all converged on (`build/probe/score.py` diffed a probe's rows against the committed
  report). One call then answers "every symbol of this unit, and what each one is worth against the build
  that landed", which is the per-iteration question. `--save` writes this run in that shape for the next
  one. A baseline that moved a row **down** is a regression, and the summary says so;
* `--diff` (or a symbol focus) adds a compact instruction-level mismatch list for the symbol, read from
  `recompile.diff_rows`' diagnostic JSON - never quoted as the score.

One compile, one report, N symbols: the cost of the search loop is the compiler, not N x the measurer.
`recompile.py --measure` remains the tool for a single-symbol proof; this one is for the round.

The unit is the path from the repository root (`Pl/pl_act`, `Camellia/camellia`, `auto/8005AA28_fn_8005AA28`);
the extension may be omitted and is inferred from the worktree's `src/`. Run it from the worktree you are
editing, or from MAIN with `--main` left to `git worktree list`; the source, `-o` directory and `-i` order
are always this tree's (that half is `recompile.py`'s, not this file's).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)

import unitutil  # noqa: E402
import recompile as rc  # noqa: E402

STATE_DIR = os.path.join("build", "tmp", "measure")


def _san(name: str) -> str:
    return re.sub(r"\W+", "_", name).strip("_") or "unit"


def normalize_unit(unit: str, wt: str) -> str:
    """`src/Camellia/camellia` / `main/Camellia/camellia` / `Camellia/camellia` -> `Camellia/camellia.c`.

    `recompile.unit_source` appends `.cpp` when no extension is given, which fails for every `.c` unit in
    the tree (`Camellia/camellia`, the `Runtime.PPCEABI.H` library, ...). Infer the extension from the
    worktree's `src/` instead, and strip the prefixes a worker is likely to paste (`src/`, `main/`,
    `build/RMHE08/src/`).
    """
    u = unit.replace("\\", "/").strip()
    while u.startswith("./"):
        u = u[2:]
    while True:
        for pre in ("build/RMHE08/src/", "build/RMHE08/obj/", "build/", "src/"):
            if u.startswith(pre):
                u = u[len(pre):]
                break
        else:
            break
    if u.startswith("main/"):
        u = u[len("main/"):]
    u = u.lstrip("/")
    if not u.endswith(rc.SRC_EXT):
        for ext in (".cpp", ".c", ".cp", ".cxx", ".cc"):
            if os.path.isfile(os.path.join(wt, "src", *u.split("/")) + ext):
                return u + ext
    return u


def objdiff_path(wt: str, main: str) -> str:
    """objdiff-cli from this tree if it has one, else MAIN's - a fresh worktree may have neither."""
    for root in (wt, main):
        cand = os.path.join(root, "build", "tools", "objdiff-cli.exe")
        if os.path.exists(cand):
            return cand
    return unitutil.OBJDIFF


def target_rel(unit: str) -> str:
    """The registered split object's path relative to a tree root, from the unit spelling."""
    head = os.path.join("build", "RMHE08", "obj", *rc.unit_source(unit).split("/"))
    return os.path.splitext(head)[0] + ".o"


def resolve_target(wt: str, main: str, unit: str, symbol: str):
    """(target object, kind, note). `kind` is `worktree-split`, `registered`, `auto-fallback` or `missing`.

    `recompile.measure_target` searches MAIN only; a proposal unit the worker has already split in its own
    tree has the *real* new object, so this one prefers it, then MAIN's registered object, then MAIN's
    retired object for the symbol's address. A registered unit in MAIN is unchanged (`registered`).
    """
    rel = target_rel(unit)
    same = os.path.normcase(os.path.abspath(wt)) == os.path.normcase(os.path.abspath(main))
    p_wt, p_main = os.path.join(wt, rel), os.path.join(main, rel)
    if not same and os.path.exists(p_wt):
        return p_wt, "worktree-split", ""
    if os.path.exists(p_main):
        return p_main, "registered", ""
    found, note = rc.proposal_target(main, symbol)
    if found:
        return found, "auto-fallback", note
    if not same:
        found, note = rc.proposal_target(wt, symbol)
        if found:
            return found, "auto-fallback", note
    return p_main, "missing", (
        "no original object for %s: no split object at %s in this tree or MAIN, and no retired "
        "`auto_*_text` object covers the address of %s" % (unit, rel, symbol))


def candidate_functions(obj: str):
    """[(name, size)] for the functions of our own object, in address order ([] when it is not ELF)."""
    try:
        return [(name, int(size)) for name, size, _frame in unitutil.frames(obj)]
    except Exception:
        return []


def score_report(target: str, base: str, unit_name: str, tmpdir: str, objdiff: str,
                 runner=subprocess.run):
    """One `report generate` over the pair -> ({name: entry}, unit measures, error-or-report-path).

    `unitutil.report_functions` is the shared primitive: one project, one report, every symbol. It reads
    its binary from the module global, so point that at the tree-resolved objdiff for the call.
    """
    previous = unitutil.OBJDIFF
    unitutil.OBJDIFF = objdiff
    try:
        entries = unitutil.report_functions(target, base, unit_name=unit_name, tmpdir=tmpdir,
                                            runner=runner)
    finally:
        unitutil.OBJDIFF = previous
    if "_error" in entries:
        return None, {}, entries["_error"]
    measures = {}
    report_path = os.path.join(tmpdir, "unitutil_report.json")
    try:
        data = json.load(open(report_path, encoding="utf-8"))
        units = data.get("units") or []
        if units:
            measures = units[0].get("measures") or {}
    except (OSError, ValueError):
        pass
    return entries, measures, report_path


def aggregates(entries: dict, measures: dict) -> dict:
    """Counts a worker decides with: the report's own `=100%` matched_functions, our `>= 80%`, the mean."""
    scores = [e.get("fuzzy_match_percent") for e in entries.values()]
    numeric = [s for s in scores if isinstance(s, (int, float))]
    return {
        "total": len(entries),
        "scored": len(numeric),
        "at100": measures.get("matched_functions"),
        "at100_percent": measures.get("matched_functions_percent"),
        "ge80": sum(1 for s in numeric if s >= 80.0),
        "mean": (sum(numeric) / len(numeric)) if numeric else None,
    }


def instruction_diff_rows(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
                          runner=subprocess.run) -> dict:
    """Compact instruction-level mismatches for one symbol, from objdiff's diagnostic `diff`.

    The metric in that JSON (`match_percent`, exposed by `recompile.diff_rows` as
    `diff_match_percent`) is **not** the report's - it is included as `diff_match_percent` and never
    printed as the score. The `functionRelocDiffs=none` the rows need is `recompile.diff_rows`'s.
    """
    detail = rc.diff_rows(target, base, symbol, objdiff, tmpdir, runner=runner)
    if "error" in detail:
        return {"error": detail["error"]}

    def find(side):
        for sym in (side.get("symbols") or []):
            if sym.get("name") == symbol:
                return sym

    try:
        data = json.load(open(detail["json"], encoding="utf-8"))
    except (OSError, KeyError, ValueError):
        return {"error": "unreadable objdiff diff output"}
    left = find(data.get("left") or {})
    right = find(data.get("right") or {})
    li = (left or {}).get("instructions") or []
    ri = (right or {}).get("instructions") or []
    rows, kinds = [], {}
    for i in range(max(len(li), len(ri))):
        a = li[i] if i < len(li) else None
        b = ri[i] if i < len(ri) else None
        kind = (a or {}).get("diff_kind") or (b or {}).get("diff_kind")
        if not kind:
            continue
        kinds[kind] = kinds.get(kind, 0) + 1
        ins = (a or {}).get("instruction") or (b or {}).get("instruction") or {}
        rows.append({
            "kind": kind,
            "address": ins.get("address"),
            "target": (a or {}).get("instruction", {}).get("formatted"),
            "ours": (b or {}).get("instruction", {}).get("formatted"),
        })
    return {"diff_match_percent": detail.get("diff_match_percent"), "instructions": len(li),
            "differ": len(rows), "kinds": kinds, "rows": rows, "json": detail.get("json")}


def cache_path(wt: str, unit: str) -> str:
    return os.path.join(wt, STATE_DIR, _san(unit) + ".scores.json")


def numeric(rows: dict) -> dict:
    """The `{symbol: score}` subset of a mapping whose values are not numbers."""
    return {name: value for name, value in (rows or {}).items()
            if isinstance(value, (int, float))}


def load_baseline(path: str, unit_name: str):
    """({symbol: score}, note) from a saved report - a project `report.json` or a `--save` file.

    The two shapes a worker already has on disk: the campaign's `build/RMHE08/report.json` (units carry
    `functions[].fuzzy_match_percent`) and a previous `measure.py --save`. A unit the file does not carry
    is an error, never an empty baseline that would read every row as "new".
    """
    try:
        data = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return None, "cannot read baseline %s: %s" % (path, exc)
    rows = numeric(data.get("scores"))
    if rows:
        return rows, path
    for u in data.get("units") or []:
        name = u.get("name") or ""
        if name == unit_name or name.endswith("/" + unit_name):
            rows = numeric({f.get("name"): f.get("fuzzy_match_percent")
                            for f in u.get("functions") or []})
            return rows, path
    return None, ("baseline %s has no unit %s (it carries %d unit(s))"
                  % (path, unit_name, len(data.get("units") or [])))


def moved_summary(functions: dict, baseline: dict) -> dict:
    """How many rows moved against a baseline, and in which direction (`down` is a regression)."""
    moved = up = down = 0
    for name, row in (functions or {}).items():
        now, before = row.get("score"), (baseline or {}).get(name)
        if not (isinstance(now, (int, float)) and isinstance(before, (int, float))):
            continue
        if abs(now - before) > 1e-9:
            moved += 1
            if now > before:
                up += 1
            else:
                down += 1
    return {"moved": moved, "up": up, "down": down}


def save_run(path: str, result: dict) -> None:
    """Write this run's scores in the `--baseline` shape, so the next iteration can diff against it."""
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    rows = {name: row.get("score") for name, row in (result.get("functions") or {}).items()
            if isinstance(row.get("score"), (int, float))}
    with open(path, "w", encoding="utf-8") as fh:
        json.dump({"unit": result.get("unit"), "target": result.get("target"),
                   "measures": result.get("measures") or {}, "scores": rows}, fh, indent=1)


def _load_scores(path: str, target: str):
    try:
        data = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError):
        return None
    same = os.path.normcase(os.path.abspath(data.get("target") or "")) == \
        os.path.normcase(os.path.abspath(target))
    return (data.get("scores") or {}) if same else None


def _save_scores(path: str, target: str, entries: dict) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    rows = {name: e.get("fuzzy_match_percent") for name, e in entries.items()
            if isinstance(e.get("fuzzy_match_percent"), (int, float))}
    with open(path, "w", encoding="utf-8") as fh:
        json.dump({"target": target, "scores": rows}, fh, indent=1)


def collect(unit: str, wt: str, main: str, symbol: str = None, runner=subprocess.run,
            use_cache: bool = True, baseline_path: str = None) -> dict:
    """Compile once, score every symbol with one report, return the whole result (no printing).

    `baseline_path` (a project `report.json` or a `--save` file) replaces the tool's own cache as the
    delta's "before": every symbol then carries its delta against that saved build. A baseline that names
    a unit the file does not carry is refused loudly (see `load_baseline`).
    """
    unit = normalize_unit(unit, wt)
    unit_name = "main/" + os.path.splitext(unit)[0]
    baseline = None
    if baseline_path:
        baseline, note = load_baseline(baseline_path, unit_name)
        if baseline is None:
            return {"unit": unit, "worktree": wt, "main": main, "compiled": False,
                    "error": note}
    tokens, command_source = rc.unit_tokens(main, wt, unit, runner=runner)
    compiled = rc.compile_unit(unit, main, wt, tokens=tokens, runner=runner)
    if not compiled.get("compiled"):
        return {"unit": unit, "worktree": wt, "main": main, "compiled": False,
                "error": compiled.get("error", "compile failed")}
    obj = compiled["object"]
    # the measurement boundary's own stale guard: a compile that wrote nothing must never be scored
    ok, why = rc.object_is_fresh(obj, rc.source_path(wt, unit))
    if not ok:
        return {"unit": unit, "worktree": wt, "main": main, "compiled": False, "error": why}
    candidates = candidate_functions(obj)
    names = [name for name, _size in candidates]

    # The fallback locates the target by address, so it needs a symbol even for a whole-unit run.
    probe = symbol or (names[0] if names else unit)
    target, kind, note = resolve_target(wt, main, unit, probe)
    result = {
        "unit": unit, "worktree": wt, "main": main, "object": obj, "compiled": True,
        "fresh": compiled.get("fresh"), "bytes": compiled.get("bytes"),
        "sections": compiled.get("sections") or {}, "command_source": command_source,
        "target": target, "target_kind": kind, "target_note": note,
        "candidate": candidates,
    }
    if kind == "missing":
        return result

    tmpdir = os.path.join(wt, STATE_DIR, _san(unit) + ".report")
    entries, measures, err = score_report(target, obj, unit_name, tmpdir,
                                          objdiff_path(wt, main), runner=runner)
    if entries is None:
        result["score_error"] = err
        return result
    ours = dict(candidates)
    functions = {}
    for name, entry in entries.items():
        size = entry.get("size")
        functions[name] = {
            "score": entry.get("fuzzy_match_percent"),
            "target_size": int(size) if str(size).lstrip("-").isdigit() else size,
            "ours_size": ours.get(name),
            "address": (entry.get("metadata") or {}).get("virtual_address") or entry.get("address"),
        }
    result["functions"] = functions
    result["measures"] = measures
    result["aggregates"] = aggregates(entries, measures)
    result["extra_functions"] = [name for name in names if name not in entries]

    path = cache_path(wt, unit)
    if baseline is not None:
        for name, row in functions.items():
            now, before = row["score"], baseline.get(name)
            if isinstance(now, (int, float)) and isinstance(before, (int, float)):
                row["delta"] = now - before
        result["baseline"] = baseline_path
        result["moved"] = moved_summary(functions, baseline)
    elif use_cache:
        previous = _load_scores(path, target)
        if previous is not None:
            for name, row in functions.items():
                now, before = row["score"], previous.get(name)
                if isinstance(now, (int, float)) and isinstance(before, (int, float)):
                    row["delta"] = now - before
        _save_scores(path, target, entries)

    if symbol:
        result["symbol"] = symbol
        if symbol not in entries:
            result["symbol_error"] = "not in the report (renamed, or not in this target object)"
        else:
            # one extra objdiff call, only when a symbol was named; `--diff` decides whether it prints
            result["detail"] = instruction_diff_rows(
                target, obj, symbol, objdiff_path(wt, main), tmpdir, runner=runner)
    return result


def _score_str(value) -> str:
    return "%.2f%%" % value if isinstance(value, (int, float)) else "-"


def _signed(value) -> str:
    if not isinstance(value, (int, float)):
        return ""
    return "+%.2f" % value if value >= 0 else "%.2f" % value


def _sort_key(row, mode, name):
    score = row["score"]
    if mode == "addr":
        key = str(row.get("address") or "0").rjust(12)
        return (0, key, name)
    if mode == "name":
        return (0, name, name)
    # score: the unscored (unpaired / missing) symbols first, then worst-first
    return (0 if not isinstance(score, (int, float)) else 1,
            score if isinstance(score, (int, float)) else 0.0, name)


def format_report(r: dict, sort: str = "score", limit: int = None, quiet: bool = False,
                  show_diff: bool = False) -> list:
    """The worker-facing text. `-q` keeps the summary and the focused symbol, drops the table."""
    out = []
    unit = r["unit"]
    out.append("unit      %s" % unit)
    if r.get("command_source"):
        out.append("command   %s" % r["command_source"])
    out.append("target    %s%s" % (r["target"],
                                  "  [%s]" % r["target_kind"] if r.get("target_kind") else ""))
    if r.get("baseline"):
        out.append("baseline  %s  (before = this saved report)" % r["baseline"])
    if r.get("target_note"):
        out.append("          %s" % r["target_note"])
    out.append("compiled  %s  (%s bytes, fresh=%s)" % (r["object"], r.get("bytes"),
                                                       r.get("fresh")))
    if r.get("score_error"):
        out.append("score     ERROR %s" % r["score_error"])
        return out
    if "functions" not in r:
        out.append("score     SKIPPED  %s" % (r.get("target_note") or "no target object"))
        return out

    agg = r.get("aggregates") or {}
    measures = r.get("measures") or {}
    if isinstance(measures.get("fuzzy_match_percent"), (int, float)):
        out.append("score     %s fuzzy   (unit, official report metric)"
                   % ("%.5f" % measures["fuzzy_match_percent"]))
    bits = ["%s functions" % agg.get("total", len(r["functions"]))]
    if agg.get("at100") is not None:
        bits.append("%s == 100%% (report matched_functions)" % agg["at100"])
    bits.append("%s >= 80%%" % agg.get("ge80"))
    if agg.get("mean") is not None:
        bits.append("mean %.2f%% of %s scored" % (agg["mean"], agg.get("scored")))
    out.append("functions " + "   ".join(bits))
    tbytes = sum(v for v in (row.get("target_size") for row in r["functions"].values())
                 if isinstance(v, int))
    obytes = sum(v for v in (row.get("ours_size") for row in r["functions"].values())
                 if isinstance(v, int))
    out.append("text      %d B target   %d B ours" % (tbytes, obytes))
    moved = r.get("moved")
    if moved:
        out.append("moved     %d row(s) moved vs baseline: %d up, %d down%s"
                   % (moved["moved"], moved["up"], moved["down"],
                      "   <- a DOWN row is a regression" if moved["down"] else ""))

    symbol = r.get("symbol")
    if symbol:
        row = (r.get("functions") or {}).get(symbol)
        if row is None:
            out.append("symbol    %s  %s" % (symbol, r.get("symbol_error", "not scored")))
        else:
            delta = _signed(row.get("delta"))
            out.append("symbol    %-38s %9s  target %5s B  ours %5s B%s" % (
                symbol, _score_str(row["score"]), row.get("target_size"), row.get("ours_size"),
                "   delta %s" % delta if delta else ""))
        if show_diff and r.get("detail"):
            detail = r["detail"]
            if "error" in detail:
                out.append("diff      ERROR %s" % detail["error"])
            else:
                kinds = ", ".join("%s %d" % (k.replace("DIFF_", "").lower(), v)
                                  for k, v in sorted(detail["kinds"].items()))
                out.append("diff      %d instructions, %d differ%s" % (
                    detail["instructions"], detail["differ"],
                    "  (%s)" % kinds if kinds else "  (identical)"))
                for row in detail["rows"][:12]:
                    addr = row.get("address") or "?"
                    out.append("          0x%-6s %-22s target: %s" % (
                        addr, row["kind"].replace("DIFF_", ""), row.get("target") or "-"))
                    out.append("          %-8s %-22s ours:   %s" % ("", "", row.get("ours") or "-"))
                if len(detail["rows"]) > 12:
                    out.append("          ... %d more differing instructions"
                               % (len(detail["rows"]) - 12))

    if not quiet:
        out.append("")
        out.append("    score      delta   target    ours   symbol")
        rows = list(r["functions"].items())
        rows.sort(key=lambda kv: _sort_key(kv[1], sort, kv[0]))
        if limit is not None:
            rows = rows[:limit]
        for name, row in rows:
            score = row["score"]
            mark = "!" if isinstance(score, (int, float)) and score < 80.0 else (
                "?" if score is None else " ")
            out.append("  %s %9s  %7s  %6s  %6s   %s" % (
                mark, _score_str(score), _signed(row.get("delta")) or ".",
                row.get("target_size"), row.get("ours_size"), name))
        hidden = len(r["functions"]) - len(rows)
        if hidden > 0:
            out.append("    ... %d more (--limit)" % hidden)
        for name in r.get("extra_functions") or []:
            out.append("    %9s  %7s  %6s  %6s   %s" % ("-", "", "", "", name + "  [ours extra]"))
    return out


def emit(r: dict, sort: str = "score", limit: int = None, quiet: bool = False,
         show_diff: bool = False) -> int:
    if not r.get("compiled"):
        print("FAILED: %s\n%s" % (r["unit"], r.get("error", "")))
        return 1
    for line in format_report(r, sort=sort, limit=limit, quiet=quiet, show_diff=show_diff):
        print(line)
    if r.get("compiled") and not r.get("fresh"):
        print("WARNING: the object's mtime did not move - treat the scores as stale", file=sys.stderr)
        return 1
    # a baseline that moved a row down is a regression: say so in the exit code so a sweep can gate on it
    if (r.get("moved") or {}).get("down"):
        return 1
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", help="unit path from the repository root, e.g. Pl/pl_act, Camellia/camellia")
    ap.add_argument("symbol", nargs="?", help="focus one symbol (adds its row and, with --diff, its rows)")
    ap.add_argument("--main", default=None, help="main worktree (default: resolved with git)")
    ap.add_argument("--json", action="store_true", help="emit the whole result as JSON")
    ap.add_argument("--diff", action="store_true", help="instruction-level mismatches for the symbol")
    ap.add_argument("--sort", choices=("score", "addr", "name"), default="score",
                    help="table order (default: worst score first)")
    ap.add_argument("--limit", type=int, default=None, help="show at most N table rows")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="summary and focused symbol only - the search-loop form")
    ap.add_argument("--no-cache", action="store_true", help="do not read or write the score cache")
    ap.add_argument("--baseline", default=None,
                    help="a saved report to diff against: a project report.json or a --save file")
    ap.add_argument("--against-main", action="store_true",
                    help="diff against MAIN's build/RMHE08/report.json (the last landed build)")
    ap.add_argument("--save", default=None, help="write this run's scores here, for a later --baseline")
    args = ap.parse_args(argv)

    wt = rc.worktree_root()
    main_wt = args.main or rc.main_root(wt)
    baseline = args.baseline
    if args.against_main:
        baseline = os.path.join(main_wt, "build", "RMHE08", "report.json")
    try:
        result = collect(args.unit, wt, main_wt, symbol=args.symbol, use_cache=not args.no_cache,
                         baseline_path=baseline)
    except SystemExit as exc:
        print("measure: %s" % exc, file=sys.stderr)
        return 2
    if args.save and result.get("compiled"):
        save_run(args.save, result)
    if args.json:
        print(json.dumps(result, indent=2))
        return 0 if result.get("compiled") else 1
    return emit(result, sort=args.sort, limit=args.limit, quiet=args.quiet, show_diff=args.diff)


if __name__ == "__main__":
    raise SystemExit(main())
