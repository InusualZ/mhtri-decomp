# `lib/refs` - Who references what: the address-keyed reference index and its query, the dump's stamp, the census over target objects, the text scan

## Purpose

Builds and caches the whole-DOL reference index (from the asm dump, or from the split objects' relocations when there is no
dump), answers "which sites reference this address", groups reader sets into runs, classifies every data symbol a unit's target
object references (the census), scans retail code for data and function references, and builds `tudiscover`'s per-function
graph over the dump. Since WP3c it also answers the query a tool prints (names, owners, the readers of an address, the runs:
`RefIndex`) and says how old the dump is (`DumpStamp`).

## Users

`callers` (the CLI over `RefIndex`, `DumpStamp`), `accessextent`, `dataclaim` and `dataclosure` (`RefIndex`: the query,
`readers_of`, `norm_reader` - WP3c, they no longer import `callers`), `datagap`/`dataclosure` (`census`, `census_records`,
`classify_address`, `object_refs`, the tree-level census), `poolseams` (`tree_census`, `census_claims`, `census_symbols`),
the `splitcheck` invariants (`text_refs` - the `Ctx` readers), `tudiscover` (`function_graph`, `resolve_name`, `rel_owners`,
the dump regexes, `DumpStamp`, `has_dump`).

## Public API

* The dump: `dump_files(asm_dir)`, `parse_dump_file(path) -> (labels, funcs, refs)`, `build_dump_index(asm_dir, files, game) ->
  (index, stats)`, `file_rank`, `is_scaffolding`, `dump_branch_target(address, byte_text)`, `symbol_operand`, `mem_kind`,
  `arg_hint`, `key_of`; `SCAN_RE`, `SYM_RE`, `MOD_RE`, `LOAD_MNEMONICS`.
* The dump's stamp (WP3c, from `tudiscover`): `has_dump(dir)`; `DumpStamp(asm_dir, symbols, splits, dol, game, local_dir,
  root, stamp=None)` with `current()`, `write(extra)`, `status() -> (state, message)` (fresh / missing / unstamped / stale /
  truncated), `is_main_fallback()`, `DumpStamp.for_tree(root, honour_env)`. The file is `<asm_dir>/.stamp.json`, the format
  `tudiscover` always wrote (`dump_asm.py` writes it).
* The objects: `object_files(obj_dir)`, `object_signature(obj_dir, files)`, `build_object_index(obj_dir, files, symbols, game)`
  where `symbols` is `{name: [(section, address, type)]}` (`Ownership.symbols`).
* The cache: `load_index(cache, signature, files, build, rebuild=False, changed=...) -> (index, info)`; `attach_derived(index)`.
* Queries: `rows_at(index, address, names=()) -> (rows, retried)` with rows `(site, kind, func, text, arg)` and kind in
  `KINDS = (call, branch, addr, read, write, pointer)`; `coalesce(rows)`; `runs_over(readers, lo, hi, step) -> {range,
  addresses, runs, seams}`.
* The census: `object_refs(path) -> {name: relocation count}`, `classify_address(section, address, ranges, unit)`,
  `census_records(unit_refs, symbols, ranges) -> (records, stats)`, `census(obj_dir, units, symbols, ranges) -> ((records,
  stats), read)`; for a tree (WP3c, from `datagap`/`dataclosure`): `census_claims(splits_text)` (`{section: [(start, end,
  unit)]}`, sorted, no extension), `census_symbols(symbols_path)` (name -> row, a duplicated name dropped),
  `census_units(ranges, units)`, `tree_census(root, units, ranges, game) -> ((records, stats), registered, read)`;
  `CENSUS_SECTIONS`, `BOOKKEEPING`, `CALL_TYPES`.
* The query (WP3c, from `callers`): `dump_dir(root, honour_env)`, `objects_dir(...)`, `dump_cache(root)`, `objects_cache(root)`,
  `load_dump_index(root, rebuild, asm_dir, cache, honour_env, game)`, `load_object_index(root, rebuild, cmap, cache,
  honour_env, game, obj_dir)`; `RefMap` (the map as an address index, `RefMap.load(root)`; `name_at`, `function_at`,
  `owner`, `owner_at` in `lib.project.ownership.owner_label`'s vocabulary); `plain_name`, `fmt_addr`, `find_target`,
  `caller_of`, `current_text(text, index, cmap, target, name)` (a dump text's symbol operands respelled by the live
  map at their addresses; `query` applies it to every row, keeping `dump_instruction`),
  `query(text, index, cmap, kinds, limit, pointers) -> report`, `norm_reader(label)`, `reader_units(report)`,
  `readers_of(index, cmap)`, `range_report(index, cmap, lo, hi, step)`; `RefIndex.load(root, rebuild, honour_env=None)` ->
  `index`, `info`, `cmap`, `source` (`asm` / `elf`), `asm_dir` with `query`, `readers_of`, `runs`.
* The text scan: `text_refs(chunks, sda13, sda2, is_data, fn_starts, same_function) -> TextRefs(refs, loads, stores, passes,
  fn_edges)`.
* The per-function dump graph: `function_graph(files, fns, labels, rel_root) -> {funcs, extab, owners, fn_check}`,
  `resolve_name(tok, names, labels=None)`, `rel_owners(txt, fns, labels)`, and its regexes (`FN_BLOCK_RE`, `TOKEN_RE`, ...).

## Invariants and rules

* **Addresses, never labels.** A dump `bl`/`b` target is the instruction's own displacement (`lib.ppc.branch_target`); a data
  reference resolves through the dump's own label table (the `# <section>:0xOFF | 0xADDR | size:` headers), which is stale in
  the same way as the reference, so an old name still lands on the right address. A name neither table resolves is kept under
  its name and retried per query (`rows_at(..., names)`), so the current map can name it.
