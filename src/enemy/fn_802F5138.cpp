/* enemy/fn_802F5138.cpp - enemy 032's program (the dump's `em032_prog_tbl` opens `.data`; the file rename to
 *   `enemy/em032_prog.cpp` is a filed unit-rename): the swimming enemy's action steps - the idle, swim, route,
 *   attack, sound and demo state machines over `state`/`state_sub`, their dispatchers, the per-frame watch for live
 *   attacks to dodge, the route tables and the static initializer.
 * RANGE. .text 0x802F5138-0x802F9994 (61 functions); .ctors 0x8056F390-0x8056F394 (the static initializer
 *   `em032_vec_pairs_init`), .rodata 0x80570700-0x805707C0 (three turn-sequence tables), .data 0x805D6D10-0x805D8210
 *   (the program table, the jump tables, the sound programs and the demo key tables), .bss 0x806BE078-0x806BE0C0
 *   (three point/step vector pairs), .sdata 0x80792880-0x807928D0 (the route tables), .sdata2 0x8079AA88-0x8079ABC0,
 *   extab, extabindex.  `enemy/em_sub_state_prog.cpp` continues the band at 0x802F9994; the static initializer ends
 *   the TU.
 * FLAGS. `cflags_main`; `#pragma fp_contract off` (retail keeps `fmuls` + `fadds`) and `#pragma peephole off` for
 *   the whole unit (retail keeps the unfused `clrlwi` + `cmpwi` pairs; no row moves with it on or off elsewhere).
 * NAMES. Every row is `em032_*` (the sound programs `em032_se_seq_<action>_<state_sub>[_<n>]` with their records
 *   `em032_se_rec_*`, the demo key tables `em032_demo<n>_*_keys`, `em032_se_kind0E_arg` - GUESSes from their readers): the program-table slots in the em006/em016 scheme (`em032_slots_clear` +0x00,
 *   `em032_init` +0x04, `em032_frame_update` +0x08, `em032_action_dispatch` +0x0C, `em032_action_remap` +0x10,
 *   `em032_event_notify` +0x14, `em032_motion_events` +0x18, `em032_state_byte_get` +0x24, `em032_event_request`
 *   +0x28, `em032_area_start_set` +0x30, `em032_noop` +0x34, `em032_retire_ck` +0x3C, `em032_rest_mode_ck` +0x48,
 *   `em032_action_select` +0x54, `em032_vacant_attack_clear` +0x64) and the dispatch tree (`em032_act<action>_*`);
 *   the step suffixes (`swim_forward`, `circle`, `leap`, `charge`, ...), `em032_half_hp_ck`, `em032_area_attacker_ck`,
 *   the route-table and vector-pair names are GUESSes from the bodies.  `eft013_spark_set_em` (0x801078BC,
 *   `ef/eft013_fx.cpp`) is a registry name.
 * RESIDUALS. Every row is written; 10 rows short of 100:
 *  - `em032_act0_idle`, `em032_act0_idle_swim`: retail tests both mode ranges before either motion block (a mixed
 *    unsigned/signed chain no if, else-if or switch spelling reproduces);
 *  - `em032_area_attacker_ck`, `em032_frame_update`, `em032_act5_leap`: register colouring only;
 *  - `em032_act5_circle`, `em032_act5_circle_wide`: retail builds the turn constant in r4 directly;
 *  - `em032_init`: retail keeps a dead `b` for its empty `case 3`;
 *  - `em032_act10_se_play`, `em032_act11_se_play`: block layout of the 3..4 mode range.
 *   Data: `.data` (the program table, the jump tables, the sound programs and the demo key tables), `.sdata`,
 *   `.bss` and the `.sdata2` pool are emitted in retail order; the pool stops 4 bytes short (the 8-byte alignment pad
 *   the split claims), `.rodata` differs only in two words dtk reads as `@etb_80008000` pointers (a filed
 *   block_relocations request), and `.ctors` is not emitted (the static initializer is a plain function until the
 *   unit is renamed and it can take MWCC's `__sinit_` name).
 * SHAPES. A byte compared with `cmpwi` is written `(s32)byte`; increments `x++`, decrements `--x`; a choice between two
 *   turn constants that feeds a call is a `(u16)` ternary; the static initializer reads the vector pairs through
 *   `extern` declarations and defines them after itself (so each is addressed on its own); `em032_frame_update`'s
 *   enemy-record search tests the searching record (`work`) with `em_work_die_ck`, as retail does.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "lobby/lb_event_request.h" /* lb_event_request (rule 2) */
#include "lobby/fn_802FA9A0.h"
#include "stage/stg_w.h"
#include "enemy/em_common.h"
#include "enemy/ENEMY_DATA.h"
#include "ef/eft_slot.h"
#include "ef/get_move_work_adrs.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 */
#include "enemy/em_motion_param_set.h"
#include "ef/eft013_spark_set_em.h"
#include "lobby/lb_quest_screen.h" /* quest_time_limit_get / quest_time_elapsed_get */
#include "enemy/em_shake_req_set.h"
#include "enemy/em_motion_kind_set.h"
#include "stage/shell.h" /* shell_pool */
#include "enemy/em_se_record.h" /* EmSeEntry / EmSeRecord */
#include "enemy/em_area_vacant_wait_ck.h"
#include "enemy/em_attack_finished_clear.h"
#include "ef/fn_800CDB2C.h" /* ran_suu */
#include "lobby/fn_8030121C.h" /* eft_em_spawn */
#include "ef/fn_8030681C.h" /* eft_spawn_type_at_area */
#pragma fp_contract off
#pragma peephole off

/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * four-word rows whose first word carries the motion in its high half. */
extern "C" const u32 em032_turn_seq_u_turn[16] = {
    0x00000000, 0x00000018, 0x00000000, 0x80008000,
    0x00030000, 0x00000006, 0x00000000, 0x00000000,
    0x00030000, 0x00000006, 0x00000000, 0x00000000,
    0x00030000, 0x00000006, 0x00000000, 0x00000000,
};

extern "C" const u32 em032_turn_seq_a[16] = {
    0x00000000, 0x000000A0, 0x00000000, 0x80008000,
    0x00020000, 0x00000006, 0x00000000, 0x00000000,
    0x00020000, 0x00000006, 0x00000000, 0x00000000,
    0x00020000, 0x00000006, 0x00000000, 0x00000000,
};

extern "C" const u32 em032_turn_seq_b[16] = {
    0x0000001A, 0x0000002E, 0x00000000, 0xFFFF2000,
    0x00020000, 0x00000006, 0x00000000, 0x00000000,
    0x00070000, 0x00000006, 0x00000000, 0x00000000,
    0x00070000, 0x00000006, 0x00000000, 0x00000000,
};

/* The sound programs `em032_act10_se_play`/`em032_act11_se_play` hand the sound player and the demo key tables
 * the demo steps apply (`.data`, defined after the functions in retail order). */
extern EmSeEntry em032_se_seq_10_0A[];
extern EmSeEntry em032_se_seq_10_0A_2[];
extern EmSeEntry em032_se_seq_10_0A_3[];
extern EmSeEntry em032_se_seq_10_89[];
extern EmSeEntry em032_se_seq_10_A1[];
extern EmSeEntry em032_se_seq_10_89_2[];
extern EmSeEntry em032_se_seq_10_A1_2[];
extern EmSeEntry em032_se_seq_10_8A[];
extern EmSeEntry em032_se_seq_10_8B[];
extern EmSeEntry em032_se_seq_10_8B_2[];
extern EmSeEntry em032_se_seq_10_AA[];
extern EmSeEntry em032_se_seq_10_AA_2[];
extern EmSeEntry em032_se_seq_10_8C[];
extern EmSeEntry em032_se_seq_11_00[];
extern EmSeEntry em032_se_seq_11_00_2[];
extern EmSeEntry em032_se_seq_11_33[];
extern EmSeEntry em032_se_seq_11_33_2[];
extern EmSeEntry em032_se_seq_11_33_3[];
extern EmSeEntry em032_se_seq_11_38[];
extern f32 em032_demo0_pos_keys[];
extern f32 em032_demo0_rot_keys[];
extern f32 em032_demo0_rot_keys_late[];
extern f32 em032_demo2_pos_keys[];
extern f32 em032_demo2_rot_keys[];

/* The band's functions, in address order. */
extern "C" u32 em032_half_hp_ck(_ENEMY_WORK* work);
extern "C" void em032_slots_clear(_ENEMY_WORK* work);
extern "C" void em032_init(_ENEMY_WORK* work, u32 arg);
extern "C" void em032_action_remap(_ENEMY_WORK* work, u8* action, u8* state_sub);
extern "C" void em032_event_notify(_ENEMY_WORK* work, u32 action, u32 state_sub);
extern "C" u32 em032_area_attacker_ck(u8 area);
extern "C" void em032_frame_update(_ENEMY_WORK* work);
extern "C" void em032_act0_idle(_ENEMY_WORK* work);
extern "C" void em032_act0_idle_swim(_ENEMY_WORK* work);
extern "C" void em032_act0_dispatch(_ENEMY_WORK* work);
extern "C" void em032_act5_swim_forward(_ENEMY_WORK* work, u8 kind, u8 alt);
extern "C" void em032_act5_swim_turn(_ENEMY_WORK* work, u8 kind, u8 alt);
extern "C" void em032_act5_turn_seq_a(_ENEMY_WORK* work);
extern "C" void em032_act5_turn_seq_b(_ENEMY_WORK* work);
extern "C" void em032_act5_drift(_ENEMY_WORK* work, u8 kind);
extern "C" void em032_swim_pace_set(_ENEMY_WORK* work, u8 pace);
extern "C" void em032_seat_pace_pick(_ENEMY_WORK* work);
extern "C" void em032_route_index_next(_ENEMY_WORK* work);
extern "C" void em032_route_index_nearest(_ENEMY_WORK* work);
extern "C" void em032_act5_seat_swim(_ENEMY_WORK* work);
extern "C" void em032_act5_dive(_ENEMY_WORK* work);
extern "C" void em032_act5_hover(_ENEMY_WORK* work, u8 kind);
extern "C" void em032_act5_retreat(_ENEMY_WORK* work, u8 kind);
extern "C" void em032_act5_circle(_ENEMY_WORK* work, u8 side, u8 alt);
extern "C" void em032_act5_rise_sink(_ENEMY_WORK* work, u8 mode);
extern "C" void em032_act5_leap(_ENEMY_WORK* work, u8 mode);
extern "C" void em032_act5_surface(_ENEMY_WORK* work);
extern "C" void em032_act5_turn(_ENEMY_WORK* work);
extern "C" void em032_act5_circle_wide(_ENEMY_WORK* work, u8 side, u8 alt);
extern "C" void em032_act5_u_turn(_ENEMY_WORK* work);
extern "C" void em032_act5_lunge(_ENEMY_WORK* work);
extern "C" void em032_act5_hold(_ENEMY_WORK* work);
extern "C" void em032_act5_approach_stop(_ENEMY_WORK* work, u8 alt);
extern "C" void em032_act5_shake_off(_ENEMY_WORK* work);
extern "C" void em032_act5_dispatch(_ENEMY_WORK* work);
extern "C" void em032_act7_bite_a(_ENEMY_WORK* work);
extern "C" void em032_act7_bite_b(_ENEMY_WORK* work);
extern "C" void em032_act7_ram(_ENEMY_WORK* work);
extern "C" void em032_act7_charge(_ENEMY_WORK* work, u8 mode);
extern "C" void em032_act7_dispatch(_ENEMY_WORK* work);
extern "C" void em032_act10_se_play(_ENEMY_WORK* work);
extern "C" void em032_act11_se_play(_ENEMY_WORK* work);
extern "C" u16 em032_motion_events(_ENEMY_WORK* work);
extern "C" u8 em032_state_byte_get(_ENEMY_WORK* work, u32 index);
extern "C" s32 em032_event_request(_ENEMY_WORK* work);
extern "C" void em032_area_start_set(_ENEMY_WORK* work, u8* action, u8* state_sub);
extern "C" void em032_act12_route(_ENEMY_WORK* work);
extern "C" void em032_act12_settle(_ENEMY_WORK* work);
extern "C" void em032_act12_dispatch(_ENEMY_WORK* work);
extern "C" void em032_act13_demo_0(_ENEMY_WORK* work);
extern "C" void em032_act13_demo_1(_ENEMY_WORK* work);
extern "C" void em032_act13_demo_2(_ENEMY_WORK* work);
extern "C" void em032_act13_demo_3(_ENEMY_WORK* work);
extern "C" void em032_act13_dispatch(_ENEMY_WORK* work);
extern "C" void em032_action_dispatch(_ENEMY_WORK* work);
extern "C" void em032_noop(void);
extern "C" s32 em032_retire_ck(_ENEMY_WORK* work);
extern "C" s32 em032_rest_mode_ck(_ENEMY_WORK* work);
extern "C" s32 em032_action_select(_ENEMY_WORK* work);
extern "C" void em032_vacant_attack_clear(_ENEMY_WORK* work);
extern "C" void em032_vec_pairs_init(void);

