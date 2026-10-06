/*
 * enemy/em007_act.cpp - enemy 007's action band: the `_ENEMY_WORK` action and step functions and their
 *   dispatchers.
 * RANGE. .text 0x801D71C4-0x801E0ADC (97 functions); extab 0x80010164-0x800103D4, extabindex
 *   0x8002C100-0x8002C4A8, .rodata 0x80570500-0x805706C0, .data 0x805B69D8-0x805B75E8 (`em007_prog_tbl` first),
 *   .sdata2 0x807994F8-0x80799788.  Left edge: `enemy/em005_act.cpp`'s static initializer ends there.  The right
 *   edge is unproven: the unwritten `fn_801E0574` reads `lbl_80799788` (three times), which sits in
 *   `lobby/fn_801E0ADC.cpp`'s `.sdata2`, and flipcheck names that unit as the fold candidate.  The source is three
 *   blocks, 0x801D80EC-0x801DB8E0, 0x801D71C4-0x801D80EC and 0x801DB8E0-0x801E0ADC; the functions are not in
 *   address order.
 * FLAGS. `cflags_main`; `#pragma peephole off` over the third block.
 * NAMES. `em007` is the runtime dump's `em007_prog_tbl`, which opens the TU's `.data`; the `_act` suffix is a
 *   GUESS.  The map has only `fn_` stems for the functions.  `fn_8013032C` returns `f32` and `fn_801303EC` takes
 *   it; the one argument-less call is a cast call.
 * RESIDUALS. 11 rows unwritten: 0x801DBE0C-0x801DF2F8, 0x801DFEEC-0x801E0058, 0x801E0240-0x801E04DC,
 *   0x801E0574-0x801E0AC0.
 *   41 partial rows; none has a recorded cause beyond the relocations below.
 *   Relocations (`relocdiff --by-owner`): `fn_801D83C0` and `fn_801D8E9C` call `CancelFade__FP11_ENEMY_WORK` where
 *   retail calls the C-linkage `CancelFade`; `fn_801D944C` and `fn_801D9584` call
 *   `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPl` where retail's last parameter is `u32*` (`...PUlPUl`); `fn_801D7890`
 *   calls `fn_8013032C` once where retail does not.
 *   flipcheck: `.rodata` claimed, not emitted; `.data` 0x1F4 against 0xC10, `.sdata2` 0x10 against 0x290 (a
 *   partial pool: candidate fold with `lobby/fn_801E0ADC.cpp`); `.text` (0x59BC of 0x9918), extab (0x218 of 0x270)
 *   and extabindex (0x324 of 0x3A8) short of the claim and differing; the two spellings above have no map row
 *   (retail's are `CancelFade` and `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`); the function order differs from
 *   the target's.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em005_act.h"
#include "enemy/em007_act.h"
#include "Pl/pl_hit_sphere.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "ef/eft009.h" /* eft009_set_pos, eft009_spawn_at_joint (rule 2: the owner's header) */
#include "sound/mhchar.h"
#include "stage/stg_w.h"

/* ===================================================================================================
 * 0x801D80EC-0x801DB8E0
 * =================================================================================================== */

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet. */

extern f32 lbl_80799520;
extern f32 lbl_80799524;
extern f32 lbl_80799528;
extern f32 lbl_8079952C;
extern f32 lbl_80799530;
extern f32 lbl_80799534;
extern f32 lbl_80799538;
extern f32 lbl_8079953C;
extern f32 lbl_80799540;
extern f32 lbl_80799544;
extern f32 lbl_80799548;
extern f32 lbl_8079954C;
extern f32 lbl_80799550;
extern f32 lbl_80799554;
extern f32 lbl_80799558;
extern f32 lbl_8079955C;
extern f32 lbl_80799560;
extern f32 lbl_80799564;
extern f32 lbl_80799568;
extern f32 lbl_8079956C;
extern f32 lbl_80799570;
extern f32 lbl_80799574;
extern f32 lbl_80799578;
extern f32 lbl_8079957C;
extern f32 lbl_80799580;
extern f32 lbl_80799584;
extern f32 lbl_80799588;
extern f32 lbl_8079958C;
extern f32 lbl_80799590;
extern f32 lbl_80799594;
extern f32 lbl_80799598;
extern f32 lbl_807995A8;
extern f32 lbl_807995AC;
extern f32 lbl_807995B0;
extern f32 lbl_807995B4;
extern f32 lbl_807995B8;
extern f32 lbl_807995BC;
extern f32 lbl_807995C0;
extern f32 lbl_807995C4;
extern f32 lbl_807995C8;
extern f32 lbl_807995CC;
extern f32 lbl_807995D0;
extern f32 lbl_807995D4;
extern f32 lbl_807995D8;
extern f32 lbl_807995DC;
extern f32 lbl_807995E0;
extern f32 lbl_807995E4;
extern f32 lbl_807995E8;
extern f32 lbl_807995EC;
extern f32 lbl_807995F0;
extern f32 lbl_807995F4;
extern f32 lbl_807995F8;
extern f32 lbl_807995FC;
extern f32 lbl_80799600;
extern f32 lbl_80799604;
extern f32 lbl_80799608;
extern f32 lbl_8079960C;
extern f32 lbl_80799610;
extern f32 lbl_80799614;
extern f32 lbl_80799618;
extern f32 lbl_8079961C;
extern f32 lbl_80799620;
extern f32 lbl_80799624;
extern f32 lbl_80799628;
extern f32 lbl_8079962C;
extern f32 lbl_80799630;
extern f32 lbl_80799634;
extern f32 lbl_80799638;
extern f32 lbl_80799648;

/* The unit's `.data` tables the code loads, declared: the source does not emit them yet. */
extern u8 lbl_80570500[];
extern u8 lbl_80570540[];
extern u8 lbl_80570580[];
extern u8 lbl_805705C0[];
extern u8 lbl_80570600[];
extern u8 lbl_80570640[];
extern u8 lbl_80570680[];
extern u8 lbl_805B6A90[];
extern u8 lbl_805B6AEC[];
extern u8 lbl_805B71A8[];
extern u8 lbl_805B6B88[];
extern u8 lbl_805B6BB0[];
extern u8 lbl_805B6BD8[];
extern u8 lbl_805B6C10[];
extern u8 lbl_805B6C50[];
extern u8 lbl_805B6C80[];
extern u8 lbl_805B6CD8[];
extern u8 lbl_805B6D48[];
extern u8 lbl_805B6D88[];
extern u8 lbl_805B6DB0[];
extern u8 lbl_805B6DD8[];
extern u8 lbl_805B6E00[];
extern u8 lbl_805B6E28[];
extern u8 lbl_805B6E50[];
extern u8 lbl_805B6E78[];
extern u8 lbl_805B6EB8[];
extern u8 lbl_805B6EE8[];
extern u8 lbl_805B6F10[];
extern u8 lbl_805B6F38[];
extern u8 lbl_805B6F60[];
extern u8 lbl_805B6F88[];
extern u8 lbl_805B6FB0[];
extern u8 lbl_805B6FD8[];
extern u8 lbl_805B7000[];
extern u8 lbl_805B7040[];
extern u8 lbl_805B70B0[];
extern u8 lbl_805B70F8[];
extern u8 lbl_805B7128[];
extern u8 lbl_805B7150[];
extern u8 lbl_805B7190[];

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotVecY(nw4r::math::VEC3* v, u32 angle);   /* rotVecY__FPQ34nw4r4math4VEC3Ul */
void calcVecAngXY(nw4r::math::VEC3* v, u32* out1, s32* out2);
                                                /* calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl */
s32 findInterSection(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, u8 d, u32 e,
                     u8 f, u16 g, u8* h);
            /* findInterSection__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3PQ34nw4r4math4VEC3UcUlUcUsPUc */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void mulVecMat(nw4r::math::VEC3* v, nw4r::math::MTX34* m);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                /* em_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self); /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
                                                /* get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34 */
void* get_move_work_adrs(u8 index);             /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                /* get_move_work_max__FUc */
void CancelFade(struct _ENEMY_WORK* self);      /* CancelFade */

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures: `enemy/em_common.cpp`'s first. */
void em_se_tbl_play(void* tbl, u32 a, u32 b);
void em_se_tbl_play_alt(void* tbl, u32 a, u32 b);
void em_part_rec_reset(struct _ENEMY_WORK* self, u32 a);
void em_part_rec_alt_set(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 em_hit_mask_get(struct _ENEMY_WORK* self, f32 a);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_target_pos_set(struct _ENEMY_WORK* self, void* p);
void em_hit_window_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);

