# `claims` - Claim a unit for a worker (branch = lock, slot or worktree, registry), ack/status/timeout, the one-shot idempotent `release` teardown with rescue ref, `expire`

<!-- rewritten by WP3e (2026-10-04): the lane model is lib.lanes; the herdr pane layer is deleted -->

## Purpose

Claim a unit for a worker: one branch (the lock), one slot or worktree, one registry row.

## Users

the landing gate (4); profiles (`.claude/agents`) (6); CLAUDE.md (3); docs (40); imported by `flipcheck`,
`handoff`, `land`, `vtslot` (the names they import are re-exported from `lib.lanes`); `integrate` reads
`claims.py list --json` (the live-claim query - its shape is kept).

## CLI

```
python tools/units/claims.py claim <unit> [--worker W] [--kind K] [--slot N] [--no-slots] [--dry-run] [--json]
python tools/units/claims.py list [--json]
python tools/units/claims.py release <unit> | --branch B | --all-merged [--force] [--dry-run] [--json]
python tools/units/claims.py ack <unit> [--agent A] [--progress P] [--json]
python tools/units/claims.py status [--ack-seconds S] [--stall-minutes M] [--json]
python tools/units/claims.py timeout [<unit>] [--apply]
python tools/units/claims.py expire [--minutes N] [--apply]
python tools/units/claims.py --selftest
```
`release`'s step list (`done`/`skip`/`would`/`FAILED` lines) and its final `complete` are parsed by lanes and the gate
and stay as they were. `list --json` rows: `unit, branch, worktree, worker, claimed_at, base, merged, outbox, acked,
exists` (unchanged for an old row), plus `units` when the claim records a unit set (a cluster claim's registry row,
a spawned lane's slot lock) and `registered: false` / `kind` for a lane `slots.py spawn` took (listed under its lock's
name, e.g. `lane/net2-l2`, instead of `(unregistered)`). The text `list` prints a `holds N unit(s): ...` line under
such a row.

## Inputs and outputs

Inputs -> outputs: git (branches, worktrees), `.pi/claims.json`, `.pi/ack/`, `.pi/outbox/`, the slot pool, the
session registry -> branch + slot/worktree + registry row; teardown steps.

## Invariants and rules

* A claim is `git worktree add -b worker/<slug> <sibling>.ws-<slug> <main's HEAD>` - or, with a slot pool,
  `lib.lanes.pool.acquire` (a fresh branch in a reset, verified slot). Git is the source of truth: `list`
  reconstructs from the worktrees when the registry is missing, and the registry is a convenience on top.
* A claim may hold several units (`units` in its row: a `queue.py next --cluster` claim); every listed unit reads as
  claimed, and its brief names the cluster's outbox (`lib.lanes.registry.record_holding`).
* A spawned lane has no registry row; its slot lock carries its unit set (`slots.py spawn --units`, else the task's
  `Units:` line), `claims_view` reads it through `lib.lanes.pool.lock_for_path`, and `integrate.py`'s live-owner check
  counts every unit it holds. Such a row is `unregistered(row)`: `release --all-merged` and `expire --apply` skip it
  (the slot's own `release` is its teardown, and clears the unit set with the lock). Before 2026-10-04 the network
  pilot's lanes listed as `(unregistered)` with no units, so integrate saw no owner for their files.
* `release` is the **one-shot, idempotent** teardown: rescue ref (+ its verdict) -> slot return, or worktree remove
  -> prune -> branch delete -> registry entry -> ack file. Every step reports what it did or why it was skipped
  (`lib.lanes.teardown`), so releasing an already-released claim is a clean no-op. Refusals: an unmerged claim with
  no outbox (unless `--force`, which costs the branch - its commits live on at the rescue ref, and the cost line names
  the restore command), and a **live session** in a worktree claim's tree (a slot claim is refused by the slot
  return's own blockers). The exit status says whether the teardown is complete.
* The rescue ref is classified the moment it is created (`lib.lanes.rescue.verdict`): `redundant` is pruned and
  said so, `landed-with-drift` reported and kept, `unlanded`/`unknown` surfaced loudly and kept. Never a gate.
* `status`: `done` (outbox), `unacked` (no ack past `--ack-seconds`), `stalled` (no progress for `--stall-minutes`),
  else `working`; a live session in the claim's tree (`lib.lanes.sessions`: a session record whose pid is alive and
  whose cwd is inside the tree) keeps an unacked/stalled claim `working`. `timeout` reclaims the rest through
  `release --force` (commits rescued first); a live session is never reclaimed, named or not.
* `ack` proves the place (the claim's worktree, on its branch) before it writes the heartbeat.

## Lib dependencies

lanes (naming, registry, pool, seed, rescue, sessions, teardown, launch), git.

## Test contract

Tier: fixture (temp repositories). Today's selftest: in-file `selftest()` (`--selftest`; 94 checks: the registry's
spellings, ack and its place check, the session probe on a real registry (live pid, dead pid, another tree),
status/timeout over a stale ack, claim with seeding and kinds and a cluster row, release (idempotent, gone worktree,
leftover directory, live session, `--force` and its cost, the rescue verdicts), the merged sweep, the branch
selector). Target: `tools/tests/units/test_claims.py`.

## Known gaps

The seeder and the slot path are `lib.lanes.seed`/`pool` now; `claims.py claim`'s CLI still imports `queue` to render
the brief and the spawn line (the `claims -> queue` edge).
