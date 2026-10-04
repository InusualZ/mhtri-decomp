"""Claim a unit (branch = lock, slot or worktree, registry row), heartbeat, stall status, the one release teardown.
Spec: docs/tools/spec/claims.md. CLI: claims.py claim <unit> [--kind K] [--slot N] [--no-slots] [--dry-run] | list |
release <unit>|--branch B|--all-merged [--force] | ack <unit> | status | timeout [--apply] | expire | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import shutil
import subprocess
import time

from tools.lib import proc as _proc
from tools.lib import testing
from tools.lib.git import Git
from tools.lib.lanes import launch, naming, pool, registry, rescue as lane_rescue, seed, sessions, teardown
from tools.lib.lanes.naming import (BRANCH_PREFIX, SLUG_MAX, branch_for, norm_unit, slug,  # noqa: F401
                                    slug_of_branch, worktree_for)
from tools.lib.lanes.registry import (ack_path, branch_exists, claim_branch, claim_slug, commits_ahead,  # noqa: F401
                                      handoff_slug, load_ack, lock_held, merged_into_main, notes_path,
                                      outbox_path, registry_path, worker_branches)
from tools.lib.lanes.seed import (ORIG_JUNCTION_MIN_BYTES, ORIG_REL, RMHE08_REL, SEED_COPY_DIRS,  # noqa: F401
                                  seed_worktree_build)

_proc.install_spawn_retry()  # a launch Windows refuses transiently (WinError 5) is retried

load_registry = registry.load
save_registry = registry.save
registry_record = registry.record
registry_key = registry.key_of
rescue_ref_name = naming.rescue_ref
rescue_verdict = lane_rescue.verdict
registered_worktree_paths = registry.worktree_paths
_same_path = registry.same_path
_age_seconds = registry.age_seconds
# the seeder's names as the older callers spell them
_build_is_current = seed.build_is_current
_main_build_is_current = seed.main_build_is_current
_ninja_deps_serialize = seed.ninja_deps_serialize
_ninja_deps_parse = seed.ninja_deps_parse
_ninja_deps_rewrite = seed.ninja_deps_rewrite
_is_reparse_point = teardown.is_reparse_point
_make_junction = teardown.make_junction


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = Git(cwd).run(*args)
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, out.stderr.strip()))
    return out.stdout


def safe_worktree_remove(path: str, main: str) -> None:
    """Unlink the reparse points under `path`, then `git worktree remove --force` it (raises on failure)."""
    teardown.unlink_reparse_points(path)
    git(["worktree", "remove", "--force", path], main)


def remove_worktree(main: str, path: str) -> str:
    """Remove one worktree, repairing the submodule and the untracked-leftover states (`lib.lanes.teardown`)."""
    return teardown.remove_worktree_checked(main, path)


rescue_exists = registry.rescue_exists


def _git_quiet(args: list[str], cwd: str) -> str | None:
    p = Git(cwd).run(*args)
    return p.stdout.strip() if p.returncode == 0 else None


# --- the live-lane probe (a session whose pid is alive and whose cwd is the claim's tree) ----------------------

def session_probe(row: dict, run_registry: str | None = None) -> dict:
    """What the session registry says about the claim's tree: `{session, known, alive}`. `known` is False when
    there is no registry to read (no signal, which is not "no lane")."""
    known = bool(run_registry) or bool(sessions.run_registries())
    runs = sessions.runs_in(row.get("worktree") or "", run_registry) if row.get("worktree") else []
    return {"session": sessions.run_label(runs[0]) if runs else None, "known": known, "alive": bool(runs)}


# --- ack --------------------------------------------------------------------------------------------------------

def ack_place_error(main: str, unit: str, cwd: str | None = None) -> str | None:
    """Refuse an ack from outside the claim's worktree or off its branch; None when right or unprovable."""
    record = registry_record(main, unit)
    want_wt, want_branch = record.get("worktree"), record.get("branch")
    if not want_wt or not want_branch:
        return None
    here = cwd or os.getcwd()
    top = _git_quiet(["rev-parse", "--show-toplevel"], here)
    branch = _git_quiet(["rev-parse", "--abbrev-ref", "HEAD"], here)
    if top is None or branch is None:
        return None
    if not _same_path(top, want_wt):
        return "the ack is not in the claim's worktree (expected %s, found %s)" % (want_wt, top)
    if branch != want_branch:
        return "the ack's HEAD is not the claim's branch (expected %s, found %s)" % (want_branch, branch)
    return None


def ack(unit: str, main: str, agent: str | None = None, progress: str | None = None) -> dict:
    """A worker's heartbeat (first action, then after every measured iteration); refused from the wrong place."""
    unit = unit.strip("/")
    place = ack_place_error(main, unit)
    if place:
        raise SystemExit("REFUSED ack %s | %s: run the ack from inside the claim's worktree, on its branch"
                         % (unit, place))
    path = ack_path(main, unit)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    data = load_ack(main, unit) or {"unit": unit}
    record = load_registry(main).get(unit, {})
    now = registry.now()
    data.setdefault("worker", record.get("worker"))
    data.setdefault("branch", record.get("branch"))
    data.setdefault("acked_at", now)
    if agent:
        data["agent"] = agent
    if progress:
        data.setdefault("progress", []).append({"at": now, "note": progress})
    data["last_progress_at"] = now
    data["iterations"] = len(data.get("progress") or [])
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)
    return data


# --- the views ------------------------------------------------------------------------------------------------

def claims_view(main: str) -> list[dict]:
    """Every worker claim git or the registry knows about, enriched with what the worker left behind."""
    reg = load_registry(main)
    rows: list[dict] = []
    seen: set[str] = set()
    for wt in Git(main).worktree_list():
        branch = wt.branch or ""
        if not branch.startswith(BRANCH_PREFIX):
            continue
        seen.add(branch)
        unit = next((u for u, v in reg.items() if v.get("branch") == branch), None)
        record = reg.get(unit or "", {})
        rows.append({"unit": unit or "(unregistered)", "branch": branch, "worktree": wt.path,
                     "worker": record.get("worker"), "claimed_at": record.get("claimed_at"),
                     "base": record.get("base"), "merged": merged_into_main(main, branch),
                     "outbox": bool(unit and os.path.exists(outbox_path(main, unit))),
                     "acked": bool(unit) and os.path.exists(ack_path(main, unit)),
                     "exists": os.path.isdir(wt.path)})
    for unit, record in reg.items():
        branch = record.get("branch")
        if branch in seen:
            continue
        path = record.get("worktree") or worktree_for(unit, main)
        rows.append({"unit": unit, "branch": branch, "worktree": path, "worker": record.get("worker"),
                     "claimed_at": record.get("claimed_at"), "base": record.get("base"),
                     "merged": bool(branch) and merged_into_main(main, branch),
                     "outbox": os.path.exists(outbox_path(main, unit)),
                     "acked": os.path.exists(ack_path(main, unit)), "exists": os.path.isdir(path)})
    return sorted(rows, key=lambda r: r["unit"])


