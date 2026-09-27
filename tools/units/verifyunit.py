#!/usr/bin/env python3
"""Independent verification for a landed batch: registration completeness, a per-symbol re-measure
that does not read the report it audits, and split-target-object drift.

Three failures got through the previous gate, and each of them has a check here.

1. **A unit registered in name only.**  `hud/fn_80334568`'s source was committed while its
   `configure.py` `Object(...)` line and its `splits.txt` block were left in the index, so the unit
   existed as a file and *not* in the build.  `ninja build/RMHE08/ok` stayed green because a
   `NonMatching` object is never linked, and the compile gate could not see it either: a unit with no
   `configure.py` line has no `build/RMHE08/src/<unit>.o` target to scope `ninja -k 0` to.
   `registration_problems` refuses it: every batch unit must have (a) an `Object(...)` line, (b) a
   `splits.txt` block, and (c) an object target in the build graph.

2. **A measurement taken on trust.**  The regression gate reads `build/RMHE08/report.json`, the very
   file the batch was measured against - so a stale or wrong report is invisible to it.  The one lane
   that did it right (the 8031A6C0 merger) re-derived everything: both objects bit-identical, the
   split target object unchanged pre/post merge, and all 26 symbols re-scored from the objects.
   `verify_units` is that independent path: it re-runs `objdiff report generate` itself over the
   target/candidate objects and compares the result symbol-for-symbol against `report.json`, and it
   reads the objects directly to check the score against the bytes.  `target_drift_problems` adds the
   merger's strongest form: a `splits.txt` change that re-ranges a neighbour moves that neighbour's
   split target object, and a neighbour the batch does not name is refused rather than assumed
   harmless.

3. **A measuring tool that lied.**  `recompile.py --measure` understated every score for months
   because it left ninja's chained `objalign` argument relative.  `measure.py`'s selftest is the
   pattern copied here: the independent run cross-checks against `report generate`'s own output (and
   against the raw object bytes) instead of trusting itself, and `verifyunit_selftest.py` pins each
   check against a fixture that must refuse.

## which `objdiff-verify` (SKILL.md) checks the gate now performs

The gate (through this module) now covers these documented checks; they no longer need to stay manual:

* **§1/§3 rebuild the unit** - the compile gate (`ninja -k 0`, scoped) is the rebuild; this module
  then re-runs `report generate` over the freshly built object pair.
* **§4.5 a symbol present on one side but absent from the other is a mismatch** - `size_gap_problems`
  names every symbol present on both sides that objdiff declines to pair (a >50 % size gap), which is
  exactly the row that otherwise reads as untouched.
* **§5.1 sizes first** - a symbol the report scores 100 % but whose sizes differ is refused
  (`symbol_problems`).
* **§5.2 per-symbol, and a function with no `fuzzy_match_percent` key is 0 %, not 100 %** -
  `_score()` reads an absent key as 0, and `arithmetic_crosscheck` proves that reading by reproducing
  the unit's `fuzzy_match_percent` from its listed partials; a mismatch refuses.
* **§5.3 cross-check the arithmetic** - `arithmetic_crosscheck` is that identity.
* **§5.4 data/byte content, not just size** - `raw_symbol_rows` byte-compares every symbol, so a
  report 100 % whose bytes differ refuses even when the size matches.
* **§7 a report number that disagrees with an object diff is a stale report** - `verify_units`'
  symbol-for-symbol comparison of a *fresh* `report generate` against the committed `report.json`.
* **the merger's regression proof (`.pi/notes/8031a6c0-fn-8031a6c0-e199.md`)** - per-symbol
  reproduction from the objects plus split-target-object pre/post comparison.

Still manual (deliberately not in the gate): naming/home decisions (§2), the symbol→unit lookup
(§4 preamble - it greps `symbols.txt`), the instruction-level `diff_kind` reading (§4.3 - diagnostic,
not a score), the flag hypotheses from `extab`/`.comment` (§5.5-5.6 - a hunch must not refuse a
batch), and the write-up (§6).

    python tools/units/verifyunit.py <unit> [<unit> ...]        # manual run against this tree
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
if os.path.dirname(HERE) not in sys.path:
    sys.path.insert(0, os.path.dirname(HERE))

import measure as ms  # noqa: E402
import unitutil  # noqa: E402

SRC_EXT = (".c", ".cpp", ".cp", ".cxx", ".cc", ".c++", ".C")
# `Object(<kind>, "<unit>")` - the same shape `recompile.OBJECT_RE` and `unionresolve._OBJECT_RE` use.
_OBJECT_RE = re.compile(r"Object\(\s*\w+\s*,\s*\"([^\"]+)\"")
# one or more ninja outputs before the `:`, e.g. `build build\RMHE08\src\hud\fn_80334568.o: mwcc_sjis`
_NINJA_BUILD_RE = re.compile(r"^build\s+(.+?):", re.M)
# objdiff declines to pair a symbol whose one side is more than 50 % smaller than the other.
OBJDIFF_SIZE_GAP = 1.5
# a float `fuzzy_match_percent` reproduces from `size * score / 100` up to its report rounding.
ARITH_TOL = 0.01


# --------------------------------------------------------------------------------------------------
# names and paths
# --------------------------------------------------------------------------------------------------

def unit_stem(unit: str) -> str:
    """`src/hud/fn_80334568.cpp` / `main/hud/fn_80334568` / `build/RMHE08/src/...o` -> `hud/fn_80334568`.

    The key every check is quoted under. The gate spells a unit the way `claims.norm_unit` does
    (extension stripped, `src/` or `main/` prefix removed), and the report names it `main/<stem>`, so
    normalising here keeps the three spellings one key.
    """
    u = (unit or "").replace("\\", "/").strip().strip("/")
    for pre in ("build/RMHE08/src/", "build/RMHE08/obj/", "src/", "main/"):
        if u.startswith(pre):
            u = u[len(pre):]
    for ext in SRC_EXT + (".o",):
        if u.endswith(ext):
            return u[: -len(ext)]
    return u


def src_object_rel(unit: str) -> str:
    """`hud/fn_80334568` -> `build/RMHE08/src/hud/fn_80334568.o` (the candidate object target)."""
    return os.path.join("build", "RMHE08", "src", *(unit_stem(unit).split("/"))) + ".o"


def target_object_rel(unit: str) -> str:
    """`hud/fn_80334568` -> `build/RMHE08/obj/hud/fn_80334568.o` (the split target object)."""
    return os.path.join("build", "RMHE08", "obj", *(unit_stem(unit).split("/"))) + ".o"


def report_unit_name(unit: str) -> str:
    """The name the objdiff report uses for a unit: `main/<stem>`."""
    return "main/" + unit_stem(unit)


def _join(main: str, rel: str) -> str:
    return os.path.join(main, *rel.replace("/", os.sep).split(os.sep))


# --------------------------------------------------------------------------------------------------
# 1. registration completeness
# --------------------------------------------------------------------------------------------------

def configure_object_names(text: str) -> list[str]:
    """Every unit name a `configure.py` text registers through `Object(...)`, in file order."""
    return [m.group(1) for m in _OBJECT_RE.finditer(text or "")]


def splits_unit_names(text: str) -> set[str]:
    """The unit keys a `splits.txt` text defines - an unindented `name:` line, `Sections:` excluded.

    The same parse `unionresolve.split_units` and `land._split_rows` use. Returned as stems so the
    comparison against `configure.py` names is extension-insensitive.
    """
    out: set[str] = set()
    for line in (text or "").splitlines():
        if line.startswith("Sections:"):
            continue
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":"):
            name = line.strip()[:-1].strip()
            if name:
                out.add(unit_stem(name))
    return out


def build_ninja_targets(text: str) -> set[str]:
    """Every output target a `build.ninja` text declares, normalised to forward slashes.

    Reads the graph directly (what `ninja -t targets` would print, without running ninja): a `build
    <out> [<out> ...]: <rule>` line. Objects are single-line rules, so the `$` line continuations the
    file also carries cannot hide one.
    """
    out: set[str] = set()
    for m in _NINJA_BUILD_RE.finditer((text or "").replace("\\", "/")):
        for token in m.group(1).split():
            if token and not token.startswith(("$", "|")):
                out.add(token)
    return out


def registration_problems(units: list[str], configure_text: str, splits_text: str,
                          build_ninja_text: str) -> list[str]:
    """Units a batch names but that are not actually in the build -> a problem per unit.

    Three independent gates, because each catches a different half-registration: (a) the
    `configure.py` `Object(...)` line, (b) the `splits.txt` block, (c) the object target the build
    graph must carry. A unit can have (a) and (b) and still be missing from a *stale* `build.ninja`
    (`configure.py` was never re-run), and it can have (a) and be absent from `splits.txt` (no split
    range), so all three are asserted rather than one standing in for the rest.
    """
    problems: list[str] = []
    conf = {unit_stem(n) for n in configure_object_names(configure_text)}
    spl = splits_unit_names(splits_text)
    targets = build_ninja_targets(build_ninja_text)
    for unit in units:
        stem = unit_stem(unit)
        if not stem:
            continue
        want = src_object_rel(unit).replace("\\", "/")
        if stem not in conf:
            problems.append("%s: no `Object(...)` line in configure.py" % stem)
        if stem not in spl:
            problems.append("%s: no block in config/RMHE08/splits.txt" % stem)
        if want not in targets:
            problems.append("%s: the build graph has no target %s (registered in name only; a "
                            "`NonMatching` object is never linked, so `ninja build/RMHE08/ok` cannot "
                            "see this)" % (stem, want))
    return problems


def registration_check(main: str, units: list[str]) -> tuple[bool, str]:
    """Read MAIN's registration files and assert every batch unit is in the build. -> (ok, detail)."""
    if not units:
        return True, "no batch unit named"
    conf_path = os.path.join(main, "configure.py")
    spl_path = os.path.join(main, "config", "RMHE08", "splits.txt")
    ninja_path = os.path.join(main, "build.ninja")
    try:
        configure_text = open(conf_path, encoding="utf-8", errors="replace").read()
        splits_text = open(spl_path, encoding="utf-8", errors="replace").read()
    except OSError as exc:
        return False, "cannot read the registration files: %s" % exc
    build_ninja_text = ""
    if os.path.exists(ninja_path):
        build_ninja_text = open(ninja_path, encoding="utf-8", errors="replace").read()
    problems = registration_problems(units, configure_text, splits_text, build_ninja_text)
    if problems:
        return False, "; ".join(problems[:4])
    return True, ("%d unit(s) registered in configure.py, splits.txt and the build graph"
                  % len(units))


