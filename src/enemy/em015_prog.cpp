/*
 * enemy/em015_prog.cpp - enemy 015's program: the `em015_prog_tbl` entry points (init, frame update, action run,
 *   motion/event hooks, TEV colour, state queries), the per-action dispatchers and their motion steps, the sound
 *   programs of actions 10 and 11, the demo steps, the per-motion event driver and the static initialiser.
 * RANGE. .text 0x80176C30-0x80182C40 (112 functions); extab 0x8000E6D4-0x8000E9E4, extabindex
 *   0x80029928-0x80029DC0, .ctors 0x8056F338-0x8056F33C (`em015_static_init`), .rodata 0x8056FDD0-0x8056FF50 (the
 *   six turn tables), .data 0x805A94A0-0x805AA930 (`em015_prog_tbl` first), .bss 0x806A79D0-0x806A7A00
 *   (`em015_se_shift`), .sdata2 0x80797B10-0x80797E88.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`+`cmpwi` and `addi`+`stb`
 *   forms, playbook 39), `#pragma fp_contract off` (the target has no fused multiply-add) and `#pragma pool_data off`
 *   (each sound table is addressed with its own `lis`/`addi`).
 * NAMES. The file name follows the runtime dump's `em015_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em024ProgTbl` slot each fills): em015_color_block_init, em015_init, em015_frame_update,
 *   em015_action_run, em015_motion_next, em015_event_notify, em015_motion_events, em015_tev_color_update,
 *   em015_state_query, em015_death_action_get, em015_aim_reset, em015_dead_mode_ck, em015_part_clamp,
 *   em015_area_start_ck, em015_area_follow.
 *   GUESS (dispatcher slot, the role word where the body shows one): em015_act<action>_<sub-state or role>, the
 *   em015_act<N>_dispatch selectors, em015_act10_se_play, em015_act11_se_play; GUESS (from the bodies):
 *   em015_swim_in, em015_circle_attack, em015_discharge, em015_leap, em015_dive, em015_rise, em015_rest,
 *   em015_surface, em015_spin, em015_lunge, em015_charge_field, em015_great_discharge, em015_tail_slam, em015_bite,
 *   em015_back_step, em015_thunder_ball, em015_thunder_ball_aimed, em015_rolling_charge, em015_turn_slam,
 *   em015_thunder_burst, em015_thunder_burst_great, em015_reposition, em015_demo_dive_in, em015_demo_wake,
 *   em015_demo_arena, em015_mouth_sparks, em015_color_step, em015_step_toward, em015_part_flag_set,
 *   em015_ally_front_ck, em015_damage_level, em015_aim_set, em015_spawn_rec_set, em015_user_data_ctor/_dtor/
 *   _node_calc, em015_static_init.
 *   GUESS (data): em015_turn_seq_*, em015_area_follow_tbl, em015_area_point_tbl, em015_se_seq_<sub-state> and
 *   em015_se_alt_seq_<sub-state> (the sub-state that first plays each table) with their em015_se_rec/pos/ofs records,
 *   em015_demo_keys, em015_se_shift; the `Em015ProgTbl` slot names.
 * RESIDUALS. `em015_rest`: retail colours `self` r31 and `kind`/`limit` r30, ours the other way round (the local's
 *   declaration place, an `if`/`else` select and a `u8`/`s32` parameter all leave it).
 *   `em015_user_data_ctor` stores the class table 0x805AA900 by hand (rule 10): the user-data class hierarchy
 *   (`enemy/fn_80138074.cpp`'s record) is not modelled, so `.data` stops at +0x145C - the 4-byte pad and the
 *   0x30-byte table are not emitted.
 *   `.sdata2` is 0x374 against the 0x378 claim: the last word is the alignment pad before the next unit's pool.
 *   flipcheck: the class table blocks a flip.
 * SHAPES. A `u8` selector parameter switched on directly (`switch (kind)`) gives retail's `subi`+`cmplwi` range
 *   lowering (`em015_back_step`, `em015_event_notify`); `switch ((u8)kind)` on a `u32` gives `cmpwi`/`blt` pairs.
 *   A masked selector compared `(s32)(u8)kind == N` gives retail's `cmpwi`.  A call written as the right operand of
 *   `*` is evaluated first: `x * get_em_chg_scale(self)` in place gives retail's operand order where a `scale` local
 *   swaps the `fmuls` operands.  The float argument of `em_eft_fx_spawn`, `em_eft_fx_spawn_pair`,
 *   `em_eft_fx_spawn_at`, `em_eft_fx_spawn_free` and `em_gauge_period_step` precedes the trailing integers in the
 *   `_sf` cast views, which schedules its load where retail has it (the owners' prototypes list it last).  The `ShellSetFuncs` +0x2C/+0x30/+0x3C/+0x50 slots are
 *   called without the table argument.  A select written `if (...) a; else b;` schedules the compare before the
 *   constants; `default:` goes first where retail lays the default body first (`em015_lunge`, `em015_color_step`).
 *   The data is defined between the functions in retail order; the two 8-byte `EmSeOffset` records are placed in
 *   `.data` with `__declspec(section ".data")`.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/fn_8012A354.h" /* fn_8012A354 (rule 2: the owner's header) */
#include "enemy/fn_8012999C.h" /* fn_8012999C (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/fn_80128030.h" /* fn_80128030 (rule 2: the owner's header) */
#include "enemy/em_gauge_tick.h" /* em_gauge_tick (rule 2: the owner's header) */
#include "enemy/em_tint_hold_start.h" /* em_tint_hold_start (rule 2: the owner's header) */
#include "enemy/em_tint_hold_end.h" /* em_tint_hold_end (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mat_blend_reset.h" /* em_mat_blend_reset (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_approach_step.h" /* em_approach_step (rule 2: the owner's header) */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start (rule 2: the owner's header) */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step (rule 2: the owner's header) */
#include "enemy/em_move_vec2_clr.h" /* em_move_vec2_clr (rule 2: the owner's header) */
#include "enemy/get_em_base_scale.h" /* get_em_base_scale (rule 2: the owner's header) */
#include "enemy/CancelFade.h" /* CancelFade (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_801251D0.h" /* em_se_tbl_play_alt/fn_80128030/fn_8012933C/fn_80129668/fn_80129724 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set/em_busy_ck */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h" /* em_demo_pos_set/fn_8014610C */
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* em_data_point_get/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* em_float_depth_get */
#include "enemy/fn_8012BDF4.h" /* em_act11_wait_ck */
#include "enemy/fn_80138074.h" /* em_res_user_data_flag_set/em_res_user_data_dtor + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* sin_idx_get/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#include "stage/stg_w.h"
#include "ef/eft009.h"          /* eft009_spawn_at_joint */
#include "enemy/em_fall_start.h" /* em_fall_start */
#include "enemy/em_status_set.h" /* em_status_set */
#include "mh3_pad/system_w.h"   /* system_w */
#include "lobby/fn_801E0ADC.h"  /* eft_em_spawn_joint */
#include "ef/fn_8030681C.h"     /* eft_spawn_type_at_area */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "ef/cp_vector.h"       /* _CP_VECTOR */
#include "enemy/em_se_record.h"  /* EmSeEntry */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play */
#include "ef/eft007.h"          /* eft007_part_set / eft007_part_spawn */
#include "ef/fn_8010D1A8.h"     /* eft_spawn_pos_in_area */
#include "g3d/g3d_calcworld.h"  /* addVec3To */
#include "enemy/em_common.h"    /* em_mot_layer_blend_set/em_rest_flag_set/em_surface_clamp/em_mot_frame_get */
#include "enemy/em_eft_fx_spawn_at.h" /* em_eft_fx_spawn_at/em_eft_fx_spawn_free */
#include "ef/eft013_em_place.h" /* eft013_em_place/eft013_em_spawn_head */
#include "ef/eft007_part_offset_get.h" /* eft007_part_offset_get */
#include "ef/eft035.h"          /* eft035_set */
#include "Pl/pl_coll.h"         /* GetGroundHit2 */
#include "camera/camera_main.h" /* camera_em_shake_req */
#include "ef/eft026_pl_spawn.h" /* eft026_pl_spawn */
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)
#define em015_part_flag_set_view1 ((void (*)(_ENEMY_WORK*, s32))em015_part_flag_set)
#define em_act_arm_unless_down_view1 ((void (*)(_ENEMY_WORK*, u32, u32))em_act_arm_unless_down)
#define em_turn_to_target_view1 ((void (*)(_ENEMY_WORK*, u32))em_turn_to_target)
#define em_turn_seq_start_view1 ((void (*)(_ENEMY_WORK*, void*, u32, u32, u32))em_turn_seq_start)
#define em_target_pos_set_view1 ((void (*)(_ENEMY_WORK*, u32))em_target_pos_set)
#define em_state_set_view1 ((void (*)(_ENEMY_WORK*, s32, s32))em_state_set)
#define em_parts_damage_level_get_view1 ((u32 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
#define em_mot_set_blend_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
#define em_mot_set_blend_view1 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
#define em_hit_window_set_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32))em_hit_window_set)
#define em_hit_window_set_view1 ((void (*)(_ENEMY_WORK*, u32, u32, u32))em_hit_window_set)
#define em_die_ck_view1 ((u32 (*)(_ENEMY_WORK*))em_die_ck)
#define em_approach_start_view1 ((void (*)(_ENEMY_WORK*, f32, s32))em_approach_start)
#define eft026_pl_spawn_c1 ((void (*)(_ENEMY_WORK*, u8, nw4r::math::VEC3*, u32, f32))eft026_pl_spawn)
#define em_turn_to_target_h ((u32 (*)(_ENEMY_WORK*, u16))em_turn_to_target)
#define calcVecAngX_s32 ((s32 (*)(nw4r::math::VEC3*))calcVecAngX)
#define assignVec3_c1 ((void (*)(nw4r::math::VEC3*, nw4r::math::VEC3*))assignVec3)
#define em_area_mot_ck_view ((u8 (*)(_ENEMY_WORK*, u8))em_area_break_ck)
#define em_fall_start_c1 ((void (*)(_ENEMY_WORK*, f32))em_fall_start)
#define em_water_check_c1 ((u32 (*)(_ENEMY_WORK*))em_water_check)
#define em_move_offset_step_update_c1 ((u32 (*)(_ENEMY_WORK*, u32*, f32))em_move_offset_step_update)

/* The +0x2C, +0x30, +0x3C and +0x50 slots spelled without their trailing table argument: this unit's call sites pass
 * none, and the compiler then loads the table last, into the register the argument would take. */
typedef void (*ShellJobInjectFn)(struct _ENEMY_WORK* self, s32 a, s32 b, nw4r::math::VEC3* pos, f32 scale,
                                 u16 flags);
typedef void (*ShellJobPairFn)(struct _ENEMY_WORK* self, u32 a, nw4r::math::VEC3* from, nw4r::math::VEC3* to,
                               f32 scale, u16 flags);
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);
typedef void (*ShellFlagJobFn)(struct _ENEMY_WORK* self, u32 a, u32 b);

/* The callees whose owners' prototypes list the float last: this unit's call sites pass it before the trailing
 * integers, which schedules its load where retail has it (the registers are the same either way). */
#define em_eft_fx_spawn_sf ((void (*)(_ENEMY_WORK*, u32, u32, f32, u32))em_eft_fx_spawn)
#define em_eft_fx_spawn_pair_sf ((void (*)(_ENEMY_WORK*, u32, u32, u32, f32, u32))em_eft_fx_spawn_pair)
#define em_eft_fx_spawn_at_sf ((void (*)(_ENEMY_WORK*, u32, u32, f32, u32, const nw4r::math::VEC3*))em_eft_fx_spawn_at)
#define em_eft_fx_spawn_free_sf     ((void (*)(u32, const nw4r::math::VEC3*, const _CP_VECTOR*, f32, u8, u8))em_eft_fx_spawn_free)
#define em_gauge_period_step_sf ((void (*)(_ENEMY_WORK*, f32, s32, s16))em_gauge_period_step)



/* The key path the dive-in demo replays through `em_demo_key3_apply` (`.data`, 0x160 bytes; GUESS name). */

/* This unit's steps, declared ahead of their first callers. */
extern "C" {
void em015_swim_in(_ENEMY_WORK* self, u32 kind, u32 motion);
void em015_act5_dispatch(_ENEMY_WORK* self);
}

/* The 12-byte helper `em015_init` allocates and `em015_user_data_ctor` constructs (+0x00: `lbl_805AA900`).
 * size: 0xC (`operator new(0xC)`) */
typedef struct Em015UserData {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Em015UserData;

/* The helper's table, in this unit's `.data`. */
extern "C" u8 lbl_805AA900[];

/* --------------------------------------------------------------------------------------------- */
/* callees                                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* The C++ free functions are declared by their real signatures, so the front-end emits the map's manglings. */
extern "C" u8 stage_map_kind_get(u8 id);
extern "C" void em015_aim_set(_ENEMY_WORK* self);
extern "C" u32 quest_id_get(void);
extern "C" void em015_part_flag_set(_ENEMY_WORK* self, u32 a);

/* This unit's own forward declaration (em015_init calls it before its definition). */
extern "C" Em015UserData* em015_user_data_ctor(Em015UserData* self);

/* The two mangled free functions, declared by their real signatures (rule 9: never the mangled
 * spelling - the front-end produces `em_die_ck__FP11_ENEMY_WORK` from this declaration). */

/* `em_frame_check` at global scope: the front-end emits `em_frame_check__FP11_ENEMY_WORKUsff` (rule 9). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {
/* Callees whose owners' headers do not declare these spellings: `lobby/fn_8030121C.cpp`'s `eft_em_spawn`
 * and `draw_shape_arm.cpp`'s `draw_shape_arm`. */
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, VEC3* v, f32 s);
void draw_shape_arm(_ENEMY_WORK* self, u32 a, u32 b);

}

/* The unit's `.sdata2` and `.rodata` constants, declared: the source does not emit them yet. */


extern "C" {

/* This unit's two entries the dispatcher tail-branches into, defined further down. */
void em015_act1_charge(_ENEMY_WORK* self);
void em015_act1_sleep(_ENEMY_WORK* self);
}

/* Callees and pool literals, declared by their map spelling. */

/* nw4r math free functions the map carries at global scope (`...__FPQ34nw4r4math4VEC3...`). */
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

/* Mangled enemy-service entry points (declared as C++ prototypes so the compiler mangles them). */
u16 em_get_mot_no(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);

extern "C" {

/* This unit's dispatch targets in 0x80177890..0x80178128. */
void em015_act2_approach(_ENEMY_WORK* self, s32 index);
void em015_act2_turn(_ENEMY_WORK* self);
void em015_act2_hit_chain(_ENEMY_WORK* self);
void em015_act2_drift(_ENEMY_WORK* self, s32 index);
void em015_act2_drift_long(_ENEMY_WORK* self, s32 index);

/* The unit's action tables and aim scale constants, declared: the source does not emit them yet. */


/* `enemy/em_model.cpp`'s pose request `em015_act13_drop_item` posts (`self`, the pose vector, the request id); its
 * callers spell it two ways, so each declares its own. */
void em_drop_item_spawn(_ENEMY_WORK* self, u32 v, u32 a);
/* This unit's unwritten table writers, which the written ones call. */
void em015_reposition(_ENEMY_WORK* self);
void em015_demo_dive_in(_ENEMY_WORK* self);
void em015_demo_wake(_ENEMY_WORK* self);
void em015_demo_arena(_ENEMY_WORK* self);

/* The unit's action tables and aim scale constants, declared: the source does not emit them yet. */





}

extern "C" {
/* `stage_map_kind_get` (the byte-table map lookup) comes from `stage/stg_w.h`. */

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions, declared up front so the later ones can call forward.
 * ------------------------------------------------------------------------------------------------ */
f32 em015_step_toward(f32 value, f32 center, f32 step);
void em015_color_step(_ENEMY_WORK* self);
void em015_tev_color_update(_ENEMY_WORK* self);
u32 em015_dead_mode_ck(_ENEMY_WORK* self);
void em015_part_clamp(_ENEMY_WORK* self, u32 part);
u32 em015_area_start_ck(_ENEMY_WORK* self, u32 arg);
void em015_area_follow(_ENEMY_WORK* self);
void em015_part_flag_set(_ENEMY_WORK* self, u32 value);
void em015_user_data_node_calc(EmUserData* self);
u32 em015_ally_front_ck(_ENEMY_WORK* self, u32 arg);
u32 em015_act13_early_ck(_ENEMY_WORK* self);
u8 em015_state_query(_ENEMY_WORK* self, u32 kind);
void em015_death_action_get(_ENEMY_WORK* self, u8* out_a, u8* out_b);
void em015_aim_reset(_ENEMY_WORK* self);
u8 em015_damage_level(_ENEMY_WORK* self);
void em015_aim_set(_ENEMY_WORK* self);
void em015_spawn_rec_set(EmSpawnRec* out, u8 a, u16 b, u16 c);
Em015UserData* em015_user_data_dtor(Em015UserData* self, s16 arg);
void em015_static_init(void);

/* The four 0xC-byte vectors `em015_static_init` seeds. */
extern nw4r::math::VEC3 em015_se_shift[4];
}

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head (the frame count,
 * the turn span and the packed rate), then three four-word rows whose first word carries the motion in its high
 * half.  The names are a GUESS from their users. */

extern "C" const u32 em015_turn_seq_wait[16] = {
    0x0000000A, 0x00000032, 0x00000000, 0x5C722AAB,
    0x00160000, 0x00000006, 0x00000000, 0x00000000,
    0x00180000, 0x0000000A, 0x00000000, 0x00000000,
    0x00190000, 0x0000000A, 0x00000000, 0x00000000,
};