* **One site per instruction**: a reference is keyed `(site, kind)` (plus the name for a `.4byte` entry); when two dump copies
  print it, the best `file_rank` wins - not the top-level `auto_*` scaffolding, then the newest mtime.
* The object index anchors each section with one map-known symbol (`map address - object offset`); every non-call code
  relocation is `addr`, every data-section relocation `pointer`; the text names the relocation with `lib.binary.elf.reloc_name`.
* `load_index` never trusts a cache with another `SCHEMA`, signature or file count; the on-disk format is the one `callers.py`
  wrote before this module (`build/tmp/callers/graph.json`, `elf-graph.json`), so no cache was invalidated.
* **The query is `callers.py`'s, verbatim** (WP3c): names through the current map (`RefMap` over `Ownership`), the container
  of a site is the dump's own block (the map wins only with a row at its start), `lis`+`addi` coalesced before counting, a
  name the dump could not resolve retried per query. `RefIndex.load` answers from the dump when the tree reads one, else from
  the objects - the choice `callers`, `accessextent`, `dataclaim` and `dataclosure` each spelled before.
* `honour_env` (let `$MHTRI_MAIN` pick MAIN) defaults to `lib.repo.is_served(root)`: the served tree only, never a fixture,
  and resolving it never touches the live tree under the fixture tier.
* **The dump's stamp** is what the dump is a function of: the map, the splits and the DOL by content, the `.s` count and
  bytes; `stale` names the input that changed, `truncated` a dump that lost files, `[MAIN's dump at ..., read-only]` a
  fallback read.
* **The census** reads a unit's TARGET object: a relocation names a symbol the object does not define, extab bookkeeping is
  skipped, the count is relocations per name; a referenced data symbol is `own`, `other` (owner named) or `orphan` (the
  registered neighbours bracketing it); an unmapped, code or non-data name is only counted.
