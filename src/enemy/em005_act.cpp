/*
 * enemy/em005_act.cpp - enemy 005's action band: the `_ENEMY_WORK` action and step functions, their dispatchers,
 *   and the static initializer that builds the four vector records.
 * RANGE. .text 0x801CA8DC-0x801D71C4 (116 functions); extab 0x8000FE4C-0x80010164, extabindex
 *   0x8002BC5C-0x8002C100, .ctors 0x8056F358-0x8056F35C (`fn_801D6FB8`), .rodata 0x80570410-0x80570500, .data
 *   0x805B4F90-0x805B69D8 (`em005_prog_tbl` first), .bss 0x806A7B30-0x806A7B90, .sdata 0x80791B20-0x80791B38,
 *   .sdata2 0x80799220-0x807994F8.  Left edge: the function after `enemy/em004_act.cpp`'s static initializer;
 *   right edge: `fn_801D6FB8` is this TU's static initializer and the 0.0 entry repeats at `lbl_807994FC` from
 *   `fn_801D71C4`.  The source is three blocks, 0x801CCBC4-0x801D428C, 0x801CA8DC-0x801CCBC4 and
 *   0x801D428C-0x801D71C4, each with its own declarations; the functions are not in address order.
 * FLAGS. `cflags_main`; `#pragma peephole off` and `#pragma fp_contract off` over the first block, the peephole off
 *   with `fp_contract` on over the second, the peephole on over the third.
 * NAMES. `em005` is the runtime dump's `em005_prog_tbl`, which opens the TU's `.data`; the `_act` suffix is a
 *   GUESS.  The map has only `fn_` stems for the functions.
 *   The `.bss` record names (`vec_pair_801CCBC4_*`) are GUESSes.
 * RESIDUALS. 23 rows unwritten: 0x801CAA20-0x801CAA8C, 0x801CAAA8-0x801CAF6C, 0x801CB350-0x801CB500,
 *   0x801CB9DC-0x801CBA4C, 0x801CBD30-0x801CC5DC, 0x801CC790-0x801CCBC4, 0x801CF0C0-0x801CF648,
 *   0x801CF840-0x801CFBF4, 0x801CFE04-0x801D076C, 0x801D0B94-0x801D0FE0, 0x801D1110-0x801D290C,
 *   0x801D320C-0x801D3C38, 0x801D3CF8-0x801D428C.
 *   28 partial rows; none has a recorded cause beyond one relocation finding (`relocdiff --by-owner`):
 *   `fn_801D4F78` calls `__cvt_fp2unsigned` four times where retail converts without the runtime call.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata` claimed, not emitted; `.data` 0x8E0 against 0x1A48, `.sdata2` 0xC
 *   against 0x2D8; `.text` (0x75D0 of 0xC8E8), extab (0x268 of 0x318) and extabindex (0x39C of 0x4A4) short of the
 *   claim and differing; the function order differs from the target's.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em005_act.h"
#include "enemy/em004_act.h"
#include "enemy/em007_act.h"
#include "Pl/pl_hit_sphere.h"
#include "enemy/fn_801251D0.h" /* fn_80128AAC, em_target_pos_set, em_hit_window_set (rule 2: the owner's header) */
#include "enemy/fn_8012EC74.h" /* fn_80133BB4 (rule 2: the owner's header) */
#include "ef/eft009.h"       /* eft009_set_pos (rule 2: the owner's header) */
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "sound/mhchar.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "stage/stg_w.h"

/* ===================================================================================================
 * 0x801CCBC4-0x801D428C
 * =================================================================================================== */
#pragma peephole off
#pragma fp_contract off

/* This block declares its callees itself: an owner header's differing spelling (`em_alt_mode_ck(void)`)
 * beside these is `10197 illegal function overloading`. */

/* The unit's `.sdata2` pool (0x80799220..0x807993E4), declared, not defined: the source does not emit it yet. */

extern f32 lbl_80799268;
extern f32 lbl_8079926C;
extern f32 lbl_80799278;
extern f32 lbl_8079927C;
extern f32 lbl_80799284;
extern f32 lbl_80799298;
extern f32 lbl_807992B8;
extern f32 lbl_807992C4;
extern f32 lbl_807992C8;
extern f32 lbl_807992CC;
extern f32 lbl_807992D0;
extern f32 lbl_807992D4;
extern f32 lbl_807992D8;
extern f32 lbl_807992DC;
extern f32 lbl_807992E0;
extern f32 lbl_807992E4;
extern f32 lbl_807992EC;
extern f32 lbl_807992F0;
extern f32 lbl_807992F8;
extern f32 lbl_807992FC;
extern f32 lbl_80799300;
extern f32 lbl_80799304;
extern f32 lbl_80799314;
extern f32 lbl_80799320;
extern f32 lbl_80799328;
extern f32 lbl_8079932C;
extern f32 lbl_80799330;
extern f32 lbl_80799334;
extern f32 lbl_80799338;
extern f32 lbl_8079933C;
extern f32 lbl_80799340;
extern f32 lbl_80799344;
extern f32 lbl_80799350;
extern f32 lbl_80799358;
extern f32 lbl_80799360;
extern f32 lbl_80799364;
extern f32 lbl_80799370;
extern f32 lbl_80799374;
extern f32 lbl_8079937C;
extern f32 lbl_80799384;
extern f32 lbl_8079938C;
extern f32 lbl_80799394;
extern f32 lbl_80799398;
extern f32 lbl_807993A8;
extern f32 lbl_807993B0;
extern f32 lbl_807993B4;
extern f32 lbl_807993B8;
extern f32 lbl_807993BC;
extern f32 lbl_807993C0;
extern f32 lbl_807993C4;
extern f32 lbl_807993C8;
extern f32 lbl_807993CC;
extern f32 lbl_807993D0;
extern f32 lbl_807993D4;
extern f32 lbl_807993D8;
extern f32 lbl_807993DC;
extern f32 lbl_807993E0;
extern f32 lbl_807993E4;

/* The three-word record `em_move_offset_rot_apply` takes; `fn_801CEF44` builds one from the latched
 * rotation word and two zeros.  size: 0x0C */
struct EmWord3 {
    /* +0x0 */ u32 x;
    /* +0x4 */ u32 y;
    /* +0x8 */ u32 z;
};

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

f32 get_em_scale(struct _ENEMY_WORK* self);              /* get_em_scale__FP11_ENEMY_WORK */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);          /* get_em_chg_scale__FP11_ENEMY_WORK */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                         /* em_frame_check__FP11_ENEMY_WORKUsff */
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                         /* em_after_frame_check__FP11_ENEMY_WORKUsff */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                                         /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */

/* The unit's `.data`, declared: `lbl_805B5088` is the record `em_key_curve_eval` reads, the `jumptable_*`s
 * the dispatch tables. */
extern u8 lbl_805B5088[];
extern u8 jumptable_805B5414[];
extern u8 jumptable_805B546C[];
extern u8 jumptable_805B54F4[];
extern u8 jumptable_805B551C[];
extern u8 jumptable_805B553C[];
extern u8 jumptable_805B5D90[];
extern u8 lbl_805704D0[];
extern u8 lbl_805B5000[];
extern u8 lbl_805B5050[];
extern u8 lbl_80570490[];
extern u8 lbl_805B50C0[];
extern u8 lbl_805B50F8[];
extern u8 lbl_805B6078[];
extern u8 lbl_805B6108[];
extern u8 lbl_805B6170[];
extern u8 lbl_805B61B8[];
extern u8 lbl_805B61E8[];
extern u8 lbl_805B5618[];
extern u8 lbl_805B5640[];
extern u8 lbl_805B5668[];
extern u8 lbl_805B5690[];
extern u8 lbl_805B56D0[];
extern u8 lbl_805B5730[];
extern u8 lbl_805B57A0[];
extern u8 lbl_805B5840[];
extern u8 lbl_805B58B0[];
extern u8 lbl_805B58D8[];
extern u8 lbl_805B5918[];
extern u8 lbl_805B5948[];
extern u8 lbl_805B59A0[];
extern u8 lbl_805B5A08[];
extern u8 lbl_805B5A40[];
extern u8 lbl_805B5AB0[];
extern u8 lbl_805B5B10[];
extern u8 lbl_805B5B38[];
extern u8 lbl_805B5B60[];
extern u8 lbl_805B5B88[];
extern u8 lbl_805B5BF8[];
extern u8 lbl_805B5C58[];
extern u8 lbl_805B5C80[];
extern u8 lbl_805B5CA8[];
extern u8 lbl_805B5CE8[];
extern u8 lbl_805B5D60[];

