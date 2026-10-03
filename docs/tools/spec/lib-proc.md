# `lib/proc` - Subprocess with the codec rule: every text run decodes UTF-8 with replacement; spawn retry on Windows; process-tree kill

## Purpose

Subprocess with the codec rule: every text run decodes UTF-8 with replacement; spawn retry on Windows; process-tree kill.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `run(args, cwd=None, check=False, input=None, timeout=None) -> Completed` (text, `encoding='utf-8'`, `errors='replace'`)
* `run_bytes(...)`; `kill_tree(proc)`; `install_spawn_retry()` (no-op off Windows, idempotent)
* `trap_sites(root)`: the AST scan for `text=True` without `encoding=` (a lib test, not a tool)

## Absorbs (today's implementations)

`subproc.run/trap_sites`, `spawnretry.install`, `selftest._kill_tree`, `land.run`

## Test contract

Tier: fixture (a lib test never reads the live tree). a non-ASCII byte in output never raises; a `WinError 5` at launch is retried then raised; `trap_sites` over `tools/` is empty

## Known gaps

None until implemented; `migration.md` names the package.
