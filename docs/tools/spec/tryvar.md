# `tryvar` - Try named source rewrites of a unit from a variants file and report the official per-function metric; `--apply` lands the winner

<!-- generated from the module docstring of `tools/flags/tryvar.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Try source rewrites of a unit and report the resulting objdiff match, per function.

## Users

skills (7); CLAUDE.md (1); docs (8)

## CLI

```
python tools/flags/tryvar.py                        # every variant of the default variant file
python tools/flags/tryvar.py --list
python tools/flags/tryvar.py <name> [<name> ...]
python tools/flags/tryvar.py -u <unit> [--variants <file.py>]
python tools/flags/tryvar.py -u <unit> --variants v.txt --symbol S   # `//@@` blocks, ranked by S
python tools/flags/tryvar.py -u <unit> --permute order.txt --symbol S  # every other order of a declaration run
python tools/flags/tryvar.py ... --json
python tools/flags/tryvar.py --apply <name>          # LAND the winning rewrite in the real source
```
Flags: `--apply`, `--flags-extra`, `--json`, `--list`, `--max-variants`, `--permute`, `--symbol`, `--unit`, `--variants`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit + variants/<lib>.py -> table, src edit.

## Invariants and rules

* The harness is generic: it writes the rewritten source as a probe file beside the unit's source (`<file>_probe<ext>`, so
  relative includes resolve), compiles it with the unit's exact ninja command line into `build/tmp/probe/`, removes the
  probe whatever happens, and diffs it against the split target object. The real source is never modified and the unit's object is never clobbered. Variants live in a separate data file, so the rewrites for one unit do not leak into the tool.
* The per-function number this prints is the **official report metric** (`report generate`'s `fuzzy_match_percent`, via `lib.report.score_entries`) - the same number `build/RMHE08/report.json`, `ledger.py` and `land.py` read. objdiff's explicit `diff` mode is deliberately not used for the score: it defaults `functionRelocDiffs` to `data_value` (the report defaults to `none`, so relocation-only differences counted as mismatches there) and its `match_percent` is a different normalisation (measured on this repo: `pl_skill` fn_80270018 reads 99.88 % positionally and **100.0 %** officially).
* A variant file (default: `tools/flags/variants/<lib>.py`, i.e. next to this script, named after the unit's library) defines:
```
VARIANTS = [(name, repls), ...]
```
* where `repls` is either a list of `(old, new)` string pairs or a callable `src -> src` (returning `None` means "the pattern did not match").
* **Text variant files and permutations (2026-10-04, the lanes' `vary.py`/`perm.py`).** A `--variants` file that is not
  `.py` is `//@@` blocks: `//@@ old` holds text that must occur in the source, each following `//@@ variant <name>` its
  replacement (a body loses its one trailing newline). `--permute FILE` takes `//@@ item` blocks that, concatenated in
  file order with a newline after each, are one contiguous run of the source, and tries every other order of them
  (refused above `--max-variants`, default 120, rather than compiling thousands). Both modes try the unmodified source
  first as `(as-is)` and end with a ranking.
* **The ranking (`--symbol S`).** Best first by S's official percent, then by S's first divergence - the index of the
  first target instruction objdiff's `diff` gives a `diff_kind` (`lib.report.diff_rows`, read as a position, never as a
  score); a later divergence ranks higher and a full match highest - then by the unit's matched bytes (`sum(pct x
  target size)`). Without `--symbol`, matched bytes alone. Equal rows are reported equal: on `main/mh3_pad`
  `fn_800417F0` three index rewrites all read 94.79 % with the first divergence at row 6, the `extsh` retail emits.
* `--apply <name>` writes that variant's rewrite into the unit's real source file (preserving its line endings), so a win becomes progress on the unit instead of staying an experiment. It refuses to write anything unless the rewrite applies cleanly and actually changes the file. Rebuild and re-measure afterwards - the recorded evidence must come from the real source, not from the probe.

## Lib dependencies

units, report, text.

## Test contract

Tier: fixture (`tools/tests/flags/test_tryvar.py`), plus `metric_selftest` for `match_pcts`. The marker and permutation
loaders, `divergence_of` on an `objdiff diff` JSON, and `try_all` over a `FixtureTree` unit with a **stub compiler** (a
Python script taking MWCC's `-o DIR -c SRC`, run through `lib.units.run_tokens`, failing on `BROKEN`) and injected
score/divergence: compile-failed and skip statuses, the ranking (score, then divergence, then name), the real source
byte-identical and no probe left. Ranking by an earlier divergence fails 2 checks; leaving the probe fails 1.
Target: `tools/tests/flags/test_tryvar.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
