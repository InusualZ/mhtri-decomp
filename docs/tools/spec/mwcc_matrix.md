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
python tools/flags/mwcc_matrix.py -u <unit> --only-open     # only the functions below 100 %
python tools/flags/mwcc_matrix.py -u <unit> 1.3 --flags-extra="-O4,s" --one   # compile + score one variant, object kept aside
python tools/flags/mwcc_matrix.py -u <unit> --one --json    # {unit, variants: [{label, flags, object, rows, hidden_full}]}
```
Flags: `--flags-extra`, `--list-versions`, `--unit`, `--only-open`, `--one`, `--json`. `mt.py matrix` forwards all of them.
A `--flags-extra` value that starts with `-` needs the `=` spelling (`--flags-extra="-O4,s"`).
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit -> build/tmp/matrix/*.

## Invariants and rules

* The compile command is the exact one ninja would run for that unit (so whatever `configure.py` puts in the unit's `cflags` is honoured); `--flags-extra` overrides same-family flags, and a compiler version argument swaps the MWCC executable. The object is written to the unit's real output path so that objdiff's project mode can diff it against the split target object.
* **`--only-open`** keeps the rows whose official match is below 100 % - filtered on the number (`open_rows`), never on
  the printed text: `grep -v 100.00%` dropped nothing it should and `grep -v 0.00%` dropped the 100 % rows too - and
  prints how many full rows it hid.
* **`--one`** is the one-step try: one variant (at most one version), compiled with the unit's real command, scored
  with the official metric, its object copied to `build/tmp/matrix/one/<label>/<unit>.o`, and the unit's own object put
  back byte- and mtime-exact (`ScratchObject`; removed if there was none), so ninja has nothing to rebuild and no
  foreign object is left. Measured on `g3d/g3d_gpu` with `-O4,s`: 67.32 % / 50.60 % rows, the object kept, the real
  object's SHA-1 unchanged.
* Writes: build/tmp/matrix/<label>.json (raw objdiff diff per variant) build/tmp/matrix/summary.txt (the printed table) Leaves a foreign object behind - restore with: rm -f <unit obj> && ninja <unit obj>

## Lib dependencies

units, report.

## Test contract

Tier: fixture (`tools/tests/flags/test_mwcc_matrix.py`, 10 checks: `open_rows`/`render_block` on 100 %, 0.00 % and
99.99 % rows; `ScratchObject` keeps the variant, restores bytes and mtime, leaves no backup, removes an object that was
not there). Mutations: filtering out 0.00 % instead of 100 % fails 3, a restore without the mtime fails 1. The compile
itself is not under test (it needs the toolchain).
Target: `tools/tests/flags/test_mwcc_matrix.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* without `--one` it still leaves a foreign object in the unit's output path (printed with the restore command)
* `--list-versions` listed the files of the compiler's own version directory until WP3b; it now lists the sibling version
  directories (`lib.units.available_versions`, shared with `frame.py --versions`).

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The `match` column is the **official report metric** (`report generate`'s `fuzzy_match_percent`, `lib.report.score_entries`) - the number that closes a symbol, not objdiff's positional `diff` value. The diff JSON is still generated and used for the two sizes and the `first-diff@` index, because the report carries neither. This matters for flag decisions: `diff` defaults `functionRelocDiffs` to `data_value` while the report defaults to `none`, so relocation-only differences used to read as sub-100 % code here (`pl_skill` fn_80270018: 99.88 % positionally, **100.0 %** officially).