# --------------------------------------------------------------------------------------------------
# 2a. split target-object drift (the merger's strongest form)
# --------------------------------------------------------------------------------------------------

def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# The two sections a symbol *rename* rewrites and a `splits.txt` re-range does not: the string tables a
# rename touches by definition. Measured 2026-09-26 on `worker/803250b0-fn-803250b0-2a24`, whose 18
# renames moved a neighbour's object hash without re-ranging anything: `extab`, `extabindex`, `.text`,
# `.relaextabindex`, `.rela.text`, `.comment`, `.note.split` and `.shstrtab` were byte-identical and
# only `.symtab`/`.strtab` differed - the renamed callees are *undefined* in that object, and dtk does
# not reorder the symbol table on a rename (`.rela.text` was byte-identical too, so the relocation
# symbol indices did not move either).
_RENAME_FREE_SECTIONS = (".symtab", ".strtab")


def target_object_fingerprint(path: str) -> str:
    """A rename-insensitive content fingerprint of a split target object.

    The drift check must catch a `splits.txt` change that re-ranged a unit the batch does not name,
    and must not fire when the batch renamed symbols: a cross-unit rename legitimately rewrites the
    symbol table of every unit that references the renamed name, and the map diff for it is already in
    the batch. So the hash covers every section except the two string tables (name, size and content),
    plus the defined symbols' geometry - `(value, size, type, section)` with the names dropped. A
    re-range moves bytes or addresses; a rename moves neither, which is the whole point of the split.

    A file that cannot be read as an ELF object falls back to its raw hash, so an unreadable object
    that changed is still drift rather than a crash.
    """
    try:
        secs, syms = unitutil.read_elf(path)
    except Exception:                                            # not an ELF (or truncated): raw hash
        return "raw:" + sha256_file(path)
    h = hashlib.sha256()
    for sec in secs:
        if sec["sname"] in _RENAME_FREE_SECTIONS:
            continue
        h.update(("%s\0%08x\0" % (sec["sname"], sec["size"])).encode())
        h.update(sec["data"])
    for geom in sorted((v, size, typ, shndx) for _n, v, size, typ, shndx in syms):
        h.update(("|%08x/%08x/%d/%d" % geom).encode())
    return h.hexdigest()


