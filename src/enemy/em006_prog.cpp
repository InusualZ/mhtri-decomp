/*
 * enemy/em006_prog.cpp - enemy 006's program: the `em006_prog_tbl` entry points (init, frame update, action
 *   dispatch, event hook, motion events, part/area/action-selection slots), the per-action state steps the
 *   `state_sub` dispatchers select, the two sound programs, the scripted entrance, the effect router and the static
 *   initializer.
 * RANGE. .text 0x801BB758-0x801C29F8 (74 functions); extab 0x8000F9A4-0x8000FB84, extabindex
 *   0x8002B560-0x8002B830, .ctors 0x8056F350-0x8056F354 (`__sinit_\em006_prog_cpp`), .rodata 0x805702A0-0x80570320, .data
 *   0x805B28D0-0x805B3B7C (`em006_prog_tbl` first), .bss 0x806A7AD0-0x806A7B18, .sdata 0x80791AB8-0x80791AD8,
 *   .sdata2 0x80798E40-0x80798FE8.  Left edge: the 0.0 pool entry repeats at 0x80798E40, first read by
 *   `em006_front_blocked_ck`; right edge: `__sinit_\em006_prog_cpp` is this TU's static initializer.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`+`cmpwi` forms),
 *   `#pragma pool_data off` (each table is addressed with its own `lis`/`addi`) and `#pragma fp_contract off` (the
 *   target has no fused multiply-add).
 * NAMES. The file name follows the runtime dump's `em006_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em024ProgTbl` slot each fills): em006_init, em006_frame_update, em006_action_dispatch, em006_noop,
 *   em006_event_notify, em006_motion_events, em006_tev_color_update, em006_part_damage_ck, em006_area_start_set,
 *   em006_part_level_ck, em006_alt_wait_ck, em006_action_select.
 *   GUESS (dispatcher slot): em006_act<action>_<first sub-state> and em006_act<action>_dispatch,
 *   em006_act10_se_play, em006_act11_se_play; GUESS (from the bodies): em006_front_blocked_ck,
 *   em006_front_slot_ck, em006_alt_mode_effect, em006_effect_spawn; the static initializer is MWCC's
 *   `__sinit_\em006_prog_cpp`.
 *   GUESS (data): em006_turn_seq_0/1, em006_lift_curve, em006_drop_curve, em006_pick_tbl,
 *   em006_se_seq_<sub-state>, em006_se_alt_seq_<sub-state> and their em006_se_rec/pos/ofs/loop records,
 *   em006_vec_pair_0..2, the `Em006ProgTbl` slot names.
 * * RESIDUALS. No row or data residual: every function and every data section is byte-identical to the target.
 * * SHAPES. A one-case test retail compares signed (`cmpwi`) is a `switch` with `default:` first (`em006_init`,
 *   `em006_act2_0`, `em006_act7_1`, `em006_act7_3`); `(s32)kind == 2` keeps `em006_action_select`'s `cmpwi`.  The
 *   +0x14 shell-table slot is called without its table argument (`ShellPosJobFn`).  `em_pack_area_pick`,
 *   `em_area_break_ck`, `em_act_ck`, `em_work_die_ck`, `UpdateValue`, `get_move_work_max`, `em_move_rot_ground_step` and
 *   `em_ground_impact_set` are called through
 *   `_c1`/`_c2` views whose widths give retail's masks and unsigned compares.  The data is defined between the
 *   functions in retail order (the jump tables fall between the definitions).
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_8011D448.h"
#include "fn_8004CAD8.h"
#include "unsplit/enemy.h"
#include "ef/eft009.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef.h"
#include "ef/cp_vector.h"
#include "ef/get_move_work_adrs.h"
#include "lobby/fn_8030121C.h"
#include "stage/shell_set_func_ptr.h"
#include "mh3_pad/system_w.h"
#include "sound/fn_800D7F54.h"
#include "stage/stg_w.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_flags836_ck.h"
#include "enemy/em_turn_in_window.h"
#include "enemy/em_roll_cooldown_ck.h"
#include "enemy/em_model.h"
#include "enemy/em_ground_impact_set.h"
#include "enemy/em_area_stun_move_ck.h"
#include "enemy/em_common.h"
#include "Pl/fn_8028F66C.h"
#include "draw_shape.h"
#include "sound/se.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_fall_height_get_c1 ((f32 (*)(struct _ENEMY_WORK*))em_fall_height_get)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_roll_cooldown_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u16, u16, s16*))em_roll_cooldown_ck)
#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)
#define draw_shape_arm_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32))draw_shape_arm)
#define UpdateValue_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))UpdateValue)
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#define em_area_break_ck_c2 ((u8 (*)(struct _ENEMY_WORK*, u32))em_area_break_ck)
#define em_work_die_ck_c1 ((u32 (*)(struct _ENEMY_WORK*))em_work_die_ck)
#define em_pack_area_pick_c1 ((u32 (*)(struct _ENEMY_WORK*, u32, u8, u16, u8*, u8*))em_pack_area_pick)
#define em_move_rot_ground_step_c1 ((void (*)(struct _ENEMY_WORK*, u32*, f32))em_move_rot_ground_step)
#define em_ground_impact_set_c1 ((void (*)(struct _ENEMY_WORK*, u8, u8, f32, nw4r::math::VEC3*))em_ground_impact_set)
#define get_move_work_max_c1 ((u16 (*)(u8))get_move_work_max)

/* The +0x14 shell-table slot spelled without its trailing table argument: the call site passes none, and the
 * compiler then loads the table last, into the register the argument would take. */
typedef void (*ShellPosJobFn)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, u32 b, u16 c);

/* The em006 program table (`em006_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em024ProgTbl`; the empty slots are zero).
 * size: 0x70 */
struct Em006ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ s32 (*part_damage_ck_0x24)(struct _ENEMY_WORK* self, u8 part);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ u32 pad_0x34[4];
    /* +0x44 */ s32 (*part_level_ck_0x44)(struct _ENEMY_WORK* self, u8 part);
    /* +0x48 */ s32 (*alt_wait_ck_0x48)(struct _ENEMY_WORK* self);
    /* +0x4C */ u32 pad_0x4C[2];
    /* +0x54 */ u32 (*action_select_0x54)(struct _ENEMY_WORK* self, u32 arg);
    /* +0x58 */ u32 pad_0x58[6];
};

extern "C" {
void em006_init(struct _ENEMY_WORK* self, u8 mode);
void em006_frame_update(struct _ENEMY_WORK* self);
void em006_action_dispatch(struct _ENEMY_WORK* self);
void em006_noop(void);
void em006_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em006_motion_events(struct _ENEMY_WORK* self);
void em006_tev_color_update(void);
s32 em006_part_damage_ck(struct _ENEMY_WORK* self, u8 part);
void em006_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub);
s32 em006_part_level_ck(struct _ENEMY_WORK* self, u8 part);
s32 em006_alt_wait_ck(struct _ENEMY_WORK* self);
u32 em006_action_select(struct _ENEMY_WORK* self, u32 arg);

Em006ProgTbl em006_prog_tbl = {
    0,
    em006_init,
    em006_frame_update,
    em006_action_dispatch,
    em006_noop,
    em006_event_notify,
    em006_motion_events,
    0,
    em006_tev_color_update,
    em006_part_damage_ck,
    {0, 0},
    em006_area_start_set,
    {0, 0, 0, 0},
    em006_part_level_ck,
    em006_alt_wait_ck,
    {0, 0},
    em006_action_select,
    {0, 0, 0, 0, 0, 0},
};
}

/* The two (frame, value) key curves `em_key_curve_eval` reads for the offset radius: the drop (`em006_act3_3`) and
 * the lift (`em006_act3_4`); a frame of -1 ends each. */
f32 em006_lift_curve[20] = {
    0.0f, 14.0f, 10.0f, 21.6f, 20.0f, 18.0f, 30.0f, 1.2f, 50.0f, -1.2f,
    70.0f, 6.6f, 90.0f, 9.0f, 100.0f, -1.2f, 120.0f, 1.2f, -1.0f, 9.6f,
};

f32 em006_drop_curve[14] = {
    0.0f, 57.0f, 8.0f, 51.0f, 16.0f, 22.0f, 24.0f, -5.0f, 32.0f, -13.0f, 48.0f, -12.0f, -1.0f, 0.0f,
};

/* The pick record `em006_action_select` hands `em_pack_area_pick` (the action pair and its weights). */
u8 em006_pick_tbl[12] = {0x00, 0x00, 0x00, 0x0F, 0x00, 0xC8, 0x00, 0x0A, 0xFF, 0xFF, 0x00, 0x00};

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * four-word rows whose first word carries the motion in its high half. */
extern "C" const u32 em006_turn_seq_0[16] = {
    0x00000000, 0x00000000, 0x0000006E, 0x40000E39,
    0x00060000, 0x00000006, 0x00000004, 0x00000000,
    0x000A0000, 0x0000000A, 0x00000000, 0x00000000,
    0x000B0000, 0x0000000A, 0x00000000, 0x00000000,
};

extern "C" const u32 em006_turn_seq_1[16] = {
    0x00000000, 0x00000014, 0x00000000, 0x40001555,
    0x000E0000, 0x00000002, 0x00000000, 0x00000000,
    0x000C0000, 0x00000002, 0x00000000, 0x00000000,
    0x000D0000, 0x00000002, 0x00000000, 0x00000000,
};

/* ------------------------------------------------------------------------------------------------ */
/* data                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* A point and its per-frame step, built by MWCC's `__sinit` (0x801C28FC): each vector is set through `setVec3` into
 * a temporary and copied with `assignVec3`.  size: 0x18 */
