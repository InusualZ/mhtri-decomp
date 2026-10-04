# `seams` - The TU-seam evidence, computed once: `.data` emission order, the literal pool model, source-file names

`tools/splits/seams/` (WP3c). One module, `evidence.py`; the entry points stay where the docs and profiles call them:
`tools/splits/tudiscover.py`, `tools/splits/dataorder.py`, `tools/units/dataseams.py`, `tools/units/poolseams.py`.

## Purpose

Holds the rules that say where a translation unit ends, so every seam tool reads one implementation: how a retail `.data`
symbol is classified and which seams its order implies (and where a narrow gap is cut), what a pool literal is and which
literals prove a value, the bare source-file name an assert carries, the union-find that groups units over shared literals,
and the map/splits/image readers those rules are applied to.

## Users

`tudiscover` (the data-order observations, the pool model, the `__FILE__` anchors, the map tables, the image),
`dataorder` (the classifier and the seams: its CLI is a view over them), `dataseams` (the strong seams with their cut),
`poolseams` (the literal and value-witness rules, `components`, the image for values), `langcheck` (the source-name rule,
the map tables, the image), the `splitcheck` invariants (`tools/splits/invariants/`: the literal rules, the seams,
`components`). No tool outside the family imports it.

## CLI

None: a module. The entry points' CLIs are in their own specs.

## Inputs and outputs

Reads `config/RMHE08/symbols.txt` (`map_rows`, `map_tables`), `config/RMHE08/splits.txt` (`section_ranges`) and the retail
`main.dol` (`Image`) at the paths a caller passes. Writes nothing.

## Public API

* The image: `Image(path)` (a `lib.binary.dol.Dol`; `read(addr, n)` is `bytes_from`: it may run past a segment end, the
  reading every seam kind was measured with; `secs` the segment table).
* The project files: `map_rows(path) -> [(section, address, size|None, name)]`, `map_tables(path) -> (functions, labels)`
  (the `tudiscover` tables: functions with `addr`/`size`/`scope`, every non-`.text` row a label with `section`/`addr`/
  `size`/`kind`/`local`), `section_ranges(splits, section) -> {start: (unit, end)}`, `range_of(ranges, addr)`.
* Data order: `Sym`, `VTABLE`/`STRING`/`DATA`, `classify(blob, tlo, thi)`, `classify_all(rows, reader)`, `text_range`,
  `data_symbols`, `seams(syms)`, `strong_seams(syms)` (strong rows with `cut`), `tail_cut(syms, at, row)` (the one cut
  rule `strong_seams` and `fragments` share), `zigzag_pairs`, `fragments(syms, weak)`,
  `inline_tail(gap)`, `is_header_name`, `is_source_name`, `retail_symbols(symbols, dol)`; `STRONG_KINDS`, `NARROW`,
  `PAD_MAX`, `TAIL_RUN_MAX`, `HEADER_NAME_RE`, `SOURCE_NAME_RE`.
* The pool: `is_literal(section, size, kind, type_="object")`, `is_value_witness(...)`, `components(pairs)`;
  `LITERAL_SECTIONS`, `MAGIC`, `VALUE_KINDS`.

## Invariants and rules

* **Data order** (`docs/data-order-seams.md`): MWCC lays out one TU's `.data` as `D* S* V* s*` - initialised globals over
  8 B, the strings of out-of-line functions, vtables in the reverse of class order, then the inline tail (the strings of
  inline functions: message, class name, header `__FILE__`, one unmerged copy per instance). So in retail:
  * **V->S** (strong): strings between two vtable groups - a boundary lies in the gap `[first string, next vtable)`, after
    the leading inline tail (`inline_tail`: strings up to the last header name before the first source-file name, at most
    `TAIL_RUN_MAX` plain strings between two header names; a non-string or a source name ends it).
  * **zigzag** (strong): two adjacent vtables whose owners (first code slot) go up are two TUs; down is one TU, equal owners
    are no evidence (`zigzag_pairs` counts the three).
  * **V->tail** and **V->D** (weak): strings with no later vtable may be an inline tail; data after a vtable may be a jump
    table. Neither is used to cut, warn or refuse.
  * A `D` of at most `PAD_MAX` (8) bytes between a vtable and the next symbol is alignment padding, skipped.
  * A vtable is at least three words, a `0, 0` header (`-RTTI off`) and code pointers or zeros, at least one pointer; a
    string is printable, NUL-terminated, at least two characters.
  * **The cut**: a V->S gap of at most `NARROW` (8) symbols is cut at `cut`, the first symbol after its tail; a wider gap is
    never cut (it only warns), a zigzag cuts at its own address.
* **The pool** (`docs/pool-seams.md`): MWCC emits one literal pool per TU, one entry per value, and `mwld` does not merge
  pools. A **literal** is an `.sdata2` 4/8-byte object or an `.sdata` string; a **value witness** is an `.sdata2`
  float/double only (an `.sdata` string may be an initialised `char[]`, an untyped word may be half of an 8-byte object the
  map cut in two). The int->float constants (`MAGIC`) are still per-TU entries. One rule for the three readers that used to
  carry their own (`poolseams.is_literal`, `tudiscover.is_pool_literal`, `splitcheck.is_literal`; measured identical on
  this tree: every `.sdata2` 4/8-byte row is `type:object`, every `.sdata2` float/double is 4 or 8 bytes).
* **Source names**: a bare `__FILE__` is `[A-Za-z0-9_][A-Za-z0-9_./\-]*` ending `.c`/`.cpp`/`.cc`/`.cxx`/`.cp`/`.c++`; one
  regex for `tudiscover`'s anchors, `dataorder`'s inline-tail end and `langcheck`'s string signal (three before; the
  union, measured to agree on all 110 anchor strings and all 3 728 classified `.data` strings, and the DOL carries no
  `.cc`/`.cxx`/`.c++` string at all).
* `components` is the union-find over unit pairs both `poolseams` (the census groups) and the `pool` invariant (the decode
  groups) use; groups come out in the order of their first pair, so both callers' outputs are unchanged.
* Nothing here resolves a tree: a caller passes paths (no module-level root), so the module imports under the fixture tier.

## Lib dependencies

`lib.project` (`SymbolMap`, `Splits`), `lib.binary.dol`.

## Test contract

Tier: fixture (`tools/tests/splits/seams/test_evidence.py`): the classifier on hand-built words, the seam kinds (zigzag with
padding, V->D weak, V->S with its tail and cut, wide gap uncut, V->tail never strong), the name rules, the literal and witness
rules, `components`, and the readers on a `FixtureTree` whose DOL is built with `DolBuilder`.

## Known gaps

* The four evidence *readers* stay apart because their inputs differ: the asm-dump graph (`tudiscover`), the retail-text
  decode (`splitcheck`), the target objects' relocations (`poolseams` through `lib.refs.census`). What is shared is the rule
  each applies; `duplication.md`'s "ctors closure vs `tudiscover.extab_runs`" pair is not one concept (`extab_runs` maps a
  function range to its unwind entries; the sinit closure is a call-graph walk), so nothing was collapsed there.
* `tudiscover.function_graph` (`lib.refs`) is still a second dump parser beside `lib.refs.parse_dump_file`; unifying them
  changes the graph cache and `tudiscover`'s numbers - not done here (see `lib-refs.md`).
* `poolseams.owner_of` keeps its own interval search over the census's `{section: [(start, end, unit)]}` shape.
