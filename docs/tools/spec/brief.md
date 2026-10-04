# `brief` - Render the one file a worker is handed (unit, inventory, residuals, decided, task, rules) for a registered unit or a proposal; `--pool` pre-renders every handable entry

<!-- generated from the module docstring of `tools/units/brief.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Write the one file a worker is handed: `tools/units/briefs/<slug>.md`.

## Users

the landing gate (5); docs (21); imported by `dataqueue`, `dossier`, `handoff`, `land`, `promote`, `queue`, `slots`

## CLI

```
python tools/units/brief.py <unit> [--task "..."] [--stdout] [--json] [--selftest]
python tools/units/brief.py --pool [--force] [--no-prune] [--prune-promoted]
python tools/units/brief.py --check-promoted
```
Flags: `--check-promoted`, `--force`, `--json`, `--no-prune`, `--out`, `--pool`, `--prune-promoted`, `--selftest`, `--stdout`, `--task`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.
* Part 6 renders the integrator-request contract (`brief.integrator_lines`, from `lib.requests.SCHEMA`): the request
  file, the kinds, the STOPGAP marker and the 2026-10-04 trial rule.

## Inputs and outputs

Inputs -> outputs: map, splits, report, outbox, dossier, typeregistry, plan.md -> tools/units/briefs/<slug>.md.

## Invariants and rules

* docs/plan.md 7.3, §5.2. Four workers in separate processes inherit nothing from the orchestrator's context, so the brief has to be self-contained and has to say the same thing every time. It has exactly six parts:
* the unit - path, lib, mw_version, the real cflags, object and target paths, the `.text` range, the **language** (C/C++, from `tools/units/langcheck.py` - the extension decides the front-end, so a worker has to be told which one it is and what that costs), and the shared headers (`include/**`) that already declare what this unit needs - so it reuses them instead of re-creating them (`tools/units/typeregistry.py`)
* the inventory - every symbol the unit owns, its address, size and current measured %
* the residuals - the unit's file-header comment, so a re-brief never re-derives settled work
* the decided - the flags landed for this lib, and the data ranges deliberately not claimed
* the task - the functions still under the bar, in address order (or an explicit --task)
* the rules - `docs/plan.md` §6.5 and §8 verbatim, plus the measurement loop
* The brief is written into MAIN (`<main>/tools/units/briefs/`), not into a worker's worktree, so it outlives the worktree the same way the outbox does. Its file name and the paths in part 4 come from the unit's **active claim**: the slug is the claim's branch minus `worker/` - the name `land.py`'s gate keys the outbox by - never a name re-derived from the unit path, and a brief for an unclaimed unit says so instead of inventing one.
* `--pool` is the one exception, and it is what lets the orchestrator start a worker the instant a slot frees: for every registered unit that has no bodies yet it writes a brief *before* any claim exists, keyed by `claims.slug(unit)` and rendered against the worktree and branch the default claim will create, so `tools/units/queue.py next` only has to claim the unit and hand the worker a brief. No claim is made and the registry is never touched. Each brief carries a **stamp** (an invisible HTML comment) of the entry's range, function count and TU verdict, so re-running `--pool` is idempotent: an unchanged entry is skipped and the file is not rewritten, a changed one is refreshed, and a brief whose unit has gained a body is pruned. `queue.py next` no longer copies a pooled brief at all - it re-renders from the current entry - so a stale pool cannot reach a worker even between `--pool` runs. `--check-promoted` reports (and `--prune-promoted` deletes) a promoted brief in `tools/units/briefs/` whose entry no longer matches the queue.

## Lib dependencies

project, report, outbox, units, lanes, text.

## Test contract

Tier: fixture (fake MAIN).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_brief.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the brief says `Four workers`; policy is six slots

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **A brief is never written for a range that is not handable work (2026-09-25).** The proposal queue is written from the *unclaimed* `.text`, but the file is not regenerated on every landing, so an entry's range can be registered by another worker before its brief is pooled - the `proposal/8008F8E4` entry capped `0x8008F8E4-0x80097D40`, five translation units, four of them already claimed and live. `--pool` leaves such an entry out (and prunes its brief if one exists), single-unit mode refuses it, and both name the unit whose range it now overlaps; `queue.py next` therefore only ever offers ranges that are still unclaimed. The same test covers a queue that overlaps *itself*: two entries sharing bytes would hand one range to two workers, and neither is offered.
