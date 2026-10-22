/*
 * enemy/em012_prog.cpp - enemy 012's program: the `em012_prog_tbl` entry points (reset, init, frame update,
 *   action dispatch, event hook, motion events, tev colour, state query, area start, gauge, part gates, action
 *   selection), the per-action state steps the `state_sub` dispatchers select, the two sound programs, the demo
 *   (cut-scene) steps, the effect router and the static initializer.
 * RANGE. .text 0x80170600-0x80176C30 (67 functions); extab 0x8000E52C-0x8000E6D4, extabindex
 *   0x800296AC-0x80029928, .ctors 0x8056F334-0x8056F338 (`em012_static_init`), .rodata 0x8056FD10-0x8056FDD0, .data
 *   0x805A8370-0x805A94A0 (`em012_prog_tbl` first), .bss 0x806A79A0-0x806A79D0, .sdata 0x80791A58-0x80791A60,
 *   .sdata2 0x80797910-0x80797B10.
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide except `em012_act1_12`..`em012_act1_dispatch` (on),
 *   `#pragma pool_data off` (each table is addressed with its own `lis`/`addi`) and `#pragma fp_contract off`.
 * NAMES. The file name follows the runtime dump's `em012_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em012ProgTbl` slot each fills, after `Em006ProgTbl`): em012_reset, em012_init, em012_frame_update,
 *   em012_action_dispatch, em012_event_notify, em012_motion_events, em012_tev_color_update, em012_state_ck,
 *   em012_area_start_set, em012_gauge_update, em012_part_level_ck, em012_part_drop, em012_action_select.
 *   GUESS (dispatcher slot): em012_act<action>_<first sub-state> and em012_act<action>_dispatch,
 *   em012_act10_se_play, em012_act11_se_play; GUESS (from the bodies): em012_spawn_call, em012_alt_mode_effect,
 *   em012_effect_spawn, em012_area_move_armed_ck, em012_spawn_rec_set, em012_static_init.
 *   GUESS (data): em012_turn_seq_0..2, em012_pick_tbl, em012_demo_keys, em012_se_seq_<sub-state>,
 *   em012_se_seq_alt_<sub-state>, em012_se_rec_<seq>[_a..], em012_se_pos_0, em012_vec_pair_0/1, the
 *   `Em012ProgTbl` slot names; `EmAreaEntry::kind_0x08`/`entry_0x09`.
 * RESIDUALS. 4 rows short of the target, every data section byte-identical:
 *   `em012_state_ck` - the pack count's `count <= 1U` gives `subfic`/`orc` with swapped operands where retail
 *   has `li r3,2; subi; orc` (`count < 2` forms are worse), and self/count take r31/r30 swapped.
 *   `em012_tev_color_update` - the rim/body constants are staged in r0/r3 where retail loads them straight into
 *   r6/r9.  `em012_act7_1` - `fmuls f0,f31,f0` operand order of the scale product.  `em012_action_select` -
 *   the first area selector lands in r0 where retail uses r4.
 *   flipcheck: the STOPGAP callee `em_mot_window_base_get` (request `enemy-em012#1`, the registry's proposal for
 *   0x8012EC74) is not a map name yet, so a flip links `undefined` until that rename lands.
 * SHAPES. A one-case test retail compares signed (`cmpwi`) is a `switch` (`em012_motion_events`,
 *   `em012_act2_27`).  A case-level `return` at the end of a state switch costs a `b`: the steps `break`.
 *   `em_mot_finished_ck`, `em_ground_ck`, `em_fall_start`, `assignVec3` and `em_act_arm_unless_down` are called
 *   through `_c1` views whose widths give retail's compares and float arguments.  The +0x3C shell-table slot is
 *   called without its table argument (`ShellRecJobFn`).  The data is defined between the functions in retail
 *   order (the jump tables fall between the definitions).
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_8011D448.h"
#include "enemy/em_area_stun_move_ck.h"
#include "enemy/em_area_player_follow_ck.h"
#include "enemy/em_mot_layer_weight_set.h"
#include "enemy/em_in_water_ck.h"
#include "fn_8004CAD8.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "ef/eft_slot.h"
#include "ef/get_move_work_adrs.h"
#include "ef.h"
#include "stage/shell_set_func_ptr.h"
#include "stage/stg_w.h"
#include "mh3_pad/system_w.h"
#include "sound/fn_800DD1F0.h"
#include "enemy/EnemyData.h"
#include "enemy/em_area_entry_tbl.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_roll_cooldown_ck.h"
#include "enemy/em_model.h"
#include "enemy/em_mot_finished_ck.h"
#include "enemy/em_fall_start.h"
#include "enemy/em_frame_flag_set.h"
#include "enemy/em_turn_in_window.h"
#include "enemy/em_mot_set_blend.h"
#include "enemy/em_move_vec2_clr.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)

/* The +0x3C shell-table slot spelled without its trailing table argument: the call site passes none. */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_mot_finished_ck_c1 ((u32 (*)(struct _ENEMY_WORK*))em_mot_finished_ck)
#define em_act_arm_unless_down_c1 ((void (*)(_ENEMY_WORK *, u8, u8))em_act_arm_unless_down)

/* The record `enemy_data_find` looks up; only the two flag bytes read here are named.
 * size: 0x0E (approximate - only +0x08 and +0x0D are observed) */
typedef struct ENEMY_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 unused_0x09[0x0D - 0x09];
    /* +0x0D */ u8 field_0x0D;
} ENEMY_ENTRY;

/* STOPGAP-BEGIN(enemy-em012#1) */
extern "C" f32 em_mot_window_base_get(struct _ENEMY_WORK* self);
/* STOPGAP-END(enemy-em012#1) */

