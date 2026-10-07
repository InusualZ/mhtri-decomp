/*
 * enemy/em029_prog.cpp - enemy 029's program: the action and sub-state dispatchers of its `_ENEMY_WORK`, the handlers
 *   they tail-call, the two turn sequences, and the program table's condition, setup, damage and stub slots.  C++.
 * RANGE. .text 0x8039E5CC-0x803A12D4 (42 functions); extab 0x80018684-0x8001877C, extabindex 0x8003882C-0x800389A0,
 *   .rodata 0x80570BA0-0x80570C20 (the two turn sequences), .data 0x805F1708-0x805F1E18 (`em029_prog_tbl` first, then
 *   the dispatchers' switch tables and the sound programs `enemy/em_se_record.h` describes), .sdata2
 *   0x8079C330-0x8079C438 (66 words, all read by this range).  Left edge: the box band of `menu/menu_result.cpp`
 *   ends at `multi_box_result_step`, and `em029_area_target_set` opens the extab/extabindex records of this TU (record
 *   9 of the old run); right edge: `lobby_flow_init` opens the lobby flow of `menu/multi_result.cpp`.  The seam is
 *   proven by three facts: every `.data` relocation of 0x0-0x70C targets this range's text or data, `em_parts_damage0_ck`
 *   (the table's slot +0x44) and `em_area_team_ck` (called only by `em029_frame_pre`/`em_action7_step`) sit past
 *   `em029_setup` and belong to the program, and the pool holds its own copies of the constants `menu/menu_result.cpp`
 *   pools (0x42F00000, 0x3F000000, 0x42700000: one TU keeps one copy).
 * FLAGS. `cflags_menu` (configure.py), the flags of the two neighbours it was cut from.
 * NAMES. The file name follows the runtime dump's `em029_prog_tbl`, which opens the TU's `.data` (the sibling
 *   `enemy/em020_prog.cpp` is named the same way); no `__FILE__` string reaches the range, so every function name is a
 *   GUESS from its body and callers.
 *   GUESS (from each body and its callers): em029_area_target_set, em029_init, em029_frame_pre, em029_effect_spawn
 *   GUESS: em029_condition_ck, em029_setup, em_area_team_ck, em029_turn_seq_a, em029_turn_seq_b, em_parts_damage0_ck
 *   GUESS (the dispatch ids and sub-states the program table reaches): em_action_dispatch, em_action0_dispatch
 *   GUESS: em_action0_step, em_action1_dispatch, em_action1_sub0, em_action1_sub1, em_action1_sub2, em_action1_sub3
 *   GUESS: em_action1_sub4, em_action1_sub5, em_action1_sub6, em_action1_sub7, em_action1_sub8, em_action2_dispatch
 *   GUESS: em_action2_sub0, em_action2_sub1, em_action2_sub2, em_action2_sub3, em_action2_sub4, em_action2_sub5
 *   GUESS: em_action3_dispatch, em_action3_sub1, em_action3_sub2, em_action6_dispatch, em_action6_sub0
 *   GUESS: em_action6_sub1, em_action7_dispatch, em_action7_step, em_action4_dispatch, em_action5_dispatch
 *   GUESS: em_action3_sub0
 *   GUESS (stubs named for their bodies; the dump's names are junk duplicates): em_action_nop, em_action_ret0
 *   GUESS (the program table's slot +0x18, `em024_prog_tbl`'s motion-event slot): em029_motion_events
 *   GUESS (`Em029ProgTbl` slot names follow `Em024ProgTbl`): Em029ProgTbl
 *   GUESS (sound programs, named for the motion that plays them and their nesting): em029_se_tbl_mot<N>,
 *   em029_se_rec_mot<N>_<x>, em029_se_tbl_act5_1/act5_2 and their em029_se_tbl_/_rec_/_branch_act5_<n>_<x> children,
 *   em029_se_rec_unused_0..2 (records no entry points at)
 * RESIDUALS. none: every row matches, and `.text`, extab, extabindex, `.rodata`, `.data` and `.sdata2` are
 *   byte-identical to the target object (flipcheck READY).
 * SHAPES. The sound programs are defined after `em_action5_dispatch` (declared `extern` above `em_action4_dispatch`)
 *   so the dispatchers load each table by name and the switch tables keep their places around them; the three
 *   records no entry points at are `__declspec(export)` (force-active in retail).
 */

