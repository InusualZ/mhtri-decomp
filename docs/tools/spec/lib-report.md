# `lib/report` - Scores: the objdiff report, the one metric, freshness, and one regression rule

## Purpose

Reads objdiff reports with the campaign's one scoring rule, produces the official score of an object pair (`report generate`
on a one-unit project), compares two reports, judges a batch's regressions, and says whether a prebuilt object or report is
older than the sources it describes.

## Users

`recompile` (`--measure`, `diff_rows`), `measure`, `ledger`, `verifyunit`, `unitscore`, `symdiff`, `relocdiff`, the `flags/` tools,
`landing` (the base snapshot and the regression row). No shim remains: WP6 deleted
`tools/objdiff/freshguard.py`, `tools/units/reportdiff.py` and `tools/unitutil.py` (with its `report_*` delegates).

## Public API

* Numbers and the rule: `num(value)` (counts are strings, percents floats); `score_of(entry) -> float` (**no numeric
  `fuzzy_match_percent` = 0.0**), `is_scored(entry)`, `entry_score(entry) -> float | None` (unscored kept apart for display);
  `arithmetic_check(measures, entries, tol=ARITH_TOL) -> (ok, detail)`.
* `Report(data, path)` (frozen): `Report.load(path)` (`ReportError` for missing / unreadable / not a report), `read(path)`
  (missing file = the empty report), `Report.coerce(report | dict | None)`; `units()`, `unit(name)`, `entries(name)`,
  `functions(name) -> {symbol: score}`, `scores() -> {unit: {symbol: score}}`, `measures(name=None)`, `unit_measures()`,
  `symbol_measures() -> {(unit, symbol): score}`, `denominators()`, `arithmetic_check(name)`.
* Comparisons: `moved(before, after, eps)` (the one moved-row rule over two `{key: score}` maps), `direction(rows)`
  (`{moved, up, down}`), `drops(rows, eps)`; `diff_units`, `diff_symbols`, `diff_denominators` (moved rows worst first,
  added/removed, regression flags; `MEASURE_ORDER`, `REGRESSION_KEYS`, `DEFAULT_EPS`); `compare(before, after, eps,
  before_path, after_path)` - the whole two-report diff with the "every moved row that fell" verdict as `exit`
  (0 clean, 1 a drop, 2 nothing comparable) - `reportdiff`'s payload.
