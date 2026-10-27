/* enemy/enemy_control.cpp - the enemy control TU: the per-kind slots, the sparkle/smoke records, the spawn requests,
 *   the live-work / area-record exchange, the enemy-control events and the level's resource buffers (`emc_work`).
 * RANGE. .text 0x801411B8-0x80147C94 (154 functions); .ctors 0x8056F31C-0x8056F320 (`__sinit_\enemy_control_cpp`),
 *   .data 0x805A1A80-0x805A1C58, .bss 0x806A4590-0x806A7734, .sbss 0x80794A90-0x80794AA0, .sdata2
 *   0x80796DC8-0x80796E08, extab, extabindex.
 * SEAM. The right edge against `enemy/em001_prog.cpp` (0x80147C94) is the end of the record constructors MWCC emits
 *   for `emc_work` (0x80147AC8-0x80147C94) after the unit's static initializer.
 * NAMES. The file from its `__FILE__` string "enemy_control.cpp" (0x805A1BB8, `em_res_file_setup`'s assertions).
 *   `senko_set`, `kemuri_set`, `em_get_unique_work` and `em_ride_hit_check` are runtime-dump names.
 *   GUESS (from each body and its callers): every other function and data name of the unit, and the callee names
 *   proposed in the registry (em_act_update, em_static_data_latch, em_post_update, light_em_slot_alloc,
 *   light_em_slot_free, nw_res_entry_get, mhchar_model_create, mhchar_draw_mode_set, mhchar_model_release,
 *   snd_em_bank_level_load, eft_res_release, pl_ride_down_set, pl_coll_tri_hit_ck, anim_key_curve_eval, em_file_name_tbl).
 * RESIDUALS. Every function is written; 24 of 154 rows are short of 100 %, register allocation or scheduling except:
 *   emc_joint_cache_fill (the "found" exit of the joint search goes straight to the next set: a jump out of the inner
 *   loop no goto-free shape reaches - the inline-helper form measured 90.0 against 90.9), em_sub_buffer_claim (the
 *   retry `c = 2` jumps into the else branch's `li r30,2`: a cross-jump into another branch, idea 213),
 *   em_stat_level_pick (retail reloads `weight` in both roll loops and keeps a dead counter in the arena scan;
 *   `opt_common_subs off` gives the reloads but loses the hoisted `variant * 4`: 89.7 against 91.4),
 *   em_size_scale_pick (`mr r4,r29` before `li r3,0` for the roll walk), emc_status_report (`clrlwi r4,r7` /
 *   `extsb r4,r4` where ours narrows through r0).
 *  - flipcheck: `.bss` 2 and `.sdata2` 4 tail-padding bytes short of the claim; `.text` 16 bytes long (the rows above).
 *  - `EmUserDataObj` is `enemy/fn_80138074.cpp`'s `ResUserDataAc` (two definitions until that unit exports it).
 * SHAPES. `#pragma peephole off` and `#pragma pool_data off` over the bodies; `optimization_level 4` on
 *   emc_event_spawn and em_level_set, `opt_lifetimes off` on em_sub_buffer_release.  Small scans are loops (MWCC
 *   unrolls them and hoists the u8 narrowing); a dead loop counter is a `u8`/`s16` variable (idea 238); a
 *   `get_move_work_max` bound is kept raw and narrowed in the condition (idea 216); search loops start
 *   `for (i = 0, work = works; ...)`; signed reads of the area-slot/element bytes go through the `_s8_` union aliases
 *   of ENEMY_WORK.h/ENEMY_MINI_WORK.h; `anim_key_curve_eval` and `pl_coll_tri_hit_ck` are called through `_c1` views
 *   in their owners' argument order (float second).  The record constructors are `inline` under
 *   `#pragma dont_inline on` so MWCC emits the implicit `EmcWork` ctor and the member ctors as weak functions after
 *   `__sinit_\enemy_control_cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/EmAreaEntry.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_80138074.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor (rule 2) */
#include "fn_8004CAD8.h"
#include "enemy/em_common.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em_ground_rec_clear.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/em_anger_clear.h"
#include "enemy/em_fx_flag_set.h"
#include "enemy/em_motion_timer_arm.h"
#include "enemy/em_shake_req_set.h"
#include "enemy/em_kind.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef/eft_slot.h"
#include "ef/fn_800CDB2C.h"
#include "ef/system_core.h"
#include "ef/mtx34_trans_get.h"
#include "g3d/mtx34_inverse.h"
#include "draw_shape/mtx34_copy.h"
#include "draw_shape.h"
#include "nw_resource.h"
#include "sound/mhchar.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800D7F54.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "hud/quest_notice_spawn.h"
#include "ef/get_move_work_adrs.h"
#include "hud/net_char_sync.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/lb_sub0f_send.h"
#include "quest/quest_entry.h"
#include "quest/quest_types.h"
#include "Pl/quest_spawn_rec_find.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/plw.h"
#include "Pl/fn_80288CEC.h" /* _PL_ROOT */
#include "mh3_pad/lb_param_w.h"
#include "MSL_C/strtoul.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "enemy/em_set_work.h"
#include "enemy/em_act_update.h"
#include "enemy/em_static_data_latch.h"
#include "enemy/em_post_update.h"
#include "light/light_em_slot_alloc.h"
#include "light/light_em_slot_free.h"
#include "nw_res_entry_get.h"
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_draw_mode_set.h"
#include "sound/mhchar_model_release.h"
#include "sound/snd_em_bank_level_load.h"
#include "ef/eft_res_release.h"
#include "Pl/pl_ride_down_set.h"
#include "Pl/pl_coll_tri_hit_ck.h"
#include "enemy/em_model.h" /* em_roster_records_release (rule 2) */
#include "draw_shape/em_file_name_tbl.h"
#include "enemy/em001_prog_tbl.h"
#include "enemy/em003_prog_tbl.h"
#include "enemy/em004_prog_tbl.h"
#include "enemy/em005_prog_tbl.h"
#include "enemy/em006_prog_tbl.h"
#include "enemy/em007_prog_tbl.h"
#include "enemy/em008_prog_tbl.h"
#include "enemy/em009_prog_tbl.h"
#include "enemy/em010_prog_tbl.h"
#include "enemy/em011_prog_tbl.h"
#include "enemy/em012_prog_tbl.h"
#include "enemy/em015_prog_tbl.h"
#include "enemy/em016_prog_tbl.h"
#include "enemy/em018_prog_tbl.h"
#include "enemy/em019_prog_tbl.h"
#include "enemy/em020_prog_tbl.h"
#include "enemy/em022_prog_tbl.h"
#include "lobby/em023_prog_tbl.h"
#include "enemy/em024_prog_tbl.h"
#include "enemy/em025_prog_tbl.h"
#include "enemy/em026_prog_tbl.h"
#include "enemy/em027_prog_tbl.h"
#include "enemy/em029_prog_tbl.h"
#include "enemy/em030_prog_tbl.h"
#include "enemy/em032_prog_tbl.h"
#include "enemy/em033_prog_tbl.h"
#include "enemy/em034_prog_tbl.h"
#include "enemy/em035_prog_tbl.h"
#include "enemy/em036_prog_tbl.h"
#include "enemy/em040_prog_tbl.h"

namespace nw4r {

namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}
}

extern "C" {
/* external callees whose owners' headers do not declare them */
s32  fn_80093B0C(void);
void res_file_init(void* self);
u32  res_file_bind_self(void* self);
u32  res_file_bind(void* self, s32* value);
u32  isServerSelectState(void);
u8   isReadyCountOne(void);
}


/* ---------------------------------------------------------------------------------------------------------------- */
/* data */

/* one `{weight, delta}` row of a stat-level roll list (weight -1 ends a list).  size: 0x4 */
struct EmStatRoll {
    /* +0x0 */ s16 weight;
    /* +0x2 */ s16 delta;
};

/* the per-frame state of the enemy-control event (`emc_event_state`, `.sbss`).  size: 0x8 */
struct EmcEventState {
    /* +0x0 */ u8 post_done;         /* `emc_event_post_move` ran this event */
    /* +0x1 */ u8 move_done;         /* `emc_event_move` ran this event */
    /* +0x2 */ u8 pre_done;          /* `emc_event_pre_move` ran this event */
    /* +0x3 */ s8 event;             /* the running event, -1 = none */
    /* +0x4 */ s8 act_event;         /* the event whose actions run, -1 = none */
    /* +0x5 */ u8 act_req;           /* start `act_event`'s actions */
    /* +0x6 */ u8 act_end_req;       /* end `act_event`'s actions */
    /* +0x7 */ u8 area;              /* the event's area */
};

/* the enemy-data entry an event highlights (`emc_highlight`, `.sbss`).  size: 0x8 */
struct EmcHighlight {
    /* +0x0 */ u8 active;
    /* +0x1 */ u8 event;
    /* +0x2 */ u8 kind;
    /* +0x3 */ u8 team;
    /* +0x4 */ u8 entry;
    /* +0x5 */ u8 unused_0x5[3];
};

/* one action an event gave a live enemy (`emc_event_act_tbl`, `.bss`).  size: 0xC */
struct EmcEventAct {
    /* +0x0 */ u8 active;
    /* +0x1 */ u8 act;               /* the action it was given (13) */
    /* +0x2 */ u8 sub;
    /* +0x3 */ u8 end_act;           /* the action it is given when the event ends */
    /* +0x4 */ u8 end_sub;
    /* +0x5 */ u8 pad_0x5[3];
    /* +0x8 */ _ENEMY_WORK* work;
};

/* one enemy an event holds in place.  size: 0x4 */
struct EmcHoldRec {
    /* +0x0 */ u8 active;
    /* +0x1 */ u8 pad_0x1;
    /* +0x2 */ u16 order;            /* the enemy's spawn order */
};

/* the enemies an event holds (`emc_event_hold`, `.bss`).  size: 0x16 */
struct EmcEventHold {
    /* +0x00 */ s8 event;            /* the holding event, -1 = none */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ EmcHoldRec rec[5];
};

/* the stat-level roll lists, by variant (0: none); the second set is the one the arena quests use. */
EmStatRoll em_stat_roll_a[] = {{5, -2}, {15, -1}, {50, 0}, {20, 1}, {10, 2}, {-1, 0}};
EmStatRoll em_stat_roll_b[] = {{25, -1}, {50, 0}, {25, 1}, {-1, 0}};
EmStatRoll em_stat_roll_c[] = {{20, -2}, {30, -1}, {50, 0}, {-1, 0}};
EmStatRoll em_stat_roll_d[] = {{50, -1}, {50, 0}, {-1, 0}};
EmStatRoll* em_stat_roll_tbl[] = {NULL, em_stat_roll_a, em_stat_roll_b};
EmStatRoll* em_stat_roll_arena_tbl[] = {NULL, em_stat_roll_c, em_stat_roll_d};

/* per monster kind: the kind's index in the enemy program tables (`enemy/em_common.cpp`'s `fn_80125F88`). */
u8 em_kind_index_tbl[0x2C] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x24, 0x24, 0x24, 0x25, 0x00, 0x00, 0x00,
};

