# `prepcommit` - Classify `git status` paths into stage/refuse, write a results-bearing message, print the commit command (land.py imports its classifier)

<!-- generated from the module docstring of `tools/git/prepcommit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Prepare a commit for the current results - stage the paths, write the message, execute nothing.

## Users

the landing gate (1); skills (2); docs (13); imported by `land`

## CLI

```
python tools/git/prepcommit.py [--dry-run] [--split] [--message SUBJECT] [--commit]
```
Flags: `--commit`, `--dry-run`, `--message`, `--split`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: git status, report.json -> staged index, .git/prepcommit_msg.txt.

## Invariants and rules

* The decompile-symbol skill's last step is "leave a prepared commit". This is that step, and it exists because the parts that go wrong are mechanical:
* `git add -A` would sweep up another agent's in-flight file (this repo has three agents in it),
* build output, `orig/`, `.lavish/`, `.pi/` and scratch must never be staged, and a stray file in the tree (a `.stackdump`, a `__pycache__`) is an accident worth refusing rather than committing,
* the commit message is supposed to carry the *results*, and those numbers already exist in `build/RMHE08/report.json`.
* What it does: classifies every path in `git status` into stage / refuse, stages only the stageable ones, writes the message (measured results included) to `.git/prepcommit_msg.txt`, prints the commit command - and stops. `--split` prints a one-concern-per-commit plan instead of staging everything at once. `--commit` runs the commit for you and is only for when the user has explicitly asked for one.

## Lib dependencies

git, report, repo.

## Test contract

Tier: fixture.
No selftest today.
Target: `tools/tests/git/test_prepcommit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
