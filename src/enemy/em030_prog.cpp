/* enemy/em030_prog.cpp - the em030 enemy's program: its condition checks, action steps, `state_sub` dispatchers and
 *   the static constructor of its two-vector record.
 * RANGE. .text 0x801B0010-0x801B4348 (58 functions); .ctors 0x8056F344-0x8056F348, .data 0x805B0FD0-0x805B17A8,
 *   .bss 0x806A7A88-0x806A7AA0, .sdata2 0x80798B38-0x80798C68, extab, extabindex.
 * NAMES. `em030_prog` from the map's `em030_prog_tbl` (0x805B0FD0, the first object of the unit's `.data`, whose
 *   slots are this range's entry points); `em030_condition_ck` and `em030_homing_range_ck` are runtime-dump names.
 *   The `.bss` record name is a GUESS (a pair of model-space points).
 * RESIDUALS. 10 rows unwritten: 0x801B2684-0x801B4240 (`fn_801B2684` .. `fn_801B2F30`; `fn_801B2D94`/`fn_801B2F30`
 *   are the third and fourth `state_sub` dispatchers, tables at 0x805B11C0 and 0x805B14F0).
 *  - `fn_801B0F00`, `fn_801B0FBC`, `fn_801B1078`, `fn_801B1134`, `fn_801B11C4`, `fn_801B1314`, `fn_801B13A4`,
 *    `fn_801B16A4`, `fn_801B1814`, `fn_801B1958`, `fn_801B1A40`, `fn_801B1CF4`, `fn_801B2298`, `fn_801B2514`: retail
 *    keeps the timer decrement as `subi` + `cmpwi`, ours fuses it into `subic.`;
 *  - `fn_801B0230`, `fn_801B0810`, `fn_801B0A28`, `fn_801B13A4`: retail keeps `clrlwi` + `cmpwi`, ours emits
 *    `clrlwi.`; `fn_801B055C` the same with `rlwinm.`;
 *  - `fn_801B0010`, `fn_801B096C`: the {1,2} / {10,11} case groups are two compares where retail uses the
 *    `(x - 1) <= 1` range test (the `(u32)(team - 1) <= 1` spelling has retail's size but reorders the blocks);
 *  - `fn_801B03E8`, `fn_801B0450`, `fn_801B0230`: retail reads the +0x328 halfword with `lhz`, ours with `lha`;
 *  - `fn_801B06D4`, `fn_801B42DC`: retail's frame is 0x30 against our 0x20 (retail spills f31 with `psq_st`);
 *  - `fn_801B0DE4`: retail switches on the byte at +0x1E6, ours on `state_sub` (+0x5);
 *  - `fn_801B0E14`, `fn_801B203C`: ours emits an extra `b` and orders the case blocks differently; `fn_801B4244`:
 *    ours places the `return 0` block inline, retail keeps it last;
 *  - `fn_801B2514`: ours hoists the `shell_set_func_ptr` slot load out of the two branches retail reloads it in;
 *    `fn_801B1B30`: retail reloads `lbl_80798B48`, ours reuses it;
 *  - `em030_homing_range_ck`, `fn_801B08BC`: retail narrows the `u8` argument with `clrlwi` before the compare;
 *    `fn_801B0168`: retail compares unsigned (`cmplwi`), ours signed.
 *   flipcheck: `.ctors`/`.sdata2` claimed, not emitted; `.data`/`.text`/extab/extabindex short of the claim.
 * SHAPES. `#pragma peephole off` around `fn_801B4244` only (retail keeps its `clrlwi` + `cmpwi` byte test unfused);
 *   every other body keeps the peephole on.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h"
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
/* 0x802D8414: `ai_torch_ck(_AINPC_W*)`, which `ai/ai_npc.cpp` defines. */
struct _AINPC_W;
u32 ai_torch_ck(_AINPC_W* npc);

/* The `void (*)(_ENEMY_WORK*, u32)` the stance action reads out of `shell_set_func_ptr`'s +0x58 slot. */
typedef void (*ShellSetFn)(_ENEMY_WORK* work, u32 id);
/* The block `shell_set_func_ptr` points at (the map types it `void*`); its +0x58 slot is the shell-set
 * function the stance action calls, so it is reached as a field rather than by arithmetic (rule 6). */
