#!/usr/bin/env python3
"""Try source rewrites of a unit and report the official per-function score of each. Spec: docs/tools/spec/tryvar.md.
CLI: python tools/flags/tryvar.py [-u U] [names] [--variants F.py|.txt|.json] [--permute F] [--symbol S] [--rename-map F] [--json] [--list]
[--apply NAME | --apply-best] | --permdecl FN[,FN] [--lines A-B] [--max-perms N] [--plain-only] [--apply] | --hill FN [--lines A-B] [--max-evals N] [--apply]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import importlib.util
import itertools
import json
import os

from tools.lib import declperm, pendingmap, repo, report, text, units

#: The marker that starts a block in a text variants file (`//@@ old`, `//@@ variant <name>`, `//@@ permute`,
#: `//@@ item`).
MARKER = "//@@"
#: The name of the unmodified source in a ranking.
AS_IS = "(as-is)"


def scratch(unit):
    return os.path.join(unit.root, "build", "tmp", "probe")


def load_variants(path):
    """`[(name, repls)]` from a `.py` variants file (`VARIANTS = [...]`), a `.json` spec (`parse_json_variants`) or a
    text file of `//@@` blocks."""
    if path.endswith(".json"):
        with open(path, encoding="utf-8-sig") as fh:
            return parse_json_variants(json.load(fh))
    if not path.endswith(".py"):
        with open(path, encoding="utf-8", newline="") as fh:
            return parse_marker_variants(fh.read().replace("\r\n", "\n"))
    spec = importlib.util.spec_from_file_location("variants", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return list(getattr(mod, "VARIANTS", []))


def marker_blocks(text_: str) -> list[tuple[str, str, str]]:
    """`[(kind, argument, body)]` of the `//@@ <kind> [argument]` blocks; a body loses its one trailing newline."""
    blocks: list[list] = []
    for line in text_.split("\n"):
        stripped = line.strip()
        if stripped.startswith(MARKER):
            head = stripped[len(MARKER):].split(None, 1)
            if not head:
                raise SystemExit("tryvar: a bare %s marker (want `old`, `variant <name>`, `permute` or `item`)" % MARKER)
            blocks.append([head[0], head[1].strip() if len(head) > 1 else "", []])
        elif blocks:
            blocks[-1][2].append(line)
    out = []
    for kind, arg, lines in blocks:
        body = "\n".join(lines)
        out.append((kind, arg, body[:-1] if body.endswith("\n") else body))
    return out


def parse_marker_variants(text_: str) -> list:
    """`[(name, [(old, new)])]`: each `//@@ variant <name>` replaces the text of the latest `//@@ old` block. A name
    used again after a later `old` block adds that replacement to the same variant, so one variant can rewrite
    several places (the replacements apply in file order)."""
    out, old, by_name = [], None, {}
    for kind, arg, body in marker_blocks(text_):
        if kind == "old":
            old = body
        elif kind == "variant":
            if old is None:
                raise SystemExit("tryvar: variant %r comes before any `%s old` block" % (arg, MARKER))
            name = arg or "variant-%d" % (len(out) + 1)
            if name in by_name:
                by_name[name].append((old, body))
            else:
                by_name[name] = [(old, body)]
                out.append((name, by_name[name]))
        else:
            raise SystemExit("tryvar: unknown block `%s %s` in a variants file" % (MARKER, kind))
    return out


def parse_json_variants(data) -> list:
    """`[(name, [(old, new)])]` from a JSON spec: `{"<name>": [["old", "new"], ...], ...}` or
    `[{"name": "<name>", "replace": [["old", "new"], ...]}, ...]`. Anything else is refused."""
    items = list(data.items()) if isinstance(data, dict) else \
        [(v.get("name"), v.get("replace")) for v in data] if isinstance(data, list) and \
        all(isinstance(v, dict) for v in data) else None
    if items is None:
        raise SystemExit("tryvar: a JSON spec is {name: [[old, new], ...]} or [{name, replace}, ...]")
    out = []
    for name, pairs in items:
        if not isinstance(name, str) or not name or not isinstance(pairs, list) or not pairs or not all(
                isinstance(p, list) and len(p) == 2 and all(isinstance(x, str) for x in p) for p in pairs):
            raise SystemExit("tryvar: variant %r: want a non-empty list of [old, new] string pairs" % (name,))
        out.append((name, [(o.replace("\r\n", "\n"), n.replace("\r\n", "\n")) for o, n in pairs]))
    names = [n for n, _p in out]
    if len(set(names)) != len(names):
        raise SystemExit("tryvar: a JSON spec names a variant twice")
    return out


def permutation_variants(text_: str, limit: int = 120) -> list:
    """`[(name, [(old, new)])]` for every order of the `//@@ item` blocks after one `//@@ permute`.

    The items, concatenated in file order (each followed by a newline), must be one contiguous run of the source;
    each variant rewrites that run in another order. The identity order is left out (it is the as-is source).
    `limit` refuses a set with more orders than that rather than compiling thousands."""
    items = [body for kind, _arg, body in marker_blocks(text_) if kind == "item"]
    if not items:
        raise SystemExit("tryvar: no `%s item` blocks in the permutation file" % MARKER)
    count = 1
    for i in range(2, len(items) + 1):
        count *= i
    if count - 1 > limit:
        raise SystemExit("tryvar: %d items give %d orders (> --max-variants %d) - split the list"
                         % (len(items), count - 1, limit))
    old = "".join(item + "\n" for item in items)
    out = []
    for order in itertools.permutations(range(len(items))):
        if list(order) == list(range(len(items))):
            continue
        out.append(("order-" + ",".join(str(i) for i in order), [(old, "".join(items[i] + "\n" for i in order))]))
    return out


def default_variants_path(unit):
    """`tools/flags/variants/<lib>.py`, falling back to the lower-case spelling (case-sensitive FS)."""
    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "variants")
    lib = unit.module or "main"            # a top-level unit has no directory: its variants file is main.py
    for name in (lib + ".py", lib.lower() + ".py"):
        if os.path.exists(os.path.join(d, name)):
            return os.path.join(d, name)
    return os.path.join(d, lib + ".py")


def match_pcts(probe_obj, target_obj, symbol=None, root=None):
    """{function: (official_match_percent, ours_size, target_size)} for a probe object.

    The percent is the report metric for the object pair (`lib.report.score_entries`), not the positional
