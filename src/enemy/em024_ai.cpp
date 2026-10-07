/*
 * enemy/em024_ai.cpp - enemy 024's AI: the program table `Em024ProgTbl` (init, an empty slot, the event
 *   notifier, the frame update, the action dispatcher, the motion-event switch and the TEV colour update), the
 *   sub-state dispatchers of actions 0..4, 7, 10, 11 and 13 and their `em024_act<N>_sub<M>` steps.
 * RANGE. .text 0x8034F138-0x80358624 (69 functions); extab 0x800171A4-0x8001736C, extabindex
 *   0x800368DC-0x80036B88, .rodata 0x80570880-0x805709D0 (the six turn tables, `const` data here), .data
 *   0x805EBBE0-0x805ED0C0 (`em024_prog_tbl` first), .sdata 0x80793330-0x80793338, .sdata2 0x8079B3C8-0x8079B640.
 *   Left seam `Pl/pl_act_class3.cpp`, right seam `ef/eft052.cpp`; one TU: the pool float `lbl_8079B3CC` is loaded
 *   by 28 functions across the range and by nothing outside it.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`, `#pragma pool_data off` and `#pragma fp_contract off`,
 *   each measured on the whole unit: without peephole off 30 rows drop (58 to 38 matched functions), without
 *   pool_data off 2 (`em024_act10_dispatch`), without fp_contract off 4 (`em024_front_ray_hit_ck`,
 *   `em024_act7_sub1`, `em024_act7_sub5`, `em024_act7_scatter_pos`).
 * NAMES. `em024` follows the runtime dump's `em024_prog_tbl` (the caller of `em024_action11_state5_ck` tests enemy
 *   id 0x18); the `_ai` suffix, every function name, the `Em024ProgTbl`/`EmSeRecord`/`EmSePos`/`EmSeEntry` field
 *   names (`EmSePos::channel_0x08`/`angle_0x0A` and `EmSeRecord::length_0x08` are thin), the `em024_0x328` view's
 *   `tev_color_*` roles and the sub-state numbers' meaning are GUESSes.  `ShellSetFuncs::method_0x74`,
 *   `method_0x84` and `method_0x8C` (the table behind `stage/stg_w.cpp`'s `shell_set_func_ptr`) take the call
 *   sites' shapes, a GUESS.
 * RESIDUALS. No row unwritten.  11 partial rows:
 *  - `em024_act4_sub0`, `em024_act7_sub1`, `em024_act7_sub2`, `em024_act7_sub5`, `em024_act7_sub13`,
 *    `em024_act7_sub16`, `em024_act7_sub25`, `em024_act7_sub26`, `em024_act7_field_burst`, `em024_act13_sub0`:
 *    ours hoists the `lwz r7` of the `shell_set_func_ptr->slot(...)` call, and register allocation and order differ;
 *  - `em024_tev_color_update`: register allocation and order.
 *   flipcheck: `.sdata2` is 0x274 against the claimed 0x278 (the claim ends at 0x8079B640 because `lbl_8079B638`
 *   is an 8-byte symbol; an end of 0x8079B63C makes `dtk dol split` fail); `.text` differs in 417 of 38124 bytes.
 * SHAPES. `enemy/ENEMY_WORK.h` carries the `em024_0x328` union view of the shared record for this monster's
 *   fields.  `em_turn_seq_*` take `void*`, so the `const` turn tables pass through a `(void*)` cast.
 */

#include "types.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_se_record.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "ef/eft052.h"
#include "mh3_pad.h"
#include "fn_8004CAD8.h"
#include "enemy/fn_80147CE0.h"
#include "ef/eft004.h"
#include "ef/fn_80105314.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft019.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "g3d/g3d_calcworld.h"
#include "sound/fn_800D7F54.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sound/fn_800DD1F0.h"
#include "sys_mem.h"
#include "Pl/fn_8028F66C.h"
#include "camera/fn_802B5C58.h"
#include "draw_shape.h"
#include "ef/fn_8010D1A8.h"
#include "stage/shell_set_func_ptr.h"
#include "lobby/fn_801E0ADC.h"
#include "lobby/fn_8030121C.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* `em_turn_seq_start`/`em_turn_seq_step` effect tables (0x40 B each): a four-word head, then three four-word rows
 * whose first word carries the effect id in its high half. */
extern "C" const u32 em024_turn_tbl_a[16] = {
    0x00000000, 0x00000028, 0x00000000, 0x40001C72,
    0x00050000, 0x00000002, 0x00000000, 0x00000000,
    0x00040000, 0x00000002, 0x00000000, 0x00000000,
    0x00030000, 0x00000002, 0x00000000, 0x00000000,
};
extern "C" const u32 em024_turn_tbl_b[16] = {
    0x00000010, 0x00000036, 0x00000000, 0x8000238E,
    0x00050000, 0x00000002, 0x00000000, 0x00000000,
    0x00060000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};
extern "C" const u32 em024_turn_tbl_c[16] = {
    0x00000028, 0x0000003C, 0x00000000, 0x55551555,
    0x00050000, 0x00000002, 0x00000000, 0x00000000,
    0x00390000, 0x00000002, 0x00000000, 0x00000000,
    0x003A0000, 0x00000002, 0x00000000, 0x00000000,
};
extern "C" const u32 em024_turn_tbl_d[16] = {
    0x00000014, 0x0000002C, 0x00000000, 0x80000000,
    0x00360000, 0x00000002, 0x00000000, 0x00000000,
    0x00360000, 0x00000002, 0x00000000, 0x00000000,
    0x00360000, 0x00000002, 0x00000000, 0x00000000,
};
extern "C" const u32 em024_turn_tbl_e[16] = {
    0x00000000, 0x00000020, 0x00000000, 0x80000000,
    0x003D0000, 0x00000002, 0x00000000, 0x00000000,
    0x003D0000, 0x00000002, 0x00000000, 0x00000000,
    0x003D0000, 0x00000002, 0x00000000, 0x00000000,
};
/* The `em_wave_amp` amplitude per motion group (0x441D8000 == 630.0f). */
extern "C" const f32 em024_turn_ratio_tbl[4] = {630.0f, 630.0f, 630.0f, 630.0f};

/* One keyframe of a motion curve: the frame number, then one to three channel values.  A row whose frame is
 * -1.0 ends the curve. */
struct EmKey1 {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 value_0x04;
}; /* size: 0x08 */

struct EmKey2 {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 value_0x04;
    /* +0x08 */ f32 value_0x08;
}; /* size: 0x0C */

struct EmKey3 {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 value_0x04;
    /* +0x08 */ f32 value_0x08;
    /* +0x0C */ f32 value_0x0C;
}; /* size: 0x10 */

/* The em024 program table (`em024_prog_tbl`): the entry points the shared enemy program driver calls.
 * Slot names are a GUESS from the callers' arguments. */
struct Em024ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 arg);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u8 (*part_damage_ck_0x24)(_ENEMY_WORK* self, u32 part);
    /* +0x28 */ u8 pad_0x28[0x38 - 0x28];
    /* +0x38 */ s32 (*part_level_even_ck_0x38)(_ENEMY_WORK* self, u32 part);
    /* +0x3C */ u8 pad_0x3C[0x70 - 0x3C];
}; /* size: 0x70 */

/* Clears the gauge state, timers and vectors, attaches the user-data helper when the work has none and
 * starts in state 2 with a full gauge for a fresh kind-1 work. */
extern "C" void em024_init(_ENEMY_WORK* self, u8 arg);
/* Counts the state timers down, sets a status bit outside state 1 and drops to state 0 once the front
 * part is broken; clears the swing flag when the mode changes. */
extern "C" void em024_frame_update(_ENEMY_WORK* self);
/* The action dispatcher of the program table: picks the handler from the action id. */
extern "C" void em024_action_dispatch(_ENEMY_WORK* self);
/* The empty program slot. */
extern "C" void em024_noop(void);
/* Re-arms the state and timers from the (kind, sub) event pair; kind 10 also drains the gauge by 200. */
extern "C" void em024_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub);
/* Per-frame motion events: effects, sound and camera cues keyed by the current motion. */
extern "C" void em024_motion_events(_ENEMY_WORK* self);
/* Fades the model's two TEV colors toward the state's targets and sets the material alphas. */
extern "C" void em024_tev_color_update(_ENEMY_WORK* self);
/* Spawns the lava effect once while motion 0x67/0x68 runs over lava. */
extern "C" void em024_magma_burst(_ENEMY_WORK* self);

extern "C" Em024ProgTbl em024_prog_tbl = {
    0,
    em024_init,
    em024_frame_update,
    em024_action_dispatch,
    em024_noop,
    em024_event_notify,
    em024_motion_events,
    0,
    em024_tev_color_update,
    eft052_part_damage_ck,
    {0},
    eft052_part_level_even_ck,
    {0},
};

EmKey1 em024_curve_mot37[] = {
    {0.0f, 44.0f}, {10.0f, 24.0f}, {20.0f, 12.0f},
    {34.0f, 0.0f}, {40.0f, -6.0f}, {50.0f, -10.0f},
    {60.0f, -16.0f}, {70.0f, -18.0f}, {80.0f, -20.0f},
    {90.0f, -20.0f}, {100.0f, -20.0f}, {108.0f, -20.0f},
    {124.0f, -16.0f}, {-1.0f, -16.0f},
};

EmKey1 em024_curve_mot39[] = {
    {0.0f, 0.0f}, {198.0f, 0.0f}, {200.0f, 25.0f},
    {215.0f, 55.0f}, {240.0f, 65.0f}, {260.0f, 35.0f},
    {280.0f, 14.0f}, {360.0f, 14.0f}, {-1.0f, 14.0f},
};

EmKey1 em024_curve_mot42[] = {
    {0.0f, 17.0f}, {4.0f, 9.0f}, {10.0f, 5.5f},
    {18.0f, 8.5f}, {22.0f, 17.5f}, {26.0f, 15.0f},
    {30.0f, 10.5f}, {34.0f, 5.0f}, {36.0f, -7.0f},
    {38.0f, -18.5f}, {40.0f, -29.5f}, {44.0f, -38.0f},
    {52.0f, -38.0f}, {-1.0f, -38.0f},
};

EmSeOffset em024_se_shift = {-50.0f, -10.0f};

/* Answers -1 unless the action is 11, otherwise whether its sub-state is 5. */
extern "C" s8 em024_action11_state5_ck(_ENEMY_WORK* self)
{
    if (self->action != 11)
        return -1;
    return self->state_sub == 5;
}

/* Casts a collision ray from the monster to a point ahead of it and answers whether it hits; 0 while the front part is broken. */
extern "C" u32 em024_front_ray_hit_ck(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 from;

    VEC3_ctor(&offset);
    VEC3_ctor(&to);
    if (em_parts_damage_level_get(self, 0) >= 4)
        return 0;
    offset.x = 115.0f;
    offset.y = 0.0f;
    offset.z = 500.0f + 550.0f * get_em_chg_scale(self);
    rotVecY(&offset, self->field_0x1C0);
    addVec3(&from, &self->pos, &offset);
    copyVec3(&to, &from);
    return pl_coll_sweep_ck(&self->pos, &to, 170.0f, 1, 0x100, self->area_no, em_hit_mask_get(self)) != 0;
}

/* Answers 1 when a live motion slot has the 0x100 flag and a value outside 0x801..0xF7FF. */
extern "C" u32 em024_slot_flag_ck(_ENEMY_WORK* self)
{
    u8 i;

    if (em_parts_damage_level_get(self, 0) >= 4)
        return 0;
    if (self->field_0x218 == 0)
        return 0;
    for (i = 0; i < 10; i++) {
        EmMotionSlot* slot = &self->slots_0x244[i];

        if (slot->flags == 0)
            break;
        if (slot->value - 0x801 > 0xEFFE && (slot->flags & 0x100) != 0)
            return 1;
    }
    return 0;
}

extern "C" void em024_init(_ENEMY_WORK* self, u8 arg)
{
    self->em024_0x328.part_lock_0x328 = 0;
    self->em024_0x328.tev_ramp_0x329 = 0;
    self->em024_0x328.flag_0x32A = 0;
    self->em024_0x328.gauge_0x32C = 0;
    self->em024_0x328.timer_0x32E = 0;
    self->em024_0x328.timer_0x330 = 0;
    self->em024_0x328.hold_timer_0x332 = 0;
    self->em024_0x328.tev_color_0x334.x = 255.0f;
    self->em024_0x328.tev_color_0x334.y = 65.0f;
    self->em024_0x328.tev_color_0x334.z = 30.0f;
    self->em024_0x328.tev_color2_0x340 = self->em024_0x328.tev_color_0x334;
    self->field_0x1E4 = 0;
    if (em_res_user_data_ck(self) == 0) {
        void* rec = operator new(0xC);

        if (rec != NULL)
            em_res_user_data_ctor(rec);
        em_res_user_data_set(self, rec);
    }
    if (arg == 0 && self->field_0x010 == 1) {
        self->field_0x1E4 = 2;
        self->em024_0x328.gauge_0x32C = 500;
    }
}

extern "C" void em024_noop(void)
{
}

extern "C" void em024_event_notify(_ENEMY_WORK* self, u8 a, u8 b)
{
    switch (a) {
    case 1:
        switch (b) {
        case 3:
            self->field_0x1E4 = 1;
            self->em024_0x328.timer_0x32E = 1800;
            break;
        }
        break;
    case 4:
        switch (b) {
        case 10:
            self->field_0x1E4 = 2;
            self->em024_0x328.gauge_0x32C = 500;
            break;
        case 11:
            self->field_0x1E4 = 0;
            self->em024_0x328.timer_0x330 = 900;
            break;
        }
        break;
    case 7:
        switch (b) {
        case 32:
            self->em024_0x328.flag_0x32A = 1;
            break;
        }
        break;
    case 10:
        switch (b) {
        case 195:
            self->field_0x1E4 = 0;
            self->em024_0x328.timer_0x32E = 0;
            self->em024_0x328.timer_0x330 = 900;
            break;
        case 202:
            em_part_hit_set(self, 0, 0);
            break;
        }
        eft052_part_gauge_add(self, -200);
        break;
    }
}