def target_object_snapshot(main: str) -> dict[str, str | None]:
    """`{unit_stem: content fingerprint | None}` for every unit `splits.txt` defines, before or after a build.

    Scoped to the *registered units'* split objects, not every `.o` under `build/RMHE08/obj/`: the
    retired `auto_*_text` objects there are not units and dtk does not reproduce four of them
    byte-for-byte, so including them would report drift that no registration caused. `None` is a unit
    whose object does not exist yet (a batch's newly registered range, before the split runs).

    The fingerprint is `target_object_fingerprint`, not the file's hash: a rename must not read as a
    re-range (see that function).
    """
    try:
        splits_text = open(_join(main, "config/RMHE08/splits.txt"), encoding="utf-8",
                           errors="replace").read()
    except OSError:
        return {}
    out: dict[str, str | None] = {}
    for stem in sorted(splits_unit_names(splits_text)):
        path = _join(main, target_object_rel(stem))
        out[stem] = target_object_fingerprint(path) if os.path.exists(path) else None
    return out


def target_drift_problems(before: dict[str, str | None], after: dict[str, str | None],
                          batch_stems: list[str]) -> list[str]:
    """Target objects that moved under the batch, naming any unit the batch does not own.

    A unit the batch names may legitimately change (its own range may be widened or newly
    registered); every *other* unit's split target object must be content-identical, because target
    objects come from the DOL split and only `splits.txt` re-ranges them. So a neighbour that moved is
    a `splits.txt` change that re-ranged a unit the batch does not own - refused, not assumed
    harmless. A newly registered unit (present-after, absent-before) is only ever a batch unit.

    "Moved" is `target_object_fingerprint`'s reading, so a batch that only *renamed* symbols (the map
    is shared, and a cross-unit rename rewrites every referencing unit's symbol table) is not drift.
    """
    batch = {unit_stem(u) for u in batch_stems}
    problems: list[str] = []
    for stem in sorted(set(before) | set(after)):
        b, a = before.get(stem), after.get(stem)
        if b == a or stem in batch:
            continue
        if a is None:
            problems.append("%s: its split target object disappeared under the batch" % stem)
        elif b is None:
            problems.append("%s: a split target object appeared for a unit the batch does not name "
                            "(the registration re-ranged a neighbour)" % stem)
        else:
            problems.append("%s: its split target object's content changed under the batch (%s -> %s) "
                            "- a `splits.txt` change re-ranged a neighbour rather than leaving it "
                            "alone (its bytes, addresses or sections moved, not just its names)"
                            % (stem, b[:8], a[:8]))
    return problems


