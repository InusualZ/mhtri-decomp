# `lib/project` - The three project files and ownership: a round-tripping splits file, a streaming symbol map, the configure registry, and one owner lookup

## Purpose

Reads `config/RMHE08/splits.txt`, `config/RMHE08/symbols.txt` and `configure.py` one way each, plans the map's renames and
merges, and answers "who owns this address / this name" from one index. No API returns the map's text.

## Users

`symbols/symedit.py` (the CLI over `SymbolMap`), `units/stylelint.py` (`Ownership` subclass with the lint's counters),
`callers`, `callees`, `symbolpreflight`, `ledger`, `sharedfiles`, `splitcheck`, `objalign`/`objextab`, `datagap`, `vtableaudit`,
`relocaudit`, `langcheck`, `unwindcut`, `m2cinput`, `dumpmap`, `unionresolve`, `land`, `mergebranch`, `brief`, `queue`,
`dataorder`, `unitscore`, `flipcheck`, `backlog`, `mwlink_debugger`, `verifyunit`, `tudiscover`, `undefrefs`, `typeregistry`,
`methodize`, `recompile`, `flags/infer`, `rso/symbols`.

## Public API

`from tools.lib.project import Splits, Range, SymbolMap, Symbol, parse_line, Configure, ObjectRow, ObjectCall, object_calls,
Ownership, Owner, AutoObjects, Refused, ShapeError`.

* `splits.py`: `Splits.parse(text)` / `Splits.read(path)` / `Splits.cached(path)` (per path, mtime, size) and `render()`;
  `units`, `ranges` (`Range(unit, section, start, end, attrs)` with `size`, `rename`, `object_section`, `contains`, `overlaps`,
  `line()`), `blocks` (`Block(unit, attrs, ranges, lines, lead)`), `sections()`, `block(unit)` / `claims(unit)` (exact key,
  else by stem), `text_ranges()`, `by_section()` (`{section: [(start, end, unit)]}` sorted), `by_unit()`
  (`{unit: {section: (start, end)}}`), `covering(section, address)`, `overlap(section|None, lo, hi)`,
  `neighbours(section, address)`; edits return a new value: `add_block(unit, ranges, attrs, before)`, `rename_unit(old, new)`,
  `remove_block(unit)`; `first_overlap(ranges, start, end, section, unit)`; `stem(unit)`.
* `symbols.py`: `parse_line(line) -> Symbol | None` (`name section address type size line lineno`, properties `comment`,
  `scope`, `kind` (`data:`), `align`, `hidden`, `sized`, `end`, `to_dict()` = symedit's dict); `SymbolMap(path)`: `rows()`
  (streamed once, then cached), `by_name()`, `by_section()`, `names()`, `find(regex, section, type)`, `in_range(lo, hi,
  section)`, `infer_section(address, section)`, `at(address, count, section)`, `check() -> CheckResult(symbols, duplicates,
  unparsed, aliases)`, `plan_rename(pairs, force) -> RenamePlan`, `plan_merge(rows, scan_refs) -> MergePlan`, `apply(plan,
  write=None)`; `write_text(path, text, rename=None)` (the verified `lib.text.Transaction` write); `rewrite_name`, `resize`,
  `infer_section(rows, address, section)`.
* `configure.py`: `Configure.load(path)` / `Configure.parse(text, args=None)`: `objects()` (`ObjectRow(path, flag, linked, lib,
  mw_version, lib_cflags, cflags_name, cflags, options, line)`), `object(path)` (exact, else stem), `libs()` (`Lib(name,
  mw_version, cflags_name, cflags, progress_category, objects, line)`), `lib(name)`, `groups()`, `cflags(group or lib)`,
  `matching_units()`, `object_line(path)`, `object_line_text(path)`, `skipped` (lines the evaluator did not model);
  `object_calls(text) -> [ObjectCall(flag, path, line, closed)]` for a whole file or a fragment.
* `ownership.py`: `Ownership(symbols, ranges, auto=None, root=None)` (`symbols` = `{name: [(section, address, type)]}`,
  `ranges` = `{section: [(start, end, unit)]}`), `Ownership.load(root, auto=False)` (cached per mtime and class),
  `Ownership.at_ref(root, ref, show=None)` (`show(ref, rel) -> bytes | None`, default `lib.git.Git(root).show`),
  `from_files`, `from_texts`; `covering`, `owner_of(section, address) -> Owner(state, unit, section, range, band)`, `band_of`
  (alias `module`), `resolve(name)`, `name_at`, `resolution_at`, `unit_of_symbol`, `symbols_of_unit(unit, section)`;
  `AutoObjects.load(config_json, obj_dir)` / `from_config`; `module_name(unit)`; `symbol_index(rows)`;
  `owner_label(resolution, source_exists) -> (label, state, unit)` (WP3c, from `callees.classify_owner`: the vocabulary
  `callers`/`callees` print) and `source_exists(root)` (from `callees.make_source_exists`).

## Invariants and rules

* **No API returns the map's text** (non-negotiable 7 made structural): rows, lookups and plans only.
* **Splits round-trip byte for byte**: every unedited block renders from its own raw lines (spacing, comments, blank lines,
  CRLF, a missing final newline all survive); an edited or added block renders canonically (`\t%-11s start:0x%08X
  end:0x%08X`). `Sections:` is the legend, never a unit; a header may carry attributes (`a.c: comment:0`); a range may carry
  attributes (`rename:.ctors$10` - `object_section` is the name the unit's object carries).
* **`covering` is the first claim in address order** that contains the address (the stylelint index's rule, kept for
  overlapping claims); it agrees with every retired interval search (`tests/lib/test_project.py`).
* **A rename is planned, then written once.** Refused (nothing written) when the old name is not defined exactly once, the
  new name is taken (unless `force`), the new name is not `[A-Za-z_][\w.$]*`, a name appears twice in the batch, or the
  rewrite would put one name at two addresses. A pair already applied (old absent, new present) is a no-op; old and new both
  absent is a typo. The shape gate (`ShapeError`, a `lib.text.AnchorError`): the line must parse as the old name's row and
  the rewrite must parse back as the new name.
* **A phantom merge** (roadmap 7.9) grows the previous row's `size:` and deletes the phantom's row, refused unless both are
  defined once, in one section, the previous ends exactly at the phantom, no other name sits at that address, the size is
  the two sizes added, both are `type:function` with one scope, and the reference scan finds nothing. An absent phantom
  whose previous already has the new size is the re-apply; with the old size it is a half-applied plan. A stale previous
  name is refused with the name of the row that actually ends at the phantom.
* **The write** keeps the file's own line ending, goes through a `lib.text.Transaction`, and reads the bytes back: anything
  else restores the previous bytes exactly.
* **`configure.py` is evaluated, never executed** (executing it runs argparse and the build generator): top-level
  assignments, `if` on `args.*` with `configure.py`'s defaults (`DEFAULT_ARGS`), `.append`/`.extend`, list spreads and
  filters, f-strings with `config.version`, single-`return` helpers (`DolphinLib`, `MatchingFor`) and `Object(...)` calls.
  A statement it cannot model is skipped and listed in `skipped`; an unresolvable lib field or `Object` option is kept as
  its source text (`Unresolved`) rather than dropping the lib. The smoke test pins agreement with `configure.py` actually
  executed, for every object's lib, path, resolved cflags and link flag.
* **`object_calls` never reads a comment or a string**: a call is `Object(<name>, "<path>"` in code; `closed` is the one-line,
  no-option shape the union check and the landing diff read.
* **Owner states**: `reconstructed` (a claim whose unit has a source under `src/`, when `root` is known), `registered` (a claim
  with no source yet), `auto` (dtk's `auto_*` object covers it), `unsplit` (nothing does; `band` is the module of the claims
  bracketing it, None when they disagree) - the vocabulary `callers`/`callees` already used.

## Absorbs (today's implementations)

The splits parsers of `objalign`, `splitcheck`, `datagap`, `stylelint`, `vtableaudit`, `symbolpreflight`, `unwindcut`,
`sharedfiles` (+ `find_overlap`), `unionresolve`, `land`, `mergebranch`, `brief`, `queue.system_hints`, `dataorder`,
`unitscore`, `flipcheck`, `backlog`, `mwlink_debugger`, `verifyunit`, `tudiscover`, `callees.text_range`,
`typeregistry.symbols_by_unit`, `langcheck`'s selftest `unit_at`; the map parsers of `symedit` (canonical), `splitcheck`,
`dataorder`, `stylelint`, `vtableaudit`, `recompile`, `mergebranch`, `flipcheck`, `undefrefs`, `typeregistry`, `datagap`,
`tudiscover`, `brief`, `methodize`, `symbolpreflight`, `unwindcut`, `m2cinput`, `dumpmap`, `rso/symbols`; the configure parsers
of `infer` (an exec of `configure.py`), `brief`, `langcheck` (+ `cflags_tokens`), `relocaudit`, `verifyunit`, `unionresolve`,
`symbolpreflight`, `backlog` (`_cflags_groups/_resolve_group/_lib_groups`), `land.flips_objects/flipped_units`,
`recompile.lib_block`, `vtableaudit`; `stylelint.Ownership/load_ownership(_at_ref)/_covering_range`, `ledger.Objects`.

## Lib dependencies

`lib.text` (endings, `Transaction`, `AnchorError`), `lib.git` (`Git.show` for `at_ref`), `lib.repo` (`VERSION`).

## Test contract

Tier: fixture (`tools/tests/lib/test_project.py`): parse/render round-trips (LF, CRLF, comments, junk, no final newline);
the views and the edits; `covering` against every retired interval search at every byte of a fixture with gaps, touching
claims and an overlap; the line parser's attributes; every rename and merge refusal of `symedit`'s selftest plus both
endings and the no-op; the evaluator on spreads, filters, appends under `if args.*`, f-strings, helpers, `MatchingFor`,
object options, loose objects and unresolved groups; `object_calls` on fragments, comments and strings; `Ownership` on a
`FixtureTree` with all four states and `at_ref`. Smoke (`tools/tests/smoke/test_project_live.py`): the live `splits.txt`
renders byte-identically, every map line parses, the evaluator equals `configure.py` executed, `resolve` equals a linear
scan on a sample.

## Known gaps

* The tools' own interval searches over their own shapes stay (`symbolpreflight.covering`, `linkorder.covering_any`,
  `poolseams.owner_of`, `vtslot.containing_range`, `dataclaim.symbol_lookup`): their inputs now come from this module.
  `dataorder.unit_of` is the seam evidence's `range_of` (WP3c) and `datagap.classify_address` is `lib.refs` (WP2a).
* `datagap.splits_plan` edits `splits.txt` text line by line; it becomes `Splits` edits in WP3a (`dataclosure`).
* `preflight_selftest.load_repo` keeps its own parser on purpose (a second opinion over the live tree).
* `land.units_from_branch` (through `unionresolve.object_names`) reads only one-line `Object(kind, "unit")` calls, as before:
  an object with options is not seen. Kept for behaviour; a candidate fix for WP4.
* `Unit` spellings (`stem`, the `main/` prefix, build paths) are `lib.units` (WP2b); `stem()` here is only the extension strip.
