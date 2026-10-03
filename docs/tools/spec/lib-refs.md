# `lib/refs` - Who references what: the address-keyed reference index, the census over target objects, the text scan

## Purpose

Builds and caches the whole-DOL reference index (from the asm dump, or from the split objects' relocations when there is no
dump), answers "which sites reference this address", groups reader sets into runs, classifies every data symbol a unit's target
object references (the census), scans retail code for data and function references, and builds `tudiscover`'s per-function
graph over the dump.

## Users

`callers` (the index, its cache, the query rows, the runs; `accessextent`, `dataclaim`, `datagap` reach it through `callers`),
`datagap` (`census`, `census_records`, `classify_address`, `object_refs`; `poolseams`, `dataclaim`, `backlog` and the gate's
data-closure rows read those through `datagap`), `splitcheck` (`text_refs` - the `Ctx` readers), `tudiscover`
(`function_graph`, `resolve_name`, `rel_owners` and the dump regexes).

## Public API

* The dump: `dump_files(asm_dir)`, `parse_dump_file(path) -> (labels, funcs, refs)`, `build_dump_index(asm_dir, files, game) ->
  (index, stats)`, `file_rank`, `is_scaffolding`, `dump_branch_target(address, byte_text)`, `symbol_operand`, `mem_kind`,
  `arg_hint`, `key_of`; `SCAN_RE`, `SYM_RE`, `MOD_RE`, `LOAD_MNEMONICS`.
* The objects: `object_files(obj_dir)`, `object_signature(obj_dir, files)`, `build_object_index(obj_dir, files, symbols, game)`
  where `symbols` is `{name: [(section, address, type)]}` (`Ownership.symbols`).
* The cache: `load_index(cache, signature, files, build, rebuild=False, changed=...) -> (index, info)`; `attach_derived(index)`.
* Queries: `rows_at(index, address, names=()) -> (rows, retried)` with rows `(site, kind, func, text, arg)` and kind in
  `KINDS = (call, branch, addr, read, write, pointer)`; `coalesce(rows)`; `runs_over(readers, lo, hi, step) -> {range,
  addresses, runs, seams}`.
* The census: `object_refs(path) -> {name: relocation count}`, `classify_address(section, address, ranges, unit)`,
  `census_records(unit_refs, symbols, ranges) -> (records, stats)`, `census(obj_dir, units, symbols, ranges) -> ((records,
  stats), read)`; `CENSUS_SECTIONS`, `BOOKKEEPING`, `CALL_TYPES`.
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
* **The census** reads a unit's TARGET object: a relocation names a symbol the object does not define, extab bookkeeping is
  skipped, the count is relocations per name; a referenced data symbol is `own`, `other` (owner named) or `orphan` (the
  registered neighbours bracketing it); an unmapped, code or non-data name is only counted.
* `text_refs` is `splitcheck.Ctx.scan`'s loop: per chunk, `scan_refs` for data (loads/stores/passes), then `scan_calls`, then
  the function addresses taken (`lis`/`addi` onto a function start) unless `same_function(site, target)`; `fn_edges` keeps
  that order, which the ctors-closure walk depends on.

## Absorbs (today's implementations)

`callers.parse_dump_file/symbol_operand/mem_kind/arg_hint/key_of/build_index/attach_derived/load_index/build_elf_index/
load_elf_index/coalesce` and the run grouping of `range_report`; `datagap.census_records/classify_address/object_data_refs`
and the census read; `splitcheck.Ctx.scan`'s scanning loop; `tudiscover.build_graph`'s per-file parse, `resolve_name`,
`rel_owners` and their regexes. `callers.build_elf_index` no longer imports `dossier.parse_elf`, nor `datagap` `undefrefs`
for the census (both read `lib.binary.elf`).

## Lib dependencies

`lib.ppc`, `lib.cache` (`stat_digest`), `lib.text` (`atomic_write`), `lib.binary.elf`.

## Test contract

Tier: fixture (`tools/tests/lib/test_refs.py`, 32 checks). A `FixtureTree` with a three-file asm dump (the unit's file, a stale
top-level copy printing the old `fn_` label of the same callee, a data file): the renamed callee resolves by address with the
canonical text, the stale copy ranks below, the shared site is dropped once, sda21 read/write kinds, `lis`/`addi` coalescing,
name retry; the cache's five reasons (built, hit, changed, forced, foreign schema); an `ElfBuilder` object for the object
index (call / addr / pointer, absolute sites) and the census (extab skipped, orphan with neighbours, counted code name,
own/other); `runs_over`; `text_refs` (data reference, call and address-taken edges in order, own-function filter);
`function_graph` and `resolve_name`.

## Known gaps

* The index is the cache's JSON dict plus functions over it, not the `RefIndex` class `design.md` sketched: `callers`,
  `accessextent`, `dataclaim` and `datagap` read the dict's keys (`labels`, `_label_at`, `refs`), and changing that shape is a
  cache-format change. The typed wrapper belongs with the 3c family that rewrites those readers.
* Names are still resolved by `callers.query` (owner labels through `callees.classify_owner`), not by `lib.refs`; the
  `readers_of` census stays in `callers` because it is defined as "what `query` reports".
* `function_graph` is a second dump parser beside `parse_dump_file` (names and blocks vs addresses and sites); unifying them
  changes `tudiscover`'s graph and is 3c's (`tools/splits/seams/`).
* `accessextent.object_sites/repair_sites` (its re-decode of objdump text) is not absorbed: it is the abstract interpreter
  the design keeps in `accessextent`.
* `splitcheck.Ctx` keeps its symbol-index mapping (`refs`/`load_refs`/... by data-symbol index) and the linker-operand filter;
  only the scan moved. Reducing `splitcheck` to invariants over `lib.refs` is 3c.
