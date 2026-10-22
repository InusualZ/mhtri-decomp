/*
 * enemy/em019_ai.cpp - enemy 019's AI: the program table `em019_prog_tbl` and its entries (work clear, init,
 *   frame update, action run, motion follow-up, event notify, motion events, colour update, state query, death
 *   action, part/stand/area checks), the sub-state steps of actions 0, 1, 2, 3, 6 and 7 with their dispatchers,
 *   the sound-table players of actions 10/11, the program steps `em_act_prog_0`..`em_act_prog_8`, the part
 *   colour helpers, the user-data helper's part reset and destructor, and the sound-offset initializer, with the
 *   unit's sound tables, demo key tables and float pool.
 * RANGE. .text 0x80378F9C-0x80383148 (92 functions); extab 0x80017C44-0x80017E8C, extabindex
 *   0x800378CC-0x80037C38, .ctors 0x8056F3A8-0x8056F3AC, .rodata 0x80570AE0-0x80570B20, .data 0x805EE518-0x805EF8C0
 *   (`em019_prog_tbl` first), .bss 0x806C23E8-0x806C2418, .sdata 0x807933B8-0x807933C0, .sbss
 *   0x80794BF8-0x80794C00, .sdata2 0x8079BC88-0x8079BF60.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi` + compare pairs) and
 *   `#pragma pool_data off` (each sound table is addressed with its own `lis`/`addi`: pooled, the two players
 *   read 78 and 81).
 * NAMES. The file name follows the runtime dump's `em019_prog_tbl`; the `_ai` suffix is a GUESS.  Every function,
 *   table and field name defined here is a GUESS from its body, its callers, its `em019_prog_tbl` slot or the
 *   data that points at it (the dump answers `zz_` or a linker-folded duplicate's name); `em019_act<N>_*` steps
 *   are named by the action id `em_act_run` dispatches on.  Where a callee's owner header declares another
 *   signature than retail's call, the call goes through a cast view (`_c1`/`_c2`).
 * RESIDUALS. Unwritten: 0x80379210 (0x3C, the user-data constructor: it stores the vtable 0x805EF890 by hand,
 *   which needs the `ResUserDataAc` base as a C++ class to emit).  Partial rows:
 *  - `em019_motion_next`: retail tests the 31..32 cases as `subi` + `cmplwi 1`; ours as `cmpwi 31`/`blt` +
 *    `cmpwi 32`/`ble` (every case order measured);
 *  - `em019_act7_rush`: retail keeps the side-step angle in r31 and multiplies `ratio * scale` as (f0, f31); ours
 *    r29 and (f31, f0);
 *  - `em019_act7_jump`: retail loads +0x1C0 after the angle conversion in the `+=`; ours before it;
 *  - `em019_spawn_point_pick`: retail narrows `first` before the loop and colours i/first r28/r30, ours r30/r28
 *    (every declaration order measured);
 *  - `em019_area_change_ck`: retail re-narrows the second `map == 5 ? 9 : 0xFF` with `clrlwi`, ours does not.
 *   Data: the vtable 0x805EF890 (0x30) and its 4-byte gap are not emitted (constructor above); `.ctors` is not
 *   emitted (`em019_se_shift_init` is a plain function: `setVec3`/`assignVec3` are C-linkage helpers, not a
 *   constructor pair); the map splits `em019_se_shift` at 0x806C2400 and `em019_se_ofs_mot28` at 0x805EE810 (one
 *   object each in retail, requests filed); `.sdata2` is 0x2D4 of the claimed 0x2D8 (a trailing pad word).
 * SHAPES. The `ShellSetFuncs` +0x2C/+0x3C/+0x74 calls go through spellings without the trailing table argument:
 *   the shared slot types hoist the table load.  The turn angles are `u32` with `-0x6000`/`-0x2000` constants
 *   (retail's `li`), a `u8` compared through `(s32)` gives retail's `cmpwi`, the colour pulses add a `(u8)`
 *   conversion and read the meter through an `f32*`.
 */

#include "types.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/EM_PART_BLOCK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef/fn_80105314.h"
#include "fn_8004CAD8.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */
#include "enemy/em_mot_finished_ck.h"
#include "enemy/em019_action13_active_ck.h"
#include "lobby/fn_8030121C.h"
#include "draw_shape.h"
#include "sys_mem.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "ef/fn_8010D1A8.h"
#include "ef/fn_8030681C.h"
#include "stage/shell_set_func_ptr.h"
#include "mh3_pad/system_w.h"
#include "g3d/g3d_calcworld.h"
#include "fn_80047398.h"
#include "stage/stg_w.h"
#include "enemy/em_model.h" /* em_drop_item_spawn */
#include "enemy/em_tint_hold_start.h"
#include "enemy/em_tint_hold_end.h"

#pragma peephole off
#pragma pool_data off

/* A 6-argument spelling of `ShellSetFuncs`' +0x2C slot: retail passes no table argument, and the shared
 * 7-argument type hoists the table load (`em_act_prog_5` scores lower). */
typedef void (*ShellJobInjectFn)(struct _ENEMY_WORK* self, s32 a, s32 b, nw4r::math::VEC3* pos, f32 scale,
                                 u16 flags);
/* The +0x74 and +0x3C slots spelled without their trailing table argument, for the same reason. */
typedef void (*ShellPartJobFn)(struct _ENEMY_WORK* self, u32 kind, u32 joint, nw4r::math::VEC3* pos, f32 scale,
                               u32 a, u16 b);
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);
/* The `em_turn_seq_start`/`em_turn_seq_step` table (0x40 B): a four-word head, then three four-word rows whose
 * first word carries the effect id in its high half. */
extern "C" const u32 em019_turn_tbl[16] = {
    0x0000000A, 0x00000032, 0x00000000, 0x5C722AAB,
    0x00160000, 0x00000006, 0x00000000, 0x00000000,
    0x00180000, 0x0000000A, 0x00000000, 0x00000000,
    0x00190000, 0x0000000A, 0x00000000, 0x00000000,
};

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet. */

/* `setVec3` through the pointer-returning signature its body has (the shared declaration returns `void`,
 * the call site consumes the result). */
typedef nw4r::math::VEC3* (*Fn80041E8C)(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);


/* `em_ground_ck` through the signature its body has: it reads the height margin in f1 (the shared
 * declaration takes none). */
typedef u32 (*EmGroundCkFn)(struct _ENEMY_WORK* self, f32 margin);

/* One 0x0C-byte sound position record a kind 0x0A sound-table entry points at (`EmSePos` without its pad word).
 * size: 0x0C */
struct Em019SePos {
    /* +0x00 */ f32 x_0x00;
    /* +0x04 */ f32 y_0x04;
    /* +0x08 */ s16 channel_0x08;
    /* +0x0A */ s16 angle_0x0A;
};

/* The two counters the kind 0x16 sound-table entries step, and the 8-byte loop record the 0xFE terminators of
 * the motion 52/178 tables point at (GUESS roles from the entry kinds). */
u32 em019_se_count_a;
u32 em019_se_count_b;
u32 em019_se_loop[2] = {0x0AE40000, 0};

/* The two sound-offset vector pairs `em019_se_shift_init` seeds (the unit's `.bss`; one object, the sound
 * tables point at +0x00 and +0x18). */
Vec em019_se_shift[4];

/* `em_area_break_ck` with the area argument retail passes in r4 (the owner headers declare `self` only). */
#define em_area_break_ck_c2 ((u8 (*)(struct _ENEMY_WORK*, u8))em_area_break_ck)
/* the follow pick and the leave test through the byte/signed views this unit's calls use. */
#define em_pack_area_pick_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8, u16, u8*, u8*))em_pack_area_pick)
#define em_area_leave_ck_c1 ((s32 (*)(struct _ENEMY_WORK*))em_area_leave_ck)
/* the user-data flag setter and destructor over this unit's `EmUserData` view. */
#define em_res_user_data_flag_set_c1 ((void (*)(EmUserData*, u32))em_res_user_data_flag_set)
#define em_res_user_data_dtor_c1 ((void (*)(EmUserData*, s16))em_res_user_data_dtor)

extern "C" {

/* This unit's own functions, in address order (the unwritten ones declared only). */
void em019_part_state_set(_ENEMY_WORK* self, u8 mask);
u8 em019_part_state_get(_ENEMY_WORK* self, u8 part);
EmUserData* em019_user_data_ctor(EmUserData* self);
void em019_act2_dispatch(_ENEMY_WORK* self);
void em019_act7_dispatch(_ENEMY_WORK* self);
void em019_act10_se_play(_ENEMY_WORK* self);
void em019_act11_se_play(_ENEMY_WORK* self);
void em019_action_start_if_idle(_ENEMY_WORK* self);
void em_act_prog_0(_ENEMY_WORK* self);
void em019_spawn_rec_set(EmSpawnRec* rec, u8 mode, u16 a, u16 angle);
void em_act_prog_dispatch(_ENEMY_WORK* self);
void em_parts_damage_ck(_ENEMY_WORK* self);
void em_act_effect_ck(_ENEMY_WORK* self);
void em019_part_flag_set(_ENEMY_WORK* self, u32 flag);
u32 em019_ally_wait_ck(_ENEMY_WORK* self, u8 near);
void em_move_mode_set(_ENEMY_WORK* self, u32 a);

void em019_work_clear(_ENEMY_WORK* self);
void em019_init(_ENEMY_WORK* self, u8 mode);
void em019_frame_update(_ENEMY_WORK* self);
void em_act_run(_ENEMY_WORK* self);
void em019_motion_next(_ENEMY_WORK* self, u8* motion, u8* next);
void em019_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub);
void em_act_mot_step(_ENEMY_WORK* self);
void em019_tev_color_update(_ENEMY_WORK* self);
u8 em019_state_query(_ENEMY_WORK* self, u8 kind);
void em019_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub);
void em019_noop(void);
u32 em019_part7_damaged_ck(_ENEMY_WORK* self);
s32 em019_stand_ck(_ENEMY_WORK* self);
u32 em019_area_change_ck(_ENEMY_WORK* self, u16 timer);

/* The em019 program table (`em019_prog_tbl`): the entry points the shared enemy program driver calls.  Slot
 * names are a GUESS from the functions they hold.
 * size: 0x6C */
struct Em019ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(_ENEMY_WORK* self, u8* motion, u8* next);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u8 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*noop_0x34)(void);
    /* +0x38 */ u32 pad_0x38;
    /* +0x3C */ u32 (*part_damaged_ck_0x3C)(_ENEMY_WORK* self);
    /* +0x40 */ u32 pad_0x40[2];
    /* +0x48 */ s32 (*stand_ck_0x48)(_ENEMY_WORK* self);
    /* +0x4C */ u32 pad_0x4C[2];
    /* +0x54 */ u32 (*area_change_ck_0x54)(_ENEMY_WORK* self, u16 timer);
    /* +0x58 */ u32 pad_0x58[5];
};

Em019ProgTbl em019_prog_tbl = {
    em019_work_clear,
    em019_init,
    em019_frame_update,
    em_act_run,
    em019_motion_next,
    em019_event_notify,
    em_act_mot_step,
    0,
    em019_tev_color_update,
    em019_state_query,
    {0, 0},
    em019_death_action_get,
    em019_noop,
    0,
    em019_part7_damaged_ck,
    {0, 0},
    em019_stand_ck,
    {0, 0},
    em019_area_change_ck,
    {0, 0, 0, 0, 0},
};

/* The two records `em019_area_change_ck` hands `em_pack_area_pick` (word-sized fields; their meaning is a GUESS:
 * a frame count, a packed id pair and an 0xFFFF terminator). */
u32 em019_follow_area_tbl[3] = {0x28, 0x012C000A, 0xFFFF0000};
u32 em019_follow_point_tbl[4] = {0x14, 0x00C8000A, 0xFFFF0000, 0};

/* The MHchar K-colour slot of each part (`em_part_reset`, `em019_tev_color_update`). */
u32 em019_part_kcolor_tbl[4] = {3, 0, 1, 4};


/* Sets the part-state bits of `mask` in +0x1E4 and clears the timer of each part whose two bits are set. */
void em019_part_state_set(_ENEMY_WORK* self, u8 mask)
{
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    self->field_0x1E4 |= mask;
    if ((mask & 0x03) != 0)
        part->timer[0] = 0;
    if ((mask & 0x0C) != 0)
        part->timer[1] = 0;
    if ((mask & 0x30) != 0)
        part->timer[2] = 0;
    if ((mask & 0xC0) != 0)
        part->timer[3] = 0;
}

/* Returns the two-bit state of one part from +0x1E4. */
u8 em019_part_state_get(_ENEMY_WORK* self, u8 part)
{
    u8 state = 0;

    switch (part) {
    case 0:
        state = self->field_0x1E4 & 0x03;
        break;
    case 1:
        state = (self->field_0x1E4 & 0x0C) >> 2;
        break;
    case 2:
        state = (self->field_0x1E4 & 0x30) >> 4;
        break;
    case 3:
        state = (self->field_0x1E4 & 0xC0) >> 6;
        break;
    }
    return state;
}

/* Arms the action's "run" flag at `+0x358`. */
void em019_action_run_flag_set(_ENEMY_WORK* self)
{
    self->field_0x358 = 1;
}

