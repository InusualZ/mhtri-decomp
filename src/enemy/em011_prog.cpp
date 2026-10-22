/*
 * enemy/em011_prog.cpp - enemy 011's program: the `em011_prog_tbl` entry points (init, frame update, action
 *   dispatch, motion events, state query, area start, idle and escape checks), the per-action state steps the
 *   `state_sub` dispatchers select, the two sound programs, the demo (cut-scene) steps, the effect router and the
 *   static initializer.
 * RANGE. .text 0x8016D1C4-0x80170600 (56 functions); extab 0x8000E3DC-0x8000E52C, extabindex
 *   0x800294B4-0x800296AC, .ctors 0x8056F330-0x8056F334, .rodata 0x8056FCD0-0x8056FD10, .data 0x805A7CE8-0x805A8370
 *   (`em011_prog_tbl` first), .bss 0x806A7970-0x806A79A0, .sdata2 0x807977D8-0x80797910.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `#pragma pool_data off` and `#pragma fp_contract off` file-wide.
 * NAMES. The file name follows the runtime dump's `em011_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em011ProgTbl` slot each fills, after `Em012ProgTbl`): em011_init, em011_frame_update,
 *   em011_action_dispatch, em011_noop, em011_event_notify, em011_motion_events, em011_tev_color_update,
 *   em011_state_ck, em011_area_start_set, em011_idle_ck, em011_escape_ck.
 *   GUESS (dispatcher slot): em011_act<action>_<first sub-state> and em011_act<action>_dispatch,
 *   em011_act10_se_play, em011_act11_se_play; GUESS (from the bodies): em011_guard_exit_2/_3,
 *   em011_act2_approach/_turn/_leap, em011_act12_hop/_approach, em011_demo_sleep_fx, em011_effect_spawn,
 *   em011_guard_state_update.
 *   GUESS (data): em011_turn_seq_0, em011_se_seq_<sub-state>, em011_se_seq_alt_<sub-state>,
 *   em011_se_rec_<seq>[_a..], em011_se_pos_0, em011_vec_pair_0/1, the `Em011ProgTbl` slot names.
 * RESIDUALS. none: every row, `.data`, `.rodata`, `.sdata2`, `.bss` and `.ctors` match (a trial flip links main.dol OK).
 * SHAPES. A case-level `return` at the end of a state switch costs a `b`: the steps `break`, the dispatchers keep
 *   `return`.  The sound data is defined between the functions in retail order (the jump tables of the action 1
 *   and 2 dispatchers come before it, the action dispatcher's after it).  The `.bss` vector pairs are built by
 *   MWCC's own `__sinit` from `Em011VecPair`'s constructor.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/EnemyData.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/em_act_arm_unless_act11.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/em_hit_window_set_default.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_mot_set_ck.h"
#include "enemy/em_mot_speed_set.h"
#include "enemy/em_approach_start.h"
#include "enemy/em_approach_step.h"
#include "enemy/em_hit_by_set.h"
#include "enemy/em_reaction_req.h"
#include "enemy/em_motion_kind_set.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"
#include "enemy/em_fx_flag_set.h"
#include "unsplit/enemy.h"
#include "ef.h"
#include "ef/eft009.h"
#include "ef/eft013_em_place.h"
#include "ef/eft_slot.h"
#include "ef/fn_8010D1A8.h"
#include "ef/fn_800CDB2C.h"
#include "stage/stg_w.h"
#include "stage/stg_area_flag_index_ck.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_act_arm_unless_act11_c1 ((void (*)(_ENEMY_WORK *, s32, s32))em_act_arm_unless_act11)
#define em_hit_window_set_default_c1 ((void (*)(_ENEMY_WORK *, s32, s32))em_hit_window_set_default)
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)

/* A point and its per-frame step, built by MWCC's `__sinit` (0x8017054C): each vector is set through `setVec3` into
 * a temporary and copied with `assignVec3`.  size: 0x18 */
struct Em011VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, y, z) and (0, dy, 0). */
    Em011VecPair(f32 y, f32 z, f32 dy)
    {
        VEC3 b;
        VEC3 a;

        assignVec3_c1(&pos, setVec3(&a, 0.0f, y, z));
        assignVec3_c1(&step, setVec3(&b, 0.0f, dy, 0.0f));
    }
};

/* The two vector pairs (`.bss`, defined at the end); three sound records point at them. */
extern Em011VecPair em011_vec_pair_0;
extern Em011VecPair em011_vec_pair_1;

/* The em011 program table (`em011_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em012ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em011ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(void);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ u8 (*state_ck_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ u32 pad_0x34[6];
    /* +0x4C */ s32 (*idle_ck_0x4C)(struct _ENEMY_WORK* self);
    /* +0x50 */ u32 pad_0x50;
    /* +0x54 */ s32 (*escape_ck_0x54)(struct _ENEMY_WORK* self);
    /* +0x58 */ u32 pad_0x58[5];
};

/* The `em_turn_seq_start`/`em_turn_seq_step` table (`.rodata`, 0x40 B): a four-word head, then three four-word
 * rows whose first word carries the motion in its high half. */
