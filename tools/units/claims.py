"""Claim a unit for a worker: one git worktree, one branch, and the branch *is* the lock.

docs/plan.md 7.2. With up to twelve externally spawned worker agents, "who owns this unit" cannot live in
anyone's memory, and a lock file has to be trusted. Git already has an atomic one: creating a branch either
succeeds or fails, and two worktrees cannot share a branch name. So a claim is

    git worktree add -b worker/<slug> <sibling>.ws-<slug> <main's HEAD>

and the unit-to-worker mapping, the batch base sha and the timestamps live in `MAIN/.pi/claims.json` - a
convenience registry on top of git, never the source of truth (git is: `list` reconstructs from it when the
registry is missing).

    python tools/units/claims.py claim <unit> [--worker NAME] [--dry-run] [--json]
    python tools/units/claims.py list [--json]
    python tools/units/claims.py release <unit> [--force] [--dry-run]
    python tools/units/claims.py release --all-merged [--dry-run]
    python tools/units/claims.py expire [--minutes N] [--apply]
    python tools/units/claims.py --selftest

`<unit>` is the path from the repository root (`Pl/pl_act`, `auto/80040598_fn_80040598`).

`release` is the **one-shot, idempotent** teardown (docs/plan.md, "Teardown is part of landing"): rescue ref ->
pane close -> `git worktree remove --force` -> `branch -D` -> `prune` -> registry entry (+ the ack file). Every
step reports what it did or why it was skipped, so releasing an already-released claim (or one whose worktree was
already removed by hand) is a clean no-op instead of an abort. The one refusal that stays is a **live pane**,
because Windows will not delete a directory a process is sitting in (5.1); it is named in the message. The exit
status says whether the teardown is complete. `release --all-merged` sweeps every claim whose branch is already
merged into main, releasing what it can and naming what it skipped.

A claim is only ever declared `stalled` from the ack *and* the worker's own pane: `herdr pane list`
is matched to the claim by its worktree name, and `herdr pane read` is sampled twice - a pane whose
content moves is a worker that is alive, whatever its ack file says. A live pane is therefore never
reclaimed on a stale ack, and `timeout --apply` closes the pane *before* it touches the worktree, because
the pane pins the worktree as its cwd on Windows (docs/plan.md 5.1).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import recompile as rc  # noqa: E402

SLUG_MAX = 40
BRANCH_PREFIX = "worker/"
# how long to watch a pane's own output before believing it is idle; a working pi TUI redraws its spinner
# and token counter faster than this, a pane whose agent exited sits on a static shell prompt
PROBE_SECONDS = 3.0
PANE_READ_LINES = 40


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, errors="replace")
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, out.stderr.strip()))
    return out.stdout


def herdr_exe() -> str:
    """The herdr binary: the one the running session exports, else whatever is on `PATH`."""
    return os.environ.get("HERDR_BIN_PATH") or "herdr"


def herdr_panes(runner=subprocess.run) -> dict[str, dict] | None:
    """The live panes herdr reports, keyed by `pane_id` - `None` when herdr cannot be asked.

    `None` is a real answer (no herdr, a broken socket, an unparseable reply) and callers must not read it
    as "no panes": on a host without herdr the ack-only rules are all there is, and the pane layer is a
    safety net, not a new single point of failure.
    """
    try:
        out = runner([herdr_exe(), "pane", "list"], capture_output=True, text=True,
                     errors="replace", timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode != 0:
        return None
    try:
        data = json.loads(out.stdout or "")
    except json.JSONDecodeError:
        return None
    panes = (data.get("result") or {}).get("panes")
    if not isinstance(panes, list):
        return None
    return {p["pane_id"]: p for p in panes if isinstance(p, dict) and p.get("pane_id")}


def herdr_read(pane_id: str, runner=subprocess.run) -> str | None:
    """A pane's own output - `None` when it cannot be read (usually because the pane is gone)."""
    try:
        out = runner([herdr_exe(), "pane", "read", pane_id, "--lines", str(PANE_READ_LINES),
                      "--format", "text"], capture_output=True, text=True, errors="replace", timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
    return out.stdout if out.returncode == 0 else None


def herdr_close(pane_id: str, runner=subprocess.run, lister=None) -> tuple[bool, str]:
    """Close a pane and confirm it is gone. `(ok, why)` - `why` is empty on success.

    Called before the worktree is removed, never after: on Windows a pane holds its worktree as its cwd and
    a `git worktree remove` against a live pane half-succeeds (it deletes `.git` and then fails on the
    directory), which is exactly how `Pl/pl_master`'s worktree got detached on 2026-09-23.
    """
    try:
        out = runner([herdr_exe(), "pane", "close", pane_id], capture_output=True, text=True,
                     errors="replace", timeout=15)
    except (OSError, subprocess.SubprocessError) as exc:
        return False, str(exc)
    if out.returncode != 0:
        return False, (out.stderr or out.stdout or "herdr pane close failed").strip()
    panes = (lister or herdr_panes)()
    if panes is not None and pane_id in panes:
        return False, "pane %s is still listed after close" % pane_id
    return True, ""


def resolve_pane(row: dict, panes: dict[str, dict]) -> str | None:
    """The live pane that belongs to a claim, matched by the claim's worktree name.

    A worker cannot be trusted to name its own pane (the 2026-09-23 acks record `"pane": "pl-act-09c6"`,
    an agent name, not a pane id), so the pane's title - which herdr sets to the worktree the pane started
    in - is the join. An explicit pane id that herdr actually lists wins, for the round where the worker
    does pass `$HERDR_PANE_ID`.
    """
    explicit = row.get("pane")
    if explicit and explicit in panes:
        return explicit
    base = os.path.basename((row.get("worktree") or "").rstrip("\\/")).lower()
    if not base:
        return None
    for pane_id, info in panes.items():
        task = (info.get("tokens") or {}).get("task") or ""
        hay = " ".join(str(info.get(k) or "") for k in ("terminal_title", "terminal_title_stripped",
                                                          "label")) + " " + task
        if base in hay.lower():
            return pane_id
    return None


def panes_for_claim(row: dict, panes: dict[str, dict] | None, slug_hint: str | None = None) -> list[str]:
    """Every pane a claim owns: the worker's own pane (by worktree) plus the panes of subagents it spawned.

    A subagent runs in its own herdr pane and nothing owns that pane when its parent finishes, so three
    `ef-big*` panes and an `ef-source-hunt` pane outlived their worker (2026-09-23) and stayed in herdr.
    The join is the **label**: a pane's `label` is the agent name, which the worker chose, and the convention
    is to prefix a subagent with the claim's slug. A pane whose label starts with the claim's slug therefore
    belongs to the claim and teardown closes it. Only the full slug is matched - the slug's bare stem would
    also catch a sibling unit's subagent (`pl-act-*` vs `pl-act-skill-*`), and closing another worker's
    pane mid-round is worse than leaving a grandchild behind.
    """
    if not panes:
        return []
    found: list[str] = []
    own = resolve_pane(row, panes)
    if own:
        found.append(own)
    hint = (slug_hint or "").strip().lower()
    if hint:
        for pane_id, info in panes.items():
            if pane_id in found:
                continue
            label = str(info.get("label") or "").strip().lower()
            if label.startswith(hint):
                found.append(pane_id)
    return found


def pane_probe(row: dict, interval: float | None = None, lister=None, reader=None,
               sleeper=None) -> dict:
    """What the claim's pane itself shows: `known`, `alive`, `active`, `status`.

    `active` is decided from herdr's own signals: an `agent_status` of `working`/`blocked` is activity, and
    otherwise the pane's output is read twice and a change between the two reads is activity. `revision` is
    recorded but not trusted on its own - a pi pane draws to the alternate screen, so its content moves while
    `revision` sits still (measured 2026-09-23: `w1:pK` revision 13, content changing every second).
    """
    interval = PROBE_SECONDS if interval is None else interval
    sleeper = sleeper or time.sleep
    panes = (lister or herdr_panes)()
    if panes is None:
        return {"pane": row.get("pane"), "known": False, "alive": None, "active": None,
                "status": None, "revision": None}
    pane_id = resolve_pane(row, panes)
    if pane_id is None:
        return {"pane": row.get("pane"), "known": True, "alive": False, "active": False,
                "status": None, "revision": None}
    info = panes.get(pane_id) or {}
    status = info.get("agent_status")
    revision = info.get("revision")
    if status in ("working", "blocked"):
        return {"pane": pane_id, "known": True, "alive": True, "active": True, "status": status,
                "revision": revision}
    reader = reader or herdr_read
    first = reader(pane_id)
    if first is None:
        return {"pane": pane_id, "known": True, "alive": True, "active": None, "status": status,
                "revision": revision}
    if interval:
        sleeper(interval)
    second = reader(pane_id)
    return {"pane": pane_id, "known": True, "alive": True,
            "active": second is not None and second != first, "status": status, "revision": revision}


def slug(unit: str) -> str:
    """A stable, filesystem- and git-safe name for a unit path, unique by a short hash of the full path."""
    # the unit's basename is what a reader recognises; the hash of the full path keeps it unique,
    # since two units can share a basename (Pl/pl_act vs auto/pl_act)
    stem = os.path.splitext(os.path.basename(unit.strip("/")))[0]
    cleaned = re.sub(r"[^A-Za-z0-9]+", "-", stem).strip("-").lower()
    digest = hashlib.sha1(unit.strip("/").encode("utf-8")).hexdigest()[:4]
    room = SLUG_MAX - len(digest) - 1
    return "%s-%s" % (cleaned[:room].strip("-"), digest)


def branch_for(unit: str) -> str:
    return BRANCH_PREFIX + slug(unit)


def worktree_for(unit: str, main: str) -> str:
    """A sibling of MAIN, following the repository's existing `<repo>.ws-<stream>` convention."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s.ws-%s" % (os.path.basename(main), slug(unit)))


def registry_path(main: str) -> str:
    return os.path.join(main, ".pi", "claims.json")


def norm_unit(unit: str) -> str:
    """The key a claim is held under: the unit path without its source extension.

    The registry is keyed by the unit's *name*, and the same unit reaches it spelled two ways - `auto/X` and
    `auto/X.c` - which is enough to defeat the mutex: on 2026-09-23 two breadth workers claimed
    `auto/802B2978_fn_802B2978` under those two spellings and duplicated the round. Strip the extension at every
    entry point so the branch (the real lock) is only ever created once.
    """
    for ext in (".cpp", ".cp", ".c"):
        if unit.endswith(ext):
            return unit[: -len(ext)]
    return unit


def seed_worktree_build(main: str, wt: str) -> str:
    """Give a fresh worktree its own `build/tools` (the toolchain is ~15 MB of read-only binaries).

    A worktree must be self-sufficient: the worker compiles and measures in *its* `build/RMHE08`, and MAIN's
    `build/` belongs to the orchestrator (docs/plan.md 5.1 - several workers share the machine, and one build
    tree cannot take two builds). Seeding this at claim time is why no worker has to junction into MAIN's.
    """
    src = os.path.join(main, "build", "tools")
    dst = os.path.join(wt, "build", "tools")
    if not os.path.isdir(src):
        return "skipped (MAIN has no build/tools yet - run `ninja tools`)"
    os.makedirs(dst, exist_ok=True)
    copied = 0
    for name in sorted(os.listdir(src)):
        s, d = os.path.join(src, name), os.path.join(dst, name)
        if os.path.isfile(s) and not os.path.exists(d):
            shutil.copy2(s, d)
            copied += 1
    return "seeded %d file(s)" % copied


def load_registry(main: str) -> dict:
    path = registry_path(main)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def save_registry(main: str, data: dict) -> None:
    os.makedirs(os.path.dirname(registry_path(main)), exist_ok=True)
    with open(registry_path(main), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)


def registry_record(main: str, unit: str) -> dict:
    """The claim record for `unit`, found under *either* spelling of its name.

    The registry key is the unit's name without its source extension (`norm_unit`), but a claim written before
    that rule existed can still carry the extension (`Gecko/Gecko_ExceptionPPC.cp`). Look the raw spelling up
    first, then the normalized one, then scan: a caller that says `Camellia/camellia.c` and a registry that
    says `Camellia/camellia` (or the reverse) must reach the same record - otherwise the branch - the lock -
    looks missing and a worker's outbox is read from the wrong slug.
    """
    registry = load_registry(main)
    unit = unit.strip("/")
    if unit in registry:
        return registry[unit]
    normalized = norm_unit(unit)
    if normalized in registry:
        return registry[normalized]
    for key, record in registry.items():
        if norm_unit(key) == normalized:
            return record
    return {}


def claim_branch(main: str, unit: str) -> str:
    """The branch holding `unit`'s claim - the stored branch when there is one, else `branch_for(unit)`.

    Prefer the stored value: the branch is the claim's identity and a round may have named it with its own
    suffix, so recomputing it from the unit path can miss a live branch.
    """
    return (registry_record(main, unit).get("branch") or branch_for(norm_unit(unit.strip("/"))))


def branch_exists(main: str, branch: str) -> bool:
    out = subprocess.run(["git", "show-ref", "--verify", "--quiet", "refs/heads/%s" % branch],
                         cwd=main, capture_output=True)
    return out.returncode == 0


def merged_into_main(main: str, branch: str) -> bool:
    """True when every commit of `branch` is already reachable from main - merged *or* cherry-picked."""
    out = subprocess.run(["git", "cherry", "main", branch], cwd=main, capture_output=True, text=True)
    if out.returncode != 0:
        return False
    return not [l for l in out.stdout.splitlines() if l.startswith("+")]


def slug_of_branch(branch: str | None) -> str | None:
    """The handoff slug a branch names: `worker/<slug>` with the prefix removed; `None` for an empty branch.

    The branch *is* the claim's identity - it is the lock - and a round may name it with a suffix of its own, so
    the name a worker keys its outbox by is read from the branch, never re-derived from the unit path. The two
    agree for a branch `claim` made itself (`worker/` + `slug(unit)`), which is exactly why the difference only
    shows up when one of them changes.
    """
    branch = branch or ""
    if branch.startswith(BRANCH_PREFIX):
        return branch[len(BRANCH_PREFIX):]
    return branch or None


def rescue_ref_name(unit: str) -> str:
    """Where a release parks an un-merged branch's commits: `refs/rescue/<slug>`.

    The branch is deleted (it is the lock and must go), so this ref is the worker's only remaining copy. A
    later `land.py` gate that finds the branch missing can name the exact restore command from it instead of
    the bare "no branch" that cost a round on 2026-09-23.
    """
    return "refs/rescue/%s" % slug(norm_unit(unit.strip("/")))


def rescue_exists(main: str, unit: str) -> str | None:
    """The rescue ref holding `unit`'s work, or `None` when there is none."""
    ref = rescue_ref_name(unit)
    out = subprocess.run(["git", "show-ref", "--verify", "--quiet", ref], cwd=main, capture_output=True)
    return ref if out.returncode == 0 else None


def claim_slug(main: str, unit: str) -> str | None:
    """The handoff slug of the unit's active claim, or `None` when the unit is unclaimed.

    Spelled either way: `registry_record` finds the entry whether the caller or the registry key carries the
    source extension.
    """
    return slug_of_branch(registry_record(main, unit).get("branch"))


def handoff_slug(main: str, unit: str) -> str:
    """The slug a worker's outbox and notes are named by: the claim's branch slug, else `slug(unit)`.

    An unclaimed unit has no branch and therefore no real outbox; the `slug(unit)` fallback exists only so a
    tool can print a path to look at. `brief.py` uses the same fallback for the brief's own file name, and says
    plainly that such a unit is unclaimed.
    """
    return claim_slug(main, unit) or slug(unit)


def outbox_path(main: str, unit: str) -> str:
    """The handoff entry `land.py`'s gate reads: `<branch minus worker/>.json` (see `handoff_slug`)."""
    return os.path.join(main, ".pi", "outbox", handoff_slug(main, unit) + ".json")


def notes_path(main: str, unit: str) -> str:
    """The worker's notes file, named by the same slug as the outbox."""
    return os.path.join(main, ".pi", "notes", handoff_slug(main, unit) + ".md")


def ack_path(main: str, unit: str) -> str:
    """The heartbeat, keyed by the *unit*: `claims.py ack` writes it, so the brief has to name this file.

    Deliberately unit-keyed, not claim-keyed: a round's branch suffix must not move the heartbeat, or a
    worker's own ack would land somewhere the stall check is not looking. The worker acks with the spelling
    its brief names, which is the spelling the claim was made with.
    """
    return os.path.join(main, ".pi", "ack", slug(unit) + ".json")


def load_ack(main: str, unit: str) -> dict:
    path = ack_path(main, unit)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def ack(unit: str, main: str, agent: str | None = None, pane: str | None = None,
        progress: str | None = None) -> dict:
    """A worker's heartbeat: its FIRST action, and again after every measured iteration.

    `agent_status` in a terminal multiplexer cannot tell "finished" from "never started" - both read as idle,
    and one of two workers sat idle for forty minutes because of it. The ack file is the worker's own promise
    that it is alive, and its `progress` list is the evidence that it is moving. The orchestrator's `status`
    combines it with the artefacts that cannot lie (commits, the outbox) and with the pane itself.

    Called from inside a worker, `--pane` and `--agent` are optional: herdr exports the pane id to every
    process in the pane as `HERDR_PANE_ID`, and the brief's `--pane <your-pane>` cannot be filled in by an
    agent that does not know its own id. Omitting them records the real pane, which is what the stall rule
    matches on - a wrong value only costs the pane safety net, never the ack itself.
    """
    unit = unit.strip("/")
    path = ack_path(main, unit)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    data = load_ack(main, unit) or {"unit": unit}
    record = load_registry(main).get(unit, {})
    now = time.strftime("%Y-%m-%dT%H:%M:%S")
    if pane is None:
        pane = os.environ.get("HERDR_PANE_ID")
    if agent is None:
        agent = os.environ.get("HERDR_AGENT") or pane
    data.setdefault("worker", record.get("worker"))
    data.setdefault("branch", record.get("branch"))
    data.setdefault("acked_at", now)
    if agent:
        data["agent"] = agent
    if pane:
        data["pane"] = pane
    if progress:
        data.setdefault("progress", []).append({"at": now, "note": progress})
    data["last_progress_at"] = now
    data["iterations"] = len(data.get("progress") or [])
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)
    return data