extern "C" {

/* The enemy-band callees, declared with the call sites' signatures: `enemy/em_common.cpp`'s first. */
void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);void em_se_tbl_play_alt(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
void fn_801277F4(struct _ENEMY_WORK* self, u32 a);
void em_action_finish(struct _ENEMY_WORK* self);
void em_action_finish_fall(struct _ENEMY_WORK* self);
void em_action_finish_walk(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hit_window_clear(struct _ENEMY_WORK* self, u32 a);

void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void CancelFade(struct _ENEMY_WORK* self);
void em_busy_set(struct _ENEMY_WORK* self);
u32 em_busy_ck(struct _ENEMY_WORK* self);

u32 fn_8012EC3C(struct _ENEMY_WORK* self);

void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012F7D4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8012F810(struct _ENEMY_WORK* self);
void fn_8012F860(struct _ENEMY_WORK* self, f32 a, f32 b);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
f32 get_em_base_scale(struct _ENEMY_WORK* self);
f32 fn_8012F8EC(struct _ENEMY_WORK* self);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
s32 fn_8012F948(struct _ENEMY_WORK* self);
u32 em_ground_ck(struct _ENEMY_WORK* self, f32 a);
f32 em_fall_height_get(struct _ENEMY_WORK* self);
f32 fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
void fn_801303FC(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void em_fall_start(struct _ENEMY_WORK* self);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_80131BD4(struct _ENEMY_WORK* self);
void em_frame_flag_set(struct _ENEMY_WORK* self);
void fn_80131D9C(struct _ENEMY_WORK* self);
void fn_80131E00(struct _ENEMY_WORK* self);
void em_busy_timer_reset(struct _ENEMY_WORK* self);
void fn_80133C3C(struct _ENEMY_WORK* self);
f32 fn_802B0430(u8 area);
u32 em_turn_to_target(struct _ENEMY_WORK* self, u32 a);
void fn_80133CC8(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);
void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);
u32 em_approach_step(struct _ENEMY_WORK* self, s32 a, s32 b);
u32 em_turn_seq_step(struct _ENEMY_WORK* self, void* tbl);
void em_lift_start(struct _ENEMY_WORK* self);
void em_lift_step(struct _ENEMY_WORK* self);
void em_dive_start(struct _ENEMY_WORK* self);
void em_dive_step(struct _ENEMY_WORK* self);
void fn_80134F70(struct _ENEMY_WORK* self, void* tbl);
void fn_80135000(struct _ENEMY_WORK* self, u32 a, void* tbl);
void em_move_vec_clr(struct _ENEMY_WORK* self);
void em_move_vec2_clr(struct _ENEMY_WORK* self);
void em_move_offset_apply(struct _ENEMY_WORK* self);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
void fn_80135584(struct _ENEMY_WORK* self, void* p);
void em_move_offset_step(struct _ENEMY_WORK* self, void* p);
u32 em_move_offset_step_update(struct _ENEMY_WORK* self, void* p);
f32 em_key_curve_eval(struct _ENEMY_WORK* self, void* tbl);
f32 fn_801356A8(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_80136D14(struct _ENEMY_WORK* self);

/* `enemy/fn_80138074.c` */
void fn_8013AAC4(struct _ENEMY_WORK* self);

/* `enemy/em001_prog.cpp` */
void em_spawn_rec_init(struct _ENEMY_WORK* self, u32 a);

/* This unit's 0x801CA8DC-0x801CCBC4 block, which the dispatchers tail-call (defined further down). */
void fn_801CAF70(struct _ENEMY_WORK* self);
void fn_801CAFBC(struct _ENEMY_WORK* self);
void fn_801CB008(struct _ENEMY_WORK* self);
void fn_801CB050(struct _ENEMY_WORK* self);
void fn_801CBA4C(struct _ENEMY_WORK* self, u8 a);
void fn_801CBB0C(struct _ENEMY_WORK* self, u8 a);
void fn_801CBBD8(struct _ENEMY_WORK* self);
void fn_801CBC64(struct _ENEMY_WORK* self, u8 a);
void fn_801CBD30(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_801CC5DC(struct _ENEMY_WORK* self, u8 a);
void fn_801CCB6C(struct _ENEMY_WORK* self, u32 a, u32 b);

/* Dispatchers `enemy/em007_act.cpp` shares (the two agree on the signatures). */
s32 fn_801D6548(struct _ENEMY_WORK* self, u8 a);
s32 fn_801D80EC();

} /* extern "C" */

/* This block's own step functions, declared before use (`fn_801D3CF8` also takes a mode word). */

extern "C" {
void fn_801CCBC4(struct _ENEMY_WORK* self, u32 mode);
void fn_801CCCE8(struct _ENEMY_WORK* self);
void fn_801CCE10(struct _ENEMY_WORK* self);
void fn_801CCF50(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD068(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD2E8(struct _ENEMY_WORK* self);
void fn_801CD2EC(struct _ENEMY_WORK* self);
void fn_801CD400(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD71C(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD8A0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD944(struct _ENEMY_WORK* self);
void fn_801CDCDC(struct _ENEMY_WORK* self, u8 mode);
void fn_801CDD94(struct _ENEMY_WORK* self);
void fn_801CDE34(struct _ENEMY_WORK* self);
void fn_801CDF10(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE014(struct _ENEMY_WORK* self);
void fn_801CE0C0(struct _ENEMY_WORK* self);
void fn_801CE190(struct _ENEMY_WORK* self);
void fn_801CE2B0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE3F0(struct _ENEMY_WORK* self);
void fn_801CE4A0(struct _ENEMY_WORK* self);
void fn_801CE5A0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE71C(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE898(struct _ENEMY_WORK* self);
void fn_801CE99C(struct _ENEMY_WORK* self);
void fn_801CEA68(struct _ENEMY_WORK* self);
void fn_801CEB28(struct _ENEMY_WORK* self);
void fn_801CEC44(struct _ENEMY_WORK* self);
void fn_801CEDE8(struct _ENEMY_WORK* self);
void fn_801CEE74(struct _ENEMY_WORK* self);
void fn_801CEF44(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF0C0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF648(struct _ENEMY_WORK* self);
void fn_801CF6A8(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF840(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFBF4(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFD28(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFE04(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D076C(struct _ENEMY_WORK* self, u8 mode);
void fn_801D08A8(struct _ENEMY_WORK* self);
void fn_801D0B94(struct _ENEMY_WORK* self, u8 mode);
void fn_801D0FE0(struct _ENEMY_WORK* self, u8 mode);
void fn_801D1110(struct _ENEMY_WORK* self, u8 mode);
void fn_801D1590(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D18DC(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D1E00(struct _ENEMY_WORK* self, u8 mode);
void fn_801D2144(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D290C(struct _ENEMY_WORK* self);
void fn_801D2B10(struct _ENEMY_WORK* self);
void fn_801D2E0C(struct _ENEMY_WORK* self);
void fn_801D2EB4(struct _ENEMY_WORK* self);
void fn_801D2EBC(struct _ENEMY_WORK* self);
void fn_801D2ED0(struct _ENEMY_WORK* self);
void fn_801D320C(struct _ENEMY_WORK* self);
void fn_801D3564(struct _ENEMY_WORK* self);
void fn_801D38A0(struct _ENEMY_WORK* self);
void fn_801D3C38(struct _ENEMY_WORK* self);
void fn_801D3CF8(struct _ENEMY_WORK* self, u32 mode);
} /* extern "C" */

extern "C" {

/* 0x801CCBC4 - one action's state machine: state 0 arms the motion and derives the aim angle, state 1
 * waits on `em_frame_check` and closes it. */
void fn_801CCBC4(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD1, 4, 0);
        {
            u32 angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            self->timer_0x020 = (s16)angle;
            if ((u32)self->timer_0x020 > 0x1555) {
                self->timer_0x020 = 0x1555;
            }
        }
        break;
    case 1:
        if ((u8)mode == 0 && em_frame_check(self, 3, lbl_80799220, lbl_807992C0) == 1) {
            f32 v = get_em_base_scale(self);
            u16 angle = (u16)(s32)((lbl_8079927C * (lbl_807992C4 * v)) / lbl_80799278 + lbl_80799280);
            em_turn_to_target(self, angle);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CCCE8 - the 22-way `state_sub` dispatcher: each arm tail-calls a step function; the default is the
 * implicit return (`cmplwi 0x15` + `bgtlr`). */
void fn_801CCCE8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CBA4C(self, 0); break;
    case 1: fn_801CBA4C(self, 1); break;
    case 2: fn_801CBB0C(self, 0); break;
    case 3: fn_801CBB0C(self, 1); break;
    case 4: fn_801CBBD8(self); break;
    case 5: fn_801CBC64(self, 0); break;
    case 6: fn_801CBD30(self, 0, 0, 0); break;
    case 7: fn_801CBD30(self, 1, 0, 0); break;
    case 8: fn_801CC5DC(self, 0); break;
    case 9: fn_801CC5DC(self, 1); break;
    case 10: fn_801CCB6C(self, 0, 0); break;
    case 11: fn_801CBD30(self, 2, 0, 0); break;
    case 12: fn_801CBD30(self, 3, 0, 0); break;
    case 13: fn_801CBC64(self, 1); break;
    case 14: fn_801CBD30(self, 0, 1, 0); break;
    case 15: fn_801CBD30(self, 1, 1, 0); break;
    case 16: fn_801CBD30(self, 2, 1, 0); break;
    case 17: fn_801CBD30(self, 3, 1, 0); break;
    case 18: fn_801CCBC4(self, 0); break;
    case 19: fn_801CCBC4(self, 1); break;
    case 20: fn_801CBD30(self, 0, 1, 1); break;
    case 21: fn_801CBD30(self, 1, 1, 1); break;
    }
}

/* 0x801CCE10 - the four-state open/close action: state 0 arms, state 1 waits and closes, state 2
 * aims at the reference position, state 3 releases the part pair. */
void fn_801CCE10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, lbl_805B5088);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x1A, 6, 0);
        }
        break;
    case 3:
        em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1) {
            fn_80128A70(self, 3, 2);
        }
        break;
    }
}

/* 0x801CCF50 - the same action shape as `fn_801CCE10` with the mode-1 prelude (`em_busy_set` +
 * `em_busy_timer_reset`) and a shorter close (state 2 ends through `em_action_finish_fall`). */
void fn_801CCF50(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, lbl_805B5088);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD068 - the five-state landing action: arm, wait under an `em_ground_ck` height test, then
 * descend onto the effect height (`field_0x20C`) at the effect scale, then release. */
void fn_801CD068(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck(self, lbl_807992C8) == 1) {
            f32 scale;
            self->state++;
            em_mot_set(self, 0x1D, 6, 0);
            scale = get_em_base_scale(self);
            self->field_0x314 = -(lbl_807992CC * get_em_scale(self) / lbl_80799284) * scale;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * scale) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x2E, 6, 0);
        }
        break;
    case 3:
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * scale) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CD2E8 - an empty body (the map's 4-byte `blr`). */
void fn_801CD2E8(struct _ENEMY_WORK* self) {
}

/* 0x801CD2EC - the two-state "motion + ::UpdateValue" action: state 0 arms the motion, state 1
 * writes the aim-angle-derived value and hands the motion its angle pair. */
void fn_801CD2EC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        fn_8012F810(self);
        em_mot_set(self, 0x1F, 0, 0);
        break;
    case 1: {
        f32 angle;
        u16 raw;
        u16 delta;
        em_target_pos_set(self, 0);
        raw = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
        delta = raw - self->field_0x1C0;
        if (delta == 0) {
            angle = lbl_80799220;
        } else {
            angle = (f32)(s16)delta * lbl_80799278 / lbl_8079927C / lbl_80799284;
        }
        fn_8012F860(self, angle, lbl_807992D0);
        fn_8012F7D4(self, 0x23, 0x24, (u32)(s32)fn_8012F8EC(self), self->field_0x464);
        break;
    }
    }
}

/* 0x801CD400 - the target search and approach action: state 1 runs two sub-steps and picks its target
 * group through `fn_80131034`; early finishes `return` to the shared epilogue. */
void fn_801CD400(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    fn_80131D9C(self);
    if ((u8)stage_map_kind_get(self->field_0x1E0) == 4) {
        if ((u32)(self->area_no - 4) <= 2) {
            fn_80136D14(self);
        }
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80134F70(self, lbl_805704D0);
        em_approach_start(self, lbl_80799220, 0x19);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        break;
    case 1:
        if (mode == 1) {
            switch (self->state_0x006) {
            case 0:
                if (self->field_0x1F9 == 0) {
                    struct _ENEMY_WORK* target;
                    self->state_0x006++;
                    target = 0;
                    if (fn_8012EC3C(self) == 1 || self->field_0x8A2 >= 0xFA) {
                        target = fn_80131034(self, 0x1C, 0);
                    }
                    if (target != 0) {
                        self->state_0x007 = 1;
                        break;
                    }
                    if (self->field_0x43D == 1) {
                        fn_80131BD4(self);
                    }
                }
                break;
            case 1:
                switch (self->state_0x007) {
                case 1: {
                    struct _ENEMY_WORK* target = fn_80131034(self, 0x1C, 1);
                    if (target != 0) {
                        fn_8012B380(self, 3, 2, target->group);
                        em_state_set(self, 0x0D, 0);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                case 2:
                    if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= lbl_807992D4) {
                        em_state_set(self, 3, 8);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                break;
            }
        }
        if (em_approach_step(self, 0, 0) == 1) {
            switch (mode) {
            case 1:
                if (self->area_no == self->field_0x9F8 || self->field_0x9F8 == 0xFF) {
                    fn_801CAF70(self);
                } else {
                    em_state_set(self, 3, 9);
                }
                return;
            case 2:
                if (self->field_0x1E7 == 0) {
                    fn_801CAF70(self);
                } else {
                    fn_801277F4(self, 0);
                    fn_80128A70(self, 3, 0x0E);
                }
                return;
            default:
                fn_801CAF70(self);
                return;
            }
        }
        switch (mode) {
        case 2:
            fn_80135000(self, 2, lbl_805704D0);
            break;
        case 3:
            fn_80135000(self, 3, lbl_805704D0);
            break;
        default:
            fn_80135000(self, 2, lbl_805704D0);
            break;
        }
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        break;
    }
}

/* 0x801CD71C - aims at the reference height: run `lbl_805B5000` twice, then `em_lift_step`; the mode picks
 * the height (area height, own vector y, or `fn_802B0430`). */
void fn_801CD71C(struct _ENEMY_WORK* self, u8 mode) {
    f32 limit;
    em_busy_set(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, lbl_805B5000);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1) {
                u8 count = self->state_0x007 + 1;
                self->state_0x007 = count;
                if (count >= 2) {
                    self->state_0x006++;
                    em_lift_start(self);
                }
            }
            break;
        case 1:
            em_lift_step(self);
            break;
        }
        switch (mode) {
        case 1:
            limit = lbl_807992E0 + self->vec_0x36C.y;
            break;
        case 2:
            limit = self->vec_0x36C.y;
            break;
        default:
            limit = fn_802B0430(self->area_no) - lbl_807992D8;
            if (limit - self->field_0x20C < lbl_807992DC) {
                limit = lbl_807992DC + self->field_0x20C;
            }
            break;
        }
        if (self->pos.y >= limit) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD8A0 - the mode-1 prelude plus a two-state hold that turns the work record by 0x200 and
 * ends through `em_action_finish_fall`. */
void fn_801CD8A0(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        fn_80131D9C(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD944 - the six-state "fly up, hover and throw" action.  State 2 runs two sub-steps through
 * `em_move_offset_step_update` (whose return selects the state-4 hand-over) and state 3/4/5 close the motion. */
void fn_801CD944(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x3D, 6, 0);
            self->vec_0x36C.y += lbl_807992E4 * get_em_chg_scale(self);
            self->state_0x006 = 0;
        }
        break;
    case 2:
        fn_80133C3C(self);
        switch (self->state_0x006) {
        case 0:
            fn_80133CC8(self, 0x100, 0x100);
            if (em_frame_check(self, 1, lbl_80799294, lbl_80799220) == 1) {
                self->state_0x006++;
                em_move_vec2_clr(self);
                self->field_0x318 = lbl_807992E8;
                self->field_0x324 = lbl_807992EC;
                em_approach_start(self, lbl_807992F0, 0x10);
            }
            break;
        case 1:
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            em_approach_step(self, 0, 0x80);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x3E, 6, 0);
        }
        break;
    case 3:
        fn_80133C3C(self);
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            em_mot_set(self, 0x2D, 6, 0);
            self->state_0x006 = 0;
            self->field_0x314 = lbl_807992F8;
            self->field_0x324 = lbl_807992FC;
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
        } else {
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
        }
        break;
    case 4: {
        u32 forward;
        switch (self->state_0x006) {
        case 0:
            em_fall_height_get(self);
            forward = em_move_offset_step_update(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, lbl_80799294, lbl_80799220) == 1) {
                self->state_0x006++;
                self->field_0x324 = lbl_80799300;
            }
            break;
        case 1:
            em_fall_height_get(self);
            forward = em_move_offset_step_update(self, &self->field_0x1BC);
            if (self->field_0x318 < lbl_80799220) {
                self->field_0x318 = lbl_80799220;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            if (forward == 1) {
                self->state++;
                em_move_mode_set(self, 3);
                em_mot_set(self, 0x1D, 6, 0);
            } else if (em_busy_ck(self) == 1) {
                em_action_finish_fall(self);
            }
        }
        break;
    }
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 3);
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CDCDC - the mode-gated turn: state 0 arms, state 1 turns by 0x180 (mode 1) and ends. */
void fn_801CDCDC(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 1:
        if (mode == 1) {
            em_turn_to_target(self, 0x180);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CDD94 - the timer action: state 0 arms a 0x3C-frame wait, state 1 counts it down and hands
 * control back to `fn_801CAF70` when it expires. */
void fn_801CDD94(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1F, 4, 0);
        self->timer_0x020 = 0x3C;
        break;
    case 1: {
        s32 timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0) {
            fn_801CAF70(self);
        }
        break;
    }
    }
}

/* 0x801CDE34 - the effect-scale action: state 0 arms with the two effect scales, state 1 clamps the
 * scale and ends through `fn_801CAF70`. */
void fn_801CDE34(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x20, 6, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_80799304;
        self->field_0x324 = lbl_807992EC;
        break;
    case 1:
        em_fall_height_get(self);
        em_move_offset_step_update(self, &self->field_0x1BC);
        if (self->field_0x318 > lbl_80799308) {
            self->field_0x318 = lbl_80799308;
        }
        if (em_mot_end_ck(self) == 1) {
            fn_801CAF70(self);
        }
        break;
    }
}

/* 0x801CDF10 - the mode-1 turn/clamp action. */
void fn_801CDF10(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
        fn_80131D9C(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        em_approach_start(self, lbl_80799220, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_807992B0;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_fall(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_8079930C) {
                self->field_0x318 = lbl_8079930C;
            }
        }
        break;
    }
}

/* 0x801CE014 - the mode-1 clamps plus a two-state open/close whose close hands over to
 * `fn_801CAFBC`. */
void fn_801CE014(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE0C0 - the three-state aim-and-close action. */
void fn_801CE0C0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799310, lbl_80799220) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE190 - the four-state height-gated close: state 1 waits on `em_ground_ck` (which reads the
 * `em_fall_height_get` return in f1), state 2 re-arms the motion, state 3 closes it. */
void fn_801CE190(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck(self, em_fall_height_get(self)) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x2E, 6, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CE2B0 - the mode-1 clamp set plus a two-sub-step hold that closes when the work record
 * reaches its own vector's height. */
void fn_801CE2B0(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1B, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, lbl_805B5050);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1) {
                u8 count = self->state_0x007 + 1;
                self->state_0x007 = count;
                if (count >= 2) {
                    self->state_0x006++;
                    em_lift_start(self);
                }
            }
            break;
        case 1:
            em_lift_step(self);
            break;
        }
        if (self->pos.y >= self->vec_0x36C.y) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE3F0 - the mode-1 clamps plus a two-state turn (state 1 turns by 0x200 and closes). */
void fn_801CE3F0(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE4A0 - the mode-1 clamps plus an `em_approach_step`-gated two-state hold. */
void fn_801CE4A0(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        em_approach_start(self, lbl_80799220, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_807992B0;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            fn_801CAFBC(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_8079930C) {
                self->field_0x318 = lbl_8079930C;
            }
        }
        break;
    }
}

/* 0x801CE5A0 - the effect-pair action: state 0 measures the height difference and seeds the two effect
 * scales, state 1 clamps `field_0x314` and runs `value_0x378` down. */
void fn_801CE5A0(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < lbl_80799220) {
            self->value_0x378 = self->value_0x378 * lbl_80799314;
        }
        em_move_vec2_clr(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = lbl_807992B0;
            self->field_0x320 = lbl_80799240;
        } else {
            self->field_0x314 = lbl_807992F8;
            self->field_0x320 = lbl_80799314;
        }
        break;
    case 1: {
        f32 limit;
        em_move_offset_step(self, &self->field_0x1BC);
        limit = (mode == 0) ? lbl_8079930C : lbl_80799284;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= lbl_80799220) {
            fn_801CAFBC(self);
        }
        break;
    }
    }
}

/* 0x801CE71C - `fn_801CE5A0`'s sibling with the other close (`em_action_finish_fall`) and mode 1's limit. */
void fn_801CE71C(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < lbl_80799220) {
            self->value_0x378 = self->value_0x378 * lbl_80799314;
        }
        em_move_vec2_clr(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = lbl_807992B0;
            self->field_0x320 = lbl_80799240;
        } else {
            self->field_0x314 = lbl_807992F8;
            self->field_0x320 = lbl_80799314;
        }
        break;
    case 1: {
        f32 limit;
        em_move_offset_step(self, &self->field_0x1BC);
        limit = (mode == 0) ? lbl_8079930C : lbl_80799284;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= lbl_80799220) {
            em_action_finish_fall(self);
        }
        break;
    }
    }
}