extern "C" const u32 em011_turn_seq_0[16] = {
    0x00000000, 0x00000014, 0x00000000, 0x40002000,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x00080000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" {

void em011_init(_ENEMY_WORK *self, u8 mode);
void em011_noop(void);
void em011_event_notify(void);
void em011_frame_update(_ENEMY_WORK *self);
void em011_guard_exit_3(_ENEMY_WORK *self);
void em011_guard_exit_2(_ENEMY_WORK *self);
void em011_act0_0(_ENEMY_WORK *self);
void em011_act0_1(_ENEMY_WORK *self);
void em011_act0_dispatch(_ENEMY_WORK *self);
void em011_act1_1(_ENEMY_WORK *self);
void em011_act1_2(_ENEMY_WORK *self);
void em011_act1_3(_ENEMY_WORK *self);
void em011_act1_4(_ENEMY_WORK *self);
void em011_act1_5(_ENEMY_WORK *self);
void em011_act1_6(_ENEMY_WORK *self);
void em011_act1_7(_ENEMY_WORK *self);
void em011_act1_8(_ENEMY_WORK *self);
void em011_act1_9(_ENEMY_WORK *self);
void em011_act1_10(_ENEMY_WORK *self);
void em011_act1_11(_ENEMY_WORK *self);
void em011_act1_dispatch(_ENEMY_WORK *self);
void em011_act2_approach(_ENEMY_WORK *self, u8 dist, u8 stop);
void em011_act2_turn(_ENEMY_WORK *self, u8 busy);
void em011_act2_6(_ENEMY_WORK *self);
void em011_act2_leap(_ENEMY_WORK *self, u8 kind);
void em011_act2_8(_ENEMY_WORK *self);
void em011_act2_dispatch(_ENEMY_WORK *self);
void em011_act7_0(_ENEMY_WORK *self);
void em011_act7_2(_ENEMY_WORK *self);
void em011_act7_3(_ENEMY_WORK *self, u8 chain);
void em011_act7_4(_ENEMY_WORK *self);
void em011_act7_dispatch(_ENEMY_WORK *self);
void em011_act10_se_play(_ENEMY_WORK *self);
void em011_act11_se_play(_ENEMY_WORK *self);
void em011_act12_hop(_ENEMY_WORK *self, u8 kind);
void em011_act12_approach(_ENEMY_WORK *self, u8 flag);
void em011_act12_3(_ENEMY_WORK *self, u8 next);
void em011_act12_dispatch(_ENEMY_WORK *self);
void em011_demo_sleep_fx(_ENEMY_WORK *self, u8 phase);
void em011_act13_0(_ENEMY_WORK *self);
void em011_act13_1(_ENEMY_WORK *self);
void em011_act13_2(_ENEMY_WORK *self);
void em011_act13_3(_ENEMY_WORK *self);
void em011_act13_4(_ENEMY_WORK *self);
void em011_act13_5(_ENEMY_WORK *self);
void em011_act13_dispatch(_ENEMY_WORK *self);
void em011_action_dispatch(_ENEMY_WORK *self);
void em011_effect_spawn(_ENEMY_WORK *self, u8 where, u8 id, s32 joint, s32 delta, f32 scale);
void em011_guard_state_update(_ENEMY_WORK *self);
void em011_motion_events(_ENEMY_WORK *self);
void em011_area_start_set(_ENEMY_WORK *self, u8 *action, u8 *sub);
void em011_tev_color_update(void);
u8 em011_state_ck(_ENEMY_WORK *self, u8 kind);
s32 em011_idle_ck(_ENEMY_WORK *self);
s32 em011_escape_ck(_ENEMY_WORK *self);

Em011ProgTbl em011_prog_tbl = {
    0,
    em011_init,
    em011_frame_update,
    em011_action_dispatch,
    em011_noop,
    em011_event_notify,
    em011_motion_events,
    0,
    em011_tev_color_update,
    em011_state_ck,
    {0, 0},
    em011_area_start_set,
    {0, 0, 0, 0, 0, 0},
    em011_idle_ck,
    0,
    em011_escape_ck,
    {0, 0, 0, 0, 0},
};

/* 0x8016D1C4 (0x68): marks the work as an em011 and, when it enters an area (mode 2), arms the area-start action. */
void em011_init(_ENEMY_WORK *self, u8 mode) {
    u8 action;
    u8 sub;

    self->field_0x835 = 1;
    switch (mode) {
    case 2:
        em011_area_start_set(self, &action, &sub);
        em_act_arm_unless_down(self, action, sub);
        em_state_refresh(self);
        break;
    }
    self->field_0x1E4 = 0;
}

/* 0x8016D22C (0x4): the empty motion-next slot. */
void em011_noop(void) {}

/* 0x8016D230 (0x4): the event slot; em011 reacts to none. */
void em011_event_notify(void) {}

/* 0x8016D234 (0x150): the per-frame step: starts the attack on a hit, follows the enemy data entry's aim point,
 * startles when the entry's leader starts its call, and re-enters the user data when the entry is gone. */
void em011_frame_update(_ENEMY_WORK *self) {
    _ENEMY_DATA *entry;

    if (em_die_ck(self) == 0) {
        if (self->field_0x834 == 1 || em_hit_timer_ck(self) == 1) {
            em_attack_start(self);
            em_motion_kind_set(self, 1, 0);
        }
        entry = (_ENEMY_DATA *)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (entry != NULL) {
            copyVec3(&self->aim, &entry->vec_0x24);
            if (self->field_0x011 == 0 && entry->work_0x18 != NULL && entry->work_0x18->area_no == self->area_no
                && em_act_ck_c1(entry->work_0x18, 1, 6) == 1
                && em_frame_check(entry->work_0x18, 0, 120.0f, 0.0f) == 1 && self->field_0x916 <= 0) {
                em_hit_by_set(self, ran_suu(0) & 0x1F, 0);
            }
            if (entry->field_0x08 == 0xFF) {
                self->field_0x46C = 0xFF;
                em_user_data_reenter(self);
                em_reaction_req(self, 6, 0xFF, 0);
            }
        }
    }
}

/* 0x8016D384 (0x48): leaves the guard into action 1 sub-state 3. */
void em011_guard_exit_3(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11_c1(self, 1, 3);
    em_finish_flag_set(self);
}

/* 0x8016D3CC (0x48): leaves the guard into action 1 sub-state 2. */
void em011_guard_exit_2(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11_c1(self, 1, 2);
    em_finish_flag_set(self);
}

/* 0x8016D414 (0x7C): plays motion 1 once. */
void em011_act0_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8016D490 (0x7C): plays motion 2 once. */
void em011_act0_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8016D50C (0x30): action 0's sub-states. */
void em011_act0_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em011_act0_0(self);
        return;
    case 1:
        em011_act0_1(self);
        return;
    case 2:
        em011_act0_0(self);
        return;
    }
}