void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_busy_set(struct _ENEMY_WORK* self);
u32 em_busy_ck(struct _ENEMY_WORK* self);
u32 fn_8012D23C(struct _ENEMY_WORK* self, u32 a, u8 b);
u8 fn_8012D3E0(struct _ENEMY_WORK* self, u32 a);

u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
f32 get_em_base_scale(struct _ENEMY_WORK* self);
f32 fn_8012F8F4(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
f32 fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
void fn_80130CE8(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
void em_busy_timer_reset(struct _ENEMY_WORK* self);
void fn_80131EC0(struct _ENEMY_WORK* self);
void em_turn_to_target(struct _ENEMY_WORK* self, u32 a);
void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);
void fn_80133F4C(struct _ENEMY_WORK* self, u32 a, f32 b, f32 c);
void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);
u32 em_approach_step(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_turn_seq_start(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b, u32 c);
u32 em_turn_seq_step(struct _ENEMY_WORK* self, void* tbl);
void em_move_vec_clr(struct _ENEMY_WORK* self);
void em_move_vec2_clr(struct _ENEMY_WORK* self);
void em_move_offset_apply(struct _ENEMY_WORK* self);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80136D14(struct _ENEMY_WORK* self);

/* `enemy/fn_8011D448.cpp`'s damage/knockback helper */
void fn_8011E6EC(struct _ENEMY_WORK* self, s32 a, u32 b, f32 c, f32 d);

/* This unit's own functions, declared before use. */
void fn_801D791C(struct _ENEMY_WORK* self);
void fn_801D8078(struct _ENEMY_WORK* self);
void fn_801DFD3C(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 scale);
u32 fn_801E0058(struct _ENEMY_WORK* self, u8 a);                     

/* Helpers of other units: `fn_8004CAD8.cpp`'s vector math, `g3d/g3d_calcworld.cpp`, `ef/eft007.cpp`,
 * `enemy/em001_prog.cpp`, `stage/stg_w.cpp`, `camera/camera_main.cpp`, `lobby/fn_8030121C.cpp` and
 * `enemy/em_model.cpp`. */
void subVec3(void* out, void* a, void* b);
f32 fn_80050EF4(void* a, void* b);
f32 calcVecDistXZ(void* a, void* b);
f32 fn_80050EAC(void* a, void* b);
void addVec3(void* out, void* a, void* b);
void fn_80051EE0(void* out, void* a, f32 b);
void addVec3To(void* a, void* b);
void eft007_part_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_spawn_rec_init(void* out);
void fn_802B43A8(void* pos, u8 a, u16 b);
void fn_802B954C(void);
void fn_802B9574(u32 a);
void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, void* pos, f32 c);
void fn_803B9BA0(struct _ENEMY_WORK* self, void* pos, u32 a);

#ifdef __cplusplus
}
#endif

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

