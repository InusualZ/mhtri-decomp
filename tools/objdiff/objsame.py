#!/usr/bin/env python3
"""Whether every compiled object of two build trees is the same modulo `@N` pool numbering. Spec: docs/tools/spec/objsame.md.
CLI: python tools/objdiff/objsame.py [BASE [TREE]] [--unit GLOB]... [--all-sections] [--json]."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import fnmatch
import json
import os
import re
import sys

from tools.lib import findings as _findings
from tools.lib import objcompare
from tools.lib import repo as _repo
from tools.lib.binary.elf import STT_FILE, ElfError

#: The compiled objects of a tree: `build/<version>/src/**.o` (what MWCC wrote, never the split targets).
SRC_OBJ_REL = os.path.join("build", _repo.VERSION, "src")
#: MWCC's compiler-generated labels: `@123`, `@456@func@var` - the numbering moves with every earlier literal.
POOL_RE = re.compile(r"@\d+")


def pool_free(name: str) -> str:
    """A symbol name with every `@<digits>` replaced by `@N`. Pure."""
    return POOL_RE.sub("@N", name or "")


def object_view(path: str, all_sections: bool = False) -> dict:
    """What two objects must share to be the same: `{sections: {name: (size, bytes)}, relocs: {section: sorted
    [(offset, type, pool_free name)]}, symbols: sorted [(pool_free name, section, value, size, bind)]}` - the
    defined symbols, names modulo `@N`. Metadata sections (`.comment`, the string and symbol tables) are left out
    unless `all_sections` (the symbol table is compared through `symbols`)."""
    view = objcompare.object_sections(path, all_sections=all_sections)
    elf = objcompare.load(path)
    nsec = len(elf.sections)
    symbols = sorted((pool_free(s.name), elf.sections[s.shndx].name if 0 < s.shndx < nsec else str(s.shndx),
                      s.value, s.size, s.bind)
                     for s in elf.symbols if s.name and s.shndx and s.type != STT_FILE)
    return {"sections": {n: (v["size"], v["data"]) for n, v in view["sections"].items()},
            "relocs": {sec: sorted((off, typ, pool_free(name)) for off, typ, name in rows)
                       for sec, rows in view["relocs"].items() if rows},
            "symbols": symbols}


def compare(a: dict, b: dict) -> list[str]:
    """Why two `object_view`s differ (empty when they are the same): one reason per section, relocation section
    and the symbol table, the first difference named. Pure."""
    why = []
    for name in sorted(set(a["sections"]) | set(b["sections"])):
        sa, sb = a["sections"].get(name), b["sections"].get(name)
        if sa is None or sb is None:
            why.append("%s only in %s" % (name, "base" if sb is None else "tree"))
        elif sa[0] != sb[0]:
            why.append("%s size 0x%X -> 0x%X" % (name, sa[0], sb[0]))
        elif sa[1] != sb[1]:
            at = objcompare.first_difference(sa[1], sb[1])
            why.append("%s bytes differ at +0x%X (%d byte(s))" % (name, at or 0,
                                                                 objcompare.differing_bytes(sa[1], sb[1])))
    for sec in sorted(set(a["relocs"]) | set(b["relocs"])):
        ra, rb = a["relocs"].get(sec, []), b["relocs"].get(sec, [])
        if ra != rb:
            first = next(((x, y) for x, y in zip(ra, rb) if x != y), None)
            what = ("%d -> %d relocation(s)" % (len(ra), len(rb)) if first is None else
                    "+0x%X %s -> +0x%X %s" % (first[0][0], first[0][2], first[1][0], first[1][2]))
            why.append(".rela%s: %s" % (sec, what))
    if a["symbols"] != b["symbols"]:
        gone = [s[0] for s in a["symbols"] if s not in b["symbols"]]
        new = [s[0] for s in b["symbols"] if s not in a["symbols"]]
        why.append("symbols: -[%s] +[%s]" % (" ".join(gone[:4]), " ".join(new[:4])))
    return why


def unit_objects(tree: str) -> dict[str, str]:
    """`{unit stem (forward slashes, no .o): path}` of every compiled object under the tree's `build/.../src/`."""
    root = os.path.join(tree, SRC_OBJ_REL)
    out = {}
    for base, _dirs, files in os.walk(root):
        for name in files:
            if name.endswith(".o"):
                path = os.path.join(base, name)
                out[os.path.relpath(path, root).replace("\\", "/")[:-2]] = path
    return out