/* Clears the four part slots and the run flag/counter pair. */
void em019_work_clear(_ENEMY_WORK* self)
{
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    s32 i;

    for (i = 0; i < 4; i++) {
        part->colour_id[i] = 0;
        part->timer[i] = 0;
        part->meter[i] = 0.0f;
        part->flag[i] = 0;
        part->r[i] = 0;
        part->g[i] = 0;
        part->b[i] = 0;
        part->a[i] = 0;
    }
    self->field_0x358 = 0;
    self->tev_0x35A = 0;
}

/* Sets the work up: a restart (2) drops into the down action, the user-data helper is attached when missing,
 * and a fresh work gets its body effect. */
void em019_init(_ENEMY_WORK* self, u8 mode)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((s32)mode == 2) {
        em_move_mode_set(self, 4);
        em_act_arm_unless_down(self, 6, 2);
        em_state_refresh(self);
    }
    if (em_res_user_data_ck(self) == 0) {
        void* rec = operator new(0xC);

        if (rec != NULL)
            em019_user_data_ctor((EmUserData*)rec);
        em_res_user_data_set(self, rec);
    }
    self->field_0x1E4 = 0;
    if (self->field_0x009 == 0) {
        setVector3(&pos, 0.0f, -60.0f, 120.0f);
        eft_em_attach_set(self, 26, &pos, 1.5f, 0x1555);
        eft013_em_set(self, 1);
    }
}

/* Picks the follow-up motion of a finished motion pair: the stand-ups after motion 2, and the part-break
 * reactions after motion 7. */
void em019_motion_next(_ENEMY_WORK* self, u8* motion, u8* next)
{
    switch (*motion) {
    case 2:
        switch (*next) {
        case 0:
        case 3:
            if ((u32)em_mot_finished_ck(self) == 1)
                *next = 4;
            break;
        case 5:
            if ((u32)em_mot_finished_ck(self) == 1)
                *next = 7;
            break;
        case 6:
            if ((u32)em_mot_finished_ck(self) == 1)
                *next = 8;
            break;
        }
        break;
    case 7:
        switch (*next) {
        case 3:
            if ((self->field_0x1E4 & 0xC0) >= 2 && em_parts_damage_level_get(self, 7) < 1)
                *next = 20;
            break;
        case 4:
        case 31:
        case 32:
            if (em_eft_active_ck(self) == 1)
                *next = 22;
            break;
        case 5:
            if ((self->field_0x1E4 & 0x0C) >= 2 && em_parts_damage_level_get(self, 1) < 2)
                *next = 21;
            break;
        }
        break;
    }
}

/* Reacts to an (kind, sub) event: the meat and flag events of kind 1, and the part-hit and break events of
 * kind 10. */
void em019_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub)
{
    switch (kind) {
    case 1:
        switch (sub) {
        case 6:
            em_bait_react(self);
            break;
        case 7:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 202:
            em_part_hit_set(self, 0, 0);
            break;
        case 182:
            em019_part_state_set(self, 0xFF);
            break;
        case 52:
        case 151:
        case 178:
            em019_part_state_set(self, 0xFF);
            em_eft_active_ck(self);
            self->timer_0x020 = 300;
            break;
        }
        break;
    }
}

/* Breaks every part while the work stands on lava. */
void em019_frame_update(_ENEMY_WORK* self)
{
    if (em_magma_check(self) == 1)
        em019_part_state_set(self, 0xFF);
}

/* Drops the work: the fall, the down action and the finish flag. */
void em019_down_start(_ENEMY_WORK* self)
{
    em_fall_height_get(self);
    em_fall_start(self);
    em_act_arm_unless_act11(self, 3, 0);
    em_finish_flag_set(self);
}

/* Action 0, sub-state 0: the target-tracking idle motion. */
void em019_act0_sub0(_ENEMY_WORK* self)
{
    em019_part_flag_set(self, 1);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 20, 0, 1);
        break;
    case 1:
        em_target_pos_set(self, NULL);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 0, sub-state 1: motion 20 once. */
void em019_act0_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 20, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 0, sub-state 2: motion 29 once. */
void em019_act0_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 29, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 0, sub-state 3: the 90-frame down, then the fall finish. */
void em019_act0_sub3(_ENEMY_WORK* self)
{
    em_busy_timer_reset(self);
    em_ground_snap(self);
    em_fall_speed_set(self, 2000.0f);
    em_shake_req_set(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 1, 0, 0);
        self->timer_0x020 = 90;
        em019_part_state_set(self, 0xFF);
        break;
    case 1:
        if (--self->timer_0x020 == 0)
            em_action_finish_fall(self);
        break;
    }
}

/* Action 0, sub-state 7: the stand-up motion. */
void em019_act0_sub7(_ENEMY_WORK* self)
{
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* The em019 motion step dispatcher: `+0x1E6` picks one of the five motion bodies. */
void em019_motion_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act0_sub0(self);
        break;
    case 1:
        em019_act0_sub1(self);
        break;
    case 2:
        em019_act0_sub2(self);
        break;
    case 3:
        em019_act0_sub3(self);
        break;
    case 7:
        em019_act0_sub7(self);
        break;
    }
}

/* Action 1, sub-state 0: the roar, with its breath effects every eighth frame of the hold. */
void em019_act1_sub0(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 10, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        setVector3(&pos, 0.0f, -50.0f, 200.0f);
        if (em_frame_check(self, 0, 106.0f, 0.0f) == 1)
            eft_em_spawn(self, 0, 24, &pos, 1.0f);
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            draw_shape_arm((u32)self, 26, 10);
            em_hit_window_set(self, 0, 12, 5);
        }
        if (em_frame_check(self, 3, 112.0f, 220.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0)
                eft_em_spawn(self, 1, 24, &pos, 1.0f);
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 1: lies down for 1800 frames asleep, then gets up. */
void em019_act1_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 1800;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 15);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 5, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 2: the blended motion 20 up to frame 170. */
void em019_act1_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 20, 20, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, 170.0f, 0.0f) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 3: the blended motion 7 once. */
void em019_act1_sub3(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 7, 10, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 4: waits 300 frames in motion 6, then moves to action 1/6. */
void em019_act1_sub4(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        self->timer_0x020 = 300;
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 1, 6);
        break;
    }
}

/* Action 1, sub-state 5: motion 130 once. */
void em019_act1_sub5(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 130, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 6: motion 10 with the 1000-frame wait. */
void em019_act1_sub6(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 6, 0);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 1, sub-state 7: the blended motion 29 once. */
void em019_act1_sub7(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 29, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* The em019 per-motion step dispatcher: `+0x1E6` tail-calls one of the eight step bodies. */
void em019_state_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act1_sub0(self);
        return;
    case 1:
        em019_act1_sub1(self);
        return;
    case 2:
        em019_act1_sub2(self);
        return;
    case 3:
        em019_act1_sub3(self);
        return;
    case 4:
        em019_act1_sub4(self);
        return;
    case 5:
        em019_act1_sub5(self);
        return;
    case 6:
        em019_act1_sub6(self);
        return;
    case 7:
        em019_act1_sub7(self);
        return;
    }
}

/* Action 2's approach: the blended walk-in toward the target, at one of three distances. */
void em019_act2_approach(_ENEMY_WORK* self, u8 kind)
{
    em019_part_flag_set(self, 1);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mat_blend_reset(self);
        em_mot_set_blend(self, 21, 40, 0, 1);
        switch (kind) {
        default:
            em_approach_start(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start(self, -400.0f, 0);
            break;
        case 2:
            em_approach_start(self, -400.0f, 0);
            if (self->value_0x378 > 2000.0f)
                self->value_0x378 = 2000.0f;
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2, sub-state 1: the turn sequence of the unit's turn table. */
void em019_act2_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em019_turn_tbl, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em019_turn_tbl) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2, sub-state 2: motion 27, turning toward the target between frames 20 and 80. */
void em019_act2_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 27, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 20.0f, 80.0f) == 1)
            em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2's walk: motion 2 toward the target at one of four distances. */
void em019_act2_walk(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 10, 0);
        switch (kind) {
        default:
            em_approach_start(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f)
                self->value_0x378 = 700.0f;
            break;
        case 2:
            em_approach_start(self, -700.0f, 0);
            break;
        case 3:
            em_approach_start(self, -900.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2's run: motion 9 toward the target at one of four distances. */
void em019_act2_run(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 10, 0);
        switch (kind) {
        default:
            em_approach_start(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start(self, -400.0f, 0);
            break;
        case 2:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f)
                self->value_0x378 = 700.0f;
            break;
        case 3:
            em_approach_start(self, -700.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 2's turn: measures the bearing to +0x36C, arms the left or right turn motion and turns inside the
 * clamped window. */
void em019_act2_turn(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0: {
        u32 angle;

        self->state++;
        em_move_mode_set(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        switch (kind) {
        case 0:
            em_mot_set(self, 31, 10, 0);
            if (angle < 0x2000)
                angle = 0x2000;
            else if (angle > 0x6000)
                angle = 0x6000;
            break;
        case 1:
            em_mot_set(self, 32, 10, 0);
            if (angle < 0xA000)
                angle = -0x6000;
            else if (angle > 0xE000)
                angle = -0x2000;
            break;
        }
        em_move_vec2_clr(self);
        self->field_0x318 = -14.0f * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000)
            angle = 0x10000 - angle;
        self->timer_0x020 = (s16)angle;
        em_hit_window_set_default(self, 0, 30);
        break;
    }
    case 1:
        switch (kind) {
        case 0:
            em_turn_in_window(self, 10.0f, 50.0f, self->timer_0x020);
            break;
        case 1:
            em_turn_in_window(self, 10.0f, 50.0f, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, 20.0f, 70.0f) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* The em019 action-2 dispatcher: `+0x1E6` picks the approach, walk, run or turn body. */
void em019_act2_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act2_approach(self, 0);
        return;
    case 1:
        em019_act2_sub1(self);
        return;
    case 2:
        em019_act2_sub2(self);
        return;
    case 3:
        em019_act2_walk(self, 0);
        return;
    case 4:
        em019_act2_run(self, 0);
        return;
    case 5:
        em019_act2_approach(self, 1);
        return;
    case 6:
        em019_act2_walk(self, 1);
        return;
    case 7:
        em019_act2_run(self, 1);
        return;
    case 8:
        em019_act2_run(self, 2);
        return;
    case 9:
        em019_act2_walk(self, 2);
        return;
    case 10:
        em019_act2_run(self, 3);
        return;
    case 11:
        em019_act2_turn(self, 0);
        return;
    case 12:
        em019_act2_turn(self, 1);
        return;
    case 13:
        em019_act2_approach(self, 2);
        return;
    case 14:
        em019_act2_walk(self, 3);
        return;
    }
}

/* Action 3's down: falls and waits 90 or 30 frames before the down start. */
void em019_act3_down_wait(_ENEMY_WORK* self, u8 kind)
{
    em_busy_timer_reset(self);
    em_ground_snap(self);
    em_fall_speed_set(self, 2000.0f);
    em_shake_req_set(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 1, 0, 0);
        switch (kind) {
        case 0:
            self->timer_0x020 = 90;
            break;
        case 1:
            self->timer_0x020 = 30;
            break;
        }
        em019_part_state_set(self, 0xFF);
        break;
    case 1:
        if (--self->timer_0x020 == 0)
            em019_down_start(self);
        break;
    }
}

/* Action 3's slide: falls and slides in motion 89 before the down start. */
void em019_act3_down_slide(_ENEMY_WORK* self, u8 kind)
{
    em_busy_timer_reset(self);
    em_ground_snap(self);
    em_fall_speed_set(self, 2000.0f);
    em_shake_req_set(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 89, 0, 0);
        switch (kind) {
        case 0:
            em_approach_start(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            em_mot_speed_set(self, 5.0f);
            break;
        }
        em019_part_state_set(self, 0xFF);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1)
            em019_down_start(self);
        break;
    }
}

/* Action 3, sub-state 2: falls through the turn sequence before the down start. */
void em019_act3_sub2(_ENEMY_WORK* self)
{
    em_busy_timer_reset(self);
    em_ground_snap(self);
    em_fall_speed_set(self, 2000.0f);
    em_shake_req_set(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_turn_seq_start(self, (void*)em019_turn_tbl, 0, 0, 0);
        em_mot_speed_set(self, 5.0f);
        em019_part_state_set(self, 0xFF);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em019_turn_tbl) == 1)
            em019_down_start(self);
        break;
    }
}

/* The em019 approach step dispatcher: `+0x1E6` picks the approach or the retreat body. */
void em019_approach_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act3_down_wait(self, 0);
        break;
    case 1:
        em019_act3_down_slide(self, 0);
        break;
    case 2:
        em019_act3_sub2(self);
        break;
    case 3:
        em019_act3_down_slide(self, 1);
        break;
    case 4:
        em019_act3_down_wait(self, 1);
        break;
    }
}

/* Action 6's stand: holds the stand motion for 90, 60 or 15 frames (kind 1 shakes the camera every eighth
 * frame), then finishes standing. */
void em019_act6_stand(_ENEMY_WORK* self, u8 kind)
{
    fn_80131E00(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, fn_8013032C(self));
        em019_part_state_set(self, 0xFF);
        switch (kind) {
        default:
            self->timer_0x020 = 90;
            break;
        case 1:
            self->timer_0x020 = 60;
            break;
        case 2:
            self->timer_0x020 = 15;
            break;
        }
        break;
    case 1:
        if ((s32)kind == 1 && (self->timer_0x020 & 7) == 0)
            em_camera_req(self, -1, 200);
        if (--self->timer_0x020 == 0)
            em_action_finish_stand(self);
        break;
    }
}

