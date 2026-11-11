# `extabdiff` - Compare a unit's exception-table records with the target's, one function at a time

## Purpose

Name the functions whose `extab` record differs from retail's, with both sizes, so a flip blocker in `extab` is a per-function list.

## Users

flip and unit lanes closing the `extab`/`extabindex` blocker `flipcheck` reports.

## CLI

```
python tools/units/extabdiff.py <unit>     # e.g. g3d/g3d_anmchr
```
Exit codes: 0 every record identical, 1 a difference.

## Inputs and outputs

Inputs: `build/RMHE08/obj/<unit>.o` (target) and `build/RMHE08/src/<unit>.o` (ours), read with the build's `powerpc-eabi-objdump`.
Output: one line per differing function (`<function>: target N B, ours M B|missing`), then `differing X of Y`.

## Invariants and rules

* Each `extabindex` entry names its function (relocation at +0) and its `extab` record (relocation at +8); records are paired through those relocations, so the comparison is per function and independent of record order.
* A bigger record of ours is usually an extra cleanup state: a block-scoped local with a destructor, or the assert macro's flag locals; doing the pointer test through a `static inline` helper that declares its locals in reverse and initialises in order has closed such a record.
* A function with no record on our side is `missing`.

## Lib dependencies

`lib.binary.objdump`.

## Test contract

Tier: fixture. `tools/tests/units/test_extabdiff.py` runs the parsers, the pairing and the difference report on objdump text fixtures.

## Known gaps

None recorded.
