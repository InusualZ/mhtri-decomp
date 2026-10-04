# `selftest` - One runner for every tool selftest: discovery, dedupe of wrapper pairs, parallel bounded execution, flake retry, park list, `--changed` source mapping

<!-- generated from the module docstring of `tools/selftest.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

One runner for every tool's selftest - discovery, parallel execution, aggregation, a park list.

## Users

the landing gate (10); profiles (`.claude/agents`) (2); skills (1); CLAUDE.md (2); docs (16)

## CLI

```
python tools/selftest.py                 # run every discovered selftest
python tools/selftest.py --changed       # only the selftests of tools this diff touches
python tools/selftest.py --changed main  # ... of everything since `main`
python tools/selftest.py --json          # machine-readable summary only (no table)
python tools/selftest.py --list          # the inventory, without running anything
python tools/selftest.py --no-dedupe     # run both halves of every wrapper pair
python tools/selftest.py --selftest      # this runner's own checks (it is discovered like any other)
```
Flags: `--changed`, `--jobs`, `--json`, `--list`, `--no-dedupe`, `--park-file`, `--root`, `--selftest`, `--tier`, `--timeout`.
`--tier fixture|smoke|all` (default all) filters the `tools/tests/**` modules by their `TIER`; legacy and `--check` entries run in every tier.
Exit codes: Exit status is the answer: 0 only when nothing failed, nothing moved the tree, and no park is stale.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: tools/** -> table/JSON, exit.

## Invariants and rules

* **Two shapes, one inventory.** Every tool is tested one of two ways:
* a standalone `tools/**/<name>_selftest.py`, run as `python <file>`; and
* a tool exposing `--selftest`, run as `python <tool.py> --selftest` - only an **entry point**
  (`lib.cli.runs_own_selftest`: the file registers the flag, has a module-level `__main__` guard and is not a module of
  a tool package). A package module that mentions the flag (`tools/units/merge/mergebranch.py`, a package's
  `cli.py`) ran nothing on its own and was an entry until WP4 (2 on main at ec8223e66, plus the deduped
  `lib/cli.py` and `mwlink/cli.py`).
* They are one *entry* per tested tool (keyed by the module path with `_selftest` stripped), because many are wrappers of each other: a tool's `--selftest` can import and run its `<tool>_selftest.py`, and `dossier_selftest.py` calls `dossier.selftest()`. Running both would run the same checks twice and waste the gate's time, so a delegating pair is collapsed to its **tool** entry (the documented contract); a genuine pair that does *not* delegate - `ledger.py --selftest` covers the per-0x10000 view, `ledger_selftest.py` the older views - keeps **both**, because neither is a duplicate of the other. `--no-dedupe` runs everything.
* **`--changed` maps sources, not only `tools/` (F37).** A batch that edits only docs can still break an invariant, and no `tools/**` selftest covers it: `docs/plan.md` is the source of the section-6.5 block generated into `.claude/agents/*.md` (`tools/agents/sync_profiles.py`), and `docs/matching/` (one file per playbook idea) is the source of the skill's `references/matching/` copy (`.claude/skills/mwcc-unit-matching/scripts/sync_reference.py --check`) and of the generated `docs/matching/index.md` (`tools/agents/sync_playbook_index.py --check`). Both are selected when the diff touches those sources, so a docs batch verifies itself instead of reporting "GREEN, 0 selftests". The mapping is `SOURCE_ENTRIES`/`SOURCE_CHECKS` below.
* **Parallel, bounded, and never able to hang the gate.** Each selftest runs in a bounded worker pool with a per-test timeout; on timeout the whole process tree is killed (several tests shell out to `ninja` and the compiler, so a wedged child is a real risk, not a formality). The aggregate table names every tool, its check count and its duration; a failure carries the head of its output.
* **The tree-dirty guard.** `git status --porcelain` is captured before and after the whole run and must be identical - a selftest that writes into the real repository is a defect, and it is invisible unless something checks. If it moved, the offender is named with the exact before/after rows. **The live slot manifest** (`.pi/slots/pool.json`) is guarded the same way but by **bytes**, because `.pi/` is gitignored and the dirty guard cannot see it: that file is the campaign's concurrency cap, and a run that rewrites it shrinks the pool for every live lane.
* **The park list** (`tools/selftests-known-failures.json`) records each *pre-existing* failure with a reason and a date, so one old red cannot hide every new one: the summary reads "green except N parked". Parking is explicit and greppable, never a silent skip, and a park whose test now **passes** is itself reported as `STALE` (and fails the run) so a debt cannot rot unnoticed.
* Exit status is the answer: 0 only when nothing failed, nothing moved the tree, and no park is stale.

## Lib dependencies

proc, repo, testing.

## Test contract

Tier: fixture (its own `--selftest`).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/top/test_selftest.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

discovery by `--selftest` flag scan and delegation regexes is replaced by `tools/tests/**` discovery (design 7)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The incident this closes.** `tools/units/measure_selftest.py` was red for weeks while 31 lanes filed "`recompile.py` is broken": the tool's own test said so and nothing ran it. A selftest nobody runs is decoration, and the land gate only ran `land.py --selftest` - the gate's own tests, never the suite. This is the one command that runs them all, so a stale tool cannot hide behind a green gate.
* **A failure is re-run once, alone (2026-09-30).** The suite runs `--jobs` tests at once, on a machine that is also building (live lanes), and a test that touches git, a temp tree or the clock can lose that race without being wrong: four landings were refused by `claims`, `ideas` and `slots` selftests that passed by hand a minute later. So every fail/timeout is re-run ONCE, serially, after the pool has drained. Passing the second time is a **flake**: it passes the row, prints `flaky: ... passed on isolated re-run` loudly on stderr, lands in the JSON summary (`flaky`) and is appended to `.pi/selftest-flakes.jsonl` (tool, time, the first failure's last lines) so flakes are counted, not silently eating landings. A test that fails twice fails the row, and the refusal carries the last 40 lines of its output (`FAIL_TAIL_LINES`).

## Moved from the module docstring (WP6)

From `tools/selftest.py`:

**The third shape: `tools/tests/**/test_*.py` (WP0, docs/tools/design.md section 7).** A `lib.testing` module
declares `TIER = "fixture"` (the default) or `TIER = "smoke"`; it is run as `python <file>` with the tier in
`TOOLS_TEST_TIER`, and a fixture-tier module runs with its cwd in a fresh temp dir. Its key is the tool it tests
(`tools/tests/units/test_land.py` -> `tools/units/land`; `tests/smoke/` keeps its own path, `tests/top/` maps to
`tools/<name>`), so a re-homed test replaces the tool's old entry (dedupe; `--no-dedupe` runs both) and a park on
the old target still matches. `--tier fixture|smoke` filters these entries only; the older entries (tier
`legacy`) and the synthetic `--check` entries (tier `check`) run in every tier until they are re-homed.
