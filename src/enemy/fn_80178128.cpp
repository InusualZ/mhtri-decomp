/* enemy/fn_80178128.cpp - enemy action helpers: two `state_0x05` steppers and one action dispatcher.
 * .text 0x80178128..0x80178378 (0x250), 3 functions.
 *
 * The range sits in the `enemy` band between `proposal/80177890_fn_80177890` (0x80177890..0x80178128)
 * and `proposal/80178378_fn_80178378` (0x80178378..).  Every callee this unit names is an enemy-module
 * function and `fn_8017827C`'s dispatch table tail-calls five of the previous range's handlers, so the
 * module is `enemy/`.
 *
 * The three functions are the classic enemy action pattern:
 *   - `fn_80178128` - two-phase action: phase 0 arms the action (`fn_80130478`, `fn_80134964`,
 *     `fn_801353F8`) and, for motion ids 0x1F/0x20, aims `v_0x310` at the target (`calcVecAng2` ->
 *     `rotVecY`, re-scaled by `fn_8012F8E4`/`get_em_chg_scale`); phase 1 runs the frame check
 *     (`em_frame_check` -> `CancelFade`) and `fn_80127F48` once `fn_80134B0C` reports done.
 *   - `fn_8017827C` - dispatcher on `state_sub` (0..0xB) tail-calling the previous range's handler
 *     functions with a sub-index argument (0..3).
 *   - `fn_801782F8` - two-phase action: phase 0 arms `fn_8012F504(self, 0x32, 0x28, 0, 3)`; phase 1
 *     runs `fn_8012F93C` and `fn_80128030` on completion.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * `fn_` name defined here is a bare `.text` entry in config/RMHE08/symbols.txt, and neither the map nor
 * the runtime dump offers a real name - `dumpmap.py lookup` returns `zz_0178128_`/`zz_017827c_`/
 * `zz_01782f8_`, which is not evidence).
 *
 * Status.  All three functions are written and measure above the 80 % bar (official `report generate`
 * `fuzzy_match_percent`, this worktree, against the re-split target object):
 *   `fn_80178128` 98.12 %, `fn_8017827C` 100.00 % (byte-identical), `fn_801782F8` 100.00 %
 *   (byte-identical).  Unit measure 98.92 %, 592 B, 2/3 functions byte-identical.
 *
 * Residual.  `fn_80178128` differs by exactly one instruction in the epilogue: retail restores the f31
 * paired-single half with the indexed `li r0, 0x18; psq_lx f31, r1, r0, 0, qr0`, this build with the
 * folded `psq_l f31, 0x18(r1), 0, qr0`.  The whole body is instruction-identical.  Tried:
 * `#pragma peephole off` reproduces the indexed restore but adds a `clrlwi r0, r0, 24` to the
 * `state_0x05` byte store that retail does not have, and drops `fn_801782F8` to 96.875 %, so peephole
 * stays on and the one-instruction epilogue difference is recorded here.
 *
 * Types.  `_ENEMY_WORK` and `_CP_VECTOR` come from the shared headers (`enemy.h`, via `ef.h`); the
 * u32 at +0x1C0 the aim code reads is `_CP_VECTOR::y` of `pos_0x1BC`.  Pool literals
 * (`lbl_8056FE10`, `lbl_80797B50/60/64`) and the not-yet-owned callees are declared, never defined
 * (playbook 29), and are filed as config_requests so they can move to the shared headers.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"

/* ------------------------------------------------------------------------------------------------
 * Callees and pool literals owned by other units (declared by their map spelling; playbook 29).
 * ------------------------------------------------------------------------------------------------ */

/* nw4r math free functions the map carries at global scope (`...__FPQ34nw4r4math4VEC3...`). */
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

/* Mangled enemy-service entry points (declared as C++ prototypes so the compiler mangles them). */
u16 em_get_mot_no(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {
/* Owner: the unsplit enemy band (`include/unsplit/enemy.h` already declares most of these). */
void fn_80130478(_ENEMY_WORK* self, u32 a);
void fn_80134964(_ENEMY_WORK* self, void* tbl, u32 a, u32 b, u32 c);
void fn_801353F8(_ENEMY_WORK* self);
u32 fn_80134B0C(_ENEMY_WORK* self, void* tbl);
f32 fn_8012F8E4(_ENEMY_WORK* self);
u32 fn_8012F93C(_ENEMY_WORK* self);
void fn_8012F504(_ENEMY_WORK* self, s32 a, s32 b, s32 c, s32 d);
void fn_80127F48(_ENEMY_WORK* self);
void fn_80128030(_ENEMY_WORK* self);
void CancelFade(_ENEMY_WORK* self);

/* Owner: `proposal/80177890_fn_80177890` (0x80177890..0x80178128) - this unit's dispatcher targets. */
void fn_80177BEC(_ENEMY_WORK* self, s32 index);
void fn_80177CC8(_ENEMY_WORK* self);
void fn_80177D54(_ENEMY_WORK* self);
void fn_80177F30(_ENEMY_WORK* self, s32 index);
void fn_8017801C(_ENEMY_WORK* self, s32 index);

/* Pool literals owned by the data pass: the enemy action table and the aim scale constants. */
extern u32 lbl_8056FE10[];
extern f32 lbl_80797B50;
extern f32 lbl_80797B60;
extern f32 lbl_80797B64;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178128 - two-phase enemy action: arm the action, then aim at the target for motion ids 0x1F/0x20
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80178128(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_8056FE10, 0, 1, 0);
        fn_801353F8(self);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            s32 ang = calcVecAng2(&self->pos, &self->target);
            u16 rel = (u16)(ang - self->pos_0x1BC.y);

            self->v_0x310.z = lbl_80797B60 * fn_8012F8E4(self) * get_em_chg_scale(self);
            rotVecY(&self->v_0x310, self->pos_0x1BC.y + rel);
        }
        break;
    }
    case 1: {
        u32 done = fn_80134B0C(self, lbl_8056FE10);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            if (em_frame_check(self, 3, lbl_80797B50, lbl_80797B64) == 1) {
                CancelFade(self);
            }
        }

        if (done == 1) {
            fn_80127F48(self);
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017827C - tail-call this action's per-sub-state handler
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8017827C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177BEC(self, 0);
        break;
    case 1:
        fn_80177CC8(self);
        break;
    case 2:
        fn_80177D54(self);
        break;
    case 3:
        fn_80177F30(self, 0);
        break;
    case 4:
        fn_8017801C(self, 0);
        break;
    case 5:
        fn_80177BEC(self, 1);
        break;
    case 6:
        fn_80177F30(self, 1);
        break;
    case 7:
        fn_8017801C(self, 1);
        break;
    case 8:
        fn_8017801C(self, 2);
        break;
    case 9:
        fn_80177F30(self, 2);
        break;
    case 10:
        fn_8017801C(self, 3);
        break;
    case 11:
        fn_80178128(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801782F8 - two-phase action: arm the timer action, then finish on `fn_8012F93C`
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801782F8(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0: {
        u8 state = self->state_0x05;
        self->state_0x05 = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x32, 0x28, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}