* Address pairing: `address_rows(report, unit=None) -> [{unit, name, address, size, score}]` (the address is the
  row's `metadata.virtual_address`), `address_key(row)`, `diff_by_address(before_rows, after_rows, eps) -> {up, down,
  new, removed, renamed, paired}` - a row renamed or moved to another unit pairs by its address (`unitscore
  --baseline`).
* The gate's rule: `snapshot(report) -> {unit: {fuzzy, matched_code, symbols: {name: score}, all: True}}`, `unit_grew(prior,
  after)`, `regression(before, after, allow=(), eps) -> (unauthorised, authorised)` as `(unit, what, before, after)`.
* Scoring: `score(target, base, unit_name, tmpdir, *, objdiff, cwd, runner) -> Report` (raises `ReportError`),
  `score_entries(...) -> {symbol: entry} | {"_error": text}`, `symbol_score(...) -> {symbol, match_percent, target_size,
  report_json} | {symbol, error}`, `write_project(target, base, unit_name, tmpdir)`, `first_unit(report)`,
  `diff_rows(target, base, symbol, objdiff, tmpdir, runner)` (rows, never a score), `project_diff(root, unit_name,
  symbol, out, objdiff, runner) -> (path | None, log)` (`diff -p . -u` in a tree: the tree's own `objdiff.json`, rows
  only), `objdiff_cli(root, main=None)`, `retry_transient(fn, attempts=4)` (a transient `PermissionError` retried
  with backoff).
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
* **The seam-move credit** (`seam_exempt`, 2026-10-07). The snapshot also holds each function's address (`addrs`) and the
  unit's `total_code`. When the batch's `splits.txt` diff moves `.text` between units (`lib.project.splits.range_moves`),
  a *unit-average* row of a unit a move names is lifted iff: (1) every function in both snapshots, paired by address,
  scores >= before; (2) every function that changed unit lies inside a move from its old unit to its new one, and none
  vanished; (3) the matched code over the touched units and `whole_fuzzy` did not fall (tolerance 1e-3 percent points);
  (4) the lifted unit lost or gained a function. A per-symbol row is never lifted; a snapshot without `addrs` lifts nothing.
* **The snapshot is taken at `record-base` and kept in `.pi/`**: `ninja baseline` rewrites the baseline a later comparison
  would need, so two verifies of one tree both read "no regression" while the ledger said 231 -> 228.
* **Freshness is strict `<`**: `report.json` is written in the same whole second as the last object, so an equal stamp is
  current. A unit's inputs are its source **and every in-tree header it reaches** (beside the includer, then `src/`, then
  `src/`; commented includes and system headers never date it). `report.json` is an order-only target of `all_source`:
  after an edit `ninja build/RMHE08/report.json` prints "no work to do" and still holds the previous scores (a lane reported
  two "improvements" that were never built).

## Absorbs (today's implementations)

`unitutil.report_functions/report_measure/measure_project/objdiff` (now delegates), `freshguard` (all of it; the file is a
re-export shim), `symdiff.retry_transient`, `mwcc_matrix.diff_unit`'s and `slotmap`'s project-mode diff, `reportdiff.build`
(now `compare`), `measure.moved_summary`'s count, `reportdiff.num/load_report/unit_measures/symbol_measures/denominators/diff_units/diff_symbols/
diff_denominators`, `land.report_snapshot/unit_grew/report_regressions`, `recompile.measure/diff_rows`,
`measure.score_report/objdiff_path`, `ledger`'s score table, `verifyunit.report_unit/_score/arithmetic_crosscheck`,
`unitscore.rows_of`'s score.

## Lib dependencies

`lib.repo` (`session_tmpdir`, imported on first use).

## Test contract

Tier: fixture (`tools/tests/lib/test_report.py`, 72 checks). The 0 % rule on every reader; `arithmetic_check` reproduces the
unit percent and fails the 100 % reading; `Report.load` refuses the three non-reports; the comparisons on the `reportdiff`
fixtures, including a symbol losing its score as a drop to 0; `moved`/`direction`/`drops` and `compare`'s three exits;
`snapshot`/`regression`/`unit_grew` on hand-built snapshots; the scoring wire with a stub runner (one `report generate`,
one-unit project, absolute paths, the pinned version, the error paths, `functionRelocDiffs=none` on diff rows and on
`project_diff`, `retry_transient`); `Freshness` on temp mtimes (strict `<`, a header edit dates the object, report mode's two
reasons). The integration rows (the real objects) stay in `tools/objdiff/metric_selftest.py`.

## Known gaps

* Two regression policies remain over the one comparison, on purpose: `compare` ("every moved row that fell", over
  two whole reports - `reportdiff`, and `measure`'s baseline count reads the same `moved`/`direction`) and `regression`
  (the gate, over two `snapshot`s). WP3b folded `reportdiff`'s verdict and `measure.moved_summary` onto `moved`; folding
  them onto `regression` would import its two blind spots below into tools that do not have them.
* WP4 decided both blind spots by replaying the last 15 report-affecting landings' fresh base/after reports
  (`.pi/notes/worktree-agent-a67d2563a558742ce.md`, the WP4 report): `snapshot` now holds every symbol (`"all": True`), so a symbol at 100 % that falls, or
  loses its score (100 -> 0.0), is a drop (none did in the 15); `unit_grew` reads `matched_code` as a number (its answer
  changed for 14 unit pairs, no verdict) and keeps reading the sub-100 % set of either format (counting a full
  snapshot's 100 % names would make a rename "growth": it would have flipped 7210b07c9's `ef/effect` allowance to
  stale). Verdicts on the 15: identical, also against a pre-WP4 (sub-100) base.
* `source_closure` delegates to `lib.cscan.include_closure` since WP3d (depth first, the preprocessor's order; comments
  removed through `cscan.remove_comments` before `cscan.includes`; no depth cap - each file is visited once). The one
  output that moved is the order of `unitscore --json`'s `sources.paths`: on the 354 registered units the set is the
  same for all, the order differs for 210 (`lib-cscan.md`); `INCLUDE_RE`/`MAX_INCLUDE_DEPTH` are gone (`freshguard`
  re-exported them; nothing read them).
* `ledger.stale` (report vs `splits.txt`/`configure.py`) and `pairgap.report_scores`/`datagap.units_from_report` (WP3a) are
  still private readers.
