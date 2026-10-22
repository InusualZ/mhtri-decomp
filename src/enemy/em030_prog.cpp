/* enemy/em030_prog.cpp - the em030 enemy's program: the `em030_prog_tbl` entry points (init, frame update, action
 *   dispatch, event, motion events, state query, item exits, area start), its item-steal latch, the per-action
 *   state steps the `state_sub` dispatchers select, the two sound programs, the effect router and the static
 *   initializer of its vector pair.
 * RANGE. .text 0x801B0010-0x801B4348 (58 functions); .ctors 0x8056F344-0x8056F348, .data 0x805B0FD0-0x805B17A8,
 *   .bss 0x806A7A88-0x806A7AA0, .sdata2 0x80798B38-0x80798C68, extab, extabindex.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `#pragma pool_data off` and `#pragma fp_contract off` file-wide.
 * NAMES. `em030_prog` from the map's `em030_prog_tbl` (0x805B0FD0, the first object of the unit's `.data`, whose
 *   slots are this range's entry points); `em030_condition_ck` and `em030_homing_range_ck` are runtime-dump names.
 *   GUESS (the `Em030ProgTbl` slot each fills): em030_init, em030_frame_update, em030_action_dispatch, em030_noop,
 *   em030_event_notify, em030_motion_events, em030_tev_color_update, em030_state_ck, em030_item_return_exit,
 *   em030_item_drop_exit, em030_area_start_set.
 *   GUESS (dispatcher slot): em030_act<action>_<first sub-state> and em030_act<action>_dispatch,
 *   em030_act10_se_play, em030_act11_se_play; GUESS (from the bodies): em030_target_gone_ck, em030_item_steal/_return/_drop, em030_flag_cycle_40/_10, em030_flag_ring_update,
 *   em030_torch_near_ck, em030_large_monster_find, em030_act1_mot, em030_act2_approach, em030_act2_circle,
 *   em030_leap_start, em030_effect_spawn.
 *   GUESS (data): em030_se_tbl_<sub-state>, em030_se_tbl_alt_<sub-state>, em030_se_rec_<table>_<letter>,
 *   em030_se_branch_00_b, em030_vec_pair_0, the `Em030ProgTbl` slot names.
 * RESIDUALS. 5 rows short, every data section byte-identical:
 *   `em030_target_gone_ck`, `em030_torch_near_ck` - retail forms `&ainpc_w` (`addi`) right after its `lis`, ours
 *   only before the first field read through it; `em030_act2_circle` - the angle word is reloaded into r3 where
 *   ours uses r0; `em030_act7_0` - the motion choice `state_0x006 == 1 ? 0x0F : 0x10` takes r4/r5 where retail
 *   takes r3/r4; `em_area_engaged_ck` - retail lays the team 1/2 block before the 5/7 switch.
 *   flipcheck: the callees `em001_act13_early_ck` and `em_team_area_alert_set_near` are the name
 *   registry's proposals (lane enemy-small3), not map names yet.
 * SHAPES. A group of consecutive cases retail range-tests (`subi` + `cmplwi`) is an `if ((u32)(x - lo) > n)`
 *   around the switch of the others; `(x & 0x1F) | 0x10` under peephole off is two compound statements (one
 *   expression keeps an extra `clrlwi`); the two flag cycles keep the peephole on.  The sound data is defined
 *   between the functions in retail order (the three action dispatchers' jump
 *   tables come before it, the motion-events table after it).  The `.bss` vector pair is built by MWCC's own
 *   `__sinit` from `Em030VecPair`'s constructor.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h" /* em_area_engaged_ck (rule 2: the owner's header) */
#include "enemy/em005_act.h"
#include "enemy/em007_act.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_801251D0.h" /* EmGroundRec + fn_80125F54 (rule 1/2: their owner) */
#include "ai/ainpc.h"   /* `_AINPC_W` (rule 1) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/ai_npc.cpp (rule 2) */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_80137604.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "unsplit/enemy.h"
#include "Pl/fn_80262940.h" /* pl_model_state_set (rule 2: its owner's header) */
#include "mh3_pad.h"
#include "quest/quest_item_slot.h" /* quest_item_work_merge (rule 2: its owner) */
#include "ai/ai_torch_ck.h"
#include "enemy/em_model.h" /* em_drop_item_spawn (rule 2) */
#include "enemy/em_team_area_alert_set.h"
#include "enemy/fn_801502C8.h" /* em001_act13_early_ck (rule 2: its owner's header) */
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "stage/shell_set_func_ptr.h"
#include "ef/eft009.h"
#include "ef/fn_8010D1A8.h"
#include "ef/fn_80105314.h"
#include "ef/cp_vector.h"
#include "enemy/em_common.h"
#include "ef/get_move_work_adrs.h"
#include "g3d/g3d_calcworld.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#define em_pl_target_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, void*))em_pl_target_ck)
#define get_move_work_max_c1 ((u16 (*)(u8))get_move_work_max)
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)

/* ------------------------------------------------------------------------------------------------ */
/* declarations this unit needs whose owner's header does not carry them yet                          */
/* ------------------------------------------------------------------------------------------------ */

/* 0x8029F6DC: the item-table accessor `menu/menu_item.cpp` defines (`GetItemData__FUs`), declared with this
 * unit's view of the row it returns (the return type is not part of the mangling). */
/* size: 0x4 */
struct EmItemRow {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 rank_0x01; /* the "kind" byte the em030 picker filters on (`< 3`) */
    /* +0x02 */ u8 unused_0x02[2];
};
EmItemRow* GetItemData(u16 id);

/* The 0x20-byte ground record `em_ground_rec_clear` prepares and `em_ground_rec_find` fills is `EmGroundRec`
 * (`enemy/ENEMY_WORK.h`); this unit passes its address and reads its `pos_0x08`. */


/* `calcVecDistXZ` (the distance between two positions) and `subVec3` come from `fn_8004CAD8.h`. */

/* A point and its per-frame step, built by MWCC's `__sinit` (0x801B42DC): each vector is set through `setVec3` into
 * a temporary and copied with `assignVec3`.  size: 0x18 */
struct Em030VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, y, z) and (0, dy, dz). */
    Em030VecPair(f32 y, f32 z, f32 dy, f32 dz)
    {
        VEC3 b;
        VEC3 a;

        assignVec3((Vec*)&pos, (Vec*)setVec3(&a, 0.0f, y, z));
        assignVec3((Vec*)&step, (Vec*)setVec3(&b, 0.0f, dy, dz));
    }
};

/* The vector pair (`.bss`, defined at the end); three sound records point at it. */
extern Em030VecPair em030_vec_pair_0;

extern "C" {
void em030_init(_ENEMY_WORK* work, u8 kind);
void em030_frame_update(_ENEMY_WORK* work);
void em030_action_dispatch(_ENEMY_WORK* work);
void em030_noop(void);
void em030_event_notify(_ENEMY_WORK* work, u32 state, u32 kind);
void em030_motion_events(_ENEMY_WORK* work);
void em030_tev_color_update(void);
u32 em030_state_ck(_ENEMY_WORK* work, u32 kind);
u32 em030_item_return_exit(_ENEMY_WORK* work);
void em030_item_drop_exit(_ENEMY_WORK* work);
void em030_area_start_set(_ENEMY_WORK* work, u8* state, u8* flag);
}