def claim_status(main: str, ack_seconds: float = 120, stall_minutes: float = 20, probe=None, view=None,
                 run_registry: str | None = None) -> list[dict]:
    """Every claim's state - done (outbox), unacked (no ack past the grace), stalled (no progress for
    `stall_minutes`), working - where a live session in the claim's tree overrides unacked/stalled."""
    reg = load_registry(main)
    rows = []
    for row in (claims_view(main) if view is None else view):
        unit = row["unit"]
        record = reg.get(unit, {})
        data = load_ack(main, unit) if unit != "(unregistered)" else {}
        claimed = _age_seconds(row.get("claimed_at"))
        acked = _age_seconds(data.get("acked_at"))
        progress = _age_seconds(data.get("last_progress_at"))
        commits = 0
        if row.get("branch") and row.get("base"):
            p = Git(main).run("rev-list", "--count", "%s..%s" % (row["base"], row["branch"]))
            if p.returncode == 0 and p.stdout.strip().isdigit():
                commits = int(p.stdout.strip())
        if row["outbox"]:
            state = "done"
        elif not data and claimed is not None and claimed > ack_seconds:
            state = "unacked"
        elif data and progress is not None and progress > stall_minutes * 60:
            state = "stalled"
        else:
            state = "working"
        rows.append({**row, "state": state, "acked_seconds_ago": round(acked) if acked is not None else None,
                     "progress_seconds_ago": round(progress) if progress is not None else None,
                     "claimed_seconds_ago": round(claimed) if claimed is not None else None,
                     "commits": commits, "iterations": data.get("iterations", 0),
                     "agent": data.get("agent") or (record.get("spawn") or {}).get("agent") or record.get("agent")})
    probe = probe or (lambda r: session_probe(r, run_registry))
    for row in rows:
        if row["state"] not in ("unacked", "stalled"):
            continue
        info = probe(row)
        row["session"] = info.get("session")
        row["session_alive"] = info.get("alive")
        if info.get("alive"):
            row["state"] = "working"
            row["session_note"] = "a live session works in the claim's tree: %s" % info.get("session")
    return rows


def claim_place_error(main: str, cwd: str | None = None) -> str | None:
    """Refuse a claim begun outside MAIN or while MAIN's HEAD is not `main`; None when right or unprovable."""
    here = cwd or os.getcwd()
    top = _git_quiet(["rev-parse", "--show-toplevel"], here)
    branch = _git_quiet(["rev-parse", "--abbrev-ref", "HEAD"], here)
    if top is None or branch is None:
        return None
    if not _same_path(top, main):
        return "the claim is not run from MAIN (expected %s, found %s)" % (main, top)
    if branch != "main":
        return ("MAIN's HEAD is on %r, not main: `git checkout main` first - a claim is rooted at MAIN's "
                "HEAD, so one made off main roots the worker on the wrong base" % branch)
    return None


# --- claim ------------------------------------------------------------------------------------------------------

def claim(unit: str, main: str, worker: str | None, dry_run: bool, cwd: str | None = None,
          slots_mode: bool | None = None, slot: int | None = None, kind: str = "unit",
          units: list[str] | None = None) -> dict:
    """Reserve `unit`: a branch (the lock) and a slot (when a pool exists, unless `slots_mode=False`) or a
    throwaway worktree, then the registry row with the kind and its profile. `units` lists the units a cluster
    claim holds (recorded in the row, so every one of them reads as claimed)."""
    unit = norm_unit(unit.strip("/"))
    place = claim_place_error(main, cwd)
    if place:
        raise SystemExit("REFUSED claim %s | %s: run the claim from MAIN, on main" % (unit, place))
    branch = branch_for(unit)
    agent = launch.profile_for_kind(kind)
    use_slots = slot is not None or (slots_mode if slots_mode is not None else pool.enabled(main))
    if use_slots and not pool.enabled(main):
        raise SystemExit("REFUSED: the slot pool is requested but not initialised - run "
                         "`python tools/units/slots.py init` (or pass --no-slots)")
    if branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)
    slot_id: int | None = None
    if not use_slots:
        path = worktree_for(unit, main)
        if os.path.exists(path):
            raise SystemExit("REFUSED: %s already exists" % path)
        base = git(["rev-parse", "HEAD"], main).strip()
        cmd = ["worktree", "add", "-b", branch, path, base]
        if dry_run:
            return {"unit": unit, "branch": branch, "worktree": path, "base": base, "kind": kind, "agent": agent,
                    "command": "git " + " ".join(cmd), "dry_run": True}
        git(cmd, main)
        seed_note = seed_worktree_build(main, path)
    else:
        if dry_run:
            info = pool.preview(main, unit, branch, slot)
            return {"unit": unit, "branch": branch, "worktree": info["dir"], "base": info["base"],
                    "slot": info["slot"], "kind": kind, "agent": agent, "command": info["command"], "dry_run": True}
        info = pool.acquire(main, unit, branch=branch, worker=worker, slot=slot)
        path, base, seed_note, slot_id = info["dir"], info["base"], info["seeded"], info["slot"]
        lock = pool.read_lock(main, slot_id)
        if lock:
            lock["kind"], lock["agent"] = kind, agent
            pool.write_lock(main, slot_id, lock)
    for sub in ("outbox", "notes"):
        os.makedirs(os.path.join(main, ".pi", sub), exist_ok=True)
    reg = load_registry(main)
    reg[unit] = {"branch": branch, "worktree": path, "worker": worker or os.environ.get("USERNAME")
                 or os.environ.get("USER") or "unknown", "base": base, "claimed_at": registry.now(),
                 "slot": slot_id, "kind": kind, "agent": agent}
    if units:
        reg[unit]["units"] = [norm_unit(u) for u in units]
    save_registry(main, reg)
    return {"unit": unit, "branch": branch, "worktree": path, "base": base, "seeded": seed_note,
            "slot": slot_id, "kind": kind, "agent": agent, **({"units": reg[unit]["units"]} if units else {})}


# --- release: the one teardown ------------------------------------------------------------------------------

