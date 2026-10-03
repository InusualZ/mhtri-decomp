# `lane` - Teardown for a non-claim lane (`experiment/*`): rescue unlanded commits to `refs/rescue/<slug>`, then remove worktree and branch; idempotent

<!-- generated from the module docstring of `tools/units/lane.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Teardown for a lane that is **not** a claim: rescue its unlanded commits, then remove it.

## Users

no caller in the tracked tree

## CLI

```
python tools/units/lane.py list                      # every lane, its unlanded commits, its rescue ref
python tools/units/lane.py teardown <branch>          # rescue, then remove the worktree and the branch
python tools/units/lane.py teardown <branch> --dry-run
python tools/units/lane.py --selftest
```
Subcommands: `list`, `teardown`.
Flags: `--base`, `--dry-run`, `--force`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: git -> refs/rescue.

## Invariants and rules

* **The order is the whole point**: every commit in `main..<branch>` is copied to `refs/rescue/<slug>` *before* the branch is deleted, and a lane with nothing unlanded says so instead of quietly making an empty ref. A rescue ref is never deleted, by anything (docs/plan.md, "A branch is never the only copy of work"), so a teardown is always reversible with:
```
git branch <branch> refs/rescue/<slug>
```
* `teardown` refuses to touch a branch that is not a lane prefix (`experiment/`, `worker/`, `wip/`) unless `--force`, so a mistyped branch name cannot delete `main`.
* A teardown is also **idempotent**: a lane whose worktree and branch are both already gone reports `nothing to tear down` and exits 0, and a branch left behind by a half-finished run is still deleted, so repeating the command *completes* the teardown instead of aborting. The `orig/` integrity guard is read from MAIN and snapshotted **before** the worktree is removed - a worker worktree has no `orig/` or `config.yml` of its own, and reading the worktree that is being deleted, after it is gone, is the bug this tool now avoids.

## Lib dependencies

lanes.rescue, lanes.teardown, git.

## Test contract

Tier: fixture (GitFixture).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_lane.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `claims.py release` is the teardown for a *worker* - it owns the branch, the worktree and the registry entry, and it makes a `refs/rescue/<slug>` ref before deleting anything. An **orchestrator's own lane** (the `experiment/*` branches a fan-out uses) has no claim, so `claims.py` does not apply to it, and the orchestrator has to do the same three steps by hand. Doing that by hand is how a verified commit was nearly lost on 2026-09-24: `git worktree remove --force` + `git branch -D` deleted `experiment/wP-promote`, whose only commit held 17 symbol renames - it survived solely because git had not yet pruned the object. This tool makes that mistake impossible.
