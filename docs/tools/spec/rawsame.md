# `rawsame` - Every function a unit scores 100 % on, compared byte for byte with the target, relocated operand bits masked

## Purpose

Prints the functions the report scores 100 % whose raw words or relocation types still differ from the target's. objdiff
scored two instruction rows 100 % while a register field beside a relocation differed (an `rA` next to an SDA21, an `rD`
next to an `ADDR16_HA`); only the gate's raw compare caught them.

## Users

The `decompiler` and `codereviewer` profiles (a self-check before a report), `mt.py rawsame`.

## CLI

```
python tools/objdiff/rawsame.py <unit> [<unit> ...]                # score the two objects on disk, judge the 100 % rows
python tools/objdiff/rawsame.py <unit> --report [R]                # read the scored rows from build/RMHE08/report.json (or R)
python tools/objdiff/rawsame.py <unit> --min-percent 90            # judge every function at 90 % or more
python tools/objdiff/rawsame.py <unit> --json
```
Exit codes: 0 no raw difference, 1 at least one function differs, 2 could not run (an object or the report is missing).
Output: one line per differing function (`unit: fn: +0xOFF word TARGET vs OURS; +0xOFF reloc R_PPC_REL24 vs -`, at most four
differences each) and `unit: N function(s) at 100 % or more, M with raw differences`. `--json`:
`{tool, units: [{unit, checked, differing, rows: [{name, status, target_size, ours_size, diffs, line}], error}], ok}`.

## Inputs and outputs

Reads the unit's split target object and our compiled object (and the report when `--report` is given); writes nothing.

## Invariants and rules

* Both sides must carry the same relocation type at the same word (a halfword relocation belongs to the word it sits in);
  a relocation on one side only is a difference.
* A word is compared outside the bits its relocation type owns (`lib.rawsame.MASKS`): REL24/ADDR24 the LI field, REL14 the
  BD field, `ADDR16*`/`SDAREL16`/`EMB_SDA2REL` and **SDA21 the low 16 bits only**, so a differing `rA` shows. An unknown
  type masks nothing, an unrelocated word is compared whole.
* Sizes differ: the row says so and no byte compare follows. A function missing on a side is a row.
* The default scores the two objects on disk with one `objdiff report generate`, so it can never read a stale report;
  `--report` trades that for speed and says nothing about freshness (`unitscore.py` owns the freshness policy).
* It does not compare relocation **symbols** or addends: `relocdiff.py --by-owner` is that check.

## Lib dependencies

`lib.rawsame` (`spec/lib-rawsame.md`), `lib.report`, `lib.units`, `lib.repo`.

## Test contract

Tier: fixture (`tools/tests/objdiff/test_rawsame.py`): synthetic objects with one function per case (masked LI field equal,
SDA21 `rA`, `ADDR16_HA` `rD`, relocation type, size, missing, unrelocated immediate) and a `FixtureTree` unit with a
report. Widening the SDA21 mask to the whole word fails 5 of 14 checks; dropping REL24's mask fails 4.

## Known gaps

Functions only (`STT_FUNC`); data sections are `datagap.py`'s. On the live tree (305 units with both objects, 8681 rows at
100 %) it reports 0 differences: the rows it exists to catch are rare.