extern "C" void fn_801D80EC(struct _ENEMY_WORK* self, u32 arg1, u32 arg2) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 0xC, 2);
        fn_80130CDC(self, (u32)-0x14);
        switch ((u8)arg1) {
        case 0: em_approach_start(self, lbl_80799524, 0); return;
        case 1: em_approach_start(self, lbl_80799528, 0); return;
        case 2: em_approach_start(self, lbl_807994FC, 0); return;
        case 3: em_approach_start(self, lbl_8079952C, 0); return;
        case 4: em_approach_start(self, lbl_80799530, 0); return;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            switch ((u8)arg2) {
            case 0:
                self->state = self->state + 1;
                em_mot_set(self, 0xB, 6, 0);
                return;
            case 1:
                em_action_finish(self);
                return;
            }
        } else {
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801D82A0(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570500, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D832C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x10, 0xA, 0);
        em_approach_start(self, lbl_807994FC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D83C0(struct _ENEMY_WORK* self) {
    f32 temp_f31;
    s32 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570580, 0, 1, 0);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799534 * get_em_base_scale(self) * temp_f31;
        self->field_0x324 = lbl_807994FC;
        temp_r3 = calcVecAng2(&self->pos, &self->vec_0x36C);
        rotVecY(&self->offset_0x30C.vec_0x310, temp_r3);
        rotVecY(&self->vec_0x31C, temp_r3);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799510) == 1) {
            CancelFade(self);
        }
        if (em_turn_seq_step(self, lbl_80570580) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D84E8(struct _ENEMY_WORK* self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799538) == 1) {
            temp_f1 = (lbl_80799540 * (lbl_80799544 * get_em_base_scale(self))) / lbl_80799548;
            em_turn_to_target(self, (u16)(s32)(lbl_8079953C + temp_f1));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D85C0(struct _ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)((((s32)~((arg1 - 1) | (1 - arg1)) >> 0x1F) + 0x25)), 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079954C, lbl_80799550) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8688(struct _ENEMY_WORK* self, u8 arg1) {
    f32 var_f1;
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        var_f1 = 0.0f;
        em_mot_set_ck(self, 0xA, 4, 0);
        switch (arg1) {
        case 0:
            var_f1 = lbl_80799554 * fn_80050EF4(&self->pos, &self->vec_0x36C);
            break;
        case 1:
            var_f1 = lbl_80799558 + (lbl_8079955C * get_em_chg_scale(self));
            break;
        }
        em_approach_start(self, var_f1, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xB, 6, 0);
            em_approach_start(self, lbl_807994FC, 0x10);
            return;
        }
        return;
    case 2:
        em_approach_step(self, 0, 0x80);
        if ((em_mot_end_ck(self) == 1) ||
            (temp_f31 = lbl_80799560 * get_em_chg_scale(self),
             calcVecDistXZ(&self->pos, &self->vec_0x36C) < temp_f31)) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D87FC(struct _ENEMY_WORK* self) {
    f32 var_f1;
    f32 var_f2;
    s32 temp_r0;
    s32 var_r31;
    u8 temp_r4;

    var_r31 = 0;
    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        if ((s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) < 0) {
            var_f1 = lbl_80799500;
            var_f2 = lbl_80799564;
        } else {
            var_f1 = lbl_80799564;
            var_f2 = lbl_80799500;
        }
        temp_r0 = self->bits_0x1EC & 3;
        switch (temp_r0) {
        case 0:
            var_r31 = (s32)(u16)(s32)(lbl_8079953C + ((lbl_80799540 * var_f1) / lbl_80799548));
            break;
        case 1:
            var_r31 = -(s32)(u16)(s32)(lbl_8079953C + ((lbl_80799540 * var_f2) / lbl_80799548));
            break;
        }
        em_turn_seq_start(self, lbl_80570640, 0, 1, (u16)var_r31);
        return;
    case 1:
        em_turn_seq_step(self, lbl_80570640);
        if (em_frame_check(self, 1, lbl_80799568, lbl_807994FC) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8950(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D80EC(self, 0, 0); break;
    case 1: fn_801D80EC(self, 0, 1); break;
    case 2: fn_801D80EC(self, 1, 1); break;
    case 3: fn_801D80EC(self, 2, 1); break;
    case 4: fn_801D80EC(self, 3, 1); break;
    case 5: fn_801D80EC(self, 4, 1); break;
    case 6: fn_801D82A0(self); break;
    case 9: fn_801D832C(self); break;
    case 10: fn_801D83C0(self); break;
    case 11: fn_801D84E8(self); break;
    case 14: fn_801D85C0(self, 0); break;
    case 15: fn_801D85C0(self, 1); break;
    case 20: fn_801D8688(self, 0); break;
    case 21: fn_801D87FC(self); break;
    case 22: fn_801D8688(self, 1); break;
    }
}

extern "C" void fn_801D89F4(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_busy_set(self);
        em_busy_timer_reset(self);
        return;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 1, lbl_8079956C, lbl_807994FC) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_801303EC(self, fn_8013032C(self));
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8AC4(struct _ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8B70(struct _ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_approach_start(self, lbl_807994FC, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8C34(struct _ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_80570500, 0, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8CE4(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x27, 0, 0);
        ((void (*)(struct _ENEMY_WORK*))fn_801303EC)(self);
        em_hit_window_set_default(self, 0, 0xA);
        em_busy_set(self);
        em_busy_timer_reset(self);
        fn_80136D14(self);
        return;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, lbl_80799570, lbl_807994FC) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8DC8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D89F4(self); return;
    case 1: fn_801D8AC4(self); return;
    case 3: fn_801D8B70(self); return;
    case 4: fn_801D8C34(self); return;
    case 5: fn_801D8CE4(self); return;
    }
}

extern "C" void fn_801D8E10(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570540, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570540) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8E9C(struct _ENEMY_WORK* self) {
    f32 temp_f31;
    u8 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_hit_window_set(self, 0, 2, 0x88);
        em_hit_window_set(self, 1, 0x34, 0x18);
        fn_80130CDC(self, (u32)-0x14);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799574 * get_em_base_scale(self) * temp_f31;
        self->field_0x324 = lbl_807994FC;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80799578, lbl_80799504) == 1) {
            CancelFade(self);
        }
        fn_80133F4C(self, 0x8000, lbl_80799578, lbl_80799504);
        temp_r3 = self->state_0x006;
        if ((temp_r3 == 0) && (self->field_0xA69 == 0)) {
            self->state_0x006 = temp_r3 + 1;
            em_hit_window_set(self, 1, 0x35, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9024(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 6, 0);
        var_r5 = 3;
        if (arg1 == 1) {
            var_r5 = 0x24;
        }
        em_hit_window_set(self, 0, var_r5, 0x88);
        em_hit_window_set(self, 1, 0x37, 0x18);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799578, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x38, 0x18);
        }
        if (em_frame_check(self, 0, lbl_8079957C, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x23, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9150(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    u32 var_r5_2;
    u32 var_r5_3;
    s32 temp_r3_2;
    u32 temp_r30;
    u8 temp_r0;
    u8 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 2, 0);
        var_r5 = 4;
        if (arg1 == 1) {
            var_r5 = 0x25;
        }
        em_hit_window_set(self, 0, var_r5, 0x88);
        em_hit_window_set(self, 1, 0x39, 0x18);
        fn_80130CDC(self, (u32)-0x32);
        self->timer_0x020 = 0x3E8;
        em_approach_start(self, lbl_807994FC, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799580, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x20, 0x90);
        }
        if (em_frame_check(self, 0, lbl_80799584, lbl_807994FC) == 1) {
            var_r5_2 = 5;
            if (arg1 == 1) {
                var_r5_2 = 0x26;
            }
            em_hit_window_set(self, 0, var_r5_2, 0x88);
            em_hit_window_set(self, 1, 0x3A, 0x18);
        }
        if (em_frame_check(self, 0, lbl_80799588, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x21, 0x90);
        }
        if (em_frame_check(self, 0, lbl_8079958C, lbl_807994FC) == 1) {
            var_r5_3 = 6;
            if (arg1 == 1) {
                var_r5_3 = 0x27;
            }
            em_hit_window_set(self, 0, var_r5_3, 0x88);
            em_hit_window_set(self, 1, 0x3B, 0x18);
        }
        if ((em_frame_check(self, 0, lbl_80799590, lbl_807994FC) == 1) ||
            (em_frame_check(self, 0, lbl_80799594, lbl_807994FC) == 1)) {
            temp_r3 = self->state_0x006;
            if (temp_r3 < 5) {
                temp_r0 = temp_r3 + 1;
                self->state_0x006 = temp_r0;
                em_mot_speed_set(self, lbl_807994F8 + (lbl_80799598 * (f32)temp_r0));
            }
        }
        temp_r3_2 = self->timer_0x020;
        if (temp_r3_2 > 0) {
            self->timer_0x020 = temp_r3_2 - 1;
        }
        temp_r30 = em_approach_step(self, 0, 0x100);
        if ((em_mot_end_ck(self) == 1) && ((temp_r30 == 1) || (self->timer_0x020 <= 0))) {
            self->state = self->state + 1;
            em_mot_set(self, 0x30, 2, 0);
            em_hit_window_set(self, 1, 0x22, 0x90);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801D944C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    s32 sp8;
    u16 var_r3;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        em_hit_window_set(self, 0, 7, 0x80);
        fn_80130CDC(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        var_r3 = sp8 - (u16)(self->field_0x1C0 + 0xD334);
        if ((u32)(var_r3 - 1) <= 0x7FFE) {
            if (var_r3 > 0xE39) {
                var_r3 = 0xE39;
            }
        } else {
            var_r3 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r3 >> 8);
        return;
    case 1:
        em_turn_in_window(self, lbl_807995A8, lbl_807995AC, self->state_0x006 << 8);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9584(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    s32 sp8;
    u16 temp_r0;
    u32 var_r0;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 6, 0);
        em_hit_window_set(self, 0, 8, 0x80);
        fn_80130CDC(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        temp_r0 = sp8 - (u16)(self->field_0x1C0 + 0x2CCD);
        if ((s32)temp_r0 > 0x8000) {
            var_r0 = 0x10000 - temp_r0;
            if (var_r0 > 0xE39) {
                var_r0 = 0xE39;
            }
        } else {
            var_r0 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r0 >> 8);
        return;
    case 1:
        em_turn_in_window(self, lbl_807995A8, lbl_807995AC, -(self->state_0x006 << 8));
        if ((arg1 == 1) && (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1)) {
            fn_803B9BA0(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9708(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp8);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(((arg1 - 1) == 0) + 0x2A), 6, 0);
        fn_80130CDC(self, (u32)-0x32);
        return;
    case 1:
        if ((em_frame_check(self, 3, lbl_807995B0, lbl_80799504) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_frame_check(self, 0, lbl_807995B4, lbl_807994FC) == 1) {
            setVector3(&sp8, lbl_807994FC, lbl_807995B8, lbl_807994FC);
            shell_set_func_ptr->method_0x54(self, 2, 0x16, &sp8, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, lbl_807995BC, lbl_807994FC) == 1) {
            eft007_part_spawn(self, 0x1C, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, lbl_807995C0, lbl_807995C4) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, lbl_807995C8, lbl_807995CC) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9918(struct _ENEMY_WORK* self, u8 arg1, u8 arg2) {
    nw4r::math::MTX34 sp60;
    nw4r::math::VEC3 sp4C;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 sp48;
    u16 sp5A;
    u8 sp58;
    u32 var_r4;
    u32 var_r28;
    u32 var_r29;
    u32 var_r27;
    u32 var_r31;
    s32 temp_r27;
    s32 temp_r29;
    f32 temp_f1;
    f32 temp_f1_2;
    u8 temp_r3;

    var_r31 = arg2;
    VEC3_ctor(&sp38);
    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    MTX34_ctor(&sp60);
    em_spawn_rec_init(&sp48);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            var_r4 = 0x34;
            var_r28 = 0x1B;
            var_r29 = 0x33;
            break;
        case 1:
            var_r4 = 0x29;
            var_r28 = 0x1A;
            var_r29 = 0x32;
            break;
        case 2:
            var_r4 = 0x34;
            var_r28 = 0x31;
            var_r29 = 0x33;
            break;
        case 3:
            var_r4 = 0x29;
            var_r28 = 0x30;
            var_r29 = 0x32;
            break;
        }
        em_mot_set(self, var_r4, 2, 0);
        em_hit_window_set(self, 0, var_r28, 0x88);
        em_hit_window_set(self, 1, var_r29, 0x90);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_8079950C, lbl_807994FC) == 1) {
            em_turn_to_target(self, 0xC0);
        }
        if ((em_frame_check(self, 3, lbl_807995D0, lbl_807995D4) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp38, lbl_807994FC, lbl_807994FC, lbl_807994FC);
            eft_em_spawn(self, 0x47, 0x16, &sp38, lbl_807994F8);
        }
        if (em_frame_check(self, 0, lbl_807995D8, lbl_807994FC) == 1) {
            sp48 = 0x17;
            copyVec3(&sp4C, setVec3(&sp8, lbl_807994FC, lbl_8079956C, lbl_80799568));
            copyVec3(&sp38, &sp4C);
            get_joint_wmat_em(self, sp48, &sp60);
            setVector3(&sp2C, sp60.m[0][3], sp60.m[1][3], sp60.m[2][3]);
            mulVecMat(&sp38, &sp60);
            addVec3To(&sp2C, &sp38);
            copyVec3(&sp14, &self->pos);
            temp_f1 = sp14.y;
            sp14.y = temp_f1 + lbl_8079950C;
            temp_r29 = findInterSection(&sp14, &sp2C, &sp20, 1, 0xFFFF, self->area_no,
                                        (u16)(em_hit_mask_get(self, temp_f1) | 0xC0), 0);
            temp_f1_2 = (lbl_807995DC + (self->vec_0x36C.y - sp2C.y)) / lbl_80799500;
            temp_r27 = -(s32)(lbl_80799538 * temp_f1_2);
            var_r27 = temp_r27 - (s32)(lbl_807995E4 *
                     ((calcVecDistXZ(&self->vec_0x36C, &sp2C) - lbl_807995E0) / lbl_8079950C));
            if ((s32)var_r27 < 0x8000) {
                if ((s32)var_r27 > 0x1000) {
                    var_r27 = 0x1000;
                }
            } else if ((s32)var_r27 < 0xF800) {
                var_r27 = 0xF800;
            }
            sp58 = 0;
            sp5A = (u16)var_r27;
            eft007_part_spawn(self, 0x24, var_r27, 0);
            switch (var_r31) {
            case 0:
                var_r31 = 0xD;
                break;
            case 1:
                var_r31 = 0x1D;
                break;
            case 2:
                var_r31 = 0x1E;
                break;
            }
            if (temp_r29 == 0) {
                shell_set_func_ptr->method_0x3C(self, &sp48, var_r31, shell_set_func_ptr);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9CE4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 9, 0x88);
        fn_80130CDC(self, (u32)-0x14);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - lbl_807995E8;
        if (var_f31 > lbl_807995EC) {
            var_f31 = lbl_807995EC;
        } else if (var_f31 < lbl_807994FC) {
            var_f31 = lbl_807994FC;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / lbl_80799500;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807995A8, lbl_807995AC) == 1) {
            em_turn_to_target(self, 0x3A0);
        }
        if (em_frame_check(self, 3, lbl_807995F0, lbl_807995F4) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_807995F8, lbl_807994FC) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, lbl_807994FC, lbl_807994FC, lbl_807994FC));
            shell_set_func_ptr->method_0x2C(self, 5, 0x21, &sp14, lbl_807994F8, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if ((em_frame_check(self, 1, lbl_807995FC, lbl_807994FC) == 1) && (em_busy_ck(self) == 1)) {
            if (fn_8012D3E0(self, 0) != 0xFF) {
                fn_8012B380(self, 1, 2);
                em_state_set(self, 8, 0);
                return;
            }
            em_state_set(self, 1, 6);
            return;
        }
        return;
    }
}

extern "C" void fn_801D9F2C(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        var_r5 = 0x11;
        if (arg1 == 1) {
            var_r5 = 0x2A;
        }
        em_hit_window_set(self, 0, var_r5, 0x80);
        em_move_vec_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799600 * get_em_base_scale(self) * temp_f31;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799604) == 1) {
            em_turn_to_target(self, 0x280);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA04C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_805705C0, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_805705C0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA0D8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    VEC3_ctor(&sp8);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        em_hit_window_set(self, 0, 1, 0x88);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799608, lbl_807994FC) == 1) {
            get_joint_wpos_em(self, 0x29, &sp14);
            shell_set_func_ptr->method_0x14(self, &sp14, 0x18, self->area_no, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, lbl_8079960C, lbl_807994FC) == 1) {
            em_camera_req(self, 0x29, 5);
            setVector3(&sp8, lbl_80799610, lbl_807994FC, lbl_807994FC);
            shell_set_func_ptr->method_0x2C(self, 9, 0x29, &sp8, lbl_80799614, self->field_0xAEA,
                                           shell_set_func_ptr);
            fn_802B43A8(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA254(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570600, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570600) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA2E0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D8E10(self); break;
    case 1: fn_801D8E9C(self); break;
    case 2: fn_801D9024(self, 0); break;
    case 3: fn_801D9150(self, 0); break;
    case 4: fn_801D944C(self); break;
    case 5: fn_801D9584(self, 0); break;
    case 6: fn_801D9708(self, 0); break;
    case 7: fn_801D9708(self, 1); break;
    case 8: fn_801D9918(self, 0, 0); break;
    case 9: fn_801D9918(self, 1, 0); break;
    case 10: fn_801D9CE4(self); break;
    case 11: fn_801D9F2C(self, 0); break;
    case 12: fn_801DA04C(self); break;
    case 13: fn_801DA0D8(self); break;
    case 14: fn_801DA254(self); break;
    case 15: fn_801D8E10(self); break;
    case 16: fn_801D9F2C(self, 1); break;
    case 17: fn_801D9024(self, 1); break;
    case 18: fn_801D9150(self, 1); break;
    case 19: fn_801D9918(self, 2, 0); break;
    case 20: fn_801D9918(self, 3, 0); break;
    case 21: fn_801D9918(self, 0, 1); break;
    case 22: fn_801D9918(self, 1, 1); break;
    case 23: fn_801D9918(self, 2, 1); break;
    case 24: fn_801D9918(self, 3, 1); break;
    case 25: fn_801D9918(self, 0, 2); break;
    case 26: fn_801D9918(self, 1, 2); break;
    case 27: fn_801D9918(self, 2, 2); break;
    case 28: fn_801D9918(self, 3, 2); break;
    case 29: fn_801DA04C(self); break;
    case 30: fn_801DA254(self); break;
    case 31: fn_801D9584(self, 1); break;
    }
}