/* per monster kind: the size class of its resource buffer (-1: none; 0 and below: no peer bookkeeping). */
s8 em_res_class_tbl[0x2C] = {
    -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 1, 0, 1, 2, 2, 0, 2, 2, 3, 0,
    0,  0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* the monster programs `enemy/em_kind.cpp`'s `fn_80140B20` copies into the locked cache (NULL ends it). */
u32* em_prog_tbl_list[] = {
    em001_prog_tbl, em003_prog_tbl, em004_prog_tbl, em005_prog_tbl, em006_prog_tbl, em007_prog_tbl,
    em008_prog_tbl, em009_prog_tbl, em010_prog_tbl, em011_prog_tbl, em012_prog_tbl, em015_prog_tbl,
    em016_prog_tbl, em017_prog_tbl, em018_prog_tbl, em019_prog_tbl, em020_prog_tbl, em021_prog_tbl,
    em022_prog_tbl, em023_prog_tbl, em024_prog_tbl, em025_prog_tbl, em026_prog_tbl, em027_prog_tbl,
    em029_prog_tbl, em030_prog_tbl, em032_prog_tbl, em033_prog_tbl, em034_prog_tbl, em035_prog_tbl,
    em036_prog_tbl, em040_prog_tbl,
    NULL,
};

extern "C" {
EmcWork emc_work;
EmAreaEntry em_area_entry_tbl[32][4];
EmcEventAct emc_event_act_tbl[5];
EmcEventHold emc_event_hold;
EmcEventState emc_event_state;
EmcHighlight emc_highlight;
}


/* The user-data accessor a live work holds at +0x484 (`enemy/fn_80138074.cpp`'s `ResUserDataAc`), as far as
 * `enemy_work_release` uses it: its first virtual function is the deleting destructor (slot +0x08).
 * size: 0xC */
struct EmUserDataObj {
    virtual ~EmUserDataObj();
    /* +0x4 */ _ENEMY_WORK* work;
    /* +0x8 */ u32 flags;
};

/* the area-record entry procedures `em_entry_proc_get` hands back, by marker: 0x17 enters, 0x18 waits, 0x19 holds,
 * 0x1A places a replayed spawn request. */
typedef void (*EmEnterProc)(_ENEMY_MINI_WORK* mini, u8 mode);
typedef void (*EmMiniProc)(_ENEMY_MINI_WORK* mini);
typedef void (*EmPlaceProc)(_ENEMY_MINI_WORK* mini, const nw4r::math::VEC3* pos, s32* rot, u8 flags);

extern "C" {
/* this unit's own functions, declared ahead of their first callers */
void em_kind_slot_clear_all(void);
void em_kind_slot_clear(u8 index);
EmcSlot* em_kind_slot_state_find(u8 state);
void senko_free(u8 index);
void senko_free_all(void);
SenkoRec* senko_free_get(void);
void senko_move(void);
void kemuri_clear(u8 index);
void kemuri_clear_all(void);
KemuriRec* kemuri_free_get(void);
void kemuri_move(void);
void emc_spawn_req_clear(u8 index);
void emc_spawn_req_clear_all(void);
s32 em_spawn_count_get(u16 order, u8 area, u8 flags, u8 large);
void emc_spawn_req_move(void);
void enemy_work_clear(_ENEMY_WORK* self);
void enemy_work_index_clear(u8 index);
_ENEMY_WORK* enemy_work_free_get(void);
u8 em_variant_element_pick(u8 kind, u8 map, u16 seed, u8 limit, Q_ElementBlock** out);
void enemy_work_ground_set(_ENEMY_WORK* self, EmGroundRec* rec);
u8 em_stat_level_pick(u8 base, u8 variant, u16 seed);
EmSizeRoll* em_size_tbl_get(u8 variant);
f32 em_size_scale_pick(u8 size, u8 variant, u16 seed);
void enemy_work_setup(_ENEMY_WORK* self, u8 kind);
_ENEMY_WORK* enemy_work_new(u8 kind);
s32 em_set_entry_ground_get(EmSetEntry* entry, EmGroundRec* rec);
_ENEMY_WORK* em_area_entry_restore(_ENEMY_MINI_WORK* mini);
void em_area_entry_clear(_ENEMY_MINI_WORK* mini);
void em_area_entry_index_clear(u8 index);
void em_area_entry_clear_all(void);
EmAreaEntry* em_area_entry_free_get(void);
void em_area_entry_ground_set(_ENEMY_MINI_WORK* mini, EmGroundRec* rec);
void em_area_entry_setup(_ENEMY_MINI_WORK* mini, u8 kind, u8 area);
EmAreaEntry* em_area_entry_new(u8 kind, u8 area);
void em_area_entry_move(_ENEMY_MINI_WORK* mini);
void em_area_entries_move(void);
void em_area_entry_enter(_ENEMY_MINI_WORK* mini, u8 mode);
void em_area_entry_reenter(_ENEMY_MINI_WORK* mini);
void em_area_entry_wait_call(_ENEMY_MINI_WORK* mini);
void em_area_entry_hold_call(_ENEMY_MINI_WORK* mini);
void em_area_entry_step_next(_ENEMY_MINI_WORK* mini);
void em_area_entry_drop(_ENEMY_MINI_WORK* mini);
void emc_status_report(u16 enemy_id, u8 phase, u8 step, u8 kind, s8 area_slot);
void emc_joint_cache_clear(u8 index);
void emc_joint_cache_clear_all(void);
void emc_joint_cache_fill(_ENEMY_WORK* self, u8 index);
u32 emc_joint_cache_mtx_get(u8 index, u32 joint, nw4r::math::MTX34* out);
void pl_ride_local_pos_get(_PLW* pl, nw4r::math::MTX34* mtx, nw4r::math::VEC3* out);
s32 em_joint_points_hit_ck(_ENEMY_WORK* self, EmJointPoints* set, nw4r::math::VEC3* pos, f32 radius, u32* joint,
                           f32* height);
void pl_ride_move(_PLW* pl);
void emc_event_size_reset(_ENEMY_WORK* self, u8 event);
EmcEvent* emc_event_tbl_get(u8 event);
void emc_event_state_init(void);
u8 emc_event_area_get(void);
void emc_event_pre_move(void);
void emc_event_post_move(void);
void emc_event_move(void);
s32 emc_event_targets_ready_ck(u8 event);
void emc_event_act_move(void);
void emc_highlight_init(void);
void emc_highlight_set(u8 kind, u8 area, u8 area_match, u8 event);
void emc_highlight_clear(u8 event);
void emc_event_highlight_set(u8 event);
void emc_event_hold_init(void);
void emc_event_hold_set(u8 event);
void emc_area_move_kind(u8 kind, u8 area);
void emc_event_area_move(u8 event);
void emc_event_spawn(u8 event);
void emc_event_act_init(void);
void emc_event_act_start(u8 event);
void emc_event_act_end(void);
s32 em_demo_angle_get(f32 deg);
u32 em_res_peers_ready_ck(s8 size_class);
void em_res_free_notify(s8 size_class);
EmResBuffer* em_res_buffer_find(u8 kind);
void em_res_buffer_release(u8 kind);
EmResBuffer* em_sub_buffer_find(u8 id);
EmResBuffer* em_sub_buffer_free_get(u8 size_class);
u8 em_sub_buffer_release(u8 kind);
}

#pragma peephole off
#pragma pool_data off

/* `anim_key_curve_eval` in its owner's argument order (`fn_8004CAD8.cpp`): key table, frame, the two outputs. */
#define anim_key_curve_eval_c1 ((void (*)(f32*, f32, f32*, f32*))anim_key_curve_eval)
/* `pl_coll_tri_hit_ck` in its owner's argument order (`Pl/pl_coll.cpp`): the radius second. */
typedef u32 (*PlCollTriHitCk)(nw4r::math::VEC3*, f32, nw4r::math::VEC3*, nw4r::math::VEC3*, nw4r::math::VEC3*, f32*);
#define pl_coll_tri_hit_ck_c1 ((PlCollTriHitCk)pl_coll_tri_hit_ck)

extern "C" {

/* ---------------------------------------------------------------------------------------------------------------- */
/* the per-kind slots */

/* Checks the resource file's revision, binds its textures (and those of "shadow_tex.brres") and asserts that every
 * texture was bound. */
void em_res_file_setup(u32* file) {
    u32 bound;
    if (fn_80093B0C() == 0) {
        nw4r::db::Panic("enemy_control.cpp", 810, "NW4R:Failed assertion bRevision");
    }
    res_file_init(file);
    bound = res_file_bind_self(file);
    if (bound != 1) {
        ResEntry* res = ckResourceName("shadow_tex.brres");
        if (res != 0) {
            s32 tmp;
            s32 value;
            res_file_ctor((u32*)&tmp, (u32)res->data);
            value = tmp;
            bound = res_file_bind(file, &value);
        }
    }
    if (bound == 0) {
        nw4r::db::Panic("enemy_control.cpp", 829, "NW4R:Failed assertion bAllBound");
    }
}

/* Resets every sub-system of the enemy control and its control words. */
void enemy_control_init(void) {
    em_kind_slot_clear_all();
    senko_free_all();
    kemuri_clear_all();
    emc_spawn_req_clear_all();
    emc_work.field_0x360 = 0;
    emc_work.field_0x364 = 0;
    emc_work.field_0x368 = -1;
    emc_event_state_init();
    emc_event_hold_init();
    emc_highlight_init();
    emc_event_act_init();
    if (isServerSelectState() == 1) {
        emc_work.field_0x36A = isReadyCountOne();
    } else {
        emc_work.field_0x36A = 1;
    }
    emc_work.field_0x36B = 0;
}

/* Clears all six per-kind slots. */
void em_kind_slot_clear_all(void) {
    u32 i;
    i = 0;
    do {
        em_kind_slot_clear(i);
        i++;
    } while (i < 6);
}

/* Clears one per-kind slot. */
void em_kind_slot_clear(u8 index) {
    EmcSlot* slot = &emc_work.slot_0x000[index];
    s32 tmp;
    res_file_ctor((u32*)&tmp, 0);
    slot->id = index;
    slot->state = 0;
    slot->kind = 0;
    slot->flags = 0;
    slot->handle_0x08 = -1;
    slot->handle_0x0C = -1;
    slot->handle_0x10 = -1;
    slot->handle_0x14 = -1;
}

/* The first of the six slots whose state byte equals `state`. */
EmcSlot* em_kind_slot_state_find(u8 state) {
    EmcSlot* slot = emc_work.slot_0x000;
    s32 i;
    for (i = 0; i < 6; i++, slot++) {
        if (slot->state == state) {
            return slot;
        }
    }
    return NULL;
}

/* Claims the first idle slot for `kind`; returns its index, 0xFF when every slot is taken. */
u8 em_kind_slot_claim(u8 kind) {
    EmcSlot* slot = em_kind_slot_state_find(0);
    if (slot != 0) {
        slot->state = 1;
        slot->kind = kind;
        slot->flags = 0;
        return slot->id;
    }
    return 0xFF;
}

/* Marks a slot's resources loaded (`state = 2`). */
void em_kind_slot_ready_set(EmcSlot* slot) {
    slot->state = 2;
}

/* The index of the claimed slot holding `kind`, 0xFF when none does. */
u8 em_kind_slot_find(u8 kind) {
    s32 i;
    for (i = 0; i < 6; i++) {
        EmcSlot* slot = &emc_work.slot_0x000[i];
        if (slot->state != 0 && slot->kind == kind) {
            return i;
        }
    }
    return 0xFF;
}

/* Releases one per-kind slot: its sound bank, its sub-buffer, its two file handles and its resource buffer. */
void em_kind_slot_release(u8 index) {
    EmcSlot* slot = &emc_work.slot_0x000[index];
    if (slot->state != 0) {
        u8 kind = slot->kind;
        snd_em_se_slot_release(kind);
        em_sub_buffer_release(kind);
        slot->id = index;
        slot->state = 0;
        slot->kind = 0;
        slot->field_0x04 = 0;
        nw_res_entry_clear(slot->handle_0x08);
        nw_res_entry_clear(slot->handle_0x10);
        slot->handle_0x08 = -1;
        slot->handle_0x0C = -1;
        slot->handle_0x10 = -1;
        slot->handle_0x14 = -1;
        em_res_buffer_release(kind);
    }
}

/* Releases all six per-kind slots. */
void em_kind_slot_release_all(void) {
    u32 i;
    i = 0;
    do {
        em_kind_slot_release(i);
        i++;
    } while (i < 6);
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the sparkle and smoke records */

/* Frees one sparkle record. */
void senko_free(u8 index) {
    emc_work.senko_0x090[index].state = 2;
}

/* Frees all eight sparkle records. */
void senko_free_all(void) {
    u32 i;
    i = 0;
    do {
        senko_free(i);
        i++;
    } while (i < 8);
}

/* The first free sparkle record (`state == 2`), NULL when all are in use. */
SenkoRec* senko_free_get(void) {
    SenkoRec* rec = &emc_work.senko_0x090[0];
    if (rec->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    return 0;
}

}

/* Takes a free sparkle record and arms it at `pos` with the value, the byte and the countdown. */
void senko_set(nw4r::math::VEC3* pos, f32 value, u8 arg2, s16 arg3) {
    SenkoRec* rec = senko_free_get();
    if (rec != 0) {
        copyVec3(&rec->pos_0x00, pos);
        rec->value_0x0C = value;
        rec->value_0x12 = arg2;
        rec->countdown = arg3;
        rec->state = 0;
    }
}

extern "C" {

/* Steps the eight sparkle records' two-stage timers (state 0 -> 1 -> 2). */
void senko_move(void) {
    SenkoRec* rec = emc_work.senko_0x090;
    s32 i;
    for (i = 0; i < 8; i++, rec++) {
        switch (rec->state) {
        case 0:
            if (--rec->countdown <= 0) {
                rec->state = 1;
                rec->countdown = 3;
            }
            break;
        case 1:
            if (--rec->countdown <= 0) {
                rec->state = 2;
                rec->countdown = 0;
            }
            break;
        }
    }
}

/* The per-record clear of the smoke records (its body is empty). */
void kemuri_clear(u8 index) {
    (void)index;
}

/* Runs the smoke record clear over the ten records. */
void kemuri_clear_all(void) {
    u32 i;
    i = 0;
    do {
        kemuri_clear(i);
        i++;
    } while (i < 10);
}

/* The first of the ten smoke records whose countdown has run out, NULL when all are in use. */
KemuriRec* kemuri_free_get(void) {
    KemuriRec* rec = &emc_work.marker_0x130[0];
    if (rec->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    return 0;
}

}

/* Takes a free smoke record and arms it at `pos` with the value, the byte and a 570-frame countdown. */
void kemuri_set(nw4r::math::VEC3* pos, f32 value, u8 arg2) {
    KemuriRec* rec = kemuri_free_get();
    if (rec != 0) {
        copyVec3(&rec->pos_0x00, pos);
        rec->value_0x0C = value;
        rec->value_0x12 = arg2;
        rec->countdown = 570;
    }
}

extern "C" {

/* Steps the ten smoke countdowns. */
void kemuri_move(void) {
    KemuriRec* rec = emc_work.marker_0x130;
    s32 i;
    for (i = 0; i < 10; i++, rec++) {
        if (rec->countdown > 0) {
            if (--rec->countdown <= 0) {
                rec->countdown = 0;
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the spawn requests */

/* Frees one spawn request. */
void emc_spawn_req_clear(u8 index) {
    emc_work.marker2_0x1F8[index].field_0x09 = 0;
}

/* Frees all ten spawn requests. */
void emc_spawn_req_clear_all(void) {
    u32 i;
    i = 0;
    do {
        emc_spawn_req_clear(i);
        i++;
    } while (i < 10);
}

/* Files `count` spawn requests of `kind`/`team` (less the records already counted, at most `max`): each one takes a
 * free request and is matched to a waiting area record of that kind; returns 1 when all were filed. */
u32 em_spawn_request(u16 order, u8 kind, u8 team, u8 area, u8 entry, u8 max, u8 count, u8 flags, u8 request,
                     nw4r::math::VEC3* pos, s32 rot_arg) {
    EmAreaEntry* entries;
    u8 req_flags;
    s32 left;
    _CP_VECTOR* rot;
    u16 i;
    s32 e;
    rot = (_CP_VECTOR*)rot_arg;
    e = 0;
    entries = &em_area_entry_tbl[0][0];
    req_flags = flags;
    Marker2Rec* req;

    if (pos != NULL) {
        req_flags = flags | 2;
    }
    if (rot != NULL) {
        req_flags |= 4;
    }
    left = count - em_spawn_count_get(order, area, req_flags, (enemy_data_by_team_get(kind, team)->flags_0x00 & 1) != 0);
    if (left > max) {
        left = max;
    }
    i = 0;
    req = &emc_work.marker2_0x1F8[0];
    for (;;) {
        if (left == 0) {
            return 1;
        }
        if (req->field_0x09 == 0) {
            req->field_0x00 = order;
            req->field_0x04 = kind;
            req->field_0x05 = team;
            req->field_0x06 = entry;
            req->field_0x07 = area;
            req->field_0x08 = req_flags;
            req->field_0x0A = request;
            if (req_flags & 2) {
                copyVec3(&req->pos_0x0C, pos);
            }
            if (req_flags & 4) {
                eft_rot_vec_copy((_CP_VECTOR*)&req->field_0x18, rot);
            }
            for (; e < 128; e++, entries++) {
                if (entries->active != 0 && entries->state_0x08 == 1 && entries->group == req->field_0x04) {
                    if (!(req->field_0x08 & 8) || req->field_0x05 == entries->team_0x03) {
                        if (req->field_0x08 & 1) {
                            if (req->field_0x07 != entries->area_0x0A) {
                                continue;
                            }
                        } else {
                            u8 want = req->field_0x07;
                            if (want != 0xFF && em_area_entry_near_ck(entries, get_now_mapno(), want) == 0) {
                                continue;
                            }
                        }
                        entries->state_0x08 = 2;
                        req->enemy_id_0x02 = entries->order_0x16;
                        req->field_0x09 = 1;
                        emc_net_send(&emc_work, 2, i);
                        break;
                    }
                }
            }
            left--;
        }
        req++;
        i++;
        if (i >= 10) {
            return 0;
        }
    }
}

/* The records (live works and waiting area records) of spawn order `order` and placement mode 3 that a request
 * would count: `flags` bit 0 asks for area `area` only, `large` selects the large kinds. */
s32 em_spawn_count_get(u16 order, u8 area, u8 flags, u8 large) {
    u32 max;
    _ENEMY_WORK* work;
    s32 by_area;
    EmAreaEntry* entry;
    s32 count;
    s32 i;
    max = get_move_work_max(3);
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = 0;
    by_area = flags & 1;

    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active != 0 && (by_area == 0 || area == work->area_no) && (large != 0 || !(work->field_0x1C8 & 1)) &&
            (large != 1 || (work->field_0x1C8 & 1)) && work->field_0x00C == 3 && order == work->link_order_0x01C) {
            count++;
        }
    }
    entry = &em_area_entry_tbl[0][0];
    for (i = 0; i < 128; i++, entry++) {
        if (entry->active != 0 && entry->state_0x08 != 1 && (by_area == 0 || area == entry->area_0x0A) &&
            (large != 0 || !(enemy_data_by_team_get(entry->group, entry->team_0x03)->flags_0x00 & 1)) &&
            (large != 1 || (enemy_data_by_team_get(entry->group, entry->team_0x03)->flags_0x00 & 1)) &&
            entry->field_0x0C == 3 && order == entry->link_order_0x18) {
            count++;
        }
    }
    return count;
}

/* Replays a spawn request on area record `mini`: in the player's area it becomes a live enemy at once, elsewhere its
 * entry procedure 0x1A places it. */
void emc_marker_replay(_ENEMY_MINI_WORK* mini, u8 team, u8 entry, u16 order, u8 area, u8 request, u8 flags,
                       const nw4r::math::VEC3* pos, s32* rot, u8 remote) {
    if (area != 0xFF) {
        mini->area_0x0A = area;
    }
    mini->team_0x03 = team;
    if (entry != 0xFF) {
        mini->entry_0x09 = entry;
    }
    mini->state_0x08 = 0;
    if (mini->work_0x30 != NULL ||
        (mini->area_0x0A == get_now_areano() && em_kind_slot_find(mini->monster_0x02) != 0xFF && remote == 0)) {
        _ENEMY_WORK* work = em_area_entry_restore(mini);
        if (work != NULL) {
            work->link_order_0x01C = order;
            work->field_0x00E = 2;
            if (flags & 2) {
                copyVec3(&work->pos, pos);
            }
            if (flags & 4) {
                eft_rot_vec_copy((_CP_VECTOR*)&work->field_0x1BC, (_CP_VECTOR*)rot);
            }
            if (request != 0xFF) {
                work->flags_0xA04 = 1;
                work->field_0xA05 = request;
                work->field_0xA06 = 1;
            } else {
                work->flags_0xA04 = 0;
                work->field_0xA05 = 0xFF;
                work->field_0xA06 = 0;
            }
            em_act_update(work);
            if (!(flags & 0x10) && (work->field_0x1C8 & 1)) {
                quest_notice_spawn((s32)work, work->area_no);
            }
        }
        em_area_entry_release(mini);
    } else {
        EmPlaceProc proc;
        mini->link_order_0x18 = order;
        mini->spawn_kind_0x0E = 2;
        proc = (EmPlaceProc)em_entry_proc_get(mini, 0x1A);
        if (proc != NULL) {
            proc(mini, pos, rot, flags);
        }
    }
}

/* Replays every matched spawn request whose area record is still waiting. */
void emc_spawn_req_move(void) {
    Marker2Rec* req;
    s32 i;
    i = 0;
    req = &emc_work.marker2_0x1F8[0];
    do {
        if (req->field_0x09 == 1) {
            _ENEMY_MINI_WORK* mini;
            req->field_0x09 = 0;
            if (em_get_unique_work(req->enemy_id_0x02, NULL, &mini) == 2 && mini->state_0x08 == 2) {
                emc_marker_replay(mini, req->field_0x05, req->field_0x06, req->field_0x00, req->field_0x07,
                                  req->field_0x0A, req->field_0x08, &req->pos_0x0C, (s32*)&req->field_0x18, 0);
            }
        }
        req++;
        i++;
    } while (i < 10);
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the live enemy works */

/* Fills `out` from the spawn record with order `id` (0xFFFF: none); returns 1 when found. */
/* untyped: caller-owned payload - each caller passes its own view of the 0x20-byte ground record */
u32 em_ground_rec_find(u32 id, void* out) {
    QuestSpawnRec* rec;
    if ((u16)id == 0xFFFF) {
        return 0;
    }
    rec = quest_spawn_rec_find((u16)id);
    if (rec != NULL) {
        em_ground_rec_set((EmGroundRec*)out, &rec->slot_0x8->element_0x08);
        return 1;
    }
    return 0;
}

/* Clears a live enemy work: its model, its state bytes, its user data and its broken parts. */
void enemy_work_clear(_ENEMY_WORK* self) {
    mhchar_reset((MHchar*)self->char_0x024);
    self->active = 0;
    self->visible_0x001 = 0;
    self->field_0x004 = 0;
    self->state = 0;
    self->state_0x006 = 0;
    self->state_0x007 = 0;
    self->field_0x008 = 0;
    self->field_0x00A = 0;
    self->user_data_0x484 = NULL;
    em_break_parts_reset(self);
}

/* Clears live enemy work `index`. */
void enemy_work_index_clear(u8 index) {
    _ENEMY_WORK* work = &((_ENEMY_WORK*)get_move_work_adrs(3))[index];
    enemy_work_clear(work);
    work->group = index;
}

/* Clears every live enemy work, the effect slot pool and the area records. */
void enemy_work_reset(void) {
    u32 max = get_move_work_max(3);
    u8 i;
    i = 0;
    while (i < (u16)max) {
        enemy_work_index_clear(i);
        i++;
    }
    eft_slot_pool_clear();
    em_area_entry_clear_all();
}

/* The first free live enemy work (stamped with its index), NULL when all are taken. */
_ENEMY_WORK* enemy_work_free_get(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u32 max = get_move_work_max(3);
    u16 i;
    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active == 0 && work->field_0x008 == 0) {
            work->group = i;
            return work;
        }
    }
    return NULL;
}

/* Fills a ground record from a quest placement element. */
void em_ground_rec_set(EmGroundRec* rec, const Q_ElementBlock* element) {
    rec->field_0x00 = element->byte_0x00;
    rec->area_0x01 = element->byte_0x01;
    rec->mode_0x02 = element->byte_0x02;
    rec->field_0x03 = element->byte_0x03;
    vec_to_mh_vec3(&rec->pos_0x08, (struct Vec*)&element->word_0x08);
    eft_rot_vec_copy(&rec->rot_0x14, (_CP_VECTOR*)&element->word_0x14);
    rec->field_0x04_b = element->byte_0x04;
    rec->field_0x05 = element->byte_0x05;
    rec->field_0x06 = element->byte_0x06;
    rec->field_0x07 = element->byte_0x07;
}

/* Picks a placement element of monster `kind` for map `map`: the variant list of the map's kind, element
 * `seed % count` (at most the second for kind 15 when `limit` is 1); 0xFF when there is none. */
u8 em_variant_element_pick(u8 kind, u8 map, u16 seed, u8 limit, Q_ElementBlock** out) {
    QuestEntrySlot* slots;
    QuestEntrySlot* slot;
    u8 count = 0;
    QuestMonsterVariantList* list = quest_monster_variant_tbl[kind];
    u8 index;

    if (list == NULL) {
        return 0xFF;
    }
    for (; list->rank != 0xFF; list++) {
        if (list->rank == stage_map_kind_get(map)) {
            break;
        }
    }
    slots = list->records;
    if (slots == NULL) {
        return 0xFF;
    }
    for (slot = slots; slot->monster_0x00 != 0; slot++) {
        count++;
    }
    if (count == 0) {
        return 0xFF;
    }
    if (limit == 1 && kind == 15 && count >= 3) {
        count = 2;
    }
    index = seed % count;
    *out = &slots[index].element_0x08;
    return index;
}

/* Seats a live enemy work at a ground record: its area, entry, mode and position. */
void enemy_work_ground_set(_ENEMY_WORK* self, EmGroundRec* rec) {
    self->area_no = rec->area_0x01;
    self->field_0x46C = rec->field_0x00;
    self->field_0x00C = rec->mode_0x02;
    self->field_0x010 = rec->field_0x04_b;
    self->frames_index_0x015 = rec->field_0x05;
    self->field_0x00A = rec->field_0x03;
    copyVec3(&self->pos, &rec->pos_0x08);
    eft_rot_vec_copy((_CP_VECTOR*)&self->field_0x1BC, &rec->rot_0x14);
}

/* The stat rows of monster kind `kind`, NULL without a table. */
u8* em_stat_tbl_get(u8 kind) {
    if (emc_work.stat_tbl != NULL) {
        return emc_work.stat_tbl[kind];
    }
    return NULL;
}

/* The stat level `base` moved by a roll of list `variant` with `seed`, clamped to 0..60. */
u8 em_stat_level_pick(u8 base, u8 variant, u16 seed) {
    s16 delta = 0;
    EmStatRoll* roll = em_stat_roll_tbl[variant];
    s16 level;

    if (isServerSelectState() == 0) {
        s32 i;
        for (i = 0; i < 4; i++) {
            if (lb_param_w.slots_0x16[i] == 25) {
                roll = em_stat_roll_arena_tbl[variant];
                break;
            }
        }
    }
    if (roll != NULL && roll->weight != 0xFFFF) {
        EmStatRoll* r = roll;
        s16 total = 0;
        for (; r->weight != -1; r++) {
            total += r->weight;
        }
        if (total != 0) {
            s16 pick = (u16)(seed % total);
            s16 sum = 0;
            for (; roll->weight != -1; roll++) {
                sum += roll->weight;
                if (pick < sum) {
                    delta = roll->delta;
                    break;
                }
            }
        }
    }
    level = base + delta;
    if (level < 0) {
        level = 0;
    } else if (level > 60) {
        level = 60;
    }
    return level;
}

/* The stat level of the quest monster with spawn order `order`, rolled with `seed`. */
u8 em_stat_level_roll(u16 order, u16 seed) {
    u8 base = quest_em_stat_tbl_get(order);
    return em_stat_level_pick(base, quest_em_stat_var_get(order), seed);
}

/* The size-scale roll list of variant `variant`, NULL without a table. */
EmSizeRoll* em_size_tbl_get(u8 variant) {
    if (emc_work.size_tbl != NULL) {
        return emc_work.size_tbl[variant];
    }
    return NULL;
}

/* The size scale for size `size` (percent) moved by a roll of list `variant` with `seed`. */
f32 em_size_scale_pick(u8 size, u8 variant, u16 seed) {
    u16 pct = 100;
    EmSizeRoll* roll = em_size_tbl_get(variant);
    if (roll != NULL && roll->weight != 0xFFFF) {
        EmSizeRoll* r;
        s16 total = 0;
        for (r = roll; r->weight != 0xFFFF; r++) {
            total += (s16)r->weight;
        }
        if (total != 0) {
            s16 pick = seed % total;
            s16 sum = 0;
            for (; roll->weight != 0xFFFF; roll++) {
                sum += (s16)roll->weight;
                if (pick < sum) {
                    u16 draw = rand_lcg_step(seed);
                    pct = roll->min + draw % (roll->max - roll->min + 1);
                    break;
                }
            }
        }
    }
    return (f32)(s32)(size + pct - 100) / 100.0f;
}

/* The size scale of the quest monster with spawn order `order`, rolled with `seed` (1.0 without a size). */
f32 em_size_scale_roll(u16 order, u16 seed) {
    u8 size = quest_em_size_get(order);
    if (size == 0) {
        return 1.0f;
    }
    return em_size_scale_pick(size, quest_em_size_var_get(order), seed);
}

/* Sets up live enemy work `self` for monster `kind`: the kind slot, the area, the defaults, the data, the light and
 * the model. */
void enemy_work_setup(_ENEMY_WORK* self, u8 kind) {
    u32 file;
    u32 tmp;
    u8 slot;
    res_file_ctor(&file, 0);
    slot = em_kind_slot_find(kind);
    if (slot != 0xFF && self != NULL) {
        EnemyData* data;
        u8 light;
        ResEntry* rec;
        self->active = 1;
        self->field_0x009 = 0;
        self->field_0x00B = slot;
        self->field_0x00C = 0xFE;
        self->field_0x1E0 = get_now_mapno();
        self->area_no = get_now_areano();
        self->field_0x7A0 = -1;
        self->field_0x7AC = -1;
        self->field_0x9F6 = 0xFF;
        memset(self->data_0xB04, 0, 0x10);
        self->field_0x9F6 = 0xFF;
        self->field_0x01A = 0xFFFF;
        self->field_0x00F = 0;
        self->field_0x011 = 0;
        self->field_0x012 = 0;
        self->field_0x010 = 0;
        self->frames_index_0x015 = 0;
        self->field_0x016 = 0;
        self->state_0x017 = 0;
        self->area_slot_s8_0x013 = (s8)quest_arena_item_count_get();
        self->field_0x014 = 0;
        self->link_order_0x01C = 0xFFFF;
        self->field_0x7C8 = 0;
        self->field_0x1D0 = 1.0f;
        self->event_slot_0x46B = -1;
        self->field_0x888 = -1;
        self->element_s8_0x00D = -1;
        self->field_0x00E = 0;
        self->team = kind;
        self->field_0x00A = 0;
        data = get_enemy_data(self);
        self->field_0x1C8 = data->flags_0x00;
        em_static_data_latch(self);
        light = light_em_slot_alloc(self);
        rec = nw_res_entry_get(emc_work.slot_0x000[slot].handle_0x08);
        if (rec != NULL) {
            res_file_assign(&file, res_file_ctor(&tmp, (u32)rec->data));
            mhchar_model_create((MHchar*)self->char_0x024, &file, data->model_flags_0x04 | 0x65, light);
        }
        mhchar_draw_mode_set((MHchar*)self->char_0x024, 2);
    }
}

/* Takes a free live enemy work and sets it up for monster `kind`. */
_ENEMY_WORK* enemy_work_new(u8 kind) {
    _ENEMY_WORK* work = enemy_work_free_get();
    if (work != NULL) {
        enemy_work_setup(work, kind);
    }
    return work;
}

/* Spawns small monster `kind` at ground record `rec` with spawn order `order` and rolled stats. */
_ENEMY_WORK* em_small_spawn(u8 kind, EmGroundRec* rec, u16 order) {
    _ENEMY_WORK* work = enemy_work_new(kind);
    if (work != NULL) {
        enemy_work_ground_set(work, rec);
        work->field_0x01A = order;
        work->field_0x7C8 = em_stat_level_roll(work->field_0x01A, ran_suu(2));
        work->field_0x1D0 = em_size_scale_roll(work->field_0x01A, ran_suu(2));
    }
    return work;
}

/* Spawns small monster `kind` at `rec` with spawn order `order`, stat level `level` and scale `scale` (the
 * companion's spawn message). */
_ENEMY_WORK* em_small_spawn_stat(u8 kind, EmGroundRec* rec, u16 order, u8 level, f32 scale) {
    _ENEMY_WORK* work = enemy_work_new(kind);
    if (work != NULL) {
        enemy_work_ground_set(work, rec);
        work->field_0x01A = order;
        work->field_0x7C8 = level;
        work->field_0x1D0 = scale;
        work->field_0x00E = 4;
        em_act_update(work);
    }
    return work;
}

/* Sends the intruder spawn of monster `kind` to the companion with rolled stats. */
void em_intruder_spawn(u8 kind, EmGroundRec* rec, u16 order) {
    u8 level = em_stat_level_roll(order, ran_suu(1));
    f32 scale = em_size_scale_roll(order, ran_suu(1));
    lb_sub0f_send(my_player_no(), kind, rec, order, level, scale);
}

/* Picks a placement of a free-hunt entry's monster on the current map into `rec`; returns 1 when there is one. */
s32 em_set_entry_ground_get(EmSetEntry* entry, EmGroundRec* rec) {
    Q_ElementBlock* element;
    u16 seed = ran_suu(1);
    em_variant_element_pick(entry->monster_0x31, stage_map_kind_get(1), seed, 1, &element);
    if (element != NULL) {
        em_ground_rec_set(rec, element);
        return 1;
    }
    return 0;
}

/* Spawns a free-hunt entry's small monster with spawn order `order`. */
_ENEMY_WORK* em_set_entry_spawn(EmSetEntry* entry, u16 order) {
    EmGroundRec rec;
    _ENEMY_WORK* work;
    em_ground_rec_clear(&rec);
    if (em_set_entry_ground_get(entry, &rec) == 0) {
        return NULL;
    }
    work = enemy_work_new(entry->monster_0x31);
    if (work != NULL) {
        enemy_work_ground_set(work, &rec);
        work->field_0x01A = order;
        work->field_0x7C8 = em_stat_level_pick(entry->act_set_0x35, entry->act_sub_0x36, ran_suu(1));
        work->field_0x1D0 = em_size_scale_pick(entry->stat_a_0x38, entry->stat_b_0x3A, ran_suu(1));
    }
    return work;
}

/* Spawns a free-hunt entry's large monster with spawn order `order` (the scale rolls the size row twice). */
_ENEMY_WORK* em_set_boss_spawn(EmSetEntry* entry, u16 order) {
    EmGroundRec rec;
    u8 level;
    em_ground_rec_clear(&rec);
    if (em_set_entry_ground_get(entry, &rec) == 0) {
        return NULL;
    }
    level = em_stat_level_pick(entry->act_set_0x35, entry->act_sub_0x36, ran_suu(1));
    return em_small_spawn_stat(entry->monster_0x31, &rec, order, level,
                               em_size_scale_pick(entry->stat_a_0x38, entry->stat_a_0x38, ran_suu(1)));
}

/* Turns area record `mini` back into a live enemy work (its own linked work, or a new one); NULL when its spawn
 * record is gone. */
_ENEMY_WORK* em_area_entry_restore(_ENEMY_MINI_WORK* mini) {
    EmGroundRec rec;
    QuestSpawnRec* spawn;
    _ENEMY_WORK* work;

    em_ground_rec_clear(&rec);
    if (mini->step_0x04 == 0) {
        mini->step_0x04++;
        em_area_entry_reenter(mini);
    }
    spawn = quest_spawn_rec_find(mini->order_0x16);
    work = mini->work_0x30;
    if (work != NULL) {
        enemy_work_setup(work, mini->monster_0x02);
    } else {
        if (spawn == NULL) {
            return NULL;
        }
        work = enemy_work_new(mini->monster_0x02);
    }
    rec.field_0x00 = mini->entry_0x09;
    rec.mode_0x02 = mini->field_0x0C;
    rec.field_0x03 = mini->team_0x03;
    rec.field_0x04_b = mini->field_0x0F;
    rec.field_0x05 = mini->field_0x13;
    rec.area_0x01 = mini->area_0x0A;
    if (spawn != NULL) {
        nw4r::math::VEC3 pos;
        copyVec3(&rec.pos_0x08, (nw4r::math::VEC3*)em_vec3_copy(&pos, &spawn->slot_0x8->element_0x08.word_0x10, 0));
        eft_rot_vec_copy(&rec.rot_0x14, (_CP_VECTOR*)&spawn->slot_0x8->element_0x08.word_0x1C);
    } else {
        nw4r::math::VEC3 zero;
        copyVec3(&rec.pos_0x08, setVec3(&zero, 0.0f, 0.0f, 0.0f));
        rec.field_0x14 = 0;
        rec.field_0x18 = 0;
        rec.field_0x1C = 0;
    }
    if (work != NULL) {
        enemy_work_ground_set(work, &rec);
        work->field_0x01A = mini->order_0x16;
        work->element_s8_0x00D = mini->element_s8_0x0D;
        work->field_0x9F6 = mini->area_prev_0x0B;
        work->link_order_0x01C = mini->link_order_0x18;
        work->field_0x7A0 = mini->field_0x28;
        work->field_0x7AC = mini->field_0x2C;
        work->field_0x016 = mini->phase_0x1C;
        work->field_0x012 = mini->step_0x12;
        work->area_slot_s8_0x013 = mini->area_slot_s8_0x1D;
        work->field_0x014 = mini->field_0x1E;
        work->field_0x7C8 = mini->stat_level_0x1F;
        work->field_0x1D0 = mini->size_scale_0x24;
        memcpy(work->data_0xB04, mini->data_0x34, 0x10);
        work->field_0x00F = 1;
    }
    return work;
}

}

/* Copies a `VEC3` (`src` -> `dst`); returns `dst`. */
extern "C" void* em_vec3_copy(void* dst, void* src, s32 unused) {
    nw4r::math::VEC3* d = (nw4r::math::VEC3*)dst;
    nw4r::math::VEC3* s = (nw4r::math::VEC3*)src;
    (void)unused;
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
    return dst;
}

extern "C" {

/* Releases a live enemy work: its hooks, model, scene work, light, user data and sound; the last work of its kind
 * releases the kind slot too. */
void enemy_work_release(_ENEMY_WORK* self) {
    u32 max = get_move_work_max(3);
    if (self->active != 0) {
        u8 team;
        if (self->field_0x004 != 0) {
            void (*hook)(_ENEMY_WORK*);
            em_roster_records_release(self);
            hook = em_hook_get(self, 11);
            if (hook != NULL) {
                hook(self);
            }
        }
        mhchar_model_release((MHchar*)self->char_0x024);
        if (self->g3d_0x130 != NULL) {
            push_g3d_wk(self->g3d_0x130);
            self->g3d_0x130 = NULL;
        }
        team = self->team;
        light_em_slot_free(self);
        if ((u32)em_res_user_data_ck(self) == 1) {
            delete self->user_data_0x484;
        }
        em_roster_records_release(self);
        if (self->se_0xB14 != NULL) {
            se_handle_clear((u8*)self->se_0xB14);
        }
        enemy_work_clear(self);
        if ((self->field_0x1C8 & 9) == 9) {
            _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
            s32 n;
            s32 i;
            EmAreaEntry* entry;
            for (n = 0; n < (u16)max; n++) {
                if (work->active != 0 && self->field_0x00B == work->field_0x00B) {
                    if (em_res_class_tbl[team] > 0) {
                        em_res_free_notify(em_res_class_tbl[team]);
                    }
                    return;
                }
                work++;
            }
            entry = &em_area_entry_tbl[0][0];
            for (i = 0; i < 128; i++, entry++) {
                if (entry->active != 0 && entry->work_0x30 != NULL) {
                    u8 slot = em_kind_slot_find(entry->monster_0x02);
                    if (slot != 0xFF && self->field_0x00B == slot) {
                        if (em_res_class_tbl[team] > 0) {
                            em_res_free_notify(em_res_class_tbl[team]);
                        }
                        return;
                    }
                }
            }
            em_kind_slot_release(self->field_0x00B);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the area records */

/* Clears an area record. */
void em_area_entry_clear(_ENEMY_MINI_WORK* mini) {
    mini->active = 0;
    mini->monster_0x02 = 0;
    mini->team_0x03 = 0;
    mini->step_0x04 = 0;
    mini->field_0x05 = 0;
    mini->field_0x06 = 0;
    mini->field_0x07 = 0;
    mini->state_0x08 = 0;
    mini->work_0x30 = NULL;
}

/* Clears area record `index` and stamps its index. */
void em_area_entry_index_clear(u8 index) {
    EmAreaEntry* entry = &em_area_entry_tbl[0][index];
    em_area_entry_clear(entry);
    entry->index_0x01 = index;
}

/* Clears all 128 area records. */
void em_area_entry_clear_all(void) {
    u32 i;
    i = 0;
    do {
        em_area_entry_index_clear(i);
        i++;
    } while (i < 128);
}

/* The first free area record, NULL when all are taken. */
EmAreaEntry* em_area_entry_free_get(void) {
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    s32 i;
    for (i = 0; i < 128; i++, entry++) {
        if (entry->active == 0) {
            return entry;
        }
    }
    return NULL;
}

/* Seats an area record at a ground record. */
void em_area_entry_ground_set(_ENEMY_MINI_WORK* mini, EmGroundRec* rec) {
    mini->entry_0x09 = rec->field_0x00;
    mini->field_0x0C = rec->mode_0x02;
    mini->team_0x03 = rec->field_0x03;
    mini->field_0x0F = rec->field_0x04_b;
    mini->field_0x13 = rec->field_0x05;
    mini->busy_0x15 = rec->field_0x06;
}

/* Sets up area record `mini` for monster `kind` in area `area` with the defaults. */
void em_area_entry_setup(_ENEMY_MINI_WORK* mini, u8 kind, u8 area) {
    em_area_entry_clear(mini);
    mini->active = 1;
    mini->state_0x08 = 0;
    mini->field_0x0C = 0xFE;
    mini->area_0x0A = area;
    mini->area_prev_0x0B = 0xFF;
    mini->order_0x16 = 0xFFFF;
    mini->phase_0x1C = 0;
    mini->field_0x11 = 0;
    mini->step_0x12 = 0;
    mini->link_order_0x18 = 0xFFFF;
    mini->element_s8_0x0D = -1;
    mini->spawn_kind_0x0E = 0;
    mini->monster_0x02 = kind;
    mini->team_0x03 = 0;
    mini->field_0x10 = 0;
    mini->field_0x0F = 0;
    mini->field_0x13 = 0;
    mini->busy_0x15 = 0;
    mini->area_slot_s8_0x1D = (s8)quest_arena_item_count_get();
    mini->field_0x1E = 0;
    mini->stat_level_0x1F = 0;
    mini->size_scale_0x24 = 1.0f;
    mini->field_0x28 = -1;
    mini->field_0x2C = -1;
    memset(mini->data_0x34, 0, 0x10);
}

/* Takes a free area record and sets it up for monster `kind` in area `area`. */
EmAreaEntry* em_area_entry_new(u8 kind, u8 area) {
    EmAreaEntry* entry = em_area_entry_free_get();
    if (entry != NULL) {
        em_area_entry_setup(entry, kind, area);
    }
    return entry;
}

/* Spawns large monster `kind` at ground record `rec` with spawn order `order`: as an area record (linked to a new
 * live work for a placement-mode-3 large kind). */
_ENEMY_MINI_WORK* em_large_spawn(u8 kind, EmGroundRec* rec, u16 order) {
    _ENEMY_MINI_WORK* mini;
    _ENEMY_WORK* work;
    work = NULL;
    if (rec->mode_0x02 == 3) {
        if (enemy_data_by_team_get(kind, rec->field_0x03)->flags_0x00 & 1) {
            work = enemy_work_free_get();
        }
        mini = em_large_entry_new(kind, rec, order, work);
    } else {
        mini = em_area_entry_new(kind, rec->area_0x01);
        if (mini != NULL) {
            em_area_entry_ground_set(mini, rec);
            mini->order_0x16 = order;
        }
        if (rec->mode_0x02 == 4) {
            mini->state_0x08 = 5;
        }
    }
    if (mini != NULL) {
        if (work == NULL) {
            mini->stat_level_0x1F = em_stat_level_roll(mini->order_0x16, 0);
            mini->size_scale_0x24 = em_size_scale_roll(mini->order_0x16, 0);
            if (quest_item_work_flag_ck() == 1) {
                mini->spawn_kind_0x0E = 4;
            }
        } else {
            mini->stat_level_0x1F = em_stat_level_roll(mini->order_0x16, ran_suu(2));
            mini->size_scale_0x24 = em_size_scale_roll(mini->order_0x16, ran_suu(2));
        }
    }
    return mini;
}

/* Stores live enemy work `self` (leaving the area) into a new area record; NULL when its spawn record is gone. */
_ENEMY_MINI_WORK* em_area_entry_store(_ENEMY_WORK* self) {
    EmGroundRec rec;
    _ENEMY_MINI_WORK* mini;

    em_ground_rec_clear(&rec);
    if (self->field_0x004 == 0) {
        self->field_0x004++;
        em_act_advance(self, self->field_0x00E);
    }
    if (quest_spawn_rec_find(self->field_0x01A) == NULL) {
        return NULL;
    }
    rec.field_0x00 = self->field_0x46C;
    rec.mode_0x02 = self->field_0x00C;
    rec.field_0x03 = self->field_0x00A;
    rec.area_0x01 = self->area_no;
    copyVec3(&rec.pos_0x08, &self->pos);
    eft_rot_vec_copy(&rec.rot_0x14, (_CP_VECTOR*)&self->field_0x1BC);
    mini = em_area_entry_new(self->team, self->area_no);
    if (mini != NULL) {
        em_area_entry_ground_set(mini, &rec);
        mini->order_0x16 = self->field_0x01A;
        mini->element_s8_0x0D = self->element_s8_0x00D;
        mini->area_prev_0x0B = self->field_0x9F6;
        mini->link_order_0x18 = self->link_order_0x01C;
        mini->field_0x28 = self->field_0x7A0;
        mini->field_0x2C = self->field_0x7AC;
        mini->field_0x0F = self->field_0x010;
        mini->field_0x13 = self->frames_index_0x015;
        mini->phase_0x1C = self->field_0x016;
        mini->step_0x12 = self->field_0x012;
        mini->area_slot_s8_0x1D = self->area_slot_s8_0x013;
        mini->field_0x1E = self->field_0x014;
        mini->stat_level_0x1F = self->field_0x7C8;
        mini->size_scale_0x24 = self->field_0x1D0;
        memcpy(mini->data_0x34, self->data_0xB04, 0x10);
        mini->field_0x10 = 1;
        if (self->action == 11) {
            if (em_spawn_rec_live_ck2((_ENEMY_WORK*)mini) == 1) {
                mini->state_0x08 = 3;
                if (self->field_0x8D2 >= 1) {
                    mini->timer_0x1A = self->field_0x018;
                } else {
                    s16 wait;
                    if (self->field_0x914 > 300) {
                        wait = 300;
                    } else {
                        wait = self->field_0x914;
                    }
                    mini->timer_0x1A = em_kind_frames_get((_ENEMY_WORK*)mini) - wait;
                }
            } else {
                em_area_entry_release(mini);
                mini = NULL;
            }
        } else if ((u32)em_act_ck(self, 12, 0xFF) == 1) {
            mini->state_0x08 = 3;
            mini->timer_0x1A = self->field_0x018;
        } else if (self->field_0x011 != 0) {
            emc_mini_step(mini);
        }
    }
    return mini;
}

/* Files large monster `kind` as an area record linked to live work `work`. */
_ENEMY_MINI_WORK* em_large_entry_new(u8 kind, EmGroundRec* rec, u16 order, _ENEMY_WORK* work) {
    _ENEMY_MINI_WORK* mini = em_area_entry_new(kind, rec->area_0x01);
    if (mini != NULL) {
        mini->state_0x08 = 1;
        if (work != NULL) {
            work->field_0x008 = 1;
        }
        em_area_entry_ground_set(mini, rec);
        mini->order_0x16 = order;
        mini->work_0x30 = work;
    }
    return mini;
}

/* Frees an area record. */
void em_area_entry_release(_ENEMY_MINI_WORK* mini) {
    if (mini->active != 0) {
        em_area_entry_clear(mini);
    }
}

/* Turns every waiting area record of the current area (or with no placement mode) back into a live enemy. */
void em_area_entries_release(void) {
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    u32 i = 0;
    do {
        if (entry->active != 0 && entry->state_0x08 == 0 &&
            (entry->area_0x0A == get_now_areano() || entry->field_0x0C == 0)) {
            em_area_entry_restore(entry);
            em_area_entry_release(entry);
        }
        i++;
        entry++;
    } while (i < 128);
}

/* Stores every live non-boss enemy work into an area record and releases the works. */
void enemy_work_store_all(void) {
    u32 max;
    _ENEMY_WORK* work;
    u32 i;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active != 0 && !(work->field_0x1C8 & 8)) {
            if (em_work_die_ck(work) == 0) {
                em_area_entry_store(work);
            }
            enemy_work_release(work);
        }
    }
}

/* Frees every area record that has a placement mode and no linked work. */
void em_area_spawn_clear(void) {
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    u32 i = 0;
    do {
        if (entry->active != 0 && entry->field_0x0C != 0 && entry->work_0x30 == NULL) {
            em_area_entry_release(entry);
        }
        i++;
        entry++;
    } while (i < 128);
}

/* 1 for the frame the ready count turned to one. */
u32 emc_host_ck(void) {
    return emc_work.field_0x36B == 1;
}

/* The per-frame enemy control: the ready count, the event, the spawn requests, the area records, the joint caches
 * and every live enemy's motion. */
void enemy_control_move(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u32 max = get_move_work_max(3);
    u32 i;
    u8 cache;

    if (isServerSelectState() == 1) {
        u8 ready;
        emc_work.field_0x36B = 0;
        ready = isReadyCountOne();
        if (emc_work.field_0x36A == 0 && ready == 1) {
            emc_work.field_0x36B = 1;
        }
        emc_work.field_0x36A = ready;
    }
    emc_event_pre_move();
    emc_spawn_req_move();
    if (event_demo_ck() == 0) {
        em_area_entries_move();
    }
    emc_joint_cache_clear_all();
    cache = 0;
    emc_event_act_move();
    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active != 0) {
            if (em_data_word_get(work) != 0) {
                emc_joint_cache_fill(work, cache);
                cache++;
            }
            if (event_demo_ck() == 0 || work->event_slot_0x46B != -1) {
                em_act_update(work);
            } else {
                work->field_0x1DE = 0;
                em_fx_flag_set(work);
                em_motion_timer_arm(work);
                em_shake_req_set(work);
            }
        }
    }
    emc_event_post_move();
    eft_slot_spawn_targets();
    if (event_demo_ck() == 0) {
        emc_event_move();
        eft_slot_pool_frame();
        senko_move();
        kemuri_move();
    }
}

/* The second per-frame pass: every live enemy's post update and every player's ride. */
void enemy_control_post_move(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    _PLW* pl = (_PLW*)get_move_work_adrs(2);
    u32 max = get_move_work_max(3);
    u32 i;
    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active != 0) {
            em_post_update(work);
        }
    }
    max = get_move_work_max(2);
    for (i = 0; i < (u16)max; i++, pl++) {
        if (pl->slot_active != 0) {
            pl_ride_move(pl);
        }
    }
}

/* One per-frame step of area record `mini`: 0 enters it (straight into a live enemy in the player's area), 1 runs
 * its state (the timed waits, the hold and the arrival), 2 holds it, 3 drops it. */
void em_area_entry_move(_ENEMY_MINI_WORK* mini) {
    switch (mini->step_0x04) {
    case 0:
        mini->step_0x04++;
        if (mini->state_0x08 == 0 && mini->area_0x0A == get_now_areano()) {
            if (em_kind_slot_find(mini->monster_0x02) != 0xFF && mini->spawn_kind_0x0E != 4) {
                _ENEMY_WORK* work = em_area_entry_restore(mini);
                if (work != NULL) {
                    work->field_0x00E = mini->spawn_kind_0x0E;
                }
                em_area_entry_release(mini);
            } else {
                em_area_entry_reenter(mini);
            }
        } else {
            em_area_entry_reenter(mini);
        }
        break;
    case 1:
        switch (mini->state_0x08) {
        case 3:
            if (--mini->timer_0x1A <= 0) {
                emc_mini_event_b(mini, 0);
            }
            break;
        case 4:
            if (mini->timer_0x1A < 0x7FFF) {
                if (--mini->timer_0x1A <= 0) {
                    emc_mini_event_b(mini, 0);
                }
            }
            break;
        case 5:
            em_area_entry_hold_call(mini);
            break;
        case 0: {
            u8 here = mini->area_0x0A == get_now_areano();
            if (here && mini->spawn_kind_0x0E != 4 && em_kind_slot_find(mini->monster_0x02) != 0xFF) {
                _ENEMY_WORK* work = em_area_entry_restore(mini);
                if (work != NULL) {
                    work->field_0x00E = 1;
                }
                em_area_entry_release(mini);
                break;
            }
            em_area_entry_wait_call(mini);
            if (!here && mini->state_0x08 != 3 && mini->state_0x08 != 4 && mini->area_0x0A == get_now_areano() &&
                em_kind_slot_find(mini->monster_0x02) != 0xFF) {
                _ENEMY_WORK* work = em_area_entry_restore(mini);
                if (work != NULL) {
                    work->field_0x00E = 3;
                }
                em_area_entry_release(mini);
            }
            break;
        }
        }
        break;
    case 2:
        em_area_entry_step_next(mini);
        break;
    case 3:
        em_area_entry_drop(mini);
        break;
    }
}

/* Steps every area record that is neither linked (state 1) nor matched to a request (state 2). */
void em_area_entries_move(void) {
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    u32 i = 0;
    do {
        if (entry->active != 0) {
            u8 state = entry->state_0x08;
            if (state != 1 && state != 2) {
                em_area_entry_move(entry);
            }
        }
        i++;
        entry++;
    } while (i < 128);
}

/* Enters area record `mini` through its entry procedure 0x17 (`mode` 1 also resets its area, links and data). */
void em_area_entry_enter(_ENEMY_MINI_WORK* mini, u8 mode) {
    EmEnterProc proc;
    mini->move_timer_0x22 = 0;
    mini->roam_state_0x20 = 0;
    mini->end_kind_0x14 = 0xFF;
    if (mode == 1) {
        mini->area_prev_0x0B = 0xFF;
        mini->field_0x28 = -1;
        mini->field_0x2C = -1;
        memset(mini->data_0x34, 0, 0x10);
        mini->field_0x11 = 0;
        mini->field_0x1E = 0;
    }
    proc = (EmEnterProc)em_entry_proc_get(mini, 0x17);
    if (proc != NULL) {
        proc(mini, mode);
    }
}

/* Re-enters area record `mini` with its own spawn kind. */
void em_area_entry_reenter(_ENEMY_MINI_WORK* mini) {
    em_area_entry_enter(mini, mini->spawn_kind_0x0E);
}

/* Runs area record `mini`'s wait procedure (0x18). */
void em_area_entry_wait_call(_ENEMY_MINI_WORK* mini) {
    EmMiniProc proc = (EmMiniProc)em_entry_proc_get(mini, 0x18);
    if (proc != NULL) {
        proc(mini);
    }
}

/* Runs area record `mini`'s hold procedure (0x19). */
void em_area_entry_hold_call(_ENEMY_MINI_WORK* mini) {
    EmMiniProc proc = (EmMiniProc)em_entry_proc_get(mini, 0x19);
    if (proc != NULL) {
        proc(mini);
    }
}

/* Advances area record `mini`'s step. */
void em_area_entry_step_next(_ENEMY_MINI_WORK* mini) {
    mini->step_0x04++;
}

/* Frees area record `mini` (its last step). */
void em_area_entry_drop(_ENEMY_MINI_WORK* mini) {
    em_area_entry_release(mini);
}

/* The end of a timed state of area record `mini`: back to its spawn record's placement (a placement-mode-3 record
 * that is still alive) or into the waiting state; `remote` comes from the net sync. */
void emc_mini_event_b(_ENEMY_MINI_WORK* mini, u8 remote) {
    EmGroundRec rec;
    u8 phase;
    u8 step;
    QuestSpawnRec* spawn;
    u8 report;
    u8 state;

    em_ground_rec_clear(&rec);
    spawn = quest_spawn_rec_find(mini->order_0x16);
    if (spawn == NULL) {
        em_area_entry_release(mini);
        return;
    }
    report = remote == 0;
    state = mini->state_0x08;
    if (state == 4) {
        phase = mini->phase_0x1C;
        step = mini->step_0x12 + 1;
        report = 0;
    } else {
        phase = mini->phase_0x1C + 1;
        step = 0;
    }
    if (mini->field_0x0C == 3 && state != 4) {
        if (em_spawn_rec_live_ck2((_ENEMY_WORK*)mini) == 1) {
            u16 order = mini->order_0x16;
            s8 element = mini->element_s8_0x0D;
            u8 level = mini->stat_level_0x1F;
            f32 scale = mini->size_scale_0x24;
            s8 area_slot = mini->area_slot_s8_0x1D;
            em_area_entry_setup(mini, mini->monster_0x02, spawn->slot_0x8->element_0x08.byte_0x01);
            em_ground_rec_set(&rec, &spawn->slot_0x8->element_0x08);
            em_area_entry_ground_set(mini, &rec);
            mini->order_0x16 = order;
            mini->element_s8_0x0D = element;
            mini->state_0x08 = 1;
            mini->phase_0x1C = phase;
            mini->step_0x12 = step;
            mini->stat_level_0x1F = level;
            mini->size_scale_0x24 = scale;
            mini->area_slot_s8_0x1D = area_slot;
            if (report == 1) {
                emc_entry_report(mini, 2);
            }
            return;
        }
    } else {
        mini->state_0x08 = 0;
        mini->phase_0x1C = phase;
        mini->step_0x12 = step;
        if (report == 1) {
            emc_entry_report(mini, 2);
        }
        if (mini->area_0x0A == get_now_areano() && remote == 0 && em_kind_slot_find(mini->monster_0x02) != 0xFF) {
            _ENEMY_WORK* work = em_area_entry_restore(mini);
            if (work != NULL) {
                work->field_0x00E = 1;
            }
        } else {
            em_area_entry_enter(mini, 1);
            return;
        }
    }
    em_area_entry_release(mini);
}

/* The kill of area record `mini`'s monster: the kill bookkeeping, the report, then the wait before it respawns (or
 * its release). */
void emc_mini_event_a(_ENEMY_MINI_WORK* mini, u8 end_kind, u8 remote) {
    mini->end_kind_0x14 = end_kind;
    quest_enemy_kill_dispatch((_ENEMY_WORK*)mini, 1);
    if (remote == 0) {
        emc_entry_report(mini, 1);
    }
    if (em_spawn_rec_live_ck2((_ENEMY_WORK*)mini) == 1) {
        mini->state_0x08 = 3;
        mini->timer_0x1A = em_kind_frames_get((_ENEMY_WORK*)mini);
    } else {
        em_area_entry_release(mini);
    }
}

/* Puts area record `mini` into the second timed state (unless it already waits). */
void emc_mini_step(_ENEMY_MINI_WORK* mini) {
    u8 state = mini->state_0x08;
    if (state != 3 && state != 4) {
        mini->state_0x08 = 4;
        mini->timer_0x1A = em_kind_frames2_get((_ENEMY_WORK*)mini);
    }
}

/* The delay frames the control work holds. */
s16 emc_delay_frames_get(void) {
    return emc_work.field_0x368;
}

/* Records an enemy-control status (the enemy, its phase and step, the event kind and its value) and sends it. */
void emc_status_report(u16 enemy_id, u8 phase, u8 step, u8 kind, s8 area_slot) {
    QuestSpawnRec* rec;
    emc_work.status_0xD9C.enemy_id = enemy_id;
    emc_work.status_0xD9C.phase = phase;
    emc_work.status_0xD9C.step = step;
    emc_work.status_0xD9C.kind = kind;
    emc_work.status_0xD9C.area_slot_0x06 = area_slot;
    rec = quest_spawn_rec_find_in(enemy_id, (s8)(u8)area_slot);
    if (rec != NULL) {
        emc_work.status_0xD9C.value_0x07 = rec->count_0x4;
    }
    emc_net_send(&emc_work, 1, 0);
}

/* Reports event `kind` of live enemy `self` (a non-boss). */
void emc_kill_report(_ENEMY_WORK* self, u8 kind) {
    if (!(self->field_0x1C8 & 8)) {
        emc_status_report(self->field_0x01A, self->field_0x016, self->field_0x012, kind, self->area_slot_s8_0x013);
    }
}

/* Reports event `kind` of area record `mini`. */
void emc_entry_report(_ENEMY_MINI_WORK* mini, u8 kind) {
    emc_status_report(mini->order_0x16, mini->phase_0x1C, mini->step_0x12, kind, mini->area_slot_s8_0x1D);
}

}

/* The record with spawn order `id`: a live work (stored in `*out`, returns 1) or an area record (stored in `*mini`,
 * returns 2); 0 when there is none. */
u8 em_get_unique_work(u16 id, _ENEMY_WORK** out, _ENEMY_MINI_WORK** mini) {
    if (out != NULL) {
        *out = NULL;
    }
    if (mini != NULL) {
        *mini = NULL;
    }
    if (out != NULL) {
        _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
        if (work != NULL) {
            u32 max = get_move_work_max(3);
            u16 i;
            for (i = 0; i < (u16)max; i++, work++) {
                if (work->active != 0 && work->field_0x01A == id) {
                    *out = work;
                    return 1;
                }
            }
        }
    }
    if (mini != NULL) {
        EmAreaEntry* entry = &em_area_entry_tbl[0][0];
        s16 i;
        for (i = 0; i < 128; i++, entry++) {
            if (entry->active != 0 && entry->order_0x16 == id) {
                *mini = entry;
                return 2;
            }
        }
    }
    return 0;
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------------------- */
/* the rideable enemies' joint caches */

/* Clears joint cache `index`. */
void emc_joint_cache_clear(u8 index) {
    EmcJointCache* cache = &emc_work.joint_cache[index];
    cache->enemy_index = 0xFF;
    cache->count = 0;
}

/* Clears both joint caches. */
void emc_joint_cache_clear_all(void) {
    u32 i;
    i = 0;
    do {
        emc_joint_cache_clear(i);
        i++;
    } while (i < 2);
}

/* Caches the world matrices of every joint live enemy `self`'s ride point sets name, in joint cache `index`. */
void emc_joint_cache_fill(_ENEMY_WORK* self, u8 index) {
    if (index < 2) {
        EmJointPoints* set = (EmJointPoints*)em_data_word_get(self);
        EmcJointCache* cache = &emc_work.joint_cache[index];
        cache->enemy_index = self->group;
        cache->count = 0;
        while (set->joint != 0xFFFFFFFF) {
            u32 joint = set->joint;
            u8 i = 0;
            u8 n;
            for (n = cache->count; n != 0; n--) {
                if (joint == cache->mtx[i].joint) {
                    break;
                }
                i++;
            }
            if (n == 0) {
                if (cache->count >= 25) {
                    return;
                }
                cache->mtx[cache->count].joint = joint;
                get_joint_wmat_em(self, set->joint, &cache->mtx[cache->count].mtx);
                cache->count++;
            }
            set++;
        }
    }
}

/* Copies joint `joint`'s cached matrix of joint cache `index` into `out`; 0 when it is not cached. */
u32 emc_joint_cache_mtx_get(u8 index, u32 joint, nw4r::math::MTX34* out) {
    EmcJointCache* cache = &emc_work.joint_cache[index];
    u8 i;
    for (i = 0; i < cache->count; i++) {
        EmcJointMtx* m = &cache->mtx[i];
        if (joint == m->joint) {
            mtx34_copy((Mtx34*)out, (Mtx34*)&m->mtx);
            return 1;
        }
    }
    return 0;
}

/* The player's position relative to `mtx` (the ridden joint's last frame matrix). */
void pl_ride_local_pos_get(_PLW* pl, nw4r::math::MTX34* mtx, nw4r::math::VEC3* out) {
    nw4r::math::MTX34 inv;
    nw4r::math::VEC3 trans;
    nw4r::math::VEC3 diff;
    MTX34_ctor(&inv);
    VEC3_ctor(&trans);
    mtx34_trans_get(mtx, &trans);
    subVec3(&diff, &pl->vec_0x03C, &trans);
    copyVec3(out, &diff);
    mtx34_inverse(&inv, mtx);
    mulVecMat(out, &inv);
}

/* Tests `pos` with radius `radius` against every triangle of live enemy `self`'s joint point sets `set`; the highest
 * hit's height goes to `*height` and its joint to `*joint`.  Returns the number of hits. */
s32 em_joint_points_hit_ck(_ENEMY_WORK* self, EmJointPoints* set, nw4r::math::VEC3* pos, f32 radius, u32* joint,
                           f32* height) {
    nw4r::math::VEC3 pts[6];
    f32 h;
    s32 hits;
    VEC3_ctor(&pts[0]);
    VEC3_ctor(&pts[1]);
    VEC3_ctor(&pts[2]);
    VEC3_ctor(&pts[3]);
    VEC3_ctor(&pts[4]);
    VEC3_ctor(&pts[5]);
    hits = 0;
    while (set->joint != 0xFFFFFFFF) {
        u8 i;
        s32 tris = em_joint_points_get(self, set, pts) - 2;
        for (i = 0; i < tris; i++) {
            if (pl_coll_tri_hit_ck_c1(pos, radius, &pts[i], &pts[i + 1], &pts[i + 2], &h) == 1) {
                if (hits != 0) {
                    if (h > *height) {
                        *height = h;
                        *joint = set->joint;
                    }
                } else {
                    *height = h;
                    *joint = set->joint;
                }
                hits++;
            }
        }
        set++;
    }
    return hits;
}

/* Keeps player `pl` on the rideable enemy it stands on: follows the ridden joint and lands it on the joint points. */
void pl_ride_move(_PLW* pl) {
    EmcJointCache* cache = &emc_work.joint_cache[0];
    _ENEMY_WORK* works = (_ENEMY_WORK*)get_move_work_adrs(3);
    nw4r::math::VEC3 probe;
    nw4r::math::VEC3 local;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 push;
    f32 height;
    u32 joint;
    s32 hit;
    u32 i;
    f32 lift;
    f32 radius;
    VEC3_ctor(&probe);
    VEC3_ctor(&local);
    MTX34_ctor(&mtx);
    setVector3(&pl->ride_ofs_0x588, 0.0f, 0.0f, 0.0f);
    hit = 0;
    i = 0;
    lift = 30.0f;
    radius = -60.0f;
    do {
        u8 index = cache->enemy_index;
        if (index != 0xFF) {
            _ENEMY_WORK* work = &works[index];
            EmJointPoints* set;
            if (work->active != 0 && em_pl_target_ck(work, pl) != 0 &&
                (set = (EmJointPoints*)em_data_word_get(work)) != NULL) {
                if (pl->field_0x585 != 0 && pl->ride_enemy_0x595 == index &&
                    emc_joint_cache_mtx_get(i, pl->ride_joint_0x594, &mtx) == 1) {
                    pl_ride_local_pos_get(pl, &mtx, &local);
                    get_joint_wmat_em(work, pl->ride_joint_0x594, &mtx);
                    mulVecMatAddTrans(&local, &mtx);
                    subVec3(&push, &pl->vec_0x03C, &local);
                    copyVec3(&pl->ride_ofs_0x588, &push);
                    copyVec3(&pl->vec_0x03C, &local);
                }
                copyVec3(&probe, &pl->vec_0x03C);
                probe.y += lift;
                if (em_joint_points_hit_ck(work, set, &probe, radius, &joint, &height) != 0) {
                    hit = 1;
                    pl->field_0x040 = height;
                    pl->ride_joint_0x594 = joint;
                    pl->ride_enemy_0x595 = index;
                    if (pl->field_0x585 == 0) {
                        if (Pl_master_ck(pl) == 1 && pl->health <= 0) {
                            pl->field_0x585 = 0;
                            pl_ride_down_set(pl);
                        } else {
                            pl->field_0x585 = 1;
                            Pl_act_set_motion_slot(pl, 0, 0, 0);
                        }
                    }
                    break;
                }
            }
        }
        i++;
        cache++;
    } while (i < 2);
    if (hit == 0) {
        pl->field_0x585 = 0;
    }
}

}

/* The rideable enemy in area `area` whose joint points `pos` hits: the work, the joint and the hit height. */
u32 em_ride_hit_check(nw4r::math::VEC3* pos, u8 area, _ENEMY_WORK** out, u32* joint, f32* height) {
    EmcJointCache* cache = &emc_work.joint_cache[0];
    _ENEMY_WORK* works = (_ENEMY_WORK*)get_move_work_adrs(3);
    nw4r::math::VEC3 probe;
    f32 hit_height;
    u32 hit_joint;
    u32 i;
    f32 lift;
    f32 radius;
    VEC3_ctor(&probe);
    i = 0;
    lift = 200.0f;
    radius = -400.0f;
    do {
        u8 index = cache->enemy_index;
        if (index != 0xFF) {
            _ENEMY_WORK* work = &works[index];
            if (work->active != 0 && work->area_no == area) {
                EmJointPoints* set;
                work = &work[index];
                set = (EmJointPoints*)em_data_word_get(work);
                if (set != NULL) {
                    copyVec3(&probe, pos);
                    probe.y += lift;
                    if (em_joint_points_hit_ck(work, set, &probe, radius, &hit_joint, &hit_height) != 0) {
                        *height = hit_height;
                        *joint = hit_joint;
                        *out = work;
                        return 1;
                    }
                }
            }
        }
        i++;
        cache++;
    } while (i < 2);
    return 0;
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------------------- */
/* the enemy-control events */

/* Resets live enemy `self`'s size scale to 1.0 when event `event` targets its kind (and area). */
void emc_event_size_reset(_ENEMY_WORK* self, u8 event) {
    if (self->active != 0 && (self->field_0x1C8 & 1)) {
        EmcEvent* ev = emc_event_tbl_get(event);
        if (ev != NULL) {
            EmcEventTarget* target = ev->targets;
            if (target != NULL) {
                for (; target->kind != 0; target++) {
                    if (self->team == target->kind && (target->area_match == 0 || self->field_0x00A == target->area)) {
                        self->field_0x1D0 = 1.0f;
                        return;
                    }
                }
            }
        }
    }
}

/* Resets live enemy `self`'s size scale when the running event targets it. */
void emc_event_size_reset_ck(_ENEMY_WORK* self) {
    if (emc_event_state.event != -1) {
        emc_event_size_reset(self, emc_event_state.event);
    }
}

/* Event `event`'s record, NULL without a table. */
EmcEvent* emc_event_tbl_get(u8 event) {
    if (emc_work.event_tbl != NULL) {
        return emc_work.event_tbl[event];
    }
    return NULL;
}

/* Resets the event state. */
void emc_event_state_init(void) {
    emc_event_state.post_done = 0;
    emc_event_state.move_done = 0;
    emc_event_state.pre_done = 0;
    emc_event_state.event = -1;
    emc_event_state.act_event = -1;
    emc_event_state.act_req = 0;
    emc_event_state.act_end_req = 0;
    emc_event_state.area = 0xFF;
}

/* Starts event `event` in area `area` (when the event exists and is active). */
void emc_event_set(u8 event, u8 area) {
    EmcEvent* ev = emc_event_tbl_get(event);
    if (ev != NULL && ev->active != 0) {
        emc_event_state.event = event;
        emc_event_state.area = area;
    }
}

/* The running event's area. */
u8 emc_event_area_get(void) {
    return emc_event_state.area;
}

/* The event's first frame step: its spawn requests. */
void emc_event_pre_move(void) {
    if (emc_event_state.pre_done == 0) {
        if (emc_event_state.event != -1) {
            emc_event_spawn(emc_event_state.event);
        }
        emc_event_state.pre_done++;
    }
}

/* The event's first frame tail: its area moves. */
void emc_event_post_move(void) {
    if (emc_event_state.post_done == 0) {
        if (emc_event_state.event != -1) {
            emc_event_area_move(emc_event_state.event);
        }
        emc_event_state.post_done++;
    }
}

/* The event's first frame step after the enemies moved: the holds and the highlight. */
void emc_event_move(void) {
    if (emc_event_state.move_done == 0) {
        if (emc_event_state.event != -1) {
            emc_event_hold_set(emc_event_state.event);
            emc_event_highlight_set(emc_event_state.event);
        }
        emc_event_state.move_done++;
    }
}

/* Matches event `event`'s targets to the live enemies in the player's area: 1 when every target has one, 0 when
 * some has none, -1 when a matched enemy is not entered yet. */
s32 emc_event_targets_ready_ck(u8 event) {
    s16 max;
    u32 waiting;
    s16 t;
    _ENEMY_WORK* works;
    EmcEvent* ev;
    s32 all;
    works = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    ev = emc_event_tbl_get(event);
    EmcEventTarget* target;

    if (ev == NULL) {
        return 0;
    }
    target = ev->targets;
    if (target == NULL) {
        return 0;
    }
    all = 1;
    waiting = 0;
    t = 0;
    for (; target->kind != 0; target++, t++) {
        s32 found = 0;
        s16 i;
        _ENEMY_WORK* work;
        for (i = 0, work = works; i < max; i++, work++) {
            if (work->active != 0 && (work->event_slot_0x46B == -1 || work->event_slot_0x46B == t) &&
                work->team == target->kind && (target->area_match == 0 || work->field_0x00A == target->area) &&
                work->area_no == get_now_areano()) {
                found = 1;
                work->event_slot_0x46B = t;
                if (work->field_0x004 == 0) {
                    waiting = 1;
                }
                break;
            }
        }
        if (found == 0) {
            all = 0;
        }
    }
    if (waiting == 1) {
        return -1;
    }
    return all;
}

/* Runs the event action requests: the end request ends the actions (and the highlight), the start request starts
 * them once every target is matched. */
void emc_event_act_move(void) {
    if (emc_event_state.act_end_req != 0) {
        if (emc_event_state.act_event != -1) {
            emc_highlight_clear(emc_event_state.act_event);
        }
        emc_event_act_end();
        emc_event_state.act_end_req = 0;
    } else if (emc_event_state.act_req != 0) {
        u32 done = 1;
        if (emc_event_state.act_event != -1) {
            if (emc_event_targets_ready_ck(emc_event_state.act_event) == -1) {
                done = 0;
            } else {
                emc_event_act_start(emc_event_state.act_event);
            }
        }
        if (done == 1) {
            emc_event_state.act_req = 0;
        }
    }
}

/* Clears the highlight. */
void emc_highlight_init(void) {
    emc_highlight.active = 0;
    emc_highlight.event = 0xFF;
    emc_highlight.kind = 0;
    emc_highlight.team = 0;
    emc_highlight.entry = 0xFF;
}

/* Highlights the enemy-data entry of the first live enemy (else area record) of `kind` (in area `area` when
 * `area_match`) for event `event`. */
void emc_highlight_set(u8 kind, u8 area, u8 area_match, u8 event) {
    s32 found = 0;
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 i;
    s16 max = get_move_work_max(3);
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];

    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->team == kind && (area_match == 0 || work->field_0x00A == area)) {
            _ENEMY_DATA* data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(kind, area), work->field_0x46C);
            if (data != NULL) {
                data->highlight_0x16 = 1;
                emc_highlight.active = 1;
                emc_highlight.event = event;
                emc_highlight.kind = kind;
                emc_highlight.team = area;
                emc_highlight.entry = work->field_0x46C;
            }
            found = 1;
            break;
        }
    }
    if (found == 0 && entry != NULL) {
        u8 n;
        for (n = 0; n < 128; n++, entry++) {
            if (entry->active != 0 && entry->monster_0x02 == kind && (area_match == 0 || entry->team_0x03 == area)) {
                _ENEMY_DATA* data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(entry->monster_0x02, entry->team_0x03),
                                                                  entry->entry_0x09);
                if (data != NULL) {
                    data->highlight_0x16 = 1;
                    emc_highlight.active = 1;
                    emc_highlight.event = event;
                    emc_highlight.kind = kind;
                    emc_highlight.team = area;
                    emc_highlight.entry = entry->entry_0x09;
                }
                break;
            }
        }
    }
}

