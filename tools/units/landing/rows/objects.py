"""The object rows (`verifyunit`, `undefrefs`, `flipcheck`): registration, references, drift, re-measure.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import sys

import tools.units.undefrefs as uref
import tools.units.verifyunit as vu
from tools.units.landing.common import Batch, KIND_BOOKKEEPING, run
from tools.units.landing.rows.build import flipped_units
from tools.units.landing.state import rename_snapshot_keys


def flipcheck_problems(main: str, units: list[str]) -> list[str]:
    """`flipcheck.py` refusals for the units a batch flips: one line per unit with its reasons."""
    if not units:
        return []
    p = run([sys.executable, os.path.join("tools", "units", "flipcheck.py"), *units], main)
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
