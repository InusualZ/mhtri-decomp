# `unionguard` - Refuse a union of a conflicted apply unless every conflict is a disjoint addition (empty diff3 base, no delete/rename); undoes the apply on refusal

<!-- generated from the module docstring of `tools/units/unionguard.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Refuse to union-resolve a conflicted branch-apply when the conflict is not a disjoint addition.

## Users

the landing gate (1); imported by `land`

## CLI

```
python tools/units/unionguard.py --branch worker/<slug> [--base <ref>] [path ...]
python tools/units/unionguard.py --branch worker/<slug> --no-cleanup [--base <ref>] [path ...]
python tools/units/unionguard.py --selftest
```
Flags: `--base`, `--branch`, `--cwd`, `--no-cleanup`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: index stages -> verdict, cleanup.

## Invariants and rules

* `land.py land --branch` lands a worker branch as `git apply -3 <merge-base diff>` and then resolves the conflicted registration paths with a plain **union** (`unionresolve`: ours block, then theirs block). That union is only correct when the two sides touched *disjoint* things - two units each appending a registration at the same anchor. When the two sides changed the *same* existing content it silently writes a tree that cannot build, and the damage is only found later by `ninja`:
* The signal that separates the two is the **base section** of each conflict hunk, which `git merge-file --diff3` makes explicit (this tool recomputes the three-way merge from the index's stage blobs, so it does not depend on the working tree's conflict-marker style):
* base section **empty** -> both sides *inserted* new lines at the same anchor. Disjoint addition. Union.
* base section **non-empty** -> one or both sides changed/removed existing lines. Overlap. Refuse.
* a stage 1 (base) entry with **no stage 2/3** -> that side *deleted* the path. Refuse.
* a rename in either side's diff that touches a conflicted path -> Refuse (the deleted half would be resurrected).
* For every conflicted path the tool prints what each side did (deleted / renamed / modified) and exits **non-zero** with the offending paths named, so the landing can stop *before* it stages a tree that cannot build. A tree whose conflicts are all disjoint additions still unions exactly as before.
* **A refusal also undoes the apply.** `land.py` runs this guard *after* `git apply -3`, which has already merged into the index: UU/AA entries for the conflicts and cleanly-applied hunks staged. Refusing and then only printing "resolve these by hand" left `main` mid-conflict - staged files, conflict stages, and a `ninja` that cannot regenerate `build.ninja` - until somebody cleaned it up by hand. So on refusal the guard restores every path the branch's diff touched (`git checkout HEAD -- <path>` when HEAD has it, a delete when it does not) and runs `git reset` to drop the index stages, printing exactly what it restored and anything still dirty. `--no-cleanup` keeps the old behaviour for a deliberate inspection.
* `land.py` passes the branch and the conflicted paths; with no paths it inspects `git ls-files -u`.

## Lib dependencies

git, merge.

## Test contract

Tier: fixture (real git repos).
Today's selftest (`tools/units/unionguard_selftest.py`): The guard exists because a plain ours-then-theirs union is only safe for **disjoint additions**; on a delete, a rename, or both sides editing the same region it silently writes a tree that cannot build (measured 2026-09-25: `configure.py` registered both halves of a rename; ten shared g3d headers unioned into "illegal function overloading"). The cases below are the contract: every unsafe case must be refused, and the disjoint union must still pass through and union. A refusal must also undo the `git apply -3` that `land.py` has already run - clean tree, no UU/AA, nothing staged - while `--no-cleanup` keeps the conflicted index for inspection. Every case builds a real git repo and a real unmerged index - no repository state, no build, no mocks.
Target: `tools/tests/units/test_unionguard.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

none (the old landing script is deleted, questions.md 2)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Deletes and renames (measured 2026-09-25, commit `6fceb8f7`).** A `.c -> .cpp` rename made the branch's copy of `configure.py` still carry the old `g3d/g3d_resanmcamera.c` registration while main had already dropped it. The conflict's base side was non-empty; the union wrote ours-then-theirs and kept the stale `.c` line, so configure.py registered both `g3d_resanmcamera.cpp` **and** `g3d_resanmcamera.c`, one of which does not exist. (A rename arrives as delete+add, so the union resurrects the deleted half.)
* **Both sides editing one file (measured 2026-09-25, worker/80093990).** Ten shared `g3d` headers had moved on in main; the union kept both sides' declarations and 12 objects failed to compile ("illegal function overloading").
