"""The rule rows: the style lint (rule 12's allowance), rule 2's band boundary, rule 10.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os
import sys

from tools.lib import project as _project
from tools.lib.project.ownership import BAND_ROOTS, band_root
from tools.lib.lanes import naming
import tools.units.stylelint_rules.api as sl
import tools.units.vtableaudit as vta
from tools.units.landing import state
from tools.units.landing.common import Batch, command_detail, run


# --------------------------------------------------------------------------------------------------
# rule 2 at the range boundary: a batch that REGISTERS a range is what makes the symbols inside it owned
# (docs/plan.md 6.5 rule 2), so any declaration of one of them still living in `src/unsplit/<band>.h`
# has just become a violation - and, when the new unit's own definition disagrees, the `(10505) illegal
# overloading` that costs a full build (`-maxerrors 1` only shows it one symbol at a time).
#
# stylelint `--diff` cannot see it: the band header is NOT a file the batch changed, so it is not in the
# diff's file set at all, and the registration edit itself (splits.txt + configure.py) carries no
# declaration to flag. The 2026-09-26 incident: a batch registered `fn_8019E9AC`'s cluster, the band header
# still declared it `s32` while the new unit defined it `u32`, and the gate only found out after a full
# build (`(10505) illegal overloading 'fn_8019E9AC(_ENEMY_WORK *, long)'`).
#
# The check is deliberately a WARNING, never a refusal: a redeclaration whose signature happens to agree
# builds cleanly, and the range can own data or functions the band legitimately spelled the same way - so
# a refusal would not be provably safe (it could block a batch that builds). An extra signal that names the
# symbol, the owner and the stale header is the whole point; the build remains the arbiter.
#
# Every rule-2 warning that says "move the declaration into the owner's header" carries this caveat, because
# that instruction is not always true and following it blindly broke `main` on 2026-09-26: commit `756023c4e`
# moved `fn_80335CE8` into its owner's header (hud/fn_80334568), whose prototype is THREE parameters, while
# the three call sites in `Pl/fn_80273B14.cpp` pass TWO - so they lost their declaration ((10140) undefined
# identifier) and the unit stopped compiling, with `ok` green because it is `NonMatching`. There was no
# malformed input, only an instruction that is not always true.
# --------------------------------------------------------------------------------------------------

RULE2_CALLSITE_CAVEAT = ("after checking every call site: moving a declaration changes its arity if the "
                         "owner's prototype differs")


def _split_rows(text: str) -> list[tuple[str, str, int, int]]:
    """`(unit, section, start, end)` rows of a `splits.txt` text (`lib.project.Splits`)."""
    return [(r.unit, r.section, r.start, r.end) for r in _project.Splits.parse(text).ranges]


def _merge_intervals(spans: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Sorted, merged, non-overlapping `(start, end)` spans."""
    out: list[tuple[int, int]] = []
    for s, e in sorted(spans):
        if s >= e:
            continue
        if out and s <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], e))
        else:
            out.append((s, e))
    return out


def _subtract_intervals(span: tuple[int, int], coverage: list[tuple[int, int]]
                        ) -> list[tuple[int, int]]:
    """The parts of `span` no merged `coverage` interval covers."""
    s, e = span
    out: list[tuple[int, int]] = []
    cur = s
    for cs, ce in coverage:
        if ce <= cur:
            continue
        if cs >= e:
            break
        if cs > cur:
            out.append((cur, cs))
        cur = max(cur, ce)
        if cur >= e:
            break
    if cur < e:
        out.append((cur, e))
    return out


