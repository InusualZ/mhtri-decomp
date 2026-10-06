# `slots` - The fixed pool of reusable lane directories: init/acquire/spawn/release/reclaim/status/verify/shadow/collect, with the `.used` owner sentinel and live-session detection

<!-- generated from the module docstring of `tools/units/slots.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

A fixed pool of reusable lane directories ("slots") - the branch is still the claim.

## Users

profiles (`.claude/agents`) (4); CLAUDE.md (4); docs (16); imports `lib.lanes.pool` (acquire, release, reclaim, verify, the sentinel and slot state live there since WP3e; `claims` and `worktreehook` call the lib, not this tool)

## CLI

```
python tools/units/slots.py init [--count 6] [--force]
python tools/units/slots.py acquire <unit> [--slot N] [--worker NAME] [--force] [--dry-run]
python tools/units/slots.py spawn --kind KIND [--slot N] [--unit U] [--task-file PATH] [--units A,B] [--json]
python tools/units/slots.py spawn --kind recon --group NAME [--units A,B] [--slot N]   # the standard recon brief
python tools/units/slots.py release [--slot N | --unit U | --branch B] [--keep-branch] [--force] [--dry-run]
python tools/units/slots.py reclaim [--slot N | --unit U | --branch B] [--json]
python tools/units/slots.py status [--json]
python tools/units/slots.py refresh N [--force] [--json]   # copy only what differs from MAIN, then re-verify
python tools/units/slots.py verify [--slot N] [--json]
python tools/units/slots.py shadow <slot> <dir> [--seed-build] [--force]
python tools/units/slots.py --selftest
```
Subcommands: `init`, `acquire`, `release`, `reclaim`, `spawn`, `collect`, `status`, `refresh`, `verify`, `shadow`.
Flags: `--branch`, `--count`, `--dry-run`, `--force`, `--group`, `--json`, `--keep-branch`, `--kind`, `--main`, `--path`, `--release`, `--seed-build`, `--selftest`, `--slot`, `--task-file`, `--unit`, `--units`, `--worker`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: <repo>.slotN, .pi/slots, ~/.claude/sessions -> state.

## Invariants and rules

```
git worktree add -b worker/<slug> <sibling>.ws-<slug> main
git submodule update --init tools/m2c
(then seed build/, orig/ and build/RMHE08 from MAIN)
```
* This replaces it with a fixed pool of `N` reusable slots at **stable paths** - `<repo>.slot1` ... `<repo>.slot6` - so each keeps a warm `build/RMHE08`, `orig/`, the toolchain and its initialised `tools/m2c` submodule across rounds. A stable path is the point: a depfile's absolute paths stay valid across a reset, so the per-seed rewrite becomes a one-time cost per slot.
* **A slot holds a directory, never a branch.** `acquire` always creates a *fresh* branch off main's current tip (`checkout -B worker/<slug> <tip>`), after asserting the slot carried no branch over from a previous round - a slot whose previous branch has not landed is a bug, not a state to reuse. One branch per claim, named for the unit, landable and auditable on its own: the invariant the whole protocol rests on (docs/plan.md 5.1).
* **One lane per slot, enforced at both ends.** A slot's claim was released while another lane was still working in it, and the release detached HEAD under a live process; the same hole let a second lane acquire a slot that still held the first lane's branch. The sentinel and the release path are now the two ends of one rule:
* the `.used` sentinel records its **OWNER label** (`mark_used`, `marker_info`), and `status` prints it, so "someone holds this" is never the only thing a reader can know;
* `acquire` refuses a slot whose sentinel **names a different owner** instead of silently reclaiming it (`--force` is the deliberate override);
* `release` **fails closed** on three signals - a RUNNING Claude session whose cwd resolves into the slot, a dirty tree, and commits on HEAD that no branch reaches - printing every reason and offering `--force`. The session registry under the Claude config dir is the *only* live-lane signal available: a lock file records what *this tool* did, and a lane is a process the tool never launched, so `release_blockers` reads the harness' own run records rather than inferring liveness from anything it wrote itself.
* The reset is **verified and fail closed** - a reused slot carries the previous round's build state, and a stale build tree is the most expensive failure this campaign has hit (a refused landing's split objects left in a tree made the next gate report "no unit's split target object moved" for an untouched unit, costing a full `rm -rf build/RMHE08` rebuild):
* **one `.used` marker per slot** in the worktree root, created by `acquire` (atomically, `O_EXCL`) and removed by `release`; `status` reports `used`/`free` from it. The marker is the primary occupancy signal - it only works because the launch pattern is now structural (acquire first, launch with the slot as `cwd`), so a lane always enters its slot. A JSON lock record at `MAIN/.pi/slots/<n>.json` still names the claim, worker and time, and a stale one is reclaimed instead of wedging forever.
* **`status` reads the worktree too, and keeps the two readings distinct.** A slot whose worktree still has a branch checked out is **in use**, whatever the marker or the JSON record say - the lock is a convenience, the worktree is the truth. The two disagreed in the wild (slot 2 was reported `free` while it held `worker/rule10-fix-14f8`), and that disagreement *was* the bug, so both readings are kept.
* **`acquire` falls through.** An occupied slot is skipped, not failed on: the search takes the next genuinely free slot and marks it atomically, so two racing acquires cannot both take one. An explicitly targeted `--slot N` still refuses when it holds an unlanded branch (never reuse a branch).
* **A spawned lane's unit set rides its lock.** `spawn --units A,B` (else the task's first `Units:` line, via
  `lib.lanes.launch.task_units`) is resolved against `splits.txt` (`launch.resolve_units`: a path as given, a bare stem
  only when exactly one registered unit has it - the rest are printed as NOT recorded) and written to the lock as
  `units`; `claims.py list --json` reads it back, so `integrate.py` defers a request whose owner a live lane holds. The
  slot's `release` clears it with the lock.
* **`--kind recon` writes its own brief.** `recon` is the read-only survey a wave starts with (`docs/pipeline.md`
  section 14), launched as the read-only global `scout`. With no `--task-file`, `spawn --kind recon --group NAME
  [--units A,B]` generates the task from `lib.lanes.recon.recon_brief` - the six recon questions with the group filled
  in, the units on a `Units:` line (recorded on the lock like any unit set) and the no-write rule; a recon with no
  group is refused, and `--group` on any other kind is refused before a slot is taken.
* **`collect`** copies a finished lane's `.pi/outbox/*.json` and `.pi/notes/*.md` into `MAIN/.pi/` (MAIN's copy kept
  when at least as new), merges its data requests and reports its branch, the commits main lacks and the dirt
  (`lib.lanes.pool.unlanded_reason_at`). `--slot N` or `--path` naming a slot as before; `--path` may also name a
  **plain worktree** of the repository (`git worktree list`, MAIN excluded - an Agent-tool lane under
  `.claude/worktrees/`), which was refused ("no slot resolves") until 2026-10-05; `--release` is refused for it, its
  teardown being `git worktree remove`. Selftest: the plain worktree's evidence, branch, commit and unlanded verdict,
  the report line, the `--release` refusal and MAIN refused (373 checks; dropping the plain path, the release refusal
  or the MAIN exclusion each fails).
* **reset** = `checkout -f --detach <main-tip>`, a selective `git clean` that keeps `build/`, `orig/`, the toolchain and `tools/m2c` but discards scratch, stale source edits and stray files, then `checkout -B worker/<slug> <main-tip>`.
* **validate** the kept build tree against MAIN's current map/DOL with the same staleness guard `seed_worktree_build` already uses (`claims._build_is_current` - `config.json` vs `symbols.txt`/`splits.txt`/`main.dol`), a byte comparison of `build/RMHE08/report.json`, AND the **compile-output set** the official scorer opens (`objdiff.json`'s `target_path`/`base_path`, existence - `compile_outputs`). A slot can pass the first two while its `obj/`/`src/` objects are gone; that tree cannot run `objdiff report generate`, so it is refused too. **If it cannot be proven current, re-seed; never proceed on a doubt.**
* **claim-time currency**: a freshly seeded slot is *always* a few `ninja` steps behind by construction (the seed copies MAIN's `build/` with MAIN's mtimes while `git worktree add` stamps the slot's own sources at checkout time), so "no work to do" is the wrong test for a handover. `acquire` runs `ninja -n`, **finishes the pending steps in the slot** (`claim_currency`, bounded - measured 3: one MWCC unit, REPORT, PROGRESS), re-counts, and prints the proof the lane can see: `report.json` byte-identical, the compile-output set, the pending count. A ninja that cannot run is reported as *unknown*, never silently as 0.
* **release** returns the slot to main's tip, keeps the warm trees, deletes the branch (rescue-ref first, as `claims.release` does) and refreshes the build tree so the next acquire is instant.
* **reclaim** turns "the slot still holds a **landed** branch" from a hand dance into one step. A branch whose content is already in main is not work in progress, so `spawn`/`acquire` test it with the campaign's own free test (`git merge-tree --write-tree main <branch>` vs `git rev-parse main^{tree}` - equal means fully applied, the same test the held-branch audit uses) and, when it is applied, park the rescue ref (`refs/rescue/<slug>`) **first**, then detach the worktree, delete the branch, release the slot and take it - printing one line saying what it did and why. The test compares **trees, not commits**, so the one rule covers both routes: a gate-landed branch stays ahead of main by its own commits but its tree equals main's, and the direct path-limited landing does the same. An **unlanded** branch keeps today's refusal verbatim, and a test that cannot run or is ambiguous **refuses** (fail closed) rather than reclaim: the refusal is load-bearing. The marker is not the signal: a slot whose `.used` is **MISSING** but whose branch is proven applied is reclaimable (a crash remnant), while the same missing marker next to an unlanded branch still refuses. `python tools/units/slots.py reclaim [--slot N | --unit U | --branch B]` runs the same step by name so the orchestrator (or the next lane) can do it deliberately instead of by hand.
* **the cap is `pool.json`'s `count`** (`pool_size`), **not** a count of slot directories: a slot whose worktree is missing or broken is still a slot. `status`/`capacity_error` enumerate every one and say per slot *why* it is not usable - `no worktree` / `branch` / `claimed` / `debris`, plus a `missing record` or `stale record: its base predates main` note with the remedy - so a full pool can never read as a phantom shortage. A bulk `init --force` **refuses while any slot holds live work** (the pool is the campaign's concurrency cap, not a scratch file), and `tools/selftest.py` guards `.pi/slots/pool.json` **by bytes** (`.pi/` is gitignored) so no test run can shrink it.

* **refresh** (`lib.lanes.pool.refresh_slot`, 2026-10-04) is the registry's `slot-build` refresh (`lib.artifacts`): a slot that verifies is left alone (`--force` re-seeds anyway); otherwise `seed_worktree_build(..., overwrite=True)` recopies only the files whose size or mtime differ from MAIN's, then `verify` runs again and the exit is its verdict. It refuses a slot a RUNNING session works in (a build tree changed under a lane changes what it measures; `--force` overrides) and a MAIN whose own build is stale. Measured on MAIN's tree (892 files, 42 MB, the asm dump excluded): a full seed 0.71 s, a refresh with nothing changed 0.16 s, with five objects changed 0.16 s (5 copied).

## Lib dependencies

lanes, git, repo, text, proc.

## Test contract

Tier: fixture (GitFixture).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_slots.py` on `lib.testing`; the pool's unit checks are in `tools/tests/lib/test_lanes.py`. The in-file selftest drives the lib through this module's re-exported names (378 checks, the recon spawn included; the ninja seam is `lib.lanes.pool.NINJA_RUNNER`).

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `claims.py` used to *construct* a lane's environment per round:
* That costs per lane, its teardown is destructive (`git worktree remove` cannot handle the `tools/m2c` submodule, so the documented path is `rm -rf` + `prune` + `branch -D` - and a careless teardown destroyed a lane's branch on 2026-09-24), and the seeder has to rewrite `.ninja_deps`' **absolute** header paths on every seed precisely because each worktree path is new.