def _age_seconds(stamp: str | None) -> float | None:
    if not stamp:
        return None
    try:
        return max(0.0, time.time() - time.mktime(time.strptime(stamp, "%Y-%m-%dT%H:%M:%S")))
    except ValueError:
        return None


def claim_status(main: str, ack_seconds: float = 120, stall_minutes: float = 20,
                 probe=None, view=None) -> list[dict]:
    """Every claim with the state a supervisor needs: unacked, stalled, working or done.

    The state is decided from the ack file *and* from what git and the filesystem can prove - so a worker that
    never acked but whose branch has commits is reported as `working`, and a worker that acked and then went
    quiet is `stalled`. `unacked` past the grace period is the case that cost forty minutes in the first round.

    A claim the ack calls unhealthy is then asked of its **pane**, because a working agent does not reliably
    advance the ack (two reclaims on 2026-09-23 fired on workers that were mid-turn). A pane that exists and
    is moving - or whose output cannot be read at all - keeps the claim `working`; only a pane that is gone or
    confirmed static makes the ack's verdict stand. When herdr cannot be asked (`probe` returns
    `known: False`) the ack-only rules are unchanged, so the tool still works on a host without herdr.
    """
    registry = load_registry(main)
    rows = []
    for row in (claims_view(main) if view is None else view):
        unit = row["unit"]
        record = registry.get(unit, {})
        data = load_ack(main, unit) if unit != "(unregistered)" else {}
        claimed = _age_seconds(row.get("claimed_at"))
        acked = _age_seconds(data.get("acked_at"))
        progress = _age_seconds(data.get("last_progress_at"))
        commits = 0
        if row.get("branch") and row.get("base"):
            out = subprocess.run(["git", "rev-list", "--count", "%s..%s" % (row["base"], row["branch"])],
                                 cwd=main, capture_output=True, text=True)
            if out.returncode == 0 and out.stdout.strip().isdigit():
                commits = int(out.stdout.strip())
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
                     "agent": data.get("agent") or (record.get("spawn") or {}).get("agent"),
                     "pane": data.get("pane") or (record.get("spawn") or {}).get("pane") or row.get("pane")})
    # the pane is consulted only where the ack says "reclaim me": a `done` or already-`working` claim needs no
    # probe, and a probe costs a couple of seconds, so the healthy path stays free
    probe = probe or pane_probe
    for row in rows:
        if row["state"] not in ("unacked", "stalled"):
            continue
        info = probe(row)
        row["pane_alive"] = info.get("alive")
        row["pane_active"] = info.get("active")
        row["pane_status"] = info.get("status")
        row["pane_revision"] = info.get("revision")
        if info.get("pane"):
            row["pane"] = info["pane"]
        # `alive is False` (pane gone) and `active is False` (pane confirmed static) both leave the ack's
        # verdict standing; anything else - a live pane that is moving, or one we could not read - is a worker
        # we must not reclaim
        if info.get("known") and info.get("alive") and info.get("active") is not False:
            row["state"] = "working"
            row["pane_note"] = "pane %s is %s" % (info.get("pane"),
                                                   "active" if info.get("active") else "unreadable")
    return rows