/* The em032 program table (`em032_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em006ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em032ProgTbl {
    /* +0x00 */ void (*slots_clear_0x00)(struct _ENEMY_WORK* work);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* work, u32 arg);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* work);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* work);
    /* +0x10 */ void (*action_remap_0x10)(struct _ENEMY_WORK* work, u8* action, u8* state_sub);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* work, u32 action, u32 state_sub);
    /* +0x18 */ u16 (*motion_events_0x18)(struct _ENEMY_WORK* work);
    /* +0x1C */ u32 pad_0x1C[2];
    /* +0x24 */ u8 (*state_byte_get_0x24)(struct _ENEMY_WORK* work, u32 index);
    /* +0x28 */ s32 (*event_request_0x28)(struct _ENEMY_WORK* work);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* work, u8* action, u8* state_sub);
    /* +0x34 */ void (*noop_0x34)(void);
    /* +0x38 */ u32 pad_0x38;
    /* +0x3C */ s32 (*retire_ck_0x3C)(struct _ENEMY_WORK* work);
    /* +0x40 */ u32 pad_0x40[2];
    /* +0x48 */ s32 (*rest_mode_ck_0x48)(struct _ENEMY_WORK* work);
    /* +0x4C */ u32 pad_0x4C[2];
    /* +0x54 */ s32 (*action_select_0x54)(struct _ENEMY_WORK* work);
    /* +0x58 */ u32 pad_0x58[3];
    /* +0x64 */ void (*vacant_attack_clear_0x64)(struct _ENEMY_WORK* work);
    /* +0x68 */ u32 pad_0x68;
};

extern "C" Em032ProgTbl em032_prog_tbl = {
    em032_slots_clear,
    em032_init,
    em032_frame_update,
    em032_action_dispatch,
    em032_action_remap,
    em032_event_notify,
    em032_motion_events,
    {0, 0},
    em032_state_byte_get,
    em032_event_request,
    0,
    em032_area_start_set,
    em032_noop,
    0,
    em032_retire_ck,
    {0, 0},
    em032_rest_mode_ck,
    {0, 0},
    em032_action_select,
    {0, 0, 0},
    em032_vacant_attack_clear,
    0,
};

/* The argument word of the kind-0x0E entry of `em032_se_seq_10_89_2` (`.sdata`). */
u8 em032_se_kind0E_arg[2] = {0x3F, 0x00};

/* Reports whether the record's own counters put it in the ratio state the band entry tests. */
extern "C" u32 em032_half_hp_ck(_ENEMY_WORK* work) {
    switch (work->field_0x00A) {
    case 0:
    case 5:
        if ((f32)(s32)work->field_0x7A0 / (f32)(s32)work->field_0x7A4 <= 0.5f) {
            return 1;
        }
        return 0;
    default:
        return em_mot_finished_ck_c1();
    }
}

/* Clears the record's effect-slot bytes. */
extern "C" void em032_slots_clear(_ENEMY_WORK* work) {
    work->uv_model_0x328.slot_0x328 = 0;
    work->uv_model_0x328.created_0x33E = 0;
    work->uv_model_0x328.armed_0x329 = 0;
}

/* Adjusts the record's blend weight by the mode's factor, then asks for the model when the record is
 * in one of the two idle modes. */
extern "C" void em032_init(_ENEMY_WORK* work, u32 arg) {
    switch (work->field_0x00A) {
    default:
        work->field_0x1CC *= 1.2f;
        break;
    case 1:
        work->field_0x1CC *= 0.7f;
        break;
    case 2:
        work->field_0x1CC *= 0.4f;
        break;
    case 3:
        break;
    case 4:
        work->field_0x1CC *= 0.3f;
        break;
    case 5:
        work->field_0x1CC *= 1.2f;
        break;
    case 6:
        work->field_0x1CC *= 0.4f;
        break;
    }
    if ((u8)arg == 0) {
        em_move_mode_set(work, 2);
        em_act_arm_unless_down(work, 0, 4);
        switch (work->field_0x00A) {
        case 0:
        case 5:
            em032_route_index_nearest(work);
            break;
        }
    }
}

/* 0x802F52F4 (0x170): remaps the requested sub-state to its alternative when the ratio test holds in an idle
 * mode and the requested action is 5. */
extern "C" void em032_action_remap(_ENEMY_WORK* work, u8* action, u8* state_sub) {
    if (em032_half_hp_ck(work) == 1) {
        if ((u32)work->field_0x00A <= 1 || (s32)work->field_0x00A == 5) {
            switch (*action) {
            case 5:
                switch (*state_sub) {
                case 0x0:
                    *state_sub = 0x20;
                    break;
                case 0x5:
                    *state_sub = 0x21;
                    break;
                case 0xC:
                    *state_sub = 0x22;
                    break;
                case 0xD:
                    *state_sub = 0x23;
                    break;
                case 0xE:
                    *state_sub = 0x24;
                    break;
                case 0x1:
                    *state_sub = 0x25;
                    break;
                case 0x4:
                    *state_sub = 0x26;
                    break;
                case 0x6:
                    *state_sub = 0x27;
                    break;
                case 0x9:
                    *state_sub = 0x28;
                    break;
                case 0xA:
                    *state_sub = 0x29;
                    break;
                case 0x12:
                    *state_sub = 0x2A;
                    break;
                case 0x14:
                    *state_sub = 0x2B;
                    break;
                case 0x1E:
                    *state_sub = 0x2C;
                    break;
                case 0x11:
                    *state_sub = 0x2D;
                    break;
                case 0x13:
                    *state_sub = 0x2E;
                    break;
                case 0x15:
                    *state_sub = 0x2F;
                    break;
                case 0x1B:
                    *state_sub = 0x30;
                    break;
                case 0x1F:
                    *state_sub = 0x31;
                    break;
                case 0x34:
                    *state_sub = 0x35;
                    break;
                }
            }
        }
    }
}

/* Arms the record's "model created" flag for the two idle modes the caller reports. */
extern "C" void em032_event_notify(_ENEMY_WORK* work, u32 action, u32 state_sub) {
    switch ((u8)action) {
    case 5: {
        u8 state = state_sub;

        if ((u32)(state - 7) <= 1) {
            work->uv_model_0x328.created_0x33E = 1;
        }
        break;
    }
    }
}

/* 0x802F548C (0xAC): reports whether a live enemy record with the attack flag set stands in the given area. */
extern "C" u32 em032_area_attacker_ck(u8 area) {
    u16 max;
    s32 i;
    _ENEMY_WORK* rec;

    rec = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if ((u32)em_work_die_ck(rec) != 1 && (rec->field_0x1C8 & 1) != 0 && rec->area_no == area) {
            return 1;
        }
        rec++;
    }
    return 0;
}

/* 0x802F5538 (0x660): the per-frame watch: starts the quest-time attacks, and in the alert modes looks for a
 * live attack aimed at the record (a player's, another enemy's or a shell's) to dodge from. */