def release(unit: str, main: str, force: bool, dry_run: bool, probe=None, branch: str | None = None,
            run_registry: str | None = None) -> dict:
    """The one-shot, idempotent teardown of a claim: rescue ref (+ its verdict), then the slot return or the
    worktree removal, prune, branch delete, registry entry and ack file - each step reported. Refused: an
    unmerged claim with no outbox (unless `force`, which costs the branch: its commits live on at the rescue
    ref) and, for a worktree claim, a live session in its tree. `branch` names the claim by branch."""
    unit = norm_unit(unit.strip("/"))
    reg = load_registry(main)
    if branch:
        key, record = registry.record_for_branch(reg, branch)
        unit = key or (slug_of_branch(branch) or branch)
    else:
        key = registry_key(reg, unit)
        branch = (reg.get(key, {}) if key else {}).get("branch") or branch_for(unit)
        record = reg.get(key, {}) if key else {}
    path = record.get("worktree")
    if not path:
        entry = next((w for w in Git(main).worktree_list() if w.branch == branch), None)
        path = entry.path if entry else worktree_for(unit, main)
    slot_id = record.get("slot")
    if slot_id is None:
        for row in pool.all_slots(main):
            if (row.get("lock") or {}).get("branch") == branch:
                slot_id = row["slot"]
                break
    handoff = slug_of_branch(branch) or slug(unit)
    ack_file = ack_path(main, unit)
    outbox_file = os.path.join(main, ".pi", "outbox", handoff + ".json")
    outbox = os.path.exists(outbox_file)
    branch_present = branch_exists(main, branch)
    merged = merged_into_main(main, branch) if branch_present else True
    result = {"unit": unit, "branch": branch, "worktree": path, "merged": merged, "outbox": outbox,
              "registry": key is not None, "session": None, "release_ref": None, "rescue_verdict": None,
              "steps": [], "complete": False, "dry_run": dry_run, "refused": None, "cost": None}
    forced = force and not (merged or outbox)
    if not force and not (merged or outbox):
        result["refused"] = ("%s's branch %s is neither merged into main nor has an outbox entry at\n  %s\n"
                             "  releasing it would drop work with no record. Finish the handoff, or pass "
                             "--force (the branch is deleted; its commits are rescued to %s)."
                             % (unit, branch, outbox_file, rescue_ref_name(unit)))
        return result
    if slot_id is None:
        # a worktree claim: a live session sitting in the tree is the one refusal (Windows will not delete a
        # directory a process is in). A slot claim is guarded by the slot return's own blockers, below.
        info = (probe or (lambda r: session_probe(r, run_registry)))({"worktree": path})
        result["session"] = info.get("session")
        if info.get("known") and info.get("alive"):
            result["refused"] = ("a live session (%s) works in %s; release again once it has exited"
                                 % (info.get("session"), path))
            return result

    steps: list[dict] = []
    if branch_present and not merged:
        ref = rescue_ref_name(unit)
        if commits_ahead(main, branch):
            result["release_ref"] = ref
            steps.append(teardown.step("git update-ref %s %s" % (ref, branch),
                                       lambda b=branch, r=ref: git(["update-ref", r, b], main) and None))

            def _audit_rescue(r=ref):
                result["rescue_verdict"] = verdict = rescue_verdict(main, r)
                return verdict["line"]
            steps.append(teardown.step("rescue ref audit %s" % ref, _audit_rescue))
        else:
            steps.append(teardown.skip("rescue ref %s" % ref, "branch has no commits of its own"))
            steps.append(teardown.skip("rescue ref audit", "no rescue ref: the branch had no commits of its own"))
    else:
        steps.append(teardown.skip("rescue ref", "branch already merged into main" if branch_present
                                   else "branch already gone"))
        steps.append(teardown.skip("rescue ref audit", "no rescue ref: nothing was parked"))
    if slot_id is not None:
        def _return_slot(sid=slot_id):
            pool.release(main, slot=sid, unit=unit, branch=branch, rescue=False, refresh=True, force=force,
                         allow_dirty=True, registry=run_registry)
            return "returned to main's tip, branch deleted, build tree refreshed"
        if os.path.isdir(path):
            steps.append(teardown.step("slot %d return (%s)" % (slot_id, path), _return_slot))
        else:
            steps.append(teardown.skip("slot %d return" % slot_id, "slot directory already gone"))
        steps.append(teardown.skip("git worktree prune", "slots are persistent worktrees - nothing to prune"))
        steps.append(teardown.skip("git branch -D %s" % branch, "the slot return deleted the branch"))
    else:
        if os.path.isdir(path):
            steps.append(teardown.step("git worktree remove --force %s" % path, lambda p=path: remove_worktree(main, p)))
        else:
            steps.append(teardown.skip("git worktree remove --force %s" % path, "worktree already gone"))
        steps.append(teardown.step("git worktree prune", lambda: git(["worktree", "prune"], main)))
        if branch_present:
            steps.append(teardown.step("git branch -D %s" % branch, lambda b=branch: git(["branch", "-D", b], main)))
        else:
            steps.append(teardown.skip("git branch -D %s" % branch, "branch already gone"))
    if forced and branch_present:
        if result["release_ref"]:
            result["cost"] = ("--force deleted the branch %s: its commits survive only at %s. Restore with:\n"
                              "    git branch %s %s" % (branch, result["release_ref"], branch, result["release_ref"]))
        else:
            result["cost"] = "--force deleted the branch %s, which had no commits of its own" % branch
    if dry_run:
        result["steps"] = steps
        result["complete"] = None
        return result
    result["complete"] = complete = teardown.run(steps)
    if result["cost"] and (result["rescue_verdict"] or {}).get("pruned"):
        result["cost"] = ("--force deleted the branch %s; its rescue ref %s was pruned by the audit (the unit "
                          "is on main and every touched path matches), so nothing was lost"
                          % (branch, result["release_ref"]))
    if complete:
        if key is not None:
            reg.pop(key, None)
            save_registry(main, reg)
            steps.append(teardown.done("registry entry %s" % key))
        else:
            steps.append(teardown.skip("registry entry %s" % unit, "no entry"))
        if os.path.exists(ack_file):
            os.remove(ack_file)
            steps.append(teardown.done("ack file %s" % os.path.basename(ack_file), "removed"))
        else:
            steps.append(teardown.skip("ack file %s" % os.path.basename(ack_file), "none"))
    else:
        steps.append(teardown.skip("registry entry %s" % unit, "kept: the teardown did not complete"))
        steps.append(teardown.skip("ack file %s" % os.path.basename(ack_file), "kept: the teardown did not complete"))
    result["steps"] = steps
    return result