extern "C" void em024_frame_update(_ENEMY_WORK* self)
{
    switch (self->field_0x1E4) {
    case 1:
        if (self->em024_0x328.timer_0x32E > 0)
            self->em024_0x328.timer_0x32E--;
        break;
    case 0:
        if (self->em024_0x328.timer_0x330 > 0)
            self->em024_0x328.timer_0x330--;
        break;
    }
    if (self->field_0x1E4 != 1)
        em_status_bit_set(self, 0);
    if (self->em024_0x328.hold_timer_0x332 > 0)
        self->em024_0x328.hold_timer_0x332--;
    if (em_parts_damage_level_get(self, 0) >= 4) {
        if ((self->flags_0x836 & 0x8000) == 0)
            self->flags_0x836 |= 0x8000;
        if (self->field_0x1E4 != 0) {
            self->field_0x1E4 = 0;
            self->em024_0x328.timer_0x32E = 0;
            self->em024_0x328.timer_0x330 = 900;
        }
    }
    if (self->em024_0x328.flag_0x32A != 0 && self->field_0x1E2 != 1)
        self->em024_0x328.flag_0x32A = 0;
}

/* Action 0 sub-state 0/2: plays motion 1 (blend 4) and finishes the action when it ends. */
extern "C" void em024_act0_sub0(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 0 sub-state 1: the same step as sub-state 0. */
extern "C" void em024_act0_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 0 sub-state 3: starts a fall, plays motion 8 (blend 4) and finishes through the fall finish. */
extern "C" void em024_act0_sub3(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* Action 0 sub-state 6: plays motion 8 (blend 4) in move mode 3 and finishes through the walk finish. */
extern "C" void em024_act0_sub6(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Picks the action-0 step from the sub-state. */
extern "C" void em024_act0_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act0_sub0(self);
        break;
    case 1:
        em024_act0_sub1(self);
        break;
    case 2:
        em024_act0_sub0(self);
        break;
    case 3:
        em024_act0_sub3(self);
        break;
    case 6:
        em024_act0_sub6(self);
        break;
    }
}

/* Action 1 sub-state 0: plays motion 12 (blend 2) and finishes when it ends. */
extern "C" void em024_act1_sub0(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 12, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1 sub-state 3: plays motion 33 with two hit windows, re-arms one at frame 40 and finishes when it ends. */
extern "C" void em024_act1_sub3(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 33, 4, 0);
        em_hit_window_set(self, 0, 16, 8);
        em_hit_window_set(self, 1, 32, 24);
        break;
    case 1:
        if (em_frame_check(self, 0, 200.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 31, 16);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1 sub-state 4: plays motion 34 under a 300-frame timer that falls back to state 1/5 when the front
 * ray is blocked, then plays motion 20 and its effect. */
extern "C" void em024_act1_sub4(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 34, 4, 0);
        em_move_vec_clr(self);
        self->offset_0x30C.vec_0x310.z = 10.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        self->timer_0x020 = 300;
        break;
    case 1:
        if (em_frame_check(self, 3, 16.0f, 36.0f) == 1)
            em_move_offset_apply(self);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 20, 14, 48);
            em_hit_window_set(self, 0, 20, 136);
            em_hit_window_set(self, 1, 23, 16);
        } else if (em024_front_ray_hit_ck(self) == 0) {
            em_state_set(self, 1, 5);
        } else if (self->timer_0x020 == 20) {
            em_camera_req(self, 21, 3);
        }
        break;
    case 2:
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 181, 21, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1 sub-state 5: plays motion 100 (blend 8) and finishes when it ends. */
extern "C" void em024_act1_sub5(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 100, 8, 8);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Picks the action-1 step from the sub-state. */
extern "C" void em024_act1_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act1_sub0(self);
        break;
    case 3:
        em024_act1_sub3(self);
        break;
    case 4:
        em024_act1_sub4(self);
        break;
    case 5:
        em024_act1_sub5(self);
        break;
    }
}

/* Action 2 sub-state 0: plays motion 2, starts an approach and finishes once the approach step reports done. */
extern "C" void em024_act2_sub0(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 2, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2 sub-state 1: runs the first turn sequence and finishes when it ends. */
extern "C" void em024_act2_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em024_turn_tbl_a, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em024_turn_tbl_a) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2 sub-state 2: runs the second turn sequence and finishes when it ends. */
extern "C" void em024_act2_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em024_turn_tbl_b, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em024_turn_tbl_b) == 1)
            em_action_finish(self);
        break;
    }
}

/* Picks the action-2 step from the sub-state. */
extern "C" void em024_act2_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act2_sub0(self);
        break;
    case 1:
        em024_act2_sub1(self);
        break;
    case 2:
        em024_act2_sub2(self);
        break;
    }
}

/* Action 3 sub-state 0: starts a fall, plays motion 8 and turns toward the target, then finishes through
 * the fall finish. */
extern "C" void em024_act3_sub0(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 8, 0, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* Action 3 sub-state 1: like sub-state 0, with an approach and a move offset that grows toward a limit. */
extern "C" void em024_act3_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 8, 0, 0);
        em_approach_start(self, -800.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_fall(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 70.0f)
                self->field_0x318 = 70.0f;
        }
        break;
    }
}

/* Action 3 sub-state 2: dives to the ground, then plays motions 71 and 73 in turn and finishes. */
extern "C" void em024_act3_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 8, 4, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 71, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 73, 0, 0);
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 3 sub-state 3: plays motion 70, lifts through motion 72 and finishes once the height test passes. */
extern "C" void em024_act3_sub3(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 70, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 72, 0, 0);
            em_lift_start(self);
        }
        break;
    case 2:
        em_lift_step(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 8, 4, 0);
        }
        /* falls through: the target runs the state-3 height test on the same tick */
    case 3:
        em_lift_step(self);
        if (self->pos.y >= 6000.0f + self->vec_0x36C.y)
            em_action_finish_fall(self);
        break;
    }
}

/* Action 3 sub-state 4: dives and finishes through the walk finish when the ground test passes. */
extern "C" void em024_act3_sub4(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 8, 4, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Picks the action-3 step from the sub-state. */
extern "C" void em024_act3_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act3_sub0(self);
        break;
    case 1:
        em024_act3_sub1(self);
        break;
    case 2:
        em024_act3_sub2(self);
        break;
    case 3:
        em024_act3_sub3(self);
        break;
    case 4:
        em024_act3_sub4(self);
        break;
    }
}

/* Action 4 sub-states 0, 6 and 9 (by `kind`): motion 21 to 38 in stages, with a shell job and hit effects for kind 2. */
extern "C" void em024_act4_sub0(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 origin;
    nw4r::math::VEC3 tmp;
    EmSpawnRec rec;

    VEC3_ctor(&origin);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 21, 2, 0);
        if (kind == 2)
            em_mot_speed_set(self, 0.8f);
        break;
    case 1:
        if (kind == 2) {
            if (em_frame_check(self, 0, 38.0f, 0.0f) == 1)
                em_mot_speed_set(self, 1.0f);
            if (em_frame_check(self, 3, 12.0f, 28.0f) == 1 && (system_w.field_0x0c & 1) == 0)
                eft007_part_set(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 22, 0, 0);
        }
        break;
    case 2:
        if (kind == 1) {
            if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 23, 4, 0);
            }
        } else {
            if (kind == 2) {
                if (em_frame_check(self, 0, 4.0f, 0.0f) == 1)
                    eft007_part_spawn(self, 64, 0x2000, 0);
                if (em_frame_check(self, 0, 6.0f, 0.0f) == 1) {
                    rec.id = 21;
                    copyVec3(&rec.pos, setVec3(&tmp, 0.0f, 0.0f, 100.0f));
                    rec.field_0x10 = 0;
                    rec.field_0x12 = 0x2000;
                    rec.field_0x14 = 0;
                    shell_set_func_ptr->method_0x3C(self, &rec, 32, shell_set_func_ptr);
                }
            }
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_move_mode_set(self, 0);
                em_mot_set(self, 38, 0, 0);
                em_hit_window_set_default(self, 0, 24);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            if (kind == 1) {
                self->state++;
                em_move_mode_set(self, 3);
                em_mot_set(self, 8, 0, 28);
            } else {
                em_action_finish(self);
            }
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-state 1: plays motion 8 in move mode 3 and finishes through the walk finish once the turn completes. */
extern "C" void em024_act4_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 8, 0, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-state 2: approaches the target with a clamped move offset and finishes through the walk finish. */
extern "C" void em024_act4_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 8, 0, 0);
        em_approach_start(self, -800.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_walk(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 70.0f)
                self->field_0x318 = 70.0f;
        }
        break;
    }
}

/* Action 4 sub-state 3: aims at the target, plays motion 14 and turns by the clamped angle. */
extern "C" void em024_act4_sub3(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 delta;
    u32 angle_x;
    u32 angle_y;
    u32 full = 0x10000;
    u32 value;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 14, 2, 0);
        subVec3(&delta, &self->vec_0x36C, &self->pos);
        copyVec3(&dir, &delta);
        calcVecAngXY(&dir, &angle_x, &angle_y);
        value = (u16)(full - (angle_y - self->field_0x1C0));
        if (value < 0x6000)
            value = 0x6000;
        else if (value > 0xA000)
            value = full - 0x6000;
        self->state_0x006 = value >> 8;
        break;
    case 1:
        em_turn_in_window(self, 6.0f, 54.0f, -(self->state_0x006 << 8));
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-states 4 and 5 (by `kind`): plays motion 16 or 15 and turns the monster by 0x4000 either way. */
extern "C" void em024_act4_sub4(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0: {
        u32 mot;
        f32 spread;

        self->state++;
        em_move_mode_set(self, 3);
        if (kind == 1) {
            mot = 16;
            spread = -50.0f;
        } else {
            mot = 15;
            spread = 50.0f;
        }
        em_mot_set(self, mot, 2, 0);
        em_move_vec_clr(self);
        self->offset_0x30C.vec_0x310.x = spread;
        break;
    }
    case 1:
        em_turn_in_window(self, 0.0f, 88.0f, kind == 1 ? 0x4000 : -0x4000);
        if (em_frame_check(self, 3, 0.0f, 88.0f) == 1)
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-state 7: plays motions 70 then 72 and finishes through the walk finish. */
extern "C" void em024_act4_sub7(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 70, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 72, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-states 8 and 11: plays motions 71 then 73 and finishes. */
extern "C" void em024_act4_sub8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 71, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 73, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 4 sub-state 10: plays motion 36, then motion 37 under the scale curve, and ends after a 22-frame wait. */
extern "C" void em024_act4_sub10(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 36, 4, 0);
        em_hit_window_set(self, 0, 30, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 37, 0, 0);
            em_move_vec_clr(self);
            self->field_0x314 = 194.0f * get_em_scale(self);
            em_move_offset_apply(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em024_curve_mot37);
        em_move_offset_apply(self);
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set_blend(self, 8, 20, 0, 1);
            self->timer_0x020 = 22;
        }
        break;
    case 3:
        if (--self->timer_0x020 <= 0)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 4 sub-state 12: plays motion 27 in move mode 3 and finishes through the walk finish. */
extern "C" void em024_act4_sub12(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 27, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Picks the action-4 step from the sub-state, passing the variant to the shared steps. */
extern "C" void em024_act4_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act4_sub0(self, 0);
        break;
    case 1:
        em024_act4_sub1(self);
        break;
    case 2:
        em024_act4_sub2(self);
        break;
    case 3:
        em024_act4_sub3(self);
        break;
    case 4:
        em024_act4_sub4(self, 0);
        break;
    case 5:
        em024_act4_sub4(self, 1);
        break;
    case 6:
        em024_act4_sub0(self, 1);
        break;
    case 7:
        em024_act4_sub7(self);
        break;
    case 8:
        em024_act4_sub8(self);
        break;
    case 9:
        em024_act4_sub0(self, 2);
        break;
    case 10:
        em024_act4_sub10(self);
        break;
    case 11:
        em024_act4_sub8(self);
        break;
    case 12:
        em024_act4_sub12(self);
        break;
    }
}