def _close_step(pane: str, close) -> callable:
    """A plan step that raises (stopping the teardown) when the pane will not close."""
    def run():
        ok, why = close(pane)
        if not ok:
            raise SystemExit("pane not closed: %s" % why)
    return run


def timeout(main: str, ack_seconds: float = 120, stall_minutes: float = 20,
            apply: bool = False, unit: str | None = None, view=None, probe=None,
            close=None, lister=None) -> list[dict]:
    """Reclaim what a silent worker holds: rescue its commits, close its pane, then free the unit.

    A worker's branch is the lock, so a timed-out claim has to *release* that lock - but a branch can hold real
    work, so the commits are copied to `refs/rescue/<slug>` first and the rescue ref is printed. Nothing is
    destroyed, and the unit can be claimed again immediately.

    The order is load-bearing: a live pane pins its worktree as its cwd, so `git worktree remove` against a
    pane that is still open deletes `.git` and *then* fails on the directory (docs/plan.md 5.1) - the claim is
    gone and the worktree is detached from git. So the pane is closed first, and if it will not close the whole
    teardown is abandoned with a reason and the claim is left alone.
    """
    close = close or herdr_close
    all_panes = (lister or herdr_panes)()
    victims = []
    for row in claim_status(main, ack_seconds, stall_minutes, probe=probe, view=view):
        if unit and row["unit"] != unit.strip("/"):
            continue
        if row["state"] in ("unacked", "stalled") or (unit and row["state"] != "done"):
            victims.append(row)
    # an explicit `--unit` may force a claim the ack calls healthy, but never one whose pane is moving: the
    # whole point of the pane layer is that a busy worker is not reclaimed, named or not
    victims = [r for r in victims if r.get("pane_active") is not True]
    for row in victims:
        slugged = slug(row["unit"])
        rescue = "refs/rescue/%s" % slugged
        branch = row.get("branch")
        worktree = row.get("worktree")
        pane = row.get("pane")
        # a pane we know is gone (`pane_alive is False`) needs no close; one we know is live, or could not
        # check because herdr was unavailable, must be closed before its worktree may go
        plan = []  # (label, callable)
        if branch and row.get("commits"):
            plan.append(("git update-ref %s %s" % (rescue, branch),
                         lambda b=branch: git(["update-ref", rescue, b], main)))
        if pane and row.get("pane_alive") is not False:
            plan.append(("herdr pane close %s" % pane, _close_step(pane, close)))
        # a subagent's pane has no other owner and outlives its parent, so a stalled worker's grandchildren
        # are closed here too (`panes_for_claim`); only the worker's own pane is a refusal, a child is a step
        for child in [p for p in panes_for_claim({"worktree": worktree, "pane": pane}, all_panes, slugged)
                      if p != pane]:
            plan.append(("herdr pane close %s (subagent pane)" % child, _close_step(child, close)))
        if worktree and os.path.isdir(worktree):
            plan.append(("git worktree remove --force %s" % worktree,
                         lambda w=worktree: git(["worktree", "remove", "--force", w], main)))
        if branch and branch_exists(main, branch):
            plan.append(("git branch -D %s" % branch, lambda b=branch: git(["branch", "-D", b], main)))
        plan.append(("git worktree prune", lambda: git(["worktree", "prune"], main)))
        row["steps"] = [label for label, _action in plan]
        row["rescue"] = rescue if row.get("commits") else None
        if apply:
            for label, action in plan:
                try:
                    action()
                except SystemExit as exc:
                    # a step refused - stop before anything else is touched and leave the claim in place
                    row["error"] = "%s: %s" % (label, exc)
                    break
            else:
                registry = load_registry(main)
                registry.pop(row["unit"], None)
                save_registry(main, registry)
                ack_file = ack_path(main, row["unit"])
                if os.path.exists(ack_file):
                    os.remove(ack_file)
    return victims


def claims_view(main: str) -> list[dict]:
    """Every worker claim git or the registry knows about, enriched with what the worker left behind."""
    registry = load_registry(main)
    rows: list[dict] = []
    seen: set[str] = set()
    for entry in rc.main_worktree_list(main):
        branch = entry.get("branch") or ""
        if not branch.startswith(BRANCH_PREFIX):
            continue
        seen.add(branch)
        unit = next((u for u, v in registry.items() if v.get("branch") == branch), None)
        record = registry.get(unit or "", {})
        rows.append({
            "unit": unit or "(unregistered)",
            "branch": branch,
            "worktree": entry["path"],
            "worker": record.get("worker"),
            "claimed_at": record.get("claimed_at"),
            "base": record.get("base"),
            "merged": merged_into_main(main, branch),
            "outbox": bool(unit and os.path.exists(outbox_path(main, unit))),
            "exists": os.path.isdir(entry["path"]),
        })
    # A registry entry whose worktree git no longer lists is the half-torn-down state: `git worktree remove`
    # deleted the worktree's `.git` and then failed on the directory because a live pane pinned it (plan 5.1).
    # It has to stay visible or `status` cannot report it and `timeout` cannot ask its pane before reclaiming.
    for unit, record in registry.items():
        branch = record.get("branch")
        if branch in seen:
            continue
        path = record.get("worktree") or worktree_for(unit, main)
        rows.append({
            "unit": unit,
            "branch": branch,
            "worktree": path,
            "worker": record.get("worker"),
            "claimed_at": record.get("claimed_at"),
            "base": record.get("base"),
            "merged": bool(branch) and merged_into_main(main, branch),
            "outbox": os.path.exists(outbox_path(main, unit)),
            "exists": os.path.isdir(path),
        })
    return sorted(rows, key=lambda r: r["unit"])