diff value. The two sizes come from the objects' own symbol tables, so every function the probe emitted
    is listed; `symbol` is accepted for backwards compatibility and ignored - the report scores the whole
    object pair, which is what a variant comparison needs.
    """
    root = root or repo.repo_root()
    entries = report.score_entries(target_obj, probe_obj, None, repo.session_tmpdir(),
                                   objdiff=report.objdiff_cli(root), cwd=root)
    if "_error" in entries:
        print("objdiff report failed: " + entries["_error"][:400])
        return None
    ours = {n: sz for n, sz, _f in units.frames(probe_obj)}
    tgt = {n: sz for n, sz, _f in units.frames(target_obj)}
    res = {}
    for name, e in entries.items():
        pct = e.get("fuzzy_match_percent")
        if not isinstance(pct, (int, float)) or name not in ours:
            continue                     # function the probe did not emit (or an unpaired row)
        res[name] = (round(pct, 2), int(ours.get(name) or 0), int(tgt.get(name) or 0))
    return res


def first_divergence(probe_obj, target_obj, symbol, root=None):
    """The index of `symbol`'s first instruction objdiff does not pair as identical, None when every one is,
    or `"?"` when the diff could not be read (`lib.report.diff_rows`, never a score)."""
    root = root or repo.repo_root()
    d = report.diff_rows(target_obj, probe_obj, symbol, report.objdiff_cli(root), repo.session_tmpdir())
    if d.get("error") or not d.get("json"):
        return "?"
    with open(d["json"], encoding="utf-8") as fh:
        return divergence_of(json.load(fh), symbol)


def divergence_of(data: dict, symbol: str):
    """The first target-side instruction of `symbol` with a `diff_kind` in an `objdiff diff` JSON (None: none,
    `"?"`: the symbol is absent)."""
    for entry in (data.get("left") or {}).get("symbols") or []:
        if entry.get("name") == symbol:
            for i, insn in enumerate(entry.get("instructions") or []):
                if insn.get("diff_kind") not in (None, "DIFF_NONE"):
                    return i
            return None
    return "?"


def rewrite(src, repls):
    """`(text, None)` with the rewrite applied, or `(None, why)`. A callable rewrite returns the text (None: it did
    not apply); each `(old, new)` pair must occur exactly once in the text as it stands after the earlier pairs -
    an absent or a repeated `old` is refused, never applied at a guessed occurrence."""
    if callable(repls):
        new = repls(src)
        return (new, None) if new is not None else (None, "the rewrite did not apply")
    for old, new in repls:
        count = src.count(old)
        if count != 1:
            return None, ("pattern not found: %r" % old[:60] if not count
                          else "pattern occurs %d times (want exactly one): %r" % (count, old[:60]))
        src = src.replace(old, new, 1)
    return src, None


def apply_variant(src, repls):
    new, why = rewrite(src, repls)
    if new is None:
        print("    (%s)" % why)
    return new


def try_variant(unit, tokens, name, repls, symbol=None, score=None, diverge=None):
    """Compile one rewrite of the unit's source as a probe file next to it and score it -> a result dict.

    `status` is `ok`, `skip` (the rewrite did not apply), `compile-failed` or `no-result`. The probe is removed
    whatever happens; the real source is only read. `score(probe_obj, target_obj) -> {fn: (pct, ours, target)}`
    and `diverge(probe_obj, target_obj, symbol)` default to the official report metric and objdiff's diff."""
    score = score or (lambda obj, tgt: match_pcts(obj, tgt, None, unit.root))
    diverge = diverge or (lambda obj, tgt, sym: first_divergence(obj, tgt, sym, unit.root))
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    res = {"name": name, "status": "ok", "functions": {}, "text_size": None, "log": ""}
    new, why = (src, None) if repls is None else rewrite(src, repls)
    if new is None:
        res.update(status="skip", log=why)
        return res
    ext = os.path.splitext(unit.source)[1]
    probe_src = os.path.join(os.path.dirname(unit.source), unit.file + "_probe" + ext)
    try:                                 # the write is inside: an interrupt mid-write still removes the probe
        with open(probe_src, "w", encoding="utf-8", errors="surrogateescape") as f:
            f.write(new)
        rc, log, obj = units.run_tokens(tokens, unit.root, scratch_dir=scratch(unit), src=probe_src)
        if rc != 0:
            res.update(status="compile-failed", log=units.quiet(log)[:600])
            return res
        pcts = score(obj, unit.obj_target)
        res["text_size"] = units.text_size(obj)
        if symbol and pcts and symbol in pcts:
            res["first_divergence"] = None if pcts[symbol][0] >= 100.0 else diverge(obj, unit.obj_target, symbol)
    finally:
        if os.path.exists(probe_src):
            os.remove(probe_src)
    if not pcts:
        res["status"] = "no-result"
        return res
    res["functions"] = {n: list(v) for n, v in pcts.items()}
    res["matched_bytes"] = round(sum(p / 100.0 * t for p, _o, t in pcts.values()), 2)
    if symbol:
        res["symbol_percent"] = pcts[symbol][0] if symbol in pcts else None
    return res


