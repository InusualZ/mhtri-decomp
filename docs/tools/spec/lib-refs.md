# `lib/refs` - Who references what: the address-keyed reference index over objects (and optionally the asm dump), cached, with the census over target objects

## Purpose

Who references what: the address-keyed reference index over objects (and optionally the asm dump), cached, with the census over target objects.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `RefIndex.build(tree, source in {objects, asm, both}) -> RefIndex` (cached by `lib.cache`); `query(address) -> [Ref(site, kind in {call, branch, addr, load, store, data-word, pointer}, func, unit, insn)]`
* `readers_of(section, lo, hi)`, `runs_over(lo, hi, step)`, `callers_of(func)`, `callees_of(unit)`, `census(units) -> [Record(unit, section, address, name, size, sites)]`
* names resolve per query through `SymbolMap`; the dump's labels are never trusted (addresses only)

## Absorbs (today's implementations)

`callers.build_index/build_elf_index/query/load_index/range_report`, `datagap.census`, `splitcheck.Ctx` readers, `tudiscover.build_graph`, `accessextent.object_sites/repair_sites`

## Test contract

Tier: fixture (a lib test never reads the live tree). a FixtureTree with objects and a three-file asm dump (the `callers` fixture): a renamed callee still resolves by address; the stale top-level copy ranks below the canonical file; `census` equals `datagap.census` on the fixture

## Known gaps

None until implemented; `migration.md` names the package.