/* Drops event `event`'s highlight. */
void emc_highlight_clear(u8 event) {
    if (emc_highlight.active != 0 && emc_highlight.event == event) {
        _ENEMY_DATA* data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(emc_highlight.kind, emc_highlight.team),
                                                          emc_highlight.entry);
        if (data != NULL) {
            data->highlight_0x16 = 0;
        }
        emc_highlight_init();
    }
}

/* Highlights event `event`'s marked monster. */
void emc_event_highlight_set(u8 event) {
    EmcEvent* ev = emc_event_tbl_get(event);
    if (ev != NULL) {
        EmcEventMark* mark = ev->mark;
        if (mark != NULL) {
            emc_highlight_set(mark->kind, mark->area, mark->area_match, event);
        }
    }
}

/* Clears the event holds. */
void emc_event_hold_init(void) {
    emc_event_hold.event = -1;
    emc_event_hold.rec[0].active = 0;
    emc_event_hold.rec[0].order = 0xFF;
    emc_event_hold.rec[1].active = 0;
    emc_event_hold.rec[1].order = 0xFF;
    emc_event_hold.rec[2].active = 0;
    emc_event_hold.rec[2].order = 0xFF;
    emc_event_hold.rec[3].active = 0;
    emc_event_hold.rec[3].order = 0xFF;
    emc_event_hold.rec[4].active = 0;
    emc_event_hold.rec[4].order = 0xFF;
}