extern "C" s32 fn_801DA410(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 temp_r31;
    u8* temp_r31_2;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    temp_r31 = (s32)get_move_work_adrs(2);
    if ((s32)arg1 < (s32)get_move_work_max(2)) {
        temp_r31_2 = (u8*)(temp_r31 + (arg1 * 0xB20));
        if (*temp_r31_2 != 0) {
            setVector3(&sp2C, lbl_807994FC, lbl_807994FC, lbl_807994F8);
            rotVecY(&sp2C, self->field_0x1C0);
            fn_80051EE0(&sp8, &sp2C, lbl_80799618 * get_em_chg_scale(self));
            addVec3(&sp14, &self->pos, &sp8);
            copyVec3(&sp20, &sp14);
            if (fn_80050EAC(temp_r31_2 + 0x3C, &sp20) == lbl_8079961C) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void fn_801DA514(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp3C;
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r3;

    VEC3_ctor(&sp3C);
    VEC3_ctor(&sp30);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_801DFD3C(self, &sp3C, &sp8, lbl_80799620);
        subVec3(&sp24, &sp3C, &self->pos);
        copyVec3(&sp30, &sp24);
        em_target_pos_set(self, &sp3C);
        em_turn_seq_start(self, lbl_80570680, 2, 1, sp8);
        em_move_vec_clr(self);
        temp_f31 = get_em_base_scale(self);
        fn_80051EE0(&spC, &sp30, lbl_80799624);
        fn_80051EE0(&sp18, &spC, temp_f31);
        copyVec3(&self->offset_0x30C.vec_0x310, &sp18);
        self->field_0x314 = lbl_807994FC;
        return;
    case 1:
        fn_80131EC0(self);
        if (em_frame_check(self, 3, lbl_80799628, lbl_8079962C) == 1) {
            em_move_offset_apply(self);
            em_turn_seq_step(self, lbl_80570680);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            self->timer_0x020 = 0x5A;
            em_mot_set(self, 0x35, 2, 0);
            return;
        }
        return;
    case 2:
        fn_80131EC0(self);
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 < 0) {
            self->timer_0x020 = 0;
        }
        if (em_busy_ck(self) == 1) {
            if (fn_801DA410(self, self->field_0x382) == 1) {
                if (fn_8012D23C(self, 1, self->field_0x382) == 1) {
                    self->field_0x1E7 = 5;
                    if (fn_8012EC3C(self) == 1) {
                        em_state_set(self, 9, 3);
                        return;
                    }
                    em_state_set(self, 9, 1);
                    return;
                }
                if ((self->timer_0x020 <= 0) || (fn_8012D23C(self, 0, self->field_0x382) == 0)) {
                    em_state_set(self, 1, 7);
                    return;
                }
                return;
            }
            em_state_set(self, 1, 7);
            return;
        }
        return;
    }
}

extern "C" void fn_801DA514(struct _ENEMY_WORK* self);

extern "C" void fn_801DA7A0(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801DA514(self);
    }
}

extern "C" void fn_801DA7B4(struct _ENEMY_WORK* self) {
    u8 temp_r0;
    u8 temp_r3;

    fn_80131EC0(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x35, 4, 0);
        return;
    case 1:
        if (em_busy_ck(self) == 1) {
            if ((fn_8012D23C(self, 1, self->field_0x382) == 1) &&
                (fn_801DA410(self, self->field_0x382) == 1)) {
                temp_r0 = self->field_0x1E7 - 1;
                self->field_0x1E7 = temp_r0;
                if (temp_r0 == 0) {
                    em_state_set(self, 9, 2);
                    return;
                }
                if (fn_8012EC3C(self) == 1) {
                    em_state_set(self, 9, 3);
                    return;
                }
                em_state_set(self, 9, 1);
                return;
            }
            em_state_set(self, 1, 7);
        } else {
            return;
        }
        break;
    }
}