/* 0x801CE898 - the `state_sub` (+0x1E6) dispatcher that owns this whole band: 34 ways, each a tail
 * call into one of the range's step functions (case 19 is the empty arm). */
void fn_801CE898(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CCE10(self); break;
    case 1: fn_801CCF50(self, 0); break;
    case 2: fn_801CD068(self, 0); break;
    case 3: fn_801CD2E8(self); break;
    case 4: fn_801CD2EC(self); break;
    case 5: fn_801CD400(self, 0); break;
    case 6: fn_801CD71C(self, 0); break;
    case 7: fn_801CD8A0(self, 0); break;
    case 8: fn_801CD944(self); break;
    case 9: fn_801CD400(self, 1); break;
    case 10: fn_801CDCDC(self, 0); break;
    case 11: fn_801CDD94(self); break;
    case 12: fn_801CDE34(self); break;
    case 13: fn_801CD71C(self, 1); break;
    case 14: fn_801CD400(self, 2); break;
    case 15: fn_801CDCDC(self, 1); break;
    case 16: fn_801CD71C(self, 2); break;
    case 17: fn_801CDF10(self, 0); break;
    case 18: fn_801CD400(self, 3); break;
    case 19: break;
    case 20: fn_801CE014(self); break;
    case 21: fn_801CE0C0(self); break;
    case 22: fn_801CE190(self); break;
    case 23: fn_801CE2B0(self, 0); break;
    case 24: fn_801CE3F0(self); break;
    case 25: fn_801CE4A0(self); break;
    case 26: fn_801CE5A0(self, 0); break;
    case 27: fn_801CD8A0(self, 1); break;
    case 28: fn_801CCF50(self, 1); break;
    case 29: fn_801CD068(self, 1); break;
    case 30: fn_801CDF10(self, 1); break;
    case 31: fn_801CE5A0(self, 1); break;
    case 32: fn_801CE71C(self, 0); break;
    case 33: fn_801CE71C(self, 1); break;
    }
}

/* 0x801CE99C - the three-state aim/close action that ends through `em_action_finish_walk`. */
void fn_801CE99C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799310, lbl_80799220) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CEA68 - the sibling of `fn_801CE99C` that closes through `em_action_finish`. */
void fn_801CEA68(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2E, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CEB28 - the fade action: state 1 seeds the two effect scales from the model scale, state 2
 * switches between `em_move_offset_apply` and `CancelFade` on a frame window and closes. */
void fn_801CEB28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1E, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80799318, lbl_80799220) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->field_0x314 = lbl_8079931C * get_em_base_scale(self);
            self->field_0x320 = lbl_80799320 * get_em_base_scale(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799324, lbl_80799220) == 0) {
            em_move_offset_apply(self);
        } else {
            CancelFade(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CEC44 - descend onto the effect height (`fn_801CD068`'s sibling): walk `pos.y` down to
 * `field_0x20C`, then `em_action_finish_walk`, or re-arm at 0x1B. */
void fn_801CEC44(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck(self, lbl_807992C8) == 1) {
            f32 scale;
            self->state++;
            em_mot_set(self, 0x1D, 6, 0);
            scale = get_em_base_scale(self);
            self->field_0x314 = -(lbl_807992CC * get_em_scale(self) / lbl_80799284) * scale;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * scale) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
                if (em_mot_end_ck(self) == 1) {
                    em_action_finish_walk(self);
                }
            } else if (em_mot_end_ck(self) == 1) {
                em_mot_set_ck(self, 0x1B, 6, 0);
            }
        }
        break;
    }
}

