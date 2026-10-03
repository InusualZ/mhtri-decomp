# `optsweep` - Sweep `-opt` sub-options on a unit and print frame sizes

<!-- generated from the module docstring of `tools/flags/optsweep.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Sweep `-opt` sub-options for a unit and print the resulting frame sizes.

## Users

skills (8); CLAUDE.md (1); docs (6)

## CLI

```
python tools/flags/optsweep.py                                  # all candidates, the only unit
python tools/flags/optsweep.py -u <unit>                        # any unit
python tools/flags/optsweep.py -u <unit> lifetimes nodeadstore  # only these candidates
python tools/flags/optsweep.py -u <unit> --flags-extra "-O3 -inline noauto"
```
Flags: `--flags-extra`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit -> table.

## Invariants and rules

* For every candidate sub-option it recompiles the unit with `-opt <current>,<sub>` (the unit's own `-opt` value is kept as the base) into a scratch object, and prints `.text` size plus each function's prologue frame next to the target object's frame. This is the fast filter for "does this option change stack allocation at all?" - confirm anything interesting with `mwcc_matrix.py`.

## Lib dependencies

units, binary.

## Test contract

Tier: none.
No selftest today.
Target: `tools/tests/flags/test_optsweep.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
