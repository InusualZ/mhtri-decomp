# `lib/testing` - The harness: one assertion shape, a fixture tree the lib accepts, a git fixture, and the tiers

## Purpose

The harness: one assertion shape, a fixture tree the lib accepts, a git fixture, and the tiers.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Checker`: `check(name, got, want)`, `expect`, `raises`, `contains`, `summary()` -> `ok - N checks` / `FAIL: ...`
* `FixtureTree(tmp)`: `configure.py`, `config/RMHE08/{symbols,splits}.txt`, `src/`, `include/`, `build/RMHE08/{obj,src}/` objects from `ElfBuilder`, `report.json`, optional asm dump; `add_unit`, `add_symbol`, `claim`; created outside the repository
* `GitFixture(tmp)`: `init`, `commit`, `branch`, `worktree`, `conflict`
* `json_indent(text)`, `rewrite_json(path, data)`: write an allow-list back in its own shape (indent unit, raw
  non-ASCII kept), so `test_prologue.py --prune` / `test_layering.py --prune` are minimal diffs (WP4)
* `temp_dir(prefix)` (a `with` block), `rmtree_retry(path)` and `isolate_live_state()`: a temp directory whose cleanup
  survives Windows holding a file (read-only bit cleared, growing backoff, never raises), and `CLAUDE_CONFIG_DIR`
  pointed at an empty directory so a slot guard never sees the live `~/.claude/sessions` (WP6: from `unitutil`)
* `TIER = 'fixture' | 'smoke'`; under `fixture`, `lib.repo.repo_root()` without `start=` raises
* `assert_live_allowed(what)`, `assert_path_allowed(path, what)` (raises only for a path inside the live tree): the seams
  `lib.repo` calls; `refusals_expected()` - a `with` block whose refusals are the point of a test OF the guard, so they do
  not fail the run

## Absorbs (today's implementations)

25 `check()` helpers, ~22 fixture builders, the delegation-pair logic of `selftest.py`

## Test contract

Tier: fixture (a lib test never reads the live tree). the harness tests itself: a fixture tree resolves as a tree; a live-tree read under `fixture` fails

## Known gaps

Implemented in WP0 (`docs/tools/wp0-report.md`). Open:

* `FixtureTree.add_object` takes raw bytes or a `lib.binary.build.ElfBuilder` (WP1b).
* The audit-hook guard cannot see `os.stat`/`os.path.exists` or anything a module does at import time (before `run()`).
  WP1a closed the part of this that matters: `lib.repo` is the choke point (every root it resolves or is handed goes
  through `assert_path_allowed`, and `repo_root()` without `start=` raises), so importing a tool never resolves the
  live tree. A tool with its own module-level `ROOT = dirname(...)` constant is still
  invisible until its package removes the constant (`lib-repo.md`, Known gaps).
* On Windows the `subprocess.Popen` audit event carries the command line as one string; WP0's argv check treated it as one
  path and so never refused a live path given to a child. The hook now splits it back into tokens (WP1a).
* `GitFixture.conflict(path, a, b, base, names)` takes the file and both texts, not two branches.