def describe(res) -> str:
    """The one-line verdict tryvar has always printed for a variant."""
    if res["status"] == "skip":
        return "%-26s SKIP (%s)" % (res["name"], res.get("log") or "rewrite did not apply")
    if res["status"] == "compile-failed":
        return "%-26s COMPILE FAILED\n%s" % (res["name"], res["log"])
    if res["status"] == "no-result":
        return "%-26s no report result" % res["name"]
    pcts = res["functions"]
    bad = ["%s=%.2f%%(%d/%d)" % (n, p, o, t) for n, (p, o, t) in pcts.items() if p != 100.0]
    verdict = "ALL %d FUNCTIONS AT 100%%" % len(pcts) if not bad else "; ".join(bad)
    return "%-26s .text=%-6d %s" % (res["name"], res["text_size"], verdict)


def merit(r, symbol=None) -> tuple:
    """A scored result's merit, larger is better: `symbol`'s percent, then its first divergence (later is better, a
    full match best), then the unit's matched bytes; without a symbol, matched bytes alone."""
    if not symbol:
        return (r["matched_bytes"],)
    d = r.get("first_divergence")
    div = float("inf") if d is None else (d if isinstance(d, int) else -1)
    return (r.get("symbol_percent") or 0.0, div, r["matched_bytes"])


def rank(results, symbol=None):
    """The scored results best first by `merit`, equal merits by name."""
    ok = [r for r in results if r["status"] == "ok"]
    return sorted(ok, key=lambda r: (tuple(-x for x in merit(r, symbol)), r["name"]))


