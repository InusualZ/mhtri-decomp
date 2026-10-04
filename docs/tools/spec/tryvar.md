# `tryvar` - Try named source rewrites of a unit from a variants file and report the official per-function metric; `--apply` lands the winner

<!-- generated from the module docstring of `tools/flags/tryvar.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Try source rewrites of a unit and report the resulting objdiff match, per function.

## Users

skills (7); CLAUDE.md (1); docs (8)

## CLI

```
python tools/flags/tryvar.py                        # every variant of the default variant file
python tools/flags/tryvar.py --list
python tools/flags/tryvar.py <name> [<name> ...]
python tools/flags/tryvar.py -u <unit> [--variants <file.py>]
python tools/flags/tryvar.py --apply <name>          # LAND the winning rewrite in the real source
```
Flags: `--apply`, `--flags-extra`, `--list`, `--unit`, `--variants`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit + variants/<lib>.py -> table, src edit.

## Invariants and rules

* The harness is generic: it copies the unit's source, applies a variant's rewrite, compiles that copy with the unit's exact ninja command line, and diffs it against the split target object. The real source is never modified and the unit's object is never clobbered. Variants live in a separate data file, so the rewrites for one unit do not leak into the tool.
* The per-function number this prints is the **official report metric** (`report generate`'s `fuzzy_match_percent`, via `lib.report.score_entries`) - the same number `build/RMHE08/report.json`, `ledger.py` and `land.py` read. objdiff's explicit `diff` mode is deliberately not used for the score: it defaults `functionRelocDiffs` to `data_value` (the report defaults to `none`, so relocation-only differences counted as mismatches there) and its `match_percent` is a different normalisation (measured on this repo: `pl_skill` fn_80270018 reads 99.88 % positionally and **100.0 %** officially).
* A variant file (default: `tools/flags/variants/<lib>.py`, i.e. next to this script, named after the unit's library) defines:
```
VARIANTS = [(name, repls), ...]
```
* where `repls` is either a list of `(old, new)` string pairs or a callable `src -> src` (returning `None` means "the pattern did not match").
* `--apply <name>` writes that variant's rewrite into the unit's real source file (preserving its line endings), so a win becomes progress on the unit instead of staying an experiment. It refuses to write anything unless the rewrite applies cleanly and actually changes the file. Rebuild and re-measure afterwards - the recorded evidence must come from the real source, not from the probe.

## Lib dependencies

units, report, text.

## Test contract

Tier: fixture via metric_selftest.
No selftest today.
Target: `tools/tests/flags/test_tryvar.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