/* Holds event `event`'s targets in place (at most five): moves each to its target position and area and freezes it. */
void emc_event_hold_set(u8 event) {
    _ENEMY_WORK* works = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 max = get_move_work_max(3);
    EmcEvent* ev = emc_event_tbl_get(event);
    if (ev != NULL && ev->active != 0) {
        EmcEventTarget* target = ev->targets;
        if (target != NULL) {
            s32 taken;
            u8 n;
            emc_event_hold.event = event;
            taken = 0;
            n = 0;
            for (; target->kind != 0; target++) {
                s16 i;
                _ENEMY_WORK* work;
                if (n >= 5) {
                    return;
                }
                for (i = 0, work = works; i < max; i++, work++) {
                    s32 bit;
                    if (work->active != 0 && ((bit = 1 << i), (taken & bit) == 0) && work->team == target->kind &&
                        (target->area_match == 0 || work->field_0x00A == target->area)) {
                        if (target->pos != NULL) {
                            if (work->area_no != emc_event_area_get() &&
                                em_area_enter_ck(work, work->field_0x1E0, emc_event_area_get()) == 1) {
                                work->field_0x9F7 = emc_event_area_get();
                                em_area_change(work, 1);
                            }
                            setVector3(&work->pos, target->pos->pos.x, target->pos->pos.y, target->pos->pos.z);
                            copyVec3(&work->vec_0x194, &work->pos);
                            work->field_0x1C0 = target->pos->yaw;
                        } else if (work->area_no != emc_event_area_get()) {
                            continue;
                        }
                        emc_event_hold.rec[n].active = 1;
                        emc_event_hold.rec[n].order = work->field_0x01A;
                        work->freeze_0x46A = 1;
                        n++;
                        taken |= bit;
                        break;
                    }
                }
            }
        }
    }
}

