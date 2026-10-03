# `lib/names` - Generated-name and mangling predicates, once

## Purpose

Generated-name and mangling predicates, once.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `is_generated(name)`: `fn_`/`lbl_`/`loc_`/`jumptable_`/`@etb_`/`@eti_`/`@NNN`/`pad_`/`gap_`; `address_of(name)` for the `_XXXXXXXX` spelling
* `is_mangled(name)` (`__F`/`__Q` argument lists), `linkage_stem(name)` (the name before `__<args>`; `__start`, `_savegpr_14` are not split)
* `estimate_member_mangling(type, method, params, const)`, `estimate_static_mangling`, `peel_tokens(name)` (the `<digits><chars>` form)

## Absorbs (today's implementations)

`callees.is_generated`, `dumpmap.is_generated`, `relocaudit.linkage_stem`, `undefrefs.linkage_stem`, `langcheck.mangled`, `mangle.estimate_*`, `typeregistry.tokens_of_name`, `stylelint` rule 7/9 regexes

## Test contract

Tier: fixture (a lib test never reads the live tree). a table of names with the expected predicate values; the stems the relocaudit test pins

## Known gaps

None until implemented; `migration.md` names the package.
