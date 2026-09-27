/* enemy/fn_80178378.cpp - the enemy action band: the 64-function action arming/stepping run
 * 0x80178378..0x80181C88 (39,184 B).  Registration is this worker's; the extent is the probe's.
 *
 * MODULE AND NAME.  `enemy`: every registered neighbour below is an `enemy/fn_*.{c,cpp}` unit
 * (0x80176C58, 0x80177608, 0x80177890, 0x80178128 - the last one's `.text` ends exactly at this
 * range's start) and every callee is in the enemy band, reached through `_ENEMY_WORK`.  The range's own
 * dispatchers confirm the subsystem: `fn_80179E98` tail-calls the low action writers through
 * `jumptable_805A9574`, `fn_8017F0D8` the later ones through `jumptable_805AA530`, and `fn_8017F138`
 * (the action selector at `_ENEMY_WORK::action_0x1E5`, `+0x1E5`) picks between the neighbouring units'
 * dispatchers (`fn_801775C0`, `fn_80177BA4`, `fn_8017827C`, `fn_80179E98`, `fn_8017D5A0`, `fn_8017D778`,
 * `fn_8017DB8C`, `fn_8017DC94`, `fn_8017F0D8`).
 *
 * SEAM: unproven (docs/plan.md 8.3).  `python tools/splits/tudiscover.py at 0x80178378` returns only
 * weak cuts on both sides, no `__FILE__` string, no class string and no data the range would own, so the
 * range is registered as one unit at the probe's own extent (which is function-aligned on both ends:
 * 0x80178128 ends the unit below, 0x80181C88 starts the next function).  The merge candidate is
 * `enemy/fn_80178128.cpp`, whose 3 functions all match and whose end is the probe's boundary rather
 * than evidence of a TU seam - if the original object is one TU from 0x80178128, the two splits blocks
 * are one file and should be merged.  Checked before registering, per the campaign's continuation rule.
 *
 * SECTIONS (evidence: the range's own per-function split objects, `build/RMHE08/asm/auto_*_text.s`).
 * 58 of the 64 functions carry a C++ exception table entry.  The extab blocks 0x8000E79C..0x8000E96C
 * are contiguous and belong to these functions only (the previous unit's extab ends at 0x8000E79C, the
 * next block starts at 0x8000E96C), and the 58 extabindex entries 0x80029A54..0x80029D0C (58 * 0xC) are
 * contiguous and point at functions of this range only - both ranges are the run's own entry
 * boundaries, not a range end:
 *   extab       start:0x8000E79C end:0x8000E96C
 *   extabindex  start:0x80029A54 end:0x80029D0C
 *   .text       start:0x80178378 end:0x80181C88
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80178378 0x801784D0 0x80178754`, which answers only
 * `zz_XXXXXXXX_` placeholders, and by reading the probe's data pool: no `__FILE__` string).
 *
 * STATUS (official report metric, `build/RMHE08/report.json` in this worktree after a full `ninja`,
 * whose link reproduces the pinned main.dol SHA-1 BF485073...): 34 of 64 functions written, all at or
 * above the 80 % bar, **27 byte-identical**; unit measure 15.72 % fuzzy / 27 matched functions /
 * 4424 matched bytes (11.29 % of the range).
 *   100.00 fn_80178754 fn_801787E4 fn_801798EC fn_8017996C fn_80179A2C fn_80179C38 fn_80179CB4
 *   100.00 fn_80179E18 fn_8017A004 fn_8017B60C fn_8017C504 fn_8017C5DC fn_8017DB8C fn_8017DC50
 *   100.00 fn_8017DC94 fn_8017DCA8 fn_8017DE8C fn_8017E178 fn_8017E248 fn_8017EDF0 fn_8017EE94
 *   100.00 fn_8017EF38 fn_8017EFE0 fn_80179D34 fn_8017F05C fn_8017F0D8 fn_8017F138
 *    99.93 fn_80178378   97.26 fn_8017DD68   94.44 fn_8017C918   93.10 fn_8017B990
 *    91.49 fn_8017BA78   90.74 fn_801797F8   90.48 fn_8017CA38
 *
 * FOLLOW-UP QUEUE (30 functions, 32,760 B - the rest of the probe's range, named per the campaign
 * rule).  The small ones first (all <= 600 B, the same templates as the written ones): fn_801794CC (304),
 * fn_80179E98 (364, a jump-table dispatcher - the `m2cinput` output
 * carries its `jumptable_805A9574` entries, which give the case order), fn_80179AC4 (372), fn_8017D5A0
 * (472), fn_8017F1F0 (472), fn_801792F0 (476), fn_8017DF9C (476), fn_801795FC (508), fn_80178E58 (536),
 * fn_8017E2D4 (564), fn_8017B748 (584).  Then fn_8017D05C fn_80179070 fn_801784D0 fn_8017C690 fn_8017BB34
 * fn_8017A9F8 fn_8017C238 fn_8017D2BC fn_8017A0BC, and the big ones: fn_8017D778 (1,044) fn_8017E508
 * (1,088) fn_8017BDC4 (1,140) fn_8017B190 (1,148) fn_8017E948 (1,192) fn_8017ACA8 (1,256) fn_8017CB34
 * (1,320) fn_8017889C (1,468) fn_8017A3BC (1,596) fn_8017F3C8 (10,432, the band's own jump-table body).
 * fn_801784D0, fn_80179E98, fn_8017D5A0, fn_8017D778 and fn_8017DF9C are already called by written
 * bodies and are declared at the top of this file, so their object symbols are unnamed externals until
 * they are written (a relocaudit note, not a link problem: a NonMatching unit links the original object).
 *
 * RESIDUALS (measured, not guessed; every one is a same-size instruction-ordering difference, which is
 * MWCC's scheduler, not a source shape - the stopping rule in docs/matching.md applies):
 *  - fn_80178378 99.93 % (344 B, 86 ins, same size): retail restores the counter with
 *    `subi r0,r3,1; stw r0,0x20; cmpwi r0,0; bgt`, this build fuses the test into
 *    `subic. r0,r3,1` unless the source writes the comparison as `< 1`; with `< 1` the bodies are the
 *    same length and only the compare constant + branch opcode differ (`cmpwi r0,1; bge`).  Tried:
 *    `<= 0`, `== 0`, `> 0` (all fused to `subic.` + 340 B), `--x` in the condition, a `u32` field, a
 *    reload of the field in the condition, `(s32)`/unsigned spellings - the best is 99.93 %.
 *  - fn_8017DD68 97.26 % (292 B): the two argument setups of `fn_80134004(self, 0x12, lbl_80797B4C)`
 *    are emitted in the other order (`li r4` before `lfs f1`; retail loads the pool float first).
 *  - fn_8017C918 94.44 % (288 B) and fn_8017BA78 91.49 % (188 B): same class - for
 *    `fn_80133E3C(self, 0x8000, f1, f2)` retail loads both pooled floats before the `lis/addi` of the
 *    integer argument, this build the other way round.
 *  - fn_8017B990 93.10 % (232 B): both `fn_80133E3C` calls, same class (`li r4, +/-0x4000` placement).
 *  - fn_8017CA38 90.48 % (252 B, same size): the three `fn_80134004(self, 0, f1)` sites, same class.
 *  - fn_801797F8 90.74 % (target 244 B, ours 240 B): the turn-rate store from the angle's high byte -
 *    retail `srawi r0,r0,8; clrlwi r0,r0,24; stb`, this build folds it to one `extrwi r0,r0,8,16`.
 *    Measured: `(u8)((s16)angle >> 8)` reaches 244 B but trades the fold for an `extsh` (90.16 %);
 *    `(u8)((s32)angle >> 8)` (kept) is the best of them at 90.74 %.
 *
 * SHAPES FOUND BY MEASUREMENT (the templates the rest of the band uses):
 *  - The armed-state machine.  Two-case three-way dispatch on `_ENEMY_WORK::state_0x05` (`+0x05`) is
 *    `switch (self->state_0x05)` with `case 0: { u8 state = self->state_0x05; self->state_0x05 =
 *    state + 1; ... break; }`; the `u8 state` copy is needed - writing `self->state_0x05 += 1` gives a
 *    different (also correct) load/add/store and a lower score.  This is what makes 24 of the 30
 *    bodies byte-identical.
 *  - The countdown idiom `self->field_0x20 -= 1; if ((s32)self->field_0x20 < 1)` is the only spelling
 *    found that does not fuse into `subic.` (see the fn_80178378 residual).
 *  - A trailing argument-setup pair (integer immediate + pooled float) is scheduled integer-first by
 *    this build, float-first by retail - the five residuals above are all instances.
 *  - `field_0x482` (+0x482) is read only here: it selects between the two `fn_80136D4C` fade floats.
 *
 * TYPES.  `_ENEMY_WORK` lives in `include/enemy.h` (the 0xB1C union copy this unit was measured
 * against; the canonical `include/enemy/ENEMY_WORK.h` record is 0xB18 - the 4-byte difference is the
 * union copy's own `pad_0xB18[0x4]` tail and predates this unit).  This unit added ONE named field
 * inside what was `pad_0x474`: `+0x482 field_0x482`, offsets unchanged (0x474 pad_0x474[0xE], 0x482
 * the field, 0x483 pad_0x483[0x141], 0x5C4 flags_0x5C4, 0xB14 se_handle_0xB14 - compile-proved, see
 * MERGE below).  `VEC3`/`Vec3` and the nw4r globals come from `include/nw4r/math.h`, `VEC3_ctor`
 * from `ef.h`.  The declarations this unit consumes live in the owner's header (rule 2):
 * `enemy/fn_801251D0.h`, `enemy/fn_8012BDF4.h`, `enemy/fn_80176C58.h`, `enemy/fn_80177890.h`,
 * `enemy/fn_80178128.h`; the ones whose owner is still unsplit are in `include/unsplit/enemy.h` -
 * except `fn_803B9BA0`, which is declared at the top of THIS file: its address is unsplit with no
 * sound band (bracketing units `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`, rule 2's named
 * gap) and the two landed consumers spell it differently (`enemy/fn_80147CE0.cpp` as
 * `(_ENEMY_WORK*, void*, s32)`, `enemy/fn_801550FC.cpp` as `(_ENEMY_WORK*, u32, u32)`), so a single
 * shared-header declaration makes one of them fail to compile (MWCC: illegal function overloading).
 * This unit matches `fn_801550FC.cpp`'s spelling, the one its body was measured against.
 *
 * MERGE (this branch merged main after the sibling `enemy/fn_8015E854.cpp` landing).  `include/enemy.h`
 * conflicted on exactly one line: both sides had split `pad_0x474` at the same offset and added the
 * same `+0x482 field_0x482`, differing only in the comment, so the resolution is ONE field whose comment
 * names both consumers (`enemy/fn_80178378.cpp`'s `fn_80178378`, `enemy/fn_8015E854.cpp`'s `fn_8015EA24`)
 * - no union, no duplicate field.  Compile-proved with the real toolchain against HEAD, main and the
 * merge (build/tmp/sizeof_probe.cpp, a scratch probe): sizeof(_ENEMY_WORK)=0xB1C and the offsets
 * 0x474/0x482/0x5C4/0xB14 are IDENTICAL in all three, i.e. the merge moved no offset.  The unit's own
 * 34 bodies re-measured after the merge to exactly the pre-merge numbers (27 byte-identical, unit 15.72 %).
 */
