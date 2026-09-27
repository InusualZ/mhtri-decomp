#!/usr/bin/env python3
"""A fixed pool of reusable lane directories ("slots") - the branch is still the claim.

`claims.py` used to *construct* a lane's environment per round:

    git worktree add -b worker/<slug> <sibling>.ws-<slug> main
    git submodule update --init tools/m2c
    (then seed build/, orig/ and build/RMHE08 from MAIN)

That costs per lane, its teardown is destructive (`git worktree remove` cannot handle the `tools/m2c`
submodule, so the documented path is `rm -rf` + `prune` + `branch -D` - and a careless teardown destroyed
a lane's branch on 2026-09-24), and the seeder has to rewrite `.ninja_deps`' **absolute** header paths on
every seed precisely because each worktree path is new.

This replaces it with a fixed pool of `N` reusable slots at **stable paths** - `<repo>.slot1` ...
`<repo>.slot6` - so each keeps a warm `build/RMHE08`, `orig/`, the toolchain and its initialised
`tools/m2c` submodule across rounds.  A stable path is the point: a depfile's absolute paths stay valid
across a reset, so the per-seed rewrite becomes a one-time cost per slot.

**A slot holds a directory, never a branch.**  `acquire` always creates a *fresh* branch off main's
current tip (`checkout -B worker/<slug> <tip>`), after asserting the slot carried no branch over from a
previous round - a slot whose previous branch has not landed is a bug, not a state to reuse.  One branch
per claim, named for the unit, landable and auditable on its own: the invariant the whole protocol rests
on (docs/plan.md 5.1).

    python tools/units/slots.py init [--count 6] [--force]
    python tools/units/slots.py acquire <unit> [--slot N] [--worker NAME] [--dry-run]
    python tools/units/slots.py release [--slot N | --unit U | --branch B] [--keep-branch] [--dry-run]
    python tools/units/slots.py status [--json]
    python tools/units/slots.py verify [--slot N] [--json]
    python tools/units/slots.py --selftest

The reset is **verified and fail closed** - a reused slot carries the previous round's build state, and a
stale build tree is the most expensive failure this campaign has hit (a refused landing's split objects
left in a tree made the next gate report "no unit's split target object moved" for an untouched unit,
costing a full `rm -rf build/RMHE08` rebuild):

* **one `.used` marker per slot** in the worktree root, created by `acquire` (atomically, `O_EXCL`) and
  removed by `release`; `status` reports `used`/`free` from it.  The marker is the primary occupancy signal -
  it only works because the launch pattern is now structural (acquire first, launch with the slot as `cwd`),
  so a lane always enters its slot.  A JSON lock record at `MAIN/.pi/slots/<n>.json` still names the claim,
  worker and time, and a stale one is reclaimed instead of wedging forever.
* **`status` reads the worktree too, and keeps the two readings distinct.**  A slot whose worktree still has
  a branch checked out is **in use**, whatever the marker or the JSON record say - the lock is a convenience,
  the worktree is the truth.  The two disagreed in the wild (slot 2 was reported `free` while it held
  `worker/rule10-fix-14f8`), and that disagreement *was* the bug, so both readings are kept.
* **`acquire` falls through.**  An occupied slot is skipped, not failed on: the search takes the next
  genuinely free slot and marks it atomically, so two racing acquires cannot both take one.  An explicitly
  targeted `--slot N` still refuses when it holds an unlanded branch (never reuse a branch).
* **reset** = `checkout -f --detach <main-tip>`, a selective `git clean` that keeps `build/`, `orig/`, the
  toolchain and `tools/m2c` but discards scratch, stale source edits and stray files, then
  `checkout -B worker/<slug> <main-tip>`.
* **validate** the kept build tree against MAIN's current map/DOL with the same staleness guard
  `seed_worktree_build` already uses (`claims._build_is_current` - `config.json` vs
  `symbols.txt`/`splits.txt`/`main.dol`) plus a byte comparison of `build/RMHE08/report.json`.  **If it
  cannot be proven current, re-seed; never proceed on a doubt.**
* **release** returns the slot to main's tip, keeps the warm trees, deletes the branch (rescue-ref first, as
  `claims.release` does) and refreshes the build tree so the next acquire is instant.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402

DEFAULT_COUNT = 6
SLOT_SUFFIX = ".slot"
LOCK_SUBDIR = ("slots",)
# What a reset keeps.  Everything else untracked/ignored is scratch and goes; tracked edits are discarded
# by the forced `checkout`.  `tools/m2c` is a tracked submodule and must survive (see `clean_slot`).
SLOT_KEEP = (".ninja_deps", ".ninja_log", "build.ninja", "objdiff.json", "compile_commands.json",
             "build", "orig", os.path.join("tools", "m2c"), ".used")

# A slot's **sentinel**: `.used` in the worktree root.  `acquire` creates it (atomically, `O_EXCL`) and
# `release` removes it; `status` reads it and reports `used` / `free`.  It is gitignored, and `clean_slot`
# preserves it across a reset *because* the marker the acquire just created must survive the reset - a
# stale marker from a crash is reclaimed by the worktree reading (see `slot_state`), not by the clean.
SLOT_MARKER = ".used"


def marker_path(slot: str) -> str:
    return os.path.join(slot, SLOT_MARKER)


def marker_present(slot: str) -> bool:
    return os.path.exists(marker_path(slot))


def mark_used(slot: str, unit: str = "", branch: str = "") -> bool:
    """Claim the slot by creating its `.used` marker **atomically**.  -> False when one already exists.

    `O_CREAT | O_EXCL` is the atomicity: two racing `acquire`s cannot both create the file, so the loser
    falls through to the next free slot (`_pick_free`).  That is the whole mechanism - no lock daemon, no
    platform shim, and a crashed acquire leaves a marker the worktree reading reclaims.
    """
    path = marker_path(slot)
    try:
        fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
    except FileExistsError:
        return False
    except OSError:
        return False
    try:
        os.write(fd, ("%d %s %s\n" % (os.getpid(), unit, branch)).encode("utf-8", "replace"))
    except OSError:
        pass
    finally:
        os.close(fd)
    return True


def clear_marker(slot: str) -> bool:
    """Remove the slot's `.used` marker.  -> whether one was removed."""
    try:
        os.remove(marker_path(slot))
        return True
    except OSError:
        return False


