#!/usr/bin/env python3
"""Try source rewrites of a unit and report the official per-function score of each. Spec: docs/tools/spec/tryvar.md.
CLI: python tools/flags/tryvar.py [-u <unit>] [<name>...] [--variants <file.py|file.txt>] [--permute <file.txt>] [--symbol S] [--max-variants N]
[--flags-extra "<flags>"] [--json] [--list] [--apply <name>]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import importlib.util
import itertools
import json
import os

from tools.lib import repo, report, text, units

#: The marker that starts a block in a text variants file (`//@@ old`, `//@@ variant <name>`, `//@@ permute`,
#: `//@@ item`).
MARKER = "//@@"
#: The name of the unmodified source in a ranking.
AS_IS = "(as-is)"


def scratch(unit):
    return os.path.join(unit.root, "build", "tmp", "probe")


def load_variants(path):
    """`[(name, repls)]` from a `.py` variants file (`VARIANTS = [...]`) or a text file of `//@@` blocks."""
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
    """`[(name, [(old, new)])]`: each `//@@ variant <name>` replaces the text of the latest `//@@ old` block."""
    out, old = [], None
    for kind, arg, body in marker_blocks(text_):
        if kind == "old":
            old = body
        elif kind == "variant":
            if old is None:
                raise SystemExit("tryvar: variant %r comes before any `%s old` block" % (arg, MARKER))
            out.append((arg or "variant-%d" % (len(out) + 1), [(old, body)]))
        else:
            raise SystemExit("tryvar: unknown block `%s %s` in a variants file" % (MARKER, kind))
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


def apply_variant(src, repls):
    if callable(repls):
        return repls(src)
    for old, new in repls:
        if old not in src:
            print("    (pattern not found: %r)" % old[:60])
            return None
        src = src.replace(old, new, 1)
    return src


def try_variant(unit, tokens, name, repls, symbol=None, score=None, diverge=None):
    """Compile one rewrite of the unit's source as a probe file next to it and score it -> a result dict.

    `status` is `ok`, `skip` (the rewrite did not apply), `compile-failed` or `no-result`. The probe is removed
    whatever happens; the real source is only read. `score(probe_obj, target_obj) -> {fn: (pct, ours, target)}`
    and `diverge(probe_obj, target_obj, symbol)` default to the official report metric and objdiff's diff."""
    score = score or (lambda obj, tgt: match_pcts(obj, tgt, None, unit.root))
    diverge = diverge or (lambda obj, tgt, sym: first_divergence(obj, tgt, sym, unit.root))
    src = open(unit.source, encoding="utf-8", errors="surrogateescape").read()
    res = {"name": name, "status": "ok", "functions": {}, "text_size": None, "log": ""}
    new = src if repls is None else apply_variant(src, repls)
    if new is None:
        res["status"] = "skip"
        return res
    ext = os.path.splitext(unit.source)[1]
    probe_src = os.path.join(os.path.dirname(unit.source), unit.file + "_probe" + ext)
    with open(probe_src, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(new)
    try:
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
        return "%-26s SKIP (rewrite did not apply)" % res["name"]
    if res["status"] == "compile-failed":
        return "%-26s COMPILE FAILED\n%s" % (res["name"], res["log"])
    if res["status"] == "no-result":
        return "%-26s no report result" % res["name"]
    pcts = res["functions"]
    bad = ["%s=%.2f%%(%d/%d)" % (n, p, o, t) for n, (p, o, t) in pcts.items() if p != 100.0]
    verdict = "ALL %d FUNCTIONS AT 100%%" % len(pcts) if not bad else "; ".join(bad)
    return "%-26s .text=%-6d %s" % (res["name"], res["text_size"], verdict)


def rank(results, symbol=None):
    """The scored results best first: by `symbol`'s percent, then its first divergence (later is better, a full
    match best), then the unit's matched bytes; without a symbol, by matched bytes alone."""
    def div(r):
        d = r.get("first_divergence")
        return float("inf") if d is None else (d if isinstance(d, int) else -1)

    ok = [r for r in results if r["status"] == "ok"]
    if symbol:
        return sorted(ok, key=lambda r: (-(r.get("symbol_percent") or 0.0), -div(r), -r["matched_bytes"], r["name"]))
    return sorted(ok, key=lambda r: (-r["matched_bytes"], r["name"]))


def print_ranking(ranked, symbol=None) -> None:
    print("ranking (%s)" % ("%s's score, then its first divergence, then the unit's matched bytes" % symbol
                            if symbol else "the unit's matched bytes"))
    print("  %-4s %-26s %9s %10s %7s %12s" % ("#", "variant", "symbol %", "first div", ".text", "matched B"))
    for i, r in enumerate(ranked, 1):
        d = r.get("first_divergence", "-")
        print("  %-4d %-26s %9s %10s %7d %12.2f"
              % (i, r["name"], "%.2f" % r["symbol_percent"] if r.get("symbol_percent") is not None else "-",
                 "full" if symbol and d is None else d, r["text_size"], r["matched_bytes"]))


def try_all(unit, tokens, selected, symbol=None, score=None, diverge=None, as_is=False, out=print):
    """Every selected variant (the unmodified source first when `as_is`) -> `[result]`, each line printed."""
    results = []
    plan = ([(AS_IS, None)] if as_is else []) + list(selected)
    for name, repls in plan:
        res = try_variant(unit, tokens, name, repls, symbol, score, diverge)
        results.append(res)
        if out:
            out(describe(res))
    return results


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("names", nargs="*", help="variant names (default: all of them)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--variants", help="variant file: a .py with VARIANTS, or a text file of `//@@ old` / "
                                       "`//@@ variant <name>` blocks (default: tools/flags/variants/<lib>.py)")
    ap.add_argument("--permute", metavar="FILE",
                    help="a text file of `//@@ item` blocks: try every other order of that contiguous run")
    ap.add_argument("--max-variants", type=int, default=120, help="refuse a permutation set larger than this")
    ap.add_argument("--symbol", help="rank by this function's score and first divergence")
    ap.add_argument("--json", action="store_true", help="the results and the ranking as JSON")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--list", action="store_true", help="list the variants and exit")
    ap.add_argument("--apply", metavar="NAME",
                    help="apply this variant's rewrite to the unit's real source and exit")
    args = ap.parse_args(argv)

    root = repo.repo_root()
    unit = units.Unit.resolve(args.unit, root)
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
        raw = open(unit.source, "rb").read()
        crlf = b"\r\n" in raw
        lf = raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n")
        new = apply_variant(lf, sel[0][1])
        if new is None:
            raise SystemExit("variant %r did not apply cleanly - nothing written" % args.apply)
        if new == lf:
            raise SystemExit("variant %r is a no-op for the current source - nothing written" % args.apply)
        out = new.replace("\n", "\r\n") if crlf else new
        text.atomic_write(unit.source, out.encode("utf-8", "surrogateescape"))
        obj = os.path.relpath(unit.obj_ours, root)
        print("applied %r to %s (%+d bytes)" % (args.apply, os.path.relpath(unit.source, root),
                                                len(out) - len(raw)))
        print("now rebuild and re-measure:")
        print("  ninja %s" % obj)
        print("  python %s diff -u %s <symbol>   (and the report's matched_functions)"
              % (os.path.relpath(__file__, root), unit.report_name))
        return 0

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
    results = try_all(unit, tokens, selected, args.symbol, as_is=marker_mode or bool(args.symbol), out=emit)
    ranked = rank(results, args.symbol)
    if args.json:
        print(json.dumps({"unit": unit.report_name, "variants": os.path.relpath(path, root), "symbol": args.symbol,
                          "results": results, "ranking": [r["name"] for r in ranked]}, indent=2))
    elif marker_mode or args.symbol:
        print_ranking(ranked, args.symbol)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