def print_ranking(ranked, symbol=None) -> None:
    print("ranking (%s)" % ("%s's score, then its first divergence, then the unit's matched bytes" % symbol
                            if symbol else "the unit's matched bytes"))
    print("  %-4s %-26s %9s %10s %7s %12s" % ("#", "variant", "symbol %", "first div", ".text", "matched B"))
    for i, r in enumerate(ranked, 1):
        d = r.get("first_divergence", "-")
        print("  %-4d %-26s %9s %10s %7d %12.2f"
              % (i, r["name"], "%.2f" % r["symbol_percent"] if r.get("symbol_percent") is not None else "-",
                 "full" if symbol and d is None else d, r["text_size"], r["matched_bytes"]))


def try_all(unit, tokens, selected, symbol=None, score=None, diverge=None, as_is=False, out=print, stop=None):
    """Every selected variant (the unmodified source first when `as_is`) -> `[result]`, each line printed.
    `stop(result)` true ends the run early (the rest are not compiled)."""
    results = []
    plan = ([(AS_IS, None)] if as_is else []) + list(selected)
    for name, repls in plan:
        res = try_variant(unit, tokens, name, repls, symbol, score, diverge)
        results.append(res)
        if out:
            out(describe(res))
        if stop and stop(res):
            break
    return results


def find_block(src, function, max_lines, initialised=True, line_range=None):
    """The run to permute: `line_range` (`(first, last)`, 1-based) as `declperm.block_run`, else the leading run
    (`declperm.find_run`). `ValueError` when the range is not a declaration block of the function."""
    if line_range:
        return declperm.block_run(src, function, line_range[0], line_range[1])
    return declperm.find_run(src, function, max_lines, initialised)


def permdecl_one(unit, tokens, function, max_lines, cap, seed, score=None, diverge=None, out=None,
                 initialised=True, line_range=None):
    """Permute `function`'s leading declarations (or the block `line_range`): `{function, status, run, orders,
    sampled, tried, table, best}`. Every order is compiled as a probe (the source is only read) and ranked by the
    function's official score; the as-is source is tried first and wins every tie, and the search ends at the first
    100 %."""
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    rec = {"function": function, "status": "ok", "run": [], "orders": 0, "sampled": False, "tried": 0,
           "table": [], "best": None, "best_order": None, "range": list(line_range) if line_range else None}
    try:
        run = find_block(src, function, max_lines, initialised, line_range)
    except ValueError as exc:
        rec.update(status="ambiguous", error=str(exc))
        return rec
    if run is None:
        rec["status"] = "no-run"
        return rec
    ords, sampled = declperm.orders(len(run.lines), cap, seed, run.deps)
    rec.update(run=[l.strip() for l in run.lines], orders=len(ords), sampled=sampled)
    by_name = {declperm.name_of(o): o for o in ords}
    results = try_all(unit, tokens, declperm.variants(run, ords), function, score, diverge, as_is=True, out=out,
                      stop=lambda r: (r.get("symbol_percent") or 0.0) >= 100.0)
    ranked = rank(results, function)
    rec["tried"] = len(results) - 1
    for r in ranked:
        o = by_name.get(r["name"])
        rec["table"].append({"name": r["name"], "percent": r.get("symbol_percent"),
                             "first_divergence": r.get("first_divergence"), "matched_bytes": r["matched_bytes"],
                             "lines": [run.lines[i].strip() for i in o] if o else [l.strip() for l in run.lines]})
    if not ranked or ranked[0]["name"] == AS_IS:
        rec["status"] = "no-gain" if ranked else "no-result"
    else:
        rec.update(best=ranked[0]["name"], best_order=list(by_name[ranked[0]["name"]]))
    return rec


def permdecl_apply(lf_text, records, max_lines, initialised=True):
    """The source text with every record's best order written, or `None` when there is nothing to write."""
    changed = False
    for rec in records:
        if not rec.get("best_order"):
            continue
        try:
            run = find_block(lf_text, rec["function"], max_lines, initialised, rec.get("range"))
        except ValueError:
            run = None
        new = declperm.reorder(lf_text, run, tuple(rec["best_order"])) if run else None
        if new is not None and new != lf_text:
            lf_text, changed = new, True
    return lf_text if changed else None


def write_source(unit, transform, root):
    """Write `transform(lf_text)` over the unit's real source, keeping its CRLF when it has one -> the source's path
    relative to `root`, or None when `transform` returned None or the text unchanged (nothing written)."""
    raw = open(unit.source, "rb").read()
    lf = raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n")
    new = transform(lf)
    if new is None or new == lf:
        return None
    out = new.replace("\n", "\r\n") if b"\r\n" in raw else new
    text.atomic_write(unit.source, out.encode("utf-8", "surrogateescape"))
    return os.path.relpath(unit.source, root)


