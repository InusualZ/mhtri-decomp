# `brief` - Render the one file a worker is handed (unit, inventory, residuals, decided, task, rules) for a registered unit; `--pool` pre-renders every pool unit

<!-- rewritten by WP3e (2026-10-04): the proposal brief is gone; the code lives in tools/units/briefing/ (spec: briefing.md) -->

## Purpose

Write the one file a worker is handed: `tools/units/briefs/<slug>.md`.

## Users

the landing gate (5); docs (21); imported by `dataqueue`, `dossier`, `handoff`, `land`, `queue`

## CLI

```
python tools/units/brief.py <unit> [--task "..."] [--out F] [--stdout] [--json] [--selftest]
python tools/units/brief.py --pool [--force] [--no-prune] [--prune-promoted] [--json]
python tools/units/brief.py --check-promoted
```
Flags: `--check-promoted`, `--force`, `--json`, `--no-prune`, `--out`, `--pool`, `--prune-promoted`, `--selftest`, `--stdout`, `--task`.
`brief.py` is the CLI and the name every caller imports (`brief.text_has_bodies`, `brief.splits_range`, ...): each
name is re-exported from `tools/units/briefing/` (`sources`, `render`, `pool`).

## Inputs and outputs

Inputs -> outputs: map, splits, report, configure, dossier, typeregistry, `docs/plan.md`/`docs/pipeline.md` ->
`tools/units/briefs/<slug>.md`.

## Invariants and rules

* docs/plan.md 7.3, §5.2. A worker inherits nothing from the orchestrator's context, so the brief is self-contained
  and says the same thing every time. It has exactly six parts:
* the unit - path, lib, mw_version, the real cflags, object and target paths, the `.text` range, the **language**
  (`langcheck.py`: the extension decides the front-end) and the shared headers (`include/**`) that already declare
  what the unit needs (`typeregistry.py`)
* the inventory - every symbol the unit owns, its address, size and current measured %
* the residuals - the unit's file-header comment, so a re-brief never re-derives settled work
* the decided - the flags landed for this lib, and the data ranges deliberately not claimed
* the task - the functions still under the bar, in address order (or an explicit --task)
* the rules - `docs/plan.md` §6.5 and §8 verbatim (found by title in either rule doc), the measurement loop, the
  integrator-request contract (`lib.requests`)
* The brief is written into MAIN (`<main>/tools/units/briefs/`) so it outlives the worktree. Its file name and the
  paths in part 4 come from the claim that holds the unit - its own registry row, or the cluster row whose `units`
  lists it - and the slug is that claim's branch minus `worker/`; a brief for an unclaimed unit says so instead of
  inventing one.
* The heartbeat line is `claims.py ack <unit> --agent <name>` (the herdr `--pane` is gone, `retired.md`).
* `--pool` writes a stamped brief for every **pool unit** (registered, not `Matching`, source present) into
  `briefs/pool/`, rendered against the branch the default claim will make; re-running is idempotent (an unchanged
  stamp is skipped), a brief whose unit left the pool is pruned. `queue.py next` never reads it - it renders at
  claim time. `--check-promoted` reports (and `--prune-promoted` deletes, unless a live claim owns it) a promoted
  brief whose stated `.text` no longer matches the registered range.

## Lib dependencies

lanes (naming, registry), project, units.

## Test contract

Tier: fixture (fake MAIN) plus a few reads of the live `docs/` and `splits.txt` (`plan_section`, `splits_range`).
Today's selftest: in-file `selftest()` (`--selftest`).

## Known gaps

`--pool` over 331 units takes minutes; it is an optional pre-render, not part of the claim path.
