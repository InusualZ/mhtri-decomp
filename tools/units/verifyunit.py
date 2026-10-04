#!/usr/bin/env python3
"""Independent verification for a batch: registration (3 axes), a per-symbol re-measure that does not read the report
it audits, size-gap rows and split-target drift. Spec: docs/tools/spec/verifyunit.md. CLI: verifyunit.py <unit>...
[--main M]."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import subprocess
import tempfile

from tools import unitutil
from tools.lib import objcompare
from tools.lib import project as _project  # the configure / splits readers
from tools.lib import report as _report  # the 0 % rule, the arithmetic identity
from tools.lib import units as _units  # the unit spellings
from tools.units import measure as ms  # the fresh `report generate` (`score_report`)

# one or more ninja outputs before the `:`, e.g. `build build\RMHE08\src\hud\fn_80334568.o: mwcc_sjis`
_NINJA_BUILD_RE = re.compile(r"^build\s+(.+?):", re.M)
# objdiff declines to pair a symbol whose one side is more than 50 % smaller than the other.
OBJDIFF_SIZE_GAP = objcompare.OBJDIFF_SIZE_GAP
ARITH_TOL = _report.ARITH_TOL


# --------------------------------------------------------------------------------------------------
# names and paths
# --------------------------------------------------------------------------------------------------

unit_stem = _units.stem          # every spelling -> `hud/fn_80334568`, the key checks are quoted under


def src_object_rel(unit: str) -> str:
    """`hud/fn_80334568` -> `build/RMHE08/src/hud/fn_80334568.o` (the candidate object target)."""
    return _units.obj_rel(unit, "src")


target_object_rel = _units.target_rel
report_unit_name = _units.report_name


def _join(main: str, rel: str) -> str:
    return os.path.join(main, *rel.replace("/", os.sep).split(os.sep))


# --------------------------------------------------------------------------------------------------
# 1. registration completeness
# --------------------------------------------------------------------------------------------------

def configure_object_names(text: str) -> list[str]:
    """Every unit name a `configure.py` text registers through `Object(...)`, in file order."""
    return [c.path for c in _project.object_calls(text or "")]


def splits_unit_names(text: str) -> set[str]:
    """The unit keys a `splits.txt` text defines - an unindented `name:` line, `Sections:` excluded.

    Read by `lib.project.Splits`. Returned as stems so the comparison against `configure.py` names is
    extension-insensitive.
    """
    return {unit_stem(u) for u in _project.Splits.parse(text or "").units if u.strip()}


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

target_object_fingerprint = objcompare.fingerprint   # rename-insensitive: string tables skipped, names dropped


def target_object_snapshot(main: str) -> dict[str, str | None]:
    """`{unit_stem: content fingerprint | None}` for every unit `splits.txt` defines, before or after a build.

    Scoped to the *registered units'* split objects, not every `.o` under `build/RMHE08/obj/`: the
    retired `auto_*_text` objects there are not units and dtk does not reproduce four of them
    byte-for-byte, so including them would report drift that no registration caused. `None` is a unit
    whose object does not exist yet (a batch's newly registered range, before the split runs).

    The fingerprint is `target_object_fingerprint`, not the file's hash: a rename must not read as a
    re-range (see that function).
    """
    return _target_objects(main, target_object_fingerprint)


def target_object_hashes(main: str) -> dict[str, str | None]:
    """`{unit_stem: sha256 of the split target object's bytes | None}` - the raw half beside
    `target_object_snapshot`: a unit whose bytes changed while its fingerprint did not changed by names only."""
    return _target_objects(main, objcompare.file_sha256)


def _target_objects(main: str, digest) -> dict[str, str | None]:
    try:
        splits_text = open(_join(main, "config/RMHE08/splits.txt"), encoding="utf-8",
                           errors="replace").read()
    except OSError:
        return {}
    out: dict[str, str | None] = {}
    for stem in sorted(splits_unit_names(splits_text)):
        path = _join(main, target_object_rel(stem))
        out[stem] = digest(path) if os.path.exists(path) else None
    return out


def names_only_changes(fp_before: dict, fp_after: dict, raw_before: dict, raw_after: dict,
                       batch_stems: list[str]) -> list[str]:
    """The units the batch does not name whose split target object changed by names only: the same fingerprint,
    different bytes (a `symbols.txt` rename rewrites every referencing unit's symbol table). Not drift - reported so
    the landing can name them."""
    batch = {unit_stem(u) for u in batch_stems}
    return sorted(s for s in set(fp_before) & set(fp_after)
                  if s not in batch and fp_before[s] is not None and fp_before[s] == fp_after[s]
                  and raw_before.get(s) is not None and raw_after.get(s) is not None
                  and raw_before[s] != raw_after[s])


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

symbol_locations = objcompare.symbol_locations   # {name: (section, offset, size, bytes)}, no objdiff involved


def object_symbols(path: str) -> dict[str, tuple[int, bytes]]:
    """`{symbol_name: (size, bytes)}` for an ELF object - `symbol_locations` with the location dropped."""
    return {n: (size, data) for n, (_sec, _off, size, data) in symbol_locations(path).items()}


raw_symbol_rows = objcompare.symbol_rows   # per target symbol: sizes, presence, identity, resolved by name/address


def report_unit(report_data: dict, unit: str) -> dict | None:
    """The report.json entry for a unit (`main/<stem>`), or None."""
    return _report.Report.coerce(report_data).unit(_units.report_name(unit))


def report_functions(entry: dict | None) -> dict[str, dict]:
    """`{function_name: entry}` for a report.json unit entry."""
    out: dict[str, dict] = {}
    for fn in ((entry or {}).get("functions") or []):
        name = fn.get("name")
        if name:
            out[name] = fn
    return out


_score = _report.entry_score          # None when unscored; callers read `or 0.0` for the 0 % rule


arithmetic_crosscheck = _report.arithmetic_check


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

    **A row that resolved by ADDRESS is compared by its bytes, not by its score** (`raw_symbol_rows`):
    objdiff pairs by name, so a row dtk named itself (`pad_*`/`auto_*`) can never pair with our
    object's symbol and *no* `report generate` - the project's own or a fresh one - can score it; the
    two reports therefore disagree by construction, which is not evidence about the objects. What the
    row actually claims is "our object carries the target's bytes at that address", and that is the
    byte rule carried out unchanged below (a 100 % row that is not byte-identical still refuses). Note
    that this is the *load-bearing* rule for such a row: `Object(Matching, ...)` sets
    `metadata.complete` in `objdiff.json`, and objdiff-cli then reports a unit complete without
    diffing it at all (measured 2026-09-28: a deliberately corrupted `.init` still reads 100 %), so
    `report.json`'s score for a Matching unit is a claim the objects have to back, never a
    measurement.
    """
    hard: list[str] = []
    soft: list[str] = []
    for name in sorted(set(rep_funcs) | set(fresh) | set(raw)):
        r = rep_funcs.get(name)
        f = fresh.get(name)
        rs, fs = _score(r), _score(f)
        row = raw.get(name)
        by_address = bool(row and row.get("resolved_by") == "address")
        if (r is None) != (f is None):
            # which symbols a report lists is never address-resolved: a row only one side knows is a
            # rename or a stale generation, whatever the row resolved its bytes through.
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
        elif r is not None and f is not None and not by_address:
            if (rs is None) != (fs is None):
                scored = "report.json" if rs is not None else "a fresh `report generate`"
                hard.append("%s: %s scores it but the other report reads it as 0%% (no "
                            "fuzzy_match_percent key)" % (name, scored))
            elif rs is not None and fs is not None and abs(rs - fs) > ARITH_TOL:
                hard.append("%s: report.json %.2f vs fresh `report generate` %.2f - the report is "
                            "not reproducible from the objects" % (name, rs, fs))
        if row is not None and r is not None and rs is not None:
            # an address-resolved row names the symbol our object actually carries, so a refusal says
            # which two things were compared (dtk's own name never exists on our side).
            ours = name if row.get("resolved_by") != "address" else "%s at the same address" % (
                row.get("candidate_name"),)
            if rs >= 100.0 - 1e-6:
                if row["target_size"] != row["candidate_size"]:
                    hard.append("%s: report scores 100 but the sizes differ (target %s, ours %s [%s])"
                                % (name, row["target_size"], row["candidate_size"], ours))
                elif not row["identical"]:
                    hard.append("%s: report scores 100 but the %s code bytes this unit claims are not "
                                "identical to the target's [%s]"
                                % (name, row["target_size"], ours))
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