def added_split_ranges(old_rows: list[tuple[str, str, int, int]],
                       new_rows: list[tuple[str, str, int, int]]) -> list[tuple[str, str, int, int]]:
    """Ranges `new_rows` claims that `old_rows` did not - i.e. what the batch newly registers.

    A brand-new unit's whole block is added; an existing unit whose `.text` was *widened* contributes only
    the strip the old range did not already cover (the range the symbols newly inside are owned by). A
    range the batch did not touch is covered by the old rows and contributes nothing, so a clean batch
    yields no added range and the check stays silent.
    """
    old_cov: dict[str, list[tuple[int, int]]] = {}
    for _unit, section, s, e in old_rows:
        old_cov.setdefault(section, []).append((s, e))
    for section in old_cov:
        old_cov[section] = _merge_intervals(old_cov[section])
    out: list[tuple[str, str, int, int]] = []
    for unit, section, s, e in new_rows:
        for cs, ce in _subtract_intervals((s, e), old_cov.get(section, [])):
            out.append((unit, section, cs, ce))
    return out


def _band_header_paths(main: str) -> list[str]:
    """Every band header (`lib.project.ownership.band_root`), as an absolute path (the whole band, changed or not)."""
    base = os.path.join(main, *band_root(main).split("/"))
    out: list[str] = []
    for dirpath, _dirnames, filenames in os.walk(base):
        for name in sorted(filenames):
            if name.endswith((".h", ".hpp", ".hh")):
                out.append(os.path.join(dirpath, name))
    return sorted(out)


def _changed_band_headers(main: str, base: str) -> list[str]:
    """The band headers this batch added or modified (a delete is not a declaration site), under either band root
    (`BAND_ROOTS`: the base may predate the 2026-10-05 move)."""
    p = run(["git", "diff", "--name-status", "-M", "--diff-filter=d", base, "--", *BAND_ROOTS], main)
    if p.returncode != 0:
        return []
    out: list[str] = []
    for line in (p.stdout or "").splitlines():
        parts = line.split("\t")
        if len(parts) >= 2 and parts[-1].endswith((".h", ".hpp", ".hh")):
            out.append(parts[-1])
    return out


def _declared_names(main: str, rel: str) -> set[str]:
    """The file-scope declaration names of `rel` (absolute path), read from the worktree."""
    path = os.path.join(main, *rel.replace("/", os.sep).split(os.sep))
    try:
        text = open(path, encoding="utf-8", errors="replace", newline="").read()
    except OSError:
        return set()
    return {name for name, _line in sl.header_declarations(sl.Source(path, rel, text))}


def _declared_names_at(main: str, base: str, rel: str) -> set[str]:
    """The file-scope declaration names of `rel` at `base`; empty when it did not exist there."""
    p = run(["git", "show", "%s:%s" % (base, rel)], main)
    if p.returncode != 0:
        return set()
    return {name for name, _line in sl.header_declarations(sl.Source(rel, rel, p.stdout))}


def rule10_violations(main: str, text_ref: str | None = None) -> dict | None:
    """`{key: {"unit", "where", "kind"}}` for every rule-10 violation in the tree as it stands.

    The key shape is `vtableaudit.violation_rows`'s, and it is **rename-stable**: `run:<section>:<addr>`
    for a code-pointer run inside the unit's own registered ranges that our object neither emits nor
    references, and `ref:<file>:<line>:<symbol>` for a source assignment to a `+0x00` function-pointer-table
    member whose table the unit itself owns (with `<file>` translated through the batch's renames when
    `text_ref` is the base). `text_ref` judges the text half as of that revision - the gate passes the
    batch base for its BEFORE snapshot, so a unit the batch re-homed (plan §12) must not read as seven
    added violations. `None` when the audit cannot read the tree at all (a missing DOL, a broken
    `configure.py`) - the row then says so instead of refusing every batch.
    """
    try:
        sweep = vta.sweep(main, text_ref=text_ref)
    except Exception as exc:                                   # noqa: BLE001 - the row must never crash
        print("rule 10: vtableaudit could not read this tree (%s)" % exc, file=sys.stderr)
        return None
    rename = vta.rename_map(main, text_ref) if text_ref else {}
    rows = {}
    for key, row in vta.violation_rows(sweep, rename).items():
        # every field kept: a `run:` row's section/address/words are what `vtableaudit.diff_rows` pairs on
        rows[key] = dict(row, unit=naming.norm_unit(row["unit"]))
    return rows


