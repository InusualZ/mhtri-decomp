# `textorder` - Compare a unit's `.text` function order with the target's and print the first symbols out of order

## Purpose

Show where our object's function order departs from retail's, and which target functions look like deferred inline emissions.

## Users

flip and unit lanes working a unit whose rows are 100 % but whose `.text` order or layout still differs.

## CLI

```
python tools/units/textorder.py <unit> [--limit N]     # e.g. g3d/g3d_anmchr
python tools/units/textorder.py <unit> --refs          # each target function, tagged D/R, with the target functions it calls
```
Exit codes: 0 same order and same function count, 1 a difference.

## Inputs and outputs

Inputs: `build/RMHE08/obj/<unit>.o` (target) and `build/RMHE08/src/<unit>.o` (ours), read with the build's `powerpc-eabi-objdump`.
Output: counts (target, ours, out of order, missing), the ours-only symbols (weak copies the link drops), then the first N positions where the common functions disagree.

## Invariants and rules

* The order is the `.text` symbol addresses of the object, functions only.
* `--refs` tags a target function `D` when a function earlier in the object first references it through a `bl` relocation (a deferred-inline candidate: helpers are emitted right after the function that first references them), `R` otherwise. The retail order of a `D` chain shows the last-in first-out reference order.
* Names only in our object are reported separately and never counted as out of order.

## Lib dependencies

`lib.binary.objdump`.

## Test contract

Tier: fixture. `tools/tests/units/test_textorder.py` runs the parsers and the comparison on objdump text fixtures.

## Known gaps

None recorded.