def print_permdecl(rec) -> None:
    fn = rec["function"]
    if rec["status"] in ("ambiguous", "no-run"):
        print("%s: %s" % (fn, rec.get("error") or "no run of two or more leading declarations"))
        return
    print("%s: %d declaration(s), %d order(s)%s, %d tried" % (
        fn, len(rec["run"]), rec["orders"], " (sampled)" if rec["sampled"] else "", rec["tried"]))
    print("  %-4s %-16s %9s %10s %12s  %s" % ("#", "order", "symbol %", "first div", "matched B", "declarations"))
    for i, row in enumerate(rec["table"][:12], 1):
        d = row["first_divergence"]
        print("  %-4d %-16s %9s %10s %12.2f  %s" % (
            i, row["name"], "%.2f" % row["percent"] if row["percent"] is not None else "-",
            "full" if row["percent"] and row["percent"] >= 100.0 else d, row["matched_bytes"],
            " | ".join(row["lines"])))
    if len(rec["table"]) > 12:
        print("  ... %d more" % (len(rec["table"]) - 12))
    print("  verdict: %s" % ({"no-gain": "no order beats the source's own", "no-result": "no order scored"}
                             .get(rec["status"]) or "best is %s" % rec["best"]))


def run_permdecl(args, unit, tokens, root, score=None, diverge=None):
    """`--permdecl`: the table per function, and with `--apply` the best orders written to the real source."""
    records = []
    line_range = getattr(args, "lines", None)
    scorers = {k: v for k, v in (("score", score), ("diverge", diverge)) if v is not None}
    for fn in [f for f in args.permdecl.split(",") if f]:
        rec = permdecl_one(unit, tokens, fn, args.max_lines, args.max_perms, args.seed,
                           initialised=not getattr(args, "plain_only", False), line_range=line_range, **scorers)
        records.append(rec)
        if not args.json:
            print_permdecl(rec)
    applied = None
    if args.apply:
        applied = write_source(unit, lambda lf: permdecl_apply(lf, records, args.max_lines,
                                                               not getattr(args, "plain_only", False)), root)
    if args.json:
        print(json.dumps({"unit": unit.report_name, "functions": records, "applied": applied}, indent=2))
    elif args.apply:
        print("applied the best orders to %s - rebuild and re-measure" % applied if applied
              else "nothing to apply: the source's own orders are the best found")
    elif any(r.get("best") for r in records):
        print("source untouched; --apply writes the best order of each function")
    return 0


def hill_one(unit, tokens, function, line_range=None, max_lines=64, max_evals=200, score=None, diverge=None,
             out=None, initialised=True):
    """Hill-climb the order of `function`'s declaration block: `{function, status, run, range, tried, path, best,
    best_order, stopped}`.

    The block is `line_range` (`declperm.block_run`) or the leading run of up to `max_lines` declarations. From the
    source's own order, every order one move away (`declperm.neighbours`: one line moved to another position,
    dependencies kept) is compiled as a probe in a fixed order; the first that beats the current order by `merit`
    (the function's official score, then its first divergence, then the unit's matched bytes) becomes the current
    order and the step restarts from it. It stops at 100 % (`stopped` `100%`), when no neighbour is better
    (`local-optimum`) or after `max_evals` probes (`max-evals`). The real source is only read; `path` is the accepted
    orders, the source's own first. `out(line)` reports each accepted step."""
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    rec = {"function": function, "status": "ok", "run": [], "range": list(line_range) if line_range else None,
           "tried": 0, "path": [], "best": None, "best_order": None, "stopped": None}
    try:
        run = find_block(src, function, max_lines, initialised, line_range)
    except ValueError as exc:
        rec.update(status="ambiguous", error=str(exc))
        return rec
    if run is None:
        rec["status"] = "no-run"
        return rec
    rec["run"] = [l.strip() for l in run.lines]
    identity = tuple(range(len(run.lines)))
    seen = {}

    def evaluate(order):
        name = AS_IS if order == identity else declperm.name_of(order)
        repls = None if order == identity else (lambda t, o=order: declperm.reorder(t, run, o))
        seen[order] = try_variant(unit, tokens, name, repls, function, score, diverge)
        return seen[order]

    def step(r, order):
        return {"name": r["name"], "percent": r.get("symbol_percent"), "first_divergence": r.get("first_divergence"),
                "matched_bytes": r["matched_bytes"], "lines": [run.lines[i].strip() for i in order]}

    cur_order, cur = identity, evaluate(identity)
    if cur["status"] != "ok":
        rec.update(status="no-result", stopped="the source's own order did not score: %s" % describe(cur))
        return rec
    rec["path"].append(step(cur, cur_order))
    if out:
        out("  start  %s" % _step_line(rec["path"][-1]))
    stopped = None
    while stopped is None:
        if (cur.get("symbol_percent") or 0.0) >= 100.0:
            stopped = "100%"
            break
        moved = False
        for cand in declperm.neighbours(cur_order, run.deps):
            if cand in seen:
                continue
            if len(seen) - 1 >= max_evals:
                stopped = "max-evals"
                break
            r = evaluate(cand)
            if r["status"] == "ok" and merit(r, function) > merit(cur, function):
                cur_order, cur, moved = cand, r, True
                rec["path"].append(step(r, cand))
                if out:
                    out("  step %-2d %s" % (len(rec["path"]) - 1, _step_line(rec["path"][-1])))
                break
        if stopped is None and not moved:
            stopped = "local-optimum"
    rec.update(tried=len(seen) - 1, stopped=stopped)
    if cur_order == identity:
        rec["status"] = "no-gain"
    else:
        rec.update(best=cur["name"], best_order=list(cur_order))
    return rec