#include "types.h"
#include "enemy/em029_prog.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_action_finish.h"
#include "menu/hit_mask_ck.h"
#include "enemy/em_hit_window_set.h"
#include "enemy/em_hit_window_clear.h"
#include "enemy/em_approach_start.h"
#include "enemy/em_approach_step.h"
#include "Pl/plw.h"
#include "enemy/fn_8011D448.h"
#include "unsplit/enemy.h"   /* the enemy band's helpers (rule 2: their owners' band header) */
#include "enemy/fn_8012BDF4.h" /* `em_busy_set` */
#include "stage/stg_w.h"
#include "enemy/em_move_target_set.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/em_hit_window_set_default.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em019_action13_active_ck.h"
#include "enemy/em_ground_rec_clear.h"
#include "enemy/enemy_control.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/fn_80137604.h"
#include "ef/eft009.h"
#include "ef/fn_8010D1A8.h"
#include "ef/get_move_work_adrs.h"
#include "ai/ainpc_w.h"
#include "ai/ainpc.h"
#include "ai/ai_torch_ck.h"
#include "mh3_pad.h"

/* The em029 program table: the entry points the shared enemy program driver calls.  Slot names follow
 * `Em024ProgTbl` (`enemy/em024_ai.cpp`), a GUESS from the slots the two tables share. */
struct Em029ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ u32 pad_0x14;
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C[2];
    /* +0x24 */ s32 (*condition_ck_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ s32 (*ret0_0x28)(void);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*setup_0x30)(struct _ENEMY_WORK* self, s8* speed, s8* far);
    /* +0x34 */ u32 pad_0x34[4];
    /* +0x44 */ u32 (*part_damage_ck_0x44)(struct _ENEMY_WORK* self, u8 part);
    /* +0x48 */ u32 pad_0x48[9];
}; /* size: 0x6C */

extern "C" Em029ProgTbl em029_prog_tbl = {
    0,
    em029_init,
    em029_frame_pre,
    em_action_dispatch,
    em_action_nop,
    0,
    em029_motion_events,
    {0},
    em029_condition_ck,
    em_action_ret0,
    0,
    em029_setup,
    {0},
    em_parts_damage0_ck,
    {0},
};

/* 0x8039E5CC (0xC0): in the type-5 map's areas 4, 5, 6 and 8, sets the move target the area's spawn names (a 300
 * reach, 200 in area 5). */
extern "C" void em029_area_target_set(struct _ENEMY_WORK* self) {
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 5) {
        switch (self->area_no) {
        case 4:
            em_move_target_set(self, 0, 1, 300.0f);
            break;
        case 5:
            em_move_target_set(self, 0, 1, 200.0f);
            break;
        case 6:
            em_move_target_set(self, 0, 1, 300.0f);
            break;
        case 8:
            em_move_target_set(self, 0, 1, 300.0f);
            break;
        }
    }
}

/* 0x8039E68C (0x88): the program's init slot: clears the hit pair, raises status bit 0x80 for a large monster and,
 * for `mode` 2, arms the idle action. */
extern "C" void em029_init(struct _ENEMY_WORK* self, u8 mode) {
    self->em029_0x328.hit_0x328 = 0;
    self->em029_0x328.hit_prev_0x329 = 0;
    if ((self->field_0x1C8 & 8) != 0) {
        em_status_bits_set(self, 0x80);
    }
    if ((s32)mode == 2) {
        em_move_mode_set(self, 0);
        em_act_arm_unless_down(self, 12, 1);
        em_state_refresh(self);
    }
}

/* An empty stub the range holds (the runtime dump's name for it is a junk duplicate). */
void em_action_nop(void) {
}

/* 0x8039E718 (0xB4): the program's per-frame slot: shifts the hit pair, starts the attack on a live hit timer and,
 * for a large monster that is busy outside action 13, lets an em019 in its area take it over. */
extern "C" void em029_frame_pre(struct _ENEMY_WORK* self) {
    self->em029_0x328.hit_prev_0x329 = self->em029_0x328.hit_0x328;
    self->em029_0x328.hit_0x328 = 0;
    if (em_die_ck(self) == 0) {
        if (em_hit_timer_ck(self) == 1) {
            em_attack_start(self);
        }
        if ((self->field_0x1C8 & 8) != 0 && em_busy_ck(self) == 1 && em_act_ck(self, 13, 0) == 0 &&
            em_area_team_ck(self->area_no) == 1) {
            em_hit_by_set(self, 0, 0);
        }
    }
}

