# `datagap` - Per-unit data-section gap target vs ours (flip blockers), the data-closure census (orphans the target references that no claim covers), strict/span/fold verdicts and the gate's `--touched-by` snapshot rows

<!-- generated from the module docstring of `tools/units/datagap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Per-unit data-section gap between a unit's target object and ours.

## Users

the landing gate (3); configure.py / the build (1); profiles (`.claude/agents`) (12); skills (11); docs (46); imported by `backlog`, `dataclaim`, `land`, `poolseams`, `stylelint`

## CLI

```
python tools/units/datagap.py --flip-blockers    # the actionable list: matched code, data-only gap
python tools/units/datagap.py                    # every unit with data we did not mean to emit
python tools/units/datagap.py --mode both        # both directions
python tools/units/datagap.py --unit Pl/fn_8026FFBC
python tools/units/datagap.py --json out.json    # machine-readable
python tools/units/datagap.py --pool-seams       # literal pools as TU evidence: which units are ONE original TU
python tools/units/datagap.py --selftest
```
Flags: `--all-sections`, `--allow-orphan`, `--base-ref`, `--base-root`, `--base-snapshot`, `--census`, `--flip-blockers`, `--json`, `--min-fuzzy`, `--mode`, `--no-readers`, `--pool-seams`, `--report`, `--root`, `--row`, `--selftest`, `--top`, `--touched-by`, `--unit`, `--unit-rename`, `--write-snapshot`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: obj/src objects, map, splits, report -> rows, snapshots.

## Invariants and rules

* A registered unit whose code matches can still differ in *data*: our source defines a constant, a table or a string pool the original translation unit did not have (MWCC pools its own copy of a floating-point constant, a string literal lands in `.data`, an array is emitted where retail referenced a map symbol). objdiff's unit score hides this - the extra section is simply not counted - so a unit can read 100 % fuzzy and still not be the target object.
* This tool compares the two objects' section sizes directly:
```
build/RMHE08/obj/<path>.o   the target, split out of the DOL
build/RMHE08/src/<path>.o   ours, compiled from src/
```
* and reports, per unit:
```
ours-extra    ours has bytes in a section the target does not have at all (the usual case: our source
              defines pooled data the original referenced from elsewhere)
target-extra  the target has bytes ours does not (a range we failed to claim, or data we dropped)
```
* `--flip-blockers` is the list to work: units whose **code already matches** (`fuzzy_match_percent` at or above `--min-fuzzy`, default 99) and whose only remaining defect is `ours-extra` bytes in a data section. A raw `ours-extra` listing is noisier than it looks - a unit with unwritten bodies reports `.text` ours-extra too, which is progress, not a defect. `--mode` selects the direction to report (default `ours-extra`, the direction that blocks a flip) and `--flip-blockers` narrows it to the data sections. Sections that only carry metadata (`.comment`, the string/symbol tables, `.note.split`) are ignored unless `--all-sections` is given; everything else - `.text`, `.data`, `.sdata`, `.sdata2`, `.bss`, `.sbss`, `.rodata`, `extab`, `extabindex`, `.ctors`, `.dtors` and the `.rela*` sections - is compared.

## Lib dependencies

objcompare, refs.census, project, report, findings, text.

## Test contract

Tier: fixture; smoke: `--census` over the live tree reports, never pins.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_datagap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the gate's snapshot rows (`--touched-by`, `--write-snapshot`, `--base-snapshot`) move to `dataclosure.py` (design 5)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The finding this tool exists for (2026-09-26): the data gap on the ten units that read `.text` 100 % was **ours-extra**, not target-extra - e.g. `Pl/fn_8026FFBC` emits an 8-byte `.sdata2` double the target does not have, and `ef/eft002` / `ef/fn_800FD864` emit 120 / 548 bytes of `.data`. The fix is therefore playbook 29's rule - reference the map's symbol (`extern`), never define it - not a `splits.txt` claim (there is no range to claim: the target object has no such section).