/* The em030 program table (`em030_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em012ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em030ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* work, u8 kind);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* work);
    /* +0x0C */ void (*action_dispatch_0x0C)(_ENEMY_WORK* work);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* work, u32 state, u32 kind);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* work);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ u32 (*state_ck_0x24)(_ENEMY_WORK* work, u32 kind);
    /* +0x28 */ u32 (*item_return_exit_0x28)(_ENEMY_WORK* work);
    /* +0x2C */ void (*item_drop_exit_0x2C)(_ENEMY_WORK* work);
    /* +0x30 */ void (*area_start_set_0x30)(_ENEMY_WORK* work, u8* state, u8* flag);
    /* +0x34 */ u32 pad_0x34[14];
};

extern "C" Em030ProgTbl em030_prog_tbl = {
    0,
    em030_init,
    em030_frame_update,
    em030_action_dispatch,
    em030_noop,
    em030_event_notify,
    em030_motion_events,
    0,
    em030_tev_color_update,
    em030_state_ck,
    em030_item_return_exit,
    em030_item_drop_exit,
    em030_area_start_set,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

/* ------------------------------------------------------------------------------------------------ */
/* the range's own view of the player work it is handed                                              */
/* ------------------------------------------------------------------------------------------------ */

/* `_PLW` comes from `pl.h`; this range reads only its slot table at +0x278 (`_SLOTENT`, the `item_id` half
 * read as a 16-bit id) and the liveness byte at +0x000. */

/* ------------------------------------------------------------------------------------------------ */
/* bodies                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801B0010 (0xFC): whether any live kind-3 work record in `area` is engaged, asking the per-team checker
 * (cases 1/2 the shared one, 5 and 7 their own) for each 0xB18-byte record. */
extern "C" u32 em_area_engaged_ck(u32 area) {
    _ENEMY_WORK* work;
    u16 max;
    s32 i;
    u8 team;

    max = get_move_work_max(3);
    work = (_ENEMY_WORK*)get_move_work_adrs(3);

    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->area_no == (u8)area) {
            team = work->team;
            if ((u32)(team - 1) > 1) {
                switch (team) {
                case 5:
                    if (em005_act13_early_ck(work) == 1) {
                        return 1;
                    }
                    break;
                case 7:
                    if (em007_act13_early_ck(work) == 1) {
                        return 1;
                    }
                    break;
                }
            } else if (em001_act13_early_ck(work) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801B010C (0x1C).  The em030 program's "condition" hook: the action 7 branch reports the record's
 * +0x330 byte, every other action reports -1. */
s32 em030_condition_ck(_ENEMY_WORK* work) {
    if (work->action != 7) {
        return -1;
    }
    return work->field_0x330;
}

/* 0x801B0128 (0x40).  The em030 program's homing-range hook: the work is in homing range while its
 * distance to its own +0x36C target point is inside the squared 200-unit radius. */
u8 em030_homing_range_ck(_ENEMY_WORK* work) {
    return vec3_dist_sq(&work->pos, &work->vec_0x36C) < 40000.0f;
}

/* 0x801B0168 (0xC8).  The em030 program's action guard: state 0 always runs, state 1 runs only while
 * the game-state block's area is still the work's area. */
extern "C" u32 em030_target_gone_ck(_ENEMY_WORK* work) {
    switch (work->field_0x380) {
    case 1:
        if (work->field_0x382 != 0xFF) {
            _ENEMY_WORK* target = (_ENEMY_WORK*)em_move_work_pick(work->state_0x381, work->field_0x382);
            if (target->active) {
                if (em_pl_target_ck_c1(work, target) == 1) {
                    return 0;
                }
            }
        }
        return 1;
    case 2:
        if (work->field_0x382 != 0xFF && ainpc_w.active != 0 && work->area_no == ainpc_w.field_0x1A4) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x801B0230 (0x1B8): picks the player's counter-move (skills 0x238 then 0xD9, else the highest-value usable
 * slot) and latches it on the work record with the player work it came from. */
extern "C" u32 em030_item_steal(_ENEMY_WORK* work, _PLW* plw) {
    if (plw == NULL) {
        return 0;
    }
    if (plw->slot_active == 0) {
        return 0;
    }
    if (Pl_master_ck(plw) != 1) {
        return 0;
    }
    if (Pl_motion_input_ck(0) == 1) {
        return 0;
    }
    if (Pl_Skill_ck(plw, 0x5A) == 1) {
        return 0;
    }
    if (Pl_item_timer_get(plw, 0x238) > 0) {
        work->item_0x328 = 0x238;
    } else if (Pl_item_timer_get(plw, 0xD9) > 0) {
        work->item_0x328 = 0xD9;
    } else {
        u8 usable[0x18];
        u8 i;
        u8 count = 0;

        for (i = 0; i < 0x18; i++) {
            u16 id = plw->slot_id[i].item_id;
            if (id != 0 && id != 0xDF && id != 0x2B) {
                if (GetItemData(id)->rank_0x01 < 3) {
                    usable[count] = i;
                    count++;
                }
            }
        }
        if (count == 0) {
            return 0;
        }
        work->item_0x328 = plw->slot_id[usable[(u8)(ran_suu(0) % count)]].item_id;
    }
    work->plw_0x32C = plw;
    pl_item_add(plw, work->item_0x328, -1);
    pl_model_state_set(plw, 2, 0x1B, work->item_0x328);
    return 1;
}

/* 0x801B03E8 (0x68).  Cancels the move `em030_item_steal` latched: the work record's item 1, then the
 * player's 0x1C command. */
extern "C" void em030_item_return(_ENEMY_WORK* work) {
    if (work->item_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    pl_item_add(work->plw_0x32C, work->item_0x328, 1);
    pl_model_state_set(work->plw_0x32C, 2, 0x1C, work->item_0x328);
    work->item_0x328 = 0;
    work->plw_0x32C = NULL;
}

/* 0x801B0450 (0x54).  The teardown of the same latch (the player's 0xFF command, no item restore). */
extern "C" void em030_item_drop(_ENEMY_WORK* work) {
    if (work->item_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    quest_item_work_merge(work->plw_0x32C, work->item_0x328, -1);
    work->item_0x328 = 0;
    work->plw_0x32C = NULL;
}

#pragma peephole on
/* 0x801B04A4 (0x5C).  Arms the 0x20 "condition" bit of `field_0x761` in state 0 and the 0x40 one in
 * state 1, then toggles `field_0x333`; returns 1 only when the state-1 branch ran. */
extern "C" u32 em030_flag_cycle_40(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 += 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}
#pragma peephole off

#pragma peephole on
/* 0x801B0500 (0x5C).  The same toggle for the 0x20/0x10 bits. */
extern "C" u32 em030_flag_cycle_10(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 += 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}
#pragma peephole off

/* 0x801B055C (0x178).  The em030 action-state machine's 4-state ring (0 -> 3 -> 1 -> 2 -> 0), plus
 * the motion-id refinement of the 0x40/0x80 bits of `field_0x761`. */
extern "C" void em030_flag_ring_update(_ENEMY_WORK* work) {
    switch (work->field_0x332) {
    case 0:
        if ((work->field_0x761 & 0x10) == 0) {
            work->field_0x761 &= 0x1F;
            work->field_0x761 |= 0x10;
        }
        if (work->field_0x762 == 1) {
            work->field_0x332 = 3;
        }
        break;
    case 1:
        if ((work->field_0x761 & 0x40) == 0) {
            work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        }
        if (work->field_0x762 == 0) {
            work->field_0x332 = 2;
        }
        break;
    case 2:
        if (em030_flag_cycle_10(work) == 1) {
            if (work->field_0x762 == 1) {
                work->field_0x332 = 3;
            } else {
                work->field_0x332 = 0;
            }
        }
        break;
    case 3:
        if (em030_flag_cycle_40(work) == 1) {
            if (work->field_0x762 == 0) {
                work->field_0x332 = 2;
            } else {
                work->field_0x332 = 1;
            }
        }
        break;
    }

    switch ((u16)em_get_mot_no(work)) {
    case 0x14:
    case 0x1E:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        break;
    case 0x27:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
        work->field_0x761 = (work->field_0x761 & 0x8F) | 0x80;
        break;
    }
}

/* 0x801B06D4 (0x13C).  "Is any live area-work record of kind 2 (or the live game-state block's own
 * record) inside the 1500-unit radius?" - `vec3_dist_sq` answers the squared distance. */
extern "C" u32 em030_torch_near_ck(_ENEMY_WORK* work) {
    u8 i;
    u16 max = get_move_work_max_c1(2);
    EmAreaWork* area = (EmAreaWork*)get_move_work_adrs(2);

    for (i = 0; i < max; i++, area++) {
        if (!area->active) {
            continue;
        }
        if (em_pl_target_ck(work, area) == 0) {
            continue;
        }
        if (pl_torch_ck((_ENEMY_WORK*)area) == 0) {
            continue;
        }
        if (vec3_dist_sq(&work->pos, &area->vec_0x3C) <= 2250000.0f) {
            return 1;
        }
    }

    if (ainpc_w.active != 0 && work->area_no == ainpc_w.field_0x1A4) {
        if (ai_torch_ck(&ainpc_w) == 1) {
            if (vec3_dist_sq(&work->pos, &ainpc_w.vec_0x178) <= 2250000.0f) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801B0810 (0xAC).  "Is any live work record of kind 3 in the same area whose +0x1C8 is armed?" -
 * the matching record's position is copied onto the work's +0x1B0. */
extern "C" u32 em030_large_monster_find(_ENEMY_WORK* work) {
    u8 i;
    u16 max = get_move_work_max_c1(3);
    _ENEMY_WORK* other = (_ENEMY_WORK*)get_move_work_adrs(3);

    for (i = 0; i < max; i++, other++) {
        if (!other->active) {
            continue;
        }
        if ((other->field_0x1C8 & 1) == 0) {
            continue;
        }
        if (other->area_no != work->area_no) {
            continue;
        }
        copyVec3(&work->aim, &other->pos);
        return 1;
    }
    return 0;
}

/* 0x801B08BC (0xAC).  The em030 program's reset/exit hook: the action-2 teardown runs the
 * motion helpers, then the record's em030 block is cleared and the "active" bit re-armed. */
extern "C" void em030_init(_ENEMY_WORK* work, u8 kind) {
    switch (kind) {
    case 2:
        em_fall_height_get(work);
        em_fall_start(work);
        em_act_arm_unless_down(work, 0x0C, 0);
        em_state_refresh(work);
        break;
    }
    work->item_0x328 = 0;
    work->plw_0x32C = NULL;
    work->field_0x330 = 0;
    work->field_0x331 = 0;
    work->field_0x332 = 0;
    work->field_0x333 = 0;
    work->field_0x334 = 0;
    work->field_0x761 &= 0x1F;
    work->field_0x761 |= 0x10;
    if (work->team == 0x1E || work->field_0x00A == 1) {
        work->field_0x1C8 |= 0x80;
    }
}

/* 0x801B0968 (0x4).  The empty slot `em030_prog_tbl` keeps for the unused state. */
extern "C" void em030_noop(void) {}

/* 0x801B096C (0xBC).  The em030 per-state hook: the states 10/11 branch on the motion kind, state 1
 * dispatches on the sub-kind. */
extern "C" void em030_event_notify(_ENEMY_WORK* work, u32 state, u32 kind) {
    if ((u32)((u8)state - 10) > 1) {
        switch ((u8)state) {
        case 1:
            switch ((u8)kind) {
            case 0x13:
                if (work->field_0x43B == 2 && work->field_0x43C == 1) {
                    em_motion_kind_set(work, 4, 0);
                }
                break;
            case 0x10:
                if (work->field_0x43B == 3) {
                    em_motion_kind_set(work, 0, 0);
                }
                break;
            }
            break;
        }
    } else if (work->field_0x331 == 1 && em_break_state_ck(work) == 0) {
        work->field_0x331 = 0;
    }
}

/* 0x801B0A28 (0x32C).  The em030 program's per-frame update: the death/approach checks, the
 * per-state action block, the shared tail flags and the +0x761 bit updates. */
extern "C" void em030_frame_update(_ENEMY_WORK* work) {
    u32 zero = 0;

    work->field_0x330 = zero;
    if (em_die_ck(work) == 0) {
        if (work->field_0x334 == 0) {
            if (em030_large_monster_find(work) == 1) {
                if (work->field_0x43B != 1) {
                    em_motion_kind_set(work, 3, 0);
                }
                work->field_0x334 = 1;
            }
        } else {
            if (em030_large_monster_find(work) == 0) {
                work->field_0x334 = zero;
            }
        }
        if (work->field_0xA07 == 1) {
            if (work->field_0x43B == 2 && work->field_0x43C == 0) {
                em_motion_kind_set(work, 2, 1);
            }
            work->field_0xA07 = 0;
        }
    }

    switch (work->field_0x43B) {
    case 0:
        if (work->action != 0x0A && em_die_ck(work) == 0 && em030_torch_near_ck(work) == 1) {
            em_motion_kind_set(work, 2, 0);
        }
        break;
    case 2:
        switch (work->field_0x43C) {
        case 0:
            if (work->field_0x833 == 1) {
                em_team_area_alert_set_near(0x1E, work->area_no, &work->pos, 2000.0f);
                em_team_area_alert_set_near(0x1F, work->area_no, &work->pos, 2000.0f);
                em_motion_kind_set(work, 2, 1);
            } else {
                if (em030_torch_near_ck(work) == 0) {
                    em_motion_kind_set(work, 0, 0);
                }
            }
            break;
        case 1:
            if (em_die_ck(work) == 0 && work->field_0x440 > 300) {
                em_motion_kind_set(work, 4, 0);
            }
            break;
        }
        break;
    case 4:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                em_motion_kind_set(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                em_motion_kind_set(work, 0, 0);
            }
        }
        break;
    case 3:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                em_motion_kind_set(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                em_motion_kind_set(work, 0, 0);
            }
        }
        break;
    }

    if (work->field_0x00A != 1 && work->field_0x833 == 1) {
        em_team_area_alert_set(0x1E, work->area_no);
        em_team_area_alert_set(0x1F, work->area_no);
    }
    if (em_break_state_ck(work) == 1 && work->field_0x331 == 0) {
        work->field_0x331 = 1;
    }
    if (work->field_0x331 == 1) {
        if ((work->field_0x761 & 0x01) == 0) {
            work->field_0x761 |= 0x01;
        }
    } else {
        if ((work->field_0x761 & 0x01) != 0) {
            work->field_0x761 &= 0xFE;
        }
    }
    em030_flag_ring_update(work);
}

/* 0x801B0D54 (0x90).  The two-step approach action: arm the motion, then wait for `em_mot_end_ck`. */
extern "C" void em030_act0_0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set_ck(work, em_break_state_ck(work) == 1 ? 0x24 : 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0DE4 (0x30).  The shared step of the three "approach" actions: all three states run the same
 * 0x801B0D54 step (the tail calls are per case, as the target's three `b` instructions show). */
extern "C" void em030_act0_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em030_act0_0(work);
        break;
    case 1:
        em030_act0_0(work);
        break;
    case 2:
        em030_act0_0(work);
        break;
    }
}

/* 0x801B0E14 (0xEC).  The kinded approach action: state 0 arms the motion the kind selects (the
 * six variants 0x14..0x21, default 0x13), state 1 waits for it. */
extern "C" void em030_act1_mot(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;

            switch ((u8)kind) {
            default:
                motion = 0x13;
                break;
            case 1:
                motion = 0x14;
                break;
            case 2:
                motion = 0x15;
                break;
            case 3:
                motion = 0x1A;
                break;
            case 4:
                motion = 0x1B;
                break;
            case 5:
                motion = 0x20;
                break;
            case 6:
                motion = 0x21;
                break;
            }
            em_mot_set(work, motion, 4, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0F00 (0xBC).  The motion-0x16 approach: arm it with the 0x78-frame countdown, run it down,
 * then arm the 0x19/6 recovery. */
extern "C" void em030_act1_3(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x16, 4, 0);
        work->timer_0x020 = 0x78;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 6, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0FBC (0xBC).  The motion-0x17 approach, 0x96 frames, then the 0x19/6 recovery with the 4 flag. */
extern "C" void em030_act1_4(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x17, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 6, 4);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1078 (0xBC).  The motion-0x18 approach, 0x96 frames, then the 0x19/4 recovery. */
extern "C" void em030_act1_5(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1134 (0x90).  The motion-0x1C approach, its countdown seeded from the record's 5-bit
 * `bits_0x1EC` field. */
extern "C" void em030_act1_8(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1C, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x3C;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B11C4 (0x150).  The motion-0x1D approach, whose countdown and follow-up depend on the kind. */
extern "C" void em030_act1_9(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1D, 4, 0);
        if ((u8)kind == 1) {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        } else {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0xFA;
        }
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            if ((u8)kind == 1) {
                work->state += 1;
                em_mot_set(work, 0x1E, 4, 0);
            } else {
                work->state += 2;
                em_mot_set(work, 0x1F, 4, 0);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em_mot_set(work, 0x1F, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1314 (0x90).  The motion-0x24 approach with the 0x96/6 recovery. */
extern "C" void em030_act1_12(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x24, 6, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B13A4 (0x1A4): the motion-0x25 approach; the kind picks the countdown (0x96/0x5A/0x32) and the follow-up
 * on expiry (kind 1 the 0x26 motion plus the position helper, otherwise the 0x67 one). */
extern "C" void em030_act1_13(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x25, 0, 0);
        switch ((u8)kind) {
        case 0:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
            break;
        case 1:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x5A;
            break;
        case 2:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x32;
            break;
        }
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            switch ((u8)kind) {
            case 1:
                work->state += 1;
                em_mot_set(work, 0x26, 0, 0);
                em_drop_item_spawn(work, &work->pos, 0x64);
                break;
            case 2:
                work->field_0x331 = 0;
                /* fall through */
            default:
                work->state += 2;
                em_mot_set(work, 0x67, 4, 0);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em_mot_set(work, 0x67, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1548 (0xB0).  The motion-0x8 retreat: the frame window drives the two scene helpers, and the
 * motion's end cancels the latch and finishes the work. */
extern "C" void em030_act1_14(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x08, 4, 0);
        break;
    case 1:
        if (em_frame_check(work, 1, 144.0f, 0.0f) == 1) {
            em_motion_timer_arm(work);
            em_fx_flag_set(work);
        }
        if (em_mot_end_ck(work) == 1) {
            em030_item_drop(work);
            em_attack_done_set(work);
        }
        break;
    }
}

/* 0x801B15F8 (0xAC): the action dispatcher; `state_sub` selects one of the 20 `jumptable_805B103C` entries, and
 * the `em030_act1_mot` entries carry their kind in r4. */
extern "C" void em030_act1_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em030_act1_mot(work, 0);
        break;
    case 1:
        em030_act1_mot(work, 1);
        break;
    case 2:
        em030_act1_mot(work, 2);
        break;
    case 3:
        em030_act1_3(work);
        break;
    case 4:
        em030_act1_4(work);
        break;
    case 5:
        em030_act1_5(work);
        break;
    case 6:
        em030_act1_mot(work, 3);
        break;
    case 7:
        em030_act1_mot(work, 4);
        break;
    case 8:
        em030_act1_8(work);
        break;
    case 9:
        em030_act1_9(work, 0);
        break;
    case 10:
        em030_act1_mot(work, 5);
        break;
    case 11:
        em030_act1_mot(work, 6);
        break;
    case 12:
        em030_act1_12(work);
        break;
    case 13:
        em030_act1_13(work, 0);
        break;
    case 14:
        em030_act1_14(work);
        break;
    case 15:
        em030_act1_13(work, 2);
        break;
    case 16:
        em030_act1_3(work);
        break;
    case 17:
        em030_act1_9(work, 1);
        break;
    case 18:
        em030_act1_13(work, 1);
        break;
    case 19:
        em030_act1_mot(work, 3);
        break;
    }
}

/* 0x801B16A4 (0x170): the knockback/recovery action; the kind picks the motion and blend scale, the flag the
 * countdown, and state 1 re-runs the 0x800-flag check and clamps the countdown once the hit is over. */
extern "C" void em030_act2_approach(_ENEMY_WORK* work, u32 kind, u32 flag) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;
            f32 scale;

            if ((u32)((u8)kind - 2) > 1) {
                switch ((u8)kind) {
                case 0:
                    motion = 2;
                    scale = -100.0f;
                    break;
                case 1:
                    motion = 3;
                    scale = -100.0f;
                    break;
                }
            } else {
                motion = 3;
                scale = -300.0f;
            }
            em_mot_set(work, motion, 4, 0);
            if ((u8)kind == 3) {
                em_mot_speed_set(work, 0.7f);
            }
            em_approach_start(work, scale, 0);
        }
        if ((u8)flag == 1) {
            work->timer_0x020 = 0x3C;
        } else {
            work->timer_0x020 = 0x96;
        }
        break;
    case 1:
        if (em_approach_step(work, 0, 0x800) == 1 || --work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        if (work->timer_0x020 > 0x0F) {
            if (em030_target_gone_ck(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    }
}

/* 0x801B1814 (0x144): the motion-0x18/0x05 recovery; the 0x100-flag check (or the 0x12C-frame countdown) picks
 * the 0x19 tail, and the countdown is clamped to 0x0F once the hit is over. */
extern "C" void em030_act2_2(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em_mot_set(work, 0x05, 4, 0);
            em_approach_start(work, -200.0f, 0);
            work->timer_0x020 = 0x12C;
        }
        break;
    case 2:
        if (em_approach_step(work, 0, 0x100) == 1 || --work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        if (work->timer_0x020 > 0x0F) {
            if (em030_target_gone_ck(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1958 (0xE8).  The motion-0x06 stagger: the 0x800-flag check (or the 0x12C-frame countdown)
 * picks the 0x19 tail. */
extern "C" void em030_act2_3(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        em_approach_start(work, -100.0f, 0);
        work->timer_0x020 = 0x12C;
        break;
    case 1:
        if (em_approach_step(work, 0, 0x800) == 1 || --work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1A40 (0xF0): the motion-0x06 turn; the 4-bit `bits_0x1EC` field is latched into `state_0x006` as the
 * turn step, and the second angle steps through `em_angle_approach` while the 0x78-frame countdown runs. */
extern "C" void em030_act2_4(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = 0x78;
        work->state_0x006 = work->bits_0x1EC & 0x0F;
        break;
    case 1:
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x19, 4, 0);
        } else {
            work->field_0x1C0 = em_angle_approach((u16)(work->state_0x006 << 12), (u16)work->field_0x1C0, 0x800);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1B30 (0xA0).  The motion-0x04 recovery: the float-armed `em_ground_ck` check, then the
 * position latch and the three teardown helpers. */
extern "C" void em030_act2_5(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set(work, 0x04, 4, 0);
        break;
    case 1:
        if (em_ground_ck_c1(work, 0.0f) != 0) {
            work->pos.y = work->field_0x20C;
            em_move_mode_set(work, 0);
            em_move_vec2_clr(work);
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1BD0 (0x90).  The motion-0x02 approach: the per-frame `em_flag_0x116_set` step, then the
 * 0x400-flag wait. */
extern "C" void em030_act2_6(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x02, 4, 0);
        em_flag_0x116_set(work);
        break;
    case 1:
        em_flag_0x116_set(work);
        if (em_turn_to_target(work, 0x400) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1C60 (0x94).  The motion-0x22 recovery with the 0x80-flag wait. */
extern "C" void em030_act2_7(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x22, 6, 0);
        em_approach_start(work, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(work, 0, 0x80) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1CF4 (0x194): the motion-0x06 turn-to-target; the angle to the +0x36C target point becomes a per-frame
 * step (clamped at 0) through `em_angle_approach`, and the 0x78-frame countdown picks the 0x19 tail. */
extern "C" void em030_act2_circle(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x78;
        break;
    case 1:
        {
            f32 v = calcVecDistXZ(&work->vec_0x36C, &work->pos) - 1000.0f;
            s32 step;

            if (v < 0.0f) {
                v = 0.0f;
            }
            step = 0x2000 - ((s32)v << 4);
            if (step < 0) {
                step = 0;
            }
            subVec3(&diff, &work->vec_0x36C, &work->pos);
            copyVec3(&vec, &diff);
            calcVecAngXY(&vec, &angA, &angB);
            if ((u8)kind == 1) {
                step = -step;
            }
            work->field_0x1C0 = em_angle_approach((u16)(angB + step), (u16)work->field_0x1C0, 0x800);
            work->timer_0x020--;
            if (work->timer_0x020 <= 0) {
                work->state += 1;
                em_mot_set(work, 0x19, 4, 0);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1E88 (0x7C).  The motion-0x02 recovery. */
extern "C" void em030_act2_12(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x02, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1F04 (0x90): the second action dispatcher (`state_sub` 0..13 through `jumptable_805B108C`); the
 * `em030_act2_approach` entries carry their kind/flag in r4/r5. */
extern "C" void em030_act2_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em030_act2_approach(work, 0, 0);
        break;
    case 1:
        em030_act2_approach(work, 1, 0);
        break;
    case 2:
        em030_act2_2(work);
        break;
    case 3:
        em030_act2_3(work);
        break;
    case 4:
        em030_act2_4(work);
        break;
    case 5:
        em030_act2_5(work);
        break;
    case 6:
        em030_act2_6(work);
        break;
    case 7:
        em030_act2_7(work);
        break;
    case 8:
        em030_act2_circle(work, 0);
        break;
    case 9:
        em030_act2_circle(work, 1);
        break;
    case 10:
        em030_act2_approach(work, 1, 1);
        break;
    case 11:
        em030_act2_approach(work, 2, 0);
        break;
    case 12:
        em030_act2_12(work);
        break;
    case 13:
        em030_act2_approach(work, 3, 0);
        break;
    }
}

/* 0x801B1F94 (0xA8): the "armed" motion; the two teardown helpers, the 0x0E motion, the position helper, and the
 * +0x310 angle triple the kind arms before `rotVecY` turns it by the second angle. */
extern "C" void em030_leap_start(_ENEMY_WORK* work, u32 kind) {
    em_fall_height_get(work);
    em_fall_start(work);
    em_mot_set(work, 0x0E, 0, 0);
    em_hit_window_set_default(work, 0, 1);
    em_move_vec2_clr(work);
    work->offset_0x30C.vec_0x310.y = 30.0f;
    if ((u8)kind == 1) {
        work->offset_0x30C.vec_0x310.z = 15.0f;
    } else {
        work->offset_0x30C.vec_0x310.z = 5.0f;
    }
    work->field_0x320 = -3.0f;
    rotVecY(&work->offset_0x30C.vec_0x310, work->field_0x1C0);
}

/* 0x801B203C (0x25C): the kinded recovery/counter action; state 0 arms the kind's motion, state 2 runs the
 * counter latch (`em030_item_steal` on the player work at +0xA34) and the `UpdateValue` wait, state 3 finishes it. */
extern "C" void em030_act7_0(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state_0x006 = 0;
        switch ((u8)kind) {
        default:
            work->state += 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0x0D, 4, 0);
            break;
        case 1:
            work->state += 2;
            em030_leap_start(work, 0);
            break;
        case 2:
            work->state += 2;
            em030_leap_start(work, 1);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em030_leap_start(work, 1);
        }
        break;
    case 2:
        CancelFade(work);
        if ((u8)kind == 2 && work->state_0x006 == 0 && work->field_0xA16 != 0xFF && work->field_0xA16 != 5
            && work->field_0xA38 == 0) {
            if (em030_item_steal(work, work->plw_0xA34) == 1) {
                em_motion_kind_set(work, 1, 0);
            }
            em_hit_window_clear(work, 0);
            work->state_0x006 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_fall_height_get(work);
            if (UpdateValue(work) != 0) {
                u16 motion;

                work->state += 1;
                em_move_mode_set(work, 0);
                if (work->field_0xA16 != 0xFF && work->field_0xA16 != 5 && work->field_0xA38 == 0) {
                    work->state_0x006 = 1;
                }
                if ((u8)kind == 1) {
                    motion = 0x10;
                } else {
                    motion = work->state_0x006 == 1 ? 0x0F : 0x10;
                }
                em_mot_set(work, motion, 0, 0);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            if ((u8)kind == 2) {
                em_action_finish(work);
            } else if (work->state_0x006 == 1) {
                em_action_finish(work);
            } else {
                em_state_set(work, 1, 1);
            }
        }
        break;
    }
}

/* 0x801B2298 (0x27C): the kinded turn action; state 0 latches the target angle (`calcVecAngXY`), state 1 waits
 * or steps the second angle through `em_angle_approach`, and the team/kind gates pick the follow-up. */
extern "C" void em030_act7_1(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state += 1;
        work->state_0x006 = 0;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        em_hit_window_set(work, 0, 2, 3);
        if ((u8)kind == 1) {
            work->timer_0x020 = 0x78;
            em_mot_speed_set(work, 1.2f);
        } else {
            work->timer_0x020 = 0x3C;
        }
        subVec3(&diff, &work->vec_0x36C, &work->pos);
        copyVec3(&vec, &diff);
        calcVecAngXY(&vec, &angA, &angB);
        work->field_0x37C = angB;
        break;
    case 1:
        if (work->field_0xA16 != 0xFF && work->field_0xA38 == 0) {
            work->state += 1;
            work->state_0x006 = 1;
            em_hit_window_clear(work, 0);
            em_mot_set(work, 0x19, 4, 0);
            break;
        }
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_hit_window_clear(work, 0);
            em_mot_set(work, 0x19, 4, 0);
        } else if ((u8)kind == 1) {
            em_turn_to_target(work, 0x800);
        } else {
            work->field_0x1C0 = em_angle_approach((u16)work->field_0x37C, (u16)work->field_0x1C0, 0x800);
        }
        if (work->timer_0x020 > 0x0F) {
            if (em030_target_gone_ck(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            if (work->team == 0x1F) {
                em_action_finish(work);
            } else if (work->state_0x006 == 1) {
                work->state += 1;
                em_mot_set(work, 0x1C, 4, 0);
            } else {
                em_action_finish(work);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B2514 (0x170).  The motion-0x11/0x12 stance: the two frame windows drive the shell-set
 * callback the kind selects, and the +0x330 latch is armed when the second/third window closes. */
extern "C" void em030_act7_2(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x11, 2, 0);
        work->timer_0x020 = 0x50;
        break;
    case 1:
        if (em_frame_check(work, 0, 20.0f, 0.0f) == 1) {
            if ((u8)kind == 1) {
                shell_set_func_ptr->method_0x58(work, 3);
            } else {
                shell_set_func_ptr->method_0x58(work, 2);
            }
        }
        if (em_frame_check(work, 1, 20.0f, 0.0f) == 1) {
            work->field_0x330 = 1;
        }
        if (--work->timer_0x020 <= 0) {
            work->state += 1;
            em_mot_set(work, 0x12, 4, 0);
        }
        break;
    case 2:
        if (em_frame_check(work, 2, 46.0f, 0.0f) == 1) {
            work->field_0x330 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B2684 (0x108): the stance in motion 17: the shell-set callback (`kind` picks 1 or 0) at frame 20, the
 * +0x330 latch from frame 20 on. */
extern "C" void em030_act7_5(_ENEMY_WORK* work, u8 kind) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x11, 2, 0);
        break;
    case 1:
        if (em_frame_check(work, 0, 20.0f, 0.0f) == 1) {
            if (kind == 1) {
                shell_set_func_ptr->method_0x58(work, 1);
            } else {
                shell_set_func_ptr->method_0x58(work, 0);
            }
        }
        if (em_frame_check(work, 1, 20.0f, 0.0f) == 1) {
            work->field_0x330 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B278C (0xC0): faces the target in motion 7 for up to 450 frames, cut to 15 once the target is gone. */
extern "C" void em030_act7_7(_ENEMY_WORK* work) {
    work->field_0x330 = 1;
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 7, 4, 0);
        work->timer_0x020 = 450;
        break;
    case 1:
        em_turn_to_target(work, 0x400);
        if (--work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        if (work->timer_0x020 > 0xF && em030_target_gone_ck(work) == 1) {
            work->timer_0x020 = 0xF;
        }
        break;
    }
}

/* 0x801B284C (0x74): action 7's sub-states. */
extern "C" void em030_act7_dispatch(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em030_act7_0(work, 0);
        break;
    case 1:
        em030_act7_1(work, 0);
        break;
    case 2:
        em030_act7_2(work, 0);
        break;
    case 3:
        em030_act7_2(work, 1);
        break;
    case 4:
        em030_act7_0(work, 1);
        break;
    case 5:
        em030_act7_5(work, 0);
        break;
    case 6:
        em030_act7_5(work, 1);
        break;
    case 7:
        em030_act7_7(work);
        break;
    case 8:
        em030_act7_1(work, 1);
        break;
    case 9:
        em030_act7_0(work, 2);
        break;
    }
}

/* The sound programs the two sound players run (`.data`; a table carries the first sub-state that plays it). */
extern EmSeBranch em030_se_branch_00_b;

EmSeRecord em030_se_rec_00_a = {0x64, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_00_c = {0x68, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_00_b_a = {0x23, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_00[3] = {
    {0x00, {0, 0, 0}, &em030_se_rec_00_a},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em030_se_branch_00_b)},
    {0x00, {0, 0, 0}, &em030_se_rec_00_c},
};

__declspec(section ".data") EmSeEntry em030_se_tbl_00_b_t[1] = {
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em030_se_tbl_00_b[2] = {
    {0x00, {0, 0, 0}, &em030_se_rec_00_b_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em030_se_tbl_00_b_t)},
};

EmSeBranch em030_se_branch_00_b = {0x03000000, 0, em030_se_tbl_00_b};

EmSeRecord em030_se_rec_17_a = {0x65, 0, 4, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em030_vec_pair_0};

EmSeRecord em030_se_rec_17_b = {0x66, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_17_c = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_17[6] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em030_se_rec_17_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em030_se_rec_17_b},
    {0x00, {0, 0, 0}, &em030_se_rec_17_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_7A_a = {0x6B, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em030_se_tbl_7A[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA8)},
    {0x00, {0, 0, 0}, &em030_se_rec_7A_a},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_7B_a = {0x6B, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em030_se_tbl_7B[2] = {
    {0x01, {0, 0, 0}, &em030_se_rec_7B_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_9F_a = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_9F[2] = {
    {0x00, {0, 0, 0}, &em030_se_rec_9F_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_7C_a = {0x69, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em030_se_tbl_7C[2] = {
    {0x00, {0, 0, 0}, &em030_se_rec_7C_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_8D_a = {0x6A, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_8D_b = {0x68, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_8D[3] = {
    {0x00, {0, 0, 0}, &em030_se_rec_8D_a},
    {0x00, {0, 0, 0}, &em030_se_rec_8D_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_7D_a = {0x6C, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em030_se_tbl_7D[2] = {
    {0x00, {0, 0, 0}, &em030_se_rec_7D_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_8E_a = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_8E[2] = {
    {0x00, {0, 0, 0}, &em030_se_rec_8E_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_A8_a = {0x6C, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_A8_b = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_A8[3] = {
    {0x00, {0, 0, 0}, &em030_se_rec_A8_a},
    {0x00, {0, 0, 0}, &em030_se_rec_A8_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_AF_a = {0x6E, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_AF_b = {0x6F, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_AF[3] = {
    {0x00, {0, 0, 0}, &em030_se_rec_AF_a},
    {0x00, {0, 0, 0}, &em030_se_rec_AF_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_DD_a = {0x6C, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em030_se_rec_DD_b = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_DD[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xF0)},
    {0x00, {0, 0, 0}, &em030_se_rec_DD_a},
    {0x00, {0, 0, 0}, &em030_se_rec_DD_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_DE_a = {0x65, 0, 4, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em030_vec_pair_0};

EmSeRecord em030_se_rec_DE_b = {0x66, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em030_se_rec_DE_c = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em030_se_tbl_DE[7] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em030_se_rec_DE_a},
    {0x04, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xF0)},
    {0x00, {0, 0, 0}, &em030_se_rec_DE_b},
    {0x00, {0, 0, 0}, &em030_se_rec_DE_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_alt_00_a = {0x64, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_00_b = {0x68, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_00_c = {0x8, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_00_d = {0x8, 0, 4, 176, 0, {0, 4, 0, 0}, 0};

EmSeEntry em030_se_tbl_alt_00[8] = {
    {0x00, {0, 0, 0}, &em030_se_rec_alt_00_a},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_00_b},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_00_c},
    {0x0D, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_00_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em030_se_rec_alt_05_a = {0x65, 0, 4, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em030_vec_pair_0};

EmSeRecord em030_se_rec_alt_05_b = {0x66, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_05_c = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_05_d = {0x8, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em030_se_rec_alt_05_e = {0x8, 0, 4, 176, 0, {0, 4, 0, 0}, 0};

EmSeEntry em030_se_tbl_alt_05[11] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_05_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_05_b},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_05_c},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_05_d},
    {0x0D, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em030_se_rec_alt_05_e},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801B28C0 (0x258): action 10 plays the sound program for its sub-state; an unknown one finishes. */
extern "C" void em030_act10_se_play(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0x0:
        em_se_tbl_play(work, em030_se_tbl_00, 0, 0);
        break;
    case 0x17:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 0x17);
        break;
    case 0x5:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 5);
        break;
    case 0x23:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 0x23);
        break;
    case 0x7A:
        em_se_tbl_play(work, em030_se_tbl_7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(work, em030_se_tbl_7B, 0, 0x7B);
        break;
    case 0x7C:
        em_se_tbl_play(work, em030_se_tbl_7C, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(work, em030_se_tbl_8D, 0, 0x8D);
        break;
    case 0x7D:
        em_se_tbl_play(work, em030_se_tbl_7D, 0, 0x7D);
        break;
    case 0x8E:
        em_se_tbl_play(work, em030_se_tbl_8E, 0, 0x8E);
        break;
    case 0x84:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 0x84);
        break;
    case 0x9F:
        em_se_tbl_play(work, em030_se_tbl_9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(work, em030_se_tbl_00, 0, 0xA0);
        break;
    case 0xA8:
        em_se_tbl_play(work, em030_se_tbl_A8, 0, 0xA8);
        break;
    case 0xA9:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 0xA9);
        break;
    case 0xAF:
        em_se_tbl_play(work, em030_se_tbl_AF, 0, 0xAF);
        break;
    case 0xB0:
        em_se_tbl_play(work, em030_se_tbl_17, 1, 0xB0);
        break;
    case 0xDD:
        em_se_tbl_play(work, em030_se_tbl_DD, 0, 0xDD);
        break;
    case 0xDE:
        em_se_tbl_play(work, em030_se_tbl_DE, 1, 0xDE);
        break;
    default:
        em_action_finish(work);
        break;
    }
}

/* 0x801B2B18 (0x54): action 11 plays the sound program for its sub-state. */
extern "C" void em030_act11_se_play(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        em_se_tbl_play_alt(work, em030_se_tbl_alt_00, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt(work, em030_se_tbl_alt_05, 1, 5);
        break;
    default:
        em_se_tbl_play_alt(work, em030_se_tbl_alt_00, 0, 0);
        break;
    }
}

/* 0x801B2B6C (0x1B4): the fall: drops in motion 9 with a short hop, lands in motion 10 and gets up in motions 11
 * and 12. */
extern "C" void em030_act12_0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state += 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set(work, 9, 0, 0);
        em_height_set(work, -78.0f);
        em_move_vec2_clr(work);
        work->offset_0x30C.vec_0x310.y = 12.0f;
        work->vec_0x31C.y = -0.7f;
        break;
    case 1:
        CancelFade(work);
        if (em_frame_check(work, 0, 28.0f, 0.0f) == 1) {
            em_move_vec2_clr(work);
            work->offset_0x30C.vec_0x310.y = 0.0f;
            work->vec_0x31C.y = -2.0f;
        }
        em_fall_height_get(work);
        if (em_ground_ck(work) == 1) {
            work->state += 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0xA, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0xB, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            work->state += 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0xC, 0, 0);
        }
        break;
    case 4:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B2D20 (0x14): action 12's sub-state 0. */
extern "C" void em030_act12_dispatch(_ENEMY_WORK* work) {
    if (work->state_sub == 0) {
        em030_act12_0(work);
    }
}

/* 0x801B2D34 (0x60): runs the current action's dispatcher. */
extern "C" void em030_action_dispatch(_ENEMY_WORK* work) {
    switch (work->action) {
    case 0:
        em030_act0_dispatch(work);
        break;
    case 1:
        em030_act1_dispatch(work);
        break;
    case 2:
        em030_act2_dispatch(work);
        break;
    case 7:
        em030_act7_dispatch(work);
        break;
    case 10:
        em030_act10_se_play(work);
        break;
    case 11:
        em030_act11_se_play(work);
        break;
    case 12:
        em030_act12_dispatch(work);
        break;
    }
}

/* 0x801B2D94 (0x19C): the effect router: `where` 0 spawns at a joint (0xFF: at the work's rotation), 1 on dry
 * ground under it, 2 under it while it is within 900 of the ground. */
extern "C" void em030_effect_spawn(_ENEMY_WORK* work, u8 where, u8 id, u32 joint, s32 delta, f32 scale) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 unused;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused);
    switch (where) {
    case 0:
        if (joint == 0xFF) {
            eft009_set_pos(id, &pos, (_CP_VECTOR*)&work->field_0x1BC, scale, work->area_no);
        } else {
            eft009_spawn_at_joint(work, joint, id, delta, scale);
        }
        break;
    case 1:
        if ((work->field_0x228 & 6) == 0) {
            if (joint == 0xFF) {
                copyVec3(&pos, &work->pos);
            } else {
                get_joint_wpos_em(work, joint, &pos);
            }
            pos.y = work->field_0x20C;
            scale *= get_em_chg_scale(work);
            eft_spawn_pos_in_area(&pos, work->area_no, id, delta, scale);
        }
        break;
    case 2:
        if (joint == 0xFF) {
            copyVec3(&pos, &work->pos);
        } else {
            get_joint_wpos_em(work, joint, &pos);
        }
        pos.y = work->field_0x20C;
        scale *= get_em_chg_scale(work);
        if (work->pos.y <= 900.0f + work->field_0x20C) {
            eft_spawn_type11(work, &pos, id, scale);
        }
        break;
    }
}

/* 0x801B2F30 (0x1310): the per-motion events: the footfall, slide and splash effects (larger in water) and the
 * shake and roar latches. */
extern "C" void em030_motion_events(_ENEMY_WORK* work) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    switch (em_get_mot_no(work)) {
    case 0x6:
        if ((em_after_frame_check(work, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 34.0f, 0.0f) == 1)) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 4, 0, 0.8f);
                break;
            }
            em030_effect_spawn(work, 0, 2, 4, 0, 0.3f);
            break;
        }
        break;
    case 0x8:
        if ((em_after_frame_check(work, 0, 14.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 44.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 74.0f, 0.0f) == 1)) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x16, 8, 0, 0.6f);
            } else {
                em030_effect_spawn(work, 0, 9, 8, 0, 0.6f);
            }
        }
        if ((em_after_frame_check(work, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 88.0f, 0.0f) == 1)) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x16, 0xC, 0, 0.6f);
            } else {
                em030_effect_spawn(work, 0, 9, 0xC, 0, 0.6f);
            }
        }
        if (em_after_frame_check(work, 0, 148.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x16, 3, 0, 1.0f);
                em030_effect_spawn(work, 0, 0x17, 3, 0, 0.8f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 0.8f);
            }
        }
        if (em_after_frame_check(work, 0, 154.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x17, 3, 0, 0.9f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 0.9f);
            }
        }
        if (em_after_frame_check(work, 0, 160.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x16, 4, 0, 1.2f);
                em030_effect_spawn(work, 0, 0x17, 4, 0, 1.0f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 1.0f);
            }
        }
        if (em_after_frame_check(work, 0, 166.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x17, 3, 0, 1.1f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 1.1f);
            }
        }
        if (em_after_frame_check(work, 0, 172.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x16, 3, 0, 1.4f);
                em030_effect_spawn(work, 0, 0x17, 3, 0, 1.2f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 1.2f);
            }
        }
        if (em_after_frame_check(work, 1, 156.0f, 0.0f) == 1) {
            em_shake_req_set(work);
            break;
        }
        break;
    case 0x9:
        if (em_after_frame_check(work, 0, 2.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 4, 0, 0.8f);
                break;
            }
            em030_effect_spawn(work, 0, 2, 4, 0, 0.8f);
            break;
        }
        break;
    case 0xA:
        if (em_after_frame_check(work, 0, 2.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 4, 0, 0.5f);
            break;
        }
        break;
    case 0xF:
        if ((em_after_frame_check(work, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 126.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 160.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 188.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 224.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 288.0f, 0.0f) == 1)) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 4, 0, 1.0f);
                break;
            }
            em030_effect_spawn(work, 0, 9, 4, 0, 0.4f);
            break;
        }
        break;
    case 0x11:
        if (em_after_frame_check(work, 0, 118.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 0x12, 0, 0.6f);
                break;
            }
            em030_effect_spawn(work, 0, 9, 0x12, 0, 0.6f);
            break;
        }
        break;
    case 0x17:
        if (em_after_frame_check(work, 0, 48.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x18:
        if (em_after_frame_check(work, 0, 36.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x19:
        if (em_after_frame_check(work, 0, 36.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 3, 0, 1.0f);
                break;
            }
            em030_effect_spawn(work, 0, 9, 3, 0, 0.4f);
            break;
        }
        break;
    case 0x1A:
        if (em_after_frame_check(work, 0, 80.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 0, 2, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x1B:
        if ((em_after_frame_check(work, 0, 8.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 38.0f, 0.0f) == 1)) {
            get_joint_wpos_em(work, 0xD, &pos);
            pos.y += 10.0f;
            if (work->field_0x228 & 6) {
                eft009_set_pos(0x16, &pos, (_CP_VECTOR*)&work->field_0x1BC, 0.7f, work->area_no);
            } else {
                eft009_set_pos(9, &pos, (_CP_VECTOR*)&work->field_0x1BC, 0.3f, work->area_no);
            }
        }
        if ((em_after_frame_check(work, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 82.0f, 0.0f) == 1)) {
            get_joint_wpos_em(work, 0x10, &pos);
            setVector3(&ofs, 0.0f, 15.0f, -40.0f);
            rotVecY(&ofs, work->field_0x1C0);
            addVec3To(&pos, &ofs);
            if (work->field_0x228 & 6) {
                eft009_set_pos(0x16, &pos, (_CP_VECTOR*)&work->field_0x1BC, 0.3f, work->area_no);
                break;
            }
            eft009_set_pos(9, &pos, (_CP_VECTOR*)&work->field_0x1BC, 0.1f, work->area_no);
            break;
        }
        break;
    case 0x1C:
        if (em_after_frame_check(work, 0, 16.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 0x12, 0, 0.7f);
            } else {
                em030_effect_spawn(work, 0, 9, 0x12, 0, 0.3f);
            }
        }
        if (em_after_frame_check(work, 0, 64.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 0x10, 0, 0.7f);
                break;
            }
            em030_effect_spawn(work, 0, 9, 0x10, 0, 0.3f);
            break;
        }
        break;
    case 0x1D:
        if (em_after_frame_check(work, 0, 28.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x33, 4, 0, 1.0f);
                break;
            }
            em030_effect_spawn(work, 0, 9, 4, 0, 1.0f);
            break;
        }
        break;
    case 0x1F:
        if (em_after_frame_check(work, 0, 38.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 10.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 4, 0x33, 0, 1.0f, &pos);
                break;
            }
            eft009_spawn_at_pos(work, 4, 9, 0, 1.0f, &pos);
            break;
        }
        break;
    case 0x21:
        if ((em_after_frame_check(work, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 48.0f, 0.0f) == 1)) {
            setVector3(&pos, 0.0f, 0.0f, 10.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 4, 0x33, 0, 1.0f, &pos);
                break;
            }
            eft009_spawn_at_pos(work, 4, 9, 0, 0.3f, &pos);
            break;
        }
        break;
    case 0x25:
        if (em_after_frame_check(work, 0, 22.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, -20.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 4, 0x33, 0, 1.0f, &pos);
            } else {
                eft009_spawn_at_pos(work, 3, 9, 0, 0.5f, &pos);
            }
        }
        if (em_after_frame_check(work, 0, 48.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 0xE, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 0xE, 0, 0.4f);
            break;
        }
        break;
    case 0x27:
        if (em_after_frame_check(work, 0, 10.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x17, 4, 0, 1.0f);
            } else {
                em030_effect_spawn(work, 0, 9, 3, 0, 0.4f);
            }
        }
        if (em_after_frame_check(work, 0, 42.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x17, 0xE, 0, 1.0f);
            } else {
                em030_effect_spawn(work, 0, 9, 0xE, 0, 0.4f);
            }
        }
        if (em_after_frame_check(work, 0, 62.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x64:
        if (em_after_frame_check(work, 0, 206.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x66:
        if ((em_after_frame_check(work, 0, 46.0f, 0.0f) == 1) || (em_after_frame_check(work, 0, 92.0f, 0.0f) == 1)) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
            } else {
                em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            }
        }
        if (em_act_ck_c1(work, 0xA, 0xDE) == 1) {
            em_roar_latch_set(work);
            break;
        }
        break;
    case 0x67:
        if (em_after_frame_check(work, 0, 124.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 10.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 3, 0x33, 0, 1.0f, &pos);
                break;
            }
            eft009_spawn_at_pos(work, 3, 9, 0, 1.0f, &pos);
            break;
        }
        break;
    case 0x6A:
        if (em_after_frame_check(work, 0, 88.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x6B:
        if (em_after_frame_check(work, 0, 152.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x6C:
        if (em_after_frame_check(work, 0, 110.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
            } else {
                em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            }
        }
        if (em_act_ck_c1(work, 0xA, 0xDD) == 1) {
            em_roar_latch_set(work);
            break;
        }
        break;
    case 0x6D:
        if (em_after_frame_check(work, 0, 20.0f, 0.0f) == 1) {
            if (work->field_0x228 & 6) {
                em030_effect_spawn(work, 0, 0x35, 3, 0, 0.5f);
                break;
            }
            em030_effect_spawn(work, 1, 7, 3, 0, 0.5f);
            break;
        }
        break;
    case 0x6E:
        if (em_after_frame_check(work, 0, 20.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 10.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 3, 0x33, 0, 1.0f, &pos);
                break;
            }
            eft009_spawn_at_pos(work, 3, 9, 0, 1.0f, &pos);
            break;
        }
        break;
    case 0x6F:
        if (em_after_frame_check(work, 0, 110.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 10.0f);
            if (work->field_0x228 & 6) {
                eft009_spawn_at_pos(work, 3, 0x33, 0, 1.0f, &pos);
                break;
            }
            eft009_spawn_at_pos(work, 3, 9, 0, 0.3f, &pos);
        }
        break;
    }
}


/* 0x801B4240 (0x4).  The empty slot `em030_prog_tbl` keeps. */
extern "C" void em030_tev_color_update(void) {}

extern "C" u32 em030_state_ck(_ENEMY_WORK* work, u32 kind) {
    if ((u8)kind == 0) {
        if (work->field_0x331 == 1) {
            return em_break_state_ck(work) == 1 ? 1 : 2;
        }
        return 0;
    }
    return 0;
}


/* 0x801B42A0 (0x24).  The em030 teardown hook: cancel the latched move and report the exit state. */
extern "C" u32 em030_item_return_exit(_ENEMY_WORK* work) {
    em030_item_return(work);
    return 2;
}

/* 0x801B42C4 (0x4).  The em030 exit hook's tail call. */
extern "C" void em030_item_drop_exit(_ENEMY_WORK* work) {
    em030_item_drop(work);
}

/* 0x801B42C8 (0x14).  The em030 state-report hook: the current state id and a zero flag. */
extern "C" void em030_area_start_set(_ENEMY_WORK* work, u8* state, u8* flag) {
    *state = 0x0C;
    *flag = 0;
}

/* The unit's `.bss`: the vector pair MWCC's `__sinit` (0x801B42DC) seeds; three sound records point at it. */
Em030VecPair em030_vec_pair_0(10.0f, -12.0f, -0.4f, 0.15f); /* 0x806A7A88 */
