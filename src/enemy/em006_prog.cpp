/*
 * enemy/em006_prog.cpp - enemy 006's program: the `_ENEMY_WORK` action steps and the static initializer that
 *   seeds the six vector records.
 * RANGE. .text 0x801BB758-0x801C29F8 (74 functions); extab 0x8000F9A4-0x8000FB84, extabindex
 *   0x8002B560-0x8002B830, .ctors 0x8056F350-0x8056F354 (`fn_801C28FC`), .rodata 0x805702A0-0x80570320, .data
 *   0x805B28D0-0x805B3B7C (`em006_prog_tbl` first), .bss 0x806A7AD0-0x806A7B18, .sdata 0x80791AB8-0x80791AD8,
 *   .sdata2 0x80798E40-0x80798FE8.  Left edge: the 0.0 pool entry repeats at `lbl_80798E40`, first read by
 *   `fn_801BB758`; right edge: `fn_801C28FC` is this TU's static initializer.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`.
 * NAMES. The file name follows the runtime dump's `em006_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.
 *   The `.bss` record names (`vec_pair_801BD6C0_*`) are GUESSes.
 * RESIDUALS. 63 rows unwritten: 0x801BB758-0x801BD6C0, 0x801BD990-0x801BDA28, 0x801BDAC4-0x801BDEF8,
 *   0x801BE020-0x801BE198, 0x801BE348-0x801BE508, 0x801BE65C-0x801BEFD4, 0x801BEFEC-0x801C2508,
 *   0x801C2534-0x801C29F8.  No partial row: the 11 written rows match.
 *   flipcheck: `.ctors`/`.data`/`.rodata`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x7DC of 0x72A0),
 *   extab (0x38 of 0x1E0) and extabindex (0x54 of 0x2D0) short of the claim and differing.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "sound/fn_800D7F54.h"
#include "sound/se.h"
#include "unsplit/ef.h"
#include "ef/eft009.h"
#include "draw_shape.h"
#include "fn_8004CAD8.h"

/* The unit's `.sdata2` pool floats the state steps compare and arm with, declared: the source does not
 * emit the pool yet. */
extern f32 lbl_80798E40;
extern f32 lbl_80798E54;
extern f32 lbl_80798E58;

extern f32 lbl_80798E7C;

extern f32 lbl_80798E90;

extern f32 lbl_80798EA4;
extern f32 lbl_80798EA8;

extern f32 lbl_80798ECC;
extern f32 lbl_80798ED0;

extern f32 lbl_80798ED8;

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */
/* 0x801BD6C0 - step 4 of the "em" action: a two-state move     */
/* ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801BD6C0(_ENEMY_WORK* self, u8 arg1)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0;
        break;
    case 1:
        if ((self->timer_0x020 & 0x1F) == 0) {
            u32 type;

            em_camera_req(self, -1, 5);
            get_joint_wpos_em(self, 3, &pos);
            if ((self->field_0x228 & 6) != 0) {
                type = 0x54;
                pos.y = lbl_80798EA4 + self->field_0x210;
            } else {
                type = 0x53;
                pos.y = lbl_80798EA4 + self->field_0x20C;
            }
            eft009_spawn_at_joint(self, 3, type, 0, lbl_80798E58);
            if (self->area_no == get_now_areano()) {
                se_req_pos_ps(self->se_0xB14, 0x27, 2, &pos);
            }
        }
        {
            s32 limit = arg1 == 1 ? 0x50 : 0x2D;

            if (++self->timer_0x020 >= limit) {
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x801BD838 - step 8 of the "em" action: a seated wait        */
/* ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801BD838(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 0, 0);
        em_mot_speed_set(self, lbl_80798E90);
        em_hit_window_set(self, 0, 0x21, 2);
        em_approach_start(self, lbl_80798EA8, 0);
        fn_801303EC(self, fn_8013032C(self));
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
            fn_801280F4(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* the per-action state steps the dispatchers below drive                                              */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BDA28 - action 0x2F: a two-state motion hand-off (one 0..1 branch). */
extern "C" void fn_801BDA28(_ENEMY_WORK* self)
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

/* 0x801BDEF8 - action 0xD1: the same shape with the 6/0x18 motion set. */
extern "C" void fn_801BDEF8(_ENEMY_WORK* self)
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

/* 0x801BDF94 - action 0xD2: the same shape with the single 0..7 motion set. */
extern "C" void fn_801BDF94(_ENEMY_WORK* self)
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

/* 0x801BE508 - the seated en/disable step (0xCE): three window timers and the finish gate. */
extern "C" void fn_801BE508(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_hit_window_set_default(self, 0, 0x16);
        fn_801303EC(self, fn_8013032C(self));
        fn_80130CDC(self, -0xA);
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_shake_req_set(self);
        fn_80131E00(self);
        break;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, lbl_80798E54, lbl_80798E40) == 1) {
            fn_80131E00(self);
        }
        if (em_frame_check(self, 2, lbl_80798ED8, lbl_80798E40) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 0, lbl_80798E58, lbl_80798E40) == 1) {
            em_move_mode_set(self, 0);
            fn_801303EC(self, lbl_80798E40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BE198 - action 0xD4: the three-state seated step. */
extern "C" void fn_801BE198(_ENEMY_WORK* self, u8 arg1)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD4, 6, 0);
        fn_80130CDC(self, -6);
        em_hit_window_set(self, 0, 9, 8);
        em_hit_window_set(self, 1, 0x29, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798ECC, lbl_80798E40) == 1) {
            em_hit_window_set(self, 0, 0xA, 0x10);
        }
        if (arg1 == 1) {
            if (em_frame_check(self, 0, lbl_80798ED0, lbl_80798E40) == 1) {
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
        if (em_frame_check(self, 0, lbl_80798E7C, lbl_80798E40) == 1) {
            em_hit_window_set(self, 0, 0x1B, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BEFD4 - a 4-byte `b fn_801BE508` tail entry. */
extern "C" void fn_801BEFD4(_ENEMY_WORK* self)
{
    fn_801BE508(self);
}

/* 0x801BEFD8 - the same entry guarded by `state_sub == 0`. */
extern "C" void fn_801BEFD8(_ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        fn_801BEFD4(self);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* the 0x801C25xx predicate helpers the action band reads                                              */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801C2508 - a 4-byte `blr`. */
extern "C" void fn_801C2508(void)
{
}

/* 0x801C250C - `(arg1 == 0) && (self->+0x328 == 0)` (the target reads the byte, not a word: it
 * loads +0x328 with `lbz`). */
extern "C" s32 fn_801C250C(_ENEMY_WORK* self, u8 arg1)
{
    if (arg1 == 0 && self->init_0x328.slots_0x328[0] == 0) {
        return 1;
    }
    return 0;
}

/* The unit's `.bss`: the three two-vector records `fn_801C28FC` seeds.  Names are GUESSes (each record is a
 * pair of model-space points). */
VEC3 vec_pair_801BD6C0_0[2];  /* +0x806A7AD0 */
VEC3 vec_pair_801BD6C0_1[2];  /* +0x806A7AE8 */
VEC3 vec_pair_801BD6C0_2[2];  /* +0x806A7B00 */