/* Action 0's per-frame step: arm the motion once, then finish the action when the motion ends. */
void em_action0_step(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 0's sub-state handler. */
void em_action0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action0_step(self);
        break;
    case 1:
        em_action0_step(self);
        break;
    case 2:
        em_action0_step(self);
        break;
    }
}

/* 0x8039E878 (0x88): action 1, sub-state 0: motion 13 at 1.5 speed. */
extern "C" void em_action1_sub0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 16, 0);
        em_mot_speed_set(self, 1.5f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039E900 (0x88): action 1, sub-state 1: motion 17 at 1.5 speed. */
extern "C" void em_action1_sub1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 17, 16, 0);
        em_mot_speed_set(self, 1.5f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039E988 (0x118): action 1, sub-state 2: the long roar (300 frames for `mode` 1, else 150) with the camera
 * request and the roar latch at frame 4, then motion 208. */
extern "C" void em_action1_sub2(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 207, 2, 0);
        if (mode == 1) {
            self->timer_0x020 = 300;
        } else {
            self->timer_0x020 = 150;
        }
        em_camera_req(self, -1, 7);
        break;
    case 1:
        if (em_frame_check(self, 1, 4.0f, 0.0f) == 1) {
            em_roar_latch_set(self);
        }
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 208, 6, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 3: the two-stage motion, the second stage starting when the first ends. */
void em_action1_sub3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 23, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 24, 20, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 4: the single-stage motion. */
void em_action1_sub4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 25, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039EBD0 (0xD8): action 1, sub-state 5: the busy two-stage motion (10 then 11), holding the hit flag until it
 * ends in state (1, 9). */
extern "C" void em_action1_sub5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 10, 0);
        em_busy_set(self);
        break;
    case 1:
        em_busy_set(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 11, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 9);
        } else {
            self->em029_0x328.hit_0x328 = 1;
        }
        break;
    }
}

/* Action 1's sub-state 6: the single-stage motion, with the busy flag left to sub-state 8. */
void em_action1_sub6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 29, 16, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039ED24 (0xCC): action 1, sub-state 7: motion 203 with the shake at frame 208 and the motion timer and effect
 * flag at frame 250; the attack ends with the motion. */
extern "C" void em_action1_sub7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 203, 0, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 208.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 1, 250.0f, 0.0f) == 1) {
            em_motion_timer_arm(self);
            em_fx_flag_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* Action 1's sub-state 8: the record stays busy for the whole step, which runs the short motion. */
void em_action1_sub8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state handlers. */
void em_action1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action1_sub0(self);
        break;
    case 1:
        em_action1_sub1(self);
        break;
    case 2:
        em_action1_sub2(self, 0);
        break;
    case 3:
        em_action1_sub3(self);
        break;
    case 4:
        em_action1_sub4(self);
        break;
    case 5:
        em_action1_sub5(self);
        break;
    case 6:
        em_action1_sub6(self);
        break;
    case 7:
        em_action1_sub7(self);
        break;
    case 8:
        em_action1_sub2(self, 1);
        break;
    case 9:
        em_action1_sub8(self);
        break;
    }
}