struct Em006VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, y, 0) and (0, dy, 0). */
    Em006VecPair(f32 y, f32 dy)
    {
        VEC3 b;
        VEC3 a;

        assignVec3_c1(&pos, setVec3(&a, 0.0f, y, 0.0f));
        assignVec3_c1(&step, setVec3(&b, 0.0f, dy, 0.0f));
    }
};

/* The three vector pairs (`.bss`, defined at the end); three sound records point at them. */
extern Em006VecPair em006_vec_pair_0;
extern Em006VecPair em006_vec_pair_1;
extern Em006VecPair em006_vec_pair_2;
/* The per-sub-state sound programs the two sound players run (`.data`; the name carries the first sub-state that
 * plays each). */
extern EmSeEntry em006_se_seq_17[]; /* 0x805B2B20 */
extern EmSeEntry em006_se_seq_18[]; /* 0x805B2AF8 */
extern EmSeEntry em006_se_seq_1A[]; /* 0x805B2C28 */
extern EmSeEntry em006_se_seq_1C[]; /* 0x805B2B78 */
extern EmSeEntry em006_se_seq_1D[]; /* 0x805B2BE8 */
extern EmSeEntry em006_se_seq_58[]; /* 0x805B2ED8 */
extern EmSeEntry em006_se_seq_23[]; /* 0x805B2CB0 */
extern EmSeEntry em006_se_seq_34[]; /* 0x805B3140 */
extern EmSeEntry em006_se_seq_7A[]; /* 0x805B2D18 */
extern EmSeEntry em006_se_seq_7B[]; /* 0x805B2D40 */
extern EmSeEntry em006_se_seq_7C[]; /* 0x805B2D68 */
extern EmSeEntry em006_se_seq_8D[]; /* 0x805B2D90 */
extern EmSeEntry em006_se_seq_7E[]; /* 0x805B2DE8 */
extern EmSeEntry em006_se_seq_7F[]; /* 0x805B2E50 */
extern EmSeEntry em006_se_seq_9F[]; /* 0x805B2EB0 */
extern EmSeEntry em006_se_seq_A8[]; /* 0x805B2E88 */
extern EmSeEntry em006_se_seq_B2[]; /* 0x805B3168 */
extern EmSeEntry em006_se_seq_B6[]; /* 0x805B2F00 */
extern EmSeEntry em006_se_seq_B7[]; /* 0x805B2F28 */
extern EmSeEntry em006_se_seq_B8[]; /* 0x805B2F98 */
extern EmSeEntry em006_se_seq_B9[]; /* 0x805B2FF0 */
extern EmSeEntry em006_se_seq_BA[]; /* 0x805B3018 */
extern EmSeEntry em006_se_seq_BB[]; /* 0x805B3040 */
extern EmSeEntry em006_se_seq_BF[]; /* 0x805B3080 */
extern EmSeEntry em006_se_seq_CA[]; /* 0x805B30F8 */
extern EmSeEntry em006_se_seq_E4[]; /* 0x805B3198 */
extern EmSeEntry em006_se_seq_E5[]; /* 0x805B3208 */
extern EmSeEntry em006_se_seq_E6[]; /* 0x805B3260 */
extern EmSeEntry em006_se_seq_E7[]; /* 0x805B3288 */
extern EmSeEntry em006_se_seq_E8[]; /* 0x805B32B0 */
extern EmSeEntry em006_se_alt_seq_00[]; /* 0x805B3628 */
extern EmSeEntry em006_se_alt_seq_05[]; /* 0x805B3680 */
extern EmSeEntry em006_se_alt_seq_0B[]; /* 0x805B36E8 */
extern EmSeEntry em006_se_alt_seq_0F[]; /* 0x805B3798 */
extern EmSeEntry em006_se_alt_seq_1C[]; /* 0x805B3768 */
extern EmSeEntry em006_se_alt_seq_1A[]; /* 0x805B3738 */

/* ------------------------------------------------------------------------------------------------ */
/* callees                                                                                          */
/* ------------------------------------------------------------------------------------------------ */


extern "C" {

/* This unit's own functions, declared so the dispatchers can call them. */
s32 em006_front_blocked_ck(struct _ENEMY_WORK* self);
u32 em006_front_slot_ck(struct _ENEMY_WORK* self);
void em006_act6_E(struct _ENEMY_WORK* self, u8 kind);
void em006_act6_F(struct _ENEMY_WORK* self);
void em006_act7_0(struct _ENEMY_WORK* self);
void em006_act7_5(struct _ENEMY_WORK* self);
void em006_act7_6(struct _ENEMY_WORK* self);
void em006_act7_8(struct _ENEMY_WORK* self, u8 kind);
void em006_act7_F(struct _ENEMY_WORK* self);
void em006_act12_0(struct _ENEMY_WORK* self);
void em006_alt_mode_effect(struct _ENEMY_WORK* self);
void em006_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, u32 joint, s32 delta, f32 scale);

/* 0x801BB758 (0xE0): true when the point ahead of the work, 400 + 130 * the scale out, is blocked for a sweep from
 * the work's position. */
s32 em006_front_blocked_ck(struct _ENEMY_WORK* self)
{
    VEC3 ofs;
    VEC3 to;
    VEC3 sum;

    VEC3_ctor(&ofs);
    VEC3_ctor(&to);
    if (em_parts_damage_level_get(self, 0) >= 2) {
        return 0;
    }
    ofs.x = 0.0f;
    ofs.y = 0.0f;
    ofs.z = 400.0f + 130.0f * get_em_chg_scale(self);
    rotVecY(&ofs, self->field_0x1C0);
    addVec3(&sum, &self->pos, &ofs);
    copyVec3(&to, &sum);
    return pl_coll_sweep_ck(&self->pos, &to, 155.0f, 1, 0x100, self->area_no, em_hit_mask_get(self)) != 0;
}

/* 0x801BB838 (0xEC): true when one of the first motion slots in use faces within 0x1000 of straight ahead with
 * flag 0x100 set. */
u32 em006_front_slot_ck(struct _ENEMY_WORK* self)
{
    u8 i;

    if (em_parts_damage_level_get(self, 0) >= 2) {
        return 0;
    }
    if (self->field_0x218 == 0) {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        EmMotionSlot* slot = &self->slots_0x244[i];

        if (slot->flags == 0) {
            break;
        }
        if ((slot->value - 0x1001 > 0xDFFE) && (slot->flags & 0x100) != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x801BB924 (0x200): the program's init: for `mode == 2` places the work at its area's start point, attaches the
 * user-data record when the work has none and arms the body effect for a fresh work. */
void em006_init(struct _ENEMY_WORK* self, u8 mode)
{
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (mode) {
    case 2:
        switch (stage_map_kind_get(self->field_0x1E0)) {
        case 2:
            switch (self->area_no) {
            case 4:
                em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 1), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 7, 0xF);
                break;
            case 6:
                em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 3), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 7, 0xF);
                break;
            }
            break;
        case 8:
            switch (self->area_no) {
            case 1:
                em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 7, 0xF);
            }
            break;
        case 9:
            switch (self->area_no) {
            case 0:
                em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 7, 0xF);
            }
            break;
        }
        em_state_refresh(self);
        break;
    }
    if (em_res_user_data_ck(self) == 0) {
        void* data = operator new(0xC);
        if (data != 0) {
            em_res_user_data_ctor(data);
        }
        em_res_user_data_set(self, data);
    }
    if (self->field_0x009 == 0) {
        setVector3(&vec, 0.0f, -60.0f, 20.0f);
        eft_em_attach_set(self, 0x19, &vec, 1.0f, 0x2000);
    }
    self->init_0x328.slots_0x328[0] = 0;
}

/* 0x801BBB24 (0x4): the empty program slot. */
void em006_noop(void)
{
}

/* 0x801BBB28 (0xC4): the program's event hook: the bait and flag reactions, the part hit of event 0xCA and the
 * 600/300-frame timer of the 0x34/0x97/0xB2 events. */
void em006_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub)
{
    switch (kind) {
    case 1:
        switch (sub) {
        case 4:
            em_bait_react(self);
            break;
        case 11:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 202:
            em_part_hit_set(self, 0, 0);
            break;
        case 52:
        case 151:
        case 178:
            if (em_eft_active_ck(self) == 1) {
                self->timer_0x020 = 600;
            } else {
                self->timer_0x020 = 300;
            }
            break;
        }
        break;
    }
}

/* 0x801BBBEC (0x70): the program's per-frame bookkeeping: drops the height outside mode 4 and raises bit 0x8000 of
 * +0x836 once part 0 is broken. */
