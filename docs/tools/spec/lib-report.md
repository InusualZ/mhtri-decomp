# `lib/report` - Scores: the objdiff report, the one metric, freshness, and one regression rule

## Purpose

Reads objdiff reports with the campaign's one scoring rule, produces the official score of an object pair (`report generate`
on a one-unit project), compares two reports, judges a batch's regressions, and says whether a prebuilt object or report is
older than the sources it describes.

## Users

`unitutil.report_functions/report_measure/measure_project`, `recompile` (`--measure`, `diff_rows`), `measure`, `ledger`,
`verifyunit`, `unitscore`, `symdiff`, `relocdiff`, `reportdiff`, `land` (the base snapshot and the regression row),
`tools/objdiff/freshguard.py` (a re-export shim until WP6).

## Public API

* Numbers and the rule: `num(value)` (counts are strings, percents floats); `score_of(entry) -> float` (**no numeric
  `fuzzy_match_percent` = 0.0**), `is_scored(entry)`, `entry_score(entry) -> float | None` (unscored kept apart for display);
  `arithmetic_check(measures, entries, tol=ARITH_TOL) -> (ok, detail)`.
* `Report(data, path)` (frozen): `Report.load(path)` (`ReportError` for missing / unreadable / not a report), `read(path)`
  (missing file = the empty report), `Report.coerce(report | dict | None)`; `units()`, `unit(name)`, `entries(name)`,
  `functions(name) -> {symbol: score}`, `scores() -> {unit: {symbol: score}}`, `measures(name=None)`, `unit_measures()`,
  `symbol_measures() -> {(unit, symbol): score}`, `denominators()`, `arithmetic_check(name)`.
* Comparisons: `diff_units`, `diff_symbols`, `diff_denominators` (moved rows worst first, added/removed, regression flags;
  `MEASURE_ORDER`, `REGRESSION_KEYS`, `DEFAULT_EPS`).
