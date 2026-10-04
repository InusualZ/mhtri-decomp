# `variants` - Per-unit source-rewrite data for `tryvar.py` (`tools/flags/variants/<lib>.py`)

## Purpose

`tryvar.py` is generic; a file under `tools/flags/variants/` is one unit's *data*: the source rewrites tried against
that unit's remaining difference. A file is kept as the record of what was tried (`docs/tools/retired.md`, "Kept but
dormant"), so nobody re-runs a rejected candidate.

## Users

`tools/flags/tryvar.py --variants tools/flags/variants/<lib>.py`.

## CLI

None (data).

## Test contract

None of its own: `tryvar.py` loads it.

## Moved from the module docstring (WP6)

From `tools/flags/variants/camellia.py`:

The harness (`tryvar.py`) is generic; this file is unit-specific *data*: the rewrites that were tried
against the one remaining difference in `camellia_setup256` (an extra 4-byte stack slot) and the small
regex helpers they use.

Each entry is `(name, repls)` where `repls` is either a list of `(old, new)` string pairs or a callable
`src -> src` (returning `None` means "the pattern did not match here").

Status: **every variant in this file is a rejected candidate** - all of them either reproduce the
baseline byte-for-byte (the front end is insensitive to statement shape) or make the code worse. The
surviving hypothesis and the full analysis are in `.pi/notes/camellia-match-process.md` (finding #14).