/* Action 7 sub-states 0, 10, 34, 35 and 39..46: the long attack step, its effects and turns selected by (attack, turn). */
extern "C" void em024_act7_sub0(_ENEMY_WORK* self, u8 attack, u8 turn)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&joint_pos);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 17, 4, 0);
        em_hit_window_set(self, 0, attack == 1 ? 27 : 1, 11);
        em_approach_start(self, turn == 1 ? -2300.0f : 500.0f, 0);
        if ((u32)(turn - 2) <= 3 && self->value_0x378 > 2500.0f)
            self->value_0x378 = 2500.0f;
        eft052_part_gauge_add(self, -20);
        self->timer_0x020 = 0;
        break;
    case 1: {
        s32 range;

        self->timer_0x020++;
        if (attack == 0) {
            if (em_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 21, &pos, 0.6f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 21, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 141, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 27, &pos, 0.5f);
            }
            if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 11, &pos, 0.7f);
            }
            if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 3, &pos, 1.0f);
            }
            if (em_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 19, &pos, 1.0f);
            }
            if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 29, &pos, 1.0f);
            }
            if (em_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 33, &pos, 1.0f);
            }
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                if ((system_w.field_0x0c & 7) == 0) {
                    setVector3(&pos, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 103, 3, &pos, 3.0f);
                }
                if (self->timer_0x020 > 5) {
                    self->timer_0x020 = 0;
                    eft_spawn_type_at_area(self, 10);
                }
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 3, &joint_pos);
                    shell_se_req(self->se_0xB14, &joint_pos, 28, self->field_0x01A);
                }
            }
        }
        range = 0x80;
        if (em_frame_check(self, 1, 40.0f, 0.0f) == 1) {
            switch (turn) {
            case 2:
                self->field_0x1C0 = (u16)(self->field_0x1C0 + 0x100);
                range = 0;
                break;
            case 3:
                self->field_0x1C0 = (u16)(self->field_0x1C0 - 0x100);
                range = 0;
                break;
            case 4:
                self->field_0x1C0 = (u16)(self->field_0x1C0 + 0x180);
                range = 0;
                break;
            case 5:
                self->field_0x1C0 = (u16)(self->field_0x1C0 - 0x180);
                range = 0;
                break;
            }
        }
        if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
            if (em024_slot_flag_ck(self) == 1) {
                em_hit_window_clear(self, 0);
                em_state_set(self, 1, 4);
                break;
            }
            if (em_approach_step(self, 0, range) == 1) {
                em_hit_window_clear(self, 0);
                self->state = 3;
                em_mot_set(self, 20, 2, 0);
                em_hit_window_set(self, 1, 23, 16);
                em_hit_window_set_default(self, 0, 20);
                break;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 18, 0, 0);
        }
        break;
    }
    case 2: {
        s32 range;

        if (attack == 0) {
            self->timer_0x020++;
            if (system_w.field_0x0c % 10 == 0) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 103, 3, &pos, 3.0f);
            }
            if (self->timer_0x020 > 7) {
                self->timer_0x020 = 0;
                eft_spawn_type_at_area(self, 10);
            }
            if (self->area_no == get_now_areano()) {
                get_joint_wpos_em(self, 3, &joint_pos);
                shell_se_req(self->se_0xB14, &joint_pos, 28, self->field_0x01A);
            }
        }
        range = 0x80;
        switch (turn) {
        case 2:
            self->field_0x1C0 = (u16)(self->field_0x1C0 + 0x100);
            range = 0;
            break;
        case 3:
            self->field_0x1C0 = (u16)(self->field_0x1C0 - 0x100);
            range = 0;
            break;
        case 4:
            self->field_0x1C0 = (u16)(self->field_0x1C0 + 0x180);
            range = 0;
            break;
        case 5:
            self->field_0x1C0 = (u16)(self->field_0x1C0 - 0x180);
            range = 0;
            break;
        }
        if (em024_slot_flag_ck(self) == 1) {
            em_hit_window_clear(self, 0);
            em_state_set(self, 1, 4);
            break;
        }
        if (em_approach_step(self, 0, range) == 1) {
            em_hit_window_clear(self, 0);
            self->state++;
            em_mot_set(self, 20, 2, 0);
            em_hit_window_set(self, 1, 23, 16);
            em_hit_window_set_default(self, 0, 20);
        }
        break;
    }
    case 3:
        if (attack == 0) {
            if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                eft_em_spawn(self, 182, 17, NULL, 1.0f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 17, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 143, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 4.0f, 0.0f) == 1)
                eft_em_spawn(self, 182, 32, NULL, 0.8f);
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                eft_em_spawn(self, 182, 29, NULL, 0.7f);
                eft_em_spawn(self, 182, 46, NULL, 0.8f);
            }
            if (em_frame_check(self, 0, 52.0f, 0.0f) == 1) {
                eft_em_spawn(self, 176, 21, NULL, 1.0f);
                eft_em_spawn(self, 177, 21, NULL, 1.0f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 21, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 148, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 100.0f, -50.0f);
                eft_em_spawn(self, 101, 21, &pos, 0.8f);
            }
            if (em_frame_check(self, 0, 126.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 50.0f);
                eft_em_spawn(self, 101, 21, &pos, 0.8f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wmat_em(self, 21, &mtx);
                    mulVecMat(&pos, &mtx);
                    mtx34_trans_add(&mtx, &pos);
                    mtx34_trans_get(&mtx, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 148, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 132.0f, 0.0f) == 1)
                eft_em_spawn(self, 101, 11, NULL, 0.8f);
            if (em_frame_check(self, 0, 144.0f, 0.0f) == 1)
                eft_em_spawn(self, 101, 36, NULL, 0.7f);
            if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft_em_spawn(self, 101, 21, &pos, 0.5f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wmat_em(self, 21, &mtx);
                    mulVecMat(&pos, &mtx);
                    mtx34_trans_add(&mtx, &pos);
                    mtx34_trans_get(&mtx, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 148, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 186.0f, 0.0f) == 1)
                eft_em_spawn(self, 101, 33, NULL, 0.4f);
            if (em_frame_check(self, 0, 192.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 101, 27, &pos, 0.4f);
            }
            if (em_frame_check(self, 0, 212.0f, 0.0f) == 1)
                eft_em_spawn(self, 101, 46, NULL, 0.4f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 1 and 24: motion 46 with two effect bursts and, for attack 1, a seeded shell job. */
extern "C" void em024_act7_sub1(_ENEMY_WORK* self, u8 attack)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 pos;

    VEC3_ctor(&dir);
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 46, 4, 0);
        em_hit_window_set(self, 0, 2, 8);
        em_hit_window_set(self, 1, 3, 16);
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            eft_em_spawn(self, 166, 48, NULL, 1.0f);
            eft_em_spawn(self, 166, 49, NULL, 1.0f);
        }
        if (em_frame_check(self, 3, 30.0f, 102.0f) == 1) {
            if (++self->timer_0x020 > 6) {
                setVector3(&pos, 0.0f, -30.0f, 0.0f);
                eft_em_spawn(self, 165, 48, &pos, 1.0f);
                eft_em_spawn(self, 165, 49, &pos, 1.0f);
                self->timer_0x020 = 0;
            }
        }
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 48, 76, 0x8000, 1.0f);
        if (em_frame_check(self, 0, 162.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 48, 76, 0, 1.0f);
        if (attack == 1 && em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            s32 bits = self->bits_0x1EC;
            f32 lo = bits & 0xFF;
            f32 hi = (bits >> 8) & 0xFF;

            dir.x = 8.0f * (lo - 128.0f);
            dir.y = 0.0f;
            dir.z = -200.0f - 5.0f * hi;
            rotVecY(&dir, self->field_0x1C0);
            addVec3To(&dir, &self->pos);
            shell_set_func_ptr->method_0x8C(self, 0, &dir, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            eft_em_spawn(self, 166, 48, NULL, 1.0f);
            eft_em_spawn(self, 166, 49, NULL, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 2 and 33: motion 47 with a shell job and effects at fixed frames; `attack` picks the width. */
extern "C" void em024_act7_sub2(_ENEMY_WORK* self, u8 attack)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint_pos;
    nw4r::math::VEC3 tmp;
    EmSpawnRec rec;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&joint_pos);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 47, 4, 0);
        em_hit_window_set_default(self, 0, 21);
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 1, 40.0f, 64.0f) == 1 && (system_w.field_0x0c & 3) == 0)
            eft007_part_set(self, 40);
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            if (attack == 1)
                eft007_part_spawn(self, 41, 0x600, 0);
            else
                eft007_part_spawn(self, 41, 0xA00, 0);
        }
        if (em_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            rec.id = 21;
            copyVec3(&rec.pos, setVec3(&tmp, 0.0f, -30.0f, 100.0f));
            copyVec3(&pos, &rec.pos);
            rec.field_0x10 = 0;
            if (attack == 1)
                rec.field_0x12 = 0x600;
            else
                rec.field_0x12 = 0xA00;
            rec.field_0x14 = 0;
            shell_set_func_ptr->method_0x3C(self, &rec, 34, shell_set_func_ptr);
            if (self->area_no == get_now_areano()) {
                get_joint_wmat_em(self, 21, &mtx);
                mulVecMat(&pos, &mtx);
                mtx34_trans_add(&mtx, &pos);
                mtx34_trans_get(&mtx, &joint_pos);
                se_req_pos_ps(self->se_0xB14, 121, 2, &joint_pos);
            }
        }
        if (em_frame_check(self, 0, 162.0f, 0.0f) == 1)
            eft007_part_set(self, 42);
        if (em_frame_check(self, 0, 90.0f, 0.0f) == 1)
            em_mot_speed_set(self, 0.8f);
        if (em_frame_check(self, 0, 220.0f, 0.0f) == 1)
            em_mot_speed_set(self, 1.0f);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 3, 4, 7, 8 and 18..21: the motion 57/58 swing; `side` and `lead` pick the joints and turn. */
extern "C" void em024_act7_sub3(_ENEMY_WORK* self, u8 side, u8 lead)
{
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&joint_pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        {
            u32 mot;

            if (side == 1 || side == 3)
                mot = 58;
            else
                mot = 57;
            self->timer_0x020 = 0;
            em_mot_set(self, mot, 2, 0);
        }
        eft052_part_gauge_add(self, -20);
        if (lead == 1) {
            em_move_vec2_clr(self);
            self->field_0x318 = -40.0f;
            if (side == 1 || side == 3)
                rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + 0xE000);
            else
                rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + 0x2000);
        }
        break;
    case 1: {
        s32 turn;
        u32 joint;

        if (side == 1 || side == 3) {
            turn = -0x4000;
            joint = 8;
        } else {
            turn = 0x4000;
            joint = 13;
        }
        if (lead == 1 && em_frame_check(self, 3, 42.0f, 70.0f) == 1)
            CancelFade(self);
        self->timer_0x020++;
        if (side <= 1) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) == 1)
                eft_em_spawn(self, 81, joint, NULL, 1.0f);
            if (em_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                eft_em_spawn(self, 83, joint, NULL, 1.0f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, joint, &joint_pos);
                    se_req_pos_ps(self->se_0xB14, 148, 2, &joint_pos);
                }
            }
            if (em_frame_check(self, 0, 50.0f, 0.0f) == 1
                || em_frame_check(self, 0, 54.0f, 0.0f) == 1)
                eft_em_spawn(self, 83, joint, NULL, 1.0f);
        } else {
            switch (side) {
            case 2:
                if (em_frame_check(self, 0, 58.0f, 0.0f) == 1)
                    eft009_spawn_at_joint(self, 14, 2, 0, 3.0f);
                break;
            case 3:
                if (em_frame_check(self, 0, 58.0f, 0.0f) == 1)
                    eft009_spawn_at_joint(self, 9, 2, 0, 3.0f);
                break;
            }
        }
        em_turn_in_window(self, 40.0f, 60.0f, turn);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
    }
}

/* Action 7 sub-states 5 and 6: motion 41 with the offset approach and, for attack 1, a seeded shell job. */
extern "C" void em024_act7_sub5(_ENEMY_WORK* self, u8 attack)
{
    nw4r::math::VEC3 dir;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 41, 2, 0);
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 1, 14.0f, 0.0f) == 1)
            em_turn_to_target(self, 0x800);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 42, 0, 0);
            em_hit_window_set(self, 0, 8, 131);
            em_move_vec_clr(self);
            self->field_0x314 = 46.0f;
            em_move_offset_apply(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em024_curve_mot42);
        em_move_offset_apply(self);
        if (em_frame_check(self, 2, 16.0f, 0.0f) == 1)
            em_turn_to_target(self, 0x800);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 43, 0, 0);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (attack == 1 && em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            s32 bits = self->bits_0x1EC;
            f32 lo = bits & 0xFF;
            f32 hi = (bits >> 8) & 0xFF;

            dir.x = 8.0f * (lo - 128.0f);
            dir.y = 0.0f;
            dir.z = 1000.0f + 5.0f * hi;
            rotVecY(&dir, self->field_0x1C0);
            addVec3To(&dir, &self->pos);
            shell_set_func_ptr->method_0x8C(self, 0, &dir, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-state 9: plays motion 45 with two hit windows and a mid-motion speed change. */
extern "C" void em024_act7_sub9(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 45, 4, 0);
        em_hit_window_set(self, 0, 25, 8);
        em_hit_window_set(self, 1, 26, 16);
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1)
            em_mot_speed_set(self, 0.8f);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 11 and 12: motion 59 or 60 with effects and a turn by 0x4000. */
extern "C" void em024_act7_sub11(_ENEMY_WORK* self, u8 side)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0: {
        u32 mot;
        u32 part;

        self->state++;
        em_move_mode_set(self, 3);
        if (side == 1) {
            mot = 60;
            part = 12;
        } else {
            mot = 59;
            part = 10;
        }
        em_mot_set(self, mot, 2, 0);
        em_hit_window_set_default(self, 1, part);
        eft052_part_gauge_add(self, -20);
        break;
    }
    case 1: {
        if (em_frame_check(self, 3, 32.0f, 64.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 50.0f, 0.0f);
            eft_em_spawn(self, 175, 48, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 50.0f, 0.0f);
            eft_em_spawn_joint(self, 48, 31, &pos, 1.0f, 50);
            eft_em_spawn(self, 171, 48, &pos, 1.0f);
        }
        em_turn_in_window(self, 40.0f, 90.0f, side == 1 ? -0x4000 : 0x4000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
    }
}

/* Action 7 sub-states 13, 14 and 37: motions 49..53 by `kind`, with shell jobs and effects. */
extern "C" void em024_act7_sub13(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 tmp;
    EmSpawnRec rec;

    VEC3_ctor(&pos);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 49, 2, 0);
        eft052_part_gauge_add(self, -20);
        if (kind == 0) {
            em_move_vec_clr(self);
            self->field_0x318 = -35.0f;
        }
        break;
    case 1:
        if (kind == 0) {
            if (em_frame_check(self, 3, 2.0f, 40.0f) == 1) {
                em_turn_to_target(self, 0x100);
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            if (em_frame_check(self, 1, 2.0f, 0.0f) == 1 && (system_w.field_0x0c & 7) == 0)
                eft007_part_set(self, 62);
        }
        if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            u32 mot;

            self->state++;
            switch (kind) {
            default:
                mot = 50;
                break;
            case 1:
                mot = 52;
                break;
            case 2:
                mot = 52;
                em_move_vec_clr(self);
                self->field_0x318 = 20.0f;
                break;
            }
            em_mot_set(self, mot, 2, 0);
        }
        break;
    case 2:
        switch (kind) {
        case 0:
            if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                rec.id = 21;
                copyVec3(&rec.pos, setVec3(&tmp, 0.0f, 0.0f, 100.0f));
                rec.field_0x10 = 0;
                rec.field_0x12 = 0x2000;
                rec.field_0x14 = 0;
                eft007_part_spawn(self, 63, 0x2000, 0);
                shell_set_func_ptr->method_0x3C(self, &rec, 31, shell_set_func_ptr);
            }
            if (em_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 51, 2, 0);
            }
            break;
        case 2:
            em_turn_to_target(self, 0x80);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            /* falls through */
        case 1:
            if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, -35.0f, 100.0f);
                shell_set_func_ptr->method_0x74(self, 4, 21, &pos, 1.0f, 0, 0xFFFF, shell_set_func_ptr);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set(self, 53, 2, 0);
            }
            break;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 7 sub-state 15: motion 48 with the jet effects and area sound, then switches to state 7/25. */
