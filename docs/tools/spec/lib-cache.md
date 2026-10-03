# `lib/cache` - Stamped caches: a JSON/pickle artefact keyed by the signature of its inputs, with a four-state freshness

## Purpose

Stamped caches: a JSON/pickle artefact keyed by the signature of its inputs, with a four-state freshness.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Stamped(path, inputs)`: `state() in {missing, unstamped, stale, fresh}`, `load()` (None unless fresh), `save(obj)`; signature = sha256 of small inputs, `(path, size, mtime)` for a large dump
* `--rebuild` is the caller's flag; the cache never rebuilds on its own

## Absorbs (today's implementations)

`callers.dump_signature/load_index`, `tudiscover.dump_stamp/asm_stamp_status`, `undefrefs._sig/link_symbol_index`, `verifyunit.target_object_snapshot`

## Test contract

Tier: fixture (a lib test never reads the live tree). each state on a temp directory; a changed input is `stale`; an edit to a file not in `inputs` leaves it `fresh`

## Known gaps

None until implemented; `migration.md` names the package.