def rule10_growth(before: dict, after: dict, units: list[str]) -> tuple[list[str], list[str]]:
    """`(added_keys, rows_for_the_batch_units)` - the rule-10 row's decision, as a pure function.

    ADD-only, like the lint's `--diff`: a key present before the batch is grandfathered, a key the batch
    introduced is a refusal. "Introduced" is `vtableaudit.diff_rows`'s verdict - the one implementation `--diff`
    uses - so a `run:` key that moved because the run's first word stopped (or started) resolving as a code
    pointer is SHIFTED (paired with the removed run it overlaps in the same section), not added; a `ref:` key is
    a set difference as before. The second element is the report the row prints even when it passes - the
    violation set for the units the batch touches, so a silent pass (rule 10's old "landing review"
    classification) cannot happen again.
    """
    grew = vta.diff_rows(before, after)["added"]
    mine = {naming.norm_unit(u) for u in units}
    touched = [after[k]["where"] for k in sorted(after) if after[k]["unit"] in mine]
    return grew, touched


def rule12_verdict(added: list[dict], detail: list[dict],
                   allowed: list[str]) -> tuple[bool, list[str], list[str], list[dict]]:
    """The rule-12 half of the style-lint row, as a pure function.

    `added`/`detail` are `stylelint --diff --json`'s: the `(rule, file)` count deltas and the added
    occurrences (each carrying its `token`). Only rule-12 additions can ever be excused by
    `--allow-rule12 <token>`; a single added violation of any other rule refuses the row. Returns
    `(ok, excused, refused, other_rules)`. An allowance that matches nothing is not an error by itself -
    it simply excuses nothing, and the refusal it was meant for stands unless its own token is named.
    """
    other = [a for a in added if a.get("rule") != 12]
    if other:
        return False, [], [], other
    sanctioned = set(allowed)
    tokens = [d.get("token") for d in detail if d.get("rule") == 12]
    excused = sorted({t for t in tokens if t and t in sanctioned})
    refused = sorted({t for t in tokens if not t or t not in sanctioned})
    rule12_added = sum(int(a.get("added") or 0) for a in added if a.get("rule") == 12)
    if len(tokens) < rule12_added:
        # the JSON named fewer added occurrences than the counts: the named tokens cannot be proven to
        # cover them, so it refuses. A silent pass on an unverifiable delta is the failure this guards.
        refused.append("<unnamed rule-12 occurrence>")
    return (not other and not refused), excused, refused, []


def rule12_lint_row(p, allowed: list[str] | None) -> tuple[bool, str, str, list[str]]:
    """The style-lint row's `(ok, detail, info, excused)` with `--allow-rule12` in play.

    Rule 12 is refused inside the style lint (it is part of its `--diff`), so the allowance has to be
    applied to *its* decision. `--json` carries the added `(rule, file)` counts and the added
    occurrences; a clean run passes, anything the allowance does not cover - any other rule, or a
    rule-12 token not named - is the refusal, with what stylelint printed. A non-zero exit whose JSON
    carries no `added` (a refusal that is not a delta: no map, no dump) is a failure too, never a pass.
    """
    if p.returncode == 0:
        return True, "", "", []
    try:
        payload = json.loads(p.stdout or "{}")
    except ValueError:
        return False, command_detail(p), "", []
    added = payload.get("added") or []
    if not added:
        return False, command_detail(p), "", []
    detail = payload.get("detail") or []
    ok, excused, refused, _other = rule12_verdict(added, detail, allowed or [])
    if ok:
        return True, "", ("rule 12: %d addition(s) authorised by --allow-rule12" % len(excused)
                           if excused else ""), excused
    lines = ["+%d rule %d %s (%d -> %d)" % (a.get("added", 0), a.get("rule"), a.get("file"),
                                            a.get("before", 0), a.get("after", 0)) for a in added]
    if refused:
        lines.append("unexcused rule 12 (add --allow-rule12 <token> to accept deliberately): %s"
                     % ", ".join(refused))
    return False, "; ".join(lines[:6]) or command_detail(p), "", excused


