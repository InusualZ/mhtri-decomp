# `tywidth` - Sweeps the integer width of a function's locals, skipping widths that cannot hold the values the local takes

## Purpose

Tries `s16`/`u16`/`u8`/`s8`/`s32`/`u32` for each integer local of one function, measures each with the official per-symbol
metric and keeps the best width. A range guard refuses a width before it is compiled when a constant assigned to or compared
with the local, or the declared width of an expression stored in it, does not fit; a kept width must also leave the
function's size equal to the target's or closer to it.

## Users

Lanes raising a partial row with a width sweep (the retired `.pi/tmp/tyall.py` run unguarded and kept `u8 base_c = 0x400`,
`u8 help` taking 0x1EE..0x1F9 and `u8 id = 0x2092` because they scored higher).

## CLI

```
python tools/flags/tywidth.py <unit> <function> --plan              # per local: widths it may take, and why the others are skipped
python tools/flags/tywidth.py <unit> <function> [--dry-run]         # sweep; the kept widths are written (--dry-run restores)
python tools/flags/tywidth.py <unit> <function> --loose-sources     # check literal constants only, not stored expressions' widths
python tools/flags/tywidth.py --rows remaining_functions.csv --out FILE   # the batch form: every partial row in turn
```
Exit codes: 0 ok, 2 usage. `--plan` doubles as an audit of widths already in the source: the current type is listed under
`skip` when the guard would refuse it.

## Inputs and outputs

Reads `src/<unit>.cpp|.c` and the headers it includes (`#include "..."` closure under `src/`) for declared widths; writes the
unit's source (the kept widths, CRLF preserved) and objects under `build/` through `tools/units/recompile.py --measure`.

## Invariants and rules

* **The guard** (`facts_for`, `allowed_types`): a width is refused when it cannot hold (1) a literal assigned (`=`, `op=`),
  initialised, compared (`==`, `<`, ... either side), or used as a `case` label under `switch (var)`; (2) the value range of an
  expression stored in or compared with it - a literal, a `(type)` cast, a ternary (union of its arms), `x % N`, `x & N`, a
  field, global, array element, local, parameter or call whose declared integer width is found (`TypeIndex`); (3) an
  expression no declaration bounds allows only `s32`/`u32`. `--loose-sources` keeps (1) and drops (2) and (3).
* **The size rule** (`size_ok`): a kept variant's size is equal to the target's or closer to it than the previous best's. The
  score alone is not enough: a width that adds or removes an instruction is a different function.
* The size rule chooses among sweep variants. It does not justify reverting a width that already landed and holds its values:
  a valid width that scores higher stays, with its size gap and range recorded in the unit header's SHAPES note.
* All allowed widths of a local are compiled and the best one kept (strictly higher than the current best); locals are visited
  in declaration order, one `recompile.py --measure` per trial.
* Only a declaration `<int type> name [= init];` with no call in its initialiser is swept; parameters and fields are not.
* The guard is a refusal, not a proof: a width it allows can still be wrong for a value it cannot see (a call result with no
  declared width, a value bounded only by program logic). Read the kept widths against their sources before landing, and give
  each a SHAPES note in the unit header.

## Lib dependencies

repo (the tree root); `tools/units/recompile.py` as a subprocess.

## Test contract

Tier: fixture - `tools/tests/flags/test_tywidth.py` pins the guard on literals, casts, ternaries, fields, arrays, calls,
locals, `%`/`&`, the `==`-is-not-assignment parse, the size rule and the sweep loop with a fake compiler (the 0x2092-in-a-byte
case never reaches the compiler; a width that leaves the target size is not kept).

## Known gaps

* Names are resolved by the last identifier only (a field `id_0x00` declared with two widths in the include closure counts as
  their union); a name in no included file is unbounded.
* `x += expr` with a non-literal `expr`, shifts, `|`/`^` masks and `var++` without a compared bound are not analysed.
* Parameters, struct fields and return types are not swept.