# --------------------------------------------------------------------------------------------------
# 2b/3. the independent per-symbol re-measure
# --------------------------------------------------------------------------------------------------

def object_symbols(path: str) -> dict[str, tuple[int, bytes]]:
    """`{symbol_name: (size, bytes)}` for an ELF object - functions and data alike.

    The raw side of the comparison: it never touches objdiff or the report. A duplicate name keeps the
    first definition (the map can carry aliases); a symbol the object does not define (section index
    0) is skipped.
    """
    secs, syms = unitutil.read_elf(path)
    out: dict[str, tuple[int, bytes]] = {}
    for name, val, size, _typ, shndx in syms:
        if name in out:
            continue
        try:
            sec = secs[shndx]
        except (IndexError, TypeError):
            continue
        out[name] = (size, bytes(sec["data"][val:val + size]))
    return out


def raw_symbol_rows(target_obj: str, candidate_obj: str) -> dict[str, dict]:
    """Per symbol: both sizes, both presences, and whether the bytes are identical."""
    t = object_symbols(target_obj)
    c = object_symbols(candidate_obj)
    rows: dict[str, dict] = {}
    for name in set(t) | set(c):
        te, ce = t.get(name), c.get(name)
        rows[name] = {
            "target_size": te[0] if te else None,
            "candidate_size": ce[0] if ce else None,
            "in_target": te is not None,
            "in_candidate": ce is not None,
            "identical": bool(te and ce and te[0] == ce[0] and te[1] == ce[1]),
        }
    return rows


