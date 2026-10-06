# `landlog` - Read and summarise the landing log

## Purpose

`MAIN/.pi/land-log.jsonl` holds one JSON line per `land.py land` attempt (`lib.lanes.landlog.Attempt`: branch,
outcome, seconds, the refused row, the conflicted paths, the units, the commit and - schema 2, 2026-10-05 - `allow`,
every allowance the command line granted: `--allow-regression`, `--allow-rule10`, `--allow-rule12`, `--allow-orphan`,
`--no-outbox`, `--no-selftests`, `--unit-rename` - and `warnings`, the WARNING rows' findings, `<row>: <finding>`;
since 2026-10-06 an optional `manifest`, the lane manifest id `land --manifest` named - absent when none, so the schema
stays 2). This CLI reads it: which rows refuse
most, which paths conflict most, how long a landing takes - the evidence the gate's cost discussions have lacked.

## Users

The orchestrator; the gate writes the log (`tools/units/landing/flow.py`: `land` and `land_branch` each append one
line per attempt, in a `finally`, so an exception is logged as `error`; `land --branch` logs once).

## CLI

```
python tools/units/landlog.py [summary] [--last N] [--json] [--main PATH]
python tools/units/landlog.py list [--last N] [--json] [--main PATH]
```
`summary` (default): attempts, outcomes, the landed ratio, total and median wall time, the refusing rows, the
allowances (per class: the attempts that used it and the entries they named; "none recorded" when none, and a note
counting the schema-1 lines that predate the record), the attempts judged against a lane manifest, the warnings by
row and conflicted paths by frequency; `--json` is
`lib.lanes.landlog.summary` plus `unreadable_lines`. `list`: one line per attempt (time, outcome, seconds, branch,
the refusing row or conflicts or commit, then `[allow: <classes>]` when it carried any and `[manifest: <id>]` when
it named one). Exit 0.

## Inputs and outputs

Inputs -> outputs: `.pi/land-log.jsonl` -> text or JSON on stdout (read-only).

## Invariants and rules

* A line that is not a JSON object is reported by number, never fatal; schema 1 and schema 2 lines read side by side.
* The commit body of a landing carries the same record: one `allow: <class> <entries>` line per class
  (`landing.state.allow_lines`), so a log grep for `^allow: rule10` finds the landings that took a rule-10 key.
* The log is append-only and gitignored (`.pi/`); `lib.lanes.landlog.log_path` resolves it through
  `lib.repo.state("land-log.jsonl")` (on `STATE_NAMES` since WP4).

## Lib dependencies

lanes (landlog, registry), cli.

## Test contract

Tier: fixture. `tools/tests/units/test_landlog.py` (the CLI, the allowance counts, the manifest id); the format is
`tools/tests/lib/test_lanes.py`; `tools/tests/units/landing/test_landlog_hook.py` lands a fixture batch with every
allowance class and checks the log line and the commit body.

## Known gaps

The log starts at WP4's landing (no history before it); the allowances start at schema 2 (2026-10-05) - the 31
schema-1 lines before it carry none, and the only allowance recoverable for them is `--allow-regression` (the old
`authorised regressions:` suffix of the commit's `gates:` line). `land.py verify` and `record-base` are not attempts and
write nothing. The `refused_row` of a refusal before the gate names the guard (`--message`, `HEAD is main`,
`branch exists`, `clean tree`, `units from the branch`, `apply`, `pre-flight (paths outside the batch)`, `paths
outside the batch (after the build)`, `already applied`, `nothing to stage`, `git commit`); a gate refusal names its
first failing row - a post-build one too since 2026-10-05: before that `verify` handed `land` only its pre-build
failures, so a post-build refusal was logged as `nothing to stage` (proved for the 2026-10-05 06:57 line; the four of
2026-10-04 ran the same code, `docs/pipeline.md` section 12).