extern "C" const u32 em015_turn_seq_aim[16] = {
    0x0000000A, 0x00000032, 0x00000000, 0x60002000,
    0x00160000, 0x00000006, 0x00000000, 0x00000000,
    0x001F0000, 0x0000000A, 0x00000000, 0x00000000,
    0x00200000, 0x0000000A, 0x00000000, 0x00000000,
};

extern "C" const u32 em015_turn_seq_swim[16] = {
    0x0000000A, 0x00000028, 0x00000000, 0x80002AAB,
    0x00380000, 0x00000014, 0x00000000, 0x00000001,
    0x00330000, 0x0000000A, 0x00000000, 0x00000001,
    0x00340000, 0x0000000A, 0x00000000, 0x00000001,
};

extern "C" const u32 em015_turn_seq_swim_long[16] = {
    0x0000000A, 0x00000064, 0x00000000, 0x80002AAB,
    0x00380000, 0x00000014, 0x00000000, 0x00000001,
    0x00260000, 0x0000000A, 0x00000000, 0x00000001,
    0x00270000, 0x0000000A, 0x00000000, 0x00000001,
};

extern "C" const u32 em015_turn_seq_left[16] = {
    0x00000050, 0x00000082, 0x00000000, 0xA0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x002C0000, 0x00000028, 0x00000050, 0x00000001,
    0x002C0000, 0x00000028, 0x00000050, 0x00000001,
};

extern "C" const u32 em015_turn_seq_right[16] = {
    0x00000050, 0x00000082, 0x00000000, 0xA0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00290000, 0x00000028, 0x00000050, 0x00000001,
    0x00290000, 0x00000028, 0x00000050, 0x00000001,
};

/* The em015 program table (`em015_prog_tbl`): the entry points the shared enemy program driver calls (slot names
 * follow `Em024ProgTbl`'s scheme, a GUESS from the functions they hold).
 * size: 0x6C */
struct Em015ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u32 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(_ENEMY_WORK* self, u8* state, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u8 (*state_query_0x24)(_ENEMY_WORK* self, u32 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*aim_reset_0x34)(_ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[4];
    /* +0x48 */ u32 (*dead_mode_ck_0x48)(_ENEMY_WORK* self);
    /* +0x4C */ u32 pad_0x4C;
    /* +0x50 */ void (*part_clamp_0x50)(_ENEMY_WORK* self, u32 part);
    /* +0x54 */ u32 (*area_start_ck_0x54)(_ENEMY_WORK* self, u32 arg);
    /* +0x58 */ void (*area_follow_0x58)(_ENEMY_WORK* self);
    /* +0x5C */ u32 pad_0x5C[4];
};

extern "C" {
void em015_init(_ENEMY_WORK* self, u32 arg);
void em015_frame_update(_ENEMY_WORK* self);
void em015_action_run(_ENEMY_WORK* self);
void em015_motion_next(_ENEMY_WORK* self, u8* state, u8* sub);
void em015_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub);
void em015_motion_events(_ENEMY_WORK* self);
void em015_color_block_init(_ENEMY_WORK* self);
}

extern "C" Em015ProgTbl em015_prog_tbl = {
    em015_color_block_init,
    em015_init,
    em015_frame_update,
    em015_action_run,
    em015_motion_next,
    em015_event_notify,
    em015_motion_events,
    0,
    em015_tev_color_update,
    em015_state_query,
    {0, 0},
    em015_death_action_get,
    em015_aim_reset,
    {0, 0, 0, 0},
    em015_dead_mode_ck,
    0,
    em015_part_clamp,
    em015_area_start_ck,
    em015_area_follow,
    {0, 0, 0, 0},
};

/* The two records `em015_area_start_ck` hands `em_pack_area_pick` (word-sized fields; their meaning is a GUESS: a frame count,
 * a packed id pair and an 0xFFFF terminator). */
u32 em015_area_follow_tbl[3] = {0x1E, 0x01F4000A, 0xFFFF0000};
u32 em015_area_point_tbl[3] = {0x1E, 0x0096000A, 0xFFFF0000};

/* 0x80176C30 (0x28): resets the colour/step block: the scalar to 255 and the K-colour bytes, flags and timer to 0. */
extern "C" void em015_color_block_init(_ENEMY_WORK* self) {
    self->color_0x328.field_0x328 = 255.0f;
    self->color_0x328.field_0x32C = 0;
    self->color_0x328.field_0x32D = 0;
    self->color_0x328.field_0x32E = 0;
    self->color_0x328.field_0x32F = 0;
    self->color_0x328.field_0x330 = 0;
    self->color_0x328.field_0x332 = 0;
}

/* --------------------------------------------------------------------------------------------- */
/* functions, in address order                                                                    */
/* --------------------------------------------------------------------------------------------- */
extern "C" void em015_init(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Em015UserData* helper;
    u8 kind;
    u8 mode;

    VEC3_ctor(&v);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Em015UserData*)operator new(0xC);
        if (helper != 0) {
            em015_user_data_ctor(helper);
        }
        em_res_user_data_set(self, helper);
    }
    em015_aim_set(self);
    self->field_0x1E4 = 0;
    mode = arg;
    if ((u32)(mode - 1) <= 1 || (s32)mode == 4) {
    } else {
        kind = stage_map_kind_get(self->field_0x1E0);
        switch (kind) {
          case 1:
            if (self->area_no == 5) {
                em_move_mode_set(self, 0);
                em_act_arm_unless_down_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                em_act_arm_unless_down_view1(self, 0, 4);
            }
            break;
          case 8:
            em_move_mode_set(self, 0);
            em_act_arm_unless_down_view1(self, 0, 0);
            break;
          case 9:
            if (self->area_no == 0) {
                em_move_mode_set(self, 0);
                em_act_arm_unless_down_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                em_act_arm_unless_down_view1(self, 0, 4);
            }
            break;
          default:
            em_move_mode_set(self, 2);
            em_act_arm_unless_down_view1(self, 0, 4);
            break;
        }
    }
    if ((u16)quest_id_get() == 0x3EC) {
        self->field_0x7B0 = 0.5f;
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, -80.0f, 90.0f);
        eft_em_attach_set(self, 0x1A, &v, 1.5f, 0x18E4);
        eft013_em_set(self, 1);
    }
}

extern "C" Em015UserData* em015_user_data_ctor(Em015UserData* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805AA900;
    return self;
}

extern "C" void em015_motion_next(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (state[0]) {
      case 2:
        switch (sub[0]) {
          case 0:
          case 3:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 4;
            }
            break;
          case 5:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 7;
            }
            break;
          case 6:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 8;
            }
            break;
          case 9:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xA;
            }
            break;
        }
        break;
      case 5:
        switch (sub[0]) {
          case 2:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xD;
            }
            break;
          case 0xA:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xE;
            }
            break;
          case 0xB:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xF;
            }
            break;
          case 0x1F:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x20;
            }
            break;
          case 0x24:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x25;
            }
            break;
        }
        break;
      case 7:
        switch (sub[0]) {
          case 6:
            if (em_eft_active_ck(self) == 1) {
                sub[0] = 0x17;
            }
            break;
          case 7:
          case 0xF:
            if (em_eft_active_ck(self) == 1) {
                sub[0] = 0x18;
            }
            break;
          case 0x22:
            if (em_mot_finished_ck_c1() == 1) {
                state[0] = 2;
                sub[0] = 4;
            }
            break;
        }
        break;
    }
}

extern "C" void em015_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub) {
    s32 step;
    s32 limit;

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
      case 5: {
        u8 s = sub;

        switch (s) {
          case 0x19:
          case 0x1A:
            fn_8012A354(self);
            break;
          case 0x1D:
            em_bait_react(self);
            break;
          case 0x26:
            em_act_flag1_set(self);
            break;
        }
        break;
      }
      case 7: {
        u8 s = sub;

        switch (s) {
          case 6: {
            u8 v;

            step = 0x32;
            if (self->field_0x7C8 >= 0x29) {
                step = 0x64;
            }
            v = em_parts_damage_level_get_view1(self, 3);
            if (v >= 3) {
                limit = 0x63;
            } else {
                limit = 0xFF;
            }
            if ((s32)self->field_0x1E4 < limit - step) {
                self->field_0x1E4 += (u8)step;
            } else {
                self->field_0x1E4 = (u8)limit;
            }
            break;
          }
          case 7:
          case 0xF:
            if (self->field_0x1E4 > 0x64) {
                self->field_0x1E4 -= 0x64;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
          case 0x24:
          case 0x25:
          case 0x2A:
          case 0x2B:
          case 0x2C:
            if (self->field_0x1E4 > 0x1E) {
                self->field_0x1E4 -= 0x1E;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
        }
        break;
      }
      case 0xA:
        switch (sub) {
          case 0xB1:
            fn_8012999C(self);
            break;
          case 0xC9:
          case 0xD1:
            em_part_hit_set(self, 0, 0);
            break;
        }
        break;
    }
}

extern "C" void em015_frame_update(_ENEMY_WORK* self) {
    em015_aim_set(self);
    if (em_die_ck_view1(self) == 0) {
        if (em_eft_active_ck(self) == 1) {
            if (self->field_0x1E2 == 0) {
                self->field_0x32F = 1;
                self->field_0x330 = 1;
            }
        } else {
            self->field_0x32F = 0;
        }
        if (self->field_0x38F != 0) {
            if (self->timer_0x332 < 0x384) {
                self->timer_0x332 += 1;
            }
        } else {
            self->timer_0x332 = 0;
        }
    }
    if (self->field_0x1E2 == 2) {
        self->field_0x38A = 1;
    } else {
        self->field_0x38A = 0;
    }
}

extern "C" void em015_act0_00(_ENEMY_WORK* self) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
      case 1:
        em_target_pos_set_view1(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em015_act0_01(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x14, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em015_act0_02(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x1D, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em015_act0_04(_ENEMY_WORK* self) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x28, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void em015_act0_05(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x1E, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void em015_act0_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        em015_act0_00(self);
        break;
      case 1:
        em015_act0_01(self);
        break;
      case 2:
        em015_act0_02(self);
        break;
      case 4:
        em015_act0_04(self);
        break;
      case 5:
        em015_act0_05(self);
        break;
    }
}

/* The map names these two with a bare `fn_XXXXXXXX` stem (no mangling), so they take C linkage to
 * emit that exact symbol; rule 9 keeps an `fn_` stem legal. */
extern "C" void em015_act1_charge(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        setVector3(&v, 0.0f, -50.0f, 200.0f);
        if (em_frame_check(self, 0, 106.0f, 0.0f) == 1U) {
            eft_em_spawn(self, 0, 0x18, &v, 1.0f);
        }
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1U) {
            draw_shape_arm(self, 0x1A, 0xA);
            em_hit_window_set_view1(self, 0, 0x1C, 5);
        }
        if (em_frame_check(self, 3, 112.0f, 220.0f) == 1U) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &v, 1.0f);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em015_act1_sleep(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 0xF);
        self->timer_0x020 = self->timer_0x020 - 1;
        if ((s32)self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 5, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}



extern "C" {
/* ---- the motion-state updates ---- */

void em015_act1_02(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x14, 0x14, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, 170.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act1_03(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 7, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}



extern "C" {
void em015_act1_wait(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 0xa, 0);
        self->timer_0x020 = 0x12c;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set_view1(self, 1, 6);
        }
        break;
    }
}
}



extern "C" {
void em015_act1_05(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x82, 0xa, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act1_06(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xa, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act1_07(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x1d, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The dispatcher: `state_sub` selects the motion update; cases 0/1 make it a dense 0..7 switch, which
 * MWCC lowers to the target's jump table. */
void em015_act1_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em015_act1_charge(self);
        break;
    case 1:
        em015_act1_sleep(self);
        break;
    case 2:
        em015_act1_02(self);
        break;
    case 3:
        em015_act1_03(self);
        break;
    case 4:
        em015_act1_wait(self);
        break;
    case 5:
        em015_act1_05(self);
        break;
    case 6:
        em015_act1_06(self);
        break;
    case 7:
        em015_act1_07(self);
        break;
    }
}

void em015_act2_approach(_ENEMY_WORK* self, s32 arg) {
    em015_part_flag_set_view1(self, 1);

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mat_blend_reset(self);
        em_mot_set_blend_view1(self, 0x15, 0x28, 0, 1);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start_view1(self, -400.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act2_turn(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em015_turn_seq_wait, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em015_turn_seq_wait) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act2_hit_chain(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1b, 6, 0);
        em_hit_window_set_view3(self, 0, 0x2e, 8);
        em_hit_window_set_view3(self, 1, 0x32, 0x18);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x2f, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x30, 0x18);
            }
            break;
        case 2:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x31, 0x18);
            }
            break;
        }

        switch (self->state_0x007) {
        case 0:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x33, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x34, 0x18);
            }
            break;
        }

        if (em_frame_check(self, 1, 20.0f, 80.0f) == 1) {
            em_turn_to_target_view1(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act2_drift(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start_view1(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            /* falls through to case 2's easing */
        case 2:
            em_approach_start_view1(self, -700.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em015_act2_drift_long(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, 0.0f, 0);
            break;
        case 1:
            em_approach_start_view1(self, -400.0f, 0);
            break;
        case 2:
            em_approach_start_view1(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        case 3:
            em_approach_start_view1(self, -700.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}

/* Two-phase enemy action: arm the action, then aim at the target for motion ids 0x1F/0x20. */
extern "C" void em015_act2_aim(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start_view1(self, (void*)em015_turn_seq_aim, 0, 1, 0);
        em_move_vec2_clr(self);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            u16 rel = (u16)(ang - self->field_0x1C0);

            self->offset_0x30C.vec_0x310.z = -12.0f * get_em_base_scale(self) * get_em_chg_scale(self);
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + rel);
        }
        break;
    }
    case 1: {
        u32 done = em_turn_seq_step(self, (void*)em015_turn_seq_aim);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            if (em_frame_check(self, 3, 20.0f, 70.0f) == 1) {
                CancelFade(self);
            }
        }

        if (done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* Tail-calls this action's per-sub-state handler. */
extern "C" void em015_act2_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em015_act2_approach(self, 0);
        break;
    case 1:
        em015_act2_turn(self);
        break;
    case 2:
        em015_act2_hit_chain(self);
        break;
    case 3:
        em015_act2_drift(self, 0);
        break;
    case 4:
        em015_act2_drift_long(self, 0);
        break;
    case 5:
        em015_act2_approach(self, 1);
        break;
    case 6:
        em015_act2_drift(self, 1);
        break;
    case 7:
        em015_act2_drift_long(self, 1);
        break;
    case 8:
        em015_act2_drift_long(self, 2);
        break;
    case 9:
        em015_act2_drift(self, 2);
        break;
    case 10:
        em015_act2_drift_long(self, 3);
        break;
    case 11:
        em015_act2_aim(self);
        break;
    }
}

/* Two-phase action: arm the timer action, then finish on `em_mot_end_ck`. */
extern "C" void em015_act5_00(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_view3(self, 0x32, 0x28, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms/steps an enemy action, aiming at the target once the arming frame reports 1. */
extern "C" void em015_act5_swim_turn(_ENEMY_WORK* self, u32 flag) {
    switch (self->state) {
    case 0: {
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em015_turn_seq_swim, 0, 1, 0);
        em_move_vec_clr(self);
        if ((u8)flag == 1) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, -5.0f);
            rotVecY(&self->offset_0x30C.vec_0x310, ang);
            self->timer_0x020 = 0x28;
        }
        break;
    }
    case 1:
        if ((u8)flag == 1 && self->state_0x006 == 0) {
            em_move_offset_apply(self);
            self->timer_0x020 -= 1;
            if (self->timer_0x020 <= 0) {
                self->state_0x006++;
            }
        }
        if (em_turn_seq_step(self, (void*)em015_turn_seq_swim) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, 5.0f);
        } else {
            fn_80136D4C(self, 10.0f);
        }
        break;
    }
}

/* 0x801784D0 (0x284): the swim-in: `motion` picks 0x37 or 0x30, `kind` the approach target and the +0x378 cap
 * (kind 3 rises to the surface on a 200-frame timer, kind 5 holds the frame flag), then motion 0x2F completes. */
extern "C" void em015_swim_in(_ENEMY_WORK* self, u32 kind, u32 motion) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        if ((s32)(u8)motion == 0) {
            em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        } else {
            em_mot_set_blend(self, 0x30, 0x14, 0, 1);
        }
        if ((s32)(u8)kind == 3) {
            self->vec_0x36C.y = self->field_0x210;
            self->timer_0x020 = 200;
        }
        switch ((u8)kind) {
        default:
            em_approach_start(self, -360.0f, 0x12);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 700.0f;
            break;
        case 2:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 2000.0f;
            break;
        case 4:
            em_approach_start(self, 0.0f, 0x12);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        break;
    case 1: {
        u32 done;

        switch ((u8)kind) {
        case 3:
            em_approach_step(self, 0, 0x80);
            fn_80136D4C(self, 3.0f);
            self->timer_0x020--;
            if (self->timer_0x020 <= 0 ||
                self->pos.y >= self->vec_0x36C.y - (10.0f + em_float_depth_get(self)) * get_em_chg_scale(self)) {
                done = 1;
            } else {
                done = 0;
            }
            break;
        case 5:
            em_frame_flag_set(self);
            done = 0;
            break;
        default:
            done = em_approach_step(self, 0, 0x80);
            break;
        }
        if (done == 1 && fn_8012F948(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x3A/0x0A motion, then finishes on `em_mot_end_ck`. */
extern "C" void em015_act5_03(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The timed-table variant: same arm, then the table's own completion test. */
extern "C" void em015_act5_swim_turn_long(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em015_turn_seq_swim_long, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, (void*)em015_turn_seq_swim_long) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, 5.0f);
        } else {
            fn_80136D4C(self, 10.0f);
        }
        break;
    }
}

/* 0x8017889C (0x5BC): the circling attack: turns through one of the two turn sequences (`dir`), circles the
 * target while the 1000-frame timer runs (`mode` picks the swim, the drift or the motion-bound variant), then
 * the closing motion and the hand-off to action 7. */
extern "C" void em015_circle_attack(_ENEMY_WORK* self, u32 dir, u32 mode) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
    case 0: {
        u16 angle;

        self->state++;
        em_move_mode_set(self, 2);
        if ((s32)(u8)mode != 0) {
            angle = 0x1800;
            em_approach_start(self, 0.0f, 0xA);
        } else {
            angle = 0x3000;
            copyVec3(&self->action_0x328.vec_0x334, &self->vec_0x36C);
            self->vec_0x36C.y += 2000.0f;
            if (self->vec_0x36C.y > self->field_0x210) {
                self->vec_0x36C.y = self->field_0x210;
            }
        }
        if ((s32)(u8)dir == 0) {
            em_turn_seq_start(self, (void*)em015_turn_seq_left, 0, 5, (u16)(0x10000 - angle));
        } else {
            em_turn_seq_start(self, (void*)em015_turn_seq_right, 0, 3, angle);
        }
        break;
    }
    case 1:
        if ((s32)(u8)dir == 0) {
            em_turn_seq_step(self, (void*)em015_turn_seq_left);
        } else {
            em_turn_seq_step(self, (void*)em015_turn_seq_right);
        }
        if ((s32)(u8)mode != 0) {
            em_approach_step(self, 0, 0);
        }
        if (em_frame_check(self, 1, 180.0f, 0.0f) == 1) {
            self->state++;
            self->state_0x006 = 0;
            self->timer_0x020 = 1000;
        }
        break;
    case 2: {
        u32 done;
        s32 step;

        switch ((u8)mode) {
        case 0:
            if (em_frame_check(self, 3, 80.0f, 130.0f) == 1) {
                fn_80133CC8(self, 0x40, 0x80);
            } else {
                fn_80133CC8(self, 0x40, 0x40);
            }
            em_hit_arm_set(self);
            done = 0;
            break;
        default:
            done = em_approach_step(self, 0, 0x20);
            if (em_frame_check(self, 3, 80.0f, 130.0f) == 1) {
                step = 0xA0;
            }
            step = step * get_em_base_scale(self);
            if (em_get_mot_no(self) == 0x29) {
                self->field_0x1C0 += step;
            } else {
                self->field_0x1C0 -= step;
            }
            break;
        case 3:
            if (em_frame_check(self, 3, 80.0f, 130.0f) == 1) {
                step = 0xA0;
            }
            step = step * get_em_base_scale(self);
            if (em_get_mot_no(self) == 0x29) {
                self->field_0x1C0 += step;
            } else {
                self->field_0x1C0 -= step;
            }
            if (em_mot_end_ck(self) == 1) {
                done = 1;
            }
            break;
        }
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        }
        if (done != 0 || self->timer_0x020 <= 0) {
            if (em_frame_check(self, 3, 50.0f, 200.0f) == 0) {
                self->state++;
                if ((u32)((u8)mode - 2) > 1) {
                    self->state_0x007 = 1;
                    em_mot_set_blend(self, 0x2F, 0x1E, 0, 1);
                } else {
                    em_mot_set_blend(self, 0x23, 0x14, 0, 1);
                }
            }
        } else if (em_mot_end_ck(self) == 1) {
            if ((s32)(u8)mode == 0) {
                self->state_0x006++;
                if (self->state_0x006 >= 2) {
                    self->state++;
                    self->state_0x006 = 0;
                    em_mot_set_blend(self, 0x13, 0x14, 0, 1);
                    copyVec3(&self->vec_0x36C, &self->action_0x328.vec_0x334);
                    break;
                }
            }
            if (self->state_0x007 != 1) {
                if (em_get_mot_no(self) == 0x29) {
                    em_mot_set_blend(self, 0x2C, 0x18, 0, 1);
                } else {
                    em_mot_set_blend(self, 0x29, 0x18, 0, 1);
                }
            }
        }
        break;
    }
    case 3:
        if (self->state_0x007 == 1) {
            if (em_mot_end_ck(self) == 1) {
                fn_80128030(self);
            }
        } else {
            if (em_frame_check(self, 1, 20.0f, 0.0f) == 1) {
                fn_80133CC8(self, 0x40, 0x200);
            }
            em_hit_arm_set(self);
            if ((s32)(u8)mode != 0) {
                if (em_mot_end_ck(self) == 1) {
                    em_mot_set(self, 0x24, 0xA, 0);
                    em_state_set(self, 7, 0x20);
                }
            } else if (em_mot_end_ck(self) == 1) {
                em_mot_set(self, 0x43, 0xA, 0);
                if (em015_damage_level(self) == 0) {
                    em_state_set(self, 7, 0x1F);
                } else {
                    em_state_set(self, 7, 0x25);
                }
            }
        }
        break;
    }
}

