# `claims` - Claim a unit for a worker (branch = lock, worktree, registry), ack/status/timeout, the one-shot idempotent `release` teardown with rescue ref, `expire`; also the worktree build seeder

<!-- generated from the module docstring of `tools/units/claims.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Claim a unit for a worker: one git worktree, one branch, and the branch *is* the lock.

## Users

the landing gate (4); configure.py / the build (1); profiles (`.claude/agents`) (6); CLAUDE.md (3); docs (40); imported by `brief`, `flipcheck`, `handoff`, `land`, `queue`, `recompile`, `slots`, `vtslot`

## CLI

```
python tools/units/claims.py claim <unit> [--worker NAME] [--dry-run] [--json]
python tools/units/claims.py list [--json]
python tools/units/claims.py release <unit> [--force] [--dry-run]
python tools/units/claims.py release --all-merged [--dry-run]
python tools/units/claims.py expire [--minutes N] [--apply]
python tools/units/claims.py --selftest
```
Subcommands: `claim`, `list`, `release`, `ack`, `status`, `timeout`, `expire`.
Flags: `--ack-seconds`, `--agent`, `--all-merged`, `--apply`, `--branch`, `--dry-run`, `--force`, `--json`, `--kind`, `--minutes`, `--no-slots`, `--pane`, `--progress`, `--selftest`, `--slot`, `--stall-minutes`, `--worker`.
Exit codes: The exit status says whether the teardown is complete.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: git, .pi/claims.json, slots -> worktree/branch.

## Invariants and rules

* docs/plan.md 7.2. With up to twelve externally spawned worker agents, "who owns this unit" cannot live in anyone's memory, and a lock file has to be trusted. Git already has an atomic one: creating a branch either succeeds or fails, and two worktrees cannot share a branch name. So a claim is
```
git worktree add -b worker/<slug> <sibling>.ws-<slug> <main's HEAD>
```
* and the unit-to-worker mapping, the batch base sha and the timestamps live in `MAIN/.pi/claims.json` - a convenience registry on top of git, never the source of truth (git is: `list` reconstructs from it when the registry is missing).
* `<unit>` is the path from the repository root (`Pl/pl_act`, `auto/80040598_fn_80040598`).
* `release` is the **one-shot, idempotent** teardown (docs/plan.md, "Teardown is part of landing"): rescue ref -> pane close -> `git worktree remove --force` -> `branch -D` -> `prune` -> registry entry (+ the ack file). Every step reports what it did or why it was skipped, so releasing an already-released claim (or one whose worktree was already removed by hand) is a clean no-op instead of an abort. The one refusal that stays is a **live pane**, because Windows will not delete a directory a process is sitting in (5.1); it is named in the message. The exit status says whether the teardown is complete. `release --all-merged` sweeps every claim whose branch is already merged into main, releasing what it can and naming what it skipped.
* The rescue ref is also **classified at the moment it is created** (`rescue_verdict`, reusing `rescue.py`'s audit - never a second implementation of "is this on main"): the verdict is reported in the teardown's own step list, a `redundant` ref (the unit is on `main` and every touched path matches) is pruned, `landed-with-drift` is reported and kept, and `unlanded`/`unknown` are surfaced loudly with the ref, its unit(s) and its date and kept. It is **never a gate**: a verdict cannot fail a teardown, and nothing is pruned without proof of containment.
* A claim is only ever declared `stalled` from the ack *and* the worker's own pane: `herdr pane list` is matched to the claim by its worktree name, and `herdr pane read` is sampled twice - a pane whose content moves is a worker that is alive, whatever its ack file says. A live pane is therefore never reclaimed on a stale ack, and `timeout --apply` closes the pane *before* it touches the worktree, because the pane pins the worktree as its cwd on Windows (docs/plan.md 5.1).

## Lib dependencies

lanes, git, repo, text.

## Test contract

Tier: fixture (GitFixture).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_claims.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the herdr pane probing is a dead path (retired.md); `seed_worktree_build` duplicates `slots` seeding
