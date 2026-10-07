/*
 * enemy/em020_prog.cpp - enemy 020's program: the `_ENEMY_WORK` motion-arming steps and their `state_sub`
 *   dispatcher, the model/K-colour refresh, the condition and area handlers, the hit-ratio and target tests, and
 *   the quest/lobby-message tail.
 * RANGE. .text 0x8036CF64-0x80378F9C (138 functions); extab 0x800178E4-0x80017C44, extabindex
 *   0x800373BC-0x800378CC, .rodata 0x80570A20-0x80570AE0, .data 0x805EE098-0x805EE518 (`em020_prog_tbl` first),
 *   .bss 0x806BF530-0x806C23E8, .sdata 0x807933B0-0x807933B8, .sbss 0x80794BF0-0x80794BF8, .sdata2
 *   0x8079B820-0x8079BC88.  Right edge: `em019_prog_tbl` opens the next `.data` block after this unit's last
 *   function `em020_row_ptr`.  The head 0x8036CF64-0x8036E26C is unproven as part of this TU: its ten bodies draw
 *   through the 2D library and read quest text ids, take a record that is not `_ENEMY_WORK`, and own no pool entry
 *   or `.data`; `em020_prog_tbl`'s first entry is 0x8036E2BC.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi` + `cmpwi`, `slwi` +
 *   `clrlwi` and `srwi` + `clrlwi` pairs in `em020_model_refresh`/`em020_condition_ck`/`em020_area_model_set`).
 * NAMES. The file name follows the runtime dump's `em020_prog_tbl`, which opens the TU's `.data`; `getInstance_`
 *   is the dump's name.  Every other named row (`em020_*`, `releaseRemotePlayerParts`, `sendMemberJoinNotice`,
 *   `handleLobbyNetMessage`, `dropLobbyMail`, `addFriendNotice`) is a GUESS from its body: the dump answers `zz_`
 *   or a linker-folded duplicate's name.
 *   GUESS (from each body and its callers): lobby_net_err_draw
 *   GUESS (from each body and its callers): em020_profile_send
 * RESIDUALS. 108 rows unwritten: 0x8036CF64-0x8036E26C, 0x8036E2BC-0x8036E570, 0x8036E574-0x8036E6B8,
 *   0x8036E8C4-0x8036EA38, 0x8036EBF4-0x8036FA58, 0x8036FB10-0x80370274, 0x80370368-0x803705A8,
 *   0x80370680-0x80370FC8, 0x803710AC-0x80375084, 0x80375628-0x803757E0, 0x8037586C-0x803759BC,
 *   0x803759C4-0x80376964, 0x803769D4-0x803788A0, 0x803788A8-0x80378F7C.
 *   3 partial rows:
 *  - `em020_arm_mot1_turn_wait79`: retail issues the `lfs`/`lis` of the float argument before the integer `li`;
 *  - `em020_arm_mot1_side_wait80`: retail computes `0x51 - (right == 0)` with `cntlzw; extrwi; neg; addi 0x51`,
 *    ours with `subfic r0,r0,0x51` (a hoisted local, the swapped ternary and `const` pool labels measure the same);
 *  - `em020_hp_ratio_ck`: retail branches on the float compare (`bne`; `li r3,1`/`li r3,0`) where ours
 *    materialises it with `mfcr; extrwi`, returns early on the kind test, and converts the two words unsigned
 *    (no `xoris r0,r0,0x8000`).
 *   flipcheck: `.bss`/`.data`/`.rodata`/`.sbss`/`.sdata` claimed, not emitted; `.sdata2` 0x8 against 0x468;
 *   `.text` (0xE48 of 0xC038), extab (0x88 of 0x360) and extabindex (0xCC of 0x510) short of the claim and
 *   differing.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/math.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "stage/stg_w.h"

/* The unit's `.sdata2` pool (0x8079B820..0x8079BC60, read only by this unit), declared, not defined: a
 * definition would emit the constant and a pool copy. */

