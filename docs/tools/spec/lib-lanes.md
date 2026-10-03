# `lib/lanes` - The lane model: naming, the registry, rescue refs, one teardown, the launch line

## Purpose

The lane model: naming, the registry, rescue refs, one teardown, the launch line.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `naming.slug(unit)`, `branch_for`, `worktree_for`, `norm_unit`; `registry`: `.pi/claims.json` records, slot files, locks, the `.used` marker, `live_runs()` from `~/.claude/sessions`
* `rescue.ref(branch)`, `make(branch, base)`, `verdict(ref)`; `teardown.run(steps)`: rescue -> unlink reparse points -> worktree remove -> branch delete -> prune -> registry, each step reported, idempotent
* `launch.lane_call(agent, cwd, task, ...)`, `resume_call`, `profile_for_kind`

## Absorbs (today's implementations)

`claims.slug/branch_for/worktree_for/norm_unit/release/rescue_verdict/_seed_*`, `slots.*state/markers/locks/live_runs/release/reclaim`, `lane.rescue/teardown`, `rescue.classify_ref`, `wtsafe.remove_worktree/unlink_reparse_points`, `lanecmd`, `queue._profile_for_kind`

## Test contract

Tier: fixture (a lib test never reads the live tree). the GitFixture rounds of the claims, slots, lane and rescue selftests; the junction round trip

## Known gaps

None until implemented; `migration.md` names the package.