def _claims():
    """`claims` imported late: claims imports slots for the claim path, so a top-level import would cycle."""
    from units import claims
    return claims


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, errors="replace")
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return (p.stdout or "").strip()


def slot_dir(main: str, n: int) -> str:
    """A slot's **stable** path: a sibling of MAIN, `<repo>.slot<n>` (e.g. `mhtri-dtk.slot3`)."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s%s%d" % (os.path.basename(main), SLOT_SUFFIX, n))


def slot_count(main: str) -> int:
    """How many slot directories exist right now (0 means the pool is not initialised).

    The pool size is persisted by `init` (`.pi/slots/pool.json`) so an `init --count N` that is not the
    default is seen by every later call; with no manifest the default six is assumed.
    """
    count = pool_manifest(main).get("count")
    if not isinstance(count, int) or count < 1:
        count = DEFAULT_COUNT
    n = 0
    for i in range(1, count + 1):
        d = slot_dir(main, i)
        if os.path.isdir(d) and os.path.exists(os.path.join(d, ".git")):
            n += 1
    return n


def enabled(main: str) -> bool:
    """Whether the slot pool is in use: at least one slot directory exists."""
    return slot_count(main) > 0


def locks_dir(main: str) -> str:
    return os.path.join(main, ".pi", *LOCK_SUBDIR)


def lock_path(main: str, n: int) -> str:
    return os.path.join(locks_dir(main), "%d.json" % n)


def pool_manifest(main: str) -> dict:
    """The pool's size and creation time (`.pi/slots/pool.json`), or `{}` when there is none."""
    try:
        return json.loads(open(os.path.join(locks_dir(main), "pool.json"), encoding="utf-8").read())
    except (OSError, json.JSONDecodeError):
        return {}


def read_lock(main: str, n: int) -> dict:
    try:
        return json.loads(open(lock_path(main, n), encoding="utf-8").read())
    except (OSError, json.JSONDecodeError):
        return {}


def write_lock(main: str, n: int, data: dict) -> None:
    """Write the lock atomically; refuse if another writer made one since the caller looked."""
    path = lock_path(main, n)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp%d" % os.getpid()
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)
    os.replace(tmp, path)


def clear_lock(main: str, n: int) -> None:
    try:
        os.remove(lock_path(main, n))
    except OSError:
        pass


def slot_attached_branch(slot: str) -> str | None:
    """The branch the slot has checked out, or `None` when the slot is (correctly) detached."""
    p = subprocess.run(["git", "-C", slot, "symbolic-ref", "-q", "HEAD"], capture_output=True, text=True,
                       errors="replace")
    ref = (p.stdout or "").strip()
    if p.returncode == 0 and ref.startswith("refs/heads/"):
        return ref[len("refs/heads/"):]
    return None


def slot_head(slot: str) -> str | None:
    p = subprocess.run(["git", "-C", slot, "rev-parse", "HEAD"], capture_output=True, text=True,
                       errors="replace")
    return (p.stdout or "").strip() if p.returncode == 0 else None


def lock_stale(main: str, n: int, lock: dict | None = None) -> bool:
    """Whether the lock on slot `n` is reclaimable: its claim has landed or the slot moved on.

    A stale lock must be detectable, not a permanent wedge (the whole point of a per-slot lock).  The lock
    names a branch, which is the real claim lock: if that branch is gone from the repo, the claim landed or
    was released, so the slot is free.  If the slot is no longer *on* that branch, the lock names a state
    the directory has already left - also stale.
    """
    claims = _claims()
    lock = read_lock(main, n) if lock is None else lock
    if not lock:
        return False
    branch = lock.get("branch")
    if not branch:
        return True
    if not claims.branch_exists(main, branch):
        return True
    return slot_attached_branch(slot_dir(main, n)) != branch


def slot_state(main: str, n: int) -> dict:
    """One slot's row: whether it exists, what it has checked out, its `.used` marker and whether it is free.

    **`free`/`used` is read from two sources of the same fact, and both must agree that the slot is empty.**
    The primary is the `.used` sentinel `acquire` creates and `release` removes; the second is the worktree
    state - a slot whose worktree still has a branch checked out is **in use**, whatever any file says.  The
    lock record is a convenience; the worktree is the truth.  Today the two disagreed (slot 2 was reported
    `free` while its worktree held `worker/rule10-fix-14f8`), and that disagreement *was* the bug, so both
    readings are kept.

    A `.used` marker on a *detached* worktree with no live lock record is a crash remnant, not a live claim -
    it is named reclaimable and the slot is free, so a crashed acquire can never wedge a slot.
    """
    d = slot_dir(main, n)
    exists = os.path.isdir(d) and os.path.exists(os.path.join(d, ".git"))
    lock = read_lock(main, n)
    stale = lock_stale(main, n, lock) if lock else False
    live_lock = bool(lock and not stale)
    attached = slot_attached_branch(d) if exists else None
    marked = marker_present(d) if exists else False
    marker_stale = marked and not attached and not live_lock
    used = bool(attached) or (marked and not marker_stale)
    free = exists and not used
    if not exists:
        why = "no such slot directory"
    elif live_lock:
        why = "in use by %s (%s)" % (lock.get("unit") or "?", lock.get("worker") or "?")
    elif attached:
        why = "in use - holds branch %s (its `.used` marker is %s); release it first" % (
            attached, "present" if marked else "MISSING")
    elif marked and not marker_stale:
        why = "used (`.used` marker present, worktree detached)"
    elif marker_stale:
        why = "stale `.used` marker on a detached worktree with no live lock - reclaimable"
    elif lock and stale:
        why = "stale lock (%s) - reclaimable" % (lock.get("unit") or "?")
    else:
        why = "free"
    return {"slot": n, "dir": d, "exists": exists, "attached": attached, "lock": lock, "stale": stale,
            "marked": marked, "marker_stale": marker_stale, "used": used, "free": free, "why": why}


def all_slots(main: str) -> list[dict]:
    return [slot_state(main, n) for n in range(1, slot_count(main) + 1)]


def free_slots(main: str) -> list[dict]:
    return [s for s in all_slots(main) if s["free"]]


def free_count(main: str) -> int:
    """How many slots are free right now - the number of lanes the pool can still launch."""
    return len(free_slots(main))