/* 0x80178E58 (0x218): the discharge: motion 0x35, the charge sparks every fourth frame of the window, the hit
 * window and the arm shape on frame 114, the trailing spark burst, then the hand-off. */
extern "C" void em015_discharge(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x35, 0x14, 0, 1);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 3, 6.0f, 112.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 2, 0x19, &vec, 1.0f);
        }
        setVector3(&vec, 0.0f, -50.0f, 100.0f);
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x14, 5);
            draw_shape_arm(self, 0x1A, 0xA);
        }
        if (em_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x19, &vec, 1.0f);
        }
        if (em_frame_check(self, 3, 114.0f, 258.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x19, &vec, 1.0f);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (em_frame_check(self, 0, 280.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 3, 0x19, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80179070 (0x280): the leap out of the water: motion 0x3C, the launch vector on frame 150, the arc until it
 * clears the surface (splash effects, motion 0x40), then the fall back down to `em_state_set(self, 5, 0x19)`. */
extern "C" void em015_leap(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3C, 0x14, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, 150.0f, 0.0f) == 1) {
            self->state++;
            em_move_vec2_clr(self);
            self->field_0x314 = 30.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
        }
        break;
    case 2:
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->pos.y >= self->field_0x210) {
            self->state++;
            self->state_0x006 = 0;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x40, 4, 0);
            eft009_spawn_at_joint(self, 0x17, 0x2C, 0, 1.5f);
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x30, 0x17, &vec, 1.0f);
            setVector3(&vec, 0.0f, 0.0f, 15.0f);
            eft_em_spawn(self, 0x32, 3, &vec, 1.0f);
            self->field_0x320 = -4.0f;
        } else {
            em_surface_clamp(self, 0.0f);
        }
        break;
    case 3: {
        u32 landed = em_move_offset_step_update_c1(self, &self->field_0x1BC, 200.0f);
        if (self->field_0x314 < -40.0f) {
            self->field_0x314 = -40.0f;
        }
        if (landed == 1 && self->field_0x314 < 0.0f) {
            em_move_mode_set(self, 0);
            em_state_set(self, 5, 0x19);
        } else if (self->field_0x314 < 0.0f) {
            if (self->field_0x210 > self->pos.y + em_float_depth_get(self) * get_em_chg_scale(self)) {
                fn_80128030(self);
            }
        }
        break;
    }
    }
}

/* 0x801792F0 (0x1DC): the dive: motion 0x3E, the fall with the 290 lift and the step-offset arc, then either the
 * surface catch (`em_state_set(self, 5, 0x1A)`) or the landing motion 0x3B with the camera shake. */
extern "C" void em015_dive(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x3E, 0xA, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_c1(self, 290.0f);
            em_mot_set(self, 0x41, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 35.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
            self->field_0x320 = -5.5f;
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2: {
        u32 landed = em_move_offset_step_update_c1(self, &self->field_0x1BC, 360.0f);
        if (em_water_check_c1(self) == 1) {
            if (self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                em_state_set(self, 5, 0x1A);
            }
        } else if (landed == 1 && self->field_0x314 < 0.0f) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0, 0);
            em_camera_req(self, 2, 7);
        }
        break;
    }
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801794CC (0x130): arms motion 0x39; on the frame window it rises by ten times the climb past 30 (capped at
 * 100) and turns to the target, completing with `fn_80128030`. */
extern "C" void em015_rise(_ENEMY_WORK* self) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x39, 0xA, 0, 1);
        break;
    case 1:
        fn_80136D4C(self, 10.0f);
        if (em_frame_check(self, 1, 28.0f, 0.0f) == 1) {
            f32 rise = 10.0f * (em_mot_frame_get(self) - 30.0f);
            if (rise > 100.0f) {
                rise = 100.0f;
            }
            em_surface_clamp(self, rise + em_float_depth_get(self));
        }
        if (em_frame_check(self, 1, 64.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x801795FC (0x1FC): the rest: motion 0x2A into the 0x2B loop, the gauge ticking until the 1800-frame (600 for
 * `kind` 1) limit or a part whose frame cost falls in 0..3000 wakes it into `em_state_set(self, 5, 0x13)`. */
extern "C" void em015_rest(_ENEMY_WORK* self, u32 kind) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2A, 0xA, 0, 1);
        em_rest_flag_set(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x2B, 0, 0);
            self->timer_0x020 = 0;
        }
        break;
    case 2: {
        s32 limit;

        em_gauge_period_step_sf(self, 1.0f, 5, 0x1E);
        if ((s32)(u8)kind == 1) {
            limit = 0x258;
        } else {
            limit = 0x708;
        }
        self->timer_0x020++;
        if (self->timer_0x020 >= limit) {
            self->timer_0x020 = limit;
        }
        if (em_busy_ck(self) == 1) {
            if ((self->timer_0x020 % 30) * 5 == 0) {
                u8 i;

                for (i = 0; i < 4; i++) {
                    f32 frames = self->frames_0x454[i];
                    if (frames > 0.0f && frames <= 3000.0f) {
                        self->state++;
                        em_reaction_req(self, 0, 2, i);
                        em_state_set(self, 5, 0x13);
                        return;
                    }
                }
            }
            if (self->timer_0x020 >= limit) {
                em_reaction_req(self, 5, 2, 5);
                em_status_set(self, 0);
                em_state_set(self, 5, 0x13);
            }
        }
        break;
    }
    }
}

/* Arms 0x2D/6, then aims the turn rate at the target: the angle difference picks one of three `step_0x07` turn
 * rates (`0`, `0x80`, or the difference's own high byte). */
extern "C" void em015_act5_turn_angle(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u16 angle;

        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 6, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle > 0x8000) {
            if (angle > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)angle >> 8);
        }
        em_busy_set(self);
        break;
    }
    case 1:
        em_turn_in_window(self, 30.0f, 110.0f, self->state_0x007 << 8);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        } else {
            em_busy_set(self);
        }
        break;
    }
}

