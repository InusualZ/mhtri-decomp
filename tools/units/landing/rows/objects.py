"""The object rows (`verifyunit`, `undefrefs`, `flipcheck`): registration, references, drift, re-measure.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import sys

from tools.lib import names as _names
from tools.lib import proc
from tools.lib import project as _project
from tools.lib.lanes import naming
import tools.units.undefrefs as uref
import tools.units.verifyunit as vu
from tools.units.landing import state
from tools.units.landing.common import Batch, KIND_BOOKKEEPING, run
from tools.units.landing.rows.build import flipped_units
from tools.units.landing.state import rename_snapshot_keys


def flipcheck_problems(main: str, units: list[str]) -> list[str]:
    """`flipcheck.py` refusals for the units a batch flips: one line per unit with its reasons."""
    if not units:
        return []
    p = proc.run_chunked([sys.executable, os.path.join("tools", "units", "flipcheck.py")], units, cwd=main)
    problems, cur = [], None
    for line in (p.stdout or "").splitlines():
        if line.startswith("NOT READY"):
            cur = [line.split(None, 2)[2].strip()]
            problems.append(cur)
        elif line.startswith("   - ") and cur is not None:
            cur.append(line[5:].strip())
        elif line.startswith("READY") or not line.strip():
            cur = None
        elif line.endswith("no splits.txt entry"):
            problems.append([line.split(":")[0], "no splits.txt entry"])
    return ["%s: %s" % (g[0], "; ".join(g[1:]) or "not ready") for g in problems]


# --- the base snapshot's object half (record-base calls these) ---------------------------------------------------

def undefrefs_snapshot(main: str, units: list[str] | None) -> dict:
    """The base tree's own unresolved references (`undefrefs.snapshot_base`) - the add-only row's base."""
    return uref.snapshot_base(main, units)


# --- the rows -------------------------------------------------------------------------------------------------

def target_snapshot_before(b: Batch) -> None:
    """The split target objects as the gate finds them, before configure.py/ninja re-split the batch's ranges."""
    b.extra["targets_before"] = vu.target_object_snapshot(b.main)
    b.extra["targets_raw_before"] = vu.target_object_hashes(b.main)


def registration_row(b: Batch) -> None:
    """11. every batch unit is registered on all three axes: `Object(...)`, `splits.txt`, build graph (`verifyunit`)."""
    ok_reg, reg_detail = vu.registration_check(b.main, b.unit_units)
    b.check("every batch unit is registered (configure.py + splits.txt + build graph)", ok_reg,
            reg_detail,
            remedy="commit the unit's `Object(...)` line in configure.py and its splits.txt block, "
                   "then re-run configure.py so build.ninja carries build/RMHE08/src/<unit>.o - a "
                   "source file alone is registered in name only and never enters the build")


NEW_UNIT_ROW = "no newly registered unit has a generated name (rule 7)"


def registered_units(splits_text: str, configure_text: str) -> set[str]:
    """Every unit a `splits.txt` + `configure.py` pair registers, by claim key (`naming.norm_unit`): a block in the
    splits or an `Object(...)` row in configure.py - either is a registration."""
    units = {naming.norm_unit(u) for u in _project.Splits.parse(splits_text).units}
    units |= {naming.norm_unit(c.path) for c in _project.object_calls(configure_text)}
    return units


def new_unit_name_problems(base_units: set[str], now_units: set[str],
                           renames: "dict[str, list[str]] | None" = None) -> tuple[list[str], list[str]]:
    """`(refused, credited)` for the units `now_units` registers that `base_units` did not. Pure.

    A new unit whose path spells a generated component (`lib.names.generated_path_components`: a `fn_`/`lbl_`/...
    stem or a DOL address, in its stem or a directory) is refused - a GUESS name is allowed, the check is the
    pattern only. A declared rename (`--unit-rename OLD=NEW`, `renames` = `{OLD: [NEW, ...]}`) is credited when every
    generated component NEW spells is one OLD already spelled (a move that keeps a generated stem adds no name; a
    rename from a generated stem to a named one adds none either); it is never a licence for a new generated name.
    """
    refused, credited = [], []
    olds_of: dict[str, list[str]] = {}
    for old, news in (renames or {}).items():
        for new in news:
            olds_of.setdefault(naming.norm_unit(new), []).append(naming.norm_unit(old))
    for unit in sorted(now_units - base_units):
        gen = {c for c, _d in _names.generated_path_components(unit)}
        olds = olds_of.get(unit, [])
        if not gen:
            if any(_names.generated_path_components(o) for o in olds):
                credited.append("%s (renamed from %s)" % (unit, ", ".join(olds)))
            continue
        if any(gen <= {c for c, _d in _names.generated_path_components(o)} for o in olds):
            credited.append("%s (keeps %s from %s)" % (unit, ", ".join(sorted(gen)), ", ".join(olds)))
            continue
        refused.append("%s: generated name %s" % (unit, ", ".join("`%s`" % c for c in sorted(gen))))
    return refused, credited