* The gate's rule: `snapshot(report) -> {unit: {fuzzy, matched_code, symbols: {name: score < 100}}}`, `unit_grew(prior,
  after)`, `regression(before, after, allow=(), eps) -> (unauthorised, authorised)` as `(unit, what, before, after)`.
* Scoring: `score(target, base, unit_name, tmpdir, *, objdiff, cwd, runner) -> Report` (raises `ReportError`),
  `score_entries(...) -> {symbol: entry} | {"_error": text}`, `symbol_score(...) -> {symbol, match_percent, target_size,
  report_json} | {symbol, error}`, `write_project(target, base, unit_name, tmpdir)`, `first_unit(report)`,
  `diff_rows(target, base, symbol, objdiff, tmpdir, runner)` (rows, never a score), `objdiff_cli(root, main=None)`.
* Freshness: `freshness(use_report, report_mtime, object_mtime, newest_source, report_path, object_path, rel)`,
  `unit_reasons(src, obj, root, rel)`, `report_reasons(report, obj, src, root, rel)`, `source_closure(src, root)`,
  `includes_of`, `resolve_include`, `newest`, `mtime`, `stamp`, `stamp_json`, `rel_path`; `Freshness` groups the three.

## Invariants and rules

* **A function entry without `fuzzy_match_percent` is 0 %, not 100 %.** The unit percent is exactly the sum of the listed
  partials over `total_code`; `arithmetic_check` proves the reading (with the key read as 100 the identity fails).
  `complete_code_percent` is not a score. Every reader here applies the rule; `entry_score` keeps "unscored" visible for a
  table but never as a number.
* **The official number is `report generate`.** `objdiff diff` defaults `functionRelocDiffs` to `data_value` (report:
  `none`) and its `match_percent` is a different normalisation (`RSOStaticLocateObject` 99.38 vs 99.64; `pl_skill`
  `fn_80270018` 99.88 vs 100.0). `score` runs `report generate` on a one-unit project (~0.04 s); `diff_rows` passes
  `-c functionRelocDiffs=none` and exposes the positional value only as `diff_match_percent`.
* **The one-unit project uses absolute paths**: objdiff joins a relative path to the project directory, and on Windows only a
  backslash-rooted path is absolute. The report is left at `<tmpdir>/unitutil_report.json` (callers read it back); `tmpdir`
  defaults to the process's own `session_tmpdir()`, so two lanes measuring one unit never share a file.
* **A failed score is an error, never 0.0** (`ReportError`, `{"_error"}`, `{"error"}`); an unknown symbol is an error.
* **The regression rule.** A symbol the previous snapshot held whose score fell is a drop, named with both numbers, even
  when the aggregate rose; a symbol the previous snapshot did not hold is new (an extension), never a drop; a symbol gone from
  `after` reached 100 %. The unit average speaks only when no symbol does and the unit did not grow (a sub-100 % symbol it
  did not hold, or more matched bytes) - the gate refused the g3d_resshp head-plus-tail join and the 800997e0 extension
  before that (2026-09-25). `auto_*` scaffold units outside `/auto/` are bookkeeping. `allow` authorises a measured cost.
* **The snapshot is taken at `record-base` and kept in `.pi/`**: `ninja baseline` rewrites the baseline a later comparison
  would need, so two verifies of one tree both read "no regression" while the ledger said 231 -> 228.
* **Freshness is strict `<`**: `report.json` is written in the same whole second as the last object, so an equal stamp is
  current. A unit's inputs are its source **and every in-tree header it reaches** (beside the includer, then `include/`, then
  `src/`; commented includes and system headers never date it). `report.json` is an order-only target of `all_source`:
  after an edit `ninja build/RMHE08/report.json` prints "no work to do" and still holds the previous scores (a lane reported
  two "improvements" that were never built).

## Absorbs (today's implementations)

`unitutil.report_functions/report_measure/measure_project` (now delegates), `freshguard` (all of it; the file is a
re-export shim), `reportdiff.num/load_report/unit_measures/symbol_measures/denominators/diff_units/diff_symbols/
diff_denominators`, `land.report_snapshot/unit_grew/report_regressions`, `recompile.measure/diff_rows`,
`measure.score_report/objdiff_path`, `ledger`'s score table, `verifyunit.report_unit/_score/arithmetic_crosscheck`,
`unitscore.rows_of`'s score.

## Lib dependencies

`lib.repo` (`session_tmpdir`, imported on first use).

## Test contract

Tier: fixture (`tools/tests/lib/test_report.py`, 59 checks). The 0 % rule on every reader; `arithmetic_check` reproduces the
unit percent and fails the 100 % reading; `Report.load` refuses the three non-reports; the comparisons on the `reportdiff`
fixtures, including a symbol losing its score as a drop to 0; `snapshot`/`regression`/`unit_grew` on hand-built snapshots;
the scoring wire with a stub runner (one `report generate`, one-unit project, absolute paths, the pinned version, the error
paths, `functionRelocDiffs=none` on diff rows); `Freshness` on temp mtimes (strict `<`, a header edit dates the object, report
mode's two reasons). The integration rows (the real objects) stay in `tools/objdiff/metric_selftest.py`.

## Known gaps

* Three regression policies remain over the one comparison: `regression` (the gate), `reportdiff`'s "every moved row that
  fell" verdict (a CLI with no caller) and `measure.moved_summary` (a count over measure's own rows). `applysplits` retired
  with the program. Folding the last two onto `regression` changes their output, so it waits for WP3b (the score family).
* `unit_grew`'s matched-bytes signal compares numbers, but the report stores `matched_code` as a string, so in practice only
  the symbol-set signal fires. Kept as found (a gate behaviour change); WP4 decides.
* `measure.load_baseline` still reads an unscored baseline row as "no baseline" rather than 0 % (it changes measure's delta
  column); WP3b.
* `source_closure` is C text scanning; `lib.cscan.include_closure` (WP2c) is its eventual home.
* `ledger.stale` (report vs `splits.txt`/`configure.py`) and `pairgap.report_scores`/`datagap.units_from_report` (WP3a) are
  still private readers.
