# `lib/trialrun` - The measure a source edit is judged by: the files a trial may write, one scored compile, the keep verdict

## Purpose

One implementation of "write a candidate to the unit's real source, compile it into a scratch directory, score it with the
official report and keep it only when the unit's matched bytes rise with no row down, else put the file back byte for
byte". `tools/units/unittry.py` and `tools/units/applyorder.py` both stand on it, so a kept edit means the same thing in
both.

## Users

`tools/units/unittry.py` (the spec trials), `tools/units/applyorder.py` (regsim's declaration orders).

## Public API

* `Workspace(allowed)`: the files a run may write (`text`, `write`, `accept`, `restore(original=)`, `changed`); a write
  keeps the file's line endings, a restore of an unchanged file gives back its bytes and mtime.
* `Measurer(cmd, cwd, objdir, target, score, ...)`: one compile into `objdir` (never the real object) and one
  score; `refresh()` rebuilds the unit's real object and rewrites `build/RMHE08/report.json` after a kept edit.
  `Measurement(ok, functions {name: (percent, size)}, fuzzy, ...)`, `.matched` the sum of percent x size.
* `compare(before, after)` / `judge(before, after, sdata2=)`: `(keep, why, up, down)`; keep needs a compile, no row
  lower and matched bytes strictly up (`EPS`).
* `TrialRunner(ws, measure, unit, root, out)`: `baseline()`, then `evaluate(trial, name, {path: lf text}, keep=)`, which
  writes the candidate, measures, judges and keeps it (the new accepted state) or restores; `rows` holds a `Row` each.
* `build_measurer(unit, root, rename_map=None, sdata2=False, out=)`: the scorer's own compile command and target for a unit.

## Inputs and outputs

Reads the unit's source and objects; writes the unit's source (restored), the scratch object and report under
`build/tmp/unittry/<unit>/`, and on `refresh` the real object and `report.json`.

## Invariants and rules

* A stale object is never scored (it must be newer than the trial's start); a failed compile is a `failed` row, not a gain.
* Nothing but the files named in `Workspace(allowed)` is written.

## Lib dependencies

report, units, pendingmap, repo, text, binary.elf.

## Test contract

Tier: fixture - `tools/tests/units/test_unittry.py` (the keep-only-when-better verdict, the byte-for-byte restore after a
losing candidate, a failed compile and an interrupt, the scratch object, the refresh) and `test_applyorder.py` (a
`TrialRunner` keeping and restoring a CRLF source).

## Known gaps

None beyond `unittry`'s: the judge is per unit, and `.sdata2` order is checked only with `sdata2=True`.
