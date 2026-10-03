# `measure` - Score a whole unit in one compile and one `report generate`, with per-symbol deltas against the last run, a saved baseline or MAIN's report

<!-- generated from the module docstring of `tools/units/measure.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Score a whole unit in one compile and one `objdiff report generate` - the search-loop measurer.

## Users

the selftest runner (1); docs (28); imported by `verifyunit`

## CLI

```
python tools/units/measure.py <unit> [symbol] [--json] [-q]
```
Flags: `--against-main`, `--baseline`, `--diff`, `--json`, `--limit`, `--main`, `--no-cache`, `--save`, `--sort`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit source -> table/JSON, build/tmp/measure.

## Invariants and rules

* What it does, in order, and the only place it differs from `recompile.py`:
* One compile, one report, N symbols: the cost of the search loop is the compiler, not N x the measurer. `recompile.py --measure` remains the tool for a single-symbol proof; this one is for the round.
* The unit is the path from the repository root (`Pl/pl_act`, `Camellia/camellia`, `auto/8005AA28_fn_8005AA28`); the extension may be omitted and is inferred from the worktree's `src/`. Run it from the worktree you are editing, or from MAIN with `--main` left to `git worktree list`; the source, `-o` directory and `-i` order are always this tree's (that half is `recompile.py`'s, not this file's).

## Lib dependencies

units, report, repo.

## Test contract

Tier: fixture (fake runners); smoke: integration against the live report.
Today's selftest (`tools/units/measure_selftest.py`): The contract this pins is the one that made 61 workers write their own driver: **one compile and one `objdiff report generate` for all N symbols of a unit**, scored with the official `fuzzy_match_percent` (the metric `build/RMHE08/report.json` carries), with the worktree's own split object preferred over MAIN's retired fallback. `recompile.py --measure` answers one symbol per run with two objdiff calls; this tool must not turn that into N runs. Layers:
Target: `tools/tests/units/test_measure.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `tools/units/recompile.py --measure <symbol>` answers one symbol per run, which is the right shape for the *proof* step and the wrong shape for a search: a worker who wants to know whether a shape helped has to run it once per symbol of the unit, and each run recompiles the source and issues **two** objdiff calls (the report for the score, the positional diff for the rows). Workers hit that and each hand-wrote the same driver - `build/tmp/mp.py`, `build/tmp/measure_all.py`, `build/scratch/*.py`, `.pi/scratch/score.py` - a compile plus **one** report over the unit's own target object, printed as a per-symbol table. This is that driver, shipped.
* takes the unit's **real** command line from the same construction `recompile.py` uses (`recompile.unit_tokens` -> MAIN's ninja, the worktree's ninja, then a same-lib sibling) and compiles it into this tree's `build/RMHE08/src/...` through `recompile.compile_unit` (fresh-object assertion included). Nothing about the command line is re-derived here;
* resolves the **target** object the way a worker needs it: the worktree's own split object first (a proposal whose registration has landed on the branch but not on MAIN), then MAIN's registered object, then MAIN's retired `auto_*_text` object for the symbol's address (`recompile.proposal_target`);
* scores **every** symbol with one `report generate` over a one-unit project - the official `fuzzy_match_percent`, the same number `build/RMHE08/report.json` carries - and prints the unit's own official measures (``fuzzy_match_percent``, ``matched_functions``) beside a per-symbol table;
* remembers the previous run's scores in `build/tmp/measure/<unit>.scores.json` and prints the **delta** per symbol, which is what tells a worker "did this shape work" without babysitting a spreadsheet;
* `--baseline <report.json>` (or `--against-main`) makes that delta compare against a **saved** project report or **MAIN's** `build/RMHE08/report.json` instead of the tool's own last run - the shape the hand-written scorers all converged on (`build/probe/score.py` diffed a probe's rows against the committed report). One call then answers "every symbol of this unit, and what each one is worth against the build that landed", which is the per-iteration question. `--save` writes this run in that shape for the next one. A baseline that moved a row **down** is a regression, and the summary says so;
* `--diff` (or a symbol focus) adds a compact instruction-level mismatch list for the symbol, read from `recompile.diff_rows`' diagnostic JSON - never quoted as the score.