/* size: 0x5C */
struct ShellSetBlock {
    /* +0x00 */ u8 unused_0x00[0x58];
    /* +0x58 */ ShellSetFn field_0x58;
};

/* The 0x20-byte ground record `em_ground_rec_clear` prepares and `em_ground_rec_find` fills is `EmGroundRec`
 * (`enemy/ENEMY_WORK.h`); this unit passes its address and reads its `pos_0x08`. */

/* 0x800CFA90 / 0x800CFAD0: the move-work record base and its count (`ef/system_core.cpp`), declared with the
 * owner's own return types: the tree's headers spell the count as `u32` and `u16`, and MWCC refuses two at once. */
void* get_move_work_adrs(u8 index);
u16 get_move_work_max(u8 index);

/* `calcVecDistXZ` (the distance between two positions) and `subVec3` come from `fn_8004CAD8.h`. */

/* The `.sdata2` pool constants this range loads, declared and never defined (playbook 29). */
extern "C" f32 lbl_80798B3C; /* 2250000.0f - the squared 1500-unit radius `vec3_dist_sq` is compared to */
extern "C" f32 lbl_80798B38; /* 40000.0f - the squared 200-unit homing radius */
extern "C" f32 lbl_80798B44; /* 144.0f - the `em_frame_check` window's first bound */
extern "C" f32 lbl_80798B48; /* 0.0f - its second bound */
extern "C" f32 lbl_80798B4C; /* -100.0f - the knockback blend scale */
extern "C" f32 lbl_80798B50; /* -300.0f - the second knockback blend scale */
extern "C" f32 lbl_80798B54; /* 0.7f - the motion blend `em_mot_speed_set` is handed */
extern "C" f32 lbl_80798B58; /* 0.2f - the `fn_801B1814` motion blend */
extern "C" f32 lbl_80798B5C; /* 200.0f - the distance `fn_801B1CF4` subtracts */
extern "C" f32 lbl_80798B60; /* 0.0f - the +0x310 triple component `fn_801B1F94` arms */
extern "C" f32 lbl_80798B64; /* 0.5f - its kind-1 .z */
extern "C" f32 lbl_80798B68; /* 0.7f - its other .z */
extern "C" f32 lbl_80798B6C; /* 0.2f - the +0x320 component it arms */
extern "C" f32 lbl_80798B70; /* 1.2f - the `em_mot_speed_set` blend `fn_801B2298` arms */
extern "C" f32 lbl_80798B74; /* 0.5f - the first `em_frame_check` window bound */
extern "C" f32 lbl_80798B78; /* 0.3f - the second */
/* The `.sdata` shell-set callback pointer the stance action calls through its +0x58 slot. */
extern "C" void* shell_set_func_ptr;
extern "C" f32 lbl_80798B94; /* 100.0f - the first record's y */
extern "C" f32 lbl_80798C58; /* 50.0f - its z */
extern "C" f32 lbl_80798C5C; /* -100.0f - the second record's y */
extern "C" f32 lbl_80798C60; /* -50.0f - its z */
/* The two three-float records the unit's static constructor fills (0x18 bytes of `.bss`). */
extern "C" VEC3 vec_pair_801B0010_0[2];
/* 0x803B9BA0 (`enemy/em_model.cpp`): r3 the work record, r4 the position, r5 the id. */
extern "C" void fn_803B9BA0(_ENEMY_WORK* work, nw4r::math::VEC3* pos, u32 id);
extern "C" f32 lbl_80798B40; /* 2000.0f */

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
extern "C" u32 fn_801B0010(u32 area) {
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 i = 0;

    for (; i < max; i++, work++) {
        if (!work->active) {
            continue;
        }
        if (work->area_no != (u8)area) {
            continue;
        }
        switch (work->team) {
        case 1:
        case 2:
            if (fn_80154784(work) == 1) {
                return 1;
            }
            break;
        case 5:
            if (fn_801D6694(work) == 1) {
                return 1;
            }
            break;
        case 7:
            if (fn_801E01BC(work) == 1) {
                return 1;
            }
            break;
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
s32 em030_homing_range_ck(_ENEMY_WORK* work) {
    return vec3_dist_sq(&work->pos, &work->vec_0x36C) < lbl_80798B38;
}

/* 0x801B0168 (0xC8).  The em030 program's action guard: state 0 always runs, state 1 runs only while
 * the game-state block's area is still the work's area. */
extern "C" u32 fn_801B0168(_ENEMY_WORK* work) {
    switch (work->field_0x380) {
    case 1:
        if (work->field_0x382 != 0xFF) {
            _ENEMY_WORK* target = (_ENEMY_WORK*)em_move_work_pick(work->state_0x381, work->field_0x382);
            if (target->active) {
                if (fn_8012D0B4(work, target) == 1) {
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
extern "C" u32 fn_801B0230(_ENEMY_WORK* work, _PLW* plw) {
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
        work->field_0x328 = 0x238;
    } else if (Pl_item_timer_get(plw, 0xD9) > 0) {
        work->field_0x328 = 0xD9;
    } else {
        u8 usable[0x18];
        u8 count = 0;
        u8 i;

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
        work->field_0x328 = plw->slot_id[usable[ran_suu(0) % count]].item_id;
    }
    work->plw_0x32C = plw;
    pl_item_add(plw, work->field_0x328, -1);
    pl_model_state_set(plw, 2, 0x1B, work->field_0x328);
    return 1;
}

/* 0x801B03E8 (0x68).  Cancels the move `fn_801B0230` latched: the work record's item 1, then the
 * player's 0x1C command. */
extern "C" void fn_801B03E8(_ENEMY_WORK* work) {
    if (work->field_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    pl_item_add(work->plw_0x32C, work->field_0x328, 1);
    pl_model_state_set(work->plw_0x32C, 2, 0x1C, work->field_0x328);
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
}

/* 0x801B0450 (0x54).  The teardown of the same latch (the player's 0xFF command, no item restore). */
extern "C" void fn_801B0450(_ENEMY_WORK* work) {
    if (work->field_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    quest_item_work_merge(work->plw_0x32C, work->field_0x328, -1);
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
}

/* 0x801B04A4 (0x5C).  Arms the 0x20 "condition" bit of `field_0x761` in state 0 and the 0x40 one in
 * state 1, then toggles `field_0x333`; returns 1 only when the state-1 branch ran. */
extern "C" u32 fn_801B04A4(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 = work->field_0x333 + 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}

/* 0x801B0500 (0x5C).  The same toggle for the 0x20/0x10 bits. */
extern "C" u32 fn_801B0500(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 = work->field_0x333 + 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}

/* 0x801B055C (0x178).  The em030 action-state machine's 4-state ring (0 -> 3 -> 1 -> 2 -> 0), plus
 * the motion-id refinement of the 0x40/0x80 bits of `field_0x761`. */
extern "C" void fn_801B055C(_ENEMY_WORK* work) {
    switch (work->field_0x332) {
    case 0:
        if ((work->field_0x761 & 0x10) == 0) {
            work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
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
        if (fn_801B0500(work) == 1) {
            if (work->field_0x762 == 1) {
                work->field_0x332 = 3;
            } else {
                work->field_0x332 = 0;
            }
        }
        break;
    case 3:
        if (fn_801B04A4(work) == 1) {
            if (work->field_0x762 == 0) {
                work->field_0x332 = 2;
            } else {
                work->field_0x332 = 1;
            }
        }
        break;
    }

    switch ((u16)em_get_mot_no(work)) {
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
    case 0x14:
    case 0x1E:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        break;
    }
}

/* 0x801B06D4 (0x13C).  "Is any live area-work record of kind 2 (or the live game-state block's own
 * record) inside the 1500-unit radius?" - `vec3_dist_sq` answers the squared distance. */
extern "C" u32 fn_801B06D4(_ENEMY_WORK* work) {
    u16 max = get_move_work_max(2);
    EmAreaWork* area = (EmAreaWork*)get_move_work_adrs(2);
    u8 i;

    for (i = 0; i < max; i++, area++) {
        if (!area->active) {
            continue;
        }
        if (fn_8012D0B4(work, area) == 0) {
            continue;
        }
        if (pl_torch_ck((_ENEMY_WORK*)area) == 0) {
            continue;
        }
        if (vec3_dist_sq(&work->pos, &area->vec_0x3C) <= lbl_80798B3C) {
            return 1;
        }
    }

    if (ainpc_w.active != 0 && work->area_no == ainpc_w.field_0x1A4) {
        if (ai_torch_ck(&ainpc_w) == 1) {
            if (vec3_dist_sq(&work->pos, &ainpc_w.vec_0x178) <= lbl_80798B3C) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801B0810 (0xAC).  "Is any live work record of kind 3 in the same area whose +0x1C8 is armed?" -
 * the matching record's position is copied onto the work's +0x1B0. */
extern "C" u32 fn_801B0810(_ENEMY_WORK* work) {
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* other = (_ENEMY_WORK*)get_move_work_adrs(3);
    u8 i;

    for (i = 0; i < max; i++, other++) {
        if (!other->active) {
            continue;
        }
        if ((other->field_0x1C8 & 0x7FFFFFFF) == 0) {
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
extern "C" void fn_801B08BC(_ENEMY_WORK* work, u8 kind) {
    if (kind == 2) {
        em_fall_height_get(work);
        em_fall_start(work);
        em_act_arm_unless_down(work, 0x0C, 0);
        em_state_refresh(work);
    }
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
    work->field_0x330 = 0;
    work->field_0x331 = 0;
    work->field_0x332 = 0;
    work->field_0x333 = 0;
    work->field_0x334 = 0;
    work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
    if (work->team == 0x1E || work->field_0x00A == 1) {
        work->field_0x1C8 |= 0x80;
    }
}

/* 0x801B0968 (0x4).  The empty slot `em030_prog_tbl` keeps for the unused state. */
extern "C" void fn_801B0968(void) {}

/* 0x801B096C (0xBC).  The em030 per-state hook: the states 10/11 branch on the motion kind, state 1
 * dispatches on the sub-kind. */
extern "C" void fn_801B096C(_ENEMY_WORK* work, u32 state, u32 kind) {
    switch ((u8)state) {
    case 1:
        switch ((u8)kind) {
        case 0x13:
            if (work->field_0x43B == 2 && work->field_0x43C == 1) {
                fn_8013072C(work, 4, 0);
            }
            break;
        case 0x10:
            if (work->field_0x43B == 3) {
                fn_8013072C(work, 0, 0);
            }
            break;
        }
        break;
    case 10:
    case 11:
        if (work->field_0x331 == 1) {
            if (em_break_state_ck(work) == 0) {
                work->field_0x331 = 0;
            }
        }
        break;
    }
}

/* 0x801B0A28 (0x32C).  The em030 program's per-frame update: the death/approach checks, the
 * per-state action block, the shared tail flags and the +0x761 bit updates. */
extern "C" void fn_801B0A28(_ENEMY_WORK* work) {
    u32 zero = 0;

    work->field_0x330 = zero;
    if (em_die_ck(work) == 0) {
        if (work->field_0x334 == 0) {
            if (fn_801B0810(work) == 1) {
                if (work->field_0x43B != 1) {
                    fn_8013072C(work, 3, 0);
                }
                work->field_0x334 = 1;
            }
        } else {
            if (fn_801B0810(work) == 0) {
                work->field_0x334 = zero;
            }
        }
        if (work->field_0xA07 == 1) {
            if (work->field_0x43B == 2 && work->field_0x43C == 0) {
                fn_8013072C(work, 2, 1);
            }
            work->field_0xA07 = 0;
        }
    }

    switch (work->field_0x43B) {
    case 0:
        if (work->action != 0x0A && em_die_ck(work) == 0 && fn_801B06D4(work) == 1) {
            fn_8013072C(work, 2, 0);
        }
        break;
    case 2:
        switch (work->field_0x43C) {
        case 0:
            if (work->field_0x833 == 1) {
                fn_8012C3C8(0x1E, work->area_no, &work->pos, lbl_80798B40);
                fn_8012C3C8(0x1F, work->area_no, &work->pos, lbl_80798B40);
                fn_8013072C(work, 2, 1);
            } else {
                if (fn_801B06D4(work) == 0) {
                    fn_8013072C(work, 0, 0);
                }
            }
            break;
        case 1:
            if (em_die_ck(work) == 0 && work->field_0x440 > 300) {
                fn_8013072C(work, 4, 0);
            }
            break;
        }
        break;
    case 3:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                fn_8013072C(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                fn_8013072C(work, 0, 0);
            }
        }
        break;
    case 4:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                fn_8013072C(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                fn_8013072C(work, 0, 0);
            }
        }
        break;
    }

    if (work->field_0x00A != 1 && work->field_0x833 == 1) {
        fn_8012C300(0x1E, work->area_no);
        fn_8012C300(0x1F, work->area_no);
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
    fn_801B055C(work);
}

/* 0x801B0D54 (0x90).  The two-step approach action: arm the motion, then wait for `em_mot_end_ck`. */
extern "C" void fn_801B0D54(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
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
extern "C" void fn_801B0DE4(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_801B0D54(work);
        break;
    case 1:
        fn_801B0D54(work);
        break;
    case 2:
        fn_801B0D54(work);
        break;
    }
}

/* 0x801B0E14 (0xEC).  The kinded approach action: state 0 arms the motion the kind selects (the
 * six variants 0x14..0x21, default 0x13), state 1 waits for it. */
extern "C" void fn_801B0E14(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;

            switch ((u8)kind) {
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
            default:
                motion = 0x13;
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
extern "C" void fn_801B0F00(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x16, 4, 0);
        work->timer_0x020 = 0x78;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
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
extern "C" void fn_801B0FBC(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x17, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
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
extern "C" void fn_801B1078(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
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
extern "C" void fn_801B1134(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1C, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x3C;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B11C4 (0x150).  The motion-0x1D approach, whose countdown and follow-up depend on the kind. */
extern "C" void fn_801B11C4(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1D, 4, 0);
        if ((u8)kind == 1) {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        } else {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0xFA;
        }
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            if ((u8)kind == 1) {
                work->state = work->state + 1;
                em_mot_set(work, 0x1E, 4, 0);
            } else {
                work->state = work->state + 2;
                em_mot_set(work, 0x1F, 4, 0);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
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
extern "C" void fn_801B1314(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x24, 6, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B13A4 (0x1A4): the motion-0x25 approach; the kind picks the countdown (0x96/0x5A/0x32) and the follow-up
 * on expiry (kind 1 the 0x26 motion plus the position helper, otherwise the 0x67 one). */
extern "C" void fn_801B13A4(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
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
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            switch ((u8)kind) {
            case 1:
                work->state = work->state + 1;
                em_mot_set(work, 0x26, 0, 0);
                fn_803B9BA0(work, &work->pos, 0x64);
                break;
            case 2:
                work->field_0x331 = 0;
                /* fall through */
            default:
                work->state = work->state + 2;
                em_mot_set(work, 0x67, 4, 0);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
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
extern "C" void fn_801B1548(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x08, 4, 0);
        break;
    case 1:
        if (em_frame_check(work, 1, lbl_80798B44, lbl_80798B48) == 1) {
            em_motion_timer_arm(work);
            em_fx_flag_set(work);
        }
        if (em_mot_end_ck(work) == 1) {
            fn_801B0450(work);
            em_attack_done_set(work);
        }
        break;
    }
}

/* 0x801B15F8 (0xAC): the action dispatcher; `state_sub` selects one of the 20 `jumptable_805B103C` entries, and
 * the `fn_801B0E14` entries carry their kind in r4. */
extern "C" void fn_801B15F8(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_801B0E14(work, 0);
        break;
    case 1:
        fn_801B0E14(work, 1);
        break;
    case 2:
        fn_801B0E14(work, 2);
        break;
    case 3:
        fn_801B0F00(work);
        break;
    case 4:
        fn_801B0FBC(work);
        break;
    case 5:
        fn_801B1078(work);
        break;
    case 6:
        fn_801B0E14(work, 3);
        break;
    case 7:
        fn_801B0E14(work, 4);
        break;
    case 8:
        fn_801B1134(work);
        break;
    case 9:
        fn_801B11C4(work, 0);
        break;
    case 10:
        fn_801B0E14(work, 5);
        break;
    case 11:
        fn_801B0E14(work, 6);
        break;
    case 12:
        fn_801B1314(work);
        break;
    case 13:
        fn_801B13A4(work, 0);
        break;
    case 14:
        fn_801B1548(work);
        break;
    case 15:
        fn_801B13A4(work, 2);
        break;
    case 16:
        fn_801B0F00(work);
        break;
    case 17:
        fn_801B11C4(work, 1);
        break;
    case 18:
        fn_801B13A4(work, 1);
        break;
    case 19:
        fn_801B0E14(work, 3);
        break;
    }
}

/* 0x801B16A4 (0x170): the knockback/recovery action; the kind picks the motion and blend scale, the flag the
 * countdown, and state 1 re-runs the 0x800-flag check and clamps the countdown once the hit is over. */
extern "C" void fn_801B16A4(_ENEMY_WORK* work, u32 kind, u32 flag) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;
            f32 scale;

            switch ((u8)kind) {
            case 0:
                motion = 2;
                scale = lbl_80798B4C;
                break;
            case 1:
                motion = 3;
                scale = lbl_80798B4C;
                break;
            case 2:
            case 3:
                motion = 3;
                scale = lbl_80798B50;
                break;
            }
            em_mot_set(work, motion, 4, 0);
            if ((u8)kind == 3) {
                em_mot_speed_set(work, lbl_80798B54);
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
            if (fn_801B0168(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    }
}

/* 0x801B1814 (0x144): the motion-0x18/0x05 recovery; the 0x100-flag check (or the 0x12C-frame countdown) picks
 * the 0x19 tail, and the countdown is clamped to 0x0F once the hit is over. */
extern "C" void fn_801B1814(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            em_mot_set(work, 0x05, 4, 0);
            em_approach_start(work, lbl_80798B58, 0);
            work->timer_0x020 = 0x12C;
        }
        break;
    case 2:
        if (em_approach_step(work, 0, 0x100) == 1 || --work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        if (work->timer_0x020 > 0x0F) {
            if (fn_801B0168(work) == 1) {
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
extern "C" void fn_801B1958(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        em_approach_start(work, lbl_80798B4C, 0);
        work->timer_0x020 = 0x12C;
        break;
    case 1:
        if (em_approach_step(work, 0, 0x800) == 1 || --work->timer_0x020 <= 0) {
            work->state = work->state + 1;
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
 * turn step, and the second angle steps through `fn_80133DB0` while the 0x78-frame countdown runs. */
extern "C" void fn_801B1A40(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = 0x78;
        work->state_0x006 = work->bits_0x1EC & 0x0F;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        } else {
            work->field_0x1C0 = fn_80133DB0((u16)(work->state_0x006 << 12), (u16)work->field_0x1C0, 0x800);
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
extern "C" void fn_801B1B30(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set(work, 0x04, 4, 0);
        break;
    case 1:
        if (em_ground_ck(work) != 0) {
            work->pos.y = work->field_0x20C;
            em_move_mode_set(work, 0);
            em_move_vec2_clr(work);
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1BD0 (0x90).  The motion-0x02 approach: the per-frame `fn_80132154` step, then the
 * 0x400-flag wait. */
extern "C" void fn_801B1BD0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x02, 4, 0);
        fn_80132154(work);
        break;
    case 1:
        fn_80132154(work);
        if (em_turn_to_target(work, 0x400) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1C60 (0x94).  The motion-0x22 recovery with the 0x80-flag wait. */
extern "C" void fn_801B1C60(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x22, 6, 0);
        em_approach_start(work, lbl_80798B48, 0);
        break;
    case 1:
        if (em_approach_step(work, 0, 0x80) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1CF4 (0x194): the motion-0x06 turn-to-target; the angle to the +0x36C target point becomes a per-frame
 * step (clamped at 0) through `fn_80133DB0`, and the 0x78-frame countdown picks the 0x19 tail. */
extern "C" void fn_801B1CF4(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x78;
        break;
    case 1:
        {
            f32 v = calcVecDistXZ(&work->vec_0x36C, &work->pos) - lbl_80798B5C;
            s32 step;

            if (v < lbl_80798B48) {
                v = lbl_80798B48;
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
            work->field_0x1C0 = fn_80133DB0((u16)(angB + step), (u16)work->field_0x1C0, 0x800);
            work->timer_0x020--;
            if (work->timer_0x020 <= 0) {
                work->state = work->state + 1;
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
extern "C" void fn_801B1E88(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
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
 * `fn_801B16A4` entries carry their kind/flag in r4/r5. */
extern "C" void fn_801B1F04(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_801B16A4(work, 0, 0);
        break;
    case 1:
        fn_801B16A4(work, 1, 0);
        break;
    case 2:
        fn_801B1814(work);
        break;
    case 3:
        fn_801B1958(work);
        break;
    case 4:
        fn_801B1A40(work);
        break;
    case 5:
        fn_801B1B30(work);
        break;
    case 6:
        fn_801B1BD0(work);
        break;
    case 7:
        fn_801B1C60(work);
        break;
    case 8:
        fn_801B1CF4(work, 0);
        break;
    case 9:
        fn_801B1CF4(work, 1);
        break;
    case 10:
        fn_801B16A4(work, 1, 1);
        break;
    case 11:
        fn_801B16A4(work, 2, 0);
        break;
    case 12:
        fn_801B1E88(work);
        break;
    case 13:
        fn_801B16A4(work, 3, 0);
        break;
    }
}

/* 0x801B1F94 (0xA8): the "armed" motion; the two teardown helpers, the 0x0E motion, the position helper, and the
 * +0x310 angle triple the kind arms before `rotVecY` turns it by the second angle. */
extern "C" void fn_801B1F94(_ENEMY_WORK* work, u32 kind) {
    em_fall_height_get(work);
    em_fall_start(work);
    em_mot_set(work, 0x0E, 0, 0);
    em_hit_window_set_default(work, 0, 1);
    em_move_vec2_clr(work);
    work->offset_0x30C.vec_0x310.y = lbl_80798B60;
    if ((u8)kind == 1) {
        work->offset_0x30C.vec_0x310.z = lbl_80798B64;
    } else {
        work->offset_0x30C.vec_0x310.z = lbl_80798B68;
    }
    work->field_0x320 = lbl_80798B6C;
    rotVecY(&work->offset_0x30C.vec_0x310, work->field_0x1C0);
}

/* 0x801B203C (0x25C): the kinded recovery/counter action; state 0 arms the kind's motion, state 2 runs the
 * counter latch (`fn_801B0230` on the player work at +0xA34) and the `UpdateValue` wait, state 3 finishes it. */
extern "C" void fn_801B203C(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state_0x006 = 0;
        switch ((u8)kind) {
        case 1:
            work->state = work->state + 2;
            fn_801B1F94(work, 0);
            break;
        case 2:
            work->state = work->state + 2;
            fn_801B1F94(work, 1);
            break;
        default:
            work->state = work->state + 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0x0D, 4, 0);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            fn_801B1F94(work, 1);
        }
        break;
    case 2:
        CancelFade(work);
        if ((u8)kind == 2 && work->state_0x006 == 0 && work->field_0xA16 != 0xFF && work->field_0xA16 != 5
            && work->field_0xA38 == 0) {
            if (fn_801B0230(work, work->plw_0xA34) == 1) {
                fn_8013072C(work, 1, 0);
            }
            em_hit_window_clear(work, 0);
            work->state_0x006 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_fall_height_get(work);
            if (UpdateValue(work) != 0) {
                u16 motion;

                work->state = work->state + 1;
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
 * or steps the second angle through `fn_80133DB0`, and the team/kind gates pick the follow-up. */
extern "C" void fn_801B2298(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        work->state_0x006 = 0;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        em_hit_window_set(work, 0, 2, 3);
        if ((u8)kind == 1) {
            work->timer_0x020 = 0x78;
            em_mot_speed_set(work, lbl_80798B70);
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
            work->state = work->state + 1;
            work->state_0x006 = 1;
            em_hit_window_clear(work, 0);
            em_mot_set(work, 0x19, 4, 0);
            break;
        }
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_hit_window_clear(work, 0);
            em_mot_set(work, 0x19, 4, 0);
        } else if ((u8)kind == 1) {
            em_turn_to_target(work, 0x800);
        } else {
            work->field_0x1C0 = fn_80133DB0((u16)work->field_0x37C, (u16)work->field_0x1C0, 0x800);
        }
        if (work->timer_0x020 > 0x0F) {
            if (fn_801B0168(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            if (work->team == 0x1F) {
                em_action_finish(work);
            } else if (work->state_0x006 == 1) {
                work->state = work->state + 1;
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
extern "C" void fn_801B2514(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x11, 2, 0);
        work->timer_0x020 = 0x50;
        break;
    case 1:
        if (em_frame_check(work, 0, lbl_80798B74, lbl_80798B48) == 1) {
            ShellSetFn set = ((ShellSetBlock*)shell_set_func_ptr)->field_0x58;

            if ((u8)kind == 1) {
                set(work, 3);
            } else {
                set(work, 2);
            }
        }
        if (em_frame_check(work, 1, lbl_80798B74, lbl_80798B48) == 1) {
            work->field_0x330 = 1;
        }
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x12, 4, 0);
        }
        break;
    case 2:
        if (em_frame_check(work, 2, lbl_80798B78, lbl_80798B48) == 1) {
            work->field_0x330 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B4240 (0x4).  The empty slot `em030_prog_tbl` keeps. */
extern "C" void fn_801B4240(void) {}

#pragma peephole off

extern "C" u32 fn_801B4244(_ENEMY_WORK* work, u32 kind) {
    if ((u8)kind == 0) {
        if (work->field_0x331 == 1) {
            return em_break_state_ck(work) == 1 ? 1 : 2;
        }
    }
    return 0;
}

#pragma peephole on

/* 0x801B42A0 (0x24).  The em030 teardown hook: cancel the latched move and report the exit state. */
extern "C" u32 fn_801B42A0(_ENEMY_WORK* work) {
    fn_801B03E8(work);
    return 2;
}

/* 0x801B42C4 (0x4).  The em030 exit hook's tail call. */
extern "C" void fn_801B42C4(_ENEMY_WORK* work) {
    fn_801B0450(work);
}

/* 0x801B42C8 (0x14).  The em030 state-report hook: the current state id and a zero flag. */
extern "C" void fn_801B42C8(_ENEMY_WORK* work, u8* state, u8* flag) {
    *state = 0x0C;
    *flag = 0;
}

/* 0x801B42DC (0x6C).  The unit's static constructor (the `.ctors` word at 0x8056F344): it fills the
 * two three-float records of the `vec_pair_801B0010_0` table. */
extern "C" void fn_801B42DC(void) {
    VEC3 rec;

    setVec3(&rec, lbl_80798B48, lbl_80798B94, lbl_80798C58);
    assignVec3((Vec*)vec_pair_801B0010_0, (Vec*)&rec);
    setVec3(&rec, lbl_80798B48, lbl_80798C5C, lbl_80798C60);
    assignVec3((Vec*)&vec_pair_801B0010_0[1], (Vec*)&rec);
}

/* The unit's `.bss` (0x806A7A88-0x806A7AA0): the two-vector record its static constructor `fn_801B42DC` builds
 * and the `.data` tables point at. */
VEC3 vec_pair_801B0010_0[2];  /* +0x806A7A88 */