extern "C" void em024_act7_sub15(_ENEMY_WORK* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&joint_pos);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 48, 4, 0);
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 1, 88.0f, 0.0f) == 1) {
            if ((system_w.field_0x0c & 3) == 0) {
                setVector3(&pos, 0.0f, -50.0f, 140.0f);
                eft_em_spawn_param(self, 145, 21, &pos, 1.0f, 0x11C7);
            }
            if (self->area_no == get_now_areano()) {
                get_joint_wmat_em(self, 21, &mtx);
                setVector3(&pos, 0.0f, -50.0f, 140.0f);
                mulVecMat(&pos, &mtx);
                mtx34_trans_add(&mtx, &pos);
                mtx34_trans_get(&mtx, &joint_pos);
                shell_se_req(self->se_0xB14, &joint_pos, 32, self->field_0x01A);
            }
            if ((system_w.field_0x0c & 7) == 0) {
                get_joint_wpos_em(self, 3, &joint_pos);
                setVector3(&pos, 0.0f, 2000.0f, 1300.0f);
                rotVecY(&pos, self->field_0x1C0);
                addVec3To(&joint_pos, &pos);
                eft019_set_vec(&joint_pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), self->area_no, 0x81, 1.0f);
            }
        }
        if (em_frame_check(self, 0, 164.0f, 0.0f) == 1) {
            if (self->area_no == get_now_areano()) {
                get_joint_wpos_em(self, 21, &joint_pos);
                se_req_pos_ps(self->se_0xB14, 164, 2, &joint_pos);
            }
            em_state_set(self, 7, 25);
        }
        break;
    }
}

/* Writes the point ahead of the monster, rotated by the offset angle `kind` selects. */
extern "C" void em024_act7_aim_pos(_ENEMY_WORK* self, u8 kind, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 target;
    s32 angle;

    setVec3(&dir, 0.0f, 0.0f, 100.0f);
    switch (kind) {
    default:
        angle = 0x2000;
        break;
    case 1:
        angle = 0xE000;
        break;
    case 2:
        angle = 0x4000;
        break;
    case 3:
        angle = 0xC000;
        break;
    }
    angle = (u16)(self->field_0x1C0 + angle);
    rotVecY(&dir, angle);
    addVec3(&target, &self->pos, &dir);
    copyVec3(out, &target);
}

/* Action 7 sub-states 16, 17, 22, 23 and 36: the turn sequences with per-frame effects, sound and shell jobs. */
extern "C" void em024_act7_sub16(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 aim;
    nw4r::math::VEC3 joint_pos;
    nw4r::math::VEC3 pos;
    u32 joint;
    f32 speed_a = 1.0f;
    f32 speed_b = speed_a;

    VEC3_ctor(&aim);
    VEC3_ctor(&joint_pos);
    VEC3_ctor(&pos);
    switch (kind) {
    case 1:
        speed_a = 0.83f;
        speed_b = 0.85f;
        break;
    case 4:
        speed_a = 0.85f;
        speed_b = 0.9f;
        self->field_0x7BC = 1.0f;
        break;
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, (void*)em024_turn_tbl_d, 0, 1, 0);
        eft052_part_gauge_add(self, -20);
        em_move_vec_clr(self);
        self->field_0x318 = -21.576923f * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, calcVecAng2(&self->pos, &self->vec_0x36C));
        switch (kind) {
        case 2:
            em024_act7_aim_pos(self, 0, &aim);
            em_target_pos_set(self, &aim);
            self->state_0x007 = 0;
            break;
        case 3:
            em024_act7_aim_pos(self, 1, &aim);
            em_target_pos_set(self, &aim);
            self->state_0x007 = 1;
            break;
        }
        break;
    case 1:
        em_turn_seq_step(self, (void*)em024_turn_tbl_d);
        if (em_frame_check(self, 3, 20.0f, 44.0f) == 1)
            em_move_offset_apply(self);
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 14, 29, &pos, 1.3f, 55);
            eft_em_spawn_joint(self, 13, 29, &pos, 1.3f, 55);
            eft_em_spawn_joint(self, 9, 29, &pos, 1.3f, 55);
            eft_em_spawn_joint(self, 8, 29, &pos, 1.3f, 55);
        }
        if ((em_frame_check(self, 3, 2.0f, 20.0f) == 1
             || em_frame_check(self, 3, 40.0f, 50.0f) == 1)
            && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 14;
                break;
            case 1:
                joint = 12;
                break;
            case 2:
                joint = 13;
                break;
            }
            eft_em_spawn(self, 169, joint, NULL, 1.0f);
        }
        if ((em_frame_check(self, 3, 4.0f, 22.0f) == 1
             || em_frame_check(self, 3, 42.0f, 52.0f) == 1)
            && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 9;
                break;
            case 1:
                joint = 7;
                break;
            case 2:
                joint = 8;
                break;
            }
            eft_em_spawn(self, 170, joint, NULL, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 55, 2, 0);
            em_hit_window_set(self, 0, 13, 136);
            if ((u32)(kind - 2) > 1) {
                if (((s32)kind == 1 || (s32)kind == 4) && self->state_0x006 != 0)
                    em_mot_speed_set(self, speed_b);
                f32 dist = calcVecDistXZ(&self->pos, &self->vec_0x36C) - 2500.0f;
                if (dist > 0.0f)
                    dist = 0.0f;
                else if (dist < -2100.0f)
                    dist = -2100.0f;
                em_move_vec_clr(self);
                self->field_0x318 = dist / 26.0f;
            } else {
                em_move_vec_clr(self);
                self->field_0x318 = -20.0f;
            }
        }
        break;
    case 2: {
        if ((u32)(kind - 2) > 1) {
            if (em_frame_check(self, 3, 2.0f, 26.0f) == 1)
                em_move_offset_rot_apply(self, &self->field_0x1BC);
        } else if (em_frame_check(self, 3, 4.0f, 48.0f) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 14, 30, &pos, 1.3f, 14);
            eft_em_spawn_joint(self, 9, 33, &pos, 1.3f, 14);
        }
        if (em_frame_check(self, 3, 2.0f, 20.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 14;
                break;
            case 1:
                joint = 12;
                break;
            case 2:
                joint = 13;
                break;
            }
            eft_em_spawn(self, 169, joint, NULL, 1.0f);
        }
        if (em_frame_check(self, 3, 4.0f, 24.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 9;
                break;
            case 1:
                joint = 7;
                break;
            case 2:
                joint = 8;
                break;
            }
            eft_em_spawn(self, 170, joint, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            eft_em_spawn(self, 172, 14, NULL, 1.0f);
            get_joint_wpos_em(self, 14, &joint_pos);
            if (kind == 1 || kind == 4)
                shell_set_func_ptr->method_0x8C(self, 1, &joint_pos, 1.0f, 60, shell_set_func_ptr);
            else
                shell_set_func_ptr->method_0x8C(self, 0, &joint_pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 9, &joint_pos);
            em_camera_req(self, 9, 9);
            joint_pos.y = self->field_0x20C;
            shell_set_func_ptr->method_0x28(self, &joint_pos, 5, self->field_0xAEA, shell_set_func_ptr, 0.8f);
        }
        if (em_mot_end_ck(self) == 1) {
            if ((u32)(kind - 2) > 1) {
                if ((s32)kind != 1 && (s32)kind != 4) {
                    self->state = 4;
                    em_mot_set(self, 56, 2, 0);
                } else if (++self->state_0x006 < 3) {
                    self->state++;
                    em_target_pos_set(self, NULL);
                    em_turn_seq_start(self, (void*)em024_turn_tbl_e, 0, 1, 0);
                    em_mot_speed_set(self, speed_a);
                    em_move_vec_clr(self);
                    self->field_0x318 = -18.0f * get_em_base_scale(self) * get_em_chg_scale(self);
                    rotVecY(&self->offset_0x30C.vec_0x310, calcVecAng2(&self->pos, &self->vec_0x36C));
                } else {
                    self->state = 4;
                    em_mot_set(self, 56, 2, 0);
                    em_mot_speed_set(self, speed_b);
                }
            } else if (++self->state_0x006 < 4) {
                self->state++;
                if (self->state_0x007 == 1) {
                    em024_act7_aim_pos(self, 2, &aim);
                    self->state_0x007 = 0;
                } else {
                    em024_act7_aim_pos(self, 3, &aim);
                    self->state_0x007 = 1;
                }
                em_target_pos_set(self, &aim);
                em_turn_seq_start(self, (void*)em024_turn_tbl_e, 0, 1, 0);
                em_move_vec_clr(self);
                self->field_0x318 = -18.0f * get_em_base_scale(self) * get_em_chg_scale(self);
                rotVecY(&self->offset_0x30C.vec_0x310, calcVecAng2(&self->pos, &self->vec_0x36C));
            } else {
                self->state = 4;
                em_mot_set(self, 56, 2, 0);
            }
        }
        break;
    }
    case 3:
        em_turn_seq_step(self, (void*)em024_turn_tbl_e);
        if (em_frame_check(self, 3, 2.0f, 32.0f) == 1)
            em_move_offset_apply(self);
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 14, 29, &pos, 1.3f, 43);
            eft_em_spawn_joint(self, 13, 29, &pos, 1.3f, 43);
            eft_em_spawn_joint(self, 9, 29, &pos, 1.3f, 43);
            eft_em_spawn_joint(self, 8, 29, &pos, 1.3f, 43);
        }
        if ((em_frame_check(self, 3, 2.0f, 10.0f) == 1
             || em_frame_check(self, 3, 30.0f, 40.0f) == 1)
            && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 14;
                break;
            case 1:
                joint = 12;
                break;
            case 2:
                joint = 13;
                break;
            }
            eft_em_spawn(self, 169, joint, NULL, 1.0f);
        }
        if ((em_frame_check(self, 3, 4.0f, 12.0f) == 1
             || em_frame_check(self, 3, 32.0f, 42.0f) == 1)
            && (system_w.field_0x0c & 3) == 0) {
            switch (ran_suu(0) % 3) {
            case 0:
                joint = 9;
                break;
            case 1:
                joint = 7;
                break;
            case 2:
                joint = 8;
                break;
            }
            eft_em_spawn(self, 170, joint, NULL, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state = 2;
            em_mot_set(self, 55, 2, 0);
            em_hit_window_set(self, 0, 13, 136);
            if ((u32)(kind - 2) > 1) {
                f32 dist;

                if ((s32)kind == 1 || (s32)kind == 4)
                    em_mot_speed_set(self, speed_b);
                dist = calcVecDistXZ(&self->pos, &self->vec_0x36C) - 2500.0f;
                if (dist > 0.0f)
                    dist = 0.0f;
                else if (dist < -2100.0f)
                    dist = -2100.0f;
                em_move_vec_clr(self);
                self->field_0x318 = dist / 26.0f;
            } else {
                em_move_vec_clr(self);
                self->field_0x318 = -20.0f;
            }
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 78.0f, 0.0f) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Builds a position around `origin` from a 16-bit seed: its low byte the sideways offset, its high byte the depth. */
extern "C" void em024_act7_scatter_pos(nw4r::math::VEC3* out, nw4r::math::VEC3* origin, s32 angle, u16 seed)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 target;

    VEC3_ctor(&offset);
    setVector3(&offset, 0.0f, 2000.0f, 1000.0f);
    offset.x += 10.0f * (f32)((u8)seed - 0x80);
    offset.z += 5.0f * (f32)(u16)(((u16)seed & 0xFF00) >> 8);
    rotVecY(&offset, angle);
    addVec3(&target, origin, &offset);
    copyVec3(out, &target);
}