/* Arms the 0x2E/0x14 motion, then finishes. */
extern "C" void em015_act5_20(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Three-phase: arm 0x31/0x14, step to 0x21/0, then `em_state_set(self, 5, 0x1D)`. */
extern "C" void em015_act5_21(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* Resets the shared action state, arms 0x3D/0 and the (2, 7) motion pair. */
extern "C" void em015_act5_shake(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        em_camera_req(self, 2, 7);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80179AC4 (0x174): the surfacing splash: lifts by 400 onto the water line with the camera shake and the three
 * splash effects, then motion 0x42 with the 500 rise, completing with `fn_80128030`. */
extern "C" void em015_surface(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, 400.0f);
        em_mot_set_blend(self, 0x3F, 4, 0, 1);
        self->pos.y = self->field_0x210;
        em_camera_req(self, -1, 7);
        eft009_spawn_at_joint(self, 3, 0x2C, 0, 1.5f);
        setVector3(&vec, 0.0f, 0.0f, 0.0f);
        eft_em_spawn(self, 0x30, 3, &vec, 1.0f);
        setVector3(&vec, 0.0f, -75.0f, 0.0f);
        eft_em_spawn(self, 0x31, 3, &vec, 1.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0x42, 0, 0);
            em_surface_clamp(self, 500.0f);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x68/4 motion (`em_mot_set` four-argument form), then finishes. */
extern "C" void em015_act5_27(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x22/4 motion, then finishes. */
extern "C" void em015_act5_29(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x22, 4, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x3A/0xA, then re-arms 0x3A/0x18/0x5C when the frame counter reports 1. */
extern "C" void em015_act5_30(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 54.0f, 0.0f) == 1) {
            self->state++;
            em_hit_window_clear(self, 0);
            em_mot_set_blend(self, 0x3A, 0x18, 0x5C, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x36/0x1E motion, then finishes. */
extern "C" void em015_act5_38(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x1E, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80179E98 (0x16C): action 5's dispatcher: `state_sub` (0..38) selects the step and its selectors. */
extern "C" void em015_act5_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em015_act5_00(self);
        break;
    case 1:
        em015_act5_swim_turn(self, 0);
        break;
    case 2:
        em015_swim_in(self, 0, 0);
        break;
    case 3:
        em015_act5_03(self);
        break;
    case 4:
        em015_act5_swim_turn_long(self);
        break;
    case 5:
        em015_circle_attack(self, 0, 0);
        break;
    case 6:
        em015_circle_attack(self, 1, 0);
        break;
    case 7:
        em015_circle_attack(self, 0, 1);
        break;
    case 8:
        em015_circle_attack(self, 1, 1);
        break;
    case 9:
        em015_discharge(self);
        break;
    case 10:
        em015_swim_in(self, 1, 0);
        break;
    case 11:
        em015_swim_in(self, 2, 0);
        break;
    case 12:
        em015_leap(self);
        break;
    case 13:
        em015_swim_in(self, 0, 1);
        break;
    case 14:
        em015_swim_in(self, 1, 1);
        break;
    case 15:
        em015_swim_in(self, 2, 1);
        break;
    case 16:
        em015_dive(self);
        break;
    case 17:
        em015_rise(self);
        break;
    case 18:
        em015_rest(self, 0);
        break;
    case 19:
        em015_act5_turn_angle(self);
        break;
    case 20:
        em015_act5_20(self);
        break;
    case 21:
        em015_act5_21(self);
        break;
    case 22:
        em015_circle_attack(self, 0, 2);
        break;
    case 23:
        em015_circle_attack(self, 1, 2);
        break;
    case 24:
        em015_swim_in(self, 3, 0);
        break;
    case 25:
        em015_act5_shake(self);
        break;
    case 26:
        em015_surface(self);
        break;
    case 27:
        em015_act5_27(self);
        break;
    case 28:
        em015_act5_swim_turn(self, 1);
        break;
    case 29:
        em015_act5_29(self);
        break;
    case 30:
        em015_act5_30(self);
        break;
    case 31:
        em015_swim_in(self, 4, 0);
        break;
    case 32:
        em015_swim_in(self, 4, 1);
        break;
    case 33:
        em015_rest(self, 1);
        break;
    case 34:
        em015_circle_attack(self, 0, 3);
        break;
    case 35:
        em015_circle_attack(self, 1, 3);
        break;
    case 36:
        em015_swim_in(self, 5, 0);
        break;
    case 37:
        em015_swim_in(self, 5, 1);
        break;
    case 38:
        em015_act5_38(self);
        break;
    }
}

/* Arms 0x4D/0x14 and plays `em_hit_window_set_default(self, 0, 1)` while the frame counter runs. */
extern "C" void em015_act7_00(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4D, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 36.0f, 0.0f) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017A0BC (0x300): the spin: motion 0x4A with the 0x4B/0x4C layer for `kind` 1..4, the material blend fading
 * in over frames 0..40 and out over 160..230 (half strength for kinds 3/4), the second hit window and the two
 * turn windows, completing with `fn_80128030`. */
extern "C" void em015_spin(_ENEMY_WORK* self, u32 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_mat_blend_reset(self);
        switch ((u8)kind) {
        case 1:
        case 3:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_mot_layer_blend_set(self, 1, 0x4B, 0xE, 0, 1);
            em_hit_window_set(self, 0, 2, 8);
            break;
        case 2:
        case 4:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_mot_layer_blend_set(self, 1, 0x4C, 0xE, 0, 1);
            em_hit_window_set(self, 0, 2, 8);
            break;
        default:
            em_mot_set_blend(self, 0x4A, 0xE, 0, 1);
            em_hit_window_set(self, 0, 2, 8);
            break;
        }
        em_hit_window_set(self, 1, 0x24, 0x18);
        break;
    case 1:
        if ((u32)((u8)kind - 1) <= 3) {
            f32 blend;

            if (em_frame_check(self, 1, 40.0f, 0.0f) == 0) {
                blend = em_mot_frame_get(self) / 40.0f;
            } else if (em_frame_check(self, 1, 160.0f, 0.0f) == 0) {
                blend = 1.0f;
            } else if (em_frame_check(self, 1, 230.0f, 0.0f) == 0) {
                blend = 1.0f - (em_mot_frame_get(self) - 160.0f) / 70.0f;
            } else {
                blend = 0.0f;
            }
            if (blend < 0.0f) {
                blend = 0.0f;
            } else if (blend > 1.0f) {
                blend = 1.0f;
            }
            if ((u32)((u8)kind - 3) <= 1) {
                blend *= 0.5f;
            }
            mhchar_mat_blend_set((MHchar*)self->char_0x024, 0, 1.0f - blend);
            mhchar_mat_blend_set((MHchar*)self->char_0x024, 1, blend);
        }
        if (self->state_0x006 == 0 && self->field_0xA0D == 0) {
            self->state_0x006++;
            em_hit_window_set(self, 0, 0x1F, 0x10);
        }
        em_turn_in_window(self, 0.0f, 70.0f, -0x1555);
        em_turn_in_window(self, 70.0f, 140.0f, 0x9555);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017A3BC (0x63C): the backward lunge: motion 0x46 (0x39 with the turn hold for `kind` 3) on the backward
 * approach with the yaw kept out of the 0x1000..0xF000 back arc, the hit windows, motion 0x47, then the
 * kind's recovery (0x48, 0x49 with the second swing, or 0x4E). */
extern "C" void em015_lunge(_ENEMY_WORK* self, u32 kind) {
    switch (self->state) {
    case 0: {
        u32 motion;

        self->state++;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        switch ((u8)kind) {
        default:
            motion = 0x46;
            em_approach_start(self, -720.0f, 0x12);
            break;
        case 1:
            motion = 0x46;
            em_approach_start(self, -1800.0f, 0x12);
            break;
        case 2:
            motion = 0x46;
            em_approach_start(self, 0.0f, 0x12);
            copyVec3(&self->action_0x328.vec_0x334, &self->pos);
            break;
        case 3:
            motion = 0x39;
            em_approach_start(self, 0.0f, 0x12);
            self->state_0x006 = 0;
            break;
        }
        em_mot_set_blend(self, motion, 0xA, 0, 1);
        break;
    }
    case 1: {
        u32 hold;
        u16 yaw;

        em_approach_step(self, 0, 0x80);
        yaw = self->field_0x1BC;
        if ((u32)(yaw - 0x1001) <= 0x6FFE) {
            self->field_0x1BC = 0x1000;
        } else if ((u32)(yaw - 0x8001) <= 0x6FFE) {
            self->field_0x1BC = 0xF000;
        }
        if ((s32)(u8)kind != 3) {
            hold = 0;
        } else {
            switch (self->state_0x006) {
            case 0:
                em015_part_flag_set(self, 1);
                if (em_frame_check(self, 1, 64.0f, 0.0f) == 1) {
                    em_turn_to_target(self, 0x30);
                }
                if (em_frame_check(self, 0, 116.0f, 0.0f) == 1) {
                    self->state_0x006++;
                    em_mot_set(self, 0x45, 4, 0);
                }
                hold = 1;
                break;
            case 1:
                hold = 0;
                break;
            }
        }
        if (hold != 0) {
            break;
        }
        if ((s32)(u8)kind != 3) {
            if (em_frame_check(self, 0, 42.0f, 0.0f) == 1) {
                em_hit_window_set(self, 0, 3, 0xB);
                em_hit_window_set(self, 1, 0x13, 0x23);
            }
        } else if (em_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 3, 0xB);
            em_hit_window_set(self, 1, 0x13, 0x23);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x47, 0, 0);
        }
        break;
    }
    case 2: {
        u16 yaw;

        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            switch ((u8)kind) {
            default:
                em_mot_set_blend(self, 0x48, 8, 0, 1);
                break;
            case 1:
                em_mot_set_blend(self, 0x4E, 8, 0, 1);
                em_hit_window_set_default(self, 0, 4);
                break;
            case 2:
                em_mot_set_blend(self, 0x49, 8, 0, 1);
                break;
            }
        }
        yaw = self->field_0x1BC;
        if ((u32)(yaw - 0x1001) <= 0x6FFE) {
            self->field_0x1BC = 0x1000;
        } else if ((u32)(yaw - 0x8001) <= 0x6FFE) {
            self->field_0x1BC = 0xF000;
        }
        break;
    }
    case 3:
        switch ((u8)kind) {
        default:
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
                em_hit_window_set(self, 0, 0x27, 0x10);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
            }
            em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
            if (em_mot_end_ck(self) == 1) {
                fn_80128030(self);
            }
            break;
        case 1:
            if (em_mot_end_ck(self) == 1) {
                fn_80128030(self);
            }
            break;
        case 2:
            em_turn_in_window(self, 40.0f, 80.0f, 0x8000);
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
                em_hit_window_set(self, 0, 0x28, 0x10);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
            }
            if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                em_hit_window_set(self, 0, 3, 0xB);
                em_hit_window_set(self, 1, 0x13, 0x23);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state = 5;
                em_mot_set_blend(self, 0x48, 8, 0, 1);
            }
            break;
        }
        break;
    case 4:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set_blend(self, 0x48, 8, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            em_hit_window_set(self, 0, 0x27, 0x10);
        }
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017A9F8 (0x2B0): the charge field: motion 0xCE; `kind` 0 opens and closes the hit window and sparks the area
 * every fifth frame of 120..230, `kind` 1 sparks the area and fires the joint burst on frame 144. */
extern "C" void em015_charge_field(_ENEMY_WORK* self, u32 kind) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xCE, 0x14, 0);
        self->state_0x006 = 0;
        self->timer_0x020 = 0;
        break;
    case 1:
        if ((s32)(u8)kind == 0) {
            switch (self->state_0x006) {
            case 0:
                if (em_frame_check(self, 1, 130.0f, 0.0f) == 1) {
                    self->state_0x006++;
                    em_hit_window_set(self, 0, 5, 3);
                }
                break;
            case 1:
                if (em_frame_check(self, 1, 220.0f, 0.0f) == 1) {
                    self->state_0x006++;
                    em_hit_window_clear(self, 0);
                }
                break;
            }
            if (em_frame_check(self, 0, 260.0f, 0.0f) == 1) {
                em_eft_fx_spawn_anchor(self, 7, 3, 1.0f);
            }
            if (em_frame_check(self, 3, 120.0f, 230.0f) == 1) {
                if (self->timer_0x020 % 5 == 0) {
                    eft_spawn_type_at_area(self, 1);
                }
                eft013_em_set(self, 5);
                self->timer_0x020 = self->timer_0x020 + 1;
            }
        } else {
            if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                eft_spawn_type_at_area(self, 1);
            }
            if (em_frame_check(self, 3, 120.0f, 230.0f) == 1) {
                if (self->timer_0x020 % 5 == 0) {
                    eft_spawn_type_at_area(self, 0);
                }
                self->timer_0x020 = self->timer_0x020 + 1;
            }
            if (em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 150.0f, 0.0f);
                eft_em_spawn_joint(self, 3, 0x15, &vec, 1.0f, 0x35);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017ACA8 (0x4E8): the great discharge: motion 0xD0, the head sparks on frames 80/90/100 placed through joint
 * 3's matrix, the area sparks every fifth frame of 220..400, the shell job on frame 220 and the head effects;
 * the hit window opens on frame 230 and closes on 400. */
extern "C" void em015_great_discharge(_ENEMY_WORK* self, u32 kind) {
    VEC3 vec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set_blend(self, 0xD0, 0x14, 0, 1);
        self->state_0x006 = 0;
        self->timer_0x020 = 0;
        break;
    case 1:
        self->timer_0x020 = self->timer_0x020 + 1;
        if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            eft013_em_set(self, 0);
        }
        if (em_frame_check(self, 3, 220.0f, 400.0f) == 1) {
            if (self->timer_0x020 % 5 == 0) {
                eft_spawn_type_at_area(self, 1);
            }
            eft013_em_set(self, 5);
        }
        switch ((u8)kind) {
        case 0:
            if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                eft013_em_spawn_head(self, 3, 1.0f);
            }
            if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                eft013_em_spawn_head(self, 6, 1.0f);
            }
            if (em_frame_check(self, 0, 80.0f, 0.0f) == 1 || em_frame_check(self, 0, 90.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 210.0f, -30.0f);
                get_joint_wmat_em(self, 3, &mtx);
                mulVecMat(&vec, &mtx);
                vec.x += mtx.m[0][3];
                vec.y += mtx.m[1][3];
                vec.z += mtx.m[2][3];
                eft_spawn_type10(self, 0xA, 3, &vec, 2.0f);
            }
            if (em_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                ((ShellFlagJobFn)shell_set_func_ptr->method_0x50)(self, 1, 0x1E);
            }
            if (em_frame_check(self, 0, 540.0f, 0.0f) == 1) {
                eft013_em_spawn_head(self, 6, 1.0f);
            }
            break;
        case 1:
            if (em_frame_check(self, 0, 80.0f, 0.0f) == 1 || em_frame_check(self, 0, 90.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 210.0f, -30.0f);
                get_joint_wmat_em(self, 3, &mtx);
                mulVecMat(&vec, &mtx);
                vec.x += mtx.m[0][3];
                vec.y += mtx.m[1][3];
                vec.z += mtx.m[2][3];
                if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                    eft_spawn_type10(self, 0xA, 3, &vec, 1.5f);
                } else {
                    eft_spawn_type10(self, 0xA, 3, &vec, 2.0f);
                }
            }
            if (em_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                ((ShellFlagJobFn)shell_set_func_ptr->method_0x50)(self, 0, 0x1E);
            }
            if (em_frame_check(self, 0, 540.0f, 0.0f) == 1) {
                eft013_em_spawn_head(self, 6, 0.6f);
            }
            break;
        }
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 0, 230.0f, 0.0f) == 1) {
                self->state_0x006++;
                em_hit_window_set(self, 0, 0xA, 3);
            }
            break;
        case 1:
            if (em_frame_check(self, 0, 400.0f, 0.0f) == 1) {
                self->state_0x006++;
                em_hit_window_clear(self, 0);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017B190 (0x47C): the tail slam: motion 0x4F with the hit window, the turn on frames 0..100, the slam effects
 * and shell jobs (`kind` 0 the ground burst, 1 the spark run and the eft035 bolt at joint 1), then either the
 * hand-off or (`chain` 1) motion 0x5C into action 7. */
extern "C" void em015_tail_slam(_ENEMY_WORK* self, u32 kind, u32 chain) {
    VEC3 vec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set_blend(self, 0x4F, 0xA, 0, 1);
        if ((s32)(u8)kind != 1) {
            em_hit_window_set_default(self, 0, 6);
        } else {
            em_hit_window_set_default(self, 0, 0x2B);
        }
        break;
    case 1:
        if (em_frame_check(self, 2, 100.0f, 0.0f) == 1) {
            fn_80136D4C(self, 10.0f);
            em_turn_to_target(self, 0x50);
        }
        if ((s32)(u8)kind != 1) {
            if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em_eft_fx_spawn_anchor(self, 5, 0x2B, 1.0f);
            }
            if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                vec.x = -150.0f;
                vec.y = 0.0f;
                vec.z = 150.0f;
                em_eft_fx_spawn_at_sf(self, 1, 1, 1.0f, 3, &vec);
                ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 3, 1, &vec, 1.0f, self->shell_flags_0xA14);
            }
        } else {
            if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                eft013_em_set(self, 7);
            }
            if (em_frame_check(self, 0, 126.0f, 0.0f) == 1 || em_frame_check(self, 0, 132.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 138.0f, 0.0f) == 1 || em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 2, 0x2B, &vec, 1.5f);
            }
            if (em_frame_check(self, 0, 128.0f, 0.0f) == 1 || em_frame_check(self, 0, 134.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 140.0f, 0.0f) == 1 || em_frame_check(self, 0, 146.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x49, 0x2B, &vec, 1.0f);
            }
            if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                setVector3(&vec, -150.0f, 0.0f, 280.0f);
                ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 8, 1, &vec, 1.0f, self->shell_flags_0xA14);
                get_joint_wmat_em(self, 1, &mtx);
                mulVecMatAddTrans(&vec, &mtx);
                eft035_set(self, 1, &vec, (_CP_VECTOR*)&self->field_0x1BC, self->area_no, 2.0f);
            }
        }
        if ((s32)(u8)chain != 1) {
            if (em_mot_end_ck(self) == 1) {
                fn_80128030(self);
            }
        } else {
            switch (self->state_0x006) {
            case 0:
                if (em_frame_check(self, 1, 140.0f, 0.0f) != 1) {
                    break;
                }
                self->state_0x006++;
                em_mot_set(self, 0x5C, 0, 0);
                /* fall through */
            case 1:
                if (em_busy_ck(self) == 1) {
                    if (em015_damage_level(self) == 0) {
                        em_state_set(self, 7, 0x29);
                    } else {
                        em_state_set(self, 7, 0x2C);
                    }
                }
                break;
            }
        }
        break;
    }
}

/* Arms the 0x50/0xA motion and two joint-pair programs keyed on `phase_0x06`/`step_0x07`. */
extern "C" void em015_act7_hit_pair(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x50, 0xA, 0);
        em_hit_window_set(self, 0, 7, 8);
        em_hit_window_set(self, 1, 0x20, 0x18);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, 20.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (self->state_0x006 == 0 && self->field_0xA0D == 0) {
            em_hit_window_set(self, 0, 0x18, 0x18);
        }
        if (self->state_0x007 == 0 && self->field_0xA69 == 0) {
            em_hit_window_set(self, 1, 0x21, 0x18);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8017B748 (0x248): the bite: motion 0x53 on a short approach (`kind` 1 sparks the area through frame 65 and arms
 * the second hit window), the hit windows on frame 78, then motion 0x55 with the follow-up window. */
extern "C" void em015_bite(_ENEMY_WORK* self, u32 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x53, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        em_approach_step(self, 0, 0x40);
        if ((u8)kind == 1) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                eft013_em_set(self, 0);
            }
            if (em_frame_check(self, 1, 65.0f, 0.0f) == 1) {
                if (self->timer_0x020 % 3 == 0) {
                    eft_spawn_type_at_area(self, 5);
                }
                eft013_em_set(self, 5);
                self->timer_0x020 = self->timer_0x020 + 1;
            }
        }
        if (em_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 8, 0xB);
            if ((u8)kind == 1) {
                em_hit_window_set(self, 1, 0x1E, 0x1B);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0x55, 6, 0);
        }
        break;
    case 2:
        if (self->state_0x006 == 0 && em_frame_check(self, 1, 14.0f, 0.0f) == 1) {
            self->state_0x006++;
            em_hit_window_clear(self, 0);
            em_hit_window_clear(self, 1);
            em_hit_window_set(self, 0, 0x26, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x52/4, then two mirrored `em_turn_in_window` poses around a 0x30 fade. */
extern "C" void em015_act7_turn_pair(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x52, 4, 0);
        em_hit_window_set_default(self, 0, 9);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, 0.0f, 60.0f) == 1) {
            em_turn_to_target(self, 0x30);
        }
        em_turn_in_window(self, 60.0f, 100.0f, -0x4000);
        em_turn_in_window(self, 200.0f, 270.0f, 0x4000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x51/0xA and two joint programs, then `em_turn_in_window(self, 0x8000, ...)`. */
extern "C" void em015_act7_turn_back(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x51, 0xA, 0);
        em_hit_window_set(self, 0, 0xE, 8);
        em_hit_window_set(self, 1, 0xF, 0x10);
        break;
    }
    case 1:
        em_turn_in_window(self, 90.0f, 180.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8017BB34 (0x290): the back step: motion 0x3A on the backward approach (`kind` picks the distance), then on the
 * approach or frame 92 either motion 0x4D (`kind` 0) or the action-7 follow-up `kind` names. */
extern "C" void em015_back_step(_ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        switch (kind) {
        case 0:
            em_approach_start(self, -1300.0f, 0x10);
            break;
        case 6:
            em_approach_start(self, -620.0f, 0x10);
            break;
        case 7:
        case 8:
        case 9:
        case 10:
            em_approach_start(self, -800.0f, 0x10);
            break;
        default:
            em_approach_start(self, -820.0f, 0x10);
            break;
        }
        break;
    case 1:
        if (em_frame_check(self, 1, 10.0f, 0.0f) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_frame_check(self, 1, 92.0f, 0.0f) == 1)) {
            switch (kind) {
            case 0:
                self->state++;
                em_mot_set_blend(self, 0x4D, 0x14, 0x32, 1);
                em_hit_window_set_default(self, 0, 1);
                break;
            case 1:
                em_act_set_unless_act11(self, 7, 1);
                break;
            case 2:
                em_act_set_unless_act11(self, 7, 2);
                break;
            case 3:
                em_act_set_unless_act11(self, 7, 3);
                break;
            case 4:
                em_act_set_unless_act11(self, 7, 9);
                break;
            case 5:
                em_act_set_unless_act11(self, 7, 0xA);
                break;
            case 6:
                em_act_set_unless_act11(self, 7, 0x1B);
                break;
            case 7:
                em_act_set_unless_act11(self, 7, 0xB);
                break;
            case 8:
                em_act_set_unless_act11(self, 7, 0x2A);
                break;
            case 9:
                em_act_set_unless_act11(self, 7, 0x27);
                break;
            case 10:
                em_act_set_unless_act11(self, 7, 0x2B);
                break;
            default:
                fn_80128030(self);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017BDC4 (0x474): the thunder ball: motion 0x5A, the rise over frames 10..40 and the drop over 70..130, the
 * part effects on frame 0, the mouth sparks through joint 0x19's matrix, the splash under it, and the shell job
 * for the ball on frame 52. */
extern "C" void em015_thunder_ball(_ENEMY_WORK* self) {
    VEC3 vec;
    EmSpawnRec rec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set(self, 0x5A, 0xA, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 5, 10.0f, 40.0f) == 1) {
            self->pos.y = self->pos.y + 3.3333333f * get_em_base_scale(self) * get_em_chg_scale(self);
        } else if (em_frame_check(self, 5, 70.0f, 130.0f) == 1) {
            self->pos.y = self->pos.y + -5.0f * get_em_base_scale(self) * get_em_chg_scale(self);
        }
        em_surface_clamp(self, 0.0f);
        if (em_frame_check(self, 0, 0.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -55.0f, 75.0f);
            eft007_part_offset_get(self, 8, &vec);
        }
        if (em_frame_check(self, 0, 0.0f, 0.0f) == 1) {
            eft007_part_set(self, 6);
        }
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1 || em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 30.0f);
            get_joint_wmat_em(self, 0x19, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 9, 0x19, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1 && self->pos.y > self->field_0x210 - 700.0f) {
            setVector3(&vec, 0.0f, 0.0f, -200.0f);
            eft009_spawn_at_pos(self, 3, 0x11, 0, 1.3f, &vec);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -55.0f, 100.0f);
            get_joint_wmat_em(self, 0x19, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 0xA, 0x19, &vec, 1.4f);
        }
        if (em_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -55.0f, 100.0f);
            get_joint_wmat_em(self, 0x19, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 0xA, 0x19, &vec, 1.2f);
        }
        if (em_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em015_spawn_rec_set(&rec, 0, 0, 0);
            eft007_part_spawn(self, 5, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 2);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017C238 (0x2CC): the aimed thunder ball: motion 0x57, the part effects on frame 12, the mouth sparks through
 * joint 0x19's matrix, and on frame 98 the shell job for the ball (`kind` 1 angles it by 0x71C). */
extern "C" void em015_thunder_ball_aimed(_ENEMY_WORK* self, u32 kind) {
    VEC3 vec;
    EmSpawnRec rec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x57, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            eft007_part_set(self, 8);
        }
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            eft007_part_set(self, 6);
        }
        if (em_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 30.0f);
            get_joint_wmat_em(self, 0x19, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 9, 0x19, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 68.0f, 0.0f) == 1 || em_frame_check(self, 0, 72.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 110.0f);
            get_joint_wmat_em(self, 0x19, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 0xA, 0x19, &vec, 1.2f);
        }
        if (em_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            s16 angle;

            switch ((u8)kind) {
            case 0:
                angle = 0;
                break;
            case 1:
                angle = 0x71C;
                break;
            }
            em015_spawn_rec_set(&rec, 0, angle, 0);
            eft007_part_spawn(self, 5, angle, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 2);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x4E/0xA and two joint programs, with the fade on the first frame check. */
extern "C" void em015_act7_27(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x10, 8);
        em_hit_window_set(self, 1, 0x11, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            fn_80136D4C(self, 10.0f);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x58/4 motion and `em_hit_window_set_default(self, 0, 0x15)`, then finishes. */
extern "C" void em015_act7_30(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x58, 4, 0);
        em_hit_window_set_default(self, 0, 0x15);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, 34.0f, 70.0f) == 1) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8017C690 (0x288): the rolling charge: motion 0x44 with the two hit windows (`kind` picks the second), the
 * turn window 190..240, then either the frame-70 player-side burst (`kind` 0) or the area sparks with the free
 * effect at joint 3 every third frame of 45..150. */
