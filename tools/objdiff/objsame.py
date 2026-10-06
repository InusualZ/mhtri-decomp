#!/usr/bin/env python3
"""Whether every compiled object of two build trees is the same modulo `@N`/`$N` numbering and named renames. Spec: docs/tools/spec/objsame.md.
CLI: python tools/objdiff/objsame.py [BASE [TREE]] [--base-tree REF] [--unit GLOB]... [--rename-map FILE|old=new,...]
[--renames-from-git [REF]] [--all-sections] [--all-objects] [--json]."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import fnmatch
import io
import json
import os
import re
import shutil
import struct
import sys
import tarfile
import time

from tools.lib import artifacts as _artifacts
from tools.lib import findings as _findings
from tools.lib import objcompare
from tools.lib import proc
from tools.lib import repo as _repo
from tools.lib.binary.elf import STT_FILE, ElfError
from tools.lib.git import Git
from tools.lib.project import symbols as _symbols

#: The compiled objects of a tree: `build/<version>/src/**.o` (what MWCC wrote, never the split targets).
SRC_OBJ_REL = os.path.join("build", _repo.VERSION, "src")
#: MWCC's compiler-generated numbering: pool labels `@123`, `@456@func@var`, and the local counters `$6556` of
#: `__arraydtor$6556` - every one moves with each earlier literal or local of the translation unit.
POOL_RE = re.compile(r"[@$]\d+")
#: Where `--base-tree` exports and builds a commit, below the compared tree (gitignored build output).
BASE_TREES_REL = os.path.join("build", "tmp", "objsame")
#: The file that says a base-tree export finished (its absence means start over).
BASE_MARKER = "objsame-base.json"
SYMBOLS_REL = "config/%s/symbols.txt" % _repo.VERSION
#: One ELF32 symbol-table entry; `st_name` is its first word.
SYM_ENTRY = 16


def pool_free(name: str) -> str:
    """A symbol name with every `@<digits>` read as `@N` and every `$<digits>` as `$N`. Pure."""
    return POOL_RE.sub(lambda m: m.group()[0] + "N", name or "")


def _normal(rename: dict | None, used: set | None):
    """The name normaliser: the rename map (base spelling -> tree spelling) first, then `pool_free`."""
    def norm(name: str) -> str:
        if rename and name in rename:
            if used is not None:
                used.add(name)
            name = rename[name]
        return pool_free(name)
    return norm


def _strtab(data: bytes, norm) -> bytes:
    """A string table with every string normalised, in order (its offsets are not compared: see `_symtab`)."""
    return b"\0".join(norm(s.decode("latin-1")).encode("latin-1") for s in data.split(b"\0"))


def _symtab(data: bytes) -> bytes:
    """A symbol table with every `st_name` zeroed: the names are compared through `symbols`, and one renumbered
    label of another length shifts every later string's offset."""
    out = bytearray(data)
    for off in range(0, len(out) - SYM_ENTRY + 1, SYM_ENTRY):
        struct.pack_into(">I", out, off, 0)
    return bytes(out)


