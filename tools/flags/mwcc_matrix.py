#!/usr/bin/env python3
"""Compile a unit with several MWCC versions and/or flag overrides and summarize the official score per function.
Spec: docs/tools/spec/mwcc_matrix.md. CLI: python tools/flags/mwcc_matrix.py [-u <unit>] [<version>...]
[--flags-extra "<flags>"] [--list-versions]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os

from tools.lib import repo, report, units


def outdir(unit):
    """`build/tmp/matrix/` of the unit's tree: the raw diffs and `summary.txt`."""
    return os.path.join(unit.root, "build", "tmp", "matrix")


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
    """Row detail (first divergence, sizes) for one variant; the score comes from the report.

    `-c functionRelocDiffs=none` matches the report's default, so the `first-diff@` index is not a
    relocation-only difference the official metric ignores.
    """
    out = os.path.join(outdir(unit), label.replace("/", "_") + ".json")
    path, log = report.project_diff(unit.root, unit.report_name, symbol, out, report.objdiff_cli(unit.root))
    return (path, "") if path else (None, log)


def summarize(path, official=None):
    """Rows for one variant: (name, target size, ours size, match percent, first diff).

    `official` is `{function: fuzzy_match_percent}` from `lib.report.score_entries` for the same object
    pair - the score a flag decision must be made on. The diff JSON's own `match_percent` is positional
    and relocation-sensitive, so it is only a fallback for a function the report did not score (marked
    with `~` in the printed row).
    """
    d = json.load(open(path))
    # Project mode (`-p . -u UNIT`): left = target, right = our build.
    ours = dict(od_symbols(d, "right"))
    official = official or {}
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
        if name in official and isinstance(official[name], (int, float)):
            pct, approx = round(official[name], 2), False
        else:
            pct, approx = round(e.get("match_percent") or 0.0, 2), True
        rows.append((name, od_size(e), od_size(ri) if ri else -1, pct, first, approx))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("versions", nargs="*", help="compiler versions (default: the unit's own)")
    ap.add_argument("--unit", "-u", help="unit spec (default: the only unit with source)")
    ap.add_argument("--flags-extra", default="", help="flags to add, replacing same-family ones")
    ap.add_argument("--list-versions", action="store_true", help="print installed compiler versions")
    args = ap.parse_args()

    unit = units.Unit.resolve(args.unit, repo.repo_root())
    root = unit.root
    head, flags, tail = units.split_command(unit)
    if args.list_versions:
        print("\n".join(units.available_versions(head)))
        return
    flags = units.override_flags(flags, args.flags_extra)
    symbol = units.function_names(unit.obj_target)[0] if os.path.exists(unit.obj_target) else None
    if symbol is None:
        raise SystemExit("no target object at %s - split the unit first" % unit.obj_target)

    os.makedirs(outdir(unit), exist_ok=True)
    lines = []
    for version in (args.versions or [None]):
        label = (version or "default") + ("__" + "_".join(args.flags_extra.split())
                                          if args.flags_extra else "")
        head_v = units.with_compiler_version(head, version) if version else head
        flags_v = list(flags)
        rc, log, obj = units.run_tokens(head_v + flags_v + tail, root, expect=unit.obj_ours)
        while rc != 0 and len(flags_v) > 1:
            trimmed = units.drop_unknown_option(flags_v, log)
            if trimmed is None:
                break
            flags_v = trimmed
            rc, log, obj = units.run_tokens(head_v + flags_v + tail, root, expect=unit.obj_ours)
        if rc != 0:
            msg = "%s: COMPILE FAILED rc=%d\n%s" % (label, rc, units.quiet(log)[:800])
            print(msg)
            lines.append(msg)
            continue
        path, err = diff_unit(unit, label, symbol)
        if not path:
            msg = "%s: objdiff FAILED\n%s" % (label, err[:400])
            print(msg)
            lines.append(msg)
            continue
        official = report.score_entries(unit.obj_target, unit.obj_ours, unit.report_name, outdir(unit),
                                        objdiff=report.objdiff_cli(root), cwd=root)
        if "_error" in official:
            print("%s: WARNING the report metric is unavailable, positional values below are marked ~:\n  %s"
                  % (label, official["_error"][:200]))
            official = {}
        rows = summarize(path, official)
        block = ["== %s   (match = official report metric)" % label]
        for name, tsz, osz, pct, first, approx in rows:
            fd = ("first-diff@%d %s" % first) if first else "IDENTICAL"
            block.append("   %-28s target %6d  ours %6d  %6.2f%%%s  %s"
                         % (name, tsz, osz, pct, "~" if approx else " ", fd))
        block.append("")
        lines += block
        print("\n".join(block))
    open(os.path.join(outdir(unit), "summary.txt"), "w").write("\n".join(lines))
    print("wrote", os.path.relpath(os.path.join(outdir(unit), "summary.txt"), root))
    print("restore the unit object with: rm -f %s && ninja %s"
          % (os.path.relpath(unit.obj_ours, root), os.path.relpath(unit.obj_ours, root)))


if __name__ == "__main__":
    main()
