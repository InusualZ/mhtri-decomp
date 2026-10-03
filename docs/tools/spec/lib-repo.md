# `lib/repo` - Trees and paths: the invocation's tree, its MAIN, the build/orig/asm/scratch/state locations, the ground truth

## Purpose

Trees and paths: the invocation's tree, its MAIN, the build/orig/asm/scratch/state locations, the ground truth.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Tree`: `root`, `main`, `is_worktree`, `is_slot`, `build(version)`, `obj_dir`, `src_obj_dir`, `asm_dir`, `orig_dol`, `config_yml`, `report_json`, `objdiff_json`
* `repo_root(start=None)`: the invocation's tree wins (its git worktree, else a `cwd` that is a tree); `start=` roots a fixture; never this file's directory
* `main_tree(tree)` via `git worktree list --porcelain`; `resolve_input(rel, tree)` worktree first, MAIN fallback
* `scratch(tool)` -> `build/tmp/<tool>/`; `session_tmpdir()` unique and removed at exit; `state(name)` -> `.pi/<name>` (the one list of state files)
* `ground_truth(tree)`: the DOL path and the hashes pinned in `config.yml`; `verify_ground_truth(tree)`

## Absorbs (today's implementations)

`unitutil.repo_root/main_tree/resolve_input/session_tmpdir`, `recompile.main_root`, `splitcheck.main_root`, `wtsafe.ground_truth`, 20 root resolvers

## Test contract

Tier: fixture (a lib test never reads the live tree). a fixture tree is a tree; `cwd` outside any tree falls back to the packaged copy; `main_tree` of a worktree is MAIN; under `TIER=fixture` a call without `start=` raises

## Known gaps

None until implemented; `migration.md` names the package.