/* The em012 program table (`em012_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em006ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em012ProgTbl {
    /* +0x00 */ void (*reset_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ u32 pad_0x10;
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u32 (*state_ck_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*gauge_update_0x34)(struct _ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[3];
    /* +0x44 */ s32 (*part_level_ck_0x44)(struct _ENEMY_WORK* self, u32 part);
    /* +0x48 */ u32 pad_0x48[2];
    /* +0x50 */ void (*part_drop_0x50)(struct _ENEMY_WORK* self, u32 part);
    /* +0x54 */ s32 (*action_select_0x54)(struct _ENEMY_WORK* self, u16 arg);
    /* +0x58 */ u32 pad_0x58[5];
};

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * four-word rows whose first word carries the motion in its high half. */
extern "C" const u32 em012_turn_seq_0[16] = {
    0x00000000, 0x00000014, 0x00000000, 0x40002000,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x00080000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em012_turn_seq_1[16] = {
    0x00000000, 0x0000000C, 0x00000000, 0x471C0000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00060000, 0x00000006, 0x00000000, 0x00000000,
    0x00050000, 0x00000006, 0x00000000, 0x00000000,
};

extern "C" const u32 em012_turn_seq_2[16] = {
    0x00000000, 0x00000022, 0x00000000, 0x40000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00160000, 0x00000004, 0x00000000, 0x00000000,
    0x00160000, 0x00000004, 0x00000000, 0x00000000,
};

/* The pick record `em012_action_select` hands `em_pack_area_pick` (`.sdata`). */
u8 em012_pick_tbl[8] = {0x00, 0x00, 0x00, 0x1E, 0xFF, 0xFF, 0x00, 0x00};

/* The two vector pairs `em012_static_init` seeds (`.bss`, defined at the end); two sound records point at them. */
extern VEC3 em012_vec_pair_0[2];
extern VEC3 em012_vec_pair_1[2];

extern "C" {

void em012_reset(_ENEMY_WORK *self);
void em012_init(_ENEMY_WORK *self, u8 a);
void em012_spawn_call(_ENEMY_WORK *self);
void em012_event_notify(_ENEMY_WORK *self, u8 a, u8 b);
void em012_frame_update(_ENEMY_WORK *self);
void em012_act0_0(_ENEMY_WORK *self);
void em012_act0_1(_ENEMY_WORK *self);
void em012_act0_2(_ENEMY_WORK *self);
void em012_act0_dispatch(_ENEMY_WORK *self);
void em012_act1_1(_ENEMY_WORK *self);
void em012_act1_2(_ENEMY_WORK *self);
void em012_act1_3(_ENEMY_WORK *self);
void em012_act1_4(_ENEMY_WORK *self);
void em012_act1_5(_ENEMY_WORK *self);
void em012_act1_6(_ENEMY_WORK *self);
void em012_act1_7(_ENEMY_WORK *self);
void em012_act1_8(_ENEMY_WORK *self);
void em012_act1_10(_ENEMY_WORK *self, u32 a);
void em012_act1_12(_ENEMY_WORK* self);
void em012_act1_13(_ENEMY_WORK* self);
void em012_act1_14(_ENEMY_WORK* self);
void em012_act1_dispatch(_ENEMY_WORK* self);
void em012_act2_0(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void em012_act2_1(_ENEMY_WORK *self, u8 arg1);
void em012_act2_2(_ENEMY_WORK *self, u8 arg1);
void em012_act2_5(_ENEMY_WORK *self, u8 arg1);
void em012_act2_8(_ENEMY_WORK *self, u8 arg1);
void em012_act2_20(_ENEMY_WORK *self);
void em012_act2_21(_ENEMY_WORK *self);
void em012_act2_23(_ENEMY_WORK *self);
void em012_act2_27(_ENEMY_WORK *self, u8 arg1);
void em012_act2_dispatch(_ENEMY_WORK *self);
void em012_act7_0(_ENEMY_WORK *self, u8 arg1);
void em012_act7_1(_ENEMY_WORK *self);
void em012_act7_2(_ENEMY_WORK *self, u8 arg1);
void em012_act7_3(_ENEMY_WORK *self, u8 arg1);
void em012_act7_4(_ENEMY_WORK *self, u8 arg1);
void em012_act7_6(_ENEMY_WORK *self);
void em012_act7_9(_ENEMY_WORK *self, u8 arg1);
void em012_act7_dispatch(_ENEMY_WORK *self);
void em012_act10_se_play(_ENEMY_WORK *self);
void em012_act11_se_play(_ENEMY_WORK *self);
void em012_act12_0(_ENEMY_WORK* self);
void em012_act12_1(_ENEMY_WORK* self);
void em012_act12_2(_ENEMY_WORK* self);
void em012_act12_dispatch(_ENEMY_WORK* self);
void em012_act13_0(_ENEMY_WORK *self);
void em012_act13_1(_ENEMY_WORK *self);
void em012_act13_2(_ENEMY_WORK *self);
void em012_act13_3(_ENEMY_WORK *self);
void em012_act13_4(_ENEMY_WORK *self);
void em012_act13_5(_ENEMY_WORK *self);
void em012_act13_dispatch(_ENEMY_WORK* self);
void em012_action_dispatch(_ENEMY_WORK* self);
void em012_alt_mode_effect(_ENEMY_WORK *self);
void em012_effect_spawn(_ENEMY_WORK *self, u8 arg1, u8 arg2, s32 arg3, s32 arg4, f32 farg0);
void em012_motion_events(_ENEMY_WORK *self);
void em012_tev_color_update(_ENEMY_WORK *self);
u32 em012_state_ck(_ENEMY_WORK *self, u8 arg1);
void em012_area_start_set(_ENEMY_WORK *self, u8 *arg1, u8 *arg2);
void em012_gauge_update(_ENEMY_WORK *self);
s32 em012_part_level_ck(_ENEMY_WORK* self, u32 arg);
void em012_part_drop(_ENEMY_WORK* self, u32 arg);
s32 em012_action_select(_ENEMY_WORK *self, u16 arg);
s32 em012_area_move_armed_ck(_ENEMY_WORK *self);
void em012_spawn_rec_set(EmSpawnRec *rec, u8 arg1, u16 arg2, u16 arg3);

void em012_static_init(void);

Em012ProgTbl em012_prog_tbl = {
    em012_reset,
    em012_init,
    em012_frame_update,
    em012_action_dispatch,
    0,
    em012_event_notify,
    em012_motion_events,
    0,
    em012_tev_color_update,
    em012_state_ck,
    {0, 0},
    em012_area_start_set,
    em012_gauge_update,
    {0, 0, 0},
    em012_part_level_ck,
    {0, 0},
    em012_part_drop,
    em012_action_select,
    {0, 0, 0, 0, 0},
};

void em012_reset(_ENEMY_WORK *self) {
    self->field_0x32A = 0;
    self->field_0x328 = 0;
}

void em012_init(_ENEMY_WORK *self, u8 a) {
    Vec3 v;
    u8 b, c;

    VEC3_ctor(&v);
    if ((a & 0xFF) == 2) {
        em012_area_start_set(self, &b, &c);
        em_act_arm_unless_down_c1(self, b, c);
        em_state_refresh(self);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, 0.0f, 45.0f);
        eft_em_attach_set(self, 0x14, &v, 0.7f, 0x1C72);
    }
}

void em012_spawn_call(_ENEMY_WORK *self) {
    if (self->team == 0xC) {
        em_spawn_request(self->field_0x01A, 0xA, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    } else {
        em_spawn_request(self->field_0x01A, 0xD, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    }
}

void em012_event_notify(_ENEMY_WORK *self, u8 a, u8 b) {
    if ((a & 0xFF) == 1) {
        switch (b) {
        case 5:
            em_bait_react(self);
            break;
        case 0xA:
            self->field_0x32A = 0;
            break;
        case 0xC:
            if (em_busy_ck(self) == 1) {
                em012_spawn_call(self);
            }
            break;
        case 0xD:
            if (em_busy_ck(self) == 1) {
                em012_spawn_call(self);
            }
            break;
        case 0xE:
            em_act_flag1_set(self);
            break;
        }
    }
}

void em012_frame_update(_ENEMY_WORK *self) {
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (em_die_ck(self) == 0) {
        u8 v = (u8)enemy_data_grp(self->team, self->field_0x00A);
        ENEMY_ENTRY *entry = (ENEMY_ENTRY *)enemy_data_find(v, self->field_0x46C);
        if (entry != 0 && entry->field_0x08 == 0xFF) {
            self->field_0x46C = 0xFF;
            em_user_data_reenter(self);
        }
    }
}

void em012_act0_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act0_1(_ENEMY_WORK *self) {
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

void em012_act0_2(_ENEMY_WORK *self) {
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

void em012_act0_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em012_act0_0(self);
        break;
    case 1:
        em012_act0_1(self);
        break;
    case 2:
        em012_act0_2(self);
        break;
    case 3:
        em012_act0_0(self);
        break;
    case 4:
        em012_act0_0(self);
        break;
    case 6:
        em012_act0_0(self);
        break;
    }
}

void em012_act1_1(_ENEMY_WORK *self) {
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

void em012_act1_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_3(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2: {
        f32 t = 0.4f;
        em_gauge_tick(self, t, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x71, 4, 0);
            em_tint_hold_end(self);
        }
    }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_4(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        self->timer_0x020 = 0x84;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        } else {
            self->state++;
            em_state_set(self, 1, 5);
        }
        break;
    }
}

void em012_act1_5(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set(self, 0xCA, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_6(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_7(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 138.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x71, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_10(_ENEMY_WORK *self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if ((u8)a == 1) {
            if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
                em_state_set(self, 1, 0xC);
            }
        } else {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
        }
        break;
    }
}

#pragma peephole on
void em012_act1_12(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 170.0f, 0.0f) == 1U) {
            em_state_set(self, 1, 0xD);
        }
        break;
    }
}

void em012_act1_13(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_14(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act1_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1: em012_act1_1(self); break;
    case 2: em012_act1_2(self); break;
    case 3: em012_act1_3(self); break;
    case 4: em012_act1_4(self); break;
    case 5: em012_act1_5(self); break;
    case 6: em012_act1_6(self); break;
    case 7: em012_act1_7(self); break;
    case 8: em012_act1_8(self); break;
    case 10: em012_act1_10(self, 1); break;
    case 11: em012_act1_10(self, 0); break;
    case 12: em012_act1_12(self); break;
    case 13: em012_act1_13(self); break;
    case 14: em012_act1_14(self); break;
    case 0:
    case 9: break;
    }
}

#pragma peephole off
void em012_act2_0(_ENEMY_WORK *self, u8 arg1, u8 arg2) {

    switch (arg1) {
    case 0xA:
        em_frame_flag_set(self);
        /* fallthrough */
    case 0x7:
        em_busy_set(self);
        em_busy_timer_reset(self);
        break;
    }
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        if (self->team == 0xC) {
            switch (arg1) {
            case 0x0:
                em_approach_start(self, -500.0f, 0);
                break;
            case 0x1:
                em_approach_start(self, -600.0f, 0);
                break;
            case 0x2:
                em_approach_start(self, -600.0f, 0);
                break;
            case 0x3:
                em_approach_start(self, -520.0f, 0);
                break;
            case 0x4:
                em_approach_start(self, -550.0f, 0);
                break;
            case 0x5:
                em_approach_start(self, -1000.0f, 0);
                break;
            case 0x6:
                em_approach_start(self, -1150.0f, 0);
                break;
            case 0x7:
                em_approach_start(self, 0.0f, 0);
                break;
            case 0x8:
                em_approach_start(self, 0.0f, 0);
                break;
            case 0x9:
                em_approach_start(self, -700.0f, 0);
                break;
            case 0xA:
                em_approach_start(self, 0.0f, 0);
                break;
            }
        } else {
            switch (arg1) {
            case 0x0:
                em_approach_start(self, -500.0f, 0);
                break;
            case 0x1:
                em_approach_start(self, -800.0f, 0);
                break;
            case 0x2:
                em_approach_start(self, -600.0f, 0);
                break;
            case 0x3:
                em_approach_start(self, -520.0f, 0);
                break;
            case 0x4:
                em_approach_start(self, -700.0f, 0);
                break;
            case 0x5:
                em_approach_start(self, -900.0f, 0);
                break;
            case 0x6:
                em_approach_start(self, -1300.0f, 0);
                break;
            case 0x7:
                em_approach_start(self, 0.0f, 0);
                break;
            case 0x8:
                em_approach_start(self, 0.0f, 0);
                break;
            case 0x9:
                em_approach_start(self, -900.0f, 0);
                break;
            case 0xA:
                em_approach_start(self, 0.0f, 0);
                break;
            }
        }
        break;
    case 0x1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            switch (arg2) {
            case 0x0:
                self->state++;
                em_mot_set(self, 0xB, 4, 0);
                return;
            case 0x1:
                em_action_finish(self);
                return;
            }
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_1(_ENEMY_WORK *self, u8 arg1) {

    if (arg1 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em012_turn_seq_0, 0, 1, 0);
        if (self->field_0x482 == 1) {
            em_mot_speed_set(self, 1.2f);
            break;
        }
        break;
    case 0x1:
        if (em_turn_seq_step(self, (void*)em012_turn_seq_0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_2(_ENEMY_WORK *self, u8 arg1) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 4, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_joint_drop_start(self, self->joint_0x204);
            em_mot_set(self, 0x21, 0, 0);
            em_move_vec2_clr(self);
            if (arg1 == 0) {
                self->field_0x314 = 40.0f;
                self->field_0x318 = -45.0f;
                self->field_0x320 = -5.7f;
                self->field_0x324 = 1.0f;
            }
            em_move_offset_step(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (arg1 == 0) {
            if (self->field_0x314 < -100.0f) {
                self->field_0x314 = -100.0f;
                self->field_0x320 = 0.0f;
            }
            if (self->field_0x318 > -20.0f) {
                self->field_0x318 = -20.0f;
                self->field_0x324 = 0.0f;
            }
        }
        if (em_ground_ck_c1(self, 200.0f) != 0) {
            self->state++;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x22, 0, 0);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_5(_ENEMY_WORK *self, u8 arg1) {

    switch (arg1) {
    case 6:
        em_frame_flag_set(self);
        em_fx_flag_set(self);
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        em_busy_set(self);
        em_busy_timer_reset(self);
        break;
    }
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_move_vec2_clr(self);
        if ((arg1 == 1) && (self->team == 0xC)) {
            em_drop_item_spawn(self, &self->pos, 0x21);
            break;
        }
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_c1(self, 240.0f);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            switch (arg1) {
            case 0x0:
            case 0x4:
            case 0x6:
                self->field_0x314 = 60.0f;
                self->field_0x318 = 40.0f;
                self->field_0x320 = -5.0f;
                self->field_0x324 = -1.0f;
                break;
            case 0x1:
                self->field_0x314 = 95.0f;
                self->field_0x318 = 50.0f;
                self->field_0x320 = -5.0f;
                self->field_0x324 = -1.0f;
                break;
            case 0x2:
                self->field_0x314 = 60.0f;
                self->field_0x318 = 60.0f;
                self->field_0x320 = -5.0f;
                self->field_0x324 = -1.0f;
                break;
            case 0x3:
                self->field_0x314 = 40.0f;
                self->field_0x318 = 60.0f;
                self->field_0x320 = -5.0f;
                self->field_0x324 = -1.0f;
                break;
            case 0x5:
                self->field_0x314 = 75.0f;
                self->field_0x318 = 60.0f;
                self->field_0x320 = -5.0f;
                self->field_0x324 = -1.0f;
                break;
            }
            em_move_offset_step(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x314 < -100.0f) {
            self->field_0x314 = -100.0f;
            self->field_0x320 = 0.0f;
        }
        if (self->field_0x318 < 20.0f) {
            self->field_0x318 = 20.0f;
            self->field_0x324 = 0.0f;
        }
        if (em_ground_ck_c1(self, 160.0f) != 0) {
            self->state++;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x23, 0, 0);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_8(_ENEMY_WORK *self, u8 arg1) {

    switch (arg1) {
    case 0x2:
        em_fx_flag_set(self);
        /* fallthrough */
    case 0x1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
        break;
    }
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x10, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        if (arg1 == 2) {
            em_drop_item_spawn(self, &self->pos, 0x21);
            break;
        }
        break;
    case 0x1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_20(_ENEMY_WORK *self) {
    f32 *temp_r31;
    f32 temp_f0;
    f32 var_f1;
    s32 temp_r3_2;

    temp_r31 = em_sight2_get(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 6, 0);
        var_f1 = -0.5f * vec3_distance(&self->pos, &self->vec_0x36C);
        temp_f0 = -*temp_r31;
        if (var_f1 > temp_f0) {
            var_f1 = temp_f0;
        }
        em_approach_start(self, var_f1, 0);
        break;
    case 0x1:
        if ((em_approach_step(self, 0, 0x80) == 1) && (em_mot_frame_started_ck(self) == 0)) {
            self->state++;
            em_mot_set(self, 0xB, 4, 0);
            em_approach_start(self, 0.0f, 0);
            self->timer_0x020 = 0xF;
            break;
        }
        break;
    case 0x2:
        em_approach_step(self, 0, 0x80);
        temp_r3_2 = self->timer_0x020;
        if (temp_r3_2 > 0) {
            self->timer_0x020 = temp_r3_2 - 1;
            break;
        }
        if ((em_mot_end_ck(self) == 1) || (calcVecDistXZ(&self->pos, &self->vec_0x36C) < *temp_r31)) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_21(_ENEMY_WORK *self) {
    f32 var_f1;
    f32 var_f2;
    s32 temp_r0;
    s32 var_r31;

    var_r31 = 0;
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        if ((s16) (calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) < 0) {
            var_f1 = 10.0f;
            var_f2 = 15.0f;
        } else {
            var_f1 = 15.0f;
            var_f2 = 10.0f;
        }
        temp_r0 = self->bits_0x1EC & 3;
        switch (temp_r0) {
        case 0x0:
            var_r31 = (u16) (0.5f + ((65536.0f * var_f1) / 360.0f));
            break;
        case 0x1:
            var_r31 = -(u16) (0.5f + ((65536.0f * var_f2) / 360.0f));
            break;
        }
        em_turn_seq_start(self, (void*)em012_turn_seq_1, 0, 1, (u16) var_r31);
        return;
    case 0x1:
        em_turn_seq_step(self, (void*)em012_turn_seq_1);
        if (em_frame_check(self, 1, 52.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act2_23(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCC, 4, 0);
        return;
    case 0x1:
        if (em_frame_check(self, 3, 0.0f, 44.0f) == 1) {
            em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (2.2727273f * get_em_base_scale(self))) / 360.0f)));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act2_27(_ENEMY_WORK *self, u8 arg1) {
    u8 temp_r0;

    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        if (arg1 == 0) {
            em_mot_set_ck(self, 0xA, 8, 0);
        } else {
            em_mot_set_ck(self, 0x10, 0xA, 0);
        }
        em_approach_start(self, 0.0f, 0);
        break;
    case 0x1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 4:
                switch (self->area_no) {
                case 0x5:
                    em_move_target_set(self, 0xFF, 0xC, 0.0f);
                    break;
                case 0x6:
                    em_move_target_set(self, 0xFF, 0xA, 0.0f);
                    break;
                }
                break;
            }
            em_turn_seq_start(self, (void*)em012_turn_seq_0, 0, 1, 0);
            if (self->field_0x482 == 1) {
                em_mot_speed_set(self, 1.2f);
                break;
            }
        } else {
            break;
        }
        break;
    case 0x2:
        if (em_turn_seq_step(self, (void*)em012_turn_seq_0) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 3, 4, 0);
            em_move_vec2_clr(self);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_c1(self, 240.0f);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 40.0f;
            self->field_0x318 = 60.0f;
            self->field_0x320 = -5.0f;
            self->field_0x324 = -1.0f;
            em_move_offset_step(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x4:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x314 < -100.0f) {
            self->field_0x314 = -100.0f;
            self->field_0x320 = 0.0f;
        }
        if (self->field_0x318 < 20.0f) {
            self->field_0x318 = 20.0f;
            self->field_0x324 = 0.0f;
        }
        if (em_ground_ck_c1(self, 160.0f) != 0) {
            self->state++;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x23, 0, 0);
            break;
        }
        break;
    case 0x5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act2_dispatch(_ENEMY_WORK *self) {

    switch (self->state_sub) {
    case 0x0:
        em012_act2_0(self, 0, 0);
        return;
    case 0x1:
        em012_act2_1(self, 0);
        return;
    case 0x2:
        em012_act2_2(self, 0);
        return;
    case 0x3:
        em012_act2_0(self, 1, 1);
        return;
    case 0x4:
        em012_act2_0(self, 8, 1);
        return;
    case 0x5:
        em012_act2_5(self, 6);
        return;
    case 0x6:
        em012_act2_0(self, 2, 1);
        return;
    case 0x7:
        em012_act2_5(self, 0);
        return;
    case 0x8:
        em012_act2_8(self, 0);
        return;
    case 0x9:
        em012_act2_5(self, 1);
        return;
    case 0xA:
        em012_act2_8(self, 1);
        return;
    case 0xB:
        em012_act2_1(self, 1);
        return;
    case 0xC:
        em012_act2_0(self, 7, 1);
        return;
    case 0xD:
        em012_act2_0(self, 3, 1);
        return;
    case 0xE:
        em012_act2_0(self, 9, 1);
        return;
    case 0xF:
        em012_act2_0(self, 0xA, 1);
        return;
    case 0x10:
        em012_act2_5(self, 2);
        return;
    case 0x11:
        em012_act2_0(self, 6, 1);
        return;
    case 0x12:
        em012_act2_0(self, 4, 1);
        return;
    case 0x13:
        em012_act2_0(self, 5, 1);
        return;
    case 0x14:
        em012_act2_20(self);
        return;
    case 0x15:
        em012_act2_21(self);
        return;
    case 0x16:
        em012_act2_8(self, 2);
        return;
    case 0x17:
        em012_act2_23(self);
        return;
    case 0x18:
        em012_act2_5(self, 3);
        return;
    case 0x19:
        em012_act2_5(self, 4);
        return;
    case 0x1A:
        em012_act2_5(self, 5);
        return;
    case 0x1B:
        em012_act2_27(self, 0);
        return;
    case 0x1C:
        em012_act2_27(self, 1);
        return;
    default:
        return;
    }
}

void em012_act7_0(_ENEMY_WORK *self, u8 arg1) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1) {
        case 0x0:
            em_mot_set(self, 0x16, 0xA, 0);
            break;
        case 0x1:
            em_turn_seq_start(self, (void*)em012_turn_seq_2, 0, 1, 0);
            break;
        }
        em_hit_window_set(self, 0, 5, 8);
        em_hit_window_set(self, 1, 6, 0x10);
        em_stamina_add(self, -0x32);
        break;
    case 0x1:
        switch (arg1) {
        case 0x0:
            if (em_frame_check(self, 3, 0.0f, 46.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (1.3043479f * get_em_base_scale(self))) / 360.0f)));
            }
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                break;
            }
            break;
        case 0x1:
            if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                em_mot_speed_set(self, 1.0f);
            }
            if (em_frame_check(self, 3, 0.0f, 32.0f) == 1) {
                em_turn_seq_step(self, (void*)em012_turn_seq_2);
            } else if (em_frame_check(self, 3, 34.0f, 46.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (0.9f * get_em_base_scale(self))) / 360.0f)));
            } else if (em_frame_check(self, 3, 70.0f, 90.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (0.5f * get_em_base_scale(self))) / 360.0f)));
            }
            if (em_frame_check(self, 0, 200.0f, 0.0f) == 1) {
                em_action_finish(self);
            }
            break;
        }
        break;
    }
}

