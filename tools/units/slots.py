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
    python tools/units/slots.py acquire <unit> [--slot N] [--worker NAME] [--force] [--dry-run]
    python tools/units/slots.py spawn --kind KIND [--slot N] [--unit U] [--task-file PATH] [--json]
    python tools/units/slots.py release [--slot N | --unit U | --branch B] [--keep-branch] [--force] [--dry-run]
    python tools/units/slots.py reclaim [--slot N | --unit U | --branch B] [--json]
    python tools/units/slots.py status [--json]
    python tools/units/slots.py verify [--slot N] [--json]
    python tools/units/slots.py shadow <slot> <dir> [--seed-build] [--force]
    python tools/units/slots.py --selftest

**One lane per slot, enforced at both ends.**  A slot's claim was released while another lane was still
working in it, and the release detached HEAD under a live process; the same hole let a second lane acquire
a slot that still held the first lane's branch.  The sentinel and the release path are now the two ends of
one rule:

* the `.used` sentinel records its **OWNER label** (`mark_used`, `marker_info`), and `status` prints it, so
  "someone holds this" is never the only thing a reader can know;
* `acquire` refuses a slot whose sentinel **names a different owner** instead of silently reclaiming it
  (`--force` is the deliberate override);
* `release` **fails closed** on three signals - a RUNNING Claude session whose cwd resolves into the slot,
  a dirty tree, and commits on HEAD that no branch reaches - printing every reason and offering `--force`.
  The session registry under the Claude config dir is the *only* live-lane signal available: a lock file records what
  *this tool* did, and a lane is a process the tool never launched, so `release_blockers` reads the
  harness' own run records rather than inferring liveness from anything it wrote itself.

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
  `symbols.txt`/`splits.txt`/`main.dol`), a byte comparison of `build/RMHE08/report.json`, AND the
  **compile-output set** the official scorer opens (`objdiff.json`'s `target_path`/`base_path`, existence -
  `compile_outputs`).  A slot can pass the first two while its `obj/`/`src/` objects are gone; that tree
  cannot run `objdiff report generate`, so it is refused too.  **If it cannot be proven current, re-seed;
  never proceed on a doubt.**
* **claim-time currency**: a freshly seeded slot is *always* a few `ninja` steps behind by construction (the
  seed copies MAIN's `build/` with MAIN's mtimes while `git worktree add` stamps the slot's own sources at
  checkout time), so "no work to do" is the wrong test for a handover.  `acquire` runs `ninja -n`, **finishes
  the pending steps in the slot** (`claim_currency`, bounded - measured 3: one MWCC unit, REPORT, PROGRESS),
  re-counts, and prints the proof the lane can see: `report.json` byte-identical, the compile-output set, the
  pending count.  A ninja that cannot run is reported as *unknown*, never silently as 0.
* **release** returns the slot to main's tip, keeps the warm trees, deletes the branch (rescue-ref first, as
  `claims.release` does) and refreshes the build tree so the next acquire is instant.
* **reclaim** turns "the slot still holds a **landed** branch" from a hand dance into one step.  A branch
  whose content is already in main is not work in progress, so `spawn`/`acquire` test it with the campaign's
  own free test (`git merge-tree --write-tree main <branch>` vs `git rev-parse main^{tree}` - equal means
  fully applied, the same test the held-branch audit uses) and, when it is applied, park the rescue ref
  (`refs/rescue/<slug>`) **first**, then detach the worktree, delete the branch, release the slot and take
  it - printing one line saying what it did and why.  The test compares **trees, not commits**, so the one
  rule covers both routes: a gate-landed branch stays ahead of main by its own commits but its tree equals
  main's, and the direct path-limited landing does the same.  An **unlanded** branch keeps today's refusal
  verbatim, and a test that cannot run or is ambiguous **refuses** (fail closed) rather than reclaim: the
  refusal is load-bearing.  The marker is not the signal: a slot whose `.used` is **MISSING** but whose
  branch is proven applied is reclaimable (a crash remnant), while the same missing marker next to an
  unlanded branch still refuses.  `python tools/units/slots.py reclaim [--slot N | --unit U | --branch B]`
  runs the same step by name so the orchestrator (or the next lane) can do it deliberately instead of by hand.
* **the cap is `pool.json`'s `count`** (`pool_size`), **not** a count of slot directories: a slot whose worktree
  is missing or broken is still a slot.  `status`/`capacity_error` enumerate every one and say per slot *why*
  it is not usable - `no worktree` / `branch` / `claimed` / `debris`, plus a `missing record` or
  `stale record: its base predates main` note with the remedy - so a full pool can never read as a phantom
  shortage.  A bulk `init --force` **refuses while any slot holds live work** (the pool is the campaign's
  concurrency cap, not a scratch file), and `tools/selftest.py` guards `.pi/slots/pool.json` **by bytes**
  (`.pi/` is gitignored) so no test run can shrink it.
"""

from __future__ import annotations

import argparse
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import lanecmd  # noqa: E402

DEFAULT_COUNT = 6
SLOT_SUFFIX = ".slot"
LOCK_SUBDIR = ("slots",)
# What a reset keeps.  Everything else untracked/ignored is scratch and goes; tracked edits are discarded
# by the forced `checkout`.  `tools/m2c` is a tracked submodule and must survive (see `clean_slot`).
SLOT_KEEP = (".ninja_deps", ".ninja_log", "build.ninja", "objdiff.json", "compile_commands.json",
             "build", "orig", os.path.join("tools", "m2c"), ".used")

# The ninja-invocation seam.  `None` means `subprocess.run`; the selftest swaps it so the claim-time
# currency checks are hermetic (they exercise the pending>0 -> complete -> 0 path without a real build).
NINJA_RUNNER = None

# A slot's **sentinel**: `.used` in the worktree root.  `acquire` creates it (atomically, `O_EXCL`) and
# `release` removes it; `status` reads it and reports `used` / `free`.  It is gitignored, and `clean_slot`
# preserves it across a reset *because* the marker the acquire just created must survive the reset - a
# stale marker from a crash is reclaimed by the worktree reading (see `slot_state`), not by the clean.
SLOT_MARKER = ".used"

#: How a lane's **kind of work** maps to the agent profile it is launched with.  This mapping is the fix
#: `spawn` exists for: `queue.py`/`claims.py` left the profile to the lane (defaulting to `decompiler`), so
#: three *tooling* lanes carried unit policy they could never satisfy and a general rule they must not
#: break (CLAUDE.md, "Operational mode").  The mapping is exhaustive - one kind per lane, never a guess.
KIND_PROFILE = {
    "unit": "surveyor",       # survey a claim, then reconstruct - the four-leg unit loop and reconstruct its bodies
    "fix": "fixer",           # a refused gate or a measured regression on a branch
    "merge": "merger",        # a refused apply
    "tooling": "worker",      # the fallback for a task that is none of the specific ones
    "docs": "worker",
    "review": "codereviewer",  # the tracked project review profile (the global `reviewer` is separate)
    "scout": "scout",          # read-only
    "plan": "planner",         # read-only
}

# Every agent profile a lane can be launched with - the values of the one table above.  A caller that takes
# a profile as an *override* (`queue next --profile`) validates against this, so the override cannot name a
# profile the harness does not have; the list is derived, never a second copy of KIND_PROFILE.
PROFILES = tuple(sorted(set(KIND_PROFILE.values())))


def marker_path(slot: str) -> str:
    return os.path.join(slot, SLOT_MARKER)


def marker_present(slot: str) -> bool:
    return os.path.exists(marker_path(slot))


def mark_used(slot: str, unit: str = "", branch: str = "", owner: str = "") -> bool:
    """Claim the slot by creating its `.used` marker **atomically**.  -> False when one already exists.

    `O_CREAT | O_EXCL` is the atomicity: two racing `acquire`s cannot both create the file, so the loser
    falls through to the next free slot (`_pick_free`).  That is the whole mechanism - no lock daemon, no
    platform shim, and a crashed acquire leaves a marker the worktree reading reclaims.

    The marker records **who** holds the slot - its OWNER label (`owner`), the unit, the branch and the
    pid - not merely that someone does.  The label is what a human reads in `slots.py status` and what
    `acquire` checks the claim it is about to launch against (`marker_claim_conflict`), so a sentinel left
    by a *different* claim is visible instead of indistinguishable from one's own.
    """
    path = marker_path(slot)
    try:
        fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
    except FileExistsError:
        return False
    except OSError:
        return False
    try:
        os.write(fd, marker_text(owner, unit, branch).encode("utf-8", "replace"))
    except OSError:
        pass
    finally:
        os.close(fd)
    return True


def marker_text(owner: str, unit: str = "", branch: str = "") -> str:
    """The `.used` sentinel's body: one `key=value` per line, the owner label first.

    Key=value rather than the older positional `<pid> <unit> <branch>` line because the owner label is the
    fact the launch check reads, and a positional line quietly means something else the next time a field
    is added.  A legacy line has no `owner=` and so reads as an **unknown** owner (`marker_info`), which is
    not a match for any claim - `marker_claim_conflict` says what that means rather than guessing.
    """
    return ("owner=%s\nunit=%s\nbranch=%s\npid=%d\nat=%s\n"
            % (owner, unit, branch, os.getpid(), time.strftime("%Y-%m-%dT%H:%M:%S")))


def marker_info(slot: str) -> dict:
    """The sentinel's contents, or `{}` when the slot has none.

    `legacy` marks a body that is not this format: the old positional `<pid> <unit> <branch>` line is
    parsed best-effort (no `owner`), and anything else unreadable still reports `legacy` so no caller
    mistakes a marker it could not read for a marker that names nobody.
    """
    try:
        with open(marker_path(slot), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return {}
    out: dict = {}
    for line in text.splitlines():
        key, sep, value = line.partition("=")
        if sep and key.strip():
            out[key.strip()] = value.strip()
    if "owner" in out:
        out["legacy"] = False
        return out
    parts = text.split()
    if len(parts) >= 3 and os.path.isdir(slot):
        out.update({"pid": parts[0], "unit": parts[1], "branch": parts[2]})
    out["legacy"] = True
    return out


def marker_owner(slot: str) -> str:
    """The sentinel's OWNER label - "" for an ownerless (legacy or half-written) marker."""
    return marker_info(slot).get("owner") or ""


def owner_label(unit: str, branch: str | None, worker: str | None = None) -> str:
    """The label a claim records in its sentinel: the worker label, else the branch slug, else the unit.

    A label rather than a slot number because the slot is what is *being taken*: the owner has to name the
    claim itself for two claims in the same slot to be tellable apart (which is the whole point of E1),
    and a branch slug is stable across rounds where a worker label is not given.
    """
    claims = _claims()
    return worker or (claims.slug_of_branch(branch) if branch else "") or unit


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


def _land():
    """`land` imported late for the same reason as `_claims`: land imports claims, which imports slots.

    Only `agents_md_real_change` is used, and only to keep the *one* implementation of "is CLAUDE.md
    really changed, or is that the LOCAL-ONLY block?" - a slot's CLAUDE.md carries live working state by
    rule 8, so a second copy of that cut here would be the bug, not the reuse.
    """
    from units import land
    return land


# --- the live-lane signal: Claude Code's session registry ------------------------------------------

#: The directory Claude Code keeps one `<pid>.json` per live session under: `<config dir>/sessions`, where the
#: config dir is `$CLAUDE_CONFIG_DIR` or `~/.claude`.  Each record carries `pid`, `sessionId`, `cwd`, `name`,
#: `kind` (`interactive` / headless) and `status` (`busy` / `idle`).
SESSIONS_DIRNAME = "sessions"


def config_dir() -> str:
    """Claude Code's config directory: `$CLAUDE_CONFIG_DIR`, else `~/.claude`."""
    return os.environ.get("CLAUDE_CONFIG_DIR") or os.path.join(os.path.expanduser("~"), ".claude")


def run_registries(config: str | None = None) -> list[str]:
    """The session registry under the config dir, as a one-element list (empty when there is none).

    **This registry is the only live-lane signal available to this tool, so it is read, never inferred.**
    A lock of any kind - the `.used` sentinel, the JSON claim record - only records what *this tool* did,
    and a lane is a process the tool never launched: the project proved that the day a lane worked in MAIN
    and left no slot file at all (roadmap 7.31), and again on 2026-09-28, when a slot's claim was released
    while another lane was still working in it and the release detached HEAD under the live process.  The
    harness records every session it starts - pid, `cwd`, `status` - and that record is what
    `release_blockers` reads.

    A host with no registry yields **no signal at all**: this returns an empty list, and the dirty-tree and
    unreachable-commit guards are then the only backstops.  That is stated rather than papered over -
    "no registry" is not "no lane".
    """
    d = os.path.join(config or config_dir(), SESSIONS_DIRNAME)
    return [d] if os.path.isdir(d) else []


def _read_json(path: str) -> dict:
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}
    return data if isinstance(data, dict) else {}


def pid_alive(pid) -> bool:
    """Whether a process with this pid is running.  Never signals it (`os.kill(pid, 0)` on Windows would
    terminate the process), so the Windows branch asks the kernel for its exit code instead."""
    if not isinstance(pid, int) or isinstance(pid, bool) or pid <= 0:
        return False
    if os.name == "nt":
        import ctypes
        kernel = ctypes.windll.kernel32
        handle = kernel.OpenProcess(0x1000, False, pid)        # PROCESS_QUERY_LIMITED_INFORMATION
        if not handle:
            return False
        code = ctypes.c_ulong()
        ok = kernel.GetExitCodeProcess(handle, ctypes.byref(code))
        kernel.CloseHandle(handle)
        return bool(ok) and code.value == 259                  # STILL_ACTIVE
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def _live_run_record(record: dict, run_id: str, registry: str) -> dict:
    """One session record, flattened to the fields a lane guard needs."""
    return {"run_id": record.get("sessionId") or run_id,
            "cwd": record.get("cwd") or "",
            "state": "running",
            "agent": record.get("agent") or record.get("kind") or "",
            "session": record.get("name") or "",
            "pid": record.get("pid"),
            "registry": registry}


def live_runs(registry: str | None = None, config: str | None = None) -> list[dict]:
    """Every **live** Claude session, from `<registry>/<pid>.json`.

    A record counts only while its process is alive: Claude Code removes the file on a clean exit, but a
    crash leaves it behind, and a stale record must not wedge a slot forever - so liveness is the pid, not
    the file's presence.  A record this function cannot read is skipped, not guessed at: an unreadable
    registry is reported as no signal (`run_registries`), never as "no lane".
    """
    runs = []
    for root in ([registry] if registry else run_registries(config)):
        try:
            names = sorted(os.listdir(root))
        except OSError:
            continue
        for name in names:
            if not name.endswith(".json"):
                continue
            record = _read_json(os.path.join(root, name))
            if not record or not pid_alive(record.get("pid")):
                continue
            runs.append(_live_run_record(record, name[:-5], root))
    return runs


def _same_tree(a: str, b: str) -> bool:
    """Whether path `a` is `b` or sits inside it (case-insensitively on Windows, separator-agnostic)."""
    if not a or not b:
        return False
    x, y = os.path.normcase(os.path.abspath(a)), os.path.normcase(os.path.abspath(b))
    return x == y or x.startswith(y.rstrip(os.sep) + os.sep)


def runs_in_slot(slot: str, registry: str | None = None, config: str | None = None) -> list[dict]:
    """Every live Claude session whose `cwd` resolves into `slot` - the slot is its working directory.

    A lane launched with its cwd at a subdirectory of the slot is in the slot too, so the test is
    "inside", not "equal".
    """
    return [run for run in live_runs(registry, config) if _same_tree(run.get("cwd"), slot)]


def run_label(run: dict) -> str:
    """`<run id> (<session name>)` - how a refusal names the run it is protecting."""
    name = (run.get("session") or "").strip()
    return "%s%s" % (run.get("run_id") or "?", " (%s)" % name[:60] if name else "")


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return (p.stdout or "").strip()


def slot_dir(main: str, n: int) -> str:
    """A slot's **stable** path: a sibling of MAIN, `<repo>.slot<n>` (e.g. `mhtri-dtk.slot3`)."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s%s%d" % (os.path.basename(main), SLOT_SUFFIX, n))


def slot_count(main: str) -> int:
    """How many slot directories exist right now (0 means the pool is not initialised).

    The manifest's `count` bounds the scan; with no manifest the default six is assumed.  This is the
    *presence* reading (`enabled`); the **cap** is `pool_size`, which is the manifest's own count - see the
    note there for why counting directories is the wrong authority.
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


