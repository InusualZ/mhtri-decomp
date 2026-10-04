# `mwcc_matrix` - Compile a unit across MWCC versions / flag overrides and summarise the official report metric per variant

<!-- generated from the module docstring of `tools/flags/mwcc_matrix.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Compile a unit with several MWCC versions and/or flag overrides and summarize the objdiff result.

## Users

skills (11); CLAUDE.md (1); docs (10)

## CLI

```
python tools/flags/mwcc_matrix.py                          # the unit's own compiler, no overrides
python tools/flags/mwcc_matrix.py -u <unit>
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "-O3 -inline noauto"
python tools/flags/mwcc_matrix.py -u <unit> 1.0 1.3 1.5    # one run per compiler version
```
Flags: `--flags-extra`, `--list-versions`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit -> build/tmp/matrix/*.

## Invariants and rules

* The compile command is the exact one ninja would run for that unit (so whatever `configure.py` puts in the unit's `cflags` is honoured); `--flags-extra` overrides same-family flags, and a compiler version argument swaps the MWCC executable. The object is written to the unit's real output path so that objdiff's project mode can diff it against the split target object.
* Writes: build/tmp/matrix/<label>.json (raw objdiff diff per variant) build/tmp/matrix/summary.txt (the printed table) Leaves a foreign object behind - restore with: rm -f <unit obj> && ninja <unit obj>

## Lib dependencies

units, report.

## Test contract

Tier: fixture via metric_selftest.
No selftest today.
Target: `tools/tests/flags/test_mwcc_matrix.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* leaves a foreign object in the unit's output path
* `--list-versions` listed the files of the compiler's own version directory until WP3b; it now lists the sibling version
  directories (`lib.units.available_versions`, shared with `frame.py --versions`).

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The `match` column is the **official report metric** (`report generate`'s `fuzzy_match_percent`, `lib.report.score_entries`) - the number that closes a symbol, not objdiff's positional `diff` value. The diff JSON is still generated and used for the two sizes and the `first-diff@` index, because the report carries neither. This matters for flag decisions: `diff` defaults `functionRelocDiffs` to `data_value` while the report defaults to `none`, so relocation-only differences used to read as sub-100 % code here (`pl_skill` fn_80270018: 99.88 % positionally, **100.0 %** officially).