/* Action 7 sub-state 25: starts motion 62 and throws three shell jobs at seed-derived positions. */
extern "C" void em024_act7_sub25(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 unused_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused_pos);
    switch (self->state) {
    case 0: {
        u16 seed;

        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_blend(self, 62, 26, 0, 1);
        seed = self->bits_0x1EC;
        em024_act7_scatter_pos(&pos, &self->pos, self->field_0x1C0, (u16)((seed * 0x12D6F3) / 10));
        shell_set_func_ptr->method_0x84(self, 0, &pos, 1.0f, 0, shell_set_func_ptr);
        em024_act7_scatter_pos(&pos, &self->pos, self->field_0x1C0, (u16)((seed * 0x74A04F) / 10));
        shell_set_func_ptr->method_0x84(self, 0, &pos, 1.0f, 8, shell_set_func_ptr);
        em024_act7_scatter_pos(&pos, &self->pos, self->field_0x1C0, (u16)(seed * 0x34BF15));
        shell_set_func_ptr->method_0x84(self, 0, &pos, 1.0f, 16, shell_set_func_ptr);
        self->em024_0x328.hold_timer_0x332 = 150;
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

nw4r::math::VEC3 em024_pick_offsets[] = {
    {800.0f, 0.0f, 800.0f}, {-800.0f, 0.0f, 800.0f}, {1200.0f, 0.0f, 0.0f},
    {-1200.0f, 0.0f, 0.0f}, {800.0f, 0.0f, -800.0f}, {-800.0f, 0.0f, -800.0f},
};

nw4r::math::VEC3 em024_field_offsets[] = {
    {-1792.1f, 250.0f, 2111.5f}, {272.3f, 244.2f, 3262.2f}, {-551.4f, 249.7f, 2475.8f},
    {477.8f, 250.0f, 1710.2f}, {-1222.0f, 250.0f, -156.8f}, {-1942.4f, 250.0f, -2254.0f},
    {-3184.1f, 250.0f, -1495.2f}, {-2635.1f, 250.0f, 3046.5f}, {-1279.7f, 246.8f, 3489.3f},
    {2405.6f, 242.0f, 3339.1f}, {1783.3f, 250.0f, 1936.8f}, {4714.9f, 250.0f, -1057.0f},
    {3655.8f, 250.0f, -2037.7f}, {1504.5f, 248.8f, -3111.1f}, {-1565.0f, 245.3f, -4898.9f},
    {-5146.9f, 291.5f, -4648.2f}, {-6152.9f, 250.0f, -2345.6f}, {-6301.9f, 250.0f, 875.6f},
    {-5057.5f, 250.0f, 2440.3f}, {-3025.7f, 250.0f, 4280.2f}, {-1879.9f, 241.5f, 5875.9f},
    {-262.5f, 230.4f, 4423.2f}, {2796.8f, 229.6f, 5199.3f}, {3816.5f, 249.9f, 3085.3f},
    {5336.8f, 250.0f, 2381.6f}, {5980.7f, 250.0f, 1222.0f}, {4793.8f, 250.0f, -2233.7f},
    {3791.7f, 245.3f, -3450.1f}, {3688.6f, 250.0f, -4853.6f}, {2658.2f, 250.0f, -3866.7f},
    {-582.0f, 250.0f, -1232.9f}, {-719.7f, 250.0f, 1121.0f}, {-2108.0f, 250.0f, -1023.9f},
    {1093.3f, 250.0f, 691.8f}, {-677.3f, 250.0f, -2654.7f}, {-4498.1f, 250.0f, -3121.6f},
    {-5593.0f, 250.0f, -1206.3f}, {-3894.8f, 250.0f, 3009.7f}, {-1822.3f, 242.7f, 4647.9f},
    {1267.7f, 231.8f, 4088.4f}, {3113.0f, 250.0f, 1933.1f}, {3473.1f, 250.0f, 683.5f},
    {2540.9f, 250.0f, -2569.7f}, {376.1f, 250.0f, -3971.1f}, {-2997.8f, 250.0f, -4509.8f},
    {-5695.2f, 248.3f, -3472.9f}, {-6279.7f, 241.4f, -411.0f}, {-6161.5f, 250.0f, 2159.4f},
    {-5002.4f, 250.0f, 3693.0f}, {-3030.5f, 250.0f, 5442.9f}, {-574.3f, 224.4f, 5665.0f},
    {1536.0f, 221.2f, 5374.4f}, {3405.3f, 250.0f, 4171.3f}, {5097.7f, 250.0f, 3643.0f},
    {4560.9f, 250.0f, 1426.9f}, {6380.5f, 250.0f, 134.3f}, {5908.4f, 250.0f, -1499.5f},
    {4883.1f, 250.0f, -3616.7f}, {2591.8f, 250.0f, -5076.9f}, {1540.5f, 250.0f, -5550.8f},
    {1562.6f, 250.0f, -4400.6f}, {-184.3f, 249.2f, -4843.6f}, {-1562.7f, 250.0f, -3669.1f},
    {-2954.2f, 250.0f, -3108.5f}, {-4419.0f, 250.0f, -1738.0f}, {-4132.5f, 250.0f, 4588.4f},
    {4211.1f, 250.0f, -11.4f}, {2238.4f, 250.0f, 619.0f}, {-17.9f, 250.0f, -0.9f},
};

u8 em024_field_index[4][35] = {
    {2, 9, 66, 13, 64, 36, 17, 48, 49, 50, 51, 39, 53, 41, 12, 4, 6, 34, 0, 37, 18, 10, 52, 21, 42, 59, 15, 22, 54, 26, 68, 3, 24, 7, 67},
    {31, 0, 68, 7, 37, 18, 31, 30, 32, 5, 15, 6, 34, 47, 17, 46, 36, 64, 35, 16, 45, 15, 44, 63, 62, 14, 4, 18, 30, 14, 17, 6, 35, 63, 37},
    {4, 5, 11, 12, 13, 14, 25, 26, 27, 28, 29, 30, 32, 33, 34, 30, 41, 42, 13, 43, 54, 55, 56, 57, 58, 59, 28, 60, 61, 14, 62, 66, 67, 33, 68},
    {0, 1, 2, 3, 7, 8, 9, 10, 18, 19, 20, 22, 23, 24, 25, 31, 33, 3, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 54, 55, 65, 66, 67, 1},
};

/* Action 7 sub-state 26: plays motion 32 and throws shell jobs at three distinct of six offsets picked from the seed. */
extern "C" void em024_act7_sub26(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 spawn_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&spawn_pos);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 32, 4, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 22, 5);
            draw_shape_arm((u32)self, 22, 10);
        }
        setVector3(&pos, 0.0f, -30.0f, 100.0f);
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1)
            eft_em_spawn(self, 0, 21, &pos, 1.0f);
        if (em_frame_check(self, 3, 56.0f, 138.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0)
                eft_em_spawn(self, 1, 21, &pos, 1.0f);
            self->timer_0x020++;
        }
        if (em_frame_check(self, 0, 164.0f, 0.0f) == 1) {
            nw4r::math::VEC3 target;

            self->state_0x006 = self->bits_0x1EC % 6;
            vec_to_mh_vec3(&pos, reinterpret_cast<struct Vec*>(&em024_pick_offsets[self->state_0x006]));
            rotVecY(&pos, self->field_0x1C0);
            addVec3(&target, &self->pos, &pos);
            copyVec3(&spawn_pos, &target);
            shell_set_func_ptr->method_0x8C(self, 1, &spawn_pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, 224.0f, 0.0f) == 1) {
            nw4r::math::VEC3 target;

            self->state_0x007 = ((s32)self->bits_0x1EC >> 4) % 5;
            if (self->state_0x007 >= self->state_0x006)
                self->state_0x007++;
            vec_to_mh_vec3(&pos, reinterpret_cast<struct Vec*>(&em024_pick_offsets[self->state_0x007]));
            rotVecY(&pos, self->field_0x1C0);
            addVec3(&target, &self->pos, &pos);
            copyVec3(&spawn_pos, &target);
            shell_set_func_ptr->method_0x8C(self, 1, &spawn_pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, 284.0f, 0.0f) == 1) {
            nw4r::math::VEC3 target;
            u8 pick = ((s32)self->bits_0x1EC >> 8) % 4;

            if (self->state_0x006 < self->state_0x007) {
                if (pick >= self->state_0x006)
                    pick += 1;
                if (pick >= self->state_0x007)
                    pick += 1;
            } else {
                if (pick >= self->state_0x007)
                    pick += 1;
                if (pick >= self->state_0x006)
                    pick += 1;
            }
            vec_to_mh_vec3(&pos, reinterpret_cast<struct Vec*>(&em024_pick_offsets[pick]));
            rotVecY(&pos, self->field_0x1C0);
            addVec3(&target, &self->pos, &pos);
            copyVec3(&spawn_pos, &target);
            shell_set_func_ptr->method_0x8C(self, 1, &spawn_pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 27 and 28 (by `side`): the turn sequence with joint effects. */
extern "C" void em024_act7_sub27(_ENEMY_WORK* self, u8 side)
{
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&joint_pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em024_turn_tbl_c, 0, 0, 0);
        self->timer_0x020 = 0;
        em_move_vec_clr(self);
        self->field_0x318 = -7.5f * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, calcVecAng2(&self->pos, &self->vec_0x36C));
        break;
    case 1:
        if (em_get_mot_no(self) != 5) {
            self->timer_0x020++;
            if (side == 0) {
                u32 joint = em_get_mot_no(self) == 58 ? 8 : 13;

                if (em_frame_check(self, 0, 20.0f, 0.0f) == 1)
                    eft_em_spawn(self, 81, joint, NULL, 1.0f);
                if (em_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                    eft_em_spawn(self, 83, joint, NULL, 1.0f);
                    if (self->area_no == get_now_areano()) {
                        get_joint_wpos_em(self, joint, &joint_pos);
                        se_req_pos_ps(self->se_0xB14, 148, 2, &joint_pos);
                    }
                }
                if (em_frame_check(self, 0, 50.0f, 0.0f) == 1
                    || em_frame_check(self, 0, 54.0f, 0.0f) == 1)
                    eft_em_spawn(self, 83, joint, NULL, 1.0f);
            } else {
                u32 joint = em_get_mot_no(self) == 58 ? 9 : 14;

                if (em_frame_check(self, 0, 58.0f, 0.0f) == 1)
                    eft009_spawn_at_joint(self, joint, 2, 0, 3.0f);
            }
            if (em_frame_check(self, 3, 6.0f, 70.0f) == 1)
                em_move_offset_apply(self);
        }
        if (em_turn_seq_step(self, (void*)em024_turn_tbl_c) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7 sub-states 29, 30 and 38 (by `kind`): motions 10, 9 and 11 with the long effect script. */
extern "C" void em024_act7_sub29(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&joint_pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 10, 0, 0);
        em_hit_window_set(self, 0, 14, 11);
        em_hit_window_set(self, 1, 15, 27);
        em_approach_start(self, 800.0f, 1);
        em_move_vec2_clr(self);
        if (kind == 2) {
            em_mot_speed_set(self, 0.7f);
            self->field_0x318 = 150.0f;
            self->field_0x324 = 10.0f;
        } else {
            self->field_0x318 = 80.0f;
            self->field_0x324 = 5.0f;
        }
        self->timer_0x020 = 0;
        break;
    case 1:
        if (kind == 0) {
            if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 29, NULL, 2.0f);
                eft_em_spawn(self, 169, 38, NULL, 0.6f);
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 29, &joint_pos);
                    shell_se_req(self->se_0xB14, &joint_pos, 29, self->field_0x01A);
                }
            }
            if (em_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 43, NULL, 0.6f);
                eft_em_spawn(self, 169, 49, NULL, 1.0f);
            }
            if (em_frame_check(self, 0, 6.0f, 0.0f) == 1)
                eft_em_spawn(self, 169, 33, NULL, 2.0f);
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 48, NULL, 1.2f);
                eft_em_spawn(self, 169, 37, NULL, 0.6f);
            }
            if (em_frame_check(self, 0, 10.0f, 0.0f) == 1)
                eft_em_spawn(self, 169, 42, NULL, 0.6f);
            if (em_frame_check(self, 0, 12.0f, 0.0f) == 1)
                eft_em_spawn(self, 169, 47, NULL, 1.4f);
            if (em_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 27, NULL, 2.0f);
                eft_em_spawn(self, 169, 36, NULL, 0.6f);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 46, NULL, 1.6f);
                eft_em_spawn(self, 169, 41, NULL, 0.6f);
            }
            if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 31, NULL, 2.0f);
                eft_em_spawn(self, 169, 45, NULL, 1.8f);
                eft_em_spawn(self, 169, 35, NULL, 0.6f);
            }
            if (em_frame_check(self, 0, 22.0f, 0.0f) == 1)
                eft_em_spawn(self, 169, 40, NULL, 0.6f);
            if (em_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                eft_em_spawn(self, 169, 18, NULL, 2.0f);
                eft_em_spawn(self, 169, 34, NULL, 2.0f);
            }
            if (em_frame_check(self, 1, 58.0f, 0.0f) == 1) {
                if (self->timer_0x020 > 5) {
                    self->timer_0x020 = 0;
                    eft_spawn_type_at_area(self, 17);
                }
                self->timer_0x020++;
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 3, &joint_pos);
                    shell_se_req(self->se_0xB14, &joint_pos, 29, self->field_0x01A);
                }
            }
        }
        if (kind == 2 && em_frame_check(self, 0, 36.0f, 0.0f) == 1)
            em_mot_speed_set(self, 1.0f);
        if (em_frame_check(self, 1, 64.0f, 0.0f) == 1) {
            f32 limit;

            if (em_approach_step(self, 0, 0) == 1) {
                self->state = 3;
                self->timer_0x020 = 0;
                em_mot_set(self, 11, 2, 0);
                break;
            }
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            if (kind == 2)
                limit = 240.0f;
            else
                limit = 180.0f;
            if (self->field_0x318 > limit) {
                self->field_0x318 = limit;
                self->field_0x324 = 0.0f;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set(self, 9, 4, 0);
        }
        break;
    case 2: {
        f32 limit;

        if (self->timer_0x020 > 5) {
            self->timer_0x020 = 0;
            eft_spawn_type_at_area(self, 17);
        }
        self->timer_0x020++;
        if (self->area_no == get_now_areano()) {
            get_joint_wpos_em(self, 3, &joint_pos);
            shell_se_req(self->se_0xB14, &joint_pos, 29, self->field_0x01A);
        }
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set(self, 11, 2, 0);
            break;
        }
        em_fall_height_get(self);
        em_move_offset_step_update(self, &self->field_0x1BC);
        if (kind == 2)
            limit = 240.0f;
        else
            limit = 180.0f;
        if (self->field_0x318 > limit) {
            self->field_0x318 = limit;
            self->field_0x324 = 0.0f;
        }
        break;
    }
    case 3:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1 && self->area_no == get_now_areano()) {
            get_joint_wpos_em(self, 3, &joint_pos);
            se_req_pos_ps(self->se_0xB14, 145, 2, &joint_pos);
        }
        if (em_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            em_hit_window_clear(self, 1);
            em_hit_window_set(self, 0, 17, 16);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* Action 7 sub-state 31: plays motion 39 under the scale curve, then motion 8. */
extern "C" void em024_act7_sub31(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 39, 4, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        self->field_0x314 = em_key_curve_eval(self, em024_curve_mot39);
        em_move_offset_apply(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 8, 10, 0);
        }
        break;
    case 2:
        em_move_offset_apply(self);
        if (self->pos.y >= self->vec_0x36C.y)
            em_action_finish_fall(self);
        break;
    }
}

/* Throws one shell job at the table position (or at the target) that `group`, `index` and `seed` select. */
extern "C" void em024_act7_field_burst(_ENEMY_WORK* self, u8 group, u8 index, u16 seed)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (group < 4 && index < 0x23) {
        u32 hit = 0;

        if (self->field_0x380 <= 2) {
            u16 mix = seed * 0x04148654;

            if (group == 0) {
                if ((index & 3) == (mix & 3))
                    hit = 1;
            } else if ((index & 7) == (mix & 7)) {
                hit = 1;
            }
        }
        if (hit == 1) {
            em_target_pos_set(self, NULL);
            copyVec3(&pos, &self->vec_0x36C);
        } else {
            u32 angle;

            vec_to_mh_vec3(&pos, reinterpret_cast<struct Vec*>(&em024_field_offsets[em024_field_index[group][index]]));
            switch (index & 3) {
            default:
                angle = (u16)((seed * 0x12D6F3) / 10);
                break;
            case 1:
                angle = (u16)((seed * 0x74A04C) / 10);
                break;
            case 2:
                angle = (u16)(seed * 0x34BF15);
                break;
            case 3:
                angle = (u16)(seed * 0x584D29);
                break;
            }
            em_wave_amp(&pos, &pos, em024_turn_ratio_tbl[group], 0, angle);
        }
        pos.y += 2000.0f;
        shell_set_func_ptr->method_0x84(self, 1, &pos, 1.4f, 0, shell_set_func_ptr);
    }
}

