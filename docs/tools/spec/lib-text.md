# `lib/text` - Bytes, line endings, byte-exact replace, atomic writes, anchors and transactions on shared files

## Purpose

Bytes, line endings, byte-exact replace, atomic writes, anchors and transactions on shared files.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `endings(data) -> {lf, crlf, mixed, cr}`, `dominant(data)`, `to_lf`, `with_ending(text, nl)`, `line_ending(text)`
* `replace_bytes(data, old, new, count=1)`: the needle matches across `\n` or `\r\n`; the replacement takes the ending of the span; 0 or more than `count` matches refuses with line numbers
* `atomic_write(path, data, append=False)`; `Transaction(paths)` temp + `os.replace`, exact restore on failure
* `insert_after_anchor(text, anchor, insertion, present=None)`, `append_blocks(text, blocks)` (idempotent), `missing_anchors(text, anchors)`

## Absorbs (today's implementations)

`edit.replace_bytes/classify/dominant`, `sharedfiles.Transaction/line_ending/insert_after_anchor/append_blocks`, `escape.atomic_write`, `symedit._write_text`, `dataqueue.write_queue`, `backlog/dataclaim.write_atomic`, `sync_profiles.nl_of/to_lf`, `mergebranch.newline_of`

## Test contract

Tier: fixture (a lib test never reads the live tree). LF, CRLF and mixed files round-trip; a failed transaction restores the previous bytes exactly; anchors are asserted before any write

## Known gaps

None until implemented; `migration.md` names the package.