/* 0x8039EED0 (0xB8): action 2, sub-state 0: walks towards the target at `speed`. */
extern "C" void em_action2_sub0(struct _ENEMY_WORK* self, f32 speed) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_approach_start(self, speed, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039EF88 (0xE0): action 2, sub-state 1: runs towards the target (its reach capped at 400 for `mode` 1). */
extern "C" void em_action2_sub1(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, 1.5f);
        if ((s32)mode != 1) {
            em_approach_start(self, 0.0f, 0);
        } else {
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 400.0f) {
                self->value_0x378 = 400.0f;
            }
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The em029 band's two turn sequences `em_turn_seq_start`/`em_turn_seq_step` walk (`.rodata` 0x80570BA0 and
 * 0x80570BE0, 0x40 bytes each): a four-word header (motion, frames, blend, the turn's float or angle word) and three
 * 0x10-byte steps (motion id in the high halfword, frames, two zero words).  The words are kept as they stand; only the
 * walker gives them meaning. */
const u32 em029_turn_seq_a[16] = {
    0x0000002A, 0x000000AA, 0x0000001E, 0x40000000,
    0x00010000, 0x00000004, 0x00000000, 0x00000000,
    0x00050000, 0x00000004, 0x00000000, 0x00000000,
    0x00060000, 0x00000004, 0x00000000, 0x00000000,
};

const u32 em029_turn_seq_b[16] = {
    0x0000000E, 0x0000014C, 0x00000000, 0x2AAB0000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
};

/* 0x8039F068 (0xEC): action 2, sub-state 2: the first turn sequence (`mode` 0 or 1 picks its direction), at 1.5
 * speed. */
extern "C" void em_action2_sub2(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_turn_seq_start(self, (void*)em029_turn_seq_a, 0, 0, 0);
            break;
        case 1:
            em_turn_seq_start(self, (void*)em029_turn_seq_a, 1, 0, 0);
            break;
        }
        em_mot_speed_set(self, 1.5f);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em029_turn_seq_a) == 1) {
            em_action_finish(self);
        } else {
            em_mot_speed_set(self, 1.5f);
        }
        break;
    }
}

/* 0x8039F154 (0xB0): action 2, sub-state 3: motion 14, turning to the target between frames 14 and 332. */
extern "C" void em_action2_sub3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 14, 4, 0);
        em_mot_speed_set(self, 1.5f);
        break;
    case 1:
        if (em_frame_check(self, 3, 14.0f, 332.0f) == 1) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039F204 (0xC4): action 2, sub-state 4: the second turn sequence (`mode` 0 or 1 picks its direction). */
extern "C" void em_action2_sub4(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (mode == 0) {
            em_turn_seq_start(self, (void*)em029_turn_seq_b, 0, 0, 0);
        } else {
            em_turn_seq_start(self, (void*)em029_turn_seq_b, 1, 0, 0);
        }
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em029_turn_seq_b) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039F2C8 (0xB8): action 2, sub-state 5: runs towards the target at `speed`. */
extern "C" void em_action2_sub5(struct _ENEMY_WORK* self, f32 speed) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, 1.5f);
        em_approach_start(self, speed, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x180) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 2's sub-state handlers. */
void em_action2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action2_sub0(self, 0.0f);
        break;
    case 1:
        em_action2_sub1(self, 0);
        break;
    case 2:
        em_action2_sub2(self, 0);
        break;
    case 3:
        em_action2_sub3(self);
        break;
    case 4:
        em_action2_sub4(self, 0);
        break;
    case 5:
        em_action2_sub4(self, 1);
        break;
    case 6:
        em_action2_sub5(self, -400.0f);
        break;
    case 7:
        em_action2_sub1(self, 1);
        break;
    }
}

/* 0x8039F3E4 (0x234): action 3, sub-state 0: rears up (motion 200/201), charges at the target and ends in state 8
 * (`mode` 1) or 2 when the charge's hit lands or a mounted hunter's kind-40 actor is caught. */
extern "C" void em_action3_sub0(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 200, 2, 0);
        em_approach_start(self, mode == 1 ? 1200.0f : 800.0f, 0);
        break;
    case 1:
        em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 201, 2, 0);
            em_mot_speed_set(self, 0.8f);
            em_hit_window_set(self, 0, 1, 10);
            em_hit_window_set(self, 1, 4, 80);
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            em_hit_window_clear(self, 0);
            em_mot_set(self, 202, 6, 0);
            em_mot_speed_set(self, 0.8f);
        } else if (hit_mask_ck(reinterpret_cast<struct _HIT_W*>(self->hit_0xA64), 1) == 1) {
            if (mode == 1) {
                em_state_set(self, 1, 8);
            } else {
                em_state_set(self, 1, 2);
            }
        } else if (self->field_0xA16 != 0xFF && self->field_0xA38 == 1 && self->plw_0xA34 != NULL
                   && self->plw_0xA34->slot_active != 0 && self->plw_0xA34->field_0x003 == 40) {
            if (mode == 1) {
                em_state_set(self, 1, 8);
            } else {
                em_state_set(self, 1, 2);
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

/* 0x8039F618 (0x8C): action 3, sub-state 1: motion 19 with the default hit window. */
extern "C" void em_action3_sub1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 19, 4, 0);
        em_hit_window_set_default(self, 0, 2);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039F6A4 (0xB4): action 3, sub-state 2: motion 206, turning to the target between frames 28 and 68. */
extern "C" void em_action3_sub2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 206, 6, 0);
        em_hit_window_set_default(self, 0, 3);
        break;
    case 1:
        if (em_frame_check(self, 3, 28.0f, 68.0f) == 1) {
            em_turn_to_target(self, 0x200);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 3's sub-state handlers. */
void em_action3_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action3_sub0(self, 0);
        break;
    case 1:
        em_action3_sub1(self);
        break;
    case 2:
        em_action3_sub2(self);
        break;
    case 3:
        em_action3_sub0(self, 1);
        break;
    }
}

