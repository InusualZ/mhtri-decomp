# `recompile` - Compile one unit without ninja from any worktree with MAIN's real command line (worktree includes first), prove the object fresh, `--measure` one symbol with the official metric; resolves proposal targets

<!-- generated from the module docstring of `tools/units/recompile.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Compile one unit **without ninja**, from any worktree, and prove the object is fresh.

## Users

the landing gate (5); the selftest runner (1); profiles (`.claude/agents`) (8); skills (1); docs (133); imported by `brief`, `claims`, `dossier`, `handoff`, `land`, `measure`, `queue`, `slots`, `undefrefs`

## CLI

```
python tools/units/recompile.py <unit> [--measure <symbol>] [--json] [--dry-run] [--selftest]
```
Flags: `--dry-run`, `--json`, `--main`, `--measure`, `--selftest`, `--source`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: MAIN build.ninja, src -> object, score.

## Invariants and rules

* What it does instead:
* resolves **MAIN** with `git worktree list --porcelain` and takes the toolchain, the include path and the *target* object from there;
* takes the real command line from MAIN's ninja (`ninja -t commands`), rewrites the three paths that must change (the source, the `-o` directory, the `-i` search path), and runs it with MAIN as cwd;
* puts the worktree's `-i` directories **first** and points every one of MAIN's at the worktree's copy of that directory, so a worker's edit to an existing shared header is the header that gets compiled (see `order_includes`; appending them - the old behaviour - left MAIN's copy first and silently measured the wrong source, which is what the `eft004` round had to work around with a scratch measurer);
* deletes the object first, then asserts the file exists and its mtime moved — a stale object is impossible;
* prints the two object paths, the section sizes, and can measure a symbol **with the same objdiff code path the official report uses** (`report generate` on a one-unit project), so the number equals `build/RMHE08/report.json`'s `fuzzy_match_percent` for the same object.
* A registered unit run from MAIN takes exactly the path it took before (MAIN's rule, MAIN's object). A unit run from a worktree that has its own copy - the filed double-take - takes **that** copy, and the CLI prints the resolved absolute path with its kind (`[worktree-split]`, `[registered]`, `[auto-fallback]`) **and the tree it came from**, so a measurement is never ambiguous about which tree it came from. The score is still `report generate`'s `fuzzy_match_percent`.
* **`--measure` prints the provenance of the number it reports.** The tree the invocation resolved in (its cwd), the target object it compared against (**path and mtime**), the object it compiled (**path and mtime**), and the map - then a `WARNING` when the target is MAIN's while the cwd is a worktree, because that score is MAIN's and a reader must not have to infer it from an absolute path. `--json` carries the same facts under `provenance`. This is the half a reader can check *after* the fact; `split_staleness` is the half that refuses before it. It re-derives nothing: `compile_unit` already deletes the object before compiling and asserts it reappears, so the printed `compiled_mtime` is a provably fresh file.
* **A stale split is refused, not silently measured (F40).** Preferring this tree's object is only safe while this tree's split actually reflects its own `symbols.txt`/`splits.txt`/DOL. A lane that edits its `splits.txt` (a seam re-draw, a new registration) and has **not** re-split still has the previous build's object on disk, so the "this tree's copy" the resolution just preferred is the *old range's* bytes - and MAIN's copy is the same old range, so falling back is not a fix either. `split_staleness` reuses the seeder's own guard (`claims._build_is_current`, the one `slots.verify` uses) and then asks which split input is both newer than this tree's `build/RMHE08/config.json` **and** an uncommitted edit to this tree (`git diff --quiet HEAD`), and the CLI refuses with the file and both mtimes named. The dirty test is load-bearing, not decoration: a fresh worktree's tracked files are all written at checkout time while `build/` keeps MAIN's mtimes, so `_build_is_current` is False in **every** fresh worktree and a pure-mtime rule would refuse every measurement. `--allow-stale-split` is the deliberate override.
* **The map follows the invocation too (F43).** The fallback locates the retired `auto_*text.o` by *address*, and the address comes from `config/RMHE08/symbols.txt`. Reading MAIN's copy alone made the tool refuse a branch that had renamed a symbol - "a symbol this branch renamed has no entry there" - because MAIN has never carried the new spelling. `resolve_map` applies `resolve_target`'s discipline to the map: the invocation tree's copy first, MAIN's as the fallback, and the address lookup merges both (the branch supplies the new name, MAIN the old one that the retired object is named after). The CLI prints the map it read with its kind, so a measurement is unambiguous about the map as well as the object.
* `<unit>` is the path from the repository root, e.g. `Pl/pl_act`, `main.cpp`, `auto/80040598_fn_80040598`.

## Lib dependencies

units, repo, report, proc, binary.

## Test contract

Tier: fixture (fake MAIN/worktree trees, fake runners); smoke: the integration cross-check against report generate.
Today's selftest (`tools/units/recompile_selftest.py`): The contract this pins is the one that was silently broken: `--measure` must return the *report* metric (`report generate`'s `fuzzy_match_percent`), not objdiff-cli's explicit-diff `match_percent`. The two are different normalisations, and `diff` additionally defaults `functionRelocDiffs` to `data_value` where the report defaults to `none`, so the old number was lower than the official one and sent workers chasing regressions that did not exist (`RSOStaticLocateObject`: 99.28205 vs 99.64103). Three layers:
Target: `tools/tests/units/test_recompile.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The problem this solves (docs/plan.md 7.1 + 7.15): a worker lives in its own git worktree, which has no `build.ninja`, no `objdiff.json` and no `build/RMHE08/` — and `unitutil` resolves all three from its own location, so `ninja build/RMHE08/src/<unit>.o` and `mt.py` simply do not work there. Two further traps it closes: MWCC writes the object into the *directory* named by `-o` (a hand-written command can silently compile nothing), and the filesystem's one-second mtime granularity makes a recompile of an unchanged file look like "no change" to anything that caches on (mtime, size).
* The measurement trap this closes (`--measure` used to lie by ~0.36 points on `RSO/runtime`, which sent a worker chasing a regression that did not exist): objdiff-cli's explicit `diff` mode is **not** the report's metric. Two differences compound - `diff` defaults `functionRelocDiffs` to `data_value` while `report generate` defaults to `none` (so relocation-only differences count as mismatches), and even at the same setting the diff JSON's per-symbol `match_percent` is a different normalisation from the report's `fuzzy_match_percent`. `report generate` over a one-unit project is the only path that is the report by construction, and it costs ~0.04 s. `--measure` calls the *same* `unitutil.report_measure` primitive `tools/units/measure.py` scores a whole unit with, so there is one implementation of the metric and the two fronts cannot drift.
* **A proposal unit measures too** (CLAUDE.md, "the proposal-unit measurement gap"). A worker registers a fresh proposal in its own worktree first (`configure.py` + `splits.txt`, per the brief) and MAIN has neither a ninja rule nor a split object for the range until that registration lands. That used to be the end of `--measure`; three workers hand-built a harness each (borrow a sibling's command line, score against the retired `auto_*_text.o`, one spent 87 turns on it). Both halves are now the tool's own path, and neither MAIN's config nor the worktree is written:
* the **command line** comes from MAIN's ninja (registered), the worktree's ninja (if the worker generated one), or - last - a registered sibling in the *same `config.libs` block* of the worktree's `configure.py`, with only the source, the `-o` directory and the `-lang` token pointed at this unit. Those are the flags `project.py` emits for that lib, not a hand-rolled approximation;
* the **target object** is resolved from the invocation's own tree outward (`resolve_target`): the worktree's split object first, then MAIN's, then the retired `auto_*_text.o` that owns the symbol's address in whichever tree has it - the same original bytes the split will put in the registered object (`auto_<symbol[:20]>_text.o` for a single symbol, else the `auto_<nn>_<address>_text` run that covers it).
