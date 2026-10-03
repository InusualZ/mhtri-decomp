# `wtsafe` - Remove a worktree after unlinking its junctions (never walk a reparse point) and verify `orig/` against the pinned hashes

<!-- generated from the module docstring of `tools/units/wtsafe.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Remove a worktree without letting a junction reach outside it, and prove `orig/` survived.

## Users

the landing gate (2); imported by `claims`, `lane`

## CLI

```
python tools/units/wtsafe.py --check      # verify orig/ against its pinned hashes
python tools/units/wtsafe.py --selftest
```
Flags: `--check`, `--selftest`, `--unlink`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: worktree -> removed; config.yml hashes -> verdict.

## Invariants and rules

* A junction is only ever safe to **unlink**, never to walk through. So every worktree removal in this repo goes through `remove_worktree()` here, which unlinks the reparse points first.
* Second line of defence: `verify_orig()` recomputes the pinned hashes of the original files (read from `config/RMHE08/config.yml`, never hard-coded) so a loss is loud instead of silent. The caller pairs it with a snapshot taken before the removal.
* Measured, not assumed (the tool's selftest asserts both): `git worktree remove --force` **does** destroy a junction's target, while Python's `shutil.rmtree` unlinks reparse points and leaves the target alone.

## Lib dependencies

lanes.teardown, repo (ground truth).

## Test contract

Tier: fixture (junction round trip on Windows).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_wtsafe.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this exists (2026-09-24). A lane teardown ran `git worktree remove --force` against a worktree that carried junctions into MAIN's tree - documented practice here (`.pi/notes/pl-act-09c6.md` junctioned `build/tools` and `build/RMHE08/obj`). A Windows directory junction is a **reparse point**, and git's worktree removal - like Python's `shutil.rmtree` - sees it as an ordinary directory and recurses through it, so removing the worktree deleted the *target's* contents: MAIN's `orig/RMHE08/sys` and `orig/RMHE08/files` were emptied. The damage pattern is the fingerprint: only the two junctioned directories lost their contents, while the plain files beside them survived.