def band_ownership_warnings(main: str, base: str | None) -> list[str]:
    """Rule-2 warnings a batch introduces at the registration boundary. Never a refusal.

    Two directions, both keyed on the batch's own diff:

    1. the batch REGISTERS a range (splits.txt/configure.py changed): every symbol of `symbols.txt` the
       range newly covers is now owned, so any `src/unsplit/<band>.h` that still *declares* one is a
       rule-2 violation and a candidate `illegal overloading` - whether that header was changed or not;
    2. the batch ADDS a declaration to a band header of a symbol an already-registered unit owns
       (ownership unchanged): the band is a fallback, not the owner.

    Returns one warning line per finding, sorted. `[]` when the map is absent, `base` is unknown, the batch
    touches neither registration file, or nothing is newly owned - so a clean batch is silent.
    """
    if not base:
        return []
    spl_rel = "config/RMHE08/splits.txt"
    sym_path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    spl_path = os.path.join(main, "config", "RMHE08", "splits.txt")
    if not (os.path.exists(sym_path) and os.path.exists(spl_path)):
        return []
    touched = run(["git", "diff", "--name-only", base, "--", spl_rel, "configure.py"], main)
    if touched.returncode != 0 or not (touched.stdout or "").strip():
        return []
    old = run(["git", "show", "%s:%s" % (base, spl_rel)], main)
    if old.returncode != 0:
        return []
    new_text = open(spl_path, encoding="utf-8", errors="replace", newline="").read()
    added = added_split_ranges(_split_rows(old.stdout), _split_rows(new_text))
    changed_band = _changed_band_headers(main, base)
    if not added and not changed_band:
        # no new ownership and no band header the batch touched: nothing this check reasons about
        return []

    symbols = sl._parse_symbols(sym_path)  # the 4.5 MB map, parsed here and never printed
    old_own = sl.Ownership(symbols, _project.Splits.parse(old.stdout).by_section())

    # 1. symbols the batch's new ranges now cover (via their address), grouped by the owner the range names.
    newly: dict[str, str] = {}
    if added:
        for name, entries in symbols.items():
            if len(entries) != 1:
                continue
            section, address, _type = entries[0]
            for unit, asec, s, e in added:
                if asec == section and s <= address < e:
                    newly[name] = unit
                    break
    # A band header may declare a C++ callee by its clean spelling (`em_act_ck(...)`) while symbols.txt
    # carries the compiler mangling (`em_act_ck__FP11_ENEMY_WORKUcUc`). Match the mangled symbol's base
    # name too, so the rule-9-correct spelling is not a blind spot; the message names the full symbol.
    newly_bases: dict[str, list[str]] = {}
    for name in newly:
        # NOTE: this must not be called `base` - that is this function's own parameter (the batch's
        # base ref, used further down for `git show`), and the earlier shadowing made every
        # pre-existing band-header declaration look newly added (46 spurious warnings on one fold).
        symbol_stem = name.split("__", 1)[0]
        if symbol_stem != name and sl.RULE9_MANGLED_RE.match(name):
            newly_bases.setdefault(symbol_stem, []).append(name)

    warnings: list[str] = []
    if newly:
        for path in _band_header_paths(main):
            rel = os.path.relpath(path, main).replace(os.sep, "/")
            try:
                text = open(path, encoding="utf-8", errors="replace", newline="").read()
            except OSError:
                continue
            src = sl.Source(path, rel, text)
            for name, line in sl.header_declarations(src):
                owner = newly.get(name)
                if owner is not None:
                    warnings.append(
                        ("WARNING: %s:%d declares `%s`, which this batch's registration now makes owned "
                         "by `src/%s` - move the declaration into that unit's header and #include it, "
                         + RULE2_CALLSITE_CAVEAT +
                         " (docs/plan.md 6.5 rule 2); a mismatched signature is the `(10505) illegal "
                         "overloading` that only a full build would show") % (rel, line, name, owner))
                    continue
                for full in sorted(newly_bases.get(name, [])):
                    warnings.append(
                        ("WARNING: %s:%d declares `%s`, the C++ spelling of `%s`, which this batch's "
                         "registration now makes owned by `src/%s` - declare it in the owner's header "
                         "and #include it, " + RULE2_CALLSITE_CAVEAT +
                         " (docs/plan.md 6.5 rule 2)")
                        % (rel, line, name, full, newly[full]))

    # 2. a declaration this batch ADDS to a band header, of a symbol a unit already owned before the batch.
    for rel in changed_band:
        now_names = _declared_names(main, rel)
        before_names = _declared_names_at(main, base, rel)
        if not now_names:
            continue
        path = os.path.join(main, *rel.replace("/", os.sep).split(os.sep))
        try:
            text = open(path, encoding="utf-8", errors="replace", newline="").read()
        except OSError:
            continue
        src = sl.Source(path, rel, text)
        for name, line in sl.header_declarations(src):
            if name in before_names or name in newly:
                continue
            res = old_own.resolve(name)
            if res is None or res.get("kind") != "owned":
                continue
            warnings.append(
                ("WARNING: %s:%d newly declares `%s`, already owned by `src/%s` - the band is a "
                 "fallback, not the owner: declare it in the owner's header and #include it, "
                 + RULE2_CALLSITE_CAVEAT + " (docs/plan.md 6.5 rule 2)") % (rel, line, name, res["unit"]))

    return sorted(dict.fromkeys(warnings))


