#!/usr/bin/env python3
"""Search *source shapes* of one function for the codegen the original object has. Spec: docs/tools/spec/shapesearch.md.
CLI: python tools/flags/shapesearch.py -u <unit> [-f <symbol>] [--gens G] [--depth N] [--expr E | --expr-file F] [--emit L] | --list-gens."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
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

from tools.flags import shapes as sh
from tools.lib import repo, report, units

#: The tree a run reads and its scratch directory (`build/tmp/shapes/`), bound by `bind()` in `main()`.
ROOT = None
SCRATCH = None


def bind(root):
    """Point the run at `root`: probe compiles run there and scratch lives under its `build/tmp/shapes/`."""
    global ROOT, SCRATCH
    ROOT, SCRATCH = root, os.path.join(root, "build", "tmp", "shapes")


def report_functions(target, obj, unit_name=None, tmpdir=None):
    """`lib.report.score_entries` for one probe object of the bound tree."""
    return report.score_entries(target, obj, unit_name, tmpdir, objdiff=report.objdiff_cli(ROOT), cwd=ROOT)


# --------------------------------------------------------------------------------------------------
# compile + score, in a worker thread
# --------------------------------------------------------------------------------------------------

def compile_tokens(unit):
    """The unit's real ninja command line, with `-MMD` dropped (the probe writes no dep file)."""
    head, flags, tail = units.split_command(unit)
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
        p = subprocess.run(toks, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
        obj = os.path.join(probe_dir, "shapes_probe.o")
        if p.returncode != 0 or not os.path.exists(obj):
            return {"idx": idx, "error": units.quiet((p.stdout or "") + (p.stderr or ""))[:300]}
        raw = open(obj, "rb").read()
        ohash = hashlib.sha1(raw).hexdigest()
        entries = report_functions(target, obj, unit_name=unit_name, tmpdir=probe_dir)
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
            size = units.text_size(obj)
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
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    func = sh.find_definition(src, symbol)
    if func is None:
        raise SystemExit("cannot find a definition of %r in %s" % (symbol, unit.source))
    ext = os.path.splitext(unit.source)[1]
    jobs_list = []
    for i, (label, body) in enumerate(candidates):
        src_text = apply_body(src, func, body)
        probe_dir = os.path.join(SCRATCH, run_id, "v%05d" % i)
        jobs_list.append((i, tokens, src_text, probe_dir, unit.obj_target, symbol,
                          unit.report_name, base_scores, ext))
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
    proj = report.write_project(target, obj, unit_name, tmpdir)
    out = os.path.join(tmpdir, "shapes_diff.json")
    p = subprocess.run([report.objdiff_cli(ROOT), "diff", "-p", proj, "-u", unit_name or "measure", symbol,
                        "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
                       cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
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
        return {"symbol": symbol, "error": "no definition in %s" % os.path.relpath(unit.source, ROOT)}
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
        print(summarize_diff(unit.obj_target, best[2]["obj"], res["symbol"], unit.report_name,
                             os.path.join(SCRATCH, run_id, "diff_best")))


def expr_candidates(exprs, path=None):
    """`[(label, body)]` from repeated `--expr` and/or an `--expr-file`, in the order given.

    Each candidate is a **function body** substituted exactly where a generator's output would go
    (`apply_body`), so it is compiled with the unit's real cflags and scored by `run_batch` - no second
    compile/score path. A file separates candidates with a line of three or more dashes; without one,
    each non-empty line that is not a `#` comment is a candidate. Candidates are deduped by
    `shapes.norm_code` (comment/whitespace), the same key `build_level` uses, so a body written twice is
    compiled once.
    """
    out = [("expr#%d" % i, e) for i, e in enumerate(exprs or [], 1)]
    if path:
        try:
            text = open(path, encoding="utf-8", errors="surrogateescape").read()
        except OSError as exc:
            raise SystemExit("cannot read --expr-file %s: %s" % (path, exc))
        if re.search(r"(?m)^\s*-{3,}\s*$", text):
            parts = re.split(r"(?m)^\s*-{3,}\s*$", text)
            out += [("file#%d" % i, part) for i, part in enumerate(parts, 1) if part.strip()]
        else:
            lines = [ln for ln in text.splitlines() if ln.strip() and not ln.lstrip().startswith("#")]
            out += [("file#%d" % i, ln) for i, ln in enumerate(lines, 1)]
    seen, deduped = set(), []
    for label, body in out:
        key = sh.norm_code(body)
        if key in seen:
            continue
        seen.add(key)
        deduped.append((label, body))
    return deduped


def divergence_line(rows):
    """One line naming a probe's first differing row, or the MATCH answer, from `probe_rows` output."""
    fd = first_divergence(rows)
    if fd is None:
        return "MATCH (no differing row)"
    i, kind, lf, rf = fd
    return "first divergence at row %d (%s): target `%s` | ours `%s`" % (i, kind, lf, rf)


def divergence_of(target, obj, symbol, unit_name, tmpdir):
    """`divergence_line` for one compiled probe object (one `objdiff diff`, never a score)."""
    return divergence_line(probe_rows(target, obj, symbol, unit_name, tmpdir))


def build_parser():
    """The CLI. Split out of `main` so the selftest can assert a flag is wired (`--expr` was not)."""
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
    ap.add_argument("--expr", action="append", default=[], metavar="BODY",
                    help="a candidate function body (repeatable): compile, score and show its first "
                         "divergence, instead of generating shapes")
    ap.add_argument("--expr-file", metavar="PATH",
                    help="candidate bodies from a file (separated by a line of ---, else one per line)")
    ap.add_argument("--emit", metavar="LABEL", help="write that candidate's full source + diff, then exit")
    ap.add_argument("--list-gens", action="store_true")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    return ap


def expr_mode(unit, tokens, src, ext, sym, exprs, base_scores, run_id, jobs, emit):
    """Compile+score each hand-written candidate body, then print its score and first divergence."""
    edir = os.path.join(run_id, "expr_" + re.sub(r"\W", "_", sym))
    print("expr mode: %d candidate body(ies) for %s, baseline %.5f%%\n"
          % (len(exprs), sym, base_scores[sym]))
    results, _elapsed = run_batch(unit, tokens, exprs, sym, base_scores, edir, jobs, verbose=True)
    cand = list(zip(exprs, results))
    # a compiled candidate whose body matched nothing carries NO `fuzzy_match_percent` key (objdiff
    # omits it when nothing paired) - that is 0 %, not a candidate to drop, so it is shown and flagged
    ok = [c for c in cand if "error" not in c[1]]
    errs = [c for c in cand if "error" in c[1]]
    ok.sort(key=lambda c: (c[1].get("func_pct") or 0.0, c[1].get("unit_mean") or 0.0), reverse=True)

    def pct_of(r):
        p = r.get("func_pct")
        return 0.0 if p is None else p

    print("%-4s %-11s %-10s %-10s %-4s %-4s %-6s %s"
          % ("rank", "func%", "delta", "unit%", "reg", "imp", "size", "candidate"))
    for n, ((label, _body), r) in enumerate(ok, 1):
        flag = "  (no score reported - no instruction matched)" if r.get("func_pct") is None else ""
        print("%-4d %-11.5f %+-10.5f %-10.5f %-4d %-4d %-6s %s%s"
              % (n, pct_of(r), pct_of(r) - base_scores[sym], r.get("unit_mean") or 0.0,
                 r["regressed"], r["improved"], r.get("size"), label, flag))
    for (label, _body), r in errs:
        print("%-4s %s" % ("err", "%s: %s" % (label, (r.get("error") or "?").splitlines()[0])))
    print("\ncandidate                              func%       first divergence")
    for (label, body), r in ok:
        div = divergence_of(unit.obj_target, r["obj"], sym, unit.report_name,
                            os.path.join(SCRATCH, run_id, "expr_diff", re.sub(r"\W", "_", label)))
        print("%-38s %-11.5f %s" % (label, pct_of(r), div))
    if errs:
        print("\n%d candidate(s) failed to compile; their code is not a shape question yet" % len(errs))
    print("\n%d candidate body(ies) scored, %d distinct object(s)"
          % (len(ok), len({r["objhash"] for _c, r in ok})))
    if emit:
        sel = [(label, body, r) for (label, body), r in ok if label == emit]
        if not sel:
            raise SystemExit("no candidate labelled %r (labels: %s)"
                             % (emit, ", ".join(l for (_l, _b), _r in cand)))
        label, body, _r = sel[0]
        func = sh.find_definition(src, sym)
        out = os.path.join(SCRATCH, edir, "expr_%s%s" % (re.sub(r"\W", "_", label), ext))
        with open(out, "w", encoding="utf-8", errors="surrogateescape", newline="") as fh:
            fh.write(apply_body(src, func, body))
        print("\n%s\n" % out)
        print(unified_body_diff(src[func[1] + 1:func[2]], body, label))
    return 0


def main():
    ap = build_parser()
    args = ap.parse_args()

    if args.selftest:
        from tools.flags import shapesearch_selftest
        return shapesearch_selftest.selftest()

    if args.list_gens:
        for g in sh.DEFAULT_ORDER:
            print("%-12s %s" % (g, (sh.GENERATORS[g].__doc__ or "").splitlines()[0]))
        return

    bind(repo.repo_root())
    unit = units.Unit.resolve(args.unit, ROOT)
    if not os.path.exists(unit.obj_target):
        raise SystemExit("no target object at %s - split the unit first" % unit.obj_target)
    tokens = compile_tokens(unit)
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    ext = os.path.splitext(unit.source)[1]
    run_id = "run_%s_%d" % (re.sub(r"\W", "_", unit.file), int(time.time()))

    # baseline: the unmodified source compiled through the probe path (also the unit's function list)
    base_dir = os.path.join(SCRATCH, run_id, "base")
    base = _run_one((0, tokens, src, base_dir, unit.obj_target, "", unit.report_name, {}, ext))
    if "error" in base:
        raise SystemExit("baseline compile failed:\n" + base["error"])
    base_entries = report_functions(unit.obj_target, base["obj"], unit_name=unit.report_name, tmpdir=base_dir)
    base_scores = {n: e.get("fuzzy_match_percent") for n, e in base_entries.items()
                   if isinstance(e.get("fuzzy_match_percent"), (int, float))}
    base_mean = sum(base_scores.values()) / len(base_scores) if base_scores else 0.0

    gens = [g.strip() for g in args.gens.split(",")] if args.gens else sh.DEFAULT_ORDER
    for g in gens:
        if g not in sh.GENERATORS:
            raise SystemExit("unknown generator %r (see --list-gens)" % g)

    print("unit %s   functions %d   unit mean %.3f%%   scratch %s"
          % (unit.report_name, len(base_scores), base_mean, os.path.relpath(os.path.join(SCRATCH, run_id), ROOT)))

    exprs = expr_candidates(args.expr, args.expr_file)
    if exprs:
        symbol = args.function or worst_function(unit, base_scores)
        if symbol is None:
            raise SystemExit("every function in %s already scores 100%% - name one with -f" % unit.report_name)
        if symbol not in base_scores:
            raise SystemExit("symbol %r is not in the target object's report (renamed? wrong unit?)"
                             % symbol)
        return expr_mode(unit, tokens, src, ext, symbol, exprs, base_scores, run_id, args.jobs,
                         args.emit)

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
            r = _run_one((0, tokens, combined, cdir, unit.obj_target, "", unit.report_name, {}, ext))
            if "error" in r:
                print("combined source did not compile: %s" % r["error"])
            else:
                ent = report_functions(unit.obj_target, r["obj"], unit_name=unit.report_name, tmpdir=cdir)
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
        raise SystemExit("every function in %s already scores 100%%" % unit.report_name)
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
    # `sys.exit(main())`: `--selftest` returns non-zero on a failed check, which `--help`-only used to hide.
    sys.exit(main())
