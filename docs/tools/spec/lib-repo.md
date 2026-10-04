# `lib/repo` - Trees and paths: the invocation's tree, its MAIN, the build/orig/asm/scratch/state locations, the ground truth

## Purpose

Says which tree a tool reads, where MAIN is, where every build/orig/asm/scratch/state path sits in a tree, and what the pinned
originals hash to. It is also the choke point of the fixture tier's live-tree refusal for what the audit hook cannot see.

## Users

`unitutil` (and through it its 40 importers), `symdiff`, `wtsafe`; every tool once WP3 removes its private resolver.

## Public API

* `repo_root(start=None) -> str`: walk up from `start` to `configure.py`. Without `start` the invocation's tree wins (its git
  worktree, else a `cwd` that is itself a tree), and only a `cwd` that is no tree falls back to the packaged copy
  (`PACKAGED_ROOT`, the tree this file sits in). `SystemExit` when no tree is found.
* `caller_worktree(start=None)`, `cwd_tree()`: the two halves of that rule.
* `main_checkout(root) -> str`: MAIN for a tool that must have one - `main_tree(root)` when it holds `configure.py`,
  else the first `git worktree list` entry, else `root` (`recompile.main_root`, `measure.py`).
* `main_tree(root, honour_env=False) -> str | None`: the parent of `git rev-parse --git-common-dir`; `honour_env` lets a
  `$MHTRI_MAIN` that names a directory override it (the tree the tools serve, never a fixture). None outside git.
* `resolve_input(rel, root, probe=os.path.exists, honour_env=False) -> str`: `rel` under `root`, else under MAIN by path,
  else `root`'s own path (so the error names it). Read-only inputs only.
* `session_tmpdir()`: one unique directory per process under the system temp, removed at exit.
* `scratch(tool, root) -> build/tmp/<tool>/` (created); `state(name, root) -> .pi/<name>` for a name on `STATE_NAMES`.
* `ground_truth(root) -> {rel path: sha1}` from `config.yml`'s `hash`/`selfile_hash`; `verify_ground_truth(root)`.
* `config_change(old, new) -> {ok, reason, changed}`: may this `config.yml` change be committed - only when every
  differing top-level key is in `CONFIG_MUTABLE_KEYS` (`block_relocations`, `add_relocations`; owner ruling
  2026-10-03); `config_blocks(text) -> ({key: lines}, problems)` is its stdlib-only reader (comments and blank lines
  dropped; a repeated key or an unplaceable line is a problem, and a problem refuses). `CONFIG_PATH` is the file's
  repo-relative path. Callers: `guard.py config` (the pre-commit hook) and `prepcommit.classify`.
* `Tree(root)` (frozen): `main`, `is_worktree`, `is_slot`, `build(version)`, `obj_dir`, `src_obj_dir`, `asm_dir`,
  `report_json`, `orig_dol`, `config_yml`, `objdiff_json`, `input(rel, probe)`; `Tree.find(start)`.
* `guard(path, what)`: the choke point (below).

## Invariants and rules

* **The invocation's tree wins.** A tool run as `python <MAIN>/tools/...` from a worktree reads the worktree: reading the
  file's own tree printed MAIN's numbers for a worktree's object (0.917 for a symbol at 100.0, 2026-09).
* **A fixture names its tree.** A fixture is not a git worktree, so without `start` the walk once began at this file's
  directory and scored the real build from a fixture (`unitscore_selftest`); now `cwd` is tried, and `start=` always roots.
* **MAIN is the git common dir's parent, not the first `git worktree list` row** (that order is registration order).
* **The live-tree choke point (WP0 gaps 1-2).** Under the fixture tier, `repo_root()` without `start` raises
  `LiveTreeError`, and every function here that is handed or resolves a root inside the live repository raises too
  (`testing.assert_path_allowed`): this is what refuses an `os.stat`/`os.path.exists` probe and a root bound at import time
  and used later, neither of which the audit hook sees.
* **Import-time roots are lazy.** A module must not resolve the live tree at import (a fixture test importing it would then
  fail before it ran). `unitutil.ROOT` / `unitutil.OBJDIFF` resolve on first use (module `__getattr__`); assigning them
  still pins them, which is how the older selftests point a tool at a fixture.

## Absorbs (today's implementations)

`unitutil.repo_root/caller_worktree/_cwd_tree/main_tree/resolve_input/session_tmpdir`, `symdiff.session_tmpdir`,
`wtsafe.ground_truth`. `splitcheck.main_root` already went through `unitutil`.

## Lib dependencies

`lib.git` (the two git questions), `lib.testing` (the tier and the live-tree refusal).

## Test contract

Tier: fixture (`tools/tests/lib/test_repo.py`). A fixture tree is a tree; `cwd` outside any tree falls back to the packaged
copy (`PACKAGED_ROOT` pointed at a fixture); `main_tree` of a worktree is MAIN; `$MHTRI_MAIN` only when honoured; the input
fallback; under the fixture tier a call without `start=`, a `Tree`/`resolve_input`/`main_tree`/`scratch` over the live
root, and a process given a live path all raise.

## Known gaps

* 19 private resolvers outside `unitutil` remain (`recompile.worktree_root/main_root` - which also falls back to the first
  worktree row -, `infer`, `methodize`, `guard`, `commitlint`, `sync_*`, `backlog`, `rescue`, `slots`, `worktreehook`,
  `wtsafe.main_worktree`, `langcheck`, `unwindcut`) and the module-level `ROOT = dirname(...)` constants of `callers`,
  `tudiscover`, `dataorder`, `symbolpreflight`, `ledger`, `flipcheck`, `datagap`, `selftest`, `accessextent`: each goes
  with its family's WP3 package, because removing a module-level `ROOT` is a fixture rewrite of that tool's tests.
* `state()` is not called yet: the 40 `.pi/` spellings move with WP3e (lanes).