def pool_size(main: str) -> int:
    """The pool's SIZE: `pool.json`'s `count` - the **authority** for the cap - or 0 with no manifest.

    The cap is a persisted fact `init` wrote, **not** a count of directories that happen to exist right
    now.  Counting directories (the old `all_slots` range) let one broken slot shrink the whole pool: slot
    2's worktree was gone, so a full pool of 6 enumerated as `slots 1..5`, silently dropping slot 6 - a
    live lane - and the refusal then read like a pool shortage instead of one slot needing a reset.  Every
    slot in the manifest is enumerated, and `status` says per slot *why* it is not usable.
    """
    count = pool_manifest(main).get("count")
    if isinstance(count, int) and count >= 1:
        return count
    # No manifest: the pool is not initialised, but any slot dirs that were made by hand still enumerate.
    return slot_count(main)


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
    p = subprocess.run(["git", "-C", slot, "symbolic-ref", "-q", "HEAD"], capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    ref = (p.stdout or "").strip()
    if p.returncode == 0 and ref.startswith("refs/heads/"):
        return ref[len("refs/heads/"):]
    return None


def slot_head(slot: str) -> str | None:
    p = subprocess.run(["git", "-C", slot, "rev-parse", "HEAD"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return (p.stdout or "").strip() if p.returncode == 0 else None


def slot_dirty(slot: str) -> list[str]:
    """The slot tree's rows a release would destroy: `[" M src/x.cpp", "?? notes.md"]`.

    `git status --porcelain -uall` is the definition - tracked edits and untracked non-ignored files (the
    slot's `build/`, `orig/`, `.pi/` and `.used` are ignored, so the warm trees never show up here).  One
    row is deliberately not dirt: an `CLAUDE.md` whose only difference is its LOCAL-ONLY block, which is
    live working state by rule 8 and is dirty in every real slot - `land.agents_md_real_change` owns that
    judgement, and a release that called it dirt would refuse every ordinary teardown.

    The porcelain output is read **raw**: `git()` strips its stdout, which eats the leading space of the
    first row and shifts every field by one (` M CLAUDE.md` -> `M CLAUDE.md`, so the path parses as
    `GENTS.md`).  `clean_slot` reads `git clean`'s output the same way, for the same reason.
    """
    p = subprocess.run(["git", "-C", slot, "status", "--porcelain", "-uall"], capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("git status failed in %s: %s" % (slot, (p.stderr or "").strip()))
    out = []
    for row in (p.stdout or "").splitlines():
        if not row.strip():
            continue
        code, path = row[:2], row[3:].strip().strip('"')
        if path == "CLAUDE.md" and code.strip() in ("M", "MM"):
            if not _land().agents_md_real_change(slot):
                continue
        out.append(row.rstrip())
    return out


def orphaned_commits(slot: str) -> list[str]:
    """Commits on the slot's HEAD that no branch or remote-tracking ref reaches.

    `release` detaches the slot and deletes its branch, so a commit only this HEAD holds would survive
    nowhere but the reflog.  A detached slot sitting on main's tip (the state every release leaves, and the
    state every acquire starts from) has none, and a lane's own branch reaches its own commits, so this
    fires exactly on the pathological case: detached work, or a branch whose commits were already deleted
    from under it.  `--force` overrides, and the refusal names where the commits *do* live (a rescue ref is
    usually the answer).
    """
    head = slot_head(slot)
    if not head:
        return []
    return [ln.strip() for ln in git(["rev-list", head, "--not", "--branches", "--remotes"],
                                     slot).splitlines() if ln.strip()]


def refs_containing(slot: str) -> list[str]:
    """Every ref whose history contains the slot's HEAD - where work would otherwise be hiding."""
    head = slot_head(slot)
    if not head:
        return []
    return [ln.strip() for ln in git(["for-each-ref", "--contains", head, "--format=%(refname)"],
                                     slot).splitlines() if ln.strip()]


def release_blockers(main: str, n: int, d: str, allow_dirty: bool = False,
                     registry: str | None = None, config: str | None = None) -> list[tuple[str, str]]:
    """Everything that makes releasing slot `n` unsafe: `[(one-line summary, detail)]`, `[]` if safe.

    Three signals, least to most precise:

    * **a RUNNING Claude session whose cwd resolves into the slot** - read from the harness' session registry
      (`run_registries`), because that is the only place a live lane is visible at all: the `.used`
      sentinel and the JSON lock record say what *this tool* did, and the lane is a process the tool never
      launched.  This is the guard the 2026-09-28 release-while-running would have hit, and it names the
      run;
    * **a dirty tree** (`slot_dirty`) - the release runs `checkout -f --detach` + `clean -ffdx` and would
      discard uncommitted work.  Skipped when `allow_dirty`, which is for a caller that has already proven
      the work is recorded elsewhere (`claims.release` refuses un-merged, un-recorded work before it ever
      reaches the slot);
    * **commits no branch reaches** (`orphaned_commits`) - the release detaches HEAD and deletes the
      branch, which is exactly how unlanded work is orphaned.

    Each blocker is a (summary, detail) pair rather than one string so the refusal's **first line** names
    every reason compactly: `claims.release` records only that first line in its teardown step
    (`_run_teardown`), and a step that said "slot 1 return failed" without naming the run would be the same
    "the reason is invisible" problem the landing gate spent a session fixing.
    """
    out: list[tuple[str, str]] = []
    runs = runs_in_slot(d, registry, config)
    if runs:
        out.append(("a RUNNING Claude session is still working in it: "
                    + ", ".join(run_label(r) for r in runs),
                    "cwd %s - the session registry under the Claude config dir is the only live-lane signal there is, "
                    "because a lock cannot see a lane" % d))
    if not allow_dirty:
        dirty = slot_dirty(d)
        if dirty:
            out.append(("the slot's tree is dirty (%d path(s), e.g. %s)"
                        % (len(dirty), dirty[0].strip()),
                        "a release would discard:\n      " + "\n      ".join(dirty[:8])
                        + ("\n      (+%d more)" % (len(dirty) - 8) if len(dirty) > 8 else "")))
    orphans = orphaned_commits(d)
    if orphans:
        refs = refs_containing(d)
        out.append(("HEAD holds %d commit(s) no branch reaches (%s)"
                    % (len(orphans), ", ".join(c[:8] for c in orphans[:4])),
                    "a release would detach and orphan them"
                    + ("; they do live in %s" % ", ".join(refs[:4]) if refs else "")))
    return out


def release_refusal(main: str, n: int, d: str, blockers: list[tuple[str, str]]) -> str:
    """The refusal `release` raises - every reason on the first line, the detail under it, then `--force`."""
    lines = ["REFUSED release slot %d (%s): %s" % (n, d, "; ".join(s for s, _ in blockers)),
             "  releasing would detach the slot and delete its branch while that is still live:"]
    for summary, detail in blockers:
        lines.append("  - %s: %s" % (summary, detail))
    lines.append("  if that is what you mean, say so deliberately: "
                 "python tools/units/slots.py release --slot %d --force" % n)
    return "\n".join(lines)


def marker_claim_conflict(slot: str, owner: str, n: int | None = None) -> str | None:
    """The refusal when the slot's sentinel names a claim other than `owner`, else None.

    `acquire` used to treat *any* marker it found on a detached worktree as reclaimable, so a sentinel
    left by a different claim was indistinguishable from one of its own and the next acquire reset the
    tree under that claim's lane.  The sentinel names its owner for exactly this check.  An **ownerless**
    marker (a legacy body, or a crash before the owner was written) is not a conflict - reclaiming it is
    the documented behaviour that keeps a crashed acquire from wedging a slot - and a marker naming this
    same label is simply this claim's own.
    """
    found = marker_info(slot)
    if not found or not found.get("owner"):
        return None
    if found.get("owner") == owner:
        return None
    how = ("--slot %d --force" % n) if n is not None else "--force"
    return ("REFUSED slot %s: its `.used` sentinel belongs to another claim - owner %r (unit %r, branch "
            "%r).\n  one lane per slot: reusing a slot under a live owner is how a lane's HEAD got "
            "detached mid-run.\n  if that owner is gone, say so deliberately: `python tools/units/slots.py "
            "acquire <unit> %s` (or release that slot first)"
            % (os.path.basename(slot), found.get("owner"), found.get("unit") or "?",
               found.get("branch") or "?", how))


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


def slot_of_path(main: str, path: str | None) -> int | None:
    """The slot number `path` resolves to, or None - a legacy registry row may carry only a `worktree`."""
    if not path:
        return None
    want = os.path.normcase(os.path.realpath(path))
    for n in range(1, pool_size(main) + 1):
        if os.path.normcase(os.path.realpath(slot_dir(main, n))) == want:
            return n
    return None


def registry_claims_by_slot(main: str) -> tuple[dict, bool]:
    """`(claims keyed by slot number, readable)` from the **claim registry** (`.pi/claims.json`).

    The claim registry is the authority for liveness: it records the unit, branch, worker and **slot** a
    claimant holds, and a `claims.py claim`/`release` pair keeps it in step.  The per-slot JSON lock
    (`.pi/slots/<n>.json`) is a convenience copy that a partial teardown can leave behind, so this reading is
    what decides "is somebody's work in this slot" - and it is read here, once, for every row.

    An **absent** file means "no claims" and is readable.  A file that exists but cannot be parsed (or is not
    a JSON object) is **not readable** (`readable=False`), and a caller that would hand a slot out or reclaim
    one must refuse rather than guess.  A row is keyed by ITS slot number; a legacy row that carries only a
    `worktree` is matched by resolving that path to a slot directory (`slot_of_path`).
    """
    path = _claims().registry_path(main)
    if not os.path.exists(path):
        return {}, True
    try:
        with open(path, encoding="utf-8") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}, False
    if not isinstance(data, dict):
        return {}, False
    by_slot: dict = {}
    for unit, rec in data.items():
        if not isinstance(rec, dict):
            continue
        n = rec.get("slot")
        if not isinstance(n, int):
            n = slot_of_path(main, rec.get("worktree"))
        if n is None or n in by_slot:
            continue
        by_slot[n] = dict(rec, unit=rec.get("unit") or unit)
    return by_slot, True


def slot_state(main: str, n: int, runs: list | None = None, claims_by_slot: dict | None = None) -> dict:
    """One slot's row: whether it exists, what it has checked out, its `.used` marker and whether it is free.

    **`free`/`used` is read from several sources of the same fact, and all must agree that the slot is
    empty.**  The primary is the `.used` sentinel `acquire` creates and `release` removes; the second is the
    worktree state - a slot whose worktree still has a branch checked out is **in use**; the third is the
    **claim registry** (`.pi/claims.json`) and the per-slot lock record, which name a live claim even when the
    sentinel has been lost.  The two disagreed in the wild (slot 2 was reported `free` while its worktree held
    `worker/rule10-fix-14f8`), and that disagreement *was* the bug, so every reading is kept.

    The row carries a one-word **`state`** so `status` can say *why* a slot is not free instead of a bare
    `in use`:

    * `free` / `no worktree`;
    * `LIVE` - a RUNNING Claude session's cwd is the slot (the only reading that sees a *lane*);
    * `claimed` - a live claim: a registry row names this slot, or a non-stale lock record does;
    * `branch` - a branch is checked out and no live claim holds it (the reclaim/refusal decision);
    * `debris` - a `.used` sentinel on a *detached* worktree with no live claim: a crash remnant.

    `why` carries the actionable detail, and for a non-free slot it also names a bad **record** (`.pi/slots/<n>
    .json`): `missing record` when a branch/sentinel exists with no record, `stale record: its base predates
    main` when the record names a state the slot has left.  A missing or broken worktree is `no worktree`, and
    the slot is still enumerated (the cap is `pool.json`'s `count`), so a full pool never reads as a shortage.

    A `.used` marker on a *detached* worktree with no live claim is a crash remnant, not a lane - it is named
    `debris`, the slot still reads **free** (so a crashed acquire can never wedge a slot) and `reclaim` clears
    it.  It still reports its OWNER label: a marker that names somebody is exactly what `acquire` refuses
    (`marker_claim_conflict`) unless the takeover is meant.

    `runs` is the already-resolved live-run list and `claims_by_slot` the already-read registry, so `all_slots`
    reads each once instead of once per slot.
    """
    d = slot_dir(main, n)
    exists = os.path.isdir(d) and os.path.exists(os.path.join(d, ".git"))
    if claims_by_slot is None:
        claims_by_slot, registry_ok = registry_claims_by_slot(main)
    else:
        registry_ok = True
    claim = claims_by_slot.get(n)
    lock = read_lock(main, n)
    stale = lock_stale(main, n, lock) if lock else False
    live_lock = bool(lock and not stale)
    attached = slot_attached_branch(d) if exists else None
    marked = marker_present(d) if exists else False
    marker = marker_info(d) if marked else {}
    live_claim = bool(claim) or live_lock
    marker_stale = marked and not attached and not live_claim
    used = bool(attached) or bool(claim) or live_lock or (marked and not marker_stale)
    free = exists and not used
    run = None
    if exists:
        runs = runs_in_slot(d) if runs is None else [r for r in runs if _same_tree(r.get("cwd"), d)]
        run = runs[0] if runs else None
    owner = marker.get("owner") or ((claim or {}).get("worker") if claim else "") \
        or (lock.get("worker") if lock else "") or ""
    if not exists:
        state = "no worktree"
        why = ("no worktree at %s (its directory or `.git` is missing) - re-create it (`slots.py init`) or "
               "use another slot" % d)
    elif run:
        state, why = "LIVE", "in use by a RUNNING Claude session %s" % run_label(run)
    elif claim:
        state = "claimed"
        why = "in use by a live claim: %s (%s, branch %s)" % (
            claim.get("unit") or "?", claim.get("worker") or "?", claim.get("branch") or "?")
    elif live_lock:
        state, why = "claimed", "in use by %s (%s)" % (lock.get("unit") or "?", lock.get("worker") or "?")
    elif attached:
        state = "branch"
        why = "in use - holds branch %s (its `.used` marker is %s%s); release it first, or reclaim it if it " \
              "is already landed: slots.py reclaim --slot %d" % (
                  attached, "present" if marked else "MISSING",
                  ", owner %s" % owner if owner else "", n)
    elif marked and not marker_stale:
        state = "claimed"
        why = "used (`.used` marker present, worktree detached%s)" % (", owner %s" % owner if owner else "")
    elif marker_stale:
        state = "debris"
        why = ("debris: a `.used` marker on a detached worktree with no live claim - reclaimable%s"
               % ("; it names owner %s" % owner if owner else ""))
    elif lock and stale:
        state, why = "free", "stale lock (%s) - reclaimable" % (lock.get("unit") or "?")
    else:
        state, why = "free", "free"
    # A bad per-slot RECORD is worth naming: it is the difference between "this slot is busy" and "this
    # slot's bookkeeping is from a claim it has left".  `missing record` when a branch/sentinel exists with
    # no `.pi/slots/<n>.json`; `stale record` when the record names a state the slot has left.
    record_note = ""
    if exists and (attached or marked) and not lock:
        record_note = "missing record (no `.pi/slots/%d.json`) - reclaim --slot %d if it is landed, else release" \
                      % (n, n)
    elif exists and lock and stale:
        record_note = ("stale record: it names branch %s (base %s) but the slot holds %s - `acquire --slot %d` "
                       "resets it" % (lock.get("branch") or "?", (lock.get("base") or "?")[:9],
                                       attached or "nothing", n))
    if record_note and "free" not in why:
        why = "%s; %s" % (why, record_note)
    return {"slot": n, "dir": d, "exists": exists, "attached": attached, "lock": lock, "stale": stale,
            "marked": marked, "marker": marker, "owner": owner, "marker_stale": marker_stale,
            "claim": claim, "live_claim": live_claim, "registry_ok": registry_ok, "state": state,
            "record_note": record_note, "used": used, "free": free, "run": run, "why": why}


def all_slots(main: str, runs: list | None = None, claims_by_slot: dict | None = None) -> list[dict]:
    """Every slot in the pool - the manifest's `count`, not the directories that happen to exist.

    The run registry and the claim registry are each read **once** (`runs`, `claims_by_slot`).
    """
    live = live_runs() if runs is None else runs
    if claims_by_slot is None:
        claims_by_slot, _ok = registry_claims_by_slot(main)
    return [slot_state(main, n, runs=live, claims_by_slot=claims_by_slot)
            for n in range(1, pool_size(main) + 1)]


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


def _manifest_outputs(root: str) -> list[str] | None:
    """The compile-output paths `root`'s `objdiff.json` names, or `None` if it is absent/unreadable.

    The official scorer (`objdiff report generate`) opens exactly these: `target_path` is the split object
    and `base_path` the object ninja compiles from `src/`.  Paths are returned in the manifest's spelling,
    with `/` normalised to the host separator so they join onto a root directly.
    """
    try:
        data = json.loads(open(os.path.join(root, "objdiff.json"), encoding="utf-8").read())
        units = data["units"]
        if not isinstance(units, list):
            return None
    except (OSError, ValueError, KeyError, TypeError):
        return None
    out: list[str] = []
    for u in units:
        if not isinstance(u, dict):
            continue
        for key in ("target_path", "base_path"):
            p = u.get(key)
            if isinstance(p, str) and p:
                out.append(p.replace("/", os.sep))
    return out


def compile_outputs(main: str, slot: str) -> tuple[list[str], int, bool]:
    """`(missing, checked, manifest_ok)` - the official scorer's compile outputs absent from `slot`.

    `_build_is_current` and `report_matches` both pass on a slot whose `build/RMHE08/obj/` or
    `build/RMHE08/src/` objects were dropped: `config.json`'s mtime and `report.json`'s bytes say nothing
    about the objects the report was computed from, so a lane could be handed a tree that cannot run the
    official scorer at all - exactly the doubt `verify` exists to catch.

    The candidate set is the union of the two trees' manifests (`this tree's` and MAIN's), and a path is
    **missing** only when the slot lacks it while MAIN holds it: the slot is seeded from MAIN, so MAIN's
    copy is the arbiter, and a gap in MAIN's own tree can never wedge the pool.  Existence is the test, not
    bytes - a stale-but-present object is `_build_is_current`'s job, a missing one is here.  `manifest_ok`
    is False when MAIN has a manifest but the slot's is gone: the scorer reads the slot's copy, so its
    absence is a doubt even if every object is present.  This is the cheap set assertion the docstring
    offers in place of a full `ninja -n`, which needs the toolchain and rebuilds the dependency graph.
    """
    slot_units = _manifest_outputs(slot)
    main_units = _manifest_outputs(main)
    manifest_ok = slot_units is not None or main_units is None
    expected: list[str] = []
    for group in (main_units or [], slot_units or []):
        expected.extend(group)
    missing: list[str] = []
    seen: set[str] = set()
    for rel in expected:
        if rel in seen:
            continue
        seen.add(rel)
        if os.path.isfile(os.path.join(slot, rel)) and os.path.getsize(os.path.join(slot, rel)) > 0:
            continue
        if os.path.isfile(os.path.join(main, rel)):   # the slot failed to carry over a MAIN output
            missing.append(rel)
    return missing, len(seen), manifest_ok


def verify(main: str, n: int) -> dict:
    """Whether slot `n`'s kept build tree can be **proven** current against MAIN's map/DOL - fail closed.

    Reuses `claims._build_is_current` (the guard `seed_worktree_build` already has) with the slot as the
    build root and MAIN as the input root, byte-compares `report.json` so a stale report (a landing that
    moved a body but not the map) cannot be handed over as current, and asserts the **compile-output set**
    the official scorer opens (`objdiff.json`'s `target_path`/`base_path`, see `compile_outputs`) so a slot
    whose `obj/`/`src/` objects were dropped is refused rather than handed to a lane that cannot score.
    """
    claims = _claims()
    d = slot_dir(main, n)
    reasons: list[str] = []
    if not os.path.isdir(d):
        return {"slot": n, "dir": d, "ok": False, "main_build_current": False, "build_current": False,
                "report_matches": False, "compile_outputs": False, "compile_outputs_checked": 0,
                "compile_outputs_missing": 0, "reasons": ["slot directory missing: %s" % d]}
    main_current = claims._main_build_is_current(main)
    build_current = claims._build_is_current(d, main)
    report_ok = report_matches(main, d)
    missing, checked, manifest_ok = compile_outputs(main, d)
    compile_ok = bool(manifest_ok and not missing)
    if not main_current:
        reasons.append("MAIN's own build tree is stale (its config.json predates the map/DOL) - `ninja` in MAIN")
    if not build_current:
        reasons.append("slot build/RMHE08/config.json predates MAIN's current map/DOL - the split is stale")
    if not report_ok:
        reasons.append("slot build/RMHE08/report.json differs from MAIN's current report")
    if not manifest_ok:
        reasons.append("slot objdiff.json is missing or unreadable - the official scorer reads it")
    if missing:
        shown = ", ".join(missing[:3])
        more = "" if len(missing) <= 3 else " (+%d more)" % (len(missing) - 3)
        reasons.append("slot is missing %d of the %d compile outputs the official scorer reads (e.g. %s%s) - "
                       "`ninja` here or re-seed" % (len(missing), checked, shown, more))
    return {"slot": n, "dir": d, "ok": bool(main_current and build_current and report_ok and compile_ok),
            "main_build_current": main_current, "build_current": build_current, "report_matches": report_ok,
            "compile_outputs": compile_ok, "compile_outputs_checked": checked,
            "compile_outputs_missing": len(missing), "reasons": reasons}


# --- claim-time currency: the proof a lane's tree is ready to work in ---------------------------------

# ninja prints one `[N/M]` progress line per pending step in dry-run mode (`[1/3] MWCC ...`, `[3/3]
# PROGRESS`); M is the total, and zero lines is "ninja: no work to do.".
_NINJA_STEP = re.compile(r"^\[\d+/\d+\]", re.M)


def ninja_pending(slot: str, runner=None) -> tuple[int | None, str]:
    """`(steps, note)` - what `ninja -n` would still run in `slot`; `steps` is None when it cannot say.

    A freshly seeded slot is **always** a few steps behind by construction: the seed copies MAIN's `build/`
    with MAIN's mtimes while `git worktree add` stamps the slot's own sources at checkout time, so the
    slot's `src/` edges look newer than the objects seeded from MAIN.  That makes "no work to do" the wrong
    test for a claim handover - a fresh slot measured `[1/3] MWCC`, `[2/3] REPORT`, `[3/3] PROGRESS`.  The
    honest answer is the count, and a completed claim is 0.  `None` means ninja could not answer (`note`
    says why) - reported as *unknown*, never rounded down to a confident 0.
    """
    runner = runner or NINJA_RUNNER or subprocess.run
    try:
        p = runner(["ninja", "-n"], cwd=slot, capture_output=True, text=True, encoding="utf-8",
                   errors="replace")
    except OSError as exc:
        return None, "ninja unavailable (%s)" % exc
    if p.returncode != 0:
        tail = [ln for ln in ((p.stderr or "") + "\n" + (p.stdout or "")).strip().splitlines() if ln.strip()]
        return None, "ninja -n failed: %s" % (tail[-1] if tail else "exit %d" % p.returncode)
    return len(_NINJA_STEP.findall(p.stdout or "")), ""


def claim_currency(main: str, n: int, v: dict | None = None, complete: bool = True, runner=None) -> dict:
    """The proof printed at handover, and the pending work finished here rather than left to the lane.

    Three things a caller (and the lane) can see: MAIN's `report.json` is byte-identical in the slot, the
    official scorer's compile-output set is complete, and `ninja` has **no pending work**.  The last is the
    one a fresh slot never has by construction, so it is *run*, not asserted: `complete=True` executes
    `ninja` in the slot - bounded and small (the seed leaves ~3 steps) - and re-counts, so the number
    printed is what is left, not what was.  `complete=False` only counts.  A ninja that cannot run is
    reported as unknown in `pending_note`, never silently as 0.
    """
    runner = runner or NINJA_RUNNER or subprocess.run
    d = slot_dir(main, n)
    if v is None:
        v = verify(main, n)
    steps, note = ninja_pending(d, runner=runner)
    completed = None
    if complete and steps:
        try:
            p = runner(["ninja"], cwd=d, capture_output=True, text=True, encoding="utf-8", errors="replace")
        except OSError as exc:
            p, note = None, "ninja unavailable (%s)" % exc
        if p is not None:
            completed = p.returncode == 0
            if not completed:
                tail = [ln for ln in ((p.stderr or "") + "\n" + (p.stdout or "")).strip().splitlines()
                        if ln.strip()]
                note = "ninja failed: %s" % (tail[-1] if tail else "exit %d" % p.returncode)
            steps, note2 = ninja_pending(d, runner=runner)
            if completed:
                note = note2
                # ninja rewrote the build tree; the proof must describe what is on disk NOW, not before
                v = verify(main, n)
    return {"report_matches": v["report_matches"], "compile_outputs": v["compile_outputs"],
            "objects_present": v["compile_outputs_checked"] - v["compile_outputs_missing"],
            "objects_checked": v["compile_outputs_checked"], "pending": steps, "pending_note": note,
            "completed": completed, "verify": v,
            # `steps` is None when ninja cannot answer, and `not None` is True - so the unknown case must be
            # compared, not negated, or this field passes on the one reading that proves nothing
            "ok": bool(v["ok"] and steps == 0)}


def currency_lines(cur: dict) -> list[str]:
    """The load-bearing one-liner the handover prints: the proof, in the lane's own words.

    A lane should never have to guess whether its tree is current.  Every value is a measurement taken here
    (never a promise): the report bytes, the object count, and the pending count after completion.  The
    "proven current" header is printed **only when the check passed** (`cur["ok"]`, which is False for an
    unknown pending count): otherwise the same measured values are printed with the doubt named, because a
    header that asserts success over an unmeasured value is the lie this line exists to prevent.
    """
    objects = "%d/%d" % (cur["objects_present"], cur["objects_checked"])
    report = "byte-identical to MAIN's" if cur["report_matches"] else "DIFFERS from MAIN's"
    if cur["pending"] is None:
        pending = "unknown (%s)" % (cur["pending_note"] or "ninja could not run")
    elif cur["pending"] == 0:
        pending = "0"
    else:
        pending = "%d still pending%s" % (cur["pending"],
                                            " (%s)" % cur["pending_note"] if cur["pending_note"] else "")
    measured = ("`report.json` is %s; the official scorer's compile outputs are complete (%s objects "
                "present); `ninja -n` pending steps: %s." % (report, objects, pending))
    doubts = []
    if not cur["report_matches"]:
        doubts.append("`report.json` differs from MAIN's")
    if not cur["compile_outputs"]:
        doubts.append("the official scorer's compile outputs are incomplete")
    if cur["pending"] is None:
        doubts.append("ninja could not count the pending steps")
    elif cur["pending"]:
        doubts.append("%d ninja step(s) are still pending" % cur["pending"])
    if cur.get("completed") is False:
        doubts.append("the ninja run that was meant to finish them failed")
    if cur.get("ok"):
        lines = ["**Your build tree is proven current at handover.** " + measured]
    else:
        lines = ["**Build tree currency: NOT PROVEN at handover.** " + measured +
                 " Doubt: %s." % "; ".join(doubts or ["the currency check did not pass"])]
    if cur["completed"]:
        lines.append("The pending steps were run **in this slot** at acquire time; nothing outside it was "
                     "touched.")
    return lines


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
    p = subprocess.run(["git", "-C", slot, *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
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

    **The pool is not a scratch file.**  `--force` removes worktrees, so it **refuses while any slot in the
    pool holds live work** - a branch checked out, a RUNNING lane, or a live claim - naming each one.  Only
    that slot's own `release`/`reclaim` may touch it; a bulk repair must not delete four live lanes.
    """
    claims = _claims()
    if force:
        live = [s for s in all_slots(main)
                if s.get("attached") or s.get("live_claim") or s.get("run") or s.get("marked")]
        if live:
            raise SystemExit("REFUSED init --force: %d slot(s) still hold live work - the pool is the "
                             "campaign's concurrency cap, not a scratch file; release or reclaim each slot "
                             "first:\n%s" % (len(live), "\n".join(
                                 "  slot %d: %s" % (s["slot"], s["why"]) for s in live)))
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
                               capture_output=True, text=True, encoding="utf-8", errors="replace")
            git(["worktree", "prune"], main)
            git(["worktree", "add", "--detach", d, tip], main)
            created.append(n)
        if seed:
            notes.append((n, claims.seed_worktree_build(main, d, copy_orig=True, overwrite=False)))
    return {"main": main, "tip": tip, "count": count, "created": created, "present": present,
            "notes": notes, "slots": [slot_dir(main, n) for n in range(1, count + 1)]}


# --- shadow ---------------------------------------------------------------------------------------

def _rmtree_force(path: str) -> None:
    """`shutil.rmtree` that also clears git's read-only objects (Windows denies unlink on them)."""
    def _onexc(func, p, exc):
        os.chmod(p, 0o700)
        func(p)

    try:
        shutil.rmtree(path, onexc=_onexc)
    except TypeError:                              # Python < 3.12 spells it `onerror`
        shutil.rmtree(path, onerror=lambda f, p, e: (os.chmod(p, 0o700), f(p)))


def shadow(main: str, slot: int, dest: str, seed_build: bool = False, force: bool = False) -> dict:
    """Materialise a **lane-shaped tree inside a slot** for fixture tests - without `git worktree add`.

    A fixture needs a tree that behaves like a lane: a real git repository (so `git rev-parse
    --show-toplevel`, `unitutil.repo_root()` and every branch helper resolve it) with the lane's directory
    layout.  `git worktree add` is the obvious way and the **wrong** one here: it writes metadata into the
    *common* git dir (`<MAIN>/.git/worktrees/...`), which is outside the slot - and a slot must contain
    everything it owns (a release `clean -ffdx`es the slot; a teardown must never reach a neighbour or
    MAIN).  `shadow` instead materialises the slot's own committed tree (`git archive HEAD`) into
    `<slot>/<dir>` and `git init`s it **there**, so every byte - `.git/` included - is inside the slot and
    no worktree is registered anywhere.

    `seed_build=True` also copies the warm build/input trees (`build/RMHE08`, `build/tools`, the compiler
    and binutils, `orig/RMHE08`, `tools/m2c`) so the shadow can compile and score; the default is
    source-only, which is what a fixture usually needs and is far smaller than the whole warm tree.

    Refuses (fail closed) when the destination is not **inside** the slot (a resolved-path comparison, so a
    symlink or junction cannot walk out), when it exists and is non-empty (unless `force`), and when the
    slot is **not free** (unless `force`) - nothing consults the `.used` marker on the way in, so a shadow
    written into a live lane's tree would show up in that lane's `git status` and its landing would refuse
    on a dirty tree.  The shadow is scratch: the next reset's `clean -ffdx` discards it, and a caller should
    remove it.
    """
    d = os.path.abspath(slot_dir(main, slot))
    if not os.path.exists(os.path.join(d, ".git")):
        raise SystemExit("REFUSED shadow: slot %d is not a worktree (%s)" % (slot, d))
    dest_abs = os.path.abspath(dest if os.path.isabs(dest) else os.path.join(d, dest))
    # realpath on BOTH sides: a prefix test on the lexical path is walked by a symlink/junction, and it
    # refused a destination reached through one (the review's finding 5, slots.py 2026-09-28)
    real_d = os.path.normcase(os.path.realpath(d))
    real_dest = os.path.normcase(os.path.realpath(dest_abs))
    if real_dest == real_d or not real_dest.startswith(real_d + os.sep):
        raise SystemExit("REFUSED shadow: %s is not INSIDE slot %d (%s) - a shadow lives in its slot, "
                         "never in MAIN or a sibling slot" % (dest_abs, slot, d))
    state = slot_state(main, slot)
    if not state["free"] and not force:
        raise SystemExit("REFUSED shadow: slot %d is not free (%s) - a shadow inside a lane's tree shows up "
                         "in ITS `git status` and refuses the lane's landing; pass --force only for a slot "
                         "you hold" % (slot, state["why"]))
    if os.path.exists(dest_abs):
        if os.listdir(dest_abs) and not force:
            raise SystemExit("REFUSED shadow: %s exists and is not empty (--force to replace it)" % dest_abs)
        if force:
            _rmtree_force(dest_abs)
    p = subprocess.run(["git", "-C", d, "archive", "--format=tar", "HEAD"], capture_output=True)
    if p.returncode != 0:
        raise SystemExit("REFUSED shadow: cannot read slot %d's tree: %s"
                         % (slot, (p.stderr or b"").decode("utf-8", "replace").strip()))
    os.makedirs(dest_abs, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(p.stdout)) as tf:
        try:
            tf.extractall(dest_abs, filter="data")
        except TypeError:                      # Python < 3.12 has no `filter`
            tf.extractall(dest_abs)
        files = sum(1 for m in tf.getmembers() if m.isfile())
    seeded: list[str] = []
    if seed_build:
        claims = _claims()
        for rel in (claims.RMHE08_REL, os.path.join("build", "tools"), os.path.join("build", "compilers"),
                    os.path.join("build", "binutils"), claims.ORIG_REL, os.path.join("tools", "m2c")):
            src, dst = os.path.join(d, rel), os.path.join(dest_abs, rel)
            if os.path.exists(dst):
                continue
            if os.path.isdir(src):
                shutil.copytree(src, dst)
                seeded.append(rel.replace(os.sep, "/"))
            elif os.path.isfile(src):
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
                seeded.append(rel.replace(os.sep, "/"))
    git(["init", "-q", "-b", "main"], dest_abs)
    git(["add", "-A"], dest_abs)
    git(["-c", "user.email=shadow@example.invalid", "-c", "user.name=shadow", "-c", "commit.gpgsign=false",
         "commit", "-q", "-m", "shadow of slot %d" % slot], dest_abs)
    return {"slot": slot, "dir": dest_abs, "files": files, "seeded": seeded,
            "head": git(["rev-parse", "HEAD"], dest_abs)}


# --- reclaim: a slot whose checked-out branch is already LANDED -----------------------------------

#: The OID shape `git merge-tree --write-tree` prints first: the tree a merge would produce.
_TREE_OID = re.compile(r"[0-9a-f]{40}|[0-9a-f]{64}")


def _merge_tree_oid(returncode: int, stdout: str) -> str | None:
    """`merge-tree`'s result tree OID, or **None** when the run cannot be read as one.

    A nonzero exit is a **conflict** - the test never reached an answer - and a first line that is not a bare
    hex OID is not a tree either.  Both are ambiguity, and the caller must refuse rather than guess.  Split
    out as a pure function so the ambiguous cases can be pinned without a repository.
    """
    if returncode != 0:
        return None
    lines = (stdout or "").splitlines()
    if not lines or not _TREE_OID.fullmatch(lines[0].strip().lower()):
        return None
    return lines[0].strip().lower()


def merge_tree_of(main: str, ref: str) -> str | None:
    """The tree `git merge-tree --write-tree main <ref>` computes, or None when the test cannot answer.

    The free test the held-branch audit uses (CLAUDE.md): it touches no worktree and no index, so it is safe
    to run on a slot that is still checked out.  A conflict (nonzero exit) or anything unparseable is None.
    """
    p = subprocess.run(["git", "merge-tree", "--write-tree", "main", ref], cwd=main, capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    return _merge_tree_oid(p.returncode, p.stdout)


def branch_fully_applied(main: str, branch: str) -> bool | None:
    """Whether `branch`'s **content** is already in main - the documented free test, fail closed.

    `git merge-tree --write-tree main <branch>` merges in memory; when its result tree equals `main^{tree}`
    the branch adds no content and is fully applied.  This is the campaign's own test (CLAUDE.md, the
    held-branch drain): `git diff main <b>` cannot decide it, because main has moved and an already-landed
    branch shows a huge deletion diff and looks unlanded.

    **It compares TREES, not commits, and that is exactly why the one test covers both landing routes.**  A
    gate-landed branch (`land.py land --branch` applies its content with `git apply -3` and commits) stays
    *ahead of main by its own commits*, so any commit-based test would call it unlanded - but its tree is
    main's tree, so the equality holds.  The same is true of the direct path-limited landing.  Measured on
    three real slots (two gate-landed, one direct-landed): all three equal.  There is deliberately **no
    second, weaker "the branch's own paths look applied" test**: a heuristic like that can read a fully
    applied branch as catastrophe (the merge base predates main's progress) and, worse, could delete a
    branch that is not actually applied.

    -> True (applied), False (not applied), or **None** when the answer cannot be established (a conflict,
    an unparseable tree line, an unreadable `main^{tree}`, git unavailable).  The caller MUST treat None as
    "do not reclaim": acting on a doubt would detach an unlanded branch under a lane.
    """
    merged = merge_tree_of(main, branch)
    if merged is None:
        return None
    p = subprocess.run(["git", "rev-parse", "main^{tree}"], cwd=main, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    main_tree = (p.stdout or "").strip().lower()
    if p.returncode != 0 or not _TREE_OID.fullmatch(main_tree):
        return None
    return merged == main_tree


def reclaim_line(out: dict) -> str:
    """The one line a reclaim prints - what it did and why, in one sentence (never a newline)."""
    if not out.get("branch"):
        return ("slot %d: RECLAIMED - debris: a `.used` sentinel with no live claim and no branch checked "
                "out; cleared the sentinel and released the slot (a slot holds a directory, never a branch)"
                % out["slot"])
    return ("slot %d: RECLAIMED - branch %r is already fully applied to main "
            "(git merge-tree --write-tree main %s == main^{tree}); parked %s at %s, detached the worktree, "
            "deleted the branch and released the slot (a slot holds a directory, never a branch)"
            % (out["slot"], out["branch"], out["branch"], out["rescue_ref"],
               (out["rescue_tip"] or "?")[:8]))


def reclaim_verdict(main: str, n: int, branch: str | None = None, run_registry: str | None = None) -> dict:
    """Whether slot `n` is reclaimable, and why - **read-only** (it touches nothing).

    This is the one place the gate lives, shared by the action (`reclaim_slot`) and the dry run
    (`preview`), so a preview can never promise a reclaim the action would refuse.  It covers the four
    states a non-free slot can be in:

    * **(a)** a branch checked out whose **tree equals main's** (`branch_fully_applied`) and **no live
      claim** -> reclaimable (`kind="branch"`);
    * **(b)** a branch checked out whose tree **differs** -> refuse (today's message; the load-bearing guard);
    * **(c)** no branch, but a `.used` sentinel and **no live claim** -> reclaimable debris (`kind="debris"`);
    * **(d)** a **live claim** holds it (a claim-registry row names this slot, or a non-stale lock record
      does) -> refuse, whatever the worktree says: *never reclaim a slot with a live claim*.  The mirror
      case - a registry row with no sentinel - is the same refusal, and is why the registry is read at all.

    **Fail closed.**  If the claim registry cannot be read, or the merge-tree test cannot run / is ambiguous,
    the slot is refused.  `run_registry` is the harness' async-run registry (`release_blockers`' seam), not
    the claim registry.  `-> {"reclaimable", "reason", "branch", "applied", "kind", "live_claim"}`.
    """
    claims = _claims()
    d = slot_dir(main, n)
    v = {"reclaimable": False, "reason": "", "branch": branch, "applied": None,
         "kind": None, "live_claim": None}
    if not os.path.exists(os.path.join(d, ".git")):
        v["reason"] = "slot %d is not a worktree (%s)" % (n, d)
        return v
    by_slot, registry_ok = registry_claims_by_slot(main)
    if not registry_ok:
        v["reason"] = ("the claim registry (%s) cannot be read - liveness cannot be established, so the "
                        "slot is NOT reclaimed (fail closed)" % claims.registry_path(main))
        return v
    claim = by_slot.get(n)
    lock = read_lock(main, n)
    live_lock = bool(lock and not lock_stale(main, n, lock))
    if claim or live_lock:
        who = claim or lock
        v["live_claim"] = who
        v["reason"] = ("a live claim holds the slot: %s (%s, branch %s) - never reclaim a slot with live "
                        "work (the claim registry is the authority for liveness)"
                        % (who.get("unit") or "?", who.get("worker") or "?", who.get("branch") or "?"))
        return v
    attached = branch or slot_attached_branch(d)
    v["branch"] = attached
    if attached:
        v["applied"] = applied = branch_fully_applied(main, attached)
        if applied is None:
            v["reason"] = ("the merge-tree test for branch %r could not run or its output was ambiguous - "
                            "failing closed, NOT reclaiming" % attached)
            return v
        if not applied:
            v["reason"] = ("branch %r is NOT fully applied to main (git merge-tree --write-tree main %s != "
                            "main^{tree}) - it is unlanded work" % (attached, attached))
            return v
        v["kind"] = "branch"
    elif marker_present(d):
        v["kind"] = "debris"
    else:
        v["reason"] = "the slot holds no branch and no `.used` sentinel - nothing to reclaim"
        return v
    blockers = release_blockers(main, n, d, registry=run_registry)
    if blockers:
        v["reason"] = ("the slot is %s but not safe to reclaim: %s"
                        % (("on a fully applied branch" if attached else
                            "carrying a `.used` sentinel with no live claim"),
                           "; ".join(summary for summary, _ in blockers)))
        v["blockers"] = blockers
        return v
    v["reclaimable"] = True
    v["reason"] = ("branch %r is fully applied to main" % attached if attached else
                    "a `.used` sentinel with no live claim and no branch checked out (debris)")
    return v


def reclaim_slot(main: str, n: int, branch: str | None = None, unit: str | None = None,
                 registry: str | None = None) -> dict:
    """Reclaim slot `n` when the branch it holds is **already landed** - never one holding real work.

    The refusal "a slot holds a directory, never a branch" is load-bearing for an *unlanded* branch, which is
    a claim's live work; but a branch whose content is already in main is bookkeeping, not work in progress,
    and detaching/deleting it by hand after every landing cost a round trip.  This does the by-hand dance -
    park the rescue ref, detach, delete, release - as one named step.  It also reclaims **debris**: a `.used`
    sentinel on a detached worktree with no live claim (a crash remnant), which has no branch to rescue.

    **Fail closed.**  It acts only when `reclaim_verdict` says so: no live claim (the claim registry is the
    authority) and either a fully applied branch or debris, with `release_blockers` reporting nothing live (a
    RUNNING run) or unrecorded (a dirty tree).  A live claim, an unreadable claim registry, a conflict, an
    ambiguous merge-tree, an unlanded branch, a live run or a dirty tree all return `reclaimed=False` with
    the reason - the caller keeps its refusal.

    For a branch, the rescue ref is parked **before** the worktree is detached or the branch deleted (`steps`
    records the order), so a crash in the middle leaves the branch's tip at `refs/rescue/<slug>` and loses
    nothing.  `registry` is the harness' async-run registry (`release_blockers`' seam).
    """
    claims = _claims()
    d = slot_dir(main, n)
    out = {"slot": n, "dir": d, "branch": branch, "unit": unit, "reclaimed": False, "reason": "",
           "applied": None, "kind": None, "live_claim": None, "rescue_ref": None, "rescue_tip": None,
           "detached": False, "branch_deleted": False, "cleaned": [], "refreshed": None,
           "marker_cleared": False, "released": False, "steps": [], "line": None}
    verdict = reclaim_verdict(main, n, branch, run_registry=registry)
    out.update({"branch": verdict["branch"], "applied": verdict["applied"], "kind": verdict["kind"],
                "live_claim": verdict["live_claim"], "reason": verdict["reason"]})
    if verdict.get("blockers"):
        out["blockers"] = verdict["blockers"]
    if not verdict["reclaimable"]:
        return out
    if verdict["kind"] == "debris":
        # No branch to rescue, detach or delete: clear the sentinel, discard scratch, clear the lock.
        out["cleaned"] = clean_slot(d)
        out["marker_cleared"] = clear_marker(d)
        clear_lock(main, n)
        out["released"] = True
        out["steps"].append({"step": "release"})
        out["reclaimed"] = True
        out["line"] = reclaim_line(out)
        return out
    attached = verdict["branch"]
    # The rescue-ref name follows the EXISTING convention (`refs/rescue/<slug>`): the lock's unit when it
    # names one, else the branch's OWN slug read straight off the branch - never re-slugged, which would add
    # a second hash suffix and park the ref under a name no one looking for the branch would find.
    named_unit = unit or read_lock(main, n).get("unit")
    branch_slug = claims.slug_of_branch(attached)
    out["unit"] = named_unit or branch_slug or attached
    ref = claims.rescue_ref_name(named_unit) if named_unit else ("refs/rescue/%s" % (branch_slug or attached))
    tip = slot_head(d)
    # 1. the rescue ref FIRST: everything after this is repeatable, and a crash here loses nothing
    p = subprocess.run(["git", "update-ref", ref, attached], cwd=main, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if p.returncode != 0:
        out["reason"] = ("could not park the rescue ref %s (%s) - refusing to delete the branch"
                         % (ref, (p.stderr or "").strip()))
        return out
    out["rescue_ref"], out["rescue_tip"] = ref, tip
    out["steps"].append({"step": "rescue", "ref": ref, "tip": tip})
    # 2. detach the worktree: the branch is no longer checked out, so it can be deleted
    main_tip = git(["rev-parse", "HEAD"], main)
    git(["checkout", "-f", "-q", "--detach", main_tip], d)
    out["detached"] = True
    out["steps"].append({"step": "detach", "at": main_tip})
    # 3. delete the branch - the lock - now that its commits are parked
    git(["branch", "-D", attached], main)
    out["branch_deleted"] = True
    out["steps"].append({"step": "delete-branch", "branch": attached})
    # 4. release the slot: discard scratch, keep the warm trees, clear the marker and the lock
    out["cleaned"] = clean_slot(d)
    if claims._main_build_is_current(main):
        out["refreshed"] = claims.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
    out["marker_cleared"] = clear_marker(d)
    clear_lock(main, n)
    out["released"] = True
    out["steps"].append({"step": "release"})
    out["reclaimed"] = True
    out["reason"] = "branch %r was fully applied to main" % attached
    out["line"] = reclaim_line(out)
    return out

def _announce_reclaim(res: dict) -> None:
    """Print a reclaim's one line to stderr - diagnostics, never part of `spawn`'s stdout artifact."""
    if res.get("reclaimed") and res.get("line"):
        print(res["line"], file=sys.stderr)


# --- acquire -------------------------------------------------------------------------------------

def _pick_free(main: str, slot: int | None = None, claim=None, reclaim: bool = True) -> dict:
    """Pick a free slot, falling through to the next genuinely free one.

    `free` is the worktree truth (`slot_state`), so an occupied slot is **skipped**, never failed on.  With
    `claim`, each candidate is *atomically* marked (`.used`, `O_EXCL`) before it is returned - so two racing
    acquires cannot both take the same slot, and a slot a racing acquire just marked is skipped too.  The
    `claim` callback is also where `acquire` refuses a sentinel that names another owner
    (`marker_claim_conflict`), i.e. *before* the mark, so a foreign claim is never silently absorbed.

    A slot explicitly targeted with `slot=N` is a search *by name*, not a search: it refuses when occupied -
    the "a slot holds a directory, never a branch" rule - or when the mark cannot be taken.

    `reclaim` is what makes a slot occupied by a **landed** branch recoverable instead of a refusal: a
    genuinely free slot is always taken first (no side effect), and only when none is free is a slot whose
    checked-out branch is *proven* fully applied to main (and no live claim holds it) reclaimed
    (`reclaim_slot`) and taken.  A slot holding an unlanded branch, or one a live claim holds, is still
    refused with the same message, verbatim.  `preview` passes `reclaim=False`, because a dry run must touch
    nothing.

    **The claim registry is read here and gates the whole search (fail closed).**  If `.pi/claims.json` cannot
    be parsed, no slot is handed out: a slot's liveness would be unknown and this tool must not guess.
    """
    by_slot, registry_ok = registry_claims_by_slot(main)
    if not registry_ok:
        raise SystemExit("REFUSED: the claim registry (%s) cannot be read - no slot can be handed out while "
                         "liveness is unknown (fail closed)" % _claims().registry_path(main))
    rows = all_slots(main, claims_by_slot=by_slot)
    if not rows:
        raise SystemExit("REFUSED: the slot pool is not initialised - run `python tools/units/slots.py init`")
    if slot is not None:
        row = next((s for s in rows if s["slot"] == slot), None)
        if row is None:
            raise SystemExit("REFUSED: no slot %d (the pool is slots 1..%d)" % (slot, len(rows)))
        if not row["free"] and reclaim:
            res = reclaim_slot(main, slot, branch=row.get("attached"))
            if res["reclaimed"]:
                _announce_reclaim(res)
                row = dict(slot_state(main, slot, claims_by_slot=by_slot), landed_reclaim=res)
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
    # A genuinely free slot first: taking one has no side effect, so reclaiming is only the fallback.
    for row in rows:
        if not row["free"]:
            continue
        if claim is None or claim(row):
            return row
    # Nothing free: reclaim a slot whose checked-out branch is ALREADY LANDED and unclaimed (safe), then
    # take it.  A debris slot is already `free` and was taken in the pass above.
    if reclaim:
        for row in rows:
            if row["free"] or not row.get("attached"):
                continue
            res = reclaim_slot(main, row["slot"], branch=row["attached"])
            if not res["reclaimed"]:
                continue
            _announce_reclaim(res)
            row = dict(slot_state(main, row["slot"], claims_by_slot=by_slot), landed_reclaim=res)
            if claim is None or claim(row):
                return row
    raise SystemExit("REFUSED: %s" % capacity_error(main))


def preview(main: str, unit: str, branch: str | None = None, slot: int | None = None,
            worker: str | None = None, force: bool = False) -> dict:
    """What `acquire` would do, touching nothing (for a dry run). -> the slot row and the command.

    The candidate slot's sentinel is checked here too, so a dry run surfaces the same
    "that `.used` belongs to another claim" refusal the real acquire would give.  A slot holding a **fully
    applied** branch is one `acquire` will reclaim; a dry run says so (in `reclaim`) without reclaiming,
    because it must touch nothing.
    """
    claims = _claims()
    unit = claims.norm_unit(unit.strip("/"))
    branch = branch or claims.branch_for(unit)
    reclaimed = None
    try:
        row = _pick_free(main, slot, reclaim=False)
    except SystemExit:
        if slot is None:
            raise
        verdict = reclaim_verdict(main, slot)
        if not verdict["reclaimable"]:
            raise
        reclaimed = {"slot": slot, "branch": verdict["branch"]}
        row = slot_state(main, slot)
    # A predicted reclaim clears the slot's `.used` marker before the claim is marked, so the foreign-marker
    # refusal does not apply to that path - checking it against the pre-reclaim state would be a false refusal.
    conflict = None if reclaimed else marker_claim_conflict(
        row["dir"], owner_label(unit, branch, worker), row["slot"])
    if conflict and not force:
        raise SystemExit(conflict.split("\n")[0] + "\n  (a dry run reports the same refusal `acquire` gives)")
    tip = git(["rev-parse", "HEAD"], main)
    return {"slot": row["slot"], "dir": row["dir"], "branch": branch, "base": tip,
            "owner": owner_label(unit, branch, worker), "reclaim": reclaimed,
            "command": "git -C %s checkout -B %s %s" % (row["dir"], branch, tip)}


def acquire(main: str, unit: str, branch: str | None = None, worker: str | None = None,
            slot: int | None = None, force: bool = False, ninja_runner=None) -> dict:
    """Take a slot for `unit`: mark it `.used`, reset it, cut a **fresh** branch off main's tip, verify, lock.

    Refuses, before touching anything, when the unit's branch already exists (the claim is taken), when an
    explicitly named slot is occupied - a slot whose previous branch has not landed is surfaced loudly, never
    silently reused - and when the slot's `.used` sentinel **names a different owner**
    (`marker_claim_conflict`): that is another claim's sentinel, and taking the slot under it is how a lane's
    HEAD got detached mid-run.  `force` is the deliberate override for a sentinel whose owner is gone.

    A slot whose worktree holds a branch whose **content is already fully applied to main** is NOT refused:
    `_pick_free` reclaims it first (`reclaim_slot` - rescue ref, detach, delete, release) and takes it, and
    the reclaimed record rides back in `landed_reclaim`.  Only an *unlanded* (or unprovable) branch keeps
    the refusal.

    The slot is **marked `.used` atomically before the reset**, with this claim's OWNER label in the marker,
    so a racing acquire falls through to the next free slot instead of colliding; a failure after the mark
    removes it, and a crash leaves a marker the worktree reading reclaims.  The kept build tree is verified
    against MAIN's current map/DOL; if it cannot be proven current it is re-seeded, and if it still cannot,
    the acquire fails closed rather than handing the lane a stale tree.

    **Claim-time currency.**  A fresh slot is always a few ninja steps behind by construction, so acquire
    finishes them here (`claim_currency`) and records the proof - report bytes, the compile-output set, the
    pending count - in the lock and the returned `currency`, which `spawn` and the CLI print.  The lane is
    never handed a tree whose pending work is unknown.  `ninja_runner` is the injection point (selftest).
    """
    claims = _claims()
    unit = claims.norm_unit(unit.strip("/"))
    branch = branch or claims.branch_for(unit)
    if claims.branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)
    owner = owner_label(unit, branch, worker)

    def claim(row: dict) -> bool:
        conflict = marker_claim_conflict(row["dir"], owner, row["slot"])
        if conflict and not force:
            raise SystemExit(conflict)
        # reclaim a crash remnant (a marker on a detached worktree) before marking, or it would never free
        if row.get("marker_stale"):
            clear_marker(row["dir"])
        return mark_used(row["dir"], unit, branch, owner)

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
                # Two of `verify`'s three checks are hard doubts and still refuse: MAIN's own tree being
                # stale, and the slot's report differing from MAIN's by even a byte (a stale report is how a
                # lane measures the wrong build).  The third is `build_current`, which compares the SLOT's
                # `config.json` mtime against MAIN's map/DOL inputs - inputs that are newer whenever MAIN's
                # own map/DOL moved after MAIN's last build.  That is a property of MAIN, not of a copy that
                # was just made from it, and no re-seed can ever satisfy it, so it refused forever: a lane
                # lost ~25 minutes here proving by hand what `report_matches` proves in a second.
                # When MAIN is current AND the report is byte-identical, accept and SAY SO - never silently.
                if not v["main_build_current"]:
                    raise SystemExit("REFUSED slot %d: MAIN's own build tree is not current (%s);\n"
                                     "  run `ninja` in MAIN so the slot can be re-seeded - never hand a lane a doubt"
                                     % (n, "; ".join(v["reasons"])))
                if not v["report_matches"]:
                    raise SystemExit("REFUSED slot %d: slot build/RMHE08/report.json differs from MAIN's - "
                                     "a stale report is how a lane measures the wrong build (%s)"
                                     % (n, "; ".join(v["reasons"])))
                if not v["compile_outputs"]:
                    # A third hard doubt, with the same shape as the report: `report.json` can be
                    # byte-identical while the objects it was computed from are gone, and a lane handed
                    # that tree cannot run the official scorer.  The re-seed above already refilled every
                    # output MAIN holds, so one still missing is a real gap, not config.json's mtime.
                    raise SystemExit("REFUSED slot %d: the slot is missing compile outputs the official "
                                     "scorer reads (%s)\n  re-seed it (release then acquire) before a lane "
                                     "lands" % (n, "; ".join(v["reasons"])))
                print("slot %d: MAIN's tree is current and report.json is byte-identical to MAIN's, so the only "
                      "remaining doubt is config.json's mtime (%s) - accepting; a stale build would have failed "
                      "report_matches" % (n, "; ".join(v["reasons"])))
                v = dict(v, ok=True, accepted_mtime_note=True)
        # Finish (or at least measure) the pending ninja work here, and record the proof.  The count is
        # taken after the verify/repair above, so it describes the tree the lane is actually handed.
        cur = claim_currency(main, n, v=v, runner=ninja_runner)
        v = cur["verify"]
        write_lock(main, n, {"slot": n, "dir": d, "unit": unit, "branch": branch,
                             "worker": worker or os.environ.get("USERNAME") or os.environ.get("USER") or "unknown",
                             "base": tip, "acquired_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                             "verified": v["ok"], "report_matches": v["report_matches"],
                             "compile_outputs": v["compile_outputs"], "pending": cur["pending"]})
        return {"slot": n, "dir": d, "worktree": d, "branch": branch, "base": tip, "unit": unit,
                "seeded": seed_note, "refreshed": refreshed, "verify": v, "currency": cur,
                "reclaimed": reclaimed, "landed_reclaim": row.get("landed_reclaim")}
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
            dry_run: bool = False, force: bool = False, allow_dirty: bool = False,
            registry: str | None = None) -> dict:
    """Return a slot to main's tip: detach, clean, delete its branch (rescue-ref first), refresh, unlock.

    Keeps the warm trees and leaves the slot pre-warmed so the next `acquire` is a validation, not a build.
    The branch is deleted because a *directory* never holds one; its commits are rescued first exactly as
    `claims.release` does, so a release can never be the only place unlanded work lived.

    **It fails closed.**  A release detaches HEAD, runs `clean -ffdx` and deletes the branch, so everything
    `release_blockers` can see - a RUNNING Claude session whose cwd is the slot (read from the harness' run
    registry: a lock cannot see a lane), a dirty tree, commits no branch reaches - refuses it, naming every
    reason, and `force` is the deliberate override that also states what it overrode.  On 2026-09-28 a
    release ran while another lane was still working in that slot: it detached HEAD under the live process
    and the lane's work was destroyed.  `allow_dirty` is for a caller that has *already* proven the work is
    recorded elsewhere (`claims.release` refuses un-merged, un-recorded work before it gets here); it never
    covers the live-run or orphaned-commit guards.
    """
    claims = _claims()
    n, lock = _resolve_slot(main, slot, unit, branch)
    d = slot_dir(main, n)
    branch = branch or lock.get("branch")
    unit = unit or lock.get("unit") or (claims.slug_of_branch(branch) if branch else None)
    result = {"slot": n, "dir": d, "branch": branch, "unit": unit, "detached": False, "cleaned": [],
              "branch_deleted": False, "rescue_ref": None, "refreshed": None, "lock_cleared": False,
              "marker_cleared": False, "dry_run": dry_run, "forced": bool(force), "overridden": []}
    if not os.path.exists(os.path.join(d, ".git")):
        raise SystemExit("REFUSED release slot %d: %s is not a worktree" % (n, d))
    # read before anything is touched: what is in the slot is the only thing that can say "not yet"
    blockers = release_blockers(main, n, d, allow_dirty=allow_dirty, registry=registry)
    if blockers and not force:
        raise SystemExit(release_refusal(main, n, d, blockers))
    result["overridden"] = [summary for summary, _detail in blockers]
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


# --- spawn ---------------------------------------------------------------------------------------

def profile_for_kind(kind: str) -> str:
    """The agent profile for a lane's kind of work, or a refusal **listing the valid kinds**.

    `KIND_PROFILE` is the whole mapping - one kind per lane - and a kind it does not know is refused rather
    than defaulted: guessing `decompiler` is exactly how a tooling lane was handed unit policy it could
    never satisfy.  The refusal names the valid kinds so the caller can correct the one word it got wrong
    without reading this file.
    """
    try:
        return KIND_PROFILE[kind]
    except KeyError:
        # no `spawn:` here: the same refusal is now raised from `claims.py claim --kind` and
        # `queue.py next --kind`, neither of which spawns anything
        raise SystemExit("REFUSED: unknown kind %r - valid kinds are: %s"
                         % (kind, ", ".join(KIND_PROFILE)))


def tree_block(main: str, path: str) -> str:
    """The standard "your tree" block every lane already gets - reused verbatim, never re-worded here.

    `brief._your_tree_lines` owns the cwd, the "write nothing outside it" rule and the
    `git rev-parse --show-toplevel` self-check ("STOP and report it instead of working");
    `brief._teardown_lines` owns the `claims.py release` ban.  Both renderers the claim path uses call those
    two helpers, so calling them here is the *one* copy of the paragraphs - a second variant pasted into
    `spawn` would be the drift this is meant to prevent.  The one clause those helpers do not carry - do not
    land - is added in the wording `brief.py`'s own §6 rules use.
    """
    from units import brief
    lines: list[str] = []
    brief._your_tree_lines(lines, {"worktree": path, "main": main})
    brief._teardown_lines(lines)
    lines.append("**Do not land.** A worker never runs `land.py` and never commits on `main` - landing is the")
    lines.append("orchestrator's job, exactly as teardown is.")
    return "\n".join(lines).strip("\n")


def spawn(main: str, kind: str, slot: int | None = None, unit: str | None = None,
          task: str | None = None, task_file: str | None = None, worker: str | None = None,
          force: bool = False) -> dict:
    """Take a slot for a lane of `kind` and return the paste-ready launch: the headless `claude --agent ...`
    line (`lanecmd.lane_call`, run with its cwd at the slot) followed by the standard "your tree" block.

    **The slot is an explicit launch parameter.**  `slot=N` takes that slot *by number* through `acquire` -
    same fail-closed reset/seed, same refusal for a slot holding an **unlanded** branch, never a
    reimplementation - and no `slot` takes the first genuinely free slot and **names which one it took**, so
    the orchestrator chooses the slot and can see it instead of two racing lanes choosing the same one.  A
    slot holding a branch whose content is already landed is reclaimed on the way in (see `reclaim_slot`),
    so a landing no longer costs a hand teardown before the next spawn.

    **`kind` is the mapping.**  It decides the agent profile (`profile_for_kind`) and is both printed and
    recorded: printed in the returned header/JSON, recorded in the slot's lock (`kind`/`agent`) and in the
    default claim name, so the lane's own branch says what it is.

    `unit` names the claim (and therefore the branch); it defaults to a timestamped `lane/<kind>-<...>` so a
    caller that knows only its slot and kind still gets a complete line.  `task`/`task_file` supply the text
    the lane is handed; without one the line carries a visible placeholder rather than an empty task.
    """
    profile = profile_for_kind(kind)
    if task is None and task_file:
        try:
            with open(task_file, encoding="utf-8") as fh:
                task = fh.read().strip()
        except OSError as exc:
            raise SystemExit("REFUSED spawn: cannot read --task-file %s: %s" % (task_file, exc))
    unit = unit or "lane/%s-%s" % (kind, time.strftime("%Y%m%d-%H%M%S"))
    info = acquire(main, unit, worker=worker or os.environ.get("USERNAME") or os.environ.get("USER") or "unknown",
                   slot=slot, force=force)
    path = info["dir"]
    # record the mapping in the slot's lock, so `status` and a later reader see the profile the lane was
    # launched with, not only the branch it happens to hold.
    lock = read_lock(main, info["slot"])
    if lock:
        lock["kind"] = kind
        lock["agent"] = profile
        write_lock(main, info["slot"], lock)
    tail = ("End your turn with your report: your final message is the result the orchestrator receives. "
            "If you need a ruling, end the turn with the request - the orchestrator resumes this session.")
    if task:
        task_text = "%s\n\n%s" % (task, tail)
    else:
        task_text = ("No task text was given (`--task-file` was not passed). Replace this placeholder with "
                     "the lane's task. " + tail)
    launch = lanecmd.lane_call(profile, path, task_text, name="%s-slot%d" % (profile, info["slot"]),
                               main=main, key="slot%d" % info["slot"])
    call = launch["call"]
    if lock:
        lock["session_id"] = launch["session_id"]
        write_lock(main, info["slot"], lock)
    # The claim-time proof goes INTO the lane's block, because the lane is who must not have to guess
    # whether its tree is current - the acquire-time completion happened before this line was rendered.
    block = "\n\n".join([tree_block(main, path)] + currency_lines(info["currency"]))
    return {"slot": info["slot"], "path": path, "agent": profile, "kind": kind,
            "branch": info["branch"], "unit": unit, "sessionId": launch["session_id"],
            "spawnLine": "%s\n\n%s" % (call, block)}


# --- status --------------------------------------------------------------------------------------

def status(main: str, registry: str | None = None) -> list[dict]:
    """Every slot's row, plus its build tree's verdict, its HEAD and any live run in it."""
    rows = []
    live = live_runs(registry)
    for s in all_slots(main, runs=live):
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

    def _raises(fn) -> bool:
        try:
            fn()
            return False
        except SystemExit:
            return True

    import contextlib
    import shutil
    import tempfile
    claims = _claims()

    # --- the ninja seam: deterministic and hermetic, and it exercises the pending>0 -> complete -> 0 path --
    global NINJA_RUNNER
    saved_ninja = NINJA_RUNNER

    def fake_ninja(pending=()):
        """A ninja runner: `-n` reports the next count in `pending` (then 0); a real run succeeds.

        `pending` is a queue of *pending-step counts*: `[3]` means the first `ninja -n` saw 3 steps and,
        once `ninja` ran, the re-count saw 0 - the exact fresh-slot shape.  `["error"]` / `["missing"]`
        simulate ninja failing to answer.
        """
        counts = list(pending) if pending else [0]
        calls = []

        class Done:
            pass

        def run(argv, cwd=None, **kw):
            calls.append(list(argv))
            p = Done()
            p.stdout, p.stderr = "", ""
            if list(argv) == ["ninja", "-n"]:
                n = counts.pop(0) if counts else 0
                if n == "error":
                    p.returncode, p.stderr = 1, "ninja: fatal: build.ninja, line 1: unknown rule"
                elif n == "missing":
                    raise FileNotFoundError("ninja")
                else:
                    p.returncode = 0
                    p.stdout = ("".join("[%d/%d] step\n" % (i + 1, n) for i in range(n))
                                if n else "ninja: no work to do.\n")
            else:
                p.returncode, p.stdout, p.stderr = 0, "", ""
            return p

        run.calls = calls
        return run

    # every acquire/spawn below reads the currency seam; a hermetic 0-pending answer stands in for ninja
    NINJA_RUNNER = fake_ninja()

    # the counting itself, before any fixture: dry-run lines -> the pending count; failure/absence -> unknown
    check("ninja -n output is counted", ninja_pending("/nonexistent", runner=fake_ninja([3]))[0], 3)
    check("... and 'no work to do' is 0", ninja_pending("/nonexistent", runner=fake_ninja([0]))[0], 0)
    check("... a failing ninja is UNKNOWN, not 0", ninja_pending("/nonexistent", runner=fake_ninja(["error"]))[0],
          None)
    check("... and it says why", "ninja -n failed" in ninja_pending("/nonexistent",
                                                                     runner=fake_ninja(["error"]))[1], True)
    check("... an absent ninja is UNKNOWN, not 0", ninja_pending("/nonexistent",
                                                                runner=fake_ninja(["missing"]))[0], None)

    def g(path, *args, check=True):
        p = subprocess.run(["git", "-c", "user.email=t@example.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True, encoding="utf-8", errors="replace")
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
        # CLAUDE.md is committed with a real **em dash** in its prose, because a slot's CLAUDE.md is dirty by
        # design (the LOCAL-ONLY block is live working state, rule 8) and `slot_dirty` has to tell that dirt
        # apart from real dirt.  The non-ASCII byte is what makes the comparison load-bearing: a decode that
        # used the host locale codec (`cp1252` here) would call every slot dirty (F34's failure, one file
        # over).  Written as UTF-8 explicitly - the fixture must not inherit the trap it guards against.
        with open(os.path.join(repo, "CLAUDE.md"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write("# repo notes\n\nprose with an em dash \u2014 in it\n")
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
        os.makedirs(os.path.join(repo, "build", "RMHE08", "src", "auto"))
        open(os.path.join(repo, "build", "RMHE08", "src", "auto", "stub.o"), "wb").write(b"SRC-OBJ-V1\n")
        open(os.path.join(repo, "build", "RMHE08", "config.json"), "w").write('{"libs": []}\n')
        open(os.path.join(repo, "build", "RMHE08", "report.json"), "w").write('{"units": {"auto/stub": 1.0}}\n')
        open(os.path.join(repo, "build.ninja"), "w").write("# ninja\n")
        open(os.path.join(repo, ".ninja_deps"), "wb").write(claims._ninja_deps_serialize(4, [
            ("path", os.path.abspath(repo).replace("\\", "/").encode() + b"/include/types.h")]))
        open(os.path.join(repo, ".ninja_log"), "w").write("# ninja log v5\n")
        # The manifest the official scorer opens: `target_path` is the split object, `base_path` the src
        # compile output.  A real one (both objects exist) so `compile_outputs` has something to assert.
        open(os.path.join(repo, "objdiff.json"), "w").write(json.dumps({"units": [
            {"name": "main/auto/stub", "target_path": "build/RMHE08/obj/auto/stub.o",
             "base_path": "build/RMHE08/src/auto/stub.o"}]}, indent=1) + "\n")
        open(os.path.join(repo, "compile_commands.json"), "w").write("[]\n")
        g(repo, "init", "-q")
        g(repo, "checkout", "-q", "-b", "main")
        g(repo, "add", "-A")
        g(repo, "commit", "-q", "-m", "base")
        # mtimes: the split is newer than its inputs, the manifest newer than configure (the guard's rule)
        for p, t in (("config/RMHE08/symbols.txt", 1_000_000), ("config/RMHE08/splits.txt", 1_000_000),
                     ("orig/RMHE08/sys/main.dol", 1_000_000), ("orig/RMHE08/files/mh3.sel", 1_000_000),
                     ("configure.py", 1_000_000), ("build/RMHE08/config.json", 2_000_000),
                     ("build/RMHE08/obj/auto/stub.o", 2_000_000),
                     ("build/RMHE08/src/auto/stub.o", 2_000_000), ("build/RMHE08/report.json", 2_000_000),
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

        # (0) THE COMPILE-OUTPUT SET: `config.json`'s mtime and `report.json`'s bytes can both pass while
        # the objects the report was computed from are gone - and that tree cannot run the official scorer.
        # `objdiff.json` names the scorer's inputs (`target_path` = the split object, `base_path` = the
        # `src/` compile output); `verify` asserts every one of them is on disk.
        v0 = verify(repo, 1)
        check("verify asserts the scorer's compile outputs", v0["compile_outputs"], True)
        check("... counting the split and the src object", v0["compile_outputs_checked"], 2)
        gone = os.path.join(slot_dir(repo, 1), "build", "RMHE08", "src", "auto", "stub.o")
        os.remove(gone)
        v0 = verify(repo, 1)
        check("a slot missing a src/ compile output is REFUSED", v0["ok"], False)
        check("... naming the missing object",
              any("compile outputs" in r and "src" in r for r in v0["reasons"]), True)
        check("... while report.json still matches MAIN (the old check would have passed it)",
              v0["report_matches"], True)
        check("... and the missing count is reported", v0["compile_outputs_missing"], 1)
        # a re-seed refills it from MAIN (the arbiter) - verify is green again, so this is repair, not a wedge
        claims.seed_worktree_build(repo, slot_dir(repo, 1), copy_orig=True, overwrite=False)
        check("re-seeding refills the dropped output, and verify is green", verify(repo, 1)["ok"], True)
        # the slot's own manifest is a doubt too: the scorer reads the slot's copy, not MAIN's
        mf = os.path.join(slot_dir(repo, 1), "objdiff.json")
        os.remove(mf)
        v0 = verify(repo, 1)
        check("a slot missing its objdiff.json is refused even with every object present", v0["ok"], False)
        check("... naming the manifest",
              any("objdiff.json" in r for r in v0["reasons"]), True)
        claims.seed_worktree_build(repo, slot_dir(repo, 1), copy_orig=True, overwrite=False)
        check("... and re-seeding restores it", verify(repo, 1)["ok"], True)
        # claim-time currency: the pending steps a fresh slot ALWAYS has by construction are run here, then
        # re-counted - so the number the lane is shown is what is left, not what was
        nrun = fake_ninja([3])
        cur = claim_currency(repo, 1, complete=True, runner=nrun)
        check("claim_currency runs the pending ninja steps", ["ninja"] in nrun.calls, True)
        check("... and re-counts to 0", cur["pending"], 0)
        check("... recording that it completed them", cur["completed"], True)
        check("... and re-verifying the tree it just touched", cur["verify"]["ok"], True)
        check("... while proving the report and the object set",
              (cur["report_matches"], cur["compile_outputs"], cur["objects_present"]), (True, True, 2))
        # count-only leaves the work to the caller and says it did not do it
        cur2 = claim_currency(repo, 1, complete=False, runner=fake_ninja([2]))
        check("claim_currency(count-only) reports the pending count", cur2["pending"], 2)
        check("... and does not claim to have completed them", cur2["completed"], None)
        # the handover line carries the proof; an unknown is never rounded down to a confident 0
        proof = "\n".join(currency_lines(cur))
        check("the handover proof names report.json", "byte-identical to MAIN's" in proof, True)
        check("... the object count", "(2/2 objects present)" in proof, True)
        check("... and 0 pending steps", "pending steps: 0" in proof, True)
        unknown = "\n".join(currency_lines({"report_matches": True, "compile_outputs": True,
                                            "objects_present": 2, "objects_checked": 2,
                                            "pending": None, "pending_note": "ninja unavailable",
                                            "completed": None}))
        check("an unknown pending count is reported as unknown, not 0",
              "pending steps: unknown" in unknown, True)
        # `ok` is the field a future caller trusts, and the UNKNOWN pending count must not pass it:
        # `not None` was True, so the currency check passed on the one reading that proves nothing
        check("pending work is not `ok`", cur2["ok"], False)
        cur3 = claim_currency(repo, 1, complete=True, runner=fake_ninja(["error"]))
        check("... and neither is an UNKNOWN pending count", cur3["ok"], False)
        check("... reported as None, never rounded to 0", cur3["pending"], None)
        doubtful = "\n".join(currency_lines(cur3))
        check("... so the handover block never claims currency",
              "proven current at handover" in doubtful, False)
        check("... naming the doubt and the measured values instead",
              "NOT PROVEN" in doubtful and "pending steps: unknown" in doubtful, True)
        check("... while the proven case still says it", "proven current at handover" in proof, True)
        # idempotent
        again = init(repo, count=2)
        check("init is idempotent", (again["created"], sorted(again["present"])), ([], [1, 2]))

        # THE CAP IS pool.json's COUNT, the authority - a broken slot must not drop a LIVE one from the
        # enumeration (slot 2's missing worktree used to shrink a pool of 6 to `slots 1..5`, hiding slot 6).
        check("pool_size reads the manifest's count", pool_size(repo), 2)
        moved = slot_dir(repo, 2) + ".moved-for-test"
        os.rename(slot_dir(repo, 2), moved)
        try:
            rows2 = all_slots(repo)
            check("... every slot in the manifest is still enumerated when one has no worktree",
                  [r["slot"] for r in rows2], [1, 2])
            check("... and the broken one says `no worktree`", rows2[1]["state"], "no worktree")
            check("... so a live slot is never hidden behind a broken one", rows2[0]["exists"], True)
        finally:
            os.rename(moved, slot_dir(repo, 2))

        # (0b) A SHADOW: a lane-shaped tree INSIDE the slot, for fixture tests.  `git worktree add` is the
        # obvious way and the wrong one - it writes metadata into MAIN's `.git/worktrees`, outside the slot.
        wts_before = [line for line in g(repo, "worktree", "list", "--porcelain").splitlines()
                      if line.startswith("worktree ")]
        shd = shadow(repo, 1, "fixture")
        check("shadow materialises INSIDE the slot", os.path.normcase(shd["dir"]),
              os.path.normcase(os.path.join(slot_dir(repo, 1), "fixture")))
        check("... as a lane-shaped tree (configure.py + src/)",
              os.path.exists(os.path.join(shd["dir"], "configure.py"))
              and os.path.isdir(os.path.join(shd["dir"], "src")), True)
        check("... with its OWN repository inside the slot", os.path.isdir(os.path.join(shd["dir"], ".git")),
              True)
        check("... resolving like a lane (rev-parse --show-toplevel)",
              os.path.normcase(g(shd["dir"], "rev-parse", "--show-toplevel")),
              os.path.normcase(shd["dir"]))
        check("... and MAIN registered NO new worktree (nothing outside the slot was touched)",
              [line for line in g(repo, "worktree", "list", "--porcelain").splitlines()
               if line.startswith("worktree ")], wts_before)
        check("shadow refuses a destination OUTSIDE the slot",
              _raises(lambda: shadow(repo, 1, os.path.join(tmp, "outside"))), True)
        check("shadow refuses a non-empty destination", _raises(lambda: shadow(repo, 1, "fixture")), True)
        check("... unless --force replaces it", shadow(repo, 1, "fixture", force=True)["files"] > 0, True)
        _rmtree_force(os.path.join(slot_dir(repo, 1), "fixture"))
        # the containment test compares RESOLVED paths: a lexical prefix test refuses a destination reached
        # through a symlink (and cannot see one that walks out).  Windows needs a privilege for `symlink`,
        # so this row is skipped where the platform refuses it.
        link = os.path.join(tmp, "slot-link")
        try:
            os.symlink(slot_dir(repo, 1), link, target_is_directory=True)
        except (OSError, NotImplementedError, AttributeError):
            link = None
        if link:
            check("shadow accepts a destination inside the slot through a symlinked path",
                  shadow(repo, 1, os.path.join(link, "fixture"))["files"] > 0, True)
            _rmtree_force(os.path.join(slot_dir(repo, 1), "fixture"))

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
        check("acquire finishes the claim-time pending work", info["currency"]["pending"], 0)
        check("... and records the currency proof on the lock",
              (read_lock(repo, 1).get("pending"), read_lock(repo, 1).get("compile_outputs")), (0, True))
        check("no src diff against main", g(d1, "diff", "--name-only", "main", "--", "src"), "")
        check("git status in the slot is clean", g(d1, "status", "--porcelain"), "")
        check("acquire creates the `.used` marker", marker_present(d1), True)
        check("... and the marker is gitignored (the tree stays clean)",
              g(d1, "status", "--porcelain", "-uall"), "")
        # A SHADOW INTO A LANE'S SLOT IS REFUSED: nothing consulted the `.used` marker, so a fixture could
        # be written into a live lane's tree - that lane's `git status` then shows an untracked directory
        # and its landing refuses on a dirty tree.  `--force` is the deliberate override.
        try:
            shadow(repo, 1, "fixture")
            check("shadow refuses a slot a lane holds", "no error", "SystemExit")
        except SystemExit as exc:
            check("shadow refuses a slot a lane holds", "not free" in str(exc), True)
            check("... naming the claim that holds it",
                  "auto/stub" in str(exc) and "w-good" in str(exc), True)
        check("... while --force overrides the occupancy check",
              shadow(repo, 1, "forced", force=True)["files"] > 0, True)
        _rmtree_force(os.path.join(d1, "forced"))
        check("... leaving the lane's tree clean again", g(d1, "status", "--porcelain", "-uall"), "")

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
        # Give the held branch a commit main does not have, so it is genuinely UNLANDED (tree differs) - the
        # shape the refusal is for; an empty branch's tree equals main's and is now reclaimable instead.
        commit(d, "unlanded work on the held branch")
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

        # a slot reset discards scratch and stale source edits, keeps the warm trees - but ONLY when the
        # release is deliberate: the dirty tree is exactly what fails the release closed (E2).
        info2 = acquire(repo, "auto/dirty", slot=1)
        dd = info2["dir"]
        open(os.path.join(dd, "src", "auto", "stub.c"), "w").write("/* dirty edit */\n")
        open(os.path.join(dd, "scratch.txt"), "w").write("junk\n")
        os.makedirs(os.path.join(dd, "out", "scratch"), exist_ok=True)
        open(os.path.join(dd, "out", "scratch", "junk.bin"), "wb").write(b"x")
        try:
            release(repo, slot=1, unit="auto/dirty", rescue=False)
            check("a dirty tree refuses the release", "no error", "SystemExit")
        except SystemExit as exc:
            check("a dirty tree refuses the release", "REFUSED release slot 1" in str(exc), True)
            check("... naming the tracked edit it would discard", "src/auto/stub.c" in str(exc), True)
            check("... and the untracked scratch", "scratch.txt" in str(exc), True)
            check("... and offering the deliberate override", "--force" in str(exc), True)
        check("... and nothing was touched by the refusal",
              "dirty edit" in open(os.path.join(dd, "src", "auto", "stub.c")).read(), True)
        forced = release(repo, slot=1, unit="auto/dirty", rescue=False, force=True)
        check("--force releases anyway", forced["detached"], True)
        check("... and records what it overrode",
              any("dirty" in line for line in forced["overridden"]), True)
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

        # --- E1: the `.used` sentinel records its OWNER, and `status` shows it -------------------------
        check("the owner label is the worker label when there is one",
              owner_label("auto/own", "worker/own-9f2a", "w-own"), "w-own")
        check("... else the branch slug (stable across rounds)",
              owner_label("auto/own", "worker/own-9f2a"), "own-9f2a")
        check("... else the unit", owner_label("auto/own", None), "auto/own")
        own = acquire(repo, "auto/own", worker="w-own")
        d_own = own["dir"]
        sentinel = marker_info(d_own)
        check("the sentinel records the owner label", sentinel.get("owner"), "w-own")
        check("... the unit", sentinel.get("unit"), "auto/own")
        check("... the branch", sentinel.get("branch"), claims.branch_for("auto/own"))
        check("... and the pid that wrote it", str(sentinel.get("pid") or "").isdigit(), True)
        check("... as key=value with the owner first",
              open(marker_path(d_own), encoding="utf-8").read().splitlines()[0], "owner=w-own")
        check("the sentinel is not reported as legacy", sentinel.get("legacy"), False)
        check("marker_owner reads it back", marker_owner(d_own), "w-own")
        check("slot_state reports the owner", slot_state(repo, own["slot"])["owner"], "w-own")
        own_row = next(r for r in status(repo) if r["slot"] == own["slot"])
        check("`status` reports the owner", own_row["owner"], "w-own")
        check("... and shows no live run when the registry names none here", own_row["run"], None)
        # a legacy positional body still parses, and is explicitly *not* an owner
        with open(marker_path(d_own), "w", encoding="utf-8") as fh:
            fh.write("%d auto/own worker/own-9f2a\n" % os.getpid())
        legacy = marker_info(d_own)
        check("a legacy positional body reads as legacy", legacy.get("legacy"), True)
        check("... with its unit parsed best-effort", legacy.get("unit"), "auto/own")
        check("... and no owner, so no conflict", marker_claim_conflict(d_own, "x", 1), None)
        check("an ownerless sentinel is not a conflict either",
              marker_info(d_own) and marker_claim_conflict(d_own, "someone-else", own["slot"]), None)
        clear_marker(d_own)
        mark_used(d_own, "auto/own", claims.branch_for("auto/own"), "")
        check("... a marker written with no owner label names nobody",
              marker_claim_conflict(d_own, "someone-else", own["slot"]), None)
        clear_marker(d_own)
        mark_used(d_own, "auto/own", claims.branch_for("auto/own"), "w-own")
        release(repo, slot=own["slot"], unit="auto/own")

        # --- E1/E3: a sentinel that names ANOTHER claim is refused at launch -------------------------
        free = acquire(repo, "auto/free", slot=1)
        d_free = free["dir"]
        release(repo, slot=1, unit="auto/free")            # detached, unlocked, marker gone: acquirable
        check("the slot is genuinely free before the foreign marker",
              slot_state(repo, 1)["free"], True)
        check("a foreign marker can be planted on a free slot",
              mark_used(d_free, "auto/other", "worker/other-9f2a", "w-other"), True)
        conflict = marker_claim_conflict(d_free, "auto/mine", 1)
        check("the sentinel is read back as the other claim's", marker_owner(d_free), "w-other")
        check("... and `slot_state` says so", slot_state(repo, 1)["owner"], "w-other")
        check("... including that it is a stale remnant", slot_state(repo, 1)["marker_stale"], True)
        check("the conflict names the owner", "w-other" in (conflict or ""), True)
        check("... and the override to use", "--force" in (conflict or ""), True)
        try:
            acquire(repo, "auto/mine")
            check("acquire REFUSES a sentinel owned by another claim", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire REFUSES a sentinel owned by another claim", "w-other" in str(exc), True)
            check("... and says why (HEAD detached mid-run)", "detached mid-run" in str(exc), True)
        check("... the refusal touched nothing (the other claim's marker survives",
              marker_owner(d_free), "w-other")
        taken = acquire(repo, "auto/mine", force=True)
        check("--force takes the slot deliberately", taken["slot"], 1)
        check("... and the sentinel is rewritten with the new owner",
              marker_owner(d_free) != "w-other", True)
        check("... naming the new claim", marker_owner(d_free), owner_label("auto/mine", taken["branch"]))
        release(repo, slot=1, unit="auto/mine")

        # --- E2/E3: the run registry is the live-lane signal -----------------------------------------
        live_slot = acquire(repo, "auto/live", slot=1)
        d_live = live_slot["dir"]
        cfg_home = os.path.join(tmp, "claude-config")
        reg_root = os.path.join(cfg_home, SESSIONS_DIRNAME)
        os.makedirs(reg_root)
        check("the harness' session registry is discovered under the config dir",
              run_registries(cfg_home), [reg_root])
        check("... and a config dir with no registry yields no signal", run_registries(tmp), [])
        run_id = "11111111-2222-3333-4444-555555555555"
        gone = subprocess.Popen([sys.executable, "-c", "pass"])
        gone.wait()
        check("a finished process is not alive", pid_alive(gone.pid), False)
        check("this process is alive", pid_alive(os.getpid()), True)

        def write_run(state="running", cwd=None):
            # `finished` is a session whose process has exited: the record stays, the pid is dead
            with open(os.path.join(reg_root, "%s.json" % run_id), "w", encoding="utf-8") as fh:
                json.dump({"pid": os.getpid() if state == "running" else gone.pid, "sessionId": run_id,
                           "cwd": cwd or d_live, "name": "worker: fix the thing", "kind": "headless",
                           "status": "busy"}, fh)

        write_run()
        check("a RUNNING record is a live run", [r["run_id"] for r in live_runs(reg_root)], [run_id])
        check("... discovered through the temp dir too",
              [r["run_id"] for r in live_runs(config=cfg_home)], [run_id])
        check("... carrying its cwd and session name",
              (live_runs(reg_root)[0]["cwd"], live_runs(reg_root)[0]["session"]),
              (d_live, "worker: fix the thing"))
        check("a run whose cwd is the slot is IN the slot",
              [r["run_id"] for r in runs_in_slot(d_live, reg_root)], [run_id])
        check("... and one working in another slot is not",
              runs_in_slot(slot_dir(repo, 2), reg_root), [])
        write_run(cwd=os.path.join(d_live, "src"))
        check("... a lane launched in a SUBdirectory is in the slot too",
              [r["run_id"] for r in runs_in_slot(d_live, reg_root)], [run_id])
        write_run()
        live_row = next(r for r in status(repo, registry=reg_root) if r["slot"] == 1)
        check("`status` reports the live run", (live_row["run"] or {}).get("run_id"), run_id)
        check("... and says a run holds the slot", "RUNNING Claude session" in live_row["why"], True)
        try:
            release(repo, slot=1, unit="auto/live", registry=reg_root)
            check("release REFUSES while a run is working in the slot", "no error", "SystemExit")
        except SystemExit as exc:
            check("release REFUSES while a run is working in the slot",
                  "REFUSED release slot 1" in str(exc), True)
            check("... NAMING the run", run_id in str(exc), True)
            check("... and its session", "worker: fix the thing" in str(exc), True)
            check("... and saying a lock cannot see a lane", "a lock cannot see a lane" in str(exc), True)
            check("... and offering --force", "--force" in str(exc), True)
        check("... and HEAD was not detached by the refusal",
              slot_attached_branch(d_live), claims.branch_for("auto/live"))
        # the session's process exits (a crash leaves the record behind): the record is dead, the release proceeds
        write_run(state="finished")
        check("a record whose process has exited retires it", live_runs(reg_root), [])
        check("the finished run is no longer in the slot", runs_in_slot(d_live, reg_root), [])
        write_run()
        forced_live = release(repo, slot=1, unit="auto/live", registry=reg_root, force=True)
        check("--force releases under a live run", forced_live["detached"], True)
        check("... recording the live run it overrode",
              any(run_id in line for line in forced_live["overridden"]), True)
        check("... and naming the lane in the recorded reason",
              any("still working" in line for line in forced_live["overridden"]), True)

        # --- E2/E3: commits no branch reaches refuse the release --------------------------------------
        held = acquire(repo, "auto/orphan", slot=1)
        d_orph = held["dir"]
        commit(d_orph, "a commit on the claim's branch")
        check("a commit the claim's branch reaches is not orphaned", orphaned_commits(d_orph), [])
        check("... so the release proceeds", release_blockers(repo, 1, d_orph), [])
        g(d_orph, "checkout", "-q", "--detach")          # the 2026-09-28 shape: detached, still working
        detached_head = slot_head(d_orph)
        commit(d_orph, "a commit no branch reaches")
        check("a detached commit is orphaned", len(orphaned_commits(d_orph)), 1)
        try:
            release(repo, slot=1, unit="auto/orphan")
            check("release REFUSES to orphan commits", "no error", "SystemExit")
        except SystemExit as exc:
            check("release REFUSES to orphan commits", "no branch reaches" in str(exc), True)
            check("... naming the commit", orphaned_commits(d_orph)[0][:8] in str(exc), True)
        check("... and the refusal left HEAD where the lane put it",
              slot_head(d_orph) != detached_head, True)
        check("... i.e. still detached on the orphan", slot_attached_branch(d_orph), None)
        orphan_forced = release(repo, slot=1, unit="auto/orphan", force=True)
        check("--force releases an orphaned HEAD", orphan_forced["detached"], True)
        check("... recording what it overrode",
              any("no branch reaches" in line for line in orphan_forced["overridden"]), True)

        # --- E2/E3: the slot's own live state (CLAUDE.md's LOCAL-ONLY block) is not dirt ---------------
        blocky = acquire(repo, "auto/blocky", slot=1)
        d_blocky = blocky["dir"]
        check("a freshly acquired slot has a clean tree", slot_dirty(d_blocky), [])
        with open(os.path.join(d_blocky, "CLAUDE.md"), "r", encoding="utf-8", newline="") as fh:
            base = fh.read()
        with open(os.path.join(d_blocky, "CLAUDE.md"), "w", encoding="utf-8", newline="") as fh:
            fh.write(base + "<!-- LOCAL-ONLY-BEGIN: stripped before every commit, see Non-negotiables "
                     "rule 8 -->\nlive state, and an em dash \u2014\n<!-- LOCAL-ONLY-END -->\n")
        check("an CLAUDE.md carrying only its LOCAL-ONLY block is not dirt", slot_dirty(d_blocky), [])
        check("... so a release needs no override", release_blockers(repo, 1, d_blocky), [])
        with open(os.path.join(d_blocky, "CLAUDE.md"), "a", encoding="utf-8", newline="") as fh:
            fh.write("a real edit below the block \u2014\n")
        check("a real CLAUDE.md edit IS dirt", any("CLAUDE.md" in r for r in slot_dirty(d_blocky)), True)
        release(repo, slot=1, unit="auto/blocky", force=True)
        check("... and the reset puts the committed CLAUDE.md back",
              open(os.path.join(d_blocky, "CLAUDE.md"), encoding="utf-8").read().count("LOCAL-ONLY"), 0)

        # --- E2/E3: the SEAM - the claim path's teardown is the release that caused the incident --------
        # 2026-09-28's release was `claims.py release`, not a bare `slots.py release`, so the guard has to
        # hold through that path too: the slot step fails, the slot is left exactly where the live lane put
        # it, and `--force` is what tears it down.
        claims.save_registry(repo, {})
        seam = claims.claim("auto/seam", repo, "w-seam", False, cwd=repo)
        d_seam = seam["worktree"]
        check("claims.claim took a slot for the seam fixture", seam.get("slot") in (1, 2), True)
        write_run(cwd=d_seam)
        no_pane = lambda _row: {"pane": None, "known": True, "alive": False, "active": False,
                                "status": None, "revision": None}
        rel_seam = claims.release("auto/seam", repo, force=False, dry_run=False, probe=no_pane,
                                 lister=lambda: None, run_registry=reg_root)
        check("claims.release refuses while a run works in the slot", rel_seam["complete"], False)
        step_text = json.dumps([(s["label"], s["status"], s["why"]) for s in rel_seam["steps"]])
        check("... naming the run in the failed step", run_id in step_text, True)
        check("... and saying the run is still working in it", "still working" in step_text, True)
        check("... and the teardown stops before it clears the claim (registry entry kept)",
              any(s["label"].startswith("registry entry") and s["status"] == "skipped"
                  and "did not complete" in s["why"] for s in rel_seam["steps"]), True)
        check("... and the slot is still on its branch, exactly as the lane left it",
              slot_attached_branch(d_seam), claims.branch_for("auto/seam"))
        check("... so the claim's branch survives",
              claims.branch_exists(repo, claims.branch_for("auto/seam")), True)
        rel_forced = claims.release("auto/seam", repo, force=True, dry_run=False, probe=no_pane,
                                   lister=lambda: None, run_registry=reg_root)
        check("claims.release --force tears it down anyway", rel_forced["complete"], True)
        check("... clearing the claim as well",
              any(s["label"].startswith("registry entry") and s["status"] == "done"
                  for s in rel_forced["steps"]), True)
        check("... and the slot is detached and unlocked",
              (slot_attached_branch(d_seam), read_lock(repo, seam["slot"])), (None, {}))

        # --- SPAWN: the slot is an explicit launch parameter and the profile follows the kind ----------
        # (the defect: the launcher left both choices to the lane, and three tooling lanes were told
        # `decompiler` - unit policy they could never satisfy. `spawn` makes both explicit.)
        check("the kind mapping is the whole table", KIND_PROFILE,
              {"unit": "surveyor", "fix": "fixer", "merge": "merger", "tooling": "worker",
               "docs": "worker", "review": "codereviewer", "scout": "scout", "plan": "planner"})
        check("PROFILES is the table's values, not a second copy", PROFILES,
              tuple(sorted(set(KIND_PROFILE.values()))))
        for _kind, _profile in sorted(KIND_PROFILE.items()):
            check("kind %s -> %s" % (_kind, _profile), profile_for_kind(_kind), _profile)
        try:
            profile_for_kind("bogus")
            check("an unknown kind is refused (never guessed)", "no error", "SystemExit")
        except SystemExit as exc:
            check("an unknown kind is refused (never guessed)", "unknown kind" in str(exc), True)
            check("... listing every valid kind", all(k in str(exc) for k in KIND_PROFILE), True)
            # the same refusal is raised from `claims.py claim --kind` and `queue.py next --kind`, neither of
            # which spawns anything, so it must not say it is a spawn refusal
            check("... and not claiming to be a spawn", "REFUSED spawn" in str(exc), False)

        # (a) BY NUMBER: takes THAT slot, and the line's cwd is it and its agent is the mapped profile
        sp = spawn(repo, "tooling", slot=1, unit="lane/spawn-tooling", task="Wire up the tool.")
        check("spawn takes the slot BY NUMBER", sp["slot"], 1)
        check("... cwd is that slot", sp["path"].replace("\\", "/"),
              slot_dir(repo, 1).replace("\\", "/"))
        check("... the kind maps to `worker`", sp["agent"], "worker")
        check("... the slot is on the claim's branch", slot_attached_branch(slot_dir(repo, 1)), sp["branch"])
        check("... the line is a headless claude call with that agent, run in the slot",
              sp["spawnLine"].startswith("cd %s && claude --agent worker "
                                          % slot_dir(repo, 1).replace("\\", "/")), True)
        check("... carrying a session id the orchestrator can resume",
              "--session-id %s" % sp["sessionId"] in sp["spawnLine"], True)
        check("... and carries task/slot/path/agent/kind/spawnLine",
              all(k in sp for k in ("slot", "path", "agent", "kind", "spawnLine")), True)
        check("... the block carries the `rev-parse --show-toplevel` self-check",
              "git rev-parse --show-toplevel" in sp["spawnLine"], True)
        check("... and the STOP-and-report rule", "STOP and report it" in sp["spawnLine"], True)
        check("... and the write-nothing-outside rule",
              "nothing" in sp["spawnLine"].lower() and "outside it" in sp["spawnLine"].lower(), True)
        check("... and the claims.py release prohibition", "claims.py release" in sp["spawnLine"], True)
        check("... and the do-not-land rule", "Do not land" in sp["spawnLine"], True)
        check("... and the claim-time currency proof the lane can see",
              "proven current at handover" in sp["spawnLine"]
              and "pending steps: 0" in sp["spawnLine"], True)
        check("... the kind/profile are recorded in the slot's lock",
              (read_lock(repo, 1).get("kind"), read_lock(repo, 1).get("agent")), ("tooling", "worker"))

        # (b) BY NUMBER, occupied: a slot holding an unlanded branch is refused
        try:
            spawn(repo, "unit", slot=1, unit="lane/spawn-intruder", task="x")
            check("spawn BY NUMBER refuses a slot holding an unlanded branch", "no error", "SystemExit")
        except SystemExit as exc:
            check("spawn BY NUMBER refuses a slot holding an unlanded branch",
                  "never a branch" in str(exc), True)
        check("... and the held branch is untouched", slot_attached_branch(slot_dir(repo, 1)), sp["branch"])
        release(repo, slot=1, unit="lane/spawn-tooling", rescue=False)

        # (c) an unknown kind is refused before any slot is touched
        try:
            spawn(repo, "not-a-kind", unit="lane/spawn-bad")
            check("spawn refuses an unknown kind", "no error", "SystemExit")
        except SystemExit as exc:
            check("spawn refuses an unknown kind", "unknown kind" in str(exc), True)
        check("... without taking a slot", free_count(repo), 2)

        # (d) NO --slot: skips the occupied slot and takes the FIRST genuinely free one, naming it
        hold = spawn(repo, "tooling", slot=1, unit="lane/spawn-hold", task="hold")
        freed = spawn(repo, "docs", unit="lane/spawn-first-free", task="write the doc")
        check("spawn with no --slot takes the first genuinely free slot", freed["slot"], 2)
        check("... naming it", freed["path"].replace("\\", "/"), slot_dir(repo, 2).replace("\\", "/"))
        check("... and `docs` maps to the `worker` fallback", freed["agent"], "worker")
        release(repo, slot=2, unit="lane/spawn-first-free", rescue=False)
        unit_sp = spawn(repo, "unit", unit="lane/spawn-unit", task="Reconstruct the unit.")
        check("... and `unit` maps to `surveyor`", unit_sp["agent"], "surveyor")
        check("... on the first free slot again", unit_sp["slot"], 2)
        release(repo, slot=2, unit="lane/spawn-unit", rescue=False)
        release(repo, slot=1, unit="lane/spawn-hold", rescue=False)

        # (e) --task-file: the printed line carries the file's text
        tf = os.path.join(tmp, "spawn-task.txt")
        with open(tf, "w", encoding="utf-8") as fh:
            fh.write("Read this and do X.")
        tfs = spawn(repo, "fix", unit="lane/spawn-taskfile", task_file=tf)
        check("spawn reads the task text from --task-file", "Read this and do X." in tfs["spawnLine"], True)
        check("... and `fix` maps to `fixer`", tfs["agent"], "fixer")
        release(repo, slot=tfs["slot"], unit="lane/spawn-taskfile", rescue=False)
        check("the pool is left as it was found (both slots free)", free_count(repo), 2)

        # --- RECLAIM: a slot holding a LANDED branch is bookkeeping, not work in progress ----------------
        # The refusal "a slot holds a directory, never a branch" is load-bearing for an *unlanded* branch,
        # but after a landing the branch's content is in main and detaching/deleting it by hand cost a round
        # trip every time.  These fixtures pin both directions, the rescue-first ordering, and fail-closed.

        def land_into_main(branch, rel="f.txt", msg="land the branch"):
            """Simulate the GATE landing (`land.py land --branch`): apply the branch's content onto main as a
            NEW commit, so the branch stays *ahead of main by its own commit* while its TREE now equals
            main's - exactly the shape the merge-tree equality must still call applied, because it compares
            trees and not commits.

            A pathspec on the commit keeps unrelated untracked state (the claims registry) out of main, so
            the only tree difference between main and the branch is the change itself.
            """
            g(repo, "cherry-pick", "--no-commit", branch)
            g(repo, "commit", "-q", "-m", msg, "--", rel)

        # (e1) the merge-tree read itself: a conflict or unreadable output is ambiguity, never "applied"
        _oid40 = "a" * 40
        check("merge-tree: a clean run yields its tree OID", _merge_tree_oid(0, _oid40 + "\n"), _oid40)
        check("... a conflict (nonzero exit) is NOT a tree", _merge_tree_oid(1, _oid40 + "\n"), None)
        check("... an unparseable first line is ambiguity", _merge_tree_oid(0, "CONFLICT (content)\n"), None)
        check("... empty output is ambiguity", _merge_tree_oid(0, ""), None)
        check("... and a merge-tree that cannot resolve the branch is unknown",
              branch_fully_applied(repo, "worker/ghost-does-not-exist"), None)

        # (c) a genuinely FREE slot is untouched: reclaim refuses and writes nothing
        rescue_before = g(repo, "for-each-ref", "--format=%(refname)", "refs/rescue/")
        free_v = reclaim_slot(repo, 1)
        check("reclaim leaves a genuinely free slot untouched", free_v["reclaimed"], False)
        check("... saying it holds no branch", "no branch" in free_v["reason"], True)
        check("... and parking no rescue ref",
              g(repo, "for-each-ref", "--format=%(refname)", "refs/rescue/"), rescue_before)
        check("... which the read-only verdict agrees with", reclaim_verdict(repo, 1)["reclaimable"], False)

        # (a)+(d) a slot holding a LANDED branch is reclaimed and taken, rescue ref FIRST
        land = acquire(repo, "auto/landed", slot=1)
        d_land = land["dir"]
        lbranch = land["branch"]
        commit(d_land, "a landed change")
        land_into_main(lbranch, msg="land: a landed change")
        clear_lock(repo, 1)                      # the claim was released; the branch/worktree remain
        ltip = g(repo, "rev-parse", lbranch)
        check("the branch's content is now fully in main (the free test)",
              branch_fully_applied(repo, lbranch), True)
        # THE GATE-ROUTE PIN: the branch is still ahead of main by its own commit (a commit-based test would
        # call it unlanded), yet its TREE equals main's - which is why the one merge-tree rule is enough.
        check("... the GATE-landed branch is still ahead of main by its own commits",
              claims.commits_ahead(repo, lbranch) >= 1, True)
        check("... and merge-tree really equals main^{tree}", merge_tree_of(repo, lbranch),
              g(repo, "rev-parse", "main^{tree}"))
        check("the read-only verdict confirms the landed branch", reclaim_verdict(repo, 1)["reclaimable"], True)
        pv = preview(repo, "auto/after-land", slot=1)
        check("a dry run predicts the reclaim without doing it",
              (pv.get("reclaim") or {}).get("branch"), lbranch)
        check("... and touches nothing (the branch is still attached)",
              slot_attached_branch(d_land), lbranch)
        check("... so the landed branch is still there", claims.branch_exists(repo, lbranch), True)
        taken = acquire(repo, "auto/after-land", slot=1)
        check("acquire RECLAIMS a slot holding a landed branch and takes it", taken["slot"], 1)
        check("... the slot now holds the NEW claim's branch", slot_attached_branch(d_land), taken["branch"])
        rec = taken.get("landed_reclaim") or {}
        check("... reporting the reclaim", rec.get("reclaimed"), True)
        lref = claims.rescue_ref_name("auto/landed")
        check("... the rescue ref exists", claims.rescue_exists(repo, "auto/landed"), lref)
        check("... pointing at the branch tip", g(repo, "rev-parse", lref), ltip)
        check("... and the landed branch is deleted", claims.branch_exists(repo, lbranch), False)
        lsteps = [s["step"] for s in rec.get("steps", [])]
        check("... the rescue ref is parked BEFORE the detach and the delete",
              lsteps.index("rescue") < lsteps.index("detach") < lsteps.index("delete-branch"), True)
        check("... and the one line says what it did and why", "RECLAIMED" in (rec.get("line") or ""), True)
        release(repo, slot=1, unit="auto/after-land", rescue=False)

        # (a2) the named action `slots.py reclaim` does the same thing deliberately, for one slot
        act = acquire(repo, "auto/act", slot=1)
        d_act = act["dir"]
        commit(d_act, "action landed")
        land_into_main(act["branch"], msg="land: action landed")
        clear_lock(repo, 1)
        atip = g(repo, "rev-parse", act["branch"])
        check("the action's branch is fully applied", branch_fully_applied(repo, act["branch"]), True)
        aout = reclaim_slot(repo, 1)
        check("the explicit reclaim action reclaims a landed slot", aout["reclaimed"], True)
        check("... freeing the slot for the next acquire", slot_state(repo, 1)["free"], True)
        check("... parking the rescue ref at the branch tip",
              g(repo, "rev-parse", claims.rescue_ref_name("auto/act")), atip)
        check("... and deleting the branch", claims.branch_exists(repo, act["branch"]), False)

        # (b) an UNLANDED branch is still refused, with today's message, verbatim
        unl = acquire(repo, "auto/unlanded", slot=1)
        d_unl = unl["dir"]
        commit(d_unl, "definitely not landed")
        clear_lock(repo, 1)
        check("the unlanded branch is NOT applied", branch_fully_applied(repo, unl["branch"]), False)
        v_unl = reclaim_slot(repo, 1)
        check("reclaim REFUSES an unlanded branch", v_unl["reclaimed"], False)
        check("... with the test's own verdict", v_unl["applied"], False)
        try:
            acquire(repo, "auto/unlanded-2", slot=1)
            check("acquire still refuses an unlanded branch", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire still refuses an unlanded branch", "it still has branch" in str(exc), True)
            check("... keeping the rule verbatim",
                  "a slot holds a directory, never a branch" in str(exc), True)
        check("... and the unlanded branch is untouched", slot_attached_branch(d_unl), unl["branch"])
        check("... with no rescue ref parked for it", claims.rescue_exists(repo, "auto/unlanded"), None)
        check("... and `status` names the missing record",
              "missing record" in slot_state(repo, 1)["record_note"], True)
        # a record that names a branch the slot has left is STALE, and `status` says so with the remedy
        write_lock(repo, 1, {"slot": 1, "unit": "ghost/unit", "branch": "worker/ghost-gone",
                             "base": "0" * 40})
        check("... a stale record is named", "stale record" in slot_state(repo, 1)["record_note"], True)
        check("... with its remedy in `why`", "acquire --slot 1" in slot_state(repo, 1)["why"], True)
        clear_lock(repo, 1)
        release(repo, slot=1, unit="auto/unlanded", rescue=False)

        # (b2) a MISSING `.used` marker is a crash remnant, not a claim: a LANDED branch is still reclaimable
        nm = acquire(repo, "auto/nomark-land", slot=1)
        commit(nm["dir"], "nomark landed")
        land_into_main(nm["branch"], msg="land: nomark landed")
        clear_lock(repo, 1)
        clear_marker(nm["dir"])                  # crash remnant: branch attached, marker MISSING
        check("a MISSING-marker slot still shows its branch attached",
              slot_state(repo, 1)["attached"], nm["branch"])
        check("... with the marker gone", marker_present(nm["dir"]), False)
        check("... and the branch proven applied", branch_fully_applied(repo, nm["branch"]), True)
        nm_taken = acquire(repo, "auto/nomark-after", slot=1)
        check("acquire RECLAIMS a MISSING-marker slot whose branch is applied", nm_taken["slot"], 1)
        check("... and takes it", slot_attached_branch(nm["dir"]), nm_taken["branch"])
        check("... with the rescue ref parked", claims.rescue_exists(repo, "auto/nomark-land"),
              claims.rescue_ref_name("auto/nomark-land"))
        check("... and the old branch deleted", claims.branch_exists(repo, nm["branch"]), False)
        release(repo, slot=1, unit="auto/nomark-after", rescue=False)

        # (b3) ... but the same MISSING marker next to an UNLANDED branch still refuses
        nm2 = acquire(repo, "auto/nomark-unl", slot=1)
        commit(nm2["dir"], "nomark unlanded")
        clear_lock(repo, 1)
        clear_marker(nm2["dir"])
        nm2_v = reclaim_slot(repo, 1)
        check("a MISSING-marker slot holding an UNLANDED branch still refuses", nm2_v["reclaimed"], False)
        check("... with the test's verdict", nm2_v["applied"], False)
        check("... and the branch untouched", slot_attached_branch(nm2["dir"]), nm2["branch"])
        release(repo, slot=1, unit="auto/nomark-unl", rescue=False)

        # (c) DEBRIS: no branch checked out, a `.used` sentinel, and NO registry row - a crash remnant
        db = acquire(repo, "auto/debris", slot=1)
        db_branch = db["branch"]
        clear_lock(repo, 1)
        g(db["dir"], "checkout", "-q", "--detach", g(repo, "rev-parse", "HEAD"))   # at main's tip
        g(repo, "branch", "-D", db_branch)      # "no branch": the ref is gone, the sentinel survives
        check("a detached sentinel with no claim is named `debris`", slot_state(repo, 1)["state"], "debris")
        check("... and reads free so it can never wedge the pool", slot_state(repo, 1)["free"], True)
        db_v = reclaim_verdict(repo, 1)
        check("... and the verdict says reclaimable debris",
              (db_v["reclaimable"], db_v["kind"]), (True, "debris"))
        db_out = reclaim_slot(repo, 1)
        check("reclaim clears the debris sentinel", (db_out["reclaimed"], db_out["kind"]), (True, "debris"))
        check("... with no branch to rescue", db_out["rescue_ref"], None)
        check("... and the sentinel is gone", marker_present(db["dir"]), False)
        check("... naming the state in its one line", "debris" in (db_out["line"] or ""), True)

        # (d) a sentinel WITH a live claim is refused - someone's work is in it
        cl = acquire(repo, "auto/claimed", slot=1)
        check("a live lock makes the slot `claimed`", slot_state(repo, 1)["state"], "claimed")
        cl_v = reclaim_slot(repo, 1)
        check("reclaim REFUSES a slot a live claim holds", cl_v["reclaimed"], False)
        check("... naming the live claim", "live claim" in cl_v["reason"], True)
        check("... and leaving the branch attached", slot_attached_branch(cl["dir"]), cl["branch"])
        release(repo, slot=1, unit="auto/claimed", rescue=False)

        # (d2) SLOT 2'S REAL SHAPE (measured 2026-09-28): marker present, branch checked out and alive, its
        # worktree missing from `git worktree list`, and the ONLY evidence of life a REGISTRY row whose
        # progress advances.  The registry row - the same liveness record `claims.expire`/`claim_status` read
        # - must refuse, even with the lock cleared.  ("The label is not the measurement.")
        l2 = acquire(repo, "auto/live2", slot=1)
        claims.save_registry(repo, {"network/dwci-band": {
            "branch": l2["branch"], "slot": 1, "worker": "loop-6", "worktree": l2["dir"],
            "claimed_at": "2026-09-28T18:00:00"}})
        clear_lock(repo, 1)                      # no owner string on the lock: the registry row is the evidence
        check("a registry-held slot reads `claimed`", slot_state(repo, 1)["state"], "claimed")
        check("... and is NOT free", slot_state(repo, 1)["free"], False)
        l2_v = reclaim_slot(repo, 1)
        check("reclaim REFUSES a registry-held slot (progress moves = life)", l2_v["reclaimed"], False)
        check("... naming the live claim", "live claim" in l2_v["reason"], True)
        try:
            acquire(repo, "auto/live2b", slot=1)
            check("... and acquire refuses it too", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and acquire refuses it too", "it still has branch" in str(exc), True)
        claims.save_registry(repo, {})
        release(repo, slot=1, unit="auto/live2", branch=l2["branch"], rescue=False)

        # (d3) the MIRROR: a registry row with NO sentinel still counts as in use (never hand a live branch
        # to a new lane)
        mir = acquire(repo, "auto/mirror", slot=1)
        claims.save_registry(repo, {"x/y": {"branch": mir["branch"], "slot": 1, "worker": "w"}})
        clear_lock(repo, 1)
        clear_marker(mir["dir"])
        g(mir["dir"], "checkout", "-q", "--detach", g(repo, "rev-parse", "HEAD"))
        check("a registry row with no sentinel still reads `claimed`", slot_state(repo, 1)["state"], "claimed")
        check("... and is NOT free", slot_state(repo, 1)["free"], False)
        check("... so reclaim refuses", reclaim_slot(repo, 1)["reclaimed"], False)
        claims.save_registry(repo, {})
        release(repo, slot=1, branch=mir["branch"], rescue=False)

        # (e-reg) fail closed: an UNREADABLE claim registry refuses rather than guesses
        os.makedirs(os.path.join(repo, ".pi"), exist_ok=True)
        with open(os.path.join(repo, ".pi", "claims.json"), "w", encoding="utf-8") as fh:
            fh.write("{ this is not json")
        bad = reclaim_slot(repo, 1)
        check("an unreadable claim registry refuses the reclaim", bad["reclaimed"], False)
        check("... naming the registry", "cannot be read" in bad["reason"], True)
        try:
            acquire(repo, "auto/noreg", slot=1)
            check("... and no slot is handed out", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and no slot is handed out", "cannot be read" in str(exc), True)
        claims.save_registry(repo, {})
        check("... restored, the pool is usable again", slot_state(repo, 1)["free"], True)

        # (pool) a bulk `init --force` REFUSES while any slot holds live work - the pool is the campaign's
        # concurrency cap, not a scratch file
        keep = acquire(repo, "auto/keep", slot=1)
        try:
            init(repo, count=2, force=True)
            check("init --force refuses while a slot holds live work", "no error", "SystemExit")
        except SystemExit as exc:
            check("init --force refuses while a slot holds live work", "REFUSED init --force" in str(exc), True)
            check("... naming the slot", "auto/keep" in str(exc), True)
        check("... and the live slot is untouched", slot_attached_branch(keep["dir"]), keep["branch"])
        release(repo, slot=1, unit="auto/keep", rescue=False)
        check("... while a drained pool may be forced", init(repo, count=2, force=True)["created"], [1, 2])

        # (e2) fail-closed for real: a CONFLICTING branch is an ambiguity, so nothing is reclaimed
        conf = acquire(repo, "auto/conflict", slot=1)
        d_conf = conf["dir"]
        with open(os.path.join(d_conf, "f.txt"), "w", encoding="utf-8") as fh:
            fh.write("branch side\n")
        g(d_conf, "add", "-A")
        g(d_conf, "commit", "-q", "-m", "branch side")
        with open(os.path.join(repo, "f.txt"), "w", encoding="utf-8") as fh:
            fh.write("main side\n")
        g(repo, "commit", "-q", "-m", "main side", "--", "f.txt")
        clear_lock(repo, 1)
        check("a conflicting branch cannot be tested - ambiguity, not 'applied'",
              branch_fully_applied(repo, conf["branch"]), None)
        conf_v = reclaim_slot(repo, 1)
        check("... so reclaim fails closed and does not act", conf_v["reclaimed"], False)
        check("... naming the doubt", "failing closed" in conf_v["reason"], True)
        check("... and the conflicting branch is left attached", slot_attached_branch(d_conf),
              conf["branch"])
        try:
            acquire(repo, "auto/conflict-2", slot=1)
            check("... and acquire keeps its refusal for it", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and acquire keeps its refusal for it", "never a branch" in str(exc), True)
        release(repo, slot=1, unit="auto/conflict", rescue=False)

        # (e3) the live-lane guard is not bypassed: a landed branch with a RUNNING lane in the slot refuses
        live_land = acquire(repo, "auto/live-land", slot=1)
        d_ll = live_land["dir"]
        commit(d_ll, "landed under a live lane")
        land_into_main(live_land["branch"], msg="land: under a live lane")
        clear_lock(repo, 1)                      # the claim is gone; the only life left is the RUNNING run
        check("the live-lane branch is fully applied", branch_fully_applied(repo, live_land["branch"]), True)
        reg_root2 = os.path.join(tmp, "claude-reclaim", SESSIONS_DIRNAME)
        os.makedirs(reg_root2, exist_ok=True)
        run_id2 = "99999999-8888-7777-6666-555555555555"
        rec2 = os.path.join(reg_root2, "%s.json" % run_id2)
        with open(rec2, "w", encoding="utf-8") as fh:
            json.dump({"pid": os.getpid(), "sessionId": run_id2, "cwd": d_ll, "kind": "headless"}, fh)
        live_v = reclaim_slot(repo, 1, registry=reg_root2)
        check("reclaim refuses a landed branch while a RUNNING lane is in the slot",
              live_v["reclaimed"], False)
        check("... naming the live run", run_id2 in live_v["reason"], True)
        check("... and leaving the branch attached", slot_attached_branch(d_ll), live_land["branch"])
        with open(rec2, "w", encoding="utf-8") as fh:
            json.dump({"pid": gone.pid, "sessionId": run_id2, "cwd": d_ll, "kind": "headless"}, fh)
        live_done = reclaim_slot(repo, 1, registry=reg_root2)
        check("... and reclaims once the run is finished", live_done["reclaimed"], True)
        check("... leaving the pool free", free_count(repo), 2)

        # (f) the exact line `spawn` prints when it reclaims, and its stdout artifact stays clean
        sl = acquire(repo, "auto/spawn-land", slot=1)
        commit(sl["dir"], "spawn landed")
        land_into_main(sl["branch"], msg="land: spawn landed")
        clear_lock(repo, 1)
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            sp_rec = spawn(repo, "tooling", slot=1, unit="lane/spawn-reclaim", task="x")
        check("spawn reclaims a landed slot on the way in", sp_rec["slot"], 1)
        check("... and prints the one RECLAIMED line",
              "RECLAIMED" in buf.getvalue() and sl["branch"] in buf.getvalue(), True)
        check("... leaving the paste-ready stdout line untouched",
              sp_rec["spawnLine"].startswith("cd %s && claude --agent worker "
                                              % slot_dir(repo, 1).replace("\\", "/")), True)
        release(repo, slot=1, unit="lane/spawn-reclaim", rescue=False)
        check("... and the pool is left as it was found", free_count(repo), 2)

    NINJA_RUNNER = saved_ninja
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
    a.add_argument("--force", action="store_true",
                   help="take a slot whose `.used` sentinel names a claim whose owner is gone")
    a.add_argument("--dry-run", action="store_true")
    a.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="return a slot to main's tip (keeps the warm trees)")
    r.add_argument("--slot", type=int, default=None)
    r.add_argument("--unit", default=None)
    r.add_argument("--branch", default=None)
    r.add_argument("--keep-branch", action="store_true", help="do not delete the claim's branch")
    r.add_argument("--force", action="store_true",
                   help="release even though a run is live in the slot / its tree is dirty / its HEAD "
                        "holds commits no branch reaches")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--json", action="store_true")
    rc = sub.add_parser("reclaim", help="release a slot whose checked-out branch is ALREADY fully applied "
                                         "to main, or whose `.used` sentinel is debris with no claim (an "
                                         "unlanded branch keeps today's refusal)")
    rc.add_argument("--slot", type=int, default=None)
    rc.add_argument("--unit", default=None)
    rc.add_argument("--branch", default=None)
    rc.add_argument("--json", action="store_true")
    sp = sub.add_parser("spawn", help="take a slot for a lane of a kind and print the paste-ready launch")
    sp.add_argument("--kind", required=True,
                    help="the kind of work -> agent profile: " + ", ".join(KIND_PROFILE))
    sp.add_argument("--slot", type=int, default=None,
                    help="take this slot by number (default: the first genuinely free slot)")
    sp.add_argument("--unit", default=None,
                    help="the claim/branch name (default: lane/<kind>-<timestamp>)")
    sp.add_argument("--task-file", default=None, help="read the lane's task text from this file")
    sp.add_argument("--worker", default=None)
    sp.add_argument("--force", action="store_true",
                    help="take a slot whose `.used` sentinel names a claim whose owner is gone")
    sp.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="every slot, its lock and whether its build tree is current")
    s.add_argument("--json", action="store_true")
    v = sub.add_parser("verify", help="validate each slot's build tree against MAIN's current map/DOL")
    v.add_argument("--slot", type=int, default=None)
    v.add_argument("--json", action="store_true")
    sh = sub.add_parser("shadow", help="materialise a lane-shaped tree INSIDE a slot (for fixture tests)")
    sh.add_argument("slot", type=int)
    sh.add_argument("dir", help="where inside the slot: relative to the slot, or an absolute path inside it")
    sh.add_argument("--seed-build", action="store_true",
                    help="also copy the warm build/input trees, so the shadow can compile and score")
    sh.add_argument("--force", action="store_true", help="replace a non-empty destination directory")
    sh.add_argument("--json", action="store_true")
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
            out = preview(main_wt, args.unit, args.branch, args.slot, args.worker, args.force)
            print("would acquire slot %d for %s (owner %s): %s"
                  % (out["slot"], args.unit, out["owner"], out["command"]))
            if out.get("reclaim"):
                print("  ... after RECLAIMING it - its branch %r is already fully applied to main"
                      % out["reclaim"]["branch"])
            return 0
        info = acquire(main_wt, args.unit, args.branch, args.worker, args.slot, args.force)
        if args.json:
            print(json.dumps(info, indent=2))
            return 0
        print("acquired slot %d for %s\n  dir     %s\n  branch  %s\n  base    %s\n  verify  %s"
              % (info["slot"], info["unit"], info["dir"], info["branch"], info["base"],
                 "current" if info["verify"]["ok"] else "; ".join(info["verify"]["reasons"])))
        if info.get("refreshed"):
            print("  refreshed %s" % info["refreshed"])
        for line in currency_lines(info["currency"]):
            print("  " + line.replace("**", ""))
        print("  owner   %s (in `.used`)" % marker_owner(info["dir"]))
        if (info.get("landed_reclaim") or {}).get("reclaimed"):
            print("  reclaimed a landed branch first: %s" % info["landed_reclaim"]["line"])
        return 0
    if args.cmd == "release":
        out = release(main_wt, args.slot, args.unit, args.branch, delete_branch=not args.keep_branch,
                      dry_run=args.dry_run, force=args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        verb = "would release" if args.dry_run else "released"
        print("%s slot %d (%s)%s%s" % (verb, out["slot"], out["dir"],
                                       "" if out["branch_deleted"] or not out["branch"] else
                                       " - branch %s kept" % out["branch"],
                                       "" if not out["refreshed"] else "\n  %s" % out["refreshed"]))
        if out.get("overridden"):
            print("  --force overrode:\n  - %s" % "\n  - ".join(out["overridden"]))
        return 0
    if args.cmd == "reclaim":
        n, lock = _resolve_slot(main_wt, args.slot, args.unit, args.branch)
        out = reclaim_slot(main_wt, n, branch=args.branch or lock.get("branch"),
                           unit=args.unit or lock.get("unit"))
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if not out["reclaimed"]:
            raise SystemExit("REFUSED reclaim slot %d: %s\n  the refusal is load-bearing: a slot holds a "
                             "directory, never a branch, and only a branch whose content is PROVEN in main "
                             "is bookkeeping to reclaim (land it, or `release --slot %d`)."
                             % (n, out["reason"], n))
        print(out["line"])
        return 0
    if args.cmd == "spawn":
        out = spawn(main_wt, args.kind, slot=args.slot, unit=args.unit, task_file=args.task_file,
                    worker=args.worker, force=args.force)
        if args.json:
            # exactly the fields a caller needs; `spawnLine` is the whole paste-ready text (line + block)
            print(json.dumps({k: out[k] for k in ("slot", "path", "agent", "kind", "spawnLine")},
                             indent=2))
            return 0
        # stdout is exactly the paste-ready artifact (the claude launch line + the block); the header names the
        # slot that was taken on stderr, so a caller can capture stdout verbatim.
        print("spawn slot %d (%s) for kind %s -> agent %s\n  path: %s"
              % (out["slot"], out["branch"], out["kind"], out["agent"], out["path"]),
              file=sys.stderr)
        print(out["spawnLine"])
        return 0
    if args.cmd == "shadow":
        out = shadow(main_wt, args.slot, args.dir, seed_build=args.seed_build, force=args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("shadowed slot %d -> %s (%d file(s)%s)"
              % (out["slot"], out["dir"], out["files"],
                 ", seeded " + ", ".join(out["seeded"]) if out["seeded"] else ""))
        print("  head %s (its OWN repository, inside the slot; nothing outside it was touched)" % out["head"])
        return 0
    if args.cmd == "status":
        rows = status(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no slot pool - run `python tools/units/slots.py init`")
            return 0
        print("%-5s %-9s %-16s %-20s %-20s %-7s %-9s %s"
              % ("slot", "state", "owner", "unit", "branch", ".used", "run", "build tree"))
        for row in rows:
            claim = row.get("claim") or {}
            lock = row.get("lock") or {}
            # one word for WHY it is not free, so a silent cap becomes a diagnosis (`branch`/`claimed`/`debris`)
            state = row.get("state") or ("free" if row["free"] else "used")
            marker = "stale" if row.get("marker_stale") else ("yes" if row["marked"] else "-")
            run = (row["run"]["run_id"][:8] if row.get("run") else "-")
            print("%-5d %-9s %-16s %-20s %-20s %-7s %-9s %s" % (
                row["slot"], state, (row.get("owner") or "-")[:16],
                (claim.get("unit") or lock.get("unit") or "-")[:20],
                (row["attached"] or "detached")[:20], marker, run,
                "current" if row["build_ok"] else "; ".join(row["build_reasons"])))
        # every slot in the manifest gets a reason line when it is not simply free, so a silent cap is a
        # diagnosis: `no worktree`, `branch`, `claimed`, `debris`, with a bad record named where there is one
        for row in rows:
            if not row["free"] or row.get("state") == "no worktree" or row.get("record_note"):
                print("  slot %d: %s" % (row["slot"], row["why"]))
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
