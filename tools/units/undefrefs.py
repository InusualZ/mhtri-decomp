#!/usr/bin/env python3
"""The flip blocker a score cannot see: a relocation our object carries that no link input can define (add-only).
Spec: docs/tools/spec/undefrefs.md. CLI: undefrefs.py <unit>... [--base REV | --base-snapshot F] [--json] |
--snapshot-base F | --census [--linkage [--unit U] [--no-decls] [--json]] [--census-out F] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

from tools.lib import artifacts as _artifacts
from tools.lib import names as libnames
from tools.lib import objcompare
from tools.lib import project as _project  # the map reader, the registered units

HERE = os.path.dirname(os.path.abspath(__file__))

# Relocations into extab/extabindex are compiler bookkeeping, not names our code calls (flipcheck agrees).
BOOKKEEPING = objcompare.BOOKKEEPING_SECTIONS
# mwldeppc defines the EABI small-data bases itself; `__start` is the linker's default entry root. No object
# and no `symbols.txt` row carries them.
LINKER_SYMBOLS = objcompare.LINKER_SYMBOLS
LINKER_ASSIGN_RE = objcompare.LINKER_ASSIGN_RE
GLOBAL_BINDING, WEAK_BINDING = 1, 2
SCHEMA = objcompare.LINK_INDEX_SCHEMA
# `build/` is untracked and gitignored, so this never shows in the gate's tree-dirty guard.
CACHE_REL = objcompare.LINK_INDEX_REL
SYMBOLS_REL = os.path.join("config", "RMHE08", "symbols.txt")
LDSCRIPT_REL = os.path.join("build", "RMHE08", "ldscript.lcf")
NINJA_REL = "build.ninja"
SRC_REL = os.path.join("build", "RMHE08", "src")
OBJ_REL = os.path.join("build", "RMHE08", "obj")
# The source extensions a unit name can carry (the name is extensionless: `claims.norm_unit`).
SOURCE_EXTS = (".c", ".cpp", ".cp")


# ---------------------------------------------------------------------------------------------------------
# readers (the selftest drives the pure ones)
# ---------------------------------------------------------------------------------------------------------

def link_inputs(main: str) -> list[str]:
    """The object inputs on `main.elf`'s link line in `build.ninja`, MAIN-relative (`objcompare.link_inputs`);
    `[]` when there is no link edge (a missing link edge is an empty index, not an error)."""
    return objcompare.link_inputs(os.path.join(main, NINJA_REL)) or []


def load_object(path: str) -> dict | None:
    """`{relocs, defined, refs}` for an ELF32 object, or None when it cannot be read (`objcompare.reloc_facts`)."""
    return objcompare.reloc_facts(path)


provides_global = objcompare.provides_global     # a `defined` entry the linker resolves across objects


def map_rows(main: str) -> set[str]:
    """The names `config/RMHE08/symbols.txt` carries a row for (the map's definition of a name)."""
    path = os.path.join(main, SYMBOLS_REL)
    if not os.path.exists(path):
        return set()
    return _project.SymbolMap(path).names()


def linker_symbols(main: str) -> set[str]:
    """The names the linker provides without an object: the script's assignments + the EABI/entry set."""
    return set(LINKER_SYMBOLS) | objcompare.linker_assigned(os.path.join(main, LDSCRIPT_REL))


def linkage_stem(name: str) -> str:
    return libnames.linkage_stem(name)


# ---------------------------------------------------------------------------------------------------------
# the cached link-symbol index
# ---------------------------------------------------------------------------------------------------------

def link_symbol_index(main: str, cache_path: str | None = None, rebuild: bool = False) -> dict:
    """`{providers: {name: [input, ...]}, ref_count: {name: n}, inputs: [input, ...]}` over the link inputs
    (`objcompare.link_index`: per-input facts cached under `build/tmp/undefrefs/link-symbols.json`)."""
    index = objcompare.link_index(main, link_inputs(main), cache_path or os.path.join(main, CACHE_REL), rebuild)
    return {"providers": index["providers"], "ref_count": index["ref_count"], "inputs": index["inputs"]}


# ---------------------------------------------------------------------------------------------------------
# the rule
# ---------------------------------------------------------------------------------------------------------

external_candidates = objcompare.external_candidates   # (section, offset, name) the link index must answer
spelling_hint = objcompare.spelling_hint               # the target's spelling at the slot, or by linkage stem


def unresolved_names(our: dict, target: dict | None, *, map_set: set[str],
                     providers: dict[str, list[str]], ref_count: dict[str, int], target_rel: str,
                     linker_set: set[str]) -> list[tuple[str, tuple[str, str] | None]]:
    """The `(name, spelling hint)` our object relocates that no link input can define (`objcompare.undefined`)."""
    return objcompare.undefined(our, target, map_set=map_set, providers=providers, ref_count=ref_count,
                                target_rel=target_rel, linker_set=linker_set)


def render_hits(hits: list[tuple[str, tuple[str, str] | None]]) -> list[str]:
    """`name` or ``name -> `spelling` (the target's spelling at the ...)``, one per hit."""
    out = []
    for name, hint in hits:
        out.append("%s -> `%s` (the target's spelling at the %s)" % (name, hint[0], hint[1])
                   if hint else name)
    return out


def check_object(unit: str, our: dict, target: dict | None, *, map_set: set[str],
                 providers: dict[str, list[str]], ref_count: dict[str, int], target_rel: str,
                 linker_set: set[str], base_names=frozenset()) -> tuple[list[str], str | None]:
    """-> (problems, pre-existing note). Only names absent from `base_names` are refused (add-only)."""
    hits = unresolved_names(our, target, map_set=map_set, providers=providers, ref_count=ref_count,
                            target_rel=target_rel, linker_set=linker_set)
    base = set(base_names)
    new = [h for h in hits if h[0] not in base]
    pre = [h for h in hits if h[0] in base]
    problems = []
    if new:
        rendered = render_hits(new)
        problems.append(
            "%s: %d referenced name(s) are defined by nothing a flip can use - %s - our object does not "
            "define them, `symbols.txt` carries no row and no link input other than the target object "
            "provides them, so a flip answers `undefined: '%s'`"
            % (unit, len(new), ", ".join(rendered), new[0][0]))
    line = None
    if pre:
        line = ("%s: %d pre-existing undefined reference(s) (the batch did not add them - not refused) - %s"
                % (unit, len(pre), render_hits(pre)[0]))
    return problems, line


# ---------------------------------------------------------------------------------------------------------
# the batch entry point the gate calls
# ---------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------
# the base snapshot (the set the row is a difference against) and the census
# ---------------------------------------------------------------------------------------------------------

def discover_units(main: str) -> list[str]:
    """Every registered unit with a compiled object in this tree (`build/RMHE08/src/**/*.o`), extensionless. An
    object no `configure.py` `Object` names (a retired unit's leftover, `lib.artifacts.orphan_objects`) is skipped."""
    root = os.path.join(main, SRC_REL)
    registered = _artifacts.registered_stems(main)
    out = []
    for dirpath, _dirs, files in os.walk(root):
        for name in sorted(files):
            if name.endswith(".o"):
                stem = os.path.relpath(os.path.join(dirpath, name), root).replace("\\", "/")[:-2]
                if registered is None or stem in registered:
                    out.append(stem)
    return sorted(out)


def source_sha(main: str, unit: str) -> str | None:
    """The SHA-1 of the unit's source file (the base snapshot's content key), or None."""
    for ext in SOURCE_EXTS:
        path = os.path.join(main, "src", unit + ext)
        if os.path.exists(path):
            return hashlib.sha1(open(path, "rb").read()).hexdigest()
    return None


def base_source_exists(main: str, base: str, unit: str) -> bool:
    """Whether the unit's source file exists at the base commit - a unit the base never had is new."""
    for ext in SOURCE_EXTS:
        rel = "src/%s%s" % (unit, ext)
        p = subprocess.run(["git", "cat-file", "-e", "%s:%s" % (base, rel)], cwd=main,
                           capture_output=True)
        if p.returncode == 0:
            return True
    return False


def _prepare(main: str, units: list[str]):
    """`(map_set, linker_set, [(unit, our, target, target_rel)], any_candidates)` - the shared read."""
    map_set = map_rows(main)
    linker_set = linker_symbols(main)
    known = map_set | linker_set
    prepared = []
    any_candidates = False
    for unit in units:
        our_path = os.path.join(main, SRC_REL, unit + ".o")
        if not os.path.exists(our_path):
            continue                                   # the compile gate names a missing object
        our = load_object(our_path)
        if our is None:
            continue
        target_rel = os.path.normpath(os.path.join(OBJ_REL, unit + ".o"))
        target_path = os.path.join(main, target_rel)
        target = load_object(target_path) if os.path.exists(target_path) else None
        any_candidates = any_candidates or bool(external_candidates(our, known))
        prepared.append((unit, our, target, target_rel))
    return map_set, linker_set, prepared, any_candidates


def refs_of(our: dict, target: dict | None, *, map_set: set[str], linker_set: set[str],
            providers: dict[str, list[str]], ref_count: dict[str, int],
            target_rel: str) -> list[str]:
    """The names `our` relocates that no link input can define, sorted - the snapshot's per-unit set."""
    hits = unresolved_names(our, target, map_set=map_set, providers=providers,
                           ref_count=ref_count, target_rel=target_rel, linker_set=linker_set)
    return sorted({n for n, _ in hits})


def snapshot_base(main: str, units: list[str] | None = None) -> dict:
    """`{unit: {"source": sha, "refs": [name, ...]}}` - the base tree's own unresolved references.

    `land.record_base` calls this once on the clean tree at the batch base (after compiling the batch's
    units), so the gate's row is exactly `current - base`. `units is None` snapshots every object already
    present - the manual `record-base` flow, which does not name its units yet.
    """
    if not os.path.isdir(os.path.join(main, SRC_REL)):
        return {}
    wanted = list(units) if units is not None else discover_units(main)
    map_set, linker_set, prepared, any_candidates = _prepare(main, wanted)
    index = link_symbol_index(main) if any_candidates else {"providers": {}, "ref_count": {}}
    out = {}
    for unit, our, target, target_rel in prepared:
        out[unit] = {"source": source_sha(main, unit),
                     "refs": refs_of(our, target, map_set=map_set, linker_set=linker_set,
                                     providers=index["providers"], ref_count=index["ref_count"],
                                     target_rel=target_rel)}
    return out


# ---------------------------------------------------------------------------------------------------------
# the base snapshot of a *revision* (`--base <rev>`), reconstructed from the git objects
# ---------------------------------------------------------------------------------------------------------

BASE_WORKTREE_PREFIX = "undefrefs-base-"


def _unit_stem(unit: str) -> str:
    """`Pl/fn_8025F088.cpp` -> `Pl/fn_8025F088`; the unit vocabulary is extensionless."""
    unit = unit.replace("\\", "/").strip("/")
    for ext in SOURCE_EXTS:
        if unit.endswith(ext):
            return unit[: -len(ext)]
    return unit


def _base_source_rel(wt: str, unit: str) -> str | None:
    """`src/<unit>.<ext>` as the base worktree spells it, or None when the base never had the unit."""
    stem = _unit_stem(unit)
    for ext in SOURCE_EXTS:
        rel = "src/%s%s" % (stem, ext)
        if os.path.exists(os.path.join(wt, rel)):
            return rel
    return None


def _add_base_worktree(main: str, rev: str, path: str) -> None:
    """A detached worktree of `rev` at `path` (which must not exist yet)."""
    p = subprocess.run(["git", "worktree", "add", "--detach", path, rev], cwd=main,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise RuntimeError("git worktree add %s failed: %s" % (rev, (p.stderr or p.stdout).strip()))


def _remove_base_worktree(main: str, path: str) -> None:
    """Remove the temporary worktree, dirty or not (it only ever holds compiled scratch)."""
    if os.path.exists(path):
        subprocess.run(["git", "worktree", "remove", "--force", path], cwd=main,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    subprocess.run(["git", "worktree", "prune"], cwd=main,
                   capture_output=True, text=True, encoding="utf-8", errors="replace")


def _compile_base_unit(unit: str, main: str, wt: str) -> dict:
    """Compile `unit` from the base worktree with the command `recompile.py` borrows from MAIN."""
    from tools.units import recompile  # noqa: PLC0415 - keep the import off the hot `--census` path
    rel = _base_source_rel(wt, unit) or ("src/%s.cpp" % _unit_stem(unit))
    return recompile.compile_unit(rel[len("src/"):], main, wt)


def snapshot_base_at(main: str, rev: str, units: list[str], *, add_worktree=None, remove_worktree=None,
                     compiler=None) -> dict:
    """`snapshot_base` for a **revision**: compile its units in a temporary worktree and read *those*.

    `--base <rev>` used to answer `UNJUDGED`, so a lane proving a refusal pre-existing had to revert the
    changed files and rebuild the objects by hand - four minutes for two objects. This reconstructs the
    same snapshot from the git objects: a detached worktree of `rev`, the named units compiled against
    the base headers, then the same `unresolved_names` comparison `record-base`/`snapshot_base` make. A
    unit the base never had is recorded with no refs (every reference is the batch's own). The worktree
    is removed afterwards, success or failure.

    `add_worktree`/`remove_worktree`/`compiler` are injection points for the selftest, which has no git
    and no compiler; production calls pass the three defaults.
    """
    units = [_unit_stem(u) for u in units if u.strip("/")]
    if not units:
        return {}
    add = add_worktree or _add_base_worktree
    remove = remove_worktree or _remove_base_worktree
    compile_unit = compiler or _compile_base_unit
    parent = tempfile.mkdtemp(prefix=BASE_WORKTREE_PREFIX)
    wt = os.path.join(parent, "tree")
    try:
        add(main, rev, wt)
        snapshot: dict = {}
        map_set = map_rows(wt)
        linker_set = linker_symbols(wt)
        prepared = []
        for unit in units:
            rel = _base_source_rel(wt, unit)
            if rel is None:
                snapshot[unit] = {"source": None, "refs": []}      # the base never had this unit
                continue
            result = compile_unit(unit, main, wt) or {}
            obj = result.get("object")
            our = load_object(obj) if obj and os.path.exists(obj) else None
            if our is None:
                snapshot[unit] = {"source": None, "refs": None}    # the base could not be read: `missing`
                continue
            target_rel = os.path.normpath(os.path.join(OBJ_REL, unit + ".o"))
            target_path = os.path.join(main, target_rel)
            target = load_object(target_path) if os.path.exists(target_path) else None
            prepared.append((unit, our, target, target_rel, rel))
        known = map_set | linker_set
        any_candidates = any(external_candidates(our, known) for _u, our, _t, _r, _s in prepared)
        index = link_symbol_index(main) if any_candidates else {"providers": {}, "ref_count": {}}
        for unit, our, target, target_rel, rel in prepared:
            path = os.path.join(wt, rel)
            sha = hashlib.sha1(open(path, "rb").read()).hexdigest() if os.path.exists(path) else None
            snapshot[unit] = {"source": sha,
                              "refs": refs_of(our, target, map_set=map_set, linker_set=linker_set,
                                              providers=index["providers"],
                                              ref_count=index["ref_count"], target_rel=target_rel)}
        return snapshot
    finally:
        try:
            remove(main, wt)
        finally:
            shutil.rmtree(parent, ignore_errors=True)


def census(main: str) -> list[tuple[str, str, str | None, str | None]]:
    """`[(unit, referenced name, target spelling, how)]` for every pre-existing unresolved reference.

    The register of the debt the add-only row reports but never refuses - not the gate's output.
    """
    map_set, linker_set, prepared, any_candidates = _prepare(main, discover_units(main))
    index = link_symbol_index(main) if any_candidates else {"providers": {}, "ref_count": {}}
    rows = []
    for unit, our, target, target_rel in prepared:
        for name, hint in unresolved_names(our, target, map_set=map_set, providers=index["providers"],
                                           ref_count=index["ref_count"], target_rel=target_rel,
                                           linker_set=linker_set):
            rows.append((unit, name, hint[0] if hint else None, hint[1] if hint else None))
    return rows


def census_markdown(rows: list[tuple[str, str, str | None, str | None]]) -> str:
    """The ranked register: most-affected unit first, one line per (unit, referenced name)."""
    counts: dict[str, int] = {}
    for unit, _name, _spelling, _how in rows:
        counts[unit] = counts.get(unit, 0) + 1
    ordered = sorted(rows, key=lambda r: (-counts[r[0]], r[0], r[1]))
    lines = [
        "# Undefined-reference census (pre-existing debt, never a gate refusal)",
        "",
        "`python tools/units/undefrefs.py --census-out <path>` - wrong-linkage/undefined references already",
        "in the landed `NonMatching` units' objects. The add-only gate row reports these as debt and refuses",
        "only the ones a batch *adds*; this register is where the pre-existing ones are worked down slowly",
        "(the credit ratio), the way rule-7 naming debt was.",
        "",
        "%d reference(s) across %d unit(s), ranked by unit." % (len(rows), len(counts)),
        "",
        "| # | unit | refs | referenced name | map spelling (same stem/offset) |",
        "|---|------|------|-----------------|--------------------------------|",
    ]
    rank = 0
    for unit, name, spelling, how in ordered:
        rank += 1
        lines.append("| %d | `%s` | %d | `%s` | %s |"
                     % (rank, unit, counts[unit], name,
                        ("`%s` (%s)" % (spelling, how)) if spelling else "_(no target spelling)_"))
    lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------------------------------------------------
# the linkage census (`--census --linkage`, relocaudit folded in): our object's symbol spellings vs the target's
# ---------------------------------------------------------------------------------------------------------

# EABI register-save/restore helpers encode register allocation, not linkage: reported apart, never a repair.
COMPILER_HELPER_RE = re.compile(r"^_(?:save|rest)(?:gpr|fpr)_\d+$")
# The source/header extensions the declaration lookup reads, and how many rows it names per symbol.
DECL_EXT = {".c", ".cpp", ".cc", ".cxx", ".cp", ".h", ".hpp", ".hh"}
DECL_CAP = 4
LINKAGE_KEYS = ("linkage_undefined", "other_undefined", "linkage_defined", "other_defined")


def mismatch_kind(name: str) -> str:
    """`compiler-helper` for an EABI register save/restore helper, `extra` for any other unmatched spelling."""
    return "compiler-helper" if COMPILER_HELPER_RE.match(name) else "extra"


def registered_units(main: str) -> list[dict]:
    """Every registered unit from `configure.py` (`lib.project.Configure`, the one reader of `config.libs`)."""
    return [{"path": o.path, "flag": o.flag}
            for o in _project.Configure.load(os.path.join(main, "configure.py")).objects()]


def object_paths(main: str, source_path: str) -> tuple[str, str]:
    """`(our_object, target_object)` for a registered source path (its stem under build/RMHE08/{src,obj})."""
    stem = os.path.splitext(source_path.replace("\\", "/"))[0]
    return (os.path.join(main, "build", "RMHE08", "src", stem + ".o"),
            os.path.join(main, "build", "RMHE08", "obj", stem + ".o"))


def declaration_index(main: str) -> dict:
    """`{token: [(relpath, lineno, text)]}` for every identifier on a declaration-shaped line of `src/`/`include/`
    (a line with `(` or `;` that is not a comment or a directive): where the repair goes, in one walk."""
    idx: dict[str, list] = {}
    for base in ("src", "include"):
        root = os.path.join(main, base)
        if not os.path.isdir(root):
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d != "__pycache__"]
            for fn in filenames:
                if os.path.splitext(fn)[1].lower() not in DECL_EXT:
                    continue
                p = os.path.join(dirpath, fn)
                try:
                    text = open(p, encoding="utf-8", errors="replace").read()
                except OSError:
                    continue
                rel = os.path.relpath(p, main).replace("\\", "/")
                for lineno, line in enumerate(text.splitlines(), 1):
                    stripped = line.strip()
                    if not stripped or stripped[0] in "*/#" or ("(" not in line and ";" not in line):
                        continue
                    for tok in set(re.findall(r"[A-Za-z_]\w*", line)):
                        idx.setdefault(tok, []).append((rel, lineno, stripped))
    return idx


def declarations_for(idx: dict, name: str) -> list[dict]:
    """Up to `DECL_CAP` src/header rows that spell `name`'s linkage stem, header rows first."""
    rows = idx.get(linkage_stem(name), [])
    rows = sorted(rows, key=lambda r: (0 if r[0].endswith((".h", ".hpp", ".hh")) else 1, r[0], r[1]))
    return [{"file": f, "line": n, "text": t} for f, n, t in rows[:DECL_CAP]]


def audit_unit(main: str, source_path: str, idx: dict | None = None) -> dict:
    """One unit's linkage record (`objcompare.linkage_audit`); `status` is `clean`, `suspect` or `unbuilt`."""
    our_obj, tgt_obj = object_paths(main, source_path)
    our = objcompare.linkage_sets(our_obj)
    tgt = objcompare.linkage_sets(tgt_obj)
    if our is None or tgt is None:
        missing = [p for p, s in ((our_obj, our), (tgt_obj, tgt)) if s is None]
        return {"unit": source_path, "status": "unbuilt", "our": our_obj, "target": tgt_obj,
                "missing": [os.path.relpath(p, main).replace("\\", "/") for p in missing],
                "linkage_undefined": [], "other_undefined": [], "linkage_defined": [], "other_defined": []}
    rec = objcompare.linkage_audit(our[0], our[1], tgt[0], tgt[1])
    for key in ("linkage_undefined", "linkage_defined"):
        for m in rec[key]:
            m["kind"] = "linkage"
    for key in ("other_undefined", "other_defined"):
        for m in rec[key]:
            m["kind"] = mismatch_kind(m["our"])
    rec["status"] = "suspect" if any(rec[k] for k in rec) else "clean"
    rec["unit"] = source_path
    rec["our"] = os.path.relpath(our_obj, main).replace("\\", "/")
    rec["target"] = os.path.relpath(tgt_obj, main).replace("\\", "/")
    if idx is not None:
        rec["declarations"] = {m["our"]: declarations_for(idx, m["our"]) for key in LINKAGE_KEYS for m in rec[key]}
    return rec


def linkage_sweep(main: str, with_decls: bool = True, only: str | None = None) -> dict:
    """Audit every registered unit's spellings; the counts, the suspect records and the wall-clock cost."""
    main = os.path.abspath(main)
    t0 = time.time()
    units = registered_units(main)
    if only:
        want = only.replace("\\", "/").strip("/")
        units = [u for u in units
                 if u["path"] == want or os.path.splitext(u["path"])[0] == os.path.splitext(want)[0]]
    idx = declaration_index(main) if with_decls else None
    records = [audit_unit(main, u["path"], idx) for u in units]
    suspects = [r for r in records if r["status"] == "suspect"]
    return {"root": main, "units_total": len(records),
            "units_built": sum(1 for r in records if r["status"] != "unbuilt"),
            "unbuilt": [r for r in records if r["status"] == "unbuilt"],
            "clean": sum(1 for r in records if r["status"] == "clean"),
            "undefined_suspects": sum(1 for r in suspects if r["linkage_undefined"] or r["other_undefined"]),
            "defined_suspects": sum(1 for r in suspects if r["linkage_defined"] or r["other_defined"]),
            "suspects": suspects, "elapsed_s": round(time.time() - t0, 3)}


def render_linkage_row(m: dict) -> str:
    return "    our %-56s -> target %s" % (m["our"], ", ".join(m["target"]) or "-")


def render_linkage(s: dict, out=None) -> None:
    """The linkage census as text (relocaudit's report, label kept so a reader's grep still finds it)."""
    out = out or sys.stdout
    print("relocaudit: %d registered units, %d built, %d clean, %d suspect (elapsed %.2fs)"
          % (s["units_total"], s["units_built"], s["clean"], len(s["suspects"]), s["elapsed_s"]), file=out)
    print("  suspects with an UNDEFINED-set disagreement: %d" % s["undefined_suspects"], file=out)
    print("  suspects with a DEFINED-set disagreement:   %d" % s["defined_suspects"], file=out)
    if s["unbuilt"]:
        print("  unbuilt (no object to compare): %d" % len(s["unbuilt"]), file=out)
        for r in s["unbuilt"]:
            print("    %-48s missing %s" % (r["unit"], ", ".join(r["missing"])), file=out)
    if not s["suspects"]:
        print("  no suspect units.", file=out)
        return
    for r in s["suspects"]:
        print("", file=out)
        print("  %s" % r["unit"], file=out)
        for label, key in (("wrong linkage (undefined)", "linkage_undefined"),
                           ("extra reference, no target spelling (undefined)", "other_undefined"),
                           ("wrong linkage (defined)", "linkage_defined"),
                           ("stray definition, no target spelling (defined)", "other_defined")):
            for m in r[key]:
                if key.startswith("other") and m.get("kind") == "compiler-helper":
                    print("  compiler register-save helper (register allocation, not linkage):", file=out)
                else:
                    print("  %s:" % label, file=out)
                print(render_linkage_row(m), file=out)
                for d in (r.get("declarations", {}).get(m["our"]) or []):
                    print("      declared at %s:%d  %s" % (d["file"], d["line"], d["text"]), file=out)



# ---------------------------------------------------------------------------------------------------------
# the batch entry point the gate calls
# ---------------------------------------------------------------------------------------------------------

def check_units(main: str, units: list[str], base_snapshot: dict | None = None,
                base: str | None = None) -> dict:
    """`{"problems": [...], "pre_existing": [...], "missing": [...]}` for the batch's units.

    `problems` are the names this batch *adds* (refused); `pre_existing` is the debt it inherited
    (reported, never refused); `missing` names a unit whose base snapshot was never taken while its source
    existed at the base (a BOOKKEEPING ask, not a claim about the batch). Cheap by construction: the map,
    the linker-provided names and the batch's own objects are read first, and the (cached) link-symbol
    index is only built when at least one unit has a candidate name.
    """
    result = {"problems": [], "pre_existing": [], "missing": []}
    if not units:
        return result
    map_set, linker_set, prepared, any_candidates = _prepare(main, units)
    base_snapshot = base_snapshot or {}
    judged = []
    for unit, our, target, target_rel in prepared:
        entry = base_snapshot.get(unit)
        if entry is not None and entry.get("refs") is None:
            result["missing"].append(unit)             # an explicit cannot-judge marker (`--base` failure)
            continue
        if entry is None:
            if base and base_source_exists(main, base, unit):
                result["missing"].append(unit)
                continue                               # cannot judge add-only without the base snapshot
            base_names = set()                         # a unit the base never had: every reference is new
        else:
            base_names = set(entry.get("refs") or [])
        judged.append((unit, our, target, target_rel, base_names))
    if not any_candidates:
        return result
    index = link_symbol_index(main)
    for unit, our, target, target_rel, base_names in judged:
        problems, line = check_object(unit, our, target, map_set=map_set, providers=index["providers"],
                                      ref_count=index["ref_count"], target_rel=target_rel,
                                      linker_set=linker_set, base_names=base_names)
        result["problems"] += problems
        if line:
            result["pre_existing"].append(line)
    return result


def selftest() -> int:
    from tools.units import undefrefs_selftest
    return undefrefs_selftest.selftest()


def _load_snapshot(path: str) -> dict:
    """Read a base snapshot from a `.pi/land-base.json` (`{"undefrefs": {...}}`) or a bare mapping."""
    try:
        loaded = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError):
        return {}
    if isinstance(loaded, dict) and isinstance(loaded.get("undefrefs"), dict):
        return loaded["undefrefs"]
    return loaded if isinstance(loaded, dict) else {}


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Refuse a unit whose object ADDS a relocation name no link input can define (a flip "
                    "blocker no score can see); pre-existing debt is reported, never refused.")
    ap.add_argument("units", nargs="*", help="unit names/paths, e.g. quest/arenatask")
    ap.add_argument("--main", default=os.path.dirname(os.path.dirname(HERE)),
                    help="the tree holding build/ and config/ (default: this tool's tree)")
    ap.add_argument("--base-snapshot", metavar="PATH",
                    help="the base snapshot (a `.pi/land-base.json` or a bare `{unit: {refs}}`)")
    ap.add_argument("--base", default=None,
                    help="the batch base commit: with no --base-snapshot, its own unresolved references "
                         "are reconstructed from the git objects (a temporary worktree, the named units "
                         "compiled there), so a pre-existing refusal is answerable from one command")
    ap.add_argument("--snapshot-base", metavar="PATH",
                    help="write THIS tree's unresolved-reference snapshot to PATH and exit")
    ap.add_argument("--census", action="store_true",
                    help="print the pre-existing-debt register and exit")
    ap.add_argument("--census-out", metavar="PATH", help="write the register to PATH (markdown)")
    ap.add_argument("--linkage", action="store_true",
                    help="with --census: the linkage census instead - every registered unit's symbol spellings "
                         "against its target object's (wrong-linkage rows, the folded relocaudit sweep)")
    ap.add_argument("--unit", default=None, help="--census --linkage: one registered unit (path, any extension)")
    ap.add_argument("--no-decls", action="store_true",
                    help="--census --linkage: skip the src/include declaration lookup")
    ap.add_argument("--rebuild-index", action="store_true", help="ignore the cached link-symbol index")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.linkage:
        sweep = linkage_sweep(args.main, with_decls=not args.no_decls, only=args.unit)
        if args.json:
            print(json.dumps(sweep, indent=2))
        else:
            render_linkage(sweep)
        return 0
    if args.rebuild_index:
        link_symbol_index(args.main, rebuild=True)
    if args.snapshot_base:
        snap = snapshot_base(args.main)
        with open(args.snapshot_base, "w", encoding="utf-8") as fh:
            json.dump(snap, fh, indent=1)
        print("%d unit(s) snapshotted to %s" % (len(snap), args.snapshot_base))
        return 0
    if args.census or args.census_out:
        text = census_markdown(census(args.main))
        if args.census_out:
            os.makedirs(os.path.dirname(os.path.abspath(args.census_out)), exist_ok=True)
            with open(args.census_out, "w", encoding="utf-8") as fh:
                fh.write(text)
            print("census written to %s" % args.census_out)
        else:
            sys.stdout.write(text)
        return 0
    snapshot = _load_snapshot(args.base_snapshot) if args.base_snapshot else {}
    units = [u.strip("/") for u in args.units]
    if args.base and not args.base_snapshot and units:
        # `--base <rev>` is a one-command answer: reconstruct the base snapshot from the git objects so
        # the add-only row can report pre-existing debt instead of asking for a `record-base` first.
        fresh = [u for u in units if u not in snapshot]
        if fresh:
            try:
                snapshot.update(snapshot_base_at(args.main, args.base, fresh))
            except (RuntimeError, SystemExit) as exc:
                # a bad rev: mark the units unjudged rather than letting an absent snapshot refuse them
                print("undefrefs: cannot read the base snapshot: %s" % exc, file=sys.stderr)
                snapshot.update({u: {"source": None, "refs": None} for u in fresh})
    result = check_units(args.main, units, base_snapshot=snapshot, base=args.base)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for p in result["problems"]:
            print("NOT READY  %s" % p)
        for line in result["pre_existing"]:
            print("note       %s" % line)
        for unit in result["missing"]:
            print("UNJUDGED   %s (no base snapshot - re-run record-base)" % unit)
        print("\n%d of %d unit(s) refused" % (len(result["problems"]), len(args.units)))
    return 1 if result["problems"] or result["missing"] else 0


if __name__ == "__main__":
    sys.exit(main())