extern "C" void em015_rolling_charge(_ENEMY_WORK* self, u32 kind) {
    VEC3 pos;
    VEC3 vec;

    VEC3_ctor(&pos);
    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set(self, 0x44, 4, 0);
        em_hit_window_set(self, 0, 0x16, 8);
        if ((s32)(u8)kind == 0) {
            em_hit_window_set(self, 1, 0x17, 0x10);
        } else {
            em_hit_window_set(self, 1, 0x1D, 0x10);
        }
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            fn_80133CC8(self, 0x180, 0x280);
        }
        if (em_frame_check(self, 2, 190.0f, 0.0f) == 1) {
            em_hit_arm_set(self);
        }
        em_turn_in_window(self, 190.0f, 240.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        if ((s32)(u8)kind == 0) {
            if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 200.0f, -1200.0f * get_em_chg_scale(self));
                eft026_pl_spawn_c1(self, 2, &vec, 3, 1.0f);
            }
        } else {
            if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                eft013_em_set(self, 0);
            }
            if (em_frame_check(self, 3, 45.0f, 150.0f) == 1) {
                if (self->timer_0x020 % 3 == 0) {
                    eft_spawn_type_at_area(self, 5);
                    get_joint_wpos_em(self, 3, &vec);
                    em_eft_fx_spawn_free_sf(6, &vec, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no, self->team);
                }
                eft013_em_set(self, 5);
                self->timer_0x020 = self->timer_0x020 + 1;
            }
        }
        break;
    }
}

/* Arms 0x25/6, two `em_hit_window_set_default` cues and the 0x180/0x280 viewport pair. */
extern "C" void em015_act7_spin_turn(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -0xA);
        em_mot_set(self, 0x25, 6, 0);
        em_hit_window_set_default(self, 0, 0x19);
        em_hit_window_set_default(self, 1, 0x1A);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            fn_80133CC8(self, 0x180, 0x280);
        }
        if (em_frame_check(self, 2, 190.0f, 0.0f) == 1) {
            em_hit_arm_set(self);
        }
        em_turn_in_window(self, 160.0f, 230.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x59/6 and one of three 0x... pose constants chosen by the caller's selector. */
extern "C" void em015_act7_approach(_ENEMY_WORK* self, u32 selector) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 6, 0);
        em_hit_window_set(self, 0, 0x1B, 2);
        switch ((u8)selector) {
        default:
            em_approach_start(self, -600.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            break;
        case 2:
            em_approach_start(self, 100.0f, 0);
            break;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_hit_window_clear(self, 0);
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8017CB34 (0x528): the turning slam: motion 0x5B with the hit window, the turn rate ramping up over frames
 * 0..24 and down over 60..70, then the slam effects and shell jobs (`kind` 0 the ground burst, 1 the spark run
 * and the eft035 bolt at joint 1). */
extern "C" void em015_turn_slam(_ENEMY_WORK* self, u32 kind) {
    VEC3 vec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x5B, 0, 0);
        if ((s32)(u8)kind != 1) {
            em_hit_window_set_default(self, 0, 0x2C);
        } else {
            em_hit_window_set_default(self, 0, 0x2D);
        }
        break;
    case 1:
        if (em_frame_check(self, 2, 70.0f, 0.0f) == 1) {
            em015_part_flag_set(self, 1);
            fn_80136D4C(self, 10.0f);
            if (em_frame_check(self, 2, 14.0f, 0.0f) == 1) {
                em_turn_to_target_h(self, 22.857143f * em_mot_frame_get(self));
            } else if (em_frame_check(self, 2, 24.0f, 0.0f) == 1) {
                em_turn_to_target_h(self, 256.0f + 70.4f * (em_mot_frame_get(self) - 14.0f));
            } else if (em_frame_check(self, 2, 60.0f, 0.0f) == 1) {
                em_turn_to_target(self, 0x400);
            } else {
                em_turn_to_target_h(self, 1024.0f - 85.333336f * (em_mot_frame_get(self) - 60.0f));
            }
        }
        switch ((u8)kind) {
        case 0:
            if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                em_eft_fx_spawn_sf(self, 3, 0x19, 1.0f, 0xF);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                em_eft_fx_spawn_anchor(self, 5, 0x2B, 1.0f);
            }
            if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                vec.x = 100.0f;
                vec.y = 0.0f;
                vec.z = 150.0f;
                em_eft_fx_spawn_at_sf(self, 1, 1, 1.0f, 3, &vec);
                ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 3, 1, &vec, 1.0f, self->shell_flags_0xA14);
            }
            break;
        case 1:
            if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                eft013_em_set(self, 7);
            }
            if (em_frame_check(self, 0, 112.0f, 0.0f) == 1 || em_frame_check(self, 0, 118.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 124.0f, 0.0f) == 1 || em_frame_check(self, 0, 130.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 136.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 2, 0x2B, &vec, 1.5f);
            }
            if (em_frame_check(self, 0, 114.0f, 0.0f) == 1 || em_frame_check(self, 0, 120.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 126.0f, 0.0f) == 1 || em_frame_check(self, 0, 132.0f, 0.0f) == 1 ||
                em_frame_check(self, 0, 138.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x49, 0x2B, &vec, 1.0f);
            }
            if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                setVector3(&vec, 150.0f, 0.0f, 280.0f);
                ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 8, 1, &vec, 1.0f, self->shell_flags_0xA14);
                get_joint_wmat_em(self, 1, &mtx);
                mulVecMatAddTrans(&vec, &mtx);
                eft035_set(self, 1, &vec, (_CP_VECTOR*)&self->field_0x1BC, self->area_no, 2.0f);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017D05C (0x260): the thunder burst: motion 0x5D (`kind` 1 drops by 14 through frame 30), the 0x180 turn
 * window over 50..110, the part effects, and the shell job for the burst (mode 0x13) on frame 110. */
extern "C" void em015_thunder_burst(_ENEMY_WORK* self, u32 kind) {
    VEC3 vec;
    EmSpawnRec rec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set(self, 0x5D, 0xA, 0);
        break;
    case 1:
        if ((s32)(u8)kind == 1 && em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            self->pos.y = self->pos.y - 14.0f;
        }
        if (em_frame_check(self, 2, 50.0f, 0.0f) == 1) {
            fn_80133CC8(self, 0x180, 0x180);
        }
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            eft007_part_set(self, 8);
        }
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            eft007_part_set(self, 6);
        }
        if (em_frame_check(self, 0, 60.0f, 0.0f) == 1 || em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            eft007_part_set(self, 0x21);
        }
        if (em_frame_check(self, 0, 72.0f, 0.0f) == 1) {
            eft007_part_set(self, 0x22);
        }
        if (em_frame_check(self, 2, 110.0f, 0.0f) == 1) {
            em_hit_arm_set(self);
        }
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em015_spawn_rec_set(&rec, 0, 0, 0);
            eft007_part_spawn(self, 0x20, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x13);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8017D2BC (0x2E4): the great thunder burst: motion 0x5E, the part offsets through frame 56, the shell job for
 * the burst (mode 0x21) on frame 90, then the hand-off on frame 130 (`chain` 0) or the motion's end. */
extern "C" void em015_thunder_burst_great(_ENEMY_WORK* self, u32 kind, u32 chain) {
    VEC3 vec;
    EmSpawnRec rec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -10);
        em_mot_set(self, 0x5E, 0xA, 0);
        break;
    case 1: {
        u8 done;

        if ((s32)(u8)kind == 1 && em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            self->pos.y = self->pos.y - 14.0f;
        }
        if (em_frame_check(self, 2, 50.0f, 0.0f) == 1) {
            fn_80133CC8(self, 0x180, 0x180);
        }
        if (em_frame_check(self, 2, 90.0f, 0.0f) == 1) {
            em_hit_arm_set(self);
        }
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 85.0f);
            eft007_part_offset_get(self, 8, &vec);
            eft007_part_offset_get(self, 6, &vec);
        }
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1 || em_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            setVector3(&vec, 40.0f, -70.0f, 120.0f);
            eft007_part_offset_get(self, 0x21, &vec);
        }
        if (em_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -65.0f, 120.0f);
            eft007_part_offset_get(self, 0x22, &vec);
        }
        if (em_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em015_spawn_rec_set(&rec, 0, 0, 0);
            eft007_part_spawn(self, 0x20, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x21);
        }
        if ((s32)(u8)chain == 0) {
            if (em_frame_check(self, 0, 130.0f, 0.0f) == 1) {
                done = 1;
            } else {
                done = 0;
            }
        } else {
            done = em_mot_end_ck(self) == 1;
        }
        if (done != 0) {
            fn_80128030(self);
        }
        break;
    }
    }
}

/* 0x8017D5A0 (0x1D8): action 7's dispatcher: `state_sub` (0..0x35) selects the step and its selectors. */
extern "C" void em015_act7_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em015_act7_00(self);
        break;
    case 1:
        em015_spin(self, 0);
        break;
    case 2:
        em015_spin(self, 1);
        break;
    case 3:
        em015_spin(self, 2);
        break;
    case 4:
        em015_lunge(self, 0);
        break;
    case 5:
        em015_lunge(self, 1);
        break;
    case 6:
        em015_charge_field(self, 0);
        break;
    case 7:
        em015_great_discharge(self, 0);
        break;
    case 8:
        em015_lunge(self, 2);
        break;
    case 9:
        em015_spin(self, 3);
        break;
    case 10:
        em015_spin(self, 4);
        break;
    case 11:
        em015_tail_slam(self, 0, 0);
        break;
    case 12:
        em015_act7_hit_pair(self);
        break;
    case 13:
        em015_bite(self, 0);
        break;
    case 14:
        em015_act7_turn_pair(self);
        break;
    case 15:
        em015_great_discharge(self, 1);
        break;
    case 16:
        em015_act7_turn_back(self);
        break;
    case 17:
        em015_back_step(self, 0);
        break;
    case 18:
        em015_back_step(self, 1);
        break;
    case 19:
        em015_back_step(self, 2);
        break;
    case 20:
        em015_back_step(self, 3);
        break;
    case 21:
        em015_back_step(self, 4);
        break;
    case 22:
        em015_back_step(self, 5);
        break;
    case 23:
        em015_charge_field(self, 1);
        break;
    case 24:
        em015_great_discharge(self, 2);
        break;
    case 25:
        em015_thunder_ball(self);
        break;
    case 26:
        em015_thunder_ball_aimed(self, 0);
        break;
    case 27:
        em015_act7_27(self);
        break;
    case 28:
        em015_back_step(self, 6);
        break;
    case 29:
        em015_lunge(self, 3);
        break;
    case 30:
        em015_act7_30(self);
        break;
    case 31:
        em015_rolling_charge(self, 0);
        break;
    case 32:
        em015_act7_spin_turn(self);
        break;
    case 33:
        em015_act7_approach(self, 0);
        break;
    case 34:
        em015_act7_approach(self, 1);
        break;
    case 35:
        em015_thunder_ball_aimed(self, 1);
        break;
    case 36:
        em015_bite(self, 1);
        break;
    case 37:
        em015_rolling_charge(self, 1);
        break;
    case 38:
        em015_back_step(self, 7);
        break;
    case 39:
        em015_tail_slam(self, 0, 1);
        break;
    case 40:
        em015_act7_approach(self, 2);
        break;
    case 41:
        em015_turn_slam(self, 0);
        break;
    case 42:
        em015_tail_slam(self, 1, 0);
        break;
    case 43:
        em015_tail_slam(self, 1, 1);
        break;
    case 44:
        em015_turn_slam(self, 1);
        break;
    case 45:
        em015_back_step(self, 8);
        break;
    case 46:
        em015_back_step(self, 9);
        break;
    case 47:
        em015_back_step(self, 0xA);
        break;
    case 48:
        em015_thunder_burst(self, 0);
        break;
    case 49:
        em015_thunder_burst(self, 1);
        break;
    case 50:
        em015_thunder_burst_great(self, 0, 0);
        break;
    case 51:
        em015_thunder_burst_great(self, 1, 0);
        break;
    case 52:
        em015_thunder_burst_great(self, 0, 1);
        break;
    case 53:
        em015_thunder_burst_great(self, 1, 1);
        break;
    }
}

/* The per-motion sound programs action 10 plays (`EmSeEntry` lists and their records; each is named for the
 * first sub-state that plays it, a GUESS). */
EmSeRecord em015_se_rec_17 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_18 = {120, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_18},
    {0xFF, {0, 0, 0}, 0},
};

EmSePosRec em015_se_pos_1B_a = {52.0f, 112.0f, 0, 16384};

EmSePosRec em015_se_pos_1B_b = {58.0f, 118.0f, -1, -16384};