void em012_act7_1(_ENEMY_WORK *self) {
    f32 temp_f31;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = 8.0f * get_em_base_scale(self) * temp_f31;
        self->field_0x324 = 0.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        return;
    case 0x1:
        if (em_frame_check(self, 3, 26.0f, 68.0f) == 1) {
            CancelFade(self);
        }
        em_turn_in_window2(self, 10.0f, 60.0f, -0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act7_2(_ENEMY_WORK *self, u8 arg1) {
    s16 temp_r0;
    s16 temp_r0_2;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        if (arg1 == 0) {
            em_mot_set(self, 0x14, 4, 0);
            em_hit_window_set_default(self, 0, 2);
            temp_r0 = calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0;
            self->timer_0x020 = temp_r0;
            if (temp_r0 < 0) {
                self->timer_0x020 = 0x5555;
                break;
            }
            if ((u32) temp_r0 < 0x31C7) {
                self->timer_0x020 = 0x31C7;
                break;
            }
            if ((u32) temp_r0 > 0x5555) {
                self->timer_0x020 = 0x5555;
                break;
            }
        } else {
            em_mot_set(self, 0x19, 4, 0);
            em_hit_window_set_default(self, 0, 7);
            temp_r0_2 = calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0;
            self->timer_0x020 = temp_r0_2;
            if (temp_r0_2 > 0) {
                self->timer_0x020 = -0x5555;
                break;
            }
            if ((u32) temp_r0_2 > -0x31C7) {
                self->timer_0x020 = -0x31C7;
                break;
            }
            if ((u32) temp_r0_2 < -0x5555) {
                self->timer_0x020 = -0x5555;
                break;
            }
            break;
        }
        break;
    case 0x1:
        if (arg1 == 0) {
            em_turn_in_window(self, 0.0f, 28.0f, 0xE39);
            em_turn_in_window(self, 42.0f, 58.0f, self->timer_0x020);
        } else {
            em_turn_in_window(self, 0.0f, 28.0f, -0xE39);
            em_turn_in_window(self, 42.0f, 58.0f, self->timer_0x020);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act7_3(_ENEMY_WORK *self, u8 arg1) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_mot_speed_set(self, 0.9f);
        em_hit_window_set(self, 0, 3, 8);
        break;
    case 0x1:
        switch (arg1) {
        case 0x0:
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                break;
            }
            break;
        case 0x1:
            if (em_frame_check(self, 1, 68.0f, 0.0f) == 1) {
                self->state++;
                em_move_mode_set(self, 0);
                em_mot_set(self, 0x1E, 4, 0);
                break;
            }
            break;
        case 0x2:
            if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (3.0f * get_em_base_scale(self))) / 360.0f)));
            }
            if (em_frame_check(self, 1, 68.0f, 0.0f) == 1) {
                self->state++;
                em_move_mode_set(self, 0);
                em_mot_set(self, 0x1E, 4, 0);
                break;
            }
            break;
        }
        break;
    case 0x2:
        if (em_frame_check(self, 1, 0.0f, 0.0f) == 1) {
            if (arg1 == 2) {
                em_state_set(self, 7, 0xC);
                break;
            }
            em_state_set(self, 7, 4);
        }
        break;
    }
}