/* Releases event `event`'s holds. */
void emc_event_hold_release(u8 event) {
    _ENEMY_WORK* works = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 max = get_move_work_max(3);
    if (emc_event_hold.event == event) {
        u8 n = 0;
        s32 k;
        for (k = 5; k != 0; k--, n++) {
            EmcHoldRec* hold = &emc_event_hold.rec[n];
            if (hold->active != 0) {
                u8 i = 0;
                _ENEMY_WORK* work = works;
                for (; i < max; i++, work++) {
                    if (work->active != 0 && work->field_0x01A == hold->order) {
                        work->freeze_0x46A = 0;
                        hold->active = 0;
                        break;
                    }
                }
            }
        }
        emc_event_hold_init();
    }
}

/* Moves every live enemy (and waiting area record) of `kind` that is not in area `area` there. */
void emc_area_move_kind(u8 kind, u8 area) {
    s16 i;
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 max = get_move_work_max(3);
    EmAreaEntry* entry = &em_area_entry_tbl[0][0];
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->team == kind && work->area_no != area &&
            em_area_enter_ck(work, work->field_0x1E0, area) == 1) {
            work->field_0x9F7 = area;
            em_area_change(work, 1);
            work->field_0x9F6 = work->area_no;
        }
    }
    if (entry != NULL) {
        s32 n;
        for (n = 0; n < 128; n++, entry++) {
            u8 state;
            if (entry->active != 0 && ((state = entry->state_0x08) == 0 || state == 2) && entry->monster_0x02 == kind &&
                entry->area_0x0A != area && em_area_entry_enter_ck(entry, get_now_mapno(), area) == 1) {
                entry->area_0x0A = area;
                entry->area_prev_0x0B = entry->area_0x0A;
            }
        }
    }
}