/* The sound-effect programs, defined after `em_action5_dispatch`, the last dispatcher that passes them. */
extern EmSeEntry em029_se_tbl_mot31[];
extern EmSeEntry em029_se_tbl_mot32[];
extern EmSeEntry em029_se_tbl_mot96[];
extern EmSeEntry em029_se_tbl_mot97[];
extern EmSeEntry em029_se_tbl_mot122[];
extern EmSeEntry em029_se_tbl_mot123[];
extern EmSeEntry em029_se_tbl_mot159[];
extern EmSeEntry em029_se_tbl_mot124[];
extern EmSeEntry em029_se_tbl_mot141[];
extern EmSeEntry em029_se_tbl_mot125[];
extern EmSeEntry em029_se_tbl_mot142[];
extern EmSeEntry em029_se_tbl_mot168[];
extern EmSeEntry em029_se_tbl_act5_1[];
extern EmSeEntry em029_se_tbl_act5_2[];

/* Action 4's sub-state handler: plays the motion's sound table, or finishes the action for any other motion. */
void em_action4_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 31:
        em_se_tbl_play(self, em029_se_tbl_mot31, 0, 31);
        break;
    case 32:
        em_se_tbl_play(self, em029_se_tbl_mot32, 0, 32);
        break;
    case 96:
        em_se_tbl_play(self, em029_se_tbl_mot96, 0, 96);
        break;
    case 97:
        em_se_tbl_play(self, em029_se_tbl_mot97, 0, 97);
        break;
    case 122:
        em_se_tbl_play(self, em029_se_tbl_mot122, 0, 122);
        break;
    case 123:
        em_se_tbl_play(self, em029_se_tbl_mot123, 0, 123);
        break;
    case 159:
        em_se_tbl_play(self, em029_se_tbl_mot159, 0, 159);
        break;
    case 160:
        em_se_tbl_play(self, em029_se_tbl_mot159, 0, 160);
        break;
    case 124:
        em_se_tbl_play(self, em029_se_tbl_mot124, 0, 124);
        break;
    case 141:
        em_se_tbl_play(self, em029_se_tbl_mot141, 0, 141);
        break;
    case 168:
        em_se_tbl_play(self, em029_se_tbl_mot168, 0, 168);
        break;
    case 125:
        em_se_tbl_play(self, em029_se_tbl_mot125, 0, 125);
        break;
    case 142:
        em_se_tbl_play(self, em029_se_tbl_mot142, 0, 142);
        break;
    case 221:
        em_se_tbl_play(self, em029_se_tbl_mot31, 0, 221);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* Action 5's sub-state handler: plays one of the two branching sound tables by motion. */
void em_action5_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        em_se_tbl_play_alt(self, em029_se_tbl_act5_1, 0, 1);
        break;
    case 2:
        em_se_tbl_play_alt(self, em029_se_tbl_act5_2, 0, 2);
        break;
    case 17:
        em_se_tbl_play_alt(self, em029_se_tbl_act5_1, 0, 17);
        break;
    case 18:
        em_se_tbl_play_alt(self, em029_se_tbl_act5_2, 0, 18);
        break;
    default:
        em_se_tbl_play_alt(self, em029_se_tbl_act5_1, 0, 1);
        break;
    }
}