EmSeRecord em015_se_rec_1B_a = {122, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em015_se_rec_1B_b = {123, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em015_se_rec_1B_c = {124, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_1B[7] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1B_a},
    {0x00, {0, 0, 0}, &em015_se_rec_1B_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em015_se_rec_1B_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1B_b},
    {0x00, {0, 0, 0}, &em015_se_rec_1B_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSePosRec em015_se_pos_1C_a = {22.0f, 62.0f, 0, 16384};

EmSePosRec em015_se_pos_1C_b = {64.0f, 104.0f, 0, 16384};

__declspec(section ".data") EmSeOffset em015_se_ofs_1C_a = {10.0f, 82.0f};

__declspec(section ".data") EmSeOffset em015_se_ofs_1C_b = {64.0f, 114.0f};

EmSeRecord em015_se_rec_1C_a = {125, 0, 6, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em015_se_shift[0]};

EmSeRecord em015_se_rec_1C_b = {126, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em015_se_rec_1C_c = {127, 0, 8, 0, 0, {0, 1, 0, 0}, (const EmSeOffset*)&em015_se_shift[2]};

EmSeEntry em015_se_seq_1C[9] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1C_a},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em015_se_ofs_1C_a},
    {0x00, {0, 0, 0}, &em015_se_rec_1C_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em015_se_rec_1C_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1C_b},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em015_se_ofs_1C_b},
    {0x00, {0, 0, 0}, &em015_se_rec_1C_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_7A = {129, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_7B = {4, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_9F = {5, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_7C = {128, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_7C},
    {0xFE, {0, 0, 0}, 0},
};

EmSePosRec em015_se_pos_7E_a = {52.0f, 112.0f, 0, 16384};

EmSePosRec em015_se_pos_7E_b = {58.0f, 118.0f, -1, -16384};

EmSeRecord em015_se_rec_7E_a = {122, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em015_se_rec_7E_b = {123, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em015_se_rec_7E_c = {124, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_7E[6] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_7E_a},
    {0x00, {0, 0, 0}, &em015_se_rec_7E_a},
    {0x00, {0, 0, 0}, &em015_se_rec_7E_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_7E_b},
    {0x00, {0, 0, 0}, &em015_se_rec_7E_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_7F_a = {125, 0, 6, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em015_se_shift[0]};

EmSeRecord em015_se_rec_7F_b = {126, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em015_se_rec_7F_c = {127, 0, 8, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em015_se_shift[2]};

EmSeEntry em015_se_seq_7F[8] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1C_a},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em015_se_ofs_1C_a},
    {0x00, {0, 0, 0}, &em015_se_rec_7F_a},
    {0x00, {0, 0, 0}, &em015_se_rec_7F_b},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos_1C_b},
    {0x19, {0, 0, 0}, (const EmSeRecord*)&em015_se_ofs_1C_b},
    {0x00, {0, 0, 0}, &em015_se_rec_7F_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_8D = {130, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_78 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_78},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_A8 = {121, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_28_a = {108, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em015_se_rec_28_b = {110, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_28[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x87)},
    {0x00, {0, 0, 0}, &em015_se_rec_28_a},
    {0x00, {0, 0, 0}, &em015_se_rec_28_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_29 = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_29[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_29},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_2B_a = {108, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em015_se_rec_2B_b = {110, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_2B[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xF0)},
    {0x00, {0, 0, 0}, &em015_se_rec_2B_a},
    {0x00, {0, 0, 0}, &em015_se_rec_2B_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_89 = {103, 0, 8, 0, 0, {0, 8, 0, 0}, 0};

EmSeEntry em015_se_seq_89[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xFA)},
    {0x00, {0, 0, 0}, &em015_se_rec_89},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_8A = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_8A[2] = {
    {0x01, {0, 0, 0}, &em015_se_rec_8A},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_8B = {102, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_8B[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_8B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_8C_a = {108, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em015_se_rec_8C_b = {110, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_8C[3] = {
    {0x00, {0, 0, 0}, &em015_se_rec_8C_a},
    {0x00, {0, 0, 0}, &em015_se_rec_8C_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_8F = {104, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_8F[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_8F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_79 = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_79[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_79},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_AA = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_AA[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_AA},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_B1 = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_B1[4] = {
    {0x1A, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC8)},
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em015_se_rec_B1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_B6 = {136, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_B6},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_B7 = {137, 0, 10, 0, 1, {2, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_B7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_B8 = {138, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_B8[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_B8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_BB = {139, 0, 10, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_BB},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_B9 = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em015_se_rec_B9},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_BA = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em015_se_rec_BA},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_BF_a = {121, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em015_se_rec_BF_b = {128, 0, 14, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_BF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x50)},
    {0x00, {0, 0, 0}, &em015_se_rec_BF_a},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em015_se_rec_BF_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em015_se_rec_C0_a = {100, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em015_se_rec_C0_b = {102, 0, 10, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em015_se_seq_C0[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em015_se_rec_C0_a},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em015_se_rec_C0_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSePos em015_se_pos16_C9 = {94.0f, 140.0f, -1, -32768, 0};

EmSeRecord em015_se_rec_C9 = {132, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_C9[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos16_C9},
    {0x00, {0, 0, 0}, &em015_se_rec_C9},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em015_se_pos16_D1 = {150.0f, 230.0f, -1, -32768, 0};

EmSeRecord em015_se_rec_D1 = {105, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_seq_D1[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em015_se_pos16_D1},
    {0x00, {0, 0, 0}, &em015_se_rec_D1},
    {0xFF, {0, 0, 0}, 0},
};
/* 0x8017D778 (0x414): action 10's sound programs: `state_sub` picks the per-motion table, the rest finish. */
extern "C" void em015_act10_se_play(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em015_se_seq_17, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em015_se_seq_18, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em015_se_seq_18, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em015_se_seq_18, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em015_se_seq_1B, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em015_se_seq_1C, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em015_se_seq_17, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em015_se_seq_18, 0, 0x1E);
        break;
    case 0x28:
        em_se_tbl_play(self, em015_se_seq_28, 2, 0x28);
        break;
    case 0x29:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x29);
        break;
    case 0x2A:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x2A);
        break;
    case 0x2B:
        em_se_tbl_play(self, em015_se_seq_2B, 2, 0x2B);
        break;
    case 0x2C:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x2C);
        break;
    case 0x2D:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x2D);
        break;
    case 0x2E:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x2E);
        break;
    case 0x2F:
        em_se_tbl_play(self, em015_se_seq_29, 2, 0x2F);
        break;
    case 0x7A:
        em_se_tbl_play(self, em015_se_seq_7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em015_se_seq_7B, 0, 0x7B);
        break;
    case 0x7C:
        em_se_tbl_play(self, em015_se_seq_7C, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(self, em015_se_seq_8D, 0, 0x8D);
        break;
    case 0x7E:
        em_se_tbl_play(self, em015_se_seq_7E, 0, 0x7E);
        break;
    case 0x7F:
        em_se_tbl_play(self, em015_se_seq_7F, 0, 0x7F);
        break;
    case 0x8E:
        em_se_tbl_play(self, em015_se_seq_8D, 0, 0x8E);
        break;
    case 0x78:
        em_se_tbl_play(self, em015_se_seq_78, 0, 0x78);
        break;
    case 0x89:
        em_se_tbl_play(self, em015_se_seq_89, 2, 0x89);
        break;
    case 0x8A:
        em_se_tbl_play(self, em015_se_seq_8A, 2, 0x8A);
        break;
    case 0x8B:
        em_se_tbl_play(self, em015_se_seq_8B, 2, 0x8B);
        break;
    case 0x8F:
        em_se_tbl_play(self, em015_se_seq_8F, 2, 0x8F);
        break;
    case 0x8C:
        em_se_tbl_play(self, em015_se_seq_8C, 2, 0x8C);
        break;
    case 0x79:
        em_se_tbl_play(self, em015_se_seq_79, 2, 0x79);
        break;
    case 0x9F:
        em_se_tbl_play(self, em015_se_seq_9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em015_se_seq_9F, 0, 0xA0);
        break;
    case 0xA1:
        em_se_tbl_play(self, em015_se_seq_8F, 2, 0xA1);
        break;
    case 0xA2:
        em_se_tbl_play(self, em015_se_seq_8F, 2, 0xA2);
        break;
    case 0xA8:
        em_se_tbl_play(self, em015_se_seq_A8, 0, 0xA8);
        break;
    case 0xAA:
        em_se_tbl_play(self, em015_se_seq_AA, 2, 0xAA);
        break;
    case 0xB1:
        em_se_tbl_play(self, em015_se_seq_B1, 2, 0xB1);
        break;
    case 0xB6:
        em_se_tbl_play(self, em015_se_seq_B6, 0, 0xB6);
        break;
    case 0xB7:
        em_se_tbl_play(self, em015_se_seq_B7, 0, 0xB7);
        break;
    case 0xB8:
        em_se_tbl_play(self, em015_se_seq_B8, 0, 0xB8);
        break;
    case 0xB9:
        em_se_tbl_play(self, em015_se_seq_B9, 0, 0xB9);
        break;
    case 0xBA:
        em_se_tbl_play(self, em015_se_seq_BA, 0, 0xBA);
        break;
    case 0xBB:
        em_se_tbl_play(self, em015_se_seq_BB, 0, 0xBB);
        break;
    case 0xBC:
        em_se_tbl_play(self, em015_se_seq_BB, 0, 0xBC);
        break;
    case 0xBF:
        em_se_tbl_play(self, em015_se_seq_BF, 0, 0xBF);
        break;
    case 0xC1:
        em_se_tbl_play(self, em015_se_seq_8D, 0, 0xC1);
        break;
    case 0xC0:
        em_se_tbl_play(self, em015_se_seq_C0, 2, 0xC0);
        break;
    case 0xC2:
        em_se_tbl_play(self, em015_se_seq_8F, 2, 0xC2);
        break;
    case 0xC9:
        em_se_tbl_play(self, em015_se_seq_C9, 0, 0xC9);
        break;
    case 0xD1:
        em_se_tbl_play(self, em015_se_seq_D1, 2, 0xD1);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* The action-11 sound programs (named for their sub-state, a GUESS). */
EmSeRecord em015_alt_se_rec_00 = {131, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_00[2] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_00},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_alt_se_rec_0A_a = {111, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em015_alt_se_rec_0A_b = {112, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_0A[4] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_0A_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em015_alt_se_rec_0A_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_alt_se_rec_0F = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_0F[3] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_0F},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_alt_se_rec_1A_a = {129, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em015_alt_se_rec_1A_b = {4, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_1A_a},
    {0x00, {0, 0, 0}, &em015_alt_se_rec_1A_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_alt_se_rec_1B = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_1B[2] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_1B},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em015_alt_se_rec_1C = {139, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em015_se_alt_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em015_alt_se_rec_1C},
    {0xFF, {0, 0, 0}, 0},
};
/* Picks the program table by `state_sub`. */
extern "C" void em015_act11_se_play(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em015_se_alt_seq_00, 0, 0);
        break;
    case 10:
        em_se_tbl_play_alt(self, em015_se_alt_seq_0A, 2, 0xA);
        break;
    case 15:
        em_se_tbl_play_alt(self, em015_se_alt_seq_0F, 0, 0xF);
        break;
    case 26:
        em_se_tbl_play_alt(self, em015_se_alt_seq_1A, 0, 0x1A);
        break;
    case 27:
        em_se_tbl_play_alt(self, em015_se_alt_seq_1B, 2, 0x1B);
        break;
    case 28:
        em_se_tbl_play_alt(self, em015_se_alt_seq_1C, 0, 0x1C);
        break;
    default:
        em_se_tbl_play_alt(self, em015_se_alt_seq_00, 0, 0);
        break;
    }
}

/* Resets the action, then re-arms the 0x37/0x14 motion with both selectors 0. */
extern "C" void em015_act12_swim_in(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em015_swim_in(self, 0, 0);
}

/* Runs `em015_act12_swim_in` while the sub-state is 0. */
extern "C" void em015_act12_dispatch(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em015_act12_swim_in(self);
    }
}

/* The table variant: arm the 0x80178378 action table, then switch to 0xD/1. */
extern "C" void em015_act13_swim_turn(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em015_turn_seq_swim, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, (void*)em015_turn_seq_swim) == 1) {
            em_state_set(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, 5.0f);
        } else {
            fn_80136D4C(self, 10.0f);
        }
        break;
    }
}

/* Three-phase: 0x37/0x14, then 0x2F/0x28 once `em_approach_step` and `fn_8012F948` agree. */
extern "C" void em015_act13_swim_in(_ENEMY_WORK* self) {
    em015_part_flag_set(self, 1);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, -400.0f, 0x12);
        break;
    }
    case 1: {
        u32 done = em_approach_step(self, 0, 0x80);

        em_mot_speed_set(self, 1.3f);
        fn_80136D4C(self, 2.0f);
        if (done == 1 && fn_8012F948(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* Arms 0x4E/0xA and two joint programs; the 0xD/3 switch needs `em015_ally_front_ck`. */
extern "C" void em015_act13_hit_pair(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x29, 8);
        em_hit_window_set(self, 1, 0x2A, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            fn_80136D4C(self, 10.0f);
            em_turn_to_target(self, 0x80);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (em015_ally_front_ck(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
            } else {
                fn_80128030(self);
            }
        }
        break;
    }
}

/* 0x8017DF9C (0x1DC): the reposition: the turn sequence toward the target with an offset step that closes the
 * distance to 900 (clamped to +-50 a frame, 50 frames), then action 13's sub-state 4 or action 5. */
extern "C" void em015_reposition(_ENEMY_WORK* self) {
    VEC3 dir;
    VEC3 unused;

    VEC3_ctor(&dir);
    VEC3_ctor(&unused);
    switch (self->state) {
    case 0: {
        u16 angle;
        f32 dist;
        f32 step;

        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em015_turn_seq_swim, 0, 1, 0);
        {
            VEC3 diff;
            subVec3(&diff, &self->vec_0x36C, &self->pos);
            copyVec3(&dir, &diff);
        }
        angle = calcVecAng3(&dir);
        dist = vec3_len((f32*)&dir);
        step = dist - 900.0f * get_em_chg_scale(self);
        step /= 50.0f;
        self->timer_0x020 = 50;
        if (step > 50.0f) {
            step = 50.0f;
        } else if (step < -50.0f) {
            step = -50.0f;
        }
        setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, step);
        rotVecY(&self->offset_0x30C.vec_0x310, angle);
        break;
    }
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
            em_move_offset_apply(self);
        }
        if (em_turn_seq_step(self, (void*)em015_turn_seq_swim) == 1) {
            if (em_busy_ck(self) == 1) {
                if (em015_ally_front_ck(self, 1) == 1) {
                    em_state_set(self, 0xD, 4);
                } else {
                    em_state_set(self, 5, 0);
                }
            }
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, 5.0f);
        } else {
            fn_80136D4C(self, 10.0f);
        }
        break;
    }
}

/* Three-phase: 0x31/0x14 plus the `em_drop_item_spawn` pose request, then 0x21/0. */
extern "C" void em015_act13_drop_item(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        em_drop_item_spawn(self, (u32)&self->pos, 0x32);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 5);
        }
        break;
    }
}

/* Arms 0x21/4 with the 0x3E8 timer, then finishes. */
extern "C" void em015_act13_05(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x21, 4, 0, 1);
        fn_80130CDC(self, 0x3E8);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The key path the dive-in demo replays through `em_demo_key3_apply`: {frame, x, y, z} rows ending at frame -1
 * (GUESS layout from the values). */
f32 em015_demo_keys[88] = {
    636.0f, -410.0f, -305.0f, -360.0f,
    644.0f, -410.0f, -305.0f, -360.0f,
    652.0f, -405.82f, -274.72f, -382.72f,
    656.0f, -401.12f, -262.09f, -391.44f,
    660.0f, -395.23f, -251.34f, -398.1f,
    664.0f, -388.53f, -242.14f, -402.98f,
    666.0f, -384.96f, -237.99f, -404.85f,
    670.0f, -377.52f, -230.36f, -407.62f,
    674.0f, -369.81f, -223.35f, -409.28f,
    678.0f, -361.97f, -216.65f, -409.97f,
    682.0f, -354.1f, -210.02f, -409.74f,
    686.0f, -346.31f, -203.2f, -408.58f,
    690.0f, -338.72f, -195.92f, -406.38f,
    694.0f, -331.47f, -187.86f, -402.98f,
    698.0f, -324.77f, -178.66f, -398.1f,
    702.0f, -318.88f, -167.91f, -391.44f,
    706.0f, -314.18f, -155.28f, -382.72f,
    708.0f, -312.4f, -148.24f, -377.59f,
    710.0f, -311.09f, -140.77f, -372.01f,
    712.0f, -310.27f, -132.97f, -366.1f,
    714.0f, -310.0f, -125.0f, -360.0f,
    -1.0f, -310.0f, -125.0f, -360.0f,
};

/* 0x8017E2D4 (0x234): the dive-in demo: demo-timed motions 0x14, 0x25 (twice, with the paired effects and the
 * demo pins), then the key path from frame 0x27C and motion 0x28 at frame 0x300. */
extern "C" void em015_demo_dive_in(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x21C) == 1) {
            self->state++;
            em_mot_set(self, 0x25, 0, 0x38);
            em_hit_window_set_default(self, 0, 0x35);
            em_demo_pos_set(self, -101.5f, -3203.5f, -1126.0f);
            em_demo_rot_set(self, -55.0f, 55.0f, 0.0f);
            em_demo_enable(self);
            em_eft_fx_spawn_pair_sf(self, 2, 0x2B, 0x16, 1.2f, 0x30);
        }
        break;
    case 2:
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x27C) == 1) {
            self->state++;
            em_demo_pos_set(self, 1548.0f, -800.0f, 133.0f);
            em_demo_rot_set(self, 50.0f, 55.0f, 0.0f);
            self->field_0x1C4 = 0;
            em_mot_set(self, 0x25, 0, 0x98);
            em_eft_fx_spawn_pair_sf(self, 2, 0x2B, 0x16, 1.2f, 0x18);
        }
        break;
    case 3:
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x27C) == 1) {
            em_demo_key3_apply(self, em_demo_frame_get(), 0, em015_demo_keys);
        }
        if (em_demo_time_ck(0x300) == 1) {
            self->state++;
            em_demo_pos_set(self, 1696.0f, -1320.5f, 150.8f);
            em_demo_rot_set(self, 0.0f, -124.9f, 0.0f);
            em_mot_set(self, 0x28, 0, 0x3A);
        }
        break;
    case 4:
        em_hit_arm_set(self);
        break;
    }
}

/* 0x8017E508 (0x440): the wake demo: demo-timed motions 0x14, 4 (the joint-0x19 effect on frame 240), 5 and 7 at
 * reduced speed, 0x18 with the turn window, then motion 0x14 with the charge sparks of frames 180..226. */
extern "C" void em015_demo_wake(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
        em_demo_rot_set(self, 0.0f, 196.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x78) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 240.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 180.0f, 60.0f);
            eft013_em_place(self, &vec, 0x19, 2.5f, 0xA);
        }
        if (em_demo_time_ck(0x296) == 1) {
            self->state++;
            em_mot_set(self, 5, 6, 0);
            em_mot_speed_set(self, 0.44f);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
        }
        break;
    case 3:
        if (em_demo_time_ck(0x306) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0xA);
            em_mot_speed_set(self, 0.52f);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
            em_demo_rot_set(self, 0.0f, 178.0f, 0.0f);
        }
        break;
    case 4:
        if (em_demo_time_ck(0x34A) == 1) {
            self->state++;
            em_mot_set(self, 7, 2, 0x2E);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
            em_demo_rot_set(self, 0.0f, 178.0f, 0.0f);
        }
        break;
    case 5:
        if (em_demo_time_ck(0x458) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
        }
        break;
    case 6:
        em_turn_in_window(self, 0.0f, 40.0f, 0x4000);
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0xC, 0x54);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
            em_demo_rot_set(self, 0.0f, 268.0f, 0.0f);
        }
        break;
    case 7:
        if (em_demo_time_ck(0x4BC) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0x14, 0, 0);
            em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
            self->timer_0x020 = 0;
        }
        break;
    case 8:
        if (self->state_0x006 == 0) {
            if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                draw_shape_arm(self, 0x1A, 0xA);
                setVector3(&vec, 0.0f, -50.0f, 100.0f);
                eft_em_spawn(self, 0, 0x19, &vec, 1.0f);
            }
            if (em_frame_check(self, 3, 186.0f, 226.0f) == 1) {
                if ((self->timer_0x020 & 7) == 0) {
                    setVector3(&vec, 0.0f, -50.0f, 100.0f);
                    eft_em_spawn(self, 1, 0x19, &vec, 1.0f);
                }
                self->timer_0x020 = self->timer_0x020 + 1;
            }
            if (em_mot_end_ck(self) == 1) {
                self->state_0x006++;
            }
        }
        break;
    }
}