extern "C" void fn_801DA8DC(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    f32 temp_f2;
    u8 temp_r0;
    u8 temp_r3;

    fn_80131EC0(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        var_r5 = 0x14;
        if (arg1 == 1) {
            var_r5 = 0x2D;
        }
        em_hit_window_set_default(self, 0, var_r5);
        return;
    case 1:
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0:
            if (em_frame_check(self, 0, lbl_80799630, lbl_807994FC) == 1) {
                temp_f2 = (f32)self->field_0x7A4;
                fn_8011E6EC(self, (s32)(lbl_80799634 * temp_f2), 1, lbl_807994F8, temp_f2);
                fn_80130CE8(self, 0x32, 1, lbl_807994F8);
            }
            if (em_frame_check(self, 0, lbl_80799638, lbl_807994FC) == 1) {
                em_camera_req(self, 0x16, 7);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state_0x006 = self->state_0x006 + 1;
                em_mot_set(self, 0x35, 4, 0);
            case 1:
                if (em_busy_ck(self) == 1) {
                    if ((fn_8012D23C(self, 1, self->field_0x382) == 1) &&
                        (fn_801DA410(self, self->field_0x382) == 1)) {
                        em_state_set(self, 9, 0);
                        return;
                    }
                    em_state_set(self, 1, 7);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

extern "C" void fn_801DAAC8(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        em_hit_window_set_default(self, 0, 0x19);
        fn_80131EC0(self);
        fn_802B954C();
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80799648, lbl_807994FC) == 1) {
            fn_802B9574(0);
        }
        if (em_frame_check(self, 3, lbl_8079951C, lbl_807994FC) == 1) {
            fn_80131EC0(self);
        }
        if (em_frame_check(self, 0, lbl_8079951C, lbl_807994FC) == 1) {
            fn_803B9BA0(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DABD4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801DA7B4(self); return;
    case 1: fn_801DA8DC(self, 0); return;
    case 2: fn_801DAAC8(self); return;
    case 3: fn_801DA8DC(self, 1); return;
    }
}

extern "C" void fn_801DAC18(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: em_se_tbl_play(lbl_805B6B88, 0, 0x17); return;
    case 0x18: em_se_tbl_play(lbl_805B6BD8, 0, 0x18); return;
    case 0x19: em_se_tbl_play(lbl_805B6BB0, 0, 0x19); return;
    case 0x1A: em_se_tbl_play(lbl_805B6BB0, 0, 0x1A); return;
    case 0x1B: em_se_tbl_play(lbl_805B6C10, 0, 0x1B); return;
    case 0x1C: em_se_tbl_play(lbl_805B6C50, 0, 0x1C); return;
    case 0x1D: em_se_tbl_play(lbl_805B6D88, 0, 0x1D); return;
    case 0x58: em_se_tbl_play(lbl_805B6C80, 0, 0x58); return;
    case 0x5C: em_se_tbl_play(lbl_805B6CD8, 0, 0x5C); return;
    case 0x5D: em_se_tbl_play(lbl_805B6D48, 0, 0x5D); return;
    case 0x7A: em_se_tbl_play(lbl_805B6DB0, 0, 0x7A); return;
    case 0x7B: em_se_tbl_play(lbl_805B6DD8, 0, 0x7B); return;
    case 0x9F: em_se_tbl_play(lbl_805B6E00, 0, 0x9F); return;
    case 0xA0: em_se_tbl_play(lbl_805B6E00, 0, 0xA0); return;
    case 0x7C: em_se_tbl_play(lbl_805B6E28, 0, 0x7C); return;
    case 0x8D: em_se_tbl_play(lbl_805B6E50, 0, 0x8D); return;
    case 0x7D: em_se_tbl_play(lbl_805B6EB8, 0, 0x7D); return;
    case 0x8E: em_se_tbl_play(lbl_805B6EE8, 0, 0x8E); return;
    case 0x78: em_se_tbl_play(lbl_805B6E78, 0, 0x78); return;
    case 0xA8: em_se_tbl_play(lbl_805B6F10, 0, 0xA8); return;
    case 0xB6: em_se_tbl_play(lbl_805B6F38, 0, 0xB6); return;
    case 0xB7: em_se_tbl_play(lbl_805B6F60, 0, 0xB7); return;
    case 0xB8: em_se_tbl_play(lbl_805B6F88, 0, 0xB8); return;
    case 0xB9: em_se_tbl_play(lbl_805B6FB0, 0, 0xB9); return;
    case 0xBA: em_se_tbl_play(lbl_805B6FD8, 0, 0xBA); return;
    case 0xBB: em_se_tbl_play(lbl_805B7000, 0, 0xBB); return;
    case 0xBC: em_se_tbl_play(lbl_805B7000, 0, 0xBC); return;
    case 0xBF: em_se_tbl_play(lbl_805B7040, 0, 0xBF); return;
    case 0xC1: em_se_tbl_play(lbl_805B6E50, 0, 0xC1); return;
    case 0xC9: em_se_tbl_play(lbl_805B70B0, 0, 0xC9); return;
    case 0xF0: em_se_tbl_play(lbl_805B6BD8, 0, 0xF0); return;
    default: em_action_finish(self); return;
    }
}

extern "C" void fn_801DAFC8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em_se_tbl_play_alt(lbl_805B70F8, 0, 0); return;
    case 15: em_se_tbl_play_alt(lbl_805B7128, 0, 0xF); return;
    case 26: em_se_tbl_play_alt(lbl_805B7190, 0, 0x1A); return;
    case 28: em_se_tbl_play_alt(lbl_805B7150, 0, 0x1C); return;
    default: em_se_tbl_play_alt(lbl_805B70F8, 0, 0); return;
    }
}

extern "C" void fn_801DB054(struct _ENEMY_WORK* self) {
    fn_801D8CE4(self);
}

extern "C" void fn_801DB058(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801DB054(self);
    }
}

extern "C" void fn_801DB06C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570500, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            em_state_set(self, 0xD, 1);
        }
        return;
    }
}

extern "C" void fn_801DB100(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 0xC, 2);
        fn_80130CDC(self, (u32)-0x14);
        em_approach_start(self, lbl_80799528, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 0xD, 2);
        }
        return;
    }
}

extern "C" void fn_801DB1C8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_hit_window_set(self, 0, 0x1D, 8);
        em_hit_window_set(self, 1, 0x36, 0x110);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - lbl_807995E8;
        if (var_f31 > lbl_807995EC) {
            var_f31 = lbl_807995EC;
        } else if (var_f31 < lbl_807994FC) {
            var_f31 = lbl_807994FC;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / lbl_80799628;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807995A8, lbl_807995AC) == 1) {
            em_turn_to_target(self, 0x300);
        }
        if (em_frame_check(self, 3, lbl_807995F0, lbl_807995F4) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_807995F8, lbl_807994FC) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, lbl_807994FC, lbl_807994FC, lbl_807994FC));
            shell_set_func_ptr->method_0x2C(self, 5, 0x21, &sp14, lbl_807994F8, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if ((em_frame_check(self, 1, lbl_807995FC, lbl_807994FC) == 1) && (em_busy_ck(self) == 1)) {
            if (fn_801E0058(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
                return;
            }
            em_state_set(self, 1, 6);
            return;
        }
        return;
    }
}

extern "C" void fn_801DB3F8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp30);
    VEC3_ctor(&sp24);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_801DFD3C(self, &sp30, &sp8, lbl_80799618);
        subVec3(&sp18, &sp30, &self->pos);
        copyVec3(&sp24, &sp18);
        em_target_pos_set(self, &sp30);
        em_turn_seq_start(self, lbl_80570680, 2, 1, sp8);
        em_move_vec_clr(self);
        fn_80051EE0(&spC, &sp24, lbl_807994F8 / fn_8012F8F4(self));
        copyVec3(&self->offset_0x30C.vec_0x310, &spC);
        self->field_0x314 = lbl_807994FC;
        return;
    case 1:
        em_move_offset_apply(self);
        em_turn_seq_step(self, lbl_80570680);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x35, 2, 0);
            return;
        }
        return;
    case 2:
        if (em_busy_ck(self) == 1) {
            if (fn_801E0058(self, 1) == 1) {
                em_state_set(self, 0xD, 4);
                return;
            }
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void fn_801DB594(struct _ENEMY_WORK* self) {
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        fn_803B9BA0(self, &self->pos, 0x64);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            temp_r0 = self->state_0x006 + 1;
            self->state_0x006 = temp_r0;
            if (temp_r0 >= 3) {
                em_state_set(self, 0xD, 5);
            }
        }
        return;
    }
}

extern "C" void fn_801DB648(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        fn_80130CDC(self, 0x3E8);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DB6D0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801DB06C(self); return;
    case 1: fn_801DB100(self); return;
    case 2: fn_801DB1C8(self); return;
    case 3: fn_801DB3F8(self); return;
    case 4: fn_801DB594(self); return;
    case 5: fn_801DB648(self); return;
    }
}

extern "C" void fn_801DB724(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0: fn_801D791C(self); break;
    case 1: fn_801D8078(self); break;
    case 2: fn_801D8950(self); break;
    case 6: fn_801D8DC8(self); break;
    case 7: fn_801DA2E0(self); break;
    case 8: fn_801DA7A0(self); break;
    case 9: fn_801DABD4(self); break;
    case 10: fn_801DAC18(self); break;
    case 11: fn_801DAFC8(self); break;
    case 12: fn_801DB058(self); break;
    case 13: fn_801DB6D0(self); break;
    }
    if (self->field_0x1E2 == 4) {
        em_busy_timer_reset(self);
    }
}