/* The sound-effect programs `em_action4_dispatch`/`em_action5_dispatch` pass to the table walker. */
EmSeRecord em029_se_rec_mot31_a = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot31[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot31_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot32_a = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot32[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot32_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot96_a = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_mot96_b = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_mot96_c = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_mot96_d = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot96[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x00, {0, 0, 0}, &em029_se_rec_mot96_a},
    {0x00, {0, 0, 0}, &em029_se_rec_mot96_b},
    {0x00, {0, 0, 0}, &em029_se_rec_mot96_c},
    {0x00, {0, 0, 0}, &em029_se_rec_mot96_d},
    {0xFF, {0, 0, 0}, 0},
};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em029_se_rec_unused_0 = {100, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_mot97_a = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_mot97_b = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_mot97_c = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot97[4] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot97_a},
    {0x00, {0, 0, 0}, &em029_se_rec_mot97_b},
    {0x00, {0, 0, 0}, &em029_se_rec_mot97_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot122_a = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot122[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot122_a},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot123_a = {107, 0, 10, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot123[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot123_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot159_a = {105, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot159[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot159_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot124_a = {100, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_mot124_b = {108, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot124[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x18)},
    {0x00, {0, 0, 0}, &em029_se_rec_mot124_a},
    {0x00, {0, 0, 0}, &em029_se_rec_mot124_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot141_a = {100, 0, 6, 30, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot141[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot141_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot125_a = {26, 0, 8, 56, 0, {1, 8, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot125[4] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x98)},
    {0x02, {0, 0, 0}, &em029_se_rec_mot125_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot142_a = {26, 0, 20, 180, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot142[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot142_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em029_se_rec_mot168_a = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em029_se_tbl_mot168[2] = {
    {0x00, {0, 0, 0}, &em029_se_rec_mot168_a},
    {0xFF, {0, 0, 0}, 0},
};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em029_se_rec_unused_1 = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_a_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_e = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_b_a = {0, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_f = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_c_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_g = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_d_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_1_h = {104, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

extern EmSeBranch em029_se_branch_act5_1_a;
extern EmSeBranch em029_se_branch_act5_1_b;
extern EmSeBranch em029_se_branch_act5_1_c;
extern EmSeBranch em029_se_branch_act5_1_d;
EmSeEntry em029_se_tbl_act5_1[11] = {
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_1_a)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_1_b)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_1_c)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_1_d)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_1_e},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_1_f},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_1_g},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_1_h},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em029_se_tbl_act5_1_a[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_1_a_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_1[7])},
};

EmSeEntry em029_se_tbl_act5_1_b[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x01, {0, 0, 0}, &em029_se_rec_act5_1_b_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_1[6])},
};

EmSeEntry em029_se_tbl_act5_1_c[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_1_c_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_1[7])},
};

EmSeEntry em029_se_tbl_act5_1_d[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_1_d_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_1[8])},
};

EmSeBranch em029_se_branch_act5_1_a = {0, 102, em029_se_tbl_act5_1_a};

EmSeBranch em029_se_branch_act5_1_b = {0, 101, em029_se_tbl_act5_1_b};

EmSeBranch em029_se_branch_act5_1_c = {0, 106, em029_se_tbl_act5_1_c};

EmSeBranch em029_se_branch_act5_1_d = {0, 103, em029_se_tbl_act5_1_d};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em029_se_rec_unused_2 = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_a_a = {0, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_a_b = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_b_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_e = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_c_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_f = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_d_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em029_se_rec_act5_2_g = {104, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

extern EmSeBranch em029_se_branch_act5_2_a;
extern EmSeBranch em029_se_branch_act5_2_b;
extern EmSeBranch em029_se_branch_act5_2_c;
extern EmSeBranch em029_se_branch_act5_2_d;
EmSeEntry em029_se_tbl_act5_2[9] = {
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_2_a)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_2_b)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_2_c)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_branch_act5_2_d)},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_2_e},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_2_f},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_2_g},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em029_se_tbl_act5_2_a[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x01, {0, 0, 0}, &em029_se_rec_act5_2_a_a},
    {0x00, {0, 0, 0}, &em029_se_rec_act5_2_a_b},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_2[5])},
};

EmSeEntry em029_se_tbl_act5_2_b[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_2_b_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_2[5])},
};

EmSeEntry em029_se_tbl_act5_2_c[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_2_c_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_2[5])},
};

EmSeEntry em029_se_tbl_act5_2_d[2] = {
    {0x01, {0, 0, 0}, &em029_se_rec_act5_2_d_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em029_se_tbl_act5_2[6])},
};

EmSeBranch em029_se_branch_act5_2_a = {0, 101, em029_se_tbl_act5_2_a};

