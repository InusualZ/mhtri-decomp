# `lib/project` - The three project files and ownership: a streaming symbol map, a round-tripping splits file, the configure registry, and one owner lookup

## Purpose

The three project files and ownership: a streaming symbol map, a round-tripping splits file, the configure registry, and one owner lookup.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `SymbolMap(path)`: `rows()` (streaming), `by_name`, `at(address, count)`, `in_range(section, lo, hi)`, `find(regex, section, type)`, `check()`, `plan_rename(pairs, force)`, `plan_merge(rows)`, `apply(plan)` through `lib.text.Transaction`; no API returns the whole text
* `Splits.parse(text)`/`render()` round-trip, `units`, `ranges` (`Range(unit, section, start, end)`), `covering(section, address)`, `claims(unit)`, `overlap(section, lo, hi)`, `add_block`, `rename_unit`, `text_ranges()`
* `Configure.load(path)`: `objects()` (flag, path, lib, kwargs), `libs()`, `cflags(lib)` with group inheritance resolved once, `matching_units()`, `object_line(path)`
* `Ownership(tree, ref=None)`: `owner_of(section, address) -> Owner(unit, range, state in {reconstructed, registered, unsplit, auto})`, `unit_of_symbol`, `symbols_of_unit`, `band_of`; `Ownership.at_ref(tree, ref)` reads the three files from git

## Absorbs (today's implementations)

24 splits parsers, 18 symbols parsers, 14 configure parsers, `stylelint.Ownership/load_ownership_at_ref`, `symbolpreflight.load_*`, `ledger.Objects`, `sharedfiles.parse_ranges/find_overlap`, `symedit.parse_line/plan_*`

## Test contract

Tier: fixture (a lib test never reads the live tree). parse/render round-trips the real `splits.txt` byte-identically (smoke); `covering` agrees with every retired interval search on a fixture; the rename/merge refusals of `symedit`'s selftest; `Ownership` on a FixtureTree with all three states

## Known gaps

None until implemented; `migration.md` names the package.