extern "C" void fn_801DB7D8(struct _ENEMY_WORK* self) {
    if (em_alt_mode_ck(self) == 1) {
        if (self->action_0x328.field_0x344 == 0) {
            em_part_rec_alt_set(self, 0, 0);
            em_part_rec_alt_set(self, 1, 1);
            em_part_rec_alt_set(self, 2, 2);
            em_part_rec_alt_set(self, 3, 3);
            em_part_rec_alt_set(self, 4, 4);
            em_part_rec_alt_set(self, 5, 5);
            self->action_0x328.field_0x344 = 1;
        }
    } else if (self->action_0x328.field_0x344 == 1) {
        em_part_rec_reset(self, 0);
        em_part_rec_reset(self, 1);
        em_part_rec_reset(self, 2);
        em_part_rec_reset(self, 3);
        em_part_rec_reset(self, 4);
        em_part_rec_reset(self, 5);
        self->action_0x328.field_0x344 = 0;
    }
}

/* ===================================================================================================
 * 0x801D71C4-0x801D80EC
 * =================================================================================================== */
/* The unit's `.data` tables the code loads, declared: the source does not emit them yet. */
extern u8 lbl_805B75B8[];

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
void fn_80128A8C(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hit_window_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 fn_8012A204(struct _ENEMY_WORK* self);

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
void em_fall_height_get(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 fn_8013023C(struct _ENEMY_WORK* self);
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

/* ef module */
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, nw4r::math::VEC3* pos,
                 f32 scale);
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_801057FC(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, s32 d);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);
void fn_800FA378(void* out);
void fn_800FA3B8(void* out);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void draw_shape_arm(struct _ENEMY_WORK* self, u32 a, u32 b);

/* `enemy/em005_act.cpp`'s dispatch targets this block tail-calls. */
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

/* ----------------------------------------------------------------------------------------------------
 * the records this unit needs locally
 * -------------------------------------------------------------------------------------------------- */

/* The 12-byte helper `fn_801D71C4` allocates; its constructor `fn_801D752C` chains `em_res_user_data_ctor`
 * and stores `lbl_805B75B8` at +0x00.  size: 0x0C (`operator new(0xC)`) */
struct EmHelper801D428C {
    /* +0x00 */ void* vtbl;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
};

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

/* this range's own functions that a later body calls before its definition */
void* fn_801D752C(EmHelper801D428C* self);

void fn_801D71C4(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;
    EmHelper801D428C* helper;

    VEC3_ctor(&pos);
    setVector3(&self->action_0x328.vec_0x328, lbl_807994F8, lbl_807994F8, lbl_807994F8);
    setVector3(&self->action_0x328.vec_0x334, lbl_807994F8, lbl_807994F8, lbl_807994F8);
    self->action_0x328.field_0x340 = lbl_807994FC;
    self->action_0x328.field_0x344 = 0;
    self->action_0x328.field_0x345 = 0;
    if (em_res_user_data_ck(self) == 0) {
        helper = (EmHelper801D428C*)operator new(0xC);
        if (helper != NULL) {
            fn_801D752C(helper);
        }
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, lbl_807994FC, lbl_807994FC, lbl_80799500);
        fn_801057FC(self, 0x14, 0x18, &pos, lbl_807994F8, 0);
    }
    if (arg1 == 2) {
        switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
        case 1:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            case 5:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                break;
            }
            break;
        case 2:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 6), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            }
            break;
        case 3:
            switch (self->area_no) {
            case 1:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                break;
            case 3:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                break;
            }
            break;
        case 4:
            switch (self->area_no) {
            case 3:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                break;
            }
            break;
        case 5:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                break;
            }
            break;
        case 8:
            if (self->area_no == 1) {
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            }
            break;
        case 9:
            if (self->area_no == 0) {
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            }
            break;
        }
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 5);
    }
}

void* fn_801D752C(EmHelper801D428C* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805B75B8;
    return self;
}

void fn_801D7568(void) {
}

void fn_801D756C(struct _ENEMY_WORK* self, u8 arg1, u8 arg2) {
    switch (arg1) {
    case 1:
        switch (arg2) {
        case 0:
            fn_801376B4(self);
            break;
        case 5:
        case 9:
        case 12:
            fn_80130F74(self);
            break;
        }
        break;
    case 10:
        if (arg2 == 201) {
            em_part_hit_set(self, 0, 0);
        }
        break;
    }
}

void fn_801D75D0(struct _ENEMY_WORK* self) {
    u16 max;
    struct _ENEMY_WORK* other;
    u16 i;
    s32 found;

    max = get_move_work_max(3);
    other = (struct _ENEMY_WORK*)get_move_work_adrs(3);
    found = 0;
    if (em_die_ck(self) == 0) {
        for (i = 0; i < max; i++) {
            if (other->active != 0 && (other->field_0x1C8 & 1) != 0 && other != self &&
                self->area_no == other->area_no &&
                fn_8013023C(other) == fn_8013023C(self) && other->action == 0xB) {
                copyVec3(&self->aim, &other->pos);
                self->action_0x328.field_0x345 = 1;
                found = 1;
                break;
            }
            other++;
        }
    }
    if (found == 0) {
        self->action_0x328.field_0x345 = 0;
    }
    if (em_alt_mode_ck(self) == 1) {
        if ((self->flags_0x836 & 0x8000) != 0) {
            self->flags_0x836 &= 0x7FFF;
        }
    } else {
        if ((self->flags_0x836 & 0x8000) == 0) {
            self->flags_0x836 |= 0x8000;
        }
    }
}

void fn_801D771C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7798(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7814(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7890(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

void fn_801D791C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D771C(self);
        break;
    case 1:
        fn_801D7798(self);
        break;
    case 2:
        fn_801D7814(self);
        break;
    case 3:
        fn_801D771C(self);
        break;
    case 4:
        fn_801D771C(self);
        break;
    case 6:
        fn_801D771C(self);
        break;
    case 7:
        fn_801D7890(self);
        break;
    }
}

void fn_801D797C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D79F8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7A74(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1) {
            em_hit_window_set(self, 0, 0xB, 5);
            draw_shape_arm(self, 0x17, 0xA);
            fn_802B43A8(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1) {
            setVector3(&pos, lbl_807994FC, lbl_80799508, lbl_8079950C);
            eft_em_spawn(self, 0, 0x16, &pos, lbl_807994F8);
        }
        if (em_frame_check(self, 3, lbl_80799510, lbl_80799514) == 1 &&
            (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, lbl_807994FC, lbl_80799508, lbl_8079950C);
            eft_em_spawn(self, 1, 0x16, &pos, lbl_807994F8);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7BF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_80799518, 1, 5);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7D14(struct _ENEMY_WORK* self, u32 arg1) {
    if ((u8)arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_8079951C, lbl_807994FC) == 1) {
            em_hit_window_set_default(self, 0, 0x3C);
        }
        if (em_mot_end_ck(self) == 1) {
            if (arg1 != 1) {
                if (arg1 != 2) {
                    em_state_set(self, 1, 5);
                } else {
                    em_state_set(self, 1, 0xC);
                }
            } else {
                self->state_0x006 = self->state_0x006 + 1;
                if (self->state_0x006 >= 4) {
                    em_state_set(self, 1, 9);
                }
            }
        }
        break;
    }
}

