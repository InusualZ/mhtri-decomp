# `slotmap` - r1-relative stack-slot map between target and ours for one symbol, by index-aligned majority vote

<!-- generated from the module docstring of `tools/objdiff/slotmap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

r1-relative stack-slot map for one symbol of an objdiff-cli diff.

## Users

skills (3); CLAUDE.md (1); docs (2)

## CLI

```
python tools/objdiff/slotmap.py -u <unit> <symbol> [--map] [--slot 0xc8] [--around 700,730]
python tools/objdiff/slotmap.py <diff.json> <symbol> [--map] [--slot 0xc8] [--left ours|target]
```
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: diff.json -> table.

## Invariants and rules

* Invocation: the `-u` path runs objdiff in **project mode** (`-p . -u <unit>`), where **left = the target object and right = our build**. A pre-existing `diff.json` is assumed to be file mode (`-1 <ours> -2 <target>`, left = ours), as this docstring always said; `--left` overrides that guess. Because the two instruction streams are identical except for the r1 offsets, the offset mapping is recovered by index-aligned majority vote.
* The rows come from a diff run with `-c functionRelocDiffs=none` (unitutil.objdiff) - `report generate`'s default - so a relocation-only difference is not reported here as an argument mismatch. This tool prints no score on purpose: objdiff's `match_percent` is positional and not the campaign's metric; use `mt.py diff -u <unit> <symbol>` for the official number.

## Lib dependencies

report, units.

## Test contract

Tier: fixture via metric_selftest.
No selftest today.
Target: `tools/tests/objdiff/test_slotmap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
