# Phase 1, band `a`: `.text` 0x80000000 .. 0x800E0000

Proposal file: `docs/splits/proposals/phase1-a.json` (format: `docs/splits-program.md`, extensions at its end).
Tree `15dc2cf7d`. The band holds 81 registered units (0x8003F200 .. 0x800E3CBC, 3,533 functions) and **one**
unowned function (0x8009CD64, 0x58 B). The 5,407 unowned functions of the program lie in other bands, so this band is
a recut audit, not a fill.

## Counts

| | |
| --- | --- |
| proposal units (rendered candidate) | 7 (8 in the file: the unowned function is a `guess` unit that the renderer folds into `ef/ef_util`) |
| cuts emitted | 8: **strong 2**, **medium 5**, **guess 1** |
| registered cuts removed (`removes_cuts`) | 2, both **strong** (`0x800DCFEC`, `0x800DD1F0`) |
| registered units recut | 5 tails cut off (`mh3_pad`, `draw_shape`, `g3d_state`, `ef_effectsystem`, `fn_800AEE48`), 1 moved (`ef_effectsystem` tail -> `ef_emitter`) |
| registered units merged | 3 -> 1 (`sound/fn_800D7F54` + `fn_800DCFEC` + the head of `fn_800DD1F0`) |
| unowned functions covered / left open | 1 covered (as a `guess`, two positions) / 0 left |
| candidate cuts kept as `open_questions` (never applied) | 14 entries: 11 candidate cuts (about 16 cuts), 3 notes that are not cuts (`nw_resource`, `main.cpp`, `ef_emform`); all `guess` |

Two of the five medium cuts (`0x8009B374`, `0x800D7F54`) restate a registered start so a unit has a left neighbour or a
cut entry; they carry a narrow `pooldup` interval (2 and 3 functions) and change nothing.

## Grade rubric used

* **strong**: an exact invariant (deferred-closure bound, a pooled literal read on both sides of a cut) **plus** an
  independent soft interval that holds the cut and no other known cut.
* **medium**: the exact invariant alone, or one narrow soft interval (at most 3 functions).
* **guess**: anything else; never emitted as a cut, listed in `open_questions` with its interval and position count.

## The finding that changed the plan: a `.ctors` word is not a cut at the end of its function

`splitcheck`'s `ctors` invariant says the function a `.ctors` word points at is the TU's last function and reads a
boundary at its end. That is wrong whenever the static-init function calls local functions located after it, which MWCC
emits after the `__sinit` (deferred inline constructors, destructors registered through `__register_global_object`,
thunks such as `lis/addi/b ctor`). Measured in this band:

| sinit | size | what follows it | where its TU really ends |
| --- | --- | --- | --- |
| `fn_80046B94` | 0x60 | dtor `fn_80046BF4` passed by address, its callee `fn_80046C28` | 0x80046C80 (not 0x80046BF4) |
| `fn_80055E58` | 0xC | `b fn_80055E64` (ctor, 0x60 B) | 0x80055EC4 (not 0x80055E64) |
| `fn_800594DC` | 0xC | `b fn_800594E8` -> `fn_8005951C` | 0x80059550 = the registered end: **no cut** |
| `fn_80088AD0` | 0xD4 | 14 local ctors, 0x80088BA4..0x80088E24 | 0x80088E24 (not 0x80088BA4) |
| `fn_800A60C8` | 0x6C | 5 local functions, 0x800A6134..0x800A6258 | 0x800A6258 (not 0x800A6134) |
| `fn_800B4ABC` | 0xC | `b fn_800B2884` (earlier), nothing after | 0x800B4AC8 (as the checker says) |
| `fn_800CCDA8` | 0x54 | 8 EmForm ctors 0x800CCDFC..0x800CCF74 | 0x800CCFB0 = the registered end: **no cut** |
| `fn_800D2F88` | 0xC | `b fn_800D2F94` | 0x800D2FEC = the registered end: **no cut** |

The rule used (R1): the TU ends at `L` = the end of the closure of the sinit's calls and address-taken functions that lie
after it (`callers.py` shows both), and only if the function at `L` is not referenced from anything before `L` in the
unit. Check on the whole tree (`.pi/tmp/allclosure.py`, not committed): of 36 single-word `ctors` FAIL units, in 4 the
closure ends exactly at the registered unit end (the registered cut came from other tools: a coincidence four times in
four), and in 32 it leaves a tail (5 in this band, 27 elsewhere). Outside this band the same `L` is the sharper cut for the tail units below; those
cuts belong to the other bands and are not in this file.

