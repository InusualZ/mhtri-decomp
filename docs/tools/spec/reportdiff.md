# `reportdiff` - Diff two `report.json` snapshots: moved unit/symbol rows, units added/removed, category denominators; exit status is the verdict (no caller today)

<!-- generated from the module docstring of `tools/units/reportdiff.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Diff two `build/<game>/report.json` snapshots: which rows moved, which units appeared, and the category denominators - the instrument two lanes hand-wrote and neither kept.

## Users

no caller in the tracked tree

## CLI

```
python tools/units/reportdiff.py <before.json> <after.json> [--json] [--changed-only] [--limit N]
python tools/units/reportdiff.py --selftest
```
Flags: `--changed-only`, `--eps`, `--json`, `--limit`, `--selftest`.
Exit codes: This tool is that diff, with the exit status as the verdict.; **Exit status is the verdict** (the house codes):; `0` nothing regressed - no unit or symbol row dropped, no numerator denominator fell;; `1` a row dropped - the drops are listed even when the aggregate improved;; `2` nothing comparable was found - a path that is missing, unreadable, not a report, or a pair with no unit and no denominator in common. A traceback is never the answer.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: two report.json -> tables, exit.

## Invariants and rules

* **The question this answers, and why it needs a tool.** Every data/declaration batch has to prove it did not move codegen ("the whole-project report is unchanged row for row"), and the two lanes that pinned the `Pl` `.sdata2` pool wrote the same JSON diff by hand - twice - from a scratch script that no one else could run. Their acceptance criteria were exactly the ones an ad-hoc diff gets wrong: *no row may drop* (a unit or symbol whose `fuzzy_match_percent` fell is a regression, even if the totals rose) and *any row that moves at all* must be named (the change is either explained by the batch or it is a surprise). This tool is that diff, with the exit status as the verdict.
* **What it prints.**
* **unit rows whose score moved**, with before -> after (worst first), keyed on the unit's `fuzzy_match_percent`;
* **symbol rows whose score moved**, the same way, keyed on `(unit, symbol)` - the report's `functions` rows, which is the level a lane can act on;
* **units added and removed** (a claim re-tiles the `auto_*` units, so the set always churns - the names are the evidence, not a count);
* **the category denominators** - the project totals and every report `category` (`game`, `sdk`, `auto`), as a before/after table with deltas: `fuzzy`, `matched_code`, `matched_data`, `total_units`, `total_code`, `total_data`, `matched_functions`, `complete_*` and the percent columns.
* **Exit status is the verdict** (the house codes):
* `0` nothing regressed - no unit or symbol row dropped, no numerator denominator fell;
* `1` a row dropped - the drops are listed even when the aggregate improved;
* `2` nothing comparable was found - a path that is missing, unreadable, not a report, or a pair with no unit and no denominator in common. A traceback is never the answer.
* **Direction matters, so it is explicit.** `<before.json>` is the baseline, `<after.json>` the candidate; swapping the arguments swaps every delta and the verdict. Both files are ordinary `report.json`s (dtk's `ninja build/<game>/report.json`), so `git show` of a committed report, a copy of an earlier build, or MAIN's own report all work unchanged.

## Lib dependencies

report.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_reportdiff.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* no caller today. Since WP3b the verdict (`build`) is `lib.report.compare` - "every moved row that fell" over
  `lib.report.moved`/`drops`, the rule `measure.py --baseline` counts with too; this file keeps only the renderer and the
  CLI. The gate's `lib.report.regression` is a different policy on purpose (it judges a batch against a `record-base`
  snapshot that holds only sub-100 % symbols): see `lib-report.md` Known gaps.
