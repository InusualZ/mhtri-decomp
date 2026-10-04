# `pairgap` - Per unit: symbols objdiff declines to pair (size gap), missing on our side, extra on ours, cross-checked with the report score

<!-- generated from the module docstring of `tools/objdiff/pairgap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Pair-gap report: the symbols a unit's object defines against the ones its target object has.

## Users

profiles (`.claude/agents`) (2); docs (4)

## CLI

```
python tools/objdiff/pairgap.py --summary                  # every unit: counts per class
python tools/objdiff/pairgap.py -u g3d/fn_80075DCC         # one unit, every class
python tools/objdiff/pairgap.py --mode gap                 # only the size gaps, whole tree
python tools/objdiff/pairgap.py --mode gap --threshold 25  # a looser gap
python tools/objdiff/pairgap.py --sections all             # include .data/.sdata2/sdata
python tools/objdiff/pairgap.py --json out.json            # machine-readable
python tools/objdiff/pairgap.py --selftest
```
Flags: `--all-sections`, `--check`, `--json`, `--limit`, `--match`, `--mode`, `--no-report`, `--report`, `--root`, `--sections`, `--selftest`, `--summary`, `--threshold`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: obj/ + src/ objects, report.json -> rows.

## Invariants and rules

* The register's top-requested feedback item (8 filers, `tools/units/tooling.py`, kind `tooling`):
```
Teach objdiff (or the report) to pair symbols with a >50 % size gap: it declines them, so they
read as 0 % and hide real unpaired code.
```
* `objdiff-cli` is a downloaded third-party binary, so it cannot be taught anything - but its *report* is what every lane reads, and the report never prints our size. A symbol whose body is a wildly different size therefore lands in the same place as a body nobody has written yet: a row at 0 % (objdiff omits the `fuzzy_match_percent` key when nothing matched - 12,777 of the 20,507 rows in this tree's report) or a row at a fraction of a percent that reads as "untouched". This tool finds those symbols and says so.
* **What it measures.** For one unit (or every unit the report knows), it reads the two split objects -
```
build/RMHE08/obj/<unit>.o   the target, split out of the DOL
build/RMHE08/src/<unit>.o   ours, compiled from src/
```
* takes the symbols each one defines, and reports three classes:
```
size-gap   defined on BOTH sides, sizes apart by more than `--threshold` (default 50 %).  objdiff
           pairs these by name, so they are neither unpaired nor absent: they are the wrong size.
missing    defined by the TARGET and not by our object (a body we have not written/emitted).
extra      defined by OUR object and not by the target (a static, a helper or a table the original
           translation unit did not own).
```
* For every row it prints the name, both sizes, the size delta, the section and the report's score for that symbol - so the human sees the gap immediately and can tell "0 % because unwritten" from "0 % because the size is wrong".
* **Only the size delta is this tool's own arithmetic.** The `report` column is a *cross-check* read out of `build/RMHE08/report.json`, which is an **order-only** target of `all_source`: after a source edit `ninja build/RMHE08/report.json` answers "no work to do" and you read the previous build's scores. The tool compares the report's mtime against the objects and sources it scanned and prints which build it is reading, so a stale number is visible rather than believed. `--no-report` skips the cross-check.
* `tools/units/verifyunit.py`'s `size_gap_problems` is the gate-side cousin of this tool: it refuses at landing time a *claimed* pair that reads as untouched. It reports only the no-key rows and only for the unit being verified; this tool is the discovery side - every unit, every class, both directions, plus the report cross-check - and it never refuses anything.
* Section scope: `.text`/`.init` by default. `.data`/`.sdata`/`.sdata2`/`.rodata`/`.bss`/`.ctors`/`.dtors` byte gaps are `tools/units/datagap.py`'s job (it compares whole sections, including sections the target does not have at all); `--sections data|all` here is for the symbol-level view of the same bytes. The metadata sections (`.comment`, `.strtab`/`.symtab`, `.note.split`, `.rela*`) are always excluded unless `--all-sections` is given.

## Lib dependencies

objcompare (`defined_symbols`, `symbols`, `section_kind`); `lib.repo.repo_root`.

## Test contract

Tier: fixture.
Today's selftest (`tools/objdiff/pairgap_selftest.py`): `tools/objdiff/pairgap.py --selftest` is the same set of checks; this half exists so the discovery in `tools/selftest.py` sees a standalone `*_selftest.py` beside the tool and collapses the two into one entry (it detects the delegation below and runs the tool, never both).
Target: `tools/tests/objdiff/test_pairgap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The one number that can be claimed.** The three classes are facts about two ELF files and are exact. What objdiff *does* with a pair is not ours to state, so it was measured (2026-09-28, this tree). objdiff **does** emit a report row for a >50 %-gap pair - it pairs by symbol name and does not decline on size - and of the 146 such pairs in the whole tree **125 carry a `fuzzy_match_percent` that is exactly one matched instruction** out of the target's (`612 B` vs `4 B` -> `1/153` = `0.6535948 %`), **16 carry a handful** (1.4 to 182.9 instructions: a body that is genuinely partial, `ef_cube`'s `fn_800CA200__FUiP2EmP2PmUiUiPvUsUif` at 5960 B vs 760 B reads `12.27 %`), and **5 carry no key at all**
* `0 %`. So the filer's "it declines them" is the *symptom* (a row that reads as untouched), not the mechanism; either way the fix is the same one this tool performs, because nothing in the report distinguishes "0 % because unwritten" from "0 % because the body is the wrong size". `--limit 0` on `--mode gap` is the answer to that question for the whole tree in under a second.
