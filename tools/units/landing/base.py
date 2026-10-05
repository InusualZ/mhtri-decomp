"""`record-base`: the batch base snapshot (`.pi/land-base.json`) and the ledger/report readers it stores.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os
import sys
import time

from tools.lib import artifacts as _artifacts
from tools.lib import proc
from tools.lib import report as _report
from tools.lib.lanes import naming
from tools.units.landing import state
from tools.units.landing.common import BASE_FILE, changed_paths, command_detail, compile_targets, git, read_base, run
from tools.units.landing.rows.data import orphans_snapshot
from tools.units.landing.rows.objects import undefrefs_snapshot


def rebuild_report(main: str) -> dict | None:
    """`ninja build/RMHE08/report.json` before the base snapshot reads it -> `{returncode, seconds[, detail]}`, or
    None in a tree with no `build.ninja`.

    `report.json` is order-only: a build or a checkout after the last report leaves it describing an older tree, and
    a base snapshot taken from it judges the batch against the wrong scores (the network pilot's L4 landing: the
    gate's "before" was not main's tip). Rebuilding is a no-op when it is current (0.06 s measured) and a build of
    exactly what moved when it is not; a refusal on the report's mtime instead would refuse the landing after every
    landing that leaves an always-rebuilt object behind (MAIN at 2026-10-04 09:09: `Camellia/camellia.o` 77 s newer
    than `report.json` straight after a green landing)."""
    if not os.path.exists(os.path.join(main, "build.ninja")):
        return None
    t0 = time.time()
    # the registry's `report` refresh command (`lib.artifacts`), run unconditionally: ninja is the freshness judge
    p = run(_artifacts.get("report").command(_artifacts.Context(main)), main)
    out = {"returncode": p.returncode, "seconds": round(time.time() - t0, 1)}
    if p.returncode != 0:
        out["detail"] = command_detail(p)
    return out


def record_base(main: str, units: list[str] | None = None) -> dict:
    head = git(["rev-parse", "HEAD"], main).strip()
    dirty_before = changed_paths(main)
    report_build = rebuild_report(main)
    # HEAD == base here, so `changed_paths` is exactly "what was already dirty when the batch opened":
    # tracked edits and untracked files alike. `land_stageable` reads it back as the foreign-path guard.
    dirty = changed_paths(main) if report_build is not None else dirty_before
    data = {"base": head, "recorded_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "subject": git(["log", "-1", "--format=%s"], main).strip(),
            "ledger": ledger_numbers(main), "report": report_snapshot(main),
            "dirty_at_base": dirty}
    if report_build is not None:
        dirtied = sorted(set(dirty) - set(dirty_before))
        if dirtied:
            report_build["dirtied"] = dirtied      # the rebuild changed the tree: the base row refuses
        data["report_build"] = report_build
    # The add-only row's set: the base tree's own unresolved references, so the gate refuses only the names
    # a batch ADDS and reports the rest as debt. The batch's units are compiled first (a handful, seconds)
    # so the snapshot is the base's own objects, not a stale build; `units is None` snapshots every object
    # already present (the manual `record-base` flow, which does not name its units).
    norm = [naming.norm_unit(u.strip("/")) for u in (units or []) if u.strip()]
    # a unit the batch renames/folds away (declared with --unit-rename OLD=NEW) is registered at the BASE, so its
    # own objects and references must be in the base snapshot too or the merge onto NEW has nothing to merge
    if norm:
        norm = norm + [o for o in state.UNIT_RENAME_LISTS if o not in norm]
    if norm and os.path.exists(os.path.join(main, "build.ninja")):
        targets = compile_targets(norm)
        if targets:
            proc.run_chunked(["ninja"], targets, cwd=main)  # best effort: a failed compile leaves the object missing
    data["undefrefs"] = undefrefs_snapshot(main, norm or None)
    # The data-closure row's base: every (unit, orphan data address) pair the base's target objects carry and
    # the claimed bytes, so the gate refuses only a pair a batch ADDS or a claim it SHRINKS. Target objects
    # only (no compile), a few seconds; a tree with no split objects records nothing and the row says so. The
    # strict half's "touched" test also needs the base's per-unit claims and the batch units' compiled-object
    # fingerprints (name-insensitive, `datagap.object_fingerprint`) - the objects were compiled just above.
    try:
        if all(os.path.exists(os.path.join(main, "config", "RMHE08", f)) for f in ("splits.txt", "symbols.txt")):
            data["orphans"] = orphans_snapshot(main, norm or None)
    except Exception as exc:                                  # noqa: BLE001 - never block record-base on it
        print("record-base: data-closure snapshot failed (%s)" % exc, file=sys.stderr)
    os.makedirs(os.path.join(main, ".pi"), exist_ok=True)
    with open(os.path.join(main, BASE_FILE), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1)
    return data


def base_dirty_paths(main: str) -> set[str]:
    """The paths already dirty when the batch base was recorded - another stream's in-flight work.

    `record_base` snapshots them while HEAD == base, so a path in this set was dirty *before* the batch
    touched anything. A base recorded before this snapshot existed has none, and the guard is off for that
    batch (re-record the base to arm it). `--base` only asserts HEAD; the recorded snapshot still names the
    batch's foreign work.
    """
    return set(read_base(main).get("dirty_at_base") or [])


def report_snapshot(main: str) -> dict:
    """Per-unit measures and the sub-100 % symbols (`lib.report.snapshot`) - the evidence a batch's delta is
    judged against, taken at `record-base` and kept in `.pi/` because `ninja baseline` rewrites the baseline
    a later comparison would need."""
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    return _report.snapshot(_report.read(path))


def ledger_numbers(main: str) -> dict:
    """The ledger's totals, mapped to the names the message uses (`ledger.py --json` nests them)."""
    p = run([sys.executable, os.path.join("tools", "units", "ledger.py"), "--json"], main)
    if p.returncode != 0:
        return {}
    try:
        data = json.loads(p.stdout)
    except json.JSONDecodeError:
        return {}
    totals = data.get("totals") or data
    return {
        "covered": totals.get("claimed_functions"),
        "closed": totals.get("closed"),
        "partial": totals.get("partial"),
        "unclaimed": totals.get("unclaimed"),
        "matched": totals.get("matched_functions"),
        "bytes": totals.get("matched_code"),
        "total_code": totals.get("total_code"),
        "matched_percent": totals.get("fuzzy_match_percent"),
    }


def summary(before: dict, after: dict) -> str:
    def num(value):
        try:
            return int(value)
        except (TypeError, ValueError):
            return value if isinstance(value, (int, float)) else None

    keys = ("covered", "closed", "partial", "matched", "bytes")
    parts = []
    for key in keys:
        b, a = num(before.get(key)), num(after.get(key))
        if isinstance(b, (int, float)) and isinstance(a, (int, float)):
            parts.append("%s %s -> %s" % (key, b, a))
    return ", ".join(parts) or "(ledger numbers unavailable)"


def ledger_line(before: dict, after: dict) -> str:
    """`LEDGER: covered A -> B, closed ..., matched ..., bytes ...` - the delta alone, short enough to survive a cut.

    `partial` is left out (it is in the LANDED line's longer form); `(ledger numbers unavailable)` when the
    base recorded none.
    """
    keep = {k: v for k, v in after.items() if k in ("covered", "closed", "matched", "bytes")}
    return "LEDGER: " + summary({k: before.get(k) for k in keep}, keep)