def claim(unit: str, main: str, worker: str | None, dry_run: bool) -> dict:
    unit = norm_unit(unit.strip("/"))
    branch = branch_for(unit)
    path = worktree_for(unit, main)
    if branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)
    if os.path.exists(path):
        raise SystemExit("REFUSED: %s already exists" % path)
    base = git(["rev-parse", "HEAD"], main).strip()
    cmd = ["worktree", "add", "-b", branch, path, base]
    if dry_run:
        return {"unit": unit, "branch": branch, "worktree": path, "base": base,
                "command": "git " + " ".join(cmd), "dry_run": True}
    git(cmd, main)
    seed_note = seed_worktree_build(main, path)
    # the worker writes its outbox and notes here (docs/plan.md 5.3); create them with the claim so the
    # paths in the brief exist before the worker tries to write to them
    for sub in ("outbox", "notes"):
        os.makedirs(os.path.join(main, ".pi", sub), exist_ok=True)
    registry = load_registry(main)
    registry[unit] = {"branch": branch, "worktree": path, "worker": worker or os.environ.get("USERNAME")
                      or os.environ.get("USER") or "unknown", "base": base,
                      "claimed_at": time.strftime("%Y-%m-%dT%H:%M:%S")}
    save_registry(main, registry)
    return {"unit": unit, "branch": branch, "worktree": path, "base": base}


def registry_key(registry: dict, unit: str) -> str | None:
    """The key `unit` is stored under - the extensionless spelling when present, else any equivalent key.

    The registry is keyed by the unit's name without its source extension (`norm_unit`), but a record written
    before that rule existed can still carry the extension (`Gecko/Gecko_ExceptionPPC.cp`). A `release` that
    looked the extensionless key up directly would miss the old record, find the default branch instead of the
    stored one, and leave the registry entry behind - exactly the half-torn-down state this tool exists to end.
    """
    unit = norm_unit(unit.strip("/"))
    if unit in registry:
        return unit
    for key in registry:
        if norm_unit(key) == unit:
            return key
    return None


def commits_ahead(main: str, branch: str) -> int:
    """How many commits `branch` has that main does not - 0 when either is missing."""
    if not branch or not branch_exists(main, branch):
        return 0
    p = subprocess.run(["git", "rev-list", "--count", "main..%s" % branch],
                       cwd=main, capture_output=True, text=True, errors="replace")
    return int(p.stdout.strip()) if p.returncode == 0 and p.stdout.strip().isdigit() else 0


def registered_worktree_paths(main: str) -> list[str] | None:
    """Every path `git worktree list` knows, or `None` when git could not be asked."""
    p = subprocess.run(["git", "worktree", "list", "--porcelain"], cwd=main, capture_output=True, text=True,
                       errors="replace")
    if p.returncode != 0:
        return None
    return [line[len("worktree "):].strip() for line in p.stdout.splitlines()
            if line.startswith("worktree ")]


def _same_path(a: str, b: str) -> bool:
    return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))


def remove_worktree(main: str, path: str) -> str:
    """Remove one worktree, repairing the two states git refuses. -> what was done.

    Two refusals are known and both have a repair, which is the difference between an abort and a teardown:

    * a **submodule** inside the worktree (`tools/m2c`, plan 5.1) needs the force flag twice - otherwise the
      worktree and therefore the branch (the lock) outlive their round forever;
    * a directory git **no longer tracks** is the 2026-09-23 half-teardown (`git worktree remove` deleted
      `.git` and then failed on the directory because a live pane pinned it). Nothing else can clear it, and it
      blocks the next claim's `git worktree add`, so the leftover tree is removed here.
    """
    try:
        git(["worktree", "remove", "--force", path], main)
        return "removed"
    except SystemExit as first:
        message = str(first)
        if "submodule" in message:
            git(["worktree", "remove", "--force", "--force", path], main)
            return "removed (force twice, for the submodule)"
        known = registered_worktree_paths(main)
        if known is not None and not any(_same_path(p, path) for p in known):
            shutil.rmtree(path)
            return "removed the leftover directory (git no longer tracked it)"
        raise


def _done_step(label: str, action) -> dict:
    """A teardown step: `action` returns a note (or None) on success, raising to fail the teardown."""
    return {"label": label, "action": action, "status": "planned", "why": ""}


def _skip_step(label: str, why: str) -> dict:
    """A step that has nothing to do, with the reason recorded instead of hidden."""
    return {"label": label, "action": None, "status": "skipped", "why": why}


def _run_teardown(steps: list[dict]) -> bool:
    """Run a teardown plan in order. -> complete.

    A step that is already `skipped` stays skipped. The first failure stops the plan - the remaining steps are
    recorded as "not attempted" rather than run against a half-torn-down claim (removing a branch whose worktree
    is still live, or a worktree behind a pane that would not close, is how the 2026-09-23 tangle started) - and
    the teardown is reported incomplete so the registry entry and the ack survive for the next attempt.
    """
    complete, failed = True, None
    for step in steps:
        if step["status"] == "skipped":
            continue
        if failed:
            step["status"] = "skipped"
            step["why"] = "not attempted: %s failed first" % failed
            continue
        try:
            note = step["action"]()
            step["status"] = "done"
            if note:
                step["why"] = note
        except (SystemExit, Exception) as exc:  # noqa: BLE001 - any refuser must be recorded, not propagated
            detail = str(exc).strip().splitlines()
            step["status"] = "failed"
            step["why"] = detail[0] if detail else exc.__class__.__name__
            complete = False
            failed = step["label"]
    return complete


def release(unit: str, main: str, force: bool, dry_run: bool, probe=None, close=None,
            lister=None) -> dict:
    """The one-shot teardown of a claim: rescue ref, pane close, worktree, branch, prune, registry, ack.

    Idempotent and total (docs/plan.md, "Teardown is part of landing"): every step reports what it did or why
    it was skipped, so a claim whose worktree was already removed by hand, or one released twice, tears down
    cleanly instead of aborting and leaving the branch - the lock - alive. A **live pane** is the one refusal
    that stays: it pins the worktree as its cwd on Windows (plan 5.1), so the teardown stops before anything is
    touched and the message names the pane. `--force` overrides the work-record refusal (merged or outbox),
    never the pane, and it **costs the branch**: its commits survive only at `refs/rescue/<slug>`, which
    `result["cost"]` states plainly along with the exact restore command. The returned `complete` says whether
    the teardown finished, and the registry entry and the ack file survive an incomplete one so the next
    attempt has the claim to work from.

    Every pane the claim owns is closed, not just the worker's own: a subagent's pane has no other owner and
    outlives its parent (`panes_for_claim`). `probe`, `close` and `lister` exist for the selftests; all three
    default to the real herdr layer.
    """
    unit = norm_unit(unit.strip("/"))
    registry = load_registry(main)
    key = registry_key(registry, unit)
    record = registry.get(key, {}) if key else {}
    branch = record.get("branch") or branch_for(unit)
    path = record.get("worktree") or worktree_for(unit, main)
    ack_file = ack_path(main, unit)
    outbox = os.path.exists(outbox_path(main, unit))
    branch_present = branch_exists(main, branch)
    merged = merged_into_main(main, branch) if branch_present else True
    result = {"unit": unit, "branch": branch, "worktree": path, "merged": merged, "outbox": outbox,
              "registry": key is not None, "pane": None, "release_ref": None, "steps": [],
              "complete": False, "dry_run": dry_run, "refused": None, "child_panes": [], "cost": None}

    forced = force and not (merged or outbox)
    if not force and not (merged or outbox):
        result["refused"] = ("%s's branch %s is neither merged into main nor has an outbox entry at\n  %s\n"
                             "  releasing it would drop work with no record. Finish the handoff, or pass "
                             "--force (the branch is deleted; its commits are rescued to %s)."
                             % (unit, branch, outbox_path(main, unit), rescue_ref_name(unit)))
        return result

    # the pane is asked before anything is touched: a live one is the one real refusal, and closing it has to
    # happen before the worktree goes (plan 5.1), so an idle pane is a step rather than a precondition.
    info = (probe or pane_probe)({"worktree": path, "pane": record.get("pane")})
    pane = info.get("pane")
    result["pane"] = pane
    if info.get("known") and info.get("alive") and info.get("active") is not False:
        result["refused"] = ("pane %s is %s and holds %s as its cwd; release again once it is closed\n"
                             "  (herdr pane close %s)"
                             % (pane, "active" if info.get("active") else "unreadable", path, pane))
        return result

    # a subagent runs in its own pane and nothing owns that pane when its parent finishes: gather every pane
    # whose label starts with the claim's slug so teardown closes the grandchildren too (2026-09-23).
    slug_hint = claim_slug(main, unit) or slug(unit)
    panes = (lister or herdr_panes)()
    child_panes = [p for p in panes_for_claim({"worktree": path, "pane": record.get("pane")}, panes,
                                              slug_hint) if p != pane]
    result["child_panes"] = child_panes

    steps: list[dict] = []
    if branch_present and not merged:
        rescue = rescue_ref_name(unit)
        ahead = commits_ahead(main, branch)
        if ahead:
            result["release_ref"] = rescue
            steps.append(_done_step("git update-ref %s %s" % (rescue, branch),
                                    lambda b=branch, r=rescue: git(["update-ref", r, b], main) and None))
        else:
            steps.append(_skip_step("rescue ref %s" % rescue, "branch has no commits of its own"))
    else:
        steps.append(_skip_step("rescue ref",
                                "branch already merged into main" if branch_present else "branch already gone"))
    if pane and info.get("alive") is not False:
        steps.append(_done_step("herdr pane close %s" % pane, _close_step(pane, close or herdr_close)))
    elif pane:
        steps.append(_skip_step("herdr pane close %s" % pane, "pane already gone"))
    elif info.get("known"):
        steps.append(_skip_step("herdr pane close", "no pane matches this claim"))
    else:
        steps.append(_skip_step("herdr pane close", "herdr unavailable - the pane was not checked"))
    for child in child_panes:
        steps.append(_done_step("herdr pane close %s (subagent pane)" % child,
                                _close_step(child, close or herdr_close)))
    if os.path.isdir(path):
        steps.append(_done_step("git worktree remove --force %s" % path,
                                lambda p=path: remove_worktree(main, p)))
    else:
        steps.append(_skip_step("git worktree remove --force %s" % path, "worktree already gone"))
    # prune runs *before* the branch: a worktree whose directory is already gone is still registered until git
    # is told, and `branch -D` refuses a branch that a registered worktree has checked out
    steps.append(_done_step("git worktree prune", lambda: git(["worktree", "prune"], main)))
    if branch_present:
        steps.append(_done_step("git branch -D %s" % branch, lambda b=branch: git(["branch", "-D", b], main)))
    else:
        steps.append(_skip_step("git branch -D %s" % branch, "branch already gone"))

    # what `--force` costs: the branch (the lock) is deleted, so the only remaining copy of its commits is the
    # rescue ref. State it and the exact restore command; the next gate names the same command from that ref.
    if forced and branch_present:
        if result["release_ref"]:
            result["cost"] = ("--force deleted the branch %s: its commits survive only at %s. Restore with:\n"
                              "    git branch %s %s" % (branch, result["release_ref"], branch,
                                                        result["release_ref"]))
        else:
            result["cost"] = ("--force deleted the branch %s, which had no commits of its own" % branch)

    if dry_run:
        result["steps"] = steps
        result["complete"] = None
        return result

    result["complete"] = complete = _run_teardown(steps)
    # the record and the heartbeat are only cleared by a *complete* teardown: an incomplete one has to stay
    # visible to `list`/`status` (and re-runnable) or the claim becomes the stale entry this tool cleans up
    if complete:
        if key is not None:
            registry.pop(key, None)
            save_registry(main, registry)
            steps.append({"label": "registry entry %s" % key, "action": None, "status": "done", "why": ""})
        else:
            steps.append(_skip_step("registry entry %s" % unit, "no entry"))
        if os.path.exists(ack_file):
            os.remove(ack_file)
            steps.append({"label": "ack file %s" % os.path.basename(ack_file), "action": None,
                          "status": "done", "why": "removed"})
        else:
            steps.append(_skip_step("ack file %s" % os.path.basename(ack_file), "none"))
    else:
        steps.append(_skip_step("registry entry %s" % unit, "kept: the teardown did not complete"))
        steps.append(_skip_step("ack file %s" % os.path.basename(ack_file), "kept: the teardown did not complete"))
    result["steps"] = steps
    return result


