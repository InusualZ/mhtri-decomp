#!/usr/bin/env python3
"""Try source rewrites of a unit and report the resulting objdiff match, per function.

The harness is generic: it copies the unit's source, applies a variant's rewrite, compiles that copy with
the unit's exact ninja command line, and diffs it against the split target object. The real source is
never modified and the unit's object is never clobbered. Variants live in a separate data file, so the
rewrites for one unit do not leak into the tool.

The per-function number this prints is the **official report metric** (`report generate`'s
`fuzzy_match_percent`, via `unitutil.report_functions`) - the same number `build/RMHE08/report.json`,
`ledger.py` and `land.py` read. objdiff's explicit `diff` mode is deliberately not used for the score:
it defaults `functionRelocDiffs` to `data_value` (the report defaults to `none`, so relocation-only
differences counted as mismatches there) and its `match_percent` is a different normalisation (measured
on this repo: `pl_skill` fn_80270018 reads 99.88 % positionally and **100.0 %** officially).

Usage:
    python tools/flags/tryvar.py                        # every variant of the default variant file
    python tools/flags/tryvar.py --list
    python tools/flags/tryvar.py <name> [<name> ...]
    python tools/flags/tryvar.py -u <unit> [--variants <file.py>]
    python tools/flags/tryvar.py --apply <name>          # LAND the winning rewrite in the real source

A variant file (default: `tools/flags/variants/<lib>.py`, i.e. next to this script, named after the
unit's library) defines:

    VARIANTS = [(name, repls), ...]

where `repls` is either a list of `(old, new)` string pairs or a callable `src -> src` (returning `None`
means "the pattern did not match").

`--apply <name>` writes that variant's rewrite into the unit's real source file (preserving its line
endings), so a win becomes progress on the unit instead of staying an experiment. It refuses to write
anything unless the rewrite applies cleanly and actually changes the file. Rebuild and re-measure
afterwards - the recorded evidence must come from the real source, not from the probe.
"""
import argparse
import importlib.util
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu

SCRATCH = os.path.join(uu.ROOT, "build", "tmp", "probe")


def load_variants(path):
    spec = importlib.util.spec_from_file_location("variants", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return list(getattr(mod, "VARIANTS", []))


def default_variants_path(unit):
    """`tools/flags/variants/<lib>.py`, falling back to the lower-case spelling (case-sensitive FS)."""
    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "variants")
    lib = unit.lib or "main"            # a top-level unit has no directory: its variants file is main.py
    for name in (lib + ".py", lib.lower() + ".py"):
        if os.path.exists(os.path.join(d, name)):
            return os.path.join(d, name)
    return os.path.join(d, lib + ".py")


def match_pcts(probe_obj, target_obj, symbol=None):
    """{function: (official_match_percent, ours_size, target_size)} for a probe object.

    The percent is the report metric for the object pair (`unitutil.report_functions`), not the positional
diff value. The two sizes come from the objects' own symbol tables, so every function the probe emitted
    is listed; `symbol` is accepted for backwards compatibility and ignored - the report scores the whole
    object pair, which is what a variant comparison needs.
    """
    entries = uu.report_functions(target_obj, probe_obj)
    if "_error" in entries:
        print("objdiff report failed: " + entries["_error"][:400])
        return None
    ours = {n: sz for n, sz, _f in uu.frames(probe_obj)}
    tgt = {n: sz for n, sz, _f in uu.frames(target_obj)}
    res = {}
    for name, e in entries.items():
        pct = e.get("fuzzy_match_percent")
        if not isinstance(pct, (int, float)) or name not in ours:
            continue                     # function the probe did not emit (or an unpaired row)
        res[name] = (round(pct, 2), int(ours.get(name) or 0), int(tgt.get(name) or 0))
    return res


def apply_variant(src, repls):
    if callable(repls):
        return repls(src)
    for old, new in repls:
        if old not in src:
            print("    (pattern not found: %r)" % old[:60])
            return None
        src = src.replace(old, new, 1)
    return src