/* Action 7 sub-state 32: plays motion 48 and steps the field-burst script over 35 positions. */
extern "C" void em024_act7_sub32(_ENEMY_WORK* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&joint_pos);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 48, 4, 0);
        self->timer_0x020 = 0;
        eft052_part_gauge_add(self, -20);
        break;
    case 1:
        if (em_frame_check(self, 1, 88.0f, 0.0f) == 1) {
            if ((system_w.field_0x0c & 3) == 0) {
                setVector3(&pos, 0.0f, -50.0f, 140.0f);
                eft_em_spawn_param(self, 145, 21, &pos, 1.0f, 0x11C7);
            }
            if (self->area_no == get_now_areano()) {
                get_joint_wmat_em(self, 21, &mtx);
                setVector3(&pos, 0.0f, -50.0f, 140.0f);
                mulVecMat(&pos, &mtx);
                mtx34_trans_add(&mtx, &pos);
                mtx34_trans_get(&mtx, &joint_pos);
                shell_se_req(self->se_0xB14, &joint_pos, 32, self->field_0x01A);
            }
            if ((system_w.field_0x0c & 7) == 0) {
                get_joint_wpos_em(self, 3, &joint_pos);
                setVector3(&pos, 0.0f, 2000.0f, 1300.0f);
                rotVecY(&pos, self->field_0x1C0);
                addVec3To(&joint_pos, &pos);
                eft019_set_vec(&joint_pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), self->area_no, 0x81,
                               1.0f);
            }
        }
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 0, 130.0f, 0.0f) == 1)
                self->state_0x006++;
            break;
        case 1:
            if ((self->timer_0x020 & 7) == 0) {
                u16 bits = self->bits_0x1EC;

                em024_act7_field_burst(self, bits % 4, self->state_0x007, bits);
                if (++self->state_0x007 >= 0x23)
                    self->state_0x006++;
            }
            self->timer_0x020++;
            break;
        }
        if (em_frame_check(self, 0, 164.0f, 0.0f) == 1 && self->state_0x006 >= 2) {
            self->state++;
            em_mot_set(self, 62, 0, 0);
            if (self->area_no == get_now_areano()) {
                get_joint_wpos_em(self, 21, &joint_pos);
                se_req_pos_ps(self->se_0xB14, 164, 2, &joint_pos);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* Picks the action-7 step and its variant arguments from the sub-state. */
extern "C" void em024_act7_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act7_sub0(self, 0, 0);
        break;
    case 1:
        em024_act7_sub1(self, 0);
        break;
    case 2:
        em024_act7_sub2(self, 0);
        break;
    case 3:
        em024_act7_sub3(self, 0, 0);
        break;
    case 4:
        em024_act7_sub3(self, 1, 0);
        break;
    case 5:
        em024_act7_sub5(self, 0);
        break;
    case 6:
        em024_act7_sub5(self, 1);
        break;
    case 7:
        em024_act7_sub3(self, 0, 1);
        break;
    case 8:
        em024_act7_sub3(self, 1, 1);
        break;
    case 9:
        em024_act7_sub9(self);
        break;
    case 10:
        em024_act7_sub0(self, 1, 0);
        break;
    case 11:
        em024_act7_sub11(self, 0);
        break;
    case 12:
        em024_act7_sub11(self, 1);
        break;
    case 13:
        em024_act7_sub13(self, 0);
        break;
    case 14:
        em024_act7_sub13(self, 1);
        break;
    case 15:
        em024_act7_sub15(self);
        break;
    case 16:
        em024_act7_sub16(self, 0);
        break;
    case 17:
        em024_act7_sub16(self, 1);
        break;
    case 18:
        em024_act7_sub3(self, 2, 0);
        break;
    case 19:
        em024_act7_sub3(self, 3, 0);
        break;
    case 20:
        em024_act7_sub3(self, 2, 1);
        break;
    case 21:
        em024_act7_sub3(self, 3, 1);
        break;
    case 22:
        em024_act7_sub16(self, 2);
        break;
    case 23:
        em024_act7_sub16(self, 3);
        break;
    case 24:
        em024_act7_sub1(self, 1);
        break;
    case 25:
        em024_act7_sub25(self);
        break;
    case 26:
        em024_act7_sub26(self);
        break;
    case 27:
        em024_act7_sub27(self, 0);
        break;
    case 28:
        em024_act7_sub27(self, 1);
        break;
    case 29:
        em024_act7_sub29(self, 0);
        break;
    case 30:
        em024_act7_sub29(self, 1);
        break;
    case 31:
        em024_act7_sub31(self);
        break;
    case 32:
        em024_act7_sub32(self);
        break;
    case 33:
        em024_act7_sub2(self, 1);
        break;
    case 34:
        em024_act7_sub0(self, 0, 1);
        break;
    case 35:
        em024_act7_sub0(self, 1, 1);
        break;
    case 36:
        em024_act7_sub16(self, 4);
        break;
    case 37:
        em024_act7_sub13(self, 2);
        break;
    case 38:
        em024_act7_sub29(self, 2);
        break;
    case 39:
        em024_act7_sub0(self, 0, 2);
        break;
    case 40:
        em024_act7_sub0(self, 0, 3);
        break;
    case 41:
        em024_act7_sub0(self, 1, 2);
        break;
    case 42:
        em024_act7_sub0(self, 1, 3);
        break;
    case 43:
        em024_act7_sub0(self, 0, 4);
        break;
    case 44:
        em024_act7_sub0(self, 0, 5);
        break;
    case 45:
        em024_act7_sub0(self, 1, 4);
        break;
    case 46:
        em024_act7_sub0(self, 1, 5);
        break;
    }
}

extern "C" void em024_magma_burst(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((u32)(em_get_mot_no(self) - 0x67) <= 1) {
        if (self->state_0x006 == 0 && em_magma_check(self) == 1 && self->pos.y < self->field_0x214) {
            self->state_0x006++;
            copyVec3(&pos, &self->pos);
            pos.y = 5.0f + self->field_0x214;
            eft009_set_pos(0x88, &pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 1.5f, self->area_no);
        }
    } else {
        self->state_0x006 = 0;
    }
}

