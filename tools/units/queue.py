"""Hand the next registered unit (or a header/module cluster of them) to a worker: claim it, render its brief,
print the spawn line. Spec: docs/tools/spec/queue.md. CLI: python tools/units/queue.py next [--count N | --cluster
NAME [--cross-module]] [--kind K] [--profile P] [--worker W] [--dry-run] [--json] | list [--json] | debt [...] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import datetime as _dt
import json
import os
import re
import subprocess

from tools.lib import cscan
from tools.lib.lanes import launch, naming, pool as slot_pool, registry
from tools.units import backlog
from tools.units import brief
from tools.units import claims

POOL_DEPTH = 5  # how many ready candidates `list` prints
#: Headers every unit shares: a shared base or fallback, never a reason to keep two lanes apart.
SHARED_HEADER_PREFIXES = ("include/dolphin/", "include/MSL", "include/unsplit/", "include/nw4r/", "include/types.h")


def pool_dir(main: str) -> str:
    return brief.pool_dir(main)


# --- the queue: registered units, minus the Matching ones ---------------------------------------------------------

def text_start(main: str, unit: str) -> int | None:
    """The unit's `.text` start from `splits.txt` (None for a data-only unit)."""
    rng = brief.splits_range(main, unit)
    return rng[".text"][0] if ".text" in rng else None


def order_key(main: str, unit: str) -> tuple:
    """The campaign's order: lowest `.text` address first, then unit name."""
    start = text_start(main, unit)
    return (start is None, start or 0, unit)


def entries(main: str) -> list[dict]:
    """Every registered unit as `{unit, flag, source}` (extensionless unit, source path from `src/`)."""
    out, seen = [], set()
    for c in brief.registered_objects(main):
        unit = naming.norm_unit(c.path)
        if unit in seen:
            continue
        seen.add(unit)
        out.append({"unit": unit, "flag": c.flag, "source": c.path})
    return out


def state(main: str, entry: dict, branches: set[str] | None = None, held: set[str] | None = None) -> str:
    """Where a registered unit stands: `matching` (done, never handed out), `nosource`, `claimed` (a registry row
    or a cluster claim holds it, or its claim branch exists - the lock), else `ready`."""
    unit = entry["unit"]
    if entry.get("flag") == "Matching":
        return "matching"
    if not os.path.exists(os.path.join(main, "src", *entry["source"].split("/"))):
        return "nosource"
    held = registry.claimed_units(registry.load(main)) if held is None else held
    if unit in held or registry.lock_held(main, unit, branches):
        return "claimed"
    return "ready"


def ready_entries(main: str, rows: list[dict] | None = None) -> list[dict]:
    """The ready units in address order."""
    rows = entries(main) if rows is None else rows
    branches = registry.worker_branches(main)
    held = registry.claimed_units(registry.load(main))
    ready = [e for e in rows if state(main, e, branches, held) == "ready"]
    return sorted(ready, key=lambda e: order_key(main, e["unit"]))


def next_entry(main: str) -> dict | None:
    ready = ready_entries(main)
    return ready[0] if ready else None


# --- header closures: what puts two units in one lane ----------------------------------------------------------

def _resolver(main: str):
    roots = [os.path.join(main, "include"), os.path.join(main, "src")]

    def resolve(name: str, includer: str) -> str | None:
        return cscan.resolve_include(name, [os.path.dirname(includer)] + roots)
    return resolve


def owner_headers(main: str, unit_source: str) -> set[str]:
    """The headers in the unit's include closure that belong to some module (repo-relative, forward slashes),
    minus the shared base and fallback headers (`SHARED_HEADER_PREFIXES`) every unit sees."""
    path = os.path.join(main, "src", *unit_source.split("/"))
    if not os.path.exists(path):
        return set()
    out = set()
    for f in cscan.include_closure(path, _resolver(main)):
        rel = os.path.relpath(f, main).replace("\\", "/")
        if rel == unit_source or rel == "src/" + unit_source or not rel.endswith((".h", ".hpp")):
            continue
        if rel.startswith(SHARED_HEADER_PREFIXES):
            continue
        out.add(rel)
    return out


def module_of(unit: str) -> str:
    return unit.split("/", 1)[0] if "/" in unit else unit


def header_module(header: str) -> str | None:
    """The module directory a header sits in (`include/<Module>/x.h` -> `<Module>`); None for a top-level header."""
    parts = header.replace("\\", "/").strip("/").split("/")
    if parts and parts[0] == "include":
        parts = parts[1:]
    return parts[0] if len(parts) > 1 else None


def cluster_members(main: str, name: str, rows: list[dict] | None = None,
                    cross_module: bool = False) -> tuple[list[dict], str]:
    """`(ready entries, reason)` for `--cluster NAME`: a header path - every ready unit **of the header's own module**
    whose closure contains it (`cross_module` drops the module bound: every unit that merely includes a hub header,
    whatever its module) - or a module (every ready unit under `src/<module>/`)."""
    ready = ready_entries(main, rows)
    norm = name.replace("\\", "/").strip("/")
    if norm.endswith((".h", ".hpp")):
        if not os.path.exists(os.path.join(main, *norm.split("/"))):
            raise SystemExit("REFUSED queue next --cluster: no header %s in %s" % (norm, main))
        hits = [e for e in ready if norm in owner_headers(main, e["source"])]
        mod = header_module(norm)
        if cross_module or mod is None:
            why = "every module" if cross_module else "a top-level header: no module bound"
            return hits, "units whose closure includes `%s` (%s)" % (norm, why)
        kept = [e for e in hits if module_of(e["unit"]).lower() == mod.lower()]
        dropped = len(hits) - len(kept)
        return kept, ("units of `src/%s/` whose closure includes `%s`%s" % (
            mod, norm, " (%d unit(s) of other modules that include it left out; --cross-module takes them)" % dropped
            if dropped else ""))
    return [e for e in ready if module_of(e["unit"]) == norm], "units under `src/%s/`" % norm


def disjoint_picks(main: str, ready: list[dict], count: int) -> list[dict]:
    """Up to `count` ready units, in address order, no two sharing an owner header or a module - so no two
    concurrent lanes edit one header (the address stride this replaces put header-sharing neighbours together)."""
    picks, used_headers, used_modules = [], set(), set()
    for e in ready:
        if len(picks) >= count:
            break
        hdrs = owner_headers(main, e["source"])
        mod = module_of(e["unit"])
        if mod in used_modules or hdrs & used_headers:
            continue
        picks.append(e)
        used_headers |= hdrs
        used_modules.add(mod)
    return picks