void em006_frame_update(struct _ENEMY_WORK* self)
{
    if (self->field_0x1E2 != 4) {
        em_height_set(self, 0.0f);
    }
    if ((self->flags_0x836 & 0x8000) == 0 && em_parts_damage_level_get(self, 0) >= 2) {
        self->flags_0x836 |= 0x8000;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 0                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BBC5C (0x7C): plays motion 1 once. */
void em006_act0_0(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BBCD8 (0x7C): plays motion 2 once. */
void em006_act0_1(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BBD54 (0x7C): plays motion 0x11 once. */
void em006_act0_2(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BBDD0 (0x80): starts the fall and plays motion 0x1A once. */
void em006_act0_3(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, em_fall_height_get_c1(self));
        em_mot_set_ck(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801BBE50 (0x8C): plays motion 1 in mode 4 at the resting height. */
void em006_act0_7(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BBEDC (0x48): selects the action-0 step by `state_sub`. */
void em006_act0_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act0_0(self);
        break;
    case 1:
        em006_act0_1(self);
        break;
    case 2:
        em006_act0_2(self);
        break;
    case 3:
        em006_act0_3(self);
        break;
    case 7:
        em006_act0_7(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 1                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BBF24 (0x8C): plays motion 5 with hit window 0xC. */
void em006_act1_0(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 4, 0);
        em_hit_window_set_default(self, 0, 0xC);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BBFB0 (0x7C): plays motion 0x11 once. */
void em006_act1_1(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC02C (0x7C): plays motion 0x12 once. */
void em006_act1_2(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC0A8 (0x9C): holds motion 0x13 busy for 300 frames, then re-enters state (1, 4). */
void em006_act1_3(struct _ENEMY_WORK* self)
{
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        self->timer_0x020 = 300;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set(self, 1, 4);
        }
        break;
    }
}

/* 0x801BC144 (0x88): plays motion 0x14 busy. */
void em006_act1_4(struct _ENEMY_WORK* self)
{
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 8, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC1CC (0x11C): the rest: motion 0x17, then motion 0x18 held for 1800 frames with the tint and the gauge
 * tick, then motion 0x19. */
void em006_act1_5(struct _ENEMY_WORK* self)
{
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
            self->timer_0x020 = 1800;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 10);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x19, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC2E8 (0x110): motion 0xCC with hit window 0x13; at frame 110 hands joint 0x32's position to the shell job
 * table. */
void em006_act1_6(struct _ENEMY_WORK* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCC, 6, 0);
        em_hit_window_set(self, 0, 0x13, 8);
        break;
    case 1:
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1 && em_flags836_ck(self, 1) == 0 && self->field_0xAE9 == 1) {
            get_joint_wpos_em(self, 0x32, &pos);
            ((ShellPosJobFn)shell_set_func_ptr->method_0x14)(self, &pos, 0x17, self->area_no, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC3F8 (0x7C): plays motion 0x7C once. */
void em006_act1_7(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC474 (0xEC): drops an item and holds motion 0xD6 for 240 frames while the way ahead stays blocked, then
 * plays motion 0xD7. */
void em006_act1_8(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD6, 6, 0);
        self->timer_0x020 = 240;
        em_drop_item_spawn(self, &self->pos, 100);
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0xD7, 0, 0);
        } else if (em006_front_blocked_ck(self) == 0) {
            em_state_set(self, 1, 0xA);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC560 (0x7C): plays motion 1 once. */
void em006_act1_9(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC5DC (0x7C): plays motion 0x64 once. */
void em006_act1_A(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x64, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC658 (0x7C): plays motion 0x11 once. */
void em006_act1_B(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC6D4 (0x58): selects the action-1 step by `state_sub`. */
void em006_act1_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act1_0(self);
        break;
    case 1:
        em006_act1_1(self);
        break;
    case 2:
        em006_act1_2(self);
        break;
    case 3:
        em006_act1_3(self);
        break;
    case 4:
        em006_act1_4(self);
        break;
    case 5:
        em006_act1_5(self);
        break;
    case 6:
        em006_act1_6(self);
        break;
    case 7:
        em006_act1_7(self);
        break;
    case 8:
        em006_act1_8(self);
        break;
    case 9:
        em006_act1_9(self);
        break;
    case 10:
        em006_act1_A(self);
        break;
    case 11:
        em006_act1_B(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 2                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BC72C (0xEC): motion 6 with an approach (backwards 200 unless `kind == 1`, then capped at 700). */
void em006_act2_0(struct _ENEMY_WORK* self, u8 kind)
{
    if (kind == 2) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 6, 0);
        switch (kind) {
        default:
            em_approach_start(self, -200.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC818 (0xEC): the same approach with motion 7. */
void em006_act2_1(struct _ENEMY_WORK* self, u8 kind)
{
    if (kind == 2) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        switch (kind) {
        default:
            em_approach_start(self, -200.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC904 (0xA4): runs the first turn sequence. */
void em006_act2_2(struct _ENEMY_WORK* self, u8 kind)
{
    if (kind == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em006_turn_seq_0, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em006_turn_seq_0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BC9A8 (0x8C): runs the second turn sequence. */
void em006_act2_3(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em006_turn_seq_1, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em006_turn_seq_1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BCA34 (0x110): motion 8 with hit window 0x22 and an approach by `kind` (350, -900 or -1100), then
 * motion 9. */
void em006_act2_6(struct _ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 0x22, 2);
        {
            f32 dist;

            switch (kind) {
            default:
                dist = 350.0f;
                break;
            case 1:
                dist = -900.0f;
                break;
            case 2:
                dist = -1100.0f;
                break;
            }
            em_approach_start(self, dist, 0);
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 9, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BCB44 (0x84): selects the action-2 step by `state_sub`. */
void em006_act2_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act2_0(self, 0);
        break;
    case 1:
        em006_act2_1(self, 0);
        break;
    case 2:
        em006_act2_2(self, 0);
        break;
    case 3:
        em006_act2_3(self);
        break;
    case 4:
        em006_act2_0(self, 1);
        break;
    case 5:
        em006_act2_1(self, 1);
        break;
    case 6:
        em006_act2_6(self, 0);
        break;
    case 7:
        em006_act2_6(self, 1);
        break;
    case 8:
        em006_act2_6(self, 2);
        break;
    case 9:
        em006_act2_0(self, 2);
        break;
    case 10:
        em006_act2_1(self, 2);
        break;
    case 11:
        em006_act2_2(self, 1);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 3                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BCBC8 (0xD4): falls with motion 0x1A and a backwards approach, the offset radius clamped at 70. */
void em006_act3_0(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, em_fall_height_get_c1(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_approach_start(self, -300.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_fall(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 70.0f) {
                self->field_0x318 = 70.0f;
            }
        }
        break;
    }
}

/* 0x801BCC9C (0x84): falls with motion 0x1A and turns to the target. */
void em006_act3_1(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, em_fall_height_get_c1(self));
        em_mot_set_ck(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x100) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801BCD20 (0x16C): the dive: motion 0x1A until the dive lands, then motions 0x1D, 0x2E and 0x41. */
void em006_act3_2(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, em_fall_height_get_c1(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (UpdateValue_c1(self, em_fall_height_get(self)) == 1) {
            self->state++;
            em_mot_set(self, 0x1D, 6, 0);
        }
        break;
    case 2:
        UpdateValue_c1(self, em_fall_height_get(self));
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x2E, 6, 0);
        }
        break;
    case 3:
        UpdateValue_c1(self, em_fall_height_get(self));
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

/* 0x801BCE8C (0xF4): motion 0x29, then a fall with motion 0x43 along the ground. */
void em006_act3_3(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_c1(self, em_fall_height_get_c1(self));
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em006_drop_curve);
        em_move_rot_ground_step_c1(self, &self->field_0x1BC, em_fall_height_get(self));
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801BCF80 (0x118): the lift: two loops of motion 0x1A, then lifts until 1600 above the target. */
void em006_act3_4(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_start_c1(self, em_fall_height_get_c1(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, em006_lift_curve);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1) {
                if (++self->state_0x007 >= 2) {
                    self->state_0x006++;
                    em_lift_start(self);
                }
            }
            break;
        case 1:
            em_lift_step(self);
            break;
        }
        if (self->pos.y >= 1600.0f + self->vec_0x36C.y) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801BD098 (0x48): selects the action-3 step by `state_sub`. */
void em006_act3_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act3_0(self);
        break;
    case 1:
        em006_act3_1(self);
        break;
    case 2:
        em006_act3_2(self);
        break;
    case 3:
        em006_act3_3(self);
        break;
    case 4:
        em006_act3_4(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 6                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BD0E0 (0x1AC): motion 0xCD (three loops when `kind == 1`), then the seated slam 0xDB with its shake and
 * hit window. */
void em006_act6_0(struct _ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        if (kind == 1) {
            self->state_0x006 = 2;
        } else {
            self->state_0x006 = 0;
        }
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCD, 4, 0);
        em_hit_window_set(self, 0, 0x15, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (self->state_0x006 != 0) {
                self->state_0x006--;
            } else {
                self->state++;
                em_mot_set(self, 0xDB, 0, 0);
                em_busy_set(self);
                em_busy_timer_reset(self);
            }
        }
        break;
    case 2:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 1, 20.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
            em_fx_flag2_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_busy_timer_clear(self);
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD28C (0xC8): rests in mode 4 for 90 frames (1 when `kind == 1`). */
void em006_act6_1(struct _ENEMY_WORK* self, u8 kind)
{
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        if (kind == 1) {
            self->timer_0x020 = 1;
        } else {
            self->timer_0x020 = 90;
        }
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD354 (0xB0): steps back 200 in mode 4 with motion 6. */
void em006_act6_2(struct _ENEMY_WORK* self)
{
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 0);
        em_approach_start(self, -200.0f, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD404 (0x17C): the seated crawl: motion 8 at speed 1.5 with an approach by `kind` (-700, 0 or -1500) and
 * the crawl sound at joint 3. */
void em006_act6_3(struct _ENEMY_WORK* self, u8 kind)
{
    VEC3 pos;
    f32 dist;

    VEC3_ctor(&pos);
    em_fx_flag2_set(self);
    if (kind == 2) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 0, 0);
        em_mot_speed_set(self, 1.5f);
        em_hit_window_set(self, 0, 0x21, 2);
        switch (kind) {
        case 0:
            dist = -700.0f;
            break;
        case 1:
            dist = 0.0f;
            break;
        case 2:
            dist = -1500.0f;
            break;
        }
        em_approach_start(self, dist, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (self->area_no == get_now_areano()) {
            get_joint_wpos_em(self, 3, &pos);
            shell_se_req(self->se_0xB14, &pos, 8, self->field_0x01A);
        }
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD580 (0x140): the seated turn: the first turn sequence at motion speed 2 or 4 by `kind`. */
void em006_act6_4(struct _ENEMY_WORK* self, u8 kind)
{
    em_fx_flag2_set(self);
    if (kind == 3) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em006_turn_seq_0, 0, 0, 0);
        switch (kind) {
        case 1:
        case 3:
            em_mot_speed_set(self, 2.0f);
            break;
        case 2:
            em_mot_speed_set(self, 4.0f);
            break;
        }
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        switch (kind) {
        case 1:
        case 3:
            em_mot_speed_set(self, 2.0f);
            break;
        case 2:
            em_mot_speed_set(self, 4.0f);
            break;
        }
        if (em_turn_seq_step(self, (void*)em006_turn_seq_0) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD6C0 (0x178): the seated breath: every 32 frames an effect and a camera cue at joint 3 for 45 frames (80
 * when `kind == 1`). */
void em006_act6_E(struct _ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0;
        break;
    case 1:
        if ((self->timer_0x020 & 0x1F) == 0) {
            u32 type;

            em_camera_req(self, -1, 5);
            get_joint_wpos_em(self, 3, &pos);
            if ((self->field_0x228 & 6) != 0) {
                type = 0x54;
                pos.y = 5.0f + self->field_0x210;
            } else {
                type = 0x53;
                pos.y = 5.0f + self->field_0x20C;
            }
            eft009_spawn_at_joint(self, 3, type, 0, 1.0f);
            if (self->area_no == get_now_areano()) {
                se_req_pos_ps(self->se_0xB14, 0x27, 2, &pos);
            }
        }
        {
            s32 limit = kind == 1 ? 0x50 : 0x2D;

            if (++self->timer_0x020 >= limit) {
                em_action_finish_stand(self);
            }
        }
        break;
    }
}

/* 0x801BD838 (0x158): the seated wait: motion 8 at speed 0.5 with a short approach and the crawl sound. */
void em006_act6_F(struct _ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 0, 0);
        em_mot_speed_set(self, 1.5f);
        em_hit_window_set(self, 0, 0x21, 2);
        em_approach_start(self, 1000.0f, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0x14;
        break;
    case 1:
        if (self->area_no == get_now_areano()) {
            get_joint_wpos_em(self, 3, &pos);
            shell_se_req(self->se_0xB14, &pos, 8, self->field_0x01A);
        }
        if (self->state_0x006 == 0 && em_approach_step(self, 0, 0x40) == 1) {
            self->state_0x006 = 1;
        }
        if (--self->timer_0x020 <= 0 && self->state_0x006 == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801BD990 (0x98): selects the action-6 step by `state_sub`. */
void em006_act6_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act6_0(self, 0);
        break;
    case 1:
        em006_act6_1(self, 0);
        break;
    case 2:
        em006_act6_2(self);
        break;
    case 3:
        em006_act6_3(self, 0);
        break;
    case 4:
        em006_act6_4(self, 0);
        break;
    case 6:
        em006_act6_3(self, 1);
        break;
    case 10:
        em006_act6_4(self, 1);
        break;
    case 11:
        em006_act6_4(self, 2);
        break;
    case 14:
        em006_act6_E(self, 0);
        break;
    case 15:
        em006_act6_F(self);
        break;
    case 16:
        em006_act6_3(self, 2);
        break;
    case 17:
        em006_act6_E(self, 1);
        break;
    case 18:
        em006_act6_4(self, 1);
        break;
    case 19:
        em006_act6_1(self, 1);
        break;
    case 20:
        em006_act6_0(self, 1);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* action 7                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BDA28 (0x9C): plays motion 0x2F with hit windows 1 and 2. */
void em006_act7_0(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2F, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        em_hit_window_set_default(self, 1, 2);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BDAC4 (0x1E4): the turning swipe (motion 0x31, or 0x32 mirrored when `side == 1`), played twice, with the
 * second hit window armed once the first hit lands. */
void em006_act7_1(struct _ENEMY_WORK* self, u8 side)
{
    s32 motion;
    s32 angle;
    s32 hit0;
    s32 hit1;
    s32 hit2;

    switch (side) {
    default:
        motion = 0x31;
        angle = 0x8000;
        hit0 = 3;
        hit1 = 0x24;
        hit2 = 0x25;
        break;
    case 1:
        motion = 0x32;
        angle = -0x8000;
        hit0 = 0x17;
        hit1 = 0x26;
        hit2 = 0x27;
        break;
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, motion, 4, 0);
        em_hit_window_set(self, 0, hit0, 8);
        em_hit_window_set(self, 1, hit1, 0x18);
        break;
    case 1:
        em_turn_in_window2(self, 0.0f, 120.0f, angle);
        if (self->state_0x006 == 0 && self->field_0xA69 == 0) {
            self->state_0x006++;
            em_hit_window_set(self, 1, hit2, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, motion, 0, 0);
            em_hit_window_set(self, 0, hit0, 8);
            em_hit_window_set(self, 1, hit1, 0x18);
        }
        break;
    case 2:
        em_turn_in_window2(self, 0.0f, 120.0f, angle);
        if (self->state_0x006 == 0 && self->field_0xA69 == 0) {
            self->state_0x006++;
            em_hit_window_set(self, 1, hit2, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BDCA8 (0x250): the charge: motion 0xC8 at speed 1.3 with an approach, aiming at the target unless `fixed`,
 * cut short by a blocked front slot, then the finish motion by `kind` (0xC9, 0xD0 or 0xCA). */
void em006_act7_3(struct _ENEMY_WORK* self, u8 kind, u8 fixed)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 6, 0);
        em_mot_speed_set(self, 1.3f);
        em_stamina_add(self, -0x14);
        em_hit_window_set(self, 0, 4, 0xB);
        em_approach_start(self, kind == 0 ? 100.0f : 0.0f, 0);
        break;
    case 1:
        if (fixed == 0 && em_frame_check(self, 3, 2.0f, 80.0f) == 1) {
            em_target_pos_set(self, 0);
            em_turn_to_target(self, 0x48);
        }
        if (em_frame_check(self, 1, 90.0f, 0.0f) == 1 && em006_front_slot_ck(self) == 1) {
            em_hit_window_clear(self, 0);
            em_state_set(self, 1, 8);
        } else if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            switch (kind) {
            default:
                em_mot_set(self, 0xC9, 4, 0);
                em_hit_window_set(self, 0, 0x19, 0x10);
                break;
            case 1:
                em_mot_set(self, 0xD0, 6, 0);
                em_hit_window_set_default(self, 0, 5);
                break;
            case 2:
                em_mot_set(self, 0xCA, 6, 0);
                em_hit_window_set_default(self, 0, 5);
                break;
            }
        }
        break;
    case 2:
        if (kind == 0 && em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BDEF8 (0x9C): plays motion 0xD1 with hit windows 6 and 0x18. */
void em006_act7_5(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD1, 6, 0);
        em_hit_window_set_default(self, 0, 6);
        em_hit_window_set_default(self, 1, 0x18);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BDF94 (0x8C): plays motion 0xD2 with hit window 7. */
void em006_act7_6(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD2, 6, 0);
        em_hit_window_set_default(self, 0, 7);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE020 (0x178): the roar: motion 0xD3, the shape arm and the breath effect at frame 110, then the breath
 * effect every 8 frames over frames 116..286. */
void em006_act7_7(struct _ENEMY_WORK* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD3, 6, 0);
        em_stamina_add(self, -6);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 8, 5);
            draw_shape_arm_c1(self, 0x19, 0x78);
        }
        setVector3(&pos, 0.0f, 20.0f, 50.0f);
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x18, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 116.0f, 286.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE198 (0x1B0): the three-state seated step: motion 0xD4, then 0xD9 when `kind == 1`. */
void em006_act7_8(struct _ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD4, 6, 0);
        em_stamina_add(self, -6);
        em_hit_window_set(self, 0, 9, 8);
        em_hit_window_set(self, 1, 0x29, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xA, 0x10);
        }
        if (kind == 1) {
            if (em_frame_check(self, 0, 148.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 0xD9, 0, 0);
                em_hit_window_set(self, 0, 0x1A, 8);
                em_hit_window_set(self, 1, 0x2A, 0x18);
                return;
            }
        } else {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x1B, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE348 (0x1C0): the short charge: motion 0xC8 at speed 1.3 aiming at the target when `aim == 1`, then motion
 * 0xDA. */
void em006_act7_A(struct _ENEMY_WORK* self, u8 aim)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 6, 0);
        em_mot_speed_set(self, 1.3f);
        em_stamina_add(self, -0x14);
        em_hit_window_set(self, 0, 4, 0xB);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (aim == 1 && em_frame_check(self, 3, 2.0f, 80.0f) == 1) {
            em_target_pos_set(self, 0);
            em_turn_to_target(self, 0x48);
        }
        if (em_frame_check(self, 1, 90.0f, 0.0f) == 1 && em006_front_slot_ck(self) == 1) {
            em_hit_window_clear(self, 0);
            em_state_set(self, 1, 8);
        } else if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0xDA, 4, 0);
            em_hit_window_set(self, 0, 0x19, 0x10);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE508 (0x154): the seated en/disable step (0xCE): three window timers and the finish gate. */
void em006_act7_F(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_hit_window_set_default(self, 0, 0x16);
        em_height_set(self, em_height_base_get(self));
        em_stamina_add(self, -0xA);
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_shake_req_set(self);
        em_fx_flag2_set(self);
        break;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, 20.0f, 0.0f) == 1) {
            em_fx_flag2_set(self);
        }
        if (em_frame_check(self, 2, 24.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 0, 1.0f, 0.0f) == 1) {
            em_move_mode_set(self, 0);
            em_height_set(self, 0.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE65C (0x42C): the seated slam (motion 0xD8): the shake, the slam effects (the water set while +0x228 bits
 * 1-2 are up) and a turn by `turn` (+-30 or +-60 degrees). */
void em006_act7_10(struct _ENEMY_WORK* self, u8 turn)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD8, 0, 0);
        em_hit_window_set(self, 0, 0x1C, 8);
        em_hit_window_set(self, 1, 0x23, 0x18);
        em_height_set(self, 0.0f);
        em_stamina_add(self, -0x14);
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_shake_req_set(self);
        em_fx_flag2_set(self);
        if ((self->field_0x228 & 6) != 0) {
            self->state_0x007 = 1;
        } else {
            self->state_0x007 = 0;
        }
        break;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 2, 14.0f, 0.0f) == 1) {
            em_fx_flag2_set(self);
        }
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if (self->state_0x007 == 1) {
                eft009_spawn_at_joint(self, 3, 0x51, 0, 1.0f);
            } else {
                eft009_spawn_at_joint(self, 3, 0x27, 0, 1.0f);
            }
        }
        if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x28, 0x10);
        }
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 72.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            if (self->state_0x007 == 1) {
                eft_em_spawn(self, 0x57, 0x28, &pos, 1.0f);
            } else {
                eft_em_spawn(self, 0x1A, 0x28, &pos, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            if (self->state_0x007 == 1) {
                eft_em_spawn(self, 0x6A, 9, &pos, 1.0f);
                eft_em_spawn(self, 0x6C, 0x10, &pos, 1.0f);
            } else {
                eft_em_spawn(self, 0x69, 9, &pos, 1.0f);
                eft_em_spawn(self, 0x6B, 0x10, &pos, 1.0f);
            }
        }
        switch (turn) {
        case 1:
            em_turn_in_window(self, 0.0f, 44.0f, 0x1555);
            break;
        case 2:
            em_turn_in_window(self, 0.0f, 44.0f, -0x1555);
            break;
        case 3:
            em_turn_in_window(self, 0.0f, 44.0f, 0x2AAB);
            break;
        case 4:
            em_turn_in_window(self, 0.0f, 44.0f, -0x2AAB);
            break;
        }
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x1D, 8);
            em_hit_window_set(self, 1, 0x1E, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BEA88 (0xD0): selects the action-7 step by `state_sub`. */
void em006_act7_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act7_0(self);
        break;
    case 1:
        em006_act7_1(self, 0);
        break;
    case 2:
        em006_act7_1(self, 1);
        break;
    case 3:
        em006_act7_3(self, 0, 0);
        break;
    case 4:
        em006_act7_3(self, 1, 0);
        break;
    case 5:
        em006_act7_5(self);
        break;
    case 6:
        em006_act7_6(self);
        break;
    case 7:
        em006_act7_7(self);
        break;
    case 8:
        em006_act7_8(self, 0);
        break;
    case 9:
        em006_act7_8(self, 1);
        break;
    case 10:
        em006_act7_A(self, 0);
        break;
    case 11:
        em006_act7_A(self, 1);
        break;
    case 12:
        em006_act7_3(self, 1, 1);
        break;
    case 13:
        em006_act7_3(self, 2, 0);
        break;
    case 14:
        em006_act7_3(self, 2, 1);
        break;
    case 15:
        em006_act7_F(self);
        break;
    case 16:
        em006_act7_10(self, 0);
        break;
    case 17:
        em006_act7_10(self, 1);
        break;
    case 18:
        em006_act7_10(self, 2);
        break;
    case 19:
        em006_act7_10(self, 3);
        break;
    case 20:
        em006_act7_10(self, 4);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* actions 10-13                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

/* The per-motion sound programs action 10 plays (`EmSeEntry` lists and their records, offsets and loop words; each
 * is named for the first sub-state that plays it). */
EmSeRecord em006_se_rec_18 = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_18},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_17 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_1C_a = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_1C_b = {104, 0, 8, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em006_se_rec_1C_c = {106, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_1C[5] = {
    {0x00, {0, 0, 0}, &em006_se_rec_1C_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em006_se_rec_1C_b},
    {0x00, {0, 0, 0}, &em006_se_rec_1C_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_1D_a = {103, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_1D_b = {105, 0, 8, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em006_se_rec_1D_c = {107, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_1D[5] = {
    {0x00, {0, 0, 0}, &em006_se_rec_1D_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em006_se_rec_1D_b},
    {0x00, {0, 0, 0}, &em006_se_rec_1D_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_1A = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_1A[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_1A},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_23_a = {112, 0, 2, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em006_vec_pair_0.pos};

EmSeOffset em006_se_ofs_0 = {-50.0f, -10.0f};

EmSeRecord em006_se_rec_23_b = {0, 0, 0, 0, 0, {0, 2, 1, 0}, &em006_se_ofs_0};

EmSeRecord em006_se_rec_23_c = {113, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_23_d = {105, 0, 8, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em006_se_rec_23_e = {107, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_23[10] = {
    {0x0B, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em006_se_rec_23_a},
    {0x01, {0, 0, 0}, &em006_se_rec_23_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_23_c},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em006_se_rec_23_d},
    {0x00, {0, 0, 0}, &em006_se_rec_23_e},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_7A = {116, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_7B = {24, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_7C = {115, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_7C},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_8D = {124, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_7E_a = {103, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_7E_b = {105, 0, 8, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em006_se_rec_7E_c = {107, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_7E[4] = {
    {0x00, {0, 0, 0}, &em006_se_rec_7E_a},
    {0x00, {0, 0, 0}, &em006_se_rec_7E_b},
    {0x00, {0, 0, 0}, &em006_se_rec_7E_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_7F_a = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_7F_b = {104, 0, 8, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em006_se_rec_7F_c = {106, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_7F[4] = {
    {0x00, {0, 0, 0}, &em006_se_rec_7F_a},
    {0x00, {0, 0, 0}, &em006_se_rec_7F_b},
    {0x00, {0, 0, 0}, &em006_se_rec_7F_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_A8 = {108, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_9F = {25, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_58 = {213, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_58[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_58},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_B6 = {127, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_B6},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_B7 = {128, 0, 0, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_B7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_B8_a = {67, 0, 4, 4, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em006_vec_pair_1.pos};

EmSeOffset em006_se_ofs_1 = {-5.0f, 100.0f};

EmSeRecord em006_se_rec_B8_b = {26, 0, 6, 0, 0, {0, 3, 4, 0}, &em006_se_ofs_1};

EmSeRecord em006_se_rec_B8_c = {46, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_B8_d = {65, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_B8[8] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_B8_a},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1F4)},
    {0x00, {0, 0, 0}, &em006_se_rec_B8_b},
    {0x00, {0, 0, 0}, &em006_se_rec_B8_c},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_B8_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_B9 = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_B9},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_BA = {130, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em006_se_rec_BA},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_BB = {130, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_BB},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_BF_a = {108, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em006_se_rec_BF_b = {115, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_BF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em006_se_rec_BF_a},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_BF_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSePosRec em006_se_pos_CA_a = {60.0f, 100.0f, 0, 16384};

EmSePosRec em006_se_pos_CA_b = {140.0f, 220.0f, 0, 16384};

EmSeRecord em006_se_rec_CA_a = {120, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em006_se_rec_CA_b = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_CA[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x64)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em006_se_pos_CA_a},
    {0x00, {0, 0, 0}, &em006_se_rec_CA_a},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em006_se_pos_CA_b},
    {0x01, {0, 0, 0}, &em006_se_rec_CA_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_34 = {207, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

u32 em006_se_loop_0 = 0x0AE40000;

EmSeEntry em006_se_seq_34[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_34},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em006_se_loop_0},
};

EmSeRecord em006_se_rec_B2 = {207, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

u32 em006_se_loop_1 = 0x0AE40000;

EmSeEntry em006_se_seq_B2[3] = {
    {0x1C, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em006_se_rec_B2},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em006_se_loop_1},
};

EmSeRecord em006_se_rec_E4 = {128, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em006_se_seq_E4[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_E4},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_E5_a = {67, 0, 4, 4, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em006_vec_pair_2.pos};

EmSeOffset em006_se_ofs_2 = {-5.0f, 100.0f};

EmSeRecord em006_se_rec_E5_b = {26, 0, 6, 0, 0, {0, 3, 4, 0}, &em006_se_ofs_2};

EmSeRecord em006_se_rec_E5_c = {46, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_E5_d = {65, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_E5[8] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_E5_a},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1F4)},
    {0x00, {0, 0, 0}, &em006_se_rec_E5_b},
    {0x00, {0, 0, 0}, &em006_se_rec_E5_c},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_E5_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_E6 = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_seq_E6[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_E6},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_E7 = {130, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_E7[2] = {
    {0x01, {0, 0, 0}, &em006_se_rec_E7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_E8 = {130, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em006_se_seq_E8[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_E8},
    {0xFE, {0, 0, 0}, 0},
};

/* 0x801BEB58 (0x39C): the sound program of the current sub-state: plays its SE sequence table, or finishes the
 * action for a sub-state with none. */
void em006_act10_se_play(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em006_se_seq_17, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em006_se_seq_18, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em006_se_seq_18, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em006_se_seq_1A, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em006_se_seq_1A, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em006_se_seq_1C, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em006_se_seq_1D, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em006_se_seq_18, 0, 0x1E);
        break;
    case 0x58:
        em_se_tbl_play(self, em006_se_seq_58, 0, 0x58);
        break;
    case 0x23:
        em_se_tbl_play(self, em006_se_seq_23, 1, 0x23);
        break;
    case 0x64:
        em_se_tbl_play(self, em006_se_seq_23, 1, 0x64);
        break;
    case 0x34:
        em_se_tbl_play(self, em006_se_seq_34, 0, 0x34);
        break;
    case 0x75:
        em_se_tbl_play(self, em006_se_seq_34, 0, 0x75);
        break;
    case 0x7A:
        em_se_tbl_play(self, em006_se_seq_7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em006_se_seq_7B, 0, 0x7B);
        break;
    case 0x7C:
        em_se_tbl_play(self, em006_se_seq_7C, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(self, em006_se_seq_8D, 0, 0x8D);
        break;
    case 0x7E:
        em_se_tbl_play(self, em006_se_seq_7E, 0, 0x7E);
        break;
    case 0x7F:
        em_se_tbl_play(self, em006_se_seq_7F, 0, 0x7F);
        break;
    case 0x8E:
        em_se_tbl_play(self, em006_se_seq_8D, 0, 0x8E);
        break;
    case 0x78:
        em_se_tbl_play(self, em006_se_seq_18, 0, 0x78);
        break;
    case 0x84:
        em_se_tbl_play(self, em006_se_seq_23, 1, 0x84);
        break;
    case 0x97:
        em_se_tbl_play(self, em006_se_seq_34, 0, 0x97);
        break;
    case 0x9F:
        em_se_tbl_play(self, em006_se_seq_9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em006_se_seq_9F, 0, 0xA0);
        break;
    case 0xA8:
        em_se_tbl_play(self, em006_se_seq_A8, 0, 0xA8);
        break;
    case 0xA9:
        em_se_tbl_play(self, em006_se_seq_23, 1, 0xA9);
        break;
    case 0xB2:
        em_se_tbl_play(self, em006_se_seq_B2, 0, 0xB2);
        break;
    case 0xB6:
        em_se_tbl_play(self, em006_se_seq_B6, 0, 0xB6);
        break;
    case 0xB7:
        em_se_tbl_play(self, em006_se_seq_B7, 0, 0xB7);
        break;
    case 0xB8:
        em_se_tbl_play(self, em006_se_seq_B8, 1, 0xB8);
        break;
    case 0xB9:
        em_se_tbl_play(self, em006_se_seq_B9, 0, 0xB9);
        break;
    case 0xBA:
        em_se_tbl_play(self, em006_se_seq_BA, 0, 0xBA);
        break;
    case 0xBB:
        em_se_tbl_play(self, em006_se_seq_BB, 0, 0xBB);
        break;
    case 0xBC:
        em_se_tbl_play(self, em006_se_seq_BB, 0, 0xBC);
        break;
    case 0xBF:
        em_se_tbl_play(self, em006_se_seq_BF, 0, 0xBF);
        break;
    case 0xC1:
        em_se_tbl_play(self, em006_se_seq_8D, 0, 0xC1);
        break;
    case 0xCA:
        em_se_tbl_play(self, em006_se_seq_CA, 0, 0xCA);
        break;
    case 0xE4:
        em_se_tbl_play(self, em006_se_seq_E4, 0, 0xE4);
        break;
    case 0xE5:
        em_se_tbl_play(self, em006_se_seq_E5, 1, 0xE5);
        break;
    case 0xE6:
        em_se_tbl_play(self, em006_se_seq_E6, 0, 0xE6);
        break;
    case 0xE7:
        em_se_tbl_play(self, em006_se_seq_E7, 0, 0xE7);
        break;
    case 0xE8:
        em_se_tbl_play(self, em006_se_seq_E8, 0, 0xE8);
        break;
    case 0xE9:
        em_se_tbl_play(self, em006_se_seq_E8, 0, 0xE9);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* The alternate sound programs action 11 plays. */
EmSeRecord em006_se_rec_alt_00 = {125, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_00[2] = {
    {0x00, {0, 0, 0}, &em006_se_rec_alt_00},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_alt_05_a = {112, 0, 2, 0, 0, {0, 2, 1, 0}, &em006_se_ofs_0};

EmSeRecord em006_se_rec_alt_05_b = {113, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_alt_05_c = {114, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_05[7] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_alt_05_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em006_se_rec_alt_05_b},
    {0x00, {0, 0, 0}, &em006_se_rec_alt_05_c},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_alt_0B_a = {207, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_alt_0B_b = {130, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_0B[4] = {
    {0x00, {0, 0, 0}, &em006_se_rec_alt_0B_a},
    {0x00, {0, 0, 0}, &em006_se_rec_alt_0B_b},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_alt_1A_a = {116, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em006_se_rec_alt_1A_b = {24, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em006_se_rec_alt_1A_a},
    {0x00, {0, 0, 0}, &em006_se_rec_alt_1A_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_alt_1C = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_1C[3] = {
    {0x00, {0, 0, 0}, &em006_se_rec_alt_1C},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em006_se_rec_alt_0F = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em006_se_alt_seq_0F[3] = {
    {0x00, {0, 0, 0}, &em006_se_rec_alt_0F},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801BEEF4 (0xE0): the alternate sound program of the current sub-state (the default plays sub-state 0's
 * table). */
void em006_act11_se_play(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x0:
        em_se_tbl_play_alt(self, em006_se_alt_seq_00, 0, 0);
        break;
    case 0x5:
        em_se_tbl_play_alt(self, em006_se_alt_seq_05, 1, 5);
        break;
    case 0xB:
        em_se_tbl_play_alt(self, em006_se_alt_seq_0B, 0, 0xB);
        break;
    case 0xF:
        em_se_tbl_play_alt(self, em006_se_alt_seq_0F, 0, 0xF);
        break;
    case 0x1C:
        em_se_tbl_play_alt(self, em006_se_alt_seq_1C, 0, 0x1C);
        break;
    case 0x1A:
        em_se_tbl_play_alt(self, em006_se_alt_seq_1A, 0, 0x1A);
        break;
    case 0x34:
        em_se_tbl_play_alt(self, em006_se_alt_seq_0F, 0, 0x34);
        break;
    default:
        em_se_tbl_play_alt(self, em006_se_alt_seq_00, 0, 0);
        break;
    }
}

/* 0x801BEFD4 (0x4): forwards to the action-7 seated step 0xF. */
void em006_act12_0(struct _ENEMY_WORK* self)
{
    em006_act7_F(self);
}

/* 0x801BEFD8 (0x14): runs `em006_act12_0` while the action-12 sub-state is 0. */
void em006_act12_dispatch(struct _ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em006_act12_0(self);
    }
}

/* 0x801BEFEC (0x8E4): the scripted entrance: the demo-timed slams at three positions, the climb effects and the
 * closing roar. */
void em006_act13_0(struct _ENEMY_WORK* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x2EE) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0xD8, 0, 0);
            em_demo_pos_set(self, 2288.7334f, -202.2364f, -3699.9988f);
            em_demo_rot_set(self, 0.0f, -15.0f, 0.0f);
            em_shake_req_set(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x27, 0, 1.0f);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x69, 9, &pos, 1.0f);
            eft_em_spawn(self, 0x6B, 0x10, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 60.0f, 0.0f) == 1 || em_frame_check(self, 0, 72.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1A, 0x28, &pos, 1.0f);
        }
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_demo_time_ck(0x302) == 1) {
            self->state++;
            em_demo_reset(self, 0);
            em_demo_pos_set(self, 1967.0365f, -1405.2365f, -3238.8828f);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x27, 0, 1.0f);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x69, 9, &pos, 1.0f);
            eft_em_spawn(self, 0x6B, 0x10, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 60.0f, 0.0f) == 1 || em_frame_check(self, 0, 72.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1A, 0x28, &pos, 1.0f);
        }
        if (em_demo_time_ck(0x30C) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0xD8, 0, 0);
            em_demo_pos_set(self, 2134.7334f, -202.2364f, -3878.9988f);
            em_shake_req_set(self);
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x27, 0, 1.0f);
        }
        if (em_frame_check(self, 0, 60.0f, 0.0f) == 1 || em_frame_check(self, 0, 72.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1A, 0x28, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x69, 9, &pos, 1.0f);
            eft_em_spawn(self, 0x6B, 0x10, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 60.0f, 100.0f) == 1 && em_demo_frame_get() % 3 == 0) {
            eft009_spawn_at_joint(self, 0x28, 0x2B, 0, 1.0f);
            eft009_spawn_at_joint(self, 0x20, 0x2B, 0, 1.0f);
        }
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 0x21, &pos);
            pos.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&pos, self->area_no, 3, self->field_0x1C0, 0.8f * get_em_chg_scale(self));
        }
        if (em_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &pos);
            pos.y = 5.0f + self->field_0x20C;
            eft_spawn_type11(self, &pos, 0, 1.0f);
        }
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_demo_time_ck(0x3DA) == 1) {
            self->state++;
            em_demo_pos_set(self, 1072.77f, 103.0f, 179.3f);
            em_demo_rot_set(self, 0.0f, -15.0f, 0.0f);
        }
        break;
    case 5:
        if (em_demo_time_ck(0x408) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
        }
        break;
    case 6:
        if (em_demo_time_ck(0x496) == 1) {
            self->state++;
            em_mot_set(self, 0xD3, 0x14, 0xC);
            self->timer_0x020 = 0;
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            draw_shape_arm_c1(self, 0x19, 0x78);
        }
        setVector3(&pos, 0.0f, 20.0f, 50.0f);
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x18, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 116.0f, 286.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_demo_time_ck(0x56A) == 1) {
            self->state++;
            em_mot_set(self, 0xD3, 0, 0x78);
            self->timer_0x020 = 0;
        }
        break;
    case 8:
        setVector3(&pos, 0.0f, 20.0f, 50.0f);
        if (em_frame_check(self, 3, 116.0f, 286.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_demo_time_ck(0x5FE) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
        }
        break;
    }
}

/* 0x801BF8D0 (0xA4): the entrance's resting pose: motion 2 at the demo position. */
void em006_act13_1(struct _ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, 1072.7655f, -202.2364f, 179.2982f);
        em_demo_rot_set(self, 0.0f, -15.0f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BF974 (0x24): selects the action-13 step by `state_sub`. */
void em006_act13_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em006_act13_0(self);
        break;
    case 1:
        em006_act13_1(self);
        break;
    }
}

/* 0x801BF998 (0xE8): the action dispatcher of the program table, then the action-7 sub-state-7 latch at +0x328. */
void em006_action_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->action) {
    case 0:
        em006_act0_dispatch(self);
        break;
    case 1:
        em006_act1_dispatch(self);
        break;
    case 2:
        em006_act2_dispatch(self);
        break;
    case 3:
        em006_act3_dispatch(self);
        break;
    case 6:
        em006_act6_dispatch(self);
        break;
    case 7:
        em006_act7_dispatch(self);
        break;
    case 10:
        em006_act10_se_play(self);
        break;
    case 11:
        em006_act11_se_play(self);
        break;
    case 12:
        em006_act12_dispatch(self);
        break;
    case 13:
        em006_act13_dispatch(self);
        break;
    }
    if (em_act_ck_c1(self, 7, 7) == 1) {
        if (self->init_0x328.slots_0x328[0] == 0) {
            self->init_0x328.slots_0x328[0] = 1;
        }
    } else if (self->action != 0 && self->init_0x328.slots_0x328[0] == 1) {
        self->init_0x328.slots_0x328[0] = 0;
    }
}

/* 0x801BFA80 (0x98): every 20 frames in the alternate mode, spawns the type-10 effect 0xE at joint 0x18. */
void em006_alt_mode_effect(struct _ENEMY_WORK* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) == 1 && system_w.field_0x0c % 20 == 0) {
        setVector3(&pos, 0.0f, -20.0f, 100.0f);
        eft_spawn_type10(self, 0xE, 0x18, &pos, 1.0f);
    }
}

/* 0x801BFB18 (0x19C): the effect router: kind 0 a joint effect (at the ground under the work for joint 0xFF), kind
 * 1 an area effect at the joint, kind 2 the type-11 effect at the work's ground position. */
void em006_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, u32 joint, s32 delta, f32 scale)
{
    MTX34 mtx;
    VEC3 pos;
    _CP_VECTOR rot;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    switch (kind) {
    case 0:
        if (joint == 0xFF) {
            copyVec3(&pos, &self->pos);
            if ((self->field_0x228 & 6) != 0) {
                pos.y = 5.0f + self->field_0x210;
            } else {
                pos.y = 5.0f + self->field_0x20C;
            }
            eft009_set_pos(type, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
            eft_rot_vec_copy(&rot, (_CP_VECTOR*)&self->field_0x1BC);
            eft009_spawn_at_joint(self, joint, type, delta, scale);
        } else {
            eft009_spawn_at_joint(self, joint, type, delta, scale);
        }
        break;
    case 1:
        get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, type, self->field_0x1C0 + delta, scale);
        break;
    case 2:
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        eft_spawn_type11(self, &pos, type, scale);
        break;
    }
}

/* 0x801BFCB4 (0x2854): the per-motion events: effects, camera cues and hit windows keyed by the
 * current motion and its frame (the water set while +0x228 bits 1-2 are up). */
void em006_motion_events(struct _ENEMY_WORK* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    em006_alt_mode_effect(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x2A, 0x8000, 1.0f);
                em006_effect_spawn(self, 0, 0xE, 0x2A, 0, 0.7f);
            } else {
                em006_effect_spawn(self, 1, 6, 0x2A, self->field_0x1C0, 0.4f);
            }
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x2A, 0x8000, 1.3f);
                em006_effect_spawn(self, 0, 0xE, 0x2A, 0, 0.9f);
            } else {
                em006_effect_spawn(self, 1, 6, 0x2A, self->field_0x1C0, 0.7f);
            }
        }
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x2A, 0x8000, 1.6f);
                em006_effect_spawn(self, 0, 0xE, 0x2A, 0, 1.1f);
                return;
            }
            em006_effect_spawn(self, 1, 6, 0x2A, self->field_0x1C0, 1.0f);
            return;
        }
        break;
    case 0x5:
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_camera_req(self, 0x22, 7);
            return;
        }
        break;
    case 0x6:
        if (self->field_0x1E2 != 4) {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1)) {
                em_hit_window_set_default(self, 0, 0xD);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xE, 0x24, 0, 1.0f);
                } else {
                    em006_effect_spawn(self, 0, 2, 0x24, 0, 1.4f);
                }
                em_camera_req(self, 0x22, 7);
            }
            if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
                em_hit_window_set_default(self, 1, 0xE);
            }
            if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xE, 0x2C, 0, 1.0f);
                } else {
                    em006_effect_spawn(self, 0, 2, 0x2C, 0, 1.4f);
                }
                em_camera_req(self, 0x2A, 7);
            }
            if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 116.0f, 0.0f) == 1)) {
                em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.0f);
                return;
            }
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xE, 0x2C, 0, 1.4f);
            } else {
                em006_effect_spawn(self, 0, 2, 0x2C, 0, 1.4f);
            }
            em_camera_req(self, 0x2A, 7);
        }
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 2, 0x24, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xE, 0x24, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 2, 0x24, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1 && ((self->field_0x228 & 6) != 0)) {
            em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.0f);
            return;
        }
        break;
    case 0x8:
        if (self->field_0x1E2 == 4) {
            if ((system_w.field_0x0c & 3) == 0) {
                if ((self->field_0x228 & 6) != 0) {
                    setVector3(&pos, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x57, 3, &pos, 1.0f);
                } else {
                    em006_effect_spawn(self, 0, 0x94, 0xFF, 0, 1.0f);
                }
            }
            if ((system_w.field_0x0c & 0xF) == 0) {
                em_camera_req(self, -1, 0);
                return;
            }
        } else {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xD, 0x2C, 0, 1.0f);
                } else {
                    em006_effect_spawn(self, 0, 0, 0x2A, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xF, 0x22, 0, 1.0f);
                }
                em_camera_req(self, 0x22, 0);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xD, 0x24, 0, 1.0f);
                } else {
                    em006_effect_spawn(self, 0, 0, 0x22, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em006_effect_spawn(self, 0, 0xF, 0x2A, 0, 1.0f);
                }
                em_camera_req(self, 0x2A, 0);
                return;
            }
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 4, 0x2A, 0, 1.6f);
            }
        }
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
                em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.0f);
                return;
            }
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xF, 8);
            em_hit_window_set(self, 1, 0x10, 0x10);
            return;
        }
        break;
    case 0xD:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x11, 8);
            em_hit_window_set(self, 1, 0x12, 0x10);
            return;
        }
        break;
    case 0x1A:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1)) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            em_ground_impact_set_c1(self, 2, 1, 1.0f, &pos);
            return;
        }
        break;
    case 0x1D:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            em_ground_impact_set_c1(self, 2, 1, 1.0f, &pos);
            return;
        }
        break;
    case 0x30:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1)) {
                em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xD, 0x2C, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xD, 0x22, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 57.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0x13, 0x22, 0, 1.2f);
            }
        } else {
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 2, 0x22, 0, 1.4f);
            }
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 2, 0x2A, 0, 1.5f);
            }
        }
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1)) {
            em_camera_req(self, 0x2A, 7);
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em_camera_req(self, 0x22, 7);
            return;
        }
        break;
    case 0x31:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0x13, 0x21, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 6, 0x22, 0x8000, 1.5f);
            }
            em_camera_req(self, 0x22, 7);
            return;
        }
        break;
    case 0x32:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 6, 0x2A, 0x8000, 1.5f);
            }
            em_camera_req(self, 0x2A, 7);
        }
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
            em006_effect_spawn(self, 0, 0x13, 0x29, 0, 1.0f);
            return;
        }
        break;
    case 0x41:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x11, 3, 0, 0.8f);
            } else {
                em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 1.0f);
            }
            em_ground_impact_set_c1(self, 1, 0, 1.0f, 0);
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x43:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x12, 3, 0, 2.4f);
            } else {
                em006_effect_spawn(self, 0, 5, 3, 0, 1.0f);
            }
            em_ground_impact_set_c1(self, 1, 0, 1.0f, 0);
            return;
        }
        break;
    case 0x65:
        if (((self->field_0x228 & 6) == 0) && (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1)) {
            em006_effect_spawn(self, 1, 6, 0x2A, self->field_0x1C0, 0.9f);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.5f);
            } else {
                em006_effect_spawn(self, 1, 6, 0x2A, self->field_0x1C0, 0.8f);
            }
        }
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1)) {
            em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 6, 0x22, self->field_0x1C0, 0.8f);
            }
        }
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1)) {
            em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            return;
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 0, 3, self->field_0x1C0 + 0x4000, 1.5f);
            }
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 0, 3, self->field_0x1C0 + 0xC000, 1.5f);
            }
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x6C:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x2C, 0, 1.5f);
                em006_effect_spawn(self, 0, 0xF, 0x2C, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 6, 0x2A, 0, 0.9f);
            }
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
                em006_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
                return;
            }
            em006_effect_spawn(self, 1, 6, 0x22, 0, 0.8f);
            return;
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 5, 3, self->field_0x1C0, 1.0f);
            }
            em_camera_req(self, -1, 1);
            return;
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, 234.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 0.7f);
            }
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x78:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xD, 0x22, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xF, 0x2A, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0xD, 0x2A, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 0x11, 0x14, 0, 1.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 6, 0x22, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 0, 6, 0x2A, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x10, 3, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 1, 1, 3, self->field_0x1C0, 1.0f);
            }
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x14, 3, 0, 2.0f);
            } else {
                em006_effect_spawn(self, 1, 2, 3, self->field_0x1C0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            return;
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            em_camera_req(self, 0x18, 7);
            return;
        }
        break;
    case 0xC8:
        if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_camera_req(self, 0x22, 0);
        }
        if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2A, 0);
        }
        if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x2A, 0x8000, 1.0f);
                em006_effect_spawn(self, 0, 0xD, 0x22, 0, 1.0f);
                em006_effect_spawn(self, 0, 0xF, 0x22, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 0, 0x2A, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1 && ((self->field_0x228 & 6) != 0)) {
            em006_effect_spawn(self, 0, 0xD, 0x2A, 0, 1.0f);
            em006_effect_spawn(self, 0, 0xF, 0x2A, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x22, 0x8000, 1.0f);
                return;
            }
            em006_effect_spawn(self, 0, 0, 0x22, 0, 1.0f);
            return;
        }
        break;
    case 0xC9:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft_em_spawn(self, 0x76, 0x24, &pos, 1.0f);
            } else {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft_em_spawn(self, 0x75, 0x24, &pos, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft_em_spawn(self, 0x76, 0x2C, &pos, 1.0f);
            } else {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft_em_spawn(self, 0x75, 0x2C, &pos, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 0);
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.6f);
                return;
            }
            em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 0.6f);
            return;
        }
        break;
    case 0xCA:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.6f);
                return;
            }
            em006_effect_spawn(self, 2, 0, 3, 0, 0.6f);
            return;
        }
        break;
    case 0xCC:
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2A, 7);
        }
        if ((em_flags836_ck(self, 1) == 0) && (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
            em006_effect_spawn(self, 0, 0x37, 0x32, 0, 1.0f);
            em_camera_req(self, 0x32, 7);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x13, 0x2A, 0x8000, 1.0f);
                return;
            }
            em006_effect_spawn(self, 0, 0, 0x2A, 0, 1.0f);
            return;
        }
        break;
    case 0xCD:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x22, 8, &pos, 1.0f);
            em_camera_req(self, -1, 2);
        }
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0x22, 0x18, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x22, 0xF, &pos, 1.0f);
            return;
        }
        break;
    case 0xCE:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 200.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x18, 0x52, 0, 1.0f, &pos);
            } else {
                eft009_spawn_at_pos(self, 0x18, 0x28, 0, 1.0f, &pos);
            }
            em_camera_req(self, -1, 5);
        }
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.6f);
                return;
            }
            em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 0.6f);
            return;
        }
        break;
    case 0xCF:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 5);
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.8f);
                return;
            }
            em006_effect_spawn(self, 2, 0, 3, 0, 0.8f);
            return;
        }
        break;
    case 0xD0:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.6f);
                return;
            }
            em006_effect_spawn(self, 2, 0, 3, 0, 0.6f);
            return;
        }
        break;
    case 0xD1:
        if ((em_flags836_ck(self, 1) == 0) && (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1 || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1))) {
            em_camera_req(self, 0x32, 1);
            em006_effect_spawn(self, 0, 0x37, 0x32, 0, 1.0f);
            return;
        }
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x17, 0x2A, 0, 3.5f);
                em006_effect_spawn(self, 0, 0x19, 0x2A, 0, 3.5f);
                return;
            }
            em006_effect_spawn(self, 0, 2, 0x2A, 0, 3.0f);
            return;
        }
        break;
    case 0xD4:
        if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2A, 1);
        }
        if (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x2A, 0, 1.5f);
                em006_effect_spawn(self, 0, 0xE, 0x2A, 0, 1.5f);
                return;
            }
            em006_effect_spawn(self, 0, 6, 0x2A, 0, 1.5f);
            return;
        }
        break;
    case 0xD6:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0x12, 0x2A, 0, 1.0f);
            } else {
                em006_effect_spawn(self, 0, 2, 0x2A, 0, 1.5f);
            }
        }
        if (em_after_frame_check(self, 1, 100.0f, 0.0f) == 1 && ((system_w.field_0x0c & 0xF) == 0)) {
            setVector3(&pos, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 0xB5, 0x18, &pos, 1.0f);
            return;
        }
        break;
    case 0xD7:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 0xB5, 0x18, &pos, 1.5f);
        }
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x2A, 0, 1.2f);
                em006_effect_spawn(self, 0, 0xE, 0x2A, 0, 1.2f);
            } else {
                em006_effect_spawn(self, 0, 2, 0x2A, 0, 1.2f);
            }
        }
        if (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x22, 0, 1.2f);
                em006_effect_spawn(self, 0, 0xE, 0x22, 0, 1.2f);
                return;
            }
            em006_effect_spawn(self, 0, 2, 0x22, 0, 1.2f);
            return;
        }
        break;
    case 0xD8:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 5);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 0);
        }
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 2, 2, 3, 0, 0.6f);
                return;
            }
            em006_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 0.6f);
            return;
        }
        break;
    case 0xD9:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_camera_req(self, 0x22, 1);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x22, 0, 1.5f);
                em006_effect_spawn(self, 0, 0xE, 0x22, 0, 1.5f);
                return;
            }
            em006_effect_spawn(self, 0, 6, 0x22, 0, 1.5f);
            return;
        }
        break;
    case 0xDA:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em006_effect_spawn(self, 0, 0xC, 0x22, 0, 1.5f);
                return;
            }
            em006_effect_spawn(self, 0, 6, 0x22, 0, 1.5f);
            return;
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x22, 8, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x59, 3, &pos, 1.0f);
            } else {
                eft009_spawn_at_joint(self, 3, 0x6A, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0x22, 0xF, 0, 1.0f);
        }
        if (em_after_frame_check(self, 3, 100.0f, 146.0f) == 1 && ((system_w.field_0x0c & 0xF) == 0)) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x59, 3, &pos, 1.0f);
            } else {
                eft009_spawn_at_joint(self, 3, 0x6A, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em_ground_impact_set_c1(self, 1, 1, 1.0f, 0);
            em_camera_req(self, -1, 4);
        }
        break;
    }
}