def report_unit(report_data: dict, unit: str) -> dict | None:
    """The report.json entry for a unit (`main/<stem>`), or None."""
    want = report_unit_name(unit)
    for entry in (report_data or {}).get("units") or []:
        if entry.get("name") == want:
            return entry
    return None


def report_functions(entry: dict | None) -> dict[str, dict]:
    """`{function_name: entry}` for a report.json unit entry."""
    out: dict[str, dict] = {}
    for fn in ((entry or {}).get("functions") or []):
        name = fn.get("name")
        if name:
            out[name] = fn
    return out


def _score(entry: dict | None):
    """The report's score, or None when the entry is absent.

    **A function with no `fuzzy_match_percent` key is 0 %, not 100 %** (SKILL §5.2): the unit's fuzzy
    equals the sum over the *listed* partials, so an unscored function contributes nothing. Callers
    that need a number read `_score(entry) or 0.0`; callers that need "is this scored at all" read the
    None.
    """
    if not entry:
        return None
    value = entry.get("fuzzy_match_percent")
    return float(value) if isinstance(value, (int, float)) else None


def arithmetic_crosscheck(measures: dict, functions: dict[str, dict],
                          tol: float = ARITH_TOL) -> tuple[bool, str]:
    """`sum(size * score / 100) / total_code == fuzzy_match_percent`, absent key = 0.

    The SKILL §5.3 identity. It is what proves the `fuzzy_match_percent`-absent trap was read
    correctly: with an absent key read as 0 the identity holds (measured on
    `main/hud/fn_80334568`: 15.664868 computed vs 15.664868 reported), and with it read as 100 the
    same unit reads 99.999 - so a mismatch here means one of the two readings is wrong and the
    measurement cannot be trusted.
    """
    try:
        total = int(measures.get("total_code"))
    except (TypeError, ValueError, AttributeError):
        return True, "no total_code to check"
    reported = measures.get("fuzzy_match_percent")
    if not isinstance(reported, (int, float)):
        return True, "no unit fuzzy_match_percent to check"
    matched = 0.0
    for fn in functions.values():
        try:
            size = int(fn.get("size"))
        except (TypeError, ValueError):
            continue
        matched += size * (_score(fn) or 0.0) / 100.0
    computed = (100.0 * matched / total) if total else 0.0
    if abs(computed - reported) <= tol:
        return True, "sum(check) %.5f == report %.5f" % (computed, reported)
    return False, ("per-symbol sum gives %.5f but the unit reports %.5f - a function with no "
                   "fuzzy_match_percent key reads as 0%%, not 100%%" % (computed, reported))