EmSeBranch em029_se_branch_act5_2_b = {0, 106, em029_se_tbl_act5_2_b};

EmSeBranch em029_se_branch_act5_2_c = {0, 102, em029_se_tbl_act5_2_c};

EmSeBranch em029_se_branch_act5_2_d = {0, 103, em029_se_tbl_act5_2_d};

/* 0x8039F9E4 (0xC4): action 6, sub-state 0: walks to the ground spot its spawn key names. */
extern "C" void em_action6_sub0(struct _ENEMY_WORK* self) {
    EmGroundRec ground;

    em_ground_rec_clear(&ground);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_approach_start(self, 0.0f, 0);
        if (em_ground_rec_find(self->field_0x01A, &ground) == 1) {
            copyVec3(&self->aim, &ground.pos_0x08);
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8039FAA8 (0xB4): action 6, sub-state 1: motion 204 with a shake at its start and at frame 54. */
extern "C" void em_action6_sub1(struct _ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 204, 0, 0);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 54.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 6's sub-state handlers. */
void em_action6_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action6_sub0(self);
        break;
    case 1:
        em_action6_sub1(self);
        break;
    }
}

/* Action 7's step: after the motion, hand the record on when its area already holds a team-19
 * enemy, otherwise finish the action. */
void em_action7_step(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 29, 16, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (em_area_team_ck(self->area_no) == 1) {
                em_state_set(self, 13, 0);
            } else {
                em_action_finish(self);
            }
        }
        break;
    }
}

/* Action 7's single step, run while the sub-state is still 0. */
void em_action7_dispatch(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em_action7_step(self);
    }
}

/* Runs the handler for the enemy's current action id (only the ids the range implements). */
void em_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em_action0_dispatch(self);
        break;
    case 1:
        em_action1_dispatch(self);
        break;
    case 2:
        em_action2_dispatch(self);
        break;
    case 7:
        em_action3_dispatch(self);
        break;
    case 10:
        em_action4_dispatch(self);
        break;
    case 11:
        em_action5_dispatch(self);
        break;
    case 12:
        em_action6_dispatch(self);
        break;
    case 13:
        em_action7_dispatch(self);
        break;
    }
}

/* 0x8039FC7C (0xD0): spawns effect `type` for joint `joint`: on the joint itself (`kind` 0) or at the joint's
 * position at the record's ground height in its area, scaled with the monster (`kind` 1). */
extern "C" void em029_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, s32 joint, s32 delta, f32 scale) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (kind) {
    case 0:
        eft009_spawn_at_joint(self, joint, type, delta, scale);
        break;
    case 1:
        get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, type, delta, scale);
        break;
    }
}

/* 0x8039FD4C (0x12E0): the program's motion-event slot: spawns each motion's effects on its key frames, one set while
 * the record is enraged (`field_0x228` bits 1-2) and another otherwise. */