/* Moves event `event`'s marked monster into the event's area. */
void emc_event_area_move(u8 event) {
    EmcEvent* ev = emc_event_tbl_get(event);
    if (ev != NULL) {
        EmcEventMark* mark = ev->mark;
        if (mark != NULL) {
            emc_area_move_kind(mark->kind, emc_event_area_get());
        }
    }
}

#pragma push
#pragma optimization_level 4
/* Issues event `event`'s spawn requests: one per spawn row, from the first live enemy (else area record) of the
 * row's kind. */
void emc_event_spawn(u8 event) {
    EmAreaEntry* entries;
    _ENEMY_WORK* works;
    s16 max;
    EmcEvent* ev;
    works = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    entries = &em_area_entry_tbl[0][0];
    ev = emc_event_tbl_get(event);
    if (ev != NULL) {
        EmcEventSpawn* spawn = ev->spawns;
        if (spawn != NULL) {
            u8 kind;
            while ((kind = spawn->kind) != 0) {
                s32 found = 0;
                s16 i;
                _ENEMY_WORK* work;
                for (i = 0, work = works; i < max; i++, work++) {
                    if (work->active != 0 && work->team == kind) {
                        u8 entry;
                        u8 count;
                        if (spawn->keep_entry == 1) {
                            entry = work->field_0x46C;
                        } else {
                            entry = 0xFF;
                        }
                        count = spawn->count;
                        em_spawn_request(work->field_0x01A, spawn->req_kind, spawn->req_team, emc_event_area_get(),
                                         entry, count, count, spawn->flags, 0xFF, NULL, 0);
                        found = 1;
                        break;
                    }
                }
                if (found == 0) {
                    u8 n;
                    EmAreaEntry* e;
                    if (entries == NULL) {
                        return;
                    }
                    for (n = 0, e = entries; n < 128; n++, e++) {
                        if (e->active != 0 && e->monster_0x02 == spawn->kind) {
                            u8 entry;
                            u8 count;
                            if (spawn->keep_entry == 1) {
                                entry = e->entry_0x09;
                            } else {
                                entry = 0xFF;
                            }
                            count = spawn->count;
                            em_spawn_request(e->order_0x16, spawn->req_kind, spawn->req_team, emc_event_area_get(),
                                             entry, count, count, spawn->flags, 0xFF, NULL, 0);
                            break;
                        }
                    }
                }
                spawn++;
            }
        }
    }
}
#pragma pop