* `text_refs` is `splitcheck.Ctx.scan`'s loop: per chunk, `scan_refs` for data (loads/stores/passes), then `scan_calls`, then
  the function addresses taken (`lis`/`addi` onto a function start) unless `same_function(site, target)`; `fn_edges` keeps
  that order, which the ctors-closure walk depends on.

## Absorbs (today's implementations)

`callers.parse_dump_file/symbol_operand/mem_kind/arg_hint/key_of/build_index/attach_derived/load_index/build_elf_index/
load_elf_index/coalesce` and the run grouping of `range_report`; WP3c: `callers.Map/load_map/find_target/query/caller_of/
plain_name/fmt_addr/norm_reader/reader_units/readers_of/range_report` and the dump-or-objects choice of `callers.main`,
`accessextent.main`, `dataclaim.census_readers`, `dataclosure.readers_index`; `tudiscover.has_dump/dump_stamp/
write_asm_stamp/asm_stamp_status`; `dataclosure.parse_splits_text/load_data_symbols/_registered/census`; `datagap.census_records/classify_address/object_data_refs`
and the census read; `splitcheck.Ctx.scan`'s scanning loop; `tudiscover.build_graph`'s per-file parse, `resolve_name`,
`rel_owners` and their regexes. `callers.build_elf_index` no longer imports `dossier.parse_elf`, nor `datagap` `undefrefs`
for the census (both read `lib.binary.elf`).

## Lib dependencies

`lib.ppc`, `lib.cache` (`stat_digest`, `content_hash`), `lib.text` (`atomic_write`), `lib.binary.elf`; lazily, for the
tree-level helpers only, `lib.project` (`Splits`, `SymbolMap`, `Ownership`, `owner_label`) and `lib.repo`
(`resolve_input`, `is_served`).

## Test contract

Tier: fixture (`tools/tests/lib/test_refs.py`, 65 checks). A `FixtureTree` with a three-file asm dump (the unit's file, a stale
top-level copy printing the old `fn_` label of the same callee, a data file): the renamed callee resolves by address with the
canonical text, the stale copy ranks below, the shared site is dropped once, sda21 read/write kinds, `lis`/`addi` coalescing,
name retry; the cache's five reasons (built, hit, changed, forced, foreign schema); an `ElfBuilder` object for the object
index (call / addr / pointer, absolute sites) and the census (extab skipped, orphan with neighbours, counted code name,
own/other); `runs_over`; `text_refs` (data reference, call and address-taken edges in order, own-function filter);
`function_graph` and `resolve_name`; WP3c: the stamp's five states, the fallback note and `for_tree`; the tree census
(claims without extensions, a duplicated name dropped, orphan/other records, the unit filter); the query on a dump tree
(map names, canonical texts, reconstructed/unsplit owners, the plain-name match, the error, the filter note, `norm_reader`,
`readers_of`, `runs`), the object fallback and the tree with neither.

## Known gaps

* `RefIndex` (WP3c) wraps the cache's JSON dict rather than replacing it: the dict's keys (`labels`, `_label_at`, `refs`)
  stay the on-disk format, so no cache was invalidated; a typed `Ref` per row is not introduced (the report dicts the tools
  print are the shape they already parse).
* `accessextent.object_sites/repair_sites` still re-decode objdump text to repair the object fallback's coarse kinds (the
  abstract interpreter the design keeps in `accessextent`).
* `function_graph` is a second dump parser beside `parse_dump_file` (names and blocks vs addresses and sites); unifying them
  changes `tudiscover`'s graph and is 3c's (`tools/splits/seams/`).
* `accessextent.object_sites/repair_sites` (its re-decode of objdump text) is not absorbed: it is the abstract interpreter
  the design keeps in `accessextent`.
* `splitcheck.Ctx` keeps its symbol-index mapping (`refs`/`load_refs`/... by data-symbol index) and the linker-operand filter;
  only the scan moved. Reducing `splitcheck` to invariants over `lib.refs` is 3c.