/* 0x801CEDE8 - the two-state file-row action: state 0 copies the `lbl_80570490` row, state 1 waits on
 * it through `em_turn_seq_step` and closes with `em_action_finish_walk`. */
void fn_801CEDE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, lbl_80570490, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570490) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CEE74 - the mode-1 clamp action with `em_approach_start` and the `em_approach_step` gate. */
void fn_801CEE74(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        em_approach_start(self, lbl_80799328, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_80799310;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_walk(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
        }
        break;
    }
}

/* 0x801CEF44 - turns and re-seats: state 0 latches the rotation word, state 1 turns by +/-0x4000 (the mode's
 * sign), re-applies the word through `em_move_offset_rot_apply` and closes. */
void fn_801CEF44(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        if (mode == 0) {
            em_mot_set(self, 0xCB, 6, 0);
        } else {
            em_mot_set(self, 0xCC, 6, 0);
        }
        em_move_vec2_clr(self);
        self->field_0x37C = self->field_0x1C0;
        self->timer_0x328.field_0x328 =
            fn_801356A8(self, lbl_80799240, lbl_8079932C, lbl_80799330);
        fn_80130CDC(self, (u32)-0x0A);
        break;
    case 1: {
        struct EmWord3 rec;
        if (mode == 0) {
            em_turn_in_window(self, lbl_80799284, lbl_80799250, -0x4000);
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, lbl_805B50C0);
        } else {
            em_turn_in_window(self, lbl_80799284, lbl_80799250, 0x4000);
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, lbl_805B50C0);
        }
        self->field_0x318 = self->timer_0x328.field_0x328 * em_key_curve_eval(self, lbl_805B50F8);
        rec.x = 0;
        rec.y = self->field_0x37C;
        rec.z = 0;
        em_move_offset_rot_apply(self, &rec);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
    }
}

/* 0x801CF6A8 - the mode-selected "hold the part pair" action: state 0 arms one of four motion/part
 * sets, state 1 closes on `em_mot_end_ck`. */
void fn_801CF6A8(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set(self, 0xDC, 4, 0);
            em_hit_window_set(self, 0, 1, 8);
            em_hit_window_set(self, 1, 0x12, 0x10);
            break;
        case 1:
            em_mot_set(self, 0xDD, 4, 0);
            em_hit_window_set(self, 0, 2, 8);
            em_hit_window_set(self, 1, 0x13, 0x10);
            break;
        case 2:
            em_mot_set(self, 0xDC, 4, 0);
            em_hit_window_set(self, 0, 0x1E, 8);
            em_hit_window_set(self, 1, 0x12, 0x10);
            break;
        case 3:
            em_mot_set(self, 0xDD, 4, 0);
            em_hit_window_set(self, 0, 0x1F, 8);
            em_hit_window_set(self, 1, 0x13, 0x10);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CFBF4 - the mode-paired "arm and wait" action: state 0 arms 0xDE/0xE1 and latches the aim
 * angle into `timer_0x020`, state 1 turns by it and hands over to the mode's band function. */
void fn_801CFBF4(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set(self, 0xDE, 2, 0);
            break;
        case 1:
            em_mot_set(self, 0xE1, 2, 0);
            break;
        }
        self->timer_0x020 = (s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        break;
    case 1:
        em_turn_in_window(self, lbl_8079930C, lbl_80799310, self->timer_0x020);
        if (em_frame_check(self, 1, lbl_80799354, lbl_80799220) == 1) {
            self->state++;
            switch (mode) {
            case 0:
            case 2:
                fn_801CB008(self);
                break;
            case 1:
            case 3:
                fn_801CB050(self);
                break;
            }
        }
        break;
    }
}

/* 0x801CFD28 - `fn_801CFBF4`'s timer-based sibling (the same mode pair with `em_mot_set_ck`). */
void fn_801CFD28(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set_ck(self, 0xDE, 0, 0x48);
            break;
        case 1:
            em_mot_set_ck(self, 0xE1, 0, 0x48);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            switch (mode) {
            case 0:
                fn_801CB008(self);
                break;
            case 1:
                fn_801CB050(self);
                break;
            }
        }
        break;
    }
}

/* 0x801D2EB4 - the tail call into `fn_801CD068(self, 0)` (the small thunk the unit above's
 * dispatcher uses). */
void fn_801D2EB4(struct _ENEMY_WORK* self) {
    return fn_801CD068(self, 0);
}

/* 0x801D2EBC - `state_sub == 0` gates the same thunk. */
void fn_801D2EBC(struct _ENEMY_WORK* self) {
    if (self->state_sub != 0) {
        return;
    }
    fn_801D2EB4(self);
}

/* 0x801D2E0C - the `state_sub`-keyed file-row selector: five sub-states (plus the default) each
 * hand `em_se_tbl_play_alt` one of the band's `.data` rows and the row's own sub-state pair. */
void fn_801D2E0C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: return em_se_tbl_play_alt(self, lbl_805B6078, 0, 0);
    case 5: return em_se_tbl_play_alt(self, lbl_805B6108, 1, 5);
    case 0x0F: return em_se_tbl_play_alt(self, lbl_805B6170, 0, 0x0F);
    case 0x1A: return em_se_tbl_play_alt(self, lbl_805B61B8, 0, 0x1A);
    case 0x1C: return em_se_tbl_play_alt(self, lbl_805B61E8, 0, 0x1C);
    default: return em_se_tbl_play_alt(self, lbl_805B6078, 0, 0);
    }
}

/* 0x801D3C38 - the three-state "advance by 0x3E8 and close" action. */
void fn_801D3C38(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x14, 8, 0);
            fn_80130CDC(self, 0x3E8);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CF648 - the `state_sub` dispatcher over this range's 0x801CE99C.. band (10 ways). */
void fn_801CF648(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CE99C(self); break;
    case 1: fn_801CEA68(self); break;
    case 2: fn_801CEB28(self); break;
    case 3: fn_801CEC44(self); break;
    case 4: fn_801CEDE8(self); break;
    case 5: fn_801CEE74(self); break;
    case 6: fn_801CEF44(self, 0); break;
    case 7: fn_801CEF44(self, 1); break;
    case 8: fn_801CF0C0(self, 0); break;
    case 9: fn_801CF0C0(self, 1); break;
    }
}

/* 0x801D0FE0 - the two-mode "arm the part pair and turn" action. */
void fn_801D0FE0(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (mode == 0) {
            em_mot_set(self, 0xD5, 4, 0);
            em_hit_window_set(self, 0, 8, 8);
            em_hit_window_set(self, 1, 9, 0x18);
        } else {
            em_mot_set(self, 0xD9, 4, 0);
            em_hit_window_set(self, 0, 0x11, 8);
            em_hit_window_set(self, 1, 0x24, 0x18);
        }
        break;
    case 1:
        if (mode == 0) {
            em_turn_in_window(self, lbl_80799380, lbl_80799384, 0x4000);
        } else {
            em_turn_in_window(self, lbl_80799380, lbl_80799384, -0x4000);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D076C - the two-frame-window action: state 0 arms the part pair, state 1 opens the 1/2
 * windows (the second one turns the work record) and closes. */
void fn_801D076C(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD4, 4, 0);
        em_hit_window_set(self, 0, 3, 8);
        em_hit_window_set(self, 1, 4, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_8079935C, lbl_80799220) == 1) {
            em_hit_window_clear(self, 1);
            em_hit_window_set(self, 1, 0x10, 0x10);
        }
        if (em_frame_check(self, 2, lbl_80799270, lbl_80799220) == 1) {
            u16 angle = (u16)(s32)((lbl_8079927C * (lbl_80799360 * get_em_base_scale(self)))
                                       / lbl_80799278
                                   + lbl_80799280);
            em_turn_to_target(self, angle);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D290C - the `state_sub` dispatcher over this range's upper action band (48 ways; the table is
 * `jumptable_805B553C`).  Each arm is a tail call, so each case is a `return`. */
void fn_801D290C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CF6A8(self, 0); break;
    case 1: fn_801CF6A8(self, 1); break;
    case 2: fn_801CF840(self, 0); break;
    case 3: fn_801CFBF4(self, 0); break;
    case 4: fn_801CFBF4(self, 1); break;
    case 5: fn_801CFD28(self, 0); break;
    case 6: fn_801CFD28(self, 1); break;
    case 7: fn_801CFE04(self, 0, 0); break;
    case 8: fn_801CFE04(self, 1, 0); break;
    case 9: fn_801CFE04(self, 2, 0); break;
    case 10: fn_801CFE04(self, 3, 0); break;
    case 11: fn_801CFE04(self, 4, 0); break;
    case 12: fn_801CFE04(self, 5, 0); break;
    case 13: fn_801D076C(self, 0); break;
    case 14: fn_801D08A8(self); break;
    case 15: fn_801D0B94(self, 0); break;
    case 16: fn_801D0FE0(self, 0); break;
    case 17: fn_801D1110(self, 0); break;
    case 18: fn_801D1590(self, 0, 0); break;
    case 19: fn_801D1590(self, 1, 0); break;
    case 20: fn_801D18DC(self, 0, 0); break;
    case 21: fn_801D1E00(self, 0); break;
    case 22: fn_801CFE04(self, 0, 1); break;
    case 23: fn_801CFE04(self, 1, 1); break;
    case 24: fn_801CFE04(self, 2, 1); break;
    case 25: fn_801CFE04(self, 3, 1); break;
    case 26: fn_801CFE04(self, 4, 1); break;
    case 27: fn_801CFE04(self, 5, 1); break;
    case 28: fn_801D0B94(self, 1); break;
    case 29: fn_801D1110(self, 1); break;
    case 30: fn_801D18DC(self, 1, 0); break;
    case 31: fn_801CF840(self, 1); break;
    case 32: fn_801D1E00(self, 1); break;
    case 33: fn_801CF6A8(self, 2); break;
    case 34: fn_801CF6A8(self, 3); break;
    case 35: fn_801D2144(self, 0, 0); break;
    case 36: fn_801D2144(self, 1, 0); break;
    case 37: fn_801D2144(self, 0, 1); break;
    case 38: fn_801D2144(self, 1, 1); break;
    case 39: fn_801D1590(self, 2, 0); break;
    case 40: fn_801D076C(self, 1); break;
    case 41: fn_801D1590(self, 0, 1); break;
    case 42: fn_801D1590(self, 2, 1); break;
    case 43: fn_801D0FE0(self, 1); break;
    case 44: fn_801CF840(self, 2); break;
    case 45: fn_801CF840(self, 3); break;
    case 46: fn_801D1E00(self, 2); break;
    case 47: fn_801D1590(self, 3, 0); break;
    }
}

/* 0x801D2B10 - the 180-way `state_sub` dispatcher (0x17..0xCA): each case hands `em_se_tbl_play` its
 * `.data` cell and index; the default closes the action. */