/* Clears the event action records. */
void emc_event_act_init(void) {
    s32 i;
    for (i = 0; i < 5; i++) {
        emc_event_act_tbl[i].active = 0;
        emc_event_act_tbl[i].act = 0xFF;
        emc_event_act_tbl[i].sub = 0xFF;
        emc_event_act_tbl[i].end_act = 0xFF;
        emc_event_act_tbl[i].end_sub = 0xFF;
        emc_event_act_tbl[i].work = NULL;
    }
}

/* Requests event `event`'s actions. */
void emc_event_act_request(u8 event) {
    emc_event_state.act_req = 1;
    emc_event_state.act_event = event;
}

/* Starts event `event`'s actions: every matched target in the player's area gets action 13. */
void emc_event_act_start(u8 event) {
    s16 i;
    u8 slot;
    s16 t;
    _ENEMY_WORK* work;
    EmcEventTarget* target;
    _ENEMY_WORK* works = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 max = get_move_work_max(3);
    EmcEvent* ev = emc_event_tbl_get(event);
    if (ev != NULL) {
        target = ev->targets;
        if (target != NULL) {
            for (i = 0, work = works; i < max; i++, work++) {
                if (work->active != 0) {
                    work->visible_0x001 = 0;
                }
            }
            slot = 0;
            t = 0;
            for (; target->kind != 0; target++, t++) {
                for (i = 0, work = works; i < max; i++, work++) {
                    if (work->active != 0 && (work->event_slot_0x46B == -1 || work->event_slot_0x46B == t) &&
                        work->team == target->kind && (target->area_match == 0 || work->field_0x00A == target->area) &&
                        work->area_no == get_now_areano()) {
                        EmcEventAct* act;
                        em_act_arm_unless_down(work, 13, target->act);
                        em_user_data_reenter(work);
                        em_state_refresh(work);
                        work->event_slot_0x46B = t;
                        work->visible_0x001 = 1;
                        em_anger_clear(work);
                        act = &emc_event_act_tbl[slot];
                        act->active = 1;
                        act->act = 13;
                        act->sub = target->act;
                        act->end_act = target->end_act;
                        act->end_sub = target->end_sub;
                        act->work = work;
                        slot++;
                        break;
                    }
                }
            }
        }
    }
}

/* Requests the end of the event's actions. */
void emc_event_act_end_request(void) {
    emc_event_state.act_end_req = 1;
}

/* Ends the event's actions: every enemy given one gets its end action and is handed back to the demo control. */
void emc_event_act_end(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s16 max = get_move_work_max(3);
    s16 j;
    u32 i;
    for (j = 0; j < max; j++, work++) {
        if (work->active != 0) {
            work->visible_0x001 = 1;
        }
    }
    i = 0;
    do {
        EmcEventAct* act = &emc_event_act_tbl[(u8)i];
        if (act->active != 0) {
            _ENEMY_WORK* target;
            act->active = 0;
            target = act->work;
            if (target != NULL) {
                if ((u32)em_act_ck(target, act->act, act->sub) == 1) {
                    em_act_arm_unless_down(target, act->end_act, act->end_sub);
                }
                target->event_slot_0x46B = -1;
                em_demo_enable(target);
            }
        }
        i++;
    } while (i < 5);
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the demo control */

/* The demo work's current frame. */
s16 em_demo_frame_get(void) {
    return get_demo_data();
}

/* 1 once the demo frame has passed half of `frames`. */
u32 em_demo_time_ck(u32 frames) {
    return (s16)frames / 2 <= em_demo_frame_get();
}

/* Places live enemy `self` at (`x`, `y`, `z`) and makes it its previous position too. */
void em_demo_pos_set(_ENEMY_WORK* self, f32 x, f32 y, f32 z) {
    setVector3(&self->pos, x, y, z);
    copyVec3(&self->vec_0x194, &self->pos);
}

/* The 16-bit angle of `deg` degrees (rounded away from zero). */
s32 em_demo_angle_get(f32 deg) {
    s32 a;
    if (deg < 0.0f) {
        deg *= -1.0f;
        a = 0.5f + 65536.0f * deg / 360.0f;
        return -(u16)a;
    }
    a = 0.5f + 65536.0f * deg / 360.0f;
    return (u16)a;
}

/* Turns live enemy `self` to the angles (`x`, `y`, `z`) in degrees. */
void em_demo_rot_set(_ENEMY_WORK* self, f32 x, f32 y, f32 z) {
    self->field_0x1BC = em_demo_angle_get(x);
    self->field_0x1C0 = em_demo_angle_get(y);
    self->field_0x1C4 = em_demo_angle_get(z);
}

/* Hands live enemy `self` out of the demo control and places it at `pos` (NULL: far away). */
void em_demo_reset(_ENEMY_WORK* self, const nw4r::math::VEC3* pos) {
    self->visible_0x001 = 0;
    if (pos != NULL) {
        em_demo_pos_set(self, pos->x, pos->y, pos->x);
        return;
    }
    em_demo_pos_set(self, 10000.0f, 10000.0f, 10000.0f);
}

/* Hands live enemy `self` back to the demo control. */
void em_demo_enable(_ENEMY_WORK* self) {
    self->visible_0x001 = 1;
}

/* Applies the position and rotation key curves at frame `frame * 2`. */
/* untyped: caller-owned payload - each enemy program passes its key tables in its own type */
void em_demo_key3_apply(_ENEMY_WORK* self, s16 frame, void* pos_keys_arg, void* rot_keys_arg) {
    f32* pos_keys = (f32*)pos_keys_arg;
    f32* rot_keys = (f32*)rot_keys_arg;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 rot;
    f32 t;
    VEC3_ctor(&pos);
    VEC3_ctor(&rot);
    t = frame * 2;
    if (pos_keys != NULL) {
        getKeyData3(pos_keys, t, &pos.x, &pos.y, &pos.z);
        em_demo_pos_set(self, pos.x, pos.y, pos.z);
    }
    if (rot_keys != NULL) {
        getKeyData3(rot_keys, t, &rot.x, &rot.y, &rot.z);
        em_demo_rot_set(self, rot.x, rot.y, rot.z);
    }
}

/* Applies one, two or three axes of the position and rotation key curves at frame `frame * 2` (axis masks: 1 x,
 * 2 y, 4 z). */
/* untyped: caller-owned payload - each enemy program passes its key tables in its own type */
void em_demo_key_apply(_ENEMY_WORK* self, s16 frame, void* pos_keys_arg, void* rot_keys_arg, u8 pos_axes,
                       u8 rot_axes) {
    f32 a;
    f32 b;
    f32 c;
    f32 t = frame * 2;
    if (pos_keys_arg != NULL && pos_axes != 0) {
        if (pos_axes == 1 || pos_axes == 2 || pos_axes == 4) {
            a = getKeyData((f32*)pos_keys_arg, t);
            switch (pos_axes) {
            case 1:
                self->pos.x = a;
                break;
            case 2:
                self->pos.y = a;
                break;
            case 4:
                self->pos.z = a;
                break;
            }
        } else if (pos_axes == 7) {
            getKeyData3((f32*)pos_keys_arg, t, &a, &b, &c);
            setVector3(&self->pos, a, b, c);
        } else {
            anim_key_curve_eval_c1((f32*)pos_keys_arg, t, &a, &b);
            switch (pos_axes) {
            case 3:
                self->pos.x = a;
                self->pos.y = b;
                break;
            case 5:
                self->pos.x = a;
                self->pos.z = b;
                break;
            case 6:
                self->pos.y = a;
                self->pos.z = b;
                break;
            }
        }
        copyVec3(&self->vec_0x194, &self->pos);
    }
    if (rot_keys_arg != NULL && rot_axes != 0) {
        if (rot_axes == 1 || rot_axes == 2 || rot_axes == 4) {
            a = getKeyData((f32*)rot_keys_arg, t);
            switch (rot_axes) {
            case 1:
                self->field_0x1BC = em_demo_angle_get(a);
                break;
            case 2:
                self->field_0x1C0 = em_demo_angle_get(a);
                break;
            case 4:
                self->field_0x1C4 = em_demo_angle_get(a);
                break;
            }
        } else if (rot_axes == 7) {
            getKeyData3((f32*)rot_keys_arg, t, &a, &b, &c);
            em_demo_rot_set(self, a, b, c);
        } else {
            anim_key_curve_eval_c1((f32*)rot_keys_arg, t, &a, &b);
            switch (rot_axes) {
            case 3:
                self->field_0x1BC = em_demo_angle_get(a);
                self->field_0x1C0 = em_demo_angle_get(b);
                break;
            case 5:
                self->field_0x1BC = em_demo_angle_get(a);
                self->field_0x1C4 = em_demo_angle_get(b);
                break;
            case 6:
                self->field_0x1C0 = em_demo_angle_get(a);
                self->field_0x1C4 = em_demo_angle_get(b);
                break;
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* the level's resource buffers */

/* the main buffer sizes, by size class. */
u32 em_res_size_tbl[4] = {0xDC000, 0x108000, 0x358000, 0x400000};

/* per level: the size classes of its six main buffers. */
s8 em_level_res_tbl[5][6] = {
    {2, 2, 1, 0, 0, 0}, {2, 2, 0, 0, 0, 0}, {2, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {3, 0, 0, 0, 0, 0},
};

#pragma push
#pragma optimization_level 4
/* Carves level `level`'s resource buffers out of one resource-memory block: the six main buffers of its size
 * classes, a sub-buffer for each of the first six main buffers' classes, and clears the effect sets. */
void em_level_set(s8 level) {
    EmcWork* work = &emc_work;
    s32 i;
    s8* cls;
    u8* base;
    EmResBuffer* main;
    EmResBuffer* sub;

    snd_em_bank_level_load(level);
    work->res_mem_handle = -1;
    work->res_mem_base = NULL;
    work->res_mem_size = 0xB6C000;
    main = work->res_buf;
    sub = work->sub_buf;
    for (i = 0; i < 6; i++, main++, sub++) {
        memset(main, 0, sizeof(EmResBuffer));
        main->size_class = 0xFF;
        memset(sub, 0, sizeof(EmResBuffer));
        sub->size_class = 0xFF;
    }
    work->res_mem_handle = pull_res_mem(NULL, work->res_mem_size, 1);
    work->res_mem_base = (u8*)getResMemAdrs(work->res_mem_handle);
    for (i = 0; i < 4; i++) {
        s32 j;
        for (j = 0; j < 4; j++) {
            work->peer_count[i][j] = 0;
        }
    }
    cls = em_level_res_tbl[level];
    main = work->res_buf;
    base = work->res_mem_base;
    for (i = 0; i < 6; i++, cls++) {
        if (*cls != -1) {
            main->size_class = *cls;
            main->base = base;
            main->size = em_res_size_tbl[main->size_class];
            main->used_size = 0;
            main->free_size = main->size;
            base += main->size;
            work->peer_count[main->size_class][0]++;
            work->peer_count[main->size_class][1]++;
            work->peer_count[main->size_class][2]++;
            work->peer_count[main->size_class][3]++;
            main++;
        }
    }
    main = work->res_buf;
    sub = work->sub_buf;
    for (i = 0; i < 6; i++, main++, sub++) {
        s32 c = main->size_class;
        if ((u32)(c + 1) > 1) {
            switch (c) {
            case 1:
                sub->size_class = 1;
                sub->base = base;
                sub->size = 0x30000;
                break;
            case 2:
                sub->size_class = 2;
                sub->base = base;
                sub->size = 0x78000;
                break;
            case 3:
                sub->size_class = 3;
                sub->base = base;
                sub->size = 0xC0000;
                break;
            }
            sub->used_size = 0;
            sub->free_size = sub->size;
            base += sub->size;
        }
    }
    for (i = 0; i < 6; i++) {
        EmcEftSet* set = work->eft_set;
        set->count_b = 0;
        set->count_c = 0;
        set->id_a = 0xFF;
        {
            s32 j;
            for (j = 0; j < 10; j++) {
                set->ids_b[j] = 0xFF;
            }
            for (j = 0; j < 10; j++) {
                set->ids_c[j] = 0xFF;
            }
        }
    }
}
#pragma pop

/* Gives the level's resource-memory block back and clears the buffers. */
void em_res_buffers_free(void) {
    s32 i;
    EmResBuffer* main;
    EmResBuffer* sub;
    push_res_mem(emc_work.res_mem_handle);
    emc_work.res_mem_handle = -1;
    emc_work.res_mem_base = NULL;
    emc_work.res_mem_size = 0xB6C000;
    main = emc_work.res_buf;
    sub = emc_work.sub_buf;
    for (i = 0; i < 6; i++) {
        memset(main, 0, sizeof(EmResBuffer));
        main->size_class = 0xFF;
        memset(sub, 0, sizeof(EmResBuffer));
        sub->size_class = 0xFF;
        main++;
        sub++;
    }
}

/* Whether every active player (that is not leaving) still has a size-`size_class` buffer to free. */
u32 em_res_peers_ready_ck(s8 size_class) {
    _PLW* pl = (_PLW*)get_move_work_adrs(2);
    s16 max = get_move_work_max(2);
    if (PlayMode_ck() != 2) {
        s32 i;
        for (i = 0; i < max; i++, pl++) {
            if (pl->slot_active != 0 && em_pl_slot_leaving_ck(pl->chunk_ofs) == 0 &&
                emc_work.peer_count[size_class][i] == 0) {
                return 0;
            }
        }
    } else if (emc_work.peer_count[size_class][0] == 0) {
        return 0;
    }
    return 1;
}

/* Whether monster kind `kind` may take its resource buffer now. */
s32 em_kind_release_ck(u8 kind) {
    EmResBuffer* buf = em_res_buffer_find(kind);
    if (buf != NULL) {
        if (em_res_class_tbl[kind] <= 0) {
            return 1;
        }
        if (em_res_peers_ready_ck(buf->size_class) == 1) {
            return 1;
        }
    }
    return 0;
}

/* Counts one size-`size_class` buffer as freed by every player. */
void em_res_peer_count_dec(s8 size_class) {
    s32 i;
    for (i = 0; i < 4; i++) {
        if (emc_work.peer_count[size_class][i] != 0) {
            emc_work.peer_count[size_class][i]--;
        }
    }
}

/* Counts one size-`size_class` buffer as still held by player `player`. */
void em_res_peer_count_inc(s8 size_class, u8 player) {
    emc_work.peer_count[size_class][player]++;
}

/* Tells the companions this player freed a size-`size_class` buffer. */
void em_res_free_notify(s8 size_class) {
    lb_sub10_send(my_player_no(), size_class);
}

/* The free main buffer monster kind `kind` takes: its size class, else the next two larger ones; NULL when none is
 * free (or the companions still hold theirs). */
EmResBuffer* em_res_buffer_find(u8 kind) {
    EmcWork* work = &emc_work;
    s8 cls = em_res_class_tbl[kind];
    EmResBuffer* buf;
    u8 i;
    if (cls == -1) {
        return NULL;
    }
    buf = work->res_buf;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used == 0 && buf->size_class == cls) {
            if (cls <= 0 || em_res_peers_ready_ck(buf->size_class) == 1) {
                return buf;
            }
            return NULL;
        }
    }
    buf = work->res_buf;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used == 0 && buf->size_class == cls + 1) {
            if (cls <= 0 || em_res_peers_ready_ck(buf->size_class) == 1) {
                return buf;
            }
            return NULL;
        }
    }
    if (cls == 1) {
        return NULL;
    }
    buf = work->res_buf;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used == 0 && buf->size_class == cls + 2) {
            if (cls <= 0 || em_res_peers_ready_ck(buf->size_class) == 1) {
                return buf;
            }
            return NULL;
        }
    }
    return NULL;
}

