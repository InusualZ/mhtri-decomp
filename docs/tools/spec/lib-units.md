# `lib/units` - The unit value type (every spelling), its paths, its real compile command, and a fresh compile

## Purpose

The unit value type (every spelling), its paths, its real compile command, and a fresh compile.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Unit.resolve(spec, tree)`: `key`, `source` (extension inferred from `src/`, MAIN fallback), `obj_target`, `obj_ours`, `report_name`, `splits_key`, `lib`, `flag`, `slug`, `module`, `language`; `Unit.list(tree)`
* `compile_command(unit, tree) -> tokens` (MAIN's ninja line, worktree `-i` first), `compile(unit, tree, dry_run) -> CompileResult` (object deleted first; mtime must move), `proposal_target(unit, symbol, tree)`

## Absorbs (today's implementations)

`unitutil.resolve_unit/compile_command/run_compile`, `recompile.unit_tokens/rewrite/order_includes/compile_unit/proposal_target`, the 30 name functions

## Test contract

Tier: fixture (a lib test never reads the live tree). every spelling in `duplication.md` (g) resolves to one `Unit`; the `recompile_selftest` include-order and proposal rows

## Known gaps

None until implemented; `migration.md` names the package.
