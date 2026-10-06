# The Pl units

The `src/Pl/` units: the evidence behind the lib's flags, and the measured alternatives the unit headers point at.
Each unit's own facts (range, names, residuals, load-bearing shapes) are its file header.

## Flags

`cflags_pl` (`configure.py`) is `cflags_base` with `-O4,p`, `-inline auto` and `-Cpp_exceptions off` replaced. Measured
with the real ninja command line on the functions named (all of them are in `Pl/pl_act.cpp`):

| flag | evidence |
| --- | --- |
| `-O4,p` -> `-O3` (playbook 27) | the master section 0x8026BA1C..0x8026FFBC: 5 of 22 functions at 100 % under `-O4,p` (`fn_8026BA1C` 83.28, `fn_8026FA6C` 71.44, `fn_8026FB20` 0, `fn_8026FD0C` 75.62, `fn_8026FD94` 86.75), 18 under `-O3`; `.text` 4316 B against 3612 B. `fn_80270018` 65.0 -> 87.6. `fn_80276B58`/`fn_80276CE8`/`fn_80276D94`/`fn_80276E08` 85.9/84.7/86.1/87.9 -> 97.3/90.0/96.4/89.6. `-O4,p` implies `-func_align 16`; the retail starts are packed on 4 B (`.text`+0x1254, +0x11c8, +0x3d98 in the master section; +0x23c, +0x588, +0xcbc in the action section). |
| `-inline auto` -> `-inline noauto` (playbook 28) | `fn_8026FA6C` (180 B) is inlined into `fn_8026FB20` under `auto` (892 B against the target's 288 B); `fn_80270CA4` keeps its five `bl fn_80270C64` only under `noauto` (828 -> 684 B, the target's size); `Pl_act_set_step_table` is inlined into the three arms of `fn_802771A0` under `auto` (57 -> 229 instructions). `-inline off` measures the same as `noauto` there; `-opt noautoinline` is not a working spelling. |
| `-opt nopeephole` | `fn_80270018` 87.6 -> 99.73, 832 -> 836 B (the target's): level 3's constant merge turns the target's `lis/stw 8(r1); lis/stw 16(r1)` pair into one `lis` and two `stw`, and hoists each skill branch's `li r0,1 / stb` above the `addi r31,r31,N`. `fn_80276B58`/`fn_80276CE8`/`fn_80276D94`/`fn_80276E08` 97.3/90.0/96.4/89.6 -> 100.0/92.7/100.0/94.0. The retail action section 0x80276B58..0x8027D684 carries exactly one record-form instruction in its 115 functions (`andi. r0,r0,20` at +0xcbc). `-O4,p -opt nopeephole` measures 92.5/87.4/78.7/93.9. |
| `-Cpp_exceptions on` | every Pl target object carries extab/extabindex and `off` emits none. With `on` the `.text` is unchanged and every score identical; the master section's emitted entries carry the target's values (`fn_8026BA1C` 0x100A, `fn_8026BE94` 0x1008, `fn_8026F7B4` 0x1008, `fn_8026F828`/`fn_8026F888` 0x0808, `fn_8026F9A4`/`fn_8026FA6C`/`fn_8026FC40` 0x2008, `fn_8026FD0C` 0x100A, `fn_8026FD94`/`Pl_master_ck`/`fn_8026FF20` 0x0808, `fn_8026BF98` 0x200A, `fn_8026CC7C` 0x180A), and with all 24 functions written its extab (0x70) and extabindex (0xA8) are byte-identical. |
| `-sdata 0` (not used) | fixes `Pl_Skill_ck` 98.7 -> 100 and `Pl_Skill_slot_item_get` 97.0 -> 100 but breaks `fn_802738B8` 100 -> 72.5 and `fn_802738D8` 100 -> 45.0 (`lbl_80792140`/`lbl_80792148` are small data in the target), net -0.12 points. |

### Optimisation level 4 (0x80270018..0x80273B14)

`cflags_pl_skill` is `cflags_pl` with `-opt nopeephole,level=4`; no object uses it, because `Pl/pl_act.cpp` scopes
`#pragma optimization_level 4` to that section. At level 3 the allocator rematerialises `fn_8027350C`'s `plw + i*4`
base across the `fn_802693C4`/`fn_80269474` call; at level 4 it keeps it in r24 like the target (416 -> 404 B,
95.50 -> 99.21). The pragma-free source under the flag set is codegen-identical to the pragma build (0 of 197 symbols
move; only MWCC's local `@NNN` names in `.strtab` shift by 2). The level cannot be the lib's: `fn_8026CC70` drops from
100 to 33.33 at level 4.

## Rule 8 shapes in `Pl/pl_act.cpp`

Four functions reached the target with a label as a control-flow device; rule 8 replaced each with a conformant shape,
and the cost is the residual their unit header records. Measured per function (objdiff match percent):

| function | label shape (removed) | conformant shape kept | other conformant shapes measured |
| --- | --- | --- | --- |
| `fn_80271BD4` / `fn_80271E0C` | label chain 98.59 / 97.97 | `switch ((u32)kind)` with 7-15 as `default` 98.01 / 97.46 | 7-15 as its own range 97.34; if/else chain (A inline) 80.56 / 90.34; `for (;;)` with `break` 80.56 / 90.34 |
| `Pl_motion_input_ck` (0x8027BC48) | shared `ret1`/`ret0` 100 | `do { ... } while (0)`, the range test `break`s into the shared `return 1`: 99.24 | plain returns 94.47; `switch` case group 93.68; one result variable 63.6; inline `return 1` per arm 94.47 |
| `fn_80278144` / `fn_80278310` | shared `ret0`/`ret1` 100 / 100 | `switch (m)` with the `m != 9` body as `default:` first and `case 9:` last: 97.74 / 97.38 | compound `\|\|` condition 84.7; `for (;;)`/`break` dispatch 87.0; shared result variable 67.5 |
| `fn_8027A57C` | `block_16`/`block_53`/`block_75` 99.96 | outer `for (;;)` whose `break` carries every skip; the case-0/2 arms hoist `mag = 5` and `if (mag == 0)` selects the pad scan: 97.63 (same 417 instructions) | compound `\|\|` dispatch 96.02; extra `skip_scan` variable 96.25; `mag = 1` marker 97.46; dropping the flag-sum test 94.30 |

## Measured alternatives behind the unit headers

* `pl_act_param_tier_ck`: the natural source (`s16 value = (s16)self->part_tbl_b_0xD4[idx];` before the early-out)
  colours `value`/the class temporary r5/r4, the mirror of retail's `add r4,r3,r0; lha r4,212(r4)`. About 200 shapes
  reproduce the mirror: declaration order, pointer/cast/temporary/array spellings, `switch`/`do`/`for`/nested-if
  early-out forms, bound and chain order, named class temporaries, dead statements, all 30 toolchain compilers and every
  `-opt` keyword and `#pragma` (`peephole`, `scheduling`, `optimization_level 0..4`, `opt_lifetimes`, ...). MWCC
  colours the two webs in the order the copy webs were born, so a three-deep dead copy chain of `self->field_0x002`
  plus a separate `weaponClass` load is the one shape that lands `value` in r4 with the same 39 instructions. The
  pointer-arithmetic spelling of the load is 15 bytes off; moving the load below the early-out puts its three
  instructions after the branch.
* `fn_8027350C`: at level 3 MWCC rematerialises the base (416 B, 95.50); an explicit `&set_applied[i]`/`&set_pending[i]`
  pair, a `u16*` walk and a `u8*` base measure 91.4 / 91.4 / 90.6 (they move `plw` out of r27).
* `Pl_cat_skill_ck`: `_PLW*`, `u16*`, `u8*` and pair-struct walks, an index form, a nested 2x2 loop, a pointer-bounded
  `while`, a straight-line two-block body, a `u32*` walk and a two-arm `||` all fold `(plw + 4) + 0x612` into
  `plw + 0x616`; the folded form measures 95.75, the `u16*` walk 99.88.
* `Pl_bari_ck`: the negated `&&` with the arms swapped and two separate `if`s measure 75.0 and 76.7 against 86.7.
* `fn_8027B0BC`: a whole-function `#pragma peephole on` removes the `clrlwi`s but turns `extsb`/`clrlwi` + `cmpwi` into
  record forms (93.95 -> 92.28); `+= (s8)7`, an `s8` field, a `u8` local and an explicit `(u8)` cast measure the same.
* `fn_8027C208`: four unused `s32`, a `VEC3` and an `f64` local were tried for the 0x40 frame; MWCC drops them all.
* The act-entry section's `.data` zero word at 0x805C60A4: forced in, `.data` is 280 B; without it 272 B; the target
  is 276 B (MWCC 8-aligns the 88-byte pick table after the word).
* `fn_80241558` (`Pl/fn_80241558.cpp`): the plain `a = +0xAF4; b = +0xAF8; c = +0xAFC;` declaration measures 99.456
  (r29/r30 swapped), the allocator's order 99.993 (loads swapped); the declared-then-assigned shape is 100.