extern "C" void em029_motion_events(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 shift;
    nw4r::math::VEC3 pos;

    VEC3_ctor(&shift);
    VEC3_ctor(&pos);
    switch (em_get_mot_no(self)) {
    case 2:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 11, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 7, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 11, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 7, 0, 1.0f);
            }
        }
        break;
    case 4:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 20, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 23, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 23, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 20, 0, 1.0f);
            }
        }
        break;
    case 7:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 538.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 20, 0, 0.6f, &shift);
            }
        } else {
            if (em_after_frame_check(self, 0, 538.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 36, 0, 1.2f, &shift);
            }
        }
        break;
    case 8:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 20, 0, 0.6f, &shift);
            }
        } else {
            if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 7, &pos);
                pos.x -= 60.0f;
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 7, self->field_0x1C0, 1.2f);
            }
        }
        break;
    case 19:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 25, 23, 0, 1.8f);
                em029_effect_spawn(self, 0, 25, 20, 0, 1.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 23, 0, 1.8f);
                em029_effect_spawn(self, 0, 9, 20, 0, 1.8f);
            }
        }
        break;
    case 27:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 96.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 20, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 23, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 96.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 23, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 20, 0, 1.0f);
            }
        }
        break;
    case 28:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 7, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 9, 7, 0, 1.2f);
            }
        }
        break;
    case 102:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 3, 0, 0.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.5f);
            }
        }
        break;
    case 103:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 43.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 88.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 23, 3, 0, 8.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 43.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 88.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.0f);
            }
        }
        break;
    case 106:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 3, 0, 0.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.5f);
            }
        }
        break;
    case 200:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.6f);
            }
        } else {
            if (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 1.0f);
            }
        }
        break;
    case 201:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 28.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 13, 24, 0, 0.7f);
                em029_effect_spawn(self, 0, 15, 24, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 28.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 24, 0, 0.8f);
            }
        }
        break;
    case 202:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 19, 13, 0, 0.7f);
                em029_effect_spawn(self, 0, 15, 13, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 4, 13, 0, 1.2f);
            }
        }
        break;
    case 203:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 13, 7, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 156.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 13, 11, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 200.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 260.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 0, 7, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 156.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 0, 11, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 200.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 1.1f);
            }
            if (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 260.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 12, 0, 1.3f);
            }
        }
        break;
    case 204:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 17, 3, 0, 0.4f);
            }
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 24, 11, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 24, 7, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 24, 20, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 24, 23, 0, 0.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 1, 4, 3, 0, 0.3f);
            }
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 11, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 7, 0, 0.6f);
            }
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 20, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 6, 23, 0, 0.8f);
            }
        }
        break;
    case 206:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 13, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 1, 2, 13, 0, 0.3f);
            }
        }
        break;
    case 207:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                em029_effect_spawn(self, 0, 20, 12, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 12, &pos);
                pos.z += 100.0f;
                pos.y = 5.0f + self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 6, self->field_0x1C0, 0.6f);
            }
        }
        break;
    }
}

/* 0x803A102C (0xDC): the program's condition slot: 0 the attack timer armed, 1 the special part's holder carrying a
 * torch (a player of the move work, or the AI companion), 2 the hit flag of last frame. */
extern "C" s32 em029_condition_ck(struct _ENEMY_WORK* self, u8 kind) {
    u8* holder;
    struct _AINPC_W* companion;

    switch (kind) {
    case 0:
        if (self->field_0x011 != 0) {
            return 1;
        }
        break;
    case 1:
        if (self->field_0x382 != 0xFF) {
            switch (self->field_0x380) {
            case 1:
                holder = em_move_work_pick(self->state_0x381, self->field_0x382);
                if (*holder != 0 && pl_torch_ck((struct _ENEMY_WORK*)holder) == 1) {
                    return 1;
                }
                break;
            case 2:
                companion = &ainpc_w;
                if (companion->active != 0 && ai_torch_ck(companion) == 1) {
                    return 1;
                }
                break;
            }
        }
        break;
    case 2:
        if (self->em029_0x328.hit_prev_0x329 != 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* A `return 0` stub the range holds (the runtime dump's name for it is a junk duplicate). */
s32 em_action_ret0(void) {
    return 0;
}

/* 0x803A1110 (0xC4): the program's setup slot: the idle move mode, motion speed 12, the reach flag (0 in the type-5
 * map's areas 4..6 and 8), the two motion parameters and the area's move target. */
extern "C" void em029_setup(struct _ENEMY_WORK* self, s8* speed, s8* far) {
    em_move_mode_set(self, 0);
    *speed = 12;
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 5) {
        if ((u32)(self->area_no - 4) <= 2 || (s32)self->area_no == 8) {
            *far = 0;
        } else {
            *far = 1;
        }
    } else {
        *far = 1;
    }
    em_motion_param_set(self, 0.0f, 0);
    em_motion_param_set(self, 1.0f, 30);
    em029_area_target_set(self);
}

/* Part 0 of the enemy's damage table is untouched. */
u32 em_parts_damage0_ck(struct _ENEMY_WORK* self, u8 part) {
    if (part == 0) {
        if ((em_parts_damage_level_get(self, 0) & 1) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803A1220 (0xB4): whether area `area` holds a live em019 (team 19) in the middle of its action 13. */
extern "C" u32 em_area_team_ck(u8 area) {
    struct _ENEMY_WORK* enemy;
    u16 max;
    s32 i;

    max = get_move_work_max(3);
    enemy = (struct _ENEMY_WORK*)get_move_work_adrs(3);

    for (i = 0; i < max; i++, enemy++) {
        if (enemy->active != 0 && enemy->area_no == area && (s32)enemy->team == 19 &&
            em019_action13_active_ck(enemy) == 1) {
            return 1;
        }
    }
    return 0;
}