/* Claims the main buffer monster kind `kind` takes. */
EmResBuffer* em_res_buffer_claim(u8 kind) {
    EmResBuffer* buf = em_res_buffer_find(kind);
    if (buf != NULL) {
        buf->used = 1;
        buf->owner = kind;
        em_res_peer_count_dec(buf->size_class);
    }
    return buf;
}

/* Releases monster kind `kind`'s main buffer (and tells the companions). */
void em_res_buffer_release(u8 kind) {
    EmResBuffer* buf = emc_work.res_buf;
    s32 n;
    for (n = 6; n != 0; n--, buf++) {
        if (buf->used != 0 && buf->owner == kind) {
            memset(buf->base, 0, buf->size);
            buf->used_size = 0;
            buf->free_size = buf->size;
            buf->used = 0;
            buf->owner = 0;
            if (em_res_class_tbl[kind] > 0) {
                em_res_free_notify(buf->size_class);
            }
            return;
        }
    }
}

/* `size` bytes (rounded up to 32) of monster kind `kind`'s main buffer; NULL when it is full. */
u8* em_res_buffer_alloc(u8 kind, u32 size) {
    u32 len = (size + 31) & ~31;
    EmResBuffer* buf = emc_work.res_buf;
    s32 n;
    for (n = 6; n != 0; n--, buf++) {
        if (buf->used != 0 && buf->owner == kind) {
            if (buf->free_size < len) {
                break;
            }
            {
                u8* p = buf->base + buf->used_size;
                buf->used_size += len;
                buf->free_size -= len;
                return p;
            }
        }
    }
    return NULL;
}

/* The sub-buffer owner id `id` holds, NULL when none. */
EmResBuffer* em_sub_buffer_find(u8 id) {
    EmResBuffer* buf = emc_work.sub_buf;
    s32 i;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used != 0 && buf->owner == id) {
            return buf;
        }
    }
    return NULL;
}

/* The first free sub-buffer of size class `size_class`, NULL when none. */
EmResBuffer* em_sub_buffer_free_get(u8 size_class) {
    EmResBuffer* buf = emc_work.sub_buf;
    s32 i;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used == 0 && buf->size_class == size_class) {
            return buf;
        }
    }
    return NULL;
}

/* Claims the sub-buffer of monster kind `kind` (0xFF: a shared class-0 one): the owner id is the number in the kind's
 * file name; a buffer that id already holds is shared (and NULL returned). */
EmResBuffer* em_sub_buffer_claim(u8 kind) {
    EmResBuffer* buf;
    s32 id;
    s32 c;
    s8 cls;
    EmcWork* work = &emc_work;
    char name[256];
    u8 slot;

    if (kind == 0xFF) {
        buf = em_sub_buffer_free_get(0);
        if (buf == NULL) {
            return NULL;
        }
        buf->owner = kind;
        buf->used = 1;
        buf->refs = 1;
        return buf;
    }
    cls = em_res_class_tbl[kind];
    if (cls <= 0) {
        return NULL;
    }
    if (em_file_name_tbl[kind] == NULL) {
        return NULL;
    }
    strcpy(name, em_file_name_tbl[kind]);
    id = atoi(&name[2]);
    slot = em_kind_slot_find(kind);
    if (slot != 0xFF) {
        work->slot_0x000[slot].field_0x04 = id;
    }
    buf = em_sub_buffer_find(id);
    if (buf != NULL) {
        buf->refs++;
        return NULL;
    }
    if (cls == 3) {
        c = 3;
    } else if (cls == 1) {
        c = 1;
    } else {
        c = 2;
    }
    for (;;) {
        buf = em_sub_buffer_free_get(c);
        if (buf != NULL) {
            break;
        }
        if (c != 1) {
            return NULL;
        }
        c = 2;
    }
    buf->owner = id;
    buf->used = 1;
    buf->refs = 1;
    return buf;
}

#pragma push
#pragma opt_lifetimes off
/* Drops one reference of monster kind `kind`'s sub-buffer; the last one releases its effect resources and clears it.
 * Returns the references left. */
u8 em_sub_buffer_release(u8 kind) {
    EmcWork* work = &emc_work;
    s32 slot = em_kind_slot_find(kind);
    EmResBuffer* buf;
    s32 n;
    if (slot != 0xFF) {
        kind = work->slot_0x000[slot].field_0x04;
        if (kind == 0) {
            return 0;
        }
    }
    buf = work->sub_buf;
    for (n = 6; n != 0; n--, buf++) {
        if (buf->used != 0 && buf->owner == kind) {
            s32 index;
            EmcEftSet* set;
            buf->refs--;
            if (buf->refs != 0) {
                return buf->refs;
            }
            index = em_sub_buffer_index_get(kind, 1);
            if (index >= 0) {
                s32 i;
                set = &emc_work.eft_set[index];
                if (set->id_a != 0xFF) {
                    eft_res_release(set->id_a);
                    set->id_a = 0xFF;
                }
                for (i = 0; i < set->count_b; i++) {
                    if (set->ids_b[i] != 0xFF) {
                        eft_res_release(set->ids_b[i]);
                        set->ids_b[i] = 0xFF;
                    }
                }
                for (i = 0; i < set->count_c; i++) {
                    if (set->ids_c[i] != 0xFF) {
                        eft_res_release(set->ids_c[i]);
                        set->ids_c[i] = 0xFF;
                    }
                }
            }
            set->count_b = 0;
            set->count_c = 0;
            memset(buf->base, 0, buf->size);
            buf->used_size = 0;
            buf->free_size = buf->size;
            buf->used = 0;
            buf->owner = 0;
            buf->refs = 0;
            return buf->refs;
        }
    }
    return 0;
}
#pragma pop

/* `size` bytes (rounded up to 32) of monster kind `kind`'s sub-buffer; NULL when it is full. */
u8* em_kind_mem_alloc(u8 kind, u32 size) {
    EmcWork* work = &emc_work;
    u32 len = (size + 31) & ~31;
    s32 slot = em_kind_slot_find(kind);
    u8 id = kind;
    EmResBuffer* buf;
    s32 n;
    if (slot != 0xFF) {
        id = work->slot_0x000[slot].field_0x04;
        if (id == 0) {
            return NULL;
        }
    }
    buf = work->sub_buf;
    for (n = 6; n != 0; n--, buf++) {
        if (buf->used != 0 && buf->owner == id) {
            if (buf->free_size < len) {
                break;
            }
            {
                u8* p = buf->base + buf->used_size;
                buf->used_size += len;
                buf->free_size -= len;
                return p;
            }
        }
    }
    return NULL;
}

/* The references of monster kind `kind`'s sub-buffer (0: none). */
u8 em_sub_buffer_refs_get(u8 kind) {
    EmcWork* work = &emc_work;
    s32 slot = em_kind_slot_find(kind);
    u8 id = kind;
    EmResBuffer* buf;
    s32 i;
    if (slot != 0xFF) {
        id = work->slot_0x000[slot].field_0x04;
        if (id == 0) {
            return 0;
        }
    }
    buf = work->sub_buf;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used != 0 && buf->owner == id) {
            return buf->refs;
        }
    }
    return 0;
}

/* The index of monster kind `kind`'s sub-buffer (`raw`: `kind` is already the owner id); -1 when none. */
s32 em_sub_buffer_index_get(u8 kind, u8 raw) {
    EmcWork* work = &emc_work;
    s32 i;
    if (raw == 0) {
        s32 slot = em_kind_slot_find(kind);
        if (slot != 0xFF) {
            kind = work->slot_0x000[slot].field_0x04;
            if (kind == 0) {
                return -1;
            }
        }
    }
    for (i = 0; i < 6; i++) {
        EmResBuffer* buf = &work->sub_buf[i];
        if (buf->used != 0 && buf->owner == kind) {
            return i;
        }
    }
    return -1;
}

/* The first free main buffer's base, NULL when all are taken. */
u8* em_res_buffer_get(void) {
    EmResBuffer* buf = emc_work.res_buf;
    s32 i;
    for (i = 0; i < 6; i++, buf++) {
        if (buf->used == 0 && buf->size_class == 0) {
            return buf->base;
        }
    }
    return NULL;
}

/* The address of entry `index` of area block `area` for entry kind `kind` (0..2): the block starts with a word table
 * of `{offset << 2}` words per kind, each pointing at `{offset << 2, count}` pairs per area. */
u32 em_area_entry_make(u8* area_arg, u8 kind, u8 index) {
    u32* area = (u32*)area_arg;
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    u32* p;
    u32 i;
    u32 n;
    if (root == NULL) {
        return 0;
    }
    if (area == NULL) {
        return 0;
    }
    if (kind > 2) {
        return 0;
    }
    if (index >= stage_map_area_count_get(root->area_no_0xED)) {
        return 0;
    }
    p = area;
    for (i = 0; i < kind; i++) {
        p++;
    }
    n = *p >> 2;
    if (n == 0) {
        return 0;
    }
    p = area;
    for (i = 0; i < n; i++) {
        p++;
    }
    for (i = 0; i < index; i++) {
        p += 2;
    }
    n = p[0] >> 2;
    if (p[1] == 0) {
        return 0;
    }
    p = area;
    for (i = 0; i < n; i++) {
        p++;
    }
    return (u32)p;
}

}

#pragma dont_inline on

/* Constructs a sparkle record's position. */
inline SenkoRec::SenkoRec() {
    VEC3_ctor(&pos_0x00);
}

/* Constructs a smoke record's position. */
inline KemuriRec::KemuriRec() {
    VEC3_ctor(&pos_0x00);
}

/* Constructs a spawn request's position. */
inline Marker2Rec::Marker2Rec() {
    VEC3_ctor(&pos_0x0C);
}

/* Constructs a cached joint matrix. */
inline EmcJointMtx::EmcJointMtx() {
    MTX34_ctor(&mtx);
}