void fn_801D2B10(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: return em_se_tbl_play(self, lbl_805B5668, 0x0, 0x17);
    case 0x18: return em_se_tbl_play(self, lbl_805B5618, 0x0, 0x18);
    case 0x19: return em_se_tbl_play(self, lbl_805B5730, 0x0, 0x19);
    case 0x1A: return em_se_tbl_play(self, lbl_805B57A0, 0x0, 0x1A);
    case 0x1B: return em_se_tbl_play(self, lbl_805B5618, 0x0, 0x1B);
    case 0x1C: return em_se_tbl_play(self, lbl_805B5730, 0x0, 0x1C);
    case 0x1D: return em_se_tbl_play(self, lbl_805B57A0, 0x0, 0x1D);
    case 0x1E: return em_se_tbl_play(self, lbl_805B5640, 0x0, 0x1E);
    case 0x23: return em_se_tbl_play(self, lbl_805B5840, 0x1, 0x23);
    case 0x38: return em_se_tbl_play(self, lbl_805B56D0, 0x0, 0x38);
    case 0x58: return em_se_tbl_play(self, lbl_805B5690, 0x0, 0x58);
    case 0x78: return em_se_tbl_play(self, lbl_805B5B10, 0x0, 0x78);
    case 0x7A: return em_se_tbl_play(self, lbl_805B58B0, 0x0, 0x7A);
    case 0x7B: return em_se_tbl_play(self, lbl_805B58D8, 0x0, 0x7B);
    case 0x7C: return em_se_tbl_play(self, lbl_805B5948, 0x0, 0x7C);
    case 0x7E: return em_se_tbl_play(self, lbl_805B59A0, 0x0, 0x7E);
    case 0x7F: return em_se_tbl_play(self, lbl_805B5A08, 0x0, 0x7F);
    case 0x84: return em_se_tbl_play(self, lbl_805B5AB0, 0x1, 0x84);
    case 0x8D: return em_se_tbl_play(self, lbl_805B5A40, 0x0, 0x8D);
    case 0x8E: return em_se_tbl_play(self, lbl_805B5A40, 0x0, 0x8E);
    case 0x9F: return em_se_tbl_play(self, lbl_805B5918, 0x0, 0x9F);
    case 0xA0: return em_se_tbl_play(self, lbl_805B5918, 0x0, 0xA0);
    case 0xA8: return em_se_tbl_play(self, lbl_805B5B38, 0x0, 0xA8);
    case 0xA9: return em_se_tbl_play(self, lbl_805B5840, 0x1, 0xA9);
    case 0xAF: return em_se_tbl_play(self, lbl_805B5618, 0x0, 0xAF);
    case 0xB0: return em_se_tbl_play(self, lbl_805B5840, 0x1, 0xB0);
    case 0xB6: return em_se_tbl_play(self, lbl_805B5B60, 0x0, 0xB6);
    case 0xB7: return em_se_tbl_play(self, lbl_805B5B88, 0x0, 0xB7);
    case 0xB8: return em_se_tbl_play(self, lbl_805B5BF8, 0x1, 0xB8);
    case 0xB9: return em_se_tbl_play(self, lbl_805B5C58, 0x0, 0xB9);
    case 0xBA: return em_se_tbl_play(self, lbl_805B5C80, 0x0, 0xBA);
    case 0xBB: return em_se_tbl_play(self, lbl_805B5CA8, 0x0, 0xBB);
    case 0xBC: return em_se_tbl_play(self, lbl_805B5CA8, 0x0, 0xBC);
    case 0xBF: return em_se_tbl_play(self, lbl_805B5CE8, 0x0, 0xBF);
    case 0xC1: return em_se_tbl_play(self, lbl_805B5A40, 0x0, 0xC1);
    case 0xCA: return em_se_tbl_play(self, lbl_805B5D60, 0x0, 0xCA);
    default: return em_action_finish(self);
    }
}

/* 0x801D08A8 - lean over and slide: state 0 latches the aim angle (clamped to 0x4000), state 1 runs three
 * frame windows, state 2 closes. */
void fn_801D08A8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0: {
        u32 diff;
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0xD6, 6, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_80799364 * get_em_base_scale(self);
        self->field_0x318 =
            self->field_0x318 * fn_801356A8(self, lbl_80799368, lbl_8079932C, lbl_8079926C);
        diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff >= 0x8000) {
            diff = 0x10000 - diff;
        }
        self->timer_0x020 = (s16)diff;
        if (self->timer_0x020 > 0x4000) {
            self->timer_0x020 = 0x4000;
        }
        break;
    }
    case 1:
        if (em_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            em_hit_window_set(self, 0, 0x0D, 0x0B);
        }
        if (em_frame_check(self, 2, lbl_80799294, lbl_80799220) == 1) {
            f32 scaled = (f32)self->timer_0x020 * lbl_80799278 / lbl_8079927C;
            u16 angle = (u16)(s32)((lbl_8079927C
                                    * (scaled / lbl_80799294 * get_em_base_scale(self)))
                                       / lbl_80799278
                                   + lbl_80799280);
            em_turn_to_target(self, angle);
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD7, 0, 0);
            self->field_0x324 = -self->field_0x318 / lbl_80799294 * get_em_base_scale(self);
            em_hit_window_set(self, 1, 0x19, 0x10);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80799370, lbl_80799220) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            em_mot_speed_set(self, lbl_80799280);
        }
        if (em_frame_check(self, 2, lbl_80799294, lbl_80799220) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 < lbl_80799220) {
                self->field_0x318 = lbl_80799220;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D2ED0 - steps onto the area's offset point: state 0 picks the per-area offset, states 1/2 slide
 * there, state 3 lands (the part levels pick the landing sub-step). */
void fn_801D2ED0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 have_spot = 0;
    VEC3_ctor(&spot);
    switch (self->state) {
    case 0:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 1:
                setVector3(&spot, lbl_807993B0, lbl_807993B4, lbl_807993B8);
                have_spot = 1;
                break;
            case 2:
                setVector3(&spot, lbl_807993BC, lbl_807993B4, lbl_807993C0);
                have_spot = 1;
                break;
            case 3:
                setVector3(&spot, lbl_807993C4, lbl_807993B4, lbl_807993C8);
                have_spot = 1;
                break;
            }
        }
        if (have_spot == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            copyVec3(&self->action_0x328.vec_0x334, &self->vec_0x36C);
            copyVec3(&self->vec_0x36C, &spot);
            fn_80134F70(self, lbl_805704D0);
            em_approach_start(self, lbl_80799220, 0x19);
            em_fall_height_get(self);
            fn_80135584(self, &self->field_0x1BC);
        } else {
            self->state = 3;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x1A, 0x0A, 0);
            em_dive_start(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            copyVec3(&self->vec_0x36C, &self->action_0x328.vec_0x334);
            em_mot_set(self, 0x1A, 0x0A, 0);
        } else {
            fn_80135000(self, 2, lbl_805704D0);
            em_fall_height_get(self);
            fn_80135584(self, &self->field_0x1BC);
        }
        break;
    case 2:
        if (em_turn_to_target(self, 0x200) == 1) {
            self->state++;
            em_dive_start(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 3: {
        f32 gap = self->vec_0x36C.y - self->pos.y;
        s32 timer;
        if (gap < lbl_807993CC) {
            em_dive_step(self);
        } else if (gap > lbl_807993D0) {
            self->pos.y = self->pos.y + lbl_807992B0;
        }
        gap = self->vec_0x36C.y - self->pos.y;
        timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0
            || (em_turn_to_target(self, 0x180) == 1 && gap >= lbl_807993CC && gap <= lbl_807993D0)) {
            if (em_parts_damage_level_get(self, 2) >= 1
                && em_parts_damage_level_get(self, 3) >= 1) {
                em_state_set(self, 0x0D, 6);
            } else {
                em_state_set(self, 0x0D, 5);
            }
        }
        break;
    }
    }
}

} /* extern "C" */

/* ===================================================================================================
 * 0x801CA8DC-0x801CCBC4
 * =================================================================================================== */
#pragma fp_contract on
extern "C" {
void fn_8013221C(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(struct _ENEMY_WORK* self);
void fn_80132264(struct _ENEMY_WORK* self);
}

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs the peephole folds. */
#pragma peephole off

/* The unit's `.rodata` tables, declared: the source does not emit them yet. */
extern u8 lbl_80570410[];
extern u8 lbl_80570450[];

extern "C" {

/* ----------------------------------------------------------------------------------------------- *
 * the action band's step functions
 * ----------------------------------------------------------------------------------------------- */

/* 0x801CAF6C - a 4-byte `blr`. */
void fn_801CAF6C(_ENEMY_WORK* self) {}

/* 0x801CAA8C - clears the action block: the float, the word and the armed byte. */
void fn_801CAA8C(_ENEMY_WORK* self) {
    self->timer_0x328.field_0x328 = lbl_80799220;
    self->timer_0x328.field_0x32C = 0;
    self->action_0x328.armed_0x328.field_0x330 = 0xFF;
}

/* 0x801CA8DC - average the motion slots' aim angles (the wrap-aware mean `fn_801CAA20` steps the
 * rotation by); 0xFFFF means none. */
u16 fn_801CA8DC(_ENEMY_WORK* self, u8 a) {
    if (self->field_0x218 == 0)
        return 0xFFFF;
    u32 angles[10];
    u8 count = 0;
    u8 i = 0;
    EmMotionSlot* slot = self->slots_0x244;
    VEC3* vec = &self->slots_0x244[0].vec;
    u32* p = angles;
    for (; i < 10; i++, slot++, vec++) {
        if (slot->flags == 0)
            break;
        if (a == 1 && (u32)(slot->value - 0x6000) > 0x4000)
            continue;
        if ((slot->flags & 0x800) == 0)
            continue;
        u32 x;
        u32 y;
        calcVecAngXY(vec, &x, &y);
        *p++ = y;
        count++;
    }
    if (count == 0)
        return 0xFFFF;
    if (count == 1)
        return (u16)angles[0];
    s32 avg = angles[0];
    u32* q = &angles[1];
    for (s8 i = 1; (u8)i < count; i++, q++) {
        s32 delta = (s32)*q - avg;
        if (delta > 0x8000)
            delta -= 0x10000;
        else if (delta < -0x8000)
            delta += 0x10000;
        avg += delta / (i + 1);
    }
    return (u16)avg;
}

/* 0x801CAF70 - enters the 0x0B motion. */
void fn_801CAF70(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 0x0B);
    fn_80133BB4(self);
}

/* 0x801CAFBC - enters the 0x14 motion. */
void fn_801CAFBC(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 0x14);
    fn_80133BB4(self);
}

/* 0x801CB008 - arms motion 7/5. */
void fn_801CB008(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 5);
    fn_80133BB4(self);
}

/* 0x801CB050 - arms motion 7/6. */
void fn_801CB050(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 6);
    fn_80133BB4(self);
}

