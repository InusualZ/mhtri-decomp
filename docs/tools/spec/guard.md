# `guard` - Pre-commit guard logic: warn on `core.autocrlf`, normalise a CRLF text blob in the index, refuse a binary with CR

<!-- generated from the module docstring of `tools/git/guard.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The pre-commit guards, as logic plus a CLI the hook calls.

## Users

no caller in the tracked tree

## CLI

```
guard.py autocrlf    # warn (never refuse) when core.autocrlf=true, which silently defeats eol=lf
guard.py eol         # EOL case: normalise a textish staged blob's CRs and re-stage it; refuse a binary
```
Flags: `--root`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: index -> index.

## Invariants and rules

* The two decisions are plain functions - `eol_case`, `autocrlf_warning` - so a test can call them directly (with the staged text or the staged paths handed in) without a shell and without first building a git command; the CLI is the thin layer that reads the staged paths from git and performs the fix.
* **The index blob, not the worktree.** A commit carries the *index*, so the EOL case reads the staged blob (`git cat-file -p :<path>`) rather than the file on disk: a CRLF blob in the index is what would land, whatever the worktree looks like.
* **Why this lives here and not in `prepcommit.py`.** `prepcommit.py` is a staging/commit-message CLI: it walks `git status`, classifies paths into stage/refuse and writes a message. The guards are a different concern and have to run for *any* commit, including a plain `git commit` that never calls prepcommit - so they live in a small importable module with no side effects on import, which both the hook and `guard_selftest.py` can call.

## Lib dependencies

git, text, repo.

## Test contract

Tier: fixture (temp repo).
Today's selftest (`tools/git/guard_selftest.py`): This is the one gate whose failure mode is silent and total - a rewritten `build.sha1` would make every later `ninja build/RMHE08/ok` meaningless - so it is tested rather than trusted.
Target: `tools/tests/git/test_guard.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `tools/git/hooks/pre-commit` used to be a shell script that could only *refuse* a bad commit. This module is the logic behind it now: it can say what a staged path needs, and its CLI does the fix.
