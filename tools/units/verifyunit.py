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
* **a dtk-generated row name is resolved by ADDRESS, not by name** - `dol split` names a range it
  cannot attribute `pad_*`/`auto_*` (the TRK interrupt vectors have no function prologue, so the
  target object carries `pad_00_80004380_init` for the bytes the map calls
  `gTRKInterruptVectorTable`, and the label cannot win: an extent on it makes `dtk dol split` fail on
  the overlap). objdiff pairs by name, so such a row is never paired and *no* report can score it -
  the row is matched to our symbol at the same section+offset and judged by its bytes, which is what
  it actually claims. Related, and the reason that byte rule is load-bearing rather than decorative:
  `Object(Matching, ...)` sets `metadata.complete` in `objdiff.json`, and objdiff-cli then pins that
  unit's completion percent at 100 **while still running its per-symbol diff** (measured 2026-09-28: a
  corrupted `.init` object keeps `complete_code_percent` at 100.0 while its fuzzy percentage falls), so
  the completion field of a Matching unit's report row is a claim the objects themselves must back.
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
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

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
from tools.lib import project as _project  # noqa: E402  (the configure / splits readers)
from tools.lib import report as _report  # noqa: E402  (the 0 % rule, the arithmetic identity)
from tools.lib import units as _units  # noqa: E402  (the unit spellings)

# one or more ninja outputs before the `:`, e.g. `build build\RMHE08\src\hud\fn_80334568.o: mwcc_sjis`
_NINJA_BUILD_RE = re.compile(r"^build\s+(.+?):", re.M)
# objdiff declines to pair a symbol whose one side is more than 50 % smaller than the other.
OBJDIFF_SIZE_GAP = 1.5
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

def symbol_locations(path: str) -> dict[str, tuple[str, int, int, bytes]]:
    """`{symbol_name: (section, offset, size, bytes)}` for an ELF object - functions and data alike.

    The raw side of the comparison: it never touches objdiff or the report. The **section and offset**
    are what let a symbol be found by ADDRESS rather than by name (`raw_symbol_rows`), which a
    dtk-generated name makes necessary. A duplicate name keeps the first definition (the map can carry
    aliases); a symbol the object does not define (section index 0, or an absolute/section index) is
    skipped.
    """
    secs, syms = unitutil.read_elf(path)
    out: dict[str, tuple[str, int, int, bytes]] = {}
    for name, val, size, _typ, shndx in syms:
        if name in out:
            continue
        try:
            sec = secs[shndx]
        except (IndexError, TypeError):
            continue
        out[name] = (sec["sname"], val, size, bytes(sec["data"][val:val + size]))
    return out


def object_symbols(path: str) -> dict[str, tuple[int, bytes]]:
    """`{symbol_name: (size, bytes)}` for an ELF object - `symbol_locations` with the location dropped."""
    return {n: (size, data) for n, (_sec, _off, size, data) in symbol_locations(path).items()}


def _same_place(locations: dict[str, tuple[str, int, int, bytes]],
                section: str, offset: int) -> list[tuple[str, int, bytes]]:
    """Candidate symbols that start exactly at `(section, offset)`, best first.

    Best first = the extent that can actually be compared: a symbol with no bytes (a section symbol,
    or a sizeless map label like the `gTRKInterruptVectorTable` that sits at the same offset as the
    `pad_` symbol dtk generated for it) is never chosen while a real one is available, and the caller
    pulls the equal-size symbol forward before falling back to a differently-shaped one.
    """
    return [(n, size, data) for n, (sec, off, size, data) in locations.items()
            if sec == section and off == offset and size > 0]


def raw_symbol_rows(target_obj: str, candidate_obj: str) -> dict[str, dict]:
    """Per symbol: both sizes, both presences, whether the bytes are identical, and how ours was found.

    A row is keyed by the **target object's** symbol name, because that is the name objdiff lists and
    the name `report.json` is quoted by. For a range dtk cannot attribute, that name is dtk's own
    generated one: the analyzer finds no function prologue in the TRK interrupt vectors, so its target
    object carries `pad_00_80004380_init` for bytes the map names `gTRKInterruptVectorTable` (and the
    map label cannot win - giving it an extent makes `dtk dol split` fail on the overlap). objdiff
    pairs by name, so such a row can never pair, and a name-keyed byte comparison reads our object as
    not defining the symbol at all - which is how a byte-identical claim measured as a refusal
    (2026-09-28, `worker/trk-init-vectors-2226`).

    So when the name lookup misses, the candidate symbol starting at the **same section and offset**
    stands in and `resolved_by` says `"address"`. `identical` stays the strict test it always was -
    equal sizes and equal bytes - so a symbol that resolves by address but is truncated, mis-sized or
    different still refuses; `symbol_problems` is what decides how far the resolution is trusted.
    """
    t = symbol_locations(target_obj)
    c = symbol_locations(candidate_obj)
    rows: dict[str, dict] = {}
    for name in set(t) | set(c):
        te, ce = t.get(name), c.get(name)
        resolved_by = "name" if ce is not None else None
        candidate_name = name if ce is not None else None
        if ce is None and te is not None:
            found = _same_place(c, te[0], te[1])
            found.sort(key=lambda e: (e[1] != te[2], e[1]))   # the equal-size symbol first
            if found:
                candidate_name, cand_size, cand_data = found[0]
                ce, resolved_by = (te[0], te[1], cand_size, cand_data), "address"
        rows[name] = {
            "target_size": te[2] if te else None,
            "candidate_size": ce[2] if ce else None,
            "in_target": te is not None,
            "in_candidate": ce is not None,
            "identical": bool(te and ce and te[2] == ce[2] and te[3] == ce[3]),
            "candidate_name": candidate_name,
            "resolved_by": resolved_by,
        }
    return rows


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
