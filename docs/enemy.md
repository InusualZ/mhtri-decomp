# The enemy units

The units of the enemy module (`src/enemy/`): the measurements behind their per-file pragmas, the source shapes that
were measured and not kept, and the seam evidence too long for a unit header.  Each unit's own facts (range, names,
residuals, load-bearing shapes) are in its file header; the enemy rows in `configure.py` carry no per-unit flags.  Scores are the objdiff report's per-function percentages.

## Peephole and fp_contract

The `enemy` lib's group is `cflags_main`, with the peephole pass on and `-fp_contract on`; `enemy/em_pop.cpp` and
`enemy/em_model.cpp` sit in the `menu` lib, whose `cflags_menu` is `cflags_main` + `-opt nopeephole`.  Retail keeps the unfused
`clrlwi`/`rlwinm` + `cmpwi`, `clrlwi` + `slwi` and `subi` + `cmpwi` pairs in most of the band (playbook 39), so the
units turn the pass off per file or per function:

* **`enemy/em030_prog.cpp`** scopes `#pragma peephole off` to `fn_801B4244` (59.78 -> 91.09); every other body
  keeps the pass on.
* **`enemy/em034_prog.cpp`** keeps the pass on: `#pragma peephole off` over the seat band fixes `fn_801B4C54`
  (96.56) and `fn_801B6C38` (94.47) to 100 but regresses the bodies whose retail keeps the fused or eliminated form
  (`fn_801B4F08` 100 -> 96.77, `fn_801B5AE4` 100 -> 96.77, `fn_801B5030` 97.87 -> 92.31, `fn_801B6010`
  97.65 -> 93.57).
* **`enemy/em_kind.cpp`** turns the pass off from the interpreter's first body to the end of the file.  On the
  parameter interpreter (0x8013BE60-0x8013F764) retail keeps `(x & 8) == 0` as `rlwinm` + `cmpwi` (`fn_8013DD48`) and
  `((x & 0xF) << 8) | y` as `clrlwi` + `slwi` + `or` rather than one `rlwimi` (`fn_8013D310`): with the pragma
  `fn_8013DD48` 97.81 -> 100, `fn_8013D310` 75.67 -> 100, `fn_8013DA7C` 99.98 -> 100, `fn_8013E388` 99.93 -> 100,
  nothing else moved.  On the run driver and stream helpers (0x8013F764-0x801411B8) retail keeps the
  `(v & 0x80) == 0` and `len & 0x30` tests unfused: `fn_80140AF8` 39.00 -> 88.00, `fn_80140B10` 0 -> 75.00,
  `fn_801406E0` 77.38 -> 99.71, `fn_80140C00` 76.20 -> 96.58, `fn_80140778` 85.76 -> 88.23, `fn_801408B4`
  59.57 -> 67.70, `fn_80140B20` 93.30 -> 96.88, `em_kind_release` 79.17 -> 90.28, nothing moved down.  Its
  `#pragma fp_contract off` over the interpreter half: the default fuses `dx*dx + dz*dz` into `fmadds` in
  `fn_8013E2B0` (99.81), retail keeps `fmuls` + `fmuls` + `fadds` (100).
