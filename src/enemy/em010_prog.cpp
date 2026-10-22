/*
 * enemy/em010_prog.cpp - enemy 010's program: the `em010_prog_tbl` entry points (init, frame update, action
 *   dispatch, motion events, area start, state query, area change and the three mini-work hooks), the per-action
 *   state steps the `state_sub` dispatchers select, the per-map seat selector, the two sound programs, the demo
 *   (cut-scene) steps, the effect spawner and the static initializer.
 * RANGE. .text 0x801663E4-0x8016D1C4 (77 functions); extab 0x8000E1D4-0x8000E3DC, extabindex
 *   0x800291A8-0x800294B4, .ctors 0x8056F32C-0x8056F330 (`__sinit_\em010_prog_cpp`), .rodata 0x8056FC10-0x8056FCD0,
 *   .data 0x805A6D58-0x805A7CE8 (`em010_prog_tbl` first), .bss 0x806A7898-0x806A7970, .sdata2 0x807974F0-0x807977D8.
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide (retail keeps the `clrlwi` re-mask of each `u8` argument
 *   before its compare and the unfused `addi`/`stb` state step), `#pragma pool_data off` (each sound table is addressed
 *   with its own `lis`/`addi`).
 * NAMES. The file name follows the runtime dump's `em010_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em010ProgTbl` slot each fills, after `Em006ProgTbl`/`Em022ProgTbl`): em010_init, em010_frame_update,
 *   em010_action_dispatch, em010_event_notify, em010_motion_events, em010_area_start_set, em010_state_clear_ck,
 *   em010_area_change_ck, em010_mini_init, em010_mini_step, em010_mini_state_reset.
 *   GUESS (dispatcher slot): em010_act<action>_<first sub-state> and em010_act<action>_dispatch,
 *   em010_act10_se_play, em010_act11_se_play; GUESS (from the bodies): em010_area_entry_target_set,
 *   em010_effect_spawn.  `__sinit_\em010_prog_cpp` is MWCC's own static initializer.
 *   GUESS (data): em010_turn_seq_0..2, em010_demo_keys, em010_se_seq_<sub-state>, em010_se_seq_alt_<sub-state>,
 *   em010_se_rec_<seq>[_a..], em010_se_pos_0..5, em010_vec_pair_0..8, the `Em010ProgTbl` slot names.
 * RESIDUALS. Every row byte-identical, every claimed data section emitted in retail order (`.data`/`.sdata2` short
 *   only by the 4-byte tail padding the link adds).  flipcheck: the three registry-named callees `em_area_vacant_wait_ck`
 *   (0x8012E968), `em_attack_finished_clear` (0x8012E8DC) and `stg_area_flag_index_ck` (0x802B0998) are registry
 *   proposals (requests `enemy-em010#1..3`), not map names yet, so a flip links `undefined` until they land.
 * SHAPES. The state steps are `switch (self->state)` with `self->state += 1` (a `u8` copy plus `copy + 1` costs a
 *   `clrlwi`); a case-level `return` at the end of a state switch costs a `b`, so the steps `break`.  The binary-tree
 *   `switch`es of `em010_motion_events` list each case retail compares (107 and 108 share a body).  A one-case test
 *   retail compares signed (`cmpwi`) is a `switch` (`em010_area_entry_target_set`'s map 8).  Callees whose result
 *   retail compares unsigned (`em_act_ck`, `em_area_vacant_wait_ck`) or that take a float the owner's header omits
 *   (`em_fall_start`, `em_ground_ck`) are called through `_c1` views.  The data is defined between the functions in
 *   retail order (the jump tables fall between the definitions); the mini-work hooks take the area-table record
 *   (`_ENEMY_MINI_WORK`) and hand it to `EmAreaEntry`/`_ENEMY_WORK` callees by cast.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_8011D448.h"
#include "fn_8004CAD8.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "ef/fn_8010D1A8.h"
#include "ef/eft_slot.h"
#include "ef.h"
#include "stage/shell_set_func_ptr.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "enemy/EnemyData.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_model.h"
#include "enemy/em_fall_start.h"
#include "enemy/em_frame_flag_set.h"
#include "enemy/em_mot_set_blend.h"
#include "enemy/em_move_vec2_clr.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/em_hit_window_set_default.h"
#include "enemy/CancelFade.h"
#include "enemy/em_common.h"
#include "enemy/EmAreaEntry.h"
#include "enemy/em012_area_move_armed_ck.h"
#include "enemy/em_part_spot_nearest_get.h"
#include "ef/fn_800CDB2C.h" /* ran_suu */
#include "enemy/em_area_vacant_wait_ck.h"
#include "enemy/em_attack_finished_clear.h"
#include "stage/stg_area_flag_index_ck.h"

#define em_area_vacant_wait_ck_c1 ((u32 (*)(_ENEMY_WORK*, u8))em_area_vacant_wait_ck)

#pragma pool_data off
#pragma peephole off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)
#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)