void em012_act7_4(_ENEMY_WORK *self, u8 arg1) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        if (self->field_0xAE9 == 1) {
            em_hit_window_set(self, 0, 4, 0x10);
        } else {
            em_hit_window_set_default(self, 0, 4);
        }
        if (arg1 == 0) {
            em_stamina_add(self, -0x32);
            return;
        }
        em_stamina_add(self, -0x14);
        return;
    case 0x1:
        if (arg1 == 0) {
            if (em_frame_check(self, 3, 0.0f, 22.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (1.3636364f * get_em_base_scale(self))) / 360.0f)));
            }
        } else if (em_frame_check(self, 3, 0.0f, 22.0f) == 1) {
            em_turn_to_target(self, (u32) (u16) (0.5f + ((65536.0f * (4.090909f * get_em_base_scale(self))) / 360.0f)));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act7_6(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 4, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x71, 4, 0);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em012_act7_9(_ENEMY_WORK *self, u8 arg1) {
    EmSpawnRec rec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 6, 0);
        return;
    case 0x1:
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 0x3A, 0, 0);
        }
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            if (arg1 == 0) {
                em012_spawn_rec_set(&rec, 0, 0x1000, 0);
            } else {
                em012_spawn_rec_set(&rec, arg1, 0xA00, 0);
            }
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x1C);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act7_dispatch(_ENEMY_WORK *self) {

    switch (self->state_sub) {
    case 0x0:
        em012_act7_0(self, 0);
        return;
    case 0x1:
        em012_act7_1(self);
        return;
    case 0x2:
        em012_act7_2(self, 0);
        return;
    case 0x3:
        em012_act7_3(self, 0);
        return;
    case 0x4:
        em012_act7_4(self, 0);
        return;
    case 0x5:
        em012_act7_3(self, 1);
        return;
    case 0x6:
        em012_act7_6(self);
        return;
    case 0x7:
        em012_act7_0(self, 1);
        return;
    case 0x8:
        em012_act7_2(self, 1);
        return;
    case 0x9:
        em012_act7_9(self, 0);
        return;
    case 0xA:
        em012_act7_9(self, 1);
        return;
    case 0xB:
        em012_act7_3(self, 2);
        return;
    case 0xC:
        em012_act7_4(self, 1);
        return;
    default:
        return;
    }
}

EmSeRecord em012_se_rec_17 = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_18 = {0x65, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_18},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em012_se_pos_0 = {0.0f, 38.0f, 0, -32768, 0};

EmSeRecord em012_se_rec_23_a = {0x66, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em012_vec_pair_0[0]};

