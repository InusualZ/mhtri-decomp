# `landing` - the landing gate as a package: `record-base`, the rows of `verify`, `land`, `land --branch`, `resolve`

## Purpose

`tools/units/land.py` is the CLI (the prologue and a re-export of `landing.api`); `tools/units/landing/` holds the
gate. The behaviour, the rows and the incidents behind them are `land.md`; this file is the layout.

## Users

`land.py` (the shim); the tools that import `land` keep their names through `landing.api` (`prepcommit`:
`read_base`, `ledger_numbers`; `merge/mergebranch`: `band_ownership_warnings`, `units_from_branch`; the briefing
prints the `land.band_ownership_warnings` command beside `stylelint.py --diff main`).

## Layout (WP4)

* `common.py` - the batch path classes (`outside_batch` - with `main=` the content door for a relocation-keys-only
  `config.yml` change, `config_verdict` -, `is_scratch`, `unit_owned_paths`), the row KIND and the
  refusal wording (`failing_checks`, `failure_summary`), `run`/`git`, `read_base`, `worktree_root`
  (`lib.repo.worktree_root`)/`main_root`, the
  compile targets, and `Batch`: one `verify` run (inputs, the `lib.findings.Row`s so far, `warn` - a WARNING row that
  never refuses, its findings kept on `warnings` for the message and the landing log -, the values a later row
  reads). Imports nothing of the package.
* `state.py` - the invocation's recorded allowances (`--allow-rule10/12`, `--allow-orphan`) and `--unit-rename`
  pairs, read as `state.<NAME>` (a re-exported copy would not see a setter's rebinding); `allowances` folds them with
  `--allow-regression`/`--no-outbox`/`--no-selftests` into the landing log's `allow` and `allow_lines` into the commit
  body's `allow:` lines. The CLI is the one path that sets them (`flow.land` takes no `allow_rule10`).
* `base.py` - `record-base` and the readers it stores (ledger, report snapshot, dirty set); the object and data
  halves of the snapshot come from `rows/objects.py` and `rows/data.py`. Its report rebuild runs the registry's `report`
  command (`lib.artifacts`) unconditionally - ninja judges freshness, a no-op is 0.06 s.
* `message.py` - `.git/land_msg.txt`: written only by a green gate, cleared by every refusal; `message_error`.
* `stage.py` - `land_stageable`, `looks_already_applied`, the pathspec commit, `land_decision`.
* `branch.py` - `land --branch`'s apply (merge-base diff, `git apply -3`), the registration union (`unionguard`,
  `unionresolve`), `resolve`, the resolve-helper sweep, `undo_apply`.
* `release.py` - `release_plan`, `release_unit` (the one `claims.release` call), the release rows.
* `rows/` - one function per row, appending to a `Batch`; grouped by what they read: `tree.py` (ground truth, the
  base, the batch-path guard and scratch, conflict markers, the branch guards, the pre-flight), `batch.py` (outbox,
  branch commits), `rules.py` (style lint with rule 12's allowance, rule 2's band boundary, rule 10), `selftests.py` (the suite row), `subject.py` (commitlint), `build.py` (command rows, the compile gate, the
  `ok` stamp), `objects.py` (`verifyunit`, `undefrefs`, `flipcheck`: registration, references, drift, re-measure),
  `data.py` (`dataclosure`), `regression.py` (`lib.report.regression`); `objects.new_unit_name_row` (pre-build, rule 7: a unit newly registered under a generated name refuses); `knowledge.py` (7.10) was deleted
  2026-10-05 with its row.
* `gate.py` - `verify`: `PRE_BUILD` rows in order, then the build and the rows that read it, the message body.
* `flow.py` - `land` and `land_branch` (gate -> stage -> commit -> release, one answer line).
* `cli.py` - the parser and `main`; `--selftest` forwards to the test modules.
* `api.py` - every name of the package in one namespace (what `land.py` re-exports).

Each foreign tool is imported by exactly one module (`layering-allow.json`: land.py's 14 edges became 11 plus the
shim's `land.py -> landing/api.py`); `recompile`, `subproc` and `unitutil` are no longer imported (`lib.repo`,
`lib.proc`), `claims` only for `release` (`lib.lanes` for naming, the registry and rescue refs), `datagap` became
`dataclosure`, `brief` became `briefing/sources`, `stylelint` became `stylelint_rules/api`.

## Invariants and rules

* A row never shells out to parse another tool's text where the tool has an API; the exceptions that remain are
  listed in `land.md`'s Known gaps (stylelint, selftest, commitlint, ledger, flipcheck run as subprocesses).
* The gate's table (row names, statuses, kinds, order) is pinned by `tools/tests/units/landing/test_gate_golden.py`
  against `gate_golden.py`, recorded from the monolithic `land.py` before the split.
* A test patches where the code looks a name up: `gate.verify` (what `flow.land` calls), `tree.run` (the scratch
  unstage), `common.worktree_root`/`common.main_root` (the CLI's tree resolution).

## Lib dependencies

findings, git, lanes (naming, registry, teardown, landlog), project, proc, repo, report.

## Test contract

Tier: fixture. `tools/tests/units/test_land.py` (the re-homed `land.py --selftest`: the guard, staging, the rows'
decisions, `land`/`land --branch` on fixture repos - among them `test_land_branch_config_relocations`: a
`block_relocations`-only `config.yml` lands with the unit batch in one commit, a frozen key or `build.sha1` refuses at
pre-flight with main unchanged -, the resolver, the CLI) and
`tools/tests/units/landing/`: `test_gate_golden.py` (thirteen scenarios, the whole table - the two `new-unit-*` ones pin the new-unit name row's refusal and its rename credit), `test_landlog_hook.py` (the
landing log), `test_base_report.py` (the base report rebuild and its row), `test_neighbours.py` (names-only
neighbours).

## Known gaps

`verify --json` is parsed but prints nothing extra (the table stays the contract); the stylelint, selftest,
commitlint, ledger and flipcheck rows still run their tools as subprocesses (`land.md`).
