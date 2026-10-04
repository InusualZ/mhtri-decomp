"""The pre-written briefs: `briefs/pool/` (one per pool unit, stamped, idempotent, pruned) and the promoted briefs
in `briefs/` whose unit's range has moved since. Spec: docs/tools/spec/briefing.md. CLI: none (`tools/units/brief.py`)."""
from __future__ import annotations

import os
import re

from tools.lib.lanes import naming, registry
from tools.units.briefing import render, sources


def brief_unit(path: str) -> str | None:
    """The unit a written brief names in its title (`# Brief: <unit>`, also `# Cluster brief:`), normalised;
    None when the file is not a brief."""
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = re.match(r"# (?:\w+ )?[Bb]rief: (\S+)", line)
                if m:
                    return naming.norm_unit(m.group(1))
    except OSError:
        return None
    return None


def pool_dir(main: str) -> str:
    return os.path.join(main, "tools", "units", "briefs", "pool")


def promoted_dir(main: str) -> str:
    """`<main>/tools/units/briefs/` - where a claim's brief is handed to a worker."""
    return os.path.join(main, "tools", "units", "briefs")


def pool(main: str, force: bool = False, prune: bool = True, prune_promoted_litter: bool = False) -> dict:
    """Write a stamped brief for every pool unit (`sources.pool_units`: registered, not Matching) into
    `briefs/pool/`, rendered against the branch the default claim will make. An unchanged stamp is skipped, a
    changed one refreshed, `force` rewrites all; a brief whose unit left the pool is pruned. No claim is made."""
    units = sources.pool_units(main)
    outdir = pool_dir(main)
    os.makedirs(outdir, exist_ok=True)
    wrote, skipped, refreshed, pruned = [], [], [], []
    want = {naming.slug(u): u for u in units}
    for unit in units:
        path = os.path.join(outdir, naming.slug(unit) + ".md")
        if os.path.exists(path) and not force:
            if render.brief_stamp(path) == render.entry_stamp(main, unit):
                skipped.append(unit)
                continue
            refreshed.append(unit)
        _b, text = render.brief_for(main, naming.worktree_for(unit, main), unit, None, assume_claim=True, pool=True)
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
        wrote.append(unit)
    if prune:
        for name in sorted(os.listdir(outdir)):
            if not name.endswith(".md") or name[:-3] in want:
                continue
            path = os.path.join(outdir, name)
            pruned.append({"slug": name[:-3], "unit": brief_unit(path)})
            os.remove(path)
    litter = promoted_litter(main)
    pruned_promoted = prune_promoted(main, litter) if prune_promoted_litter else []
    return {"dir": outdir, "kind": "unit", "units": units, "wrote": wrote, "skipped": skipped,
            "refreshed": refreshed, "pruned": pruned, "litter": litter, "pruned_promoted": pruned_promoted}


def brief_text_range(path: str) -> list[int] | None:
    """The `.text` range a written brief states (its `| sections |` row), or None."""
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return None
    m = re.search(r"\| sections \| [^\n]*?\.text (0x[0-9A-Fa-f]+)-(0x[0-9A-Fa-f]+)", text)
    if not m:
        return None
    return [int(m.group(1), 16), int(m.group(2), 16)]


def promoted_litter(main: str) -> list[dict]:
    """Promoted briefs whose stated `.text` no longer matches: a registered unit whose split range moved, or a
    range no registered unit carries any more (a range a unit now covers is history, not litter). `claimed`
    says whether a live claim still owns the brief (reported, never deleted)."""
    d = promoted_dir(main)
    out: list[dict] = []
    if not os.path.isdir(d):
        return out
    registered = {naming.norm_unit(u) for u in sources.registered_units(main)}
    covered = sources.registered_text_ranges(main)
    branches = registry.worker_branches(main)
    branch_slugs = {naming.slug_of_branch(b) for b in branches}
    for name in sorted(os.listdir(d)):
        if not name.endswith(".md"):
            continue
        path = os.path.join(d, name)
        unit = brief_unit(path)
        stated = brief_text_range(path) if unit else None
        if stated is None:
            continue
        if naming.norm_unit(unit) in registered:
            rng = sources.splits_range(main, unit).get(".text")
            expected = [rng[0], rng[1]] if rng else None
            if expected is None or stated == expected:
                continue
            reason = "the unit's split range moved"
        else:
            if any(s <= stated[0] < e for s, e, _u in covered):
                continue
            expected, reason = None, "no registered unit carries this range"
        claimed = (bool(sources.claim_for(main, unit)) or registry.lock_held(main, unit, branches)
                   or name[:-3] in branch_slugs)
        out.append({"slug": name[:-3], "path": path, "unit": unit, "stated": stated, "expected": expected,
                    "reason": reason, "claimed": claimed})
    return out


def prune_promoted(main: str, litter: list[dict] | None = None) -> list[dict]:
    """Delete the promoted litter no live claim owns; a claimed one is reported, never removed."""
    litter = promoted_litter(main) if litter is None else litter
    removed = []
    for row in litter:
        if row.get("claimed"):
            continue
        try:
            os.remove(row["path"])
        except OSError:
            continue
        removed.append(row)
    return removed