EmSeRecord em012_se_rec_23_b = {0x67, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em012_se_rec_23_c = {0x71, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_23[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x14)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em012_se_pos_0},
    {0x00, {0, 0, 0}, &em012_se_rec_23_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em012_se_rec_23_b},
    {0x00, {0, 0, 0}, &em012_se_rec_23_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_7A = {0x6D, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_7B = {0x6E, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_9F = {0x71, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_7C = {0x6F, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_7C},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_8D = {0xF, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_78 = {0x64, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_78},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_7D = {0x79, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_7D[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_7D},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_8E = {0xF, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_8E},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_A8 = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_B6 = {0x75, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_B6},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_B7 = {0x76, 0, 2, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_B7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_B8_a = {0x4, 0, 4, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em012_vec_pair_1[0]};

EmSeRecord em012_se_rec_B8_b = {0x23, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_B8[5] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA0)},
    {0x00, {0, 0, 0}, &em012_se_rec_B8_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em012_se_rec_B8_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_B9 = {0x77, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_B9},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_BA = {0x0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em012_se_rec_BA},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_BB = {0x77, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_BB},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_BF_a = {0x64, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em012_se_rec_BF_b = {0x6F, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em012_se_seq_BF[5] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em012_se_rec_BF_a},
    {0x00, {0, 0, 0}, &em012_se_rec_BF_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_alt_00_a = {0x66, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em012_vec_pair_0[0]};

EmSeRecord em012_se_rec_alt_00_b = {0x67, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em012_se_rec_alt_00_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_alt_00[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x14)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em012_se_pos_0},
    {0x00, {0, 0, 0}, &em012_se_rec_alt_00_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em012_se_rec_alt_00_b},
    {0x00, {0, 0, 0}, &em012_se_rec_alt_00_c},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_alt_0F = {0x77, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_alt_0F[3] = {
    {0x00, {0, 0, 0}, &em012_se_rec_alt_0F},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_alt_1A_a = {0x6D, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em012_se_rec_alt_1A_b = {0x6E, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_alt_1A[3] = {
    {0x00, {0, 0, 0}, &em012_se_rec_alt_1A_a},
    {0x00, {0, 0, 0}, &em012_se_rec_alt_1A_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em012_se_rec_alt_1C = {0x77, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em012_se_seq_alt_1C[2] = {
    {0x00, {0, 0, 0}, &em012_se_rec_alt_1C},
    {0xFF, {0, 0, 0}, 0},
};

void em012_act10_se_play(_ENEMY_WORK *self) {

    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em012_se_seq_17, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em012_se_seq_18, 0, 0x18);
        return;
    case 0x59:
        em_se_tbl_play(self, em012_se_seq_23, 1, 0x59);
        return;
    case 0x23:
        em_se_tbl_play(self, em012_se_seq_23, 1, 0x23);
        return;
    case 0x64:
        em_se_tbl_play(self, em012_se_seq_23, 1, 0x64);
        return;
    case 0x7A:
        em_se_tbl_play(self, em012_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em012_se_seq_7B, 0, 0x7B);
        return;
    case 0x9F:
        em_se_tbl_play(self, em012_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em012_se_seq_9F, 0, 0xA0);
        return;
    case 0x7C:
        em_se_tbl_play(self, em012_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em012_se_seq_8D, 0, 0x8D);
        return;
    case 0x78:
        em_se_tbl_play(self, em012_se_seq_78, 0, 0x78);
        return;
    case 0xA8:
        em_se_tbl_play(self, em012_se_seq_A8, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play(self, em012_se_seq_23, 1, 0xA9);
        return;
    case 0x7D:
        em_se_tbl_play(self, em012_se_seq_7D, 0, 0x7D);
        return;
    case 0x8E:
        em_se_tbl_play(self, em012_se_seq_8E, 0, 0x8E);
        return;
    case 0xB6:
        em_se_tbl_play(self, em012_se_seq_B6, 0, 0xB6);
        return;
    case 0xB7:
        em_se_tbl_play(self, em012_se_seq_B7, 0, 0xB7);
        return;
    case 0xB8:
        em_se_tbl_play(self, em012_se_seq_B8, 1, 0xB8);
        return;
    case 0xB9:
        em_se_tbl_play(self, em012_se_seq_B9, 0, 0xB9);
        return;
    case 0xBA:
        em_se_tbl_play(self, em012_se_seq_BA, 0, 0xBA);
        return;
    case 0xBB:
        em_se_tbl_play(self, em012_se_seq_BB, 0, 0xBB);
        return;
    case 0xBC:
        em_se_tbl_play(self, em012_se_seq_BB, 0, 0xBC);
        return;
    case 0xBF:
        em_se_tbl_play(self, em012_se_seq_BF, 0, 0xBF);
        return;
    case 0xC1:
        em_se_tbl_play(self, em012_se_seq_8D, 0, 0xC1);
        return;
    case 0x84:
        em_se_tbl_play(self, em012_se_seq_23, 1, 0x84);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

void em012_act11_se_play(_ENEMY_WORK *self) {

    switch (self->state_sub) {
    case 0x0:
        em_se_tbl_play_alt(self, em012_se_seq_alt_00, 1, 0);
        return;
    case 0x5:
        em_se_tbl_play_alt(self, em012_se_seq_alt_00, 1, 5);
        return;
    case 0xF:
        em_se_tbl_play_alt(self, em012_se_seq_alt_0F, 0, 0xF);
        return;
    case 0x1A:
        em_se_tbl_play_alt(self, em012_se_seq_alt_1A, 0, 0x1A);
        return;
    case 0x1C:
        em_se_tbl_play_alt(self, em012_se_seq_alt_1C, 0, 0x1C);
        return;
    default:
        em_se_tbl_play_alt(self, em012_se_seq_alt_00, 1, 0);
        return;
    }
}

void em012_act12_0(_ENEMY_WORK* self) {
    em012_act2_5(self, 2);
}

void em012_act12_1(_ENEMY_WORK* self) {
    em012_act2_0(self, 10, 1);
}

void em012_act12_2(_ENEMY_WORK* self) {
    em012_act2_27(self, 0);
}

void em012_act12_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em012_act12_0(self);
        break;
    case 1:
        em012_act12_1(self);
        break;
    case 2:
        em012_act12_2(self);
        break;
    }
}

f32 em012_demo_keys[135][4] = {
    {0.0f, 2412.96f, -28.56f, -4527.4f},
    {2.0f, 2389.38f, -30.44f, -4500.51f},
    {6.0f, 2345.52f, -41.39f, -4450.5f},
    {10.0f, 2309.61f, -52.92f, -4409.55f},
    {12.0f, 2290.74f, -55.52f, -4388.04f},
    {16.0f, 2247.37f, -55.02f, -4338.58f},
    {18.0f, 2227.76f, -53.45f, -4316.22f},
    {22.0f, 2193.54f, -48.86f, -4277.2f},
    {26.0f, 2161.34f, -43.92f, -4240.48f},
    {28.0f, 2145.51f, -41.91f, -4222.43f},
    {30.0f, 2128.67f, -40.46f, -4203.23f},
    {32.0f, 2108.89f, -39.28f, -4180.67f},
    {34.0f, 2085.38f, -38.25f, -4153.87f},
    {36.0f, 2057.68f, -37.33f, -4122.28f},
    {38.0f, 2032.29f, -36.46f, -4093.33f},
    {40.0f, 2012.07f, -35.57f, -4070.27f},
    {42.0f, 1994.47f, -34.62f, -4050.2f},
    {44.0f, 1977.39f, -33.54f, -4030.73f},
    {46.0f, 1959.75f, -32.42f, -4010.62f},
    {48.0f, 1939.45f, -31.39f, -3987.46f},
    {50.0f, 1918.74f, -30.6f, -3963.85f},
    {52.0f, 1899.13f, -30.21f, -3941.49f},
    {54.0f, 1881.06f, -30.09f, -3920.88f},
    {56.0f, 1864.06f, -30.02f, -3901.49f},
    {58.0f, 1848.34f, -29.81f, -3883.58f},
    {60.0f, 1832.75f, -29.09f, -3865.8f},
    {62.0f, 1817.08f, -25.72f, -3847.93f},
    {64.0f, 1800.36f, -22.4f, -3828.86f},
    {66.0f, 1778.84f, -20.51f, -3804.32f},
    {68.0f, 1752.97f, -18.83f, -3774.83f},
    {70.0f, 1728.48f, -17.39f, -3746.89f},
    {72.0f, 1698.67f, -16.17f, -3712.91f},
    {74.0f, 1671.12f, -15.91f, -3681.49f},
    {76.0f, 1647.25f, -18.12f, -3654.27f},
    {78.0f, 1625.38f, -4.14f, -3629.34f},
    {80.0f, 1600.85f, -3.94f, -3601.37f},
    {82.0f, 1573.36f, -3.44f, -3570.02f},
    {84.0f, 1548.05f, -2.75f, -3541.16f},
    {86.0f, 1526.14f, -2.0f, -3516.18f},
    {88.0f, 1505.34f, -1.31f, -3492.46f},
    {90.0f, 1485.28f, -0.81f, -3469.59f},
    {92.0f, 1465.56f, -0.61f, -3447.09f},
    {94.0f, 1444.35f, -0.63f, -3422.91f},
    {96.0f, 1418.46f, -0.68f, -3393.39f},
    {98.0f, 1385.91f, -0.76f, -3356.27f},
    {100.0f, 1352.99f, -0.87f, -3318.74f},
    {102.0f, 1327.77f, -1.0f, -3289.97f},
    {104.0f, 1306.1f, -1.15f, -3265.27f},
    {106.0f, 1284.71f, -1.31f, -3240.88f},
    {108.0f, 1259.97f, -1.48f, -3212.66f},
    {110.0f, 1234.15f, -1.66f, -3183.22f},
    {112.0f, 1210.2f, -1.83f, -3155.91f},
    {114.0f, 1188.35f, -2.01f, -3131.0f},
    {116.0f, 1168.37f, -2.17f, -3108.21f},
    {118.0f, 1148.88f, -2.33f, -3086.0f},
    {120.0f, 1129.11f, -2.47f, -3063.45f},
    {122.0f, 1105.63f, -2.59f, -3036.67f},
    {124.0f, 1074.33f, -2.68f, -3000.99f},
    {126.0f, 1047.05f, -2.75f, -2969.87f},
    {128.0f, 1017.24f, -2.79f, -2935.89f},
    {130.0f, 989.69f, -2.77f, -2904.47f},
    {132.0f, 965.82f, -2.62f, -2877.25f},
    {134.0f, 943.95f, -2.26f, -2852.31f},
    {136.0f, 919.42f, -1.66f, -2824.34f},
    {138.0f, 891.93f, -0.75f, -2792.99f},
    {140.0f, 866.62f, 0.62f, -2764.14f},
    {142.0f, 844.71f, 2.19f, -2739.16f},
    {144.0f, 823.91f, 3.68f, -2715.44f},
    {146.0f, 803.85f, 4.81f, -2692.56f},
    {148.0f, 784.13f, 5.29f, -2670.07f},
    {150.0f, 762.92f, 5.18f, -2645.89f},
    {152.0f, 737.03f, 4.71f, -2616.37f},
    {154.0f, 704.48f, 4.12f, -2579.25f},
    {156.0f, 671.56f, 3.63f, -2541.72f},
    {158.0f, 646.34f, 3.44f, -2512.95f},
    {160.0f, 624.67f, 3.38f, -2488.24f},
    {162.0f, 603.28f, 3.38f, -2463.86f},
    {164.0f, 578.54f, 3.43f, -2435.64f},
    {166.0f, 552.72f, 3.48f, -2406.2f},
    {168.0f, 528.77f, 3.5f, -2378.89f},
    {170.0f, 506.92f, 4.53f, -2353.98f},
    {172.0f, 486.94f, 6.99f, -2331.19f},
    {174.0f, 467.45f, 9.93f, -2308.97f},
    {176.0f, 447.68f, 12.38f, -2286.43f},
    {178.0f, 424.2f, 13.42f, -2259.65f},
    {180.0f, 392.9f, 13.42f, -2223.97f},
    {182.0f, 365.62f, 13.42f, -2192.85f},
    {184.0f, 335.81f, 13.42f, -2158.87f},
    {186.0f, 308.26f, 13.42f, -2127.45f},
    {188.0f, 284.39f, 14.13f, -2100.23f},
    {190.0f, 262.52f, 18.21f, -2075.29f},
    {192.0f, 237.99f, 22.29f, -2047.32f},
    {194.0f, 210.5f, 22.77f, -2015.97f},
    {196.0f, 185.19f, 21.22f, -1987.12f},
    {198.0f, 163.28f, 18.78f, -1962.13f},
    {200.0f, 142.48f, 16.18f, -1938.42f},
    {202.0f, 122.42f, 14.1f, -1915.54f},
    {204.0f, 102.7f, 13.25f, -1893.05f},
    {206.0f, 81.49f, 13.25f, -1868.87f},
    {208.0f, 55.6f, 13.25f, -1839.34f},
    {210.0f, 23.05f, 13.25f, -1802.22f},
    {212.0f, -9.87f, 13.25f, -1764.7f},
    {214.0f, -35.09f, 13.25f, -1735.93f},
    {216.0f, -56.76f, 13.25f, -1711.22f},
    {218.0f, -78.15f, 13.25f, -1686.84f},
    {222.0f, -128.71f, 13.25f, -1629.18f},
    {224.0f, -152.66f, 13.25f, -1601.87f},
    {226.0f, -174.51f, 13.25f, -1576.96f},
    {228.0f, -194.49f, 13.25f, -1554.17f},
    {230.0f, -213.98f, 13.25f, -1531.95f},
    {232.0f, -233.75f, 13.25f, -1509.4f},
    {234.0f, -257.23f, 13.25f, -1482.63f},
    {236.0f, -288.46f, 13.25f, -1447.02f},
    {238.0f, -317.11f, 13.25f, -1414.35f},
    {240.0f, -340.62f, 13.25f, -1387.55f},
    {242.0f, -360.08f, 13.25f, -1365.36f},
    {244.0f, -378.76f, 13.25f, -1344.06f},
    {246.0f, -396.82f, 13.25f, -1323.46f},
    {248.0f, -410.55f, 13.25f, -1307.81f},
    {252.0f, -437.2f, 13.25f, -1277.41f},
    {256.0f, -463.27f, 13.25f, -1247.69f},
    {260.0f, -487.81f, 13.25f, -1219.7f},
    {262.0f, -499.81f, 13.25f, -1206.02f},
    {264.0f, -511.78f, 13.25f, -1192.37f},
    {266.0f, -523.86f, 13.25f, -1178.6f},
    {268.0f, -536.17f, 13.25f, -1164.57f},
    {270.0f, -549.67f, 13.46f, -1149.17f},
    {272.0f, -559.93f, 14.0f, -1137.47f},
    {274.0f, -568.95f, 14.71f, -1127.19f},
    {278.0f, -585.58f, 15.95f, -1108.22f},
    {282.0f, -596.06f, 16.17f, -1096.27f},
    {288.0f, -605.3f, 16.15f, -1085.74f},
    {296.0f, -612.65f, 16.11f, -1077.36f},
    {323.0f, -612.65f, 15.97f, -1077.36f},
    {-1.0f, -612.65f, 15.97f, -1077.36f},
};

void em012_act13_0(_ENEMY_WORK *self) {
    VEC3 sp8;
    f32 temp_f31;
    f32 temp_f31_2;

    VEC3_ctor(&sp8);
    temp_f31 = (f32) (em_demo_frame_get() * 2);
    switch (self->state) {
    case 0x0:
        self->state++;
        self->timer_0x020 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = -0x1D55;
        self->field_0x1C4 = 0;
        em_demo_pos_set(self, -600.0f, 0.0f, 1370.0f);
        em_demo_reset(self, 0);
        em_shake_req_set(self);
        break;
    case 0x1:
        em_shake_req_set(self);
        if (em_demo_time_ck(0x298) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0xA, 0, 0x1A);
            em_mot_speed_set(self, 0.8f);
            self->field_0x1C0 = -0x1D55;
            em_flag_0x116_set(self);
            em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x14C), em012_demo_keys, 0);
            em_height_set(self, self->pos.y - self->field_0x20C);
            setVector3(&sp8, 0.0f, 0.0f, -200.0f);
            rotVecY(&sp8, self->field_0x1C0);
            em_demo_pos_set(self, self->pos.x + sp8.x, self->pos.y + sp8.y, self->pos.z + sp8.z);
            break;
        }
    default:
        break;
    case 0x2:
        em_shake_req_set(self);
        em_flag_0x116_set(self);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x14C), em012_demo_keys, 0);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (666.0f == temp_f31) {
            setVector3(&sp8, 0.0f, 0.0f, -200.0f);
        } else {
            setVector3(&sp8, 0.0f, 0.0f, -120.0f);
        }
        rotVecY(&sp8, self->field_0x1C0);
        em_demo_pos_set(self, self->pos.x + sp8.x, self->pos.y + sp8.y, self->pos.z + sp8.z);
        if (em_demo_time_ck(0x2DC) == 1) {
            self->state++;
            em_mot_set(self, 0xA, 2, 0x1A);
            em_mot_speed_set(self, 1.0f);
            break;
        }
        break;
    case 0x3:
        if (temp_f31 < 772.0f) {
            em_shake_req_set(self);
        }
        em_flag_0x116_set(self);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x14C), em012_demo_keys, 0);
        em_height_set(self, self->pos.y - self->field_0x20C);
        setVector3(&sp8, 0.0f, 0.0f, -120.0f);
        rotVecY(&sp8, self->field_0x1C0);
        em_demo_pos_set(self, self->pos.x + sp8.x, self->pos.y + sp8.y, self->pos.z + sp8.z);
        if (em_demo_time_ck(0x386) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xB, 2, 0, 0);
            em_mot_speed_set(self, 0.8f);
            break;
        }
        break;
    case 0x4:
        em_shake_req_set(self);
        em_flag_0x116_set(self);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x14C), em012_demo_keys, 0);
        em_height_set(self, self->pos.y - self->field_0x20C);
        setVector3(&sp8, 0.0f, 0.0f, -120.0f);
        rotVecY(&sp8, self->field_0x1C0);
        em_demo_pos_set(self, self->pos.x + sp8.x, self->pos.y + sp8.y, self->pos.z + sp8.z);
        if (em_demo_time_ck(0x3C0) == 1) {
            self->state++;
            em_mot_set_blend(self, 9, 0x18, 0x6A, 1);
            em_mot_speed_set(self, 0.5f);
            em_height_set(self, 0.0f);
            break;
        }
        break;
    case 0x5:
        em_shake_req_set(self);
        if (em_demo_time_ck(0x3E2) == 1) {
            self->state++;
            em_mot_layer_weight_set(self, 9, 0, 1.0f);
            em_mot_speed_set(self, 0.5f);
            self->timer_0x020 = 0;
            break;
        }
        break;
    case 0x6:
        em_shake_req_set(self);
        self->timer_0x020 += 1;
        temp_f31_2 = 1.0f - ((f32) self->timer_0x020 * (0.5f / (40.0f / get_em_base_scale(self))));
        em_mot_layer_weight_set(self, 9, (s32) ((f32) self->timer_0x020 * get_em_base_scale(self)), temp_f31_2);
        if (em_demo_time_ck(0x436) == 1) {
            self->state++;
            em_mot_set_blend(self, 9, 0, 0, 0);
            em_mot_speed_set(self, 0.6f);
            self->field_0x1C0 = -0x1EEF;
            em_demo_pos_set(self, 544.8445f, 17.4916f, -2514.2f);
            break;
        }
        break;
    case 0x7:
        if (em_demo_time_ck(0x566) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xC9, 0, 0x12, 0);
            em_mot_speed_set(self, 0.8f);
            self->field_0x1C0 = 0x800;
            em_demo_pos_set(self, -314.772f, 17.4916f, -54.9744f);
            break;
        }
        break;
    case 0x8:
        if (em_demo_time_ck(0x7CE) == 1) {
            self->state++;
            em_mot_set_blend(self, 9, 0, 8, 0);
            em_mot_speed_set(self, 0.65f);
            self->field_0x1C0 = -0x2C44;
            em_demo_pos_set(self, -314.772f, 17.4916f, -54.9744f);
        }
        break;
    }
}

void em012_act13_1(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        setVector3(&self->pos, -314.772f, 0.0f, -54.9744f);
        copyVec3(&self->prev_pos, &self->pos);
        self->field_0x1BC = 0;
        self->field_0x1C0 = -0x2C44;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act13_2(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 0x1:
        if (em_demo_time_ck(0xF2) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0xA, 0, 0x1A);
            em_demo_pos_set(self, 3219.435f, -63.0735f, 1769.0487f);
            em_demo_rot_set(self, 0.0f, -114.1486f, 0.0f);
            break;
        }
        break;
    case 0x2:
        if (em_demo_time_ck(0x198) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 4, 0);
            break;
        }
        break;
    case 0x3:
        if (em_demo_time_ck(0x1C0) == 1) {
            self->state++;
            em_mot_set(self, 2, 6, 0);
        }
        /* fallthrough */
    case 0x4:
        if (em_demo_time_ck(0x21C) == 1) {
            self->state++;
            em_mot_set(self, 9, 0, 0);
            em_demo_pos_set(self, 188.9194f, -22.7001f, -37.8379f);
            em_demo_rot_set(self, 0.0f, -99.0f, 0.0f);
        }
        /* fallthrough */
    case 0x5:
        if (em_demo_time_ck(0x480) == 1) {
            self->state++;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 417.9485f, -8.4067f, 400.6546f);
            em_demo_rot_set(self, 0.0f, -105.0f, 0.0f);
            break;
        }
        break;
    case 0x6:
        if (em_demo_time_ck(0x5DC) == 1) {
            self->state++;
            em_demo_pos_set(self, 645.4836f, -8.4067f, 779.2169f);
            em_demo_rot_set(self, 0.0f, 15.0f, 0.0f);
        }
        break;
    }
}

void em012_act13_3(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 0, 0);
        em_demo_pos_set(self, 645.4836f, -8.4067f, 779.2169f);
        em_demo_rot_set(self, 0.0f, 15.0f, 0.0f);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act13_4(_ENEMY_WORK *self) {
    EmSpawnRec rec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 0x1:
        if (em_demo_time_ck(0x1B2) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 0, 0);
            em_demo_pos_set(self, -1507.4f, 10.0f, -413.8f);
            em_demo_rot_set(self, 0.0f, 83.0f, 0.0f);
            em_demo_enable(self);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 6, 0);
            break;
        }
        break;
    case 0x3:
        if (em_frame_check(self, 0, 232.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0xCB, 8, 0);
            break;
        }
        break;
    case 0x4:
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 0x3A, 0, 0);
        }
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em012_spawn_rec_set(&rec, 1, 0xE00, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x1C);
        }
        if (em_demo_time_ck(0x36A) == 1) {
            self->state++;
            em_mot_set(self, 0xCB, 2, 0);
            em_mot_speed_set(self, 0.54f);
            break;
        }
        break;
    case 0x5:
        if (em_demo_time_ck(0x47C) == 1) {
            self->state++;
            em_mot_set(self, 0xC, 0, 0);
            em_demo_pos_set(self, -432.2f, 4.0f, -254.1f);
            em_demo_rot_set(self, 0.0f, 83.0f, 0.0f);
            break;
        }
        break;
    case 0x6:
        if (em_demo_time_ck(0x74C) == 1) {
            self->state++;
            em_mot_set(self, 0xC9, 0, 0);
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xC, 0, 0);
        }
        break;
    }
}

