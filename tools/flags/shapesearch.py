#!/usr/bin/env python3
"""Search *source shapes* of one function for the codegen the original object has.

This is the source-side twin of `tools/flags/tryvar.py`: where tryvar varies the compiler flags, this
varies the source - declaration order and types, named temporaries, casts and signedness, statement
order, compound assignment vs assignment, field form vs pointer arithmetic, dead copies, the switch tail
and `default`-first shapes, condition/branch form, ternaries and loop shape. It exists because the
residual on a near-matching unit is almost always *codegen* (allocator web order, a branch direction, a
register colouring) and finding it by hand cost the earlier sessions hundreds of hand-written variants
per function.

The loop is the one tryvar already uses for flags, lifted to the source:

    generate variants -> compile each with the unit's *real* ninja command line -> score each with
    objdiff's official report metric -> deduplicate identical objects -> rank

Nothing here writes to the repository's source: every candidate is compiled from a scratch copy under
`build/tmp/shapes/<run>/`, and only a single object is compiled (no link, no `ninja` build edge). The
score is `unitutil.report_functions` - `report generate`'s `fuzzy_match_percent`, the number
`build/RMHE08/report.json`, `ledger.py` and `land.py` read - not objdiff `diff`'s positional value.

Usage:
    python tools/flags/shapesearch.py -u Pl/pl_act                     # worst function, all generators
    python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48
    python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --gens switch,cond --depth 2
    python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --top 10 --jobs 8
    python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --emit <label>   # dump the winner
    python tools/flags/shapesearch.py --list-gens

Ranking is by the target function's official score; the table also shows the unit mean and the number of
functions the variant regressed, so a shape that fixes the function by breaking its neighbours is not
mistaken for a win.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import difflib

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu
import shapes as sh

SCRATCH = os.path.join(uu.ROOT, "build", "tmp", "shapes")


# --------------------------------------------------------------------------------------------------
# compile + score, in a worker thread
# --------------------------------------------------------------------------------------------------

def compile_tokens(unit):
    """The unit's real ninja command line, with `-MMD` dropped (the probe writes no dep file)."""
    head, flags, tail = uu.split_flags(uu.compile_command(unit))
    tail = [t for t in tail if t != "-MMD"]
    return head + flags + tail