/* 0x8016D53C (0x7C): plays motion 15 once. */
void em011_act1_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8016D5B8 (0x84): guards in motion 24, then leaves into sub-state 2. */
void em011_act1_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x18, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_2(self);
        }
        return;
    }
}

/* 0x8016D63C (0x84): guards in motion 25 from frame 94, then leaves into sub-state 3. */
void em011_act1_3(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 8, 0x5E);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_3(self);
        }
        return;
    }
}

/* 0x8016D6C0 (0x84): guards in motion 27, then leaves into sub-state 3. */
void em011_act1_4(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_3(self);
        }
        return;
    }
}

/* 0x8016D744 (0x84): guards in motion 28, then leaves into sub-state 3. */
void em011_act1_5(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_3(self);
        }
        return;
    }
}

/* 0x8016D7C8 (0x84): guards in motion 29, then leaves into sub-state 2. */
void em011_act1_6(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_2(self);
        }
        return;
    }
}

/* 0x8016D84C (0x8C): guards in motion 26, then drops the guard and finishes. */
void em011_act1_7(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1A, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_guard_clr(self);
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8016D8D8 (0x84): guards in motion 31, then leaves into sub-state 3. */
void em011_act1_8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1F, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em011_guard_exit_3(self);
        }
        return;
    }
}

/* 0x8016D95C (0x90): guards in motion 25 until frame 94, then leaves into sub-state 3. */
void em011_act1_9(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 4, 0);
        em_guard_set(self);
        return;
    case 1:
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            em011_guard_exit_3(self);
        }
        return;
    }
}

/* 0x8016D9EC (0x134): the hop: crouches in motion 3, jumps in motion 4 and ends the attack once it lands. */
void em011_act1_10(_ENEMY_WORK *self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_start_c1(self, 220.0f);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = 48.0f;
            self->offset_0x30C.vec_0x310.z = 50.0f;
            self->vec_0x31C.y = -4.0f;
            em_motion_param_set(self, 0.0f, 0x10);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x1D4 <= 0.0f || self->pos.y <= self->field_0x20C) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* 0x8016DB20 (0xC8): the stand-off: motion 10 for 30 frames, then ends the attack. */
void em011_act1_11(_ENEMY_WORK *self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 4, 0);
        em_motion_param_set(self, 0.0f, 0x1C);
        self->timer_0x020 = 30;
        break;
    case 1:
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= 0.0f) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* 0x8016DBE8 (0x54): action 1's sub-states 1..11. */
void em011_act1_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 1:
        em011_act1_1(self);
        return;
    case 2:
        em011_act1_2(self);
        return;
    case 3:
        em011_act1_3(self);
        return;
    case 4:
        em011_act1_4(self);
        return;
    case 5:
        em011_act1_5(self);
        return;
    case 6:
        em011_act1_6(self);
        return;
    case 7:
        em011_act1_7(self);
        return;
    case 8:
        em011_act1_8(self);
        return;
    case 9:
        em011_act1_9(self);
        return;
    case 10:
        em011_act1_10(self);
        return;
    case 11:
        em011_act1_11(self);
        return;
    }
}

/* 0x8016DC3C (0x204): the approach: walks in motion 10 at the `dist` set's speed, then either stops in motion 11
 * (`stop` 0) or finishes at once. */