/* 0x801CB098 - state 0 arms motion 1/0x0A, state 1 closes on the motion end. */
void fn_801CB098(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB114 - state 0 arms motion 2/4, state 1 closes on the motion end. */
void fn_801CB114(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB190 - the same body as 0x801CB114 (the band's second 2/4 step). */
void fn_801CB190(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB20C - state 0 re-seats the motion and arms 0x1A/6, state 1 runs it out. */
void fn_801CB20C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* 0x801CB28C - state 0 arms motion 0x1B/6, state 1 runs it out. */
void fn_801CB28C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* 0x801CB308 - the sub-state dispatcher over the six step functions above. */
void fn_801CB308(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801CB098(self);
        break;
    case 1:
        fn_801CB114(self);
        break;
    case 2:
        fn_801CB190(self);
        break;
    case 3:
        fn_801CB20C(self);
        break;
    case 6:
        fn_801CB28C(self);
        break;
    }
}

/* 0x801CB500 - state 0 arms motion 0x12/0x0A, state 1 runs it out. */
void fn_801CB500(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0x0A, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB57C - state 0 arms 0x13/6 and the 0x12C-frame timer, state 1 counts it down. */
void fn_801CB57C(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        self->timer_0x020 = 0x12C;
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 1, 3);
        break;
    }
}

/* 0x801CB618 - state 0 arms 0x14/8, state 1 runs it out. */
void fn_801CB618(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 8, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB694 - the four-step 0x17/0x18/0x19 action. */
void fn_801CB694(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_8079924C, 1, 0x14);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x19, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB7B0 - state 0 arms 0x7C/2, state 1 runs it out. */
void fn_801CB7B0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB82C - state 0 arms 4/4; a nonzero `a` first re-seats the motion. */
void fn_801CB82C(_ENEMY_WORK* self, u8 a) {
    if (a == 1)
        em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB8C0 - state 0 arms 3/4, state 1 runs it out. */
void fn_801CB8C0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB93C - state 0 arms 2/4, state 1 waits the `a`-selected frame window. */
void fn_801CB93C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, ((s32)(u8)a == 1) ? lbl_80799250 : lbl_80799254, lbl_80799220) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBA4C - state 0 arms 6/4 and starts the `a`-selected fade, state 1 waits it out. */
void fn_801CBA4C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 4, 0);
        if ((s32)(u8)a != 1)
            em_approach_start(self, lbl_80799258, 0);
        else
            em_approach_start(self, lbl_80799220, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBB0C - state 0 arms 7/6, sets the motion rate and starts the `a`-selected fade. */
void fn_801CBB0C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 6, 0);
        em_mot_speed_set(self, lbl_8079925C);
        if ((s32)(u8)a != 1)
            em_approach_start(self, lbl_80799258, 0);
        else
            em_approach_start(self, lbl_80799220, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBBD8 - state 0 starts the `lbl_80570410` table effect, state 1 waits it out. */
void fn_801CBBD8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570410, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570410) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBC64 - state 0 starts the `lbl_80570450` table effect (and the 0x482 fade), state 1 waits. */
void fn_801CBC64(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570450, 0, 1, 0);
        if (self->field_0x482 == 1)
            em_mot_speed_set(self, lbl_80799260);
        break;
    case 1:
        if (a == 1 && em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (em_turn_seq_step(self, lbl_80570450) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CC5DC - the strafe/approach action: state 0 seats the offset vector, state 1 steers it. */
void fn_801CC5DC(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_move_vec2_clr(self);
        switch (a) {
        case 0: {
            em_mot_set(self, 0xD2, 4, 0);
            em_mot_speed_set(self, lbl_8079925C);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = lbl_807992A0 * rate * scale;
            break;
        }
        case 1: {
            em_mot_set(self, 0xD3, 4, 0);
            em_mot_speed_set(self, lbl_8079925C);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = lbl_807992A4 * rate * scale;
            break;
        }
        }
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        em_hit_window_set_default(self, 0, 0x18);
        break;
    case 1:
        switch (a) {
        case 0:
            em_turn_in_window(self, lbl_80799220, lbl_807992A8, 0x4000);
            break;
        case 1:
            em_turn_in_window(self, lbl_80799220, lbl_807992A8, -0x4000);
            break;
        }
        if (em_frame_check(self, 3, lbl_80799220, lbl_807992A8) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

} /* extern "C" */

/* ===================================================================================================
 * 0x801D428C-0x801D71C4
 * =================================================================================================== */
#pragma peephole on
/* The unit's `.data` tables the code loads, declared: the source does not emit them yet. */
extern u8 lbl_805B61F8[];
extern u8 lbl_805B63A8[];
extern u8 lbl_805B63E8[];
extern u8 lbl_805B6950[];
extern VEC3 vec_pair_801CCBC4_0[2];
extern VEC3 vec_pair_801CCBC4_1[2];
extern VEC3 vec_pair_801CCBC4_2[2];
extern VEC3 vec_pair_801CCBC4_3[2];

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

u16 calcVecAngX(nw4r::math::VEC3* v);                       /* calcVecAngX__FPQ34nw4r4math4VEC3 */
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);  /* setVector3__FPQ34nw4r4math4VEC3fff */
void rotVecY(nw4r::math::VEC3* v, u32 angle);               /* rotVecY__FPQ34nw4r4math4VEC3Ul */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                            /* em_frame_check__FP11_ENEMY_WORKUsff */
s32 em_die_ck(struct _ENEMY_WORK* self);                    /* em_die_ck__FP11_ENEMY_WORK */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                /* em_get_mot_no__FP11_ENEMY_WORK */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                                            /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);             /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                            /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void* get_move_work_adrs(u8 index);                         /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                            /* get_move_work_max__FUc */

/* The runtime's allocator pair (`__nw__FUl`/`__dl__FPv`), declared as the C++ functions (rule 9). */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures: `enemy/enemy_control.cpp`'s first. */
s16 em_demo_frame_get();
u32 em_demo_time_ck(u32 id);
void em_demo_pos_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_reset(struct _ENEMY_WORK* self, u32 a);
void em_demo_key3_apply(struct _ENEMY_WORK* self, s16 a, void* b, u32 c);
void em_demo_key_apply(struct _ENEMY_WORK* self, s16 a, void* b, void* c, u32 d, u32 e);
void em_demo_enable(struct _ENEMY_WORK* self);

/* `enemy/em_common.cpp` */
void fn_80126278(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);

void em_busy_set(struct _ENEMY_WORK* self);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);

/* `fn_8012EC3C` and `em_alt_mode_ck` read the work record (+0x89F, +0x8AA). */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

void em_mot_set_blend(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
s32 fn_8012F948(struct _ENEMY_WORK* self);
void em_hit_window_clear(struct _ENEMY_WORK* self, u32 a);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 fn_8013023C(struct _ENEMY_WORK* self);
f32 fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void em_fall_start(struct _ENEMY_WORK* self);
void fn_80133C3C(struct _ENEMY_WORK* self);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
f32 em_key_curve_eval(struct _ENEMY_WORK* self, void* tbl);
void fn_801369A0(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
void fn_80130F74(struct _ENEMY_WORK* self);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_8013221C(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(struct _ENEMY_WORK* self);
void fn_80132264(struct _ENEMY_WORK* self);
u8* fn_801377D0(u8 index);
void em_part_hit_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801376B4(struct _ENEMY_WORK* self);

/* `enemy/fn_80138074.c` */
void em_res_user_data_set(struct _ENEMY_WORK* self, void* helper);
void fn_8013918C(void* helper, s16 flag);
s32 em_res_user_data_ck(struct _ENEMY_WORK* self);

/* `enemy/em001_prog.cpp` */
void* em_res_user_data_ctor(void* self);

/* The effect and runtime callees. */
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, nw4r::math::VEC3* pos,
                 f32 scale);
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_801057FC(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, s32 d);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);
void fn_800FA378(void* out);
void fn_800FA3B8(void* out);
void assignVec3(void* out, const void* in);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void draw_shape_arm(struct _ENEMY_WORK* self, u32 a, u32 b);

/* This unit's dispatch targets in the blocks above, declared again for this block. */
void fn_801CCCE8(struct _ENEMY_WORK* self);
void fn_801CE898(struct _ENEMY_WORK* self);
void fn_801CF648(struct _ENEMY_WORK* self);
void fn_801D290C(struct _ENEMY_WORK* self);
void fn_801D2B10(struct _ENEMY_WORK* self);
void fn_801D2E0C(struct _ENEMY_WORK* self);
void fn_801D2EBC(struct _ENEMY_WORK* self);
void fn_801D2ED0(struct _ENEMY_WORK* self);
void fn_801D320C(struct _ENEMY_WORK* self);
void fn_801D3564(struct _ENEMY_WORK* self);
void fn_801D38A0(struct _ENEMY_WORK* self);
void fn_801D3C38(struct _ENEMY_WORK* self);
void fn_801D3CF8(struct _ENEMY_WORK* self, u32 a);

void eft_em_spawn_joint(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, u32 d);

s32 fn_802907BC(s32 a, void* b);
void fn_802B43A8(void* pos, u8 a, u16 b);
void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);

/* ----------------------------------------------------------------------------------------------------
 * the records this unit needs locally
 * -------------------------------------------------------------------------------------------------- */

/* The two stack records `fn_801D66BC` builds (`fn_800FA3B8`/`fn_800FA378` fill them); only their sizes
 * are known.  size: 0x20 */
struct EmScratchA {
    /* +0x00 */ u8 unused_0x00[0x20];
};

/* size: 0x40 */
struct EmScratchB {
    /* +0x00 */ u8 unused_0x00[0x40];
};

/* The area-entry record `fn_801377D0` returns, as `fn_801D6758` reads it.
 * size: 0x5A8 (only the three bytes the caller reads are named) */
struct EmAreaEntry801CCBC4 {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x016 - 0x001];
    /* +0x016 */ u8 area_no;
    /* +0x017 */ u8 unused_0x017[0x5A6 - 0x017];
    /* +0x5A6 */ u8 flags_0x5A6;
    /* +0x5A7 */ u8 unused_0x5A7[0x5A8 - 0x5A7];
};

/* One `lbl_805B6950` entry: the two records `fn_8004FFC8` interpolates between and the scale at +0x18.
 * size: 0x1C */
struct EmGrowTable801CCBC4 {
    /* +0x00 */ u8 rec_a[0xC];
    /* +0x0C */ u8 rec_b[0xC];
    /* +0x18 */ f32 scale;
};

/* The 0x18-byte spawn record `fn_801D6EDC` fills (`id` word, a vector, then the three scalars).
 * size: 0x18 */
struct EmSpawnRec801CCBC4 {
    /* +0x00 */ u32 id;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
};

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

/* this range's own functions that a later body calls before its definition */
void fn_801D4CE0(struct _ENEMY_WORK* self);
void fn_801D4DD8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);