extern "C" {
extern f32 lbl_8079B83C; /* 0.0 */
extern f32 lbl_8079B854; /* the approach scale `fn_80136D4C` takes */
extern f32 lbl_8079B858;
extern f32 lbl_8079B85C;
extern f32 lbl_8079B874;
extern f32 lbl_8079B87C;
extern f32 lbl_8079B8A4;
extern f32 lbl_8079B8C4;
extern f32 lbl_8079B8EC;
extern f32 lbl_8079B8F0;
}

#ifdef __cplusplus

extern "C" {
#endif
}
#include "gx.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck (the C++ declaration, which mangles to the map's name) */
#include "enemy/fn_80138074.h" /* fn_8013A9F4, the owner's own declaration */
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"     /* ScnMdl::CopiedMatAccess */
#include "g3d/g3d_resmat.h" /* ResTexSrt */
#include "fn_8004CAD8.h"         /* MTX34_ctor, fn_8005024C (their owner's header) */
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor (rule 2) */
#include "unsplit/sound.h"       /* fn_800E2994 */
#include "unsplit/unknown.h"     /* SystemWork / system_w */
#include "enemy/em020_ai.h"      /* em020_aim_target_ck (this unit's) */
#include "stage/stg_w.h"         /* fn_802B0A98, stage_map_kind_get (the owner is stage/stg_w.cpp) */
#include "ai/fn_802D44F4.h"      /* fn_802D94C4 (the owner is ai/ai_npc.cpp) */
#include "Pl/fn_8027D684.h"      /* fn_8027DC64 (the owner is Pl/pl_act.cpp) */

/* More of the unit's `.sdata2` floats, declared, not defined (values below). */
extern "C" const f32 lbl_8079B8CC; /* 0.5f   - the shake amplitude */
extern "C" const f32 lbl_8079BC64; /* 0.0025f - the per-frame step of the wrap test */
extern "C" const f32 lbl_8079B848; /* 1.0f   - the wrap bound */
extern "C" const f32 lbl_8079B870; /* 400.0f */
extern "C" const f32 lbl_8079B8D0; /* -1400.0f */
extern "C" const f32 lbl_8079BC68; /* -400.0f */
#include "enemy/em020_ai.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_80138074.h"
#include "Network/network_pat_control.h"
#include "sys_mem.h"

/* The unit's `.bss` lobby state block (0x2EB8 bytes; `lobby/lb_npc.cpp` reads the same +0x03 quest-active
 * byte), declared: the source does not emit it yet. */
extern u8 lobby_state_block[];
/* `lobby/lb_companion_ui.cpp`'s quest-page block (ten 0x130-byte records), from its leaf header. */
#include "lobby/lobby_hunter_cards.h"
/* The `.sbss` one-byte flag `em020_unknown_flag_set` writes. */
extern u8 lbl_80794BF4;
/* The pooled `.sdata2` constants this unit loads through `r2`.  Declared, never defined (playbook
 * 29/58): a definition would make MWCC emit a second copy and grow `.sdata2` instead of pairing. */
extern f32 lbl_8079BC6C; /* 0.65f */

#pragma peephole off

extern "C" {

/* Steps the +0x1BC angle (an `s16` in the low halfword) 0x40 toward zero, snapping inside +/-0x40, and
 * hands the record on. */
void em020_angle_step_to_zero(_ENEMY_WORK* self)
{
    u32 ang = self->field_0x1BC;
    u16 half = (u16)ang;

    if (half < 0x8000) {
        if (half < 0x40)
            self->field_0x1BC = 0;
        else
            self->field_0x1BC = ang - 0x40;
    } else if (half > 0xFFC0) {
        self->field_0x1BC = 0;
    } else {
        self->field_0x1BC = ang + 0x40;
    }

    fn_80133C3C(self);
}

/* Ticks the +0x334 countdown, and +0x33E in area 2 at stack depth 6; +0x338 latches "map 7, area 3"
 * each frame. */
void em020_timers_tick(_ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3)
        self->em020_0x328.latch_0x338 = 1;
    else
        self->em020_0x328.latch_0x338 = 0;

    if (self->em020_0x328.timer_0x334 > 0)
        self->em020_0x328.timer_0x334 -= 1;

    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 2 && self->stack_0x961[0] == 6) {
        if (self->em020_0x328.timer_0x33E > 0)
            self->em020_0x328.timer_0x33E -= 1;
    }
}