def _step_line(row) -> str:
    d = row["first_divergence"]
    return "%-20s %8s  div %-5s %10.2f B  %s" % (
        row["name"], "%.2f%%" % row["percent"] if row["percent"] is not None else "-",
        "full" if row["percent"] and row["percent"] >= 100.0 else d, row["matched_bytes"], " | ".join(row["lines"]))


def print_hill(rec) -> None:
    fn = rec["function"]
    if rec["status"] in ("ambiguous", "no-run", "no-result"):
        print("%s: %s" % (fn, rec.get("error") or rec.get("stopped") or "no run of two or more declarations"))
        return
    print("%s: %d declaration(s)%s, %d order(s) tried, %d accepted step(s), stopped: %s" % (
        fn, len(rec["run"]), " (lines %d-%d)" % tuple(rec["range"]) if rec["range"] else "", rec["tried"],
        len(rec["path"]) - 1, rec["stopped"]))
    first, last = rec["path"][0], rec["path"][-1]
    print("  from %s" % _step_line(first))
    print("  to   %s" % _step_line(last))
    if rec["status"] != "no-gain":
        verdict = "best is %s" % rec["best"]
    elif rec["stopped"] == "max-evals":
        verdict = "no better order among the %d tried (--max-evals)" % rec["tried"]
    else:
        verdict = "no order one move away beats the source's own"
    print("  verdict: %s" % verdict)


def run_hill(args, unit, tokens, root, score=None, diverge=None):
    """`--hill`: climb each function's block, print the path, and with `--apply` write the best orders."""
    records = []
    for fn in [f for f in args.hill.split(",") if f]:
        rec = hill_one(unit, tokens, fn, getattr(args, "lines", None), max(args.max_lines, 64), args.max_evals,
                       score=score, diverge=diverge, out=None if args.json else print,
                       initialised=not getattr(args, "plain_only", False))
        records.append(rec)
        if not args.json:
            print_hill(rec)
    applied = None
    if args.apply:
        applied = write_source(unit, lambda lf: permdecl_apply(lf, records, max(args.max_lines, 64),
                                                               not getattr(args, "plain_only", False)), root)
    if args.json:
        print(json.dumps({"unit": unit.report_name, "functions": records, "applied": applied}, indent=2))
    elif args.apply:
        print("applied the best orders to %s - rebuild and re-measure" % applied if applied
              else "nothing to apply: the source's own orders are the best found")
    elif any(r.get("best") for r in records):
        print("source untouched; --apply writes the best order of each function")
    return 0


def best_to_apply(results, symbol=None):
    """The variant result `--apply-best` writes: the top of the ranking when it beats the as-is source by `merit`
    (a tie keeps the source), else None."""
    ranked = rank(results, symbol)
    as_is = next((r for r in ranked if r["name"] == AS_IS), None)
    if not ranked or ranked[0]["name"] == AS_IS:
        return None
    if as_is is not None and merit(ranked[0], symbol) <= merit(as_is, symbol):
        return None
    return ranked[0]