#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/enemy.h"
#include "enemy/fn_801251D0.h" /* fn_801251D8/fn_80128030/fn_8012933C/fn_80129668/fn_80129724 */
#include "enemy/fn_8012BDF4.h" /* fn_8012CF20/fn_8012D1A0 */
#include "enemy/fn_80176C58.h" /* fn_801775C0 */
#include "enemy/fn_80177890.h" /* fn_80177BA4 */
#include "enemy/fn_80178128.h" /* fn_8017827C (the master dispatcher's case 2) */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h" /* fn_80146058/fn_8014610C */

/* ------------------------------------------------------------------------------------------------
 * Callees and pool literals owned by other units (declared by their map spelling; playbook 29).
 * ------------------------------------------------------------------------------------------------ */

/* nw4r math free functions the map carries at global scope (`...__FPQ34nw4r4math4VEC3...`). */
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

extern "C" {
/* 0x803B9BA0 - the stage-side pose request `fn_8017E178` posts: r3 `self`, r4 the pose vector's address,
 * r5 the request id.  Declared HERE rather than in a shared header on purpose: the address is unsplit with
 * no sound band (its bracketing registered units are `hud/fn_80324F7C.c` and
 * `Network/NetworkWiiMediator.c` - rule 2's named gap), and the two landed consumers spell it
 * differently (`enemy/fn_80147CE0.cpp` as `(_ENEMY_WORK*, void*, s32)`, `enemy/fn_801550FC.cpp` as
 * `(_ENEMY_WORK*, u32, u32)`), so a single header declaration makes one of them fail to compile
 * (MWCC `illegal function overloading`).  This matches fn_801550FC.cpp's spelling, the one this unit's
 * body was measured against. */
void fn_803B9BA0(_ENEMY_WORK* self, u32 v, u32 a);
/* This unit's own not-yet-written functions (same TU, so the declaration is legal here): the action
 * band's table writers call each other.  Each one is a follow-up-queue entry named in the header. */
void fn_801784D0(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80179E98(_ENEMY_WORK* self);
void fn_8017D5A0(_ENEMY_WORK* self);
void fn_8017D778(_ENEMY_WORK* self);
void fn_8017DF9C(_ENEMY_WORK* self);
void fn_8017E2D4(_ENEMY_WORK* self);
void fn_8017E508(_ENEMY_WORK* self);
void fn_8017E948(_ENEMY_WORK* self);

/* Pool literals owned by the data pass: the enemy action table and the aim scale constants. */
extern u32 lbl_8056FE50[];
extern u32 lbl_8056FE90[];
extern u8 lbl_805AA260[];
extern u8 lbl_805AA2A0[];
extern u8 lbl_805AA2D8[];
extern u8 lbl_805AA320[];
extern u8 lbl_805AA350[];
extern u8 lbl_805AA378[];
extern f32 lbl_80797B18;
extern f32 lbl_80797BF4;
extern f32 lbl_80797B50;
extern f32 lbl_80797B4C;
extern f32 lbl_80797BB0;
extern f32 lbl_80797BE0;
extern f32 lbl_80797BF0;
extern f32 lbl_80797CC0;
extern f32 lbl_80797BF8;
extern f32 lbl_80797BFC;
extern f32 lbl_80797CD0;
extern f32 lbl_80797C94;
extern f32 lbl_80797C34;
extern f32 lbl_80797B2C;
extern f32 lbl_80797B20;
extern f32 lbl_80797B80;
extern f32 lbl_80797B9C;
extern f32 lbl_80797C1C;
extern f32 lbl_80797C68;
extern f32 lbl_80797B88;
extern f32 lbl_80797CBC;
extern f32 lbl_80797B64;
extern f32 lbl_80797D28;
extern f32 lbl_80797D2C;
extern f32 lbl_80797DC8;
extern f32 lbl_80797D34;
extern f32 lbl_80797D38;
extern f32 lbl_80797D50;
extern f32 lbl_80797DB8;
extern f32 lbl_80797D88;
extern f32 lbl_80797DBC;
extern f32 lbl_80797DC0;
extern f32 lbl_80797DC4;
extern f32 lbl_80797B58;
extern f32 lbl_80797B74;
extern f32 lbl_80797B78;
extern f32 lbl_80797B7C;
extern f32 lbl_80797B68;
extern f32 lbl_80797B6C;
extern f32 lbl_80797B70;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178378 - arm/step an enemy action, aiming at the target once the arming frame reports 1
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80178378(_ENEMY_WORK* self, u32 flag) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        self->phase_0x06 = 0;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_8056FE50, 0, 1, 0);
        fn_801353E4(self);
        if ((u8)flag == 1) {
            s32 ang = calcVecAng2(&self->pos, &self->target);
            setVector3(&self->v_0x310, lbl_80797B18, lbl_80797B18, lbl_80797B68);
            rotVecY(&self->v_0x310, ang);
            self->field_0x20 = 0x28;
        }
        break;
    }
    case 1:
        if ((u8)flag == 1 && self->phase_0x06 == 0) {
            fn_80135418(self);
            self->field_0x20 -= 1;
            if ((s32)self->field_0x20 < 1) {
                self->phase_0x06 = self->phase_0x06 + 1;
            }
        }
        if (fn_80134B0C(self, lbl_8056FE50) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178754 - arm the 0x3A/0x0A motion, then finish on `fn_8012F93C`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80178754(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x3A, 0xA, 0, 1);
        fn_80129668(self, 0, 0x25);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801787E4 - the timed-table variant: same arm, then the table's own completion test
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801787E4(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_8056FE90, 0, 1, 0);
        break;
    }
    case 1:
        if (fn_80134B0C(self, lbl_8056FE90) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801798EC - arm the 0x2E/0x14 motion, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801798EC(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017996C - three-phase: arm 0x31/0x14, step to 0x21/0, then `fn_80128A14(self, 5, 0x1D)`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017996C(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state_0x05 = self->state_0x05 + 1;
            fn_8012F5B8(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 5, 0x1D);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179A2C - reset the shared action state, arm 0x3D/0 and the (2, 7) motion pair
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80179A2C(_ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x3D, 0, 0);
        fn_80136B50(self, 2, 7);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179C38 - arm the 0x68/4 motion (`fn_8012F5B8` four-argument form), then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80179C38(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x68, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179CB4 - arm the 0x22/4 motion, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80179CB4(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x22, 4, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179E18 - arm the 0x36/0x1E motion, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80179E18(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x36, 0x1E, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017A004 - arm 0x4D/0x14 and play `fn_80129668(self, 0, 1)` while the frame counter runs
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017A004(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x4D, 0x14, 0, 1);
        fn_80129668(self, 0, 1);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF4, lbl_80797B18) == 0) {
            fn_80133C50(self, 0x40);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017B60C - arm the 0x50/0xA motion and two joint-pair programs keyed on `phase_0x06`/`step_0x07`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017B60C(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        self->phase_0x06 = 0;
        self->step_0x07 = 0;
        fn_80130478(self, 0);
        fn_80130CDC(self, -5);
        fn_8012F5B8(self, 0x50, 0xA, 0);
        fn_8012933C(self, 0, 7, 8);
        fn_8012933C(self, 1, 0x20, 0x18);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797B50, lbl_80797B18) == 1) {
            fn_80133C50(self, 0x30);
        }
        if (self->phase_0x06 == 0 && self->field_0xA0D == 0) {
            fn_8012933C(self, 0, 0x18, 0x18);
        }
        if (self->step_0x07 == 0 && self->field_0xa69 == 0) {
            fn_8012933C(self, 1, 0x21, 0x18);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}


/* ------------------------------------------------------------------------------------------------
 * fn_8017B990 - arm 0x52/4, then two mirrored `fn_80133E3C` poses around a 0x30 fade
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017B990(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_80130CDC(self, -5);
        fn_8012F5B8(self, 0x52, 4, 0);
        fn_80129668(self, 0, 9);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797B18, lbl_80797C1C) == 1) {
            fn_80133C50(self, 0x30);
        }
        fn_80133E3C(self, -0x4000, lbl_80797C1C, lbl_80797B9C);
        fn_80133E3C(self, 0x4000, lbl_80797B2C, lbl_80797C68);
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017BA78 - arm 0x51/0xA and two joint programs, then `fn_80133E3C(self, 0x8000, ...)`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017BA78(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x51, 0xA, 0);
        fn_8012933C(self, 0, 0xE, 8);
        fn_8012933C(self, 1, 0xF, 0x10);
        break;
    }
    case 1:
        fn_80133E3C(self, 0x8000, lbl_80797B20, lbl_80797B80);
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C504 - arm 0x4E/0xA and two joint programs, with the fade on the first frame check
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017C504(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x4E, 0xA, 0);
        fn_8012933C(self, 0, 0x10, 8);
        fn_8012933C(self, 1, 0x11, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            fn_80133C50(self, 0x40);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C5DC - arm the 0x58/4 motion and `fn_80129668(self, 0, 0x15)`, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017C5DC(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x58, 4, 0);
        fn_80129668(self, 0, 0x15);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797CBC, lbl_80797B64) == 1) {
            fn_80133C50(self, 0x40);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C918 - arm 0x25/6, two `fn_80129668` cues and the 0x180/0x280 viewport pair
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017C918(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_80130CDC(self, -0xA);
        fn_8012F5B8(self, 0x25, 6, 0);
        fn_80129668(self, 0, 0x19);
        fn_80129668(self, 1, 0x1A);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797BB0, lbl_80797B18) == 1) {
            fn_80133CC8(self, 0x180, 0x280);
        }
        if (em_frame_check(self, 2, lbl_80797CC0, lbl_80797B18) == 1) {
            fn_80133C3C(self);
        }
        fn_80133E3C(self, 0x8000, lbl_80797BF8, lbl_80797BFC);
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017CA38 - arm 0x59/6 and one of three 0x... pose constants chosen by the caller's selector
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017CA38(_ENEMY_WORK* self, u32 selector) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x59, 6, 0);
        fn_8012933C(self, 0, 0x1B, 2);
        switch ((u8)selector) {
        default:
            fn_80134004(self, 0, lbl_80797CD0);
            break;
        case 1:
            fn_80134004(self, 0, lbl_80797B18);
            break;
        case 2:
            fn_80134004(self, 0, lbl_80797B9C);
            break;
        }
        break;
    }
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80129724(self, 0);
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DB8C - pick the program table by `state_sub`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DB8C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801251D8(self, lbl_805AA260, 0, 0);
        break;
    case 10:
        fn_801251D8(self, lbl_805AA2A0, 2, 0xA);
        break;
    case 15:
        fn_801251D8(self, lbl_805AA2D8, 0, 0xF);
        break;
    case 26:
        fn_801251D8(self, lbl_805AA320, 0, 0x1A);
        break;
    case 27:
        fn_801251D8(self, lbl_805AA350, 2, 0x1B);
        break;
    case 28:
        fn_801251D8(self, lbl_805AA378, 0, 0x1C);
        break;
    default:
        fn_801251D8(self, lbl_805AA260, 0, 0);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DC50 - reset the action, then re-arm the 0x37/0x14 motion with both selectors 0
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DC50(_ENEMY_WORK* self) {
    fn_80131E74(self);
    fn_80131D84(self);
    fn_801784D0(self, 0, 0);
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DC94 - run `fn_8017DC50` while the sub-state is 0
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DC94(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_8017DC50(self);
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DCA8 - the table variant: arm the 0x80178378 action table, then switch to 0xD/1
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DCA8(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_8056FE50, 0, 1, 0);
        break;
    }
    case 1:
        if (fn_80134B0C(self, lbl_8056FE50) == 1) {
            fn_80128A14(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DD68 - three-phase: 0x37/0x14, then 0x2F/0x28 once `fn_80134114` and `fn_8012F948` agree
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DD68(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x37, 0x14, 0, 1);
        fn_80134004(self, 0x12, lbl_80797B4C);
        break;
    }
    case 1: {
        u32 done = fn_80134114(self, 0, 0x80);

        fn_8012F8C8(self, lbl_80797C94);
        fn_80136D4C(self, lbl_80797C34);
        if (done == 1 && fn_8012F948(self) == 0) {
            self->state_0x05 = self->state_0x05 + 1;
            fn_8012F504(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 0xD, 2);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DE8C - arm 0x4E/0xA and two joint programs; the 0xD/3 switch needs `fn_80182430`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017DE8C(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x4E, 0xA, 0);
        fn_8012933C(self, 0, 0x29, 8);
        fn_8012933C(self, 1, 0x2A, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            fn_80133C50(self, 0x80);
        }
        if (fn_8012F93C(self) == 1 && fn_8012D1A0(self) == 1) {
            if (fn_80182430(self, 0) == 1) {
                fn_80128A14(self, 0xD, 3);
            } else {
                fn_80128030(self);
            }
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017E178 - three-phase: 0x31/0x14 plus the `fn_803B9BA0` pose request, then 0x21/0
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017E178(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x31, 0x14, 0, 1);
        fn_803B9BA0(self, (u32)&self->pos, 0x32);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state_0x05 = self->state_0x05 + 1;
            fn_8012F5B8(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 0xD, 5);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017E248 - arm 0x21/4 with the 0x3E8 timer, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017E248(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x21, 4, 0, 1);
        fn_80130CDC(self, 0x3E8);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EDF0 - arm 0x28/0 with the two `fn_80146058`/`fn_8014610C` pose pairs
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017EDF0(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_80146058(self, lbl_80797DC4, lbl_80797D28, lbl_80797D2C);
        fn_8014610C(self, lbl_80797B18, lbl_80797DC8, lbl_80797B18);
        fn_8012F5B8(self, 0x28, 0, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EE94 - arm 0x14/0, then the two pose pairs in the other order, ending on `fn_80127F48`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017EE94(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x14, 0, 0);
        fn_80146058(self, lbl_80797D34, lbl_80797B18, lbl_80797D38);
        fn_8014610C(self, lbl_80797B18, lbl_80797D50, lbl_80797B18);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EF38 - arm 0x28/0 and the stage-side `fn_802B1FEC`, then the two pose pairs
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017EF38(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x28, 0, 0);
        fn_802B1FEC();
        fn_80146058(self, lbl_80797DB8, lbl_80797D88, lbl_80797DBC);
        fn_8014610C(self, lbl_80797B18, lbl_80797DC0, lbl_80797B18);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EFE0 - arm 0x79/4, then finish on `fn_80127F48`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017EFE0(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x79, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F05C - arm 0x64/4, then finish
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017F05C(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x64, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F0D8 - the second action-table dispatcher: `state_sub` (0..13) selects a writer, and nothing
 * runs for a value outside the table (the target's `bgtlr`).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017F0D8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8017DCA8(self);
        break;
    case 1:
        fn_8017DD68(self);
        break;
    case 2:
        fn_8017DE8C(self);
        break;
    case 3:
        fn_8017DF9C(self);
        break;
    case 4:
        fn_8017E178(self);
        break;
    case 5:
        fn_8017E248(self);
        break;
    case 6:
        fn_8017E2D4(self);
        break;
    case 7:
        fn_8017E508(self);
        break;
    case 8:
        fn_8017E948(self);
        break;
    case 9:
        fn_8017EDF0(self);
        break;
    case 10:
        fn_8017EE94(self);
        break;
    case 11:
        fn_8017EF38(self);
        break;
    case 12:
        fn_8017EFE0(self);
        break;
    case 13:
        fn_8017F05C(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F138 - the band's master selector on `_ENEMY_WORK::action_0x1E5` (+0x1E5): it hands the frame
 * to the neighbouring units' dispatchers (0, 1, 2, 5, 7) and to this unit's own ones (10..13), and runs
 * `fn_80127F48` for the actions it does not own.  The trailing pair is the action's own frame end.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017F138(_ENEMY_WORK* self) {
    switch (self->action_0x1E5) {
    case 0:
        fn_801775C0(self);
        break;
    case 1:
        fn_80177BA4(self);
        break;
    case 2:
        fn_8017827C(self);
        break;
    case 5:
        fn_80179E98(self);
        break;
    case 7:
        fn_8017D5A0(self);
        break;
    case 10:
        fn_8017D778(self);
        break;
    case 11:
        fn_8017DB8C(self);
        break;
    case 12:
        fn_8017DC94(self);
        break;
    case 13:
        fn_8017F0D8(self);
        break;
    default:
        fn_80127F48(self);
        break;
    }
    if (self->state_0x1E2 == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801797F8 - arm 0x2D/6, then aim the turn rate at the target: the angle difference picks one of
 * three `step_0x07` turn rates (`0`, `0x80`, or the difference's own high byte).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801797F8(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        u16 angle;

        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x2D, 6, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->target) - self->pos_0x1BC.y);
        if (angle > 0x8000) {
            if (angle > 0xC000) {
                self->step_0x07 = 0;
            } else {
                self->step_0x07 = 0x80;
            }
        } else {
            self->step_0x07 = (u8)((s32)angle >> 8);
        }
        fn_8012CF20(self);
        break;
    }
    case 1:
        fn_80133E3C(self, self->step_0x07 << 8, lbl_80797BB0, lbl_80797BE0);
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        } else {
            fn_8012CF20(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179D34 - arm 0x3A/0xA, then re-arm 0x3A/0x18/0x5C when the frame counter reports 1
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80179D34(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x3A, 0xA, 0, 1);
        fn_80129668(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF0, lbl_80797B18) == 1) {
            self->state_0x05 = self->state_0x05 + 1;
            fn_80129724(self, 0);
            fn_8012F504(self, 0x3A, 0x18, 0x5C, 1);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}
