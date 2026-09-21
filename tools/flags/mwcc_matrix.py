#!/usr/bin/env python3
"""Compile a unit with several MWCC versions and/or flag overrides and summarize the objdiff result.

The compile command is the exact one ninja would run for that unit (so whatever `configure.py` puts in
the unit's `cflags` is honoured); `--flags-extra` overrides same-family flags, and a compiler version
argument swaps the MWCC executable. The object is written to the unit's real output path so that
objdiff's project mode can diff it against the split target object.

Usage:
    python tools/flags/mwcc_matrix.py                          # the unit's own compiler, no overrides
    python tools/flags/mwcc_matrix.py -u <unit>
    python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "-O3 -inline noauto"
    python tools/flags/mwcc_matrix.py -u <unit> 1.0 1.3 1.5    # one run per compiler version

Writes: build/tmp/matrix/<label>.json (raw objdiff diff per variant)
        build/tmp/matrix/summary.txt (the printed table)
Leaves a foreign object behind - restore with:
    rm -f <unit obj> && ninja <unit obj>
"""
import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
import unitutil as uu

OUTDIR = os.path.join(uu.ROOT, "build", "tmp", "matrix")
OBJDIFF = os.path.join(uu.ROOT, "build", "tools", "objdiff-cli.exe")


def od_symbols(d, side):
    """Symbols of one diff side; handles objdiff-cli v3.6.1 (flat top-level `symbols[]`, only present
    when a symbol argument was passed) and the older nested layout."""
    top = d[side].get("symbols")
    if top:
        return [(e.get("name") or e.get("symbol", {}).get("name"), e) for e in top]
    out = []
    for s in d[side].get("sections", []):
        for e in s.get("symbols") or []:
            out.append((e.get("name") or e.get("symbol", {}).get("name"), e))
    return out


def od_size(e):
    return int(e.get("size") or e.get("symbol", {}).get("size") or 0)


def diff_unit(unit, label, symbol):
    out = os.path.join(OUTDIR, label.replace("/", "_") + ".json")
    cmd = [OBJDIFF, "diff", "-p", ".", "-u", unit.name, symbol, "--format", "json", "-o", out]
    p = subprocess.run(cmd, cwd=uu.ROOT, capture_output=True, text=True)
    if p.returncode != 0:
        return None, (p.stdout or "") + (p.stderr or "")
    return out, ""


def summarize(path):
    d = json.load(open(path))
    # Project mode (`-p . -u UNIT`): left = target, right = our build.
    ours = dict(od_symbols(d, "right"))
    rows = []
    for name, e in od_symbols(d, "left"):
        if not e.get("instructions"):
            continue
        ri = ours.get(name)
        first = None
        for i, ins in enumerate(e.get("instructions") or []):
            if "instruction" not in ins:
                continue
            if ins.get("diff_kind") in (None, "DIFF_NONE"):
                continue
            first = (i, ins["instruction"]["formatted"])
            break
        rows.append((name, od_size(e), od_size(ri) if ri else -1,
                     round(e.get("match_percent") or 0.0, 2), first))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("versions", nargs="*", help="compiler versions (default: the unit's own)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--list-versions", action="store_true", help="print installed compiler versions")
    args = ap.parse_args()

    unit = uu.resolve_unit(args.unit)
    head, flags, tail = uu.split_flags(uu.compile_command(unit))
    if args.list_versions:
        print("\n".join(uu.available_versions(head)))
        return
    flags = uu.override_flags(flags, args.flags_extra)
    symbol = uu.function_names(unit.target)[0] if os.path.exists(unit.target) else None
    if symbol is None:
        raise SystemExit("no target object at %s - split the unit first" % unit.target)

    os.makedirs(OUTDIR, exist_ok=True)
    lines = []
    for version in (args.versions or [None]):
        label = (version or "default") + ("__" + "_".join(args.flags_extra.split())
                                          if args.flags_extra else "")
        head_v = uu.with_compiler_version(head, version) if version else head
        flags_v = list(flags)
        rc, log, obj = uu.run_compile(head_v + flags_v + tail, expect=unit.obj)
        while rc != 0 and len(flags_v) > 1:
            trimmed = uu.drop_unknown_option(flags_v, log)
            if trimmed is None:
                break
            flags_v = trimmed
            rc, log, obj = uu.run_compile(head_v + flags_v + tail, expect=unit.obj)
        if rc != 0:
            msg = "%s: COMPILE FAILED rc=%d\n%s" % (label, rc, uu.quiet(log)[:800])
            print(msg)
            lines.append(msg)
            continue
        path, err = diff_unit(unit, label, symbol)
        if not path:
            msg = "%s: objdiff FAILED\n%s" % (label, err[:400])
            print(msg)
            lines.append(msg)
            continue
        rows = summarize(path)
        block = ["== %s" % label]
        for name, tsz, osz, pct, first in rows:
            fd = ("first-diff@%d %s" % first) if first else "IDENTICAL"
            block.append("   %-28s target %6d  ours %6d  %6.2f%%  %s" % (name, tsz, osz, pct, fd))
        block.append("")
        lines += block
        print("\n".join(block))
    open(os.path.join(OUTDIR, "summary.txt"), "w").write("\n".join(lines))
    print("wrote", os.path.relpath(os.path.join(OUTDIR, "summary.txt"), uu.ROOT))
    print("restore the unit object with: rm -f %s && ninja %s"
          % (os.path.relpath(unit.obj, uu.ROOT), os.path.relpath(unit.obj, uu.ROOT)))


if __name__ == "__main__":
    main()