/* Action 6, sub-state 1: motion 201 with its four frame windows. */
void em019_act6_sub1(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 201, 4, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 1, 196.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 34, 10);
        if (em_frame_check(self, 0, 56.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 35, 26);
        if (em_frame_check(self, 0, 62.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* Action 6, sub-state 2: motion 202, the slam with its two hit windows and the shell job at frame 220. */
void em019_act6_sub2(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 202, 0, 0);
        em_mot_speed_set(self, 1.25f);
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 0xFF);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 1, 80.0f, 0.0f) == 1)
            em_mot_speed_set(self, 1.0f);
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 33, 2);
        if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 27, 8);
            em_hit_window_set(self, 1, 21, 24);
        }
        if (em_frame_check(self, 2, 90.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 0, 220.0f, 0.0f) == 1) {
            nw4r::math::VEC3 off;

            em_camera_req(self, -1, 5);
            copyVec3(&pos, ((Fn80041E8C)setVec3)(&off, 0.0f, 0.0f, 110.0f));
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 9, 1, &pos, 1.5f, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 6's slide: motion 89 toward the target, then finishes standing. */
void em019_act6_slide(_ENEMY_WORK* self, u8 kind)
{
    fn_80131E00(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 89, 0, 0);
        switch (kind) {
        case 0:
            em_approach_start(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start(self, -1400.0f, 0);
            em_mot_speed_set(self, 5.0f);
            break;
        case 2:
        case 3:
            em_approach_start(self, 0.0f, 0);
            em_mot_speed_set(self, 5.0f);
            break;
        }
        em_height_set(self, fn_8013032C(self));
        em019_part_state_set(self, 0xFF);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* Action 6's turn: the turn sequence (kind 1 shakes the camera), then finishes standing. */
void em019_act6_turn(_ENEMY_WORK* self, u8 kind)
{
    fn_80131E00(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em019_turn_tbl, 0, 0, 0);
        if ((s32)kind != 2)
            em_mot_speed_set(self, 5.0f);
        else
            em_mot_speed_set(self, 1.0f);
        em_height_set(self, fn_8013032C(self));
        em019_part_state_set(self, 0xFF);
        self->timer_0x020 = 0;
        break;
    case 1:
        if ((s32)kind == 1 && (self->timer_0x020 & 7) == 1)
            em_camera_req(self, -1, 200);
        if (em_turn_seq_step(self, (void*)em019_turn_tbl) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* The em019 battle step dispatcher: `+0x1E6` selects one of the twelve battle step bodies. */
void em019_battle_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act6_stand(self, 0);
        return;
    case 1:
        em019_act6_sub1(self);
        return;
    case 2:
        em019_act6_sub2(self);
        return;
    case 3:
        em019_act6_slide(self, 0);
        return;
    case 4:
        em019_act6_slide(self, 1);
        return;
    case 5:
        em019_act6_turn(self, 0);
        return;
    case 6:
        em019_act6_slide(self, 2);
        return;
    case 7:
        em019_act6_stand(self, 1);
        return;
    case 8:
        em019_act6_slide(self, 3);
        return;
    case 9:
        em019_act6_turn(self, 1);
        return;
    case 10:
        em019_act6_turn(self, 2);
        return;
    case 11:
        em019_act6_stand(self, 2);
        return;
    }
}


/* Action 7, sub-state 0: motion 80 with two hit windows the arm states re-arm. */
void em019_act7_sub0(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 80, 10, 0);
        em_hit_window_set(self, 0, 1, 8);
        em_hit_window_set(self, 1, 2, 24);
        break;
    case 1:
        if (em_frame_check(self, 2, 20.0f, 0.0f) == 1)
            em_turn_to_target(self, 0x30);
        if (self->state_0x006 == 0 && self->field_0xA0D == 0)
            em_hit_window_set(self, 0, 3, 24);
        if (self->state_0x007 == 0 && self->field_0xA69 == 0)
            em_hit_window_set(self, 1, 4, 24);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7, sub-state 1: the lunge (motion 83) into the follow-through (motion 85). */
void em019_act7_sub1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        em_mot_set(self, 83, 10, 0);
        em_approach_start(self, 0.0f, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        em_approach_step(self, 0, 0x100);
        if (em_frame_check(self, 0, 78.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 5, 11);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 85, 6, 0);
        }
        break;
    case 2:
        if (self->state_0x006 == 0 && em_frame_check(self, 1, 14.0f, 0.0f) == 1) {
            self->state_0x006++;
            em_hit_window_clear(self, 0);
            em_hit_window_clear(self, 1);
            em_hit_window_set(self, 0, 6, 16);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7, sub-state 2: motion 82, turning toward the target inside two windows. */
void em019_act7_sub2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        em_mot_set(self, 82, 4, 0);
        em_hit_window_set(self, 0, 22, 8);
        em_hit_window_set(self, 1, 7, 24);
        break;
    case 1:
        if (em_frame_check(self, 0, 134.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            em_hit_window_set(self, 0, 23, 16);
        }
        if (em_frame_check(self, 3, 2.0f, 60.0f) == 1)
            em_turn_to_target(self, 0x200);
        em_turn_in_window(self, 60.0f, 100.0f, -0x4000);
        em_turn_in_window(self, 200.0f, 270.0f, 0x4000);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7's swipe: motion 81 with its hit windows; kind 1 adds the two slash effects. */
void em019_act7_swipe(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 81, 10, 0);
        em_hit_window_set(self, 0, 8, 8);
        if (kind == 1)
            em_hit_window_set(self, 1, 28, 16);
        else
            em_hit_window_set(self, 1, 9, 16);
        break;
    case 1:
        em_turn_in_window(self, 86.0f, 180.0f, 0x8000);
        if (kind == 1) {
            if (em_frame_check(self, 0, 140.0f, 0.0f) == 1)
                eft_em_spawn(self, 174, 47, NULL, 1.0f);
            if (em_frame_check(self, 0, 170.0f, 0.0f) == 1)
                eft_em_spawn(self, 187, 48, NULL, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7's bite: motion 87 (216 for kind 1) with its part effects and the shell job at frame 120. */
void em019_act7_bite(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;
    u32 alt;
    s32 angle;

    VEC3_ctor(&pos);
    alt = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        if ((s32)kind != 1)
            em_mot_set(self, 87, 6, 0);
        else
            em_mot_set(self, 216, 6, 0);
        em019_part_state_set(self, 0x0C);
        break;
    case 1:
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1)
            em019_part_state_set(self, 0x30);
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_frame_check(self, 0, 44.0f, 0.0f) == 1 ||
            em_frame_check(self, 0, 54.0f, 0.0f) == 1 || em_frame_check(self, 0, 68.0f, 0.0f) == 1)
            eft007_part_set(self, 59);
        if (em_frame_check(self, 0, 90.0f, 0.0f) == 1)
            eft007_part_set(self, 49);
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1)
            eft007_part_spawn(self, 50, 0, 0);
        switch (kind) {
        default:
            angle = 0;
            break;
        case 1:
            alt = 1;
            break;
        case 2:
            angle = -0x222;
            break;
        case 3:
            angle = 0x222;
            break;
        }
        if (alt == 0) {
            if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, -120.0f, 150.0f);
                ((ShellPartJobFn)shell_set_func_ptr->method_0x74)(self, 2, 25, &pos, 1.0f, angle, 0xFFFF);
            }
            if (em_frame_check(self, 0, 300.0f, 0.0f) == 1)
                eft007_part_set(self, 51);
        } else if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            eft007_part_set(self, 65);
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7's slam: motion 88 with the camera shake, the shell job and the landing effect. */
void em019_act7_slam(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 88, 4, 0);
        if (kind == 1)
            em_hit_window_set(self, 0, 29, 8);
        else
            em_hit_window_set(self, 0, 10, 8);
        break;
    case 1:
        if (em_frame_check(self, 3, 34.0f, 70.0f) == 1)
            em_turn_to_target(self, 0x180);
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1)
            em_camera_req(self, 3, 5);
        if (em_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            nw4r::math::VEC3 off;

            copyVec3(&pos, ((Fn80041E8C)setVec3)(&off, 0.0f, 0.0f, 300.0f));
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 9, 3, &pos, 1.25f, self->field_0xAEA);
        }
        if (kind == 1) {
            if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 100.0f);
                eft009_spawn_at_pos(self, 16, 145, 0, 1.5f, &pos);
            }
        } else if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 16, 17, 0, 1.7f);
            } else {
                get_joint_wpos_em(self, 16, &pos);
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 4, 0, 1.2f * get_em_chg_scale(self));
            }
        }
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7's slide: motion 89 toward the target at one of four distances, with its hit window. */
void em019_act7_slide(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 89, 6, 0);
        em_hit_window_set(self, 0, 11, 2);
        switch (kind) {
        default:
            em_approach_start(self, -600.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            break;
        case 2:
            em_approach_start(self, 100.0f, 0);
            break;
        case 3:
            em_approach_start(self, -1000.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_hit_window_clear(self, 0);
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7's spin: the turn into the blended spin (kinds 1 and 2), or the spin straight away (kind 0), with
 * its eighteen hit windows. */
void em019_act7_spin(_ENEMY_WORK* self, u8 kind)
{
    switch (self->state) {
    case 0:
        if ((u8)(kind + 0xFF) <= 1) {
            u32 angle;

            self->state++;
            em_move_mode_set(self, 0);
            angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            switch (kind) {
            case 1:
                em_mot_set(self, 31, 10, 0);
                if (angle < 0x2000)
                    angle = 0x2000;
                else if (angle > 0x6000)
                    angle = 0x6000;
                break;
            case 2:
                em_mot_set(self, 32, 10, 0);
                if (angle < 0xA000)
                    angle = -0x6000;
                else if (angle > 0xE000)
                    angle = -0x2000;
                break;
            }
            em_move_vec2_clr(self);
            self->field_0x318 = -14.0f * get_em_base_scale(self) * get_em_chg_scale(self);
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
            if (angle > 0x8000)
                angle = 0x10000 - angle;
            self->timer_0x020 = (s16)angle;
            em_hit_window_set_default(self, 0, 30);
        } else {
            self->state = 2;
            em_move_mode_set(self, 0);
            em_mot_set(self, 210, 4, 0);
        }
        break;
    case 1:
        switch (kind) {
        case 1:
            em_turn_in_window(self, 10.0f, 50.0f, self->timer_0x020);
            break;
        case 2:
            em_turn_in_window(self, 10.0f, 50.0f, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, 20.0f, 70.0f) == 1)
            CancelFade(self);
        if (em_frame_check(self, 1, 84.0f, 0.0f) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 210, 18, 14, 1);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 58.0f, 0.0f) == 1)
            em019_part_state_set(self, 0x30);
        if (em_frame_check(self, 0, 42.0f, 0.0f) == 1)
            em_hit_window_set(self, 1, 15, 10);
        if (em_frame_check(self, 0, 54.0f, 0.0f) == 1)
            em_hit_window_clear(self, 1);
        if (em_frame_check(self, 0, 116.0f, 0.0f) == 1)
            em_hit_window_set(self, 1, 14, 26);
        if (em_frame_check(self, 0, 130.0f, 0.0f) == 1)
            em_hit_window_clear(self, 1);
        if (em_frame_check(self, 0, 184.0f, 0.0f) == 1)
            em_hit_window_set(self, 1, 15, 26);
        if (em_frame_check(self, 0, 200.0f, 0.0f) == 1)
            em_hit_window_clear(self, 1);
        if (em_frame_check(self, 0, 262.0f, 0.0f) == 1)
            em_hit_window_set(self, 1, 14, 26);
        if (em_frame_check(self, 0, 274.0f, 0.0f) == 1)
            em_hit_window_clear(self, 1);
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 13, 26);
        if (em_frame_check(self, 0, 58.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, 64.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 13, 26);
        if (em_frame_check(self, 0, 132.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, 138.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_frame_check(self, 0, 196.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 13, 26);
        if (em_frame_check(self, 0, 204.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, 210.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_frame_check(self, 0, 272.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 13, 26);
        if (em_frame_check(self, 0, 280.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, 286.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Action 7's roar: motion 209 (204 for kind 1) with its breath effects and shell job, then the recovery. */
void em019_act7_roar(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        switch (kind) {
        case 0:
            em_mot_set(self, 209, 0, 0);
            break;
        case 1:
            em_mot_set(self, 204, 0, 0);
            break;
        }
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 0xFF);
        em_guard_set(self);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 55.0f, 0.0f) == 1)
            em_shake_req_set(self);
        switch (kind) {
        case 0:
            if (em_frame_check(self, 0, 408.0f, 0.0f) == 1)
                em_mot_speed_set(self, 0.8f);
            if (em_frame_check(self, 0, 630.0f, 0.0f) == 1)
                em_mot_speed_set(self, 1.0f);
            if ((self->field_0x228 & 6) == 0 && em_frame_check(self, 0, 2.0f, 0.0f) == 1)
                eft009_spawn_at_joint(self, 25, 97, 0, 1.0f);
            if (em_frame_check(self, 0, 48.0f, 0.0f) == 1)
                eft009_spawn_at_joint(self, 25, 98, 0, 1.0f);
            if (em_frame_check(self, 0, 174.0f, 0.0f) == 1 || em_frame_check(self, 0, 190.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 206.0f, 0.0f) == 1 || em_frame_check(self, 0, 214.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 224.0f, 0.0f) == 1)
                eft007_part_set(self, 52);
            if (em_frame_check(self, 0, 234.0f, 0.0f) == 1)
                eft007_part_spawn(self, 61, 0, 0);
            if (em_frame_check(self, 0, 236.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, -27.0f, 150.0f);
                ((ShellPartJobFn)shell_set_func_ptr->method_0x74)(self, 3, 25, &pos, 1.0f, 0x247, 0xFFFF);
            }
            if (em_frame_check(self, 0, 640.0f, 0.0f) == 1)
                eft007_part_set(self, 53);
            break;
        case 1:
            if ((self->field_0x228 & 6) == 0 && em_frame_check(self, 0, 2.0f, 0.0f) == 1)
                eft009_spawn_at_joint(self, 25, 97, 0, 1.0f);
            if (em_frame_check(self, 0, 48.0f, 0.0f) == 1)
                eft009_spawn_at_joint(self, 25, 98, 0, 1.0f);
            if (em_frame_check(self, 0, 166.0f, 0.0f) == 1)
                eft007_part_spawn(self, 61, 0, 0);
            if (em_frame_check(self, 0, 168.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, -27.0f, 150.0f);
                ((ShellPartJobFn)shell_set_func_ptr->method_0x74)(self, 6, 25, &pos, 1.0f, 0, 0xFFFF);
            }
            if (em_frame_check(self, 0, 510.0f, 0.0f) == 1)
                eft007_part_set(self, 53);
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 215, 0, 0);
            em_guard_clr(self);
            if (kind == 1)
                em_mot_speed_set(self, 1.2f);
        }
        break;
    case 2:
        em_busy_timer_reset(self);
        if (em_frame_check(self, 3, 120.0f, 144.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 134, 47, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 47, 126, 0, 1.0f);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Converts degrees to a 16-bit angle, rounding to nearest. */
#define EM019_DEG2ANG(deg) ((u16)(0.5f + 65536.0f * (deg) / 360.0f))

/* Action 7's rush: motion 211 toward the target (or a point beside it), turning by the kind's angle on the
 * approach frames, into motion 212. */
void em019_act7_rush(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;
    u16 speed;
    u32 turn;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 211, 0, 0);
        em_mot_speed_set(self, 1.25f);
        em_height_set(self, 0.0f);
        switch (kind) {
        default:
            em_approach_start(self, 1000.0f, 0);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            em_approach_start(self, 1200.0f, 0);
            break;
        case 11: {
            u32 angle = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000;
            f32 scale = 2.0f / get_em_base_scale(self);

            setVector3(&pos, 0.0f, 0.0f, 1650.0f * em_target_dist_ratio_get(self, 3.0f, 0.3f, 2000.0f) * scale);
            rotVecY(&pos, angle);
            addVec3To(&self->vec_0x36C, &pos);
            em_approach_start(self, 200.0f, 0);
            break;
        }
        case 12: {
            u32 angle = calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000;
            f32 scale = 2.0f / get_em_base_scale(self);

            setVector3(&pos, 0.0f, 0.0f, 1650.0f * em_target_dist_ratio_get(self, 3.0f, 0.3f, 2000.0f) * scale);
            rotVecY(&pos, angle);
            addVec3To(&self->vec_0x36C, &pos);
            em_approach_start(self, 200.0f, 0);
            break;
        }
        }
        em019_part_state_set(self, 0xFF);
        em_hit_window_set(self, 0, 20, 8);
        em_hit_window_set(self, 1, 25, 24);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 16, 19);
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1)
            em_shake_req_set(self);
        switch (kind) {
        default:
            speed = 0x100;
            turn = 0;
            break;
        case 1:
            speed = 0;
            turn = EM019_DEG2ANG(0.26666668f * get_em_base_scale(self));
            break;
        case 2:
            speed = 0;
            turn = -EM019_DEG2ANG(0.26666668f * get_em_base_scale(self));
            break;
        case 3:
            speed = 0;
            turn = EM019_DEG2ANG(0.6f * get_em_base_scale(self));
            break;
        case 4:
            speed = 0;
            turn = -EM019_DEG2ANG(0.6f * get_em_base_scale(self));
            break;
        case 5:
            speed = 0;
            turn = EM019_DEG2ANG(0.23333333f * get_em_base_scale(self));
            break;
        case 6:
            speed = 0;
            turn = -EM019_DEG2ANG(0.23333333f * get_em_base_scale(self));
            break;
        case 7:
            speed = 0;
            turn = EM019_DEG2ANG(0.36666667f * get_em_base_scale(self));
            break;
        case 8:
            speed = 0;
            turn = -EM019_DEG2ANG(0.36666667f * get_em_base_scale(self));
            break;
        case 9:
            speed = 0;
            turn = EM019_DEG2ANG(0.6666667f * get_em_base_scale(self));
            break;
        case 10:
            speed = 0;
            turn = -EM019_DEG2ANG(0.6666667f * get_em_base_scale(self));
            break;
        case 11:
        case 12:
            speed = em_frame_check(self, 1, 70.0f, 0.0f) == 1 ? 0x400 : 0;
            turn = 0;
            break;
        }
        if (em_frame_check(self, 1, 70.0f, 0.0f) == 1)
            self->field_0x1C0 += turn;
        if (em_approach_step(self, 0, speed) == 1) {
            self->state++;
            em_mot_set(self, 212, 4, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 18.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* Action 7's jump: the leap (motion 205), the spinning fall (motion 213) and the landing (motion 206). */
void em019_act7_jump(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        em_mot_set(self, 205, 0, 0);
        em_mot_speed_set(self, 1.25f);
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 0xFF);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 1, 70.0f, 0.0f) == 1)
            em_mot_speed_set(self, 1.0f);
        if (em_frame_check(self, 2, 94.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 33, 2);
        if (em_frame_check(self, 0, 74.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 18, 3);
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 101, 0, 1.0f);
        if (em_frame_check(self, 3, 12.0f, 62.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft009_spawn_at_joint(self, 25, 99, 0, 1.0f);
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 100, 0, 1.0f);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_joint_drop_start(self, self->joint_0x204);
            em_mot_set(self, 213, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 50.0f * get_em_base_scale(self) * get_em_chg_scale(self);
        }
        break;
    case 2: {
        f32 turn = 65536.0f * (10.0f * get_em_base_scale(self)) / 360.0f;

        self->field_0x1C0 += (u16)(0.5f + turn);
        em_move_offset_apply(self);
        if (em_frame_check(self, 3, 2.0f, 62.0f) == 1 && system_w.field_0x0c % 12 == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 133, 3, &pos, 1.0f);
        }
        if (self->field_0x23C == 0 && self->pos.y >= 4000.0f + self->field_0x20C) {
            em019_down_start(self);
        } else if (em_ground_snap_offset_ck(self, 700.0f) == 1) {
            self->state++;
            em_ground_snap(self);
            em_mot_set(self, 206, 0, 0);
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 100, 25, &pos, 1.0f);
            em_hit_window_clear(self, 0);
        }
        break;
    }
    case 3:
        if (em_frame_check(self, 3, 30.0f, 60.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 99, 25, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1)
            em019_down_start(self);
        break;
    }
}

/* Action 7's dive: the fall (motion 207), the spinning drop (motion 214) and the landing quake (motion 208). */
void em019_act7_dive(_ENEMY_WORK* self)
{
    EmSpawnRec rec;
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 pos;

    em_spawn_rec_init(&rec);
    VEC3_ctor(&joint);
    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 207, 0, 0);
        em_ground_snap(self);
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 0xFF);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 86.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 3, 2.0f, 80.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 99, 25, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 98, 25, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 74.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 19, 3);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            get_joint_wpos_em(self, self->joint_0x204, &joint);
            self->pos.y = joint.y;
            self->fall_speed_0x1A4 = 0.0f;
            self->field_0x1AC = 0.0f;
            em_mot_set(self, 214, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = -50.0f * get_em_base_scale(self) * get_em_chg_scale(self);
        }
        break;
    case 2: {
        f32 turn = 65536.0f * (10.0f * get_em_base_scale(self)) / 360.0f;

        self->field_0x1C0 -= (u16)(0.5f + turn);
        em_move_offset_apply(self);
        if (((EmGroundCkFn)em_ground_ck)(self, 700.0f) == 1) {
            self->state++;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_mot_set(self, 208, 0, 0);
            em_hit_window_clear(self, 0);
            eft009_spawn_at_joint(self, 25, 92, 0, 1.0f);
            eft_spawn_type_at_area(self, 9);
            em019_spawn_rec_set(&rec, 0, 0, self->bits_0x1EC & 0x4000);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 24);
        }
        break;
    }
    case 3:
        if (em_frame_check(self, 3, 30.0f, 62.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft009_spawn_at_joint(self, 25, 111, 0, 1.0f);
        if (em_frame_check(self, 0, 30.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 108, 0, 1.0f);
        if (em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em019_spawn_rec_set(&rec, 2, 0, (u16)((((self->bits_0x1EC + 0x157E7) >> 4) & 0x4000) + 0x8000));
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 24);
        }
        if (em_frame_check(self, 1, 78.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* Action 7's magma dig: motion 200 (203 for kind 1) with the lava splash on each surfacing of the head joint. */
void em019_act7_magma_dig(_ENEMY_WORK* self, u8 kind)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    em019_action_run_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -15);
        if (kind == 0)
            em_mot_set(self, 200, 0, 0);
        else
            em_mot_set(self, 203, 0, 0);
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 0xFF);
        em_mot_speed_set(self, 1.2f);
        self->state_0x007 = 0;
        em_shake_req_set(self);
        break;
    case 1:
        if (kind == 0) {
            switch (self->state_0x007) {
            case 0:
                if (em_magma_check(self) == 1) {
                    get_joint_wpos_em(self, 25, &pos);
                    if (pos.y > self->field_0x214) {
                        pos.y = 5.0f + self->field_0x214;
                        eft009_set_pos(136, &pos, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
                        self->state_0x007++;
                    }
                }
                break;
            case 1:
                if (em_magma_check(self) == 1) {
                    get_joint_wpos_em(self, 25, &pos);
                    if (pos.y < self->field_0x214) {
                        pos.y = 5.0f + self->field_0x214;
                        eft009_set_pos(136, &pos, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
                        self->state_0x007++;
                    }
                }
                break;
            }
            if (em_frame_check(self, 2, 72.0f, 0.0f) == 1)
                em_shake_req_set(self);
            else if (em_frame_check(self, 1, 180.0f, 0.0f) == 1)
                em_shake_req_set(self);
            if (em_frame_check(self, 0, 64.0f, 0.0f) == 1)
                em_hit_window_set(self, 0, 17, 2);
            if (em_frame_check(self, 0, 120.0f, 0.0f) == 1)
                em_hit_window_clear(self, 0);
        } else {
            switch (self->state_0x007) {
            case 0:
                if (em_magma_check(self) == 1) {
                    get_joint_wpos_em(self, 25, &pos);
                    if (pos.y > self->field_0x214) {
                        pos.y = 5.0f + self->field_0x214;
                        eft009_set_pos(136, &pos, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
                        self->state_0x007 = 1;
                    }
                }
                break;
            case 1:
                if (em_magma_check(self) == 1) {
                    get_joint_wpos_em(self, 25, &pos);
                    if (pos.y < self->field_0x214) {
                        pos.y = 5.0f + self->field_0x214;
                        eft009_set_pos(136, &pos, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
                        self->state_0x007 = 0;
                    }
                }
                break;
            }
            if (em_frame_check(self, 2, 32.0f, 0.0f) == 1)
                em_shake_req_set(self);
            else if (em_frame_check(self, 1, 138.0f, 0.0f) == 1)
                em_shake_req_set(self);
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1)
                em_hit_window_set(self, 0, 17, 2);
            if (em_frame_check(self, 0, 78.0f, 0.0f) == 1)
                em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 3, 0.0f, 10.0f) == 1)
            em_turn_to_target(self, EM019_DEG2ANG(18.0f * get_em_base_scale(self)));
        if (em_mot_end_ck(self) == 1)
            em_action_finish_stand(self);
        break;
    }
}

/* The em019 action-7 dispatcher: `+0x1E6` picks one of the attack bodies. */
void em019_act7_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em019_act7_sub0(self);
        return;
    case 1:
        em019_act7_sub1(self);
        return;
    case 2:
        em019_act7_sub2(self);
        return;
    case 3:
        em019_act7_swipe(self, 0);
        return;
    case 4:
        em019_act7_bite(self, 0);
        return;
    case 5:
        em019_act7_slam(self, 0);
        return;
    case 6:
        em019_act7_slide(self, 0);
        return;
    case 7:
        em019_act7_slide(self, 1);
        return;
    case 8:
        em019_act7_slide(self, 2);
        return;
    case 9:
        em019_act7_spin(self, 0);
        return;
    case 10:
        em019_act7_roar(self, 0);
        return;
    case 11:
        em019_act7_rush(self, 0);
        return;
    case 12:
        em019_act7_jump(self);
        return;
    case 13:
        em019_act7_dive(self);
        return;
    case 14:
        em019_act7_magma_dig(self, 0);
        return;
    case 15:
        em019_act7_magma_dig(self, 1);
        return;
    case 16:
        em019_act7_rush(self, 1);
        return;
    case 17:
        em019_act7_rush(self, 2);
        return;
    case 18:
        em019_act7_rush(self, 3);
        return;
    case 19:
        em019_act7_rush(self, 4);
        return;
    case 20:
        em019_act7_swipe(self, 1);
        return;
    case 21:
        em019_act7_slam(self, 1);
        return;
    case 22:
        em019_act7_bite(self, 1);
        return;
    case 23:
        em019_act7_rush(self, 5);
        return;
    case 24:
        em019_act7_rush(self, 6);
        return;
    case 25:
        em019_act7_rush(self, 7);
        return;
    case 26:
        em019_act7_rush(self, 8);
        return;
    case 27:
        em019_act7_rush(self, 9);
        return;
    case 28:
        em019_act7_rush(self, 10);
        return;
    case 29:
        em019_act7_spin(self, 1);
        return;
    case 30:
        em019_act7_spin(self, 2);
        return;
    case 31:
        em019_act7_bite(self, 2);
        return;
    case 32:
        em019_act7_bite(self, 3);
        return;
    case 33:
        em019_act7_roar(self, 1);
        return;
    case 34:
        em019_act7_slide(self, 3);
        return;
    case 35:
        em019_act7_rush(self, 11);
        return;
    case 36:
        em019_act7_rush(self, 12);
        return;
    }
}

EmSeRecord em019_se_rec_mot23 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot23[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot23},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot24 = {120, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot24[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot24},
    {0xFF, {0, 0, 0}, 0},
};

Em019SePos em019_se_pos_mot25_a = {52.0f, 112.0f, 0, 16384};

Em019SePos em019_se_pos_mot25_b = {58.0f, 118.0f, -1, -16384};

EmSeRecord em019_se_rec_mot25_a = {122, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_rec_mot25_b = {123, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em019_se_rec_mot25_c = {124, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot25[7] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot25_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot25_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em019_se_rec_mot25_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot25_b},
    {0x00, {0, 0, 0}, &em019_se_rec_mot25_c},
    {0xFF, {0, 0, 0}, 0},
};

Em019SePos em019_se_pos_mot28_a = {22.0f, 62.0f, 0, 16384};

Em019SePos em019_se_pos_mot28_b = {64.0f, 104.0f, 0, 16384};

EmSeOffset em019_se_ofs_mot28[2] = {
    {10.0f, 82.0f},
    {64.0f, 114.0f},
};

EmSeRecord em019_se_rec_mot28_a = {125, 0, 6, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em019_se_shift[0]};

EmSeRecord em019_se_rec_mot28_b = {126, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em019_se_rec_mot28_c = {127, 0, 8, 0, 0, {0, 1, 0, 0}, (const EmSeOffset*)&em019_se_shift[2]};

EmSeEntry em019_se_tbl_mot28[9] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot28_a},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em019_se_ofs_mot28[0]},
    {0x00, {0, 0, 0}, &em019_se_rec_mot28_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em019_se_rec_mot28_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot28_b},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em019_se_ofs_mot28[1]},
    {0x00, {0, 0, 0}, &em019_se_rec_mot28_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot122 = {129, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot122[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot122},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot123 = {4, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot123[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot123},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot159 = {5, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot159[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot159},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot124 = {128, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot124[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot124},
    {0xFE, {0, 0, 0}, 0},
};

Em019SePos em019_se_pos_mot126_a = {52.0f, 112.0f, 0, 16384};

Em019SePos em019_se_pos_mot126_b = {58.0f, 118.0f, -1, -16384};

EmSeRecord em019_se_rec_mot126_a = {122, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_rec_mot126_b = {123, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em019_se_rec_mot126_c = {124, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot126[6] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot126_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot126_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot126_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot126_b},
    {0x00, {0, 0, 0}, &em019_se_rec_mot126_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot127_a = {125, 0, 6, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em019_se_shift[0]};

EmSeRecord em019_se_rec_mot127_b = {126, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em019_se_rec_mot127_c = {127, 0, 8, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em019_se_shift[2]};

EmSeEntry em019_se_tbl_mot127[8] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot28_a},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em019_se_ofs_mot28[0]},
    {0x00, {0, 0, 0}, &em019_se_rec_mot127_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot127_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot28_b},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em019_se_ofs_mot28[1]},
    {0x00, {0, 0, 0}, &em019_se_rec_mot127_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot141 = {130, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot141[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot141},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot120 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot120[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot120},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot168 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot168[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot168},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot182 = {136, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot182[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot182},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot183 = {137, 0, 10, 0, 1, {2, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot183[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot183},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot184 = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot184[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot184},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot187 = {139, 0, 10, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot187[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot187},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot185 = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot185[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot185},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot186 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot186[2] = {
    {0x01, {0, 0, 0}, &em019_se_rec_mot186},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot191_a = {121, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em019_se_rec_mot191_b = {128, 0, 14, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot191[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x50)},
    {0x00, {0, 0, 0}, &em019_se_rec_mot191_a},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em019_se_rec_mot191_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSePos em019_se_pos_mot202 = {94.0f, 140.0f, -1, -32768, 0};

EmSeRecord em019_se_rec_mot202 = {132, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot202[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em019_se_pos_mot202},
    {0x00, {0, 0, 0}, &em019_se_rec_mot202},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot52 = {134, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot52[4] = {
    {0x1C, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x64)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em019_se_count_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot52},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em019_se_loop},
};

EmSeEntry em019_se_tbl_mot178[5] = {
    {0x1C, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x64)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em019_se_count_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot52},
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em019_se_loop},
};

EmSeRecord em019_se_rec_mot55_a = {140, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_rec_mot55_b = {141, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot55[4] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot55_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em019_se_rec_mot55_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot228 = {137, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot228[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot228},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot229 = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot229[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot229},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot230 = {139, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot230[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot230},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot231 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot231[2] = {
    {0x01, {0, 0, 0}, &em019_se_rec_mot231},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot232 = {139, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot232[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot232},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot154 = {139, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot154[3] = {
    {0x1D, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em019_se_rec_mot154},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot155_a = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em019_se_rec_mot155_b = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot155[4] = {
    {0x1D, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em019_se_rec_mot155_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot155_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot156 = {139, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot156[3] = {
    {0x1D, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em019_se_rec_mot156},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot145 = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot145[2] = {
    {0x00, {0, 0, 0}, &em019_se_rec_mot145},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_rec_mot174_a = {139, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_rec_mot174_b = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_tbl_mot174[4] = {
    {0x1D, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em019_se_rec_mot174_a},
    {0x00, {0, 0, 0}, &em019_se_rec_mot174_b},
    {0xFF, {0, 0, 0}, 0},
};

/* Action 10: plays the sound table of the motion id at +0x1E6, or ends the action. */
void em019_act10_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 23:
        em_se_tbl_play(self, em019_se_tbl_mot23, 0, 23);
        break;
    case 24:
        em_se_tbl_play(self, em019_se_tbl_mot24, 0, 24);
        break;
    case 25:
        em_se_tbl_play(self, em019_se_tbl_mot25, 0, 25);
        break;
    case 26:
        em_se_tbl_play(self, em019_se_tbl_mot25, 0, 26);
        break;
    case 27:
        em_se_tbl_play(self, em019_se_tbl_mot25, 0, 27);
        break;
    case 28:
        em_se_tbl_play(self, em019_se_tbl_mot28, 0, 28);
        break;
    case 29:
        em_se_tbl_play(self, em019_se_tbl_mot23, 0, 29);
        break;
    case 30:
        em_se_tbl_play(self, em019_se_tbl_mot24, 0, 30);
        break;
    case 122:
        em_se_tbl_play(self, em019_se_tbl_mot122, 0, 122);
        break;
    case 123:
        em_se_tbl_play(self, em019_se_tbl_mot123, 0, 123);
        break;
    case 124:
        em_se_tbl_play(self, em019_se_tbl_mot124, 0, 124);
        break;
    case 141:
        em_se_tbl_play(self, em019_se_tbl_mot141, 0, 141);
        break;
    case 126:
        em_se_tbl_play(self, em019_se_tbl_mot126, 0, 126);
        break;
    case 127:
        em_se_tbl_play(self, em019_se_tbl_mot127, 0, 127);
        break;
    case 142:
        em_se_tbl_play(self, em019_se_tbl_mot141, 0, 142);
        break;
    case 120:
        em_se_tbl_play(self, em019_se_tbl_mot120, 0, 120);
        break;
    case 159:
        em_se_tbl_play(self, em019_se_tbl_mot159, 0, 159);
        break;
    case 160:
        em_se_tbl_play(self, em019_se_tbl_mot159, 0, 160);
        break;
    case 168:
        em_se_tbl_play(self, em019_se_tbl_mot168, 0, 168);
        break;
    case 182:
        em_se_tbl_play(self, em019_se_tbl_mot182, 0, 182);
        break;
    case 183:
        em_se_tbl_play(self, em019_se_tbl_mot183, 0, 183);
        break;
    case 184:
        em_se_tbl_play(self, em019_se_tbl_mot184, 0, 184);
        break;
    case 185:
        em_se_tbl_play(self, em019_se_tbl_mot185, 0, 185);
        break;
    case 186:
        em_se_tbl_play(self, em019_se_tbl_mot186, 0, 186);
        break;
    case 187:
        em_se_tbl_play(self, em019_se_tbl_mot187, 0, 187);
        break;
    case 188:
        em_se_tbl_play(self, em019_se_tbl_mot187, 0, 188);
        break;
    case 191:
        em_se_tbl_play(self, em019_se_tbl_mot191, 0, 191);
        break;
    case 193:
        em_se_tbl_play(self, em019_se_tbl_mot141, 0, 193);
        break;
    case 202:
        em_se_tbl_play(self, em019_se_tbl_mot202, 0, 202);
        break;
    case 52:
        em_se_tbl_play(self, em019_se_tbl_mot52, 0, 52);
        break;
    case 151:
        em_se_tbl_play(self, em019_se_tbl_mot52, 0, 151);
        break;
    case 178:
        em_se_tbl_play(self, em019_se_tbl_mot178, 0, 178);
        break;
    case 228:
        em_se_tbl_play(self, em019_se_tbl_mot228, 0, 228);
        break;
    case 229:
        em_se_tbl_play(self, em019_se_tbl_mot229, 0, 229);
        break;
    case 230:
        em_se_tbl_play(self, em019_se_tbl_mot230, 0, 230);
        break;
    case 231:
        em_se_tbl_play(self, em019_se_tbl_mot231, 0, 231);
        break;
    case 232:
        em_se_tbl_play(self, em019_se_tbl_mot232, 0, 232);
        break;
    case 233:
        em_se_tbl_play(self, em019_se_tbl_mot232, 0, 233);
        break;
    case 55:
        em_se_tbl_play(self, em019_se_tbl_mot55, 0, 55);
        break;
    case 154:
        em_se_tbl_play(self, em019_se_tbl_mot154, 0, 154);
        break;
    case 155:
        em_se_tbl_play(self, em019_se_tbl_mot155, 0, 155);
        break;
    case 156:
        em_se_tbl_play(self, em019_se_tbl_mot156, 0, 156);
        break;
    case 145:
        em_se_tbl_play(self, em019_se_tbl_mot145, 0, 145);
        break;
    case 157:
        em_se_tbl_play(self, em019_se_tbl_mot156, 0, 157);
        break;
    case 146:
        em_se_tbl_play(self, em019_se_tbl_mot145, 0, 146);
        break;
    case 166:
        em_se_tbl_play(self, em019_se_tbl_mot229, 0, 166);
        break;
    case 167:
        em_se_tbl_play(self, em019_se_tbl_mot229, 0, 167);
        break;
    case 174:
        em_se_tbl_play(self, em019_se_tbl_mot174, 0, 174);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

EmSeRecord em019_se_alt_rec_0 = {131, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_alt_tbl_0[2] = {
    {0x00, {0, 0, 0}, &em019_se_alt_rec_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_alt_rec_11_a = {134, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_alt_rec_11_b = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_alt_tbl_11[5] = {
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em019_se_count_b},
    {0x00, {0, 0, 0}, &em019_se_alt_rec_11_a},
    {0x00, {0, 0, 0}, &em019_se_alt_rec_11_b},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_alt_rec_15 = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_alt_tbl_15[3] = {
    {0x00, {0, 0, 0}, &em019_se_alt_rec_15},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_alt_rec_26_a = {129, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em019_se_alt_rec_26_b = {4, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_alt_tbl_26[3] = {
    {0x00, {0, 0, 0}, &em019_se_alt_rec_26_a},
    {0x00, {0, 0, 0}, &em019_se_alt_rec_26_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em019_se_alt_rec_28 = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em019_se_alt_tbl_28[2] = {
    {0x00, {0, 0, 0}, &em019_se_alt_rec_28},
    {0xFF, {0, 0, 0}, 0},
};

/* Action 11: plays the alternate sound table of the sub-state at +0x1E6 (table 0 by default). */
void em019_act11_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_0, 0, 0);
        break;
    case 11:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_11, 0, 11);
        break;
    case 15:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_15, 0, 15);
        break;
    case 26:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_26, 0, 26);
        break;
    case 28:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_28, 0, 28);
        break;
    case 14:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_15, 0, 14);
        break;
    case 52:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_15, 0, 52);
        break;
    default:
        em_se_tbl_play_alt(self, em019_se_alt_tbl_0, 0, 0);
        break;
    }
}

/* The em019 action start: runs action 6's slam step. */
void em019_action_start(_ENEMY_WORK* self)
{
    em019_act6_sub2(self);
}

/* The em019 action start's idle gate: runs the action only from sub-state 0. */
void em019_action_start_if_idle(_ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em019_action_start(self);
    }
}

/* The demo entrance's key tables (`em_demo_key_apply`): frame/value pairs ending at frame -1. */
f32 em019_demo_key_pos[62] = {
    236.0f, 0.0f,
    299.0f, 0.0f,
    300.0f, 877.51f,
    302.0f, 1258.3f,
    304.0f, 1419.88f,
    306.0f, 1581.47f,
    308.0f, 1743.05f,
    310.0f, 1904.63f,
    311.0f, 1985.43f,
    389.0f, 1985.43f,
    390.0f, 2829.04f,
    498.0f, 2829.04f,
    499.0f, 1951.53f,
    599.0f, 1951.53f,
    600.0f, 1667.58f,
    602.0f, 1467.79f,
    604.0f, 1268.01f,
    606.0f, 1068.22f,
    608.0f, 868.44f,
    610.0f, 59.91f,
    612.0f, -53.03f,
    614.0f, -60.86f,
    616.0f, -64.09f,
    618.0f, -63.58f,
    620.0f, -60.36f,
    622.0f, -55.51f,
    624.0f, -50.08f,
    626.0f, -45.07f,
    628.0f, -41.4f,
    699.0f, -39.98f,
    -1.0f, -39.98f,
};

f32 em019_demo_key_rot[12] = {
    236.0f, 0.0f,
    600.0f, 0.0f,
    602.0f, -10.1f,
    610.0f, -90.89f,
    699.0f, -90.89f,
    -1.0f, -90.89f,
};

/* Program channel 0: the demo entrance, a timed sequence of motions, demo positions and landing effects. */
void em_act_prog_0(_ENEMY_WORK* self)
{
    EmSpawnRec rec;
    nw4r::math::VEC3 pos;

    em_spawn_rec_init(&rec);
    VEC3_ctor(&pos);
    em_hide_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        em019_part_state_set(self, 0xFF);
        break;
    case 1:
        if (em_demo_time_ck(200) == 1) {
            self->state++;
            em_mot_set(self, 205, 0, 0);
            em_demo_pos_set(self, 0.0f, 0.0f, 1574.645f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em_demo_enable(self);
            em019_part_state_set(self, 0xFF);
            em_shake_req_set(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 2, 94.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 101, 0, 1.0f);
        if (em_frame_check(self, 3, 12.0f, 62.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft009_spawn_at_joint(self, 25, 99, 0, 1.0f);
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 100, 0, 1.0f);
        if (em_demo_time_ck(300) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 206, 0, 0);
            em_demo_pos_set(self, 0.0f, 877.5102f, 1574.645f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em019_part_state_set(self, 0xFF);
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 100, 25, &pos, 1.0f);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 30.0f, 60.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 99, 25, &pos, 1.0f);
        }
        if (em_demo_time_ck(390) == 1)
            em_demo_pos_set(self, 728.17f, 2829.04f, -533.56f);
        em_demo_key_apply(self, em_demo_frame_get(), em019_demo_key_pos, em019_demo_key_rot, 2, 2);
        if (em_demo_time_ck(500) == 1) {
            self->state++;
            em_mot_set(self, 207, 0, 0);
            em_demo_pos_set(self, 728.1702f, 1951.5258f, -533.5574f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em019_part_state_set(self, 0xFF);
            em_shake_req_set(self);
        }
        break;
    case 4:
        if (em_frame_check(self, 2, 86.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 3, 2.0f, 80.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 99, 25, &pos, 1.0f);
        }
        if (em_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 98, 25, &pos, 1.0f);
        }
        em_demo_key_apply(self, em_demo_frame_get(), em019_demo_key_pos, em019_demo_key_rot, 2, 2);
        if (em_demo_time_ck(600) == 1) {
            self->state++;
            em_mot_set(self, 214, 0, 0);
            em_demo_pos_set(self, 728.1702f, 1667.5762f, -533.5574f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em019_part_state_set(self, 0xFF);
        }
        break;
    case 5:
        em_demo_key_apply(self, em_demo_frame_get(), em019_demo_key_pos, em019_demo_key_rot, 2, 2);
        if (em_demo_time_ck(610) == 1) {
            self->state++;
            em_mot_set(self, 208, 0, 0);
            em_demo_pos_set(self, 728.1702f, 59.9131f, -533.5574f);
            em_demo_rot_set(self, 0.0f, -90.8909f, 0.0f);
            em019_part_state_set(self, 0xFF);
            eft009_spawn_at_joint(self, 25, 92, 0, 1.0f);
            eft_spawn_type_at_area(self, 9);
            em019_spawn_rec_set(&rec, 0, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 24);
        }
        break;
    case 6:
        if (em_frame_check(self, 3, 30.0f, 62.0f) == 1 && (system_w.field_0x0c & 7) == 0)
            eft009_spawn_at_joint(self, 25, 111, 0, 1.0f);
        if (em_frame_check(self, 0, 30.0f, 0.0f) == 1)
            eft009_spawn_at_joint(self, 25, 108, 0, 1.0f);
        if (em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em019_spawn_rec_set(&rec, 2, 0, 0xE000);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 24);
        }
        if (em_frame_check(self, 1, 78.0f, 0.0f) == 1)
            em_shake_req_set(self);
        em_demo_key_apply(self, em_demo_frame_get(), em019_demo_key_pos, em019_demo_key_rot, 2, 2);
        if (em_demo_time_ck(700) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 203, 0, 0);
            em_demo_pos_set(self, 98.5533f, 0.0f, -6129.5186f);
            em_demo_rot_set(self, 0.0f, 2.1178f, 0.0f);
            em019_part_state_set(self, 0xFF);
        }
        break;
    case 7:
        if (em_demo_time_ck(886) == 1) {
            self->state++;
            em_mot_set(self, 203, 0, 0);
            em_demo_pos_set(self, 220.0189f, 0.0f, -3654.3633f);
            em_demo_rot_set(self, 0.0f, 2.1178f, 0.0f);
            em019_part_state_set(self, 0xFF);
        }
        break;
    case 8:
        if (em_demo_time_ck(1050) == 1) {
            self->state++;
            em_mot_set(self, 202, 0, 70);
            em_demo_pos_set(self, 0.0f, 0.0f, 0.0f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em019_part_state_set(self, 0xFF);
        }
        break;
    case 9:
        if (em_demo_time_ck(1168) == 1) {
            self->state++;
            em_mot_set(self, 202, 0, 194);
            em_demo_pos_set(self, 0.0f, 0.0f, -202.678f);
            em019_part_state_set(self, 0xFF);
        }
        break;
    case 10:
        if (em_demo_time_ck(1250) == 1) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, 0.0f, 0.0f, 0.0f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            self->field_0x1E4 = 0;
        }
        break;
    case 11:
        if (em_demo_time_ck(1620) == 1) {
            self->state++;
            em_mot_set(self, 20, 0, 0);
            em_demo_pos_set(self, 0.0f, 0.0f, 0.0f);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        }
        break;
    }
}

/* Program channel 1: phase 0 clears +0x1E4, the four part slots and +0x358/+0x35A and arms motion 20,
 * phase 1 ends the program when the motion is done. */
extern "C" void em_act_prog_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
        s32 i;

        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 0, 0);
        em_demo_pos_set(self, 0.0f, 0.0f, 0.0f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        em_demo_enable(self);
        self->field_0x1E4 = 0;
        for (i = 0; i < 4; i++) {
            part->colour_id[i] = 0;
            part->timer[i] = 0;
            part->meter[i] = 0.0f;
            part->flag[i] = 0;
            part->r[i] = 0;
            part->g[i] = 0;
            part->b[i] = 0;
            part->a[i] = 0;
        }
        self->field_0x358 = 0;
        self->tev_0x35A = 0;
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Program channel 2: phase 0 arms motion 201/4, phase 1 runs four frame windows that latch the part
 * helpers, then hands +0x1E4 to part 13/3. */
extern "C" void em_act_prog_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 201, 4, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 1, 196.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 13, 10);
        if (em_frame_check(self, 0, 56.0f, 0.0f) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, 62.0f, 0.0f) == 1)
            em_hit_window_clear(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 4);
            em_height_set(self, fn_8013032C(self));
            em_state_set(self, 13, 3);
        }
        break;
    }
}

/* Program channel 3: phase 0 arms motion 4 and the `em019_turn_tbl` sequence, phase 1 polls it and lands
 * +0x1E4 on part 13/4 (`fn_80131E00` runs every frame). */
extern "C" void em_act_prog_3(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em019_turn_tbl, 0, 0, 0);
        em_mot_speed_set(self, 5.0f);
        em_height_set(self, fn_8013032C(self));
        em019_part_state_set(self, 255);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em019_turn_tbl) == 1)
            em_state_set(self, 13, 4);
        break;
    }
}

/* Program channel 4: phase 0 arms motion 89 and the `em019_turn_tbl` table, phase 1 waits 256 frames and
 * lands +0x1E4 on part 13/5. */
extern "C" void em_act_prog_4(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 89, 0, 0);
        em_approach_start(self, 0.0f, 0);
        em_mot_speed_set(self, 5.0f);
        em_height_set(self, fn_8013032C(self));
        em019_part_state_set(self, 255);
        break;
    case 1:
        if (em_approach_step(self, 0, 256) == 1)
            em_state_set(self, 13, 5);
        break;
    }
}

/* Program channel 5: phase 0 arms motion 202 and two hit windows, phase 1 runs three frame windows, then
 * lands the part 13/6 hit or ends the program. */
extern "C" void em_act_prog_5(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 202, 0, 0);
        em_height_set(self, 0.0f);
        em019_part_state_set(self, 255);
        em_shake_req_set(self);
        em_hit_window_set(self, 0, 32, 8);
        em_hit_window_set(self, 1, 31, 24);
        fn_80131E00(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 90.0f, 0.0f) == 1)
            em_shake_req_set(self);
        if (em_frame_check(self, 2, 200.0f, 0.0f) == 1)
            fn_80131E00(self);
        if (em_frame_check(self, 0, 220.0f, 0.0f) == 1) {
            nw4r::math::VEC3 off;

            em_camera_req(self, -1, 5);
            copyVec3(&pos, ((Fn80041E8C)setVec3)(&off, 0.0f, 0.0f,
                                                        110.0f));
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 9, 1, &pos, 1.5f, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (em019_ally_wait_ck(self, 0) == 1)
                em_state_set(self, 13, 6);
            else
                em_action_finish(self);
        }
        break;
    }
}

/* Program channel 6: phase 0 measures the bearing to +0x36C and arms motion 31 or 32, phases 1 and 2 run the
 * turn and the `em019_turn_tbl` sequence, phase 3 hands on. */
extern "C" void em_act_prog_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u32 angle;

        self->state++;
        em_move_mode_set(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000)
            self->state_0x006 = 1;
        else
            self->state_0x006 = 0;
        switch (self->state_0x006) {
        case 0:
            em_mot_set(self, 31, 10, 0);
            if (angle < 0x2000)
                angle = 0x2000;
            else if (angle > 0x6000)
                angle = 0x6000;
            break;
        case 1:
            em_mot_set(self, 32, 10, 0);
            if (angle < 0xA000)
                angle = -0x6000;
            else if (angle > 0xE000)
                angle = -0x2000;
            break;
        }
        em_move_vec2_clr(self);
        self->field_0x318 = -14.0f * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000)
            angle = 0x10000 - angle;
        self->timer_0x020 = (s16)angle;
        em_hit_window_set_default(self, 0, 30);
        break;
    }
    case 1:
        switch (self->state_0x006) {
        case 0:
            em_turn_in_window(self, 10.0f, 50.0f, self->timer_0x020);
            break;
        case 1:
            em_turn_in_window(self, 10.0f, 50.0f, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, 20.0f, 70.0f) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_turn_seq_start(self, (void*)em019_turn_tbl, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step(self, (void*)em019_turn_tbl) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_move_mode_set(self, 0);
            em_mot_set(self, 2, 10, 0);
            em_approach_start(self, -900.0f, 0);
        }
        break;
    case 3:
        switch (self->state_0x006) {
        case 0:
            if (em_approach_step(self, 0, 128) != 1)
                break;
            self->state_0x006++;
            if (em_busy_ck(self) == 0)
                fn_8012F5C4(self, 20, 20, 0, 1);
            /* fall through */
        case 1:
            if (em_busy_ck(self) == 1) {
                if (em019_ally_wait_ck(self, 1) == 1)
                    em_state_set(self, 13, 7);
                else
                    em_action_finish(self);
            }
            break;
        }
        break;
    }
}

/* Program channel 7: phase 0 arms motion 6/10, a 300-frame countdown and the `em_drop_item_spawn` job, phase 1
 * lands +0x1E4 on part 13/8 when the countdown ends. */
extern "C" void em_act_prog_7(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        self->timer_0x020 = 300;
        em_drop_item_spawn(self, &self->pos, 60);
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 13, 8);
        break;
    }
}

/* The enemy work's action-program channel 8: phase 0 arms motion 10 sub-motion 6 and sets the
 * 1000-frame wait at +0x020; phase 1 ends the program once `em_mot_end_ck` reports the motion done. */
extern "C" void em_act_prog_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 6, 0);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* The program dispatcher: tail-calls the step of the program id at +0x1E6 through the 9-entry jump table
 * (case 0 is `em_act_prog_0`). */
extern "C" void em_act_prog_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_act_prog_0(self);
        break;
    case 1:
        em_act_prog_1(self);
        break;
    case 2:
        em_act_prog_2(self);
        break;
    case 3:
        em_act_prog_3(self);
        break;
    case 4:
        em_act_prog_4(self);
        break;
    case 5:
        em_act_prog_5(self);
        break;
    case 6:
        em_act_prog_6(self);
        break;
    case 7:
        em_act_prog_7(self);
        break;
    case 8:
        em_act_prog_8(self);
        break;
    }
}

/* The per-frame action runner: dispatches on the action id, ticks +0x35A while +0x358 is 1 (clamped at 450)
 * and runs the +0x1E2 hook pair. */
extern "C" void em_act_run(_ENEMY_WORK* self) {
    self->field_0x358 = 0;
    switch (self->action) {
    case 0:
        em019_motion_dispatch(self);
        break;
    case 1:
        em019_state_dispatch(self);
        break;
    case 2:
        em019_act2_dispatch(self);
        break;
    case 3:
        em019_approach_dispatch(self);
        break;
    case 6:
        em019_battle_dispatch(self);
        break;
    case 7:
        em019_act7_dispatch(self);
        break;
    case 10:
        em019_act10_se_play(self);
        break;
    case 11:
        em019_act11_se_play(self);
        break;
    case 12:
        em019_action_start_if_idle(self);
        break;
    case 13:
        em_act_prog_dispatch(self);
        break;
    default:
        em_action_finish(self);
        break;
    }
    if (self->field_0x358 == 1) {
        if (++self->tev_0x35A > 450)
            self->tev_0x35A = 450;
    } else {
        self->tev_0x35A = 0;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* The per-part damage check: per slot, raise the meter (`em_part_rec_reset`) while unarmed and below the
 * threshold, else tick it down (`em_part_rec_alt_set`). */
extern "C" void em_parts_damage_ck(_ENEMY_WORK* self) {
    if (em019_part_state_get(self, 2) == 0 && em_parts_damage_level_get(self, 0) < 2)
        em_part_rec_reset(self, 0);
    else
        em_part_rec_alt_set(self, 0, 0);
    if (em019_part_state_get(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 1);
    else
        em_part_rec_alt_set(self, 1, 1);
    if (em019_part_state_get(self, 1) == 0 && em_parts_damage_level_get(self, 1) < 2)
        em_part_rec_reset(self, 2);
    else
        em_part_rec_alt_set(self, 2, 2);
    if (em019_part_state_get(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 2) < 1 || em_parts_damage_level_get(self, 3) < 1))
        em_part_rec_reset(self, 3);
    else
        em_part_rec_alt_set(self, 3, 3);
    if (em019_part_state_get(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 4) < 1 || em_parts_damage_level_get(self, 5) < 1))
        em_part_rec_reset(self, 4);
    else
        em_part_rec_alt_set(self, 4, 4);
    if (em019_part_state_get(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 5);
    else
        em_part_rec_alt_set(self, 5, 5);
    if (em019_part_state_get(self, 3) == 0 && em_parts_damage_level_get(self, 7) < 1)
        em_part_rec_reset(self, 6);
    else
        em_part_rec_alt_set(self, 6, 6);
}

/* Every twentieth frame, drops the record's own ground-stamp vector: builds the (0, -20, 100) offset
 * and hands it to `eft_spawn_type10` with the record's own flag. */
extern "C" void em_act_effect_ck(_ENEMY_WORK* self) {
    nw4r::math::VEC3 off;

    VEC3_ctor(&off);
    if (em_alt_mode_ck(self) == 1) {
        if (system_w.field_0x0c % 20 == 0) {
            setVector3(&off, 0.0f, -20.0f, 100.0f);
            eft_spawn_type10(self, 25, 25, &off, 1.0f);
        }
    }
}

/* Per-frame motion events: the effects and camera cues each motion fires on its frames (the broken-part
 * variants when +0x228 has bits 1-2 set). */
void em_act_mot_step(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 pos;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&pos);
    em_parts_damage_ck(self);
    em_act_effect_ck(self);
    switch (em_get_mot_no(self)) {
    case 2:
        if ((self->field_0x228 & 6) != 0 &&
            (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 68.0f, 0.0f) == 1))
            eft009_spawn_at_joint(self, 19, 14, 0, 2.0f);
        break;
    case 9:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 13, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 8, 14, 0, 1.5f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 6, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
                break;
            }
        }
        break;
    case 21:
        if (((self->field_0x228 & 6) != 0) && ((em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1))) {
            eft009_spawn_at_joint(self, 19, 14, 0, 2.0f);
            break;
        }
        break;
    case 22:
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 19, 14, 0, 2.0f);
            break;
        }
        break;
    case 24:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 14, 12, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 15, 0, 0.7f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 14, 2, 0, 1.2f);
                break;
            }
        }
        break;
    case 25:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 12, 0, 1.3f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 15, 0, 1.3f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 2, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.2f);
                break;
            }
        }
        break;
    case 27:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 3, 12, 0, 2.0f);
                eft009_spawn_at_joint(self, 3, 14, 0, 2.0f);
                break;
            }
        } else if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 8, 0, 3.5f);
            break;
        }
        break;
    case 31:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 8, 14, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 14, 14, 0, 1.5f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 2, 0, 1.5f);
                break;
            }
        }
        break;
    case 32:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 14, 14, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 8, 14, 0, 1.5f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 2, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
                break;
            }
        }
        break;
    case 81:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 13, 0, 1.5f);
                eft009_spawn_at_joint(self, 14, 15, 0, 1.5f);
            }
        } else if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 14, 0, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 82:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 142.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 16, 18, 0, 1.7f);
            }
            if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 31, 18, 0, 1.4f);
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 44, 18, 0, 1.2f);
            }
        } else {
            if (em_after_frame_check(self, 0, 142.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 16, &pos);
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 6, 0, 2.3f * get_em_chg_scale(self));
            }
            if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 31, &pos);
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 6, 0, 2.0f * get_em_chg_scale(self));
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 44, &pos);
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 6, 0, 1.7f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 83:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 26, 14, 0, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 26, 12, 0, 2.0f);
                break;
            }
        } else if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 26, 8, 0, 2.5f);
            break;
        }
        break;
    case 84:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 26, 14, 0, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 26, 12, 0, 2.0f);
                break;
            }
        } else if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 26, 8, 0, 2.5f);
            break;
        }
        break;
    case 85:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1)) {
                setVector3(&pos, 0.0f, 0.0f, 20.0f);
                eft009_spawn_at_pos(self, 19, 12, 0, 2.5f, &pos);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 19, 17, 0, 1.0f);
                break;
            }
        } else {
            if ((em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1)) {
                setVector3(&pos, 0.0f, 0.0f, 20.0f);
                eft009_spawn_at_pos(self, 19, 8, 0, 2.0f, &pos);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 19, 8, 0, 2.5f);
                break;
            }
        }
        break;
    case 89:
        if (self->field_0x1E2 == 0) {
            if ((em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1)) {
                em_camera_req(self, 12, 0);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em_camera_req(self, 6, 0);
                break;
            }
        }
        break;
    case 121:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 18, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 18, 0, 1.0f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 6, 0, 1.6f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 6, 0, 1.6f);
                break;
            }
        }
        break;
    case 122:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 18, 20, 0, 2.0f);
            }
        } else if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
            eft_spawn_pos_in_area(&pos, self->area_no, 4, self->field_0x1C0, get_em_chg_scale(self));
        }
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 124:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 18, 0, 0.8f);
                break;
            }
        } else if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
            break;
        }
        break;
    case 125:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 3, 17, 0, 1.3f);
            }
        } else if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            eft_spawn_pos_in_area(&self->pos, self->area_no, 4, self->field_0x1C0, get_em_chg_scale(self));
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 127:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 18, 0, 0.8f);
                break;
            }
        } else if (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
            break;
        }
        break;
    case 129:
        if (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 17, 0, 1.3f);
                break;
            }
            setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
            eft_spawn_pos_in_area(&pos, self->area_no, 4, 0, 0.8f * get_em_chg_scale(self));
            break;
        }
        break;
    case 130:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 14, 18, 0, 0.8f);
                break;
            }
        } else if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 14, 6, 0, 1.8f);
            break;
        }
        break;
    case 131:
        if (em_after_frame_check(self, 0, 328.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 16, 0, 1.2f);
            } else {
                get_joint_wpos_em(self, 3, &pos);
                pos.y = self->pos.y;
                eft_spawn_pos_in_area(&pos, self->area_no, 1, 0, 1.2f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 326.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 132:
        if ((em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 17, 0, 1.3f);
                break;
            }
            setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
            eft_spawn_pos_in_area(&pos, self->area_no, 0, 0, 1.5f * get_em_chg_scale(self));
            break;
        }
        break;
    case 134:
        if (((self->field_0x228 & 6) == 0) && (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 91, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 25, 12, 0, 3.0f);
                eft009_spawn_at_joint(self, 25, 14, 0, 3.0f);
                break;
            }
            eft009_spawn_at_joint(self, 25, 2, 0, 3.0f);
            break;
        }
        break;
    case 136:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 20, 0, 2.3f);
            } else {
                setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&pos, self->area_no, 2, 0, 1.2f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 139:
        if (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 20, 17, 0, 1.2f);
            } else {
                get_joint_wpos_em(self, 20, &pos);
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 0, 0, 1.5f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 140:
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 17, 0, 1.0f);
            } else {
                setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&pos, self->area_no, 5, 0, get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 141:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 12, 0, 1.5f);
                eft009_spawn_at_joint(self, 8, 14, 0, 1.5f);
                break;
            }
        } else if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 8, 2, 0, 1.8f);
            break;
        }
        break;
    case 200:
        if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 25, 128, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 25, 127, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 150.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 156.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 162.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 168.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 174.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 25, 128, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1)) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 135, 25, &pos, 1.0f);
        }
        if (((em_after_frame_check(self, 3, 0.0f, 100.0f) == 1) || (em_after_frame_check(self, 3, 146.0f, 180.0f) == 1)) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) {
            em_camera_req(self, 25, 7);
            break;
        }
        break;
    case 201:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 88, 0, 1.0f);
            eft009_spawn_at_joint(self, 25, 89, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 110.0f, 206.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            eft009_spawn_at_joint(self, 25, 90, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em_camera_req(self, 25, 7);
        }
        if ((em_after_frame_check(self, 3, 110.0f, 200.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 202:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 107, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 91, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 120.0f, 216.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 119, 18, &pos, 2.0f);
        }
        if (em_after_frame_check(self, 0, 216.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft_spawn_type11(self, &self->pos, 2, 1.1f);
            } else {
                setVector3(&pos, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&pos, self->area_no, 4, 0, 1.1f * get_em_chg_scale(self));
            }
        }
        if ((em_after_frame_check(self, 3, 330.0f, 346.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 119, 25, &pos, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 0.0f, 110.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 203:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 127, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 135, 25, &pos, 1.0f);
        }
        if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 127, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 116.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 25, 128, 0, 1.0f);
        }
        if (((em_after_frame_check(self, 3, 0.0f, 54.0f) == 1) || (em_after_frame_check(self, 3, 108.0f, 130.0f) == 1)) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em_camera_req(self, 25, 7);
            break;
        }
        break;
    case 204:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 199);
        }
        if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 205:
        if ((em_after_frame_check(self, 1, 10.0f, 0.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 208:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        if ((em_after_frame_check(self, 3, 32.0f, 64.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 209:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 197);
        }
        if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 210:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            setVector3(&pos, 100.0f, 0.0f, 80.0f);
            eft009_spawn_at_pos(self, 25, 95, 0, 1.0f, &pos);
        }
        if (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            setVector3(&pos, -120.0f, 0.0f, 0.0f);
            eft009_spawn_at_pos(self, 25, 95, 0, 1.0f, &pos);
        }
        if (em_after_frame_check(self, 0, 202.0f, 0.0f) == 1) {
            setVector3(&pos, 120.0f, 0.0f, 0.0f);
            eft009_spawn_at_pos(self, 25, 95, 0, 1.0f, &pos);
        }
        if (em_after_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            setVector3(&pos, -120.0f, 0.0f, 30.0f);
            eft009_spawn_at_pos(self, 25, 95, 0, 1.0f, &pos);
        }
        if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 202.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 278.0f, 0.0f) == 1)) {
            em_camera_req(self, 25, 7);
            break;
        }
        break;
    case 211:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 109, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 25, 110, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 1, 110.0f, 0.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            eft009_spawn_at_joint(self, 25, 93, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        if ((em_after_frame_check(self, 1, 110.0f, 0.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            em_camera_req(self, -1, 0);
            break;
        }
        break;
    case 215:
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    default:
        break;
    }
}

extern "C" {

/* Resets one part slot: stores the caller's K-colour index, clears the slot's timer, meter, arm byte
 * and alpha, and takes the slot's RGB either from the caller's colour or from the shared id table. */
extern "C" void em_part_reset(_ENEMY_WORK* self, u8 idx, u8 colour_id, const u8* colour) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    part->colour_id[idx] = colour_id;
    part->timer[idx] = 0;
    part->meter[idx] = 0.0f;
    part->flag[idx] = 0;
    if (colour == NULL) {
        GXColor tmp;

        ((MHchar*)self->char_0x024)->getTevKColor(em019_part_kcolor_tbl[idx], GX_KCOLOR3, &tmp);
        part->r[idx] = tmp.r;
        part->g[idx] = tmp.g;
        part->b[idx] = tmp.b;
    } else {
        part->r[idx] = colour[0];
        part->g[idx] = colour[1];
        part->b[idx] = colour[2];
    }
    part->a[idx] = 0;
}

/* Lerps one part slot's RGB toward a target colour by the slot's alpha over the caller's ceiling,
 * and stamps the caller's alpha on the result. */
extern "C" void em_part_colour_lerp(_ENEMY_WORK* self, u8* out, u8 idx, const u8* colour, u8 ceiling) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    f32 t = (f32)part->a[idx] / (f32)ceiling;

    out[0] = part->r[idx] + (s32)((f32)(colour[0] - part->r[idx]) * t);
    out[1] = part->g[idx] + (s32)((f32)(colour[1] - part->g[idx]) * t);
    out[2] = part->b[idx] + (s32)((f32)(colour[2] - part->b[idx]) * t);
    out[3] = 255;
}