# --- the rows -------------------------------------------------------------------------------------------------

STYLE_LINT_REMEDY = (
    "rule 12 refuses an `extern` of data no registered `splits.txt` range covers: claim "
    "the range into the unit (the whole map symbol extent, `end:` 4-aligned), or "
    "register a named data-only unit when several units read the pool - "
    "`python tools/units/dataclaim.py --unit <unit>` prints the claim. A rule-12 "
    "addition a landing must take now, with the claim already scheduled, is accepted "
    "by `--allow-rule12 <token>` (recorded in the landing log). Rule 13 refuses a "
    "`<Type>_<name>(<Type>* self, ...)` free function (a member spelled the C way): "
    "declare `name` in the class, define `Type::name`, rename the map row to the "
    "mangling and sweep the call sites (`python tools/units/methodize.py <Type>` prints "
    "the plan), or mark a genuine C function `/* free: <retail C linkage evidenced|SDK "
    "C struct> */` on the declaration. Rule 15 refuses a stale path in a comment (a retired "
    "`include/`, `proposal/`, `auto/<hex>_`, `.pi/` or phase-4 path, or a retired tool's "
    "name: name the live path or drop it) and a function comment whose `0xADDR (0xSIZE)` "
    "prefix is not the function's (`symedit.py show <name>`); its advisory classes never refuse.")


def style_lint_row(b: Batch) -> None:
    """5. the style lint (section 6.5) adds no violation: `stylelint --diff <base> --json`, add-only, with rule 12's
    recorded allowance applied to its decision."""
    lint = os.path.join(b.main, "tools", "units", "stylelint.py")
    if not os.path.exists(lint):
        b.check("style lint (§6.5)", True, info="not built yet (roadmap 7.21) - skipped")
        return
    # `--json` carries the added (rule, file) counts and the added occurrences, so rule 12's own
    # allowance (`--allow-rule12 <token>`) can be applied to the lint's decision: a token the command
    # line sanctioned is excused and printed here; anything else the delta added still refuses.
    p = run([sys.executable, lint, "--diff", b.base or "HEAD", "--json"], b.main)
    lint_ok, lint_detail, lint_info, lint_excused = rule12_lint_row(p, state.ALLOW_RULE12)
    if lint_excused:
        print("rule 12: %d authorised by --allow-rule12 (recorded, not a file-level exemption): %s"
              % (len(lint_excused), "; ".join(lint_excused)))
    # the head of the output, never the tail: stylelint prints its findings first and its "not enforced"
    # legend last, so a tail hides the violation the batch has to fix (2026-09-25, `.pi/land.log`).
    b.check("style lint (§6.5) adds no violation", lint_ok, lint_detail, info=lint_info, remedy=STYLE_LINT_REMEDY)