# --- the spawn and the brief ---------------------------------------------------------------------------------

def _profiles() -> list[str]:
    """`--profile`'s valid values: the profiles of the one kind table."""
    return list(launch.PROFILES)


def spawn_line(main: str, unit: str, slug: str, wt: str, brief_path: str,
               kind: str = "unit", profile: str | None = None) -> dict:
    """The paste-ready spawn (`launch.lane_call`): agent from the kind (or the override), the brief named by its
    MAIN path, cwd the claim's own worktree - a cwd that resolves to MAIN is refused outright."""
    profile = profile or launch.profile_for_kind(kind)
    if registry.same_path(wt, main):
        raise SystemExit("REFUSED spawn %s: cwd resolves to MAIN (%s) - a lane runs in its own worktree, "
                         "never the orchestrator's tree; take the claim first so the slot is the cwd." % (unit, wt))
    task = ("Read %s (in MAIN) and do exactly what it says. "
            "Ack first: python tools/units/claims.py ack %s --agent %s-%s. "
            "End your turn with your report: your final message is the result the orchestrator receives."
            % (brief_path.replace("\\", "/"), unit, profile, slug))
    name = "%s-%s" % (profile, slug)
    call = launch.lane_call(profile, wt, task, name=name, main=main, key=slug)
    return {"kind": kind, "agent": profile, "name": name, "cwd": wt, "task": task,
            "sessionId": call["session_id"], "call": call["call"]}


def briefs_dir(main: str) -> str:
    return os.path.join(main, "tools", "units", "briefs")