def new_unit_name_row(b: Batch) -> None:
    """The registration family's name check (owner, 2026-10-05): a REFUSAL for a unit the batch newly registers under
    a generated stem or directory. Reported only when a new registration or a declared rename spells a generated
    component - a batch registering named units is the lint's to show (rule 7's file names), and the gate's table
    for it is unchanged. Cheap: two `git show`s and the working copies, before the build."""
    if not b.base:
        return
    shown = [run(["git", "show", "%s:%s" % (b.base, rel)], b.main) for rel in ("config/RMHE08/splits.txt",
                                                                            "configure.py")]
    if any(p.returncode != 0 for p in shown):
        return
    spl_base, conf_base = (p.stdout or "" for p in shown)
    try:
        with open(os.path.join(b.main, "config", "RMHE08", "splits.txt"), encoding="utf-8", errors="replace") as fh:
            spl_now = fh.read()
        with open(os.path.join(b.main, "configure.py"), encoding="utf-8", errors="replace") as fh:
            conf_now = fh.read()
    except OSError:
        return
    refused, credited = new_unit_name_problems(registered_units(spl_base, conf_base),
                                               registered_units(spl_now, conf_now), state.UNIT_RENAME_LISTS)
    if not refused and not credited:
        return
    b.check(NEW_UNIT_ROW, not refused, "; ".join(refused[:4]),
            info=("credited: %s" % "; ".join(credited[:4])) if credited else "",
            remedy="register the unit under a name for what it holds - a guess is allowed (say so in the unit's "
                   "header); a `fn_`/`lbl_`/`loc_`/`dtor_`/`zz_` + address stem or an address-named path is the "
                   "map's placeholder, never a unit name (docs/plan.md 6.5 rule 7). A unit renamed by "
                   "`--unit-rename OLD=NEW` may keep a generated component OLD already had")


def undefrefs_rows(b: Batch) -> None:
    """12-13. no batch unit ADDS a relocation to a name the link cannot provide (`undefrefs`, add-only against the
    base snapshot); every unit the batch flips to Matching is `flipcheck` READY; the base carried the snapshot."""
    result = uref.check_units(b.main, b.unit_units,
                              base_snapshot=rename_snapshot_keys(b.recorded.get("undefrefs") or {}),
                              base=b.base)
    b.check("every batch unit's relocations resolve against the link (no new undefined reference)",
            not result["problems"], "; ".join(result["problems"][:4]),
            info=("; ".join(result["pre_existing"][:3]) if result["pre_existing"]
                  else "no batch unit adds a name the link cannot provide"),
            remedy="our object ADDS a relocation to a name no `symbols.txt` row and no other link input "
                   "defines, so a flip would answer `undefined: '<name>'` even when the row reads 100 %. "
                   "The refusal names the target's own spelling where it records a different one - match "
                   "that spelling and its map row; `python tools/units/undefrefs.py <unit>` prints the "
                   "detail. A pre-existing wrong reference is reported, not refused (the `--census` "
                   "register is where it is worked down)")
    for line in result["pre_existing"]:
        print("note: %s" % line, file=sys.stderr)
    # the flip-readiness row (2026-09-29): a batch that turns a unit `Matching` links our object into main.dol,
    # so `flipcheck.py` must call it READY first - READY is necessary, not sufficient (the DOL hash row proves it).
    flips = flipped_units(b.main)
    if flips:
        fp = flipcheck_problems(b.main, [u for u in flips if u in b.unit_units] or flips)
        b.check("every unit the batch flips to Matching is flipcheck READY", not fp, "; ".join(fp[:3]),
                info="flipped: %s" % ", ".join(flips),
                remedy="`python tools/units/flipcheck.py <unit>` names the section that does not match the "
                       "claim; fix it, or keep the unit `NonMatching` (playbook 36/46/55/59/62)")
    if result["missing"]:
        b.check("the batch base carries an unresolved-reference snapshot for every unit", False,
                "record-base did not snapshot: %s" % ", ".join(result["missing"][:4]),
                kind=KIND_BOOKKEEPING,
                remedy="re-run `python tools/units/land.py record-base --units <batch units>` at the base "
                       "(it compiles the base objects and caches their unresolved references)")


def drift_row(b: Batch) -> None:
    """16. no unit's split target object moved under the batch (a neighbour re-ranged) - `verifyunit`'s
    rename-insensitive fingerprint, so a names-only change is not drift."""
    before, after = b.extra.get("targets_before") or {}, vu.target_object_snapshot(b.main)
    drift = vu.target_drift_problems(before, after, b.unit_units)
    # a unit whose bytes changed by names only (a map rename rewrites its symbol table) is not drift; it is named
    # separately, because the landing still belongs to it (pass it in --units so its rows judge it)
    names_only = vu.names_only_changes(before, after, b.extra.get("targets_raw_before") or {},
                                       vu.target_object_hashes(b.main), b.unit_units)
    if names_only:
        print("NOTE: %d unit(s) the batch does not name changed by names only (not drift; name them in --units): %s"
              % (len(names_only), ", ".join(names_only)), file=sys.stderr)
    b.check("no unit's split target object moved under the batch (a neighbour re-ranged)", not drift,
            "; ".join(drift[:4]),
            info=("names only, not drift (name them in --units): %d - %s"
                  % (len(names_only), ", ".join(names_only[:6]))) if names_only else "",
            remedy="the batch's splits.txt re-ranged a unit it does not name; include that unit in the "
                   "batch or fix its registration anchor, then re-run")


def remeasure_row(b: Batch) -> None:
    """17. the per-symbol re-measure reproduces the report from the objects (`verifyunit.verify_units`)."""
    ok_ind, ind_detail, ind_warn = vu.verify_units(b.main, b.units)
    b.check("per-symbol re-measure reproduces the report from the objects", ok_ind, ind_detail,
            info="; ".join(ind_warn[:3]) if ind_warn else "",
            remedy="a symbol's report score is not reproducible from the objects (a stale report.json, "
                   "a measuring-tool lie, or a 100% claim whose bytes differ) - rebuild and re-read, or "
                   "fix the symbol, before landing. A row dtk named itself (`pad_*`/`auto_*`, a range "
                   "with no function prologue) is resolved to our symbol at the same section+offset "
                   "and judged by its bytes, because objdiff cannot pair such a row by name at all - "
                   "so a dtk-generated row name is never the reason on its own")
