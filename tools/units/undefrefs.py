#!/usr/bin/env python3
"""The flip blocker a score cannot see: a relocation whose name no link input defines.

**The incident this closes (2026-09-28, three times in one day).** A unit's rows can score 96-100 % while
the object calls a symbol that no link input defines, because a `bl` under a *different relocation name*
scores exactly the same as the right one. Measured: `quest/arenatask`'s `arena_eqdata_from_userdata` scored
100.00 % while our object referenced `dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc` and the target's was
`dl_acdata_to_ar_eqdata__FP14_arena_eq_dataUc` (an 11-char struct tag against a 14-char one);
`hud/cockpit_quest`'s two rows scored 96.38 / 96.50 % while referencing `get_move_work_adrs` under C
linkage against the target's mangled `get_move_work_adrs__FUc`; and the Pat vtable's bytes were 114/114
identical while only 50 of 112 slots relocated correctly. Each was a flip blocker, and no gate row that
reads a score could see any of them.

**What this row asks.** Not a relocation diff - one question, per unit in the batch: *does our object
relocate a name that no link input can define?* For each relocation our compiled object
(`build/RMHE08/src/<unit>.o`) carries, the name it references is fine when

* our object defines it itself (a local/partial definition),
* `config/RMHE08/symbols.txt` carries a row for it (the map is the project's name oracle),
* another link input on `main.elf`'s link line defines it (global/weak) - the target object is excluded,
  because a flip *replaces* it,
* the linker script assigns it (`_stack_addr`, ...) or it is the EABI base / entry symbol,
* the target object references it too but does not define it (the reference is already in the link,
  unresolved - the flip adds nothing), or
* no input defines it and some input already references it (the link is already broken the same way).

Everything else is a name that must come from nowhere: a flip would leave it `undefined: '<name>'`. The
refusal names the name, and where the target records a **different spelling at the same relocation offset**
(or, when the layouts differ, the single same-stem spelling in the target) it names both - that spelling is
the fix.

**Add-only (the row is a *delta*, not a verdict).** The tree already carries pre-existing debt: a census
of the landed `NonMatching` units found 61 of 285 with a wrong-linkage/undefined reference already in the
base object (`enemy/enemy_control` calls `ckResourceName` where the map's row is `ckResourceName__FPc`).
Refusing those would refuse every batch that touches such a unit for debt it did not create - the same
mistake `stylelint.py --diff` avoids ("an existing finding never blocks a landing, an *added* one
refuses"). So the row refuses only a name that is **not in the batch base's own unresolved set**: at
`record-base` time the base's objects for the batch's units are compiled once and their unresolved names
cached (`snapshot_base`, keyed by the base source hash); the gate subtracts that set, refuses the new
names only, and **reports** the pre-existing ones as debt (a note naming the unit, the count and the two
spellings for the first). A unit whose base source did not exist is new, so every reference is its own.
The census is a separate register (`--census`), never the gate's output.

**Why it is cheap.** The batch is a handful of units; the only non-trivial part is "which names does the
link provide". `link_symbol_index` caches the link inputs' defined globals and references under
`build/tmp/undefrefs/link-symbols.json` keyed by each input's size+mtime (the pattern `callers.py` uses for
its ELF fallback), and on a miss re-reads only the inputs that changed - so a gate run pays for its own
batch's objects, not the whole 2200-object link. `dossier.parse_elf` is the one ELF reader.

**Not a nuisance.** A unit whose rows are wrong in *other* ways still passes: `Network/NetworkPat`'s 62
wrong vtable slots point at real functions that *are* `symbols.txt` rows, so this row is silent (the
selftest pins it as a negative fixture). And a unit whose *only* wrong reference is pre-existing passes too
(the selftest pins that as the regression test for add-only).

    python tools/units/undefrefs.py <unit> [...]   # the refusal, spelled out
    python tools/units/undefrefs.py --census [PATH]  # the pre-existing-debt register
    python tools/units/undefrefs.py --selftest
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if os.path.dirname(HERE) not in sys.path:
    sys.path.insert(0, os.path.dirname(HERE))          # `tools/`, so `from units import dossier` works
from units import dossier as dossier_mod  # noqa: E402

# The relocation-type reader (`dossier.parse_elf`) is the one ELF scan. Relocations into extab/extabindex
# are compiler bookkeeping, not names our code calls, so they are not this row's business (flipcheck treats
# them the same way).
BOOKKEEPING = ("extab", "extabindex")
# mwldeppc defines the EABI small-data bases itself; `__start` is the linker's default entry root. No object
# and no `symbols.txt` row carries them.
LINKER_SYMBOLS = ("_SDA_BASE_", "_SDA2_BASE_", "__start")
# The linker script's own assignments (`_stack_addr = ...;`): the link supplies these, no object does.
LINKER_ASSIGN_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_.]*)\s*=")
# MWCC's mangling: `__F...` (free function), `__Q<digits>...` (method), `__ct`/`__dt`.
MANGLE_RE = re.compile(r"__(?:F[A-Za-z0-9]|Q\d|ct|dt)")
GLOBAL_BINDING, WEAK_BINDING = 1, 2
SCHEMA = 1
# `build/` is untracked and gitignored, so this never shows in the gate's tree-dirty guard.
CACHE_REL = os.path.join("build", "tmp", "undefrefs", "link-symbols.json")
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
    """The object inputs on `main.elf`'s link line in `build.ninja`, MAIN-relative, normalised like the
    caller builds a target path (`os.path.normpath`). `[]` when there is no link edge (never `None` here:
    a missing link edge is an empty index, not an error)."""
    path = os.path.join(main, NINJA_REL)
    if not os.path.exists(path):
        return []
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    for i, line in enumerate(lines):
        head = line.split(":", 1)[0]
        if not head.startswith("build ") or ": link " not in line or not head.rstrip().endswith("main.elf"):
            continue
        edge = [line]
        while edge[-1].rstrip().endswith("$"):
            i += 1
            edge.append(lines[i])
        inputs = []
        for token in " ".join(edge).replace("$", " ").split()[3:]:     # skip `build`, target, `link`
            if token in ("|", "||"):
                break
            inputs.append(os.path.normpath(token.replace("\\", os.sep)))
        return inputs
    return []


def load_object(path: str) -> dict | None:
    """`{relocs, defined, refs}` for an ELF32 object, or None when it cannot be read.

    `defined` is `{name: (section, info)}` for every symbol the object defines (locals included - a
    relocation to a name the same object defines is not external). `refs` is every name its non-bookkeeping
    relocations reference.
    """
    try:
        blob = open(path, "rb").read()
        _sections, symbols, relocs = dossier_mod.parse_elf(blob)
    except (OSError, ValueError, struct.error):
        return None
    defined = {}
    for s in symbols:
        if s["name"] and s["shndx"]:
            defined[s["name"]] = (s["section"], s["info"])
    refs = set()
    for r in relocs:
        if r["symbol"] and not (r["target"] or "").startswith(BOOKKEEPING):
            refs.add(r["symbol"])
    return {"relocs": relocs, "defined": defined, "refs": refs}


def provides_global(entry) -> bool:
    """Whether a `defined` entry has a binding the linker resolves across objects (global/weak)."""
    return bool(entry) and entry[1] >> 4 in (GLOBAL_BINDING, WEAK_BINDING)


def map_rows(main: str) -> set[str]:
    """The names `config/RMHE08/symbols.txt` carries a row for (the map's definition of a name)."""
    path = os.path.join(main, SYMBOLS_REL)
    if not os.path.exists(path):
        return set()
    out = set()
    for line in open(path, encoding="utf-8", errors="replace"):
        text = line.strip()
        if not text or text.startswith(("#", "//")):
            continue
        name = text.split("=", 1)[0].strip()
        if name:
            out.add(name)
    return out


def linker_symbols(main: str) -> set[str]:
    """The names the linker provides without an object: the script's assignments + the EABI/entry set."""
    out = set(LINKER_SYMBOLS)
    path = os.path.join(main, LDSCRIPT_REL)
    if os.path.exists(path):
        for line in open(path, encoding="utf-8", errors="replace"):
            m = LINKER_ASSIGN_RE.match(line)
            if m:
                out.add(m.group(1))
    return out


def linkage_stem(name: str) -> str:
    """The identifier a mangled name is built from: everything before MWCC's `__<args>` suffix.

    `get_move_work_adrs__FUc` -> `get_move_work_adrs`; `dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc` ->
    `dl_acdata_to_ar_eqdata`. Two names with the same stem are one symbol under two linkages - the fix.
    """
    m = MANGLE_RE.search(name)
    return name[:m.start()] if m else name


# ---------------------------------------------------------------------------------------------------------
# the cached link-symbol index
# ---------------------------------------------------------------------------------------------------------

def _sig(path: str):
    try:
        st = os.stat(path)
    except OSError:
        return None
    return [st.st_mtime_ns, st.st_size]


def link_symbol_index(main: str, cache_path: str | None = None, rebuild: bool = False) -> dict:
    """`{providers: {name: [input, ...]}, ref_count: {name: n}, inputs: [input, ...]}` over the link inputs.

    `providers` is the global/weak definition map; `ref_count` is how many link inputs reference each name
    (the "already unresolved" exemption's signal). The per-input symbol lists are cached under
    `build/tmp/undefrefs/link-symbols.json`, keyed by size+mtime, and only changed inputs are re-read.
    """
    inputs = link_inputs(main)
    cache_path = cache_path or os.path.join(main, CACHE_REL)
    cached = {}
    if not rebuild and os.path.exists(cache_path):
        try:
            loaded = json.load(open(cache_path, encoding="utf-8"))
            if isinstance(loaded, dict) and loaded.get("schema") == SCHEMA:
                cached = loaded.get("inputs") or {}
        except (OSError, ValueError):
            cached = {}
    fresh: dict[str, dict] = {}
    changed = set(cached) != set(inputs)
    for rel in inputs:
        path = os.path.join(main, rel)
        sig = _sig(path)
        if sig is None:
            continue
        old = cached.get(rel)
        if old and old.get("sig") == sig and "defined" in old and "refs" in old:
            fresh[rel] = old
            continue
        changed = True
        facts = load_object(path)
        if facts is None:
            fresh[rel] = {"sig": sig, "defined": [], "refs": []}
            continue
        fresh[rel] = {
            "sig": sig,
            "defined": sorted(n for n, e in facts["defined"].items() if provides_global(e)),
            "refs": sorted(facts["refs"]),
        }
    providers: dict[str, list[str]] = {}
    ref_count: dict[str, int] = {}
    for rel, entry in fresh.items():
        for name in entry["defined"]:
            providers.setdefault(name, []).append(rel)
        for name in entry["refs"]:
            ref_count[name] = ref_count.get(name, 0) + 1
    if changed:
        # only write when an input appeared, vanished or moved: a warm gate run pays no 2 MB serialise
        try:
            os.makedirs(os.path.dirname(cache_path), exist_ok=True)
            tmp = cache_path + ".tmp"
            with open(tmp, "w", encoding="utf-8") as fh:
                json.dump({"schema": SCHEMA, "inputs": fresh}, fh)
            os.replace(tmp, cache_path)
        except OSError:
            pass                               # a read-only tree is not a reason to refuse
    return {"providers": providers, "ref_count": ref_count, "inputs": inputs}


# ---------------------------------------------------------------------------------------------------------
# the rule
# ---------------------------------------------------------------------------------------------------------

def external_candidates(ours: dict, known: set[str]) -> list[tuple[str, int, str]]:
    """`(section, offset, name)` for every non-bookkeeping relocation our object carries whose name it does
    not define and the map/linker does not already name. The link index only has to answer these few."""
    out = []
    seen = set()
    for r in ours["relocs"]:
        name = r["symbol"]
        if not name or (r["target"] or "").startswith(BOOKKEEPING):
            continue
        if name in ours["defined"] or name in known or name in seen:
            continue
        seen.add(name)
        out.append((r["target"], r["offset"], name))
    return out


def spelling_hint(section: str, offset: int, name: str, target: dict | None) -> tuple[str, str] | None:
    """`(target spelling, how)` for a candidate the target object records differently, or None.

    First the exact relocation slot (`Network/arenatask`'s wrong struct tag sits at the same `.text`
    offset). When the layouts differ (`hud/cockpit_quest`: our partial object is much shorter than the
    target's full TU), fall back to the single name in the target with the same linkage stem.
    """
    if target is None:
        return None
    at = {r["symbol"] for r in target["relocs"]
          if r["symbol"] and r["target"] == section and r["offset"] == offset
          and not (r["target"] or "").startswith(BOOKKEEPING)}
    other = sorted(x for x in at if x != name)
    if len(other) == 1:
        return other[0], "same offset"
    stem = linkage_stem(name)
    same = sorted({n for n in (set(target["defined"]) | target["refs"]) if n != name and
                   linkage_stem(n) == stem})
    if len(same) == 1:
        return same[0], "same stem"
    return None


def unresolved_names(our: dict, target: dict | None, *, map_set: set[str],
                     providers: dict[str, list[str]], ref_count: dict[str, int], target_rel: str,
                     linker_set: set[str]) -> list[tuple[str, tuple[str, str] | None]]:
    """The `(name, spelling hint)` our object relocates that no link input can define - the row's set."""
    known = map_set | linker_set
    target_refs = target["refs"] if target is not None else set()
    target_defined = target["defined"] if target is not None else {}
    target_providers = {target_rel} if target is not None else set()
    hits = []
    for section, offset, name in external_candidates(our, known):
        if set(providers.get(name, ())) - target_providers:
            continue                                   # another link input defines it
        if name in target_refs and not provides_global(target_defined.get(name)):
            continue                                   # the target only references it: already unresolved
        if not providers.get(name) and ref_count.get(name, 0) > 0:
            continue                                   # no input defines it, the link already references it
        hits.append((name, spelling_hint(section, offset, name, target)))
    return hits


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
    """Every unit with a compiled object in this tree (`build/RMHE08/src/**/*.o`), extensionless."""
    root = os.path.join(main, SRC_REL)
    out = []
    for dirpath, _dirs, files in os.walk(root):
        for name in sorted(files):
            if name.endswith(".o"):
                out.append(os.path.relpath(os.path.join(dirpath, name), root).replace("\\", "/")[:-2])
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
        hits = unresolved_names(our, target, map_set=map_set, providers=index["providers"],
                                ref_count=index["ref_count"], target_rel=target_rel, linker_set=linker_set)
        out[unit] = {"source": source_sha(main, unit), "refs": sorted({n for n, _ in hits})}
    return out


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
    sys.path.insert(0, HERE)
    import undefrefs_selftest
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


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Refuse a unit whose object ADDS a relocation name no link input can define (a flip "
                    "blocker no score can see); pre-existing debt is reported, never refused.")
    ap.add_argument("units", nargs="*", help="unit names/paths, e.g. quest/arenatask")
    ap.add_argument("--main", default=os.path.dirname(os.path.dirname(HERE)),
                    help="the tree holding build/ and config/ (default: this tool's tree)")
    ap.add_argument("--base-snapshot", metavar="PATH",
                    help="the base snapshot (a `.pi/land-base.json` or a bare `{unit: {refs}}`)")
    ap.add_argument("--base", default=None, help="the batch base commit (for the missing-snapshot ask)")
    ap.add_argument("--snapshot-base", metavar="PATH",
                    help="write THIS tree's unresolved-reference snapshot to PATH and exit")
    ap.add_argument("--census", action="store_true",
                    help="print the pre-existing-debt register and exit")
    ap.add_argument("--census-out", metavar="PATH", help="write the register to PATH (markdown)")
    ap.add_argument("--rebuild-index", action="store_true", help="ignore the cached link-symbol index")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
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
    result = check_units(args.main, [u.strip("/") for u in args.units],
                         base_snapshot=snapshot, base=args.base)
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
