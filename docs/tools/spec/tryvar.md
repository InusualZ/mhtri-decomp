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
python tools/flags/tryvar.py -u <unit> --variants v.json --symbol S  # {name: [[old, new], ...]}
python tools/flags/tryvar.py ... --json
python tools/flags/tryvar.py --apply <name>          # LAND the winning rewrite in the real source
python tools/flags/tryvar.py -u <unit> --variants v.json --symbol S --apply-best   # run, then land the winner
python tools/flags/tryvar.py ... --rename-map pending.txt   # score as if these map renames/splits had landed
python tools/flags/tryvar.py -u <unit> --hill FN [--lines A-B] [--max-evals N] [--apply]   # hill-climb a block
python .claude/skills/mwcc-unit-matching/scripts/mt.py hill <unit> FN [--lines A-B]   # the same
python tools/flags/tryvar.py -u <unit> --permdecl FN[,FN...] [--max-lines N] [--max-perms N] [--seed N] [--plain-only] [--apply]
python .claude/skills/mwcc-unit-matching/scripts/mt.py permdecl <unit> FN[,FN...]   # the same, as the skill spells it
#   (`-u/--unit U` and `--permdecl FN` may also be spelled out; `mt.permdecl_argv` forwards each part once)
```
Flags: `--apply`, `--apply-best`, `--flags-extra`, `--hill`, `--json`, `--lines`, `--list`, `--max-evals`, `--max-variants`,
`--permute`, `--rename-map`, `--symbol`, `--unit`, `--variants`.
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

* **`--permdecl FN[,FN...]` (2026-10-07, the lanes' `permdecl.py`/`autoperm.py`).** For each function, `lib.declperm` finds the leading local declarations, initialised ones included (at most `--max-lines`, default 6; an
initialised declaration moves whole and stays after the declarations its initialiser names; `--plain-only` restores the
old reading: plain lines only, leading initialised ones skipped), every other order is compiled as a probe and ranked by that function's official score, first divergence and the unit's matched bytes, and a table is printed (best 12 rows). Above `--max-perms` (default 120) orders a fixed-seed sample is tried. The as-is source is tried first and wins every tie; the search ends at the first 100 %. The real source is never written - it is not touched even transiently, unlike the lane scripts that rewrote it per order and scored a stale object when the rewrite landed in the build's second - unless `--apply` (no name) is given, which writes each function's best order (CRLF kept) and says to rebuild. Functions are scored independently against the source as it is, not cumulatively. `--json` prints `{unit, functions: [{function, status, run, orders, sampled, tried, table, best, best_order}], applied}`; `status` is `ok`, `no-gain`, `no-run` or `ambiguous`.
  `--lines A-B` (one function) permutes that block instead of the leading run (`declperm.block_run`: every line a
  single-line declaration of one indent inside the function's body, else refused as `ambiguous`).

* **A variant is a list of exact replacements (2026-10-21, the lanes' `vary.py`/`vrun.py`/`tryv.py`).** Each `(old, new)`
  pair must occur **exactly once** in the text as it stands after the earlier pairs: an absent `old` and one that occurs
  twice are both a `skip` with the reason (`pattern occurs 2 times (want exactly one)`), never applied at a guessed
  occurrence. Three spec shapes: a `.py` `VARIANTS` list; `//@@` blocks, where a `//@@ variant <name>` repeated after a
  later `//@@ old` adds that pair to the same variant; a `.json` spec, `{name: [[old, new], ...]}` or
  `[{name, replace: [[old, new], ...]}]`.
* **The real source is never written by a run.** Each variant is a probe file beside the source (`<file>_probe<ext>`, so
  relative includes resolve), compiled with the unit's own command line into `build/tmp/probe/`; the probe is written inside
  the `try` whose `finally` removes it, so a compile failure or a Ctrl-C leaves the source byte-identical and no probe
  behind. The unit's object in `build/RMHE08/src/` is not touched either.
* **`--apply-best`** runs the selected variants (the as-is source included), prints the ranking and writes the top variant
  into the real source when it beats the as-is source by the ranking's key; a tie keeps the source. Demo (2026-10-21,
  `g3d/g3d_dcc`, `--symbol g3d_calc_tex_mtx`): as-is 95.18 %, the climbed order 95.87 %; applied, the unit rebuilt from the
  real source read 94.075 against 93.70.
* **`--hill FN[,FN...]` (2026-10-21, the lanes' `hill.py`/`nb.py`).** For a block too long to enumerate (8 declarations are
  40 320 orders): from the source's own order, every order one move away (`declperm.neighbours`: one line moved to another
  position, initialiser dependencies kept) is compiled as a probe in a fixed order, and the first one that beats the current
  order by the ranking's key (function score, first divergence, matched bytes) becomes the current order. It stops at
  100 %, at a local optimum (no neighbour better) or after `--max-evals` probes (default 200); a tie never moves. The block
  is `--lines A-B` or the leading run (at least 64 lines read). The source is only written with `--apply`. `--json`:
  `{unit, functions: [{function, status, run, range, tried, path, best, best_order, stopped}], applied}`. Demo
  (`g3d/g3d_dcc` `g3d_calc_tex_mtx`, lines 61-68): 95.18 % -> 95.87 % in 6 steps within 60 probes, about 1.6 s a probe.
* **`--rename-map FILE`** scores every mode against a copy of the target with pending map edits applied (`lib.pendingmap`:
  `old new` and `split <row> <offset> <new> [--scope S]` lines), written to `build/tmp/tryvar/<unit>.pending.o`; a refused
  line exits 1 before anything is compiled.

## Lib dependencies

units, report, text, declperm (`spec/lib-declperm.md`), pendingmap (`spec/lib-pendingmap.md`), repo.

## Test contract

`tools/tests/flags/test_mt_permdecl.py` (smoke: it loads the skill's `mt.py`) pins `permdecl_argv`: positional, spelled-out and mixed
forms; the old `-u rest[0] --permdecl rest[1]` fails the spelled-out ones.

Tier: fixture (`tools/tests/flags/test_tryvar.py`), plus `metric_selftest` for `match_pcts`. The marker and permutation
loaders, `divergence_of` on an `objdiff diff` JSON, and `try_all` over a `FixtureTree` unit with a **stub compiler** (a
Python script taking MWCC's `-o DIR -c SRC`, run through `lib.units.run_tokens`, failing on `BROKEN`) and injected
score/divergence: compile-failed and skip statuses, the ranking (score, then divergence, then name), the real source
byte-identical and no probe left. Ranking by an earlier divergence fails 2 checks; leaving the probe fails 1.
Since 2026-10-21: multi-pair marker and JSON specs and the exactly-once rule (applying at the first of two occurrences
fails 1 check); `block_run` and `neighbours`; the hill climb over a stub scorer whose optimum is two moves away (accepting
an equal-merit order fails "every accepted step is strictly better"); a `KeyboardInterrupt` mid-probe leaving the source
and the directory as they were; `best_to_apply` (a tie keeps the source) and `_apply_named` keeping CRLF.
`tools/tests/flags/test_mt_permdecl.py` also pins the `mt.py hill` alias.
Target: `tools/tests/flags/test_tryvar.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* A probe is named `<file>_probe<ext>`, so a body that prints `__FILE__` compiles a different string than the real source
  (the function's relocations still pair; a `.data` comparison would not).
* The hill climb takes the first improving neighbour, not the best one, and never accepts a sideways move: a plateau of equal
  scores stops it (`local-optimum`). `--seed`-driven restarts are not built.
* `--hill` and `--permdecl` reorder declarations only; reordering statements (the lanes' assignment-order searches) is a
  `//@@ permute` file.