def run(base: str, tree: str, units: list[str] | None = None, all_sections: bool = False) -> dict:
    """Compare the two trees: `{base, tree, compared, same, differ: [{unit, reasons}], only_base, only_tree,
    unreadable}`. A unit glob list narrows both sides (`fnmatch` on the stem)."""
    a, b = unit_objects(base), unit_objects(tree)

    def wanted(stem: str) -> bool:
        return not units or any(fnmatch.fnmatchcase(stem, g) for g in units)

    a = {k: v for k, v in a.items() if wanted(k)}
    b = {k: v for k, v in b.items() if wanted(k)}
    out = {"base": base, "tree": tree, "compared": 0, "same": 0, "differ": [], "unreadable": [],
           "only_base": sorted(set(a) - set(b)), "only_tree": sorted(set(b) - set(a))}
    for stem in sorted(set(a) & set(b)):
        out["compared"] += 1
        try:
            reasons = compare(object_view(a[stem], all_sections), object_view(b[stem], all_sections))
        except (OSError, ElfError, ValueError) as exc:
            out["unreadable"].append({"unit": stem, "error": str(exc)})
            continue
        if reasons:
            out["differ"].append({"unit": stem, "reasons": reasons})
        else:
            out["same"] += 1
    return out


def verdict(result: dict) -> _findings.Verdict:
    """The result as `lib.findings` rows: one FAIL per differing, unreadable or one-sided unit."""
    rows = [_findings.Row.check(d["unit"], False, "; ".join(d["reasons"])) for d in result["differ"]]
    rows += [_findings.Row.check(u["unit"], False, "unreadable: " + u["error"]) for u in result["unreadable"]]
    rows += [_findings.Row.check(u, False, "only in the base tree") for u in result["only_base"]]
    rows += [_findings.Row.check(u, False, "only in the compared tree") for u in result["only_tree"]]
    return _findings.Verdict.of(rows)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="objsame.py", description=__doc__.split("\n")[0])
    ap.add_argument("base", nargs="?", help="the reference tree (default: MAIN)")
    ap.add_argument("tree", nargs="?", help="the tree to compare (default: the invocation's tree)")
    ap.add_argument("--unit", action="append", metavar="GLOB", help="only units whose stem matches; repeatable")
    ap.add_argument("--all-sections", action="store_true", help="compare .comment and the string tables too")
    ap.add_argument("--json", action="store_true", help="the lib.findings schema plus the counts")
    args = ap.parse_args(argv)

    tree = os.path.abspath(args.tree) if args.tree else _repo.repo_root()
    base = os.path.abspath(args.base) if args.base else _repo.main_checkout(tree)
    for t in (base, tree):
        if not os.path.isdir(os.path.join(t, SRC_OBJ_REL)):
            print("objsame: %s has no %s - build it first (this tool builds nothing)" % (t, SRC_OBJ_REL),
                  file=sys.stderr)
            return _findings.EXIT_ERROR
    result = run(base, tree, args.unit, args.all_sections)
    v = verdict(result)
    if args.json:
        print(_findings.render_json("objsame", v, base=base, tree=tree, compared=result["compared"],
                                    same=result["same"]))
    else:
        for d in result["differ"]:
            print("DIFFER  %s  %s" % (d["unit"], "; ".join(d["reasons"][:3])))
        for u in result["unreadable"]:
            print("UNREAD  %s  %s" % (u["unit"], u["error"]))
        for u in result["only_base"]:
            print("BASE    %s  (no object in the compared tree)" % u)
        for u in result["only_tree"]:
            print("TREE    %s  (no object in the base tree)" % u)
        print("objsame: %d of %d object(s) the same modulo @N pool numbering; %d differ, %d one-sided"
              % (result["same"], result["compared"], len(result["differ"]) + len(result["unreadable"]),
                 len(result["only_base"]) + len(result["only_tree"])))
    return _findings.exit_code(v)


if __name__ == "__main__":
    sys.exit(main())