def pending_scorers(unit, rename_map, root):
    """`(score, diverge, outcome, path)` scoring against a copy of the target with the pending map edits of
    `rename_map` applied (`lib.pendingmap`), written under `build/tmp/tryvar/`."""
    ops = pendingmap.load(rename_map)
    path = os.path.join(str(repo.scratch("tryvar", root)), unit.key.replace("/", "_") + ".pending.o")
    outcome = pendingmap.apply_file(unit.obj_target, path, ops)
    return ((lambda obj, _tgt: match_pcts(obj, path, None, root)),
            (lambda obj, _tgt, sym: first_divergence(obj, path, sym, root)), outcome, path)


def _line_range(value):
    try:
        first, last = (int(x) for x in value.split("-", 1))
    except ValueError:
        raise argparse.ArgumentTypeError("--lines wants A-B (1-based, inclusive): %r" % value) from None
    return first, last


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("names", nargs="*", help="variant names (default: all of them)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--variants", help="variant file: a .py with VARIANTS, a .json spec ({name: [[old, new], ...]}), "
                                       "or a text file of `//@@ old` / `//@@ variant <name>` blocks "
                                       "(default: tools/flags/variants/<lib>.py)")
    ap.add_argument("--permute", metavar="FILE",
                    help="a text file of `//@@ item` blocks: try every other order of that contiguous run")
    ap.add_argument("--max-variants", type=int, default=120, help="refuse a permutation set larger than this")
    ap.add_argument("--symbol", help="rank by this function's score and first divergence")
    ap.add_argument("--json", action="store_true", help="the results and the ranking as JSON")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--list", action="store_true", help="list the variants and exit")
    ap.add_argument("--apply", metavar="NAME", nargs="?", const=True,
                    help="apply this variant's rewrite to the unit's real source and exit (with --permdecl or "
                         "--hill: write the best declaration order of each function, no name)")
    ap.add_argument("--apply-best", action="store_true",
                    help="run the variants, then write the top-ranked one into the real source when it beats the "
                         "as-is source (a tie keeps the source)")
    ap.add_argument("--rename-map", metavar="FILE",
                    help="score against the target with these pending map edits applied (`old new` and "
                         "`split <row> <offset> <new> [--scope S]` lines; lib.pendingmap)")
    ap.add_argument("--permdecl", metavar="FN[,FN...]",
                    help="permute each function's leading plain declarations, rank the orders by the "
                         "function's score and print the table (the source is only written with --apply)")
    ap.add_argument("--hill", metavar="FN[,FN...]",
                    help="hill-climb the order of each function's declaration block (one line moved per step), "
                         "for blocks too long to enumerate (the source is only written with --apply)")
    ap.add_argument("--lines", type=_line_range, metavar="A-B",
                    help="--permdecl/--hill: the block is these source lines (1-based, inclusive; every one a "
                         "declaration in the function's body) instead of the leading run")
    ap.add_argument("--max-evals", type=int, default=200,
                    help="--hill: compile at most this many orders per function (default %(default)s)")
    ap.add_argument("--max-lines", type=int, default=declperm.DEFAULT_MAX_LINES,
                    help="--permdecl: permute at most this many leading declarations (default %(default)s; "
                         "--hill reads at least 64)")
    ap.add_argument("--max-perms", type=int, default=120,
                    help="--permdecl: compile at most this many orders per function; more are a fixed-seed "
                         "sample (default %(default)s)")
    ap.add_argument("--plain-only", action="store_true",
                    help="--permdecl: permute only declarations without an initialiser (initialised ones are "
                         "otherwise members of the run, kept after the declarations their initialiser uses)")
    ap.add_argument("--seed", type=int, default=0, help="--permdecl: the sample's seed (default %(default)s)")
    args = ap.parse_args(argv)

    root = repo.repo_root()
    unit = units.Unit.resolve(args.unit, root)
    if args.permdecl and args.hill:
        raise SystemExit("--permdecl and --hill are two searches: pick one")
    if args.lines and len([f for f in (args.permdecl or args.hill or "").split(",") if f]) != 1:
        raise SystemExit("--lines names one block: give exactly one function with --permdecl or --hill")
    score = diverge = None
    if args.rename_map:
        if not os.path.exists(unit.obj_target):
            raise SystemExit("no target object at %s - split the unit first" % unit.obj_target)
        try:
            score, diverge, outcome, pending = pending_scorers(unit, args.rename_map, root)
        except (OSError, ValueError) as exc:
            raise SystemExit("--rename-map %s: %s" % (args.rename_map, exc))
        if not args.json:
            print("pending map %s: %s  (scored against %s)" % (args.rename_map, outcome.summary(),
                                                               os.path.relpath(pending, root)))
        for line, why in outcome.refused:
            print("  REFUSED line %s: %s" % (line, why), file=sys.stderr)
        if outcome.refused:
            return 1
    if args.permdecl or args.hill:
        if not os.path.exists(unit.obj_target):
            raise SystemExit("no target object at %s - split the unit first" % unit.obj_target)
        head, flags, tail = units.split_command(unit)
        tokens = head + units.override_flags(flags, args.flags_extra) + tail
        if args.hill:
            return run_hill(args, unit, tokens, root, score, diverge)
        return run_permdecl(args, unit, tokens, root, score, diverge)
    if args.apply is True:
        raise SystemExit("--apply takes a variant name (or comes with --permdecl/--hill; --apply-best runs and "
                         "applies the winner)")
    if args.permute:
        with open(args.permute, encoding="utf-8", newline="") as fh:
            variants = permutation_variants(fh.read().replace("\r\n", "\n"), args.max_variants)
        path = args.permute
    else:
        path = args.variants or default_variants_path(unit)
        if not os.path.exists(path):
            raise SystemExit("no variant file at %s (pass --variants)" % os.path.relpath(path, root))
        variants = load_variants(path)
    marker_mode = bool(args.permute) or not path.endswith(".py")
    if args.list:
        for name, _ in variants:
            print(name)
        return 0
    if args.apply:
        sel = [(n, r) for n, r in variants if n == args.apply]
        if not sel:
            raise SystemExit("no variant named %r in %s" % (args.apply, os.path.relpath(path, root)))
        return _apply_named(unit, args.apply, sel[0][1], root)

    if not os.path.exists(unit.obj_target):
        raise SystemExit("no target object at %s - split the unit first" % unit.obj_target)

    head, flags, tail = units.split_command(unit)
    tokens = head + units.override_flags(flags, args.flags_extra) + tail
    selected = [(n, r) for n, r in variants if not args.names or n in args.names]
    emit = None if args.json else print
    if emit:
        print("unit %s   variants from %s   (%d/%d selected)"
              % (unit.report_name, os.path.relpath(path, root), len(selected), len(variants)))
        print("%-26s %s" % ("variant", "result"))
    results = try_all(unit, tokens, selected, args.symbol, score, diverge,
                      as_is=marker_mode or bool(args.symbol) or args.apply_best, out=emit)
    ranked = rank(results, args.symbol)
    if args.json:
        print(json.dumps({"unit": unit.report_name, "variants": os.path.relpath(path, root), "symbol": args.symbol,
                          "results": results, "ranking": [r["name"] for r in ranked]}, indent=2))
    elif marker_mode or args.symbol or args.apply_best:
        print_ranking(ranked, args.symbol)
    if args.apply_best:
        best = best_to_apply(results, args.symbol)
        if best is None:
            print("--apply-best: nothing written - no variant beats the as-is source", file=sys.stderr
                  if args.json else sys.stdout)
            return 0
        return _apply_named(unit, best["name"], dict(selected)[best["name"]], root)
    return 0


def _apply_named(unit, name, repls, root):
    """Write variant `name`'s rewrite into the unit's real source (CRLF kept); refuse one that does not apply or
    changes nothing."""
    before = os.path.getsize(unit.source)
    failure = []

    def transform(lf):
        new, why = rewrite(lf, repls)
        if new is None:
            failure.append(why)
        return new

    applied = write_source(unit, transform, root)
    if failure:
        raise SystemExit("variant %r did not apply cleanly (%s) - nothing written" % (name, failure[0]))
    if applied is None:
        raise SystemExit("variant %r is a no-op for the current source - nothing written" % name)
    print("applied %r to %s (%+d bytes)" % (name, applied, os.path.getsize(unit.source) - before))
    print("now rebuild and re-measure:")
    print("  ninja %s" % os.path.relpath(unit.obj_ours, root).replace(os.sep, "/"))
    print("  python tools/units/measure.py %s" % unit.key)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
