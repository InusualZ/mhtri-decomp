# `lib/lanes` - The lane model: naming, the claim registry, live sessions, the slot pool, seeding, rescue refs, one teardown, the launch line, the landing log

## Purpose

Everything a lane tool needs to name, claim, seed, launch, tear down and audit a lane, written once. The CLIs
(`claims`, `slots`, `lane`, `rescue`, `wtsafe`, `queue`, `lanecmd`, `worktreehook`, `landlog`) are thin layers over it;
none of them imports another lane tool for this any more (WP3e: claims->slots/rescue/wtsafe, worktreehook->slots,
lane->wtsafe and slots->lanecmd are gone). Import the module you need; `lanes/__init__.py` imports nothing.

## Users

`units/claims.py`, `units/slots.py`, `units/lane.py`, `units/rescue.py`, `units/wtsafe.py`, `units/queue.py`,
`units/lanecmd.py`, `units/worktreehook.py`, `units/landlog.py`, `units/brief.py` (via `briefing/`),
`units/backlog.py` (`launch.lane_call`), `units/recompile.py` (`seed.build_is_current`).

## Public API

* `naming`: `norm_unit`, `slug` (normalised first: `X` and `X.cpp` share one slug, so one branch - the lock),
  `branch_for` (`worker/<slug>`), `worktree_for` (`<repo>.ws-<slug>`), `slug_of_branch`, `lane_slug`,
  `is_lane_branch` (`LANE_PREFIXES`), `rescue_ref(unit)` / `rescue_ref_for_branch(branch)`, `ack_name`.
* `registry`: `.pi/claims.json` - `load`/`save`/`read` (`read` says whether the file parses: an unreadable registry
  makes the pool refuse rather than guess), `record` (either spelling), `key_of`, `record_holding` (a unit's own row,
  else a cluster row whose `units` lists it), `record_for_branch`, `claimed_units` (keys plus every cluster member),
  `claim_branch`, `claim_slug`, `handoff_slug`, `outbox_path`, `notes_path`, `ack_path`, `load_ack`; the git lock -
  `branch_exists`, `ref_exists`, `worker_branches`, `lock_held` (branch or throwaway worktree), `merged_into_main`
  (`git cherry`), `commits_ahead`, `worktree_paths`; `toplevel_of`/`main_of` (the caller's tree and its MAIN);
  `now`/`age_seconds`.
* `sessions`: the only live-lane signal - Claude Code's `<config dir>/sessions/<pid>.json`. `live_runs` (a record
  counts while its pid is alive; Windows asks for the exit code, never signals), `runs_in(path)` (cwd inside the
  path), `run_registries` (none = no signal, not "no lane"), `run_label`, `pid_alive`, `same_tree`.
* `pool`: the slot pool - `slot_dir` (`<repo>.slot<n>`), the manifest (`pool_size` is `pool.json`'s count, so a
  broken slot never hides a live one), lock records, the `.used` sentinel (`mark_used` is `O_EXCL`; `marker_info`
  names the owner, a legacy body names nobody), `slot_state`/`all_slots` (`free`, `no worktree`, `LIVE`, `claimed`,
  `branch`, `debris`), `capacity_error`, `verify`/`claim_currency` (the build tree proven current, pending ninja
  steps run at handover; `NINJA_RUNNER` is the test seam), `release_blockers` (a live session, a dirty tree, commits
  no branch reaches), `acquire`/`preview`/`release`/`reclaim_verdict`/`reclaim_slot` (a slot on a branch whose tree
  `merge-tree` proves applied is reclaimed, rescue ref first), `unlanded_reason`.
* `seed`: `seed_worktree_build` (toolchain and build tree by copy, `orig/` by copy below 64 MB else junctioned, the
  `tools/m2c` submodule, ninja state with `.ninja_deps` re-pointed via `ninja_deps_rewrite`, inputs aged behind the
  outputs), `build_is_current(build_root, input_root)` (the one staleness rule the seeder, the slot guard and
  `recompile` share).
* `rescue`: `make`, `delete`, `list_refs`, `classify`/`audit` (redundant / landed-with-drift / unlanded / unknown, from
  the ref's registration diff against its merge-base, else its touched `src/` paths), `verdict(main, ref)` (read at
  the teardown that creates the ref; prunes only `redundant`; never raises).
* `teardown`: the step list - `step`, `skip`, `done`, `run` (the first failure marks every later step "not attempted"),
  `public`; reparse points - `is_reparse_point`, `unlink_reparse_points`, `make_junction`; `remove_worktree` (unlink,
  then `git worktree remove --force`), `remove_worktree_checked` (also repairs a submodule and an untracked leftover);
  the `orig/` guard - `main_worktree`, `verify_orig`, `snapshot`.
* `launch`: `KIND_PROFILE`, `PROFILES`, `profile_for_kind` (an unknown kind is refused with the list), `lane_call`
  (`claude --agent ... -p`, cwd first, task single-quoted or staged under `.pi/lanes/` past `INLINE_LIMIT`),
  `resume_call`, `your_tree_lines`/`teardown_lines`/`tree_block` (the block every lane gets, one copy for the brief and
  `slots.py spawn`).
* `landlog`: `Attempt` (branch, outcome in `landed`/`refused`/`conflict`/`error`, seconds, refused row, conflicted
  paths, units, commit), `append(main, attempt)` (one line to `.pi/land-log.jsonl`), `read` (records plus the numbers
  of unreadable lines), `summary`.

## Invariants and rules

* One teardown. `claims.release` builds its plan from `teardown.step`/`skip` and runs it with `teardown.run`; a slot
  claim's teardown is `pool.release`; `claims.timeout` and `claims.expire` are `release --force`; `lane.teardown` uses
  the same junction-safe `remove_worktree` and the `orig/` snapshot pair.
* A worktree claim with a live session in its tree is refused before anything is touched (Windows will not delete a
  directory a process sits in); a slot claim is refused by `pool.release_blockers` on the same signal. The herdr pane
  probing this replaces is deleted (`retired.md`).
* The landing log hook WP4 adds to `land.py land` (three lines, around the existing landing call):
  `t0 = time.time()`, then after the outcome is known
  `landlog.append(main, landlog.Attempt(branch, outcome, time.time() - t0, refused_row=row, conflicts=tuple(paths), units=tuple(units), commit=sha))`.

## Test contract

Tier: fixture. `tools/tests/lib/test_lanes.py` (naming, registry incl. cluster rows, the git lock, sessions with a
live and a dead pid, launch, the step list, junction-safe removal, rescue verdicts, the deps-log rewrite and the
staleness guard, the sentinel, the landing log). The tools' own tests cover the flows: `claims.py --selftest`,
`slots.py --selftest` (the pool on a real repository), `worktreehook.py --selftest`, `tools/tests/units/test_lane.py`,
`test_rescue.py`, `test_wtsafe.py` (smoke: MAIN's `orig/`), `test_lanecmd.py`, `test_landlog.py`.

## Known gaps

`pool.release` takes the session-registry directory as a parameter named `registry` (the slot CLI's historical
spelling), so inside `pool` the claim-registry module is reached as `registry_mod` in that function.