* **`enemy/em_common.cpp`** turns the pass off from its first body to the end.  With the pass on, `fn_80127CB0`
  reads 68, `fn_801269E8` 96.8 and `fn_801252C0` 85 (byte-identical with it off); on the action/status band
  `fn_8012C220` loses 5.7 points and `fn_8012C600` 3.9, and with it off `fn_8012BDF4` goes 83.7 -> 85.2 and
  `fn_8012C0EC` 90.6 -> 96.7; `fn_8012E968` is 99.03 with it off (724 B, retail's size) and 97.71 with it on
  (716 B).
* **`enemy/fn_8011D448.cpp`**: with the pass on, `fn_8011E530` reads 96.60 and `fn_8011D558` 94.23.
* **`enemy/fn_8013ACC4.cpp`**: with the pragma `fn_8013ACC4` 99.23, `fn_8013BDE4` and `fn_8013BDC8` 100; without it
  `fn_8013ACC4` 98.69 (the record index's `clrlwi` + `slwi` fuses into `clrlslwi` and the `field_0x95C` range test
  folds into a two-sided chain); nothing else moves.
* **`enemy/fn_802F5138.cpp`**: a file-wide `#pragma peephole off` was measured to cost `fn_802F5B98`, so the pragma is
  scoped to `fn_802F8B28`, `fn_802F9774`, `fn_802F96B4` and `fn_802F51DC`.

## Load-bearing shapes in em_common

`fn_8012BA00` (the motion frame cost, byte-identical) depends on its spelling:

* the tail is written negated and first (`if (enemy->field_0x43D != 1) ... else if (flag == 0 ||
  em_sleep_ck(...) == 1) ... else ...`); with `== 1` first MWCC inverts the branch and the `frames[4]` body lands at
  the end of the function;
* `type` and `part` are `u32` parameters narrowed with `(u8)`/`(u16)` at every use, which emits retail's `clrlwi`;
  a `u8`/`u16` parameter is treated as already narrow and emits no mask;
* the last-but-one arm is a separate `else if (enemy->field_0x1E2 == 2)` with an empty body: the equivalent
  `!= 2 && add == 1` drops the guard and is two instructions short;
* `flag`, `add` and the `fn_8012D7FC`/`em_sleep_ck` returns are `u32`, since retail's `== 1` tests are `cmplwi`;
* the local order `result, thresholds, frames, flag, add` colours r31..r27, with `result = 0` before
  `VEC3_ctor(&vec)` and `add = 0` after it;
* `0.5f * (0.7f * x)` keeps the two `fmuls` separate (the folded spelling is one `fmadds`).

## Shapes measured and not kept

* `em_common.cpp` `fn_8012E040`, `fn_8012E5D4`: the if-chain in target order, a `switch` over a `u8` local, a
  `switch` over the cast expression, `(u32)`/`(u8)` casts on the range test, and the two range tests merged with
  `||` all emit inline returns or a two-compare range test.
* `em_common.cpp` `fn_8012E968`: with `return 1` and a fresh counter MWCC drops the counter (98.3), and
  `return (idx == 0x60)` makes a three-instruction `cntlzw` tail where retail has `li r3,1` (97.5); reusing `mode`
  as the index keeps retail's `addi r4,r4,3`.
* `em_common.cpp` `em_motion_window_ck`: an `if (...) return 1; return 0;` shape is 212 B (95.98) against the
  value-return form; `em_sleep_ck`: the `&&`, nested and `||` shapes score 65-89; `fn_8012F39C`: both load orders and
  a local temporary were measured.
* `em_kind.cpp` `fn_801408B4` (the 7-way dispatch on `field_0x95C`): retail lowers it as a range test
  (`subi r0,r4,3` / `cmplwi r0,4` / `ble` into the body shared with case 0xB) before the 0/2/0xB/1/0xA compares.
  `switch` with the six cases sharing that body has retail's 276 bytes but lowers as a `cmplwi`/`blt` chain
  (67.70, kept); `if (v - 3 <= 4)` emits the range test but duplicates the 0xB body (316 B).
* `em_kind.cpp` `fn_80140B10` (the trace wrapper): retail ends `b .+4` + `blr`, a branch to an empty static body
  placed after the function.  `static void em_prog_trace2(u8, u32)` called as `em_prog_trace2((u8)a, b)` reproduces
  the three setup instructions; with the helper inlined the mask survives but `mr r4,r5` does not (58.75).  Candidate matching idea (not
  demonstrated: the row is still 75): an empty static helper placed after a function reproduces its trailing `b .+4`.
* `em_kind.cpp` `fn_80140AF8`: the parameters declared u32/u8/u8 and the cast pulled into a local both leave the
  argument shuffle.
* `em_act_step.cpp`: the `work[i]` stride spelling of the attacker scan scores 92.4 on `em_act_arm_mot1_hit1_1`, so
  `work->` stays; `em_eff_offset_set`'s 12-byte copy measures the same as a `VEC3` copy, a `_CP_VECTOR` copy and a
  cast.
* `em035_prog.cpp` `em035_frame_tick` (98.54): retail keeps the field value in f2 and the pool constant in f1, ours
  the reverse.  Eleven source shapes were measured (`weight`/`limit` in both declaration orders, the constant as a
  local, the comparison and compound-assignment spellings, the nested-expression form); the best two are the current
  one (98.54) and the reversed declaration order (95.00, which swaps the two loads instead).  `tools/m2c` drafts the
  same shape with the same f2/f0 split.
* `em_model.cpp` `em_roster_record_result_get`: if-chain and nested-switch spellings measure 49-60 against the plain
  switch's 71.11; `em_roster_record_get`: four spellings measured.
* `fn_8013ACC4.cpp`: its locals' declaration order alone was worth 99.06 -> 99.23; the exit test at the top of
  `for (;;)` measures 97.60.

## Shapes for the unwritten rows

`enemy/fn_8011D448.cpp`: every creator shares one skeleton - the `get_now_areano() == act_id` guard,
`eft_res_slot_get(size)`, `field_0x03 = 0x1f`, `type_0x02 = type`, `release_0x40`, `dispatch_0x34`,
`event_demo_ck() == 1 -> field_0x07 = 1`; a creator whose first argument is a bare type (`fn_8011D7B0`) owns no
`_ENEMY_WORK` and stores `source_0x30 = NULL`.  The dispatchers are a `switch` on `state_0x05` whose first case
falls through to a second `switch` on `lbl_805A0CB8[type_0x02]` (`fn_8011D9B8`); the release callbacks are the same
`switch` with two tail calls; the int-to-float scale conversions read `field_0x7a4` as signed (`xoris` + 0x4330);
the per-part table is `parts_0x838[part]`, stride 6.  In `fn_8011E658` every spelling that keeps the `if`-clamp into
the ceiling parameter reloads `amount_0x7A0` once instead of twice.

## Seams

**`enemy/em_pop.cpp` (0x803B465C-0x803B936C) and `enemy/em_model.cpp` (0x803B936C-0x803BE30C)** were one range and
are probably several TUs.  From the target objects:

* the `.sdata2` pool repeats values at separate addresses (0.0f at 0x8079C530/C5A8/C5E0, 60.0f at C524/C568, 50.0f
  at C578/C5D8, the two int-to-float magics at C528/C610 and C570/C600), and MWCC keeps one pool per TU;
* the entries from 0x8079C568 on are monotone in first-use address and tile into three bands: 0x803B7CE8-0x803B8A48
  (C568..C5B4, the em_set loader), 0x803B9588-0x803B9D74 (C5B8..C5D0, the roster spawn) and 0x803BA6F0-0x803BE1A8
  (C5D8..C628, the model helpers); 0x8079C524..C560 is read by `fn_803B6998`/`fn_803B6B14` and shared with
  `quest/quest_entry.cpp`;
* the one `.ctors` word names `fn_803B92D4` (`lis` + `b fn_803B92E0`, a static-initializer thunk over the `.bss`
  object at 0x806CC420 whose constructor `fn_803B92E0`/`fn_803B9338` follows), which closes the em_set TU near
  0x803B936C;
* `tudiscover` answers the match sets 0x803B7CE8-0x803B91A0 (16 functions) and 0x803BA6F0-0x803BE30C (35 functions).

Proposed tiling (unmeasured): [0x803B465C, ~0x803B7490) joins quest_entry + arena_result, [~0x803B7490,
0x803B936C) is the em_set TU (the `.ctors` word, the `.bss` object, `lbl_80793688`), [0x803B936C, 0x803BA6F0) the
roster spawn TU and [0x803BA6F0, 0x803BE30C) the model TU.
