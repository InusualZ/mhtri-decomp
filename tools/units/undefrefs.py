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

**Why it is cheap.** The batch is a handful of units; the only non-trivial part is "which names does the
link provide". `link_symbol_index` caches the link inputs' defined globals and references under
`build/tmp/undefrefs/link-symbols.json` keyed by each input's size+mtime (the pattern `callers.py` uses for
its ELF fallback), and on a miss re-reads only the inputs that changed - so a gate run pays for its own
batch's objects, not the whole 2200-object link. `dossier.parse_elf` is the one ELF reader.

**Not a nuisance.** A unit whose rows are wrong in *other* ways still passes: `Network/NetworkPat`'s 62
wrong vtable slots point at real functions that *are* `symbols.txt` rows, so this row is silent (the
selftest pins it as a negative fixture).

    python tools/units/undefrefs.py <unit> [...]   # the refusal, spelled out
    python tools/units/undefrefs.py --selftest
"""
from __future__ import annotations

import argparse
import json
import os
import re
import struct
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


def check_object(unit: str, our: dict, target: dict | None, *, map_set: set[str],
                 providers: dict[str, list[str]], ref_count: dict[str, int], target_rel: str,
                 linker_set: set[str]) -> list[str]:
    """One problem line when our object relocates a name no link input can define, else []."""
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
        hint = spelling_hint(section, offset, name, target)
        hits.append((name, hint))
    if not hits:
        return []
    rendered = []
    for name, hint in hits:
        rendered.append("%s -> `%s` (the target's spelling at the %s)" % (name, hint[0], hint[1])
                        if hint else name)
    first = hits[0][0]
    return ["%s: %d referenced name(s) are defined by nothing a flip can use - %s - our object does not "
            "define them, `symbols.txt` carries no row and no link input other than the target object "
            "provides them, so a flip answers `undefined: '%s'`"
            % (unit, len(hits), ", ".join(rendered), first)]


# ---------------------------------------------------------------------------------------------------------
# the batch entry point the gate calls
# ---------------------------------------------------------------------------------------------------------

def check_units(main: str, units: list[str]) -> list[str]:
    """The problem line of every batch unit (a unit-shaped entry) that relocates a name nothing defines.

    Cheap by construction: the map, the linker-provided names and the batch's own objects are read first,
    and the (cached) link-symbol index is only built when at least one unit has a candidate name - so a
    clean batch never pays for it.
    """
    if not units:
        return []
    map_set = map_rows(main)
    linker_set = linker_symbols(main)
    known = map_set | linker_set
    prepared = []
    any_candidates = False
    for unit in units:
        our_path = os.path.join(main, SRC_REL, unit + ".o")
        target_rel = os.path.normpath(os.path.join(OBJ_REL, unit + ".o"))
        our = load_object(our_path) if os.path.exists(our_path) else None
        if our is None:
            continue                                   # the compile gate names a missing object
        target_path = os.path.join(main, target_rel)
        target = load_object(target_path) if os.path.exists(target_path) else None
        candidates = external_candidates(our, known)
        any_candidates = any_candidates or bool(candidates)
        prepared.append((unit, our, target, target_rel))
    if not any_candidates:
        return []
    index = link_symbol_index(main)
    problems = []
    for unit, our, target, target_rel in prepared:
        problems += check_object(unit, our, target, map_set=map_set, providers=index["providers"],
                                 ref_count=index["ref_count"], target_rel=target_rel,
                                 linker_set=linker_set)
    return problems


def selftest() -> int:
    sys.path.insert(0, HERE)
    import undefrefs_selftest
    return undefrefs_selftest.selftest()


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Refuse a unit whose object relocates a name no link input can define (a flip blocker "
                    "no score can see).")
    ap.add_argument("units", nargs="*", help="unit names/paths, e.g. quest/arenatask")
    ap.add_argument("--main", default=os.path.dirname(os.path.dirname(HERE)),
                    help="the tree holding build/ and config/ (default: this tool's tree)")
    ap.add_argument("--rebuild-index", action="store_true", help="ignore the cached link-symbol index")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
    problems = []
    for unit in args.units:
        problems += check_units(args.main, [unit.strip("/")])
    if args.json:
        print(json.dumps({"problems": problems}, indent=2))
    else:
        for p in problems:
            print("NOT READY  %s" % p)
        print("\n%d of %d unit(s) refused" % (len(problems), len(args.units)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