/* 0x801C2508 (0x4): the empty TEV colour slot. */
void em006_tev_color_update(void)
{
}

/* 0x801C250C (0x28): `(part == 0) && the +0x328 latch is clear` (the target reads the byte with `lbz`). */
s32 em006_part_damage_ck(struct _ENEMY_WORK* self, u8 part)
{
    if (part == 0 && self->init_0x328.slots_0x328[0] == 0) {
        return 1;
    }
    return 0;
}

/* 0x801C2534 (0x124): the area-start slot: mode 4, the (0xC, 0) action pair, and the start point of the work's
 * area. */
void em006_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub)
{
    em_move_mode_set(self, 4);
    *action = 0xC;
    *sub = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 2:
        switch (self->area_no) {
        case 4:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 1), &self->pos);
            break;
        case 6:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 3), &self->pos);
            break;
        }
        break;
    case 8:
        switch (self->area_no) {
        case 1:
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        }
        break;
    case 9:
        switch (self->area_no) {
        case 0:
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        }
        break;
    }
}

/* 0x801C2658 (0x4C): `(part == 0) && part 0's damage level is 1 or 2`. */
s32 em006_part_level_ck(struct _ENEMY_WORK* self, u8 part)
{
    if (part == 0 && (u8)(em_parts_damage_level_get(self, 0) + 0xFF) <= 1) {
        return 1;
    }
    return 0;
}