/* The two-step wake-up: tick, step the angle, then arm the 0x14-frame pose or wait for the motion and
 * release the record. */
void em020_arm_mot1_wait20(_ENEMY_WORK* self)
{
    fn_80133C3C(self);
    em020_angle_step_to_zero(self);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The per-frame byte the +0x1E6 sub-state makes `em020_arm_mot1_wait20` run for each of its five steps, with
 * every other sub-state falling straight through. */
void em020_substate_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot1_wait20(self);
        break;
    case 1:
        em020_arm_mot1_wait20(self);
        break;
    case 2:
        em020_arm_mot1_wait20(self);
        break;
    case 4:
        em020_arm_mot1_wait20(self);
        break;
    case 5:
        em020_arm_mot1_wait20(self);
        break;
    default:
        break;
    }
}

/* The same two-step wake-up for the second half of the pose: step 0 arms a 0x32/0x28-frame motion
 * (kind 3), step 1 waits for it and releases the record. */
void em020_arm_mot3_wait50(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x28, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The three-step wake-up: arm 0x37/0x14 and re-aim (the argument picks the +0x378 approach float), arm the
 * closing 0x2f/0x28 pose after the 0x80-frame window, then release. */
void em020_arm_approach_mot1_wait55(_ENEMY_WORK* self, u8 mode)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);

        switch (mode) {
        default:
            em_approach_start(self, lbl_8079B83C, 0x12);
            break;
        case 1:
            em_approach_start(self, lbl_8079B83C, 0x12);
            self->value_0x378 = lbl_8079B858;
            break;
        case 2:
            em_approach_start(self, lbl_8079B83C, 0x12);
            self->value_0x378 = lbl_8079B85C;
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state += 1;
            em_mot_set_blend(self, 0x2f, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x3a/0x0a-frame pose's two-step wake-up. */
void em020_arm_mot1_wait58(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3a, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4d/0x14-frame pose's wake-up: step 1 keeps facing the target (`em_turn_to_target`) until the
 * motion is done. */
void em020_arm_mot1_facing_wait77(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4d, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0xa);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B874, lbl_8079B83C) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4f/0x0a-frame pose's wake-up: step 1 re-aims with `fn_80136D4C`, closes with `em_turn_to_target`
 * when a frame window ends, and re-arms the turn. */
void em020_arm_mot1_turn_wait79(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4f, 0xa, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1: {
        if (em_frame_check(self, 2, lbl_8079B8EC, lbl_8079B83C) == 1 ||
            em_frame_check(self, 3, lbl_8079B8F0, lbl_8079B8C4) == 1) {
            fn_80136D4C(self, lbl_8079B854);
            em_turn_to_target(self, 0x50);
        }
        f32 lo = lbl_8079B8EC;
        f32 hi = lbl_8079B8F0;
        em_turn_in_window(self, lo, hi, 0x10000);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* The 0x4e/0x0a-frame pose's wake-up: step 0 arms the motion and the two 8/0x10-part effect slots,
 * step 1 re-aims and closes the pose once the frame window reports done. */
void em020_arm_mot1_effects_wait78(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4e, 0xa, 0);
        em_hit_window_set(self, 0, 0xb, 8);
        em_hit_window_set(self, 1, 0xc, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B8A4, lbl_8079B83C) == 0) {
            fn_80136D4C(self, lbl_8079B854);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The two-directional 0x50/0x51-frame pose's wake-up: the argument picks the motion (0x51 when set,
 * 0x50 otherwise) and the sign of the 0x4000-step turn `em_turn_in_window` re-arms every frame. */
void em020_arm_mot1_side_wait80(_ENEMY_WORK* self, u8 right)
{
    Vec3 keys;

    VEC3_ctor(&keys);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, (u16)(0x51 - (right == 0)), 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x11);
        break;
    case 1: {
        f32 lo = lbl_8079B87C;
        f32 hi = lbl_8079B8C4;
        em_turn_in_window(self, lo, hi, right == 0 ? 0x4000 : -0x4000);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* One handler slot of `em020_prog_tbl` that does nothing - the table's index 4. */
void em020_nop(_ENEMY_WORK* self)
{
    (void)self;
}

#ifdef __cplusplus
}
#endif

/* Pushes the model state into the scene model: the effect matrices of material slots 8 and 9, then the
 * K-colours (6/3 from the record, 0..3 from the area/action test). */
extern "C" void em020_model_refresh(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access_a((nw4r::g3d::ScnMdl*) self->field_0x13C, 8);
        nw4r::g3d::ScnMdl::CopiedMatAccess access_b((nw4r::g3d::ScnMdl*) self->field_0x13C, 9);

        if (fn_800E2994(&access_a) != 0 && fn_800E2994(&access_b) != 0) {
            nw4r::g3d::ResTexSrt srt_a;
            nw4r::g3d::ResTexSrt srt_b;

            res_tex_srt_copy_ctor(&srt_a, access_a.GetResTexSrt(false));
            res_tex_srt_copy_ctor(&srt_b, access_b.GetResTexSrt(false));
            srt_a.GetEffectMtx(1, &mtx);
            mtx.m[0][3] = lbl_8079B8CC * fn_8005024C((u16) (system_w.field_0x0c << 7));
            mtx.m[1][3] += lbl_8079BC64;
            if (mtx.m[1][3] > lbl_8079B848) {
                mtx.m[1][3] -= lbl_8079B848;
            }
            srt_a.SetEffectMtx(1, &mtx);
            srt_b.SetEffectMtx(1, &mtx);
        }
    }

    ((MHchar*) &self->char_0x024)->getTevKColor(6, GX_KCOLOR3, &color);
    color.r = self->em020_kcolor_0x328.kcolor_r_0x339;
    color.g = self->em020_kcolor_0x328.kcolor_g_0x33A;
    color.b = self->em020_kcolor_0x328.kcolor_b_0x33B;
    ((MHchar*) &self->char_0x024)->setTevKColor(6, GX_KCOLOR3, &color);

    ((MHchar*) &self->char_0x024)->getTevKColor(0, GX_KCOLOR0, &color);
    if (self->area_no == 0 || em_act_ck(self, 13, 2) != 0) {
        color.r = 150;
        color.g = 150;
        color.b = 150;
    } else {
        color.r = 255;
        color.g = 255;
        color.b = 255;
    }
    ((MHchar*) &self->char_0x024)->setTevKColor(0, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(1, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(2, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(3, GX_KCOLOR0, &color);
}

/* The program's per-mode condition query: mode 0 is the target/self height difference as a 0..4
 * level, mode 1 the aim-target flag, mode 2 the slot-free test and mode 3 a timer's sign. */
extern "C" u8 em020_condition_ck(_ENEMY_WORK* self, u32 mode) {
    f32 diff;

    switch ((u8) mode) {
    case 0:
        diff = self->vec_0x36C.y - self->pos.y;
        if (diff >= lbl_8079B858) {
            return 2;
        }
        if (diff >= lbl_8079B870) {
            return 1;
        }
        if (diff <= lbl_8079B8D0) {
            return 4;
        }
        if (diff <= lbl_8079BC68) {
            return 3;
        }
        return 0;
    case 1:
        return em020_aim_target_ck(self) == 1;
    case 2:
        if (self->em020_0x328.timer_0x334 <= 0 && fn_8027DC64() == 1) {
            return 1;
        }
        return 0;
    case 3:
        return self->em020_0x328.timer_0x33E > 0;
    default:
        return 0;
    }
}

/* On the map that owns the two area models: reset the interpreter stack, then pick the stage table entry
 * and the model's area mode by area. */
extern "C" void em020_area_model_set(_ENEMY_WORK* self) {
    fn_8013A9F4(self);

    if (stage_map_kind_get(self->field_0x1E0) == 7) {
        switch (self->area_no) {
        case 2:
            fn_802B0A98(9, 1);
            fn_802D94C4(0);
            break;
        case 3:
            fn_802B0A98(10, 1);
            fn_802D94C4(1);
            break;
        }
    }
}

extern "C" {

/* 0x80375540: the area hit's damage-level gate: fold every part's damage level into `out->levels_0x01` and
 * copy out the facing angle and damage numerator. */
void em020_hit_info_get(struct _ENEMY_WORK* self, struct Em020HitInfo* out)
{
    if (self->area_no == 3) {
        out->hit_0x00 = 1;
        out->levels_0x01 = 0;
        if ((self->flags_0x836 & 2) != 0) {
            out->levels_0x01 |= 1;
        }
        if ((self->flags_0x836 & 0x8000) != 0) {
            out->levels_0x01 |= 2;
        }
        if (em_parts_damage_level_get(self, 3) >= 2) {
            out->levels_0x01 |= 4;
        }
        if (em_parts_damage_level_get(self, 5) >= 1) {
            out->levels_0x01 |= 8;
        }
        out->angle_0x02 = self->parts_0x838[0].value_0x04;
        out->damage_0x04 = self->field_0x7A0;
    } else {
        out->hit_0x00 = 0;
    }
}

/* 0x803754EC: hands the area's third resource record to the `fn_8013A654` installer. */
void em020_res_user_data_apply(struct _ENEMY_WORK* self)
{
    fn_8013A654(self, 3);
}

/* The em020 aim-target predicate: whether the `+0x836` bit 15 "aim target found" flag is set.
 * 0x803754F4 */
u32 em020_aim_target_ck(struct _ENEMY_WORK* self)
{
    return (self->flags_0x836 & 0x8000) != 0;
}

/* 0x80375424: the low-HP predicate: area 3 and the damage numerator's share of the denominator at or below
 * 0.65. */
u32 em020_hp_ratio_ck(struct _ENEMY_WORK* self)
{
    f32 ratio;

    if (self->area_no != 3) {
        return 0;
    }
    ratio = (f32)self->field_0x7A0 / (f32)self->field_0x7A4;
    if (ratio <= lbl_8079BC6C) {
        return 1;
    }
    return 0;
}

/* 0x80375494: on map 7 in area 3, request action 13 sub-state 3. */
void em020_map_area_action_set(struct _ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) {
        fn_80128AEC(self, 13, 3);
    }
}

/* The em020 area-2 action-20 sub-state-30 predicate.
 * 0x8037550C */
u32 em020_area2_action20_ck(struct _ENEMY_WORK* self)
{
    if (self->team == 20 && self->area_no == 2 && self->stack_0x961[0] == 30) {
        return 1;
    }
    return 0;
}

/* 0x803757E0: releases the sub-record through the shared teardown, freeing the block on a positive size. */
void* em020_work_free(struct _ENEMY_WORK* self, s16 size)
{
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (size > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The empty stub the program table reserves for em020.
 * 0x80376964 */
void em020_noop(void)
{
}

/* The em020 quest-page pointer: the address of the shared quest-page block's fourth record.
 * 0x80376968 */
u8* em020_quest_page_ptr(void)
{
    return lobby_hunter_cards[0].key_0x03;
}

/* The em020 "false" program-table stub.
 * 0x803788A0 */
u32 em020_false_ck(void)
{
    return 0;
}

/* Writes the em020 one-byte `.sbss` flag.
 * 0x803759BC */
void em020_unknown_flag_set(u8 value)
{
    lbl_80794BF4 = value;
}

/* The em020 quest-active predicate: whether the shared lobby block's `+0x03` byte is 1.
 * 0x8037583C */
u32 em020_quest_active_ck(void)
{
    return lobby_state_block[3] == 1;
}

/* Clears the shared lobby block's `+0x03` quest-active byte.
 * 0x80375858 */
void em020_quest_active_clear(void)
{
    lobby_state_block[3] = 0;
}

/* Clears the nine quest-page records after the first in the shared quest-page block.
 * 0x80376978 */
void em020_quest_pages_clear(void)
{
    s32 i;

    for (i = 1; i < 10; i++) {
        memset(&lobby_hunter_cards[i], 0, sizeof(LbQuestPage));
    }
}

/* The address of the em020 twelve-entry 31-byte row array at the shared lobby block's `+0x13E6`.
 * 0x80378F7C */
u8* em020_row_ptr(u8 index)
{
    u8* rows = lobby_state_block + 0x13E6;
    return rows + index * 31;
}

#ifdef __cplusplus
}
#endif

