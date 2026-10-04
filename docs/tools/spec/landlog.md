# `landlog` - Read and summarise the landing log

## Purpose

`MAIN/.pi/land-log.jsonl` holds one JSON line per `land.py land` attempt (`lib.lanes.landlog.Attempt`: branch,
outcome, seconds, the refused row, the conflicted paths, the units, the commit). This CLI reads it: which rows refuse
most, which paths conflict most, how long a landing takes - the evidence the gate's cost discussions have lacked.

## Users

The orchestrator; the WP4 gate writes the log (`landlog.append`, three lines in `land.py land` - `lib-lanes.md`).

## CLI

```
python tools/units/landlog.py [summary] [--last N] [--json] [--main PATH]
python tools/units/landlog.py list [--last N] [--json] [--main PATH]
python tools/units/landlog.py --selftest
```
`summary` (default): attempts, outcomes, the landed ratio, total and median wall time, the refusing rows and
conflicted paths by frequency; `--json` is `lib.lanes.landlog.summary` plus `unreadable_lines`. `list`: one line per
attempt (time, outcome, seconds, branch, the refusing row or conflicts or commit). Exit 0.

## Inputs and outputs

Inputs -> outputs: `.pi/land-log.jsonl` -> text or JSON on stdout (read-only).

## Invariants and rules

* A line that is not a JSON object is reported by number, never fatal.
* The log is append-only and gitignored (`.pi/`); `lib.repo.STATE_NAMES` does not list it yet (the gate writes it
  through `lib.lanes.landlog.log_path`).

## Lib dependencies

lanes (landlog, registry), cli.

## Test contract

Tier: fixture. `tools/tests/units/test_landlog.py` (the CLI); the format is `tools/tests/lib/test_lanes.py`.

## Known gaps

Nothing writes the log until WP4 adds the hook to `land.py`.