extern "C" void em032_frame_update(_ENEMY_WORK* work) {
    f32 reach;
    s32 found = 0;
    s32 remain = quest_time_limit_get();

    remain -= quest_time_elapsed_get();
    em_shake_req_set(work);
    if (em_die_ck(work) == 0) {
        switch (work->field_0x010) {
        case 2:
            if (remain > 21600) {
                em_attack_start(work);
                em_hit_by_set(work, ran_suu(0) & 0x3F, 1);
            }
            break;
        case 3:
            if (remain > 39600) {
                em_attack_start(work);
                em_hit_by_set(work, ran_suu(0) & 0x3F, 1);
            }
            break;
        case 4:
            if (remain > 57600) {
                em_attack_start(work);
                em_hit_by_set(work, ran_suu(0) & 0x3F, 1);
            }
            break;
        }
    }
    if (em_die_ck(work) == 0) {
        switch (work->field_0x00A) {
        case 2:
        case 6:
            switch (work->field_0x43B) {
            case 0: {
                u16 max;
                EmAreaWork* pl;
                u16 i;
                _ENEMY_WORK* em;
                _SHELL_W* shell;

                max = get_move_work_max(2);
                pl = (EmAreaWork*)get_move_work_adrs(2);
                for (i = 0; i < max && found == 0; i++, pl++) {
                    if ((s32)pl->active != 0 && em_pl_slot_leaving_ck(pl->field_0x008) == 0 &&
                        work->area_no == pl->area_no && calcVecDistXZ(&work->pos, &pl->vec_0x3C) <= 200.0f &&
                        pl->vec_0x3C.y <= work->pos.y + 100.0f && pl->vec_0x3C.y >= work->pos.y - 100.0f &&
                        (u16)((u16)(work->field_0x1C0 - calcVecAng2(&work->pos, &pl->vec_0x3C)) + 0xB1C6) > 0x638C) {
                        u16 j;

                        for (j = 0; j < 2 && found == 0; j++) {
                            if (pl->hit_slots_0x484[j].power_0x3C > 0.0f && pl->hit_slots_0x484[j].active_0x05 == 1) {
                                found = 1;
                                break;
                            }
                        }
                    }
                }
                max = get_move_work_max(3);
                em = (_ENEMY_WORK*)get_move_work_adrs(3);
                for (i = 0; i < max && found == 0; i++, em++) {
                    if ((u32)em_work_die_ck(work) != 1 && work->area_no == em->area_no && work != em) {
                        if ((em->team == 0x20 && em->field_0x00A == 2) || (em->team == 0x20 && em->field_0x00A == 6)) {
                            if (em->field_0x43B == 2 && vec3_distance(&work->pos, &em->pos) <= 500.0f) {
                                found = 1;
                                break;
                            }
                        } else {
                            switch (em_data_kind_get(work)) {
                            case 0:
                            case 1:
                            case 3:
                                reach = 100.0f;
                                break;
                            case 4:
                                reach = 200.0f;
                                break;
                            case 7:
                                reach = 500.0f;
                                break;
                            case 8:
                            case 9:
                                reach = 1000.0f;
                                break;
                            }
                            if (calcVecDistXZ(&work->pos, &em->pos) <= reach &&
                                em->pos.y <= work->pos.y + 0.5f * reach && em->pos.y >= work->pos.y - 0.5f * reach &&
                                (u16)((u16)(work->field_0x1C0 - calcVecAng2(&work->pos, &em->pos)) + 0xB1C6) > 0x638C) {
                                u16 j;

                                for (j = 0; j < 2 && found == 0; j++) {
                                    if (0.0f == em->hit_slots_0xA08[j].value_0x38 &&
                                        em->hit_slots_0xA08[j].power_0x3C > 0.0f &&
                                        em->hit_slots_0xA08[j].active_0x05 == 1) {
                                        found = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                max = shell_pool.work_max_0x04;
                shell = shell_pool.work_0x08;
                for (i = 0; i < max && found == 0; i++, shell++) {
                    if ((s32)shell->active_0x00 != 0 && work->area_no == shell->area_0x08 &&
                        vec3_distance(&work->pos, &shell->pos_0x18) <= 200.0f &&
                        ((EmHitSlot*)shell->hit_0x5C)->power_0x3C > 0.0f &&
                        ((EmHitSlot*)shell->hit_0x5C)->active_0x05 == 1) {
                        found = 1;
                        break;
                    }
                }
                if ((u32)found == 1) {
                    em_motion_kind_set(work, 2, 0);
                    em_hit_by_set(work, 0, 0);
                }
                break;
            }
            case 2:
                if (work->field_0x440 > 150) {
                    em_motion_kind_set(work, 3, 0);
                }
                break;
            case 3:
                if (work->field_0x440 > 300) {
                    em_motion_kind_set(work, 0, 0);
                }
                break;
            }
            break;
        case 1:
            if (em032_area_attacker_ck(work->area_no) == 1) {
                em_attack_start(work);
                em_hit_by_set(work, ran_suu(0) & 0x3F, 1);
            }
            break;
        case 0:
        case 5:
            if (work->uv_model_0x328.created_0x33E == 1 && (s32)em032_half_hp_ck(work) == 0) {
                work->uv_model_0x328.created_0x33E = 0;
            }
            if (work->uv_model_0x328.armed_0x329 == 1 && (s32)work->field_0x38F != 0) {
                work->uv_model_0x328.armed_0x329 = 0;
            }
            break;
        }
    }
}

/* First step of the "idle" action: advances the state byte, starts the motion and asks for the
 * model, then retires one state later. */
extern "C" void em032_act0_idle(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 0);
        if (work->field_0x00A > 2 && (u32)(work->field_0x00A - 5) > 1) {
            if ((s32)work->field_0x00A >= 3 && (s32)work->field_0x00A <= 4) {
                em_mot_set_ck(work, 10, 10, 0);
            }
        } else {
            em_mot_set_ck(work, 1, 20, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* Second step of the "idle" action: the same shape as the first, with the second motion set and the
 * band's own retire helper. */
extern "C" void em032_act0_idle_swim(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if (work->field_0x00A > 2 && (u32)(work->field_0x00A - 5) > 1) {
            if ((s32)work->field_0x00A >= 3 && (s32)work->field_0x00A <= 4) {
                em_mot_set_ck(work, 10, 10, 0);
            }
        } else {
            em_mot_set_ck(work, 1, 20, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* Sub-state dispatcher of the "idle" action. */
extern "C" void em032_act0_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em032_act0_idle(work);
        break;
    case 1:
        em032_act0_idle(work);
        break;
    case 2:
        em032_act0_idle(work);
        break;
    case 4:
        em032_act0_idle_swim(work);
        break;
    }
}

/* 0x802F5D60 (0x240): the swim-forward step: starts the motion and the approach for the given kind, then swims
 * until the approach ends and coasts out. */
extern "C" void em032_act5_swim_forward(_ENEMY_WORK* work, u8 kind, u8 alt) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if (alt == 0) {
            em_mot_set_ck(work, 1, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        switch (kind) {
        case 0:
            em_approach_start(work, 0.0f, 2);
            break;
        case 1:
            em_approach_start(work, 0.0f, 2);
            break;
        case 2:
            em_approach_start(work, -500.0f, 2);
            break;
        case 3:
            em_approach_start(work, -950.0f, 2);
            break;
        case 4:
            em_approach_start(work, -500.0f, 2);
            break;
        }
        em_move_vec2_clr(work);
        if ((s32)work->field_0x00A != 2 && (s32)work->field_0x00A != 6) {
            work->offset_0x30C.vec_0x310.z = 5.0f;
            work->vec_0x31C.z = -0.375f;
        } else {
            work->offset_0x30C.vec_0x310.z = 10.0f;
            work->vec_0x31C.z = -0.75f;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1: {
        s32 step;

        if ((s32)work->field_0x00A == 0 || (s32)work->field_0x00A == 5) {
            step = 0x5B;
        } else {
            step = 0x100;
        }
        if (em_approach_step(work, 0, step) == 1) {
            switch (kind) {
            case 0:
            case 2:
                work->state++;
                break;
            case 1:
            case 3:
            case 4:
                em_action_finish_swim(work);
                break;
            }
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    }
    case 2:
        if (work->offset_0x30C.vec_0x310.z > 0.0f) {
            em_move_offset_step(work, &work->field_0x1BC);
        } else {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F5FA0 (0x27C): the swim-turn step: the same shape as the swim-forward step with its own approach
 * offsets, and the busy/frame flags raised for kind 5. */
extern "C" void em032_act5_swim_turn(_ENEMY_WORK* work, u8 kind, u8 alt) {
    s32 step = 0x180;

    if ((s32)kind == 5) {
        em_busy_set(work);
        em_frame_flag_set(work);
        step = 0x300;
    }
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if (alt == 0) {
            em_mot_set_ck(work, 3, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        switch (kind) {
        case 0:
            em_approach_start(work, 0.0f, 2);
            break;
        case 1:
            em_approach_start(work, 0.0f, 2);
            break;
        case 2:
            em_approach_start(work, -950.0f, 2);
            break;
        case 3:
            em_approach_start(work, -500.0f, 2);
            break;
        case 4:
            em_approach_start(work, -950.0f, 2);
            break;
        case 5:
            em_approach_start(work, 0.0f, 2);
            break;
        case 6:
            em_approach_start(work, -400.0f, 2);
            break;
        case 7:
            em_approach_start(work, -400.0f, 2);
            break;
        }
        em_move_vec2_clr(work);
        switch (work->field_0x00A) {
        default:
            work->offset_0x30C.vec_0x310.z = 10.0f;
            work->vec_0x31C.z = -0.75f;
            break;
        case 1:
            work->offset_0x30C.vec_0x310.z = 10.0f;
            work->vec_0x31C.z = -0.75f;
            break;
        case 2:
        case 6:
            work->offset_0x30C.vec_0x310.z = 20.0f;
            work->vec_0x31C.z = -1.5f;
            break;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1:
        if (em_approach_step(work, 0, step) == 1) {
            if (kind == 0) {
                work->state++;
            } else {
                em_action_finish_swim(work);
            }
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 2:
        if (work->offset_0x30C.vec_0x310.z > 0.0f) {
            em_move_offset_step(work, &work->field_0x1BC);
        } else {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* First step of the walk-into-place action: keeps the motion set and the model record alive one
 * state longer. */
extern "C" void em032_act5_turn_seq_a(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_turn_seq_start(work, (void*)em032_turn_seq_a, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(work, (void*)em032_turn_seq_a) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* The second walk-into-place action, on the second model record. */
extern "C" void em032_act5_turn_seq_b(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_turn_seq_start(work, (void*)em032_turn_seq_b, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(work, (void*)em032_turn_seq_b) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F6334 (0x120): the drift step: swims on for 150 frames, then coasts out (kind 0) or ends (kind 1). */
extern "C" void em032_act5_drift(_ENEMY_WORK* work, u8 kind) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set_ck(work, 5, 20, 0);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 10.0f;
        work->vec_0x31C.z = -0.75f;
        work->timer_0x020 = 150;
        break;
    case 1:
        if (--work->timer_0x020 < 0) {
            switch (kind) {
            case 0:
                work->state++;
                break;
            case 1:
                em_action_finish_swim(work);
                break;
            }
        } else {
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    case 2:
        if (work->offset_0x30C.vec_0x310.z > 0.0f) {
            em_move_offset_step(work, &work->field_0x1BC);
        } else {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F6454 (0x10C): starts the swim motion at the speed and the forward drive of the given pace. */
extern "C" void em032_swim_pace_set(_ENEMY_WORK* work, u8 pace) {
    switch (pace) {
    case 0:
        em_mot_set_ck(work, 3, 6, 0);
        em_mot_speed_set(work, 1.0f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 10.0f;
        break;
    case 1:
        em_mot_set_ck(work, 3, 6, 0);
        em_mot_speed_set(work, 1.5f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 16.0f;
        break;
    case 2:
        em_mot_set_ck(work, 3, 6, 0);
        em_mot_speed_set(work, 1.0f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 8.0f;
        break;
    case 3:
        em_mot_set_ck(work, 3, 6, 0);
        em_mot_speed_set(work, 1.5f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 20.0f;
        break;
    }
}

/* 0x802F6560 (0x10C): picks the swim pace from the seat entry's distance and bearing: near and ahead is pace 1,
 * far and ahead 3, near behind 0, far behind 2. */
extern "C" void em032_seat_pace_pick(_ENEMY_WORK* work) {
    _ENEMY_DATA* data =
        (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(work->team, work->field_0x00A), work->field_0x46C);
    f32 dist;
    u32 angle;

    if (data != NULL) {
        dist = calcVecDistXZ(&work->pos, &data->vec_0x24);
        angle = (u16)(calcVecAng2(&work->pos, &data->vec_0x24) - work->field_0x1C0);
        if (angle >= 0x8000) {
            angle = 0x10000 - angle;
        }
        if (angle < 0x4000) {
            if (dist < 800.0f) {
                em032_swim_pace_set(work, 1);
            } else {
                em032_swim_pace_set(work, 3);
            }
        } else if (dist < 800.0f) {
            em032_swim_pace_set(work, 0);
        } else {
            em032_swim_pace_set(work, 2);
        }
    } else {
        em032_swim_pace_set(work, 0);
    }
}

/* 0x802F666C (0x214): advances the route index modulo the route length of the area the record swims in, or
 * resets it where the area has no route. */
extern "C" void em032_route_index_next(_ENEMY_WORK* work) {
    u32 count = 0;
    u8 kind = stage_map_kind_get(work->field_0x1E0);

    switch (work->field_0x00A) {
    case 2:
        if ((s32)kind == 1) {
            switch (work->area_no) {
            case 7:
                count = 4;
                break;
            case 12:
                count = 4;
                break;
            }
        }
        break;
    case 3:
        if ((s32)kind == 1) {
            switch (work->area_no) {
            case 7:
                count = 4;
                break;
            case 12:
                count = 4;
                break;
            }
        }
        break;
    case 6:
        if ((s32)kind == 3) {
            switch (work->area_no) {
            case 2:
                count = 5;
                break;
            case 4:
                count = 4;
                break;
            case 6:
                count = 4;
                break;
            }
        }
        break;
    case 0:
    case 5:
        switch (kind) {
        case 1:
            if (work->field_0x010 == 0) {
                switch (work->area_no) {
                case 7:
                    count = 4;
                    break;
                case 12:
                    count = 4;
                    break;
                }
            } else {
                switch (work->area_no) {
                case 7:
                    count = 4;
                    break;
                case 12:
                    count = 3;
                    break;
                }
            }
            break;
        case 3:
            switch (work->area_no) {
            case 2:
                count = 3;
                break;
            case 3:
                count = 3;
                break;
            case 4:
                count = 3;
                break;
            case 6:
                count = 4;
                break;
            case 8:
                count = 3;
                break;
            }
            break;
        }
        break;
    }
    if (count != 0) {
        work->uv_model_0x328.slot_0x328++;
        if (work->uv_model_0x328.slot_0x328 >= count) {
            work->uv_model_0x328.slot_0x328 = 0;
        }
    } else {
        work->uv_model_0x328.slot_0x328 = 0;
    }
}

/* 0x802F6880 (0x1D4): sets the route index to the route point of the record's area nearest to it. */
extern "C" void em032_route_index_nearest(_ENEMY_WORK* work) {
    s32 count = 0;
    u8 point = 0;
    f32 best = 0.0f;
    nw4r::math::VEC3 pos;
    s32 i;
    u8 nearest;
    u8 kind;
    f32 dist;

    VEC3_ctor(&pos);
    nearest = 0;
    kind = stage_map_kind_get(work->field_0x1E0);
    if ((s32)work->field_0x00A == 0 || (s32)work->field_0x00A == 5) {
        switch (kind) {
        case 1:
            if (work->field_0x010 == 0) {
                switch (work->area_no) {
                case 7:
                    count = 3;
                    point = 9;
                    break;
                case 12:
                    count = 4;
                    point = 10;
                    break;
                }
            } else {
                switch (work->area_no) {
                case 7:
                    count = 4;
                    point = 17;
                    break;
                case 12:
                    count = 3;
                    point = 14;
                    break;
                }
            }
            break;
        case 3:
            switch (work->area_no) {
            case 2:
                count = 3;
                point = 7;
                break;
            case 3:
                count = 3;
                point = 2;
                break;
            case 4:
                count = 3;
                point = 7;
                break;
            case 6:
                count = 4;
                point = 10;
                break;
            case 8:
                count = 3;
                point = 2;
                break;
            }
            break;
        }
    }
    for (i = 0; i < count; i++) {
        em_data_point_get(work, (u16)(((work->area_no & 0xF) << 8) | point), &pos);
        dist = vec3_distance(&work->pos, &pos);
        if (i == 0) {
            best = dist;
            nearest = i;
        } else if (best > dist) {
            best = dist;
            nearest = i;
        }
        point++;
    }
    work->uv_model_0x328.slot_0x328 = nearest;
}

/* 0x802F6A54 (0x160): swims to the seat entry: the pace is chosen once, then the approach runs until it ends and
 * the route index advances. */
extern "C" void em032_act5_seat_swim(_ENEMY_WORK* work) {
    _ENEMY_DATA* data =
        (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(work->team, work->field_0x00A), work->field_0x46C);

    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if (data != NULL) {
            if (work == data->work_0x18) {
                if ((work->bits_0x1EC & 1) == 0) {
                    em032_swim_pace_set(work, 0);
                } else {
                    em032_swim_pace_set(work, 1);
                }
            } else {
                em032_seat_pace_pick(work);
            }
        } else {
            em032_swim_pace_set(work, 0);
        }
        em_approach_start(work, 0.0f, 2);
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1:
        if (data != NULL && work != data->work_0x18 && em_mot_end_ck(work) == 1) {
            em032_seat_pace_pick(work);
        }
        if (em_approach_step(work, 0, 0x100) == 1) {
            em032_route_index_next(work);
            em_action_finish_swim(work);
        } else {
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    }
}

/* 0x802F6BB4 (0x10C): the dive step: offsets the target height by the record's bits and swims to it. */
extern "C" void em032_act5_dive(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set_ck(work, 3, 6, 0);
        em_mot_speed_set(work, 1.5f);
        {
            f32 off = 100.0f * (f32)((work->bits_0x1EC & 0xF) - 7);
            work->vec_0x36C.y += off;
        }
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 20.0f;
        work->vec_0x31C.z = -1.5f;
        break;
    case 1:
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        if (em_approach_step(work, 0, 0x280) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F6CC0 (0x15C): the hover step: kind 0 drifts for 150 frames; kind 1 keeps the motion to its end, or
 * clears the armed flag for one in five records. */
extern "C" void em032_act5_hover(_ENEMY_WORK* work, u8 kind) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set_ck(work, 1, 20, 0);
        work->timer_0x020 = 0;
        em_move_vec2_clr(work);
        if ((s32)kind == 0) {
            work->offset_0x30C.vec_0x310.z = 3.0f;
        } else if ((s32)(work->bits_0x1EC % 100) < 20) {
            work->uv_model_0x328.armed_0x329 = 0;
        } else {
            work->offset_0x30C.vec_0x310.z = 2.0f;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1:
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        if (kind == 1 && (s32)work->uv_model_0x328.armed_0x329 == 0) {
            em_action_finish_swim(work);
        } else if ((s32)kind == 0) {
            if (++work->timer_0x020 >= 150) {
                em_action_finish_swim(work);
            }
        } else if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F6E1C (0x1E4): the retreat step: sets the target 1000 units back along the line from the target to the
 * record and swims there for at most 300 frames. */
extern "C" void em032_act5_retreat(_ENEMY_WORK* work, u8 kind) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 back;

    em_hit_arm_set(work);
    VEC3_ctor(&dir);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if ((s32)kind == 0) {
            em_mot_set_ck(work, 1, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        subVec3(&diff, &work->vec_0x36C, &work->pos);
        copyVec3(&dir, &diff);
        dir.y = 0.0f;
        if (vec3_length_sq(&dir.x) > 0.001f) {
            vec3_normalize_into(&dir, &dir);
        } else {
            setVector3(&dir, 0.0f, 0.0f, 1.0f);
        }
        vec3_scale(&back, &dir, 1000.0f);
        subVec3(&target, &work->pos, &back);
        copyVec3(&work->vec_0x36C, &target);
        work->field_0x380 = 6;
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        if ((s32)work->field_0x00A != 2 && (s32)work->field_0x00A != 6) {
            work->offset_0x30C.vec_0x310.z = 5.0f;
            work->vec_0x31C.z = -0.375f;
        } else {
            work->offset_0x30C.vec_0x310.z = 10.0f;
            work->vec_0x31C.z = -0.75f;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        work->timer_0x020 = 300;
        break;
    case 1:
        if (--work->timer_0x020 < 0) {
            em_action_finish_swim(work);
        }
        if (em_approach_step(work, 0, 0x100) == 1) {
            em_action_finish_swim(work);
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    }
}

/* 0x802F7000 (0x148): the circling step: swims around the target, turning a quarter left or right whenever the
 * approach ends, for 300 frames. */
extern "C" void em032_act5_circle(_ENEMY_WORK* work, u8 side, u8 alt) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if ((s32)alt == 0) {
            em_mot_set_ck(work, 1, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 15.0f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        work->timer_0x020 = 300;
        break;
    case 1: {
        if (em_approach_step(work, (u16)((s32)side == 0 ? 0xC000 : 0x4000), 200) == 1) {
            em_approach_start(work, 0.0f, 2);
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        if (--work->timer_0x020 <= 0) {
            em_action_finish_swim(work);
        }
        break;
    }
    }
}

/* 0x802F7148 (0x194): the rise/sink step: swims to the target while the frame windows follow its height. */
extern "C" void em032_act5_rise_sink(_ENEMY_WORK* work, u8 mode) {
    f32 dy;

    if (mode == 1) {
        em_busy_set(work);
        em_frame_flag_set(work);
    }
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set_ck(work, 11, 20, 0);
        em_move_vec2_clr(work);
        if (work->field_0x00A == 3) {
            work->offset_0x30C.vec_0x310.z = 5.0f;
        } else {
            work->offset_0x30C.vec_0x310.z = 2.0f;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        em_approach_start(work, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(work, 0, 0x80) == 1) {
            if ((s32)mode == 0 && work->field_0x00A == 3) {
                em032_route_index_next(work);
            }
            em_action_finish_swim(work);
        } else {
            dy = work->pos.y - work->vec_0x36C.y;
            if (dy > 0.0f) {
                if (em_frame_check(work, 3, 50.0f, 100.0f) != 0) {
                    em_height_follow(work, 5.0f);
                }
            } else if (dy < 0.0f && em_frame_check(work, 3, 110.0f, 160.0f) != 0) {
                em_height_follow(work, 5.0f);
            }
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    }
}

/* 0x802F72DC (0x264): the leap step: mode 1 aims the climb along the line to the target tilted by 22.5
 * degrees; otherwise it climbs or sinks at a fixed rate until the motion frame or the climb runs out. */
extern "C" void em032_act5_leap(_ENEMY_WORK* work, u8 mode) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 sum;
    nw4r::math::VEC3 scaled;
    f32 dist;
    f32 frames;
    f32 tilt;

    VEC3_ctor(&dir);
    frames = 57.5f;
    tilt = -22.5f;
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 10, 20, 0);
        em_mot_speed_set(work, 2.0f);
        em_approach_start(work, 0.0f, 0);
        if ((s32)mode == 1) {
            dist = calcVecDistXZ(&work->pos, &work->vec_0x36C);
            subVec3(&diff, &work->pos, &work->vec_0x36C);
            copyVec3(&dir, &diff);
            dir.y = 0.0f;
            if (vec3_length_sq(&dir.x) > 0.001f) {
                vec3_normalize_into(&dir, &dir);
            } else {
                setVector3(&dir, 0.0f, 0.0f, 1.0f);
            }
            rotVecX(&dir, (u16)(s32)(0.5f + (65536.0f * tilt) / 360.0f));
            if (dir.y > 0.0f) {
                dir.y = -dir.y;
            }
            vec3_scale(&scaled, &dir, dist);
            addVec3(&sum, &work->vec_0x36C, &scaled);
            copyVec3(&dir, &sum);
            work->action_0x328.vec_0x328.y = (dir.y - work->pos.y) / frames;
        } else if (work->pos.y <= work->vec_0x36C.y) {
            work->action_0x328.vec_0x328.y = -3.0f;
        } else {
            work->action_0x328.vec_0x328.y = -5.0f;
        }
        break;
    case 1:
        if (em_mot_frame_get(work) >= 230.0f || work->action_0x328.vec_0x328.y >= 0.0f) {
            em_action_finish_swim(work);
        } else {
            em_approach_step(work, 0, 0x100);
            work->pos.y += work->action_0x328.vec_0x328.y;
        }
        break;
    }
}

/* 0x802F7540 (0x12C): the surfacing step: rises fast or slowly by where the target is, to the motion's end. */
extern "C" void em032_act5_surface(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 11, 20, 100);
        em_mot_speed_set(work, 1.5f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 7.0f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        em_approach_start(work, 0.0f, 0);
        if (work->pos.y <= work->vec_0x36C.y) {
            work->state_0x006 = 0;
        } else {
            work->state_0x006 = 1;
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        } else {
            em_approach_step(work, 0, 0x80);
            if ((s32)work->state_0x006 == 0) {
                work->pos.y += 10.0f;
            } else {
                work->pos.y += 5.0f;
            }
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    }
}

/* 0x802F766C (0x80): the turn-in-place step: turns to the target until it faces it. */
extern "C" void em032_act5_turn(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 10, 20, 0);
        break;
    case 1:
        if (em_turn_to_target(work, 0xB6) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F76EC (0x16C): the wide circling step: swims around the target until it is within 700 units, for at
 * most 900 frames. */
extern "C" void em032_act5_circle_wide(_ENEMY_WORK* work, u8 side, u8 alt) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if ((s32)alt == 0) {
            em_mot_set_ck(work, 3, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 25.0f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        work->timer_0x020 = 0;
        work->em003_0x328.tick = 0;
        break;
    case 1: {
        if (em_approach_step(work, (u16)((s32)side == 0 ? 0xD1C8 : 0x2E39), 0x2D8) == 1) {
            em_approach_start(work, 0.0f, 2);
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        if (calcDistanceSqXZ(&work->pos, &work->vec_0x36C) < 490000.0f) {
            em_action_finish_swim(work);
        }
        if (++work->timer_0x020 >= 900) {
            em_action_finish_swim(work);
        }
        break;
    }
    }
}

/* 0x802F7858 (0xB4): the U-turn step: runs the band's first turn sequence while swimming on. */
extern "C" void em032_act5_u_turn(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_turn_seq_start(work, (void*)em032_turn_seq_u_turn, 0, 1, 0);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 20.0f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1:
        if (em_turn_seq_step(work, (void*)em032_turn_seq_u_turn) == 1) {
            em_action_finish_swim(work);
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    }
}

/* 0x802F790C (0xF4): the lunge step: lunges forward (scaled by the record's size in mode 4) for 32 frames, then
 * reports the attack done. */
extern "C" void em032_act5_lunge(_ENEMY_WORK* work) {
    em_frame_flag_set(work);
    em_busy_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 3, 6, 0);
        em_move_vec_clr(work);
        if (work->field_0x00A == 4) {
            setVector3(&work->offset_0x30C.vec_0x310, 0.0f, 0.0f, 20.0f);
            vec3_scale_in_place(&work->offset_0x30C.vec_0x310, get_em_chg_scale(work));
        }
        work->timer_0x020 = 32;
        em_motion_param_set(work, 0.0f, 30);
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            em_attack_done_set(work);
        } else {
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    }
}

/* 0x802F7A00 (0x100): the hold-position step: swims slowly while the heading is clamped to the forward arc. */
extern "C" void em032_act5_hold(_ENEMY_WORK* work) {
    u32 rot;

    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set_ck(work, 1, 6, 0);
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 3.0f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 1:
        if (em_approach_step(work, 0, 0x50) == 1) {
            em_action_finish_swim(work);
        }
        rot = work->field_0x1BC;
        if (rot > 0x1000 && rot < 0x4000) {
            work->field_0x1BC = 0x1000;
        } else if (rot > 0xC000 && rot < 0xE000) {
            work->field_0x1BC = 0xE000;
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    }
}

/* 0x802F7B00 (0x17C): the approach-and-stop step: swims to the target, coasts out, and advances the route index
 * once when it comes within 300 units. */
extern "C" void em032_act5_approach_stop(_ENEMY_WORK* work, u8 alt) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if ((s32)alt == 0) {
            em_mot_set_ck(work, 1, 6, 0);
        } else {
            em_mot_set_ck(work, 5, 20, 0);
        }
        em_approach_start(work, 0.0f, 2);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.z = 5.0f;
        work->vec_0x31C.z = -0.375f;
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        work->uv_model_0x328.armed_0x329 = 0;
        break;
    case 1:
        if (em_approach_step(work, 0, 0x5B) == 1) {
            work->state++;
        } else {
            em_move_offset_rot_apply(work, &work->field_0x1BC);
        }
        break;
    case 2:
        if (work->offset_0x30C.vec_0x310.z > 0.0f) {
            em_move_offset_step(work, &work->field_0x1BC);
        } else {
            em_action_finish_swim(work);
        }
        break;
    }
    if ((s32)work->uv_model_0x328.armed_0x329 == 0 && vec3_dist_sq(&work->pos, &work->vec_0x36C) <= 90000.0f) {
        work->uv_model_0x328.armed_0x329 = 1;
        em032_route_index_next(work);
    }
}

/* 0x802F7C7C (0xAC): the shake-off step: plays the mode's shake motion to its end. */
extern "C" void em032_act5_shake_off(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        if (work->field_0x00A == 1) {
            em_mot_set_ck(work, 0x66, 6, 0);
        } else {
            em_mot_set_ck(work, 0x68, 6, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F7D28 (0x250): sub-state dispatcher of the swim action. */
extern "C" void em032_act5_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0x0:
        em032_act5_swim_forward(work, 0, 0);
        break;
    case 0x1:
        em032_act5_swim_turn(work, 0, 0);
        break;
    case 0x2:
        em032_act5_turn_seq_a(work);
        break;
    case 0x3:
        em032_act5_turn_seq_b(work);
        break;
    case 0x4:
        em032_act5_swim_turn(work, 2, 0);
        break;
    case 0x5:
        em032_act5_swim_forward(work, 1, 0);
        break;
    case 0x6:
        em032_act5_swim_turn(work, 1, 0);
        break;
    case 0x7:
        em032_act5_drift(work, 0);
        break;
    case 0x8:
        em032_act5_drift(work, 1);
        break;
    case 0x9:
        em032_act5_swim_turn(work, 3, 0);
        break;
    case 0xA:
        em032_act5_swim_turn(work, 4, 0);
        break;
    case 0xB:
        em032_act5_seat_swim(work);
        break;
    case 0xC:
        em032_act5_swim_forward(work, 2, 0);
        break;
    case 0xD:
        em032_act5_swim_forward(work, 3, 0);
        break;
    case 0xE:
        em032_act5_swim_forward(work, 4, 0);
        break;
    case 0xF:
        em032_act5_dive(work);
        break;
    case 0x10:
        em032_act5_hover(work, 0);
        break;
    case 0x11:
        em032_act5_retreat(work, 0);
        break;
    case 0x12:
        em032_act5_swim_turn(work, 5, 0);
        break;
    case 0x13:
        em032_act5_circle(work, 0, 0);
        break;
    case 0x14:
        em032_act5_swim_turn(work, 6, 0);
        break;
    case 0x15:
        em032_act5_circle(work, 1, 0);
        break;
    case 0x16:
        em032_act5_rise_sink(work, 0);
        break;
    case 0x17:
        em032_act5_leap(work, 0);
        break;
    case 0x18:
        em032_act5_surface(work);
        break;
    case 0x19:
        em032_act5_turn(work);
        break;
    case 0x1A:
        em032_act5_leap(work, 1);
        break;
    case 0x1B:
        em032_act5_circle_wide(work, 0, 0);
        break;
    case 0x1C:
        em032_act5_hover(work, 1);
        break;
    case 0x1D:
        em032_act5_u_turn(work);
        break;
    case 0x1E:
        em032_act5_swim_turn(work, 7, 0);
        break;
    case 0x1F:
        em032_act5_circle_wide(work, 1, 0);
        break;
    case 0x20:
        em032_act5_swim_forward(work, 0, 1);
        break;
    case 0x21:
        em032_act5_swim_forward(work, 1, 1);
        break;
    case 0x22:
        em032_act5_swim_forward(work, 2, 1);
        break;
    case 0x23:
        em032_act5_swim_forward(work, 3, 1);
        break;
    case 0x24:
        em032_act5_swim_forward(work, 4, 1);
        break;
    case 0x25:
        em032_act5_swim_turn(work, 0, 1);
        break;
    case 0x26:
        em032_act5_swim_turn(work, 2, 1);
        break;
    case 0x27:
        em032_act5_swim_turn(work, 1, 1);
        break;
    case 0x28:
        em032_act5_swim_turn(work, 3, 1);
        break;
    case 0x29:
        em032_act5_swim_turn(work, 4, 1);
        break;
    case 0x2A:
        em032_act5_swim_turn(work, 5, 1);
        break;
    case 0x2B:
        em032_act5_swim_turn(work, 6, 1);
        break;
    case 0x2C:
        em032_act5_swim_turn(work, 7, 1);
        break;
    case 0x2D:
        em032_act5_retreat(work, 1);
        break;
    case 0x2E:
        em032_act5_circle(work, 0, 1);
        break;
    case 0x2F:
        em032_act5_circle(work, 1, 1);
        break;
    case 0x30:
        em032_act5_circle_wide(work, 0, 1);
        break;
    case 0x31:
        em032_act5_circle_wide(work, 1, 1);
        break;
    case 0x32:
        em032_act5_lunge(work);
        break;
    case 0x33:
        em032_act5_hold(work);
        break;
    case 0x34:
        em032_act5_approach_stop(work, 0);
        break;
    case 0x35:
        em032_act5_approach_stop(work, 1);
        break;
    case 0x36:
        em032_act5_shake_off(work);
        break;
    case 0x37:
        em032_act5_rise_sink(work, 1);
        break;
    case 0x38:
        em032_act5_rise_sink(work, 2);
        break;
    }
}

/* 0x802F7F78 (0xC4): the first bite: arms hit window 1 and turns to the target for the first 60 frames. */
extern "C" void em032_act7_bite_a(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 8, 6, 0);
        em_hit_window_set_default(work, 0, 1);
        break;
    case 1:
        if (em_frame_check(work, 3, 0.0f, 60.0f) == 1) {
            em_turn_to_target_3d(work, 0xB6, 0xB6);
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F803C (0xC4): the second bite: arms hit window 2 and turns to the target for the first 50 frames. */
extern "C" void em032_act7_bite_b(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 9, 6, 0);
        em_hit_window_set_default(work, 0, 2);
        break;
    case 1:
        if (em_frame_check(work, 3, 0.0f, 50.0f) == 1) {
            em_turn_to_target_3d(work, 0x92, 0x92);
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F8100 (0x190): the ram: winds up facing the target, then charges with hit window 3 armed for at least 20
 * frames and coasts out. */
extern "C" void em032_act7_ram(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 6, 6, 0);
        break;
    case 1:
        em_turn_to_target_3d(work, 0x5B, 0x5B);
        if (em_mot_end_ck(work) == 1) {
            work->state++;
            em_mot_set_ck(work, 3, 6, 0);
            em_approach_start(work, 500.0f, 2);
            em_move_vec2_clr(work);
            work->offset_0x30C.vec_0x310.z = 50.0f;
            work->vec_0x31C.z = -1.5f;
            work->timer_0x020 = 20;
            em_hit_window_set(work, 0, 3, 3);
        }
        break;
    case 2:
        if (work->timer_0x020 > 0) {
            work->timer_0x020--;
        }
        if (em_approach_step(work, 0, 0x20) == 1 && work->timer_0x020 <= 0) {
            work->state++;
            em_hit_window_clear(work, 0);
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    case 3:
        if (work->offset_0x30C.vec_0x310.z > 0.0f) {
            em_move_offset_step(work, &work->field_0x1BC);
        } else {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* 0x802F8290 (0x4D8): the charge attack: turns to the target while the motion winds up, then charges on the
 * second motion until the measured distance is used up; mode 0 spawns the wake effects, mode 1 the sparks. */
extern "C" void em032_act7_charge(_ENEMY_WORK* work, u8 mode) {
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 unused_b;
    f32 turn = 3.0f;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&pos);
    VEC3_ctor(&unused_b);
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 12, 20, 0);
        work->value_0x378 = vec3_distance(&work->pos, &work->vec_0x36C);
        copyVec3(&work->uv_model_0x328.pos_0x330, &work->pos);
        em_move_vec2_clr(work);
        if (work->field_0x00A == 4) {
            work->offset_0x30C.vec_0x310.z = 15.0f;
            work->vec_0x31C.z = -1.0f;
        }
        break;
    case 1:
        em_turn_to_target_3d(work, (u16)(s32)(0.5f + (65536.0f * turn) / 360.0f),
                             (u16)(s32)(0.5f + (65536.0f * turn) / 360.0f));
        if (em_mot_frame_get(work) >= 120.0f) {
            work->state++;
            work->timer_0x020 = 0;
            if (mode == 1) {
                em_hit_window_set_default(work, 0, 4);
            } else {
                em_hit_window_set_default(work, 0, 5);
            }
        }
        switch (mode) {
        case 0:
            if (em_frame_check(work, 0, 92.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(work, 0x4E, 3, &pos, 1.5f);
            }
            break;
        case 1:
            if (em_frame_check(work, 3, 2.0f, 76.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
                eft013_spark_set_em(work, 0x1A, 3, 1.8f + 0.1f * (f32)(ran_suu(0) & 3));
            }
            if (em_frame_check(work, 3, 86.0f, 120.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
                eft013_spark_set_em(work, 0x1A, 3, 1.6f + 0.1f * (f32)(ran_suu(0) & 3));
            }
            break;
        }
        break;
    case 2:
        if (work->value_0x378 > 0.0f) {
            work->value_0x378 -= vec3_distance(&work->pos, &work->uv_model_0x328.pos_0x330);
            if (em_mot_end_ck(work) == 1) {
                work->state = 1;
                em_mot_set(work, 12, 20, 0);
            }
        } else if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        if (work->field_0x00A == 4) {
            if (em_frame_check(work, 3, 126.0f, 170.0f) == 1) {
                em_move_offset_rot_apply(work, &work->field_0x1BC);
            } else if (work->offset_0x30C.vec_0x310.z > 0.0f) {
                em_move_offset_step(work, &work->field_0x1BC);
            }
        }
        switch (mode) {
        case 0:
            if (em_frame_check(work, 0, 132.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(work, 0x4D, 3, &pos, 1.0f);
            }
            break;
        case 1:
            if (em_frame_check(work, 0, 130.0f, 0.0f) == 1) {
                eft_spawn_type_at_area(work, 7);
            }
            if (em_frame_check(work, 3, 130.0f, 180.0f) == 1 && ++work->timer_0x020 > 6) {
                work->timer_0x020 = 0;
                eft013_spark_set_em(work, 0x1B, 3, 2.0f + 0.1f * (f32)(ran_suu(0) & 3));
            }
            break;
        }
        copyVec3(&work->uv_model_0x328.pos_0x330, &work->pos);
        break;
    }
}

/* Selects the record table of the UV-model placement by the record's state byte. */
extern "C" void em032_act7_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em032_act7_bite_a(work);
        break;
    case 1:
        em032_act7_bite_b(work);
        break;
    case 2:
        em032_act7_ram(work);
        break;
    case 3:
        em032_act7_charge(work, 0);
        break;
    case 4:
        em032_act7_charge(work, 1);
        break;
    }
}

/* Places the model record of a sub-state whose table is chosen by the record's mode. */
extern "C" void em032_act10_se_play(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0xA:
        if ((u32)(work->field_0x00A - 3) > 1) {
            if ((s32)work->field_0x00A == 1) {
                em_se_tbl_play(work, em032_se_seq_10_0A_2, 2, 0xA);
            } else {
                em_se_tbl_play(work, em032_se_seq_10_0A, 2, 0xA);
            }
        } else {
            em_se_tbl_play(work, em032_se_seq_10_0A_3, 2, 0xA);
        }
        break;
    case 0x89:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_89_2, 2, 0x89);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_89, 2, 0x89);
        }
        break;
    case 0x8A:
        em_se_tbl_play(work, em032_se_seq_10_8A, 2, 0x8A);
        break;
    case 0xA1:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_A1_2, 2, 0xA1);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_A1, 2, 0xA1);
        }
        break;
    case 0xA2:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_A1_2, 2, 0xA2);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_A1, 2, 0xA2);
        }
        break;
    case 0x8B:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_8B_2, 2, 0x8B);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_8B, 2, 0x8B);
        }
        break;
    case 0x8C:
        em_se_tbl_play(work, em032_se_seq_10_8C, 2, 0x8C);
        break;
    case 0xAA:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_AA_2, 2, 0xAA);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_AA, 2, 0xAA);
        }
        break;
    case 0xB1:
        em_se_tbl_play(work, em032_se_seq_10_0A, 2, 0xB1);
        break;
    case 0xCB:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play(work, em032_se_seq_10_0A_2, 2, 0xCB);
        } else {
            em_se_tbl_play(work, em032_se_seq_10_0A, 2, 0xCB);
        }
        break;
    default:
        em_action_finish_swim(work);
        break;
    }
}

/* Places the second model record set of the same table family. */
extern "C" void em032_act11_se_play(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0x0:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, em032_se_seq_11_00_2, 2, 0);
        } else {
            em_se_tbl_play_alt(work, em032_se_seq_11_00, 2, 0);
        }
        break;
    case 0xA:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, em032_se_seq_11_00_2, 2, 0xA);
        } else {
            em_se_tbl_play_alt(work, em032_se_seq_11_00, 2, 0xA);
        }
        break;
    case 0x33:
        if ((u32)(work->field_0x00A - 3) > 1) {
            if ((s32)work->field_0x00A == 1) {
                em_se_tbl_play_alt(work, em032_se_seq_11_33_2, 2, 0x33);
            } else {
                em_se_tbl_play_alt(work, em032_se_seq_11_33, 2, 0x33);
            }
        } else {
            em_se_tbl_play_alt(work, em032_se_seq_11_33_3, 2, 0x33);
        }
        break;
    case 0x38:
        em_se_tbl_play_alt(work, em032_se_seq_11_38, 2, 0x38);
        break;
    default:
        if ((s32)work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, em032_se_seq_11_00_2, 2, 0xA);
        } else {
            em_se_tbl_play_alt(work, em032_se_seq_11_00, 2, 0xA);
        }
        break;
    }
}

/* Advances the record's own motion. */
extern "C" u16 em032_motion_events(_ENEMY_WORK* work) {
    return em_get_mot_no(work);
}

extern "C" u8 em032_state_byte_get(_ENEMY_WORK* work, u32 index) {
    u8 slot = index;

    switch (slot) {
    case 0:
        return work->uv_model_0x328.slot_0x328;
    case 1:
        return work->uv_model_0x328.created_0x33E;
    case 2:
        return work->uv_model_0x328.armed_0x329;
    default:
        return 0;
    }
}

/* Retires the record's model when it is in the idle mode. */
extern "C" s32 em032_event_request(_ENEMY_WORK* work) {
    if (work->field_0x00A == 1) {
        lb_event_request(20);
    }
    return 0;
}

/* 0x802F8B9C (0x388): places the record at the start of its area's route: picks the first action and sub-state
 * for the map kind and area and sets the start target, or parks the record at the origin where the area has no
 * route. */
extern "C" void em032_area_start_set(_ENEMY_WORK* work, u8* action, u8* state_sub) {
    work->field_0x1C0 = 0;
    em_move_mode_set(work, 2);
    switch (stage_map_kind_get(work->field_0x1E0)) {
    case 1:
        if ((s32)work->field_0x00A == 4) {
            *action = 12;
            *state_sub = 1;
        } else {
            *action = 12;
            *state_sub = 0;
        }
        switch (work->area_no) {
        case 7:
            if ((s32)work->field_0x00A == 4) {
                em_motion_param_set(work, 0.0f, 0);
                em_motion_param_set(work, 1.0f, 20);
            } else {
                em_move_target_set(work, 0, 1, 0.0f);
            }
            break;
        case 12:
            if ((s32)work->field_0x00A == 4) {
                em_motion_param_set(work, 0.0f, 0);
                em_motion_param_set(work, 1.0f, 30);
            } else {
                em_move_target_set(work, 0, 1, 0.0f);
            }
            break;
        default:
            *action = 0;
            *state_sub = 4;
            setVector3(&work->pos, 0.0f, 0.0f, 0.0f);
            work->field_0x1C4 = 0;
            work->field_0x1C0 = 0;
            work->field_0x1BC = 0;
            break;
        }
        break;
    case 3:
        if ((u32)(work->field_0x00A - 5) <= 1 || (s32)work->field_0x00A == 1) {
            *action = 12;
            *state_sub = 0;
        } else {
            *action = 0;
            *state_sub = 4;
        }
        switch (work->area_no) {
        case 2:
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0xF000;
            break;
        case 3:
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0x8000;
            break;
        case 4:
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0xB400;
            break;
        case 6:
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0xC00;
            break;
        case 8:
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0x400;
            break;
        default:
            *action = 0;
            *state_sub = 4;
            setVector3(&work->pos, 0.0f, 0.0f, 0.0f);
            work->field_0x1C4 = 0;
            work->field_0x1C0 = 0;
            work->field_0x1BC = 0;
            break;
        }
        break;
    case 11:
        if ((s32)work->field_0x00A == 4) {
            *action = 12;
            *state_sub = 1;
        } else {
            *action = 12;
            *state_sub = 0;
        }
        if ((s32)work->area_no == 1) {
            em_move_target_set(work, 0, 1, 0.0f);
            work->field_0x1C0 = 0x5C00;
        } else {
            *action = 0;
            *state_sub = 4;
            setVector3(&work->pos, 0.0f, 0.0f, 0.0f);
            work->field_0x1C4 = 0;
            work->field_0x1C0 = 0;
            work->field_0x1BC = 0;
        }
        break;
    default:
        *action = 0;
        *state_sub = 4;
        setVector3(&work->pos, 0.0f, 0.0f, 0.0f);
        work->field_0x1C4 = 0;
        work->field_0x1C0 = 0;
        work->field_0x1BC = 0;
        break;
    }
}

/* The route tables of the areas the record patrols, per (map kind, area): the route point ids (0xFF ends the
 * route) and the per-point value `em_move_target_set` takes. */
static u8 em032_route_ids_k1_a7[] = {0xFF};
static f32 em032_route_vals_k1_a7[] = {-1.0f};
static u8 em032_route_ids_k1_a12[] = {0x02, 0xFF};
static f32 em032_route_vals_k1_a12[] = {0.0f, -1.0f};
static u8 em032_route_ids_k3_a2[] = {0xFF};
static f32 em032_route_vals_k3_a2[] = {-1.0f};
static u8 em032_route_ids_k3_a3[] = {0xFF};
static f32 em032_route_vals_k3_a3[] = {-1.0f};
static u8 em032_route_ids_k3_a4[] = {0x02, 0xFF};
static f32 em032_route_vals_k3_a4[] = {0.0f, -1.0f};
static u8 em032_route_ids_k3_a6[] = {0xFF};
static f32 em032_route_vals_k3_a6[] = {-1.0f};
static u8 em032_route_ids_k3_a8[] = {0xFF};
static f32 em032_route_vals_k3_a8[] = {-1.0f};
static u8 em032_route_ids_k11_a1[] = {0xFF};
static f32 em032_route_vals_k11_a1[] = {-1.0f};

/* 0x802F8F24 (0x378): the route step: swims to the current route point, then targets the next one of the area's
 * route, or ends the patrol (and re-picks the nearest point in the idle modes) when the route runs out. */
extern "C" void em032_act12_route(_ENEMY_WORK* work) {
    u8* ids;
    f32* vals;
    f32 dy;

    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_approach_start(work, 0.0f, 2);
        if ((s32)work->field_0x00A == 3) {
            em_mot_set_ck(work, 11, 20, 0);
        } else {
            em_mot_set_ck(work, 1, 6, 0);
        }
        em_move_vec2_clr(work);
        switch (work->field_0x00A) {
        default:
            work->offset_0x30C.vec_0x310.z = 5.0f;
            work->vec_0x31C.z = -0.375f;
            break;
        case 2:
        case 6:
            work->offset_0x30C.vec_0x310.z = 10.0f;
            work->vec_0x31C.z = -0.75f;
            break;
        case 1:
            work->offset_0x30C.vec_0x310.z = 5.0f;
            work->vec_0x31C.z = -0.375f;
            break;
        case 3:
            work->offset_0x30C.vec_0x310.z = 5.0f;
            work->vec_0x31C.z = 0.0f;
            break;
        }
        break;
    case 1:
        if (em_approach_step(work, 0, 0x100) == 1) {
            switch (stage_map_kind_get(work->field_0x1E0)) {
            case 1:
                switch (work->area_no) {
                case 7:
                    ids = em032_route_ids_k1_a7;
                    vals = em032_route_vals_k1_a7;
                    break;
                case 12:
                    ids = em032_route_ids_k1_a12;
                    vals = em032_route_vals_k1_a12;
                    break;
                default:
                    ids = NULL;
                    vals = NULL;
                    break;
                }
                break;
            case 3:
                switch (work->area_no) {
                case 2:
                    ids = em032_route_ids_k3_a2;
                    vals = em032_route_vals_k3_a2;
                    break;
                case 3:
                    ids = em032_route_ids_k3_a3;
                    vals = em032_route_vals_k3_a3;
                    break;
                case 4:
                    ids = em032_route_ids_k3_a4;
                    vals = em032_route_vals_k3_a4;
                    break;
                case 6:
                    ids = em032_route_ids_k3_a6;
                    vals = em032_route_vals_k3_a6;
                    break;
                case 8:
                    ids = em032_route_ids_k3_a8;
                    vals = em032_route_vals_k3_a8;
                    break;
                default:
                    ids = NULL;
                    vals = NULL;
                    break;
                }
                break;
            case 11:
                if ((s32)work->area_no == 1) {
                    ids = em032_route_ids_k11_a1;
                    vals = em032_route_vals_k11_a1;
                } else {
                    ids = NULL;
                    vals = NULL;
                }
                break;
            default:
                ids = NULL;
                vals = NULL;
                break;
            }
            if (ids != NULL && vals != NULL) {
                if (ids[work->uv_model_0x328.slot_0x328] != 0xFF) {
                    em_move_target_set(work, 0xFF, ids[work->uv_model_0x328.slot_0x328],
                                       vals[work->uv_model_0x328.slot_0x328]);
                    work->uv_model_0x328.slot_0x328++;
                    em_state_set(work, 12, 0);
                } else {
                    work->uv_model_0x328.slot_0x328 = 0;
                    em_action_finish_swim(work);
                    if ((s32)work->field_0x00A == 0 || (s32)work->field_0x00A == 5) {
                        em032_route_index_nearest(work);
                    }
                }
            } else {
                work->uv_model_0x328.slot_0x328 = 0;
                em_action_finish_swim(work);
                if ((s32)work->field_0x00A == 0 || (s32)work->field_0x00A == 5) {
                    em032_route_index_nearest(work);
                }
            }
        } else if ((s32)work->field_0x00A == 3) {
            dy = work->pos.y - work->vec_0x36C.y;
            if (dy > 0.0f) {
                if (em_frame_check(work, 3, 50.0f, 100.0f) != 0) {
                    em_height_follow(work, 5.0f);
                }
            } else if (dy < 0.0f && em_frame_check(work, 3, 110.0f, 160.0f) != 0) {
                em_height_follow(work, 5.0f);
            }
        }
        em_move_offset_rot_apply(work, &work->field_0x1BC);
        break;
    }
}

/* First step of the band's own "place" action. */
extern "C" void em032_act12_settle(_ENEMY_WORK* work) {
    em_move_mode_set(work, 2);
    em_mot_set(work, 10, 0, 0);
    em_action_finish_swim(work);
}

/* Sub-state dispatcher of the band's own "place" action. */
extern "C" void em032_act12_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em032_act12_route(work);
        break;
    case 1:
        em032_act12_settle(work);
        break;
    default:
        break;
    }
}

/* First step of the "roar" action: starts the second motion set and seeds the three effect sizes. */
extern "C" void em032_act13_demo_0(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 2, 0, 0x46);
        em_demo_pos_set(work, -3900.01f, -3147.0f, 2104.83f);
        em_demo_rot_set(work, 0.14f, 50.57f, 0.83f);
        em_demo_key_apply(work, em_demo_frame_get(), em032_demo0_pos_keys, em032_demo0_rot_keys, 5, 7);
        break;
    case 1:
        if (em_demo_time_ck(0x26C) == 0) {
            em_demo_key_apply(work, em_demo_frame_get(), em032_demo0_pos_keys, em032_demo0_rot_keys, 5, 7);
        } else {
            em_demo_key_apply(work, em_demo_frame_get(), em032_demo0_pos_keys, em032_demo0_rot_keys_late, 5, 5);
        }
        break;
    }
}

/* First step of the "stance" action. */
extern "C" void em032_act13_demo_1(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 1, 0, 0);
        em_demo_pos_set(work, -2474.0f, -3147.0f, 2377.0f);
        em_demo_rot_set(work, 0.0f, 82.0f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* First step of the "movement" action. */
extern "C" void em032_act13_demo_2(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 2, 0, 0x14);
        em_demo_pos_set(work, -4263.98f, -2706.18f, 2100.8f);
        em_demo_key_apply(work, em_demo_frame_get(), em032_demo2_pos_keys, em032_demo2_rot_keys, 5, 7);
        break;
    case 1:
        em_demo_key_apply(work, em_demo_frame_get(), em032_demo2_pos_keys, em032_demo2_rot_keys, 5, 7);
        break;
    }
}

/* First step of the "crouch" action. */
extern "C" void em032_act13_demo_3(_ENEMY_WORK* work) {
    em_hit_arm_set(work);
    switch (work->state) {
    case 0:
        work->state++;
        em_move_mode_set(work, 2);
        em_mot_set(work, 1, 0, 0);
        em_demo_pos_set(work, -3199.0f, -2706.0f, 2678.0f);
        em_demo_rot_set(work, 0.0f, 0.0f, 65.0f);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish_swim(work);
        }
        break;
    }
}

/* Sub-state dispatcher of the "crouch" action. */
extern "C" void em032_act13_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em032_act13_demo_0(work);
        break;
    case 1:
        em032_act13_demo_1(work);
        break;
    case 2:
        em032_act13_demo_2(work);
        break;
    case 3:
        em032_act13_demo_3(work);
        break;
    }
}

/* The action dispatcher: picks the record's step machine by its action id. */
extern "C" void em032_action_dispatch(_ENEMY_WORK* work) {
    switch (work->action) {
    case 0:
        em032_act0_dispatch(work);
        break;
    case 5:
        em032_act5_dispatch(work);
        break;
    case 7:
        em032_act7_dispatch(work);
        break;
    case 10:
        em032_act10_se_play(work);
        break;
    case 11:
        em032_act11_se_play(work);
        break;
    case 12:
        em032_act12_dispatch(work);
        break;
    case 13:
        em032_act13_dispatch(work);
        break;
    }
}

/* Does nothing; the action's state byte is never 4. */
extern "C" void em032_noop(void) {
}

/* Reports whether the record's mode puts it in the retirable states. */
extern "C" s32 em032_retire_ck(_ENEMY_WORK* work) {
    u8 mode = work->field_0x00A;

    if (mode == 0 || mode == 5 || mode == 3) {
        return em032_half_hp_ck(work);
    }
    return 1;
}

/* Reports whether the record's mode puts it in one of the four "seated" modes. */
extern "C" s32 em032_rest_mode_ck(_ENEMY_WORK* work) {
    u8 mode = work->field_0x00A;

    if (mode == 0 || mode == 2 || mode == 5 || mode == 6) {
        return 1;
    }
    return 0;
}

/* Reports whether the record's area lookup and attack timer allow the action. */
extern "C" s32 em032_action_select(_ENEMY_WORK* work) {
    switch (stage_map_kind_get(work->field_0x1E0)) {
    case 1:
    case 3:
    case 11:
        if (work->field_0x011 != 0) {
            work->field_0x1FC = 1;
            work->field_0x1FE = 7;
            work->field_0x1FF = 0;
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x802F97F0 (0xA8): clears the attack-finished state once the area of the record's frame table is vacant. */
extern "C" void em032_vacant_attack_clear(_ENEMY_WORK* work) {
    u8 mode = 0xFF;

    switch (work->frames_index_0x015) {
    case 0:
        mode = 0;
        break;
    case 1:
        mode = 1;
        break;
    case 2:
        mode = 2;
        break;
    case 3:
        mode = 3;
        break;
    case 4:
        mode = 4;
        break;
    case 5:
        mode = 5;
        break;
    }
    if (em_area_vacant_wait_ck(work, mode) != 0) {
        em_attack_finished_clear(work);
    }
}

/* The three point/step vector pairs the static initializer seeds (`.bss`, defined after it). */
extern nw4r::math::VEC3 em032_vec_pair_0[2];
extern nw4r::math::VEC3 em032_vec_pair_1[2];
extern nw4r::math::VEC3 em032_vec_pair_2[2];

/* 0x802F9898 (0xFC): the static initializer: seeds the three point/step vector pairs. */
extern "C" void em032_vec_pairs_init(void) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;
    nw4r::math::VEC3 e;
    nw4r::math::VEC3 f;

    assignVec3((Vec*)em032_vec_pair_0, (Vec*)setVec3(&a, 0.0f, 0.0f, 40.0f));
    assignVec3((Vec*)&em032_vec_pair_0[1], (Vec*)setVec3(&b, 0.0f, 0.0f, 0.0f));
    assignVec3((Vec*)em032_vec_pair_1, (Vec*)setVec3(&c, 0.0f, 2.0f, 0.0f));
    assignVec3((Vec*)&em032_vec_pair_1[1], (Vec*)setVec3(&d, 0.0f, -0.02f, 0.0f));
    assignVec3((Vec*)em032_vec_pair_2, (Vec*)setVec3(&e, 0.0f, 0.0f, 20.0f));
    assignVec3((Vec*)&em032_vec_pair_2[1], (Vec*)setVec3(&f, 0.0f, 0.0f, 0.0f));
}

EmSeRecord em032_se_rec_10_0A_a = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_0A[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_0A_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_0A_2_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_0A_2[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_0A_2_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_0A_3_a = {109, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_0A_3[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_0A_3_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_89_a = {103, 0, 16, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em032_se_seq_10_89[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x2)},
    {0x00, {0, 0, 0}, &em032_se_rec_10_89_a},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_A1_a = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_A1[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_A1_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_89_2_a = {103, 0, 16, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em032_se_seq_10_89_2[4] = {
    {0x0E, {0, 0, 0}, (const EmSeRecord*)em032_se_kind0E_arg},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x2)},
    {0x00, {0, 0, 0}, &em032_se_rec_10_89_2_a},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_A1_2_a = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_A1_2[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_A1_2_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_8A_a = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em032_se_seq_10_8A[2] = {
    {0x01, {0, 0, 0}, &em032_se_rec_10_8A_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_8B_a = {104, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em032_se_seq_10_8B[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_8B_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_8B_2_a = {102, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em032_se_seq_10_8B_2[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_8B_2_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_AA_a = {104, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_AA[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_AA_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_10_AA_2_a = {102, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_10_AA_2[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_AA_2_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em032_se_seq_10_8C[2] = {
    {0x00, {0, 0, 0}, &em032_se_rec_10_8B_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_00_a = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em032_se_rec_11_00_b = {3, 0, 6, 0, 0, {0, 4, 3, 0}, (const EmSeOffset*)em032_vec_pair_0};

EmSeEntry em032_se_seq_11_00[5] = {
    {0x00, {0, 0, 0}, &em032_se_rec_11_00_a},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em032_se_rec_11_00_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_00_2_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em032_se_rec_11_00_2_b = {3, 0, 6, 0, 0, {0, 4, 3, 0}, (const EmSeOffset*)em032_vec_pair_0};

EmSeEntry em032_se_seq_11_00_2[5] = {
    {0x00, {0, 0, 0}, &em032_se_rec_11_00_2_a},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em032_se_rec_11_00_2_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_33_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em032_se_rec_11_33_b = {106, 0, 2, 0, 0, {0, 129, 3, 0}, (const EmSeOffset*)em032_vec_pair_1};

EmSeEntry em032_se_seq_11_33[3] = {
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_a},
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_33_2_a = {108, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em032_se_rec_11_33_2_b = {107, 0, 2, 0, 0, {0, 129, 3, 0}, (const EmSeOffset*)em032_vec_pair_1};

EmSeEntry em032_se_seq_11_33_2[3] = {
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_2_a},
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_2_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_33_3_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em032_se_rec_11_33_3_b = {110, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em032_se_seq_11_33_3[3] = {
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_3_a},
    {0x00, {0, 0, 0}, &em032_se_rec_11_33_3_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em032_se_rec_11_38_a = {3, 0, 6, 0, 0, {0, 4, 3, 0}, (const EmSeOffset*)em032_vec_pair_0};

EmSeEntry em032_se_seq_11_38[5] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em032_se_rec_11_38_a},
    {0xFF, {0, 0, 0}, 0},
};

f32 em032_demo0_pos_keys[166] = {
    0.0f, -3900.01f, 2104.83f,
    12.0f, -3884.84f, 2111.73f,
    24.0f, -3860.08f, 2120.81f,
    36.0f, -3827.61f, 2131.5f,
    48.0f, -3789.35f, 2143.22f,
    60.0f, -3747.19f, 2155.4f,
    72.0f, -3703.01f, 2167.48f,
    84.0f, -3658.73f, 2178.88f,
    96.0f, -3616.22f, 2189.04f,
    108.0f, -3577.4f, 2197.38f,
    120.0f, -3544.16f, 2203.33f,
    132.0f, -3513.93f, 2207.98f,
    144.0f, -3482.95f, 2212.78f,
    156.0f, -3451.27f, 2217.71f,
    168.0f, -3418.99f, 2222.78f,
    180.0f, -3386.17f, 2227.95f,
    192.0f, -3352.89f, 2233.23f,
    204.0f, -3319.22f, 2238.59f,
    216.0f, -3285.24f, 2244.02f,
    228.0f, -3251.01f, 2249.51f,
    240.0f, -3216.63f, 2255.05f,
    252.0f, -3182.15f, 2260.62f,
    264.0f, -3147.66f, 2266.21f,
    276.0f, -3113.23f, 2271.81f,
    288.0f, -3078.93f, 2277.4f,
    300.0f, -3044.83f, 2282.97f,
    312.0f, -3011.02f, 2288.51f,
    324.0f, -2977.57f, 2294.0f,
    336.0f, -2944.54f, 2299.44f,
    348.0f, -2912.02f, 2304.8f,
    360.0f, -2880.08f, 2310.08f,
    372.0f, -2848.8f, 2315.26f,
    384.0f, -2818.24f, 2320.32f,
    396.0f, -2788.48f, 2325.26f,
    408.0f, -2759.6f, 2330.06f,
    420.0f, -2731.68f, 2334.72f,
    432.0f, -2704.78f, 2339.2f,
    444.0f, -2678.97f, 2343.51f,
    456.0f, -2654.35f, 2347.63f,
    468.0f, -2630.97f, 2351.55f,
    480.0f, -2608.91f, 2355.24f,
    492.0f, -2588.26f, 2358.71f,
    504.0f, -2569.07f, 2361.93f,
    516.0f, -2551.43f, 2364.9f,
    528.0f, -2535.41f, 2367.6f,
    540.0f, -2521.09f, 2370.01f,
    552.0f, -2508.54f, 2372.13f,
    564.0f, -2497.83f, 2373.93f,
    576.0f, -2489.04f, 2375.42f,
    588.0f, -2482.24f, 2376.57f,
    600.0f, -2477.51f, 2377.37f,
    612.0f, -2474.92f, 2377.81f,
    620.0f, -2474.42f, 2377.89f,
    720.0f, -2474.42f, 2377.89f,
    -1.0f, -2474.42f, 2377.89f,
    0.0f,
};

f32 em032_demo0_rot_keys[320] = {
    0.0f, 0.14f, 50.57f, 0.83f,
    8.0f, 0.14f, 50.65f, 0.83f,
    16.0f, 0.13f, 50.85f, 0.83f,
    24.0f, 0.13f, 51.14f, 0.82f,
    32.0f, 0.12f, 51.51f, 0.81f,
    40.0f, 0.11f, 51.98f, 0.8f,
    48.0f, 0.1f, 52.51f, 0.78f,
    56.0f, 0.09f, 53.13f, 0.75f,
    64.0f, 0.08f, 53.8f, 0.71f,
    72.0f, 0.06f, 54.55f, 0.66f,
    80.0f, 0.05f, 55.34f, 0.59f,
    88.0f, 0.03f, 56.19f, 0.51f,
    96.0f, 0.02f, 57.09f, 0.41f,
    104.0f, 0.01f, 58.03f, 0.29f,
    112.0f, 0.0f, 59.0f, 0.16f,
    120.0f, 0.0f, 60.0f, 0.0f,
    128.0f, 0.0f, 61.02f, -0.18f,
    136.0f, 0.01f, 62.06f, -0.37f,
    144.0f, 0.03f, 63.1f, -0.58f,
    152.0f, 0.06f, 64.16f, -0.81f,
    160.0f, 0.1f, 65.21f, -1.05f,
    168.0f, 0.14f, 66.27f, -1.29f,
    176.0f, 0.2f, 67.33f, -1.55f,
    184.0f, 0.27f, 68.39f, -1.81f,
    192.0f, 0.35f, 69.44f, -2.08f,
    200.0f, 0.44f, 70.48f, -2.35f,
    208.0f, 0.54f, 71.51f, -2.63f,
    216.0f, 0.65f, 72.53f, -2.9f,
    224.0f, 0.77f, 73.54f, -3.18f,
    232.0f, 0.89f, 74.52f, -3.45f,
    240.0f, 1.03f, 75.49f, -3.72f,
    248.0f, 1.18f, 76.43f, -3.98f,
    256.0f, 1.33f, 77.35f, -4.24f,
    264.0f, 1.48f, 78.24f, -4.49f,
    272.0f, 1.64f, 79.1f, -4.73f,
    280.0f, 1.8f, 79.92f, -4.96f,
    288.0f, 1.96f, 80.72f, -5.17f,
    296.0f, 2.12f, 81.47f, -5.38f,
    304.0f, 2.28f, 82.18f, -5.58f,
    312.0f, 2.44f, 82.86f, -5.76f,
    320.0f, 2.58f, 83.49f, -5.93f,
    328.0f, 2.73f, 84.07f, -6.08f,
    336.0f, 2.86f, 84.6f, -6.22f,
    344.0f, 2.98f, 85.08f, -6.34f,
    352.0f, 3.09f, 85.51f, -6.44f,
    360.0f, 3.18f, 85.89f, -6.53f,
    368.0f, 3.26f, 86.2f, -6.6f,
    376.0f, 3.33f, 86.46f, -6.65f,
    384.0f, 3.37f, 86.65f, -6.68f,
    392.0f, 3.39f, 86.78f, -6.69f,
    400.0f, 3.4f, 86.85f, -6.68f,
    408.0f, 3.38f, 86.87f, -6.63f,
    416.0f, 3.33f, 86.85f, -6.55f,
    424.0f, 3.27f, 86.8f, -6.44f,
    432.0f, 3.18f, 86.71f, -6.3f,
    440.0f, 3.08f, 86.59f, -6.14f,
    448.0f, 2.97f, 86.43f, -5.95f,
    456.0f, 2.84f, 86.26f, -5.74f,
    464.0f, 2.7f, 86.06f, -5.51f,
    472.0f, 2.55f, 85.84f, -5.26f,
    480.0f, 2.4f, 85.6f, -4.99f,
    488.0f, 2.24f, 85.35f, -4.71f,
    496.0f, 2.07f, 85.09f, -4.42f,
    504.0f, 1.91f, 84.82f, -4.11f,
    512.0f, 1.74f, 84.54f, -3.8f,
    520.0f, 1.57f, 84.27f, -3.48f,
    528.0f, 1.41f, 83.99f, -3.16f,
    536.0f, 1.25f, 83.72f, -2.84f,
    544.0f, 1.09f, 83.46f, -2.52f,
    552.0f, 0.94f, 83.21f, -2.2f,
    560.0f, 0.8f, 82.97f, -1.89f,
    568.0f, 0.66f, 82.75f, -1.58f,
    576.0f, 0.54f, 82.56f, -1.29f,
    584.0f, 0.42f, 82.38f, -1.01f,
    592.0f, 0.31f, 82.24f, -0.75f,
    600.0f, 0.21f, 82.13f, -0.51f,
    608.0f, 0.12f, 82.05f, -0.29f,
    616.0f, 0.04f, 82.01f, -0.09f,
    620.0f, 0.0f, 82.0f, 0.0f,
    -1.0f, 0.0f, 82.0f, 0.0f,
};

f32 em032_demo0_rot_keys_late[48] = {
    620.0f, 0.0f, 0.0f,
    628.0f, -0.06f, 0.14f,
    636.0f, -0.1f, 0.24f,
    644.0f, -0.12f, 0.3f,
    652.0f, -0.13f, 0.32f,
    660.0f, -0.12f, 0.31f,
    668.0f, -0.11f, 0.28f,
    676.0f, -0.09f, 0.23f,
    684.0f, -0.07f, 0.18f,
    692.0f, -0.05f, 0.12f,
    700.0f, -0.03f, 0.07f,
    708.0f, -0.01f, 0.03f,
    712.0f, -0.01f, 0.01f,
    716.0f, -0.0f, 0.0f,
    720.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
};

f32 em032_demo2_pos_keys[136] = {
    0.0f, -4263.98f, 2100.8f,
    12.0f, -4237.36f, 2112.96f,
    24.0f, -4210.37f, 2125.3f,
    36.0f, -4183.12f, 2137.83f,
    48.0f, -4155.53f, 2150.69f,
    60.0f, -4127.22f, 2164.21f,
    72.0f, -4098.24f, 2178.34f,
    84.0f, -4068.66f, 2193.04f,
    96.0f, -4038.56f, 2208.27f,
    108.0f, -4007.99f, 2223.97f,
    120.0f, -3977.04f, 2240.08f,
    132.0f, -3945.77f, 2256.57f,
    144.0f, -3914.24f, 2273.38f,
    156.0f, -3882.54f, 2290.46f,
    168.0f, -3850.72f, 2307.76f,
    180.0f, -3818.87f, 2325.24f,
    192.0f, -3787.04f, 2342.83f,
    204.0f, -3755.31f, 2360.5f,
    216.0f, -3723.74f, 2378.19f,
    228.0f, -3692.42f, 2395.85f,
    240.0f, -3661.4f, 2413.44f,
    252.0f, -3630.75f, 2430.9f,
    264.0f, -3600.55f, 2448.18f,
    276.0f, -3570.87f, 2465.23f,
    288.0f, -3541.77f, 2482.01f,
    300.0f, -3513.32f, 2498.46f,
    312.0f, -3485.6f, 2514.53f,
    324.0f, -3458.67f, 2530.17f,
    336.0f, -3432.61f, 2545.34f,
    348.0f, -3407.47f, 2559.99f,
    360.0f, -3383.34f, 2574.05f,
    372.0f, -3360.28f, 2587.5f,
    384.0f, -3338.37f, 2600.26f,
    396.0f, -3317.66f, 2612.3f,
    408.0f, -3298.23f, 2623.57f,
    420.0f, -3280.16f, 2634.01f,
    432.0f, -3263.5f, 2643.57f,
    444.0f, -3248.33f, 2652.21f,
    456.0f, -3234.72f, 2659.87f,
    468.0f, -3222.74f, 2666.51f,
    480.0f, -3212.46f, 2672.08f,
    492.0f, -3203.94f, 2676.52f,
    500.0f, -3199.28f, 2678.83f,
    720.0f, -3199.28f, 2678.83f,
    -1.0f, -3199.28f, 2678.83f,
    0.0f,
};

f32 em032_demo2_rot_keys[284] = {
    0.0f, 0.04f, 37.69f, 0.38f,
    8.0f, 0.03f, 39.11f, 0.35f,
    16.0f, 0.02f, 40.47f, 0.29f,
    24.0f, 0.01f, 41.75f, 0.21f,
    32.0f, 0.0f, 42.94f, 0.11f,
    40.0f, 0.0f, 44.0f, -0.02f,
    48.0f, 0.0f, 45.03f, -0.17f,
    56.0f, 0.01f, 46.06f, -0.34f,
    64.0f, 0.03f, 47.11f, -0.53f,
    72.0f, 0.06f, 48.16f, -0.74f,
    80.0f, 0.09f, 49.22f, -0.96f,
    88.0f, 0.13f, 50.28f, -1.2f,
    96.0f, 0.19f, 51.34f, -1.45f,
    104.0f, 0.26f, 52.4f, -1.7f,
    112.0f, 0.33f, 53.45f, -1.97f,
    120.0f, 0.42f, 54.49f, -2.24f,
    128.0f, 0.52f, 55.52f, -2.51f,
    136.0f, 0.63f, 56.54f, -2.79f,
    144.0f, 0.75f, 57.54f, -3.06f,
    152.0f, 0.87f, 58.52f, -3.34f,
    160.0f, 1.01f, 59.49f, -3.61f,
    168.0f, 1.15f, 60.43f, -3.88f,
    176.0f, 1.31f, 61.34f, -4.14f,
    184.0f, 1.46f, 62.23f, -4.4f,
    192.0f, 1.62f, 63.08f, -4.65f,
    200.0f, 1.79f, 63.9f, -4.88f,
    208.0f, 1.95f, 64.69f, -5.11f,
    216.0f, 2.11f, 65.44f, -5.33f,
    224.0f, 2.27f, 66.15f, -5.53f,
    232.0f, 2.43f, 66.82f, -5.72f,
    240.0f, 2.58f, 67.44f, -5.9f,
    248.0f, 2.72f, 68.01f, -6.06f,
    256.0f, 2.86f, 68.54f, -6.2f,
    264.0f, 2.98f, 69.02f, -6.33f,
    272.0f, 3.09f, 69.44f, -6.44f,
    280.0f, 3.19f, 69.8f, -6.53f,
    288.0f, 3.27f, 70.11f, -6.6f,
    296.0f, 3.33f, 70.36f, -6.65f,
    304.0f, 3.37f, 70.55f, -6.68f,
    312.0f, 3.4f, 70.67f, -6.69f,
    320.0f, 3.39f, 70.73f, -6.67f,
    328.0f, 3.36f, 70.75f, -6.6f,
    336.0f, 3.3f, 70.73f, -6.49f,
    344.0f, 3.22f, 70.67f, -6.35f,
    352.0f, 3.12f, 70.58f, -6.17f,
    360.0f, 2.99f, 70.45f, -5.96f,
    368.0f, 2.85f, 70.3f, -5.71f,
    376.0f, 2.7f, 70.12f, -5.45f,
    384.0f, 2.53f, 69.92f, -5.16f,
    392.0f, 2.35f, 69.7f, -4.85f,
    400.0f, 2.17f, 69.46f, -4.53f,
    408.0f, 1.99f, 69.21f, -4.19f,
    416.0f, 1.8f, 68.95f, -3.85f,
    424.0f, 1.62f, 68.68f, -3.49f,
    432.0f, 1.43f, 68.4f, -3.14f,
    440.0f, 1.26f, 68.12f, -2.79f,
    448.0f, 1.08f, 67.85f, -2.44f,
    456.0f, 0.92f, 67.58f, -2.1f,
    464.0f, 0.77f, 67.31f, -1.77f,
    472.0f, 0.62f, 67.06f, -1.46f,
    480.0f, 0.49f, 66.83f, -1.17f,
    488.0f, 0.38f, 66.61f, -0.9f,
    496.0f, 0.27f, 66.41f, -0.66f,
    504.0f, 0.18f, 66.24f, -0.45f,
    512.0f, 0.11f, 66.1f, -0.27f,
    520.0f, 0.06f, 65.99f, -0.14f,
    528.0f, 0.02f, 65.91f, -0.05f,
    536.0f, 0.0f, 65.88f, -0.0f,
    540.0f, 0.0f, 65.87f, 0.0f,
    720.0f, 0.0f, 65.87f, 0.0f,
    -1.0f, 0.0f, 65.87f, 0.0f,
};

/* The unit's `.bss`: three point/step vector pairs the static initializer seeds.  Names are GUESSes. */
nw4r::math::VEC3 em032_vec_pair_0[2];
nw4r::math::VEC3 em032_vec_pair_1[2];
nw4r::math::VEC3 em032_vec_pair_2[2];