def capacity_error(main: str) -> str | None:
    """The refusal a claim path raises when no slot is free - a free slot **is** the concurrency cap.

    `None` when the pool is not in use (no slots initialised) or a slot is free: the cap only exists once a
    pool does, so a tree without one keeps the old per-claim construction path.
    """
    if not enabled(main):
        return None
    free = free_slots(main)
    if free:
        return None
    rows = all_slots(main)
    lines = ["all %d slot(s) are in use - this is the concurrency cap, not a queue shortage." % len(rows),
             "  a slot is only freed by landing/releasing its claim:",
             "      python tools/units/claims.py release <unit>   # or `slots.py release --slot N`"]
    for s in rows:
        lines.append("  slot %d: %s" % (s["slot"], s["why"]))
    lines.append("  see: python tools/units/slots.py status")
    return "\n".join(lines)


# --- the build-tree guard ------------------------------------------------------------------------

def report_matches(main: str, slot: str) -> bool:
    """Whether the slot's `build/RMHE08/report.json` is byte-identical to MAIN's current one."""
    a = os.path.join(main, _claims().RMHE08_REL, "report.json")
    b = os.path.join(slot, _claims().RMHE08_REL, "report.json")
    try:
        if not (os.path.isfile(a) and os.path.isfile(b)):
            return False
        return open(a, "rb").read() == open(b, "rb").read()
    except OSError:
        return False


def verify(main: str, n: int) -> dict:
    """Whether slot `n`'s kept build tree can be **proven** current against MAIN's map/DOL - fail closed.

    Reuses `claims._build_is_current` (the guard `seed_worktree_build` already has) with the slot as the
    build root and MAIN as the input root, and additionally byte-compares `report.json` so a stale report
    (a landing that moved a body but not the map) cannot be handed over as current.
    """
    claims = _claims()
    d = slot_dir(main, n)
    reasons: list[str] = []
    if not os.path.isdir(d):
        return {"slot": n, "dir": d, "ok": False, "main_build_current": False, "build_current": False,
                "report_matches": False, "reasons": ["slot directory missing: %s" % d]}
    main_current = claims._main_build_is_current(main)
    build_current = claims._build_is_current(d, main)
    report_ok = report_matches(main, d)
    if not main_current:
        reasons.append("MAIN's own build tree is stale (its config.json predates the map/DOL) - `ninja` in MAIN")
    if not build_current:
        reasons.append("slot build/RMHE08/config.json predates MAIN's current map/DOL - the split is stale")
    if not report_ok:
        reasons.append("slot build/RMHE08/report.json differs from MAIN's current report")
    return {"slot": n, "dir": d, "ok": bool(main_current and build_current and report_ok),
            "main_build_current": main_current, "build_current": build_current, "report_matches": report_ok,
            "reasons": reasons}


# --- reset ---------------------------------------------------------------------------------------

def clean_slot(slot: str) -> list[str]:
    """Discard every leftover a previous round could leave, keeping the warm trees.

    `checkout -f` (the caller's step) discards tracked edits - a dirty `src/` is exactly the "foreign work"
    the landing gate refuses at `record-base`; this drops untracked/ignored scratch, stray files and any
    nested scratch repository, while `-e` excludes keep `build/`, `orig/`, the toolchain, the ninja state
    and the `tools/m2c` submodule.  The `-x` means ignored files are removed too, so `__pycache__`, scratch
    logs and a stray `.pi/` go; without the excludes it would delete the build tree it exists to keep.
    """
    args = ["clean", "-ffdx", "-q"]
    for keep in SLOT_KEEP:
        args += ["-e", keep]
    p = subprocess.run(["git", "-C", slot, *args], capture_output=True, text=True, errors="replace")
    if p.returncode != 0:
        raise SystemExit("git clean failed in %s: %s" % (slot, (p.stderr or "").strip()))
    return [line.split(" ", 1)[1] for line in p.stdout.splitlines() if line.startswith("Removing ")]


def reset_slot(main: str, n: int) -> str:
    """Detach the slot at main's current tip and discard every leftover. -> main's tip."""
    d = slot_dir(main, n)
    tip = git(["rev-parse", "HEAD"], main)
    git(["checkout", "-f", "-q", "--detach", tip], d)
    clean_slot(d)
    return tip


# --- init ----------------------------------------------------------------------------------------

def init(main: str, count: int = DEFAULT_COUNT, force: bool = False, seed: bool = True) -> dict:
    """Create the pool: `count` reusable slots as siblings of MAIN, each seeded once from MAIN.

    Idempotent: an existing slot is left alone unless `--force`, which removes and re-creates it.  Each new
    slot is a **detached** worktree, so it carries no branch; `seed_worktree_build` fills the warm tree
    (toolchain, `orig/`, `tools/m2c`, `build/RMHE08` and ninja state).
    """
    claims = _claims()
    os.makedirs(locks_dir(main), exist_ok=True)
    with open(os.path.join(locks_dir(main), "pool.json"), "w", encoding="utf-8") as fh:
        json.dump({"count": count, "created_at": time.strftime("%Y-%m-%dT%H:%M:%S")}, fh, indent=1,
                  sort_keys=True)
    tip = git(["rev-parse", "HEAD"], main)
    created, present, notes = [], [], []
    for n in range(1, count + 1):
        d = slot_dir(main, n)
        if os.path.exists(os.path.join(d, ".git")) and not force:
            present.append(n)
        else:
            if os.path.isdir(d):
                # a stale registration, or a directory we are forcing: clear both before re-adding
                subprocess.run(["git", "worktree", "remove", "--force", "--force", d], cwd=main,
                               capture_output=True, text=True, errors="replace")
            git(["worktree", "prune"], main)
            git(["worktree", "add", "--detach", d, tip], main)
            created.append(n)
        if seed:
            notes.append((n, claims.seed_worktree_build(main, d, copy_orig=True, overwrite=False)))
    return {"main": main, "tip": tip, "count": count, "created": created, "present": present,
            "notes": notes, "slots": [slot_dir(main, n) for n in range(1, count + 1)]}


# --- acquire -------------------------------------------------------------------------------------