def band_row(b: Batch) -> None:
    """7. the rule-2 registration boundary - a WARNING row, never a failure (`band_ownership_warnings`)."""
    b.band_warnings = band_ownership_warnings(b.main, b.base)
    for warning in b.band_warnings:
        print(warning, file=sys.stderr)
    b.check("rule 2 registration boundary (warning)", True,
            info=("%d newly-owned symbol declaration(s) still in the unsplit band (src/unsplit/*.h) - see the WARNING "
                  "lines above" % len(b.band_warnings)) if b.band_warnings
                 else "no newly-owned symbol left declared in the unsplit band (src/unsplit/*.h)")


def rule10_before(b: Batch) -> None:
    """Rule 10's BEFORE snapshot, taken before `configure.py` re-splits: text at the batch base, objects as built."""
    b.extra["rule10_before"] = rule10_violations(b.main, text_ref=b.base)


def rule10_row(b: Batch) -> None:
    """14. rule 10 (vtable ownership) adds no violation - add-only, `--allow-rule10` a recorded allowance."""
    before, after = b.extra.get("rule10_before"), rule10_violations(b.main)
    if before is None or after is None:
        b.check("rule 10 (vtable ownership) adds no violation", True,
                info="vtableaudit could not read this tree - the row is skipped (see the note above)")
        return
    grew, touched_rows = rule10_growth(before, after, b.unit_units)
    delta = vta.diff_rows(before, after)
    if delta["shifted"] or delta["removed"]:
        # the pairing is printed, never silent: a reader can check each SHIFTED pair is one table
        print("rule 10: %d shifted (added run overlapping a removed run of its section - the same table, not an "
              "addition): %s; %d removed: %s"
              % (len(delta["shifted"]), "; ".join("%s <- %s" % (a, r) for a, r in delta["shifted"]) or "-",
                 len(delta["removed"]), "; ".join(delta["removed"]) or "-"))
    if delta.get("reowned"):
        # a recut's re-owned run is credited by `diff_rows`, and the credit is printed, never silent
        print("rule 10: %d re-owned (a run the batch's splits.txt moved from another unit, credited): %s"
              % (len(delta["reowned"]), "; ".join("%s <- %s" % (k, was) for k, was in delta["reowned"])))
    # A sanctioned claim is accepted by an explicit, recorded allowance (`--allow-rule10 <key>`) - never by a key
    # in a file - and the acceptance is printed so the landing's own log carries the exception **and** the key it
    # excused. An allowance that matches nothing stays out of `accepted` and the row still refuses.
    authorised = set(state.ALLOW_RULE10)
    accepted = sorted(k for k in grew if k in authorised)
    grew = sorted(k for k in grew if k not in authorised)
    if accepted:
        print("rule 10: %d authorised by --allow-rule10 (recorded, not a file-level exemption): %s"
              % (len(accepted), "; ".join(accepted)))
    pairing = ("%d shifted, %d removed; " % (len(delta["shifted"]), len(delta["removed"]))
               if delta["shifted"] or delta["removed"] else "")
    b.check("rule 10 (vtable ownership) adds no violation", not grew,
            detail="%d added: %s" % (len(grew), "; ".join(grew[:4])),
            info=pairing + (("rule 10 report for this batch: %s" % "; ".join(touched_rows)) if touched_rows
                            else "no owned-but-unemitted code-pointer run or own-range vtable write in the "
                                 "batch's units"),
            remedy="declare the class with its `virtual` methods and let MWCC emit the table and the "
                   "store (rule 10 / playbook 52), or claim the `.data` range and emit it; run "
                   "`python tools/units/vtableaudit.py --unit <unit>` for the detail")
