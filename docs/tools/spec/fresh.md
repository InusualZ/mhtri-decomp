# `fresh` - Every derived artifact of a tree: fresh or stale, why, its cost and its refresh command

## Purpose

Prints one row per registered derived artifact (`lib.artifacts.REGISTRY`) of a tree - state, cost, refresh command, reason -
and with `refresh` rebuilds the stale ones in dependency order, each under its lock.

## Users

Lanes and the orchestrator (the brief and the decompiler profile name it: "a tool refreshes its own derived artifacts; run
`fresh.py status` to see"); `docs/build-performance.md`.

## CLI

```
python tools/units/fresh.py [status] [NAME ...] [--unit SRC] [--json] [--root TREE]
python tools/units/fresh.py refresh [NAME ...] [--unit SRC] [--json] [--root TREE]
python tools/units/fresh.py prune-orphans [--json] [--root TREE]   # what `refresh orphan-objects` runs
```
`NAME` is a registry name (`manifest split asm-dump tudiscover-graph callers-index objects report slot-build
orphan-objects`); a named
artifact brings its dependencies. `--unit Camellia/camellia.c` scopes `objects` and `report` to that unit's source.
Exit codes: 0 every row fresh (or `n/a`), 1 something stale / missing / unknown (or a refresh failed), 2 could not run.
`--json`: `{root, artifacts: [{name, state, reason, command, cost, path, refreshed, seconds, note}]}`.

## Inputs and outputs

Reads the tree's build output and inputs (mtimes, the dump stamp, the caches, `ninja -n` dry runs, `tudiscover cache --check`).
`status` writes nothing; `refresh` runs the registry's commands in the tree (`slot-build` from MAIN), all of them writing only
gitignored output.

## Invariants and rules

* `status` never rebuilds and never runs a non-dry `ninja`.
* `refresh` is `lib.artifacts.ensure(..., "auto")`: in order, once, locked, re-checked after.
* `n/a` is not stale: `slot-build` outside a slot, `callers-index` with no dump (callers then answers from the objects).
* `orphan-objects` is stale while `build/RMHE08/src` holds an object no `configure.py` `Object` names, or
  `build/RMHE08/obj` one the split's `config.json` does not list; `prune-orphans` (`lib.artifacts.prune_orphan_objects`)
  deletes exactly those and their `.d` files, and exits 1 if any remain. `--json`: `{root, deleted}`.

## Lib dependencies

`lib.artifacts`, `lib.cli`, `lib.repo`.

## Test contract

Tier: fixture (`tools/tests/units/test_fresh.py`): on a temp tree with a stamped dump, `--json` lists every artifact, the
dump is fresh, a tree without `build.ninja` exits 1, a single fresh artifact exits 0, an unknown name is refused; orphan
objects on both sides are found, pruned (and nothing else), and skipped by `undefrefs.discover_units`. The same
file pins the adopting tools' defaults (tudiscover refreshes, callers falls back, refuse refuses).

## Known gaps

* MAIN read `objects`/`report` stale on every run while a selftest rewrote `Camellia/camellia.o` outside ninja (fixed in
  the same batch; see lib-artifacts.md).