void em012_act13_5(_ENEMY_WORK *self) {

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 0, 0);
        em_demo_pos_set(self, -432.2f, 4.0f, -254.1f);
        em_demo_rot_set(self, 0.0f, 83.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em012_act13_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em012_act13_0(self);
        break;
    case 1:
        em012_act13_1(self);
        break;
    case 2:
        em012_act13_2(self);
        break;
    case 3:
        em012_act13_3(self);
        break;
    case 4:
        em012_act13_4(self);
        break;
    case 5:
        em012_act13_5(self);
        break;
    }
}

void em012_action_dispatch(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em012_act0_dispatch(self);
        break;
    case 1:
        em012_act1_dispatch(self);
        break;
    case 2:
        em012_act2_dispatch(self);
        break;
    case 7:
        em012_act7_dispatch(self);
        break;
    case 10:
        em012_act10_se_play(self);
        break;
    case 11:
        em012_act11_se_play(self);
        break;
    case 12:
        em012_act12_dispatch(self);
        break;
    case 13:
        em012_act13_dispatch(self);
        break;
    }
}

void em012_alt_mode_effect(_ENEMY_WORK *self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    if ((em_alt_mode_ck(self) == 1) && ((system_w.field_0x0c % 20) == 0)) {
        setVector3(&sp8, 0.0f, 10.0f, 70.0f);
        eft_spawn_type10(self, 1, 0x13, &sp8, 1.0f);
    }
}