| unit | sinit-end (checker) | `L` (closure) | unit end |
| --- | --- | --- | --- |
| sound/fn_800DD1F0.cpp | 0x800E3AE8 | 0x800E3B3C | 0x800E3CBC |
| ef/effect.cpp | 0x800FAC78 | 0x800FACAC | 0x800FAE08 |
| ef/fn_80114E34.cpp | 0x801153D0 | 0x801153D0 | 0x8011722C |
| ef/eft029.cpp | 0x8011AD58 | 0x8011AD58 | 0x8011D448 |
| enemy/fn_80137604.cpp | 0x8013791C | 0x8013791C | 0x80138074 |
| enemy/enemy_control.cpp | 0x80147AC8 | 0x80147C94 | 0x80147CE0 |
| enemy/fn_80165FC8.cpp | 0x801663E4 | 0x801663E4 | 0x801679B0 |
| enemy/fn_80171194.cpp | 0x80176C30 | 0x80176C30 | 0x80176C58 |
| enemy/fn_80181C88.cpp | 0x80182C40 | 0x80182C40 | 0x80182D5C |
| enemy/fn_80191598.cpp | 0x80192348 | 0x80192348 | 0x801926EC |
| enemy/fn_801B0010.cpp | 0x801B4348 | 0x801B4348 | 0x801B4458 |
| lobby/lb_npc.cpp | 0x801FF758 | 0x801FF9E0 | 0x802029B4 |
| Pl/fn_80262940.cpp | 0x802673A4 | 0x802673A4 | 0x802693C4 |
| Pl/fn_80288CEC.cpp | 0x8028F44C | 0x8028F44C | 0x8028F66C |
| Pl/fn_80295EF4.cpp | 0x80297D9C | 0x80297E34 | 0x8029F3C8 |
| stage/fn_802B2AA0.cpp | 0x802B53CC | 0x802B5640 | 0x802B5C58 |
| ai/fn_802D44F4.cpp | 0x802D9E20 | 0x802D9EA4 | 0x802DDC04 |
| ef/eft035.cpp | 0x802F2120 | 0x802F2238 | 0x802F5138 |
| enemy/fn_802F5138.cpp | 0x802F9994 | 0x802F9994 | 0x802FA9A0 |
| lobby/fn_802FA9A0.cpp | 0x802FF234 | 0x802FF234 | 0x8030121C |
| menu/fn_8031A6C0.cpp | 0x8031DAA8 | 0x8031DAA8 | 0x8031EA8C |
| enemy/em_act_step.cpp | 0x80330194 | 0x80330194 | 0x8033041C |
| ef/eft_slot.cpp | 0x80348A14 | 0x80348A48 | 0x80349DD8 |
| enemy/em_pop.cpp | 0x803B92E0 | 0x803B936C | 0x803BE30C |
| menu/get_pop_dat_ptr.cpp | 0x803C3A5C | 0x803C3A5C | 0x803C4BA0 |
| homebutton/keyboard.cpp | 0x80566440 | 0x80566440 | 0x80569DAC |
| tiHKBManager.cpp | 0x8056D7D4 | 0x8056D814 | 0x8056F2B4 |

These are the 27 single-`.ctors`-word units outside the band whose closure leaves a tail (the 5 in-band ones are the table above); units with several words are not covered. `L` equals the checker's address where the sinit has no local callee.

## Units proposed / recut / merged