void fn_801D7E44(struct _ENEMY_WORK* self, u32 arg1) {
    if ((u8)arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set_blend(self, 0xC9, 6, 0, 1);
        em_hit_window_set_default(self, 0, 0x3D);
        if (arg1 == 1) {
            fn_80130CDC(self, 0x3E8);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7F04(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x36, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7F80(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D8078(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D797C(self);
        break;
    case 1:
        fn_801D79F8(self);
        break;
    case 2:
        fn_801D7A74(self);
        break;
    case 3:
        fn_801D7BF8(self);
        break;
    case 4:
        fn_801D7D14(self, 0);
        break;
    case 5:
        fn_801D7E44(self, 0);
        break;
    case 6:
        fn_801D7F04(self);
        break;
    case 7:
        fn_801D7F80(self);
        break;
    case 8:
        fn_801D7D14(self, 1);
        break;
    case 9:
        fn_801D7E44(self, 1);
        break;
    case 10:
        fn_801D7FFC(self);
        break;
    case 11:
        fn_801D7D14(self, 2);
        break;
    case 12:
        fn_801D7E44(self, 2);
        break;
    }
}

#ifdef __cplusplus
}
#endif

/* ===================================================================================================
 * 0x801DB8E0-0x801E0ADC
 * =================================================================================================== */
#pragma peephole off

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet. */

extern f64 lbl_807995A0;
extern f32 lbl_80799614;
extern f32 lbl_8079964C;
extern f32 lbl_80799650;
extern f32 lbl_80799654;
extern f32 lbl_80799668;
extern f32 lbl_8079975C;
extern f32 lbl_80799760;
extern f32 lbl_80799764;
extern f32 lbl_80799768;
extern f32 lbl_8079976C;
extern f32 lbl_80799770;
extern f32 lbl_80799774;
extern f32 lbl_80799778;
extern f32 lbl_8079977C;
extern f32 lbl_80799780;

/* The unit's `.data` tables the code loads, declared: the source does not emit them yet. */
extern u8 lbl_805B6A44[];
extern u8 lbl_805B6A50[];

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

void rotVecY(nw4r::math::VEC3* v, u32 angle);                    /* rotVecY__FPQ34nw4r4math4VEC3Ul */
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotMatrixX(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixX__FUlPQ34nw4r4math5MTX34 */
void rotMatrixZ(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixZ__FUlPQ34nw4r4math5MTX34 */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                          /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                       /* em_get_mot_no__FP11_ENEMY_WORK */
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                                  /* em_after_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);                  /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                        /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */

/* The runtime's allocator pair (`__nw__FUl`/`__dl__FPv`), declared as the C++ functions (rule 9). */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures (`enemy/em_common.cpp`'s first). */
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);
u32 fn_8012EC3C(struct _ENEMY_WORK* self);

void fn_80126278(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
u32 fn_80129D3C(struct _ENEMY_WORK* self);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 fn_8012A204(struct _ENEMY_WORK* self);

u32 fn_8012E5A8(struct _ENEMY_WORK* self);

void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
u32 fn_8013023C(struct _ENEMY_WORK* self);
u8* fn_801377D0(u8 index);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 em_flags836_ck(struct _ENEMY_WORK* self, u32 a);
void fn_8013A654(struct _ENEMY_WORK* self, u32 a);

/* `enemy/fn_80138074.c` */
void fn_8013918C(void* helper, s16 flag);

/* The part-manager record `fn_801E0538` dispatches on (only the step byte is named).  size: 0x6 */
struct EmEftPartsMan {
    /* +0x00 */ u8 unused_0x00[0x05];
    /* +0x05 */ u8 state; /* the step index `fn_801E0538` switches on */
};

/* The part-manager step functions: `fn_801E0574` (unwritten) and three of `lobby/fn_801E0ADC.cpp`. */
void fn_801E0574(struct EmEftPartsMan* self);
void fn_801E0D38(struct EmEftPartsMan* self);
void fn_801E1A2C(struct EmEftPartsMan* self);
void fn_801E1A3C(struct EmEftPartsMan* self);

/* The effect clusters. */
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);

/* Runtime helpers. */
f32 fn_80050EF4(void* a, void* b);
void fn_8005D0CC(void* out, void* src);
void fn_8005D1AC(void* out, u32 a);
void fn_8006FDCC(void* a);
void fn_800810DC(void* self, u32 a);

/* `stage_map_kind_get` and `get_now_areano` come from `stage/stg_w.h`, `get_now_mapno` from
 * `unsplit/unknown.h`. */

/* ----------------------------------------------------------------------------------------------------
 * the definitions (C linkage: they keep the map's own `fn_XXXXXXXX` names)
 * -------------------------------------------------------------------------------------------------- */

/* 0x801DB8E0 (0x98) - arms the +0x1BC/-0x20C effect spawn once the work record answers the map query
 * and `system_w`'s counter has run a multiple of 24 frames. */
void fn_801DB8E0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) == 1) {
        if (system_w.field_0x0c % 0x18 == 0) {
            setVector3(&pos, lbl_807994FC, lbl_8079964C, lbl_80799650);
            eft_spawn_type10(self, 0x13, 0x16, &pos, lbl_807994F8);
        }
    }
}

/* 0x801DB978 (0x494) - the effect-spawn dispatcher: per effect id, `eft009_set_pos` when the joint is 0xFF,
 * else the joint goes to `eft009_spawn_at_joint`/`eft_spawn_pos_in_area`/`eft_spawn_type11`. */
void fn_801DB978(struct _ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, u32 id, f32 scale) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (mode == 0) {
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
                kind = 0xd;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xf, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xf, id, scale);
                }
                break;
            case 2:
                kind = 0xc;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xe, id, scale);
                }
                break;
            case 4:
                kind = 0x13;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xe, id, scale);
                }
                break;
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 0xa:
            case 0xb:
            case 0x24:
            case 0x25:
            case 0x29:
            case 0x2a:
                return;
            default:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)kind - 0xc > 0xd && kind != 0x26) {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x20C;
                    pos.z = self->pos.z;
                }
            }
        }
        if (joint == 0xff) {
            eft009_set_pos(kind, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, kind, id, scale);
        }
    } else if (mode == 1) {
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
            case 6: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x11, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x11, id, scale);
                }
                break;
            }
            case 1: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x10, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x10, id, scale);
                }
                break;
            }
            case 5: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x14, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x14, id, scale);
                }
                break;
            }
            default: /* 3, 4, ...: nothing */
                break;
            }
        } else {
            if (joint == 0xff) {
                copyVec3(&pos, &self->pos);
            } else {
                get_joint_wpos_em(self, joint, &pos);
            }
            pos.y = self->field_0x20C;
            scale = scale * get_em_chg_scale(self);
            eft_spawn_pos_in_area(&pos, self->area_no, kind, id, scale);
        }
    } else if (mode == 2) {
        if (joint == 0xff) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale = scale * get_em_chg_scale(self);
        eft_spawn_type11(self, &pos, kind, scale);
    }
}

/* 0x801DF2F8 (0x248) - fades the four K-colour alphas of the `MHchar` with `field_0x340` (stepped toward
 * `lbl_80799760`/0), scaled and clamped for part kind 4 in area 4/6. */