void fn_801D4F78(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 area_flag;
    u8 area;
    u8 flags;

    VEC3_ctor(&spot);
    fn_801D4CE0(self);
    switch (em_get_mot_no(self)) {
    case 0x8:
        if ((em_after_frame_check(self, 0, lbl_807993A4, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799444, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 0, 0x25, 0, lbl_8079925C);
            fn_801D4DD8(self, 0, 0, 0x2D, 0, lbl_8079925C);
        }
        if ((em_after_frame_check(self, 0, lbl_807992F4, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079934C, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
            em_camera_req(self, -1, 0);
        }
        if ((em_after_frame_check(self, 0, lbl_8079944C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799450, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, lbl_80799390, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799454);
            em_camera_req(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799458);
            em_camera_req(self, 0x11, 7);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            em_hit_window_set(self, 0, 0x14, 8);
            em_hit_window_set(self, 1, 0x15, 0x10);
        }
        if (em_after_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x11, 0, lbl_8079925C);
        }
        break;
    case 0xD:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            em_hit_window_set(self, 0, 0x16, 8);
            em_hit_window_set(self, 1, 0x17, 0x10);
        }
        if (em_after_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 8, 0, lbl_8079925C);
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, lbl_807992C0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x1E:
        if (em_after_frame_check(self, 0, lbl_8079945C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x25:
    case 0x26:
        if (em_after_frame_check(self, 0, lbl_807992B0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x29:
        if (em_after_frame_check(self, 0, lbl_807992F4, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079928C);
            fn_801369A0(self, 1, 0, &spot, lbl_80799460);
        }
        break;
    case 0x2A:
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
        }
        break;
    case 0x41:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 3, 0, lbl_80799288);
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079928C);
            fn_801369A0(self, 2, 0, &spot, lbl_80799368);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x44:
        if (em_after_frame_check(self, 0, lbl_807992C0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799310, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, lbl_80799464, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 0, 4, 0x2AAB, lbl_80799260);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 0, 4, 0xD556, lbl_80799260);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 5, 4, 0, lbl_80799240);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x72:
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x19, 0, lbl_8079924C);
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, lbl_8079946C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x19, 0, lbl_8079924C);
        }
        break;
    case 0x78:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x25, 0, lbl_8079925C);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x2D, 0, lbl_8079925C);
        }
        if (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 0x18, 0, lbl_80799288);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, lbl_80799310, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799288);
        }
        if (em_after_frame_check(self, 0, lbl_80799470, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 4, 0, lbl_80799240);
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 2, 4, 0, lbl_8079925C);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x80:
        if (em_after_frame_check(self, 0, lbl_80799474, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x1C, 0, lbl_80799454);
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, lbl_80799478, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x1A, 0, lbl_807993A0);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x83:
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            em_camera_req(self, -1, 7);
        }
        if ((em_after_frame_check(self, 0, lbl_80799354, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079944C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079939C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0x84:
        if (em_after_frame_check(self, 0, lbl_8079947C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xC9:
    case 0xCA:
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 6, 4, 0, lbl_80799348);
        }
        break;
    case 0xCB:
    case 0xCC:
        if ((em_after_frame_check(self, 0, lbl_807992B0, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 2, 0, 4, 0, lbl_80799230);
        }
        break;
    case 0xCD:
    case 0xCF:
        if (em_after_frame_check(self, 0, lbl_80799294, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 4, 0, lbl_80799460);
        }
        break;
    case 0xCE:
    case 0xD0:
        if (em_after_frame_check(self, 0, lbl_807993A4, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 4, 0, lbl_80799230);
        }
        break;
    case 0xD1:
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 8, 2, 0, &spot, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0xD3:
        if (em_after_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 0x11, 2, 0, &spot, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xD4:
        if ((u8) self->action == 0xA) {
            if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
                em_hit_window_set(self, 0, 3, 8);
                em_hit_window_set(self, 1, 4, 0x18);
            }
            if (em_frame_check(self, 0, lbl_8079935C, lbl_80799220) == 1) {
                em_hit_window_clear(self, 1);
                em_hit_window_set(self, 1, 0x10, 0x10);
            }
        }
        if (em_after_frame_check(self, 0, lbl_80799270, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
            em_camera_req(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            em_camera_req(self, 0x11, 7);
        }
        break;
    case 0xD5:
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x2D, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799324, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xD7:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xD9:
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x25, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799324, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xDC:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, lbl_80799480, lbl_80799220) == 1)) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            eft_em_spawn_joint(self, 1, 0x1A, &spot, 0xC, lbl_80799240);
            eft_em_spawn_joint(self, 1, 0x1C, &spot, 0xC, lbl_80799240);
        }
        break;
    case 0xDD:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, lbl_80799480, lbl_80799220) == 1)) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            eft_em_spawn_joint(self, 1, 0x19, &spot, 0xC, lbl_80799240);
            eft_em_spawn_joint(self, 1, 0x1B, &spot, 0xC, lbl_80799240);
        }
        break;
    case 0xDE:
    case 0xE1:
        if (em_after_frame_check(self, 0, lbl_80799484, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
        }
        break;
    case 0xDF:
        if (em_after_frame_check(self, 0, lbl_80799488, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            eft_em_spawn(self, 0x7C, 0x14, &spot, lbl_80799240);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xE2:
        if (em_after_frame_check(self, 0, lbl_80799488, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            eft_em_spawn(self, 0x7D, 0xB, &spot, lbl_80799240);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xE4:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xE9:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0xEC:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
        }
        break;
    case 0xED:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801369A0(self, 1, 0, NULL, lbl_80799240);
        }
        break;
    case 0xEF:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xF0:
    case 0xF1:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xF2:
    case 0xF3:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xF6:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            em_camera_req(self, -1, 1);
        }
        break;
    }
    area_flag = 0;
    if ((u8)stage_map_kind_get(self->field_0x1E0) == 4 && (self->area_no == 4 || self->area_no == 6)) {
        area_flag = 1;
    }
    if (em_alt_mode_ck(self) == 1 || area_flag == 1) {
        self->field_0x761 = (u8)(self->field_0x761 | 1);
    } else {
        flags = self->field_0x761;
        if ((flags & 1) != 0) {
            self->field_0x761 = (u8)(flags & 0xFE);
        }
    }
    if (area_flag == 1 && self->field_0x762 == 0) {
        self->field_0x761 = (u8)(self->field_0x761 | 2);
        return;
    }
    flags = self->field_0x761;
    if ((flags & 2) != 0) {
        self->field_0x761 = (u8)(flags & 0xFD);
    }
}

void fn_801D428C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;

    VEC3_ctor(&spot);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x10E) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 1, 0, 0);
            em_demo_rot_set(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
            em_demo_pos_set(self, lbl_807993F4, lbl_807993F8, lbl_807993FC);
        }
        break;
    case 2:
        em_demo_rot_set(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (em_demo_time_ck(0x1B4) == 1) {
            self->state++;
            em_mot_set(self, 2, 0xA, 0);
        }
        break;
    case 3:
        em_demo_rot_set(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (em_frame_check(self, 1, lbl_80799254, lbl_80799220) == 1) {
            self->state++;
            em_mot_set(self, 8, 8, 0);
            em_mot_speed_set(self, lbl_80799368);
        }
        break;
    case 4:
        em_demo_rot_set(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (em_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 1, 5, 0, &spot, lbl_807993A0);
        }
        if (em_demo_time_ck(0x294) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDA, 0, 0x56);
            em_mot_speed_set(self, lbl_80799280);
            em_demo_rot_set(self, lbl_80799220, lbl_80799400, lbl_80799220);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805B61F8, lbl_805B63A8, 7, 2);
            fn_801303EC(self, self->pos.y - self->field_0x20C);
        }
        break;
    case 5:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x2AE) == 1) {
            self->state++;
            em_mot_set(self, 0xDB, 0, 0);
        }
        break;
    case 6:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            get_joint_wpos_em(self, 3, &spot);
            spot.y = lbl_807992E8 + self->pos.y;
            eft_spawn_pos_in_area(&spot, self->area_no, 5, 0, lbl_80799408);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, lbl_80799288);
        }
        break;
    case 7:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, lbl_80799354, lbl_80799220) == 1) {
            self->state++;
            em_mot_speed_set(self, lbl_80799240);
        }
        break;
    case 8:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x3FC) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0x110);
            em_demo_rot_set(self, lbl_80799220, lbl_8079940C, lbl_80799220);
            em_demo_pos_set(self, lbl_80799410, lbl_80799220, lbl_80799414);
            fn_801303EC(self, lbl_80799220);
        }
        break;
    case 9:
        if (em_frame_check(self, 1, lbl_80799418, lbl_80799220) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0xA, 0);
        }
        break;
    case 10:
        if (em_demo_time_ck(0x42A) == 1) {
            em_demo_rot_set(self, lbl_80799220, lbl_8079941C, lbl_80799220);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805B63E8, 0);
            fn_801303EC(self, self->pos.y - self->field_0x20C);
        } else if (fn_8012F948(self) == 0) {
            self->field_0x318 = em_key_curve_eval(self, lbl_805B52D8);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_demo_time_ck(0x454) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0, 0);
            em_mot_speed_set(self, lbl_80799420);
        }
        break;
    case 11:
        em_demo_rot_set(self, lbl_80799220, lbl_8079941C, lbl_80799220);
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805B63E8, 0);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, lbl_80799424, lbl_80799220) == 1) {
            self->state++;
            em_mot_speed_set(self, lbl_80799240);
        }
        break;
    case 12:
        em_demo_rot_set(self, lbl_80799220, lbl_8079941C, lbl_80799220);
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805B63E8, 0);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x49C) == 1) {
            self->state++;
            em_demo_rot_set(self, lbl_80799220, lbl_80799428, lbl_80799220);
            em_demo_pos_set(self, lbl_8079942C, lbl_80799220, lbl_80799430);
        }
        break;
    case 13:
        if (em_demo_time_ck(0x5AA) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0);
            em_mot_speed_set(self, lbl_80799434);
        }
        break;
    case 14:
        if (em_demo_time_ck(0x6E4) == 1) {
            self->state++;
            em_mot_set(self, 5, 0, 0);
            em_demo_rot_set(self, lbl_80799220, lbl_80799428, lbl_80799220);
            em_demo_pos_set(self, lbl_80799438, lbl_80799220, lbl_8079943C);
            self->timer_0x020 = 0;
        }
        break;
    case 15:
        if (em_frame_check(self, 0, lbl_80799238, lbl_80799220) == 1) {
            draw_shape_arm(self, 0x1D, 0xA);
        }
        if (em_frame_check(self, 0, lbl_80799238, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            eft_em_spawn(self, 0, 0x1B, &spot, lbl_80799240);
        }
        if (em_frame_check(self, 3, lbl_80799244, lbl_80799248) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
                eft_em_spawn(self, 1, 0x1B, &spot, lbl_80799240);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, lbl_80799440);
        }
        break;
    }
}

void fn_801D4B84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_rot_set(self, lbl_80799220, lbl_80799428, lbl_80799220);
        em_demo_pos_set(self, lbl_80799438, lbl_80799220, lbl_8079943C);
        fn_801303EC(self, lbl_80799220);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D4C3C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D2ED0(self);
        break;
    case 1:
        fn_801D320C(self);
        break;
    case 2:
        fn_801D3564(self);
        break;
    case 3:
        fn_801D38A0(self);
        break;
    case 4:
        fn_801D3C38(self);
        break;
    case 5:
        fn_801D3CF8(self, 0);
        break;
    case 6:
        fn_801D3CF8(self, 1);
        break;
    case 7:
        fn_801D428C(self);
        break;
    case 8:
        fn_801D4B84(self);
        break;
    }
}

void fn_801D4C90(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801CB308(self);
        break;
    case 1:
        fn_801CB9DC(self);
        break;
    case 2:
        fn_801CCCE8(self);
        break;
    case 3:
        fn_801CE898(self);
        break;
    case 4:
        fn_801CF648(self);
        break;
    case 5:
        fn_801D290C(self);
        break;
    case 6:
        fn_801D2B10(self);
        break;
    case 7:
        fn_801D2E0C(self);
        break;
    case 8:
        fn_801D2EBC(self);
        break;
    case 9:
        fn_801D4C3C(self);
        break;
    }
}

void fn_801D4CE0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u16 ang;

    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            u16 mot = em_get_mot_no(self);
            if (mot != 0xD8 && mot != 0xE4) {
                setVector3(&pos, lbl_80799220, lbl_807992BC, lbl_80799380);
                if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38) {
                        eft_spawn_type10(self, 0x16, 0x1C, &pos, lbl_80799240);
                    } else {
                        eft_spawn_type10(self, 0x15, 0x1C, &pos, lbl_80799240);
                    }
                }
            }
        }
    }
}

