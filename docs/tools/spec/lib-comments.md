# `lib.comments` - The comment vocabulary rule 15 and the comment sweep share: stale-path markers, narrative markers, the stale judgement

## Purpose

Holds the hand-curated stale-path markers and narrative markers once, and decides whether a marker hit is a stale path
(a path the tree has is live). One implementation for section 6.5 rule 15 (`stylelint_rules/r15_comments.py`) and the
comment sweep's census (`sweepcomments.py --list-stale`, `--markers`).

## Users

`tools/units/stylelint_rules/r15_comments.py`, `tools/units/sweepcomments.py`.

## CLI

None (library).

## Inputs and outputs

Pure: a text and an `exists(path) -> bool` callback in, `Hit(marker, token, pos)` rows out. Reads and writes no file.

## Invariants and rules

* `STALE_MARKERS`: `include/`, `src/auto`, `auto/<hex>_`, `proposal/`, `.pi/`, `docs/splits/phase4` (not
  `homebutton-carried-notes.md`) and the retired tool names (`attribute.py`, `applysplits`, `dataattach`,
  `matchinggain`, `promote.py`, `promote_batch`, `herdr`, `applybranch`, `union.py`, `mergelane`). Hand-curated: a
  marker is added when a comment cites a retired name, never derived from `docs/tools/retired.md`.
* `stale_hits(text, exists)`: every marker hit in marker then text order, minus a hit whose path token `is_live` - it
  holds a `/`, `exists` says the tree has it, and it is not under `NEVER_LIVE` (`include/`, `proposal/`, `auto/`,
  `src/auto`, `docs/splits/phase4`, `.pi/`). `.pi/` is never live because it is gitignored scratch: whether it exists
  depends on the checkout (MAIN or a worktree), and a verdict must not.
* The token is the whole path around the hit (quoting and trailing punctuation stripped; a leading `.` kept, so
  `.pi/notes/x.md` stays itself), or the matched text when the run holds no `/` (a retired tool named bare).
* `HISTORY_MARKERS` (`phase 4`, `next pass`, a date, `round N`, `pilot`, `wave`, `lane`, `header inherited`):
  advisory - game text can say "round" or "lane". `marker_hits(text)` returns every hit.
* `comment_only(text)`: the text with code and literals blanked, length and newlines kept (`lib.cscan.spans`). The
  census reads `lib.cscan.strip`'s comment view instead (which keeps code), so its counts are unchanged.

## Lib dependencies

`lib.cscan`.

## Test contract

Tier: fixture, `tools/tests/lib/test_comments.py` (the tokens, live and never-live paths, the homebutton exception, a
nested `include/` directory, marker order, the comment-only view); the census through
`tools/tests/units/test_sweepcomments.py`; rule 15 through `stylelint.py --selftest`. Moving the data here left
`sweepcomments --list-stale` and `--markers` byte-identical (`--scope src` and `all`, `--json`).

## Known gaps

* The markers are fixed strings; a stale path spelled another way (`inc/`, a renamed tool not in the list) is not seen.
