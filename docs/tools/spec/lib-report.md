# `lib/report` - Scores: the objdiff report, the one metric, freshness, and one regression rule

## Purpose

Scores: the objdiff report, the one metric, freshness, and one regression rule.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Report.load(path)`, `unit(name)`, `functions(unit) -> {symbol: percent}` (no `fuzzy_match_percent` key = 0 %), `measures(unit)`, `denominators()`, `arithmetic_check(unit)`
* `score(target, base, unit_name, tmpdir, runner) -> Report` = `objdiff report generate` on a one-unit project; `diff_rows(target, base, symbol)` (rows, never a score); `objdiff_cli(tree)`
* `Freshness.unit_reasons(src, obj, tree)`, `report_reasons(report, obj, src)`: strict `<`, include closure through `include/` and `src/`
* `regression(before, after, allow, eps) -> (moved, dropped)`: a unit or symbol row that fell is a drop even if the aggregate rose

## Absorbs (today's implementations)

`unitutil.report_functions/report_measure/measure_project`, `freshguard`, 16 report readers, `reportdiff`, `land.report_snapshot/report_regressions`, `measure.moved_summary`

## Test contract

Tier: fixture (a lib test never reads the live tree). the `metric_selftest` wire checks (one `report generate`, `functionRelocDiffs=none` on diff rows); the 0 % rule; `arithmetic_check` reproduces the unit percent; `Freshness` on temp mtimes; `regression` on the `reportdiff` fixtures

## Known gaps

None until implemented; `migration.md` names the package.