/* 0x8017E948 (0x4A8): the arena demo: demo-timed motions 0x2B, 0x2D (sped up over frames 60..200 with the turn
 * window), 0x4F three times at reduced speed with the slam effects and the stage event, then motion 0x28. */
extern "C" void em015_demo_arena(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2B, 0, 0);
        em_demo_pos_set(self, -4460.0f, -680.0f, -6400.0f);
        em_demo_rot_set(self, 0.0f, 330.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x98) == 1) {
            self->state++;
            em_demo_pos_set(self, -4030.0f, -680.0f, -6880.0f);
            em_demo_rot_set(self, 0.0f, 330.0f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(0x1C4) == 1) {
            self->state++;
            em_mot_set(self, 0x2D, 0, 0);
            em_demo_pos_set(self, -4850.0f, -680.0f, -7360.0f);
            em_demo_rot_set(self, 0.0f, 240.0f, 0.0f);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 60.0f, 200.0f) == 1) {
            em_mot_speed_set(self, 1.382f);
        } else {
            em_mot_speed_set(self, 1.0f);
        }
        em_turn_in_window(self, 30.0f, 96.0f, 0x8000);
        if (em_demo_time_ck(0x2EE) == 1) {
            self->state++;
            em_mot_set(self, 0x4F, 0, 0);
            em_demo_pos_set(self, -4650.0f, -480.0f, -7470.0f);
            em_demo_rot_set(self, 0.0f, 420.0f, 0.0f);
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor(self, 5, 0x2B, 1.0f);
        }
        if (em_demo_time_ck(0x376) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0x4F, 0, 0x74);
            em_mot_speed_set(self, 0.708f);
            em_frame_flag_set(self);
            em_demo_pos_set(self, -5003.2f, -480.0f, -7635.8f);
            em_demo_rot_set(self, 0.0f, 420.0f, 0.0f);
        }
        break;
    case 5:
        em_frame_flag_set(self);
        if (self->state_0x006 == 0 && em_demo_time_ck(0x394) == 1) {
            self->state_0x006++;
            stage_demo_event_set();
        }
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor(self, 5, 0x2B, 1.0f);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            vec.x = -150.0f;
            vec.y = 0.0f;
            vec.z = 150.0f;
            em_eft_fx_spawn_at_sf(self, 1, 1, 1.0f, 3, &vec);
        }
        if (em_frame_check(self, 0, 156.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x4F, 2, 0x9E);
            em_mot_speed_set(self, 0.57f);
        }
        break;
    case 6:
        em_frame_flag_set(self);
        if (em_demo_time_ck(0x3B6) == 1) {
            self->state++;
            em_mot_set(self, 0x4F, 0, 0x9E);
            em_demo_pos_set(self, -4213.0f, -480.0f, -6928.0f);
            em_demo_rot_set(self, 0.0f, 420.0f, 0.0f);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 248.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x4F, 2, 0xFA);
            em_mot_speed_set(self, 0.75f);
        }
        break;
    case 8:
        if (em_demo_time_ck(0x44E) == 1) {
            self->state++;
            em_mot_set(self, 0x28, 0, 0);
            em_demo_pos_set(self, -4150.0f, -480.0f, -7130.0f);
            em_demo_rot_set(self, 0.0f, 427.0f, 0.0f);
        }
        break;
    }
}

/* Arms 0x28/0 with the two `em_demo_pos_set`/`em_demo_rot_set` pose pairs. */
extern "C" void em015_act13_demo_a(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_demo_pos_set(self, 1969.5f, -1320.5f, 150.8f);
        em_demo_rot_set(self, 0.0f, 124.8f, 0.0f);
        em_mot_set(self, 0x28, 0, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x14/0, then the two pose pairs in the other order, ending on `em_action_finish`. */
extern "C" void em015_act13_demo_b(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, 1278.0f, 0.0f, 890.0f);
        em_demo_rot_set(self, 0.0f, 268.0f, 0.0f);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x28/0 and the stage-side `fn_802B1FEC`, then the two pose pairs. */
extern "C" void em015_act13_demo_c(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        fn_802B1FEC();
        em_demo_pos_set(self, -4150.0f, -480.0f, -7130.0f);
        em_demo_rot_set(self, 0.0f, 427.0f, 0.0f);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x79/4, then finishes on `em_action_finish`. */
extern "C" void em015_act13_12(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x79, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x64/4, then finishes. */
extern "C" void em015_act13_13(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x64, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The second action-table dispatcher: `state_sub` (0..13) selects a writer, and nothing runs for a value outside
 * the table (the target's `bgtlr`). */
extern "C" void em015_act13_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em015_act13_swim_turn(self);
        break;
    case 1:
        em015_act13_swim_in(self);
        break;
    case 2:
        em015_act13_hit_pair(self);
        break;
    case 3:
        em015_reposition(self);
        break;
    case 4:
        em015_act13_drop_item(self);
        break;
    case 5:
        em015_act13_05(self);
        break;
    case 6:
        em015_demo_dive_in(self);
        break;
    case 7:
        em015_demo_wake(self);
        break;
    case 8:
        em015_demo_arena(self);
        break;
    case 9:
        em015_act13_demo_a(self);
        break;
    case 10:
        em015_act13_demo_b(self);
        break;
    case 11:
        em015_act13_demo_c(self);
        break;
    case 12:
        em015_act13_12(self);
        break;
    case 13:
        em015_act13_13(self);
        break;
    }
}

/* The master selector on `action_0x1E5`: actions 0, 1, 2, 5, 7 and 10..13 go to their dispatchers, the rest
 * run `em_action_finish`; the trailing pair ends the frame. */
extern "C" void em015_action_run(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em015_act0_dispatch(self);
        break;
    case 1:
        em015_act1_dispatch(self);
        break;
    case 2:
        em015_act2_dispatch(self);
        break;
    case 5:
        em015_act5_dispatch(self);
        break;
    case 7:
        em015_act7_dispatch(self);
        break;
    case 10:
        em015_act10_se_play(self);
        break;
    case 11:
        em015_act11_se_play(self);
        break;
    case 12:
        em015_act12_dispatch(self);
        break;
    case 13:
        em015_act13_dispatch(self);
        break;
    default:
        em_action_finish(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* 0x8017F1F0 (0x1D8): the charged-mode mouth sparks: outside the charge and spin motions, every 32nd frame it sparks
 * at joint 0x1A (the underwater pair while submerged), picking the head-up or head-down effect by the pitch. */
extern "C" void em015_mouth_sparks(_ENEMY_WORK* self) {
    VEC3 vec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    if (em_alt_mode_ck(self) == 1) {
        u16 pitch;

        switch (em_get_mot_no(self)) {
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4E:
        case 0x4F:
        case 0x35:
        case 0xCE:
        case 0xD0:
            return;
        }
        pitch = calcVecAngX_s32(&self->vec_0x76C);
        setVector3(&vec, 0.0f, -35.0f, 85.0f);
        get_joint_wmat_em(self, 0x1A, &mtx);
        mulVecMat(&vec, &mtx);
        vec.x += mtx.m[0][3];
        vec.y += mtx.m[1][3];
        vec.z += mtx.m[2][3];
        if (self->field_0x1E2 == 2 && vec.y < self->field_0x210) {
            if ((system_w.field_0x0c & 0x1F) == 0) {
                if ((u16)(pitch + 0x8000) > 0x6E38) {
                    eft_spawn_type10(self, 3, 0x19, &vec, 1.0f);
                } else {
                    eft_spawn_type10(self, 5, 0x19, &vec, 1.0f);
                }
            }
        } else if ((system_w.field_0x0c & 0x1F) == 0) {
            if ((u16)(pitch + 0x8000) > 0x6E38) {
                eft_spawn_type10(self, 0xA, 0x19, &vec, 1.0f);
            } else {
                eft_spawn_type10(self, 9, 0x19, &vec, 1.0f);
            }
        }
    }
}

/* 0x8017F3C8 (0x28C0): the per-motion frame-window driver: the mouth sparks, then each `em_after_frame_check`
 * window of the current motion that fires spawns its effects (`field_0x228 & 6` picks the submerged variant),
 * the shell jobs and the camera shakes. */
extern "C" void em015_motion_events(_ENEMY_WORK* self) {
    VEC3 from;
    VEC3 to;
    VEC3 vec;
    VEC3 sum;
    u8 ground;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    VEC3_ctor(&vec);
    em015_mouth_sparks(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (((self->field_0x228 & 6) != 0) && ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1))) {
            eft009_spawn_at_joint(self, 0x13, 0xE, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em_camera_req(self, 6, 7);
        }
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            em_camera_req(self, 0xC, 7);
        }
        break;
    case 0x9:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x13, 0xE, 0, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 8, 0xC, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0xF, 0, 1.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 6, 0, 1.6f);
            }
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.2f);
            }
        }
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em_camera_req(self, 6, 7);
        }
        break;
    case 0x15:
        if (((self->field_0x228 & 6) != 0) && ((em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1))) {
            eft009_spawn_at_joint(self, 0x13, 0xE, 0, 2.0f);
        }
        if ((em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1)) {
            em_camera_req(self, 0xC, 7);
        }
        if ((em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1)) {
            em_camera_req(self, 6, 7);
        }
        break;
    case 0x16:
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 0x13, 0xE, 0, 2.0f);
        }
        break;
    case 0x18:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0xC, 0, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0xE, 0xC, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0xF, 0, 0.7f);
            }
        } else {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0xE, 2, 0, 1.2f);
            }
        }
        break;
    case 0x19:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 0xC, 0, 1.3f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 8, 0xC, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0xF, 0, 1.3f);
            }
        } else {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 2, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 8, 2, 0, 1.2f);
            }
        }
        break;
    case 0x1B:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 3, 0xC, 0, 2.0f);
                eft009_spawn_at_joint(self, 3, 0xE, 0, 2.0f);
            }
        } else if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 3, 8, 0, 3.5f);
        }
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1)) {
            em_camera_req(self, 2, 7);
        }
        break;
    case 0x21:
        if ((em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 132.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 200.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x22:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 1.2f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 0.8f, 0x3C);
        }
        if ((em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 198.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 238.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 0.7f, 0x3C);
        }
        break;
    case 0x25:
        if ((em_act_ck(self, 0xD, 6) == 0) && (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1)) {
            em_eft_fx_spawn_sf(self, 2, 0x17, 1.0f, 0x18);
        }
        break;
    case 0x2D:
        if (em_after_frame_check(self, 0, 104.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor(self, 0xB, 3, 1.0f);
        }
        if (em_after_frame_check(self, 0, 224.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 120.0f);
            eft_em_spawn_joint(self, 0x1A, 5, &vec, 1.2f, 0x3C);
        }
        break;
    case 0x31:
        if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, -20.0f, 100.0f);
            eft_em_spawn_joint(self, 0x19, 5, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x33:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0xC);
        }
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&vec, 120.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 8, 3, &vec, 0.6f, 0x10);
            setVector3(&vec, 140.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 8, 5, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x34:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0xD);
        }
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&vec, -120.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0xE, 3, &vec, 0.6f, 0x10);
            setVector3(&vec, -140.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0xE, 5, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x39:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_eft_fx_spawn_sf(self, 0, 0x2B, 1.0f, 5);
            vec.x = 0.0f;
            vec.y = -150.0f;
            vec.z = 150.0f;
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 3, 1, &vec, 1.0f, 0xFFFF);
        }
        break;
    case 0x3B:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 3, 0x14, 0, 1.5f);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft_spawn_pos_in_area(&self->pos, self->area_no, 4, 0, 0.8f);
        }
        break;
    case 0x3D:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 200.0f);
                eft009_spawn_at_pos(self, 3, 0x11, 0, 1.3f, &vec);
            }
        } else if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            rotVecY(&vec, self->field_0x1C0);
            addVec3To(&vec, &self->pos);
            eft_spawn_pos_in_area(&self->pos, self->area_no, 4, 0, 1.0f);
        }
        break;
    case 0x3E:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 0x2B, &vec);
                eft_spawn_type11(self, &vec, 2, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 0x2B, &vec);
            vec.y = self->field_0x20C;
            eft_spawn_pos_in_area(&vec, self->area_no, 4, 0, get_em_chg_scale(self));
        }
        break;
    case 0x43:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft013_em_set(self, 0);
        }
        break;
    case 0x45:
        if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_eft_fx_spawn_sf(self, 2, 0x17, 1.2f, 0x18);
        }
        break;
    case 0x46:
        if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair_sf(self, 2, 0x2B, 0x17, 1.2f, 0x18);
        }
        break;
    case 0x47:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair_sf(self, 2, 0x2B, 0x17, 1.2f, 0x18);
        }
        break;
    case 0x49:
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair_sf(self, 2, 0x2B, 0x17, 1.2f, 0x18);
        }
        break;
    case 0x4A:
    case 0x4B:
    case 0x4C:
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x1A, 4, &vec, 1.0f, 0x1A);
            eft_em_spawn_joint(self, 0x1A, 6, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 162.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x30, 4, &vec, 1.0f, 0x1A);
            eft_em_spawn_joint(self, 0x30, 6, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            em_camera_req(self, 0x17, 7);
        }
        break;
    case 0x4D:
        if ((self->flags_0x836 & 1) == 0) {
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                em_eft_fx_spawn_anchor(self, 8, 0x31, 1.0f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
                em_eft_fx_spawn_anchor(self, 9, 0x32, 1.0f);
            }
            if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 886.0f * get_em_chg_scale(self));
                eft026_pl_spawn_c1(self, 1, &vec, 3, get_em_chg_scale(self));
                setVector3(&vec, 0.0f, 0.0f, 700.0f * get_em_chg_scale(self));
                rotVecY(&vec, self->field_0x1C0);
                addVec3(&sum, &self->pos, &vec);
                copyVec3(&from, copyVec3(&to, &sum));
                from.y -= 500.0f * get_em_chg_scale(self);
                to.y += 500.0f * get_em_chg_scale(self);
                ((ShellJobPairFn)shell_set_func_ptr->method_0x30)(self, 4, &from, &to, 1.0f, 0xFFFF);
            }
        }
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2C, 1);
        }
        break;
    case 0x4E:
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x1A, 3, &vec, 1.0f, 0x10);
            eft_em_spawn_joint(self, 0x1A, 5, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x4F:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor(self, 4, 0x19, 1.0f);
        }
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em_eft_fx_spawn_sf(self, 3, 0x19, 1.0f, 0x1A);
        }
        if (em_after_frame_check(self, 0, 132.0f, 0.0f) == 1) {
            em_camera_req(self, 2, 7);
        }
        break;
    case 0x50:
        if ((em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 0xE, 0xC, 0, 1.0f);
                eft009_spawn_at_joint(self, 0xE, 0xF, 0, 1.0f);
            } else {
                eft009_spawn_at_joint(self, 0xE, 6, 0, 1.2f);
            }
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 8, 0xC, 0, 1.0f);
                eft009_spawn_at_joint(self, 8, 0xF, 0, 1.0f);
            } else {
                eft009_spawn_at_joint(self, 8, 6, 0, 1.2f);
            }
        }
        break;
    case 0x51:
        get_joint_wpos_em(self, 0x1A, &from);
        GetGroundHit2(&from, 6, self->area_no, &ground);
        if (ground == 1) {
            if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 0x13, 0x3000, 2.0f);
            }
            if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 0x13, 0x4000, 2.3f);
                eft009_spawn_at_joint(self, 0xE, 0xC, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 0x13, 0x7000, 2.6f);
            }
        } else {
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 6, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 7, 0x3000, 0.6f);
            }
            if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 7, 0x4000, 0.8f);
            }
            if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x1A, 7, 0x7000, 1.0f);
            }
        }
        break;
    case 0x52:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 142.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x10, 0x12, 0, 1.7f);
            }
            if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x22, 0x12, 0, 1.4f);
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x2F, 0x12, 0, 1.2f);
            }
        } else {
            if (em_after_frame_check(self, 0, 142.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 0x10, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 6, 0, 2.3f * get_em_chg_scale(self));
            }
            if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 0x22, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 6, 0, 2.0f * get_em_chg_scale(self));
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 0x2F, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 6, 0, 1.7f * get_em_chg_scale(self));
            }
        }
        break;
    case 0x53:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x1A, 0xE, 0, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x1A, 0xC, 0, 2.0f);
            }
        } else if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 0x1A, 8, 0, 2.5f);
        }
        break;
    case 0x54:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x1A, 0xE, 0, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x1A, 0xC, 0, 2.0f);
            }
        } else if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1)) {
            eft009_spawn_at_joint(self, 0x1A, 8, 0, 2.5f);
        }
        break;
    case 0x55:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1)) {
                setVector3(&vec, 0.0f, 0.0f, 20.0f);
                eft009_spawn_at_pos(self, 0x13, 0xC, 0, 2.5f, &vec);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x13, 0x11, 0, 1.0f);
            }
        } else {
            if ((em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1)) {
                setVector3(&vec, 0.0f, 0.0f, 20.0f);
                eft009_spawn_at_pos(self, 0x13, 8, 0, 2.0f, &vec);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x13, 8, 0, 2.5f);
            }
        }
        break;
    case 0x58:
        if (em_after_frame_check(self, 0, 118.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 0x10, 0x11, 0, 1.7f);
            } else {
                get_joint_wpos_em(self, 0x10, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 4, 0, 1.2f * get_em_chg_scale(self));
            }
            camera_em_shake_req(self, 2);
        }
        break;
    case 0x59:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em_camera_req(self, 0xC, 0);
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em_camera_req(self, 6, 0);
        }
        break;
    case 0x6F:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x1A, 6, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 118.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 0x1A, 5, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 306.0f, 0.0f) == 1) {
            em_eft_fx_spawn_anchor(self, 7, 3, 1.0f);
        }
        if (em_after_frame_check(self, 0, 418.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 120.0f);
            eft_em_spawn_joint(self, 0x1A, 6, &vec, 1.2f, 0x3C);
        }
        break;
    case 0x79:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 0xC, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0xC, 0, 1.5f);
            }
        } else {
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 6, 0, 1.6f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 6, 0, 1.6f);
            }
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7A:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0x12, 0x14, 0, 2.0f);
            }
        } else if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            setVector3(&vec, self->pos.x, self->field_0x20C, self->pos.z);
            eft_spawn_pos_in_area(&vec, self->area_no, 4, self->field_0x1C0, get_em_chg_scale(self));
        }
        if (em_after_frame_check(self, 0, 116.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7C:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0x12, 0, 0.8f);
            }
        } else if (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
        }
        break;
    case 0x7D:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 3, 0x11, 0, 1.3f);
            }
        } else if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            eft_spawn_pos_in_area(&self->pos, self->area_no, 4, self->field_0x1C0, get_em_chg_scale(self));
        }
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7F:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 8, 0x12, 0, 0.8f);
            }
        } else if (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 8, 2, 0, 1.5f);
        }
        break;
    case 0x81:
        if (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0x11, 0, 1.3f);
            } else {
                setVector3(&vec, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&vec, self->area_no, 4, 0, 0.8f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 156.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x82:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                eft009_spawn_at_joint(self, 0xE, 0x12, 0, 0.8f);
            }
        } else if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0xE, 6, 0, 1.8f);
        }
        break;
    case 0x83:
        if (em_after_frame_check(self, 0, 326.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0x10, 0, 1.3f);
            } else {
                get_joint_wpos_em(self, 3, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 1, 0, 1.2f * get_em_chg_scale(self));
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x84:
        if ((em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0x11, 0, 1.3f);
            } else {
                setVector3(&vec, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&vec, self->area_no, 0, 0, 1.5f * get_em_chg_scale(self));
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x88:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0x14, 0, 2.3f);
            } else {
                setVector3(&vec, self->pos.x, self->field_0x20C, self->pos.z);
                eft_spawn_pos_in_area(&vec, self->area_no, 2, 0, 1.2f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x8B:
        if (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 0x14, 0x11, 0, 1.2f);
            } else {
                get_joint_wpos_em(self, 0x14, &vec);
                vec.y = self->field_0x20C;
                eft_spawn_pos_in_area(&vec, self->area_no, 0, 0, 1.5f * get_em_chg_scale(self));
            }
        }
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xCE:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            eft013_em_set(self, 0);
        }
        if ((em_after_frame_check(self, 3, 18.0f, 98.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            eft_spawn_type_at_area(self, 0);
        }
        if (em_after_frame_check(self, 3, 126.0f, 134.0f) == 1) {
            setVector3(&vec, 0.0f, -90.0f, 100.0f);
            if ((system_w.field_0x0c & 3) == 0) {
                eft_em_spawn(self, 0x11, 0x19, &vec, 1.0f);
            }
        }
        if (em_after_frame_check(self, 3, 142.0f, 206.0f) == 1) {
            setVector3(&vec, 0.0f, -90.0f, 100.0f);
            if ((system_w.field_0x0c & 7) == 0) {
                eft_em_spawn(self, 0x10, 0x19, &vec, 1.0f);
            }
        }
        break;
    }
}

