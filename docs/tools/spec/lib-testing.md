# `lib/testing` - The harness: one assertion shape, a fixture tree the lib accepts, a git fixture, and the tiers

## Purpose

The harness: one assertion shape, a fixture tree the lib accepts, a git fixture, and the tiers.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Checker`: `check(name, got, want)`, `expect`, `raises`, `contains`, `summary()` -> `ok - N checks` / `FAIL: ...`
* `FixtureTree(tmp)`: `configure.py`, `config/RMHE08/{symbols,splits}.txt`, `src/`, `include/`, `build/RMHE08/{obj,src}/` objects from `ElfBuilder`, `report.json`, optional asm dump; `add_unit`, `add_symbol`, `claim`; created outside the repository
* `GitFixture(tmp)`: `init`, `commit`, `branch`, `worktree`, `conflict`
* `TIER = 'fixture' | 'smoke'`; under `fixture`, `lib.repo.repo_root()` without `start=` raises

## Absorbs (today's implementations)

25 `check()` helpers, ~22 fixture builders, the delegation-pair logic of `selftest.py`

## Test contract

Tier: fixture (a lib test never reads the live tree). the harness tests itself: a fixture tree resolves as a tree; a live-tree read under `fixture` fails

## Known gaps

None until implemented; `migration.md` names the package.