void em011_act2_approach(_ENEMY_WORK *self, u8 dist, u8 stop) {
    if (dist == 5) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_mot_speed_set(self, 1.2f);
        switch (dist) {
        case 0:
            em_approach_start(self, -500.0f, 0);
            break;
        case 1:
            em_approach_start(self, -300.0f, 0);
            break;
        case 2:
            em_approach_start(self, -600.0f, 0);
            break;
        case 3:
            em_approach_start(self, -1000.0f, 0);
            break;
        case 4:
            em_approach_start(self, -1500.0f, 0);
            break;
        case 5:
            em_approach_start(self, 0.0f, 0);
            break;
        case 6:
            em_approach_start(self, 0.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x120) == 1) {
            switch (stop) {
            case 0:
                self->state += 1;
                em_mot_set(self, 0xB, 2, 0);
                em_mot_speed_set(self, 1.2f);
                break;
            case 1:
                em_action_finish(self);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016DE40 (0xB8): turns through the `em011_turn_seq_0` program. */
void em011_act2_turn(_ENEMY_WORK *self, u8 busy) {
    if (busy == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void *)em011_turn_seq_0, 0, 0, 0);
        em_mot_speed_set(self, 1.2f);
        break;
    case 1:
        if (em_turn_seq_step(self, (void *)em011_turn_seq_0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016DEF8 (0x190): drops a part: motion 17, the fall in motion 33 and the landing in motion 34. */
void em011_act2_6(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_joint_drop_start(self, self->joint_0x204);
            em_mot_set(self, 0x21, 0, 0);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = 40.0f;
            self->offset_0x30C.vec_0x310.z = -38.0f;
            self->vec_0x31C.y = -6.0f;
            self->vec_0x31C.z = 1.0f;
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.y < -100.0f) {
            self->offset_0x30C.vec_0x310.y = -100.0f;
            self->vec_0x31C.y = 0.0f;
        }
        if (self->offset_0x30C.vec_0x310.z > -34.0f) {
            self->offset_0x30C.vec_0x310.z = -34.0f;
            self->vec_0x31C.z = 0.0f;
        }
        if (em_ground_ck_c1(self, 100.0f) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x22, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E088 (0x240): the leap: crouches in motion 3, jumps in motion 4 along the `kind` arc and lands in
 * motion 35. */
void em011_act2_leap(_ENEMY_WORK *self, u8 kind) {
    if ((u32)(kind - 1) <= 1U) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_move_vec2_clr(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_start_c1(self, 140.0f);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            switch (kind) {
            case 0:
                self->offset_0x30C.vec_0x310.y = 60.0f;
                self->offset_0x30C.vec_0x310.z = 40.0f;
                self->vec_0x31C.y = -5.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            case 1:
                self->offset_0x30C.vec_0x310.y = 80.0f;
                self->offset_0x30C.vec_0x310.z = 55.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            case 2:
                self->offset_0x30C.vec_0x310.y = 60.0f;
                self->offset_0x30C.vec_0x310.z = 40.0f;
                self->vec_0x31C.y = -5.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            }
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.y < -150.0f) {
            self->offset_0x30C.vec_0x310.y = -150.0f;
            self->vec_0x31C.y = 0.0f;
        }
        if (self->offset_0x30C.vec_0x310.z < 20.0f) {
            self->offset_0x30C.vec_0x310.z = 20.0f;
            self->vec_0x31C.z = 0.0f;
        }
        if (em_ground_ck_c1(self, 80.0f) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x23, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E2C8 (0xD8): walks in motion 10 until frame 20, then stops in motion 11. */
void em011_act2_8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_mot_speed_set(self, 1.2f);
        break;
    case 1:
        if (em_frame_check(self, 1, 20.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0xB, 2, 0);
            em_mot_speed_set(self, 1.2f);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E3A0 (0xA4): action 2's sub-states. */
void em011_act2_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em011_act2_approach(self, 0, 0);
        return;
    case 1:
        em011_act2_approach(self, 1, 1);
        return;
    case 2:
        em011_act2_approach(self, 2, 1);
        return;
    case 3:
        em011_act2_approach(self, 3, 1);
        return;
    case 4:
        em011_act2_approach(self, 4, 1);
        return;
    case 5:
        em011_act2_turn(self, 0);
        return;
    case 6:
        em011_act2_6(self);
        return;
    case 7:
        em011_act2_leap(self, 0);
        return;
    case 8:
        em011_act2_8(self);
        return;
    case 9:
        em011_act2_leap(self, 1);
        return;
    case 10:
        em011_act2_approach(self, 5, 1);
        return;
    case 11:
        em011_act2_turn(self, 1);
        return;
    case 12:
        em011_act2_approach(self, 6, 1);
        return;
    }
}

/* 0x8016E444 (0x100): the bite: motion 22 with two hit windows, turning toward the target in its window. */
void em011_act7_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0);
        em_hit_window_set(self, 0, 5, 8);
        em_hit_window_set(self, 1, 6, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 46.0f) == 1) {
            em_turn_to_target(self, (u32)(u16)(s32)(0.5f + ((65536.0f * (0.9782609f * get_em_base_scale(self))) / 360.0f)));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E544 (0xCC): the lunge: motion 20, the hit window switched at frame 112. */
void em011_act7_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_hit_window_set_default_c1(self, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, 112.0f, 0.0f) == 1) {
            self->state += 1;
            em_hit_window_set_default_c1(self, 0, 2);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E610 (0xDC): the tail swing: motion 18; `chain` 1 chains into action 7 sub-state 4 at frame 68. */
void em011_act7_3(_ENEMY_WORK *self, u8 chain) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_hit_window_set_default_c1(self, 0, 3);
        break;
    case 1:
        switch (chain) {
        case 0:
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
            break;
        case 1:
            if (em_frame_check(self, 1, 68.0f, 0.0f) == 1) {
                self->state += 1;
                em_state_set(self, 7, 4);
            }
            break;
        }
        break;
    }
}

/* 0x8016E6EC (0xE8): the spin: motion 19, turning toward the target in its window. */
void em011_act7_4(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        em_hit_window_set_default_c1(self, 0, 4);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 22.0f) == 1) {
            em_turn_to_target(self, (u32)(u16)(s32)(0.5f + ((65536.0f * (1.3636364f * get_em_base_scale(self))) / 360.0f)));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016E7D4 (0x50): action 7's sub-states. */
void em011_act7_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em011_act7_0(self);
        return;
    case 2:
        em011_act7_2(self);
        return;
    case 3:
        em011_act7_3(self, 0);
        return;
    case 4:
        em011_act7_4(self);
        return;
    case 5:
        em011_act7_3(self, 1);
        return;
    }
}

/* The sound programs the two sound players run (`.data`; a program carries the first sub-state that plays it). */
EmSeRecord em011_se_rec_00 = {0x65, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_00[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_00},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em011_se_pos_0 = {0.0f, 38.0f, 0, -32768, 0};

EmSeRecord em011_se_rec_17_a = {0x66, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em011_vec_pair_0};

EmSeRecord em011_se_rec_17_b = {0x67, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_17_c = {0x71, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_17[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(20)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em011_se_pos_0},
    {0x00, {0, 0, 0}, &em011_se_rec_17_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em011_se_rec_17_b},
    {0x00, {0, 0, 0}, &em011_se_rec_17_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_7A = {0x6D, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_7B = {0x6E, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_9F = {0x71, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_7C = {0x6F, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_7C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_7D = {0x74, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_7D[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_7D},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_8E = {0xF, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_8E},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_A8 = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_D6_a = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_D6_b = {0x6D, 0, 20, 28, 0, {0, 12, 0, 0}, 0};

EmSeRecord em011_se_rec_D6_c = {0xF, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_D6[7] = {
    {0x00, {0, 0, 0}, &em011_se_rec_D6_a},
    {0x06, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(120)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(90)},
    {0x02, {0, 0, 0}, &em011_se_rec_D6_b},
    {0x00, {0, 0, 0}, &em011_se_rec_D6_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_12 = {0x65, 0, 16, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_12[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_12},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_9A = {0x6D, 0, 16, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_9A[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_9A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_9B = {0x6E, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_9B[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_9B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_A6 = {0x71, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_A6[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_A6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_9C = {0x6F, 0, 16, 0, 1, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_9C[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_9C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_9D = {0x74, 0, 16, 0, 1, {1, 0, 0, 0}, 0};

EmSeEntry em011_se_seq_9D[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_9D},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_92 = {0xF, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_92[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_92},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_AE = {0x64, 0, 16, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_AE[2] = {
    {0x00, {0, 0, 0}, &em011_se_rec_AE},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_DC_a = {0x64, 0, 16, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_DC_b = {0x6D, 0, 20, 28, 0, {0, 12, 0, 0}, 0};

EmSeRecord em011_se_rec_DC_c = {0xF, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_DC[7] = {
    {0x00, {0, 0, 0}, &em011_se_rec_DC_a},
    {0x06, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(120)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(90)},
    {0x02, {0, 0, 0}, &em011_se_rec_DC_b},
    {0x00, {0, 0, 0}, &em011_se_rec_DC_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_alt_00_a = {0x66, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em011_vec_pair_0};

EmSeRecord em011_se_rec_alt_00_b = {0x67, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_alt_00_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_alt_00_d = {0x73, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_alt_00[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(20)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em011_se_pos_0},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_00_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_00_b},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_00_c},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_00_d},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_alt_38_a = {0x3, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em011_se_rec_alt_38_b = {0x4, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em011_vec_pair_1};

EmSeRecord em011_se_rec_alt_38_c = {0x23, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em011_se_seq_alt_38[8] = {
    {0x0C, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_38_a},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(16)},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(220)},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_38_b},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_38_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em011_se_rec_alt_39 = {0xA, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em011_se_seq_alt_39[5] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(28)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(30)},
    {0x00, {0, 0, 0}, &em011_se_rec_alt_39},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x8016E824 (0x328): action 10 plays the sound program for its sub-state; an unknown one finishes. */
void em011_act10_se_play(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0x0:
        em_se_tbl_play(self, em011_se_seq_00, 0, 0);
        break;
    case 0x17:
        em_se_tbl_play(self, em011_se_seq_17, 1, 0x17);
        break;
    case 0x5:
        em_se_tbl_play(self, em011_se_seq_17, 1, 5);
        break;
    case 0x23:
        em_se_tbl_play(self, em011_se_seq_17, 1, 0x23);
        break;
    case 0x7A:
        em_se_tbl_play(self, em011_se_seq_7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em011_se_seq_7B, 0, 0x7B);
        break;
    case 0x9F:
        em_se_tbl_play(self, em011_se_seq_9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em011_se_seq_9F, 0, 0xA0);
        break;
    case 0x7C:
        em_se_tbl_play(self, em011_se_seq_7C, 0, 0x7C);
        break;
    case 0xA8:
        em_se_tbl_play(self, em011_se_seq_A8, 0, 0xA8);
        break;
    case 0xA9:
        em_se_tbl_play(self, em011_se_seq_17, 1, 0xA9);
        break;
    case 0x7D:
        em_se_tbl_play(self, em011_se_seq_7D, 0, 0x7D);
        break;
    case 0x8E:
        em_se_tbl_play(self, em011_se_seq_8E, 0, 0x8E);
        break;
    case 0x84:
        em_se_tbl_play(self, em011_se_seq_17, 1, 0x84);
        break;
    case 0xD6:
        em_se_tbl_play(self, em011_se_seq_D6, 0, 0xD6);
        break;
    case 0x12:
        em_se_tbl_play(self, em011_se_seq_12, 0, 0x12);
        break;
    case 0x37:
        em_se_tbl_play(self, em011_se_seq_12, 0, 0x37);
        break;
    case 0x9A:
        em_se_tbl_play(self, em011_se_seq_9A, 0, 0x9A);
        break;
    case 0x9B:
        em_se_tbl_play(self, em011_se_seq_9B, 0, 0x9B);
        break;
    case 0x9C:
        em_se_tbl_play(self, em011_se_seq_9C, 0, 0x9C);
        break;
    case 0x9D:
        em_se_tbl_play(self, em011_se_seq_9D, 0, 0x9D);
        break;
    case 0x92:
        em_se_tbl_play(self, em011_se_seq_92, 0, 0x92);
        break;
    case 0xA6:
        em_se_tbl_play(self, em011_se_seq_A6, 0, 0xA6);
        break;
    case 0xA7:
        em_se_tbl_play(self, em011_se_seq_A6, 0, 0xA7);
        break;
    case 0xAE:
        em_se_tbl_play(self, em011_se_seq_AE, 0, 0xAE);
        break;
    case 0xDC:
        em_se_tbl_play(self, em011_se_seq_DC, 0, 0xDC);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* 0x8016EB4C (0xA8): action 11 plays the sound program for its sub-state. */
void em011_act11_se_play(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0x0:
        em_se_tbl_play_alt(self, em011_se_seq_alt_00, 1, 0);
        break;
    case 0x5:
        em_se_tbl_play_alt(self, em011_se_seq_alt_00, 1, 5);
        break;
    case 0xE:
        em_se_tbl_play_alt(self, em011_se_seq_alt_00, 1, 0xE);
        break;
    case 0x38:
        em_se_tbl_play_alt(self, em011_se_seq_alt_38, 0, 0x38);
        break;
    case 0x39:
        em_se_tbl_play_alt(self, em011_se_seq_alt_39, 0, 0x39);
        break;
    default:
        em_se_tbl_play_alt(self, em011_se_seq_alt_00, 1, 0);
        break;
    }
}

/* 0x8016EBF4 (0x20C): the hop back: crouches in motion 3, jumps in motion 4 along the `kind` arc and lands in
 * motion 35. */
void em011_act12_hop(_ENEMY_WORK *self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_move_vec2_clr(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_start_c1(self, 140.0f);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            switch (kind) {
            case 0:
                self->offset_0x30C.vec_0x310.y = 40.0f;
                self->offset_0x30C.vec_0x310.z = 60.0f;
                self->vec_0x31C.y = -5.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            case 1:
                self->offset_0x30C.vec_0x310.y = 60.0f;
                self->offset_0x30C.vec_0x310.z = 85.0f;
                self->vec_0x31C.y = -5.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            case 2:
                self->offset_0x30C.vec_0x310.y = 40.0f;
                self->offset_0x30C.vec_0x310.z = 60.0f;
                self->vec_0x31C.y = -5.0f;
                self->vec_0x31C.z = -1.0f;
                break;
            }
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.y < -150.0f) {
            self->offset_0x30C.vec_0x310.y = -150.0f;
            self->vec_0x31C.y = 0.0f;
        }
        if (self->offset_0x30C.vec_0x310.z < 20.0f) {
            self->offset_0x30C.vec_0x310.z = 20.0f;
            self->vec_0x31C.z = 0.0f;
        }
        if (em_ground_ck_c1(self, 80.0f) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x23, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016EE00 (0xE8): the approach in motion 10, stopping in motion 11. */
void em011_act12_approach(_ENEMY_WORK *self, u8 flag) {
    if (flag == 1) {
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xB, 2, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016EEE8 (0x15C): walks to the target, turns through `em011_turn_seq_0` and moves on to action 12
 * sub-state 0 or 2. */
void em011_act12_3(_ENEMY_WORK *self, u8 next) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_mot_speed_set(self, 1.2f);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x120) == 1) {
            self->state += 1;
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 2:
                switch (self->area_no) {
                case 5:
                    em_move_target_set(self, 0xFF, 0xB, 400.0f);
                    break;
                }
                break;
            }
            em_turn_seq_start(self, (void *)em011_turn_seq_0, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step(self, (void *)em011_turn_seq_0) == 1) {
            switch (next) {
            case 0:
                em_state_set(self, 0xC, 0);
                break;
            case 1:
                em_state_set(self, 0xC, 2);
                break;
            }
        }
        break;
    }
}

/* 0x8016F044 (0x5C): action 12's sub-states. */
void em011_act12_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em011_act12_hop(self, 0);
        return;
    case 1:
        em011_act12_approach(self, 0);
        return;
    case 2:
        em011_act12_hop(self, 1);
        return;
    case 3:
        em011_act12_3(self, 1);
        return;
    case 4:
        em011_act12_approach(self, 1);
        return;
    }
}

/* 0x8016F0A0 (0xE0): on the demo frames whose low five bits are `phase`, puts the sleep bubbles at the enemy
 * data's sleep offset (default: joint 2, 100 up). */
void em011_demo_sleep_fx(_ENEMY_WORK *self, u8 phase) {
    VEC3 offset;
    VEC3 a;
    VEC3 b;
    EmAilmentFx *fx;

    VEC3_ctor(&offset);
    if (phase == (em_demo_frame_get() & 0x1F)) {
        fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
        if (fx != NULL) {
            copyVec3(&offset, setVec3(&a, fx->x, fx->y, fx->z));
            eft013_em_place(self, &offset, fx->joint, fx->scale, 0xA);
            return;
        }
        copyVec3(&offset, setVec3(&b, 0.0f, 100.0f, 0.0f));
        eft013_em_place(self, &offset, 2, 2.0f, 0xA);
    }
}

/* 0x8016F180 (0x27C): demo 0: asleep at its mark from frame 200, wakes at 500 and rises. */
void em011_act13_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, NULL);
        em_latch_0x762_set(self, -1);
        break;
    case 1:
        if (em_demo_time_ck(200) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = -0x4444;
            em_demo_pos_set(self, 152.5f, 20.0f, -18.7f);
        }
        break;
    case 2:
        em011_demo_sleep_fx(self, 0x10);
        if (em_demo_time_ck(500) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
        }
        break;
    case 3:
        if (em_frame_check(self, 1, 220.0f, 0.0f) == 0) {
            em011_demo_sleep_fx(self, 0x10);
        }
        if (em_frame_check(self, 0, 256.0f, 0.0f) == 1) {
            em_latch_0x762_roll(self);
        }
        if (em_frame_check(self, 1, 258.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x19, 0xC, 0);
        }
        break;
    case 4:
        if (em_demo_time_ck(876) == 1) {
            self->state += 1;
            em_mot_set(self, 0x19, 0, 0x5E);
        }
        break;
    case 5:
        if (em_frame_check(self, 1, 114.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x1A, 8, 0);
        }
        break;
    case 6:
        if (em_frame_check(self, 1, 102.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 2, 0x10, 6);
        }
        break;
    }
}

/* 0x8016F3FC (0xB4): demo 1: awake at demo 0's mark, then done. */
void em011_act13_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, 152.5f, 0.0f, -18.7f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = -0x4444;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        em_latch_0x762_roll(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016F4B0 (0x180): demo 2: asleep at its mark from frame 170, wakes at 1212 and rises at 1324. */
void em011_act13_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, NULL);
        em_latch_0x762_set(self, -1);
        break;
    case 1:
        if (em_demo_time_ck(170) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = 0x6555;
            em_demo_pos_set(self, -534.5f, 2.0f, 361.2f);
        }
        break;
    case 2:
        if (em_demo_time_ck(876) == 0) {
            em011_demo_sleep_fx(self, 8);
        }
        if (em_demo_time_ck(1212) == 1) {
            self->state += 1;
            em_mot_set(self, 0x1A, 0, 0);
            em_latch_0x762_roll(self);
        }
        break;
    case 3:
        if (em_demo_time_ck(1324) == 1) {
            self->state += 1;
            em_mot_set(self, 2, 0, 0);
        }
        break;
    }
}

/* 0x8016F630 (0xB4): demo 3: awake at demo 2's mark, then done. */
void em011_act13_3(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, -534.5f, 2.0f, 361.2f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x6555;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        em_latch_0x762_roll(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016F6E4 (0x214): demo 4: asleep at its mark from frame 160, wakes at 1528 and rises. */
void em011_act13_4(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = -0x4444;
        self->field_0x1C4 = 0;
        em_demo_reset(self, NULL);
        em_latch_0x762_set(self, -1);
        break;
    case 1:
        if (em_demo_time_ck(160) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = 0x89;
            em_demo_pos_set(self, -243.4f, 20.0f, -551.5f);
        }
        break;
    case 2:
        if (em_demo_time_ck(876) == 0) {
            em011_demo_sleep_fx(self, 0x1E);
        }
        if (em_demo_time_ck(1528) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
            em_latch_0x762_roll(self);
        }
        break;
    case 3:
        if (em_frame_check(self, 1, 118.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x19, 0xC, 0);
        }
        break;
    case 4:
        if (em_frame_check(self, 1, 76.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x1A, 8, 0);
        }
        break;
    case 5:
        if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 2, 0x10, 0);
        }
        break;
    }
}

/* 0x8016F8F8 (0xB4): demo 5: awake at demo 4's mark, then done. */
void em011_act13_5(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x89;
        self->field_0x1C4 = 0;
        em_demo_pos_set(self, -243.4f, 20.0f, -551.5f);
        em_demo_enable(self);
        em_latch_0x762_roll(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8016F9AC (0x54): action 13's demo steps. */
void em011_act13_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em011_act13_0(self);
        return;
    case 1:
        em011_act13_1(self);
        return;
    case 2:
        em011_act13_2(self);
        return;
    case 3:
        em011_act13_3(self);
        return;
    case 4:
        em011_act13_4(self);
        return;
    case 5:
        em011_act13_5(self);
        return;
    }
}

/* 0x8016FA00 (0x48): runs the current action's dispatcher. */
void em011_action_dispatch(_ENEMY_WORK *self) {
    switch (self->action) {
    case 0:
        em011_act0_dispatch(self);
        return;
    case 1:
        em011_act1_dispatch(self);
        return;
    case 2:
        em011_act2_dispatch(self);
        return;
    case 7:
        em011_act7_dispatch(self);
        return;
    case 10:
        em011_act10_se_play(self);
        return;
    case 11:
        em011_act11_se_play(self);
        return;
    case 12:
        em011_act12_dispatch(self);
        return;
    case 13:
        em011_act13_dispatch(self);
        return;
    }
}

/* 0x8016FA48 (0xD0): the effect router: `where` 0 spawns at a joint, 1 on the ground under it. */
void em011_effect_spawn(_ENEMY_WORK *self, u8 where, u8 id, s32 joint, s32 delta, f32 scale) {
    VEC3 pos;

    VEC3_ctor(&pos);
    switch (where) {
    case 0x0:
        eft009_spawn_at_joint(self, (u32)joint, (u32)id, delta, scale);
        return;
    case 0x1:
        get_joint_wpos_em(self, (u32)joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, id, delta, scale);
        return;
    }
}

/* 0x8016FB18 (0xD0): the guard state the state query reports: 2 in the guard motions 26..29 and late in 25/31,
 * 1 in 24 and early in 25. */
void em011_guard_state_update(_ENEMY_WORK *self) {
    u16 mot;

    self->field_0x1E4 = 0;
    mot = em_get_mot_no(self);
    switch (mot) {
    case 24:
        self->field_0x1E4 = 1;
        break;
    case 25:
        if (em_after_frame_check(self, 1, 92.0f, 0.0f) == 1) {
            self->field_0x1E4 = 2;
        } else {
            self->field_0x1E4 = 1;
        }
        break;
    case 26:
    case 27:
    case 28:
    case 29:
        self->field_0x1E4 = 2;
        break;
    case 31:
        if (em_after_frame_check(self, 1, 58.0f, 0.0f) == 1) {
            self->field_0x1E4 = 2;
        }
        break;
    }
}

/* 0x8016FBE8 (0x5DC): the per-motion events: the footfall and splash effects, larger while the work is wet. */
void em011_motion_events(_ENEMY_WORK *self) {
    VEC3 pos;

    VEC3_ctor(&pos);
    em011_guard_state_update(self);
    switch (em_get_mot_no(self)) {
    case 0xB:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 0x1B, 0, 0.8f);
        }
        break;
    case 0x12:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x33, 0x1F, 0, 2.2f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x16, 0x1B, 0, 2.2f);
            }
        } else {
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 9, 0x1F, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 9, 0x1B, 0, 0.8f);
            }
        }
        break;
    case 0x13:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x33, 0x1B, 0, 2.2f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x33, 0x1F, 0, 2.2f);
            }
        } else {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 9, 0x1B, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 9, 0x1F, 0, 0.8f);
            }
        }
        break;
    case 0x16:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x33, 0x1F, 0, 3.3f);
            }
        } else if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 0x1F, 0, 1.0f);
        }
        break;
    case 0x22:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, -40.0f);
                eft009_spawn_at_pos(self, 0x17, 0x33, 0, 3.0f, &pos);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 0x1F, 0, 1.4f);
        }
        break;
    case 0x23:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x17, 0x17, 0, 3.5f);
                em011_effect_spawn(self, 0, 0x16, 0x17, 0, 3.0f);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 0x1F, 0, 1.2f);
        }
        break;
    case 0x67:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x36, 3, 0, 1.5f);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 3, 0, 1.8f);
        }
        break;
    case 0x6D:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
                em011_effect_spawn(self, 0, 0x36, 3, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em011_effect_spawn(self, 0, 9, 3, 0, 1.8f);
        }
        break;
    }
}