def promote(main: str, unit: str, claim_slug: str, wt: str | None = None) -> str:
    """Render `unit`'s brief at the claim's slug path against the claim's own worktree (never a copied pool file)."""
    dest = os.path.join(briefs_dir(main), claim_slug + ".md")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    _b, text = brief.brief_for(main, wt or naming.worktree_for(unit, main), unit, None)
    with open(dest, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    return dest


def promote_cluster(main: str, key: str, claim_slug: str, wt: str, units: list[str], reason: str) -> str:
    """Render one brief per cluster unit under `briefs/<claim slug>/` and the cluster index at `<claim slug>.md`."""
    sub = os.path.join(briefs_dir(main), claim_slug)
    os.makedirs(sub, exist_ok=True)
    paths = {}
    for unit in units:
        paths[unit] = os.path.join(sub, naming.slug(unit) + ".md")
        _b, text = brief.brief_for(main, wt, unit, None)
        with open(paths[unit], "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    handoff = brief.handoff_paths(main, key)
    dest = os.path.join(briefs_dir(main), claim_slug + ".md")
    with open(dest, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(brief.cluster_index(main, key, wt, units, paths, handoff, reason))
    return dest


# --- the guards --------------------------------------------------------------------------------------------------

def branch_error(main: str) -> str | None:
    """Refuse to claim from a MAIN whose HEAD is not `main` (None when git cannot be asked)."""
    p = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=main,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    branch = p.stdout.strip()
    if p.returncode != 0 or not branch:
        return None
    if branch != "main":
        return ("HEAD is on %r, not main: `git checkout main` first - a claim is rooted at MAIN's HEAD, "
                "so one made off main roots the worker on the wrong base" % branch)
    return None


def strictly_newer(main_lines, branch_lines):
    """Lines the branch has that main's copy lacks when main's are a subset of the branch's; None if diverged."""
    m, b = set(main_lines), set(branch_lines)
    if m - b:
        return None
    return len(b - m) or None


def _file_lines(main: str, ref: str, path: str) -> list[str]:
    p = subprocess.run(["git", "show", "%s:%s" % (ref, path)], cwd=main,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    return p.stdout.splitlines() if p.returncode == 0 else []


def unlanded_branches(main: str, ignore: set[str] | None = None) -> list[tuple[str, list[tuple[str, int]]]]:
    """Local branches whose content is strictly newer than main's, with the files and line counts."""
    ignore = set(ignore or ())
    p = subprocess.run(["git", "for-each-ref", "--format=%(refname:short)", "refs/heads"], cwd=main,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        return []
    out = []
    for branch in sorted(b.strip() for b in p.stdout.splitlines()):
        if not branch or branch == "main" or branch in ignore:
            continue
        d = subprocess.run(["git", "diff", "--name-only", "main...%s" % branch], cwd=main,
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if d.returncode != 0:
            continue
        hits = []
        for path in [ln for ln in d.stdout.splitlines() if ln.strip()]:
            n = strictly_newer(_file_lines(main, "main", path), _file_lines(main, branch, path))
            if n:
                hits.append((path, n))
        if hits:
            out.append((branch, hits))
    return out


def unlanded_error(main: str, allow: set[str] | None = None) -> str | None:
    """The refusal while any branch holds work main does not have; None when clear."""
    hits = unlanded_branches(main, allow)
    if not hits:
        return None
    lines = ["%d branch(es) hold work main does not have - land them (or delete them once the audit "
             "proves them stale) before claiming another unit:" % len(hits)]
    for branch, files in hits:
        shown = ", ".join("%s +%d lines" % (f, n) for f, n in files[:4])
        more = "" if len(files) <= 4 else " (+%d more file(s))" % (len(files) - 4)
        lines.append("  %s: %s%s" % (branch, shown, more))
    lines.append("  remedy: land it (docs/plan.md 12's recipe), or `git worktree remove <wt> && git branch -D "
                 "<branch>` once its delta is proven stale; `--allow-unlanded <branch>` parks one on purpose")
    return "\n".join(lines)


def no_ready(main: str) -> str:
    return ("no ready unit: every registered unit is Matching or claimed\n"
            "  see: python tools/units/queue.py list")


def slot_cap(main: str, slots_mode: bool | None = None) -> str | None:
    """The slot-pool refusal when every slot is taken (None with `--no-slots` or no pool)."""
    if slots_mode is False:
        return None
    return slot_pool.capacity_error(main)


def _guards(main: str, what: str, dry_run: bool, slots_mode, ignore_backlog: bool, ratio: int, allow_unlanded) -> None:
    """The refusals every claim path runs first: MAIN off `main`, the slot cap, the credit gate, unlanded work."""
    if not dry_run:
        bad_branch = branch_error(main)
        if bad_branch:
            raise SystemExit("REFUSED queue %s | %s" % (what, bad_branch))
    cap = slot_cap(main, slots_mode)
    if cap:
        raise SystemExit("REFUSED queue %s | %s" % (what, cap))
    if not ignore_backlog:
        msg = backlog.refusal(main, ratio=ratio)
        if msg:
            raise SystemExit("REFUSED queue %s | %s" % (what, msg))
    if not dry_run:
        blocked = unlanded_error(main, set(allow_unlanded or ()))
        if blocked:
            raise SystemExit("REFUSED queue %s | %s" % (what, blocked))


def _claim_record(out: dict, worker: str | None, ratio: int) -> dict:
    """One ledger entry for a handed-out claim."""
    return {"unit": out.get("unit"), "worker": worker or "", "branch": (out.get("claim") or {}).get("branch"),
            "worktree": out.get("worktree"), "ratio": ratio,
            "when": _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat()}


# --- the claim paths ---------------------------------------------------------------------------------------------

def claim_entry(main: str, entry: dict, worker: str | None, dry_run: bool, claim_fn,
                kind: str = "unit", profile: str | None = None, slots_mode: bool | None = None,
                units: list[str] | None = None, reason: str = "") -> dict:
    """Claim one selected unit (or a cluster: `units` set), render its brief at the claim's slug path, and return
    its spawn. The claim comes first, so a worker is never handed an outbox path that does not exist."""
    unit = entry["unit"]
    slug = naming.slug(unit)
    use_slots = slots_mode is not False and slot_pool.enabled(main)
    if dry_run:
        if use_slots:
            pv = slot_pool.preview(main, unit, naming.branch_for(unit))
            wt = pv["dir"]
            info = {"unit": unit, "branch": pv["branch"], "worktree": wt, "slot": pv["slot"], "dry_run": True}
        else:
            wt = naming.worktree_for(unit, main)
            info = {"unit": unit, "branch": naming.branch_for(unit), "worktree": wt, "dry_run": True}
        claim_slug = slug
        brief_path = os.path.join(briefs_dir(main), claim_slug + ".md")
    else:
        info = claim_fn(unit, main, worker, False, kind=kind, **({"units": units} if units else {}))
        wt = info.get("worktree") or naming.worktree_for(unit, main)
        claim_slug = registry.claim_slug(main, unit) or slug
        if use_slots and info.get("slot") is not None:
            expected = slot_pool.slot_dir(main, info["slot"])
            if not registry.same_path(wt, expected):
                raise SystemExit("REFUSED spawn %s: claim slot %s has worktree %s, not the slot dir %s - a "
                                 "slot claim must hand out the slot's cwd" % (unit, info["slot"], wt, expected))
        brief_path = (promote_cluster(main, unit, claim_slug, wt, units, reason) if units
                      else promote(main, unit, claim_slug, wt))
    out = {"unit": unit, "slug": slug, "claim_slug": claim_slug, "worktree": wt, "brief": brief_path,
           "claim": info, "dry_run": dry_run, "spawn": spawn_line(main, unit, claim_slug, wt, brief_path, kind, profile)}
    if units:
        out["units"] = list(units)
    return out


def next_brief(main: str, worker: str | None, dry_run: bool, claim_fn=None, kind: str = "unit",
               profile: str | None = None, allow_unlanded=None, ignore_backlog: bool = False,
               ratio: int = backlog.RATIO_DEFAULT, slots_mode: bool | None = None) -> dict:
    """Claim the next ready unit (lowest `.text` address) behind the guards; a real claim is recorded."""
    claim_fn = claim_fn or claims.claim
    _guards(main, "next", dry_run, slots_mode, ignore_backlog, ratio, allow_unlanded)
    entry = next_entry(main)
    if entry is None:
        raise SystemExit(no_ready(main))
    out = claim_entry(main, entry, worker, dry_run, claim_fn, kind, profile, slots_mode)
    if not dry_run and not ignore_backlog:
        backlog.record_claims(main, [_claim_record(out, worker, ratio)], ratio=ratio)
    return out


def next_briefs(main: str, worker: str | None, dry_run: bool, count: int, claim_fn=None, kind: str = "unit",
                profile: str | None = None, allow_unlanded=None, ignore_backlog: bool = False,
                ratio: int = backlog.RATIO_DEFAULT, slots_mode: bool | None = None) -> dict:
    """Claim up to `count` lanes at once, no two sharing an owner header or a module (`disjoint_picks`), capped
    by the free slots and re-checked against the credit balance; `shortfall` says how many could not be filled."""
    if count < 1:
        raise SystemExit("REFUSED queue next | --count must be at least 1")
    claim_fn = claim_fn or claims.claim
    requested = count
    _guards(main, "next", dry_run, slots_mode, ignore_backlog, ratio, allow_unlanded)
    if slots_mode is not False and slot_pool.enabled(main):
        count = min(count, slot_pool.free_count(main))
    chosen = disjoint_picks(main, ready_entries(main), count)
    if not chosen:
        raise SystemExit(no_ready(main))
    if not ignore_backlog:
        msg = backlog.refusal(main, ratio=ratio, wants=len(chosen))
        if msg:
            raise SystemExit("REFUSED queue next | %s" % msg)
    out = [claim_entry(main, e, worker, dry_run, claim_fn, kind, profile, slots_mode) for e in chosen]
    if not dry_run and not ignore_backlog:
        backlog.record_claims(main, [_claim_record(c, worker, ratio) for c in out], ratio=ratio)
    return {"requested": requested, "claimed": len(out), "shortfall": requested - len(out), "dry_run": dry_run,
            "claims": out}


def cluster_key(name: str) -> str:
    """The claim key a cluster is held under: `cluster/<module>` or `cluster/<header path without .h>`."""
    norm = name.replace("\\", "/").strip("/")
    return "cluster/" + re.sub(r"\.(h|hpp)$", "", norm)


def next_cluster(main: str, name: str, worker: str | None, dry_run: bool, claim_fn=None, kind: str = "unit",
                 profile: str | None = None, allow_unlanded=None, ignore_backlog: bool = False,
                 ratio: int = backlog.RATIO_DEFAULT, slots_mode: bool | None = None,
                 cross_module: bool = False) -> dict:
    """Claim ONE lane for every ready unit of a cluster - the units of a header's module sharing that owner header
    (`cross_module`: every module's), or a module - so no two concurrent lanes ever hold header-sharing units. The
    claim records the units; one brief indexes them."""
    claim_fn = claim_fn or claims.claim
    _guards(main, "next", dry_run, slots_mode, ignore_backlog, ratio, allow_unlanded)
    members, reason = cluster_members(main, name, cross_module=cross_module)
    if not members:
        raise SystemExit("REFUSED queue next --cluster %s: no ready unit in it (%s)" % (name, reason))
    units = [e["unit"] for e in members]
    key = cluster_key(name)
    out = claim_entry(main, {"unit": key}, worker, dry_run, claim_fn, kind, profile, slots_mode,
                      units=units, reason=reason)
    out["reason"] = reason
    if not dry_run and not ignore_backlog:
        backlog.record_claims(main, [_claim_record(out, worker, ratio)], ratio=ratio)
    return out


# --- debt: a naming/band-header backlog item, claimed like a unit -------------------------------------------------

def debt_candidates(main: str, items=None, **kw) -> list:
    """The claimable debt items in rank order: open `naming`/`band-header` items whose file is free."""
    pool = backlog.debt_items(items) if items is not None else backlog.open_debt_items(main, **kw)
    branches = registry.worker_branches(main)
    out = []
    for it in pool:
        unit = backlog.debt_unit(it)
        if brief.claim_for(main, unit) or registry.lock_held(main, unit, branches):
            continue
        out.append(it)
    return out


def no_debt(main: str) -> str:
    return ("no claimable debt: no open naming/band-header item has names outstanding and an unclaimed "
            "file\n  run `python tools/units/backlog.py --print` to see the register")


def write_debt_brief(main: str, claim_slug: str, text: str) -> str:
    dest = os.path.join(briefs_dir(main), claim_slug + ".md")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with open(dest, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    return dest


def next_debt_brief(main: str, worker: str | None, dry_run: bool, claim_fn=None,
                    ratio: int = backlog.RATIO_DEFAULT, ignore_backlog: bool = False,
                    slots_mode: bool | None = None, **kw) -> dict:
    """Claim the top open naming/band-header debt item (held on its file) and hand a lane its name list."""
    claim_fn = claim_fn or claims.claim
    if not dry_run:
        bad_branch = branch_error(main)
        if bad_branch:
            raise SystemExit("REFUSED queue debt | %s" % bad_branch)
    cap = slot_cap(main, slots_mode)
    if cap:
        raise SystemExit("REFUSED queue debt | %s" % cap)
    items, meta = backlog.build(main, **kw)
    if not ignore_backlog:
        open_ = [i for i in items if i.status == "open"]
        if open_:
            summary = backlog.ledger_summary(items, meta["ledger"]["claims"], ratio,
                                             free=int(meta["ledger"].get("free") or 0))
            if summary["balance"] < ratio:
                raise SystemExit("REFUSED queue debt | %s" % backlog.refusal(main, ratio=ratio, **kw))
    if not dry_run:
        blocked = unlanded_error(main, set())
        if blocked:
            raise SystemExit("REFUSED queue debt | %s" % blocked)
    candidates = debt_candidates(main, items=items, **kw)
    if not candidates:
        raise SystemExit(no_debt(main))
    item = candidates[0]
    unit = backlog.debt_unit(item)
    if dry_run:
        info = {"unit": unit, "branch": naming.branch_for(unit), "worktree": naming.worktree_for(unit, main),
                "dry_run": True}
    else:
        info = claim_fn(unit, main, worker, False, kind="fix")
    wt = info.get("worktree") or naming.worktree_for(unit, main)
    claim_slug = registry.claim_slug(main, unit) or naming.slug(unit)
    brief_path = os.path.join(briefs_dir(main), claim_slug + ".md")
    if not dry_run:
        brief_path = write_debt_brief(main, claim_slug, backlog.debt_brief(main, item))
    spawn = backlog.debt_task(main, item, cwd=wt, brief=brief_path)
    out = {"item": item.key, "kind": item.kind, "target": item.target, "names": list(item.names),
           "weight": item.weight, "count": item.count, "unit": unit, "claim": info,
           "worktree": wt, "brief": brief_path, "dry_run": dry_run, "spawn": spawn}
    if not dry_run and not ignore_backlog:
        backlog.record_claims(main, [{"unit": unit, "worker": worker or "", "branch": info.get("branch"),
                                      "worktree": wt, "ratio": ratio, "kind": "debt", "backlog": item.key,
                                      "when": _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat()}],
                              ratio=ratio)
    return out


def pool_state(main: str) -> dict:
    """The registered units' counts by state, plus the ready ones in address order."""
    rows = entries(main)
    branches = registry.worker_branches(main)
    held = registry.claimed_units(registry.load(main))
    counts: dict[str, int] = {}
    ready = []
    for e in rows:
        st = state(main, e, branches, held)
        counts[st] = counts.get(st, 0) + 1
        if st == "ready":
            ready.append(e)
    ready.sort(key=lambda e: order_key(main, e["unit"]))
    return {"dir": pool_dir(main), "entries": rows, "counts": counts, "ready": ready}


# --- selftest ---------------------------------------------------------------------------------------------------

def selftest() -> int:
    import contextlib
    import io
    from tools.lib import testing
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def _raises(fn) -> bool:
        try:
            fn()
            return False
        except SystemExit:
            return True

    def write(root, rel, text):
        p = os.path.join(root, *rel.split("/"))
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)

    def fake_claim(unit, main, worker, dry_run, **kw):
        reg = registry.load(main)
        reg[unit] = {"branch": naming.branch_for(unit), "worktree": naming.worktree_for(unit, main),
                     "kind": kw.get("kind"), **({"units": kw["units"]} if kw.get("units") else {})}
        registry.save(main, reg)
        return {"unit": unit, "branch": naming.branch_for(unit), "worktree": naming.worktree_for(unit, main),
                "kind": kw.get("kind")}

    check("strictly_newer: identical / gained / new file / diverged / main newer / empty branch",
          [strictly_newer(["a", "b"], ["a", "b"]), strictly_newer(["a"], ["a", "b", "c"]), strictly_newer([], ["x", "y"]),
           strictly_newer(["a", "new"], ["a", "old"]), strictly_newer(["a", "b"], ["a"]), strictly_newer(["a"], [])],
          [None, 2, 2, None, None, None])
    with testing.temp_dir() as tmp:
        check("unlanded_branches / unlanded_error outside a repo", (unlanded_branches(tmp), unlanded_error(tmp)), ([], None))

    # the queue is the registered units minus the Matching ones (2026-10-04: NOT the body-less ones only)
    with testing.temp_dir() as tmp:
        write(tmp, "src/auto/stubA.c", "/* header only */\n")
        write(tmp, "src/auto/stubB.c", "/* header only */\n")
        write(tmp, "src/auto/done.c", "int f(void) { return 1; }\n")
        write(tmp, "src/auto/ok.c", "int g(void) { return 2; }\n")
        write(tmp, "configure.py",
              'config.libs = [\n    {\n        "lib": "auto",\n        "objects": [\n'
              '            Object(NonMatching, "auto/stubA.c"),\n            Object(NonMatching, "auto/stubB.c"),\n'
              '            Object(NonMatching, "auto/done.c"),\n            Object(Matching, "auto/ok.c"),\n'
              '            Object(NonMatching, "auto/missing.c"),\n        ],\n    },\n]\n')
        write(tmp, "config/RMHE08/splits.txt",
              "auto/stubA.c:\n\t.text       start:0x80200000 end:0x80200100\n\n"
              "auto/stubB.c:\n\t.text       start:0x80100000 end:0x80100100\n\n"
              "auto/done.c:\n\t.text       start:0x80300000 end:0x80300100\n\n"
              "auto/ok.c:\n\t.text       start:0x80400000 end:0x80400100\n")
        write(tmp, "config/RMHE08/symbols.txt",
              "fn_80100000 = .text:0x80100000; // type:function size:0x40\n"
              "fn_80200000 = .text:0x80200000; // type:function size:0x40\n")
        st = pool_state(tmp)
        check("every registered unit has a state", st["counts"], {"ready": 3, "matching": 1, "nosource": 1})
        check("a NonMatching unit with a body is ready (it used to be hidden as `written`)",
              [e["unit"] for e in st["ready"]], ["auto/stubB", "auto/stubA", "auto/done"])
        check("the next ready unit is the lowest .text address", next_entry(tmp)["unit"], "auto/stubB")
        check("text_start reads the split range", text_start(tmp, "auto/stubA"), 0x80200000)
        registry.save(tmp, {"auto/stubB": {"branch": naming.branch_for("auto/stubB")}})
        check("a registry claim takes the unit out of the ready set",
              (next_entry(tmp)["unit"], pool_state(tmp)["counts"].get("claimed")), ("auto/stubA", 1))
        registry.save(tmp, {"cluster/auto": {"branch": "worker/cluster-auto-1", "units": ["auto/stubA", "auto/done"]}})
        check("a unit a cluster claim lists is claimed too", [e["unit"] for e in ready_entries(tmp)], ["auto/stubB"])
        registry.save(tmp, {})
        os.makedirs(naming.worktree_for("auto/stubB", tmp))
        check("a leftover claim worktree is a held lock", next_entry(tmp)["unit"], "auto/stubA")
        os.rmdir(naming.worktree_for("auto/stubB", tmp))

        dry = next_brief(tmp, "w-demo", dry_run=True)
        check("dry-run claims nothing", registry.load(tmp), {})
        check("dry-run picks the lowest address, the surveyor profile and the worktree as cwd",
              (dry["unit"], dry["spawn"]["agent"], dry["spawn"]["name"], dry["spawn"]["cwd"], dry["spawn"]["kind"]),
              ("auto/stubB", "surveyor", "surveyor-" + naming.slug("auto/stubB"), naming.worktree_for("auto/stubB", tmp),
               "unit"))
        check("dry-run's task names the brief and the unit, as a headless claude call",
              (dry["brief"].replace("\\", "/") in dry["spawn"]["task"], "auto/stubB" in dry["spawn"]["task"],
               " claude --agent surveyor " in dry["spawn"]["call"]), (True, True, True))
        check("the kind decides the profile: fix -> fixer, tooling -> worker, review -> codereviewer",
              [spawn_line(tmp, "auto/x", "s", "/w", "/b", k)["agent"] for k in ("fix", "tooling", "review")],
              ["fixer", "worker", "codereviewer"])
        check("... an unknown kind is refused", _raises(lambda: spawn_line(tmp, "auto/x", "s", "/w", "/b", "nope")), True)
        check("... an explicit profile overrides the mapping",
              spawn_line(tmp, "auto/x", "s", "/w", "/b", "unit", "fixer")["agent"], "fixer")
        check("--profile's valid values are the kind table's profiles", _profiles(),
              sorted(set(launch.KIND_PROFILE.values())))
        saved_argv, err = sys.argv, io.StringIO()
        try:
            sys.argv = ["queue.py", "next", "--profile", "decompilerr", "--dry-run"]
            with contextlib.redirect_stderr(err):
                try:
                    main()
                    check("an unknown `--profile` is refused", "no error", "SystemExit")
                except SystemExit as exc:
                    check("an unknown `--profile` is refused", exc.code, 2)
        finally:
            sys.argv = saved_argv
        check("... by argparse, listing the real profiles", "choose from" in err.getvalue(), True)
        check("the spawn asks for a final-message handoff", "final message" in dry["spawn"]["task"], True)
        check("spawn_line refuses a cwd that is MAIN", _raises(lambda: spawn_line(tmp, "auto/x", "s", tmp, "/b")), True)

        real = next_brief(tmp, "w-demo", dry_run=False, claim_fn=fake_claim, ignore_backlog=True)
        check("the real flow claims the unit", "auto/stubB" in registry.load(tmp), True)
        check("the brief is at the claim's slug path", os.path.basename(real["brief"]),
              registry.claim_slug(tmp, "auto/stubB") + ".md")
        text = open(real["brief"], encoding="utf-8").read()
        check("... rendered for the unit's current range, with the claim's outbox",
              ("0x80100000" in text, "outbox" in text, "Pooled brief" in text), (True, True, False))
        registry.save(tmp, {"auto/stubA": {"branch": naming.branch_for("auto/stubA") + "-zz"}})
        suffixed = promote(tmp, "auto/stubA", naming.slug("auto/stubA") + "-zz")
        check("a suffixed claim's brief names its own outbox", "-zz.json" in open(suffixed, encoding="utf-8").read(), True)
        registry.save(tmp, {})

    # clusters: owner headers decide which units share a lane; a wave never splits them across lanes
    with testing.temp_dir() as tmp:
        write(tmp, "include/types.h", "typedef int s32;\n")
        write(tmp, "include/net/net.h", '#include "types.h"\nstruct Net { s32 x; };\n')
        write(tmp, "include/net/inner.h", "struct Inner;\n")
        write(tmp, "include/g/g.h", '#include "types.h"\n')
        write(tmp, "src/net/a.cpp", '#include "net/net.h"\n#include "net/inner.h"\nvoid a() {}\n')
        write(tmp, "src/net/b.cpp", '#include "net/net.h"\nvoid b() {}\n')
        write(tmp, "src/g/c.cpp", '#include "g/g.h"\nvoid c() {}\n')
        write(tmp, "src/h/d.cpp", '#include "net/inner.h"\nvoid d() {}\n')
        write(tmp, "configure.py", 'config.libs = [{"lib": "x", "objects": [Object(NonMatching, "net/a.cpp"), '
                                   'Object(NonMatching, "net/b.cpp"), Object(NonMatching, "g/c.cpp"), '
                                   'Object(NonMatching, "h/d.cpp")]}]\n')
        write(tmp, "config/RMHE08/splits.txt",
              "net/a.cpp:\n\t.text       start:0x80100000 end:0x80100100\n\n"
              "net/b.cpp:\n\t.text       start:0x80100100 end:0x80100200\n\n"
              "g/c.cpp:\n\t.text       start:0x80200000 end:0x80200100\n\n"
              "h/d.cpp:\n\t.text       start:0x80300000 end:0x80300100\n")
        write(tmp, "config/RMHE08/symbols.txt", "fn_80100000 = .text:0x80100000; // type:function size:0x40\n")
        check("owner_headers is the closure's module headers, minus the shared base",
              sorted(owner_headers(tmp, "net/a.cpp")), ["include/net/inner.h", "include/net/net.h"])
        check("a header cluster is every ready unit OF THE HEADER'S MODULE whose closure includes it",
              ([e["unit"] for e in cluster_members(tmp, "include/net/net.h")[0]],
               [e["unit"] for e in cluster_members(tmp, "include/net/inner.h")[0]]),
              (["net/a", "net/b"], ["net/a"]))
        check("... the reason names the units of other modules it left out",
              "1 unit(s) of other modules" in cluster_members(tmp, "include/net/inner.h")[1], True)
        check("--cross-module takes back a unit of another module that merely includes the header",
              [e["unit"] for e in cluster_members(tmp, "include/net/inner.h", cross_module=True)[0]],
              ["net/a", "h/d"])
        check("header_module: include/<Module>/x.h -> Module, a top-level header has none",
              (header_module("include/Network/network_transport.h"), header_module("include/types.h")),
              ("Network", None))
        check("a module cluster is every ready unit under src/<module>/",
              [e["unit"] for e in cluster_members(tmp, "net")[0]], ["net/a", "net/b"])
        check("a missing header is refused", _raises(lambda: cluster_members(tmp, "include/nope.h")), True)
        check("a wave never puts header- or module-sharing units in two lanes",
              [e["unit"] for e in disjoint_picks(tmp, ready_entries(tmp), 4)], ["net/a", "g/c"])
        check("... and a unit sharing nothing joins it", [e["unit"] for e in disjoint_picks(
            tmp, [e for e in ready_entries(tmp) if e["unit"] != "net/a"], 4)], ["net/b", "g/c", "h/d"])
        short = next_briefs(tmp, "w", dry_run=False, count=3, claim_fn=fake_claim, ignore_backlog=True)
        check("a wave reports its shortfall instead of breaking the rule",
              ((short["requested"], short["claimed"], short["shortfall"]), [c["unit"] for c in short["claims"]]),
              ((3, 2, 1), ["net/a", "g/c"]))
        check("--count below 1 is refused", _raises(lambda: next_briefs(tmp, None, dry_run=True, count=0)), True)
        registry.save(tmp, {})
        out = next_cluster(tmp, "include/net/net.h", "w", dry_run=False, claim_fn=fake_claim, ignore_backlog=True)
        check("a cluster is ONE claim holding every member", (out["unit"], out["units"]),
              ("cluster/include/net/net", ["net/a", "net/b"]))
        check("... recorded in the registry, so every member reads claimed",
              (registry.load(tmp)["cluster/include/net/net"]["units"], [e["unit"] for e in ready_entries(tmp)]),
              (["net/a", "net/b"], ["g/c", "h/d"]))
        index = open(out["brief"], encoding="utf-8").read()
        check("the cluster brief indexes the per-unit briefs and acks with the cluster key",
              ("# Cluster brief: cluster/include/net/net" in index, "`net/a`" in index and "`net/b`" in index,
               "claims.py ack cluster/include/net/net" in index, "## 0 · Your tree" in index),
              (True, True, True, True))
        member = os.path.join(briefs_dir(tmp), out["claim_slug"], naming.slug("net/a") + ".md")
        check("... each member's brief names the cluster's outbox",
              (os.path.exists(member), out["claim_slug"] + ".json" in open(member, encoding="utf-8").read()), (True, True))
        check("... and the spawn hands out that index", out["brief"].replace("\\", "/") in out["spawn"]["task"], True)
        check("a cluster with no ready member is refused",
              _raises(lambda: next_cluster(tmp, "net", None, dry_run=True, ignore_backlog=True)), True)

    with testing.temp_dir() as empty:
        write(empty, "src/a/x.c", "int x;\n")
        write(empty, "configure.py", 'config.libs = [{"lib": "a", "objects": [Object(Matching, "a/x.c")]}]\n')
        check("an all-Matching tree has nothing to hand out", (pool_state(empty)["ready"], next_entry(empty)), ([], None))
        check("... next refuses", _raises(lambda: next_brief(empty, None, dry_run=True)), True)
        check("... and so does a wave", _raises(lambda: next_briefs(empty, None, dry_run=True, count=3)), True)

    def qgit(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    with testing.temp_dir() as tmp:
        repo = os.path.join(tmp, "mhtri-dtk")
        write(repo, "src/u/lo.c", "/* */\n")
        write(repo, "src/u/hi.c", "/* */\n")
        write(repo, "configure.py", 'config.libs = [{"lib": "u", "objects": [Object(NonMatching, "u/lo.c"), '
                                    'Object(NonMatching, "u/hi.c")]}]\n')
        write(repo, "config/RMHE08/splits.txt", "u/lo.c:\n\t.text       start:0x80100000 end:0x80100100\n\n"
                                                "u/hi.c:\n\t.text       start:0x80200000 end:0x80200100\n")
        qgit(repo, "init", "-q")
        qgit(repo, "checkout", "-q", "-b", "main")
        qgit(repo, "add", "-A")
        qgit(repo, "commit", "-q", "-m", "claim-time main")
        check("a claim on main passes the branch guard", branch_error(repo), None)
        qgit(repo, "branch", naming.branch_for("u/lo"))
        check("a claim branch with no registry row is a held lock", next_entry(repo)["unit"], "u/hi")
        qgit(repo, "branch", "-D", naming.branch_for("u/lo"))
        check("... and a released one is ready again", next_entry(repo)["unit"], "u/lo")
        qgit(repo, "checkout", "-q", "-b", "throwaway-check")
        err = branch_error(repo)
        check("a claim off main is refused, naming the branch", "throwaway-check" in (err or ""), True)
        check("queue next refuses off main and claims nothing",
              (_raises(lambda: next_brief(repo, None, dry_run=False)), os.path.exists(registry.registry_path(repo))),
              (True, False))

    # the credit ledger (owner, 2026-09-27): a `done` earns 1, a claim spends `ratio`; --ignore-backlog spends none
    with testing.temp_dir() as tmp:
        write(tmp, "src/u/one.c", "/* */\n")
        write(tmp, "configure.py", 'config.libs = [{"lib": "u", "objects": [Object(NonMatching, "u/one.c")]}]\n')
        write(tmp, "config/RMHE08/splits.txt", "u/one.c:\n\t.text       start:0x80100000 end:0x80100100\n")
        check("an empty backlog hands out a normal claim", next_brief(tmp, None, dry_run=True)["unit"], "u/one")
        write(tmp, ".pi/outbox/lane.json", json.dumps(
            {"unit": "auto/x", "worker": "w1", "finished_at": "2026-09-01T00:00:00",
             "config_requests": [{"kind": "shared-file", "file": "include/unsplit/lobby.h",
                                  "why": "the header's `s32 fn_80215C98(...)` has the wrong arity - every call "
                                         "site passes five arguments."}]}))
        items_before, _ = backlog.build(tmp)
        check("the ledger starts at 1 credit", backlog.ledger_summary(items_before, [])["balance"], 1)
        over = next_brief(tmp, "w-led", dry_run=False, claim_fn=fake_claim)
        check("balance >= 1 hands out the claim and records it",
              (over["unit"], [c["unit"] for c in backlog.load_ledger(tmp)["claims"]]), ("u/one", ["u/one"]))
        registry.save(tmp, {})
        try:
            next_brief(tmp, None, dry_run=True)
            check("balance 0 refuses the next claim", "no error", "SystemExit")
        except SystemExit as exc:
            msg = str(exc)
            check("balance 0 refuses, naming the balance, the top item and a paste-ready lane",
                  ("REFUSED queue next" in msg, "balance is 0" in msg, "include/unsplit/lobby.h" in msg,
                   "claude --agent" in msg), (True, True, True, True))
        over2 = next_brief(tmp, "w-ignore", dry_run=False, claim_fn=fake_claim, ignore_backlog=True)
        check("--ignore-backlog hands out the claim and spends nothing",
              (over2["unit"], len(backlog.load_ledger(tmp)["claims"])), ("u/one", 1))

    # the debt itself is claimable (`queue.py debt`), on the same lock and the same currency
    with testing.temp_dir() as tmp:
        for sub in ("src/mod", "config/RMHE08", ".pi/outbox", ".pi/notes"):
            os.makedirs(os.path.join(tmp, sub), exist_ok=True)
        write(tmp, "configure.py", "config.libs = [\n]\n")
        write(tmp, "config/RMHE08/symbols.txt", "")
        write(tmp, "config/RMHE08/splits.txt", "")
        write(tmp, "src/mod/debt.c", "void fn_80040598(void) {}\nvoid fn_80040599(void) {}\nvoid fn_8004059A(void) {}\n")
        debt_items = backlog.open_debt_items(tmp)
        check("the register offers a naming debt item to claim", [(i.kind, i.target, i.weight) for i in debt_items],
              [("naming", "src/mod/debt.c", 3)])
        before = backlog.build(tmp)[1]["summary"]["balance"]
        out = next_debt_brief(tmp, "w-debt", dry_run=False, claim_fn=fake_claim, slots_mode=False)
        check("the queue hands out the debt item with its names and task",
              ((out["kind"], out["target"]), sorted(out["names"]), out["unit"],
               "clean the 3 distinct name(s) in `src/mod/debt.c`" in out["spawn"]["task"]),
              (("naming", "src/mod/debt.c"), ["fn_80040598", "fn_80040599", "fn_8004059A"], "src/mod/debt.c", True))
        check("... spending exactly one credit, recorded as debt",
              (before - backlog.build(tmp)[1]["summary"]["balance"], backlog.load_ledger(tmp)["claims"][0]["kind"]),
              (1, "debt"))
        check("a claimed debt item is not re-offered",
              _raises(lambda: next_debt_brief(tmp, None, dry_run=True, slots_mode=False, ignore_backlog=True)), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --- CLI ----------------------------------------------------------------------------------------------------------

def _print_spawn(sp: dict) -> None:
    print("spawn this worker:")
    print("  agent: %s" % sp["agent"])
    print("  label: %s  (the tool takes no name)" % sp["name"])
    print("  cwd:   %s" % sp["cwd"])
    print("  task:  %s" % sp["task"])
    print("\n%s" % sp["call"])


def main() -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    n = sub.add_parser("next", help="claim the next ready unit (or a cluster), render its brief, print the spawn")
    n.add_argument("--count", type=int, default=1,
                   help="claim up to N lanes at once, no two sharing an owner header or a module (default 1)")
    n.add_argument("--cluster", default=None, metavar="MODULE|HEADER",
                   help="claim ONE lane for every ready unit of a module (`Network`) or of a header's include "
                        "closure within the header's own module (`include/Network/net.h`)")
    n.add_argument("--cross-module", action="store_true",
                   help="with a header --cluster: also take units of other modules whose closure includes it")
    n.add_argument("--worker", default=None)
    n.add_argument("--kind", default="unit",
                   help="the lane kind -> agent profile (`unit`->surveyor, `fix`->fixer, `merge`->merger, "
                        "`tooling`/`docs`->worker, `review`->codereviewer, `scout`/`plan` read-only; default unit)")
    n.add_argument("--profile", default=None, choices=_profiles(),
                   help="override the agent profile the lane is launched with (default: derived from --kind)")
    n.add_argument("--dry-run", action="store_true")
    n.add_argument("--allow-unlanded", action="append", default=[], metavar="BRANCH",
                   help="name a branch that is parked on purpose, so the unlanded-branch guard lets it through")
    n.add_argument("--ignore-backlog", action="store_true",
                   help="hand out a claim even when the backlog credit balance does not cover it, without spending")
    n.add_argument("--no-slots", action="store_true",
                   help="construct throwaway worktrees instead of taking from the reusable slot pool")
    n.add_argument("--ratio", type=int, default=backlog.RATIO_DEFAULT,
                   help="credits one claim spends (default 1: one resolved `done` buys one claim)")
    n.add_argument("--json", action="store_true")
    lp = sub.add_parser("list", help="the registered units' state and the next ready candidates")
    lp.add_argument("--json", action="store_true")
    d = sub.add_parser("debt", help="claim the top naming/band-header backlog item (clean its names)")
    d.add_argument("--worker", default=None)
    d.add_argument("--dry-run", action="store_true")
    d.add_argument("--ignore-backlog", action="store_true",
                   help="hand out the debt even when the credit balance does not cover it, without spending")
    d.add_argument("--no-slots", action="store_true",
                   help="construct a throwaway worktree instead of taking from the reusable slot pool")
    d.add_argument("--ratio", type=int, default=backlog.RATIO_DEFAULT, help="credits the debt claim spends")
    d.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0

    main_wt = registry.main_of()
    if args.cmd == "list":
        st = pool_state(main_wt)
        if args.json:
            print(json.dumps({"dir": st["dir"], "counts": st["counts"],
                              "next": [e["unit"] for e in st["ready"][:POOL_DEPTH]]}, indent=2))
            return 0
        c = st["counts"]
        print("queue: the registered units of %s" % os.path.join(main_wt, "configure.py"))
        print("  registered     : %d" % len(st["entries"]))
        print("  ready          : %d  (not Matching, unclaimed)" % c.get("ready", 0))
        print("  claimed        : %d  (in flight, alone or in a cluster)" % c.get("claimed", 0))
        print("  matching       : %d  (done - never handed out)" % c.get("matching", 0))
        print("  nosource       : %d  (registered with no source file)" % c.get("nosource", 0))
        un = unlanded_branches(main_wt)
        if un:
            print("  unlanded       : %d branch(es) hold work main does not have - `next` refuses until they "
                  "are landed or deleted" % len(un))
            for branch, files in un[:6]:
                print("      %s (%s)" % (branch, ", ".join("%s +%d" % (f, n) for f, n in files[:3])))
        else:
            print("  unlanded       : none - every branch's content is contained in main")
        if st["ready"]:
            print("\nnext %d ready (address order):" % min(POOL_DEPTH, len(st["ready"])))
            for e in st["ready"][:POOL_DEPTH]:
                start = text_start(main_wt, e["unit"])
                print("  %-10s  %s" % ("0x%X" % start if start is not None else "?", e["unit"]))
        else:
            print("\nno ready unit - every registered unit is Matching or claimed")
        return 0

    if args.cmd == "debt":
        out = next_debt_brief(main_wt, args.worker, args.dry_run, ratio=args.ratio,
                              ignore_backlog=args.ignore_backlog, slots_mode=(False if args.no_slots else None))
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if out["dry_run"]:
            print("DRY RUN - nothing claimed, nothing written\n")
        else:
            print("claimed debt %s (%s %s: %d name(s))\n  branch   %s\n  worktree %s\n  brief    %s\n"
                  % (out["item"], out["kind"], out["target"], len(out["names"]),
                     out["claim"].get("branch"), out["worktree"], out["brief"]))
        _print_spawn(out["spawn"])
        return 0

    if args.cmd == "next":
        common = dict(kind=args.kind, profile=args.profile, allow_unlanded=args.allow_unlanded,
                      ignore_backlog=args.ignore_backlog, ratio=args.ratio,
                      slots_mode=(False if args.no_slots else None))
        if args.cluster and args.count != 1:
            print("--cluster claims one lane for the whole cluster; --count does not apply")
            return 2
        if args.cluster:
            out = next_cluster(main_wt, args.cluster, args.worker, args.dry_run, cross_module=args.cross_module,
                               **common)
            if args.json:
                print(json.dumps(out, indent=2))
                return 0
            if out["dry_run"]:
                print("DRY RUN - nothing claimed, nothing written\n")
            print("cluster %s: %d unit(s) (%s)\n  %s" % (out["unit"], len(out["units"]), out["reason"],
                                                       "\n  ".join(out["units"])))
            if not out["dry_run"]:
                print("  branch   %s\n  worktree %s\n  brief    %s\n" % (out["claim"].get("branch"), out["worktree"],
                                                                      out["brief"]))
            _print_spawn(out["spawn"])
            return 0
        if args.count != 1:
            out = next_briefs(main_wt, args.worker, args.dry_run, args.count, **common)
            if args.json:
                print(json.dumps(out, indent=2))
                return 0
            if out["dry_run"]:
                print("DRY RUN - nothing claimed, nothing written\n")
            else:
                print("claimed %d of %d lane(s) (no two share an owner header or a module):\n"
                      % (out["claimed"], out["requested"]))
            for i, c in enumerate(out["claims"], 1):
                start = text_start(main_wt, c["unit"])
                print("--- %d/%d  %s  %s" % (i, out["claimed"], "0x%X" % start if start is not None else "?", c["unit"]))
                if not c["dry_run"]:
                    print("  branch   %s\n  worktree %s\n  brief    %s" % (c["claim"].get("branch"), c["worktree"],
                                                                         c["brief"]))
                _print_spawn(c["spawn"])
                print()
            if out["shortfall"]:
                print("NOTE: claimed %d of the %d requested - the other ready units share an owner header or a "
                      "module with a lane in this wave; `--cluster <module|header>` gives them one lane"
                      % (out["claimed"], out["requested"]))
            return 0
        out = next_brief(main_wt, args.worker, args.dry_run, **common)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if out["dry_run"]:
            print("DRY RUN - nothing claimed, nothing written\n")
        else:
            print("claimed %s\n  branch   %s\n  worktree %s\n  brief    %s\n"
                  % (out["unit"], out["claim"].get("branch"), out["worktree"], out["brief"]))
        _print_spawn(out["spawn"])
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