/* Steps one part slot's damage meter by 1/damage, arming the slot when it reaches 1.0 and disarming
 * it when it falls back to 0.0. */
extern "C" void em_part_damage_meter(_ENEMY_WORK* self, u8 idx, f32 damage) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    if (damage <= 0.0f)
        return;
    if (part->flag[idx] == 0) {
        part->meter[idx] += 1.0f / damage;
        if (part->meter[idx] >= 1.0f) {
            part->meter[idx] = 1.0f;
            part->flag[idx] = 1;
        }
    } else {
        part->meter[idx] -= 1.0f / damage;
        if (part->meter[idx] <= 0.0f) {
            part->meter[idx] = 0.0f;
            part->flag[idx] = 0;
        }
    }
}
}



/* Steps the four part colours: re-arms a slot whose +0x1E4 state changed, ramps it toward the state's colour,
 * pulses broken parts by their damage meter, decays the state on its timer, and writes the K-colours back. */
void em019_tev_color_update(_ENEMY_WORK* self)
{
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    u8 i;

    for (i = 0; i < 4; i++) {
        u32 state;
        GXColor kcolor;
        GXColor colour;
        GXColor target;

        ((MHchar*)self->char_0x024)->getTevKColor(em019_part_kcolor_tbl[i], GX_KCOLOR3, &kcolor);
        state = (self->field_0x1E4 >> (i * 2)) & 3;
        if (state != part->colour_id[i])
            em_part_reset(self, i, state, (u8*)&kcolor);
        switch (part->colour_id[i]) {
        case 3:
            target.r = 255;
            target.g = 255;
            target.b = 255;
            target.a = 255;
            if (part->a[i] < 60) {
                em_part_colour_lerp(self, (u8*)&colour, i, (u8*)&target, 60);
                part->a[i]++;
            } else {
                color_rgba_copy((u8*)&colour, (u8*)&target);
                part->timer[i]++;
                if (part->timer[i] > 120)
                    em_part_reset(self, i, 2, (u8*)&kcolor);
            }
            break;
        case 2:
            target.r = 210;
            target.g = 160;
            target.b = 130;
            target.a = 255;
            if (part->a[i] < 180) {
                em_part_colour_lerp(self, (u8*)&colour, i, (u8*)&target, 180);
                part->a[i]++;
            } else {
                color_rgba_copy((u8*)&colour, (u8*)&target);
                f32* meter = &part->meter[i];

                colour.r += (u8)(45.0f * *meter);
                colour.g += (u8)(95.0f * *meter);
                colour.b += (u8)(125.0f * *meter);
                em_part_damage_meter(self, i, 60.0f);
                part->timer[i]++;
                if (part->timer[i] > 300)
                    em_part_reset(self, i, 1, (u8*)&kcolor);
            }
            break;
        case 1:
            target.r = 160;
            target.g = 60;
            target.b = 0;
            target.a = 255;
            if (part->a[i] < 180) {
                em_part_colour_lerp(self, (u8*)&colour, i, (u8*)&target, 180);
                part->a[i]++;
            } else {
                color_rgba_copy((u8*)&colour, (u8*)&target);
                f32* meter = &part->meter[i];

                colour.r += (u8)(60.0f * *meter);
                colour.g += (u8)(20.0f * *meter);
                em_part_damage_meter(self, i, 30.0f);
                part->timer[i]++;
                if (part->timer[i] > 180)
                    em_part_reset(self, i, 0, (u8*)&kcolor);
            }
            break;
        case 0:
            target.r = 0;
            target.g = 0;
            target.b = 0;
            target.a = 255;
            if (part->a[i] < 180) {
                em_part_colour_lerp(self, (u8*)&colour, i, (u8*)&target, 180);
                part->a[i]++;
            } else {
                color_rgba_copy((u8*)&colour, (u8*)&target);
                colour.r += (u8)(20.0f * part->meter[i]);
                em_part_damage_meter(self, i, 60.0f);
            }
            break;
        }
        ((MHchar*)self->char_0x024)->setTevKColor(em019_part_kcolor_tbl[i], GX_KCOLOR3, &colour);
    }
    self->field_0x1E4 = part->colour_id[0] | (part->colour_id[1] << 2) | (part->colour_id[2] << 4) |
                        (part->colour_id[3] << 6);
}