def worktree_dirty(path: str) -> bool | None:
    """True when the worktree has uncommitted changes; None when it cannot be asked (gone or not a repo).

    The sweep uses this so "finished" cannot mean "the branch is merged": a branch with no commits of its own
    is trivially merged, and a worker that has claimed a unit and is still editing has exactly that shape. An
    uncommitted change is work that no branch and no outbox records, so it is a skip, not a casualty.
    """
    if not os.path.isdir(path):
        return None
    p = subprocess.run(["git", "-C", path, "status", "--porcelain"], capture_output=True, text=True,
                       errors="replace")
    return bool(p.stdout.strip()) if p.returncode == 0 else None


def release_merged(main: str, dry_run: bool = False, probe=None, close=None, lister=None) -> dict:
    """Sweep: release every finished claim, one worker at a time (the owner's rule, plan "Teardown is part of
    landing").

    A claim is **finished** when main already has its commits (merged or cherry-picked) and it left no
    uncommitted work behind - or it carries an outbox entry, the worker's own statement that the round is
    delivered. Everything else is skipped with the reason: a branch that is not merged is not finished, an
    unregistered worktree has no registry entry to key by, a dirty worktree without an outbox is work nothing
    records, and a **live pane** is a refusal - reported and named, never forced. -> {"released", "skipped",
    "refused", "complete", "dry_run"}.
    """
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
            out["skipped"].append({"unit": unit, "branch": branch,
                                   "why": "worktree has uncommitted changes and no outbox"})
            continue
        result = release(unit, main, force=False, dry_run=dry_run, probe=probe, close=close, lister=lister)
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


def expire(main: str, minutes: int, apply: bool) -> list[dict]:
    now = time.time()
    stale = []
    for row in claims_view(main):
        if not row.get("claimed_at") or row["outbox"]:
            continue
        try:
            age = (now - time.mktime(time.strptime(row["claimed_at"], "%Y-%m-%dT%H:%M:%S"))) / 60.0
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
                continue          # nothing to key a release by: the worktree has to be dealt with by hand
            out = release(row["unit"], main, force=True, dry_run=False)
            row["released"] = bool(out.get("complete"))
            if not row["released"]:
                failed = next((s for s in out["steps"] if s["status"] == "failed"), None)
                row["error"] = (out.get("refused") or ("%s: %s" % (failed["label"], failed["why"])
                                                        if failed else "incomplete"))
    return stale