| unit (derived) | range | cut | grade | note |
| --- | --- | --- | --- | --- |
| `main/pad_connect` | 0x80046C80..0x80047398 | 0x80046C80 | strong | tail of `mh3_pad.cpp`; ctors closure + two `pooldup` rows that hold only this cut |
| `main/draw_shape_arm` | 0x80055EC4..0x80056F24 | 0x80055EC4 | medium | tail of `draw_shape.cpp`; named from `draw_shape_arm`; the registered edge 0x80054C64 (draw_shape.cpp start) is probably interior to its TU (open question) |
| `g3d/fn_80088E24` | 0x80088E24..0x800898B0 | 0x80088E24 | medium | tail of `g3d_state.cpp`; placeholder name; may join `g3d_resanm.c`; none of its 35 functions is referenced from `g3d_state` or its sinit closure |
| `ef/ef_emitter` | 0x800A6258..0x800A99B4 | 0x800A6258 | medium | `ef_effectsystem`'s tail joins `ef_emitter` (callers, a create/destroy pair in `nw_resource`, and the vtable store: `fn_800A6258` is the ctor of `lbl_80592BA8`, whose slots are `ef_emitter.cpp`'s); absorbs the registered `ef_emitter` |
| `ef/ef_drawstripestrategy_base` | 0x800B4AC8..0x800B9A44 | 0x800B4AC8 | strong | tail of `fn_800AEE48.cpp`; three `pooldup` rows and the `__FILE__` transition (`ef_resource.cpp` last 0x800B4898 | `ef_drawstripestrategy.cpp` first 0x800B4BA4) agree; ends past `fn_800B99E8`, the DrawStripeStrategy deleting destructor (slot 0 of `lbl_80593CB4`, stored by this unit's ctor); the registered ef_drawstripestrategy.cpp keeps 0x800B9A44..0x800BE154 (billboard + directional, cuts 0x800B9A44 / 0x800BBDEC open); named `_base` because the renderer refuses two units called ef_drawstripestrategy |
| `ef/ef_util` (+ `fn_8009CD64`) | 0x8009B374..0x8009CDBC | 0x8009B374 / 0x8009CD64 | medium / guess | the unowned function joins the left neighbour (two positions; its 15 callers are 3 in ef_util, 3 in ef_emitter, 9 in ef_drawstripestrategy: a weak signal for the left fold) |
| `sound/fn_800D7F54` | 0x800D7F54..0x800DD40C | 0x800D7F54 | medium | swallows the registered cuts 0x800DCFEC and 0x800DD1F0 (strong: `lbl_807963E0`, 0.0f, read on both sides; tudiscover's `strong` rows for these two are two adjacent switch tables `jumptable_80597B50 -> jumptable_80597B7C`, not a seam) |

Naming: root-level units have no module in the format, so `main` is used (a guess); placeholders are `fn_<addr>`.

## Top open questions (all `guess`; full list in the JSON)

1. `mh3_pad.cpp` head: a second TU starts in 0x80041640..0x80041AA4 (23 positions).
2. `fn_80056F24.cpp`: four TUs (0.0f held at four pool addresses); three cuts, 4 / 10 / 8 positions.
3. `g3d_anmchr.cpp`: three TUs; 26 and 58 positions.
4. `fn_80075DCC.cpp`: one start in 37 positions plus a zigzag seam in 0x8007B2D4..0x8007B99C.
5. `ef_drawstripestrategy.cpp`: starts at 0x800B9A44 now (billboard) with 0x800BBDEC (directional) as the second cut (guess); two more `.data` seams inside.
6. The sound merge's end: 0x800DD40C (lower bound) .. 0x800E0560.
7. `fn_8009CD64` left or right (0x8009CD64 or 0x8009CDBC).
8. `g3d/fn_80088E24` and the registered `g3d_resanm.c`, and `ef/fn_800B4AC8`'s inner `4330000080000000` row (0x800B7628..0x800B9DF8): one TU or more.

## Remaining splitcheck failures (rendered candidate, `--proposal ... --emit-splits`)

Candidate 309 units (baseline 306), band alone. `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`, `local-static`: 0 FAIL, unchanged.
`ctors` 39 -> 34, `pool` 129 -> 126, `data-order` 3, `jumptable` 1 (both unchanged, outside the band or phase 2).
**New failures outside the proposal units: 0**; all 7 proposal units PASS every checked invariant; lint: none.

The closure "false positive" of the first revision is gone: the checker reads the closure `L` of the sinit's callees (and the unit's own vtable slots), so the band's `ctors` FAILs are
the baseline units this band does not touch (`splitcheck.py --baseline --only ctors --unit X` prints the `detail` lines with both ends; `ef_emform`, whose sinit registers EmForm
classes whose vtable slots are another TU's large functions, stays PASS).

Band `pool` FAILs that remain are the `guess` intervals above (units that hold one value at two pool addresses: `mh3_pad`,
`fn_80047398`, `fn_8004CAD8`, `fn_80056F24`, `g3d_anmchr`, `fn_80075DCC`, `ef_particlemanager`, `ef/fn_800AEE48`'s head,
`ef_drawstripestrategy`, `sound/fn_800DD1F0`'s remainder) plus `main.cpp` (a false decode, below).

## Tool gaps hit

* **`splitcheck` `ctors`** (fixed): the deferred closure `L` (sinit + local callees and address-taken functions after it) replaces the sinit's end, `cut_at` is exact, and the own vtable slots
  after it count too, also past the unit end; fixtures cover a `b`-thunk sinit, a slot run and a cut over a unit's own slots.
* **`splitcheck` pool decode**: a `lis/addi` that only forms an address (RSO `RSOStaticLocateObject` taking the start of
  `.sdata2`) is counted as a read of `lbl_80795AA0`; `callers.py` lists the two real readers, both in `main.cpp`.
* **Format**: no way to say "guess: stay merged with the *baseline* neighbour". `merge_guess` folds into the previous
  *proposal* unit whatever its address (non-adjacent units would be coalesced), and a `guess` cut on the first proposal
  unit is kept as a unit. The work-around restates the neighbour as a proposal unit (`ef/ef_util`). Also no module for a
  root-level unit, and no field for "this proposal removes a registered cut" (added: `removes_cuts`).
* **`tudiscover` `pooldup`** reports the last reader of the *exact* literal; reading `.sdata2` by address order (last
  reader of any literal at or below the first copy, first reader of any at or above the second) is tighter; the
  interval sizes above come from that, per unit (`.pi/tmp/intervals2.py`, not committed).
* **`dataorder` seams near deferred constructors are noise**: the three seams inside `ef_emform` are vtable stores of
  constructors instantiated in this TU for classes defined in others.