/* Picks, among `count` consecutive spawn points from `first`, the farthest one within 3500 units of the
 * target position, or the nearest one when none is that close. */
u8 em019_spawn_point_pick(_ENEMY_WORK* self, u8 first, u8 count)
{
    nw4r::math::VEC3 pos;
    f32 best;
    s32 i;
    u8 result;

    VEC3_ctor(&pos);
    for (i = 0; i < count; i++, first++) {
        em_data_point_get(self, ((self->area_no & 0xF) << 8) | first, &pos);
        if (i == 0) {
            best = calcDistanceSqXZ(&self->vec_0x36C, &pos);
            result = i;
        } else {
            f32 dist = calcDistanceSqXZ(&self->vec_0x36C, &pos);
            u32 take = 0;

            if (best <= 12250000.0f) {
                if (best < dist && dist <= 12250000.0f)
                    take = 1;
            } else if (dist <= 12250000.0f) {
                take = 1;
            } else if (best > dist) {
                take = 1;
            }
            if (take == 1) {
                best = dist;
                result = i;
            }
        }
    }
    return result;
}

/* Answers the program driver's state queries: the stand state (0), the area/ground state (1), the run
 * counter's cap (2), the off-lava test (3) and the spawn point of the current area (4). */
u8 em019_state_query(_ENEMY_WORK* self, u8 kind)
{
    u32 on_area = 0;

    switch (kind) {
    case 0:
        if (self->field_0x1E2 == 4)
            return 1;
        if ((u32)em_act_ck(self, 3, 0) == 1 || (u32)em_act_ck(self, 3, 1) == 1 || (u32)em_act_ck(self, 3, 2) == 1)
            return 2;
        return 0;
    case 1:
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 5 && self->area_no == 8)
            on_area = 1;
        if (on_area == 1)
            return 2;
        return self->field_0x23C != 0;
    case 2:
        return self->tev_0x35A >= 450;
    case 3:
        return em_magma_check(self) != 1;
    case 4:
        switch (stage_map_kind_get(self->field_0x1E0)) {
        case 5:
            switch (self->area_no) {
            case 4:
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 8, 4);
                return em019_spawn_point_pick(self, 4, 4);
            case 5:
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 7, 4);
                return em019_spawn_point_pick(self, 3, 4);
            case 7:
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 8, 5);
                return em019_spawn_point_pick(self, 3, 5);
            case 9:
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 4, 2);
                return em019_spawn_point_pick(self, 2, 2);
            }
            break;
        case 8:
            if ((s32)self->area_no == 1) {
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 3, 3);
                return em019_spawn_point_pick(self, 0, 3);
            }
            break;
        case 9:
            if ((s32)self->area_no == 0) {
                if (self->field_0x010 == 1)
                    return em019_spawn_point_pick(self, 3, 3);
                return em019_spawn_point_pick(self, 0, 3);
            }
            break;
        }
        return 0xFF;
    }
    return 0;
}


