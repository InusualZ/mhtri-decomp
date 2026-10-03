# `lib/cache` - Stamped caches: a JSON artefact keyed by the signature of its inputs, with a four-state freshness

## Purpose

Says whether a cached artefact still describes its inputs, and gives the per-file keys the existing caches are built on.

## Users

`callers.dump_signature` (`stat_digest`), `tudiscover.dump_stamp` (`content_hash`), `undefrefs._sig` (`stat_key`); `Stamped`
for every new cache.

## Public API

* `Stamped(path, inputs, mode="content"|"stat", schema=None, base=None)`: `inputs` is a list of paths or a callable returning
  one (a discovered set); `state() in {missing, unstamped, stale, fresh}`, `load()` (the payload when fresh, else None),
  `save(payload)` (atomic), `signature()`.
* `content_hash(path, algo="sha1")`, `stat_key(path) -> [mtime_ns, size] | None`, `stat_digest(paths, base, algo="sha1")`
  (one digest over each file's base-relative name, size and mtime - `callers.py`'s dump signature, byte-identical).

## Invariants and rules

* **The cache never rebuilds itself**: `--rebuild` is the caller's flag; `load()` only answers.
* `content` signs each input by sha256 of its bytes; `stat` by name, size and mtime (a large dump, where hashing would cost
  more than the cache saves). The schema is mixed into the signature, so a format change reads `stale`, never misread.
* A missing input signs as absent (a deleted input is `stale`); an edit to a file not in `inputs` leaves it `fresh`.
* The existing caches keep their on-disk formats (`callers`' `graph.json`, `tudiscover`'s `.stamp.json`, `undefrefs`'
  `link-symbols.json`): only their signature functions moved, so no cache is invalidated by this package.

## Absorbs (today's implementations)

`callers.dump_signature`, `tudiscover.dump_stamp`'s three hashes, `undefrefs._sig`.

## Lib dependencies

`lib.text` (`atomic_write`).

## Test contract

Tier: fixture (`tools/tests/lib/test_cache.py`). Each state on a temp directory; a changed, added or deleted input is
`stale`; a schema change is `stale`; an edit to a file not in `inputs` leaves it `fresh`; `stat_digest` reproduces the
`callers.py` signature exactly.

## Known gaps

* `callers.load_index`, `tudiscover.asm_stamp_status` (five states, with `truncated`), `undefrefs.link_symbol_index` (a
  per-input incremental cache) and `verifyunit.target_object_snapshot` keep their own envelopes; moving them onto `Stamped`
  changes their file format, so it rides their packages (2a refs, 3a, 3c).