/* 0x801701C4 (0x2E8): the area start: action 12, moving to the map's set point for the area and its sub-state;
 * an area with none puts the work at the origin and asks for the reset reaction. */
void em011_area_start_set(_ENEMY_WORK *self, u8 *action, u8 *sub) {
    u32 none;

    none = 0;
    em_move_mode_set(self, 0);
    *action = 0xC;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 2:
            em_move_target_set(self, 0x13, 0x14, 320.0f);
            *sub = 0;
            break;
        case 3:
            em_move_target_set(self, 0xA, 0xC, 440.0f);
            *sub = 1;
            break;
        case 4:
            em_move_target_set(self, 0xA, 0xC, 140.0f);
            *sub = 1;
            break;
        case 5:
            em_move_target_set(self, 0xA, 0xC, 400.0f);
            *sub = 1;
            break;
        case 9:
            em_move_target_set(self, 9, 0xB, 360.0f);
            *sub = 1;
            break;
        case 10:
            em_move_target_set(self, 4, 5, 300.0f);
            *sub = 0;
            break;
        default:
            none = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 2:
            em_move_target_set(self, 0xA, 0xC, 200.0f);
            *sub = 1;
            break;
        case 3:
            em_move_target_set(self, 9, 0xB, 160.0f);
            *sub = 1;
            break;
        case 4:
            if (stg_area_flag_index_ck(2) == 1) {
                em_move_target_set(self, 0x13, 0x14, 160.0f);
            } else {
                em_move_target_set(self, 9, 0xA, 100.0f);
            }
            *sub = 4;
            break;
        case 5:
            em_move_target_set(self, 9, 0xA, 240.0f);
            *sub = 3;
            break;
        case 7:
            em_move_target_set(self, 0xB, 0xD, 160.0f);
            *sub = 1;
            break;
        case 10:
            em_move_target_set(self, 1, 3, 200.0f);
            *sub = 1;
            break;
        default:
            none = 1;
            break;
        }
        break;
    default:
        none = 1;
        break;
    }
    if (none == 1) {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        em_reaction_req(self, 5, 2, 0xA);
        *sub = 0;
    }
}

