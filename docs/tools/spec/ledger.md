# `ledger` - Campaign progress from the repository: covered/registered/closed totals, `next` unclaimed symbols, one unit's rows; `Objects` maps addresses to split objects

<!-- generated from the module docstring of `tools/units/ledger.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Derive the campaign's progress from the repository: what is covered, what is registered, what is next.

## Users

the landing gate (2); the selftest runner (1); profiles (`.claude/agents`) (1); CLAUDE.md (2); docs (16); imported by `dataclaim`, `dataqueue`

## CLI

```
python tools/units/ledger.py [--json]           # totals, per-module table, units at >= 80 %
python tools/units/ledger.py next [N]           # the next N unclaimed symbols, in address order
python tools/units/ledger.py unit <unit>        # one unit: ranges, symbols, per-symbol score
```
Subcommands: `next`, `unit`.
Flags: `--config`, `--json`, `--named`, `--report`, `--selftest`, `--type`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: splits, configure.py, config.json, report.json -> totals.

## Invariants and rules

* Nothing here writes and nothing is stored: the loop's state *is* the repository. `splits.txt` says which addresses a unit owns, `configure.py` says which units are registered and how they are built, `build/RMHE08/config.json` says which split object holds a given address, and `build/RMHE08/report.json` says what each unit scores. A ledger file would be a fourth copy of all that, and it would be the wrong one the first time a subagent edited the repo without updating it.
* Metric: report version 2, where a *function* entry carries `fuzzy_match_percent` and an entry **without** that key is 0 %, not 100 % (`complete_code_percent` is not a score). The bar is 80 on that per-symbol number, which is what step 3 of `docs/plan.md` closes a symbol against; playbook 15 pins it to the objdiff version in `configure.py`, so a tool bump means re-baselining.
* `next` is what step 1 of the campaign consumes - an address, and the split object step 2 has to disassemble. Both come out of the same repository state the other tools read, so a fresh session resumes with no history.

## Lib dependencies

project, report.

## Test contract

Tier: fixture (text fixtures).
Today's selftest (`tools/units/ledger_selftest.py`): No build, no `ninja` and no repository state: the ledger's input is text, so the fixture below is one tiny `symbols.txt` / `splits.txt` / `configure.py` / `report.json` set, injected through the constructor. That keeps the contract explicit - which symbols count as covered, when a score counts as closed, which symbol `next` picks, and when the report is too old to believe - instead of re-deriving it from whatever the tree happens to contain today. The real-data counterpart is `symbolpreflight`'s self-test, which pins the parsers these views stand on.
Target: `tools/tests/units/test_ledger.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