def run(unit, tokens, symbol, name, repls):
    src = open(unit.src, encoding="utf-8", errors="surrogateescape").read()
    new = apply_variant(src, repls)
    if new is None:
        print("%-26s SKIP (rewrite did not apply)" % name)
        return
    ext = os.path.splitext(unit.src)[1]
    probe_src = os.path.join(os.path.dirname(unit.src), unit.file + "_probe" + ext)
    with open(probe_src, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(new)
    try:
        rc, log, obj = uu.run_compile(tokens, scratch_dir=SCRATCH, src=probe_src)
        if rc != 0:
            print("%-26s COMPILE FAILED\n%s" % (name, uu.quiet(log)[:600]))
            return
        pcts = match_pcts(obj, unit.target, symbol)
        size = uu.text_size(obj)
    finally:
        if os.path.exists(probe_src):
            os.remove(probe_src)
    if not pcts:
        print("%-26s no report result" % name)
        return
    bad = ["%s=%.2f%%(%d/%d)" % (n, p, o, t) for n, (p, o, t) in pcts.items() if p != 100.0]
    verdict = "ALL %d FUNCTIONS AT 100%%" % len(pcts) if not bad else "; ".join(bad)
    print("%-26s .text=%-6d %s" % (name, size, verdict))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("names", nargs="*", help="variant names (default: all of them)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--variants", help="variant file (default: tools/flags/variants/<lib>.py)")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--list", action="store_true", help="list the variants and exit")
    ap.add_argument("--apply", metavar="NAME",
                    help="apply this variant's rewrite to the unit's real source and exit")
    args = ap.parse_args()

    unit = uu.resolve_unit(args.unit)
    path = args.variants or default_variants_path(unit)
    if not os.path.exists(path):
        raise SystemExit("no variant file at %s (pass --variants)" % os.path.relpath(path, uu.ROOT))
    variants = load_variants(path)
    if args.list:
        for name, _ in variants:
            print(name)
        return
    if args.apply:
        sel = [(n, r) for n, r in variants if n == args.apply]
        if not sel:
            raise SystemExit("no variant named %r in %s" % (args.apply, os.path.relpath(path, uu.ROOT)))
        raw = open(unit.src, "rb").read()
        crlf = b"\r\n" in raw
        text = raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n")
        new = apply_variant(text, sel[0][1])
        if new is None:
            raise SystemExit("variant %r did not apply cleanly - nothing written" % args.apply)
        if new == text:
            raise SystemExit("variant %r is a no-op for the current source - nothing written" % args.apply)
        out = new.replace("\n", "\r\n") if crlf else new
        open(unit.src, "wb").write(out.encode("utf-8", "surrogateescape"))
        obj = os.path.relpath(unit.obj, uu.ROOT)
        print("applied %r to %s (%+d bytes)" % (args.apply, os.path.relpath(unit.src, uu.ROOT),
                                                len(out) - len(raw)))
        print("now rebuild and re-measure:")
        print("  ninja %s" % obj)
        print("  python %s diff -u %s <symbol>   (and the report's matched_functions)"
              % (os.path.relpath(__file__, uu.ROOT), unit.name))
        return

    if not os.path.exists(unit.target):
        raise SystemExit("no target object at %s - split the unit first" % unit.target)

    head, flags, tail = uu.split_flags(uu.compile_command(unit))
    tokens = head + uu.override_flags(flags, args.flags_extra) + tail
    symbol = uu.function_names(unit.target)[0]
    selected = [(n, r) for n, r in variants if not args.names or n in args.names]
    print("unit %s   variants from %s   (%d/%d selected)"
          % (unit.name, os.path.relpath(path, uu.ROOT), len(selected), len(variants)))
    print("%-26s %s" % ("variant", "result"))
    for name, repls in selected:
        run(unit, tokens, symbol, name, repls)


if __name__ == "__main__":
    main()