/* 0x801704AC (0x4): the tint slot; em011 has none. */
void em011_tev_color_update(void) {}

/* 0x801704B0 (0x1C): the state query: kind 0 is the guard state. */
u8 em011_state_ck(_ENEMY_WORK *self, u8 kind) {
    if (kind == 0) {
        return self->field_0x1E4;
    }
    return 0;
}

/* 0x801704CC (0x10): reports whether the work is idle (+0x1E2 clear). */
s32 em011_idle_ck(_ENEMY_WORK *self) {
    return self->field_0x1E2 == 0;
}

/* 0x801704DC (0x70): on maps 1 and 2, a work with its attack timer armed asks for the escape action (7). */
s32 em011_escape_ck(_ENEMY_WORK *self) {
    if ((u32)(stage_map_kind_get(self->field_0x1E0) - 1) <= 1U && self->field_0x011 != 0) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 7;
        self->field_0x1FF = 0;
        return 1;
    }
    return 0;
}

}

/* The unit's `.bss`: the two vector pairs MWCC's `__sinit` (0x8017054C) seeds; two sound records point at them. */
Em011VecPair em011_vec_pair_0(34.0f, -20.0f, -4.0f); /* 0x806A7970 */
Em011VecPair em011_vec_pair_1(48.0f, 50.0f, -4.0f); /* 0x806A7988 */