void fn_801DF2F8(struct _ENEMY_WORK* self) {
    _GXColor color;
    u32 strength;
    u32 scaled;
    if (em_alt_mode_ck(self) == 1) {
        self->action_0x328.field_0x340 += lbl_8079975C;
        if (self->action_0x328.field_0x340 > lbl_80799760) {
            self->action_0x328.field_0x340 = lbl_80799760;
        }
    } else {
        self->action_0x328.field_0x340 -= lbl_8079975C;
        if (self->action_0x328.field_0x340 < lbl_807994FC) {
            self->action_0x328.field_0x340 = lbl_807994FC;
        }
    }
    scaled = 0;
    strength = stage_map_kind_get(self->field_0x1E0);
    if (strength == 4 && (self->area_no == 4 || self->area_no == 6)) {
        scaled = 1;
    }
    if (scaled == 1) {
        f32 v = lbl_80799518 * self->action_0x328.field_0x340 + lbl_80799668;
        if (v > lbl_80799760) {
            v = lbl_80799760;
        }
        strength = (u32)(s32)v;
    } else {
        strength = (u32)(s32)self->action_0x328.field_0x340;
    }
    ((MHchar*)self->char_0x024)->getTevKColor(0, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(2, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    if (em_flags836_ck(self, 1) == 1) {
        u32 fade = 0;
        if (strength == 4 && (self->field_0x48E == 4 || self->field_0x48E == 6)) {
            fade = 1;
        }
        if (fade == 1) {
            if ((f32)color.a > lbl_80799764) {
                color.a--;
            }
        } else {
            if (color.a != 0) {
                color.a--;
            }
        }
    } else {
        color.a = strength;
    }
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
}

/* 0x801DF540 (0x28) - "is part `a` the aim target": only true for part 0 and when the action block's
 * +0x345 byte is 1. */
u32 fn_801DF540(struct _ENEMY_WORK* self, u8 a) {
    if (a == 0 && self->action_0x328.field_0x345 == 1) {
        return 1;
    }
    return 0;
}

/* 0x801DF568 (0x2A4) - arms the part's motion (`em_move_mode_set`) and picks the `fn_80126278` effect id
 * from the work record's map kind (`stage_map_kind_get`) and `area_no`. */
void fn_801DF568(struct _ENEMY_WORK* self, u8* outA, u8* outB) {
    u32 kind;
    em_move_mode_set(self, 4);
    *outA = 0xc;
    *outB = 0;
    kind = stage_map_kind_get(self->field_0x1E0);
    switch ((u8)kind) {
    case 1:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 5:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 6), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 5:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 8:
        switch (self->area_no) {
        case 1:
            fn_80126278(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    case 9:
        switch (self->area_no) {
        case 0:
            fn_80126278(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    }
}

/* 0x801DF80C (0x4) - empty body (the band's null override). */
void fn_801DF80C(void) {
}

/* 0x801DF810 (0xDC) - "may this part be damaged": the work record's +0x1E2 gate plus the map- and
 * part-level checks for parts 0/4/5. */
u32 fn_801DF810(struct _ENEMY_WORK* self, u8 part) {
    if (self->field_0x1E2 == 0) {
        switch (part) {
        case 0:
            if (em_alt_mode_ck(self) == 1) {
                return 1;
            }
            break;
        case 4:
            if (fn_8012EC3C(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 4) & 1) == 0) {
                return 1;
            }
            break;
        case 5:
            if (fn_8012EC3C(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 5) & 1) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x801DF8EC (0x31C) - the part-state step: gate on map kind 1..5, test the state (`fn_80129DB8`), request
 * a special part (`fn_8012A014`) and arm +0x1FC/+0x1FE/+0x1FF. */
u32 fn_801DF8EC(struct _ENEMY_WORK* self, u16 a) {
    u32 kind;
    u32 found;
    u8 id;
    kind = stage_map_kind_get(self->field_0x1E0);
    if ((u8)kind - 1 > 4) {
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    found = 0;
    switch ((u8)kind) {
    case 1:
        id = 9;
        break;
    case 2:
        id = 9;
        break;
    case 3:
        id = 0xa;
        break;
    case 4:
        id = 9;
        break;
    case 5:
        id = 4;
        break;
    default:
        id = 0xff;
        break;
    }
    if (id != 0xff) {
        u8 s = fn_80129DB8(self);
        if (s == 1) {
            found = 1;
        } else if (s == 2) {
            return 1;
        }
    }
    if (found == 0) {
        u8 r4;
        u8 r5;
        switch ((u8)kind) {
        case 1:
            r4 = 0x1b;
            r5 = 9;
            break;
        case 2:
            r4 = 0x1b;
            r5 = 6;
            break;
        case 3:
            r4 = 0x1b;
            r5 = 3;
            break;
        case 4:
            r4 = 0x1c;
            r5 = 5;
            break;
        case 5:
            r4 = 0x1b;
            r5 = 3;
            break;
        default:
            r4 = 0;
            r5 = 0xff;
            break;
        }
        if (r4 != 0 || r5 != 0xff) {
            if (fn_8012A014(self, r4, r5, a, lbl_805B6A44, lbl_805B6A50) == 1) {
                return 1;
            }
        }
    }
    if (fn_80129A70(self, a) == 1) {
        return 1;
    }
    if (!(self->value_0x452 < self->field_0x450 && self->field_0x43D == 1)) {
        if (((u8)kind == 4 && self->area_no == 1) ||
            ((u8)kind == 2 && self->area_no == 9)) {
            if (a % 100 < 0x32) {
                if (self->field_0x382 != 0xff) {
                    u8 state;
                    if (self->field_0x380 == 1) {
                        u8* rec = fn_801377D0(self->state_0x381);
                        state = rec[0x5a6] & 0x7f;
                    } else {
                        state = 0xff;
                    }
                    if (state != 0xff && state != fn_8013023C(self)) {
                        u8 mode;
                        if ((u8)kind == 2) {
                            mode = 8;
                        } else if ((u8)kind == 4) {
                            mode = 2;
                        } else {
                            mode = 0xff;
                        }
                        if (mode != 0xff) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 0xa;
                            self->field_0x1FF = mode;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return fn_8012A204(self) == 1;
}

/* 0x801DFC08 (0x134) - steps the four action-block floats toward their clamps (up when `em_alt_mode_ck`
 * answers 1, down otherwise). */
void fn_801DFC08(struct _ENEMY_WORK* self) {
    EmActionBlock* blk = &self->action_0x328;
    if (em_alt_mode_ck(self) == 1) {
        blk->vec_0x328.x += lbl_80799768;
        if (blk->vec_0x328.x > lbl_80799614) {
            blk->vec_0x328.x = lbl_80799614;
        }
        blk->vec_0x328.y += lbl_807994F8;
        if (blk->vec_0x328.y > lbl_8079976C) {
            blk->vec_0x328.y = lbl_8079976C;
        }
        blk->vec_0x334.x += lbl_80799770;
        if (blk->vec_0x334.x > lbl_80799774) {
            blk->vec_0x334.x = lbl_80799774;
        }
        blk->vec_0x334.y += lbl_80799778;
        if (blk->vec_0x334.y > lbl_8079977C) {
            blk->vec_0x334.y = lbl_8079977C;
        }
    } else {
        blk->vec_0x328.x -= lbl_80799768;
        if (blk->vec_0x328.x < lbl_807994F8) {
            blk->vec_0x328.x = lbl_807994F8;
        }
        blk->vec_0x328.y -= lbl_807994F8;
        if (blk->vec_0x328.y < lbl_807994F8) {
            blk->vec_0x328.y = lbl_807994F8;
        }
        blk->vec_0x334.x -= lbl_80799770;
        if (blk->vec_0x334.x < lbl_807994F8) {
            blk->vec_0x334.x = lbl_807994F8;
        }
        blk->vec_0x334.y -= lbl_80799778;
        if (blk->vec_0x334.y < lbl_807994F8) {
            blk->vec_0x334.y = lbl_807994F8;
        }
    }
}

/* 0x801DFD3C (0x170) - aims at `self->vec_0x36C`: turns the angle from `pos` into a rotation about y
 * and writes the resulting point into `out`. */
void fn_801DFD3C(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 scale) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 rot;
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 res;
    u16 ang;
    u16 delta;
    VEC3_ctor(&dir);
    VEC3_ctor(&rot);
    copyVec3(&dir, &self->vec_0x36C);
    ang = calcVecAng2(&self->pos, &dir);
    *angleOut = ang;
    delta = (u16)(ang - self->field_0x1C0);
    scale = scale * get_em_chg_scale(self);
    if ((u16)(delta + 0xbfff) > 0x7ffe) {
        f32 base = fn_80050EF4(&dir, &self->pos);
        setVector3(&rot, lbl_807994FC, lbl_807994FC, base - scale);
        rotVecY(&rot, *angleOut);
        addVec3(&tmp, &self->pos, &rot);
        copyVec3(out, &tmp);
    } else {
        u16 a2 = (delta < 0x8000) ? 0x4000 : 0xc000;
        *angleOut = (u16)(a2 + self->field_0x1C0);
        setVector3(&rot, lbl_807994FC, lbl_807994FC, -scale);
        rotVecY(&rot, *angleOut);
        addVec3(&res, &dir, &rot);
        copyVec3(out, &res);
    }
}

/* 0x801DFEAC (0x40) - clears the helper and sets part 7. */
void fn_801DFEAC(struct _ENEMY_WORK* self) {
    u8 tmp[0x0C];
    fn_8005D1AC(&tmp, 0);
    fn_8013A654(self, 7);
}

/* 0x801E0058 (0x164) - "is either part 0x1B or 0x1C within (scale * lbl_80799780)^2 of me". */
u32 fn_801E0058(struct _ENEMY_WORK* self, u8 a) {
    struct _ENEMY_WORK* p;
    f32 d;
    f32 lim;
    p = fn_80131034(self, 0x1b, 0);
    if (p != NULL) {
        if (fn_8012E5A8(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (lbl_80799780 * get_em_chg_scale(self)) *
                  (lbl_80799780 * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    p = fn_80131034(self, 0x1c, 0);
    if (p != NULL) {
        if (fn_8012E5A8(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (lbl_80799780 * get_em_chg_scale(self)) *
                  (lbl_80799780 * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801E01BC (0x28) - "action 0xD with sub-state <= 5". */
u32 fn_801E01BC(struct _ENEMY_WORK* self) {
    if (self->action == 0xd && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* 0x801E01E4 (0x5C) - the deleting destructor of the +0x4 helper. */
void* fn_801E01E4(void* self, s16 flag) {
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801E04DC (0x5C) - the deleting destructor of the res-object helper. */
void* fn_801E04DC(void* self, s16 flag) {
    if (self != NULL) {
        fn_800810DC(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801E0538 (0x3C) - the part-manager state dispatcher (index at +0x5). */
void fn_801E0538(struct EmEftPartsMan* self) {
    switch (self->state) {
    case 0:
        fn_801E0574(self);
        break;
    case 1:
        fn_801E0D38(self);
        break;
    case 2:
        fn_801E1A2C(self);
        break;
    case 3:
        fn_801E1A3C(self);
        break;
    }
}

/* 0x801E0AC0 (0x1C) - `base + offset`, or NULL when the offset is 0. */
void* fn_801E0AC0(void** base, u32 offset) {
    u8* p = (u8*)*base;
    if (offset != 0) {
        return p + offset;
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
