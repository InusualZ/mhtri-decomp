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
    python tools/units/slots.py status [--json]
    python tools/units/slots.py verify [--slot N] [--json]
    python tools/units/slots.py --selftest

**One lane per slot, enforced at both ends.**  A slot's claim was released while another lane was still
working in it, and the release detached HEAD under a live process; the same hole let a second lane acquire
a slot that still held the first lane's branch.  The sentinel and the release path are now the two ends of
one rule:

* the `.used` sentinel records its **OWNER label** (`mark_used`, `marker_info`), and `status` prints it, so
  "someone holds this" is never the only thing a reader can know;
* `acquire` refuses a slot whose sentinel **names a different owner** instead of silently reclaiming it
  (`--force` is the deliberate override);
* `release` **fails closed** on three signals - a RUNNING subagent run whose cwd resolves into the slot,
  a dirty tree, and commits on HEAD that no branch reaches - printing every reason and offering `--force`.
  The run registry under the temp dir is the *only* live-lane signal available: a lock file records what
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
import tempfile
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

#: How a lane's **kind of work** maps to the agent profile it is launched with.  This mapping is the fix
#: `spawn` exists for: `queue.py`/`claims.py` left the profile to the lane (defaulting to `decompiler`), so
#: three *tooling* lanes carried unit policy they could never satisfy and a general rule they must not
#: break (AGENTS.md, "Operational mode").  The mapping is exhaustive - one kind per lane, never a guess.
KIND_PROFILE = {
    "unit": "decompiler",     # register a proposal at its final home and reconstruct its bodies
    "fix": "fixer",           # a refused gate or a measured regression on a branch
    "merge": "merger",        # a refused apply
    "tooling": "worker",      # the fallback for a task that is none of the specific ones
    "docs": "worker",
    "review": "codereviewer",  # the tracked project review profile (the global `reviewer` is separate)
    "scout": "scout",          # read-only
    "plan": "planner",         # read-only
}


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

    Only `agents_md_real_change` is used, and only to keep the *one* implementation of "is AGENTS.md
    really changed, or is that the LOCAL-ONLY block?" - a slot's AGENTS.md carries live working state by
    rule 8, so a second copy of that cut here would be the bug, not the reuse.
    """
    from units import land
    return land


# --- the live-lane signal: the harness' async run registry -----------------------------------------

#: The directory a `pi` harness keeps its async subagent runs under: `<temp>/pi-subagents-*/async-subagent-runs`.
RUNS_DIRNAME = "async-subagent-runs"


def run_registries(temp: str | None = None) -> list[str]:
    """Every async-subagent run registry under the temp dir: `<temp>/pi-subagents-*/async-subagent-runs`.

    **This registry is the only live-lane signal available to this tool, so it is read, never inferred.**
    A lock of any kind - the `.used` sentinel, the JSON claim record - only records what *this tool* did,
    and a lane is a process the tool never launched: the project proved that the day a lane worked in MAIN
    and left no slot file at all (roadmap 7.31), and again on 2026-09-28, when a slot's claim was released
    while another lane was still working in it and the release detached HEAD under the live process.  The
    harness records every run it launches - id, `cwd`, `state` - and that record is what
    `release_blockers` reads.

    A host with no registry (no harness, or a different one) yields **no signal at all**: this returns an
    empty list, and the dirty-tree and unreachable-commit guards are then the only backstops.  That is
    stated rather than papered over - "no registry" is not "no lane".
    """
    temp = temp or tempfile.gettempdir()
    try:
        names = sorted(os.listdir(temp))
    except OSError:
        return []
    out = []
    for name in names:
        if not name.startswith("pi-subagents-"):
            continue
        d = os.path.join(temp, name, RUNS_DIRNAME)
        if os.path.isdir(d):
            out.append(d)
    return out


def _read_json(path: str) -> dict:
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}
    return data if isinstance(data, dict) else {}


def _live_run_record(record: dict, run_id: str, registry: str) -> dict:
    """One run record, flattened to the fields a lane guard needs."""
    steps = record.get("steps") or []
    first = steps[0] if steps and isinstance(steps[0], dict) else {}
    return {"run_id": record.get("runId") or run_id,
            "cwd": record.get("cwd") or first.get("cwd") or "",
            "state": record.get("state") or first.get("status") or "",
            "agent": first.get("agent") or "",
            "session": first.get("sessionName") or "",
            "pid": record.get("pid"),
            "registry": registry}


def live_runs(registry: str | None = None, temp: str | None = None) -> list[dict]:
    """Every **RUNNING** subagent run, from `status.json` in each run directory.

    `<registry>/<run-id>/status.json` is the live record (`state: running`, `cwd`, `steps[0].agent` /
    `.sessionName`); `.terminal-runs/<id>` is the harness' own finish marker, and a run named there is
    finished **whatever a stale `status.json` says** - that ordering is what makes "the same record marked
    finished must proceed" true rather than hopeful.  A record this function cannot read is skipped, not
    guessed at: an unreadable registry is reported as no signal (`run_registries`), never as "no lane".
    """
    runs = []
    for root in ([registry] if registry else run_registries(temp)):
        terminal = set(os.listdir(os.path.join(root, ".terminal-runs"))
                       if os.path.isdir(os.path.join(root, ".terminal-runs")) else [])
        try:
            names = sorted(os.listdir(root))
        except OSError:
            continue
        for name in names:
            if name.startswith("."):
                continue
            d = os.path.join(root, name)
            if not os.path.isdir(d) or name in terminal:
                continue
            record = _read_json(os.path.join(d, "status.json"))
            if not record:
                continue
            run = _live_run_record(record, name, root)
            if run["state"] == "running":
                runs.append(run)
    return runs


def _same_tree(a: str, b: str) -> bool:
    """Whether path `a` is `b` or sits inside it (case-insensitively on Windows, separator-agnostic)."""
    if not a or not b:
        return False
    x, y = os.path.normcase(os.path.abspath(a)), os.path.normcase(os.path.abspath(b))
    return x == y or x.startswith(y.rstrip(os.sep) + os.sep)


def runs_in_slot(slot: str, registry: str | None = None, temp: str | None = None) -> list[dict]:
    """Every RUNNING subagent run whose `cwd` resolves into `slot` - the slot is its working directory.

    A lane launched with its cwd at a subdirectory of the slot is in the slot too, so the test is
    "inside", not "equal".
    """
    return [run for run in live_runs(registry, temp) if _same_tree(run.get("cwd"), slot)]


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
    row is deliberately not dirt: an `AGENTS.md` whose only difference is its LOCAL-ONLY block, which is
    live working state by rule 8 and is dirty in every real slot - `land.agents_md_real_change` owns that
    judgement, and a release that called it dirt would refuse every ordinary teardown.

    The porcelain output is read **raw**: `git()` strips its stdout, which eats the leading space of the
    first row and shifts every field by one (` M AGENTS.md` -> `M AGENTS.md`, so the path parses as
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
        if path == "AGENTS.md" and code.strip() in ("M", "MM"):
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
                     registry: str | None = None, temp: str | None = None) -> list[tuple[str, str]]:
    """Everything that makes releasing slot `n` unsafe: `[(one-line summary, detail)]`, `[]` if safe.

    Three signals, least to most precise:

    * **a RUNNING subagent run whose cwd resolves into the slot** - read from the harness' run registry
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
    runs = runs_in_slot(d, registry, temp)
    if runs:
        out.append(("a RUNNING subagent run is still working in it: "
                    + ", ".join(run_label(r) for r in runs),
                    "cwd %s - the run registry under the temp dir is the only live-lane signal there is, "
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


def slot_state(main: str, n: int, runs: list | None = None) -> dict:
    """One slot's row: whether it exists, what it has checked out, its `.used` marker and whether it is free.

    **`free`/`used` is read from two sources of the same fact, and both must agree that the slot is empty.**
    The primary is the `.used` sentinel `acquire` creates and `release` removes; the second is the worktree
    state - a slot whose worktree still has a branch checked out is **in use**, whatever any file says.  The
    lock record is a convenience; the worktree is the truth.  Today the two disagreed (slot 2 was reported
    `free` while its worktree held `worker/rule10-fix-14f8`), and that disagreement *was* the bug, so both
    readings are kept.

    A **third** reading rides along and is the only one that can see a lane rather than a claim: `run`, the
    RUNNING subagent run whose cwd is this slot (`runs_in_slot`).  A lane holds the slot even when the
    sentinel, the lock and the worktree all say otherwise - the sentinel can be cleared and the worktree
    detached by a release while the lane keeps working - so `status` reports it and `release_blockers`
    refuses on it.

    A `.used` marker on a *detached* worktree with no live lock record is a crash remnant, not a live claim -
    it is named reclaimable and the slot is free, so a crashed acquire can never wedge a slot.  It still
    reports its OWNER label: a reclamable marker that names somebody is exactly what `acquire` refuses
    (`marker_claim_conflict`) unless the takeover is meant.

    `runs` is the already-resolved live-run list, so `all_slots` reads the registry once instead of once per
    slot.
    """
    d = slot_dir(main, n)
    exists = os.path.isdir(d) and os.path.exists(os.path.join(d, ".git"))
    lock = read_lock(main, n)
    stale = lock_stale(main, n, lock) if lock else False
    live_lock = bool(lock and not stale)
    attached = slot_attached_branch(d) if exists else None
    marked = marker_present(d) if exists else False
    marker = marker_info(d) if marked else {}
    marker_stale = marked and not attached and not live_lock
    used = bool(attached) or (marked and not marker_stale)
    free = exists and not used
    run = None
    if exists:
        runs = runs_in_slot(d) if runs is None else [r for r in runs if _same_tree(r.get("cwd"), d)]
        run = runs[0] if runs else None
    owner = marker.get("owner") or (lock.get("worker") if lock else "") or ""
    if not exists:
        why = "no such slot directory"
    elif run:
        why = "in use by a RUNNING subagent run %s" % run_label(run)
    elif live_lock:
        why = "in use by %s (%s)" % (lock.get("unit") or "?", lock.get("worker") or "?")
    elif attached:
        why = "in use - holds branch %s (its `.used` marker is %s%s); release it first" % (
            attached, "present" if marked else "MISSING",
            ", owner %s" % owner if owner else "")
    elif marked and not marker_stale:
        why = "used (`.used` marker present, worktree detached%s)" % (", owner %s" % owner if owner else "")
    elif marker_stale:
        why = "stale `.used` marker on a detached worktree with no live lock - reclaimable%s" % (
            "; it names owner %s" % owner if owner else "")
    elif lock and stale:
        why = "stale lock (%s) - reclaimable" % (lock.get("unit") or "?")
    else:
        why = "free"
    return {"slot": n, "dir": d, "exists": exists, "attached": attached, "lock": lock, "stale": stale,
            "marked": marked, "marker": marker, "owner": owner, "marker_stale": marker_stale,
            "used": used, "free": free, "run": run, "why": why}


def all_slots(main: str, runs: list | None = None) -> list[dict]:
    """Every slot's row.  The registry is read **once** and handed to each row (`runs`)."""
    live = live_runs() if runs is None else runs
    return [slot_state(main, n, runs=live) for n in range(1, slot_count(main) + 1)]


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
                               capture_output=True, text=True, encoding="utf-8", errors="replace")
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
    acquires cannot both take the same slot, and a slot a racing acquire just marked is skipped too.  The
    `claim` callback is also where `acquire` refuses a sentinel that names another owner
    (`marker_claim_conflict`), i.e. *before* the mark, so a foreign claim is never silently absorbed.

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


def preview(main: str, unit: str, branch: str | None = None, slot: int | None = None,
            worker: str | None = None, force: bool = False) -> dict:
    """What `acquire` would do, touching nothing (for a dry run). -> the slot row and the command.

    The candidate slot's sentinel is checked here too, so a dry run surfaces the same
    "that `.used` belongs to another claim" refusal the real acquire would give.
    """
    claims = _claims()
    unit = claims.norm_unit(unit.strip("/"))
    branch = branch or claims.branch_for(unit)
    row = _pick_free(main, slot)
    conflict = marker_claim_conflict(row["dir"], owner_label(unit, branch, worker), row["slot"])
    if conflict and not force:
        raise SystemExit(conflict.split("\n")[0] + "\n  (a dry run reports the same refusal `acquire` gives)")
    tip = git(["rev-parse", "HEAD"], main)
    return {"slot": row["slot"], "dir": row["dir"], "branch": branch, "base": tip,
            "owner": owner_label(unit, branch, worker),
            "command": "git -C %s checkout -B %s %s" % (row["dir"], branch, tip)}


def acquire(main: str, unit: str, branch: str | None = None, worker: str | None = None,
            slot: int | None = None, force: bool = False) -> dict:
    """Take a slot for `unit`: mark it `.used`, reset it, cut a **fresh** branch off main's tip, verify, lock.

    Refuses, before touching anything, when the unit's branch already exists (the claim is taken), when an
    explicitly named slot is occupied - a slot whose previous branch has not landed is surfaced loudly, never
    silently reused - and when the slot's `.used` sentinel **names a different owner**
    (`marker_claim_conflict`): that is another claim's sentinel, and taking the slot under it is how a lane's
    HEAD got detached mid-run.  `force` is the deliberate override for a sentinel whose owner is gone.

    The slot is **marked `.used` atomically before the reset**, with this claim's OWNER label in the marker,
    so a racing acquire falls through to the next free slot instead of colliding; a failure after the mark
    removes it, and a crash leaves a marker the worktree reading reclaims.  The kept build tree is verified
    against MAIN's current map/DOL; if it cannot be proven current it is re-seeded, and if it still cannot,
    the acquire fails closed rather than handing the lane a stale tree.
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
                print("slot %d: MAIN's tree is current and report.json is byte-identical to MAIN's, so the only "
                      "remaining doubt is config.json's mtime (%s) - accepting; a stale build would have failed "
                      "report_matches" % (n, "; ".join(v["reasons"])))
                v = dict(v, ok=True, accepted_mtime_note=True)
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
            dry_run: bool = False, force: bool = False, allow_dirty: bool = False,
            registry: str | None = None) -> dict:
    """Return a slot to main's tip: detach, clean, delete its branch (rescue-ref first), refresh, unlock.

    Keeps the warm trees and leaves the slot pre-warmed so the next `acquire` is a validation, not a build.
    The branch is deleted because a *directory* never holds one; its commits are rescued first exactly as
    `claims.release` does, so a release can never be the only place unlanded work lived.

    **It fails closed.**  A release detaches HEAD, runs `clean -ffdx` and deletes the branch, so everything
    `release_blockers` can see - a RUNNING subagent run whose cwd is the slot (read from the harness' run
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
        raise SystemExit("REFUSED spawn: unknown kind %r - valid kinds are: %s"
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
    """Take a slot for a lane of `kind` and return the paste-ready launch: the `subagent({...})` line
    followed by the standard "your tree" block.

    **The slot is an explicit launch parameter.**  `slot=N` takes that slot *by number* through `acquire` -
    same fail-closed reset/seed, same refusal for a slot holding an unlanded branch, never a reimplementation
    - and no `slot` takes the first genuinely free slot and **names which one it took**, so the orchestrator
    chooses the slot and can see it instead of two racing lanes choosing the same one.

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
    if task:
        task_text = ("%s\n\nYou may fan out subagents. End your turn with your report: "
                     "your final message is the result the orchestrator receives." % task)
    else:
        task_text = ("No task text was given (`--task-file` was not passed). Replace this placeholder with "
                     "the lane's task. You may fan out subagents. End your turn with your report: your final "
                     "message is the result the orchestrator receives.")
    call = ("subagent(agent=%s, cwd=%s, task=%s)"
            % (json.dumps(profile), json.dumps(path.replace("\\", "/")), json.dumps(task_text)))
    block = tree_block(main, path)
    return {"slot": info["slot"], "path": path, "agent": profile, "kind": kind,
            "branch": info["branch"], "unit": unit, "spawnLine": "%s\n\n%s" % (call, block)}


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

    import shutil
    import tempfile
    claims = _claims()

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
        # AGENTS.md is committed with a real **em dash** in its prose, because a slot's AGENTS.md is dirty by
        # design (the LOCAL-ONLY block is live working state, rule 8) and `slot_dirty` has to tell that dirt
        # apart from real dirt.  The non-ASCII byte is what makes the comparison load-bearing: a decode that
        # used the host locale codec (`cp1252` here) would call every slot dirty (F34's failure, one file
        # over).  Written as UTF-8 explicitly - the fixture must not inherit the trap it guards against.
        with open(os.path.join(repo, "AGENTS.md"), "w", encoding="utf-8", newline="\n") as fh:
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
        reg_root = os.path.join(tmp, "pi-subagents-selftest", RUNS_DIRNAME)
        os.makedirs(reg_root)
        check("the harness' registry directory is discovered under the temp dir",
              run_registries(tmp), [reg_root])
        run_id = "11111111-2222-3333-4444-555555555555"

        def write_run(state="running", cwd=None):
            os.makedirs(os.path.join(reg_root, run_id), exist_ok=True)
            with open(os.path.join(reg_root, run_id, "status.json"), "w", encoding="utf-8") as fh:
                json.dump({"runId": run_id, "state": state, "cwd": cwd or d_live, "pid": os.getpid(),
                           "steps": [{"agent": "worker", "sessionName": "worker: fix the thing",
                                      "status": state}]}, fh)

        write_run()
        check("a RUNNING record is a live run", [r["run_id"] for r in live_runs(reg_root)], [run_id])
        check("... discovered through the temp dir too",
              [r["run_id"] for r in live_runs(temp=tmp)], [run_id])
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
        check("... and says a run holds the slot", "RUNNING subagent run" in live_row["why"], True)
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
        # the harness' own finish marker: the same record is finished and the release proceeds
        os.makedirs(os.path.join(reg_root, ".terminal-runs"), exist_ok=True)
        open(os.path.join(reg_root, ".terminal-runs", run_id), "w").close()
        check("a terminal-run marker retires the record", live_runs(reg_root), [])
        check("the finished run is no longer in the slot", runs_in_slot(d_live, reg_root), [])
        os.remove(os.path.join(reg_root, ".terminal-runs", run_id))
        write_run(state="finished")
        check("a finished state is not a live lane either", runs_in_slot(d_live, reg_root), [])
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

        # --- E2/E3: the slot's own live state (AGENTS.md's LOCAL-ONLY block) is not dirt ---------------
        blocky = acquire(repo, "auto/blocky", slot=1)
        d_blocky = blocky["dir"]
        check("a freshly acquired slot has a clean tree", slot_dirty(d_blocky), [])
        with open(os.path.join(d_blocky, "AGENTS.md"), "r", encoding="utf-8", newline="") as fh:
            base = fh.read()
        with open(os.path.join(d_blocky, "AGENTS.md"), "w", encoding="utf-8", newline="") as fh:
            fh.write(base + "<!-- LOCAL-ONLY-BEGIN: stripped before every commit, see Non-negotiables "
                     "rule 8 -->\nlive state, and an em dash \u2014\n<!-- LOCAL-ONLY-END -->\n")
        check("an AGENTS.md carrying only its LOCAL-ONLY block is not dirt", slot_dirty(d_blocky), [])
        check("... so a release needs no override", release_blockers(repo, 1, d_blocky), [])
        with open(os.path.join(d_blocky, "AGENTS.md"), "a", encoding="utf-8", newline="") as fh:
            fh.write("a real edit below the block \u2014\n")
        check("a real AGENTS.md edit IS dirt", any("AGENTS.md" in r for r in slot_dirty(d_blocky)), True)
        release(repo, slot=1, unit="auto/blocky", force=True)
        check("... and the reset puts the committed AGENTS.md back",
              open(os.path.join(d_blocky, "AGENTS.md"), encoding="utf-8").read().count("LOCAL-ONLY"), 0)

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
              {"unit": "decompiler", "fix": "fixer", "merge": "merger", "tooling": "worker",
               "docs": "worker", "review": "codereviewer", "scout": "scout", "plan": "planner"})
        for _kind, _profile in sorted(KIND_PROFILE.items()):
            check("kind %s -> %s" % (_kind, _profile), profile_for_kind(_kind), _profile)
        try:
            profile_for_kind("bogus")
            check("an unknown kind is refused (never guessed)", "no error", "SystemExit")
        except SystemExit as exc:
            check("an unknown kind is refused (never guessed)", "unknown kind" in str(exc), True)
            check("... listing every valid kind", all(k in str(exc) for k in KIND_PROFILE), True)

        # (a) BY NUMBER: takes THAT slot, and the line's cwd is it and its agent is the mapped profile
        sp = spawn(repo, "tooling", slot=1, unit="lane/spawn-tooling", task="Wire up the tool.")
        check("spawn takes the slot BY NUMBER", sp["slot"], 1)
        check("... cwd is that slot", sp["path"].replace("\\", "/"),
              slot_dir(repo, 1).replace("\\", "/"))
        check("... the kind maps to `worker`", sp["agent"], "worker")
        check("... the slot is on the claim's branch", slot_attached_branch(slot_dir(repo, 1)), sp["branch"])
        check("... the line is a subagent call with that agent and cwd",
              sp["spawnLine"].startswith('subagent(agent="worker", cwd="%s"'
                                          % slot_dir(repo, 1).replace("\\", "/")), True)
        check("... and carries task/slot/path/agent/kind/spawnLine",
              all(k in sp for k in ("slot", "path", "agent", "kind", "spawnLine")), True)
        check("... the block carries the `rev-parse --show-toplevel` self-check",
              "git rev-parse --show-toplevel" in sp["spawnLine"], True)
        check("... and the STOP-and-report rule", "STOP and report it" in sp["spawnLine"], True)
        check("... and the write-nothing-outside rule",
              "nothing" in sp["spawnLine"].lower() and "outside it" in sp["spawnLine"].lower(), True)
        check("... and the claims.py release prohibition", "claims.py release" in sp["spawnLine"], True)
        check("... and the do-not-land rule", "Do not land" in sp["spawnLine"], True)
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
        check("... and `unit` maps to `decompiler`", unit_sp["agent"], "decompiler")
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
        print("  owner   %s (in `.used`)" % marker_owner(info["dir"]))
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
    if args.cmd == "spawn":
        out = spawn(main_wt, args.kind, slot=args.slot, unit=args.unit, task_file=args.task_file,
                    worker=args.worker, force=args.force)
        if args.json:
            # exactly the fields a caller needs; `spawnLine` is the whole paste-ready text (line + block)
            print(json.dumps({k: out[k] for k in ("slot", "path", "agent", "kind", "spawnLine")},
                             indent=2))
            return 0
        # stdout is exactly the paste-ready artifact (the subagent line + the block); the header names the
        # slot that was taken on stderr, so a caller can capture stdout verbatim.
        print("spawn slot %d (%s) for kind %s -> agent %s\n  path: %s"
              % (out["slot"], out["branch"], out["kind"], out["agent"], out["path"]),
              file=sys.stderr)
        print(out["spawnLine"])
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
            lock = row.get("lock") or {}
            if row["free"]:
                state = "free"
            elif row.get("run"):
                state = "LIVE"
            elif row["attached"]:
                state = "in use"
            else:
                state = "used"
            marker = "stale" if row.get("marker_stale") else ("yes" if row["marked"] else "-")
            run = (row["run"]["run_id"][:8] if row.get("run") else "-")
            print("%-5d %-9s %-16s %-20s %-20s %-7s %-9s %s" % (
                row["slot"], state, (row.get("owner") or "-")[:16], (lock.get("unit") or "-")[:20],
                (row["attached"] or "detached")[:20], marker, run,
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