EmSeRecord em024_se_rec_mot23 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot23[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot23},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot56_a = {111, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em024_se_rec_mot56_b = {101, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot56[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x5C)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot56_a},
    {0x00, {0, 0, 0}, &em024_se_rec_mot56_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot195 = {114, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot195[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot195},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot61_a = {111, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em024_se_rec_mot61_b = {105, 0, 0, 24, 0, {0, 4, 0, 0}, 0};

EmSeRecord em024_se_rec_mot61_c = {106, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot61[5] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot61_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x96)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot61_b},
    {0x00, {0, 0, 0}, &em024_se_rec_mot61_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot35 = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot35[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot35},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot100_a = {103, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em024_se_rec_mot100_b = {0, 0, 0, 0, 0, {0, 2, 1, 0}, &em024_se_shift};

EmSeRecord em024_se_rec_mot100_c[2] = {
    {104, 0, 0, 0, 0, {0, 1, 0, 0}, 0},
    {105, 0, 2, 0, 0, {0, 4, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot100_d = {106, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot100[8] = {
    {0x23, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(em024_magma_burst)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x34)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot100_a},
    {0x01, {0, 0, 0}, &em024_se_rec_mot100_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, em024_se_rec_mot100_c},
    {0x00, {0, 0, 0}, &em024_se_rec_mot100_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot122 = {110, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot122[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xE4)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot122},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot123 = {110, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot123[2] = {
    {0x01, {0, 0, 0}, &em024_se_rec_mot123},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot159 = {106, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot159[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot159},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot124 = {108, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot124[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot124},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot141 = {109, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot141[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot141},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot125_a = {111, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em024_se_rec_mot125_b = {105, 0, 0, 24, 0, {1, 0, 0, 0}, 0};

EmSeRecord em024_se_rec_mot125_c = {106, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot125[4] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot125_a},
    {0x00, {0, 0, 0}, &em024_se_rec_mot125_b},
    {0x00, {0, 0, 0}, &em024_se_rec_mot125_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSePos em024_se_pos_mot202 = {76.0f, 142.0f, -1, -32768, 0};

EmSeRecord em024_se_rec_mot202 = {112, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot202[3] = {
    {0x0A, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em024_se_pos_mot202)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot202},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot0 = {107, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot0[2] = {
    {0x00, {0, 0, 0}, &em024_se_rec_mot0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em024_se_rec_mot5_a = {103, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em024_se_rec_mot5_b = {0, 0, 0, 0, 0, {0, 2, 1, 0}, &em024_se_shift};

EmSeRecord em024_se_rec_mot5_c = {104, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em024_se_rec_mot5_d = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em024_se_rec_mot5_e = {106, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em024_se_rec_mot5_f = {107, 0, 20, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em024_se_tbl_mot5[11] = {
    {0x23, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(em024_magma_burst)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x34)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot5_a},
    {0x01, {0, 0, 0}, &em024_se_rec_mot5_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em024_se_rec_mot5_c},
    {0x00, {0, 0, 0}, &em024_se_rec_mot5_d},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em024_se_rec_mot5_e},
    {0x00, {0, 0, 0}, &em024_se_rec_mot5_f},
    {0xFF, {0, 0, 0}, 0},
};

/* Starts the per-motion sound table that the motion selects (the default finishes the action). */
extern "C" void em024_act10_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0x1E);
        break;
    case 0x38:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x38);
        break;
    case 0x39:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x39);
        break;
    case 0x3A:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x3A);
        break;
    case 0x3B:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x3B);
        break;
    case 0x3C:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x3C);
        break;
    case 0x3D:
        em_se_tbl_play(self, em024_se_tbl_mot61, 0, 0x3D);
        break;
    case 0x3E:
        em_se_tbl_play(self, em024_se_tbl_mot61, 0, 0x3E);
        break;
    case 0x3F:
        em_se_tbl_play(self, em024_se_tbl_mot56, 0, 0x3F);
        break;
    case 0x23:
        em_se_tbl_play(self, em024_se_tbl_mot35, 3, 0x23);
        break;
    case 0x44:
        em_se_tbl_play(self, em024_se_tbl_mot35, 3, 0x44);
        break;
    case 0x64:
        em_se_tbl_play(self, em024_se_tbl_mot100, 1, 0x64);
        break;
    case 0x7A:
        em_se_tbl_play(self, em024_se_tbl_mot122, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em024_se_tbl_mot123, 0, 0x7B);
        break;
    case 0x7C:
        em_se_tbl_play(self, em024_se_tbl_mot124, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(self, em024_se_tbl_mot141, 0, 0x8D);
        break;
    case 0x7D:
        em_se_tbl_play(self, em024_se_tbl_mot125, 0, 0x7D);
        break;
    case 0x8E:
        em_se_tbl_play(self, em024_se_tbl_mot141, 0, 0x8E);
        break;
    case 0x84:
        em_se_tbl_play(self, em024_se_tbl_mot100, 1, 0x84);
        break;
    case 0x9F:
        em_se_tbl_play(self, em024_se_tbl_mot159, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em024_se_tbl_mot159, 0, 0xA0);
        break;
    case 0xA8:
        em_se_tbl_play(self, em024_se_tbl_mot23, 0, 0xA8);
        break;
    case 0xA9:
        em_se_tbl_play(self, em024_se_tbl_mot100, 1, 0xA9);
        break;
    case 0xC3:
        em_se_tbl_play(self, em024_se_tbl_mot195, 0, 0xC3);
        break;
    case 0xCA:
        em_se_tbl_play(self, em024_se_tbl_mot202, 0, 0xCA);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* Starts the sound table for motion 5, or the default one. */
extern "C" void em024_act11_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em024_se_tbl_mot0, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt(self, em024_se_tbl_mot5, 1, 5);
        break;
    default:
        em_se_tbl_play_alt(self, em024_se_tbl_mot0, 0, 0);
        break;
    }
}

EmKey1 em024_intro_curve_a[] = {
    {290.0f, -13940.56f}, {298.0f, -13868.82f}, {306.0f, -13781.72f},
    {314.0f, -13696.54f}, {322.0f, -13604.65f}, {330.0f, -13505.01f},
    {338.0f, -13411.04f}, {346.0f, -13328.46f}, {354.0f, -13245.44f},
    {362.0f, -13159.58f}, {370.0f, -13078.14f}, {378.0f, -12994.42f},
    {386.0f, -12915.05f}, {394.0f, -12836.63f}, {402.0f, -12754.56f},
    {410.0f, -12665.25f}, {418.0f, -12573.68f}, {426.0f, -12484.28f},
    {434.0f, -12400.51f}, {442.0f, -12325.79f}, {450.0f, -12260.18f},
    {458.0f, -12197.74f}, {466.0f, -12136.58f}, {474.0f, -12074.97f},
    {482.0f, -12012.73f}, {490.0f, -11947.19f}, {498.0f, -11865.82f},
    {506.0f, -11777.32f}, {514.0f, -11685.73f}, {-1.0f, -11685.73f},
};

EmKey2 em024_intro_curve_b[] = {
    {514.0f, -11685.73f, -75.42f}, {522.0f, -11583.47f, -75.58f}, {530.0f, -11454.31f, -74.14f},
    {538.0f, -11374.35f, -71.63f}, {546.0f, -11398.01f, -80.01f}, {550.0f, -11413.31f, -92.23f},
    {554.0f, -11428.5f, -105.03f}, {558.0f, -11443.65f, -113.12f}, {562.0f, -11458.85f, -113.8f},
    {566.0f, -11474.32f, -107.06f}, {570.0f, -11495.15f, -82.91f}, {574.0f, -11521.29f, -49.19f},
    {578.0f, -11551.11f, -25.34f}, {582.0f, -11579.52f, -13.3f}, {586.0f, -11604.41f, -7.19f},
    {590.0f, -11627.49f, -8.7f}, {594.0f, -11647.05f, -17.29f}, {602.0f, -11671.83f, -40.59f},
    {610.0f, -11683.91f, -66.53f}, {618.0f, -11678.06f, -94.47f}, {626.0f, -11662.31f, -126.23f},
    {634.0f, -11642.28f, -147.52f}, {638.0f, -11632.01f, -152.66f}, {642.0f, -11622.28f, -155.52f},
    {646.0f, -11613.63f, -156.06f}, {650.0f, -11606.27f, -154.22f}, {654.0f, -11599.71f, -149.96f},
    {662.0f, -11586.56f, -133.48f}, {670.0f, -11570.14f, -108.75f}, {678.0f, -11551.43f, -79.95f},
    {686.0f, -11525.13f, -49.64f}, {690.0f, -11506.39f, -31.54f}, {694.0f, -11480.61f, -9.77f},
    {698.0f, -11444.66f, 17.46f}, {702.0f, -11390.01f, 55.73f}, {710.0f, -11161.68f, 146.89f},
    {718.0f, -11027.57f, 238.74f}, {726.0f, -10985.94f, 317.61f}, {730.0f, -10996.24f, 314.99f},
    {734.0f, -11024.99f, 249.69f}, {736.0f, -11044.75f, 203.07f}, {740.0f, -11097.81f, 79.34f},
    {744.0f, -11169.58f, -116.86f}, {748.0f, -11253.93f, -323.13f}, {750.0f, -11298.21f, -382.52f},
    {752.0f, -11344.21f, -398.48f}, {754.0f, -11392.79f, -386.64f}, {756.0f, -11442.84f, -357.21f},
    {760.0f, -11542.87f, -273.83f}, {762.0f, -11593.0f, -226.29f}, {-1.0f, -11593.0f, -226.29f},
};

EmKey3 em024_intro_curve_c[] = {
    {762.0f, -11593.0f, 4260.0f, -226.29f}, {764.0f, -11639.02f, 4320.79f, -177.46f},
    {766.0f, -11639.45f, 5226.58f, -75.1f}, {770.0f, -11652.34f, 5357.55f, -5.96f},
    {774.0f, -11651.52f, 5479.13f, 30.11f}, {778.0f, -11638.31f, 5600.71f, 45.06f},
    {782.0f, -11614.32f, 5722.29f, 47.67f}, {786.0f, -11583.31f, 5773.68f, 39.4f},
    {790.0f, -11549.78f, 5773.68f, 17.7f}, {794.0f, -11515.85f, 5773.68f, -1.76f},
    {798.0f, -11487.85f, 5773.68f, -12.02f}, {806.0f, -11454.19f, 5773.68f, -23.88f},
    {814.0f, -11420.96f, 5773.68f, -35.61f}, {822.0f, -11390.23f, 5773.68f, -45.98f},
    {830.0f, -11363.46f, 5773.68f, -53.95f}, {838.0f, -11343.51f, 5773.68f, -57.96f},
    {846.0f, -11330.75f, 5773.68f, -57.31f}, {854.0f, -11323.26f, 5773.68f, -52.51f},
    {862.0f, -11320.94f, 5773.68f, -43.82f}, {870.0f, -11321.38f, 5773.68f, -34.73f},
    {878.0f, -11323.17f, 5773.68f, -25.82f}, {886.0f, -11325.21f, 5773.68f, -18.18f},
    {894.0f, -11480.38f, 5773.68f, -17.01f}, {902.0f, -12040.0f, 5790.37f, -17.01f},
    {910.0f, -12680.96f, 5914.3f, -17.01f}, {918.0f, -12825.69f, 6192.09f, -17.01f},
    {926.0f, -12781.72f, 6635.94f, -17.01f}, {934.0f, -12345.82f, 6772.55f, -17.01f},
    {942.0f, -11216.95f, 6453.93f, -17.01f}, {946.0f, -10532.53f, 6079.89f, -17.01f},
    {950.0f, -9597.76f, 5523.45f, -17.01f}, {954.0f, -9177.07f, 5380.81f, -17.01f},
    {-1.0f, -9177.07f, 5380.81f, -17.01f},
};

EmKey3 em024_intro_curve_d[] = {
    {954.0f, -2187.73f, 1695.27f, 2068.0f}, {982.0f, -2187.73f, 1695.27f, 2068.0f},
    {986.0f, -1863.72f, 1590.18f, 1657.41f}, {990.0f, -1527.07f, 1485.09f, 1227.58f},
    {994.0f, -1143.37f, 1380.0f, 726.17f}, {998.0f, -696.22f, 1274.91f, 128.24f},
    {1000.0f, -457.2f, 1181.37f, -194.21f}, {1002.0f, -212.19f, 1046.82f, -525.78f},
    {1004.0f, 35.63f, 912.27f, -861.63f}, {1006.0f, 283.06f, 845.0f, -1196.88f},
    {1010.0f, 795.06f, 845.0f, -1893.47f}, {1012.0f, 1053.64f, 845.0f, -2245.68f},
    {1014.0f, 1307.67f, 690.74f, -2545.27f}, {1016.0f, 1475.69f, 250.01f, -2670.26f},
    {-1.0f, 1475.69f, 250.01f, -2670.26f},
};

EmKey2 em024_intro_curve_e[] = {
    {1016.0f, 1475.69f, -2670.26f}, {1017.0f, 1492.58f, -2699.38f}, {1025.0f, 1608.1f, -2883.13f},
    {1033.0f, 1717.59f, -3038.25f}, {1041.0f, 1816.43f, -3188.62f}, {1049.0f, 1914.51f, -3337.83f},
    {1057.0f, 2001.66f, -3470.41f}, {1061.0f, 2037.96f, -3525.64f}, {1065.0f, 2067.51f, -3570.58f},
    {1069.0f, 2089.02f, -3603.31f}, {1073.0f, 2103.99f, -3626.09f}, {1077.0f, 2114.23f, -3641.66f},
    {1081.0f, 2121.53f, -3652.77f}, {1085.0f, 2127.7f, -3662.15f}, {1093.0f, 2142.23f, -3684.26f},
    {1101.0f, 2135.44f, -3673.93f}, {1109.0f, 2130.83f, -3666.92f}, {1117.0f, 2127.2f, -3661.39f},
    {1125.0f, 2125.15f, -3658.27f}, {1133.0f, 2123.94f, -3656.43f}, {1141.0f, 2123.3f, -3655.47f},
    {1149.0f, 2123.14f, -3655.23f}, {1157.0f, 2122.92f, -3656.21f}, {1165.0f, 2123.79f, -3659.38f},
    {1638.0f, 2123.79f, -3659.38f}, {-1.0f, 2123.79f, -3659.38f},
};

/* Action 13 sub-state 0: the scripted sequence, driven by the demo frame and the curve tables. */
extern "C" void em024_act13_sub0(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint_pos;

    VEC3_ctor(&pos);
    VEC3_ctor(&joint_pos);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x122) == 1) {
            self->state++;
            reinterpret_cast<void (*)(_ENEMY_WORK*, f32)>(em_fall_start)(self, 0.0f);
            em_demo_enable(self);
            em_mot_set(self, 2, 0, 0);
            em_demo_pos_set(self, -13940.56f, 4260.0f, -75.42f);
            em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_a, 0, 1, 0);
            em_demo_rot_set(self, 0.0f, 90.0f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(0x202) == 0)
            em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_a, 0, 1, 0);
        else
            em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_b, 0, 5, 0);
        if (em_demo_time_ck(0x208) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x24, 0x10, 0, 1);
        }
        break;
    case 3:
        if (em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 14, &joint_pos);
            joint_pos.y = 5.0f + self->pos.y;
            eft009_set_pos(0x85, &joint_pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 4.5f,
                           self->area_no);
        }
        if (em_after_frame_check(self, 0, 212.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &joint_pos);
            joint_pos.y = 5.0f + self->pos.y;
            eft009_set_pos(0x93, &joint_pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 1.3f,
                           self->area_no);
        }
        if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &joint_pos);
            joint_pos.y = 5.0f + self->pos.y;
            eft009_set_pos(0x88, &joint_pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 2.0f,
                           self->area_no);
        }
        if (em_demo_time_ck(0x2FA) == 0)
            em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_b, 0, 5, 0);
        else
            em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_c, 0);
        if (em_demo_time_ck(0x2FE) == 1) {
            self->state++;
            em_mot_set(self, 0x25, 0, 0);
        }
        break;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_c, 0);
        if (em_demo_time_ck(0x384) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x2A, 0x10, 0, 1);
        }
        break;
    case 5:
        em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_c, 0);
        if (em_demo_time_ck(0x3BA) == 1) {
            self->state++;
            em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_d, 0);
        }
        break;
    case 6:
        em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_d, 0);
        if (em_demo_time_ck(0x3D6) == 1) {
            self->state++;
            em_mot_set(self, 0x2A, 0, 0x14);
            em_demo_rot_set(self, 0.0f, 146.6816f, 0.0f);
        }
        break;
    case 7:
        em_demo_key3_apply(self, em_demo_frame_get(), em024_intro_curve_d, 0);
        if (em_demo_time_ck(0x3F8) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x2B, 0, 0);
        }
        break;
    case 8:
        em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_e, 0, 5, 0);
        if (em_demo_time_ck(0x480) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xC, 0xC, 0, 1);
        }
        break;
    case 9:
        em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_e, 0, 5, 0);
        if (em_demo_time_ck(0x506) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xC, 6, 0, 1);
        }
        break;
    case 10:
        em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_e, 0, 5, 0);
        if (em_demo_time_ck(0x58C) == 1) {
            self->state++;
            em_mot_set_blend(self, 1, 8, 0, 1);
        }
        break;
    case 11:
        em_demo_key_apply(self, em_demo_frame_get(), em024_intro_curve_e, 0, 5, 0);
        if (em_demo_time_ck(0x666) == 1) {
            self->state++;
            em_mot_set(self, 0x20, 0, 0);
            em_demo_pos_set(self, -84.7694f, 250.0058f, -299.5185f);
            em_demo_rot_set(self, 0.0f, 146.6816f, 0.0f);
            self->timer_0x020 = 0;
        }
        break;
    case 12: {
        s16 frame;

        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1)
            draw_shape_arm((u32)self, 0x16, 0xA);
        setVector3(&joint_pos, 0.0f, -30.0f, 100.0f);
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1)
            eft_em_spawn(self, 0, 0x15, &joint_pos, 1.0f);
        if (em_frame_check(self, 3, 56.0f, 138.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0)
                eft_em_spawn(self, 1, 0x15, &joint_pos, 1.0f);
            self->timer_0x020++;
        }
        frame = em_demo_frame_get() * 2;
        if (frame == 0x684) {
            setVector3(&pos, 1640.0f, 0.0f, -460.0f);
            shell_set_func_ptr->method_0x8C(self, 0, &pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (frame == 0x698) {
            setVector3(&pos, 700.0f, 0.0f, -3740.0f);
            shell_set_func_ptr->method_0x8C(self, 0, &pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (frame == 0x6AC) {
            setVector3(&pos, 3230.0f, 0.0f, -4220.0f);
            shell_set_func_ptr->method_0x8C(self, 0, &pos, 1.0f, 60, shell_set_func_ptr);
        }
        if (em_demo_time_ck(0x7C8) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set_blend(self, 1, 0x10, 0, 1);
        }
        break;
    }
    }
}

/* Action 13 sub-state 1: plays motion 1 with the demo pose and finishes when it ends. */
extern "C" void em024_act13_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -84.7694f, 250.0058f, -299.5185f);
        em_demo_rot_set(self, 0.0f, 146.6816f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Picks the action-13 step from the sub-state. */
extern "C" void em024_act13_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em024_act13_sub0(self);
        break;
    case 1:
        em024_act13_sub1(self);
        break;
    }
}

extern "C" void em024_action_dispatch(_ENEMY_WORK* self)
{
    switch (self->action) {
    case 0:
        em024_act0_dispatch(self);
        break;
    case 1:
        em024_act1_dispatch(self);
        break;
    case 2:
        em024_act2_dispatch(self);
        break;
    case 3:
        em024_act3_dispatch(self);
        break;
    case 4:
        em024_act4_dispatch(self);
        break;
    case 7:
        em024_act7_dispatch(self);
        break;
    case 10:
        em024_act10_dispatch(self);
        break;
    case 11:
        em024_act11_dispatch(self);
        break;
    case 13:
        em024_act13_dispatch(self);
        break;
    }
}

/* Arms the six alternate part records when the state becomes 2 and restores the defaults when it leaves. */
extern "C" void em024_part_lock_sync(_ENEMY_WORK* self)
{
    if (self->field_0x1E4 == 2) {
        if (self->em024_0x328.part_lock_0x328 == 0) {
            em_part_rec_alt_set(self, 0, 0);
            em_part_rec_alt_set(self, 1, 1);
            em_part_rec_alt_set(self, 2, 2);
            em_part_rec_alt_set(self, 3, 3);
            em_part_rec_alt_set(self, 4, 4);
            em_part_rec_alt_set(self, 5, 5);
            self->em024_0x328.part_lock_0x328 = 1;
        }
    } else if (self->em024_0x328.part_lock_0x328 == 1) {
        em_part_rec_reset(self, 0);
        em_part_rec_reset(self, 1);
        em_part_rec_reset(self, 2);
        em_part_rec_reset(self, 3);
        em_part_rec_reset(self, 4);
        em_part_rec_reset(self, 5);
        self->em024_0x328.part_lock_0x328 = 0;
    }
}

/* Every 24 frames, while the mode flag is set, spawns the aura effect that the state selects. */
extern "C" void em024_gauge_aura_fx(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) == 1 && system_w.field_0x0c % 24 == 0) {
        setVector3(&pos, 0.0f, -20.0f, 100.0f);
        if (self->field_0x1E4 == 2)
            eft_spawn_type10(self, 0x1B, 0x15, &pos, 1.0f);
        else
            eft_spawn_type10(self, 0x1A, 0x15, &pos, 1.0f);
    }
}

