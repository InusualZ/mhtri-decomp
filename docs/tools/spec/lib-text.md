# `lib/text` - Bytes, line endings, byte-exact replace, atomic writes, anchors and transactions on shared files

## Purpose

Edits text without ever guessing a line ending: classifies endings, replaces bytes across `\n` or `\r\n`, writes atomically,
and rewrites the shared files (`splits.txt`, `configure.py`) all-or-nothing with their anchors asserted first.

## Users

`agents/edit.py` (replace/normalise/check), `units/escape.py --write`, `units/sharedfiles.py` (and through it `symedit`,
`dataqueue`, `recordmerge`), `dataclaim`, `backlog`, `agents/sync_profiles.py`, `lib.cache`.

## Public API

* `ending_counts(data) -> {crlf, lf, lone_cr}` (bare `\n`, and `\r` with no `\n`, counted apart from CRLF - `checklf.endings`); `endings(data, lone_cr=True) -> lf | crlf | cr | mixed | none` (classified from those counts); `dominant(data)`; `to_lf`; `to_ending(data, ending)`;
  `line_ending(text)`; `with_ending(text, nl)`; `read_text(path)` (`newline=""`).
* `find_matches(data, old)`, `replace_bytes(data, old, new, count=1) -> (bytes, lines)`; `MatchCountError(expected, lines)`.
* `atomic_write(path, data, append=False)`.
* `c_escape(text) -> str` (a C string-literal body), `c_unescape(text) -> bytes` (`\n \t \r \a \b \f \v \\ \" \' \0`, `\xHH`, `\NNN`; an unknown escape and a bare `\x` stay literal) - `escape.encode/decode`, now also what `edit.py replace --old/--new` reads.
* `Transaction(rename=None)`: `write(path, text|bytes)`, `rollback()`, `cleanup()`; `TMP_SUFFIX`.
* `insert_after_anchor(text, anchor, insertion, present=None)`, `append_blocks(text, blocks)`, `missing_anchors(text, anchors)`;
  `AnchorError`.

## Invariants and rules

* **The needle matches either ending; the replacement takes the span's.** Working copies mixed CRLF and LF while the index
  is LF, so a scripted `str.replace` with `\n` silently matched nothing on a CRLF file. A span with no line break takes the
  file's dominant ending, so a mixed file keeps each region's own.
* **Assert the match count**: 0 or more than `count` matches raise `MatchCountError` with the match lines; nothing is replaced.
* `endings(lone_cr=False)` is `edit.py`'s reading (a bare CR is not a break), kept so its report does not change.
* **`atomic_write`** is byte-exact (a `str` is UTF-8, never newline-translated), uses a unique temp file beside the target
  (two writers never share one), keeps the target's permission bits, and never leaves the temp file behind.
* **Shared files** (docs/plan.md 7.12): a CRLF file written back with LF silently voided two edits (the anchor stopped
  matching), so the file's own ending is kept, every anchor is asserted before a byte is written, a block already present is
  not appended twice, and a `Transaction` restores the previous bytes exactly (deleting what it created, newest first) on any
  failure; recovery never goes through the injectable `rename`.

## Absorbs (today's implementations)

`edit.replace_bytes/find_matches/classify/dominant/to_lf/to_ending/_write`, `sharedfiles.Transaction/read_text/line_ending/
with_ending/insert_after_anchor/append_blocks/missing_anchors/AnchorError/TMP_SUFFIX`, `escape.atomic_write`,
`dataclaim.write_atomic`, `backlog.write_atomic`, `sync_profiles.write`; `escape.encode/decode` and `escape --edit` (now `replace_bytes`), `checklf.endings`, `mergebranch.newline_of/restore` (`line_ending`, `atomic_write`) - WP3f.

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_text.py`). LF, CRLF, mixed and lone-CR data classify; an LF needle matches a CRLF file
and the replacement takes CRLF; a mixed file keeps each region's ending; 0/too many matches refuse with line numbers;
`atomic_write` leaves no temp file; a failed transaction restores the previous bytes exactly and removes what it created;
a missing anchor raises before any write; `append_blocks`/`present=` are idempotent; `ending_counts` separates CRLF, bare LF and lone CR; `c_escape`/`c_unescape` round-trip and keep an unknown escape literal.

## Known gaps

* `symedit._write_text`, `dataqueue.write_queue`, `sync_playbook_index.write` and `sync_profiles.nl_of/to_lf` (a lone
  CR counts there) still carry their own copy: they move with the symbols (3d) and agents packages.