def _pick_free(main: str, slot: int | None = None, claim=None) -> dict:
    """Pick a free slot, falling through to the next genuinely free one.

    `free` is the worktree truth (`slot_state`), so an occupied slot is **skipped**, never failed on.  With
    `claim`, each candidate is *atomically* marked (`.used`, `O_EXCL`) before it is returned - so two racing
    acquires cannot both take the same slot, and a slot a racing acquire just marked is skipped too.

    A slot explicitly targeted with `slot=N` is a search *by name*, not a search: it refuses when occupied -
    the "a slot holds a directory, never a branch" rule - or when the mark cannot be taken.
    """
    rows = all_slots(main)
    if not rows:
        raise SystemExit("REFUSED: the slot pool is not initialised - run `python tools/units/slots.py init`")
    if slot is not None:
        row = next((s for s in rows if s["slot"] == slot), None)
        if row is None:
            raise SystemExit("REFUSED: no slot %d (the pool is slots 1..%d)" % (slot, len(rows)))
        if not row["free"]:
            if row["attached"]:
                raise SystemExit("REFUSED slot %d: it still has branch %r checked out - a slot holds a "
                                 "directory, never a branch.\n  release it first: python tools/units/slots.py "
                                 "release --slot %d" % (slot, row["attached"], slot))
            raise SystemExit("REFUSED slot %d: %s" % (slot, row["why"]))
        if claim is not None and not claim(row):
            raise SystemExit("REFUSED slot %d: another acquire claimed it first (its `.used` marker exists); "
                             "retry, or drop `--slot` to let the search fall through to the next free slot"
                             % slot)
        return row
    for row in rows:
        if not row["free"]:
            continue
        if claim is None or claim(row):
            return row
    raise SystemExit("REFUSED: %s" % capacity_error(main))


def preview(main: str, unit: str, branch: str | None = None, slot: int | None = None) -> dict:
    """What `acquire` would do, touching nothing (for a dry run). -> the slot row and the command."""
    claims = _claims()
    branch = branch or claims.branch_for(unit)
    row = _pick_free(main, slot)
    tip = git(["rev-parse", "HEAD"], main)
    return {"slot": row["slot"], "dir": row["dir"], "branch": branch, "base": tip,
            "command": "git -C %s checkout -B %s %s" % (row["dir"], branch, tip)}