/* The +0x3C shell-table slot spelled without its trailing table argument: the call site passes none. */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The em010 program table (`em010_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em006ProgTbl` and `Em022ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em010ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ u32 pad_0x10;
    /* +0x14 */ void (*event_notify_0x14)(void);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C[5];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ u32 pad_0x34[6];
    /* +0x4C */ s32 (*state_clear_ck_0x4C)(struct _ENEMY_WORK* self);
    /* +0x50 */ u32 pad_0x50;
    /* +0x54 */ s32 (*area_change_ck_0x54)(struct _ENEMY_WORK* self);
    /* +0x58 */ u32 pad_0x58;
    /* +0x5C */ void (*mini_init_0x5C)(void);
    /* +0x60 */ void (*mini_step_0x60)(_ENEMY_MINI_WORK* mini);
    /* +0x64 */ void (*mini_state_reset_0x64)(_ENEMY_MINI_WORK* mini);
    /* +0x68 */ u32 pad_0x68;
};

/* A point and its per-frame step, built by MWCC's `__sinit`: each vector is set through `setVec3` into a temporary
 * and copied with `assignVec3`.  size: 0x18 */
struct Em010VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (x, y, z) and (dx, dy, dz). */
    Em010VecPair(f32 x, f32 y, f32 z, f32 dx, f32 dy, f32 dz)
    {
        VEC3 b;
        VEC3 a;

        assignVec3_c1(&pos, setVec3(&a, x, y, z));
        assignVec3_c1(&step, setVec3(&b, dx, dy, dz));
    }
};

/* The nine vector pairs (`.bss`, defined at the end); the sound records point at them. */
extern Em010VecPair em010_vec_pair_0;
extern Em010VecPair em010_vec_pair_1;
extern Em010VecPair em010_vec_pair_2;
extern Em010VecPair em010_vec_pair_3;
extern Em010VecPair em010_vec_pair_4;
extern Em010VecPair em010_vec_pair_5;
extern Em010VecPair em010_vec_pair_6;
extern Em010VecPair em010_vec_pair_7;
extern Em010VecPair em010_vec_pair_8;

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * four-word rows whose first word carries the motion in its high half. */
extern "C" const u32 em010_turn_seq_0[16] = {
    0x00000028, 0x00000032, 0x0000001E, 0x400031C7,
    0x000A0000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
    0x00060000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em010_turn_seq_1[16] = {
    0x00000000, 0x0000001E, 0x00000000, 0x40000000,
    0x00100000, 0x00000004, 0x00000000, 0x00000000,
    0x00110000, 0x00000004, 0x00000000, 0x00000000,
    0x00100000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em010_turn_seq_2[16] = {
    0x00000028, 0x00000032, 0x0000001E, 0x400031C7,
    0x000A0000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000010, 0x00000024, 0x00000000,
    0x00060000, 0x00000010, 0x00000024, 0x00000000,
};

extern "C" {

u32 em010_area_entry_target_set(_ENEMY_WORK* self);
void em010_init(_ENEMY_WORK* self, u8 kind);
void em010_event_notify(void);
void em010_frame_update(_ENEMY_WORK* self);
void em010_act0_0(_ENEMY_WORK* self);
void em010_act0_1(_ENEMY_WORK* self);
void em010_act0_dispatch(_ENEMY_WORK* self);
void em010_act1_1(_ENEMY_WORK* self);
void em010_act1_2(_ENEMY_WORK* self);
void em010_act1_3(_ENEMY_WORK* self);
void em010_act1_4(_ENEMY_WORK* self);
void em010_act1_5(_ENEMY_WORK* self);
void em010_act1_6(_ENEMY_WORK* self);
void em010_act1_7(_ENEMY_WORK* self);
void em010_act1_8(_ENEMY_WORK* self);
void em010_act1_dispatch(_ENEMY_WORK* self);
void em010_act2_0(_ENEMY_WORK* self, u8 a, u8 b);
void em010_act2_1(_ENEMY_WORK* self, u8 a);
void em010_act2_2(_ENEMY_WORK* self);
void em010_act2_5(_ENEMY_WORK* self);
void em010_act2_7(_ENEMY_WORK* self, u8 a);
void em010_act2_8(_ENEMY_WORK* self);
void em010_act2_9(_ENEMY_WORK* self, u8 a, u8 b);
void em010_act2_10(_ENEMY_WORK* self, u8 a, u8 b);
void em010_act2_16(_ENEMY_WORK* self, u8 a);
void em010_act2_19(_ENEMY_WORK* self, u8 a);
void em010_act2_40(_ENEMY_WORK* self, u8 a);
void em010_act2_dispatch(_ENEMY_WORK* self);
void em010_act7_0(_ENEMY_WORK* self);
void em010_act7_1(_ENEMY_WORK* self);
void em010_act7_2(_ENEMY_WORK* self, u8 a);
void em010_act7_4(_ENEMY_WORK* self);
void em010_act7_dispatch(_ENEMY_WORK* self);
void em010_act10_se_play(_ENEMY_WORK* self);
void em010_act11_se_play(_ENEMY_WORK* self);
void em010_act12_0(_ENEMY_WORK* self, u8 a);
void em010_act12_1(_ENEMY_WORK* self, u8 a);
void em010_act12_2(_ENEMY_WORK* self);
void em010_act12_4(_ENEMY_WORK* self, u8 a);
void em010_act12_dispatch(_ENEMY_WORK* self);
void em010_act13_0(_ENEMY_WORK* self);
void em010_act13_1(_ENEMY_WORK* self);
void em010_act13_2(_ENEMY_WORK* self);
void em010_act13_3(_ENEMY_WORK* self);
void em010_act13_4(_ENEMY_WORK* self);
void em010_act13_5(_ENEMY_WORK* self);
void em010_act13_6(_ENEMY_WORK* self);
void em010_act13_7(_ENEMY_WORK* self);
void em010_act13_8(_ENEMY_WORK* self);
void em010_act13_9(_ENEMY_WORK* self);
void em010_act13_10(_ENEMY_WORK* self);
void em010_act13_11(_ENEMY_WORK* self);
void em010_act13_12(_ENEMY_WORK* self);
void em010_act13_13(_ENEMY_WORK* self);
void em010_act13_14(_ENEMY_WORK* self);
void em010_act13_15(_ENEMY_WORK* self);
void em010_act13_16(_ENEMY_WORK* self);
void em010_act13_17(_ENEMY_WORK* self);
void em010_act13_18(_ENEMY_WORK* self);
void em010_act13_19(_ENEMY_WORK* self);
void em010_act13_20(_ENEMY_WORK* self);
void em010_act13_21(_ENEMY_WORK* self);
void em010_act13_22(_ENEMY_WORK* self);
void em010_act13_23(_ENEMY_WORK* self);
void em010_act13_24(_ENEMY_WORK* self);
void em010_act13_25(_ENEMY_WORK* self);
void em010_act13_dispatch(_ENEMY_WORK* self);
void em010_action_dispatch(_ENEMY_WORK* self);
void em010_effect_spawn(_ENEMY_WORK* self, u8 a, u8 b, s32 c, s32 d, f32 scale);
void em010_motion_events(_ENEMY_WORK* self);
void em010_area_start_set(_ENEMY_WORK* self, u8* action, u8* sub);
s32 em010_state_clear_ck(_ENEMY_WORK* self);
s32 em010_area_change_ck(_ENEMY_WORK* self);
void em010_mini_init(void);
void em010_mini_step(_ENEMY_MINI_WORK* mini);
void em010_mini_state_reset(_ENEMY_MINI_WORK* mini);

Em010ProgTbl em010_prog_tbl = {
    0,
    em010_init,
    em010_frame_update,
    em010_action_dispatch,
    0,
    em010_event_notify,
    em010_motion_events,
    {0, 0, 0, 0, 0},
    em010_area_start_set,
    {0, 0, 0, 0, 0, 0},
    em010_state_clear_ck,
    0,
    em010_area_change_ck,
    0,
    em010_mini_init,
    em010_mini_step,
    em010_mini_state_reset,
    0,
};

u32 em010_area_entry_target_set(_ENEMY_WORK* self) {
    u32 stale = 0;
    u8 map = stage_map_kind_get(self->field_0x1E0);

    switch (map) {
    case 1:
        switch (self->area_no) {
        case 1:
            em_move_target_set(self, 2, 5, 300.0f);
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 9, 300.0f);
                break;
            case 3:
                em_move_target_set(self, 0x15, 0xA, 300.0f);
                break;
            case 4:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 3, 9, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 9, 300.0f);
                break;
            case 9:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 9, 300.0f);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            case 9:
                em_move_target_set(self, 3, 9, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 4:
                em_move_target_set(self, 2, 0x11, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 0x11, 300.0f);
                break;
            }
            break;
        case 10:
            em_move_target_set(self, 4, 5, 300.0f);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 4:
                em_move_target_set(self, 3, 7, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            case 3:
                em_move_target_set(self, 0x13, 0x14, 300.0f);
                break;
            case 9:
                em_move_target_set(self, 3, 4, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 0x14, 300.0f);
                break;
            case 4:
                em_move_target_set(self, 3, 0x13, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 0x14, 300.0f);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 2:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 3:
                em_move_target_set(self, 0x24, 0x27, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            case 6:
                em_move_target_set(self, 0x25, 0x26, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 4:
                em_move_target_set(self, 2, 6, 300.0f);
                break;
            case 7:
                em_move_target_set(self, 0x18, 7, 300.0f);
                break;
            case 9:
                em_move_target_set(self, 3, 7, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 6, 300.0f);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 4:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            case 7:
                em_move_target_set(self, 3, 4, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 5, 300.0f);
                break;
            }
            break;
        case 7:
            switch (self->field_0x9F6) {
            case 5:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 6:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 10:
            em_move_target_set(self, 1, 3, 200.0f);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 3, 6, 300.0f);
                break;
            case 3:
                em_move_target_set(self, 4, 6, 300.0f);
                break;
            case 6:
                em_move_target_set(self, 5, 7, 300.0f);
                break;
            default:
                em_move_target_set(self, 3, 6, 300.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 3, 6, 300.0f);
                break;
            case 2:
                em_move_target_set(self, 4, 6, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 5, 7, 300.0f);
                break;
            default:
                em_move_target_set(self, 3, 6, 300.0f);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 6:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            case 5:
                em_move_target_set(self, 3, 5, 300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 300.0f);
                break;
            }
            break;
        case 7:
            em_move_target_set(self, 0, 2, 200.0f);
            return 1;
        case 9:
            em_move_target_set(self, 0, 2, 200.0f);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 8:
        switch (self->area_no) {
        case 1:
            em_move_target_set(self, 0, 1, 100.0f);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 9:
        if (self->area_no == 0) {
            em_move_target_set(self, 0, 1, 100.0f);
            return 1;
        }
        stale = 1;
        break;
    default:
        stale = 1;
        break;
    }
    if (stale == 1) {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        em_reaction_req(self, 5, 2, 0xA);
    }
    return 0;
}

void em010_init(_ENEMY_WORK* self, u8 kind) {
    VEC3 origin;
    u8 a;
    u8 b;
    u32 running;
    u8 map;

    setVec3(&origin, 0.0f, 0.0f, 0.0f);
    self->field_0x835 = 1;
    switch (kind) {
    case 0:
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->area_no == data->field_0x14) {
                    em_part_spot_nearest_get(self, &data->vec_0x24, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, 300.0f, 0, ran_suu(0));
                } else {
                    em_part_spot_nearest_get(self, &origin, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, 300.0f, 0, ran_suu(0));
                }
                self->field_0x1C4 = 0;
                self->field_0x1BC = 0;
                self->field_0x1C0 = (u16)ran_suu(1);
            }
        }
        break;
    case 2:
        em010_area_start_set(self, &a, &b);
        em_act_arm_unless_down(self, a, b);
        em_state_refresh(self);
        break;
    case 3:
        map = stage_map_kind_get(self->field_0x1E0);
        running = 0;
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0 && em012_area_move_armed_ck(data->work_0x18) == 1) {
                switch (map) {
                case 1:
                    if (self->area_no == 2 && self->field_0x9F6 == 1) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 2:
                    if ((self->area_no == 2 && self->field_0x9F6 == 1)
                        || (self->area_no == 4 && (self->field_0x9F6 == 1 || self->field_0x9F6 == 6))
                        || (self->area_no == 5 && self->field_0x9F6 == 9)
                        || (self->area_no == 6 && (self->field_0x9F6 == 4 || self->field_0x9F6 == 7))
                        || (self->area_no == 7 && self->field_0x9F6 == 6)) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 4:
                    running = 1;
                    break;
                }
            }
        }
        if (running == 0) {
            if (em010_area_entry_target_set(self) == 0) {
                em_move_mode_set(self, 0);
                em_act_arm_unless_down(self, 2, 0);
            } else {
                em_move_mode_set(self, 0);
                em_act_arm_unless_down(self, 0xC, 1);
            }
        } else if (map == 2 && self->area_no == 4) {
            em_move_mode_set(self, 0);
            em_move_target_set(self, 0x13, 0x14, 160.0f);
            em_act_arm_unless_down(self, 0xC, 1);
        } else if (map == 2 && self->area_no == 5) {
            em_move_mode_set(self, 0);
            em_move_target_set(self, 0x13, 0x15, 100.0f);
            em_act_arm_unless_down(self, 0xC, 1);
        } else {
            em010_area_start_set(self, &a, &b);
            em_act_arm_unless_down(self, a, b);
        }
        em_state_refresh(self);
        break;
    }
}

void em010_event_notify(void) {
}

void em010_frame_update(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (self->field_0x834 == 1 || em_hit_timer_ck(self) == 1) {
            em_attack_start(self);
            em_motion_kind_set(self, 1, 0);
        }
        {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->field_0x46C == data->field_0x17) {
                    copyVec3(&self->aim, &data->vec_0x24);
                }
                if (self->field_0x011 == 0) {
                    if (data->work_0x18 != 0 && data->work_0x18->area_no == self->area_no
                        && em_act_ck_c1(data->work_0x18, 1, 6) == 1
                        && em_frame_check(data->work_0x18, 0, 120.0f, 0.0f) == 1
                        && self->field_0x916 <= 0) {
                        em_hit_by_set(self, (s16)(ran_suu(0) & 0x1F), 0);
                    }
                }
                if (data->field_0x08 == 0xFF) {
                    self->field_0x46C = 0xFF;
                    em_user_data_reenter(self);
                    em_reaction_req(self, 6, 0xFF, 0);
                }
            }
        }
    }
}

void em010_act0_0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act0_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xB, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act0_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        return em010_act0_0(self);
    case 1:
        return em010_act0_1(self);
    case 2:
        return em010_act0_0(self);
    case 3:
        return em010_act0_0(self);
    case 4:
        return em010_act0_0(self);
    case 6:
        return em010_act0_0(self);
    }
}

void em010_act1_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act1_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act1_3(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act1_4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act1_5(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act1_6(_ENEMY_WORK* self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        em_motion_param_set(self, 0.0f, 0x1A);
        break;
    case 1:
        if (self->field_0x1D4 <= 0.0f) {
            em_attack_done_set(self);
        }
        break;
    }
}

void em010_act1_7(_ENEMY_WORK* self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            self->state += 1;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->field_0x314 = 45.0f;
            self->field_0x318 = 50.0f;
            self->field_0x320 = -4.0f;
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

void em010_act1_8(_ENEMY_WORK* self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 8, 0);
        em_motion_param_set(self, 0.0f, 0x1E);
        self->timer_0x020 = 0x20;
        break;
    case 1:
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= 0.0f) {
            em_attack_done_set(self);
        }
        break;
    }
}

void em010_act1_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        return em010_act1_1(self);
    case 2:
        return em010_act1_2(self);
    case 3:
        return em010_act1_3(self);
    case 4:
        return em010_act1_4(self);
    case 5:
        return em010_act1_5(self);
    case 6:
        return em010_act1_6(self);
    case 7:
        return em010_act1_7(self);
    case 8:
        return em010_act1_8(self);
    }
}

void em010_act2_0(_ENEMY_WORK *self, u8 a, u8 b) {
    if (a == 7) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
    }

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        switch (a) {
        case 0:
            em_approach_start(self, -500.0f, 0);
            break;
        case 1:
            em_approach_start(self, -600.0f, 0);
            break;
        case 2:
            em_approach_start(self, -300.0f, 0);
            break;
        case 3:
            em_approach_start(self, -1000.0f, 0);
            break;
        case 4:
            em_approach_start(self, -300.0f, 0);
            break;
        case 5:
            em_approach_start(self, -600.0f, 0);
            break;
        case 6:
            em_approach_start(self, -1500.0f, 0);
            break;
        case 7:
            em_approach_start(self, 0.0f, 0);
            break;
        case 8:
            em_approach_start(self, 0.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x200) == 1) {
            switch (b) {
            case 0:
                self->state += 1;
                em_mot_set(self, 0xF, 0, 0);
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

void em010_act2_1(_ENEMY_WORK *self, u8 a) {
    if (a == 2) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em010_turn_seq_0, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em010_turn_seq_0) == 1) {
            if (a == 0 || a == 2) {
                self->state += 1;
                em_mot_set(self, 0x13, 2, 4);
                return;
            }
            em_action_finish(self);
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act2_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act2_5(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act2_7(_ENEMY_WORK* self, u8 a) {

    if ((u32) (a - 1) <= 2) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        em_move_vec2_clr(self);
        return;
    case 1:
        if (em_frame_check(self, 1, 34.0f, 0.0f) == 1) {
            self->state += 1;
            em_fall_start_c1(self, em_fall_height_get(self));
            em_move_vec2_clr(self);
            switch ((s32) a) {
            case 0:
                self->offset_0x30C.vec_0x310.y = 40.0f;
                self->offset_0x30C.vec_0x310.z = 40.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            case 1:
                self->offset_0x30C.vec_0x310.y = 80.0f;
                self->offset_0x30C.vec_0x310.z = 50.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            case 2:
                self->offset_0x30C.vec_0x310.y = 45.0f;
                self->offset_0x30C.vec_0x310.z = 50.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            case 3:
                self->offset_0x30C.vec_0x310.y = 70.0f;
                self->offset_0x30C.vec_0x310.z = 60.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            }
        } else {
            break;
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
        if (em_ground_ck_c1(self, em_fall_height_get(self)) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 5, 0, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act2_8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act2_9(_ENEMY_WORK* self, u8 a, u8 b) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = 40.0f;
        self->vec_0x31C.z = -1.3f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        em_turn_seq_start(self, (void*)em010_turn_seq_1, 0, 0, 0);
        return;
    case 1:
        em_turn_seq_step(self, (void*)em010_turn_seq_1);
        CancelFade(self);
        if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
            if ((s32) a == 0) {
                em_state_set(self, 2, (u32) b);
                return;
            }
            em_action_finish(self);
        } else {
            return;
        }
        break;
    }
}

void em010_act2_10(_ENEMY_WORK* self, u8 a, u8 b) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state += 1;
        vec3_scale_in_place(&self->offset_0x30C.vec_0x310, 1.4f);
        /* fallthrough */
    case 1:
        if (em_frame_check(self, 1, 35.0f, 0.0f) == 1) {
            if (em_frame_check(self, 1, 58.0f, 0.0f) == 0) {
                em_turn_to_target(self, 0x71C);
            }
            if (self->state_0x006 == 0) {
                CancelFade(self);
                if (vec3_dot(&self->offset_0x30C.vec_0x310.x, &self->vec_0x31C.x) > 0.0f
                    || em_frame_check(self, 1, 58.0f, 0.0f) == 1) {
                    self->state_0x006 += 1;
                    em_move_vec2_clr(self);
                }
            }
            if (em_mot_end_ck(self) == 1) {
                if ((s32) a == 0) {
                    self->state += 1;
                    em_mot_set(self, 0x16, 4, 0);
                    return;
                }
                em_action_finish(self);
                return;
            }
        } else {
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 2, (u32) b);
        }
        break;
    }
}

void em010_act2_16(_ENEMY_WORK* self, u8 a) {

    if (a == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em010_turn_seq_2, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em010_turn_seq_2) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act2_19(_ENEMY_WORK* self, u8 a) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xCB, 4, 0);
        em_mot_speed_set(self, 1.7f);
        if ((s32) a == 0) {
            em_approach_start(self, -500.0f, 0);
        } else {
            em_approach_start(self, -100.0f, 0);
        }
        self->timer_0x020 = (s32) (s16) ((self->bits_0x1EC & 0x3F) + 0x3C);
        return;
    case 1:
        if ((em_frame_check(self, 3, 70.0f, 146.0f) == 1) || (em_frame_check(self, 3, 254.0f, 350.0f) == 1)) {
            em_turn_to_target(self, 0x200);
        } else if (em_frame_check(self, 1, 44.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x40);
        }
        if ((s32) a == 0) {
            self->timer_0x020 -= 1;
            if ((em_approach_step(self, 0, 0) == 1) || ((s32) self->timer_0x020 < 0)) {
                em_mot_speed_set(self, 1.0f);
                em_action_finish(self);
                return;
            }
        } else {
            if (em_mot_end_ck(self) == 1) {
                em_mot_speed_set(self, 1.0f);
                em_action_finish(self);
            }
            return;
        }
        break;
    }
}

void em010_act2_40(_ENEMY_WORK* self, u8 a) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        em_mot_speed_set(self, 0.8f);
        switch ((s32) a) {
        case 0:
            em_approach_start(self, -300.0f, 0);
            break;
        case 1:
            em_approach_start(self, -600.0f, 0);
            break;
        }
        self->timer_0x020 = 0x3C;
        return;
    case 1:
        self->timer_0x020 -= 1;
        if ((em_approach_step(self, 0, 0x200) == 1) || ((s32) self->timer_0x020 <= 0)) {
            em_mot_speed_set(self, 1.0f);
            em_action_finish(self);
        }
        return;
    }
}

void em010_act2_dispatch(_ENEMY_WORK* self) {
    u8 sub;

    sub = self->state_sub;
    switch (sub) {
    case 0:
        em010_act2_0(self, 0, 0);
        return;
    case 1:
        em010_act2_1(self, 0);
        return;
    case 2:
        em010_act2_2(self);
        return;
    case 3:
        em010_act2_0(self, 1, 1);
        return;
    case 4:
        em010_act2_1(self, 1);
        return;
    case 5:
        em010_act2_5(self);
        return;
    case 6:
        em010_act2_0(self, 2, 1);
        return;
    case 7:
        em010_act2_7(self, 0);
        return;
    case 8:
        em010_act2_8(self);
        return;
    case 9:
        em010_act2_9(self, 0, 0xA);
        return;
    case 10:
        em010_act2_10(self, 0, 0xD);
        return;
    case 11:
        em010_act2_9(self, 0, 0xC);
        return;
    case 12:
        em010_act2_10(self, 1, 0);
        return;
    case 13:
        em010_act2_0(self, 2, 1);
        return;
    case 14:
        em010_act2_0(self, 8, 1);
        return;
    case 16:
        em010_act2_16(self, 0);
        return;
    case 17:
        em010_act2_0(self, 3, 1);
        return;
    case 18:
        em010_act2_0(self, 6, 1);
        return;
    case 19:
        em010_act2_19(self, 0);
        return;
    case 20:
        em010_act2_0(self, 4, 1);
        return;
    case 21:
        em010_act2_0(self, 5, 1);
        return;
    case 22:
        em010_act2_9(self, 0, 0x17);
        return;
    case 23:
        em010_act2_10(self, 0, 0x15);
        return;
    case 24:
        em010_act2_9(self, 0, 0x19);
        return;
    case 25:
        em010_act2_10(self, 0, 0x14);
        return;
    case 26:
        em010_act2_9(self, 0, 0x1B);
        return;
    case 27:
        em010_act2_10(self, 0, 3);
        return;
    case 30:
        em010_act2_9(self, 1, 0);
        return;
    case 31:
        em010_act2_9(self, 0, 0x20);
        return;
    case 32:
        em010_act2_10(self, 0, 0x11);
        return;
    case 34:
        em010_act2_7(self, 3);
        return;
    case 35:
        em010_act2_0(self, 7, 1);
        return;
    case 36:
        em010_act2_1(self, 2);
        return;
    case 37:
        em010_act2_7(self, 1);
        return;
    case 38:
        em010_act2_16(self, 1);
        return;
    case 39:
        em010_act2_7(self, 2);
        return;
    case 40:
        em010_act2_40(self, 0);
        return;
    case 41:
        em010_act2_40(self, 1);
        return;
    case 42:
        em010_act2_19(self, 1);
        return;
        return;
    default:
        return;
    }
}

void em010_act7_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act7_1(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        em_hit_window_set(self, 0, 2, 2);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.y = 25.0f;
        self->offset_0x30C.vec_0x310.z = 30.0f;
        self->vec_0x31C.y = -4.2f;
        self->vec_0x31C.z = -1.0f;
        return;
    case 1:
        if (em_frame_check(self, 1, 62.0f, 0.0f) == 1) {
            self->state += 1;
            em_fall_start_c1(self, em_fall_height_get(self));
        }
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
            return;
        }
        break;
    case 2:
        if (self->offset_0x30C.vec_0x310.y < -100.0f) {
            self->offset_0x30C.vec_0x310.y = -100.0f;
            self->vec_0x31C.y = 0.0f;
        }
        if (self->offset_0x30C.vec_0x310.z < 50.0f) {
            self->offset_0x30C.vec_0x310.z = 50.0f;
            self->vec_0x31C.z = 0.0f;
        }
        if (em_ground_ck_c1(self, 100.0f) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x1D, 0, 0);
            return;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        return;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act7_2(_ENEMY_WORK* self, u8 a) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_hit_window_set_default(self, 0, 3);
        return;
    case 1:
        if ((s32) a == 0) {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                return;
            }
        } else {
            if (em_frame_check(self, 1, 72.0f, 0.0f) == 1) {
                self->state += 1;
                em_mot_set(self, 0x1A, 6, 0);
                em_hit_window_set_default(self, 0, 4);
                return;
            }
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act7_4(_ENEMY_WORK* self) {
    EmSpawnRec rec;
    VEC3 vec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCC, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            rec.id = 0x14;
            copyVec3(&rec.pos, setVec3(&vec, 0.0f, 0.0f, -20.0f));
            rec.field_0x10 = 0;
            rec.field_0x12 = 0;
            rec.field_0x14 = 0;
            eft007_part_spawn(self, 0x39, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x1B);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act7_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em010_act7_0(self);
        return;
    case 1:
        em010_act7_1(self);
        return;
    case 2:
        em010_act7_2(self, 0);
        return;
    case 3:
        em010_act7_2(self, 1);
        return;
    case 4:
        em010_act7_4(self);
        return;
    }
}

EmSePosRec em010_se_pos_0 = {0.0f, 31.0f, 0, 16384};

EmSePosRec em010_se_pos_1 = {0.0f, 31.0f, -1, -16384};

EmSePosRec em010_se_pos_2 = {0.0f, 31.0f, 0, -32768};

EmSePosRec em010_se_pos_3 = {0.0f, 41.0f, 0, 16384};

EmSePosRec em010_se_pos_4 = {0.0f, 41.0f, -1, -16384};

EmSePos em010_se_pos_5 = {0.0f, 41.0f, -1, -32768, 0};

EmSeRecord em010_se_rec_00 = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_00[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_00},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_1F_a = {0x67, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_0.pos};

EmSeRecord em010_se_rec_1F_b = {0x6A, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_1F_c = {0x6C, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_1F_d = {0x78, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_1F[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_0},
    {0x00, {0, 0, 0}, &em010_se_rec_1F_a},
    {0x00, {0, 0, 0}, &em010_se_rec_1F_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_1F_c},
    {0x00, {0, 0, 0}, &em010_se_rec_1F_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_20_a = {0x68, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_1.pos};

EmSeRecord em010_se_rec_20_b = {0x69, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_20_c = {0x6B, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_20_d = {0x79, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_20[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_1},
    {0x00, {0, 0, 0}, &em010_se_rec_20_a},
    {0x00, {0, 0, 0}, &em010_se_rec_20_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_20_c},
    {0x00, {0, 0, 0}, &em010_se_rec_20_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_21_a = {0x65, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_2.pos};

EmSeRecord em010_se_rec_21_b = {0x69, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_21_c = {0x6B, 0, 0, 2, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_21_d = {0x79, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_21[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em010_se_rec_21_a},
    {0x00, {0, 0, 0}, &em010_se_rec_21_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_21_c},
    {0x00, {0, 0, 0}, &em010_se_rec_21_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_22_a = {0x66, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_3.pos};

EmSeRecord em010_se_rec_22_b = {0x6A, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_22_c = {0x6C, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_22_d = {0x78, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_22[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_2},
    {0x00, {0, 0, 0}, &em010_se_rec_22_a},
    {0x00, {0, 0, 0}, &em010_se_rec_22_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_22_c},
    {0x00, {0, 0, 0}, &em010_se_rec_22_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_06_a = {0x6F, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_4.pos};

EmSeRecord em010_se_rec_06_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_06_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_06_d = {0x7A, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_06[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_3},
    {0x00, {0, 0, 0}, &em010_se_rec_06_a},
    {0x00, {0, 0, 0}, &em010_se_rec_06_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_06_c},
    {0x00, {0, 0, 0}, &em010_se_rec_06_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_07_a = {0x70, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_5.pos};

EmSeRecord em010_se_rec_07_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_07_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_07_d = {0x7A, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_07[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_4},
    {0x00, {0, 0, 0}, &em010_se_rec_07_a},
    {0x00, {0, 0, 0}, &em010_se_rec_07_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_07_c},
    {0x00, {0, 0, 0}, &em010_se_rec_07_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_08_a = {0x6D, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_6.pos};

EmSeRecord em010_se_rec_08_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_08_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_08_d = {0x7A, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_08[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em010_se_rec_08_a},
    {0x00, {0, 0, 0}, &em010_se_rec_08_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_08_c},
    {0x00, {0, 0, 0}, &em010_se_rec_08_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_09_a = {0x6E, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_7.pos};

EmSeRecord em010_se_rec_09_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_09_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_09_d = {0x7A, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_09[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_5},
    {0x00, {0, 0, 0}, &em010_se_rec_09_a},
    {0x00, {0, 0, 0}, &em010_se_rec_09_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_09_c},
    {0x00, {0, 0, 0}, &em010_se_rec_09_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_7A = {0x74, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_7B = {0x75, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em010_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_9F = {0x79, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_7C = {0x76, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em010_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_7C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_7D = {0x74, 0, 8, 15, 0, {1, 8, 0, 0}, 0};

EmSeEntry em010_se_seq_7D[4] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x41)},
    {0x02, {0, 0, 0}, &em010_se_rec_7D},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_8E = {0x64, 0, 4, 80, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_8E},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_A8 = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em010_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_D6_a = {0x64, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_D6_b = {0x1E, 0, 20, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em010_se_seq_D6[4] = {
    {0x00, {0, 0, 0}, &em010_se_rec_D6_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em010_se_rec_D6_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_01_a = {0x6F, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_4.pos};

EmSeRecord em010_se_rec_alt_01_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_alt_01_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_alt_01_d = {0x7B, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_01[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_3},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_01_a},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_01_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_01_c},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_01_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_02_a = {0x70, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_5.pos};

EmSeRecord em010_se_rec_alt_02_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_alt_02_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_alt_02_d = {0x7B, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_02[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_4},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_02_a},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_02_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_02_c},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_02_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_03_a = {0x6D, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_6.pos};

EmSeRecord em010_se_rec_alt_03_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_alt_03_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_alt_03_d = {0x7B, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_03[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_03_a},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_03_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_03_c},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_03_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_04_a = {0x6E, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_7.pos};

EmSeRecord em010_se_rec_alt_04_b = {0x71, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em010_se_rec_alt_04_c = {0x72, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em010_se_rec_alt_04_d = {0x7B, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_04[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em010_se_pos_5},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_04_a},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_04_b},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_04_c},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_04_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_38 = {0x3, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_38[4] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1A)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_38},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_39_a = {0x4, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em010_se_rec_alt_39_b = {0x0, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em010_vec_pair_8.pos};

EmSeEntry em010_se_seq_alt_39[7] = {
    {0x0C, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x22)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_39_a},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x10)},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em010_se_rec_alt_39_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em010_se_rec_alt_3A = {0xE, 0, 8, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em010_se_seq_alt_3A[5] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em010_se_rec_alt_3A},
    {0xFF, {0, 0, 0}, 0},
};

void em010_act10_se_play(_ENEMY_WORK* self) {
    u8 sub;

    sub = self->state_sub;
    switch ((s32) sub) {
    case 0x0:
        em_se_tbl_play(self, em010_se_seq_00, 0, 0);
        return;
    case 0x1F:
        em_se_tbl_play(self, em010_se_seq_1F, 1, 0x1F);
        return;
    case 0x20:
        em_se_tbl_play(self, em010_se_seq_20, 1, 0x20);
        return;
    case 0x21:
        em_se_tbl_play(self, em010_se_seq_21, 1, 0x21);
        return;
    case 0x22:
        em_se_tbl_play(self, em010_se_seq_22, 1, 0x22);
        return;
    case 0x6:
        em_se_tbl_play(self, em010_se_seq_06, 1, 6);
        return;
    case 0x7:
        em_se_tbl_play(self, em010_se_seq_07, 1, 7);
        return;
    case 0x8:
        em_se_tbl_play(self, em010_se_seq_08, 1, 8);
        return;
    case 0x9:
        em_se_tbl_play(self, em010_se_seq_09, 1, 9);
        return;
    case 0x24:
        em_se_tbl_play(self, em010_se_seq_06, 1, 0x24);
        return;
    case 0x25:
        em_se_tbl_play(self, em010_se_seq_07, 1, 0x25);
        return;
    case 0x26:
        em_se_tbl_play(self, em010_se_seq_08, 1, 0x26);
        return;
    case 0x27:
        em_se_tbl_play(self, em010_se_seq_09, 1, 0x27);
        return;
    case 0x7A:
        em_se_tbl_play(self, em010_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em010_se_seq_7B, 0, 0x7B);
        return;
    case 0x9F:
        em_se_tbl_play(self, em010_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em010_se_seq_9F, 0, 0xA0);
        return;
    case 0x7C:
        em_se_tbl_play(self, em010_se_seq_7C, 0, 0x7C);
        return;
    case 0xA8:
        em_se_tbl_play(self, em010_se_seq_A8, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play(self, em010_se_seq_21, 1, 0xA9);
        return;
    case 0x7D:
        em_se_tbl_play(self, em010_se_seq_7D, 0, 0x7D);
        return;
    case 0x8E:
        em_se_tbl_play(self, em010_se_seq_8E, 0, 0x8E);
        return;
    case 0x85:
        em_se_tbl_play(self, em010_se_seq_06, 1, 0x85);
        return;
    case 0x86:
        em_se_tbl_play(self, em010_se_seq_07, 1, 0x86);
        return;
    case 0x87:
        em_se_tbl_play(self, em010_se_seq_08, 1, 0x87);
        return;
    case 0x88:
        em_se_tbl_play(self, em010_se_seq_09, 1, 0x88);
        return;
    case 0xD6:
        em_se_tbl_play(self, em010_se_seq_D6, 0, 0xD6);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

void em010_act11_se_play(_ENEMY_WORK* self) {
    u8 sub;

    sub = self->state_sub;
    switch ((s32) sub) {
    case 0x1:
        em_se_tbl_play_alt(self, em010_se_seq_alt_01, 1, 1);
        return;
    case 0x2:
        em_se_tbl_play_alt(self, em010_se_seq_alt_02, 1, 2);
        return;
    case 0x3:
        em_se_tbl_play_alt(self, em010_se_seq_alt_03, 1, 3);
        return;
    case 0x4:
        em_se_tbl_play_alt(self, em010_se_seq_alt_04, 1, 4);
        return;
    case 0x6:
        em_se_tbl_play_alt(self, em010_se_seq_alt_01, 1, 6);
        return;
    case 0x7:
        em_se_tbl_play_alt(self, em010_se_seq_alt_02, 1, 7);
        return;
    case 0x8:
        em_se_tbl_play_alt(self, em010_se_seq_alt_03, 1, 8);
        return;
    case 0x9:
        em_se_tbl_play_alt(self, em010_se_seq_alt_04, 1, 9);
        return;
    case 0x38:
        em_se_tbl_play_alt(self, em010_se_seq_alt_38, 0, 0x38);
        return;
    case 0x39:
        em_se_tbl_play_alt(self, em010_se_seq_alt_39, 0, 0x39);
        return;
    case 0x3A:
        em_se_tbl_play_alt(self, em010_se_seq_alt_3A, 0, 0x3A);
        return;
    default:
        em_se_tbl_play_alt(self, em010_se_seq_alt_03, 1, 3);
        return;
    }
}

void em010_act12_0(_ENEMY_WORK* self, u8 a) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        em_move_vec2_clr(self);
        return;
    case 1:
        if (em_frame_check(self, 1, 34.0f, 0.0f) == 1) {
            self->state += 1;
            em_fall_start_c1(self, em_fall_height_get(self));
            em_move_vec2_clr(self);
            switch ((s32) a) {
            case 0:
                self->offset_0x30C.vec_0x310.y = 40.0f;
                self->offset_0x30C.vec_0x310.z = 40.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            case 1:
                self->offset_0x30C.vec_0x310.y = 55.0f;
                self->offset_0x30C.vec_0x310.z = 85.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            case 2:
                self->offset_0x30C.vec_0x310.y = 40.0f;
                self->offset_0x30C.vec_0x310.z = 50.0f;
                self->vec_0x31C.y = -4.0f;
                self->vec_0x31C.z = -1.0f;
                return;
            }
        } else {
            break;
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
        if (em_ground_ck_c1(self, em_fall_height_get(self)) != 0) {
            self->state += 1;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 5, 0, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act12_1(_ENEMY_WORK *self, u8 a) {
    if (a == 1) {
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        em_approach_start(self, 0.0f, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xCA, 6, 0);
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em010_act12_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act12_4(_ENEMY_WORK* self, u8 a) {
    s32 timer;
    u32 angle;
    u8 area;
    u8 map;

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        self->timer_0x020 = self->bits_0x1EC & 0x7F;
        return;
    case 1:
        timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0) {
            self->state += 1;
            self->state_0x006 = 0;
            em_move_mode_set(self, 0);
            em_mot_set_ck(self, 0xE, 8, 0);
            em_approach_start(self, 0.0f, 0);
            return;
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= 22500.0f || self->state_0x006 >= 3 || angle > 0x4000) {
                self->state += 1;
                map = stage_map_kind_get(self->field_0x1E0);
                switch ((s32) map) {
                case 2:
                    if ((s32) self->area_no == 5) {
                        em_move_target_set(self, 0xFF, 0xB, 400.0f);
                    }
                    break;
                case 4:
                    area = self->area_no;
                    switch ((s32) area) {
                    case 5:
                        em_move_target_set(self, 0xFF, 0xC, 200.0f);
                        break;
                    case 6:
                        em_move_target_set(self, 0xFF, 0xA, 200.0f);
                        break;
                    }
                    break;
                }
                em_turn_seq_start(self, (void*)em010_turn_seq_2, 0, 0, 0);
                return;
            }
            self->state_0x006++;
            em_approach_start(self, 0.0f, 0);
            return;
        }
        break;
    case 3:
        if (em_turn_seq_step(self, (void*)em010_turn_seq_2) == 1) {
            switch ((s32) a) {
            case 0:
                em_state_set(self, 0xC, 0);
                return;
            case 1:
                em_state_set(self, 0xC, 3);
                return;
            case 2:
                em_state_set(self, 0xC, 6);
                break;
            }
        }
        break;
    }
}

void em010_act12_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em010_act12_0(self, 0);
        return;
    case 1:
        em010_act12_1(self, 0);
        return;
    case 2:
        em010_act12_2(self);
        return;
    case 3:
        em010_act12_0(self, 1);
        return;
    case 4:
        em010_act12_4(self, 1);
        return;
    case 5:
        em010_act12_4(self, 0);
        return;
    case 6:
        em010_act12_0(self, 2);
        return;
    case 7:
        em010_act12_4(self, 2);
        return;
    case 8:
        em010_act12_1(self, 1);
        return;
    }
}

void em010_act13_0(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        em_demo_pos_set(self, -600.0f, 0.0f, 1370.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x5C72;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_demo_time_ck(0xD2) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD, 2, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x17C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xB, 2, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x458) == 1) {
            self->state += 1;
            em_demo_pos_set(self, -1480.0f, 0.0f, 1980.0f);
        }
        break;
    }
}

void em010_act13_1(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        em_demo_pos_set(self, -600.0f, 0.0f, 1370.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x5C72;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_2(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        em_demo_pos_set(self, -820.0f, 0.0f, 1000.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x471C;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_demo_time_ck(0x40) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD, 2, 0);
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xD, 2, 0);
        }
        if (em_demo_time_ck(0x458) == 1) {
            self->state += 1;
            em_demo_pos_set(self, -1700.0f, 0.0f, 1600.0f);
        }
        break;
    }
}

void em010_act13_3(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 0, 0);
        em_demo_pos_set(self, -600.0f, 0.0f, 670.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x38E4;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_4(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x2AC) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = -0x1D55;
            em_demo_pos_set(self, 2234.189f, 0.0f, -4866.12f);
            return;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0x3DC) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 2, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x436) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0x86);
            self->field_0x1C0 = -0x3333;
            em_demo_pos_set(self, 186.1639f, 0.0f, -2540.55f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x566) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0x86);
            self->field_0x1C0 = -0xC44;
            em_demo_pos_set(self, -673.452f, 0.0f, -81.3234f);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x644) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            self->field_0x1C0 = 0x782E;
            em_demo_pos_set(self, -1410.26f, 269.3286f, 3905.86f);
            self->field_0x1CC = 2.0f;
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x668) == 1) {
            self->state += 1;
            em_mot_set(self, 0xCA, 0x10, 0);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x7CE) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC, 0, 0);
            self->field_0x1C0 = 0x81DE;
            em_demo_pos_set(self, -189.768f, 0.0f, 491.7192f);
            self->field_0x1CC = 1.0f;
            return;
        }
        break;
    case 8:
        if ((em_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_frame_check(self, 0, 92.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        if (em_demo_time_ck(0x84C) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0x18, 0);
            return;
        }
        break;
    case 9:
        if (em_demo_time_ck(0x86A) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 2, 2);
        }
        break;
    }
}

void em010_act13_5(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -185.66f, 0.0f, 501.3072f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x81DE;
        self->field_0x1C4 = 0;
        self->field_0x1CC = 1.0f;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

f32 em010_demo_keys[34][4] = {
    {0.0f, -3109.69f, 734.21f, 2577.1f},
    {182.0f, -3108.99f, 734.38f, 2576.0f},
    {186.0f, -3065.43f, 745.2f, 2506.76f},
    {188.0f, -3049.6f, 749.14f, 2481.59f},
    {192.0f, -3021.94f, 756.01f, 2437.62f},
    {196.0f, -2991.25f, 763.64f, 2388.85f},
    {200.0f, -2958.44f, 771.8f, 2336.69f},
    {202.0f, -2941.75f, 775.94f, 2310.17f},
    {206.0f, -2909.46f, 783.97f, 2258.84f},
    {210.0f, -2884.56f, 790.16f, 2219.25f},
    {214.0f, -2861.97f, 795.77f, 2183.35f},
    {218.0f, -2841.83f, 800.77f, 2151.34f},
    {222.0f, -2826.18f, 804.66f, 2126.46f},
    {224.0f, -2820.4f, 806.1f, 2117.27f},
    {228.0f, -2813.05f, 807.93f, 2105.59f},
    {232.0f, -2807.79f, 809.23f, 2097.24f},
    {236.0f, -2803.41f, 810.32f, 2090.27f},
    {238.0f, -2801.47f, 810.8f, 2087.19f},
    {242.0f, -2798.11f, 811.64f, 2081.85f},
    {244.0f, -2796.7f, 811.99f, 2079.6f},
    {358.0f, -2796.7f, 811.99f, 2079.6f},
    {360.0f, -2796.38f, 812.07f, 2079.1f},
    {368.0f, -2792.16f, 813.12f, 2072.38f},
    {372.0f, -2791.03f, 813.4f, 2070.6f},
    {376.0f, -2788.05f, 814.14f, 2065.86f},
    {378.0f, -2783.94f, 815.16f, 2059.33f},
    {380.0f, -2762.11f, 820.59f, 2024.62f},
    {382.0f, -2730.02f, 828.56f, 1973.62f},
    {384.0f, -2673.24f, 861.94f, 1888.08f},
    {386.0f, -2603.58f, 908.54f, 1784.36f},
    {388.0f, -2529.83f, 959.29f, 1674.82f},
    {390.0f, -2453.67f, 1012.43f, 1561.85f},
    {392.0f, -2375.7f, 1067.29f, 1446.28f},
    {-1.0f, -2375.7f, 1067.29f, 1446.28f},
};

void em010_act13_6(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x2AC) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = -0x1D55;
            em_demo_pos_set(self, 3009.504f, 0.0f, -4454.12f);
            return;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0x3AE) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 0, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x3FA) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 1, 0x16, 0, 1);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x436) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0x28);
            self->field_0x1C0 = 0x19DE;
            em_demo_pos_set(self, 716.8636f, 0.0f, -2320.97f);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x566) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0x28);
            self->field_0x1C0 = 0x17BC;
            em_demo_pos_set(self, 42.3666f, 0.0f, -296.273f);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x6FA) == 1) {
            self->state += 1;
            em_fall_start_c1(self, em_fall_height_get(self));
            em_mot_set(self, 0xF, 0, 0);
            em_demo_rot_set(self, -4.42f, 147.31f, 5.81f);
            em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x322), em010_demo_keys, NULL);
            em_height_set(self, -35.0f);
            self->field_0x1BC = -0x325;
            self->field_0x1C0 = 0x68C1;
            self->field_0x1C4 = -0x422;
            self->field_0x1CC = 2.0f;
            return;
        }
        break;
    case 7:
        em_demo_rot_set(self, -4.42f, 147.31f, 5.81f);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x322), em010_demo_keys, NULL);
        em_height_set(self, -35.0f);
        self->field_0x1BC = -0x325;
        self->field_0x1C0 = 0x68C1;
        self->field_0x1C4 = -0x422;
        if (em_demo_time_ck(0x738) == 1) {
            self->state += 1;
            em_mot_set(self, 0xCA, 0x1A, 0);
            return;
        }
        break;
    case 8:
        em_demo_rot_set(self, -4.42f, 147.31f, 5.81f);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x322), em010_demo_keys, NULL);
        em_height_set(self, -35.0f);
        self->field_0x1BC = -0x325;
        self->field_0x1C0 = 0x68C1;
        self->field_0x1C4 = -0x422;
        if (em_demo_time_ck(0x7AA) == 1) {
            self->state += 1;
            em_mot_set(self, 3, 0xE, 8);
            return;
        }
        break;
    case 9:
        em_demo_rot_set(self, -4.42f, 147.31f, 5.81f);
        em_demo_key3_apply(self, (s16) (em_demo_frame_get() - 0x322), em010_demo_keys, NULL);
        em_height_set(self, -35.0f);
        self->field_0x1BC = -0x325;
        self->field_0x1C0 = 0x68C1;
        self->field_0x1C4 = -0x422;
        if (em_demo_time_ck(0x7CE) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 8, 0, 0x28);
            em_height_set(self, 0.0f);
            self->field_0x1C0 = -0x26AB;
            em_demo_pos_set(self, 467.3252f, 0.0f, -846.302f);
            self->field_0x1CC = 1.0f;
        }
        break;
    }
}

void em010_act13_7(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 491.3173f, 0.0f, -861.303f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = -0x26AB;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        self->field_0x1CC = 1.0f;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_8(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x7D0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_enable(self);
            self->field_0x1C0 = 0xFFFF6111;
            em_demo_pos_set(self, -2172.53f, 0.0f, 1947.828f);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x844) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 4, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x86C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC8, 6, 0x10);
            return;
        }
        break;
    case 4:
        if ((em_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_frame_check(self, 0, 118.0f, 0.0f) == 1) || (em_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_frame_check(self, 0, 224.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        break;
    }
}

void em010_act13_9(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -752.654f, 0.0f, 451.5945f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xFFFF6111;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_10(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x7CE) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0x18);
            em_demo_enable(self);
            self->field_0x1C0 = 0x2BBC;
            em_demo_pos_set(self, -1050.94f, 0.0f, -734.158f);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x7DE) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 2, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x808) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC8, 0xC, 0xE);
            return;
        }
        break;
    case 4:
        if ((em_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_frame_check(self, 0, 118.0f, 0.0f) == 1) || (em_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_frame_check(self, 0, 224.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        break;
    }
}

void em010_act13_11(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -612.798f, 0.0f, -496.267f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x2BBC;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_12(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0xF2) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 3077.379f, -97.1295f, 2099.3252f);
            em_demo_rot_set(self, 0.0f, -113.0f, 0.0f);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x1D0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 0, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x21C) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0);
            em_demo_pos_set(self, 104.5894f, -22.9088f, 349.023f);
            em_demo_rot_set(self, 0.0f, -102.0f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x454) == 1) {
            self->state += 1;
            em_mot_set(self, 0xD, 0, 0);
            em_demo_pos_set(self, 370.9045f, -22.5139f, 794.5763f);
            em_demo_rot_set(self, 0.0f, -102.6396f, 0.0f);
        }
        /* fallthrough */
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xD, 0, 0);
        }
        if (em_demo_time_ck(0x5DC) == 1) {
            self->state += 1;
            em_demo_pos_set(self, 211.9051f, -347.25f, 1176.1667f);
            em_demo_rot_set(self, 0.0f, -347.25f, 0.0f);
        }
        /* fallthrough */
    case 6:
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xD, 0, 0);
        }
        break;
    }
}

void em010_act13_13(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 0, 0);
        em_demo_pos_set(self, 211.9051f, -347.25f, 1176.1667f);
        em_demo_rot_set(self, 0.0f, -347.25f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_14(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0xB4) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0);
            em_demo_pos_set(self, 535.0f, 15.0f, 97.0f);
            em_demo_rot_set(self, 0.0f, -24.0f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x158) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0xA4);
            em_demo_pos_set(self, 566.0f, 15.0f, 65.0f);
            em_demo_rot_set(self, 0.0f, -60.0f, 0.0f);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x218) == 1) {
            self->state += 1;
            em_mot_set(self, 0x12, 0, 0);
            em_demo_pos_set(self, 569.0f, 15.0f, 64.0f);
            em_demo_rot_set(self, 0.0f, -60.0f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 186.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 9, 0, 0.6f);
        }
        if (em_demo_time_ck(0x2E0) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x3D6) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0xB4);
            em_demo_pos_set(self, 418.0f, 15.0f, -448.0f);
            em_demo_rot_set(self, 0.0f, -226.0f, 0.0f);
            em_demo_enable(self);
        }
        break;
    }
}

void em010_act13_15(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 418.0f, 15.0f, -448.0f);
        em_demo_rot_set(self, 0.0f, -226.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_16(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x2E0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, -5445.0f, 0.0f, 331.0f);
            em_demo_rot_set(self, 0.0f, 91.0f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x3D6) == 1) {
            self->state += 1;
            em_mot_set(self, 8, 0, 0x64);
            em_demo_pos_set(self, 968.0f, 6.0f, -2005.0f);
            em_demo_rot_set(self, 0.0f, -148.0f, 0.0f);
        }
        break;
    }
}

void em010_act13_17(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 0, 0x64);
        em_demo_pos_set(self, 200.0f, 6.0f, -2005.0f);
        em_demo_rot_set(self, 0.0f, -148.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_18(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x2E0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, -5978.0f, 0.0f, 253.0f);
            em_demo_rot_set(self, 0.0f, 91.0f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x302) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 2, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x3D6) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x460) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, -955.0f, 0.0f, -1250.0f);
            em_demo_rot_set(self, 0.0f, 90.0f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x4B0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xF, 2, 0);
        }
        break;
    }
}

void em010_act13_19(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 0, 0);
        em_demo_pos_set(self, 853.0f, 20.0f, -1113.0f);
        em_demo_rot_set(self, 0.0f, -90.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_20(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x3D6) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, 405.0f, 22.0f, -2169.0f);
            em_demo_rot_set(self, 0.0f, 148.0f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x43C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC8, 2, 0);
            return;
        }
        break;
    case 3:
        if ((em_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_frame_check(self, 0, 118.0f, 0.0f) == 1) || (em_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_frame_check(self, 0, 224.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        break;
    }
}

void em010_act13_21(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0xB4);
        em_demo_pos_set(self, 405.0f, 22.0f, -2169.0f);
        em_demo_rot_set(self, 0.0f, 148.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_22(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 0, 0);
        em_demo_pos_set(self, 1027.6f, 4.0f, 3362.7f);
        em_demo_rot_set(self, 0.0f, 174.0f, 0.0f);
        return;
    case 1:
        if (em_demo_time_ck(0x96) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0xF2) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 1856.4f, 4.0f, 1362.7f);
            em_demo_rot_set(self, 0.0f, 217.2f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x13C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 3962.7f, 4.0f, 658.9f);
            em_demo_rot_set(self, 0.0f, 261.0f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x1F6) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x47C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC, 0, 0);
            em_demo_pos_set(self, 172.0f, 4.0f, -388.7f);
            em_demo_rot_set(self, 0.0f, 323.2f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 6:
        if ((em_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_frame_check(self, 0, 92.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        if (em_demo_time_ck(0x74C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC8, 0, 0);
            return;
        }
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xC, 0, 0);
            return;
        }
        break;
    case 7:
        if ((em_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_frame_check(self, 0, 118.0f, 0.0f) == 1) || (em_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_frame_check(self, 0, 224.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.8f);
        }
        break;
    }
}

void em010_act13_23(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC, 0, 0);
        em_demo_pos_set(self, 172.0f, 4.0f, -388.7f);
        em_demo_rot_set(self, 0.0f, 323.2f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_24(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 0, 0);
        em_demo_pos_set(self, 1262.9f, 4.0f, 3557.6f);
        em_demo_rot_set(self, 0.0f, 175.5f, 0.0f);
        return;
    case 1:
        if (em_demo_time_ck(0x96) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0xF2) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 2080.4f, 4.0f, 1380.9f);
            em_demo_rot_set(self, 0.0f, 213.7f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x13C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE, 0, 0);
            em_demo_pos_set(self, 4055.6f, 4.0f, 200.4f);
            em_demo_rot_set(self, 0.0f, 261.0f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x1F6) == 1) {
            self->state += 1;
            em_mot_set(self, 1, 0, 0);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x56E) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC, 0, 0);
            em_demo_pos_set(self, -131.3f, 4.0f, 141.5f);
            em_demo_rot_set(self, 0.0f, 153.7f, 0.0f);
            em_demo_enable(self);
            return;
        }
        break;
    case 6:
        if ((em_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_frame_check(self, 0, 92.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        if (em_demo_time_ck(0x678) == 1) {
            self->state += 1;
            em_mot_set(self, 0x12, 0xC, 0);
            return;
        }
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xC, 0, 0);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x74C) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC, 0, 0);
            return;
        }
        break;
    case 8:
        if ((em_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_frame_check(self, 0, 92.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0xC8, 0x10, 0);
            return;
        }
        if (em_mot_end_ck(self) == 1) {
            em_mot_set(self, 0xC, 0, 0);
            return;
        }
        break;
    case 9:
        if ((em_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_frame_check(self, 0, 118.0f, 0.0f) == 1) || (em_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_frame_check(self, 0, 224.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 9, 0, 1.8f);
        }
        break;
    }
}

void em010_act13_25(_ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 0, 0);
        em_demo_pos_set(self, -131.4f, 4.0f, 141.4f);
        em_demo_rot_set(self, 0.0f, 153.7f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em010_act13_dispatch(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        em010_act13_0(self);
        return;
    case 1:
        em010_act13_1(self);
        return;
    case 2:
        em010_act13_2(self);
        return;
    case 3:
        em010_act13_3(self);
        return;
    case 4:
        em010_act13_4(self);
        return;
    case 5:
        em010_act13_5(self);
        return;
    case 6:
        em010_act13_6(self);
        return;
    case 7:
        em010_act13_7(self);
        return;
    case 8:
        em010_act13_8(self);
        return;
    case 9:
        em010_act13_9(self);
        return;
    case 10:
        em010_act13_10(self);
        return;
    case 11:
        em010_act13_11(self);
        return;
    case 12:
        em010_act13_12(self);
        return;
    case 13:
        em010_act13_13(self);
        return;
    case 14:
        em010_act13_14(self);
        return;
    case 15:
        em010_act13_15(self);
        return;
    case 16:
        em010_act13_16(self);
        return;
    case 17:
        em010_act13_17(self);
        return;
    case 18:
        em010_act13_18(self);
        return;
    case 19:
        em010_act13_19(self);
        return;
    case 20:
        em010_act13_20(self);
        return;
    case 21:
        em010_act13_21(self);
        return;
    case 22:
        em010_act13_22(self);
        return;
    case 23:
        em010_act13_23(self);
        return;
    case 24:
        em010_act13_24(self);
        return;
    case 25:
        em010_act13_25(self);
        return;
    }
}

void em010_action_dispatch(_ENEMY_WORK *self) {
    switch (self->action) {
    case 0:
        em010_act0_dispatch(self);
        return;
    case 1:
        em010_act1_dispatch(self);
        return;
    case 2:
        em010_act2_dispatch(self);
        return;
    case 7:
        em010_act7_dispatch(self);
        return;
    case 10:
        em010_act10_se_play(self);
        return;
    case 11:
        em010_act11_se_play(self);
        return;
    case 12:
        em010_act12_dispatch(self);
        return;
    case 13:
        em010_act13_dispatch(self);
        return;
    }
}

void em010_effect_spawn(_ENEMY_WORK* self, u8 a, u8 b, s32 c, s32 d, f32 e) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch ((s32) a) {
    case 0:
        eft009_spawn_at_joint(self, (u32) c, (u32) b, d, e);
        return;
    case 1:
        get_joint_wpos_em(self, (u32) c, &vec);
        vec.y = self->field_0x20C;
        e *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&vec, self->area_no, b, d, e);
        return;
    }
}

void em010_motion_events(_ENEMY_WORK* self) {
    VEC3 unused;
    VEC3 pos;

    VEC3_ctor(&unused);
    VEC3_ctor(&pos);
    switch (em_get_mot_no(self)) {
    case 3:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            if (self->field_0x228 & 6) {
                setVector3(&pos, 0.0f, 0.0f, 20.0f);
                eft009_spawn_at_pos(self, 0x17, 0x33, 0, 2.0f, &pos);
            } else {
                setVector3(&pos, 0.0f, 0.0f, 30.0f);
                eft009_spawn_at_pos(self, 3, 9, 0, 1.0f, &pos);
            }
        }
        break;
    case 5:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x33, 0x17, 0, 2.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 0x1F, 0, 1.0f);
            }
        }
        break;
    case 21:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, -70.0f);
                eft009_spawn_at_pos(self, 0x17, 0x33, 0, 2.0f, &pos);
            }
        } else {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 0x1F, 0, 0.6f);
            }
        }
        break;
    case 26:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x33, 0x17, 0, 2.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 0x1B, 0, 0.6f);
            }
        }
        break;
    case 27:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x33, 0x17, 0, 2.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 0x1F, 0, 0.6f);
            }
        }
        break;
    case 29:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x33, 0x17, 0, 2.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 0x1F, 0, 1.0f);
            }
        }
        break;
    case 107:
    case 108:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x36, 3, 0, 1.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 3, 0, 1.4f);
            }
        }
        break;
    case 114:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 0x36, 3, 0, 1.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
                em010_effect_spawn(self, 0, 9, 3, 0, 1.8f);
            }
        }
        break;
    }
}

void em010_area_start_set(_ENEMY_WORK* self, u8 *a, u8 *b) {
    u32 stale;
    u8 area;
    u8 area2;
    u8 area3;
    u8 map;

    stale = 0;
    em_move_mode_set(self, 0);
    *a = 0xC;
    map = stage_map_kind_get(self->field_0x1E0);
    switch ((s32) map) {
    case 1:
        area = self->area_no;
        switch ((s32) area) {
        case 1:
            em_move_target_set(self, 2, 5, 200.0f);
            *b = 1;
            break;
        case 2:
            em_move_target_set(self, 0x13, 0x14, 320.0f);
            *b = 0;
            break;
        case 3:
            em_move_target_set(self, 0xA, 0xC, 440.0f);
            *b = 1;
            break;
        case 4:
            em_move_target_set(self, 0xA, 0xC, 140.0f);
            *b = 1;
            break;
        case 5:
            em_move_target_set(self, 0xA, 0xC, 400.0f);
            *b = 1;
            break;
        case 9:
            em_move_target_set(self, 9, 0xB, 360.0f);
            *b = 1;
            break;
        case 10:
            em_move_target_set(self, 4, 5, 300.0f);
            *b = 1;
            break;
        default:
            stale = 1;
            break;
        }
        break;
    case 2:
        area2 = self->area_no;
        switch (area2) {
        case 1:
            em_move_target_set(self, 3, 7, 200.0f);
            *b = 1;
            break;
        case 2:
            em_move_target_set(self, 0xA, 0xC, 200.0f);
            *b = 1;
            break;
        case 3:
            em_move_target_set(self, 9, 0xB, 160.0f);
            *b = 1;
            break;
        case 4:
            if (stg_area_flag_index_ck(2) == 1) {
                em_move_target_set(self, 0x13, 0x14, 160.0f);
            } else {
                em_move_target_set(self, 9, 0xA, 100.0f);
            }
            *b = 1;
            break;
        case 5:
            em_move_target_set(self, 9, 0xA, 100.0f);
            *b = 4;
            break;
        case 6:
            em_move_target_set(self, 2, 5, 200.0f);
            *b = 1;
            break;
        case 7:
            em_move_target_set(self, 0xB, 0xD, 160.0f);
            *b = 1;
            break;
        case 9:
            em_move_target_set(self, 3, 4, 200.0f);
            *b = 1;
            break;
        case 10:
            em_move_target_set(self, 1, 3, 200.0f);
            *b = 1;
            break;
        default:
            stale = 1;
            break;
        }
        break;
    case 4:
        area3 = self->area_no;
        switch ((s32) area3) {
        case 1:
            em_move_target_set(self, 7, 9, 200.0f);
            *b = 1;
            break;
        case 2:
            em_move_target_set(self, 9, 0xB, 200.0f);
            *b = 1;
            break;
        case 3:
            em_move_target_set(self, 0xA, 0xC, 200.0f);
            *b = 1;
            break;
        case 5:
            em_move_target_set(self, 0xA, 0x10, 100.0f);
            *b = 7;
            break;
        case 6:
            em_move_target_set(self, 8, 0xE, 100.0f);
            *b = 5;
            break;
        case 7:
            em_move_target_set(self, 0, 2, 200.0f);
            *b = 1;
            break;
        case 9:
            em_move_target_set(self, 0, 2, 200.0f);
            *b = 1;
            break;
        default:
            stale = 1;
            break;
        }
        break;
    case 8:
        if ((s32) self->area_no == 1) {
            em_move_target_set(self, 0, 1, 100.0f);
            *b = 1;
        } else {
            stale = 1;
        }
        break;
    case 9:
        if ((s32) self->area_no == 0) {
            em_move_target_set(self, 0, 1, 100.0f);
            *b = 1;
        } else {
            stale = 1;
        }
        break;
    default:
        stale = 1;
        break;
    }
    if (stale == 1) {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        em_reaction_req(self, 5, 2, 0xA);
        *b = 0;
    }
}

s32 em010_state_clear_ck(_ENEMY_WORK *self) {
    return self->field_0x1E2 == 0;
}

s32 em010_area_change_ck(_ENEMY_WORK* self) {
    _ENEMY_WORK* partner;
    _ENEMY_DATA *data;
    u32 map_index;
    u8 map;
    u8 entry_state;

    map = stage_map_kind_get(self->field_0x1E0);
    map_index = map - 1;
    if ((map_index > 1) && ((s32) map != 4)) {
        return 0;
    }
    if (((map_index <= 1) || ((s32) map == 4)) && ((s32) self->field_0x011 != 0)) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 7;
        self->field_0x1FF = 0;
        return 1;
    }
    data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
    if (data != NULL) {
        partner = data->work_0x18;
        if ((partner != NULL) && ((map_index <= 1) || ((s32) map == 4)) && ((u8) self->field_0x46C == (u8) data->field_0x17)) {
            entry_state = data->field_0x08;
            if ((entry_state == 5) && ((u8) self->area_no != (u8) data->field_0x10)) {
                if (em012_area_move_armed_ck(partner) == 1) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = data->field_0x10;
                } else {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 0;
                    self->field_0x1FF = data->field_0x10;
                }
                return 1;
            }
            if ((entry_state != 5) && ((u8) self->area_no != (u8) data->field_0x14)) {
                if (em012_area_move_armed_ck(partner) == 1) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 1;
                    self->field_0x1FF = data->field_0x14;
                } else {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 0;
                    self->field_0x1FF = data->field_0x14;
                }
                return 1;
            }
        }
    }
    return 0;
}

void em010_mini_init(void) {

}

void em010_mini_step(_ENEMY_MINI_WORK *self) {
    _ENEMY_DATA *data;
    u8 roam;
    u8 map;
    u8 next_area;

    map = stage_map_kind_get(get_now_mapno());
    if ((((u32) (map - 1) <= 1) || ((s32) map == 4)) && (data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->monster_0x02, self->team_0x03), self->entry_0x09), ((data == NULL) == 0)) && ((u8) self->mode_0x0C == 3)) {
        roam = self->roam_state_0x20;
        switch ((s32) roam) {
        case 0:
            if ((u8) self->area_0x0A != (u8) data->field_0x14) {
                self->roam_state_0x20 = 1;
                self->move_timer_0x22 = (ran_suu(1) & 0x3F) + 0xF;
                return;
            }
            break;
        case 1:
            self->move_timer_0x22 -= 1;
            if (self->move_timer_0x22 < 0) {
                self->roam_state_0x20 = 0;
                self->move_timer_0x22 = 0;
                if ((u8) data->field_0x0F == 3) {
                    next_area = em_kind_area_next(self->monster_0x02, self->team_0x03, self->area_0x0A, data->field_0x10, (u16)ran_suu(1));
                } else {
                    next_area = em_kind_area_next(self->monster_0x02, self->team_0x03, self->area_0x0A, data->field_0x14, (u16)ran_suu(1));
                }
                if (em_area_entry_enter_ck((EmAreaEntry*)self, data->map_0x13, next_area) == 1) {
                    self->area_prev_0x0B = self->area_0x0A;
                    self->area_0x0A = next_area;
                }
            }
            break;
        }
    }
}

void em010_mini_state_reset(_ENEMY_MINI_WORK *self) {
    if (((s32) self->busy_0x15 == 0) && (em_area_vacant_wait_ck_c1((_ENEMY_WORK*)self, 6) == 1)) {
        em_attack_finished_clear((_ENEMY_WORK*)self);
    }
}

} /* extern "C" */

/* The unit's `.bss`: the nine two-vector records MWCC's `__sinit` seeds (each a point and its step). */
Em010VecPair em010_vec_pair_0(-26.0f, 32.0f, 0.0f, 0.0f, -3.4f, 0.0f); /* 0x806A7898 */
Em010VecPair em010_vec_pair_1(26.0f, 32.0f, 0.0f, 0.0f, -3.4f, 0.0f);  /* 0x806A78B0 */
Em010VecPair em010_vec_pair_2(0.0f, 32.0f, -26.0f, 0.0f, -3.4f, 0.0f); /* 0x806A78C8 */
Em010VecPair em010_vec_pair_3(0.0f, 32.0f, 26.0f, 0.0f, -3.4f, 0.0f);  /* 0x806A78E0 */
Em010VecPair em010_vec_pair_4(-30.0f, 34.0f, 0.0f, 0.0f, -3.4f, 0.0f); /* 0x806A78F8 */
Em010VecPair em010_vec_pair_5(30.0f, 34.0f, 0.0f, 0.0f, -3.4f, 0.0f);  /* 0x806A7910 */
Em010VecPair em010_vec_pair_6(0.0f, 34.0f, -30.0f, 0.0f, -3.4f, 0.0f); /* 0x806A7928 */
Em010VecPair em010_vec_pair_7(0.0f, 34.0f, 30.0f, 0.0f, -3.4f, 0.0f);  /* 0x806A7940 */
Em010VecPair em010_vec_pair_8(0.0f, 45.0f, 50.0f, 0.0f, -4.0f, 0.0f);  /* 0x806A7958 */