/* Spawns effect `id` for the work in one of three modes, at `joint` (or the position when it is 0xFF). */
extern "C" void em024_effect_spawn(_ENEMY_WORK* self, u8 mode, u8 id, u32 joint, s32 arg, f32 scale)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (mode) {
    case 0:
        if (joint == 0xFF) {
            pos.x = self->pos.x;
            pos.y = 5.0f + self->field_0x20C;
            pos.z = self->pos.z;
            eft009_set_pos(id, &pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, id, arg, scale);
        }
        break;
    case 1:
        if (joint == 0xFF)
            copyVec3(&pos, &self->pos);
        else
            get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, id, arg, scale);
        break;
    case 2:
        if (joint == 0xFF)
            copyVec3(&pos, &self->pos);
        else
            get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        if (self->pos.y <= 900.0f + self->field_0x20C)
            eft_spawn_type11(self, &pos, id, scale);
        break;
    }
}

extern "C" void em024_motion_events(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 unused_pos;
    nw4r::math::VEC3 pos;
    u16 mot;

    VEC3_ctor(&unused_pos);
    VEC3_ctor(&pos);
    em024_part_lock_sync(self);
    em024_gauge_aura_fx(self);
    mot = em_get_mot_no(self);
    switch (mot) {
    case 0x2:
        if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0x2C, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 192.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0x27, 0, 1.5f);
        }
        break;
    case 0x3:
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        break;
    case 0x4:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        break;
    case 0x5:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 2.0f);
        }
        break;
    case 0x6:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0x12);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
            em_camera_req(self, 0xE, 7);
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0x13);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 3.0f);
            em_camera_req(self, 9, 7);
        }
        break;
    case 0x8:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x9:
        if ((system_w.field_0x0c & 7) == 0) {
            em024_effect_spawn(self, 2, 1, 3, 0, 1.3f);
        }
        break;
    case 0xA:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 1, 3, 0, 1.3f);
        }
        if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
            em_camera_req(self, 3, 0);
        }
        break;
    case 0xB:
        if ((em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x11:
        if ((em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0x2C, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 150.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
            em_camera_req(self, -1, 0);
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0x27, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 162.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 0, 0x27, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 0, 0x2C, 0, 1.5f);
        }
        break;
    case 0x12:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 0, 0x27, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 0, 0x2C, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
            em_camera_req(self, -1, 0);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        break;
    case 0x14:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
            em_camera_req(self, 0xE, 7);
        }
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0x27, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 182.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
        }
        break;
    case 0x15:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 5, 3, 0, 1.0f);
        }
        break;
    case 0x16:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x17:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x1B:
        if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 174.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x20:
        if (em_after_frame_check(self, 0, 164.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 0x11, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
            em_camera_req(self, 0x11, 1);
        }
        break;
    case 0x21:
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 4, 0xE, 0, 2.5f);
            em_camera_req(self, 0xE, 1);
        }
        if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
        }
        if (em_after_frame_check(self, 0, 256.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 0x11, 0, 1.2f);
            em_camera_req(self, 0x11, 1);
        }
        break;
    case 0x22:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 3.0f);
        }
        if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0x2C, 0, 3.0f);
        }
        if ((em_after_frame_check(self, 1, 50.0f, 0.0f) == 1) && ((system_w.field_0x0c & 0xF) == 0)) {
            setVector3(&pos, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 0xB5, 0x15, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_camera_req(self, 0x15, 1);
        }
        break;
    case 0x24:
        if (em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
            em_camera_req(self, 0xE, 1);
        }
        if (em_after_frame_check(self, 0, 212.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 3, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 232.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 5, 3, 0, 1.0f);
        }
        break;
    case 0x26:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            if (em_magma_check(self) == 1) {
                copyVec3(&pos, &self->pos);
                pos.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x88, &pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 1.5f, self->area_no);
            } else {
                em024_effect_spawn(self, 1, 4, 3, 0, 1.2f);
            }
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x27:
        if (em_after_frame_check(self, 0, 204.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 5, 3, 0, 1.3f);
        }
        break;
    case 0x29:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 5, 3, 0, 1.0f);
        }
        break;
    case 0x2B:
        if (em_magma_check(self) == 1) {
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 3, &pos);
                pos.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x88, &pos, reinterpret_cast<_CP_VECTOR*>(&self->field_0x1BC), 1.5f, self->area_no);
            }
        } else if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 0x11, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x2D:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 218.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        break;
    case 0x2F:
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 2.0f);
        }
        break;
    case 0x30:
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 188.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x31:
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x32:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x35:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x37:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em_camera_req(self, 0xE, 5);
        }
        break;
    case 0x39:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            u32 part;

            if (em_act_ck(self, 7, 0x12) != 0 || em_act_ck(self, 7, 0x14) != 0 || em_act_ck(self, 7, 0x1C) != 0)
                part = 0x1D;
            else
                part = 6;
            em_hit_window_set(self, 0, part, 8);
            em_hit_window_set(self, 1, 7, 0x10);
        }
        break;
    case 0x3A:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            u32 part;

            if (em_act_ck(self, 7, 0x13) != 0 || em_act_ck(self, 7, 0x15) != 0 || em_act_ck(self, 7, 0x1C) != 0)
                part = 0x1C;
            else
                part = 4;
            em_hit_window_set(self, 0, part, 8);
            em_hit_window_set(self, 1, 5, 0x10);
        }
        break;
    case 0x3B:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x3C:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x46:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 5, 3, 0, 1.0f);
        }
        break;
    case 0x47:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x48:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x49:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 3, 0, 1.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x64:
        if ((em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 2.0f);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 3.0f);
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
            em_camera_req(self, 0x11, 7);
        }
        break;
    case 0x66:
        if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 2, 0, 3, 0, 1.0f);
        }
        break;
    case 0x68:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 5, 3, 0, 1.2f);
            em_camera_req(self, 3, 5);
        }
        break;
    case 0x69:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 3, 0, 1.5f);
            em_camera_req(self, 3, 1);
        }
        break;
    case 0x6A:
        if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            em024_effect_spawn(self, 0, 2, 9, 0, 3.0f);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 204.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 9, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 420.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 1, 3, 0, 1.5f);
            em_camera_req(self, 3, 5);
        }
        break;
    case 0x6E:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 2.5f);
        }
        if (em_after_frame_check(self, 0, 194.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 0, 3, self->field_0x1C0, 2.5f);
            em_camera_req(self, 3, 1);
        }
        break;
    case 0x6F:
        if (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 2, 0, 3, self->field_0x1C0, 1.0f);
            em_camera_req(self, 3, 5);
        }
        break;
    case 0x70:
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 1.5f);
            em_camera_req(self, 3, 5);
        }
        break;
    case 0x72:
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 1, 4, 3, 0, 1.0f);
            em_camera_req(self, 3, 1);
        }
        if (em_after_frame_check(self, 0, 184.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 2, 0xE, 0, 3.0f);
            em_camera_req(self, 3, 1);
        }
        if (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            em024_effect_spawn(self, 0, 4, 9, 0, 2.5f);
        }
        break;
    }
    if (self->field_0x1E4 == 1 && (system_w.field_0x0c & 0x1F) == 0) {
        setVector3(&pos, 0.0f, 90.0f, 0.0f);
        eft_em_spawn(self, 0xBC, 0x15, &pos, 1.0f);
    }
    if ((u32)em_act_ck(self, 0xB, 5) == 1 && em_get_mot_no(self) == 0x68 &&
        em_after_frame_check(self, 2, 2.0f, 0.0f) == 1) {
        camera_event_set(0x42, (u32)self);
        camera_shake_req(3);
    }
}

extern "C" void em024_tev_color_update(_ENEMY_WORK* self)
{
    _GXColor color;

    if (self->field_0x1E4 == 2) {
        if (self->em024_0x328.tev_ramp_0x329 == 1) {
            self->em024_0x328.tev_color_0x334.x += 0.88f;
            if (self->em024_0x328.tev_color_0x334.x > 88.0f)
                self->em024_0x328.tev_color_0x334.x = 88.0f;
            self->em024_0x328.tev_color_0x334.y += 1.54f;
            if (self->em024_0x328.tev_color_0x334.y > 154.0f)
                self->em024_0x328.tev_color_0x334.y = 154.0f;
            self->em024_0x328.tev_color_0x334.z += 1.32f;
            if (self->em024_0x328.tev_color_0x334.z > 132.0f)
                self->em024_0x328.tev_color_0x334.z = 132.0f;
            self->em024_0x328.tev_color2_0x340.x += 1.48f;
            if (self->em024_0x328.tev_color2_0x340.x > 148.0f)
                self->em024_0x328.tev_color2_0x340.x = 148.0f;
            self->em024_0x328.tev_color2_0x340.y += 2.14f;
            if (self->em024_0x328.tev_color2_0x340.y > 214.0f)
                self->em024_0x328.tev_color2_0x340.y = 214.0f;
            self->em024_0x328.tev_color2_0x340.z += 1.92f;
            if (self->em024_0x328.tev_color2_0x340.z > 192.0f)
                self->em024_0x328.tev_color2_0x340.z = 192.0f;
        } else {
            self->em024_0x328.tev_color_0x334.x -= 2.55f;
            if (self->em024_0x328.tev_color_0x334.x < 0.0f)
                self->em024_0x328.tev_color_0x334.x = 0.0f;
            self->em024_0x328.tev_color_0x334.y -= 0.65f;
            if (self->em024_0x328.tev_color_0x334.y < 0.0f)
                self->em024_0x328.tev_color_0x334.y = 0.0f;
            self->em024_0x328.tev_color_0x334.z -= 0.3f;
            if (self->em024_0x328.tev_color_0x334.z < 0.0f)
                self->em024_0x328.tev_color_0x334.z = 0.0f;
            self->em024_0x328.tev_color2_0x340.x = self->em024_0x328.tev_color_0x334.x;
            self->em024_0x328.tev_color2_0x340.y = self->em024_0x328.tev_color_0x334.y;
            self->em024_0x328.tev_color2_0x340.z = self->em024_0x328.tev_color_0x334.z;
            if (0.0f == self->em024_0x328.tev_color2_0x340.x && 0.0f == self->em024_0x328.tev_color2_0x340.y
                && 0.0f == self->em024_0x328.tev_color2_0x340.z)
                self->em024_0x328.tev_ramp_0x329 = 1;
        }
    } else if (self->em024_0x328.tev_ramp_0x329 == 1) {
        self->em024_0x328.tev_color_0x334.x -= 0.88f;
        if (self->em024_0x328.tev_color_0x334.x < 0.0f)
            self->em024_0x328.tev_color_0x334.x = 0.0f;
        self->em024_0x328.tev_color_0x334.y -= 1.54f;
        if (self->em024_0x328.tev_color_0x334.y < 0.0f)
            self->em024_0x328.tev_color_0x334.y = 0.0f;
        self->em024_0x328.tev_color_0x334.z -= 1.32f;
        if (self->em024_0x328.tev_color_0x334.z < 0.0f)
            self->em024_0x328.tev_color_0x334.z = 0.0f;
        self->em024_0x328.tev_color2_0x340.x -= 1.48f;
        if (self->em024_0x328.tev_color2_0x340.x < 0.0f)
            self->em024_0x328.tev_color2_0x340.x = 0.0f;
        self->em024_0x328.tev_color2_0x340.y -= 2.14f;
        if (self->em024_0x328.tev_color2_0x340.y < 0.0f)
            self->em024_0x328.tev_color2_0x340.y = 0.0f;
        self->em024_0x328.tev_color2_0x340.z -= 1.92f;
        if (self->em024_0x328.tev_color2_0x340.z < 0.0f)
            self->em024_0x328.tev_color2_0x340.z = 0.0f;
        if (0.0f == self->em024_0x328.tev_color_0x334.x && 0.0f == self->em024_0x328.tev_color_0x334.y
            && 0.0f == self->em024_0x328.tev_color_0x334.z)
            self->em024_0x328.tev_ramp_0x329 = 0;
    } else {
        self->em024_0x328.tev_color_0x334.x += 2.55f;
        if (self->em024_0x328.tev_color_0x334.x > 255.0f)
            self->em024_0x328.tev_color_0x334.x = 255.0f;
        self->em024_0x328.tev_color_0x334.y += 0.65f;
        if (self->em024_0x328.tev_color_0x334.y > 65.0f)
            self->em024_0x328.tev_color_0x334.y = 65.0f;
        self->em024_0x328.tev_color_0x334.z += 0.3f;
        if (self->em024_0x328.tev_color_0x334.z > 30.0f)
            self->em024_0x328.tev_color_0x334.z = 30.0f;
        self->em024_0x328.tev_color2_0x340.x = self->em024_0x328.tev_color_0x334.x;
        self->em024_0x328.tev_color2_0x340.y = self->em024_0x328.tev_color_0x334.y;
        self->em024_0x328.tev_color2_0x340.z = self->em024_0x328.tev_color_0x334.z;
    }
    color.r = self->em024_0x328.tev_color2_0x340.x;
    color.g = self->em024_0x328.tev_color2_0x340.y;
    color.b = self->em024_0x328.tev_color2_0x340.z;
    color.a = 0xFF;
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    color.r = self->em024_0x328.tev_color_0x334.x;
    color.g = self->em024_0x328.tev_color_0x334.y;
    color.b = self->em024_0x328.tev_color_0x334.z;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR3, &color);
    if (em_flags836_ck(self, 1) != 1)
        ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &color);
    {
        u32 rim_a;
        u32 body_a;
        u32 rim_b;
        u32 body_b;
        s32 out_rim;
        s32 out_body;

        if (em_parts_damage_level_get(self, 4) < 2) {
            rim_a = 100;
            body_a = 255;
        } else {
            rim_a = 200;
            body_a = 255;
        }
        out_rim = (s32)((f32)rim_a * self->field_0x1D4);
        out_body = (s32)((f32)body_a * self->field_0x1D4);
        mhchar_mat_tev_set((MHchar*)self->char_0x024, 2, 6, out_rim, 0, 3, out_body, self->field_0x1D4);
        if (em_parts_damage_level_get(self, 3) < 2) {
            rim_b = 100;
            body_b = 255;
        } else {
            rim_b = 100;
            body_b = 220;
        }
        out_rim = (s32)((f32)rim_b * self->field_0x1D4);
        out_body = (s32)((f32)body_b * self->field_0x1D4);
        mhchar_mat_tev_set((MHchar*)self->char_0x024, 3, 6, out_rim, 0, 3, out_body, self->field_0x1D4);
    }
}