void fn_801D4DD8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (arg1) {
    case 0:
        if ((u32)(arg2 - 0xC) > 0xD) {
            if (arg2 != 0x26) {
                if (arg3 == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = lbl_807992E8 + self->field_0x20C;
                    pos.z = self->pos.z;
                    eft009_set_pos(arg2, &pos, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, arg2, arg4, arg5);
                }
            }
        }
        break;
    case 1:
        if (arg3 == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &pos);
        }
        pos.y = self->field_0x20C;
        eft_spawn_pos_in_area(&pos, self->area_no, arg2, self->field_0x1C0,
                    arg5 * get_em_chg_scale(self));
        break;
    case 2:
        if (arg3 == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &pos);
        }
        pos.y = self->field_0x20C;
        eft_spawn_type11(self, &pos, arg2, arg5 * get_em_chg_scale(self));
        break;
    }
}

void fn_801D64A4(struct _ENEMY_WORK* self) {
    _GXColor color;

    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (em_alt_mode_ck(self) == 1) {
        if (color.a < 251) {
            color.a = color.a + 4;
        } else {
            color.a = 255;
        }
    } else {
        if (color.a > 4) {
            color.a = color.a - 4;
        } else {
            color.a = 0;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

s32 fn_801D6548(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;
    struct _ENEMY_WORK* other;
    f32 dist;
    f32 scale;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    other = fn_80131034(self, 0x1C, 0);
    if (other != NULL && fn_8012E5A8(other) == 1) {
        if (arg1 == 0) {
            return 1;
        }
        copyVec3(&b, setVec3(&d, lbl_80799220, lbl_80799220,
                                    lbl_80799264 * get_em_chg_scale(self)));
        rotVecY(&b, self->field_0x1C0);
        addVec3(&c, &self->pos, &b);
        copyVec3(&a, &c);
        dist = calcDistanceSqXZ(&a, &other->pos);
        scale = lbl_8079948C * get_em_chg_scale(self);
        if (dist < lbl_8079948C * get_em_chg_scale(self) * scale) {
            return 1;
        }
    }
    return 0;
}

u32 fn_801D6694(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 4) {
        return 1;
    }
    return 0;
}

s32 fn_801D66BC(u8 arg0, void* target) {
    EmScratchA scratchA;
    EmScratchB scratchB;
    EmGrowTable801CCBC4* entry;

    fn_800FA3B8(&scratchA);
    fn_800FA378(&scratchB);
    if (arg0 >= 3) {
        return 0;
    }
    entry = (EmGrowTable801CCBC4*)lbl_805B6950 + arg0;
    fn_8004FFC8(entry->rec_a, entry->rec_b, &scratchA, entry->scale);
    fn_8028F558(&scratchA, &scratchB);
    return fn_802907BC((s32)target, &scratchB) - 1 == 0;
}

s32 fn_801D6758(struct _ENEMY_WORK* self) {
    EmAreaEntry801CCBC4* entry;

    if (self->field_0x382 != 0xFF && self->field_0x380 == 1) {
        entry = (EmAreaEntry801CCBC4*)fn_801377D0(self->state_0x381);
        if (entry->active != 0 && entry->area_no == self->area_no &&
            (entry->flags_0x5A6 & 0x7F) == 1) {
            return 1;
        }
    }
    return 0;
}

u8 fn_801D67D8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, nw4r::math::VEC3* target) {
    nw4r::math::VEC3 pos;
    f32 dist;
    u8 best;
    u8 id;
    s32 i;

    VEC3_ctor(&pos);
    i = 0;
    id = arg1;
    best = arg2;
    for (i = 0; i < (s32)arg2; i++) {
        fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | id), &pos);
        if (i == 0) {
            dist = calcDistanceSqXZ(target, &pos);
            best = (u8)i;
        } else {
            f32 d = calcDistanceSqXZ(target, &pos);
            if (dist > d) {
                dist = d;
                best = (u8)i;
            }
        }
        id++;
    }
    return best;
}

u32 fn_801D68A8(struct _ENEMY_WORK* self, u32 arg1) {
    nw4r::math::VEC3 pos;
    u32 t;
    u8 sel;

    VEC3_ctor(&pos);
    sel = (u8)arg1;
    switch (sel) {
    case 0:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 2:
                setVector3(&pos, lbl_80799490, lbl_8079930C, lbl_80799494);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                break;
            case 3:
                setVector3(&pos, lbl_8079949C, lbl_8079931C, lbl_807994A0);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                break;
            case 7:
                setVector3(&pos, lbl_807994A4, lbl_80799220, lbl_807994A8);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                setVector3(&pos, lbl_807994AC, lbl_807994B0, lbl_807994B4);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 1;
                }
                break;
            }
        }
        return 0xFF;
    case 1:
        return fn_80131034(self, 0x1C, 1) != NULL;
    case 2:
        return self->action_0x328.armed_0x328.field_0x330;
    case 3:
        if (em_parts_damage_level_get(self, 2) >= 1) {
            return 1;
        }
        break;
    case 4:
        if (em_parts_damage_level_get(self, 3) >= 1) {
            return 1;
        }
        break;
    case 5:
        if (self->field_0x010 == 2) {
            sel = fn_801D67D8(self, 0x11, 3, &self->pos);
        } else {
            sel = fn_801D67D8(self, 0xC, 3, &self->pos);
        }
        switch (sel) {
        case 0:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 1:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 2:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        }
        if (fn_801D6758(self) == 1) {
            return 0;
        }
        if (self->field_0x010 == 2) {
            return (u8)(fn_801D67D8(self, 9, 3, &self->vec_0x36C) + 1);
        }
        return (u8)(fn_801D67D8(self, 6, 3, &self->vec_0x36C) + 1);
    case 6:
        t = fn_801D6758(self);
        return (u32)((1 - t) | (t - 1)) >> 31;
    }
    return 0;
}

void fn_801D6C50(struct _ENEMY_WORK* self, u8* out1, u8* out2) {
    *out1 = 12;
    *out2 = 0;
    self->pos.y = self->pos.y + lbl_80799224;
}

s32 fn_801D6C74(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && em_alt_mode_ck(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

s32 fn_801D6CCC(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && fn_8012EC3C(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

void fn_801D6D24(struct _ENEMY_WORK* self) {
    if (self->action_0x328.armed_0x328.field_0x330 == 0xFF) {
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 4) {
            if (self->area_no == 5 && self->field_0x9F6 == 7) {
                self->action_0x328.armed_0x328.field_0x330 = 1;
            } else {
                self->action_0x328.armed_0x328.field_0x330 = 0;
            }
        } else {
            self->action_0x328.armed_0x328.field_0x330 = 0xFF;
        }
    }
}

s32 fn_801D6DA4(struct _ENEMY_WORK* self, u16 arg1) {
    u8 kind = (u8)stage_map_kind_get(self->field_0x1E0);
    s32 found;
    u8 sel;
    u8 sub;

    if (kind != 4) {
        return 0;
    }
    found = 0;
    sel = 0xFF;
    if (kind == 4) {
        sel = 7;
    }
    if (sel != 0xFF) {
        sub = fn_80129DB8(self);
        if (sub == 1) {
            found = 1;
        } else if (sub == 2) {
            return 1;
        }
    }
    if (found == 0 && self->value_0x452 >= 0x384) {
        u8 arg = 0xFF;
        if (kind == 4) {
            arg = 5;
        }
        if (fn_8012A014(self, 0x1C, arg, arg1, lbl_805B5310, lbl_805B531C) == 1) {
            return 1;
        }
    }
    if (fn_80129A70(self, arg1) == 1) {
        return 1;
    }
    return fn_8012A204(self) - 1 == 0;
}

void fn_801D6EDC(EmSpawnRec801CCBC4* rec, u8 a1, u16 a2, u16 a3) {
    nw4r::math::VEC3 pos;

    setVec3(&pos, lbl_80799220, lbl_807992BC, lbl_80799380);
    rec->id = 29;
    copyVec3(&rec->pos, &pos);
    rec->field_0x10 = a1;
    rec->field_0x12 = a2;
    rec->field_0x14 = a3;
}

void* fn_801D6F5C(void* self, s16 arg1) {
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (arg1 > 0) {
            operator delete(self);
        }
    }
    return self;
}

void fn_801D6FB8(void) {
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::VEC3 v3;
    nw4r::math::VEC3 v4;
    nw4r::math::VEC3 v5;
    nw4r::math::VEC3 v6;
    nw4r::math::VEC3 v7;
    nw4r::math::VEC3 v8;
    nw4r::math::VEC3 v9;
    nw4r::math::VEC3 v10;
    nw4r::math::VEC3 v11;
    nw4r::math::VEC3 v12;
    nw4r::math::VEC3 v13;

    assignVec3(vec_pair_801CCBC4_0, setVec3(&v0, lbl_80799220, lbl_807992B0, lbl_80799220));
    assignVec3(&vec_pair_801CCBC4_0[1], setVec3(&v1, lbl_80799220, lbl_807994B8, lbl_80799220));
    assignVec3(vec_pair_801CCBC4_1, setVec3(&v2, lbl_80799220, lbl_807992B0, lbl_80799220));
    assignVec3(&vec_pair_801CCBC4_1[1], setVec3(&v3, lbl_80799220, lbl_807994B8, lbl_80799220));
    assignVec3(vec_pair_801CCBC4_2, setVec3(&v4, lbl_80799220, lbl_807992F4, lbl_80799220));
    assignVec3(&vec_pair_801CCBC4_2[1], setVec3(&v5, lbl_80799220, lbl_807994BC, lbl_80799220));
    assignVec3(vec_pair_801CCBC4_3, setVec3(&v6, lbl_80799220, lbl_807992B0, lbl_80799220));
    assignVec3(&vec_pair_801CCBC4_3[1], setVec3(&v7, lbl_80799220, lbl_807994B8, lbl_80799220));
    assignVec3(lbl_805B6950, setVec3(&v8, lbl_807994C0, lbl_80799220, lbl_807994C4));
    assignVec3(lbl_805B6950 + 0xC, setVec3(&v9, lbl_807994C8, lbl_807994CC, lbl_807994D0));
    assignVec3(lbl_805B6950 + 0x1C, setVec3(&v10, lbl_807994D4, lbl_80799220, lbl_807994D8));
    assignVec3(lbl_805B6950 + 0x28, setVec3(&v11, lbl_807994DC, lbl_80799220, lbl_807994E0));
    assignVec3(lbl_805B6950 + 0x38, setVec3(&v12, lbl_807994E4, lbl_80799220, lbl_807994E8));
    assignVec3(lbl_805B6950 + 0x44, setVec3(&v13, lbl_807994EC, lbl_80799220, lbl_807994F0));
}

#ifdef __cplusplus
}
#endif

/* The unit's `.bss`: the four two-vector records `fn_801D6FB8` seeds.  Names are GUESSes (each record is a
 * pair of model-space points). */
VEC3 vec_pair_801CCBC4_0[2];  /* +0x806A7B30 */
VEC3 vec_pair_801CCBC4_1[2];  /* +0x806A7B48 */
VEC3 vec_pair_801CCBC4_2[2];  /* +0x806A7B60 */
VEC3 vec_pair_801CCBC4_3[2];  /* +0x806A7B78 */