void em012_effect_spawn(_ENEMY_WORK *self, u8 arg1, u8 arg2, s32 arg3, s32 arg4, f32 farg0) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (arg1) {
    case 0x0:
        eft009_spawn_at_joint(self, (u32) arg3, (u32) arg2, arg4, farg0);
        return;
    case 0x1:
        get_joint_wpos_em(self, (u32) arg3, &sp8);
        sp8.y = self->field_0x20C;
        farg0 *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&sp8, self->area_no, arg2, arg4, farg0);
        return;
    }
}

void em012_motion_events(_ENEMY_WORK *self) {
    u16 temp_r3;
    u16 temp_r3_2;
    u32 var_r30;
    u8 temp_r0;

    em012_alt_mode_effect(self);
    temp_r3 = em_get_mot_no(self);
    switch (temp_r3) {
    case 0x3:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0x12, 3, 0, 0.8f);
            }
        } else if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 2, 3, 0, 1.0f);
        }
        break;
    case 0x5:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
        }
        break;
    case 0x6:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
        }
        break;
    case 0xA:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
        } else {
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0, 0x23, 0, 0.7f);
            }
        }
        break;
    case 0xB:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
        }
        break;
    case 0xF:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xD, 0x23, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xD, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.7f);
                em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.7f);
                em012_effect_spawn(self, 0, 0xE, 0x1F, 0, 0.7f);
            }
        } else {
            if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0, 0x23, 0, 0.7f);
            }
            if ((em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0, 0x1F, 0, 0.7f);
            }
        }
        break;
    case 0x10:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.6f);
            }
        } else {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
        }
        break;
    case 0x11:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 3, 0, 0.6f);
                em012_effect_spawn(self, 0, 0xF, 3, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 8, 0x1F, 0, 1.2f);
        }
        break;
    case 0x12:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
        }
        break;
    case 0x13:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
        }
        break;
    case 0x14:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 6, 0x23, 0, 0.6f);
        }
        break;
    case 0x15:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x23, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
        }
        break;
    case 0x16:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.6f);
            }
            if ((em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xE, 0x1F, 0, 0.6f);
            }
            if ((em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.6f);
            }
        } else {
            if ((em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1)) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
        }
        break;
    case 0x19:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.6f);
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
        }
        break;
    case 0x22:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x24, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x24, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 1, 4, 3, 0, 0.3f);
        }
        if ((self->team == 0xE) && (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1)) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x23:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 3, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 3, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 1, 4, 3, 0, 0.3f);
        }
        if ((self->team == 0xE) && (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1)) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x64:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.7f);
                em012_effect_spawn(self, 0, 0xE, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.7f);
                em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
            }
        }
        break;
    case 0x67:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0x14, 3, 0, 0.8f);
                em_camera_req(self, 3, 7);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 1, 5, 3, 0, 0.3f);
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x6D:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0x14, 3, 0, 0.8f);
            }
        } else if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 1, 4, 3, 0, 0.3f);
        }
        if ((self->team == 0xE) && (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1)) {
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x71:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xF, 0x1F, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
        }
        break;
    case 0x75:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em012_effect_spawn(self, 0, 0x14, 0xFF, 0, 1.0f);
            } else {
                em012_effect_spawn(self, 1, 2, 3, 0, 0.4f);
            }
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x77:
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em012_effect_spawn(self, 0, 0x15, 0xE, 0, 0.6f);
            } else {
                em012_effect_spawn(self, 1, 0, 0xE, 0, 0.4f);
            }
        }
        break;
    }
    if (self->team == 0xE) {
        temp_r3_2 = em_get_mot_no(self);
        switch (temp_r3_2) {
        case 0xCB:
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                if ((self->field_0x228 & 6) != 0) {
                    em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.7f);
                    em012_effect_spawn(self, 0, 0xE, 0x1F, 0, 0.7f);
                } else {
                    em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
                }
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em012_effect_spawn(self, 0, 0xD, 0x23, 0, 0.7f);
                    em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.7f);
                } else {
                    em012_effect_spawn(self, 0, 0, 0x23, 0, 0.7f);
                }
            }
            if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.7f);
                    em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.7f);
                } else {
                    em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
                }
            }
            break;
        case 0xCC:
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em012_effect_spawn(self, 0, 0xC, 0x1F, 0, 0.7f);
                    em012_effect_spawn(self, 0, 0xE, 0x1F, 0, 0.7f);
                } else {
                    em012_effect_spawn(self, 0, 2, 0x1F, 0, 0.7f);
                }
            }
            if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em012_effect_spawn(self, 0, 0xC, 0x23, 0, 0.7f);
                    em012_effect_spawn(self, 0, 0xE, 0x23, 0, 0.7f);
                } else {
                    em012_effect_spawn(self, 0, 2, 0x23, 0, 0.7f);
                }
            }
            break;
        }
    }
    if (self->team == 0xE) {
        var_r30 = 0;
        switch (stage_map_kind_get(self->field_0x1E0)) {
        case 4:
            switch (self->area_no) {
            case 4:
            case 6:
                var_r30 = 1;
                break;
            }
            break;
        }
        if ((var_r30 == 1) && (self->field_0x762 == 0)) {
            self->field_0x761 |= 1;
            return;
        }
        self->field_0x761 = 0;
    }
}

void em012_tev_color_update(_ENEMY_WORK *self) {
    if (self->team != 14) {
        u32 mat;
        u32 rim;
        u32 body;
        s32 out_rim;
        s32 out_body;

        if (em_parts_damage_level_get(self, 0) < 1) {
            mat = 2;
            rim = 60;
            body = 255;
        } else {
            mat = 2;
            rim = 140;
            body = 255;
        }
        out_rim = (s32)((f32)rim * self->field_0x1D4);
        out_body = (s32)((f32)body * self->field_0x1D4);
        mhchar_mat_tev_set((MHchar *)self->char_0x024, mat, 6, out_rim, 0, 3, out_body, self->field_0x1D4);
    }
}

u32 em012_state_ck(_ENEMY_WORK *self, u8 arg1) {
    switch (arg1) {
    case 0:
        return (u8)(self->field_0x1E4 == 1) & 0xFF;
    case 1: {
        u8 count = 0;
        _ENEMY_WORK *work = (_ENEMY_WORK *)get_move_work_adrs(3);
        u32 max = get_move_work_max(3);
        EmAreaEntry *entry;
        u16 i;
        u16 j;

        if (work != 0) {
            for (i = 0; i < (u16)max; i++, work++) {
                if (work->active != 0 && work->team == 10 && work->field_0x46C == self->field_0x46C &&
                    work->field_0x011 != 0) {
                    count++;
                }
            }
        }
        entry = &em_area_entry_tbl[0][0];
        for (j = 0; j < 128; j++, entry++) {
            if (entry->active != 0 && entry->group == 10 && entry->entry_0x09 == self->field_0x46C &&
                entry->kind_0x08 == 4) {
                count++;
            }
        }
        return count <= 1U;
    }
    case 2: {
        s32 blocked = 0;

        if ((stage_map_kind_get(self->field_0x1E0) == 1 && self->area_no == 9) ||
            (stage_map_kind_get(self->field_0x1E0) == 2 && self->area_no == 1)) {
            blocked = 1;
        }
        if (blocked == 0 && em_in_water_ck(self, 0) == 1) {
            return 1;
        }
        return 0;
    }
    }
    return 0;
}