/* Steps a scalar toward `center` by `step`, never overshooting it. */
f32 em015_step_toward(f32 value, f32 center, f32 step) {
    if (value < center) {
        value += step;
        if (value < center) {
            return value;
        }
    } else if (value > center) {
        if (value > center + step) {
            return value - step;
        }
    }
    return center;
}

/* Drives the colour scalar and the three K-colour bytes toward the mode's targets (`em015_damage_level`'s damage
 * level picks the byte targets). */
void em015_color_step(_ENEMY_WORK* self) {
    f32 target = (self->field_0x1E2 == 2) ? 255.0f : 100.0f;
    {
        f32 step = 0.08611111f;
        self->color_0x328.field_0x328 = em015_step_toward(self->color_0x328.field_0x328, target, step);
    }
    u8 level = (u8)em015_damage_level(self);
    f32 byte_a;
    f32 byte_b;
    switch (level) {
    default:
        target = 0.0f;
        byte_a = target;
        byte_b = target;
        break;
    case 1:
        target = 40.0f;
        byte_a = 90.0f;
        byte_b = 120.0f;
        break;
    case 2:
        target = 100.0f;
        byte_a = 220.0f;
        byte_b = 255.0f;
        break;
    }
    self->color_0x328.field_0x32C =
        (u8)em015_step_toward((f32)self->color_0x328.field_0x32C, target, 8.0f);
    self->color_0x328.field_0x32D =
        (u8)em015_step_toward((f32)self->color_0x328.field_0x32D, byte_a, 8.0f);
    self->color_0x328.field_0x32E =
        (u8)em015_step_toward((f32)self->color_0x328.field_0x32E, byte_b, 8.0f);
}

/* The MHchar K-colour material update: rebuild the four channel colours from the work's scalar and byte colours,
 * then set the fifth from the aim/special state. */
void em015_tev_color_update(_ENEMY_WORK* self) {
    _GXColor color;
    em015_color_step(self);
    color.r = self->color_0x328.field_0x328;
    color.g = self->color_0x328.field_0x328;
    color.b = self->color_0x328.field_0x328;
    color.a = 0xFF;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, &color);
    if ((self->flags_0x836 & 1) != 0) {
        if (self->field_0x48F == 1) {
            color.r = 0xFF;
            color.g = 0xFF;
            color.b = 0xFF;
        } else {
            color.r = 100;
            color.g = 100;
            color.b = 100;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    color.r = self->color_0x328.field_0x32C;
    color.g = self->color_0x328.field_0x32D;
    color.b = self->color_0x328.field_0x32E;
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (em_eft_active_ck(self) == 1) {
        color.a = 0;
    } else if (em_alt_mode_ck(self) == 1) {
        color.a = (u8)((s32)(30.0f * (0.5f *
                    (1.0f + sin_idx_get((u16)(system_w.field_0x0c << 13))))) + 225);
    } else {
        color.a = (u8)((s32)(30.0f * (0.5f *
                    (1.0f + sin_idx_get((u16)(system_w.field_0x0c << 13))))) + 135);
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

/* True for the dead mode (0x1E2 == 2) while the record has not latched its aim state. */
u32 em015_dead_mode_ck(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 2 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* Clamps the 0x1E4 counter: part 3 at full damage caps it at 99. */
void em015_part_clamp(_ENEMY_WORK* self, u32 part) {
    if ((s32)(part & 0xFF) == 3) {
        if ((u8)em_parts_damage_level_get(self, 3) == 3) {
            if (self->field_0x1E4 >= 100) {
                self->field_0x1E4 = 99;
            }
        }
    }
}

/* The area/action start predicate: true when the record's map/area state arms the next action, either by setting
 * the 0x1FC/0x1FE/0x1FF request or by running the 0x8012A014 test. */
u32 em015_area_start_ck(_ENEMY_WORK* self, u32 arg) {
    u8 mode = stage_map_kind_get(self->field_0x1E0);
    u32 armed;
    u32 area;
    switch (mode) {
    case 1:
    case 3:
        break;
    default:
        return 0;
    }
    armed = 0;
    switch (mode) {
    case 1:
        area = 8;
        break;
    case 3:
        area = 6;
        break;
    default:
        area = 0xFF;
        break;
    }
    if (area != 0xFF) {
        switch (em_area_mot_ck_view(self, area)) {
        case 1:
            armed = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (armed == 0) {
        if (self->field_0x1FC == 1 && self->field_0x1FE == 8) {
            return 1;
        }
        if (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) {
            if (em_eft_active_ck(self) == 1 && self->color_0x328.field_0x32F == 0) {
                switch (mode) {
                case 1:
                    if ((s32)self->area_no == 7) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 12;
                        return 1;
                    }
                    break;
                case 3:
                    switch (self->area_no) {
                    case 4:
                    case 8:
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 3;
                        return 1;
                    }
                    break;
                }
            } else if (self->color_0x328.field_0x330 != 0) {
                u32 sel;
                switch (mode) {
                case 1:
                    sel = 8;
                    break;
                case 3:
                    sel = 3;
                    break;
                default:
                    sel = 0xFF;
                    break;
                }
                if (em_pack_area_pick(self, 23, sel, (u16)arg, em015_area_follow_tbl, em015_area_point_tbl) == 1) {
                    return 1;
                }
            }
        }
    }
    if (em_area_roam_pick(self, (u16)arg) == 1) {
        return 1;
    }
    return em_area_leave_ck(self) == 1;
}

/* The area's follow-up arming: when the action has run out (not action 10, 0x81A <= 0) pick the two motion ids
 * the 0x1E2 mode names. */
void em015_area_follow(_ENEMY_WORK* self) {
    fn_80128B80(self);
    if (self->action != 10 && self->field_0x81A <= 0) {
        switch (self->field_0x1E2) {
        case 0:
            fn_80128AEC(self, 13, 12);
            break;
        case 2:
            fn_80128AEC(self, 13, 13);
            break;
        }
    }
}

/* Sets the part flag from a one-value selector. */
void em015_part_flag_set(_ENEMY_WORK* self, u32 value) {
    if (value == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* Resets the work's part state after the user-data teardown. */
void em015_user_data_node_calc(EmUserData* self) {
    em_res_user_data_flag_set((_ENEMY_WORK*)self, 5);
    self->work_0x04->part_0x740.field_0x740 = 1;
    self->work_0x04->part_0x740.field_0x742 = 0;
    self->work_0x04->part_0x740.field_0x744 = 0;
    self->work_0x04->part_0x740.field_0x746 = 0;
    self->work_0x04->part_0x740.field_0x748 = 0;
    self->work_0x04->part_0x740.field_0x74A = 0;
    self->work_0x04->part_0x740.field_0x74C = 0;
}

/* Is the work within the aimed target's attack range: compare the squared XZ distance to the rotated
 * offset point against the scaled range. */
u32 em015_ally_front_ck(_ENEMY_WORK* self, u32 arg) {
    VEC3 a;
    VEC3 b;
    VEC3 rel;
    VEC3 probe;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    _ENEMY_WORK* target = em_ally_find(self, 23, 0);
    if (target != 0 && em_act11_wait_ck(target) == 1) {
        if ((arg & 0xFF) == 0) {
            return 1;
        }
        {
            copyVec3(&b, setVec3(&rel, 0.0f, 0.0f, 900.0f * get_em_chg_scale(self)));
            rotVecY(&b, self->field_0x1C0);
            addVec3(&probe, &self->pos, &b);
            copyVec3(&a, &probe);
            f32 dist = calcDistanceSqXZ(&a, &target->pos);
            if (dist < 400.0f * get_em_chg_scale(self) * (400.0f * get_em_chg_scale(self))) {
                return 1;
            }
        }
    }
    return 0;
}

/* The "action 13, sub-state <= 5" predicate. */
u32 em015_act13_early_ck(_ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* The per-kind aim/approach query the action band dispatches over. */
u8 em015_state_query(_ENEMY_WORK* self, u32 kind) {
    switch (kind & 0xFF) {
    case 0: {
        f32 delta = self->vec_0x36C.y - self->pos.y;
        if (delta >= 700.0f) {
            return 2;
        }
        if (delta >= 200.0f) {
            return 1;
        }
        if (delta <= -700.0f) {
            return 4;
        }
        if (delta <= -200.0f) {
            return 3;
        }
        return 0;
    }
    case 1:
        return em015_damage_level(self);
    case 2:
        return self->color_0x328.field_0x32F;
    case 3: {
        _ENEMY_WORK* target = em_ally_find(self, 23, 0);
        if (target != 0) {
            return em_act11_wait_ck(target) != 1;
        }
        return 0;
    }
    case 4:
        return self->color_0x328.field_0x332 >= 900;
    case 5: {
        return self->pos.y > self->field_0x210 - (100.0f + em_float_depth_get(self)) * get_em_chg_scale(self);
    }
    case 6:
        return self->color_0x328.field_0x330 != 0;
    case 7:
        if ((u8)em_parts_damage_level_get(self, 3) >= 3) {
            return 0;
        }
        return em015_damage_level(self) != 0;
    default:
        return 0;
    }
}

/* Area/action -> motion pair: fills the two out-bytes and arms the motion ids the
 * map/area state names. */
void em015_death_action_get(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 7:
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            em_move_target_set(self, 14, 15, 0.0f);
            break;
        case 12:
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            em_move_target_set(self, 23, 24, 0.0f);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 4:
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            em_move_target_set(self, 6, 7, 0.0f);
            break;
        case 8:
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            em_move_target_set(self, 6, 0, 0.0f);
            break;
        }
        break;
    case 9:
    case 11:
        if ((s32)self->area_no == 1) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            em_move_target_set(self, 0, 1, 0.0f);
        }
        break;
    default:
        break;
    }
}

/* Tail-calls `em015_aim_set`. */
void em015_aim_reset(_ENEMY_WORK* self) {
    em015_aim_set(self);
}

/* The 0x1E4 damage counter's level: 0 below 1, 1 below 100, and past that the part-3 damage level maps to 5, 4,
 * 3, ... (retail's branchless `subfc`/`adde` run). */
u8 em015_damage_level(_ENEMY_WORK* self) {
    u8 health = self->field_0x1E4;
    if (health < 1) {
        return 0;
    }
    if (health < 100) {
        return 1;
    }
    u8 part = (u8)em_parts_damage_level_get(self, 3);
    if (part >= 3) {
        return 1;
    }
    return 2;
}

/* The map/area aim/rotation setter: clears the aim vector then arms the rotation id
 * the map/area state names. */
void em015_aim_set(_ENEMY_WORK* self) {
    f32 zero = 0.0f;
    self->aim.x = zero;
    self->aim.y = zero;
    self->aim.z = zero;
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((s32)self->area_no == 7 || (s32)self->area_no == 12) {
            em_data_point_get(self, (u16)((self->area_no & 0xF) * 256), &self->aim);
        }
        break;
    case 3:
        switch (self->area_no) {
        case 3:
        case 6:
            em_data_point_get(self, (u16)((self->area_no & 0xF) * 256 | 8), &self->aim);
            break;
        case 4:
            em_data_point_get(self, (u16)((self->area_no & 0xF) * 256 | 3), &self->aim);
            break;
        case 8:
            em_data_point_get(self, (u16)((self->area_no & 0xF) * 256 | 4), &self->aim);
            break;
        }
        break;
    case 9:
    case 11:
        if ((s32)self->area_no == 1) {
            f32 v = 0.0f;
            self->aim.x = v;
            self->aim.y = -1500.0f;
            self->aim.z = v;
        }
        break;
    default:
        break;
    }
}

/* Fills the shell job's spawn record: type 0x1A, the fixed offset and the three scalars. */
void em015_spawn_rec_set(EmSpawnRec* out, u8 a, u16 b, u16 c) {
    VEC3 vec;
    setVec3(&vec, 0.0f, -45.0f, 70.0f);
    out->id = 0x1A;
    copyVec3(&out->pos, &vec);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* The record's release step: unregister, then free when the caller asks (arg > 0); returns the record so a caller
 * can chain. */
Em015UserData* em015_user_data_dtor(Em015UserData* self, s16 arg) {
    if (self != 0) {
        em_res_user_data_dtor((_ENEMY_WORK*)self, 0);
        if (arg > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x80182B94 (0xAC): the static initialiser: seeds the four sound-offset vectors. */
void em015_static_init(void) {
    VEC3 a;
    VEC3 b;
    VEC3 c;
    VEC3 d;
    assignVec3_c1(&em015_se_shift[0], setVec3(&a, 0.0f, 0.0f, 12.5f));
    assignVec3_c1(&em015_se_shift[1], setVec3(&b, 0.0f, 0.0f, 0.0f));
    assignVec3_c1(&em015_se_shift[2], setVec3(&c, 2.0f, 0.0f, 0.0f));
    assignVec3_c1(&em015_se_shift[3], setVec3(&d, 0.0f, 0.0f, 0.0f));
}

/* The `.ctors` word (0x8056F338) the split assigns to this unit - it points at the static initialiser. */
__declspec(section ".ctors") void* const em015_ctor = (void*)em015_static_init;

/* The unit's `.bss`: the four-vector table `em015_static_init` fills (the map's second row at +0x18 is folded
 * in).  The name is a GUESS. */
VEC3 em015_se_shift[4];  /* +0x806A79D0 */
