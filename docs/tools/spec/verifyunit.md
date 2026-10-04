# `verifyunit` - Independent verification for the gate: registration completeness (3 axes), per-symbol re-measure that does not read the report it audits, size-gap rows, split-target drift

<!-- generated from the module docstring of `tools/units/verifyunit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Independent verification for a landed batch: registration completeness, a per-symbol re-measure that does not read the report it audits, and split-target-object drift.

## Users

the landing gate (2); profiles (`.claude/agents`) (2); skills (1); docs (3); imported by `land`, `rescue`, `unitscore`

## CLI

```
python tools/units/verifyunit.py <unit> [<unit> ...]        # manual run against this tree
```
Flags: `--main`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: configure.py, splits, build.ninja, objects, report -> problems.

## Invariants and rules

* Three failures got through the previous gate, and each of them has a check here.
* **A unit registered in name only.** `hud/fn_80334568`'s source was committed while its `configure.py` `Object(...)` line and its `splits.txt` block were left in the index, so the unit existed as a file and *not* in the build. `ninja build/RMHE08/ok` stayed green because a `NonMatching` object is never linked, and the compile gate could not see it either: a unit with no `configure.py` line has no `build/RMHE08/src/<unit>.o` target to scope `ninja -k 0` to. `registration_problems` refuses it: every batch unit must have (a) an `Object(...)` line, (b) a `splits.txt` block, and (c) an object target in the build graph.
* **A measurement taken on trust.** The regression gate reads `build/RMHE08/report.json`, the very file the batch was measured against - so a stale or wrong report is invisible to it. The one lane that did it right (the 8031A6C0 merger) re-derived everything: both objects bit-identical, the split target object unchanged pre/post merge, and all 26 symbols re-scored from the objects. `verify_units` is that independent path: it re-runs `objdiff report generate` itself over the target/candidate objects and compares the result symbol-for-symbol against `report.json`, and it reads the objects directly to check the score against the bytes. `target_drift_problems` adds the merger's strongest form: a `splits.txt` change that re-ranges a neighbour moves that neighbour's split target object, and a neighbour the batch does not name is refused rather than assumed harmless.
* **A measuring tool that lied.** `recompile.py --measure` understated every score for months because it left ninja's chained `objalign` argument relative. `measure.py`'s selftest is the pattern copied here: the independent run cross-checks against `report generate`'s own output (and against the raw object bytes) instead of trusting itself, and `verifyunit_selftest.py` pins each check against a fixture that must refuse.
* ## which `objdiff-verify` (SKILL.md) checks the gate now performs
* The gate (through this module) now covers these documented checks; they no longer need to stay manual:
* Still manual (deliberately not in the gate): naming/home decisions (§2), the symbol→unit lookup (§4 preamble - it greps `symbols.txt`), the instruction-level `diff_kind` reading (§4.3 - diagnostic, not a score), the flag hypotheses from `extab`/`.comment` (§5.5-5.6 - a hunch must not refuse a batch), and the write-up (§6).

## Lib dependencies

objcompare (`fingerprint`, `symbol_locations`, `symbol_rows`), report, project, units; tool API: `measure.score_report`.

## Test contract

Tier: fixture; smoke: the live cross-check when objdiff and a built unit exist.
Today's selftest (`tools/units/verifyunit_selftest.py`): Each check is pinned against a fixture that must **refuse** (or, for the happy path, must pass), so the failure the check exists for is reproducible rather than described: * a unit registered in name only (a source with no `Object(...)` line, no `splits.txt` block, and no object target in the build graph) must refuse - and one registered on all three axes must pass; * a per-symbol score that is not reproducible from a fresh `report generate`, and a 100 % claim whose bytes are not identical, must refuse; * a function with no `fuzzy_match_percent` key must be read as 0 %, and the unit arithmetic that proves it must refuse when the two readings disagree; * a split target object that moved for a unit the batch does not name must refuse (a neighbour the `splits.txt` change re-ranged), while the batch's own unit may move; * a row whose name is dtk's own (`pad_*`, for a range with no function prologue) resolves to our symbol at the same section and offset, passes when the bytes match, and still refuses when they do not - the two directions the `worker/trk-init-vectors-2226` refusal turned on.
Target: `tools/tests/units/test_verifyunit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **§1/§3 rebuild the unit** - the compile gate (`ninja -k 0`, scoped) is the rebuild; this module then re-runs `report generate` over the freshly built object pair.
* **§4.5 a symbol present on one side but absent from the other is a mismatch** - `size_gap_problems` names every symbol present on both sides that objdiff declines to pair (a >50 % size gap), which is exactly the row that otherwise reads as untouched.
* **§5.1 sizes first** - a symbol the report scores 100 % but whose sizes differ is refused (`symbol_problems`).
* **§5.2 per-symbol, and a function with no `fuzzy_match_percent` key is 0 %, not 100 %** - `_score()` reads an absent key as 0, and `arithmetic_crosscheck` proves that reading by reproducing the unit's `fuzzy_match_percent` from its listed partials; a mismatch refuses.
* **§5.3 cross-check the arithmetic** - `arithmetic_crosscheck` is that identity.
* **§5.4 data/byte content, not just size** - `raw_symbol_rows` byte-compares every symbol, so a report 100 % whose bytes differ refuses even when the size matches.
* **a dtk-generated row name is resolved by ADDRESS, not by name** - `dol split` names a range it cannot attribute `pad_*`/`auto_*` (the TRK interrupt vectors have no function prologue, so the target object carries `pad_00_80004380_init` for the bytes the map calls `gTRKInterruptVectorTable`, and the label cannot win: an extent on it makes `dtk dol split` fail on the overlap). objdiff pairs by name, so such a row is never paired and *no* report can score it - the row is matched to our symbol at the same section+offset and judged by its bytes, which is what it actually claims. Related, and the reason that byte rule is load-bearing rather than decorative: `Object(Matching, ...)` sets `metadata.complete` in `objdiff.json`, and objdiff-cli then pins that unit's completion percent at 100 **while still running its per-symbol diff** (measured 2026-09-28: a corrupted `.init` object keeps `complete_code_percent` at 100.0 while its fuzzy percentage falls), so the completion field of a Matching unit's report row is a claim the objects themselves must back.
* **§7 a report number that disagrees with an object diff is a stale report** - `verify_units`' symbol-for-symbol comparison of a *fresh* `report generate` against the committed `report.json`.
* **the merger's regression proof (`.pi/notes/8031a6c0-fn-8031a6c0-e199.md`)** - per-symbol reproduction from the objects plus split-target-object pre/post comparison.
