# `lib/units` - The unit value type (every spelling), its paths, its real compile command, and a fresh compile

## Purpose

Turns any spelling of a translation unit into one key and one `Unit`, gives its paths in a tree, builds the exact compile
command the build would run (from MAIN's ninja, the worktree's, or a same-lib sibling's), compiles it into the worktree with a
fresh-object proof, and finds the original object to score against.

## Users

`unitutil.resolve_unit/list_units/compile_command/run_compile/unquote` (delegates), `recompile`, `measure`, `verifyunit`,
`ledger`, `unitscore`.

## Public API

* Spellings: `stem(spec)` (`Pl/pl_act` from `Pl/pl_act`, `Pl/pl_act.cpp`, `src/...`, `./src/...`, `main/...`,
  `build/RMHE08/{src,obj}/....o`, backslashes, stray slashes), `normalize(spec)` (a path from `src/`, extension kept),
  `report_name(spec)` (`main/<stem>`), `with_ext`, `has_ext`, `obj_rel(spec, side)`, `target_rel(spec)`,
  `source_path(tree, spec)`, `source_spelling(spec, roots, source=None)`, `same_tree(a, b)`.
* `Unit(key, ext, root, version)` (frozen): `module`, `file`, `spelling` (= `splits_key`), `report_name`, `source`,
  `obj_dir`, `obj_ours`, `obj_target`, `language`; `registration` / `lib` / `flag` from `configure.py` (read on first use);
  `Unit.make(key, root)`, `Unit.list(root)`, `Unit.resolve(spec, root)`; `versions(root)`.
* The command: `unquote(tokens)`, `ninja_target(spec)`, `ninja_lines(tree, target, runner)`, `ninja_command(tree, spec)`,
  `lib_block(tree, spec)`, `sibling_for(main, wt, spec)`, `retarget(tokens, spec)`, `unit_tokens(main, wt, spec) ->
  (tokens, source)`, `rewrite(tokens, spec, main, wt) -> (tokens, object)`, `order_includes`, `include_pairs`,
  `retarget_object_helpers`, `absolutize`, `is_switch`.
* The compile: `compile(spec, main, wt, dry_run, runner, tokens) -> {object, compiled, fresh, bytes, sections, log} |
  {object, compiled: False, error} | {command, object, dry_run}`; `object_is_fresh(object, source)`; `section_sizes(obj)`;
  `run_tokens(tokens, cwd, expect, scratch_dir, src, verbose)` (the flag tools' raw run).
* The target: `resolve_target(wt, main, spec, symbol) -> (path, kind in {worktree-split, registered, auto-fallback, missing},
  note)`, `proposal_target(wt, main, symbol)`, `resolve_map(wt, main, rel)`, `symbol_addresses`, `text_symbol_addresses`,
  `auto_text_runs`, `retired_object_dirs`.

## Invariants and rules

* **One key per unit.** Every spelling reduces to the same `stem`; a lane pasting `src/NHTTP/NHTTP_bgnend` once produced a
  doubled `src/src/...` path. `normalize` strips `./`, `build/RMHE08/{src,obj}/`, `build/`, `src/` repeatedly and one
  objdiff `main/` when a path follows it (the top-level unit `main`/`main.cpp` stays).
* **The extension comes from the tree, never a guess.** `source_spelling`: an explicit source, an explicit extension, the
  `configure.py` registration, the file under `src/` (`.cpp` first), then `.cpp`. Defaulting to `.cpp` told MWCC to compile a
  `.c` unit's non-existent `.cpp` (four lanes fell back to ninja + symdiff).
* **`Unit.resolve` never guesses**: a bare stem shared by two units is refused with the candidates; no spec with several units
  is refused; a unit without source is refused. A fixture names its tree (`root`); nothing reads a module-global root.
* **The command is the build's.** MAIN's `ninja -t commands`, else the worktree's, else a registered sibling in the same
  `config.libs` block (same `mw_version` and `cflags`) with only `-o` and `-lang` retargeted - never hand-rolled flags.
* **The worktree's headers win.** MWCC searches `-i` in order and MAIN's list is relative to MAIN: the worktree's own
  `include/` and `build/RMHE08/include` go first, and each of MAIN's directories is pointed at the worktree's copy when it has
  one (appending them once measured MAIN's copy of an edited header - the `eft004` round).
* **MWCC's `-o` is a directory**; the object's name comes from the source. The chained `objalign.py`/`objextab.py` arguments
  are absolutised to the object MWCC just wrote (left relative they rewrote MAIN's object, or raised for an unregistered unit).
* **A switch is never a path**: a `-`/`/` token is left alone (`cmd /c` became `cmd C:\c` - an interactive shell - on a host
  with `C:\c`); other tokens naming a file in MAIN are absolutised (Windows resolves a relative executable against the parent's
  cwd, not `cwd=`).
* **A stale object is impossible**: the object is deleted before the compile, a 0 exit without an object is refused, and an
  object older than its source is refused; `fresh` says the mtime moved.
* **The target follows the invocation**: this tree's split object, then MAIN's (with a note that the score is MAIN's), then the
  retired `auto_*_text` object located **by address** through this tree's map merged over MAIN's (a branch's rename keeps
  working), then `missing` - never an invented number.

## Absorbs (today's implementations)

`unitutil._versions/_find_src/_make/list_units/resolve_unit/compile_command/unquote/run_compile` (now delegates),
`recompile.unit_source/normalize_unit/resolve_unit_source/_ninja_compile_lines/ninja_command/_unit_stem/retarget/lib_block/
sibling_for/unit_tokens/retarget_object_helpers/rewrite/include_pairs/order_includes/source_path/object_is_fresh/
section_sizes/is_switch/absolutize/same_tree/resolve_map/text_symbol_addresses/auto_text_runs/symbol_addresses/
retired_object_dirs/proposal_target/target_rel/resolve_target/compile_unit` (aliases), `measure.normalize_unit`,
`verifyunit.unit_stem/src_object_rel/target_object_rel/report_unit_name`, `ledger.report_name/bare`, `unitscore`'s use of
`verifyunit.unit_stem`.

## Lib dependencies

`lib.repo` (`VERSION`), `lib.project.configure` (registration), `lib.project.symbols` (the map), `lib.binary.elf` (section
sizes) - the last three imported on first use.

## Test contract

Tier: fixture (`tools/tests/lib/test_units.py`, 47 checks). Every spelling in `duplication.md` (g) has one stem and report name;
`Unit.resolve` on a `FixtureTree` (nested, top-level, bare stem, refusals, registration lib/flag); `source_spelling`'s order;
`rewrite`/`order_includes` (worktree include first even when MAIN lists its generated include first), helper retargeting, the
`cmd /c` switch; `unit_tokens`' three sources with a stub ninja; `compile` with stub compilers (fresh, no object, stale);
`proposal_target` by address through a renamed map and by run; `resolve_target`'s order.

## Known gaps

* `compile` returns a dict, not a `CompileResult` value: `recompile`, `measure` and their selftests read it with `.get`; the
  value type waits for WP3b.
* Not yet collapsed onto `stem`: `claims.norm_unit` and `promote.norm_unit` (lane keys, `lib/lanes/naming.py`, WP3e),
  `stylelint._unit_stem` (WP3d), `undefrefs._unit_stem` / `flipcheck.unit_name_for` / `datagap.object_path` /
  `relocaudit.object_paths` / `vtableaudit.object_paths` / `pairgap._stem_of` (WP3a), `dossier`/`unwindcut.resolve_unit`
  (WP3c), `infer.obj_path_for` (WP3b), `brief.source_name` (WP3e), `mwlink_debugger.unit_of_object` (WP5).
* `slug` is not here: the lane slug is `lib/lanes/naming.py`'s (WP3e).
* `recompile.split_staleness` stays in `recompile` (it calls `claims._build_is_current`, a tool).