def _run_one(job):
    """Compile one candidate source and score it. Returns a result dict (never raises)."""
    idx, tokens, src_text, probe_dir, target, symbol, unit_name, base_scores, ext = job
    try:
        os.makedirs(probe_dir, exist_ok=True)
        probe_src = os.path.join(probe_dir, "shapes_probe" + ext)
        with open(probe_src, "w", encoding="utf-8", errors="surrogateescape") as fh:
            fh.write(src_text)
        toks = list(tokens)
        toks[toks.index("-c") + 1] = probe_src
        toks[toks.index("-o") + 1] = probe_dir
        p = subprocess.run(toks, cwd=uu.ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
        obj = os.path.join(probe_dir, "shapes_probe.o")
        if p.returncode != 0 or not os.path.exists(obj):
            return {"idx": idx, "error": uu.quiet((p.stdout or "") + (p.stderr or ""))[:300]}
        raw = open(obj, "rb").read()
        ohash = hashlib.sha1(raw).hexdigest()
        entries = uu.report_functions(target, obj, unit_name=unit_name, tmpdir=probe_dir)
        if "_error" in entries:
            return {"idx": idx, "error": entries["_error"][:300]}
        pcts = {n: e.get("fuzzy_match_percent") for n, e in entries.items()
                if isinstance(e.get("fuzzy_match_percent"), (int, float))}
        fpct = pcts.get(symbol)
        mean = sum(pcts.values()) / len(pcts) if pcts else 0.0
        regressed = sum(1 for n, v in pcts.items()
                        if n in base_scores and v < base_scores[n] - 0.005)
        improved = sum(1 for n, v in pcts.items()
                       if n in base_scores and v > base_scores[n] + 0.005)
        size = None
        try:
            size = uu.text_size(obj)
        except Exception:
            pass
        return {"idx": idx, "func_pct": fpct, "unit_mean": mean, "regressed": regressed,
                "improved": improved, "size": size, "objhash": ohash, "obj": obj}
    except Exception as exc:                      # a broken candidate must not kill the run
        return {"idx": idx, "error": "%s: %s" % (type(exc).__name__, exc)}


# --------------------------------------------------------------------------------------------------
# candidate construction
# --------------------------------------------------------------------------------------------------

def apply_body(src, func, new_body):
    """Replace the function's body in the full unit source."""
    return src[:func[1] + 1] + new_body + src[func[2]:]


def build_level(parents, gens, per_gen, round_robin=True):
    """[(label, body)] for one search level, deduped by normalised source text.

    `parents` is a list of (parent_label, body); the emitted label is `parent|gen:name` so a composed
    shape keeps its history.
    """
    out = []
    seen = set()
    per_parent = []
    for plabel, body in parents:
        gen_out = []
        for g in gens:
            fn = sh.GENERATORS.get(g)
            if fn is None:
                continue
            for name, new in fn(body)[:per_gen]:
                label = "%s|%s:%s" % (plabel, g, name) if plabel else "%s:%s" % (g, name)
                gen_out.append((label, new))
        per_parent.append(gen_out)
    if round_robin:
        max_len = max((len(x) for x in per_parent), default=0)
        merged = []
        for k in range(max_len):
            for x in per_parent:
                if k < len(x):
                    merged.append(x[k])
    else:
        merged = [v for x in per_parent for v in x]
    for label, new in merged:
        key = sh.norm_code(new)
        if key in seen:
            continue
        seen.add(key)
        out.append((label, new))
    return out


def run_batch(unit, tokens, candidates, symbol, base_scores, run_id, jobs, verbose=False):
    """Compile+score every (label, body) candidate. Returns the result dicts in order."""
    src = open(unit.src, encoding="utf-8", errors="surrogateescape").read()
    func = sh.find_definition(src, symbol)
    if func is None:
        raise SystemExit("cannot find a definition of %r in %s" % (symbol, unit.src))
    ext = os.path.splitext(unit.src)[1]
    jobs_list = []
    for i, (label, body) in enumerate(candidates):
        src_text = apply_body(src, func, body)
        probe_dir = os.path.join(SCRATCH, run_id, "v%05d" % i)
        jobs_list.append((i, tokens, src_text, probe_dir, unit.target, symbol,
                          unit.name, base_scores, ext))
    results = [None] * len(jobs_list)
    t0 = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        for r in pool.map(_run_one, jobs_list):
            results[r["idx"]] = r
    elapsed = time.time() - t0
    if verbose:
        print("    %d candidates in %.1fs (%.2fs/candidate)" % (len(candidates), elapsed,
                                                                elapsed / max(1, len(candidates))))
    return results, elapsed


# --------------------------------------------------------------------------------------------------
# reporting
# --------------------------------------------------------------------------------------------------

def probe_rows(target, obj, symbol, unit_name, tmpdir):
    """Side-by-side rows for one probe object, or None. Rows are (kind, target_fmt, ours_fmt)."""
    os.makedirs(tmpdir, exist_ok=True)
    proj = uu.measure_project(target, obj, unit_name, tmpdir)
    out = os.path.join(tmpdir, "shapes_diff.json")
    p = subprocess.run([uu.OBJDIFF, "diff", "-p", proj, "-u", unit_name or "measure", symbol,
                        "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
                       cwd=uu.ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return None
    d = json.load(open(out, encoding="utf-8"))
    def get(side):
        for e in d[side].get("symbols") or []:
            if e.get("name") == symbol:
                return e
        return None
    left, right = get("left"), get("right")
    if not left or not right:
        return None
    rows = []
    li = left.get("instructions") or []
    ri = right.get("instructions") or []
    for i in range(max(len(li), len(ri))):
        l = li[i] if i < len(li) else {}
        r = ri[i] if i < len(ri) else {}
        kind = r.get("diff_kind") or l.get("diff_kind") or ""
        lf = (l.get("instruction") or {}).get("formatted", "")
        rf = (r.get("instruction") or {}).get("formatted", "")
        rows.append((kind, lf, rf))
    return rows


def first_divergence(rows):
    for i, (kind, lf, rf) in enumerate(rows or []):
        if kind:
            return i, kind, lf, rf
    return None


def summarize_diff(target, obj, symbol, unit_name, tmpdir, width=3):
    rows = probe_rows(target, obj, symbol, unit_name, tmpdir)
    fd = first_divergence(rows)
    if fd is None:
        return "MATCH (no differing row)"
    i, kind, lf, rf = fd
    lo = max(0, i - 1)
    hi = min(len(rows), i + width)
    lines = []
    for j in range(lo, hi):
        k, l, r = rows[j]
        mark = ">>" if k else "  "
        lines.append("%s %3d %-28s | %-28s %s" % (mark, j, l, r, k))
    n_diff = sum(1 for k, _, _ in rows if k)
    return "%d differing rows, first at %d (%s)\n%s" % (n_diff, i, kind, "\n".join(lines))


def unified_body_diff(old_body, new_body, name):
    a = [l + "\n" for l in old_body.splitlines()]
    b = [l + "\n" for l in new_body.splitlines()]
    return "".join(difflib.unified_diff(a, b, fromfile="original", tofile=name, n=2))


def worst_function(unit, base_scores):
    """The lowest-scoring function in the unit (target object order)."""
    bad = sorted((v, k) for k, v in base_scores.items() if v < 100.0)
    return bad[0][1] if bad else None


# --------------------------------------------------------------------------------------------------

def search_function(unit, tokens, src, symbol, gens, depth, beam, max_c, per_gen, jobs,
                    run_id, base_scores, verbose=True):
    """Run the shape search for one function. Returns a result dict."""
    func = sh.find_definition(src, symbol)
    if func is None:
        return {"symbol": symbol, "error": "no definition in %s" % os.path.relpath(unit.src, uu.ROOT)}
    base_body = src[func[1] + 1:func[2]]
    base_pct = base_scores[symbol]
    if verbose:
        print("function %s   baseline %.3f%%   unit mean %.3f%%"
              % (symbol, base_pct, sum(base_scores.values()) / len(base_scores)))
        print("generators: %s   depth %d   jobs %d" % (",".join(gens), depth, jobs))
        print()
    parents = [("", base_body)]
    all_results = []
    total_cands = 0
    total_time = 0.0
    seen_obj = {}
    for level in range(1, depth + 1):
        cands = build_level(parents, gens, per_gen)
        if max_c and len(cands) > max_c:
            cands = cands[:max_c]
        total_cands += len(cands)
        if verbose:
            print("level %d: %d candidates" % (level, len(cands)))
        results, elapsed = run_batch(unit, tokens, cands, symbol, base_scores, run_id, jobs)
        total_time += elapsed
        kept = []
        for (label, body), r in zip(cands, results):
            if "error" in r:
                kept.append((label, body, r))
                continue
            h = r["objhash"]
            if h in seen_obj:
                continue
            seen_obj[h] = label
            kept.append((label, body, r))
        all_results.extend(kept)
        if level < depth:
            ranked = sorted([k for k in kept if "error" not in k[2] and k[2].get("func_pct") is not None],
                            key=lambda k: (k[2]["func_pct"], k[2]["unit_mean"]), reverse=True)
            parents = [(k[0], k[1]) for k in ranked[:beam]]
            if not parents:
                break
    ok = sorted([k for k in all_results if "error" not in k[2] and k[2].get("func_pct") is not None],
                key=lambda k: (k[2]["func_pct"], k[2]["unit_mean"]), reverse=True)
    errs = [k for k in all_results if "error" in k[2]]
    return {"symbol": symbol, "base_pct": base_pct, "base_body": base_body, "func": func,
            "ok": ok, "errs": errs, "cands": total_cands, "time": total_time,
            "distinct": len(seen_obj)}


def print_table(unit, res, run_id, top):
    base_pct = res["base_pct"]
    print()
    print("%-4s %-8s %-7s %-6s %-4s %-4s %-9s %s"
          % ("rank", "func%", "delta", "unit%", "reg", "imp", "size", "variant"))
    for n, (label, body, r) in enumerate(res["ok"][:top], 1):
        print("%-4d %-8.3f %+-7.3f %-6.3f %-4d %-4d %-9s %s"
              % (n, r["func_pct"], r["func_pct"] - base_pct, r["unit_mean"], r["regressed"],
                 r["improved"], r.get("size"), label))
    if res["errs"]:
        print("\n%d candidates failed to compile (e.g. %s)" % (len(res["errs"]), res["errs"][0][0]))
    best = res["ok"][0] if res["ok"] else None
    if best and best[2]["func_pct"] > base_pct + 0.001:
        label, body, r = best
        print("\nbest: %s  %.3f%% (+%.3f)  unit mean %.3f%%"
              % (label, r["func_pct"], r["func_pct"] - base_pct, r["unit_mean"]))
    elif best:
        print("\nno candidate beat the baseline (best %.3f%% at %s)" % (best[2]["func_pct"], best[0]))
    if best:
        print(summarize_diff(unit.target, best[2]["obj"], res["symbol"], unit.name,
                             os.path.join(SCRATCH, run_id, "diff_best")))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-u", "--unit", help="unit spec (default: the only unit with source)")
    ap.add_argument("-f", "--function", help="source name of the function (default: the worst one)")
    ap.add_argument("--gens", help="comma-separated generator names (default: all)")
    ap.add_argument("--depth", type=int, default=1, help="search depth / beam rounds (default 1)")
    ap.add_argument("--beam", type=int, default=12, help="parents kept per round for depth > 1")
    ap.add_argument("--max", type=int, default=4000, help="cap on candidates per level")
    ap.add_argument("--per-gen", type=int, default=400, help="cap on variants per generator")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 1))
    ap.add_argument("--top", type=int, default=12, help="rows to print (default 12)")
    ap.add_argument("--scan", type=int, metavar="N",
                    help="search the N worst functions and print one summary line each")
    ap.add_argument("--emit", metavar="LABEL", help="write that candidate's full source + diff, then exit")
    ap.add_argument("--list-gens", action="store_true")
    args = ap.parse_args()

    if args.list_gens:
        for g in sh.DEFAULT_ORDER:
            print("%-12s %s" % (g, (sh.GENERATORS[g].__doc__ or "").splitlines()[0]))
        return

    unit = uu.resolve_unit(args.unit)
    if not os.path.exists(unit.target):
        raise SystemExit("no target object at %s - split the unit first" % unit.target)
    tokens = compile_tokens(unit)
    src = open(unit.src, encoding="utf-8", errors="surrogateescape").read()
    ext = os.path.splitext(unit.src)[1]
    run_id = "run_%s_%d" % (re.sub(r"\W", "_", unit.file), int(time.time()))

    # baseline: the unmodified source compiled through the probe path (also the unit's function list)
    base_dir = os.path.join(SCRATCH, run_id, "base")
    base = _run_one((0, tokens, src, base_dir, unit.target, "", unit.name, {}, ext))
    if "error" in base:
        raise SystemExit("baseline compile failed:\n" + base["error"])
    base_entries = uu.report_functions(unit.target, base["obj"], unit_name=unit.name, tmpdir=base_dir)
    base_scores = {n: e.get("fuzzy_match_percent") for n, e in base_entries.items()
                   if isinstance(e.get("fuzzy_match_percent"), (int, float))}
    base_mean = sum(base_scores.values()) / len(base_scores) if base_scores else 0.0

    gens = [g.strip() for g in args.gens.split(",")] if args.gens else sh.DEFAULT_ORDER
    for g in gens:
        if g not in sh.GENERATORS:
            raise SystemExit("unknown generator %r (see --list-gens)" % g)

    print("unit %s   functions %d   unit mean %.3f%%   scratch %s"
          % (unit.name, len(base_scores), base_mean, os.path.relpath(os.path.join(SCRATCH, run_id), uu.ROOT)))

    if args.scan:
        worst = sorted((v, k) for k, v in base_scores.items() if v < 100.0)[:args.scan]
        if not worst:
            raise SystemExit("every function already scores 100%%")
        print("scanning %d function(s), depth %d, %s\n" % (len(worst), args.depth, ",".join(gens)))
        print("%-44s %-8s %-8s %-8s %s" % ("function", "base", "best", "delta", "variant"))
        t0 = time.time()
        wins = 0
        winners = []            # (start offset, end offset, body, symbol, label)
        for _pct, sym in worst:
            res = search_function(unit, tokens, src, sym, gens, args.depth, args.beam, args.max,
                                  args.per_gen, args.jobs, os.path.join(run_id, re.sub(r"\W", "_", sym)),
                                  base_scores, verbose=False)
            if "error" in res:
                print("%-44s %s" % (sym, res["error"]))
                continue
            best = res["ok"][0] if res["ok"] else None
            if best:
                label, body, r = best
                d = r["func_pct"] - res["base_pct"]
                if d > 0.001:
                    wins += 1
                    winners.append((res["func"][1] + 1, res["func"][2], body, sym, label))
                print("%-44s %-8.3f %-8.3f %+-8.3f %s"
                      % (sym, res["base_pct"], r["func_pct"], d, label))
        print("\n%d/%d improved" % (wins, len(worst)))
        if winners:
            # apply every winning body at once (right to left, so the offsets stay valid) and score the
            # combined source - the number a worker would get by landing all of them
            combined = src
            for lo, hi, body, _sym, _label in sorted(winners, key=lambda w: w[0], reverse=True):
                combined = combined[:lo] + body + combined[hi:]
            cdir = os.path.join(SCRATCH, run_id, "combined")
            r = _run_one((0, tokens, combined, cdir, unit.target, "", unit.name, {}, ext))
            if "error" in r:
                print("combined source did not compile: %s" % r["error"])
            else:
                ent = uu.report_functions(unit.target, r["obj"], unit_name=unit.name, tmpdir=cdir)
                sc = {n: e.get("fuzzy_match_percent") for n, e in ent.items()
                      if isinstance(e.get("fuzzy_match_percent"), (int, float))}
                cm = sum(sc.values()) / len(sc) if sc else 0.0
                exact = sum(1 for v in sc.values() if v >= 100.0)
                print("combined: unit mean %.3f%% (was %.3f%%, %+.3f), %d/%d functions at 100%%"
                      % (cm, base_mean, cm - base_mean, exact, len(sc)))
        print("%.1fs wall" % (time.time() - t0))
        return

    symbol = args.function or worst_function(unit, base_scores)
    if symbol is None:
        raise SystemExit("every function in %s already scores 100%%" % unit.name)
    if symbol not in base_scores:
        raise SystemExit("symbol %r is not in the target object's report (renamed? wrong unit?)" % symbol)
    fdir = os.path.join(run_id, re.sub(r"\W", "_", symbol))
    res = search_function(unit, tokens, src, symbol, gens, args.depth, args.beam, args.max,
                          args.per_gen, args.jobs, fdir, base_scores, verbose=True)
    if "error" in res:
        raise SystemExit(res["error"])
    print_table(unit, res, fdir, args.top)
    print("\n%d candidates scored, %d distinct objects, %.1fs wall"
          % (res["cands"], res["distinct"], res["time"]))

    if args.emit:
        sel = [k for k in res["ok"] if k[0] == args.emit]
        if not sel:
            raise SystemExit("no candidate labelled %r" % args.emit)
        label, body, _r = sel[0]
        out = os.path.join(SCRATCH, fdir, "winner_%s%s" % (re.sub(r"\W", "_", label), ext))
        full = apply_body(src, res["func"], body)
        with open(out, "w", encoding="utf-8", errors="surrogateescape", newline="") as fh:
            fh.write(full)
        print("\n%s\n" % out)
        print(unified_body_diff(res["base_body"], body, label))


if __name__ == "__main__":
    main()