void em012_area_start_set(_ENEMY_WORK *self, u8 *arg1, u8 *arg2) {
    u32 var_r31;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r0_3;
    u8 temp_r3;

    var_r31 = 0;
    em_move_mode_set(self, 0);
    *arg1 = 0xC;
    temp_r3 = stage_map_kind_get(self->field_0x1E0);
    switch (temp_r3) {
    case 0x1:
        temp_r0 = self->area_no;
        switch (temp_r0) {
        case 0x2:
            em_move_target_set(self, 0x12, 0xC, 0.0f);
            *arg2 = 0;
            break;
        case 0x3:
            em_move_target_set(self, 0x11, 0xB, 0.0f);
            *arg2 = 1;
            break;
        case 0x9:
            em_move_target_set(self, 0x10, 0xA, 0.0f);
            *arg2 = 1;
            break;
        default:
            var_r31 = 1;
            break;
        }
        break;
    case 0x2:
        temp_r0_2 = self->area_no;
        switch (temp_r0_2) {
        case 0x4:
            em_move_target_set(self, 0xA, 0xB, 0.0f);
            *arg2 = 1;
            break;
        case 0x5:
            em_move_target_set(self, 9, 0xB, 0.0f);
            *arg2 = 1;
            break;
        default:
            var_r31 = 1;
            break;
        }
        break;
    case 0x4:
        temp_r0_3 = self->area_no;
        switch (temp_r0_3) {
        case 0x2:
            em_move_target_set(self, 9, 0xB, 0.0f);
            *arg2 = 1;
            break;
        case 0x5:
            em_move_target_set(self, 0xA, 0x10, 0.0f);
            *arg2 = 2;
            break;
        default:
            var_r31 = 1;
            break;
        }
        break;
    default:
        var_r31 = 1;
        break;
    }
    if (var_r31 == 1) {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        em_reaction_req(self, 5, 2, 0xA);
        *arg2 = 0;
    }
}

void em012_gauge_update(_ENEMY_WORK *self) {
    self->field_0x328 = 0;
    if (em_mot_finished_ck_c1(self) == 1) {
        em_gauge_period_step(self, 0.05f * (f32)(s32)self->field_0x7A4, 1, em_mot_window_base_get(self) - 0.05f);
    }
}

s32 em012_part_level_ck(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 1 && ((u8)em_parts_damage_level_get(self, 1) & 1) == 0) {
        return 1;
    }
    return 0;
}

void em012_part_drop(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 0 && self->team == 14 && em_parts_damage_level_get(self, 0) == 1) {
        em_drop_item_spawn(self, &self->pos, 100);
    }
}

s32 em012_action_select(_ENEMY_WORK *self, u16 arg) {
    s16 *cooldown = &self->field_0x328;
    _ENEMY_WORK *work = (_ENEMY_WORK *)get_move_work_adrs(3);
    u16 max = get_move_work_max(3);
    u8 kind = stage_map_kind_get(self->field_0x1E0);
    u32 hit;
    u32 area;

    switch (kind) {
    case 1:
    case 2:
    case 4:
        break;
    default:
        return 0;
    }
    if (em_area_stun_move_ck(self) == 1) {
        self->field_0x1E4 = 0;
        return 1;
    }
    hit = 0;
    switch (kind) {
    case 1:
        area = 9;
        break;
    case 2:
        area = 5;
        break;
    case 4:
        area = 5;
        break;
    default:
        area = 0xFF;
        break;
    }
    if (area != 0xFF) {
        switch (em_area_break_ck(self)) {
        case 1:
            hit = 1;
            break;
        case 2:
            self->field_0x1E4 = 1;
            return 1;
        }
    }
    if (hit == 0) {
        switch (em_area_player_follow_ck(self)) {
        case 1:
            return 0;
        case 2:
            self->field_0x1E4 = 0;
            return 1;
        }
    }
    if (em_mot_finished_ck(self) == 0 && self->value_0x452 >= 900) {
        u32 pick;

        switch (kind) {
        case 1:
            pick = 9;
            break;
        case 2:
            pick = 5;
            break;
        case 4:
            pick = 5;
            break;
        default:
            pick = 0xFF;
            break;
        }
        if (em_pack_area_pick(self, 0, pick, arg, 0, em012_pick_tbl) == 1) {
            self->field_0x1E4 = 0;
            return 1;
        }
    }
    if (self->value_0x452 >= self->field_0x450 && self->field_0x43D == 1) {
        ENEMY_ENTRY *entry = (ENEMY_ENTRY *)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);

        if (entry != 0 && entry->field_0x0D == 1) {
            s32 found = 0;
            s32 i;

            for (i = 0; i < max; i++, work++) {
                if (work->active != 0 && work->action != 11 && work->action != 12 && work->area_no == self->area_no) {
                    if (self->team == 12) {
                        if ((u8)(work->team + 0xF6) <= 1) {
                            found = 1;
                            break;
                        }
                    } else if (self->team == 14 && work->team == 13) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 0 && em_roll_cooldown_ck(self, arg, 30, cooldown) == 1) {
                if (em_mot_finished_ck_c1(self) == 1) {
                    switch (kind) {
                    case 1:
                        if (self->area_no != 9) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            self->field_0x1FF = 9;
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        if (self->value_0x44C <= 0) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            if ((arg & 1) == 0) {
                                self->field_0x1FF = 4;
                            } else {
                                self->field_0x1FF = 5;
                            }
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        break;
                    case 2:
                        if (self->area_no != 5) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            self->field_0x1FF = 5;
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        if (self->value_0x44C <= 0) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            self->field_0x1FF = 4;
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        break;
                    case 4:
                        if (self->area_no != 5) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            self->field_0x1FF = 5;
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        if (self->value_0x44C <= 0) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 1;
                            if ((arg & 1) == 0) {
                                self->field_0x1FF = 3;
                            } else {
                                self->field_0x1FF = 6;
                            }
                            self->field_0x1E4 = 1;
                            return 1;
                        }
                        break;
                    }
                } else {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 0;
                    self->field_0x1FF = 0xFF;
                    self->field_0x1E4 = 0;
                    return 1;
                }
            }
        }
    }
    if (em_area_roam_pick(self, arg) == 1) {
        if (em_mot_finished_ck_c1(self) == 1) {
            switch (kind) {
            case 1:
                if (self->area_no != 9) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = 9;
                    self->field_0x1E4 = 1;
                    return 1;
                }
                if (self->value_0x44C <= 0) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    if ((arg & 1) == 0) {
                        self->field_0x1FF = 4;
                    } else {
                        self->field_0x1FF = 5;
                    }
                    self->field_0x1E4 = 1;
                    return 1;
                }
                break;
            case 2:
                if (self->area_no != 5) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = 5;
                    self->field_0x1E4 = 1;
                    return 1;
                }
                if (self->value_0x44C <= 0) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = 4;
                    self->field_0x1E4 = 1;
                    return 1;
                }
                break;
            case 4:
                if (self->area_no != 5) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = 5;
                    self->field_0x1E4 = 1;
                    return 1;
                }
                if (self->value_0x44C <= 0) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    if ((arg & 1) == 0) {
                        self->field_0x1FF = 3;
                    } else {
                        self->field_0x1FF = 6;
                    }
                    self->field_0x1E4 = 1;
                    return 1;
                }
                break;
            }
        }
        self->field_0x1E4 = 0;
        return 1;
    }
    if (em_area_leave_ck(self) == 1) {
        if (hit == 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 1;
            switch (kind) {
            case 1:
                if ((arg & 1) == 0) {
                    self->field_0x1FF = 4;
                } else {
                    self->field_0x1FF = 5;
                }
                break;
            case 2:
                self->field_0x1FF = 4;
                break;
            case 4:
                if ((arg & 1) == 0) {
                    self->field_0x1FF = 3;
                } else {
                    self->field_0x1FF = 6;
                }
                break;
            }
            self->field_0x1E4 = 1;
        } else {
            self->field_0x1E4 = 0;
        }
        return 1;
    }
    return 0;
}

s32 em012_area_move_armed_ck(_ENEMY_WORK *self) {
    u8 temp_r0;

    if (self == NULL) {
        return 0;
    }
    if (self->active == 0) {
        return 0;
    }
    temp_r0 = self->team;
    if (((temp_r0 == 0xC) || (temp_r0 == 0xE)) && (self->field_0x1E4 == 1)) {
        return 1;
    }
    return 0;
}

void em012_spawn_rec_set(EmSpawnRec *rec, u8 arg1, u16 arg2, u16 arg3) {
    VEC3 sp8;

    setVec3(&sp8, 0.0f, 0.0f, 0.0f);
    rec->id = 0x13;
    copyVec3(&rec->pos, &sp8);
    rec->field_0x10 = arg1;
    rec->field_0x12 = arg2;
    rec->field_0x14 = arg3;
}

}

/* The unit's `.bss`: the two two-vector records `em012_static_init` seeds (each a pair of model-space points). */
VEC3 em012_vec_pair_0[2]; /* 0x806A79A0 */
VEC3 em012_vec_pair_1[2]; /* 0x806A79B8 */

/* 0x80176B7C (0xB4): the static initializer: seeds the two vector pairs. */
extern "C" void em012_static_init(void)
{
    VEC3 v3;
    VEC3 v2;
    VEC3 v1;
    VEC3 v0;

    assignVec3_c1(&em012_vec_pair_0[0], setVec3(&v3, 0.0f, 38.0f, -25.0f));
    assignVec3_c1(&em012_vec_pair_0[1], setVec3(&v2, 0.0f, -5.0f, 0.0f));
    assignVec3_c1(&em012_vec_pair_1[0], setVec3(&v1, 0.0f, 45.0f, 0.0f));
    assignVec3_c1(&em012_vec_pair_1[1], setVec3(&v0, 0.0f, -5.0f, 0.0f));
}

/* The `.ctors` word (0x8056F334) - it points at the static initializer. */
__declspec(section ".ctors") void* const em012_ctor = (void*)em012_static_init;