/* Sets the down action the program driver arms on death: the move mode 4 and action 12/0. */
void em019_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub)
{
    em_move_mode_set(self, 4);
    *action = 12;
    *sub = 0;
}

/* The program table's empty slot. */
void em019_noop(void)
{
}

/* Answers whether part 7 is damaged at all. */
u32 em019_part7_damaged_ck(_ENEMY_WORK* self)
{
    return em_parts_damage_level_get(self, 7) >= 1;
}

/* Answers whether the work stands (state 4) outside its alternate mode. */
s32 em019_stand_ck(_ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 4) {
        if (em_alt_mode_ck(self) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Decides an area change on the volcano map (kind 5): the plain area move, the follow move after 900 frames
 * of the area timer, the target's area, or the escape. */
u32 em019_area_change_ck(_ENEMY_WORK* self, u16 timer)
{
    u8 map = stage_map_kind_get(self->field_0x1E0);
    u32 moved;
    u32 area;

    if ((s32)map != 5)
        return 0;
    moved = 0;
    area = 0xFF;
    if ((s32)map == 5)
        area = 9;
    if (area != 0xFF) {
        switch (em_area_break_ck_c2(self, area)) {
        case 1:
            moved = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (moved == 0 && self->value_0x452 >= 900 &&
        em_pack_area_pick_c1(self, 29, (s32)map == 5 ? 9 : 0xFF, timer, (u8*)em019_follow_area_tbl, (u8*)em019_follow_point_tbl) == 1)
        return 1;
    if (em_area_roam_pick(self, timer) == 1)
        return 1;
    return em_area_leave_ck_c1(self) == 1;
}

/* Sets or clears the part flag at +0x740. */
void em019_part_flag_set(_ENEMY_WORK* self, u32 flag)
{
    if (flag == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* The user-data helper's part reset: flags the owner's part state and clears its six counters. */
void em019_user_data_part_reset(EmUserData* data)
{
    em_res_user_data_flag_set_c1(data, 5);
    data->work_0x04->part_0x740.field_0x740 = 1;
    data->work_0x04->part_0x740.field_0x742 = 0;
    data->work_0x04->part_0x740.field_0x744 = 0;
    data->work_0x04->part_0x740.field_0x746 = 0;
    data->work_0x04->part_0x740.field_0x748 = 0;
    data->work_0x04->part_0x740.field_0x74A = 0;
    data->work_0x04->part_0x740.field_0x74C = 0;
}

/* Answers whether the waiting ally (enemy 29 in action 11) is there and, when `near` is set, within
 * 400 units of the point 900 units ahead of this work. */
u32 em019_ally_wait_ck(_ENEMY_WORK* self, u8 near)
{
    nw4r::math::VEC3 ahead;
    nw4r::math::VEC3 offset;
    _ENEMY_WORK* ally;

    VEC3_ctor(&ahead);
    VEC3_ctor(&offset);
    ally = em_ally_find(self, 29, 0);
    if (ally != NULL && em_act11_wait_ck(ally) == 1) {
        nw4r::math::VEC3 step;
        nw4r::math::VEC3 sum;
        f32 dist;

        if (near == 0)
            return 1;
        copyVec3(&offset, ((Fn80041E8C)setVec3)(&step, 0.0f, 0.0f, 900.0f * get_em_chg_scale(self)));
        rotVecY(&offset, self->field_0x1C0);
        addVec3(&sum, &self->pos, &offset);
        copyVec3(&ahead, &sum);
        dist = calcDistanceSqXZ(&ahead, &ally->pos);
        if (dist < 400.0f * get_em_chg_scale(self) * (400.0f * get_em_chg_scale(self)))
            return 1;
    }
    return 0;
}

/* Answers 1 while the record is in action 13 with a sub-state of 2..8. */
u32 em019_action13_active_ck(_ENEMY_WORK* self)
{
    if (self->action == 13) {
        if ((u32)(self->state_sub - 2) <= 6) {
            return 1;
        }
    }
    return 0;
}

/* Fills a shell spawn record: kind 25 at the origin with the caller's three parameters. */
void em019_spawn_rec_set(EmSpawnRec* rec, u8 mode, u16 a, u16 angle)
{
    nw4r::math::VEC3 zero;

    setVec3(&zero, 0.0f, 0.0f, 0.0f);
    rec->id = 25;
    copyVec3(&rec->pos, &zero);
    rec->field_0x10 = mode;
    rec->field_0x12 = a;
    rec->field_0x14 = angle;
}

/* The user-data helper's destructor: runs the base destructor and frees the helper when asked to. */
EmUserData* em019_user_data_dtor(EmUserData* data, s32 flags)
{
    if (data != NULL) {
        em_res_user_data_dtor_c1(data, 0);
        if ((s16)flags > 0)
            operator delete(data);
    }
    return data;
}

/* Seeds the two sound-offset vector pairs the sound tables point at. */
void em019_se_shift_init(void)
{
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;

    assignVec3(&em019_se_shift[0], (Vec*)((Fn80041E8C)setVec3)(&a, 0.0f, 0.0f, 12.5f));
    assignVec3(&em019_se_shift[1], (Vec*)((Fn80041E8C)setVec3)(&b, 0.0f, 0.0f, 0.0f));
    assignVec3(&em019_se_shift[2], (Vec*)((Fn80041E8C)setVec3)(&c, 2.0f, 0.0f, 0.0f));
    assignVec3(&em019_se_shift[3], (Vec*)((Fn80041E8C)setVec3)(&d, 0.0f, 0.0f, 0.0f));
}

}