def worktree_dirty(path: str) -> bool | None:
    """True when the worktree has uncommitted changes; None when it cannot be asked."""
    if not os.path.isdir(path):
        return None
    p = subprocess.run(["git", "-C", path, "status", "--porcelain"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return bool(p.stdout.strip()) if p.returncode == 0 else None


def release_merged(main: str, dry_run: bool = False, probe=None) -> dict:
    """Release every finished claim (merged and clean, or with an outbox); skip and refuse the rest, named."""
    out: dict = {"released": [], "skipped": [], "refused": [], "complete": True, "dry_run": dry_run}
    for row in claims_view(main):
        unit, branch = row["unit"], row.get("branch")
        if unit == "(unregistered)":
            out["skipped"].append({"unit": unit, "branch": branch, "why": "no registry entry"})
            continue
        if not row.get("merged"):
            out["skipped"].append({"unit": unit, "branch": branch, "why": "branch is not merged into main"})
            continue
        if not row.get("outbox") and worktree_dirty(row.get("worktree") or ""):
            out["skipped"].append({"unit": unit, "branch": branch, "why": "worktree has uncommitted changes and no outbox"})
            continue
        result = release(unit, main, force=False, dry_run=dry_run, probe=probe)
        if result.get("refused"):
            out["refused"].append({"unit": unit, "branch": result["branch"], "why": result["refused"]})
            out["complete"] = False
        elif dry_run or result.get("complete"):
            out["released"].append(result)
        else:
            failed = next((s["label"] for s in result["steps"] if s["status"] == "failed"), "a step")
            out["refused"].append({"unit": unit, "branch": result["branch"],
                                   "why": "teardown incomplete: %s failed" % failed})
            out["complete"] = False
    return out


def timeout(main: str, ack_seconds: float = 120, stall_minutes: float = 20, apply: bool = False,
            unit: str | None = None, view=None, probe=None, run_registry: str | None = None) -> list[dict]:
    """Reclaim what a silent worker holds - unacked or stalled, and no live session in its tree - through the
    one teardown (`release --force`: the commits are parked at the rescue ref first). `unit` forces one claim
    the ack calls healthy, never one with a live session."""
    victims = []
    for row in claim_status(main, ack_seconds, stall_minutes, probe=probe, view=view, run_registry=run_registry):
        if unit and row["unit"] != unit.strip("/"):
            continue
        if row["state"] in ("unacked", "stalled") or (unit and row["state"] != "done"):
            victims.append(row)
    victims = [r for r in victims if not r.get("session_alive")]
    for row in victims:
        plan = release(row["unit"], main, force=True, dry_run=True, probe=probe, branch=row.get("branch") or None,
                       run_registry=run_registry)
        row["steps"] = [s["label"] for s in plan["steps"] if s["status"] != "skipped"]
        row["rescue"] = plan.get("release_ref")
        if apply:
            done = release(row["unit"], main, force=True, dry_run=False, probe=probe,
                           branch=row.get("branch") or None, run_registry=run_registry)
            if done.get("refused") or not done.get("complete"):
                failed = next((s for s in done.get("steps", []) if s["status"] == "failed"), None)
                row["error"] = done.get("refused") or ("%s: %s" % (failed["label"], failed["why"]) if failed
                                                       else "incomplete")
    return victims


def expire(main: str, minutes: int, apply: bool) -> list[dict]:
    """Claims older than `minutes` with no outbox; `apply` releases each with `--force`."""
    now = time.time()
    stale = []
    for row in claims_view(main):
        if not row.get("claimed_at") or row["outbox"]:
            continue
        try:
            age = (now - time.mktime(time.strptime(row["claimed_at"], registry.STAMP))) / 60.0
        except ValueError:
            continue
        if age >= minutes:
            row["age_minutes"] = round(age, 1)
            stale.append(row)
    if apply:
        for row in stale:
            if row["unit"] == "(unregistered)":
                row["released"] = False
                row["error"] = "no registry entry"
                continue
            out = release(row["unit"], main, force=True, dry_run=False)
            row["released"] = bool(out.get("complete"))
            if not row["released"]:
                failed = next((s for s in out["steps"] if s["status"] == "failed"), None)
                row["error"] = out.get("refused") or ("%s: %s" % (failed["label"], failed["why"]) if failed
                                                      else "incomplete")
    return stale


# --- selftest -----------------------------------------------------------------------------------------------

def selftest() -> int:
    testing.isolate_live_state()   # no real ~/.claude/sessions: a live lane must not change the verdict
    fails = []
    checks = 0

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

    check("slug: simple", slug("Pl/pl_act").startswith("pl-act-"), True)
    check("slug: stable", slug("Pl/pl_act"), slug("Pl/pl_act"))
    check("slug: unique per unit", slug("Pl/pl_act") == slug("Pl/pl_skill"), False)
    check("slug: sanitised", bool(re.fullmatch(r"[a-z0-9-]+", slug("auto/80040598_fn_80040598"))), True)
    check("slug: the source extension does not fork it",
          slug("proposal/801679B0_fn_801679B0"), slug("proposal/801679B0_fn_801679B0.cpp"))
    check("slug: length cap", len(slug("auto/" + "x" * 200)) <= SLUG_MAX, True)
    check("branch prefix", branch_for("main.cpp").startswith("worker/"), True)
    check("worktree is a sibling of main", os.path.basename(worktree_for("Pl/pl_act", "/tmp/mhtri-dtk"))
          .startswith("mhtri-dtk.ws-"), True)
    check("outbox path is in MAIN", os.path.dirname(os.path.dirname(outbox_path("/tmp/mhtri-dtk", "Pl/pl_act"))),
          os.path.join("/tmp/mhtri-dtk", ".pi"))

    with testing.temp_dir() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(main)
        check("registry starts empty", load_registry(main), {})
        save_registry(main, {"Camellia/camellia": {"branch": "worker/camellia-67ed"},
                             "Gecko/Gecko_ExceptionPPC.cp": {"branch": "worker/gecko-exceptionppc-313d"}})
        check("a registry record is found under the raw key",
              registry_record(main, "Gecko/Gecko_ExceptionPPC.cp")["branch"], "worker/gecko-exceptionppc-313d")
        check("and under the extensionless spelling",
              registry_record(main, "Gecko/Gecko_ExceptionPPC")["branch"], "worker/gecko-exceptionppc-313d")
        check("an unknown unit has no record", registry_record(main, "Nope/nothing"), {})
        check("the claim branch prefers the stored value",
              claim_branch(main, "Gecko/Gecko_ExceptionPPC"), "worker/gecko-exceptionppc-313d")
        check("the claim branch falls back to a computed branch", claim_branch(main, "RSO/runtime"),
              branch_for("RSO/runtime"))
        check("the outbox path ignores the spelling",
              outbox_path(main, "Camellia/camellia.c"), outbox_path(main, "Camellia/camellia"))
        check("the outbox is the stored branch's slug", os.path.basename(outbox_path(main, "Camellia/camellia")),
              "camellia-67ed.json")

    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, ".pi"), exist_ok=True)
        save_registry(tmp, {"Pl/x": {"branch": "worker/x", "claimed_at": "2026-01-01T00:00:00"}})
        check("an unacked claim has no ack file", load_ack(tmp, "Pl/x"), {})
        out = ack("Pl/x", tmp, agent="w-a", progress="fn_1")
        check("ack records the agent and counts iterations", (out.get("agent"), out.get("iterations")), ("w-a", 1))
        out = ack("Pl/x", tmp, progress="fn_2")
        check("a second ack appends progress", out.get("iterations"), 2)
        check("acked_at is not overwritten", out.get("acked_at"), load_ack(tmp, "Pl/x")["acked_at"])
        check("_age_seconds parses / tolerates junk",
              (_age_seconds(out["last_progress_at"]) is not None, _age_seconds("nonsense")), (True, None))
        check("ack takes no herdr pane any more", "pane" in out, False)

    def repo_git(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def repo_commit(path, msg):
        with open(os.path.join(path, "f.txt"), "a", encoding="utf-8") as fh:
            fh.write(msg + "\n")
        repo_git(path, "add", "-A")
        repo_git(path, "commit", "-q", "-m", msg)

    def new_repo(tmp):
        path = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(path)
        repo_git(path, "init", "-q")
        repo_git(path, "checkout", "-q", "-b", "main")
        repo_commit(path, "claim-time main")
        return path

    def claimed(main, unit_name, worktree=True, outbox=True):
        branch = branch_for(unit_name)
        wt = worktree_for(unit_name, main)
        if worktree:
            repo_git(main, "worktree", "add", "-b", branch, wt)
            repo_commit(wt, "the worker's work: %s" % unit_name)
        else:
            repo_git(main, "branch", branch)
        if outbox:
            entry = outbox_path(main, unit_name)
            os.makedirs(os.path.dirname(entry), exist_ok=True)
            with open(entry, "w") as fh:
                json.dump({"unit": unit_name}, fh)
        heartbeat = ack_path(main, unit_name)
        os.makedirs(os.path.dirname(heartbeat), exist_ok=True)
        with open(heartbeat, "w") as fh:
            json.dump({"unit": unit_name}, fh)
        reg = load_registry(main)
        reg[unit_name] = {"branch": branch, "worktree": wt}
        save_registry(main, reg)
        return branch, wt

    def step_of(result, needle):
        return next((s for s in result["steps"] if needle in s["label"]), {})

    no_session = lambda _row: {"session": None, "known": True, "alive": False}
    live_session = lambda _row: {"session": "sid-1 (worker: x)", "known": True, "alive": True}

    # the live-lane signal is the session registry: a record whose pid is alive and whose cwd is the claim's tree
    with testing.temp_dir() as tmp:
        reg_root = os.path.join(tmp, "claude", sessions.SESSIONS_DIRNAME)
        os.makedirs(reg_root)
        wt = os.path.join(tmp, "ws-live")
        os.makedirs(os.path.join(wt, "src"))
        gone = subprocess.Popen([sys.executable, "-c", "pass"])
        gone.wait()

        def write_run(pid, cwd):
            with open(os.path.join(reg_root, "1.json"), "w", encoding="utf-8") as fh:
                json.dump({"pid": pid, "sessionId": "sid-1", "cwd": cwd, "name": "worker: x"}, fh)

        write_run(os.getpid(), os.path.join(wt, "src"))
        p = session_probe({"worktree": wt}, reg_root)
        check("a live session in a subdirectory of the claim's tree is alive", (p["alive"], p["known"]), (True, True))
        check("... and is named", p["session"], "sid-1 (worker: x)")
        write_run(gone.pid, wt)
        check("a session whose process exited is not alive", session_probe({"worktree": wt}, reg_root)["alive"], False)
        write_run(os.getpid(), os.path.join(tmp, "elsewhere"))
        check("a session in another tree is not this claim's", session_probe({"worktree": wt}, reg_root)["alive"], False)
        check("no registry is no signal (known=False)", session_probe({"worktree": wt})["known"], False)

    # status and timeout: a stale ack is not enough to reclaim a worker whose session is alive
    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, "Pl/live", outbox=False)
        stamp = time.strftime(registry.STAMP, time.localtime(time.time() - 3600))
        with open(ack_path(main, "Pl/live"), "w", encoding="utf-8") as fh:
            json.dump({"unit": "Pl/live", "acked_at": stamp, "last_progress_at": stamp, "iterations": 1}, fh)
        check("a live session keeps a stale ack working",
              claim_status(main, probe=live_session)[0]["state"], "working")
        check("... with the session named", "sid-1" in claim_status(main, probe=live_session)[0]["session_note"], True)
        check("no live session leaves the stale ack stalled", claim_status(main, probe=no_session)[0]["state"], "stalled")
        check("timeout does not victimize a live session", timeout(main, probe=live_session), [])
        check("even --unit does not", timeout(main, probe=live_session, unit="Pl/live"), [])
        dry = timeout(main, probe=no_session)
        check("timeout's dry run plans the one teardown",
              [s for s in dry[0]["steps"] if s.startswith(("git update-ref", "git worktree remove", "git branch -D"))],
              ["git update-ref %s %s" % (rescue_ref_name("Pl/live"), branch),
               "git worktree remove --force %s" % wt, "git branch -D %s" % branch])
        check("... names the rescue ref", dry[0]["rescue"], rescue_ref_name("Pl/live"))
        check("... and touches nothing", (branch_exists(main, branch), os.path.isdir(wt)), (True, True))
        tip = repo_git(main, "rev-parse", branch)
        applied = timeout(main, probe=no_session, apply=True)
        check("timeout --apply reclaims through release --force", applied[0].get("error"), None)
        check("... the commits survive at the rescue ref", repo_git(main, "rev-parse", rescue_ref_name("Pl/live")), tip)
        check("... the branch, worktree, registry row and ack are gone",
              (branch_exists(main, branch), os.path.isdir(wt), load_registry(main), os.path.exists(ack_path(main, "Pl/live"))),
              (False, False, {}, False))

    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, ".pi"), exist_ok=True)
        save_registry(tmp, {"Pl/pl_act": {"branch": branch_for("Pl/pl_act")},
                            "Pl/pl_skill": {"branch": branch_for("Pl/pl_skill") + "-dd6e"}})
        check("slug_of_branch strips the prefix / tolerates None",
              (slug_of_branch("worker/pl-act-1234"), slug_of_branch(None)), ("pl-act-1234", None))
        check("claim_slug keeps a round's suffix", claim_slug(tmp, "Pl/pl_skill"), slug("Pl/pl_skill") + "-dd6e")
        check("claim_slug of an unclaimed unit", claim_slug(tmp, "RSO/runtime"), None)
        check("the outbox and notes follow the branch",
              (os.path.basename(outbox_path(tmp, "Pl/pl_skill")), os.path.basename(notes_path(tmp, "Pl/pl_skill"))),
              (slug("Pl/pl_skill") + "-dd6e.json", slug("Pl/pl_skill") + "-dd6e.md"))
        check("the ack stays keyed by the unit", os.path.basename(ack_path(tmp, "Pl/pl_skill")),
              slug("Pl/pl_skill") + ".json")

    # claim() end to end: the worktree it cuts is seeded, the kind maps to its profile, and it releases clean
    with testing.temp_dir() as tmp:
        seed_main = new_repo(tmp)
        os.makedirs(os.path.join(seed_main, "build", "tools"))
        open(os.path.join(seed_main, "build", "tools", "dtk.exe"), "wb").write(b"dtk")
        os.makedirs(os.path.join(seed_main, "build", "compilers", "Wii", "1.3"))
        open(os.path.join(seed_main, "build", "compilers", "Wii", "1.3", "mwcceppc.exe"), "wb").write(b"cc")
        os.makedirs(os.path.join(seed_main, "orig", "RMHE08", "sys"))
        open(os.path.join(seed_main, "orig", "RMHE08", "sys", "main.dol"), "wb").write(b"dol")
        out = claim("Pl/seeded", seed_main, "w", False, cwd=seed_main)
        seed_wt = out["worktree"]
        check("the claim records the lane kind and its profile", (out["kind"], out["agent"]), ("unit", "surveyor"))
        check("... in the registry too", load_registry(seed_main)["Pl/seeded"].get("agent"), "surveyor")
        tooling = claim("Pl/tooled", seed_main, "w", True, cwd=seed_main, kind="tooling")
        check("a tooling claim's agent is `worker`", (tooling["kind"], tooling["agent"], tooling["dry_run"]),
              ("tooling", "worker", True))
        check("an unknown kind is refused",
              _raises(lambda: claim("Pl/bad", seed_main, "w", True, cwd=seed_main, kind="not-a-kind")), True)
        check("claim seeds the toolchain as a copy",
              (os.path.exists(os.path.join(seed_wt, "build", "tools", "dtk.exe")),
               _is_reparse_point(os.path.join(seed_wt, "build", "compilers"))), (True, False))
        check("claim reports the seed", out.get("seeded", "").startswith("seeded"), True)
        check("the claim cut a worktree on its own branch", repo_git(seed_wt, "rev-parse", "--abbrev-ref", "HEAD"),
              out["branch"])
        cluster = claim("cluster/net", seed_main, "w", False, cwd=seed_main, units=["Net/a.cpp", "Net/b"])
        check("a cluster claim records the units it holds", load_registry(seed_main)["cluster/net"]["units"],
              ["Net/a", "Net/b"])
        check("... and registry.claimed_units reads every one of them",
              {"Net/a", "Net/b", "cluster/net"} <= registry.claimed_units(load_registry(seed_main)), True)
        check("the cluster claim releases cleanly", release("cluster/net", seed_main, True, False,
                                                            probe=no_session)["complete"], True)
        rel = release("Pl/seeded", seed_main, force=True, dry_run=False, probe=no_session)
        check("the seeded claim releases cleanly (no stray claim)", rel.get("complete"), True)
        check("MAIN's original survives the teardown",
              open(os.path.join(seed_main, "orig", "RMHE08", "sys", "main.dol"), "rb").read(), b"dol")

    unit = "Pl/pl_act"
    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        check("ack_place_error accepts the claim's worktree and branch", ack_place_error(main, unit, wt), None)
        wrong_tree = ack_place_error(main, unit, main)
        check("ack from MAIN is refused, naming both trees",
              (wrong_tree is not None, wt.replace("\\", "/") in (wrong_tree or "").replace("\\", "/")), (True, True))
        repo_git(wt, "checkout", "-q", "-b", "throwaway-check")
        wrong_branch = ack_place_error(main, unit, wt)
        check("ack on the wrong branch is refused, naming both",
              (branch in (wrong_branch or ""), "throwaway-check" in (wrong_branch or "")), (True, True))
        check("ack() refuses from the wrong place", _raises(lambda: ack(unit, main, agent="w-a")), True)
        check("... and writes no heartbeat agent", load_ack(main, unit).get("agent"), None)

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        _worker_branch, worker_wt = claimed(main, "Pl/pl_act")
        check("a claim from MAIN on main passes the guard", claim_place_error(main, main), None)
        check("a claim from a worker's worktree is refused", claim_place_error(main, worker_wt) is not None, True)
        repo_git(main, "checkout", "-q", "-b", "throwaway-check")
        err = claim_place_error(main, main)
        check("a claim off main is refused, naming the branch", "throwaway-check" in (err or ""), True)
        check("claim refuses off main and creates nothing",
              (_raises(lambda: claim("Pl/pl_skill", main, "w-a", False, cwd=main)),
               "Pl/pl_skill" in load_registry(main), branch_exists(main, branch_for("Pl/pl_skill"))),
              (True, False, False))

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        tip = repo_git(main, "rev-parse", branch)
        first = release(unit, main, force=False, dry_run=False, probe=no_session)
        check("release: complete", first["complete"], True)
        check("release: the worktree and branch are gone", (os.path.isdir(wt), branch_exists(main, branch)), (False, False))
        check("release: the unmerged work is rescued", repo_git(main, "rev-parse", first["release_ref"]), tip)
        check("release: the registry entry and heartbeat are gone",
              (load_registry(main), os.path.exists(ack_path(main, unit))), ({}, False))
        second = release(unit, main, force=False, dry_run=False, probe=no_session)
        check("release twice: still complete, every step accounted for",
              (second["complete"], all(s["status"] in ("done", "skipped") for s in second["steps"])), (True, True))
        check("release twice: the worktree and branch steps are skips",
              (step_of(second, "worktree remove")["status"], step_of(second, "branch -D")["status"]), ("skipped", "skipped"))

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        shutil.rmtree(wt)
        out = release(unit, main, force=False, dry_run=False, probe=no_session)
        check("a worktree already gone is a clean release", out["complete"], True)
        check("... the skip says so, and the branch still goes",
              ("already gone" in step_of(out, "worktree remove")["why"], branch_exists(main, branch)), (True, False))

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit, worktree=False)
        repo_git(main, "branch", "-D", branch)
        os.makedirs(wt)
        open(os.path.join(wt, "junk.txt"), "w").close()
        out = release(unit, main, force=False, dry_run=False, probe=no_session)
        check("a branch already deleted is a clean release", out["complete"], True)
        check("... a leftover directory git no longer tracks is removed", os.path.isdir(wt), False)

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=False, probe=live_session)
        check("a live session refuses the release, naming it", (bool(out["refused"]), "sid-1" in (out["refused"] or "")),
              (True, True))
        check("... and touches nothing",
              (unit in load_registry(main), os.path.isdir(wt), branch_exists(main, branch), out["complete"]),
              (True, True, True, False))
        # the default probe reads the real session registry under CLAUDE_CONFIG_DIR
        reg_root = os.path.join(os.environ["CLAUDE_CONFIG_DIR"], sessions.SESSIONS_DIRNAME)
        os.makedirs(reg_root, exist_ok=True)
        rec = os.path.join(reg_root, "77.json")
        with open(rec, "w", encoding="utf-8") as fh:
            json.dump({"pid": os.getpid(), "sessionId": "sid-77", "cwd": wt, "name": "worker"}, fh)
        out = release(unit, main, force=False, dry_run=False)
        check("the default probe sees a live session in the worktree", "sid-77" in (out["refused"] or ""), True)
        os.remove(rec)
        check("... and once it is gone the release completes",
              release(unit, main, force=False, dry_run=False)["complete"], True)

    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=True, probe=no_session)
        check("a dry run is a plan that touches nothing",
              (out["complete"], os.path.isdir(wt), branch_exists(main, branch), unit in load_registry(main)),
              (None, True, True, True))
        os.remove(outbox_path(main, unit))
        out = release(unit, main, force=False, dry_run=False, probe=no_session)
        check("an unmerged claim with no outbox is refused, naming the rescue ref",
              (bool(out["refused"]), rescue_ref_name(unit) in (out["refused"] or "")), (True, True))
        tip = repo_git(main, "rev-parse", branch)
        forced = release(unit, main, force=True, dry_run=False, probe=no_session)
        check("... force releases it anyway, parking the work", (forced["complete"],
              repo_git(main, "rev-parse", forced["release_ref"])), (True, tip))
        check("... the cost names the restore command",
              "git branch %s refs/rescue/%s" % (branch, slug(unit)) in (forced["cost"] or ""), True)
        check("... and rescue_exists finds it", rescue_exists(main, unit), "refs/rescue/%s" % slug(unit))

    # the rescue ref's verdict is read where release creates it; redundant is pruned, the rest are kept
    def commit_in(path, msg):
        repo_git(path, "add", "-A")
        repo_git(path, "commit", "-q", "-m", msg)

    def write(root, rel, text):
        p = os.path.join(root, *rel.split("/"))
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)

    def apply_unit(root, unit_name, src=None):
        conf = open(os.path.join(root, "configure.py"), encoding="utf-8").read().replace(
            "\n]", '\n    Object(NonMatching, "%s.cpp"),\n]' % unit_name)
        write(root, "configure.py", conf)
        with open(os.path.join(root, "config", "RMHE08", "splits.txt"), "a", encoding="utf-8", newline="\n") as fh:
            fh.write("\n%s.cpp:\n\t.text       start:0x80001000 end:0x80001100\n" % unit_name)
        write(root, "src/%s.cpp" % unit_name, src or "int f(void) { return 0; }\n")

    def registered_main(tmp):
        m = new_repo(tmp)
        write(m, "configure.py", "config.libs = [\n]\n")
        write(m, "config/RMHE08/splits.txt", "Sections:\n\t.text       type:code align:32\n\n")
        commit_in(m, "the registration files")
        return m

    def claim_shell(m, unit_name):
        b, w = branch_for(unit_name), worktree_for(unit_name, m)
        repo_git(m, "worktree", "add", "-b", b, w)
        write(m, ".pi/outbox/%s.json" % slug(unit_name), "{}")
        reg = load_registry(m)
        reg[unit_name] = {"branch": b, "worktree": w}
        save_registry(m, reg)
        return b, w

    with testing.temp_dir() as tmp:
        main = registered_main(tmp)
        branch, wt = claim_shell(main, "red/red")
        write(wt, "src/red/red.cpp", "int f(void) { return 1; }\n")
        commit_in(wt, "the worker's work")
        apply_unit(main, "red/red", "int f(void) { return 1; }\n")
        commit_in(main, "land red/red")
        out = release("red/red", main, force=False, dry_run=False, probe=no_session)
        check("a redundant ref: parked, classified, pruned, said so",
              (out["release_ref"], out["rescue_verdict"]["verdict"], out["rescue_verdict"]["pruned"],
               "pruned %s" % rescue_ref_name("red/red") in step_of(out, "rescue ref audit")["why"],
               rescue_exists(main, "red/red")),
              (rescue_ref_name("red/red"), "redundant", True, True, None))
    with testing.temp_dir() as tmp:
        main = registered_main(tmp)
        branch, wt = claim_shell(main, "unl/unl")
        apply_unit(wt, "unl/unl")
        commit_in(wt, "the worker's work")
        out = release("unl/unl", main, force=False, dry_run=False, probe=no_session)
        check("an unlanded ref: kept and surfaced loudly",
              (out["rescue_verdict"]["verdict"], out["rescue_verdict"]["pruned"], rescue_exists(main, "unl/unl"),
               "UNLANDED WORK" in step_of(out, "rescue ref audit")["why"], out["complete"]),
              ("unlanded", False, rescue_ref_name("unl/unl"), True, True))
    with testing.temp_dir() as tmp:
        main = registered_main(tmp)
        branch, wt = claim_shell(main, "clean/clean")
        apply_unit(wt, "clean/clean")
        commit_in(wt, "the worker's work")
        repo_git(main, "cherry-pick", branch)
        out = release("clean/clean", main, force=False, dry_run=False, probe=no_session)
        check("a clean landing parks no ref and invents no verdict",
              (out["release_ref"], out["rescue_verdict"], step_of(out, "rescue ref audit")["why"], out["complete"]),
              (None, None, "no rescue ref: nothing was parked", True))

    # the sweep: only merged claims; a live session is reported rather than forced
    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        done_branch, _ = claimed(main, "Pl/pl_act")
        repo_git(main, "cherry-pick", done_branch)
        live_branch, _ = claimed(main, "Pl/pl_skill")
        repo_git(main, "cherry-pick", live_branch)
        open_branch, open_wt = claimed(main, "Pl/pl_master")
        dirty_branch, dirty_wt = claimed(main, "Pl/pl_misc")
        repo_git(main, "cherry-pick", dirty_branch)
        os.remove(outbox_path(main, "Pl/pl_misc"))
        open(os.path.join(dirty_wt, "uncommitted.c"), "w").close()
        swept = release_merged(main, probe=lambda row: live_session(row) if "ws-pl-skill" in row.get("worktree", "")
                               else no_session(row))
        check("the sweep releases the merged claim", [r["unit"] for r in swept["released"]], ["Pl/pl_act"])
        check("... refuses the live one", [r["unit"] for r in swept["refused"]], ["Pl/pl_skill"])
        check("... skips the unmerged one and the dirty one", [r["unit"] for r in swept["skipped"]],
              ["Pl/pl_master", "Pl/pl_misc"])
        check("... and reports itself incomplete, keeping what it skipped",
              (swept["complete"], branch_exists(main, done_branch), branch_exists(main, live_branch),
               os.path.isdir(open_wt)), (False, False, True, True))

    # a branch selector: one unit can carry two claims and the registry records only one of them
    with testing.temp_dir() as tmp:
        main = new_repo(tmp)
        old_branch, _old_wt = claimed(main, unit)
        second_branch = "worker/8033f270-second-claim"
        second_wt = os.path.join(os.path.dirname(main), os.path.basename(main) + ".ws-second")
        repo_git(main, "worktree", "add", "-b", second_branch, second_wt)
        repo_commit(second_wt, "the second claim's work")
        with open(os.path.join(main, ".pi", "outbox", slug_of_branch(second_branch) + ".json"), "w") as fh:
            json.dump({"unit": unit}, fh)
        out = release("", main, force=False, dry_run=True, probe=no_session, branch=second_branch)
        check("a branch selector resolves that branch and its outbox", (out["branch"], out["outbox"]),
              (second_branch, True))
        done = release("", main, force=False, dry_run=False, probe=no_session, branch=second_branch)
        check("... tears only that branch down", (done["complete"], branch_exists(main, second_branch),
              branch_exists(main, old_branch), unit in load_registry(main)), (True, False, True, True))
        out = release("", main, force=False, dry_run=False, probe=no_session, branch=old_branch)
        check("a branch selector on a registered claim releases that unit and its row", (out["unit"], load_registry(main)),
              (unit, {}))

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --- CLI ----------------------------------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    c = sub.add_parser("claim", help="reserve a unit (worktree + branch)")
    c.add_argument("unit")
    c.add_argument("--worker", default=None)
    c.add_argument("--dry-run", action="store_true")
    c.add_argument("--json", action="store_true")
    c.add_argument("--no-slots", action="store_true",
                   help="construct a throwaway worktree instead of taking a slot (the pre-pool path)")
    c.add_argument("--slot", type=int, default=None, help="take this slot instead of the first free one")
    c.add_argument("--kind", default="unit",
                   help="the lane kind; the agent profile comes from it (`lib.lanes.launch.KIND_PROFILE`: "
                        "unit->surveyor, fix->fixer, merge->merger, tooling/docs->worker, ...). "
                        "Recorded on the slot's lock and in the registry (default: unit)")
    lp = sub.add_parser("list", help="every claim git knows about")
    lp.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="remove the worktree and the branch (idempotent, total)")
    r.add_argument("unit", nargs="?", default=None, help="the unit to release; omit with --all-merged")
    r.add_argument("--branch", default=None,
                   help="release the claim holding this branch instead of the one the registry records for the unit")
    r.add_argument("--all-merged", action="store_true", dest="all_merged",
                   help="sweep every claim whose branch is already merged into main")
    r.add_argument("--force", action="store_true",
                   help="release even with no outbox and an unmerged branch (the work is rescued first)")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--json", action="store_true")
    a = sub.add_parser("ack", help="a worker's heartbeat: call this first, then after each iteration")
    a.add_argument("unit")
    a.add_argument("--agent", default=None, help="your name")
    a.add_argument("--progress", default=None, help="what you just finished (a symbol, usually)")
    a.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="unacked / stalled / working / done, per claim")
    s.add_argument("--ack-seconds", type=float, default=120)
    s.add_argument("--stall-minutes", type=float, default=20)
    s.add_argument("--json", action="store_true")
    t = sub.add_parser("timeout", help="reclaim what a silent worker holds (rescues its commits first)")
    t.add_argument("unit", nargs="?", default=None)
    t.add_argument("--ack-seconds", type=float, default=120)
    t.add_argument("--stall-minutes", type=float, default=20)
    t.add_argument("--apply", action="store_true", help="act; without it, only report")
    e = sub.add_parser("expire", help="claims older than N minutes with no outbox")
    e.add_argument("--minutes", type=int, default=120)
    e.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0

    main_wt = registry.main_of()
    if args.cmd == "claim":
        out = claim(args.unit, main_wt, args.worker, args.dry_run,
                    slots_mode=(False if args.no_slots else None), slot=args.slot, kind=args.kind)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        claim_slug_ = slug_of_branch(out.get("branch") or "") or slug(out["unit"])
        brief_path = os.path.join(main_wt, "tools", "units", "briefs", claim_slug_ + ".md")
        if out.get("dry_run"):
            print("would run: %s" % out["command"])
        else:
            print("claimed %s\n  branch   %s\n  worktree %s%s\n  base     %s\n  seeded   %s"
                  % (out["unit"], out["branch"], out["worktree"],
                     " (slot %s)" % out["slot"] if out.get("slot") else "", out["base"], out.get("seeded", "")))
            try:
                from tools.units import queue as queue_mod
                queue_mod.promote(main_wt, out["unit"], claim_slug_, out["worktree"])
                print("\n  brief    %s" % brief_path)
                print("  to replace part 5: python tools/units/brief.py %s --out %s --task \"...\""
                      % (out["unit"], brief_path))
            except Exception as exc:  # a brief failure must never lose the claim
                print("\n  brief    NOT WRITTEN (%s)" % exc)
                print("  write it before launching: python tools/units/brief.py %s --out %s" % (out["unit"], brief_path))
        try:
            from tools.units import queue as queue_mod
            sp = queue_mod.spawn_line(main_wt, out["unit"], claim_slug_, out["worktree"], brief_path, kind=args.kind)
        except SystemExit as exc:
            print("\nspawn line REFUSED: %s" % exc)
            return 0
        print("\nspawn this worker (the cwd is the claim's worktree, never MAIN):")
        print("  agent: %s" % sp["agent"])
        print("  label: %s  (the tool takes no name)" % sp["name"])
        print("  cwd:   %s" % sp["cwd"])
        print("  task:  %s" % sp["task"])
        print("\n%s" % sp["call"])
        return 0
    if args.cmd == "list":
        rows = claims_view(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no active claims")
            return 0
        print("%-34s %-30s %-10s %-7s %-7s %-7s %s" % ("unit", "branch", "worker", "merged", "outbox", "acked", "claimed"))
        for row in rows:
            print("%-34s %-30s %-10s %-7s %-7s %-7s %s"
                  % (row["unit"][:34], (row["branch"] or "")[:30], (row.get("worker") or "")[:10],
                     row["merged"], row["outbox"], row.get("acked", False), row.get("claimed_at") or ""))
        stranded = [r for r in rows if not r.get("acked", False)]
        if stranded:
            print()
            print("%d claim(s) below have NO ack file - no worker may ever have been launched for them." % len(stranded))
            print("Check `python tools/units/claims.py status` (it reports them as `unacked`), then either")
            print("launch the spawn line from the brief or release the claim: %s"
                  % ", ".join(r["unit"] for r in stranded[:6]))
        return 0
    if args.cmd == "release":
        if args.all_merged and args.branch:
            print("--branch names one claim; --all-merged sweeps every finished one - pass one or the other")
            return 2
        if args.all_merged:
            out = release_merged(main_wt, args.dry_run)
            if args.json:
                print(json.dumps({**out, "released": ["%s (%s)" % (r["unit"], r["branch"]) for r in out["released"]]},
                                 indent=2))
            else:
                for result in out["released"]:
                    done = sum(1 for st in result["steps"] if st["status"] == "done")
                    skipped = sum(1 for st in result["steps"] if st["status"] == "skipped")
                    print("%s %s (branch %s) - %d done, %d skipped"
                          % ("would release" if args.dry_run else "released", result["unit"], result["branch"],
                             done, skipped))
                for row in out["skipped"]:
                    print("skip      %s - %s" % (row["unit"], row["why"]))
                for row in out["refused"]:
                    print("REFUSED   %s - %s" % (row["unit"], row["why"].replace("\n  (", " (")))
                print("%d released, %d skipped, %d refused%s"
                      % (len(out["released"]), len(out["skipped"]), len(out["refused"]),
                         "" if out["complete"] else " - the teardown is INCOMPLETE"))
            return 0 if out["complete"] else 1
        if not args.unit and not args.branch:
            print("release needs a <unit>, --branch <name>, or --all-merged to sweep")
            return 2
        out = release(args.unit or "", main_wt, args.force, args.dry_run, branch=args.branch)
        if args.json:
            print(json.dumps({k: v for k, v in out.items() if k != "steps"} | {"steps": teardown.public(out["steps"])},
                             indent=2))
            return 0 if (out["complete"] or out["dry_run"]) and not out.get("refused") else 1
        if out.get("refused"):
            print("REFUSED: %s" % out["refused"])
            return 1
        for step in out["steps"]:
            mark = {"done": "done  ", "skipped": "skip  ", "planned": "would ", "failed": "FAILED"}.get(step["status"],
                                                                                                    step["status"])
            print("  %s %s%s" % (mark, step["label"], (" - " + step["why"]) if step.get("why") else ""))
        if out.get("cost"):
            print("COST: %s" % out["cost"])
        if out["dry_run"]:
            print("would release %s (branch %s, merged=%s, outbox=%s)" % (out["unit"], out["branch"], out["merged"],
                                                                         out["outbox"]))
            return 0
        print("released %s (branch %s, merged=%s, outbox=%s)%s"
              % (out["unit"], out["branch"], out["merged"], out["outbox"],
                 "" if out["complete"] else " - the teardown is INCOMPLETE, the claim is kept"))
        return 0 if out["complete"] else 1
    if args.cmd == "ack":
        out = ack(args.unit, main_wt, args.agent, args.progress)
        print(json.dumps(out, indent=2) if args.json
              else "ack %s: %d iteration(s), agent=%s" % (out["unit"], out.get("iterations", 0), out.get("agent")))
        return 0
    if args.cmd == "status":
        rows = claim_status(main_wt, args.ack_seconds, args.stall_minutes)
        if args.json:
            print(json.dumps(rows, indent=2))
        else:
            print("%-16s %-9s %8s %8s %8s %-8s %-20s %s"
                  % ("unit", "state", "acked", "progress", "commits", "outbox", "session", "agent"))
            for row in rows:
                print("%-16s %-9s %8s %8s %8d %-8s %-20s %s"
                      % (row["unit"][:16], row["state"],
                         row["acked_seconds_ago"] if row["acked_seconds_ago"] is not None else "-",
                         row["progress_seconds_ago"] if row["progress_seconds_ago"] is not None else "-",
                         row["commits"], str(row["outbox"]), (row.get("session") or "-")[:20], row["agent"] or "?"))
                if row.get("session_note"):
                    print("%-16s   kept: %s" % ("", row["session_note"]))
            unhealthy = [r for r in rows if r["state"] in ("unacked", "stalled")]
            if unhealthy:
                print("\n%d unhealthy claim(s) - reclaim with: python tools/units/claims.py timeout --apply"
                      % len(unhealthy))
                return 1
        return 0 if all(r["state"] not in ("unacked", "stalled") for r in rows) else (0 if args.json else 1)
    if args.cmd == "timeout":
        rows = timeout(main_wt, args.ack_seconds, args.stall_minutes, args.apply, args.unit)
        for row in rows:
            print("%-16s %-9s commits=%d%s" % (row["unit"][:16], row["state"], row["commits"],
                                               "  rescue: %s" % row["rescue"] if row["rescue"] else ""))
            for step in row["steps"]:
                print("   %s %s" % ("ran " if args.apply else "would run", step))
            if row.get("error"):
                print("   STOPPED: %s" % row["error"])
                print("   the claim is kept; fix the step (or wait for the live session to exit) and run again")
        print("%d claim(s) %s" % (len(rows), "reclaimed" if args.apply else "(dry run: pass --apply)"))
        return 0
    if args.cmd == "expire":
        rows = expire(main_wt, args.minutes, args.apply)
        for row in rows:
            note = ""
            if args.apply:
                note = "  released" if row.get("released") else "  KEPT: %s" % (row.get("error") or "incomplete")
            print("%-34s %-30s %.1f min  outbox=%s%s"
                  % (row["unit"], row["branch"], row["age_minutes"], row["outbox"], note))
        released = sum(1 for row in rows if row.get("released"))
        print("%d stale claim(s)%s" % (len(rows), " (%d released, %d kept)" % (released, len(rows) - released)
                                        if args.apply else " (dry run: pass --apply)"))
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