def size_gap_problems(rep_funcs: dict[str, dict], raw: dict[str, dict]) -> list[str]:
    """Symbols present in BOTH objects that objdiff refuses to pair, so they read as untouched.

    objdiff declines a pair when one side is >50 % smaller. The dangerous shape is not an unwritten
    body (the candidate does not define the symbol at all - that is honest 0 %), it is a symbol our
    object *does* define at a wildly different size: the report then lists it with no
    `fuzzy_match_percent`, the regression scan sees no drop, and the size gap - the one number that
    would reveal the mistake - is the number objdiff declined to show. Named, so it is visible.
    """
    out: list[str] = []
    for name, row in sorted(raw.items()):
        if not (row["in_target"] and row["in_candidate"]):
            continue
        ts, cs = row["target_size"], row["candidate_size"]
        if not ts or not cs:
            continue
        if max(ts, cs) / min(ts, cs) <= OBJDIFF_SIZE_GAP:
            continue
        if _score(rep_funcs.get(name)) is None:
            out.append("%s: present in both objects but sizes %s vs %s (>50%% apart) - objdiff "
                       "declines the pair, so it reads as untouched" % (name, ts, cs))
    return out


def symbol_problems(rep_funcs: dict[str, dict], fresh: dict[str, dict],
                    raw: dict[str, dict]) -> tuple[list[str], list[str]]:
    """-> (hard problems, advisories) for one unit's symbols.

    `rep_funcs` is `build/RMHE08/report.json`; `fresh` is a `report generate` *we* ran over the same
    object pair; `raw` is the byte comparison. The three are cross-checked against each other, so no
    one of them is taken on trust.

    Hard problems:
    * the two reports disagree about which symbols are paired, or by more than a rounding step - the
      committed report is not reproducible from the objects (SKILL §7);
    * a symbol the report scores 100 % is not byte-identical, or its sizes differ (SKILL §5.1/§5.4).

    Advisory: bytes identical but the report scores below 100 % - a byte-identical symbol should
    close, so this is surfaced but not refused (objdiff's own normalisation is the arbiter).
    """
    hard: list[str] = []
    soft: list[str] = []
    for name in sorted(set(rep_funcs) | set(fresh) | set(raw)):
        r = rep_funcs.get(name)
        f = fresh.get(name)
        rs, fs = _score(r), _score(f)
        if (r is None) != (f is None):
            if r is None and f is not None and not (raw.get(name) or {}).get("in_target"):
                # our object defines a symbol the target does not - not a report disagreement, just a
                # candidate-only helper; surfaced, never a refusal.
                soft.append("%s: our object defines it but the target does not (not in report.json)"
                            % name)
            else:
                # one report knows the symbol, the other does not: the committed report is not
                # reproducible from the objects (a rename, or a stale generation).
                which = "report.json" if r is not None else "a fresh `report generate`"
                hard.append("%s: %s lists it but the other report does not pair it" % (name, which))
        elif r is not None and f is not None:
            if (rs is None) != (fs is None):
                scored = "report.json" if rs is not None else "a fresh `report generate`"
                hard.append("%s: %s scores it but the other report reads it as 0%% (no "
                            "fuzzy_match_percent key)" % (name, scored))
            elif rs is not None and fs is not None and abs(rs - fs) > ARITH_TOL:
                hard.append("%s: report.json %.2f vs fresh `report generate` %.2f - the report is "
                            "not reproducible from the objects" % (name, rs, fs))
        row = raw.get(name)
        if row is not None and r is not None and rs is not None:
            if rs >= 100.0 - 1e-6:
                if row["target_size"] != row["candidate_size"]:
                    hard.append("%s: report scores 100 but the sizes differ (target %s, ours %s)"
                                % (name, row["target_size"], row["candidate_size"]))
                elif not row["identical"]:
                    hard.append("%s: report scores 100 but the %s code bytes are not identical"
                                % (name, row["target_size"]))
            elif row["identical"]:
                soft.append("%s: bytes are identical but the report scores only %.2f" % (name, rs))
    return hard, soft


