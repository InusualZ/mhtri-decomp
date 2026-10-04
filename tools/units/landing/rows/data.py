"""The data-closure rows (`dataclosure`): no unclaimed data a batch unit references stays behind.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import tools.units.dataclosure as dg
from tools.units.landing import state
from tools.units.landing.common import Batch, KIND_BOOKKEEPING


def _git_unit_renames_since(main: str, base: str | None) -> dict:
    """git's `{old unit: new unit}` rename detection between the recorded base and the tree (empty when unknown)."""
    try:
        return dg.git_unit_renames(main, base) if base else {}
    except Exception:                                         # noqa: BLE001 - evidence only, never blocks the row
        return {}


def orphans_snapshot(main: str, units: list[str] | None) -> dict:
    """The data-closure base `record-base` stores (`dataclosure.snapshot_orphans`)."""
    return dg.snapshot_orphans(main, units)


# --- the rows -------------------------------------------------------------------------------------------------

ORPHAN_REMEDY = ("claim the data: add a `splits.txt` range for it to the unit that owns it (or a named "
                 "data-only unit for a pool several units share - `python tools/units/dataclaim.py "
                 "--unit <unit>` prints the exact text), or restore the claim a recut dropped. Run "
                 "`python tools/units/datagap.py --census --unit <unit>` for the orphan list with its "
                 "neighbours and readers. An unavoidable case takes `--allow-orphan <addr>`")
SOLE_OWNED_REMEDY = ("claim the unit's own data: `python tools/units/dataclaim.py --unit <unit>` prints the exact "
                     "`splits.txt` lines (link-order position, sections, start/end, partial-run note) for every "
                     "refusable run; apply them, force a re-split and re-measure (playbook 23). A pair you cannot "
                     "claim is named in the lane's report and the orchestrator passes `--allow-orphan <addr>`")


def data_closure_rows(b: Batch) -> None:
    """15. the data closure (owner, 2026-09-30 / strict 2026-09-29): no batch unit's TARGET object references data
    no claim covers (add-only), and a TOUCHED unit leaves no sole-owned data unclaimed; `--allow-orphan` is the
    recorded allowance and every deferred pair is printed with its class."""
    if not b.unit_units:
        return
    orphans = dg.batch_orphans(b.main, b.unit_units, b.recorded.get("orphans"), state.ALLOW_ORPHAN,
                               unit_map=state.UNIT_RENAMES,
                               git_renames=_git_unit_renames_since(b.main, b.recorded.get("base")))
    for line in orphans["unit_map_lines"]:
        print(line)
    if orphans["accepted"] or orphans["strict"]["accepted"]:
        print("data closure: %d authorised by --allow-orphan (recorded, not a file-level exemption): %s"
              % (len(orphans["accepted"]) + len(orphans["strict"]["accepted"]),
                 "; ".join((orphans["accepted"] + orphans["strict"]["accepted"])[:6])))
    if orphans["unmatched_allowances"]:
        print("data closure: --allow-orphan %s matched no refusal (it excuses nothing)"
              % ", ".join(orphans["unmatched_allowances"]))
    for unit, verdict_t in sorted(orphans["touch"].items()):
        print("data closure: %s" % dg.render_touch(unit, verdict_t))
    if orphans["added_deferred"]:
        print("data closure: %d NEW pair(s) deferred by class, not refused: %s"
              % (len(orphans["added_deferred"]), orphans["added_deferred"][0]))
    if orphans["claim_exposed"]:
        print("data closure: %d NEW pair(s) deferred (claim-exposed), not refused, no allowance earned, "
              "backlog `data-claim`: %s" % (len(orphans["claim_exposed"]), orphans["claim_exposed"][0]))
    for cls, lines in sorted(orphans["strict"]["deferred"].items()):
        print("data closure: deferred %s, %d pair(s), not refused: %s" % (cls, len(lines), lines[0]))
    if not orphans["have_base"]:
        b.check("the batch base carries a data-closure snapshot", False,
                "record-base did not snapshot the orphan set", kind=KIND_BOOKKEEPING,
                remedy="re-run `python tools/units/land.py record-base --units <batch units>` at the base "
                       "(it reads every registered unit's target object)")
    exposed = ("; deferred (claim-exposed): %d" % len(orphans["claim_exposed"])) if orphans["claim_exposed"] else ""
    b.check("no batch unit's target object references data no claim covers (unowned data stays behind)",
            not orphans["added"], "%d added: %s" % (len(orphans["added"]), "; ".join(orphans["added"][:4])),
            info=("pre-existing, reported: %d orphan reference(s) in the batch's units, e.g. %s"
                  % (len(orphans["pre_existing"]), "; ".join(orphans["pre_existing"][:2])) + exposed
                  if orphans["pre_existing"] else "no orphan data reference in the batch's units" + exposed),
            remedy=ORPHAN_REMEDY)
    counts = orphans["strict_counts"]
    b.check("no batch unit the batch really changes still has data only it references left unclaimed "
            "(touched = registered, recut or compiled object changed)",
            not orphans["sole_owned"],
            "%d pair(s) refused: %s" % (len(orphans["sole_owned"]), "; ".join(orphans["sole_owned"][:4])),
            info="strict data claim: refusable %d, deferred %s (every deferred pair is named with its class); "
                 "%d sole-owned pair(s) of %d untouched unit(s) reported, not demanded"
                 % (counts.get("refuse", 0), ", ".join("%s %d" % (c, counts.get(c, 0))
                                                        for c in dg.STRICT_CLASSES),
                    sum(orphans["untouched_pairs"].values()), len(orphans["untouched_pairs"])),
            remedy=SOLE_OWNED_REMEDY)