/* 0x801C26A4 (0x40): true in mode 4 of the enemy-control state machine while `em_alt_mode_ck` reports not-yet-armed. */
s32 em006_alt_wait_ck(struct _ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 4 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* 0x801C26E4 (0x218): the action selection of map kind 2: the stun move, the part-break reply, the table pick, the
 * pack call when another live team-6 work shares the area, then the common selectors. */
u32 em006_action_select(struct _ENEMY_WORK* self, u32 arg)
{
    u8 kind = stage_map_kind_get(self->field_0x1E0);
    u32 hit;
    u32 probe;

    switch (kind) {
    case 2:
        break;
    default:
        return 0;
    }
    if (em_area_stun_move_ck(self) == 1) {
        return 1;
    }
    hit = 0;
    probe = (s32)kind == 2 ? 10 : 0xFF;
    if (probe != 0xFF) {
        switch (em_area_break_ck_c2(self, probe)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        if (em_pack_area_pick_c1(self, 0, (s32)kind == 2 ? (u8)7 : (u8)0xFF, (u16)arg, 0, em006_pick_tbl) == 1) {
            return 1;
        }
    }
    if (hit == 0 && self->value_0x452 >= 900) {
        u32 found = 0;
        struct _ENEMY_WORK* work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
        u16 max = get_move_work_max_c1(3);
        u16 i;
        for (i = 0; i < max; i++, work++) {
            if (self->active != 0 && self->group != i && em_work_die_ck_c1(work) != 1 && work->team == 6 &&
                work->area_no == self->area_no) {
                found = 1;
                break;
            }
        }
        if (found == 1 && em_roll_cooldown_ck_c1(self, (u16)arg, 0x32, &self->timer_0x32A) == 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 2;
            self->field_0x1FF = 0xFF;
            return 1;
        }
    }
    if (em_area_roam_pick(self, (u16)arg) == 1) {
        return 1;
    }
    return (em_area_leave_ck(self) - 1) == 0;
}

}

/* The unit's `.bss`: the three two-vector records MWCC's `__sinit` seeds (each a point and its step). */
Em006VecPair em006_vec_pair_0(10.0f, -0.4f); /* 0x806A7AD0 */
Em006VecPair em006_vec_pair_1(70.0f, -0.6f); /* 0x806A7AE8 */
Em006VecPair em006_vec_pair_2(70.0f, -0.6f); /* 0x806A7B00 */