def _objdiff_path(main: str) -> str:
    cand = os.path.join(main, "build", "tools", "objdiff-cli.exe")
    return cand if os.path.exists(cand) else unitutil.OBJDIFF


def verify_units(main: str, units: list[str], objdiff: str | None = None,
                 runner=subprocess.run) -> tuple[bool, str, list[str]]:
    """Re-measure the batch's units from the objects -> (ok, detail, advisories).

    For every unit: re-run `objdiff report generate` ourselves (a different path from reading
    `build/RMHE08/report.json`), compare it symbol-for-symbol against the committed report, cross-check
    the unit's own arithmetic, and read the two objects to check the score against the bytes. A unit
    whose objects are absent is skipped (`[]` - the compile gate owns "does it build").
    """
    report_path = _join(main, "build/RMHE08/report.json")
    if not os.path.exists(report_path):
        return True, "no build/RMHE08/report.json - the regression gate owns that", []
    try:
        report_data = json.loads(open(report_path, encoding="utf-8").read())
    except (OSError, ValueError) as exc:
        return False, "cannot read build/RMHE08/report.json: %s" % exc, []
    objdiff = objdiff or _objdiff_path(main)
    problems: list[str] = []
    advisories: list[str] = []
    checked = 0
    for unit in units:
        stem = unit_stem(unit)
        if not stem:
            continue
        target = _join(main, target_object_rel(unit))
        candidate = _join(main, src_object_rel(unit))
        if not (os.path.exists(target) and os.path.exists(candidate)):
            continue
        entry = report_unit(report_data, unit)
        if entry is None:
            problems.append("%s: not in build/RMHE08/report.json" % stem)
            continue
        rep_funcs = report_functions(entry)
        ok_arith, arith = arithmetic_crosscheck(entry.get("measures") or {}, rep_funcs)
        if not ok_arith:
            problems.append("%s: %s" % (stem, arith))
        tmpdir = tempfile.mkdtemp(prefix="verifyunit_")
        fresh, _measures, err = ms.score_report(target, candidate, report_unit_name(unit), tmpdir,
                                                objdiff, runner=runner)
        if fresh is None:
            problems.append("%s: a fresh `report generate` failed: %s" % (stem, err))
            continue
        raw = raw_symbol_rows(target, candidate)
        hard, soft = symbol_problems(rep_funcs, fresh, raw)
        problems += ["%s: %s" % (stem, p) for p in hard]
        advisories += ["%s: %s" % (stem, p) for p in soft]
        advisories += ["%s: %s" % (stem, p) for p in size_gap_problems(rep_funcs, raw)]
        checked += 1
    if problems:
        return False, "; ".join(problems[:4]), advisories
    return True, ("%d unit(s) re-measured from the objects; every per-symbol score reproduced and "
                  "every 100%% symbol's bytes matched" % checked), advisories


# --------------------------------------------------------------------------------------------------
# manual entry point
# --------------------------------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("units", nargs="+", help="unit paths, e.g. hud/fn_80334568")
    ap.add_argument("--main", default=None, help="the tree to check (default: this repository)")
    args = ap.parse_args(argv)
    main_root = args.main or unitutil.ROOT
    ok_reg, reg = registration_check(main_root, args.units)
    print("%s registration: %s" % ("PASS" if ok_reg else "FAIL", reg))
    ok_ind, ind, adv = verify_units(main_root, args.units)
    print("%s independent re-measure: %s" % ("PASS" if ok_ind else "FAIL", ind))
    for line in adv:
        print("WARN " + line)
    return 0 if (ok_reg and ok_ind) else 1


if __name__ == "__main__":
    raise SystemExit(main())