def object_view(path: str, all_sections: bool = False, rename: dict | None = None, used: set | None = None) -> dict:
    """What two objects must share to be the same: `{sections: {name: (size, bytes)}, relocs: {section: sorted
    [(offset, type, name)]}, symbols: sorted [(name, section, value, size, bind)]}` - the defined symbols, every name
    through `rename` (old -> new; the old names it applied are added to `used`) and then modulo `@N`/`$N`.
    Metadata sections (`.comment`, the string and symbol tables) are left out unless `all_sections`; then a string
    table is compared string by string, normalised, and the symbol table with its name offsets zeroed."""
    norm = _normal(rename, used)
    view = objcompare.object_sections(path, all_sections=all_sections)
    elf = objcompare.load(path)
    nsec = len(elf.sections)
    symbols = sorted((norm(s.name), elf.sections[s.shndx].name if 0 < s.shndx < nsec else str(s.shndx),
                      s.value, s.size, s.bind)
                     for s in elf.symbols if s.name and s.shndx and s.type != STT_FILE)
    sections = {}
    for n, v in view["sections"].items():
        data = v["data"]
        if n == ".strtab" and data is not None:
            data = _strtab(data, norm)
            sections[n] = (len(data), data)
        elif n == ".symtab" and data is not None:
            sections[n] = (v["size"], _symtab(data))
        else:
            sections[n] = (v["size"], data)
    return {"sections": sections,
            "relocs": {sec: sorted((off, typ, norm(name)) for off, typ, name in rows)
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


def unit_objects(tree: str, registered_only: bool = True) -> tuple[dict[str, str], list[str]]:
    """`({unit stem (forward slashes, no .o): path}, [skipped stems])` of the compiled objects under the tree's
    `build/.../src/`. With `registered_only` an object no `configure.py` `Object` names (`lib.artifacts.
    orphan_objects`: a retired or renamed unit's leftover) is skipped and listed instead."""
    root = os.path.join(tree, SRC_OBJ_REL)
    known = _artifacts.registered_stems(tree) if registered_only else None
    out, skipped = {}, []
    for base, _dirs, files in os.walk(root):
        for name in files:
            if name.endswith(".o"):
                path = os.path.join(base, name)
                stem = os.path.relpath(path, root).replace("\\", "/")[:-2]
                if known is not None and stem not in known:
                    skipped.append(stem)
                else:
                    out[stem] = path
    return out, sorted(skipped)


def run(base: str, tree: str, units: list[str] | None = None, all_sections: bool = False,
        rename: dict | None = None, registered_only: bool = True) -> dict:
    """Compare the two trees: `{base, tree, compared, same, renamed: [{unit, renames}], differ: [{unit, reasons}],
    only_base, only_tree, unreadable, skipped_base, skipped_tree}`. `same` counts the identical objects; an object
    that is the same only once `rename` (old -> new) is applied to the base side is `renamed`, with the renames it
    used. A unit glob list narrows both sides (`fnmatch` on the stem)."""
    a, skip_a = unit_objects(base, registered_only)
    b, skip_b = unit_objects(tree, registered_only)

    def wanted(stem: str) -> bool:
        return not units or any(fnmatch.fnmatchcase(stem, g) for g in units)

    a = {k: v for k, v in a.items() if wanted(k)}
    b = {k: v for k, v in b.items() if wanted(k)}
    out = {"base": base, "tree": tree, "compared": 0, "same": 0, "renamed": [], "differ": [], "unreadable": [],
           "only_base": sorted(set(a) - set(b)), "only_tree": sorted(set(b) - set(a)),
           "skipped_base": [s for s in skip_a if wanted(s)], "skipped_tree": [s for s in skip_b if wanted(s)]}
    for stem in sorted(set(a) & set(b)):
        out["compared"] += 1
        try:
            theirs = object_view(b[stem], all_sections)
            reasons = compare(object_view(a[stem], all_sections), theirs)
            used: set = set()
            if reasons and rename:
                if not compare(object_view(a[stem], all_sections, rename, used), theirs):
                    out["renamed"].append({"unit": stem, "renames": ["%s -> %s" % (o, rename[o])
                                                                     for o in sorted(used)]})
                    continue
        except (OSError, ElfError, ValueError) as exc:
            out["unreadable"].append({"unit": stem, "error": str(exc)})
            continue
        if reasons:
            out["differ"].append({"unit": stem, "reasons": reasons})
        else:
            out["same"] += 1
    return out


def verdict(result: dict) -> _findings.Verdict:
    """The result as `lib.findings` rows: one FAIL per differing, unreadable or one-sided unit (an object that
    differs only by the named renames is not a failure)."""
    rows = [_findings.Row.check(d["unit"], False, "; ".join(d["reasons"])) for d in result["differ"]]
    rows += [_findings.Row.check(u["unit"], False, "unreadable: " + u["error"]) for u in result["unreadable"]]
    rows += [_findings.Row.check(u, False, "only in the base tree") for u in result["only_base"]]
    rows += [_findings.Row.check(u, False, "only in the compared tree") for u in result["only_tree"]]
    return _findings.Verdict.of(rows)


# --- renames -------------------------------------------------------------------------------------------------------

def parse_rename_map(spec: str) -> dict[str, str]:
    """`{old: new}` from a file (one `old new` or `old=new` per line, `#` comments; the `symedit.py rename-batch`
    input) or an inline `old=new,old2=new2`. A malformed entry raises ValueError."""
    out: dict[str, str] = {}
    if os.path.isfile(spec):
        with open(spec, encoding="utf-8") as fh:
            entries = [ln.split("#", 1)[0].strip() for ln in fh]
    else:
        entries = [e.strip() for e in spec.split(",")]
    for e in entries:
        if not e:
            continue
        parts = e.split("=", 1) if "=" in e else e.split()
        if len(parts) != 2 or not parts[0].strip() or not parts[1].strip():
            raise ValueError("rename map entry %r is not `old=new` / `old new`" % e)
        out[parts[0].strip()] = parts[1].strip()
    return out


def renames_from_git(tree: str, ref: str) -> dict[str, str]:
    """`{old: new}` from the tree's `symbols.txt`: every row renamed at an unchanged address against `ref`
    (`lib.project.symbols.rename_pairs`), plus every generated stem the map already names otherwise
    (`stem_renames`: a base source that still spelled `fn_<ADDR>` for a row the map had renamed)."""
    out: dict[str, str] = {}
    path = os.path.join(tree, *SYMBOLS_REL.split("/"))
    if os.path.isfile(path):
        out.update(_symbols.stem_renames(_symbols.read(path).rows()))
    p = Git(tree).run("diff", "--no-color", "--no-ext-diff", "-U0", ref, "--", SYMBOLS_REL)
    if p.returncode != 0:
        raise ValueError("git diff %s -- %s failed: %s" % (ref, SYMBOLS_REL, (p.stderr or "").strip()))
    removed, added = [], []
    for line in (p.stdout or "").splitlines():
        if line.startswith("-") and not line.startswith("---"):
            removed.append(line[1:])
        elif line.startswith("+") and not line.startswith("+++"):
            added.append(line[1:])
    out.update(_symbols.rename_pairs(removed, added))
    return out


# --- the base tree of a commit -------------------------------------------------------------------------------------

def toolchain_args(tree: str) -> list[str] | None:
    """The `configure.py` flags that point a scratch tree at an existing toolchain (`build/compilers`,
    `build/tools`, `build/binutils`) - the compared tree's, else its MAIN checkout's; None when neither has one."""
    for root in (tree, _repo.main_checkout(tree)):
        b = os.path.join(root, "build")
        need = [os.path.join(b, "compilers"), os.path.join(b, "tools", "sjiswrap.exe"),
                os.path.join(b, "tools", "dtk.exe"), os.path.join(b, "binutils")]
        if all(os.path.exists(p) for p in need):
            args = ["--compilers", need[0], "--sjiswrap", need[1], "--dtk", need[2], "--binutils", need[3]]
            objdiff = os.path.join(b, "tools", "objdiff-cli.exe")
            return args + (["--objdiff", objdiff] if os.path.exists(objdiff) else [])
    return None


def base_tree_dir(tree: str, sha: str) -> str:
    return os.path.join(tree, BASE_TREES_REL, "base-" + sha[:12])


#: The original files a split reads (`lib.artifacts.SPLIT_INPUTS` below `orig/`).
ORIG_INPUTS = tuple(r for r in _artifacts.SPLIT_INPUTS if r.startswith("orig" + os.sep))


def _orig_source(tree: str) -> str | None:
    """The tree whose `orig/` holds every original file the split reads: the compared tree, else MAIN."""
    for root in (tree, _repo.main_checkout(tree)):
        if all(os.path.isfile(os.path.join(root, r)) for r in ORIG_INPUTS):
            return root
    return None


def export_base_tree(tree: str, ref: str, runner=None, out=sys.stderr) -> str:
    """Export commit `ref` into `build/tmp/objsame/base-<sha>/` of `tree` (reused when its marker says the export
    finished; otherwise wiped and redone) with `git archive`, a COPY of `orig/RMHE08`, and `configure.py` pointed
    at the existing toolchain. Never a worktree (nothing is registered with git) and never a copied object: a
    fresh export has no objects, so ninja compiles every requested one from the commit's sources - copying objects
    or sources in with their mtimes kept (`shutil.copy2`) is the trap where ninja rebuilds nothing."""
    git = Git(tree)
    sha = git.out("rev-parse", "--verify", ref + "^{commit}").strip()
    d = base_tree_dir(tree, sha)
    marker = os.path.join(d, BASE_MARKER)
    if os.path.isfile(marker):
        return d
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    print("objsame: exporting %s (%s) to %s" % (ref, sha[:12], d), file=out)
    data = git.run_bytes("archive", "--format=tar", sha, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(data)) as tf:
        tf.extractall(d, filter="data")
    orig = _orig_source(tree)
    if orig is None:
        raise RuntimeError("no %s in %s or its MAIN checkout: the base tree cannot be split"
                           % (" + ".join(r.replace(os.sep, "/") for r in ORIG_INPUTS), tree))
    for rel in ORIG_INPUTS:                            # copies, never a link: orig/ is read-only ground truth
        os.makedirs(os.path.dirname(os.path.join(d, rel)), exist_ok=True)
        shutil.copy2(os.path.join(orig, rel), os.path.join(d, rel))
    tools = toolchain_args(tree)
    if tools is None:
        raise RuntimeError("no toolchain (build/compilers, build/tools) in %s or its MAIN checkout: run `ninja "
                           "tools` there first" % tree)
    run = runner or (lambda argv, cwd: proc.run(argv, cwd=cwd))
    p = run([sys.executable, "configure.py", *tools], d)
    if p.returncode != 0:
        raise RuntimeError("configure.py failed in %s: %s" % (d, ((p.stderr or "") + (p.stdout or "")).strip()[-400:]))
    with open(marker, "w", encoding="utf-8") as fh:
        json.dump({"ref": ref, "sha": sha, "exported": time.strftime("%Y-%m-%dT%H:%M:%S")}, fh)
    return d


def build_base_tree(tree: str, ref: str, units: list[str] | None = None, runner=None, out=sys.stderr) -> str:
    """`export_base_tree`, then `ninja` the base's compiled objects in it - every registered unit's
    (`all_source`), or with `units` only those whose stem matches a glob. The first build runs the commit's own
    `dtk dol split` (the manifest depends on it). Returns the base tree's root."""
    d = export_base_tree(tree, ref, runner, out)
    targets = ["all_source"]
    if units:
        stems = sorted(s for s in (_artifacts.registered_stems(d) or ())
                       if any(fnmatch.fnmatchcase(s, g) for g in units))
        targets = [os.path.join(SRC_OBJ_REL, *(s + ".o").split("/")) for s in stems] or ["build.ninja"]
    run = runner or (lambda argv, cwd: proc.run(argv, cwd=cwd))
    t0 = time.time()
    p = run(["ninja", *targets], d)
    if p.returncode != 0:
        try:
            os.unlink(os.path.join(d, BASE_MARKER))    # the next run starts over rather than reuse a broken export
        except OSError:
            pass
        tail = [ln for ln in ((p.stdout or "") + "\n" + (p.stderr or "")).splitlines() if ln.strip()]
        raise RuntimeError("ninja failed in %s: %s" % (d, " | ".join(tail[-4:])))
    print("objsame: base tree %s built in %.1f s" % (d, time.time() - t0), file=out)
    return d


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="objsame.py", description=__doc__.split("\n")[0])
    ap.add_argument("base", nargs="?", help="the reference tree (default: MAIN)")
    ap.add_argument("tree", nargs="?", help="the tree to compare (default: the invocation's tree)")
    ap.add_argument("--base-tree", metavar="REF",
                    help="build (or reuse) the objects of commit REF in build/tmp/objsame/ and use them as the base")
    ap.add_argument("--unit", action="append", metavar="GLOB", help="only units whose stem matches; repeatable")
    ap.add_argument("--rename-map", metavar="FILE|old=new,...",
                    help="symbol renames base -> tree: an object that is the same once they apply 'differs only by "
                         "renames' (not a failure)")
    ap.add_argument("--renames-from-git", nargs="?", const="", metavar="REF",
                    help="derive the renames from the tree's symbols.txt diff against REF (default: --base-tree's "
                         "REF, else main)")
    ap.add_argument("--all-sections", action="store_true", help="compare .comment and the string tables too")
    ap.add_argument("--all-objects", action="store_true",
                    help="compare objects of unregistered units too (default: skip them, they are orphans)")
    ap.add_argument("--json", action="store_true", help="the lib.findings schema plus the counts")
    args = ap.parse_args(argv)

    tree = os.path.abspath(args.tree) if args.tree else _repo.repo_root()
    if args.base and args.base_tree:
        print("objsame: give BASE or --base-tree, not both", file=sys.stderr)
        return _findings.EXIT_ERROR
    rename: dict[str, str] = {}
    try:
        if args.rename_map:
            rename.update(parse_rename_map(args.rename_map))
        if args.renames_from_git is not None:
            rename.update(renames_from_git(tree, args.renames_from_git or args.base_tree or "main"))
    except (OSError, ValueError) as exc:
        print("objsame: %s" % exc, file=sys.stderr)
        return _findings.EXIT_ERROR
    if args.base_tree:
        try:
            base = build_base_tree(tree, args.base_tree, args.unit)
        except (RuntimeError, OSError) as exc:
            print("objsame: --base-tree %s: %s" % (args.base_tree, exc), file=sys.stderr)
            return _findings.EXIT_ERROR
    else:
        base = os.path.abspath(args.base) if args.base else _repo.main_checkout(tree)
    for t in (base, tree):
        if not os.path.isdir(os.path.join(t, SRC_OBJ_REL)):
            print("objsame: %s has no %s - build it first (this tool builds nothing unless --base-tree REF)"
                  % (t, SRC_OBJ_REL),
                  file=sys.stderr)
            return _findings.EXIT_ERROR
    result = run(base, tree, args.unit, args.all_sections, rename, registered_only=not args.all_objects)
    v = verdict(result)
    summary = ("%d identical, %d differ only by renames, %d differ, %d one-sided (of %d compared; %d unregistered "
               "object(s) skipped)" % (result["same"], len(result["renamed"]),
                                       len(result["differ"]) + len(result["unreadable"]),
                                       len(result["only_base"]) + len(result["only_tree"]), result["compared"],
                                       len(result["skipped_base"]) + len(result["skipped_tree"])))
    if args.json:
        print(_findings.render_json("objsame", v, base=base, tree=tree, compared=result["compared"],
                                    same=result["same"], renamed=result["renamed"],
                                    skipped=result["skipped_base"] + result["skipped_tree"], counts=summary,
                                    renames=len(rename)))
    else:
        for r in result["renamed"]:
            print("RENAMED %s  %s" % (r["unit"], ", ".join(r["renames"][:6])))
        for d in result["differ"]:
            print("DIFFER  %s  %s" % (d["unit"], "; ".join(d["reasons"][:3])))
        for u in result["unreadable"]:
            print("UNREAD  %s  %s" % (u["unit"], u["error"]))
        for u in result["only_base"]:
            print("BASE    %s  (no object in the compared tree)" % u)
        for u in result["only_tree"]:
            print("TREE    %s  (no object in the base tree)" % u)
        print("objsame: " + summary)
    return _findings.exit_code(v)


if __name__ == "__main__":
    sys.exit(main())
