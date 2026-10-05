# `lib/cache` - Stamped caches: a JSON artefact keyed by the signature of its inputs, with a four-state freshness

## Purpose

Says whether a cached artefact still describes its inputs, and gives the per-file keys the existing caches are built on.

## Users

`lib.refs.dump_signature` (`content_digest`: `callers`' index and `tudiscover`'s graph), `tudiscover.dump_stamp` (`content_hash`), `undefrefs._sig` (`stat_key`); `Stamped`
for every new cache.

## Public API

* `Stamped(path, inputs, mode="content"|"stat", schema=None, base=None)`: `inputs` is a list of paths or a callable returning
  one (a discovered set); `state() in {missing, unstamped, stale, fresh}`, `load()` (the payload when fresh, else None),
  `save(payload)` (atomic), `signature()`.
* `content_hash(path, algo="sha1")`, `stat_key(path) -> [mtime_ns, size] | None`, `stat_digest(paths, base, algo="sha1")`
  (one digest over each file's base-relative name, size and mtime - `callers.py`'s dump signature until 2026-10-05).
* `content_digest(paths, base, memo=None, algo="sha1")`: one digest over each file's base-relative name and the hash of its
  bytes, so a tool that rewrites a file without changing it (dtk's split rewrites every `.s`) keeps the digest. `memo` (a
  JSON path, `{"version": 1, "files": {abs path: [mtime_ns, size, sha1]}}`) answers an unmoved stat without reading the
  file; only a file whose stat moved is re-read, the memo is rewritten only when an entry changed, a gone file's entry is
  dropped and an unreadable memo is ignored. Measured over the 415-file / 89 MB dump: ~0.2 s with every file re-read
  (sha1 0.37 s, crc32 0.28 s, sha256 0.31 s, stat-only 0.05 s over MAIN's 2 488 files / 142 MB), the stat cost when the memo
  answers.

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

* `callers.load_index` (now `lib.refs.load_index`), the asm dump's stamp (now `lib.refs.DumpStamp`, WP3c: five states, with
  `truncated`), `undefrefs.link_symbol_index` (a per-input incremental cache) and `verifyunit.target_object_snapshot` keep their
  own envelopes: moving them onto `Stamped` changes their file format (every existing dump would read `unstamped` until
  `dump_asm.py` reran), so WP3c moved the code and kept the format.