def selftest() -> int:
    fails = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("slug: simple", slug("Pl/pl_act").startswith("pl-act-"), True)
    check("slug: separators collapse", slug("Pl/pl_act")[:7], "pl-act-")
    check("slug: basename only", slug("Pl/pl_act").startswith("pl-act"), True)
    check("slug: stable", slug("Pl/pl_act"), slug("Pl/pl_act"))
    check("slug: unique per unit", slug("Pl/pl_act") == slug("Pl/pl_skill"), False)
    check("slug: sanitised", bool(re.fullmatch(r"[a-z0-9-]+", slug("auto/80040598_fn_80040598"))), True)
    check("slug: length cap", len(slug("auto/" + "x" * 200)) <= SLUG_MAX, True)
    check("branch prefix", branch_for("main.cpp").startswith("worker/"), True)
    check("worktree is a sibling of main", os.path.basename(worktree_for("Pl/pl_act", "/tmp/mhtri-dtk"))
          .startswith("mhtri-dtk.ws-"), True)
    check("outbox path is in MAIN",
          os.path.dirname(os.path.dirname(outbox_path("/tmp/mhtri-dtk", "Pl/pl_act"))),
          os.path.join("/tmp/mhtri-dtk", ".pi"))

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(main)
        check("registry starts empty", load_registry(main), {})
        save_registry(main, {"Pl/pl_act": {"branch": "worker/pl-act-1234", "claimed_at": "2026-01-01T00:00:00"}})
        check("registry round-trips", load_registry(main)["Pl/pl_act"]["branch"], "worker/pl-act-1234")
        # the registry key may or may not carry the source extension, and the caller may spell it either way:
        # both must reach the same record, branch, outbox and ack (the 2026-09-23 `land.py --units` failure)
        save_registry(main, {"Camellia/camellia": {"branch": "worker/camellia-67ed"},
                             "Gecko/Gecko_ExceptionPPC.cp": {"branch": "worker/gecko-exceptionppc-313d"}})
        check("a registry record is found under the raw key",
              registry_record(main, "Gecko/Gecko_ExceptionPPC.cp")["branch"], "worker/gecko-exceptionppc-313d")
        check("and under the extensionless spelling",
              registry_record(main, "Gecko/Gecko_ExceptionPPC")["branch"], "worker/gecko-exceptionppc-313d")
        check("an unknown unit has no record", registry_record(main, "Nope/nothing"), {})
        check("the claim branch prefers the stored value",
              claim_branch(main, "Gecko/Gecko_ExceptionPPC"), "worker/gecko-exceptionppc-313d")
        check("the claim branch falls back to a computed branch for an unclaimed unit",
              claim_branch(main, "RSO/runtime"), branch_for("RSO/runtime"))
        check("claim_slug ignores the spelling",
              claim_slug(main, "Camellia/camellia.c"), claim_slug(main, "Camellia/camellia"))
        check("the outbox path ignores the spelling",
              outbox_path(main, "Camellia/camellia.c"), outbox_path(main, "Camellia/camellia"))
        check("the outbox is the stored branch's slug",
              os.path.basename(outbox_path(main, "Camellia/camellia")), "camellia-67ed.json")

    # ack/status/timeout against a throwaway registry
    import tempfile as _tf
    with _tf.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi"), exist_ok=True)
        save_registry(tmp, {"Pl/x": {"branch": "worker/x", "claimed_at": "2026-01-01T00:00:00"}})
        check("an unacked claim has no ack file", load_ack(tmp, "Pl/x"), {})
        out = ack("Pl/x", tmp, agent="w-a", pane="w1:p2", progress="fn_1")
        check("ack records the agent", out.get("agent"), "w-a")
        check("ack counts iterations", out.get("iterations"), 1)
        out = ack("Pl/x", tmp, progress="fn_2")
        check("a second ack appends progress", out.get("iterations"), 2)
        check("acked_at is not overwritten", out.get("acked_at"), load_ack(tmp, "Pl/x")["acked_at"])
        check("_age_seconds parses", _age_seconds(out["last_progress_at"]) is not None, True)
        check("_age_seconds tolerates junk", _age_seconds("nonsense"), None)

    # the pane layer: a stale ack is not enough to reclaim a worker that is still moving. herdr is matched
    # to the claim by the worktree name, then the pane's own output is watched (the 2026-09-23 reclaims fired
    # on live workers because the ack is the only signal the old code had)
    with _tf.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "ack"), exist_ok=True)

        def stale_ack(main, unit, minutes, pane="w1:pK"):
            path = ack_path(main, unit)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            stamp = time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime(time.time() - minutes * 60))
            with open(path, "w", encoding="utf-8") as fh:
                json.dump({"unit": unit, "pane": pane, "acked_at": stamp, "last_progress_at": stamp,
                           "iterations": 1}, fh)

        def fixture_row(unit, worktree, pane="w1:pK"):
            return {"unit": unit, "branch": None, "worktree": worktree, "worker": "w",
                    "claimed_at": "2026-01-01T00:00:00", "base": None, "merged": False,
                    "outbox": False, "exists": os.path.isdir(worktree), "pane": pane}

        panes = {
            "w1:pK": {"pane_id": "w1:pK", "agent_status": "unknown", "revision": 13,
                      "terminal_title_stripped": "pi - mhtri-dtk.ws-pl-master-6337",
                      "tokens": {"task": "You are a campaign worker in your own worktree ..."}},
            "w1:p1": {"pane_id": "w1:p1", "agent_status": "working", "revision": 41,
                      "terminal_title_stripped": "pi - mhtri-dtk"},
        }
        check("a claim's pane is found by its worktree name",
              resolve_pane({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337", "pane": "pl-master-6337"},
                           panes), "w1:pK")
        check("an explicit live pane id wins",
              resolve_pane({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337", "pane": "w1:p1"}, panes), "w1:p1")
        check("an unknown worktree resolves to no pane",
              resolve_pane({"worktree": r"C:\x\mhtri-dtk.ws-nothing"}, panes), None)

        # a subagent's pane has no owner once its parent finishes: the label prefix (the worker names each
        # subagent with its claim's slug) is the join teardown uses to close the grandchildren too
        labelled = {
            "w1:pK": {**panes["w1:pK"], "label": "pl-master-6337"},
            "w1:pA": {"pane_id": "w1:pA", "label": "pl-master-6337-ef-big-1", "tokens": {}},
            "w1:pB": {"pane_id": "w1:pB", "label": "pl-master-6337-ef-big-2", "tokens": {}},
            "w1:pC": {"pane_id": "w1:pC", "label": "pl-master-skill-hunt", "tokens": {}},
        }
        check("a claim's own pane is matched by its worktree",
              panes_for_claim({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337"}, labelled, "pl-master-6337"),
              ["w1:pK", "w1:pA", "w1:pB"])
        check("a sibling unit's subagent pane is not closed",
              "w1:pC" in panes_for_claim({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337"}, labelled,
                                         "pl-master-6337"), False)
        check("no herdr means no child panes",
              panes_for_claim({"worktree": r"C:\x\ws-pl-master-6337"}, None, "pl-master-6337"), [])

        moving = iter(["one", "two"])
        p = pane_probe({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337"}, interval=0,
                       lister=lambda: panes, reader=lambda _pane: next(moving), sleeper=lambda _s: None)
        check("a moving pane is active", p["active"], True)
        p = pane_probe({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337"}, interval=0,
                       lister=lambda: panes, reader=lambda _pane: "same", sleeper=lambda _s: None)
        check("a static pane is not active", p["active"], False)
        # a fresh worktree is seeded with the toolchain, so it never reaches into MAIN's build/ for one
        _seed_main = os.path.join(tmp, "seed-main")
        os.makedirs(os.path.join(_seed_main, "build", "tools"), exist_ok=True)
        open(os.path.join(_seed_main, "build", "tools", "dtk.exe"), "w").close()
        _seed_wt = os.path.join(tmp, "ws-seeded")
        seed_worktree_build(_seed_main, _seed_wt)
        check("a claim seeds the worktree's toolchain",
              os.path.exists(os.path.join(_seed_wt, "build", "tools", "dtk.exe")), True)
        check("the claim key ignores the source extension",
              norm_unit("auto/802B2978_fn_802B2978.c") == norm_unit("auto/802B2978_fn_802B2978"), True)
        check("the claim key ignores a .cpp too", norm_unit("Pl/pl_act.cpp") == norm_unit("Pl/pl_act"), True)
        check("seeding reports what it did", seed_worktree_build(_seed_main, _seed_wt).startswith("seeded"), True)
        check("seeding skips cleanly when MAIN has no toolchain",
              seed_worktree_build(os.path.join(tmp, "nowhere"), os.path.join(tmp, "ws-x")).startswith("skipped"), True)
        p = pane_probe({"worktree": r"C:\x\mhtri-dtk.ws-pl-master-6337"},
                       lister=lambda: {"w1:pK": {**panes["w1:pK"], "agent_status": "working"}},
                       reader=lambda _pane: (_ for _ in ()).throw(AssertionError("must not read")),
                       sleeper=lambda _s: None)
        check("agent_status working is active without reading", p["active"], True)
        check("a missing pane is not alive",
              pane_probe({"worktree": r"C:\x\mhtri-dtk.ws-nothing", "pane": "w1:zz"},
                         lister=lambda: panes)["alive"], False)
        check("no herdr is unknown, not 'no panes'",
              pane_probe({"worktree": r"C:\x\ws"}, lister=lambda: None)["known"], False)

        def _no_herdr(*_a, **_k):
            raise FileNotFoundError("herdr")
        check("herdr_panes tolerates no herdr", herdr_panes(runner=_no_herdr), None)

        os.environ["HERDR_PANE_ID"] = "w1:pX"
        auto = ack("Pl/auto", tmp)
        check("ack takes the pane from HERDR_PANE_ID", auto.get("pane"), "w1:pX")
        os.environ.pop("HERDR_PANE_ID", None)

        active = lambda _row: {"pane": "w1:pK", "known": True, "alive": True, "active": True,
                               "status": "unknown", "revision": 13}
        gone = lambda _row: {"pane": "w1:pK", "known": True, "alive": False, "active": False,
                             "status": None, "revision": None}
        idle = lambda _row: {"pane": "w1:pK", "known": True, "alive": True, "active": False,
                             "status": "unknown", "revision": 13}

        stale_ack(tmp, "Pl/live", 60)
        row = fixture_row("Pl/live", os.path.join(tmp, "ws-live"))
        os.makedirs(row["worktree"], exist_ok=True)
        check("a live pane keeps a stale ack working", claim_status(tmp, view=[row], probe=active)[0]["state"],
              "working")
        check("a gone pane leaves the stale ack stalled",
              claim_status(tmp, view=[row], probe=gone)[0]["state"], "stalled")
        check("a static pane leaves the stale ack stalled",
              claim_status(tmp, view=[row], probe=idle)[0]["state"], "stalled")
        check("timeout does not victimize a live pane",
              timeout(tmp, view=[row], probe=active, apply=False, lister=lambda: None), [])
        check("even --unit does not victimize a live pane",
              timeout(tmp, view=[row], probe=active, apply=False, unit="Pl/live", lister=lambda: None), [])
        save_registry(tmp, {"Pl/live": {"branch": None, "worktree": row["worktree"]}})
        check("timeout --apply leaves a live pane's claim alone",
              timeout(tmp, view=[row], probe=active, apply=True, lister=lambda: None), [])
        check("... and its registry entry", "Pl/live" in load_registry(tmp), True)
        check("... and its worktree", os.path.isdir(row["worktree"]), True)

        victims = timeout(tmp, view=[row], probe=idle, apply=False, lister=lambda: None)
        steps = victims[0]["steps"]
        check("the pane is closed before the worktree goes",
              steps.index("herdr pane close w1:pK")
              < steps.index("git worktree remove --force " + row["worktree"]), True)

        # a live pane that will not close must abort the whole teardown: the claim and the worktree stay
        save_registry(tmp, {"Pl/live": {"branch": None, "worktree": row["worktree"]}})
        victims = timeout(tmp, view=[row], probe=idle, apply=True, lister=lambda: None,
                          close=lambda _pane: (False, "pane still live"))
        check("a pane that will not close leaves the claim", "Pl/live" in load_registry(tmp), True)
        check("a pane that will not close leaves the worktree", os.path.isdir(row["worktree"]), True)
        check("and the abort is reported", bool(victims[0].get("error")), True)

        # a stalled worker's subagent panes are closed too: they have no other owner
        save_registry(tmp, {"Pl/live": {"branch": None, "worktree": row["worktree"]}})
        child_pane = {"pane_id": "w1:pA", "label": slug("Pl/live") + "-child", "tokens": {}}
        victims = timeout(tmp, view=[row], probe=idle, apply=True, close=lambda p: (True, ""),
                          lister=lambda: {"w1:pK": {"pane_id": "w1:pK", "label": ""}, "w1:pA": child_pane})
        check("timeout closes a stalled worker's subagent pane",
              any("w1:pA" in s for s in victims[0]["steps"]), True)

    # the handoff slug is the claim's branch minus worker/, so outbox/notes follow the branch - the name
    # brief.py writes and land.py's gate reads - while the ack stays keyed by the unit
    with _tf.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi"), exist_ok=True)
        save_registry(tmp, {
            "Pl/pl_act": {"branch": branch_for("Pl/pl_act")},
            "Pl/pl_skill": {"branch": branch_for("Pl/pl_skill") + "-dd6e"},
        })
        check("slug_of_branch strips the prefix", slug_of_branch("worker/pl-act-1234"), "pl-act-1234")
        check("slug_of_branch tolerates None", slug_of_branch(None), None)
        check("claim_slug reads the branch", claim_slug(tmp, "Pl/pl_act"), slug("Pl/pl_act"))
        check("claim_slug keeps a round's suffix", claim_slug(tmp, "Pl/pl_skill"), slug("Pl/pl_skill") + "-dd6e")
        check("claim_slug of an unclaimed unit", claim_slug(tmp, "RSO/runtime"), None)
        check("the outbox follows the branch, not the unit path",
              os.path.basename(outbox_path(tmp, "Pl/pl_skill")), slug("Pl/pl_skill") + "-dd6e.json")
        check("the notes follow the same slug",
              os.path.basename(notes_path(tmp, "Pl/pl_skill")), slug("Pl/pl_skill") + "-dd6e.md")
        check("an unclaimed unit falls back to the registry slug",
              os.path.basename(outbox_path(tmp, "RSO/runtime")), slug("RSO/runtime") + ".json")
        check("the ack stays keyed by the unit", os.path.basename(ack_path(tmp, "Pl/pl_skill")),
              slug("Pl/pl_skill") + ".json")

    # release: idempotent and total (docs/plan.md, "Teardown is part of landing"). Real temp repos, because
    # the check is git reachability plus the worktree registration; the panes are always injected, so no real
    # worker's claim is ever touched by a selftest.
    unit = "Pl/pl_act"

    def no_pane(_row):
        return {"pane": None, "known": True, "alive": False, "active": False, "status": None, "revision": None}

    def idle_pane(_row):
        return {"pane": "w1:pK", "known": True, "alive": True, "active": False, "status": None, "revision": 13}

    def live_pane(_row):
        return {"pane": "w1:pK", "known": True, "alive": True, "active": True, "status": "working",
                "revision": 41}

    def step_of(result, needle):
        return next((s for s in result["steps"] if needle in s["label"]), {})

    def repo_git(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True)
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def repo_commit(path, msg):
        with open(os.path.join(path, "f.txt"), "a", encoding="utf-8") as fh:
            fh.write(msg + "\n")
        repo_git(path, "add", "-A")
        repo_git(path, "commit", "-q", "-m", msg)

    def new_repo(tmp):
        """A throwaway repo in a subdirectory, so the sibling worktrees it creates are cleaned up with it."""
        path = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(path)
        repo_git(path, "init", "-q")
        repo_git(path, "checkout", "-q", "-b", "main")
        repo_commit(path, "claim-time main")
        return path

    def claimed(main, unit_name, worktree=True):
        """A real claim in a real repo: branch (+ worktree with one commit), outbox, heartbeat, registry."""
        branch = branch_for(unit_name)
        wt = worktree_for(unit_name, main)
        if worktree:
            repo_git(main, "worktree", "add", "-b", branch, wt)
            repo_commit(wt, "the worker's work: %s" % unit_name)
        else:
            repo_git(main, "branch", branch)
        entry = os.path.join(main, ".pi", "outbox", slug(unit_name) + ".json")
        os.makedirs(os.path.dirname(entry), exist_ok=True)
        json.dump({"unit": unit_name}, open(entry, "w"))
        heartbeat = ack_path(main, unit_name)
        os.makedirs(os.path.dirname(heartbeat), exist_ok=True)
        json.dump({"unit": unit_name}, open(heartbeat, "w"))
        registry = load_registry(main)
        registry[unit_name] = {"branch": branch, "worktree": wt}
        save_registry(main, registry)
        return branch, wt

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        tip = repo_git(main, "rev-parse", branch)
        first = release(unit, main, force=False, dry_run=False, probe=no_pane)
        check("release: complete", first["complete"], True)
        check("release: the worktree is gone", os.path.isdir(wt), False)
        check("release: the branch is gone", branch_exists(main, branch), False)
        check("release: the unmerged work is rescued", repo_git(main, "rev-parse", first["release_ref"]), tip)
        check("release: the registry entry is gone", load_registry(main), {})
        check("release: the heartbeat is gone", os.path.exists(ack_path(main, unit)), False)
        second = release(unit, main, force=False, dry_run=False, probe=no_pane)
        check("release twice: still complete", second["complete"], True)
        check("release twice: the worktree step is skipped", step_of(second, "worktree remove")["status"], "skipped")
        check("release twice: the branch step is skipped", step_of(second, "branch -D")["status"], "skipped")
        check("release twice: every step is accounted for",
              all(s["status"] in ("done", "skipped") for s in second["steps"]), True)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        shutil.rmtree(wt)                      # removed by hand: git's registration is still there
        out = release(unit, main, force=False, dry_run=False, probe=no_pane)
        check("a worktree already gone is a clean release", out["complete"], True)
        check("... and the skip says so", "already gone" in step_of(out, "worktree remove")["why"], True)
        check("... and the branch still goes", branch_exists(main, branch), False)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit, worktree=False)
        repo_git(main, "branch", "-D", branch)   # the branch is already gone
        os.makedirs(wt)                            # ... but a leftover directory remains (the 2026-09-23 state)
        open(os.path.join(wt, "junk.txt"), "w").close()
        out = release(unit, main, force=False, dry_run=False, probe=no_pane)
        check("a branch already deleted is a clean release", out["complete"], True)
        check("... and the skip says so", "already gone" in step_of(out, "branch -D")["why"], True)
        check("... a leftover directory git no longer tracks is removed", os.path.isdir(wt), False)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=False, probe=live_pane)
        check("a live pane refuses the release", bool(out["refused"]), True)
        check("... and the refusal names the pane", "w1:pK" in (out["refused"] or ""), True)
        check("... the registry entry is kept", unit in load_registry(main), True)
        check("... the worktree is kept", os.path.isdir(wt), True)
        check("... the branch is kept", branch_exists(main, branch), True)
        check("... and the teardown is not complete", out["complete"], False)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        closed = []
        out = release(unit, main, force=False, dry_run=False, probe=idle_pane,
                      close=lambda p: (closed.append(p) is None, ""))
        check("an idle pane is closed", closed, ["w1:pK"])
        labels = [s["label"] for s in out["steps"]]
        check("the pane is closed before the worktree goes",
              labels.index("herdr pane close w1:pK") < labels.index("git worktree remove --force " + wt), True)
        check("and then the release completes", out["complete"], True)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=False, probe=idle_pane,
                      close=lambda p: (False, "pane %s will not close" % p))
        check("a pane that will not close stops the teardown", out["complete"], False)
        check("... the pane failure is recorded", step_of(out, "herdr pane close")["status"], "failed")
        check("... the worktree is left alone", os.path.isdir(wt), True)
        check("... the branch is left alone", branch_exists(main, branch), True)
        check("... the registry entry is kept", unit in load_registry(main), True)
        check("... and no later step is attempted", step_of(out, "worktree remove")["status"], "skipped")

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=True, probe=no_pane)
        check("a dry run is a plan, not a verdict", out["complete"], None)
        check("a dry run touches nothing",
              (os.path.isdir(wt), branch_exists(main, branch), unit in load_registry(main)), (True, True, True))
        os.remove(outbox_path(main, unit))
        out = release(unit, main, force=False, dry_run=False, probe=no_pane)
        check("an unmerged claim with no outbox is refused", bool(out["refused"]), True)
        check("... and the refusal names the rescue ref", rescue_ref_name(unit) in (out["refused"] or ""), True)
        tip = repo_git(main, "rev-parse", branch)
        forced = release(unit, main, force=True, dry_run=False, probe=no_pane)
        check("... and force releases it anyway", forced["complete"], True)
        # --force deletes the branch - the lock - so the rescue ref is the only copy left, and the cost names
        # the exact command that restores it (the 2026-09-23 "no branch" refusal the next gate hit)
        check("... force parks the work at the rescue ref",
              repo_git(main, "rev-parse", forced["release_ref"]), tip)
        check("... force deletes the branch", branch_exists(main, branch), False)
        check("... the cost names the restore command",
              "git branch %s refs/rescue/%s" % (branch, slug(unit)) in (forced["cost"] or ""), True)
        check("... and rescue_exists finds it", rescue_exists(main, unit),
              "refs/rescue/%s" % slug(unit))

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        # a subagent pane (label starts with the claim's slug) and a sibling's pane (it must not be touched)
        child = {"pane_id": "w1:pA", "label": slug(unit) + "-ef-big-1", "tokens": {}}
        sibling = {"pane_id": "w1:pB", "label": "pl-skill-9c1a-hunt", "tokens": {}}
        closed = []
        out = release(unit, main, force=False, dry_run=False, probe=no_pane,
                      close=lambda p: (closed.append(p) is None, ""), lister=lambda: {"w1:pA": child, "w1:pB": sibling})
        check("teardown closes a subagent pane", closed, ["w1:pA"])
        check("... records it in the plan",
              any("w1:pA" in s["label"] for s in out["steps"]), True)
        check("... and does not touch a sibling's pane", "w1:pB" in closed, False)
        check("... and the release completes", out["complete"], True)

    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        branch, wt = claimed(main, unit)
        out = release(unit, main, force=False, dry_run=False, probe=no_pane,
                      close=lambda p: (False, "pane %s will not close" % p),
                      lister=lambda: {"w1:pA": {"pane_id": "w1:pA", "label": slug(unit) + "-a", "tokens": {}}})
        check("a subagent pane that will not close stops the teardown", out["complete"], False)
        check("... the worktree is left alone", os.path.isdir(wt), True)
        check("... and the failure names the subagent pane",
              any(s["status"] == "failed" and "w1:pA" in s["label"] for s in out["steps"]), True)

    # the sweep: only merged claims, and a live pane is reported rather than forced
    with tempfile.TemporaryDirectory() as tmp:
        main = new_repo(tmp)
        done_branch, done_wt = claimed(main, "Pl/pl_act")
        repo_git(main, "cherry-pick", done_branch)
        live_branch, live_wt = claimed(main, "Pl/pl_skill")
        repo_git(main, "cherry-pick", live_branch)
        open_branch, open_wt = claimed(main, "Pl/pl_master")
        # a merged claim whose worktree still holds uncommitted work: nothing records it, so the sweep skips it
        dirty_branch, dirty_wt = claimed(main, "Pl/pl_misc")
        repo_git(main, "cherry-pick", dirty_branch)
        os.remove(outbox_path(main, "Pl/pl_misc"))
        open(os.path.join(dirty_wt, "uncommitted.c"), "w").close()
        swept = release_merged(
            main, probe=lambda row: live_pane(row) if "ws-pl-skill" in row.get("worktree", "") else no_pane(row))
        check("the sweep releases the merged claim", [r["unit"] for r in swept["released"]], ["Pl/pl_act"])
        check("... refuses the live one", [r["unit"] for r in swept["refused"]], ["Pl/pl_skill"])
        check("... skips the unmerged one and the dirty one", [r["unit"] for r in swept["skipped"]],
              ["Pl/pl_master", "Pl/pl_misc"])
        check("... naming the uncommitted work",
              [r["why"] for r in swept["skipped"] if r["unit"] == "Pl/pl_misc"],
              ["worktree has uncommitted changes and no outbox"])
        check("... and reports the sweep incomplete", swept["complete"], False)
        check("... the merged branch is gone", branch_exists(main, done_branch), False)
        check("... the live pane's branch is kept", branch_exists(main, live_branch), True)
        check("... the unmerged branch is kept", branch_exists(main, open_branch), True)
        check("... the unmerged worktree is kept", os.path.isdir(open_wt), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    c = sub.add_parser("claim", help="reserve a unit (worktree + branch)")
    c.add_argument("unit")
    c.add_argument("--worker", default=None)
    c.add_argument("--dry-run", action="store_true")
    c.add_argument("--json", action="store_true")
    l = sub.add_parser("list", help="every claim git knows about")
    l.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="remove the worktree and the branch (idempotent, total)")
    r.add_argument("unit", nargs="?", default=None, help="the unit to release; omit with --all-merged")
    r.add_argument("--all-merged", action="store_true", dest="all_merged",
                   help="sweep every claim whose branch is already merged into main")
    r.add_argument("--force", action="store_true",
                   help="release even with no outbox and an unmerged branch (the work is rescued first)")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--json", action="store_true")
    a = sub.add_parser("ack", help="a worker's heartbeat: call this first, then after each iteration")
    a.add_argument("unit")
    a.add_argument("--agent", default=None, help="your name; defaults to the herdr pane")
    a.add_argument("--pane", default=None, help="defaults to $HERDR_PANE_ID, so inside a worker this can be omitted")
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

    main_wt = rc.main_root(rc.worktree_root())
    if args.cmd == "claim":
        out = claim(args.unit, main_wt, args.worker, args.dry_run)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if out.get("dry_run"):
            print("would run: %s" % out["command"])
        else:
            print("claimed %s\n  branch   %s\n  worktree %s\n  base     %s"
                  % (out["unit"], out["branch"], out["worktree"], out["base"]))
            # the brief is named after the branch the claim just made, not re-derived from the unit path
            print("\nnext: python tools/units/brief.py %s          # write the brief\n"
                  "      read tools/units/briefs/%s.md in %s, do it, write your report"
                  % (out["unit"], slug_of_branch(out["branch"]) or slug(out["unit"]), out["worktree"]))
        return 0
    if args.cmd == "list":
        rows = claims_view(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no active claims")
            return 0
        print("%-34s %-30s %-10s %-7s %-7s %s" % ("unit", "branch", "worker", "merged", "outbox", "claimed"))
        for row in rows:
            print("%-34s %-30s %-10s %-7s %-7s %s"
                  % (row["unit"][:34], (row["branch"] or "")[:30], (row.get("worker") or "")[:10],
                     row["merged"], row["outbox"], row.get("claimed_at") or ""))
        return 0
    if args.cmd == "release":
        if args.all_merged:
            out = release_merged(main_wt, args.dry_run)
            if args.json:
                print(json.dumps({**out, "released": ["%s (%s)" % (r["unit"], r["branch"]) for r in out["released"]]},
                                 indent=2))
            else:
                for result in out["released"]:
                    done = sum(1 for s in result["steps"] if s["status"] == "done")
                    skipped = sum(1 for s in result["steps"] if s["status"] == "skipped")
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
        if not args.unit:
            print("release needs a <unit>, or --all-merged to sweep")
            return 2
        out = release(args.unit, main_wt, args.force, args.dry_run)
        if args.json:
            print(json.dumps({k: v for k, v in out.items() if k != "steps"} | {
                "steps": [{k: v for k, v in s.items() if k != "action"} for s in out["steps"]]}, indent=2))
            return 0 if (out["complete"] or out["dry_run"]) and not out.get("refused") else 1
        if out.get("refused"):
            print("REFUSED: %s" % out["refused"])
            return 1
        for step in out["steps"]:
            mark = {"done": "done  ", "skipped": "skip  ", "planned": "would ", "failed": "FAILED"}\
                .get(step["status"], step["status"])
            print("  %s %s%s" % (mark, step["label"], (" - " + step["why"]) if step.get("why") else ""))
        if out.get("cost"):
            print("COST: %s" % out["cost"])
        if out["dry_run"]:
            print("would release %s (branch %s, merged=%s, outbox=%s)"
                  % (out["unit"], out["branch"], out["merged"], out["outbox"]))
            return 0
        print("released %s (branch %s, merged=%s, outbox=%s)%s"
              % (out["unit"], out["branch"], out["merged"], out["outbox"],
                 "" if out["complete"] else " - the teardown is INCOMPLETE, the claim is kept"))
        return 0 if out["complete"] else 1
    if args.cmd == "ack":
        out = ack(args.unit, main_wt, args.agent, args.pane, args.progress)
        print(json.dumps(out, indent=2) if args.json
              else "ack %s: %d iteration(s), agent=%s pane=%s" % (out["unit"], out.get("iterations", 0),
                                                                  out.get("agent"), out.get("pane")))
        return 0
    if args.cmd == "status":
        rows = claim_status(main_wt, args.ack_seconds, args.stall_minutes)
        if args.json:
            print(json.dumps(rows, indent=2))
        else:
            print("%-16s %-9s %8s %8s %8s %-8s %-12s %s"
                  % ("unit", "state", "acked", "progress", "commits", "outbox", "pane", "agent"))
            for row in rows:
                pane = row.get("pane") or "?"
                if row.get("pane_alive") is False:
                    pane += "(gone)"
                elif row.get("pane_active") is True:
                    pane += "(active)"
                elif row.get("pane_active") is False:
                    pane += "(idle)"
                print("%-16s %-9s %8s %8s %8d %-8s %-12s %s"
                      % (row["unit"][:16], row["state"],
                         row["acked_seconds_ago"] if row["acked_seconds_ago"] is not None else "-",
                         row["progress_seconds_ago"] if row["progress_seconds_ago"] is not None else "-",
                         row["commits"], str(row["outbox"]), pane[:12], row["agent"] or "?"))
                if row.get("pane_note"):
                    print("%-16s   kept by the pane: %s" % ("", row["pane_note"]))
            unhealthy = [r for r in rows if r["state"] in ("unacked", "stalled")]
            if unhealthy:
                print("\n%d unhealthy claim(s) - reclaim with: python tools/units/claims.py timeout%s"
                      % (len(unhealthy), " --apply"))
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
                print("   the claim is untouched; close the pane (or fix the step) and run again")
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