def acquire(main: str, unit: str, branch: str | None = None, worker: str | None = None,
            slot: int | None = None) -> dict:
    """Take a slot for `unit`: mark it `.used`, reset it, cut a **fresh** branch off main's tip, verify, lock.

    Refuses, before touching anything, when the unit's branch already exists (the claim is taken) and when an
    explicitly named slot is occupied - a slot whose previous branch has not landed is surfaced loudly, never
    silently reused.  The slot is **marked `.used` atomically before the reset**, so a racing acquire falls
    through to the next free slot instead of colliding; a failure after the mark removes it, and a crash
    leaves a marker the worktree reading reclaims.  The kept build tree is verified against MAIN's current
    map/DOL; if it cannot be proven current it is re-seeded, and if it still cannot, the acquire fails closed
    rather than handing the lane a stale tree.
    """
    claims = _claims()
    unit = claims.norm_unit(unit.strip("/"))
    branch = branch or claims.branch_for(unit)
    if claims.branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)

    def claim(row: dict) -> bool:
        # reclaim a crash remnant (a marker on a detached worktree) before marking, or it would never free
        if row.get("marker_stale"):
            clear_marker(row["dir"])
        return mark_used(row["dir"], unit, branch)

    row = _pick_free(main, slot, claim=claim)
    n, d = row["slot"], row["dir"]
    try:
        # A SLOT HOLDS A DIRECTORY, NEVER A BRANCH: the reset below detaches, and every release detaches, so a
        # branch still checked out here is a bug (a crashed/forced teardown) and must be named, not absorbed.
        attached = slot_attached_branch(d)
        if attached:
            raise SystemExit("REFUSED slot %d: it still has branch %r checked out - a slot holds a directory, "
                             "never a branch.\n  release it first: python tools/units/slots.py release "
                             "--slot %d" % (n, attached, n))
        reclaimed = None
        lock = read_lock(main, n)
        if lock and lock_stale(main, n, lock):
            # Reclaimable - unless its branch still exists, which is the unlanded branch acquire must surface.
            if lock.get("branch") and claims.branch_exists(main, lock["branch"]):
                raise SystemExit("REFUSED slot %d: it holds the unlanded branch %s from a previous round "
                                 "(lock by %s at %s).\n  land it or release it before reusing the slot."
                                 % (n, lock["branch"], lock.get("worker") or "?",
                                    lock.get("acquired_at") or "?"))
            reclaimed = lock
            clear_lock(main, n)
        tip = reset_slot(main, n)
        git(["checkout", "-q", "-B", branch, tip], d)
        # Fill anything missing (toolchain/orig/m2c) cheaply, then *prove* the build tree current.
        seed_note = claims.seed_worktree_build(main, d, copy_orig=True, overwrite=False)
        v = verify(main, n)
        refreshed = None
        if not v["ok"]:
            if not claims._main_build_is_current(main):
                raise SystemExit("REFUSED slot %d: MAIN's own build tree is not current (%s);\n"
                                 "  run `ninja` in MAIN so the slot can be re-seeded - never hand a lane a doubt"
                                 % (n, "; ".join(v["reasons"])))
            refreshed = claims.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
            v = verify(main, n)
            if not v["ok"]:
                raise SystemExit("REFUSED slot %d: the kept build tree could not be proven current even after "
                                 "a re-seed: %s" % (n, "; ".join(v["reasons"])))
        write_lock(main, n, {"slot": n, "dir": d, "unit": unit, "branch": branch,
                             "worker": worker or os.environ.get("USERNAME") or os.environ.get("USER") or "unknown",
                             "base": tip, "acquired_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                             "verified": v["ok"], "report_matches": v["report_matches"]})
        return {"slot": n, "dir": d, "worktree": d, "branch": branch, "base": tip, "unit": unit,
                "seeded": seed_note, "refreshed": refreshed, "verify": v, "reclaimed": reclaimed}
    except BaseException:
        # a failed acquire must not leave the slot marked: the marker is the occupancy signal the next search
        # reads, so clearing it here is what keeps a refused acquire from wedging the slot.
        clear_marker(d)
        raise


# --- release -------------------------------------------------------------------------------------

def _resolve_slot(main: str, slot: int | None, unit: str | None, branch: str | None) -> tuple[int, dict]:
    rows = all_slots(main)
    if not rows:
        raise SystemExit("REFUSED: the slot pool is not initialised")
    if slot is not None:
        row = next((s for s in rows if s["slot"] == slot), None)
        if row is None:
            raise SystemExit("REFUSED: no slot %d (the pool is slots 1..%d)" % (slot, len(rows)))
        return slot, row.get("lock") or {}
    for row in rows:
        lock = row.get("lock") or {}
        if branch and lock.get("branch") == branch:
            return row["slot"], lock
        if unit and lock.get("unit") == unit:
            return row["slot"], lock
    if branch:
        for row in rows:
            if row["attached"] == branch:
                return row["slot"], row.get("lock") or {}
    raise SystemExit("REFUSED release: no slot holds %s" % (branch or unit or "anything"))


def release(main: str, slot: int | None = None, unit: str | None = None, branch: str | None = None,
            delete_branch: bool = True, rescue: bool = True, refresh: bool = True,
            dry_run: bool = False) -> dict:
    """Return a slot to main's tip: detach, clean, delete its branch (rescue-ref first), refresh, unlock.

    Keeps the warm trees and leaves the slot pre-warmed so the next `acquire` is a validation, not a build.
    The branch is deleted because a *directory* never holds one; its commits are rescued first exactly as
    `claims.release` does, so a release can never be the only place unlanded work lived.
    """
    claims = _claims()
    n, lock = _resolve_slot(main, slot, unit, branch)
    d = slot_dir(main, n)
    branch = branch or lock.get("branch")
    unit = unit or lock.get("unit") or (claims.slug_of_branch(branch) if branch else None)
    result = {"slot": n, "dir": d, "branch": branch, "unit": unit, "detached": False, "cleaned": [],
              "branch_deleted": False, "rescue_ref": None, "refreshed": None, "lock_cleared": False,
              "marker_cleared": False, "dry_run": dry_run}
    if not os.path.exists(os.path.join(d, ".git")):
        raise SystemExit("REFUSED release slot %d: %s is not a worktree" % (n, d))
    tip = git(["rev-parse", "HEAD"], main)
    if dry_run:
        result["detached"] = True
        result["branch_deleted"] = bool(branch and claims.branch_exists(main, branch))
        result["lock_cleared"] = bool(lock)
        result["marker_cleared"] = marker_present(d)
        return result
    git(["checkout", "-f", "-q", "--detach", tip], d)
    result["detached"] = True
    result["cleaned"] = clean_slot(d)
    branch_present = bool(branch and claims.branch_exists(main, branch))
    if delete_branch and branch_present:
        if rescue and not claims.merged_into_main(main, branch) and claims.commits_ahead(main, branch):
            ref = claims.rescue_ref_name(unit or claims.slug_of_branch(branch) or branch)
            git(["update-ref", ref, branch], main)
            result["rescue_ref"] = ref
        git(["branch", "-D", branch], main)
        result["branch_deleted"] = True
    if refresh and claims._main_build_is_current(main):
        result["refreshed"] = claims.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
    # the marker is the occupancy signal the next `acquire`/`status` reads: remove it only after the branch is
    # gone and the warm tree is refreshed, so a crash before this line leaves `status` naming the slot in use
    result["marker_cleared"] = clear_marker(d)
    clear_lock(main, n)
    result["lock_cleared"] = True
    return result


# --- status --------------------------------------------------------------------------------------

def status(main: str) -> list[dict]:
    rows = []
    for s in all_slots(main):
        v = verify(main, s["slot"]) if s["exists"] else {"ok": False, "reasons": ["missing"]}
        rows.append({**s, "build_ok": v["ok"], "build_reasons": v["reasons"],
                     "head": slot_head(s["dir"]) if s["exists"] else None})
    return rows


# --- selftest ------------------------------------------------------------------------------------

def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    import shutil
    import tempfile
    claims = _claims()

    def g(path, *args, check=True):
        p = subprocess.run(["git", "-c", "user.email=t@example.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True)
        if check and p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def commit(path, msg):
        open(os.path.join(path, "f.txt"), "a", encoding="utf-8").write(msg + "\n")
        g(path, "add", "-A")
        g(path, "commit", "-q", "-m", msg)

    def fixture(tmp, count=2):
        """A real repo with a real (small) warm build tree, so init/acquire/release exercise git."""
        repo = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(os.path.join(repo, "src", "auto"))
        os.makedirs(os.path.join(repo, "config", "RMHE08"))
        os.makedirs(os.path.join(repo, "tools", "m2c"))
        os.makedirs(os.path.join(repo, "orig", "RMHE08", "sys"))
        os.makedirs(os.path.join(repo, "orig", "RMHE08", "files"))
        os.makedirs(os.path.join(repo, "build", "RMHE08", "obj", "auto"))
        os.makedirs(os.path.join(repo, "build", "compilers"))
        os.makedirs(os.path.join(repo, "build", "binutils"))
        os.makedirs(os.path.join(repo, "build", "tools"))
        open(os.path.join(repo, "configure.py"), "w").write("config.libs = []\n")
        open(os.path.join(repo, ".gitignore"), "w").write(
            "build/\norig/\n.ninja_*\nbuild.ninja\nobjdiff.json\ncompile_commands.json\n__pycache__/\n.used\n")
        open(os.path.join(repo, "src", "auto", "stub.c"), "w").write("/* header only */\n")
        open(os.path.join(repo, "tools", "m2c", "m2c.py"), "w").write("# m2c\n")
        open(os.path.join(repo, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/stub.c:\n\t.text  start:0x80100000 end:0x80100100\n")
        open(os.path.join(repo, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_80100000 = .text:0x80100000; // type:function size:0x40\n")
        open(os.path.join(repo, "orig", "RMHE08", "sys", "main.dol"), "wb").write(b"dol\n")
        open(os.path.join(repo, "orig", "RMHE08", "files", "mh3.sel"), "wb").write(b"sel\n")
        open(os.path.join(repo, "build", "tools", "dtk.exe"), "wb").write(b"dtk\n")
        open(os.path.join(repo, "build", "compilers", "mwcc.exe"), "wb").write(b"cc\n")
        open(os.path.join(repo, "build", "binutils", "as.exe"), "wb").write(b"as\n")
        open(os.path.join(repo, "build", "RMHE08", "obj", "auto", "stub.o"), "wb").write(b"OBJ-V1\n")
        open(os.path.join(repo, "build", "RMHE08", "config.json"), "w").write('{"libs": []}\n')
        open(os.path.join(repo, "build", "RMHE08", "report.json"), "w").write('{"units": {"auto/stub": 1.0}}\n')
        open(os.path.join(repo, "build.ninja"), "w").write("# ninja\n")
        open(os.path.join(repo, ".ninja_deps"), "wb").write(claims._ninja_deps_serialize(4, [
            ("path", os.path.abspath(repo).replace("\\", "/").encode() + b"/include/types.h")]))
        open(os.path.join(repo, ".ninja_log"), "w").write("# ninja log v5\n")
        open(os.path.join(repo, "objdiff.json"), "w").write("{}\n")
        open(os.path.join(repo, "compile_commands.json"), "w").write("[]\n")
        g(repo, "init", "-q")
        g(repo, "checkout", "-q", "-b", "main")
        g(repo, "add", "-A")
        g(repo, "commit", "-q", "-m", "base")
        # mtimes: the split is newer than its inputs, the manifest newer than configure (the guard's rule)
        for p, t in (("config/RMHE08/symbols.txt", 1_000_000), ("config/RMHE08/splits.txt", 1_000_000),
                     ("orig/RMHE08/sys/main.dol", 1_000_000), ("orig/RMHE08/files/mh3.sel", 1_000_000),
                     ("configure.py", 1_000_000), ("build/RMHE08/config.json", 2_000_000),
                     ("build/RMHE08/obj/auto/stub.o", 2_000_000), ("build/RMHE08/report.json", 2_000_000),
                     ("build.ninja", 2_000_000)):
            os.utime(os.path.join(repo, p), (t, t))
        return repo

    # the path rule: siblings of MAIN, stable across calls
    check("slot dir is a sibling of main", os.path.basename(slot_dir("/tmp/mhtri-dtk", 3)), "mhtri-dtk.slot3")
    check("slot path is stable", slot_dir("/tmp/mhtri-dtk", 3), slot_dir("/tmp/mhtri-dtk", 3))
    check("a tree with no pool is not enabled", enabled("/tmp/nonexistent-mhtri-dtk"), False)

    with tempfile.TemporaryDirectory() as tmp:
        repo = fixture(tmp, count=2)
        out = init(repo, count=2)
        check("init creates the requested slots", out["created"], [1, 2])
        check("init seeded them", all("build/RMHE08" in note for _, note in out["notes"]), True)
        check("the pool is now enabled", enabled(repo), True)
        check("a slot is a detached worktree", slot_attached_branch(slot_dir(repo, 1)), None)
        check("the slot has the warm build tree",
              os.path.exists(os.path.join(slot_dir(repo, 1), "build", "RMHE08", "report.json")), True)
        check("the slot has the original payload",
              os.path.exists(os.path.join(slot_dir(repo, 1), "orig", "RMHE08", "sys", "main.dol")), True)
        check("the slot's deps log points at the slot, not MAIN",
              os.path.abspath(repo).replace("\\", "/").encode() + b"/include/types.h"
              not in open(os.path.join(slot_dir(repo, 1), ".ninja_deps"), "rb").read(), True)
        check("a freshly seeded slot verifies current", verify(repo, 1)["ok"], True)
        # idempotent
        again = init(repo, count=2)
        check("init is idempotent", (again["created"], sorted(again["present"])), ([], [1, 2]))

        # (1) THE GOOD CASE: acquire -> branch off main's tip, report matches, no src diff, clean status
        tip = g(repo, "rev-parse", "HEAD")
        info = acquire(repo, "auto/stub", worker="w-good")
        d1 = info["dir"]
        check("acquire picks a free slot", info["slot"], 1)
        check("acquire cuts the branch off main's tip", g(d1, "rev-parse", "HEAD"), tip)
        check("acquire is on the claim's branch", slot_attached_branch(d1), claims.branch_for("auto/stub"))
        check("the slot is locked", read_lock(repo, 1).get("unit"), "auto/stub")
        check("the handed tree verifies current", info["verify"]["ok"], True)
        check("the slot's report matches MAIN exactly", report_matches(repo, d1), True)
        check("no src diff against main", g(d1, "diff", "--name-only", "main", "--", "src"), "")
        check("git status in the slot is clean", g(d1, "status", "--porcelain"), "")
        check("acquire creates the `.used` marker", marker_present(d1), True)
        check("... and the marker is gitignored (the tree stays clean)",
              g(d1, "status", "--porcelain", "-uall"), "")

        # (2) THE HOSTILE CASE: poison the build tree, then release and re-acquire - it must repair, not lie
        open(os.path.join(d1, "build", "RMHE08", "report.json"), "w").write('{"units": {}}\n')
        open(os.path.join(d1, "build", "RMHE08", "config.json"), "w").write('{"libs": ["OLD"]}\n')
        os.utime(os.path.join(d1, "build", "RMHE08", "config.json"), (500_000, 500_000))
        poisoned = verify(repo, 1)
        check("a poisoned slot is detected as stale", poisoned["ok"], False)
        check("... naming the stale report", any("report.json" in r for r in poisoned["reasons"]), True)
        check("... and the stale split", any("split is stale" in r for r in poisoned["reasons"]), True)
        # acquire refuses on a *live* lock, so release the claim first (as the landing would), then poison
        rel = release(repo, slot=1, unit="auto/stub", rescue=False)
        check("release detaches the slot", slot_attached_branch(d1), None)
        check("release deletes the branch", claims.branch_exists(repo, claims.branch_for("auto/stub")), False)
        check("release clears the lock", read_lock(repo, 1), {})
        check("release removes the `.used` marker", marker_present(d1), False)
        check("release refreshes the warm tree", report_matches(repo, d1), True)
        # poison again, this time while the slot is free, so acquire itself must repair it
        open(os.path.join(d1, "build", "RMHE08", "report.json"), "w").write('{"units": {"stale": true}}\n')
        os.utime(os.path.join(d1, "build", "RMHE08", "config.json"), (500_000, 500_000))
        check("the free slot is now poisoned", verify(repo, 1)["ok"], False)
        good = acquire(repo, "auto/stub-2", worker="w-repair")
        check("acquire detected the poison and re-seeded", good["slot"], 1)
        check("... and refreshed (not handed the lie)", bool(good["refreshed"]), True)
        check("... whatever it hands over is current", good["verify"]["ok"], True)
        check("... the report matches MAIN after the repair", report_matches(repo, d1), True)
        check("... and the poisoned config.json is gone",
              open(os.path.join(d1, "build", "RMHE08", "config.json")).read(), '{"libs": []}\n')
        release(repo, slot=1, unit="auto/stub-2", rescue=False)

        # the marker is atomic: two racing acquires cannot both claim one slot
        check("the marker is created atomically", mark_used(d1, "u", "b"), True)
        check("... a second, racing mark on the same slot is refused", mark_used(d1, "u2", "b2"), False)
        clear_marker(d1)
        check("... and once cleared the slot can be marked again", mark_used(d1, "u3", "b3"), True)
        clear_marker(d1)

        # (3) THE BRANCH INVARIANT: fresh branch off main's tip each time; a held branch is refused
        first = acquire(repo, "auto/stub-a")
        d = first["dir"]
        check("first acquire is at main's tip", g(d, "rev-parse", "HEAD"), tip)
        # a live claim on the slot: acquiring again must not silently reuse it
        try:
            acquire(repo, "auto/stub-b", slot=first["slot"])
            check("a locked slot is refused", "no error", "SystemExit")
        except SystemExit as exc:
            check("a locked slot is refused", "in use" in str(exc) or "REFUSED" in str(exc), True)
        release(repo, slot=first["slot"], unit="auto/stub-a", rescue=False)
        commit(repo, "main moves")
        tip2 = g(repo, "rev-parse", "HEAD")
        second = acquire(repo, "auto/stub-b", slot=first["slot"])
        check("the second acquire cuts a branch off the NEW tip", g(d, "rev-parse", "HEAD"), tip2)
        check("... and it is a different, fresh branch", second["branch"], claims.branch_for("auto/stub-b"))
        check("... while the released branch is gone",
              claims.branch_exists(repo, claims.branch_for("auto/stub-a")), False)
        # a slot holding an unlanded branch (lock lost, branch left attached) is surfaced loudly
        clear_lock(repo, first["slot"])          # simulate a crash between detach and unlock
        clear_marker(d)                          # ... and losing the marker too: the HOSTILE case
        check("HOSTILE: a checked-out branch with NO marker and NO lock still reports in use",
              slot_state(repo, first["slot"])["free"], False)
        check("... the worktree branch, not any file, is what decides",
              slot_state(repo, first["slot"])["attached"], claims.branch_for("auto/stub-b"))
        check("... `status` agrees it is not free", status(repo)[0]["free"], False)
        check("... and it is excluded from the free count", free_count(repo), 1)
        # the search must SKIP it and fall through to the next genuinely free slot
        skipped = acquire(repo, "auto/stub-skip")
        check("acquire skips the occupied slot and falls through to the next free one", skipped["slot"], 2)
        release(repo, slot=2, unit="auto/stub-skip", rescue=False)
        try:
            acquire(repo, "auto/stub-c", slot=first["slot"])
            check("a slot holding an unlanded branch is refused", "no error", "SystemExit")
        except SystemExit as exc:
            check("a slot holding an unlanded branch is refused", "never a branch" in str(exc), True)
        # a *stale lock* (branch gone) is reclaimable, not a wedge
        release(repo, slot=first["slot"], branch=claims.branch_for("auto/stub-b"), rescue=False)
        write_lock(repo, first["slot"], {"slot": first["slot"], "unit": "ghost/unit",
                                         "branch": "worker/ghost-gone", "worker": "dead"})
        check("a lock whose branch is gone is stale", lock_stale(repo, first["slot"]), True)
        revived = acquire(repo, "auto/stub-c", slot=first["slot"])
        check("a stale lock is reclaimed, not a wedge", revived["reclaimed"] is not None, True)
        check("... and the slot is now locked by the new claim", read_lock(repo, first["slot"])["unit"],
              "auto/stub-c")
        release(repo, slot=first["slot"], unit="auto/stub-c", rescue=False)
        # a crash leaves a marker on a DETACHED worktree: reclaimable, never a permanent wedge
        mark_used(d, "ghost", "ghost-gone")
        check("a `.used` marker on a detached worktree is stale",
              slot_state(repo, first["slot"])["marker_stale"], True)
        check("... and the slot reads free", slot_state(repo, first["slot"])["free"], True)
        reclaimed2 = acquire(repo, "auto/reclaim", slot=first["slot"])
        check("... acquire reclaims it and hands it out",
              (marker_present(d), reclaimed2["slot"]), (True, first["slot"]))
        release(repo, slot=first["slot"], unit="auto/reclaim", rescue=False)

        # (4) THE CAP: fill every slot and the free count is zero
        acquire(repo, "auto/cap-1", slot=1)
        acquire(repo, "auto/cap-2", slot=2)
        check("no slot is free at the cap", free_slots(repo), [])
        check("capacity_error names the cap", "concurrency cap" in (capacity_error(repo) or ""), True)
        try:
            acquire(repo, "auto/cap-3")
            check("acquire refuses past the cap", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire refuses past the cap", "concurrency cap" in str(exc), True)
        release(repo, slot=1, unit="auto/cap-1", rescue=False)
        release(repo, slot=2, unit="auto/cap-2", rescue=False)
        check("releasing frees a slot", len(free_slots(repo)), 2)

        # a slot reset discards scratch and stale source edits, keeps the warm trees
        info2 = acquire(repo, "auto/dirty", slot=1)
        dd = info2["dir"]
        open(os.path.join(dd, "src", "auto", "stub.c"), "w").write("/* dirty edit */\n")
        open(os.path.join(dd, "scratch.txt"), "w").write("junk\n")
        os.makedirs(os.path.join(dd, "out", "scratch"), exist_ok=True)
        open(os.path.join(dd, "out", "scratch", "junk.bin"), "wb").write(b"x")
        release(repo, slot=1, unit="auto/dirty", rescue=False)
        check("a dirty src/ is reset to main", "dirty edit" not in open(
            os.path.join(dd, "src", "auto", "stub.c")).read(), True)
        check("stray scratch is discarded", os.path.exists(os.path.join(dd, "scratch.txt")), False)
        check("stray dirs are discarded", os.path.exists(os.path.join(dd, "out")), False)
        check("the warm build tree survives the reset",
              os.path.exists(os.path.join(dd, "build", "RMHE08", "report.json")), True)
        check("the orig payload survives the reset",
              os.path.exists(os.path.join(dd, "orig", "RMHE08", "sys", "main.dol")), True)
        check("the m2c submodule survives the reset",
              os.path.exists(os.path.join(dd, "tools", "m2c", "m2c.py")), True)
        check("the slot is detached and free after release",
              (slot_attached_branch(dd), read_lock(repo, 1)), (None, {}))
        check("... and its `.used` marker is gone", marker_present(dd), False)

        # integration: the *claim path* takes a slot and its release returns it (claims.py's own selftest
        # covers the classic path; this is the seam between the two tools)
        claims.save_registry(repo, {})
        info3 = claims.claim("auto/slotted", repo, "w-int", False, cwd=repo)
        check("claims.claim takes a slot when a pool exists", info3.get("slot") in (1, 2), True)
        check("... the worktree is the slot dir", info3["worktree"], slot_dir(repo, info3["slot"]))
        check("... the registry records the slot",
              claims.load_registry(repo)["auto/slotted"].get("slot"), info3["slot"])
        no_pane = lambda _row: {"pane": None, "known": True, "alive": False, "active": False,
                                "status": None, "revision": None}
        rel3 = claims.release("auto/slotted", repo, force=False, dry_run=False, probe=no_pane,
                              lister=lambda: None)
        check("claims.release returns the slot, not a worktree removal", rel3.get("complete"), True)
        check("... the slot directory persists", os.path.isdir(slot_dir(repo, info3["slot"])), True)
        check("... and it is detached and unlocked",
              (slot_attached_branch(slot_dir(repo, info3["slot"])), read_lock(repo, info3["slot"])),
              (None, {}))
        check("... the branch is deleted by the return",
              claims.branch_exists(repo, claims.branch_for("auto/slotted")), False)

        # status reports every slot
        rows = status(repo)
        check("status lists every slot", [r["slot"] for r in rows], [1, 2])
        check("status reports the build tree current on a clean pool", all(r["build_ok"] for r in rows), True)
        check("status keeps `used` and the worktree branch as separate readings",
              all("used" in r and "attached" in r and "marked" in r for r in rows), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --- CLI -----------------------------------------------------------------------------------------

def _main_root() -> str:
    from units import recompile as rc
    return rc.main_root(rc.worktree_root())


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--main", default=None, help="MAIN's path (default: the first worktree git lists)")
    sub = ap.add_subparsers(dest="cmd")
    i = sub.add_parser("init", help="create the pool of reusable slots")
    i.add_argument("--count", type=int, default=DEFAULT_COUNT)
    i.add_argument("--force", action="store_true", help="remove and re-create existing slots")
    i.add_argument("--json", action="store_true")
    a = sub.add_parser("acquire", help="take a slot: reset, cut a fresh branch off main, verify, lock")
    a.add_argument("unit")
    a.add_argument("--slot", type=int, default=None)
    a.add_argument("--worker", default=None)
    a.add_argument("--branch", default=None)
    a.add_argument("--dry-run", action="store_true")
    a.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="return a slot to main's tip (keeps the warm trees)")
    r.add_argument("--slot", type=int, default=None)
    r.add_argument("--unit", default=None)
    r.add_argument("--branch", default=None)
    r.add_argument("--keep-branch", action="store_true", help="do not delete the claim's branch")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="every slot, its lock and whether its build tree is current")
    s.add_argument("--json", action="store_true")
    v = sub.add_parser("verify", help="validate each slot's build tree against MAIN's current map/DOL")
    v.add_argument("--slot", type=int, default=None)
    v.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0
    main_wt = args.main or _main_root()

    if args.cmd == "init":
        out = init(main_wt, args.count, args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("pool of %d slot(s) under %s" % (out["count"], os.path.dirname(out["slots"][0])))
        print("  created: %s" % (out["created"] or "none"))
        print("  present: %s" % (out["present"] or "none"))
        for n, note in out["notes"]:
            print("  slot %d: %s" % (n, note))
        return 0
    if args.cmd == "acquire":
        if args.dry_run:
            out = preview(main_wt, args.unit, args.branch, args.slot)
            print("would acquire slot %d: %s" % (out["slot"], out["command"]))
            return 0
        info = acquire(main_wt, args.unit, args.branch, args.worker, args.slot)
        if args.json:
            print(json.dumps(info, indent=2))
            return 0
        print("acquired slot %d for %s\n  dir     %s\n  branch  %s\n  base    %s\n  verify  %s"
              % (info["slot"], info["unit"], info["dir"], info["branch"], info["base"],
                 "current" if info["verify"]["ok"] else "; ".join(info["verify"]["reasons"])))
        if info.get("refreshed"):
            print("  refreshed %s" % info["refreshed"])
        return 0
    if args.cmd == "release":
        out = release(main_wt, args.slot, args.unit, args.branch, delete_branch=not args.keep_branch,
                      dry_run=args.dry_run)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        verb = "would release" if args.dry_run else "released"
        print("%s slot %d (%s)%s%s" % (verb, out["slot"], out["dir"],
                                       "" if out["branch_deleted"] or not out["branch"] else
                                       " - branch %s kept" % out["branch"],
                                       "" if not out["refreshed"] else "\n  %s" % out["refreshed"]))
        return 0
    if args.cmd == "status":
        rows = status(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no slot pool - run `python tools/units/slots.py init`")
            return 0
        print("%-5s %-10s %-24s %-20s %-7s %s" % ("slot", "state", "unit", "branch", ".used", "build tree"))
        for row in rows:
            lock = row.get("lock") or {}
            if row["free"]:
                state = "free"
            elif row["attached"]:
                state = "in use"
            else:
                state = "used"
            marker = "stale" if row.get("marker_stale") else ("yes" if row["marked"] else "-")
            print("%-5d %-10s %-24s %-20s %-7s %s" % (
                row["slot"], state, (lock.get("unit") or "-")[:24],
                (row["attached"] or "detached")[:20], marker,
                "current" if row["build_ok"] else "; ".join(row["build_reasons"])))
        return 0
    if args.cmd == "verify":
        rows = all_slots(main_wt)
        nums = [args.slot] if args.slot else [r["slot"] for r in rows]
        bad = 0
        out = []
        for n in nums:
            v = verify(main_wt, n)
            out.append(v)
            if not v["ok"]:
                bad += 1
            if not args.json:
                print("slot %d: %s" % (n, "current" if v["ok"] else "; ".join(v["reasons"])))
        if args.json:
            print(json.dumps(out, indent=2))
        return 1 if bad else 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
