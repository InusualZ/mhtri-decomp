/*
 * enemy/em016_prog.cpp - the program shared by enemies 016, 017 and 021 (`em016_prog_tbl`, `em017_prog_tbl`,
 *   `em021_prog_tbl`): init, frame update, event hook, the per-action step machines (idle, look, walk, swim, guard,
 *   attack, sound, route, demo) and their dispatchers, the motion-event driver and effect router, the TEV tint,
 *   the action selection slots, the user-data hooks and the static initializer.
 * RANGE. .text 0x80182C40-0x80192348 (155 functions, in address order); extab 0x8000E9E4-0x8000EDC4, extabindex
 *   0x80029DC0-0x8002A390, .ctors 0x8056F33C-0x8056F340, .rodata 0x8056FF50-0x80570050, .data 0x805AA930-0x805AD370
 *   (the three program tables first), .bss 0x806A7A00-0x806A7A6C, .sdata 0x80791A60-0x80791A78, .sbss
 *   0x80794AA0-0x80794AA8, .sdata2 0x80797E88-0x80798238.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `extsh`+`cmpwi`, `clrlwi`+`cmpwi`
 *   forms and the raw-constant `clrlwi` of a `u16` argument), `#pragma fp_contract off` (no fused multiply-adds),
 *   `#pragma pool_data off` (the data is defined in the TU; pooled, the `.bss` users take a base register).
 * NAMES. The file name follows the runtime dump's `em016_prog_tbl`, which opens the TU's `.data`.  Every function
 *   and data name is a GUESS from the body, its program-table slot (`Em015ProgTbl`'s scheme: em016_init,
 *   em016_frame_update, em016_action_dispatch, em016_event_notify, em016_motion_events, em016_tev_color_update,
 *   em016_effect_window_ck, em016_action_select, em016_action_rearm, em016_ready_ck, em016_state_clear_ck,
 *   em016_part_drop, em016_action_allow_ck) or the action group it serves (em016_idle_*, em016_look_*,
 *   em016_walk_*, em016_swim_*, em016_guard_*, em016_atk_*, em016_sound_*, em016_route_*, em016_demo_*); the SE
 *   programs are named `<species>_se_seq_<sub>`/`_se_alt_seq_<sub>` after the first `state_sub` that plays them,
 *   their records `<table>_rec<N>`; `em017_0x328`'s alert pair and `EmActWork` are this unit's views.
 * RESIDUALS. 13 rows short of 100:
 *  - `em016_frame_em017`: self sits in r31 where retail has r29 (the loop's found/rec/max/i take r31..r27);
 *  - `em016_frame_effect`: retail recomputes `(u16)(angle + 0x8000)` at each test, ours keeps it in a register;
 *  - `em016_action_select`: retail lowers the `act_id` 5/6/8 case group as a range test (`subi`+`cmplwi`);
 *  - `em016_strafe_point_get`: the selected offset lands in r4 where retail has r3 (`&v` computed late);
 *  - `em016_atk_spit`, `em016_walk_approach`, `em016_walk_approach_hit`, `em016_swim_short_turn`,
 *    `em016_swim_team_mot`: retail truncates a two-constant motion choice with `clrlwi r4,r4,16` at the call; every
 *    spelling tried (u16/u32 local, ternary, if, u16 parameter view, cast) folds it away;
 *  - `em016_atk_volley_charge`, `em016_atk_volley_run`: `(kind == 1) + 0xC9` computed into r0 where retail uses r4;
 *  - `em016_action_allow_ck`, `em016_swim_rest`: one scratch register coloured differently.
 *  `.ctors` is not emitted: `em016_static_init` is a plain function (a real `__sinit` needs the vector pair
 *  constructors). flipcheck also names `fn_805012E8` (no map row in this tree).
 * SHAPES. `Em016UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   A one-case test retail compares signed (`cmpwi`) is a `switch`; a trailing `default:` retail lays out
 *   first is written first.  The float argument of the effect spawners is evaluated first, so they are called
 *   through float-first views (`em_eft_fx_spawn_ff`, `eft_em_spawn_joint`'s owner order).  The shell slots +0x2C,
 *   +0x30 and +0x3C are called without the table argument the header spells (r6/r8 there is only the table base),
 *   and their id argument is a `u16`.  `x -= n; if (x < lim)` (not a local) gives retail's store-then-`extsh`.
 *   The aim fade step is a named `.sdata2` word: a literal `x - -0.005f` folds into `x + 0.005f`.
 */

#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "ef/eft026_pl_spawn.h" /* eft026_pl_spawn (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/em_alt_mode_ck.h" /* em_alt_mode_ck (rule 2: the owner's header) */
#include "enemy/em_eft_active_ck.h" /* em_eft_active_ck (rule 2: the owner's header) */
#include "enemy/em_mot_set_blend.h" /* em_mot_set_blend (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mot_speed_set.h" /* em_mot_speed_set (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_float_depth_get.h" /* em_float_depth_get (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_frame_flag_set.h" /* em_frame_flag_set (rule 2: the owner's header) */
#include "enemy/em_busy_timer_reset.h" /* em_busy_timer_reset (rule 2: the owner's header) */
#include "enemy/em_flag_0x8AE_ck.h" /* em_flag_0x8AE_ck (rule 2: the owner's header) */
#include "enemy/em_hit_arm_set.h" /* em_hit_arm_set (rule 2: the owner's header) */
#include "enemy/em_angle_approach.h" /* em_angle_approach (rule 2: the owner's header) */
#include "enemy/em_turn_in_window.h" /* em_turn_in_window (rule 2: the owner's header) */
#include "enemy/em_flags836_ck.h" /* em_flags836_ck (rule 2: the owner's header) */
#include "enemy/em_camera_req.h" /* em_camera_req (rule 2: the owner's header) */
#include "enemy/em_shake_req_set.h" /* fn_80136D14 (rule 2: the owner's header) */
#include "enemy/em_area_stun_move_ck.h" /* em_area_stun_move_ck (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "enemy/ENEMY_WORK.h"
#define em_eft_active_ck fn_8012EC3C_hidden_fn_8012E968_h
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#include "unsplit/enemy.h"
#undef em_alt_mode_ck
#undef em_eft_active_ck
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "ef.h"
#include "enemy/fn_80138074.h" /* em_res_user_data_flag_set, ResUserDataAc */
#include "MSL/new.h" /* the placement `operator new` */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "enemy/em_eft_fx.h"
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#define stage_map_kind_get stage_map_kind_get_hidden_stg_w_h
#include "stage/stg_w.h"
#undef stage_map_kind_get
#include "enemy/fn_801251D0.h"
#include "ef.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80147CE0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_80105314.h"
#include "mh3_pad.h"
#include "sys_mem.h"
#include "enemy/fn_8012EC74.h" /* fn_8013026C/fn_8012FE3C/fn_801337FC/em_height_follow */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "enemy/em016_prog_types.h"
#include "enemy/em_reaction_req.h" /* em_reaction_req */
#include "enemy/em_motion_kind_set.h" /* em_motion_kind_set */
#include "enemy/em_act_set_unless_act11.h" /* em_act_set_unless_act11 */
#include "enemy/em_common.h" /* em_rest_flag_set */
#pragma peephole off
#pragma fp_contract off
#pragma pool_data off
#include "enemy/enemy_control.h" /* em_spawn_request */
#include "ef/eft007.h" /* eft007_part_set / eft007_part_spawn */
#include "ef/ef_postfield.h" /* vec3_sub_assign */
#include "sound/fn_800DD1F0.h" /* mhchar_mat_blend_set */
#include "enemy/em_se_record.h" /* EmSeEntry */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play */
#include "enemy/em_se_tbl_play_alt.h" /* em_se_tbl_play_alt */
#include "lobby/fn_801E0ADC.h" /* eft_em_spawn_joint */
#include "ef/get_move_work_adrs.h" /* get_move_work_max / get_move_work_adrs */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
/* The SE programs the three sound dispatchers play (this unit's `.data`, not emitted yet). */
extern EmSeEntry em016_se_seq_18[]; /* 0x805AAD90 */
extern EmSeEntry em016_se_seq_17[]; /* 0x805AADB8 */
extern EmSeEntry em016_se_seq_1A[]; /* 0x805AAE10 */
extern EmSeEntry em016_se_seq_1B[]; /* 0x805AAE80 */
extern EmSeEntry em016_se_seq_7A[]; /* 0x805AAEC0 */
extern EmSeEntry em016_se_seq_7B[]; /* 0x805AAEE8 */
extern EmSeEntry em016_se_seq_9F[]; /* 0x805AAF10 */
extern EmSeEntry em016_se_seq_7C[]; /* 0x805AAF38 */
extern EmSeEntry em017_se_seq_7D[]; /* 0x805AAF60 */
extern EmSeEntry em016_se_seq_7E[]; /* 0x805AAFB8 */
extern EmSeEntry em016_se_seq_7F[]; /* 0x805AB020 */
extern EmSeEntry em016_se_seq_8D[]; /* 0x805AB058 */
extern EmSeEntry em016_se_seq_29[]; /* 0x805AB080 */
extern EmSeEntry em016_se_seq_28[]; /* 0x805AB0A8 */
extern EmSeEntry em016_se_seq_89[]; /* 0x805AB0D0 */
extern EmSeEntry em016_se_seq_8A[]; /* 0x805AB100 */
extern EmSeEntry em016_se_seq_8B[]; /* 0x805AB128 */
extern EmSeEntry em016_se_seq_8C[]; /* 0x805AB180 */
extern EmSeEntry em017_se_seq_8C[]; /* 0x805AB1C0 */
extern EmSeEntry em016_se_seq_8F[]; /* 0x805AB1E8 */
extern EmSeEntry em016_se_seq_B1[]; /* 0x805AB210 */
extern EmSeEntry em016_se_seq_B6[]; /* 0x805AB248 */
extern EmSeEntry em016_se_seq_B7[]; /* 0x805AB270 */
extern EmSeEntry em016_se_seq_B8[]; /* 0x805AB298 */
extern EmSeEntry em016_se_seq_B9[]; /* 0x805AB2C0 */
extern EmSeEntry em016_se_seq_BA[]; /* 0x805AB2E8 */
extern EmSeEntry em016_se_seq_BB[]; /* 0x805AB310 */
extern EmSeEntry em016_se_seq_BF[]; /* 0x805AB350 */
extern EmSeEntry em016_se_seq_C0[]; /* 0x805AB3B0 */
extern EmSeEntry em016_se_seq_C8[]; /* 0x805AB408 */
extern EmSeEntry em016_se_seq_D0[]; /* 0x805AB448 */
extern EmSeEntry em017_se_seq_D6[]; /* 0x805AB478 */
extern EmSeEntry em021_se_seq_23[]; /* 0x805AB4F0 */
extern EmSeEntry em021_se_seq_34[]; /* 0x805AB598 */
extern EmSeEntry em021_se_seq_37[]; /* 0x805AB650 */

extern EmSeEntry em016_se_alt_seq_00[]; /* 0x805AB990 */
extern EmSeEntry em016_se_alt_seq_0F[]; /* 0x805AB9B8 */
extern EmSeEntry em016_se_alt_seq_1A[]; /* 0x805ABA00 */
extern EmSeEntry em016_se_alt_seq_1B[]; /* 0x805ABA30 */
extern EmSeEntry em016_se_alt_seq_1C[]; /* 0x805ABA58 */
extern EmSeEntry em016_se_alt_seq_0A[]; /* 0x805ABA98 */
extern EmSeEntry em017_se_alt_seq_05[]; /* 0x805ABB60 */
extern EmSeEntry em021_se_alt_seq_05[]; /* 0x805ABC38 */
extern EmSeEntry em021_se_alt_seq_0B[]; /* 0x805ABCC0 */
extern EmSeEntry em021_se_alt_seq_0E[]; /* 0x805ABD58 */

/* signatures the new bodies' calls use, when they differ from the owner header's: the same direct call. */
#define em_gauge_period_step_c1 ((void (*)(_ENEMY_WORK*, f32, s32, s16))em_gauge_period_step)
#define em_reaction_req_view1 ((void (*)(_ENEMY_WORK*, u32, u32))em_reaction_req)
#define em_move_offset_step_update_view1 ((u32 (*)(_ENEMY_WORK*, u32*, f32))em_move_offset_step_update)
/* Slots +0x2C, +0x30 and +0x3C of `shell_set_func_ptr` as this unit calls them: without the trailing table argument
 * the header spells. */
typedef void (*EmShellJobFn)(_ENEMY_WORK* self, u32 kind, u32 count, VEC3* pos, f32 scale, u16 id);
typedef void (*EmShellLineJobFn)(_ENEMY_WORK* self, u32 kind, VEC3* from, VEC3* to, f32 scale, u16 id);
typedef void (*EmShellShotFn)(_ENEMY_WORK* self, EmSpawnRec* req, u32 mode);
/* the effect spawners with the float-first order the call sites evaluate (the owner header lists it last) */
#define em_eft_fx_spawn_ff ((void (*)(_ENEMY_WORK*, u32, u32, f32, u32))em_eft_fx_spawn)
#define em_eft_fx_spawn_pair_ff ((void (*)(_ENEMY_WORK*, u32, u32, u32, f32, u32))em_eft_fx_spawn_pair)
/* the turn-sequence calls with the tables' `const` type (the owner header takes a plain pointer) */
#define em_turn_seq_start_c ((void (*)(_ENEMY_WORK*, const u32*, s32, s32, s32))em_turn_seq_start)
#define em_turn_seq_step_c ((u32 (*)(_ENEMY_WORK*, const u32*))em_turn_seq_step)
#define em_die_ck_u32 ((u32 (*)(_ENEMY_WORK*))em_die_ck)
#define em_mot_set_u16 ((void (*)(_ENEMY_WORK*, u16, s32, s32))em_mot_set)
#define em_target_pos_set_view1 ((void (*)(_ENEMY_WORK*, VEC3*))em_target_pos_set)
#define em_ground_ck_view1 ((u32 (*)(_ENEMY_WORK*, f32))em_ground_ck)

#define fn_80117E58_c1 ((void (*)(_ENEMY_WORK*, u32, nw4r::math::VEC3*, u32, f32))eft026_pl_spawn)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_alt_mode_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_alt_mode_ck)
#define fn_8012EC3C_c1 ((u32 (*)(_ENEMY_WORK*))em_eft_active_ck)
#define fn_80129D3C_c1 ((u32 (*)(EmActWork*))em_area_stun_move_ck)
#define fn_80182D5C_noarg ((void (*)(void))em016_area_route_set)
#define stage_map_kind_get_view3 ((u8 (*)(u8))stage_map_kind_get)
#define stage_map_kind_get_view1 ((u8 (*)(u8))stage_map_kind_get)
#define fn_80192080_view1 ((void (*)(_ENEMY_WORK*))em016_aim_fade)
#define fn_80191EF8_view1 ((void (*)(_ENEMY_WORK*))em016_aim_step)
#define fn_80182D5C_view1 ((void (*)(void))em016_area_route_set)
#define fn_8013A654_view1 ((void (*)(EmUserData*, u32))em_res_user_data_flag_set)
#define fn_801321DC_view1 ((u32 (*)(EmActWork*))em_flag_0x8AE_ck)
#define fn_8012EC3C_view1 ((u32 (*)(EmActWork*))em_eft_active_ck)
#define fn_8012A204_view1 ((s32 (*)(EmActWork*))em_area_leave_ck)
#define fn_8012A014_view1 ((u32 (*)(EmActWork*, u32, u32, u16, u32, u8*))em_pack_area_pick)
#define em_area_break_ck_c1 ((u8 (*)(EmActWork*, u8))em_area_break_ck)
#define fn_80129A70_view1 ((u32 (*)(EmActWork*, u16))em_area_roam_pick)
#define fn_80126324_view1 ((void (*)(EmActWork*, u8, u8, f32))em_move_target_set)
#define em_water_check_view1 ((u32 (*)(struct _ENEMY_WORK*))em_water_check)
#define em_move_mode_set_view1 ((void (*)(EmActWork*, u32))em_move_mode_set)
#define em_mot_set_view1 ((void (*)(EmActWork*, s32, s32, s32))em_mot_set)
#define em_fall_start_view1 ((void (*)(_ENEMY_WORK*, f32))em_fall_start)
#define em_alt_mode_ck_view1 ((u32 (*)(EmActWork*))em_alt_mode_ck)
#define eft_spawn_type11_view1 ((void (*)(_ENEMY_WORK*, nw4r::math::VEC3*, u32, f32))eft_spawn_type11)
#define assignVec3_view1 ((void (*)(void*, const void*))assignVec3)

extern "C" {
void em016_strafe_point_get(_ENEMY_WORK* self, u8 kind, VEC3* out);
void em016_team16_motion_req(_ENEMY_WORK* self, u32 arg);

/* `stage/stg_w.cpp`'s byte-table lookup (0xFF in `lbl_805CED40` returns the argument), declared with the
 * `u32` return this block's call sites mask themselves (`clrlwi r0,r3,24`). */
u32 stage_map_kind_get(u32 kind);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can tail-call them.
 * ------------------------------------------------------------------------------------------------ */
void em016_frame_em016(_ENEMY_WORK* self);
void em016_frame_em017(_ENEMY_WORK* self);
void em016_look_roar(_ENEMY_WORK* self);
void em016_look_mot0B(_ENEMY_WORK* self);
void em016_look_guard_roar(_ENEMY_WORK* self);
void em016_look_dispatch_em016(_ENEMY_WORK* self);
void em016_walk_turn_seq(_ENEMY_WORK* self, u32 arg);
void em016_walk_mot1B(_ENEMY_WORK* self, u32 arg);
void em016_walk_approach(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void em016_walk_busy_approach(_ENEMY_WORK* self);
void em016_walk_approach_hit(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void em016_walk_approach_timed(_ENEMY_WORK* self, u32 arg);
void em016_walk_mot15(_ENEMY_WORK* self);
void em016_walk_dispatch_em016(_ENEMY_WORK* self);
void em016_walk_dispatch_em017(_ENEMY_WORK* self);
void em016_walk_dispatch_em021(_ENEMY_WORK* self);
void em016_look_dispatch_em017(_ENEMY_WORK* self);
void em016_look_dispatch_em021(_ENEMY_WORK* self);
}

/* The user-data accessor `em016_init` installs for enemy 016: the base accessor with its own destructor, attach
 * hook and two item hooks (its table is `.data` 0x805AD340).  size: 0xC (`operator new(0xC)`) */
class Em016UserData : public ResUserDataAc {
public:
    Em016UserData();
    virtual ~Em016UserData();
    virtual void Attach();
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    virtual void CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};

/* The mangled callees, outside `extern "C"` with the widths each mangling encodes (rule 9). */
u16 calcVecAngX(nw4r::math::VEC3* v);
void eft009_set_pos(u8 id, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 arg);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
f32 get_em_scale(struct _ENEMY_WORK* self);
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

extern "C" {
/* ------------------------------------------------------------------------------------------------ *
 * This unit's own functions, declared up front so the dispatchers can call them.
 * ------------------------------------------------------------------------------------------------ */
void em016_route_busy_walk(_ENEMY_WORK* self);
void em016_route_swim_approach(_ENEMY_WORK* self);
void em016_route_dispatch(_ENEMY_WORK* self);
void em016_demo_opening(_ENEMY_WORK* self);
void em016_demo_pose_1(_ENEMY_WORK* self);
void em016_demo_run_2(_ENEMY_WORK* self);
void em016_demo_pose_2(_ENEMY_WORK* self);
void em016_demo_run_3(_ENEMY_WORK* self);
void em016_demo_pose_3(_ENEMY_WORK* self);
void em016_demo_run_4(_ENEMY_WORK* self);
void em016_demo_pose_4(_ENEMY_WORK* self);
void em016_demo_run_5(_ENEMY_WORK* self);
void em016_demo_pose_5(_ENEMY_WORK* self);
void em016_demo_run_6(_ENEMY_WORK* self);
void em016_demo_pose_6(_ENEMY_WORK* self);
void em016_demo_run_7(_ENEMY_WORK* self);
void em016_demo_pose_7(_ENEMY_WORK* self);
void em016_demo_dispatch(_ENEMY_WORK* self);
void em016_action_dispatch(_ENEMY_WORK* self);
void em016_frame_effect(_ENEMY_WORK* self);
void em016_effect_spawn(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);
void em016_motion_events(_ENEMY_WORK* self);
void em016_tev_color_update(_ENEMY_WORK* self);
s32 em016_effect_window_ck(_ENEMY_WORK* self, u8 arg1);

void em016_swim_approach(_ENEMY_WORK* self, u8 kind, u8 variant);
void em016_idle_dispatch(_ENEMY_WORK* self);
void em016_look_dispatch(_ENEMY_WORK* self);
void em016_walk_dispatch(_ENEMY_WORK* self);
void em016_swim_dispatch(_ENEMY_WORK* self);
void em016_guard_dispatch(_ENEMY_WORK* self);
void em016_atk_dispatch(_ENEMY_WORK* self);
void em016_sound_prog(_ENEMY_WORK* self);
void em016_sound_alt(_ENEMY_WORK* self);
void em016_route_approach(_ENEMY_WORK* self);
void em016_route_guard_drop(_ENEMY_WORK* self);
void em016_route_mot01(_ENEMY_WORK* self);
void em016_route_swim(_ENEMY_WORK* self);
void em016_route_walk(_ENEMY_WORK* self);
void em016_route_water_entry(_ENEMY_WORK* self);
void em016_route_walk_mot59(_ENEMY_WORK* self);
void em016_route_walk_target(_ENEMY_WORK* self);
s16 em_demo_frame_get(void);
u32 em_demo_time_ck(u32 a);
void em_demo_pos_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_enable(_ENEMY_WORK* self);
void eft009_spawn_at_joint(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void eft009_spawn_at_pos(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d, nw4r::math::VEC3* p);
void eft_spawn_type10(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 d);
void eft_spawn_pos_in_area(nw4r::math::VEC3* p, u8 a, u32 b, u32 c, f32 d);
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 c);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 sin_idx_get(u16 a);

}

/* `_CP_VECTOR` is `ef.h`'s three-word type; `ef.h` is not included because its `setVec3` returns `void`
 * where these call sites read r3. */
struct _CP_VECTOR;

/* The 3x3 matrix `fn_805012E8` fills (nine floats; `Em016UserData::CalcAnmItemEx`'s frame is sized for it).
 * size: 0x24 */
struct EmMtx33 {
    /* +0x00 */ f32 m[3][3];
};

/* The static vector pair `em016_static_init` builds (`VEC3[2]` per 0x18-byte object).
 * size: 0x18 */
struct EmVecPair {
    /* +0x00 */ VEC3 vec_0x00;
    /* +0x0C */ VEC3 vec_0x0C;
};

/* ------------------------------------------------------------------------------------------------ */
/* the unit's pool, declared, not defined                                                            */
/* ------------------------------------------------------------------------------------------------ */


/* The five static vectors `em016_static_init` builds (the last one is the default `em016_shot_req_init` copies
 * from). */
extern EmVecPair em016_aim_vec_pair_0;
extern EmVecPair em016_aim_vec_pair_1;
extern EmVecPair em016_aim_vec_pair_2;
extern EmVecPair em016_aim_vec_pair_3;
extern VEC3 em016_shot_default_pos;

/* The one-shot latch `em016_shot_req_init` sets. */
s8 em016_shot_default_ready;

/* The `.data` word `em016_action_allow_ck` hands to `em_pack_area_pick`, in the unit's class-table block. */

u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);

void cpSetRotMatrix(struct _CP_VECTOR* rot, MTX34* mtx);

extern "C" {
/* The plain callees, declared with the call sites' arity and return widths. */
void em_drop_item_spawn(EmActWork* self, VEC3* pos, s32 value);

void joint_mtx_store(EmMtxHolder* holder, void* src);
void joint_mtx_load(EmMtxHolder* holder, void* mtx);
void anm_chr_result_scale_set(void* holder, void* vec);
void anm_chr_result_rot_mul(void* holder, void* mtx);
void mtx34_set_trans(MTX34* mtx, VEC3* vec);
void eft_rot_vec_copy(void* dst, void* src);
void fn_805012E8(EmMtx33* dst, const MTX34* src);

void MTX33_ctor(void* mtx);

/* This block's own entry points. */
void em016_action_select(EmActWork* self, u8* out_class, u8* out_state);
void em016_action_rearm(EmActWork* self);
u32 em016_ready_ck(EmActWork* self);
u32 em016_state_clear_ck(EmActWork* self);
void em016_part_drop(EmActWork* self, u32 kind);
s32 em016_action_allow_ck(EmActWork* self, u32 arg);
void em016_aim_step(EmActWork* self);
void em016_aim_fade(EmActWork* self);
void em016_shot_req_init(EmSpawnRec* out, u8 a, u16 b, u16 c);
void em016_static_init(void);
}

/* ---- the per-team step dispatch (`team` 0x10/0x11/0x15) and its steps ---- */

/* ------------------------------------------------------------------------------------------------ *
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------ */

extern "C" {
/* ------------------------------------------------------------------------------------------------ */
/* bodies, in address order                                                                          */
/* ------------------------------------------------------------------------------------------------ */

}

/* The unit's `.bss`: the four vector pairs and the default vector `em016_static_init` builds (`em016_shot_req_init` copies
 * the default).  Names are GUESSes. */
EmVecPair em016_aim_vec_pair_0;  /* +0x806A7A00 */
EmVecPair em016_aim_vec_pair_1;  /* +0x806A7A18 */
EmVecPair em016_aim_vec_pair_2;  /* +0x806A7A30 */
EmVecPair em016_aim_vec_pair_3;  /* +0x806A7A48 */
VEC3 em016_shot_default_pos;      /* +0x806A7A60 */

/* ------------------------------------------------------------------------------------------------ */
/* the unit's data, in retail order                                                                  */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {
void em016_init(_ENEMY_WORK* self, u8 mode);
void em016_frame_update(_ENEMY_WORK* self);
void em016_event_notify(_ENEMY_WORK* self, u8 kind, u8 value);
}

/* The program table each of the three species runs (`em016_prog_tbl`, `em017_prog_tbl`, `em021_prog_tbl`): the
 * entry points the shared enemy program driver calls (slot names follow `Em015ProgTbl`'s scheme, a GUESS from the
 * functions they hold).
 * size: 0x6C */
struct Em016ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ u32 pad_0x10;
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 value);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ s32 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*action_select_0x30)(EmActWork* self, u8* out_class, u8* out_state);
    /* +0x34 */ void (*action_rearm_0x34)(EmActWork* self);
    /* +0x38 */ u32 pad_0x38[4];
    /* +0x48 */ u32 (*ready_ck_0x48)(EmActWork* self);
    /* +0x4C */ u32 (*state_clear_ck_0x4C)(EmActWork* self);
    /* +0x50 */ void (*part_drop_0x50)(EmActWork* self, u32 kind);
    /* +0x54 */ s32 (*action_allow_ck_0x54)(EmActWork* self, u32 arg);
    /* +0x58 */ u32 pad_0x58[5];
};

extern "C" Em016ProgTbl em016_prog_tbl = {
    0,
    em016_init,
    em016_frame_update,
    em016_action_dispatch,
    0,
    em016_event_notify,
    em016_motion_events,
    0,
    em016_tev_color_update,
    em016_effect_window_ck,
    {0, 0},
    em016_action_select,
    em016_action_rearm,
    {0, 0, 0, 0},
    em016_ready_ck,
    em016_state_clear_ck,
    em016_part_drop,
    em016_action_allow_ck,
    {0, 0, 0, 0, 0},
};

extern "C" Em016ProgTbl em017_prog_tbl = {
    0,
    em016_init,
    em016_frame_update,
    em016_action_dispatch,
    0,
    em016_event_notify,
    em016_motion_events,
    0,
    em016_tev_color_update,
    em016_effect_window_ck,
    {0, 0},
    em016_action_select,
    em016_action_rearm,
    {0, 0, 0, 0},
    em016_ready_ck,
    em016_state_clear_ck,
    0,
    em016_action_allow_ck,
    {0, 0, 0, 0, 0},
};

extern "C" Em016ProgTbl em021_prog_tbl = {
    0,
    em016_init,
    em016_frame_update,
    em016_action_dispatch,
    0,
    em016_event_notify,
    em016_motion_events,
    0,
    em016_tev_color_update,
    em016_effect_window_ck,
    {0, 0},
    em016_action_select,
    em016_action_rearm,
    {0, 0, 0, 0},
    em016_ready_ck,
    em016_state_clear_ck,
    0,
    em016_action_allow_ck,
    {0, 0, 0, 0, 0},
};

/* The pack-area record `em016_action_allow_ck` hands `em_pack_area_pick` (word-sized fields, `Em015`'s scheme: a frame
 * count, a packed id pair and an 0xFFFF terminator; GUESS). */
u32 em016_pack_area_rec[3] = {0x64, 0x01F40000, 0xFFFF0000};

/* The four turn sequences `em_turn_seq_start` runs (`.rodata`, raw words: the layout belongs to `em_common`). */
extern "C" const u32 em016_turn_seq_a[16] = {
    0x00000006, 0x00000020, 0x00000000, 0x5C722AAB,
    0x00160000, 0x00000006, 0x00000000, 0x00000000,
    0x00180000, 0x0000000A, 0x00000000, 0x00000000,
    0x00190000, 0x0000000A, 0x00000000, 0x00000000,
};
extern "C" const u32 em016_turn_seq_b[16] = {
    0x00000014, 0x0000003C, 0x00000000, 0x5C722AAB,
    0x00160000, 0x00000006, 0x00000000, 0x00000000,
    0x00180000, 0x0000000A, 0x00000000, 0x00000000,
    0x00190000, 0x0000000A, 0x00000000, 0x00000000,
};
extern "C" const u32 em016_turn_seq_c[16] = {
    0x00000014, 0x0000003C, 0x00000000, 0x80002AAB,
    0x00380000, 0x00000014, 0x00000000, 0x00000001,
    0x00260000, 0x0000000A, 0x00000000, 0x00000001,
    0x00270000, 0x0000000A, 0x00000000, 0x00000001,
};
extern "C" const u32 em016_turn_seq_d[16] = {
    0x0000000A, 0x00000028, 0x00000000, 0x80002AAB,
    0x00380000, 0x00000014, 0x00000000, 0x00000001,
    0x00330000, 0x0000000A, 0x00000000, 0x00000001,
    0x00340000, 0x0000000A, 0x00000000, 0x00000001,
};

/* The `.sdata` offsets the shifted sound records point at. */
EmSeOffset em016_se_shift_a = {0.0f, -10.0f};
EmSeOffset em016_se_shift_b = {-180.0f, 0.0f};
EmSeOffset em016_se_shift_c = {0.0f, -10.0f};

/* The select record an entry of kind 0x11 points at: a kind byte, a value and the sub-table it runs (`em017`'s alternate
 * sound program; field names a GUESS).
 * size: 0x10 */
struct Em016SeSelect {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ u32 value_0x04;
    /* +0x08 */ const EmSeEntry* table_0x08;
    /* +0x0C */ u32 pad_0x0C;
};
extern Em016SeSelect em017_se_alt_seq_05_select;

/* Rotates a per-kind offset into the work's position and writes the resulting relative vector the movement code
 * steers by. */
void em016_strafe_point_get(_ENEMY_WORK* self, u8 kind, VEC3* out) {
    VEC3 v;
    VEC3 rel;
    VEC3_ctor(&v);
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 2500.0f;
    u32 offset;
    switch (kind) {
    default:
        offset = 0x11C7;
        v.z = 2200.0f;
        break;
    case 1:
        offset = 0x9555;
        v.z = 2500.0f;
        break;
    case 2:
        offset = 0xAAAB;
        v.z = 2600.0f;
        break;
    case 10:
        offset = 0xA000;
        v.z = 1700.0f;
        break;
    case 11:
        offset = 0x4E39;
        v.z = 2200.0f;
        break;
    }
    rotVecY(&v, (u16)(self->field_0x1C0 + offset));
    addVec3(&rel, &self->pos, &v);
    copyVec3(out, &rel);
}

/* Forwards the value to the motion setter only for team 0x10. */
void em016_team16_motion_req(_ENEMY_WORK* self, u32 arg) {
    if (self->team == 0x10) {
        em_stamina_add(self, (s16)arg);
    }
}

/* Per-(map,area) motion start: two timed motion sets, then the area's pair. */
extern "C" void em016_area_route_set(_ENEMY_WORK* self) {
    em_motion_param_set(self, 0.0f, 0);
    em_motion_param_set(self, 1.0f, 0x1E);

    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 5:
            em_move_target_set(self, 6, 7, 300.0f);
            break;
        case 6:
            em_move_target_set(self, 9, 10, 100.0f);
            break;
        case 7:
            em_move_target_set(self, 0xD, 0xE, 300.0f);
            break;
        case 8:
            em_move_target_set(self, 8, 9, 300.0f);
            break;
        case 0xC:
            em_move_target_set(self, 0x19, 0x1A, 300.0f);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            em_move_target_set(self, 6, 7, 200.0f);
            break;
        case 2:
            em_move_target_set(self, 7, 8, 300.0f);
            break;
        case 3:
            em_move_target_set(self, 9, 10, 150.0f);
            break;
        case 4:
            em_move_target_set(self, 5, 6, 300.0f);
            break;
        case 5:
            em_move_target_set(self, 7, 8, 300.0f);
            break;
        case 6:
            em_move_target_set(self, 0xD, 0xE, 150.0f);
            break;
        case 7:
            em_move_target_set(self, 0, 2, 300.0f);
            break;
        case 8:
            em_move_target_set(self, 6, 7, 300.0f);
            break;
        case 10:
            em_move_target_set(self, 0, 1, 150.0f);
            break;
        }
        break;
    }
}

/* Whether this map/area pair has a motion set at all. */
extern "C" u32 em016_area_route_ck(_ENEMY_WORK* self) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((u32)(self->area_no - 5) <= 1U || (s32)self->area_no == 8) {
            return 1;
        }
        break;
    case 3:
        if ((s32)self->area_no == 1 || (s32)self->area_no == 5 || (s32)self->area_no == 7
            || (s32)self->area_no == 10) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Arms the motion that this map/area pair starts with. */
extern "C" void em016_opening_act_arm(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    em_act_arm_unless_act11(self, 6, 5);
    em_finish_flag_set(self);
}

/* 0x80183040 (0x400): Arms the record for the program it runs: enemy 016 builds its user-data helper and seeds the
 * aim block, enemy 017 clears its alert pair, and each picks the opening action for the map/area it starts in. */
extern "C" void em016_init(_ENEMY_WORK* self, u8 mode) {
    VEC3 offset;
    VEC3_ctor(&offset);
    if (self->team == 0x10) {
        if (em_res_user_data_ck(self) == 0) {
            Em016UserData* helper = (Em016UserData*)operator new(sizeof(Em016UserData));
            new (helper) Em016UserData;
            em_res_user_data_set(self, helper);
        }
        EmActWork* work = (EmActWork*)self;
        work->aim_0x328.angle_0x00 = 0xEAAC;
        work->aim_0x328.rot_0x04.x = 0xEE3A;
        work->aim_0x328.rot_0x04.y = 0xEE3A;
        work->aim_0x328.rot_0x04.z = 0;
        work->aim_0x328.rot_0x10.x = 0xEE3A;
        work->aim_0x328.rot_0x10.y = 0x11C7;
        work->aim_0x328.rot_0x10.z = 0;
        work->aim_0x328.vec_0x1C.x = 1.0f;
        work->aim_0x328.vec_0x1C.y = 1.0f;
        work->aim_0x328.vec_0x1C.z = 1.0f;
        self->tev_0x350 = 0xFF;
        self->tev_0x352 = 0xFF;
        self->tev_0x354 = 0xFF;
        self->tev_0x356 = 0xFF;
        self->tev_0x358 = 0xFF;
        self->tev_0x35A = 0;
        self->tev_0x35C = 0;
        self->tev_0x35E = 0;
        self->tev_0x35F = 0;
        if (self->field_0x009 == 0) {
            setVector3(&offset, 0.0f, -55.0f, 30.0f);
            eft_em_attach_set(self, 0x17, &offset, 0.8f, 0x1C72);
        }
        switch (mode) {
        case 0:
            switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 7:
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 0, 4);
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 2:
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 0, 4);
                    break;
                }
                break;
            case 9:
                switch (self->area_no) {
                case 1:
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 0, 4);
                    break;
                }
                break;
            case 11:
                em_move_mode_set(self, 2);
                em_act_arm_unless_down(self, 0, 4);
                break;
            }
            break;
        case 2:
            switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 6:
                    em_move_target_set(self, 0xE, 0xF, 0.0f);
                    em_move_mode_set(self, 0);
                    em_act_arm_unless_down(self, 2, 5);
                    break;
                case 7:
                    em_move_target_set(self, 0xD, 0xE, 0.0f);
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 5, 0x20);
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 2:
                    em_move_target_set(self, 6, 4, 0.0f);
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 5, 0x20);
                    break;
                case 3:
                    em_move_target_set(self, 0xF, 0x10, 0.0f);
                    em_move_mode_set(self, 2);
                    em_act_arm_unless_down(self, 5, 0x20);
                    break;
                }
                break;
            }
            em_state_refresh(self);
            break;
        }
    } else if (self->team == 0x11) {
        self->em017_0x328.alert_0x328 = 0;
        self->em017_0x328.ally_near_0x329 = 0;
        self->field_0x835 = 1;
        switch (mode) {
        case 0:
            if (self->field_0x00A == 1) {
                em_move_mode_set(self, 2);
                em_act_arm_unless_down(self, 0, 4);
            }
            break;
        case 2:
            em016_action_select((EmActWork*)self, &self->action, &self->state_sub);
            em_act_arm_unless_down(self, self->action, self->state_sub);
            em_state_refresh(self);
            break;
        }
    } else if (self->team == 0x15) {
        if (mode == 0 && self->field_0x00A == 1) {
            em_move_mode_set(self, 4);
            em_act_arm_unless_down(self, 0, 7);
        }
    }
}

/* 0x80183440 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em016UserData::Em016UserData() {
}

/* 0x8018347C (0x29C): Reacts to a program event `(kind, value)`: enemy 016 raises its flags, calls for help or
 * arms its countdown, enemy 017 latches the alert, enemy 021 sets its timer from the hit kind. */
extern "C" void em016_event_notify(_ENEMY_WORK* self, u8 kind, u8 value) {
    if (self->team == 0x10) {
        switch (kind) {
        case 1:
            switch (value) {
            case 5:
                em_act_flag1_set(self);
                break;
            case 7:
                em_bait_react(self);
                break;
            }
            break;
        case 2:
            switch (value) {
            case 6:
                if (em_busy_ck(self) == 1) {
                    s32 big = 0;
                    if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->area_no == 3) {
                        big = 1;
                    }
                    em_spawn_request(self->field_0x01A, 0x11, big, self->area_no, 0xFF, 2, 2, 9, 0xFF, 0, 0);
                }
                break;
            }
            break;
        case 5:
            switch (value) {
            case 0x16:
                self->tev_0x35C = 900;
                break;
            case 0x17:
                em_act_flag1_set(self);
                break;
            case 0x22:
            case 0x23:
                if (em_busy_ck(self) == 1) {
                    s32 big = 1;
                    if ((u8)stage_map_kind_get(self->field_0x1E0) == 1 && self->area_no == 0xC) {
                        big = 0;
                    }
                    em_spawn_request(self->field_0x01A, 0x11, big, self->area_no, 0xFF, 2, 2, 9, 0xFF, 0, 0);
                }
                break;
            }
            break;
        case 10:
            switch (value) {
            case 200:
            case 208:
                em_part_hit_set(self, 0, 0);
                break;
            }
            break;
        }
    } else if (self->team == 0x11) {
        switch (kind) {
        case 0xA:
            self->em017_0x328.alert_0x328 = 1;
            break;
        default:
            self->em017_0x328.alert_0x328 = 0;
            break;
        }
    } else if (self->team == 0x15) {
        switch (kind) {
        case 0xA:
            switch (value) {
            case 0x23:
            case 0x84:
            case 0x97:
            case 0x9E:
                self->timer_0x020 = 0;
                break;
            case 0x34:
            case 0x37:
                self->timer_0x020 = 0x3C;
                break;
            case 0xB2:
            case 0xB5:
                self->timer_0x020 = 0x12C;
                break;
            }
            break;
        }
    }
}

/* 0x80183718 (0x98): Enemy 016's per-frame bookkeeping: mirrors the effect state into bit 0x8000, runs the two
 * countdowns down and latches whether the record is in its second stance. */
extern "C" void em016_frame_em016(_ENEMY_WORK* self) {
    if (fn_8012EC3C_c1(self) == 1) {
        self->flags_0x836 |= 0x8000;
    } else {
        self->flags_0x836 &= 0x7FFF;
    }
    if (self->tev_0x35A > 0) {
        self->tev_0x35A--;
    }
    if (self->tev_0x35C > 0) {
        self->tev_0x35C--;
    }
    if (self->field_0x1E2 == 2) {
        self->field_0x38A = 1;
    } else {
        self->field_0x38A = 0;
    }
}

/* 0x801837B0 (0x2A4): Enemy 017's per-frame watch: settles its motion kind from the hit state, then every 32nd
 * frame scans the area for an enemy 016 to follow or a ranked ally close enough to rally to. */
extern "C" void em016_frame_em017(_ENEMY_WORK* self) {
    if (em_die_ck_u32(self) != 1) {
        if (self->field_0x1E2 == 2 && self->field_0x43B == 1) {
            em_motion_kind_set(self, 0, 0);
            self->field_0x834 = 0;
        }
        if (self->field_0x834 == 1 || em_hit_timer_ck(self) == 1) {
            em_motion_kind_set(self, 1, 0);
            if (em016_area_route_ck(self) == 1) {
                em_attack_start(self);
            }
        }
        if (self->field_0x43B != 1 && self->field_0x011 == 0) {
            if ((system_w.field_0x0c & 0x1F) == self->group) {
                _ENEMY_WORK* rec;
                s32 found;
                u16 max;
                u8 i;
                f32 limit;
                max = (u16)get_move_work_max(3);
                rec = (_ENEMY_WORK*)get_move_work_adrs(3);
                found = 0;
                self->em017_0x328.ally_near_0x329 = 0;
                i = 0;
                limit = 4000000.0f;
                for (; i < max; i++, rec++) {
                    if (rec->active != 0 && (rec->field_0x1C8 & 1) != 0 && self->area_no == rec->area_no) {
                        if (rec->team == 0x10) {
                            u32 stance;
                            switch (rec->field_0x1E2) {
                            case 0:
                                stance = 0;
                                break;
                            case 2:
                                stance = 1;
                                break;
                            default:
                                stance = 2;
                                break;
                            }
                            em_motion_kind_set(self, 4, stance);
                            copyVec3(&self->aim, &rec->pos);
                            found = 1;
                            break;
                        }
                        if (em_get_rank(rec) > 1 && rec->field_0x43D == 1) {
                            found = 1;
                            if (vec3_dist_sq(&self->pos, &rec->pos) < limit) {
                                if (self->field_0x43B == 0) {
                                    em_motion_kind_set(self, 3, 0);
                                }
                                copyVec3(&self->aim, &rec->pos);
                                self->em017_0x328.ally_near_0x329 = 1;
                                break;
                            }
                        }
                    }
                }
                if (found == 0) {
                    em_motion_kind_set(self, 0, 0);
                }
            }
            switch (self->field_0x43B) {
            case 2:
                if (self->field_0x440 > 0x1C2) {
                    em_motion_kind_set(self, 3, 0);
                }
                break;
            case 3:
                if (self->em017_0x328.ally_near_0x329 == 1 && self->em017_0x328.alert_0x328 == 1) {
                    em_motion_kind_set(self, 2, 0);
                }
                break;
            }
        }
    }
}

/* The map-0x15 (teardown) step: run it once `em_die_ck` and the state check agree. */
extern "C" void em016_frame_em021(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (em_hit_timer_ck(self) == 1) {
            em_attack_start(self);
        }
    }
}

/* Per-`team` step dispatch into the five armed-motion steps above (tail calls). */
extern "C" void em016_frame_update(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_frame_em016(self);
        return;
    case 0x11:
        em016_frame_em017(self);
        return;
    case 0x15:
        em016_frame_em021(self);
        return;
    }
}

/* The first of five armed-motion steps: phase 0 latches the state byte and arms the motion pair, phase 1
 * waits for `em_mot_end_ck` and runs the finish call. */
extern "C" void em016_idle_mot01(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_idle_mot14(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x14, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_idle_mot1D(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_idle_mot28(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_idle_mot36(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_idle_guard_motCA(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCA, 6, 0);
        em_height_set(self, 0.0f);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Per-`state_sub` step dispatch into the same five steps (tail calls). */
extern "C" void em016_idle_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_idle_mot01(self);
        return;
    case 1:
        em016_idle_mot14(self);
        return;
    case 2:
        em016_idle_mot1D(self);
        return;
    case 4:
        em016_idle_mot28(self);
        return;
    case 5:
        em016_idle_mot36(self);
        return;
    case 7:
        em016_idle_guard_motCA(self);
        return;
    }
}

/* The armed-motion step with the (0x1A, 0xA) pair. */
extern "C" void em016_look_mot1A(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The four-phase armed-motion step: arm, run the sub-action and its 0x708-frame timer, then count it down and
 * finish. */
extern "C" void em016_look_tint_hold(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2: {
        em_gauge_tick(self, 0.4f, 1, 0xF);
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            self->state++;
            em_mot_set(self, 5, 4, 0);
            em_tint_hold_end(self);
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

/* The first of four more armed-motion steps, the same two phases. */
extern "C" void em016_look_mot14(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x14, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_look_mot07(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_look_mot06(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 2, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void em016_look_mot1D(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step with two frame checks and its finish step. */
extern "C" void em016_look_roar(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 4, 0);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 72.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 1, 100.0f, 0.0f) == 1) {
            em_motion_timer_arm(self);
            em_fx_flag_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* The first of the second family's last two armed-motion steps. */
extern "C" void em016_look_mot0B(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em016_look_guard_roar(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        em_height_set(self, 0.0f);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 102.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
            em_motion_timer_arm(self);
            em_fx_flag_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* The first of the second family's three per-`state_sub` dispatchers (tail calls). */
extern "C" void em016_look_dispatch_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_look_mot1A(self);
        return;
    case 1:
        em016_look_tint_hold(self);
        return;
    case 2:
        em016_look_mot14(self);
        return;
    case 3:
        em016_look_mot07(self);
        return;
    case 4:
        em016_look_mot06(self);
        return;
    case 5:
        em016_look_mot1D(self);
        return;
    case 7:
        em016_look_mot0B(self);
        return;
    }
}

extern "C" void em016_look_dispatch_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        em016_look_mot14(self);
        return;
    case 6:
        em016_look_roar(self);
        return;
    }
}

extern "C" void em016_look_dispatch_em021(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        em016_look_mot14(self);
        return;
    case 3:
        em016_look_mot07(self);
        return;
    case 8:
        em016_look_guard_roar(self);
        return;
    }
}

/* Per-`team` dispatch into the second step family (tail calls). */
extern "C" void em016_look_dispatch(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_look_dispatch_em016(self);
        return;
    case 0x11:
        em016_look_dispatch_em017(self);
        return;
    case 0x15:
        em016_look_dispatch_em021(self);
        return;
    }
}

/* The armed-motion step that hands a float to `em_approach_start` and waits on `em_approach_step`. */
extern "C" void em016_walk_mot15(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same step, but the argument picks one of two action tables. */
extern "C" void em016_walk_turn_seq(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start_c(self, (u8)arg == 1 ? em016_turn_seq_b : em016_turn_seq_a, 0, 0, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step_c(self, (u8)arg == 1 ? em016_turn_seq_b : em016_turn_seq_a) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step whose phase 1 gates `em_turn_to_target` on an argument and a frame
 * check. */
extern "C" void em016_walk_mot1B(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 6, 0);
        break;
    }
    case 1:
        if ((u8)arg == 1 && em_frame_check(self, 3, 20.0f, 80.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step whose argument picks the motion pair and whether the target
 * distance is clamped. */
extern "C" void em016_walk_approach(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        u32 motion = (u8)arg1 == 1 ? 9 : 2;
        em_mot_set(self, (u16)motion, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        if ((u8)arg2 == 1 && self->value_0x378 > 700.0f) {
            self->value_0x378 = 700.0f;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step shared by two sub-states (`em_busy_set`/`em_busy_timer_reset` first). */
extern "C" void em016_walk_busy_approach(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);

    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 5, 5);
        }
        break;
    }
}

/* The armed-motion step whose argument picks the motion pair, the blend float and the
 * `em_approach_step` mask. */
extern "C" void em016_walk_approach_hit(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 0xD, 2);

        f32 blend;
        switch ((u8)arg2) {
        default:
            blend = 0.0f;
            break;
        case 1:
            blend = -600.0f;
            break;
        case 2:
            blend = 600.0f;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, (u16)((u8)arg1 == 1 ? 0xC0 : 0x40)) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step with the 0x96/0x5A countdown derived from `bits_0x1EC`. */
extern "C" void em016_walk_approach_timed(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 4, 0);

        u32 ticks = self->bits_0x1EC & 0x1F;
        self->timer_0x020 = ticks + 0x96;

        f32 blend;
        switch ((u8)arg) {
        default:
            blend = 0.0f;
            break;
        case 1:
            blend = -300.0f;
            break;
        case 2:
            blend = 0.0f;
            break;
        case 3:
            blend = -300.0f;
            self->timer_0x020 = ticks + 0x5A;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1: {
        u32 done = 0;
        if ((u8)arg != 2) {
            s32 left = self->timer_0x020 - 1;
            self->timer_0x020 = left;
            if (left <= 0) {
                done = 1;
            }
        }
        if (em_approach_step(self, 0, 0x40) == 1 || done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* The first of the third family's three per-`state_sub` dispatchers (tail calls; the dense ones are jump
 * tables). */
extern "C" void em016_walk_dispatch_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_walk_mot15(self);
        return;
    case 1:
        em016_walk_turn_seq(self, 0);
        return;
    case 2:
        em016_walk_mot1B(self, 0);
        return;
    case 3:
        em016_walk_approach(self, 0, 0);
        return;
    case 4:
        em016_walk_approach(self, 1, 0);
        return;
    case 5:
        em016_walk_busy_approach(self);
        return;
    case 6:
        em016_walk_turn_seq(self, 0);
        return;
    case 7:
        em016_walk_approach_hit(self, 0, 1);
        return;
    case 8:
        em016_walk_mot1B(self, 1);
        return;
    case 9:
        em016_walk_approach_hit(self, 1, 0);
        return;
    case 10:
        em016_walk_approach(self, 0, 1);
        return;
    case 11:
        em016_walk_approach(self, 1, 1);
        return;
    case 13:
        em016_walk_approach_hit(self, 0, 0);
        return;
    case 16:
        em016_walk_approach_hit(self, 0, 2);
        return;
    }
}

extern "C" void em016_walk_dispatch_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        em016_walk_mot1B(self, 0);
        return;
    case 3:
        em016_walk_approach(self, 0, 0);
        return;
    case 8:
        em016_walk_mot1B(self, 1);
        return;
    case 12:
        em016_walk_approach_timed(self, 1);
        return;
    case 14:
        em016_walk_approach_timed(self, 0);
        return;
    case 15:
        em016_walk_approach_timed(self, 2);
        return;
    case 17:
        em016_walk_turn_seq(self, 1);
        return;
    case 18:
        em016_walk_approach_timed(self, 3);
        return;
    }
}

extern "C" void em016_walk_dispatch_em021(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        em016_walk_mot1B(self, 0);
        return;
    case 3:
        em016_walk_approach(self, 0, 0);
        return;
    case 8:
        em016_walk_mot1B(self, 1);
        return;
    case 12:
        em016_walk_approach_timed(self, 1);
        return;
    case 14:
        em016_walk_approach_timed(self, 0);
        return;
    case 15:
        em016_walk_approach_timed(self, 2);
        return;
    case 17:
        em016_walk_turn_seq(self, 1);
        return;
    }
}

/* Per-`team` dispatch into the third step family (tail calls). */
extern "C" void em016_walk_dispatch(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_walk_dispatch_em016(self);
        return;
    case 0x11:
        em016_walk_dispatch_em017(self);
        return;
    case 0x15:
        em016_walk_dispatch_em021(self);
        return;
    }
}

/* `em016_swim_turn_seq`'s sibling with the other action table. */
extern "C" void em016_swim_turn_seq_b(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start_c(self, em016_turn_seq_c, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step_c(self, em016_turn_seq_c) == 1) {
            em_action_finish_swim(self);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, 5.0f);
        } else {
            em_height_follow(self, 10.0f);
        }
        break;
    }
}

/* The armed-motion step with the `field_0x482`-selected approach float. */
extern "C" void em016_swim_turn_seq(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start_c(self, em016_turn_seq_d, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step_c(self, em016_turn_seq_d) == 1) {
            em_action_finish_swim(self);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, 5.0f);
        } else {
            em_height_follow(self, 10.0f);
        }
        break;
    }
}

/* 0x80184D98 (0x360): The approach run: arms the approach for `kind` (its distance, speed and countdown), steps it
 * until it arrives or times out (the dive kind also stops at the surface), then plays the stop motion. */
extern "C" void em016_swim_approach(_ENEMY_WORK* self, u8 kind, u8 variant) {
    if (kind == 8) {
        em_frame_flag_set(self);
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        if (variant == 0) {
            em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        } else {
            em_mot_set_blend(self, 0x30, 0x14, 0, 1);
        }
        if (kind == 3) {
            self->vec_0x36C.y = self->field_0x210;
            self->timer_0x020 = 200;
        }
        switch (kind) {
        default:
            em_approach_start(self, -360.0f, 0x12);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 700.0f;
            break;
        case 2:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 1500.0f;
            break;
        case 4:
            em_approach_start(self, 0.0f, 0x12);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        case 5:
            em_approach_start(self, 0.0f, 0x12);
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x96;
            break;
        case 9:
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x5A;
        case 6:
            em_mot_speed_set(self, 2.0f);
            em_approach_start(self, -360.0f, 0x12);
            break;
        case 7:
            em_mot_speed_set(self, 3.0f);
            em_approach_start(self, -360.0f, 0x12);
            break;
        case 8:
            em_approach_start(self, 0.0f, 0x12);
            break;
        }
        break;
    case 1: {
        u32 done;
        switch (kind) {
        case 3:
            em_approach_step(self, 0, 0x80);
            em_height_follow(self, 3.0f);
            if (--self->timer_0x020 <= 0
                || self->pos.y >= self->vec_0x36C.y - (10.0f + em_float_depth_get(self)) * get_em_chg_scale(self)) {
                done = 1;
            } else {
                done = 0;
            }
            break;
        case 5:
        case 9:
            if (--self->timer_0x020 <= 0 || em_approach_step(self, 0, 0x80) == 1) {
                done = 1;
            } else {
                done = 0;
            }
            break;
        default:
            done = em_approach_step(self, 0, 0x80);
            break;
        }
        if (done == 1 && em_mot_frame_started_ck(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* The motion step that runs the angle-driven approach: two `em_frame_check` gates and a clamped blend of
 * `em_mot_frame_get`'s value. */
extern "C" void em016_swim_rise(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x39, 0xA, 0, 1);
        break;
    }
    case 1:
        em_height_follow(self, 10.0f);

        if (em_frame_check(self, 1, 28.0f, 0.0f) == 1) {
            f32 blend = 10.0f * (em_mot_frame_get(self) - 30.0f);
            if (blend > 100.0f) {
                blend = 100.0f;
            }
            em_surface_clamp(self, blend + em_float_depth_get(self));
        }

        if (em_frame_check(self, 1, 64.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80185218 (0x2C8): The leap out of the water: rises along the offset path until it clears the surface, splashes
 * and falls, then lands into the next state (or swims again when it drops back under). */
extern "C" void em016_swim_leap(_ENEMY_WORK* self, u8 reenter) {
    VEC3 offset;
    VEC3_ctor(&offset);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3C, 0x14, 0, 1);
        if (reenter == 1) {
            em_user_data_reenter(self);
        }
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
            em_fall_start_view1(self, 0.0f);
            em_mot_set(self, 0x40, 4, 0);
            if (self->team == 0x10) {
                eft009_spawn_at_joint(self, 0x15, 0x2C, 0, 1.2f);
            } else {
                eft009_spawn_at_joint(self, 0x15, 0x2D, 0, 1.2f);
            }
            setVector3(&offset, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x30, 0x15, &offset, 0.8f);
            setVector3(&offset, 0.0f, -75.0f, 0.0f);
            eft_em_spawn(self, 0x32, 3, &offset, 1.0f);
            self->field_0x320 = -4.0f;
        } else {
            em_surface_clamp(self, 0.0f);
        }
        break;
    case 3: {
        u32 landed = em_move_offset_step_update_view1(self, &self->field_0x1BC, 200.0f);
        if (self->field_0x314 < -40.0f) {
            self->field_0x314 = -40.0f;
        }
        if (landed == 1 && self->field_0x314 < 0.0f) {
            em_move_mode_set(self, 0);
            em_state_set(self, 5, 0x15);
        } else if (self->field_0x314 < 0.0f) {
            if (self->field_0x210 > self->pos.y + em_float_depth_get(self) * get_em_chg_scale(self)) {
                em_action_finish_swim(self);
            }
        }
        break;
    }
    }
}

/* 0x801854E0 (0x1D8): The jump onto land: crouches, springs along the offset path and either lands on the bank or
 * drops back into the water. */
extern "C" void em016_swim_bank_jump(_ENEMY_WORK* self, u8 reenter) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x3E, 0xA, 0, 1);
        if (reenter == 1) {
            em_user_data_reenter(self);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_view1(self, 205.0f);
            em_mot_set(self, 0x41, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 35.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
            self->field_0x320 = -5.5f;
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2: {
        u32 landed = em_move_offset_step_update_view1(self, &self->field_0x1BC, 360.0f);
        if (em_water_check_view1(self) == 1) {
            if (self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                em_state_set(self, 5, 0x16);
            }
        } else if (landed == 1 && self->field_0x314 < 0.0f) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0, 0);
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

/* The first of the fifth family's five armed-motion steps: arm the motion, then finish on `em_mot_end_ck`. */
extern "C" void em016_swim_mot2E(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_swim_mot35(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x35, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_swim_mot32(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_swim_mot31(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_swim_mot33(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80185938 (0x1D4): The rest: lies down and regenerates; every 30th frame a part whose frame cost is within reach
 * wakes it, and after 0x708 frames it gets up on its own. */
extern "C" void em016_swim_rest(_ENEMY_WORK* self) {
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
    case 2:
        em_gauge_period_step_c1(self, 1.0f, 5, 0x1E);
        if (++self->timer_0x020 >= 0x708) {
            self->timer_0x020 = 0x708;
        }
        if (em_busy_ck(self) == 1) {
            if (self->timer_0x020 % 30 * 5 == 0) {
                for (u8 i = 0; i < 4; i++) {
                    f32 cost = self->frames_0x454[i];
                    if (cost > 0.0f && cost <= 3000.0f) {
                        em_reaction_req_view1(self, 0, 2);
                        em_state_set(self, 5, 0x11);
                        return;
                    }
                }
            }
            if (self->timer_0x020 >= 0x708) {
                em_reaction_req(self, 5, 2, 5);
                em_status_set(self, 0);
                em_state_set(self, 5, 0x11);
            }
        }
        break;
    }
}

/* The motion step that stores the target angle byte: the difference between the body angle and `field_0x1C0`,
 * quantised into `state_0x007` (0 / 0x80 / the >>8 byte). */
extern "C" void em016_swim_turn_angle(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 0xA, 0);

        u32 diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff > 0x8000) {
            if (diff > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)diff >> 8);
        }
        break;
    }
    case 1:
        em_turn_in_window(self, 30.0f, 110.0f, (s32)(self->state_0x007 << 8));
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* One more step of the same family. */
extern "C" void em016_swim_mot3A(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80185C6C (0xF4): The short turn: plays the turn motion (the second one for `alt`), turns toward the target after
 * frame 100 and follows the surface height until the motion ends. */
extern "C" void em016_swim_short_turn(_ENEMY_WORK* self, u8 alt) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        u32 motion = alt == 1 ? 0x2C : 0x29;
        em_mot_set_blend(self, (u16)motion, 0x18, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        } else {
            if (em_frame_check(self, 2, 100.0f, 0.0f) == 1) {
                em_turn_to_target(self, 0x180);
            }
            if (self->field_0x482 != 0) {
                em_height_follow(self, 5.0f);
            } else {
                em_height_follow(self, 10.0f);
            }
        }
        break;
    }
}

/* The team-selected motion step (`em_busy_set` runs first). */
extern "C" void em016_swim_team_mot(_ENEMY_WORK* self) {
    em_busy_set(self);

    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(self->team == 0x10 ? 0x3D : 0x3B), 0, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80185DFC (0x1A4): The surfacing splash: snaps to the surface, spawns the splash effects for its species, then
 * plays the two surfacing motions. */
extern "C" void em016_swim_surface(_ENEMY_WORK* self) {
    VEC3 offset;
    VEC3_ctor(&offset);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_view1(self, 400.0f);
        em_mot_set_blend(self, 0x3F, 4, 0, 1);
        self->pos.y = self->field_0x210;
        setVector3(&offset, 0.0f, 0.0f, 0.0f);
        if (self->team == 0x10) {
            eft009_spawn_at_joint(self, 3, 0x2C, 0, 1.2f);
            eft_em_spawn(self, 0x30, 3, &offset, 0.8f);
        } else {
            eft009_spawn_at_joint(self, 3, 0x2D, 0, 1.2f);
            eft_em_spawn(self, 0xD1, 3, &offset, 0.8f);
        }
        setVector3(&offset, 0.0f, 0.0f, 15.0f);
        eft_em_spawn(self, 0x31, 3, &offset, 1.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0x42, 0, 0);
            em_surface_clamp(self, 300.0f);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

extern "C" void em016_swim_mot36(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80186020 (0x128): Enemy 016's movement dispatch on `state_sub` (0..37) into the swim, leap and approach steps. */
extern "C" void em016_swim_dispatch_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_swim_turn_seq_b(self);
        return;
    case 1:
        em016_swim_turn_seq(self);
        return;
    case 2:
        em016_swim_approach(self, 0, 0);
        return;
    case 3:
        em016_swim_rise(self);
        return;
    case 4:
        em016_swim_leap(self, 0);
        return;
    case 5:
        em016_swim_bank_jump(self, 0);
        return;
    case 6:
        em016_swim_mot2E(self);
        return;
    case 7:
        em016_swim_mot35(self);
        return;
    case 8:
        em016_swim_mot32(self);
        return;
    case 9:
        em016_swim_mot31(self);
        return;
    case 10:
        em016_swim_mot33(self);
        return;
    case 11:
        em016_swim_approach(self, 1, 0);
        return;
    case 12:
        em016_swim_approach(self, 2, 0);
        return;
    case 13:
        em016_swim_approach(self, 0, 1);
        return;
    case 14:
        em016_swim_approach(self, 1, 1);
        return;
    case 15:
        em016_swim_approach(self, 2, 1);
        return;
    case 16:
        em016_swim_rest(self);
        return;
    case 17:
        em016_swim_turn_angle(self);
        return;
    case 18:
        em016_swim_mot3A(self);
        return;
    case 19:
        em016_swim_short_turn(self, 0);
        return;
    case 20:
        em016_swim_short_turn(self, 1);
        return;
    case 21:
        em016_swim_team_mot(self);
        return;
    case 22:
        em016_swim_surface(self);
        return;
    case 23:
        em016_swim_mot36(self);
        return;
    case 24:
        em016_swim_approach(self, 3, 0);
        return;
    case 25:
        em016_swim_approach(self, 3, 1);
        return;
    case 26:
        em016_swim_approach(self, 4, 0);
        return;
    case 27:
        em016_swim_approach(self, 4, 1);
        return;
    case 31:
        em016_swim_approach(self, 8, 1);
        return;
    case 32:
        em016_swim_approach(self, 8, 0);
        return;
    case 34:
        em016_swim_turn_seq_b(self);
        return;
    case 35:
        em016_swim_turn_seq(self);
        return;
    case 36:
        em016_swim_leap(self, 1);
        return;
    case 37:
        em016_swim_bank_jump(self, 1);
        return;
    }
}

/* 0x80186148 (0x9C): Enemy 017's movement dispatch on `state_sub` into the shared swim steps. */
extern "C" void em016_swim_dispatch_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_swim_turn_seq_b(self);
        return;
    case 2:
        em016_swim_approach(self, 0, 0);
        return;
    case 3:
        em016_swim_rise(self);
        return;
    case 4:
        em016_swim_leap(self, 0);
        return;
    case 5:
        em016_swim_bank_jump(self, 0);
        return;
    case 8:
        em016_swim_mot32(self);
        return;
    case 10:
        em016_swim_mot33(self);
        return;
    case 19:
        em016_swim_short_turn(self, 0);
        return;
    case 20:
        em016_swim_short_turn(self, 1);
        return;
    case 21:
        em016_swim_team_mot(self);
        return;
    case 22:
        em016_swim_surface(self);
        return;
    case 28:
        em016_swim_approach(self, 5, 0);
        return;
    case 29:
        em016_swim_approach(self, 6, 0);
        return;
    case 30:
        em016_swim_approach(self, 7, 0);
        return;
    case 33:
        em016_swim_approach(self, 9, 0);
        return;
    }
}

/* 0x801861E4 (0x14): Picks the movement dispatch for the species. */
extern "C" void em016_swim_dispatch(_ENEMY_WORK* self) {
    if (self->team == 0x10) {
        em016_swim_dispatch_em016(self);
    } else {
        em016_swim_dispatch_em017(self);
    }
}

/* 0x801861F8 (0xD4): Rears up into the guard stance and holds it once the motion passes frame 110. */
extern "C" void em016_guard_rear_up(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_move_mode_set(self, 4);
            em_guard_set(self);
            self->field_0x38C = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x801862CC (0xD8): Drops out of the guard stance and stands. */
extern "C" void em016_guard_drop(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCB, 0, 0);
        em_height_set(self, 0.0f);
        em_guard_set(self);
        self->field_0x38C = 1;
        break;
    case 1:
        if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em_guard_clr(self);
            self->field_0x38C = 3;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801863A4 (0x94): Plays the guard-stance motion 0xCC and stands. */
extern "C" void em016_guard_motCC(_ENEMY_WORK* self) {
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCC, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80186438 (0xC8): Rears up through motion 0xCD and takes the guard stance after frame 120. */
extern "C" void em016_guard_rear_up_motCD(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCD, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            em_guard_set(self);
            self->field_0x38C = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x80186500 (0x94): Plays the guard-stance motion 0xCE and stands. */
extern "C" void em016_guard_motCE(_ENEMY_WORK* self) {
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80186594 (0xA8): Guarded idle motion 1 at the base height, then back to the opening action. */
extern "C" void em016_guard_idle(_ENEMY_WORK* self) {
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_guard_set(self);
        self->field_0x38C = 1;
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x8018663C (0xB8): Guarded turn through the second turn table, then back to the opening action. */
extern "C" void em016_guard_turn(_ENEMY_WORK* self) {
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start_c(self, em016_turn_seq_b, 0, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_guard_set(self);
        self->field_0x38C = 1;
        break;
    case 1:
        if (em_turn_seq_step_c(self, em016_turn_seq_b) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x801866F4 (0xEC): Guarded approach (backing off 600 for `back`), then back to the opening action. */
extern "C" void em016_guard_approach(_ENEMY_WORK* self, u8 back) {
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x59, 0, 0);
        em_mot_speed_set(self, 2.0f);
        f32 dist;
        if (back == 1) {
            dist = -600.0f;
        } else {
            dist = 0.0f;
        }
        em_approach_start(self, dist, 0);
        em_height_set(self, em_height_base_get(self));
        em_guard_set(self);
        self->field_0x38C = 1;
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x400) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x801867E0 (0xE8): Guarded look-around motion 0x1B (turning toward the target for `turn`). */
extern "C" void em016_guard_look(_ENEMY_WORK* self, u8 turn) {
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x1B, 6, 0);
        em_height_set(self, em_height_base_get(self));
        em_guard_set(self);
        self->field_0x38C = 1;
        break;
    case 1:
        if (turn == 1 && em_frame_check(self, 3, 20.0f, 80.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em016_opening_act_arm(self);
        }
        break;
    }
}

/* 0x801868C8 (0x98): Turns to face the target in motion 0xD3 and stands. */
extern "C" void em016_guard_face_target(_ENEMY_WORK* self) {
    em_fx_flag3_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xD3, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_turn_to_target(self, 0x300) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80186960 (0x68): The guard-stance dispatch on `state_sub` (0..11). */
extern "C" void em016_guard_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_guard_rear_up(self);
        return;
    case 1:
        em016_guard_drop(self);
        return;
    case 2:
        em016_guard_motCC(self);
        return;
    case 3:
        em016_guard_rear_up_motCD(self);
        return;
    case 4:
        em016_guard_motCE(self);
        return;
    case 5:
        em016_guard_idle(self);
        return;
    case 6:
        em016_guard_turn(self);
        return;
    case 7:
        em016_guard_approach(self, 0);
        return;
    case 8:
        em016_guard_approach(self, 1);
        return;
    case 9:
        em016_guard_look(self, 0);
        return;
    case 10:
        em016_guard_look(self, 1);
        return;
    case 11:
        em016_guard_face_target(self);
        return;
    }
}

/* 0x801869C8 (0xA4): The bite: motion 0x50 with its two hit windows. */
extern "C" void em016_atk_bite(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x50, 0xA, 0);
        em_hit_window_set(self, 0, 1, 8);
        em_hit_window_set(self, 1, 2, 0x10);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80186A6C (0xC8): The turning bite: motion 0x51, its two hit windows, and a turn within the window. */
extern "C" void em016_atk_turn_bite(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x51, 0xA, 0);
        em_hit_window_set(self, 0, 3, 8);
        em_hit_window_set(self, 1, 4, 0x10);
        em016_team16_motion_req(self, -0xF);
        break;
    case 1:
        em_turn_in_window(self, 64.0f, 128.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80186B34 (0x1D4): The spit: motion 0x57 (0x5B for `kind` 2) with its hit window, a sparkle every fourth frame of
 * the wind-up and the projectile request at frame 82. */
extern "C" void em016_atk_spit(_ENEMY_WORK* self, u8 kind) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        u32 motion = kind == 2 ? 0x5B : 0x57;
        em_mot_set(self, (u16)motion, 0xA, 0);
        em_hit_window_set_default(self, 0, 0x21);
        em016_team16_motion_req(self, -0xF);
        break;
    }
    case 1:
        if (kind != 2) {
            if (em_frame_check(self, 3, 12.0f, 60.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
                eft007_part_set(self, 0xD);
            }
            if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                switch (kind) {
                default:
                    em016_shot_req_init(&req, 0, 0x1000, 0);
                    break;
                case 1:
                    em016_shot_req_init(&req, 0, 0x400, 0);
                    break;
                }
                eft007_part_spawn(self, 0xE, 0, 0);
                ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 5);
                self->tev_0x35A = 300;
            }
        } else if (em_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 0x11, 0, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80186D08 (0x7D8): The lunge: picks the run-up motion for `kind` (the turn kinds pull the target point 500 units
 * back along the approach), clamps the heading while it closes in, arms and clears the hit windows through the
 * strike, then plays the recovery (a second strike for kind 2). */
extern "C" void em016_atk_lunge(_ENEMY_WORK* self, u8 kind) {
    VEC3 dir;
    VEC3_ctor(&dir);
    switch (self->state) {
    case 0: {
        u32 motion;
        s32 blend;
        em_move_mode_set(self, 2);
        if ((u32)(kind - 6) > 1) {
            switch (kind) {
            default:
                self->state += 2;
                motion = 0x46;
                em_approach_start(self, -100.0f, 0x12);
                blend = 0xA;
                break;
            case 3:
                self->state++;
                motion = 0x39;
                blend = 0xA;
                break;
            case 4:
                self->state++;
                motion = 0x29;
                blend = 0x18;
                break;
            case 5:
                self->state++;
                motion = 0x2C;
                blend = 0x18;
                break;
            }
        } else {
            self->state++;
            motion = kind == 6 ? 0x29 : 0x2C;
            blend = 0x18;
            VEC3 diff;
            subVec3(&diff, &self->vec_0x36C, &self->pos);
            copyVec3(&dir, &diff);
            if (vec3_length_sq(&dir.x) > 0.001f) {
                VEC3 back;
                vec3_normalize_into(&dir, &dir);
                vec3_scale(&back, &dir, 500.0f);
                vec3_sub_assign((Vec*)&self->vec_0x36C, (Vec*)&back);
            }
        }
        em_mot_set_blend(self, (u16)motion, blend, 0, 1);
        em016_team16_motion_req(self, -0xA);
        break;
    }
    case 1:
        if ((u32)(kind - 4) > 3) {
            u32 heading = (u16)self->field_0x1BC;
            if (heading > 0x1000 && heading < 0x8000) {
                self->field_0x1BC = 0x1000;
            } else if (heading > 0x8000 && heading < 0xF000) {
                self->field_0x1BC = 0xF000;
            }
            if (em_frame_check(self, 1, 70.0f, 0.0f) == 1) {
                em_turn_to_target(self, 0x30);
            }
            if (em_frame_check(self, 0, 130.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 0x45, 0, 0);
                em_approach_start(self, 0.0f, 0x12);
            }
        } else {
            em_turn_to_target(self, 0x180);
            if (self->field_0x482 != 0) {
                em_height_follow(self, 5.0f);
            } else {
                em_height_follow(self, 10.0f);
            }
            if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set_blend(self, 0x46, 0xF, 0x26, 1);
                em_approach_start(self, 0.0f, 0x12);
            }
        }
        break;
    case 2: {
        em_approach_step(self, 0, 0x20);
        u32 heading = (u16)self->field_0x1BC;
        if (heading > 0x1000 && heading < 0x8000) {
            self->field_0x1BC = 0x1000;
        } else if (heading > 0x8000 && heading < 0xF000) {
            self->field_0x1BC = 0xF000;
        }
        if (kind == 3) {
            if (em_frame_check(self, 0, 90.0f, 0.0f) == 1) {
                if (self->team == 0x10) {
                    em_hit_window_set(self, 0, 5, 3);
                    em_hit_window_set(self, 1, 0x11, 3);
                } else {
                    em_hit_window_set(self, 0, 3, 3);
                }
            }
        } else if (em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            if (self->team == 0x10) {
                em_hit_window_set(self, 0, 5, 3);
                em_hit_window_set(self, 1, 0x11, 3);
            } else {
                em_hit_window_set(self, 0, 3, 3);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x47, 8, 0, 1);
        }
        break;
    }
    case 3: {
        u32 heading = (u16)self->field_0x1BC;
        if (heading > 0x800 && heading < 0x8000) {
            self->field_0x1BC = 0x800;
        } else if (heading > 0x8000 && heading < 0xF800) {
            self->field_0x1BC = 0xF800;
        }
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            if ((u32)(kind - 1) > 1) {
                em_mot_set_blend(self, 0x48, 8, 0, 1);
            } else {
                em_mot_set_blend(self, 0x49, 8, 0, 1);
            }
        }
        break;
    }
    case 4:
        if ((u32)(kind - 1) > 1) {
            em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
            }
        } else {
            em_turn_in_window(self, 40.0f, 80.0f, 0x8000);
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
            }
            if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                if (self->team == 0x10) {
                    em_hit_window_set(self, 0, 5, 3);
                    em_hit_window_set(self, 1, 0x11, 3);
                } else {
                    em_hit_window_set(self, 0, 3, 3);
                }
            }
            if (kind == 2 && em_frame_check(self, 3, 40.0f, 110.0f) == 1) {
                em_turn_to_target(self, 0x100);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            switch (kind) {
            default:
                em_action_finish_swim(self);
                break;
            case 1:
                self->state++;
                em_mot_set_blend(self, 0x48, 8, 0, 1);
                break;
            case 2:
                self->state++;
                em_mot_set_blend(self, 0x4F, 8, 0, 1);
                if (self->team == 0x10) {
                    em_hit_window_set(self, 0, 0xF, 8);
                    em_hit_window_set(self, 1, 0x10, 0x10);
                }
                break;
            }
        }
        break;
    case 5:
        if (kind == 1) {
            if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 0);
            }
            if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
            }
            em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801874E0 (0xC4): The tail sweep: motion 0x4D with its hit window, turning toward the target until frame 60. */
extern "C" void em016_atk_tail_sweep(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4D, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 6);
        em016_team16_motion_req(self, -0xA);
        break;
    case 1:
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801875A4 (0xF0): The swimming bite: motion 0x4E with the species' two hit windows, following the surface height
 * until frame 50. */
extern "C" void em016_atk_swim_bite(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        u32 first;
        u32 second;
        if (self->team == 0x10) {
            first = 7;
            second = 8;
        } else {
            first = 5;
            second = 6;
        }
        em_hit_window_set(self, 0, first, 8);
        em_hit_window_set(self, 1, second, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            em_height_follow(self, 10.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80187694 (0xE8): The spinning tail strike: motion 0x52, turning toward the target early and swinging through
 * the two quarter-turn windows. */
extern "C" void em016_atk_spin_tail(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x52, 4, 0);
        em_hit_window_set_default(self, 0, 0xB);
        em016_team16_motion_req(self, -0xF);
        break;
    case 1:
        if (em_frame_check(self, 3, 14.0f, 50.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        em_turn_in_window(self, 48.0f, 68.0f, -0x4000);
        em_turn_in_window(self, 138.0f, 158.0f, 0x4000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018777C (0x128): The spray: motion 0x58 with two hit windows and the spray effect at frame 96 (the stronger
 * one while no effect is active). */
extern "C" void em016_atk_spray(_ENEMY_WORK* self) {
    VEC3 offset;
    VEC3_ctor(&offset);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x58, 4, 0);
        em_hit_window_set(self, 0, 0xC, 8);
        em_hit_window_set(self, 1, 0x15, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            setVector3(&offset, 0.0f, 20.0f, 240.0f);
            if (em_eft_active_ck(self) == 0) {
                eft_em_spawn(self, 0x50, 1, &offset, 1.2f);
            } else {
                eft_em_spawn(self, 0x24, 1, &offset, 1.0f);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801878A4 (0x140): The side slam: measures the turn toward the target point (at most 0x2000, its side in
 * `state_0x006`) and swings through it during the strike window. */
extern "C" void em016_atk_side_slam(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x5C, 4, 0);
        em_hit_window_set_default(self, 0, self->team == 0x10 ? 0xE : 7);
        em_target_pos_set(self, 0);
        em016_team16_motion_req(self, -0xF);
        u32 turn = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (turn > 0x8000) {
            turn = (u16)(0x10000 - turn);
            self->state_0x006 = 1;
        }
        if (turn > 0x2000) {
            turn = 0x2000;
        }
        self->state_0x007 = (s32)turn >> 8;
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else {
            s32 angle;
            if (self->state_0x006 == 1) {
                angle = -(self->state_0x007 << 8);
            } else {
                angle = self->state_0x007 << 8;
            }
            em_turn_in_window(self, 54.0f, 106.0f, angle);
        }
        break;
    }
}

/* 0x801879E4 (0x388): The water shot: picks the shot motion for `kind`, then at the release frame builds the shot
 * request (its side and spread from `kind`) and hands it to the shell spawner. */
extern "C" void em016_atk_water_shot(_ENEMY_WORK* self, u8 kind) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 2);
        u32 motion;
        switch (kind) {
        case 0:
        case 4:
            motion = 0x4A;
            break;
        default:
            motion = 0x4B;
            break;
        case 2:
            motion = 0x4C;
            break;
        case 3:
            motion = 0x44;
            break;
        }
        em_mot_set(self, motion, 4, 0);
        em016_team16_motion_req(self, -0xA);
        if (kind == 1 || kind == 6) {
            em_move_vec_clr(self);
            self->field_0x314 = -5.0f;
        }
        break;
    }
    case 1:
        switch (kind) {
        case 3:
            if (em_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                eft007_part_spawn(self, 0x14, 0, 0);
            }
            break;
        case 4:
            if (em_frame_check(self, 3, 22.0f, 64.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
                eft007_part_set(self, 0x23);
            }
            if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                em016_shot_req_init(&req, 0, 0xFE00, 0);
                eft007_part_spawn(self, 0xE, req.field_0x12, req.field_0x14);
                ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0x14);
                self->tev_0x35A = 300;
            }
            break;
        case 1:
        case 6:
            if (em_frame_check(self, 3, 20.0f, 64.0f) == 1) {
                em_move_offset_apply(self);
            }
        default:
            if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
                em_turn_to_target(self, 0x100);
            }
            if (em_frame_check(self, 3, 12.0f, 52.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
                eft007_part_set(self, 0x12);
            }
            if (em_frame_check(self, 0, 76.0f, 0.0f) == 1) {
                u8 side = 0;
                u32 mode;
                switch (kind) {
                default:
                    side = 2;
                case 0:
                    em016_shot_req_init(&req, side + 1, 0xF000, 0);
                    mode = 6;
                    break;
                case 6:
                    side = 2;
                case 1:
                    em016_shot_req_init(&req, side, 0, 0);
                    mode = 6;
                    break;
                case 7:
                    side = 2;
                case 2:
                    em016_shot_req_init(&req, side, 0x1000, 0);
                    mode = 6;
                    break;
                }
                eft007_part_spawn(self, 0x13, req.field_0x12, req.field_0x14);
                ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, mode);
                self->tev_0x35A = 300;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80187D6C (0x1D8): The backstep: backs off the distance for `kind`, then follows with the tail sweep, a new
 * action or the finish. */
extern "C" void em016_atk_backstep(_ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em016_team16_motion_req(self, -0xA);
        switch (kind) {
        case 0:
            em_approach_start(self, -670.0f, 0x10);
            break;
        case 1:
            em_approach_start(self, -400.0f, 0x10);
            break;
        case 2:
            em_approach_start(self, -480.0f, 0x10);
            break;
        default:
            em_approach_start(self, -480.0f, 0x10);
            break;
        }
        break;
    case 1:
        if (em_frame_check(self, 1, 10.0f, 0.0f) == 1
            && (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1)) {
            switch (kind) {
            case 0:
                self->state++;
                em_mot_set_blend(self, 0x4D, 0x12, 0x1E, 1);
                em_hit_window_set_default(self, 0, 6);
                break;
            case 1:
                em_act_set_unless_act11(self, 7, 5);
                break;
            case 2:
                em_act_set_unless_act11(self, 7, 0x20);
                break;
            default:
                em_action_finish_swim(self);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80187F44 (0x164): The delayed side slam: rears up, then at frame 85 measures the turn toward the target point
 * (at most 0x2000) and swings through it during the strike window. */
extern "C" void em016_atk_side_slam_delayed(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 6, 0);
        em016_team16_motion_req(self, -0xF);
        break;
    case 1:
        if (em_frame_check(self, 1, 85.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x5C, 0xA, 0x19, 1);
            em_hit_window_set_default(self, 0, 0xE);
            u32 turn = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (turn > 0x8000) {
                turn = (u16)(0x10000 - turn);
                self->state_0x006 = 1;
            }
            if (turn > 0x2000) {
                turn = 0x2000;
            }
            self->state_0x007 = (s32)turn >> 8;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else {
            s32 angle;
            if (self->state_0x006 == 1) {
                angle = -(self->state_0x007 << 8);
            } else {
                angle = self->state_0x007 << 8;
            }
            em_turn_in_window(self, 54.0f, 106.0f, angle);
        }
        break;
    }
}

/* 0x801880A8 (0xA4): The head butt: motion 0x62 with its two hit windows. */
extern "C" void em016_atk_head_butt(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x62, 0xA, 0);
        em_hit_window_set(self, 0, 0x16, 8);
        em_hit_window_set(self, 1, 0x17, 0x10);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018814C (0x2E4): The charging volley: advances 600 while firing four shots left and right of the heading, then
 * plays the recovery (motion 0xCA for `kind` 1). */
extern "C" void em016_atk_volley_charge(_ENEMY_WORK* self, u8 kind) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 0xA, 0);
        em_approach_start(self, 600.0f, 0);
        em_hit_window_set(self, 0, 0x18, 2);
        em016_team16_motion_req(self, -0xF);
        break;
    case 1:
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 208.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (self->state_0x006 == 0) {
            if (em_approach_step(self, 0, 0x40) == 1) {
                self->state_0x006++;
            }
        } else if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, (u16)(kind == 1 ? 0xCA : 0xC9), 0xA, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80188430 (0xB4): The body slam: motion 0x60 (0x61 for `kind` 1) with its hit window. */
extern "C" void em016_atk_body_slam(_ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        u32 motion;
        u32 window;
        if (kind == 1) {
            motion = 0x61;
            window = 0x1A;
        } else {
            motion = 0x60;
            window = 0x19;
        }
        em_mot_set(self, motion, 0xA, 0);
        em_hit_window_set_default(self, 0, window);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801884E4 (0x54C): The double spit: two layered motions per `kind`, three shots at frames 82, 142 and 202 and a
 * material blend that crossfades the body as the timer runs. */
extern "C" void em016_atk_double_spit(_ENEMY_WORK* self, u8 kind) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        u32 motion;
        u32 layer;
        switch (kind) {
        default:
            motion = 0x55;
            layer = 0x63;
            break;
        case 1:
            motion = 0x63;
            layer = 0x55;
            break;
        case 2:
            motion = 0x56;
            layer = 0x55;
            break;
        }
        em_mot_set(self, motion, 0xA, 0);
        em_mot_layer_set(self, 1, layer, 0xA, 0);
        em016_team16_motion_req(self, -0xF);
        self->timer_0x020 = 0;
        break;
    }
    case 1: {
        if (em_frame_check(self, 3, 12.0f, 70.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            eft007_part_set(self, 0xD);
        }
        if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            switch (kind) {
            default:
                em016_shot_req_init(&req, 0, 0, 0x1800);
                break;
            case 1:
                em016_shot_req_init(&req, 0, 0, 0);
                break;
            case 2:
                em016_shot_req_init(&req, 0, 0x800, 0xE000);
                break;
            }
            eft007_part_spawn(self, 0xE, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 5);
            self->tev_0x35A = 300;
        }
        if (em_frame_check(self, 0, 142.0f, 0.0f) == 1) {
            switch (kind) {
            default:
                em016_shot_req_init(&req, 0, 0xFA00, 0x400);
                break;
            case 1:
                em016_shot_req_init(&req, 0, 0xFA00, 0x1600);
                break;
            case 2:
                em016_shot_req_init(&req, 0, 0, 0x1800);
                break;
            }
            eft007_part_spawn(self, 0xE, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 5);
            self->tev_0x35A = 300;
        }
        if (em_frame_check(self, 1, 114.0f, 0.0f) == 1) {
            self->timer_0x020++;
        }
        f32 blend = 0.1f * self->timer_0x020;
        if (blend < 0.0f) {
            blend = 0.0f;
        } else if (blend > 1.0f) {
            blend = 1.0f;
        }
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 0, 1.0f - blend);
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 1, blend);
        if (em_frame_check(self, 0, 150.0f, 0.0f) == 1) {
            self->state++;
            u32 motion;
            u32 layer;
            switch (kind) {
            default:
                motion = 0x63;
                layer = 0x56;
                break;
            case 1:
                motion = 0x55;
                layer = 0x56;
                break;
            case 2:
                motion = 0x55;
                layer = 0x63;
                break;
            }
            em_mot_set(self, motion, 0, 0x97);
            em_mot_layer_set(self, 1, layer, 0, 0x97);
            em_hit_window_set_default(self, 0, 0x20);
            self->timer_0x020 = 0;
        }
        break;
    }
    case 2: {
        if (em_frame_check(self, 0, 202.0f, 0.0f) == 1) {
            switch (kind) {
            default:
                em016_shot_req_init(&req, 0, 0, 0xE400);
                break;
            case 1:
                em016_shot_req_init(&req, 0, 0x400, 0xE000);
                break;
            case 2:
                em016_shot_req_init(&req, 0, 0, 0);
                break;
            }
            eft007_part_spawn(self, 0xE, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 5);
            self->tev_0x35A = 300;
        }
        if (em_frame_check(self, 1, 174.0f, 0.0f) == 1) {
            self->timer_0x020++;
        }
        f32 blend = 0.1f * self->timer_0x020;
        if (blend < 0.0f) {
            blend = 0.0f;
        } else if (blend > 1.0f) {
            blend = 1.0f;
        }
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 0, 1.0f - blend);
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 1, blend);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* 0x80188A30 (0x9C4): The volley run: fires its four-shot volley toward the first of two strafe points for
 * `point`, dives sideways to the second, fires again (twice for any point but 1) and recovers (motion 0xCA for
 * `kind` 1). */
extern "C" void em016_atk_volley_run(_ENEMY_WORK* self, u8 point, u8 kind) {
    VEC3 target;
    EmSpawnRec req;
    VEC3_ctor(&target);
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 0;
        em_move_mode_set(self, 0);
        em016_team16_motion_req(self, -0xF);
        em016_strafe_point_get(self, (u8)(point * 0xA), &target);
        em_target_pos_set_view1(self, &target);
        em_mot_set(self, 0xC8, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        em_hit_window_set(self, 0, 0x18, 0xA);
        break;
    case 1:
        if (em_frame_check(self, 3, 40.0f, 60.0f) == 1) {
            em_turn_to_target(self, 0x200);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
        }
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 138.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 208.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            eft007_part_spawn(self, 0xF, req.field_0x12, req.field_0x14);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (self->timer_0x020 == 0) {
            if (em_approach_step(self, 0, 0x40) == 1) {
                self->timer_0x020++;
            }
        } else if (em_mot_end_ck(self) == 1) {
            self->state++;
            em016_strafe_point_get(self, (u8)(point * 0xA + 1), &target);
            em_target_pos_set_view1(self, &target);
            u32 turn = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            u32 motion;
            if (turn > 0x8000) {
                turn = (u16)(0x10000 - turn);
                self->state_0x006 = 1;
                motion = 0xCC;
            } else {
                self->state_0x006 = 0;
                motion = 0xCB;
            }
            self->state_0x007 = (s32)turn >> 8;
            em_mot_set(self, motion, 4, 0);
            em_move_vec_clr(self);
            self->field_0x318 = 15.6f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2: {
        if (em_frame_check(self, 3, 2.0f, 34.0f) == 1) {
            em_move_offset_apply(self);
        }
        s32 angle;
        if (self->state_0x006 == 1) {
            angle = -(self->state_0x007 << 8);
        } else {
            angle = self->state_0x007 << 8;
        }
        em_turn_in_window(self, 4.0f, 76.0f, angle);
        if (em_mot_end_ck(self) == 1) {
            if (point == 1) {
                self->state = 5;
            } else {
                self->state++;
            }
            self->timer_0x020 = 0;
            em_mot_set(self, 0xC8, 0, 0xA4);
            em_approach_start(self, 0.0f, 0);
            em_hit_window_set(self, 0, 0x18, 0x1A);
        }
        break;
    }
    case 3:
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 208.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (self->timer_0x020 == 0) {
            if (em_approach_step(self, 0, 0x40) == 1) {
                self->timer_0x020++;
            }
        } else if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_target_pos_set_view1(self, 0);
            u32 turn = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            u32 motion;
            if (turn > 0x8000) {
                turn = (u16)(0x10000 - turn);
                self->state_0x006 = 1;
                motion = 0xCC;
            } else {
                self->state_0x006 = 0;
                motion = 0xCB;
            }
            if (turn > 0x6AAB) {
                turn = 0x6AAB;
            } else if (turn < 0x471C) {
                turn = 0x471C;
            }
            self->state_0x007 = (s32)turn >> 8;
            em_mot_set(self, motion, 4, 0);
            em_move_vec_clr(self);
            self->field_0x318 = 15.6f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 4: {
        if (em_frame_check(self, 3, 2.0f, 34.0f) == 1) {
            em_move_offset_apply(self);
        }
        s32 angle;
        if (self->state_0x006 == 1) {
            angle = -(self->state_0x007 << 8);
        } else {
            angle = self->state_0x007 << 8;
        }
        em_turn_in_window(self, 4.0f, 76.0f, angle);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set(self, 0xC8, 0, 0xA4);
            em_approach_start(self, 0.0f, 0);
            em_hit_window_set(self, 0, 0x18, 0x12);
        }
        break;
    }
    case 5:
        if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0xE400);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (em_frame_check(self, 0, 208.0f, 0.0f) == 1) {
            em016_shot_req_init(&req, 0, 0, 0x1C00);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xA);
        }
        if (self->timer_0x020 == 0) {
            if (em_approach_step(self, 0, 0x40) == 1) {
                self->timer_0x020++;
            }
        } else if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, (u16)(kind == 1 ? 0xCA : 0xC9), 0xA, 0);
        }
        break;
    case 6:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801893F4 (0xC8): The swimming sweep: motion 0x43 with its hit window, swinging left then right across the two
 * windows. */
extern "C" void em016_atk_swim_sweep(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x43, 0xE, 0, 1);
        em_hit_window_set_default(self, 0, 0x22);
        em016_team16_motion_req(self, -0xA);
        break;
    case 1:
        em_turn_in_window(self, 0.0f, 70.0f, -0x1555);
        em_turn_in_window(self, 70.0f, 130.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801894BC (0x1B0): The bubble shot: motion 0xC8 and one shell request at the release frame (the larger bubble
 * for `kind` 0, the low one otherwise). */
extern "C" void em016_atk_bubble_shot(_ENEMY_WORK* self, u8 kind) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        switch (kind) {
        case 0:
            if (em_frame_check(self, 0, 66.0f, 0.0f) == 1) {
                VEC3 offset;
                req.id = 0x14;
                copyVec3(&req.pos, setVec3(&offset, 0.0f, 30.0f, 0.0f));
                req.field_0x10 = 0;
                req.field_0x12 = 0;
                req.field_0x14 = 0;
                eft007_part_spawn(self, 0x18, 0, 0);
                ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0xB);
            }
            break;
        default:
            if (em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
                VEC3 offset;
                req.id = 0x14;
                copyVec3(&req.pos, setVec3(&offset, 0.0f, 30.0f, -10.0f));
                req.field_0x10 = 0;
                req.field_0x12 = 0;
                req.field_0x14 = 0;
                eft007_part_spawn(self, 0x38, 0, 0);
                ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0x1A);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018966C (0x144): The standing bubble shot: turns toward the target, releases one shell request at frame 84 and
 * stands. */
extern "C" void em016_atk_bubble_shot_stand(_ENEMY_WORK* self) {
    EmSpawnRec req;
    em_spawn_rec_init(&req);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCF, 6, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 3, 10.0f, 34.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            VEC3 offset;
            req.id = 0x14;
            copyVec3(&req.pos, setVec3(&offset, 0.0f, 0.0f, 0.0f));
            req.field_0x10 = 0;
            req.field_0x12 = 0;
            req.field_0x14 = 0;
            eft007_part_spawn(self, 0x38, 0, 0);
            ((EmShellShotFn)shell_set_func_ptr->method_0x3C)(self, &req, 0x1A);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801897B0 (0x204): The guarded leap: holds the guard through the crouch, springs up and falls until it lands,
 * then plays the landing motion. */
extern "C" void em016_atk_guard_leap(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xD0, 0, 0);
        em_hit_window_set(self, 0, 9, 3);
        em_height_set(self, 0.0f);
        em_guard_set(self);
        self->field_0x38C = 1;
        em_fx_flag_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 66.0f, 0.0f) == 1) {
            em_fx_flag_set(self);
        }
        if (em_frame_check(self, 0, 68.0f, 0.0f) == 1) {
            em_guard_clr(self);
            self->field_0x38C = 3;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_view1(self, em_fall_height_get(self));
            em_mot_set(self, 0xD1, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 400.0f * get_em_scale(self);
            CancelFade(self);
            em_move_vec2_clr(self);
            self->field_0x314 = 50.0f;
            self->field_0x320 = -2.4f;
        }
        break;
    case 2:
        CancelFade(self);
        if (em_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_ground_ck_view1(self, em_fall_height_get(self)) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD2, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801899B4 (0xB4): The tail slam: motion 0x5E (0x5F for `kind` 1) with its hit window. */
extern "C" void em016_atk_tail_slam(_ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_move_mode_set(self, 0);
        u32 motion;
        u32 window;
        if (kind == 1) {
            motion = 0x5F;
            window = 0x13;
        } else {
            motion = 0x5E;
            window = 0x12;
        }
        em_mot_set(self, motion, 4, 0);
        em_hit_window_set_default(self, 0, window);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80189A68 (0x178): Enemy 016's attack dispatch on `state_sub` (0..48) into the strike steps. */
extern "C" void em016_atk_dispatch_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_atk_bite(self);
        return;
    case 1:
        em016_atk_turn_bite(self);
        return;
    case 2:
        em016_atk_spit(self, 0);
        return;
    case 3:
        em016_atk_lunge(self, 0);
        return;
    case 4:
        em016_atk_tail_sweep(self);
        return;
    case 5:
        em016_atk_swim_bite(self);
        return;
    case 6:
        em016_atk_lunge(self, 1);
        return;
    case 7:
        em016_atk_spin_tail(self);
        return;
    case 8:
        em016_atk_spray(self);
        return;
    case 9:
        em016_atk_spit(self, 2);
        return;
    case 10:
        em016_atk_side_slam(self);
        return;
    case 11:
        em016_atk_lunge(self, 2);
        return;
    case 12:
        em016_atk_lunge(self, 3);
        return;
    case 13:
        em016_atk_water_shot(self, 0);
        return;
    case 14:
        em016_atk_water_shot(self, 1);
        return;
    case 15:
        em016_atk_water_shot(self, 2);
        return;
    case 16:
        em016_atk_water_shot(self, 3);
        return;
    case 17:
        em016_atk_backstep(self, 0);
        return;
    case 18:
        em016_atk_backstep(self, 1);
        return;
    case 19:
        em016_atk_side_slam_delayed(self);
        return;
    case 20:
        em016_atk_head_butt(self);
        return;
    case 21:
        em016_atk_volley_charge(self, 0);
        return;
    case 22:
        em016_atk_volley_charge(self, 1);
        return;
    case 23:
        em016_atk_body_slam(self, 0);
        return;
    case 24:
        em016_atk_body_slam(self, 1);
        return;
    case 25:
        em016_atk_double_spit(self, 0);
        return;
    case 26:
        em016_atk_double_spit(self, 1);
        return;
    case 27:
        em016_atk_double_spit(self, 2);
        return;
    case 28:
        em016_atk_volley_run(self, 0, 0);
        return;
    case 29:
        em016_atk_volley_run(self, 0, 1);
        return;
    case 30:
        em016_atk_volley_run(self, 1, 0);
        return;
    case 31:
        em016_atk_spit(self, 1);
        return;
    case 32:
        em016_atk_swim_sweep(self);
        return;
    case 33:
        em016_atk_backstep(self, 2);
        return;
    case 34:
        em016_atk_water_shot(self, 4);
        return;
    case 35:
        em016_atk_volley_run(self, 1, 1);
        return;
    case 37:
        em016_atk_water_shot(self, 5);
        return;
    case 38:
        em016_atk_water_shot(self, 6);
        return;
    case 39:
        em016_atk_water_shot(self, 7);
        return;
    case 40:
        em016_atk_lunge(self, 4);
        return;
    case 41:
        em016_atk_lunge(self, 5);
        return;
    case 45:
        em016_atk_tail_slam(self, 0);
        return;
    case 46:
        em016_atk_tail_slam(self, 1);
        return;
    case 47:
        em016_atk_lunge(self, 6);
        return;
    case 48:
        em016_atk_lunge(self, 7);
        return;
    }
}

/* 0x80189BE0 (0x50): Enemy 017's attack dispatch on `state_sub`. */
extern "C" void em016_atk_dispatch_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_atk_bite(self);
        return;
    case 3:
        em016_atk_lunge(self, 0);
        return;
    case 5:
        em016_atk_swim_bite(self);
        return;
    case 10:
        em016_atk_side_slam(self);
        return;
    case 36:
        em016_atk_bubble_shot(self, 0);
        return;
    }
}

/* 0x80189C30 (0x4C): Enemy 021's attack dispatch on `state_sub`. */
extern "C" void em016_atk_dispatch_em021(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_atk_bite(self);
        return;
    case 10:
        em016_atk_side_slam(self);
        return;
    case 42:
        em016_atk_bubble_shot(self, 1);
        return;
    case 43:
        em016_atk_bubble_shot_stand(self);
        return;
    case 44:
        em016_atk_guard_leap(self);
        return;
    }
}

/* 0x80189C7C (0x30): Picks the attack dispatch for the species. */
extern "C" void em016_atk_dispatch(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_atk_dispatch_em016(self);
        return;
    case 0x11:
        em016_atk_dispatch_em017(self);
        return;
    case 0x15:
        em016_atk_dispatch_em021(self);
        return;
    }
}

/* The SE programs `em016_sound_prog_*` play, with their records. */
EmSeRecord em016_se_seq_18_rec0 = {0x79, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_18_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_17_rec0 = {0x78, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_17_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_1A_rec0 = {0x7A, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_seq_1A_rec1 = {0x7B, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em016_se_seq_1A_rec2 = {0x7C, 0x0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_1A[5] = {
    {0x00, {0, 0, 0}, &em016_se_seq_1A_rec0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em016_se_seq_1A_rec1},
    {0x00, {0, 0, 0}, &em016_se_seq_1A_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_1B_rec0 = {0x7D, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_seq_1B_rec1 = {0x7E, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em016_se_seq_1B_rec2 = {0x7F, 0x0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_1B[5] = {
    {0x00, {0, 0, 0}, &em016_se_seq_1B_rec0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em016_se_seq_1B_rec1},
    {0x00, {0, 0, 0}, &em016_se_seq_1B_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_7A_rec0 = {0x81, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_7A_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_7B_rec0 = {0x4, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_7B_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_9F_rec0 = {0x5, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_9F_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_7C_rec0 = {0x80, 0x0, 6, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_7C_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em017_se_seq_7D_rec0 = {0x85, 0x0, 6, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em017_se_seq_7D[2] = {
    {0x00, {0, 0, 0}, &em017_se_seq_7D_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_7E_rec0 = {0x7D, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_seq_7E_rec1 = {0x7E, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em016_se_seq_7E_rec2 = {0x7F, 0x0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_7E[4] = {
    {0x00, {0, 0, 0}, &em016_se_seq_7E_rec0},
    {0x00, {0, 0, 0}, &em016_se_seq_7E_rec1},
    {0x00, {0, 0, 0}, &em016_se_seq_7E_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_7F_rec0 = {0x7A, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_seq_7F_rec1 = {0x7B, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em016_se_seq_7F_rec2 = {0x7C, 0x0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_7F[4] = {
    {0x00, {0, 0, 0}, &em016_se_seq_7F_rec0},
    {0x00, {0, 0, 0}, &em016_se_seq_7F_rec1},
    {0x00, {0, 0, 0}, &em016_se_seq_7F_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_8D_rec0 = {0x82, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_8D_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_29_rec0 = {0x65, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_29[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_29_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_28_rec0 = {0x64, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_28[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_28_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_89_rec0 = {0x67, 0x0, 8, 0, 0, {0, 8, 0, 0}, 0};
EmSeEntry em016_se_seq_89[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xFA)},
    {0x00, {0, 0, 0}, &em016_se_seq_89_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_8A_rec0 = {0x67, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_8A[2] = {
    {0x01, {0, 0, 0}, &em016_se_seq_8A_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_8B_rec0 = {0x66, 0x0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_8B[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_8B_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_8C_rec0 = {0x6C, 0x0, 4, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_0};
EmSeRecord em016_se_seq_8C_rec1 = {0x0, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em016_se_seq_8C_rec2 = {0x6E, 0x0, 10, 0, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_1};
EmSeEntry em016_se_seq_8C[5] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x68)},
    {0x00, {0, 0, 0}, &em016_se_seq_8C_rec0},
    {0x01, {0, 0, 0}, &em016_se_seq_8C_rec1},
    {0x00, {0, 0, 0}, &em016_se_seq_8C_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em017_se_seq_8C_rec0 = {0x86, 0x0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em017_se_seq_8C[2] = {
    {0x00, {0, 0, 0}, &em017_se_seq_8C_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_8F_rec0 = {0x68, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_8F[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_8F_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_B1_rec0 = {0x65, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_B1[4] = {
    {0x1A, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x96)},
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x00, {0, 0, 0}, &em016_se_seq_B1_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_B6_rec0 = {0x88, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_B6_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_B7_rec0 = {0x89, 0x0, 6, 0, 1, {2, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_B7_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_B8_rec0 = {0x8A, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_B8[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_B8_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_B9_rec0 = {0x8B, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_B9_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_BA_rec0 = {0x8B, 0x0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em016_se_seq_BA_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_BB_rec0 = {0x8B, 0x0, 10, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em016_se_seq_BB_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_BF_rec0 = {0x79, 0x0, 4, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em016_se_seq_BF_rec1 = {0x80, 0x0, 6, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_BF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x50)},
    {0x00, {0, 0, 0}, &em016_se_seq_BF_rec0},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em016_se_seq_BF_rec1},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em016_se_seq_C0_rec0 = {0x64, 0x0, 4, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em016_se_seq_C0_rec1 = {0x66, 0x0, 6, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em016_se_seq_C0[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em016_se_seq_C0_rec0},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em016_se_seq_C0_rec1},
    {0xFE, {0, 0, 0}, 0},
};
EmSePos em016_se_seq_C8_pos0 = {94.0f, 140.0f, -1, -32768, 0};
EmSeRecord em016_se_seq_C8_rec1 = {0x84, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_C8[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em016_se_seq_C8_pos0},
    {0x00, {0, 0, 0}, &em016_se_seq_C8_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSePos em016_se_seq_D0_pos0 = {150.0f, 230.0f, -1, -32768, 0};
EmSeRecord em016_se_seq_D0_rec1 = {0x69, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_seq_D0[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em016_se_seq_D0_pos0},
    {0x00, {0, 0, 0}, &em016_se_seq_D0_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em017_se_seq_D6_rec0 = {0x85, 0x0, 6, 0, 0, {0, 4, 0, 0}, 0};
EmSeEntry em017_se_seq_D6[3] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xB4)},
    {0x00, {0, 0, 0}, &em017_se_seq_D6_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em021_se_seq_23_rec0 = {0x77, 0x0, 4, 0, 0, {0, 2, 1, 0}, &em016_se_shift_a};
EmSeRecord em021_se_seq_23_rec1 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_seq_23_rec2 = {0x74, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em021_se_seq_23_rec3 = {0x75, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_seq_23[6] = {
    {0x00, {0, 0, 0}, &em021_se_seq_23_rec0},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_seq_23_rec1},
    {0x00, {0, 0, 0}, &em021_se_seq_23_rec2},
    {0x00, {0, 0, 0}, &em021_se_seq_23_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em021_se_seq_34_rec0 = {0x71, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_seq_34_rec1 = {0x72, 0x0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_2};
EmSeRecord em021_se_seq_34_rec2 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_seq_34_rec3 = {0x74, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em021_se_seq_34_rec4 = {0x75, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_seq_34[8] = {
    {0x00, {0, 0, 0}, &em021_se_seq_34_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em021_se_seq_34_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_seq_34_rec2},
    {0x00, {0, 0, 0}, &em021_se_seq_34_rec3},
    {0x00, {0, 0, 0}, &em021_se_seq_34_rec4},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em021_se_seq_37_rec0 = {0x70, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_seq_37_rec1 = {0x72, 0x0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_2};
EmSeRecord em021_se_seq_37_rec2 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_seq_37_rec3 = {0x74, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em021_se_seq_37_rec4 = {0x75, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_seq_37[8] = {
    {0x00, {0, 0, 0}, &em021_se_seq_37_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em021_se_seq_37_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_seq_37_rec2},
    {0x00, {0, 0, 0}, &em021_se_seq_37_rec3},
    {0x00, {0, 0, 0}, &em021_se_seq_37_rec4},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x80189CAC (0x524): Enemy 016's sound program: plays the SE table of the current `state_sub`, or ends the action. */
extern "C" void em016_sound_prog_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em016_se_seq_17, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x18);
        return;
    case 0x19:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x19);
        return;
    case 0x1A:
        em_se_tbl_play(self, em016_se_seq_1A, 0, 0x1A);
        return;
    case 0x1B:
        em_se_tbl_play(self, em016_se_seq_1B, 0, 0x1B);
        return;
    case 0x1C:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x1C);
        return;
    case 0x1D:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x1D);
        return;
    case 0x1E:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x1E);
        return;
    case 0x28:
        em_se_tbl_play(self, em016_se_seq_28, 2, 0x28);
        return;
    case 0x29:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x29);
        return;
    case 0x2A:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2A);
        return;
    case 0x2B:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2B);
        return;
    case 0x2C:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2C);
        return;
    case 0x2D:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2D);
        return;
    case 0x2E:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2E);
        return;
    case 0x2F:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x2F);
        return;
    case 0x7A:
        em_se_tbl_play(self, em016_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em016_se_seq_7B, 0, 0x7B);
        return;
    case 0x89:
        em_se_tbl_play(self, em016_se_seq_89, 2, 0x89);
        return;
    case 0x8A:
        em_se_tbl_play(self, em016_se_seq_8A, 2, 0x8A);
        return;
    case 0x7C:
        em_se_tbl_play(self, em016_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8D);
        return;
    case 0x8B:
        em_se_tbl_play(self, em016_se_seq_8B, 2, 0x8B);
        return;
    case 0x8F:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0x8F);
        return;
    case 0x7E:
        em_se_tbl_play(self, em016_se_seq_7E, 0, 0x7E);
        return;
    case 0x7F:
        em_se_tbl_play(self, em016_se_seq_7F, 0, 0x7F);
        return;
    case 0x8E:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8E);
        return;
    case 0x8C:
        em_se_tbl_play(self, em016_se_seq_8C, 2, 0x8C);
        return;
    case 0x78:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x78);
        return;
    case 0x79:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x79);
        return;
    case 0x9F:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0xA0);
        return;
    case 0xA1:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0xA1);
        return;
    case 0xA2:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0xA2);
        return;
    case 0xA8:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0xA8);
        return;
    case 0xAA:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0xAA);
        return;
    case 0xB6:
        em_se_tbl_play(self, em016_se_seq_B6, 0, 0xB6);
        return;
    case 0xB7:
        em_se_tbl_play(self, em016_se_seq_B7, 0, 0xB7);
        return;
    case 0xB8:
        em_se_tbl_play(self, em016_se_seq_B8, 0, 0xB8);
        return;
    case 0xB9:
        em_se_tbl_play(self, em016_se_seq_B9, 0, 0xB9);
        return;
    case 0xBA:
        em_se_tbl_play(self, em016_se_seq_BA, 0, 0xBA);
        return;
    case 0xBB:
        em_se_tbl_play(self, em016_se_seq_BB, 0, 0xBB);
        return;
    case 0xBC:
        em_se_tbl_play(self, em016_se_seq_BB, 0, 0xBC);
        return;
    case 0xBF:
        em_se_tbl_play(self, em016_se_seq_BF, 0, 0xBF);
        return;
    case 0xC1:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0xC1);
        return;
    case 0xC0:
        em_se_tbl_play(self, em016_se_seq_C0, 2, 0xC0);
        return;
    case 0xC2:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0xC2);
        return;
    case 0xC8:
        em_se_tbl_play(self, em016_se_seq_C8, 0, 0xC8);
        return;
    case 0xD0:
        em_se_tbl_play(self, em016_se_seq_D0, 2, 0xD0);
        return;
    case 0xB1:
        em_se_tbl_play(self, em016_se_seq_B1, 2, 0xB1);
        return;
    default:
        if (em_water_check_view1(self) == 1) {
            em_action_finish_swim(self);
            return;
        }
        em_action_finish(self);
        return;
    }
}

/* 0x8018A1D0 (0x3F8): Enemy 017's sound program: plays the SE table of the current `state_sub`, or ends the action. */
extern "C" void em016_sound_prog_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {                        
    case 0x17:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x18);
        return;
    case 0x28:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x28);
        return;
    case 0x29:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0x29);
        return;
    case 0x7A:
        em_se_tbl_play(self, em016_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em016_se_seq_7B, 0, 0x7B);
        return;
    case 0x89:
        em_se_tbl_play(self, em016_se_seq_89, 2, 0x89);
        return;
    case 0x8A:
        em_se_tbl_play(self, em016_se_seq_8A, 2, 0x8A);
        return;
    case 0x7C:
        em_se_tbl_play(self, em016_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8D);
        return;
    case 0x8B:
        em_se_tbl_play(self, em016_se_seq_8B, 2, 0x8B);
        return;
    case 0x8F:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0x8F);
        return;
    case 0x7D:
        em_se_tbl_play(self, em017_se_seq_7D, 0, 0x7D);
        return;
    case 0x8E:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8E);
        return;
    case 0x8C:
        em_se_tbl_play(self, em017_se_seq_8C, 2, 0x8C);
        return;
    case 0x90:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0x90);
        return;
    case 0x9F:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0xA0);
        return;
    case 0xA1:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0xA1);
        return;
    case 0xA2:
        em_se_tbl_play(self, em016_se_seq_8F, 2, 0xA2);
        return;
    case 0xA8:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0xA8);
        return;
    case 0xAA:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0xAA);
        return;
    case 0xB1:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0xB1);
        return;
    case 0xD6:
        em_se_tbl_play(self, em017_se_seq_D6, 0, 0xD6);
        return;
    case 0xD8:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0xD8);
        return;
    case 0xDD:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0xDD);
        return;
    case 0xDF:
        em_se_tbl_play(self, em016_se_seq_29, 2, 0xDF);
        return;
    default:
        if (em_water_check_view1(self) == 1) {
            em_action_finish_swim(self);
            return;
        }
        em_action_finish(self);
        return;
    }
}

/* 0x8018A5C8 (0x3AC): Enemy 021's sound program: plays the SE table of the current `state_sub`, or ends the action. */
extern "C" void em016_sound_prog_em021(_ENEMY_WORK* self) {
    switch (self->state_sub) {                        
    case 0x17:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0x18);
        return;
    case 0x23:
        em_se_tbl_play(self, em021_se_seq_23, 1, 0x23);
        return;
    case 0x34:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0x34);
        return;
    case 0x37:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0x37);
        return;
    case 0x7A:
        em_se_tbl_play(self, em016_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em016_se_seq_7B, 0, 0x7B);
        return;
    case 0x7C:
        em_se_tbl_play(self, em016_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8D);
        return;
    case 0x7D:
        em_se_tbl_play(self, em017_se_seq_7D, 0, 0x7D);
        return;
    case 0x8E:
        em_se_tbl_play(self, em016_se_seq_8D, 0, 0x8E);
        return;
    case 0x84:
        em_se_tbl_play(self, em021_se_seq_23, 1, 0x84);
        return;
    case 0x97:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0x97);
        return;
    case 0x9E:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0x9E);
        return;
    case 0x9F:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em016_se_seq_9F, 0, 0xA0);
        return;
    case 0xA8:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play(self, em021_se_seq_23, 1, 0xA9);
        return;
    case 0xAB:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0xAB);
        return;
    case 0xAE:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0xAE);
        return;
    case 0xB2:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0xB2);
        return;
    case 0xB5:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0xB5);
        return;
    case 0xD6:
        em_se_tbl_play(self, em017_se_seq_D6, 0, 0xD6);
        return;
    case 0xD7:
        em_se_tbl_play(self, em021_se_seq_23, 1, 0xD7);
        return;
    case 0xD9:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0xD9);
        return;
    case 0xDC:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0xDC);
        return;
    case 0xDD:
        em_se_tbl_play(self, em016_se_seq_18, 0, 0xDD);
        return;
    case 0xDE:
        em_se_tbl_play(self, em021_se_seq_23, 1, 0xDE);
        return;
    case 0xE0:
        em_se_tbl_play(self, em021_se_seq_34, 0, 0xE0);
        return;
    case 0xE3:
        em_se_tbl_play(self, em021_se_seq_37, 0, 0xE3);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

/* 0x8018A974 (0x30): Picks the sound program for the species. */
extern "C" void em016_sound_prog(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_sound_prog_em016(self);
        return;
    case 0x11:
        em016_sound_prog_em017(self);
        return;
    case 0x15:
        em016_sound_prog_em021(self);
        return;
    }
}

/* The SE programs `em016_sound_alt_*` play, with their records. */
EmSeRecord em016_se_alt_seq_00_rec0 = {0x83, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_00[2] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_00_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_alt_seq_0F_rec0 = {0x8B, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_0F[3] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_0F_rec0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_alt_seq_1A_rec0 = {0x81, 0x0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_alt_seq_1A_rec1 = {0x4, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_1A_rec0},
    {0x00, {0, 0, 0}, &em016_se_alt_seq_1A_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_alt_seq_1B_rec0 = {0x67, 0x0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_1B[2] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_1B_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_alt_seq_1C_rec0 = {0x8B, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_1C_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em016_se_alt_seq_0A_rec0 = {0x6F, 0x0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em016_se_alt_seq_0A_rec1 = {0x70, 0x0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeEntry em016_se_alt_seq_0A[4] = {
    {0x00, {0, 0, 0}, &em016_se_alt_seq_0A_rec0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em016_se_alt_seq_0A_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em017_se_alt_seq_05_rec1 = {0x41, 0x0, 4, 34, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_3};
EmSeRecord em017_se_alt_seq_05_rec2 = {0x3B, 0x0, 0, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em017_se_alt_seq_05_rec3 = {0x83, 0x0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em017_se_alt_seq_05_sub_rec0 = {0x3F, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em017_se_alt_seq_05_sub_rec1 = {0x42, 0x0, 0, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em017_se_alt_seq_05_sub_rec2 = {0x6F, 0x0, 10, 0, 1, {0, 1, 0, 0}, 0};
EmSeRecord em017_se_alt_seq_05_sub_rec3 = {0x70, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em017_se_alt_seq_05[7] = {
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em017_se_alt_seq_05_select},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_rec2},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_rec3},
    {0x07, {0, 0, 0}, 0},
};
__declspec(section ".data") EmSeEntry em017_se_alt_seq_05_end[1] = {
    {0xFF, {0, 0, 0}, 0},
};
EmSeEntry em017_se_alt_seq_05_sub[8] = {
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_sub_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x2)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em016_se_shift_b},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x24)},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_sub_rec1},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_sub_rec2},
    {0x00, {0, 0, 0}, &em017_se_alt_seq_05_sub_rec3},
    {0x08, {0, 0, 0}, (const EmSeRecord*)&em017_se_alt_seq_05_end},
};
Em016SeSelect em017_se_alt_seq_05_select = {0x1, {0, 0, 0}, 0, em017_se_alt_seq_05_sub, 0};
EmSeRecord em021_se_alt_seq_05_rec0 = {0x77, 0x0, 4, 0, 0, {0, 2, 1, 0}, &em016_se_shift_c};
EmSeRecord em021_se_alt_seq_05_rec1 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_alt_seq_05_rec2 = {0x76, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_alt_seq_05[5] = {
    {0x00, {0, 0, 0}, &em021_se_alt_seq_05_rec0},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_05_rec1},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_05_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em021_se_alt_seq_0B_rec0 = {0x71, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_alt_seq_0B_rec1 = {0x72, 0x0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_2};
EmSeRecord em021_se_alt_seq_0B_rec2 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_alt_seq_0B_rec3 = {0x76, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_alt_seq_0B[7] = {
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0B_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0B_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0B_rec2},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0B_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em021_se_alt_seq_0E_rec0 = {0x70, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_alt_seq_0E_rec1 = {0x72, 0x0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em016_aim_vec_pair_2};
EmSeRecord em021_se_alt_seq_0E_rec2 = {0x73, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em021_se_alt_seq_0E_rec3 = {0x76, 0x0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em021_se_alt_seq_0E[7] = {
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0E_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0E_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0E_rec2},
    {0x00, {0, 0, 0}, &em021_se_alt_seq_0E_rec3},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x8018A9A4 (0xC4): Enemy 016's alternate sound program on `state_sub` (the idle program by default). */
extern "C" void em016_sound_alt_em016(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    case 10:
        em_se_tbl_play_alt(self, em016_se_alt_seq_0A, 2, 0xA);
        return;
    case 15:
        em_se_tbl_play_alt(self, em016_se_alt_seq_0F, 0, 0xF);
        return;
    case 28:
        em_se_tbl_play_alt(self, em016_se_alt_seq_1C, 0, 0x1C);
        return;
    case 26:
        em_se_tbl_play_alt(self, em016_se_alt_seq_1A, 0, 0x1A);
        return;
    case 27:
        em_se_tbl_play_alt(self, em016_se_alt_seq_1B, 2, 0x1B);
        return;
    default:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    }
}

/* 0x8018AA68 (0x70): Enemy 017's alternate sound program on `state_sub`. */
extern "C" void em016_sound_alt_em017(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt(self, em017_se_alt_seq_05, 1, 5);
        return;
    case 10:
        em_se_tbl_play_alt(self, em016_se_alt_seq_0A, 2, 0xA);
        return;
    default:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    }
}

/* 0x8018AAD8 (0x8C): Enemy 021's alternate sound program on `state_sub`. */
extern "C" void em016_sound_alt_em021(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt(self, em021_se_alt_seq_05, 1, 5);
        return;
    case 11:
        em_se_tbl_play_alt(self, em021_se_alt_seq_0B, 0, 0xB);
        return;
    case 14:
        em_se_tbl_play_alt(self, em021_se_alt_seq_0E, 0, 0xE);
        return;
    default:
        em_se_tbl_play_alt(self, em016_se_alt_seq_00, 0, 0);
        return;
    }
}

/* 0x8018AB64 (0x30): Picks the alternate sound program for the species. */
extern "C" void em016_sound_alt(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        em016_sound_alt_em016(self);
        return;
    case 0x11:
        em016_sound_alt_em017(self);
        return;
    case 0x15:
        em016_sound_alt_em021(self);
        return;
    }
}

/* 0x8018AB94 (0xD0): The plain approach: motion 0x37 until it arrives, then the stop motion. */
extern "C" void em016_route_approach(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x37, 0, 0);
        em_approach_start(self, 0.0f, 0x12);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x8018AC64 (0x88): Drops out of the guard stance (motion 0xCB) and stands. */
extern "C" void em016_route_guard_drop(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCB, 0, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x8018ACEC (0x7C): Plays motion 1 once and finishes. */
extern "C" void em016_route_mot01(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018AD68 (0x80): Plays the swim motion 0x28 once and ends the swim action. */
extern "C" void em016_route_swim(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x28, 0, 0, 2);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x8018ADE8 (0x94): Walks motion 2 up to the target and finishes. */
extern "C" void em016_route_walk(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018AE7C (0x348): Walks to the bank, aims at the area's water target, turns, and jumps in (landing in the water
 * goes straight to the swim state). */
extern "C" void em016_route_water_entry(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, -10.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 5:
                    em_move_target_set(self, 0xFF, 8, 300.0f);
                    break;
                case 6:
                    em_move_target_set(self, 0xFF, 0xB, 300.0f);
                    break;
                case 8:
                    em_move_target_set(self, 0xFF, 0xA, 300.0f);
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 1:
                    em_move_target_set(self, 0xFF, 8, 300.0f);
                    break;
                case 5:
                    em_move_target_set(self, 0xFF, 9, 300.0f);
                    break;
                case 7:
                    em_move_target_set(self, 0xFF, 2, 300.0f);
                    break;
                }
                break;
            }
            em_turn_seq_start_c(self, em016_turn_seq_b, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step_c(self, em016_turn_seq_b) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x3E, 0xA, 0, 1);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_view1(self, 205.0f);
            em_mot_set(self, 0x41, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 35.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
            self->field_0x320 = -5.5f;
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 4: {
        u32 landed = em_move_offset_step_update_view1(self, &self->field_0x1BC, 360.0f);
        if (em_water_check_view1(self) == 1) {
            if (self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                eft009_spawn_at_joint(self, 3, 0x2C, 0, 0.7f);
                em_state_set(self, 5, 0x16);
            }
        } else if (landed == 1 && self->field_0x314 < 0.0f) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0, 0);
        }
        break;
    }
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018B1C4 (0x94): Walks motion 0x59 up to the target and finishes. */
extern "C" void em016_route_walk_mot59(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 4, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018B258 (0x160): Walks up, aims at the area's target (map 3, area 10 only), turns, and walks on to it. */
extern "C" void em016_route_walk_target(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, -10.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
            case 3:
                switch (self->area_no) {
                case 0xA:
                    em_move_target_set(self, 0xFF, 2, 300.0f);
                    break;
                }
                break;
            }
            em_turn_seq_start_c(self, em016_turn_seq_b, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step_c(self, em016_turn_seq_b) == 1) {
            self->state++;
            em_mot_set(self, 0x59, 4, 0);
            em_approach_start(self, 0.0f, 0);
        }
        break;
    case 3:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8018B3B8 - `state_sub` 8 of the multi-motion dispatch: a tail call into the armed-motion step. */
void em016_route_busy_walk(_ENEMY_WORK* self) {
    em016_walk_busy_approach(self);
}

/* 0x8018B3BC - `state_sub` 9: the motion-table step with id 8, no sub. */
void em016_route_swim_approach(_ENEMY_WORK* self) {
    em016_swim_approach(self, 8, 0);
}

/* 0x8018B3C8 - the multi-motion dispatcher keyed on the work's +0x1E6 class (0..9). */
void em016_route_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_route_approach(self);
        return;
    case 1:
        em016_route_guard_drop(self);
        return;
    case 2:
        em016_route_mot01(self);
        return;
    case 3:
        em016_route_swim(self);
        return;
    case 4:
        em016_route_walk(self);
        return;
    case 5:
        em016_route_water_entry(self);
        return;
    case 6:
        em016_route_walk_mot59(self);
        return;
    case 7:
        em016_route_walk_target(self);
        return;
    case 8:
        em016_route_busy_walk(self);
        return;
    case 9:
        em016_route_swim_approach(self);
        return;
    default:
        return;
    }
}

/* The demo key tracks (frame, then the track's values per key). */
f32 em016_demo_opening_keys_a[94] = {
    506.0f, 7113.8f, 3348.87f, 510.0f,
    7124.1f, 3308.3f, 514.0f, 7139.69f,
    3246.93f, 518.0f, 7167.47f, 3137.54f,
    522.0f, 7200.21f, 3008.63f, 526.0f,
    7233.49f, 2877.56f, 530.0f, 7264.9f,
    2753.9f, 534.0f, 7295.39f, 2633.83f,
    538.0f, 7325.04f, 2517.08f, 542.0f,
    7356.12f, 2394.72f, 546.0f, 7382.38f,
    2291.3f, 550.0f, 7403.83f, 2206.84f,
    554.0f, 7422.25f, 2134.32f, 558.0f,
    7438.51f, 2070.32f, 562.0f, 7455.21f,
    2004.57f, 566.0f, 7472.1f, 1938.03f,
    570.0f, 7488.58f, 1873.14f, 574.0f,
    7504.03f, 1812.33f, 578.0f, 7517.81f,
    1758.05f, 582.0f, 7529.46f, 1712.2f,
    586.0f, 7539.94f, 1670.9f, 590.0f,
    7549.69f, 1632.52f, 594.0f, 7558.81f,
    1596.64f, 598.0f, 7567.42f, 1562.7f,
    602.0f, 7577.18f, 1524.29f, 606.0f,
    7586.99f, 1485.65f, 610.0f, 7595.29f,
    1452.99f, 614.0f, 7600.49f, 1432.51f,
    618.0f, 7601.41f, 1428.89f, 690.0f,
    7601.41f, 1428.89f, -1.0f, 7601.41f,
    1428.89f, 0.0f,
};
f32 em016_demo_opening_keys_b[176] = {
    690.0f, 7601.41f, -587.43f, 1428.89f,
    694.0f, 7599.01f, -597.59f, 1429.52f,
    698.0f, 7594.77f, -618.5f, 1428.07f,
    702.0f, 7591.07f, -635.83f, 1427.59f,
    706.0f, 7588.61f, -638.67f, 1434.81f,
    710.0f, 7582.51f, -638.67f, 1458.83f,
    714.0f, 7572.96f, -631.3f, 1481.68f,
    718.0f, 7562.66f, -623.92f, 1507.5f,
    722.0f, 7547.07f, -616.55f, 1554.12f,
    726.0f, 7519.29f, -609.17f, 1648.76f,
    730.0f, 7486.55f, -601.8f, 1762.92f,
    734.0f, 7453.27f, -594.42f, 1879.24f,
    738.0f, 7421.86f, -587.05f, 1988.15f,
    742.0f, 7391.37f, -579.68f, 2093.47f,
    746.0f, 7361.72f, -572.3f, 2195.47f,
    750.0f, 7330.64f, -564.93f, 2303.09f,
    754.0f, 7304.37f, -557.55f, 2391.75f,
    758.0f, 7282.93f, -550.18f, 2461.46f,
    762.0f, 7264.51f, -542.8f, 2519.24f,
    766.0f, 7248.25f, -535.43f, 2568.49f,
    770.0f, 7231.55f, -528.05f, 2619.49f,
    774.0f, 7214.65f, -520.68f, 2671.28f,
    778.0f, 7198.17f, -513.31f, 2721.43f,
    782.0f, 7182.73f, -505.93f, 2767.48f,
    786.0f, 7168.95f, -498.56f, 2807.02f,
    790.0f, 7157.3f, -491.18f, 2838.12f,
    794.0f, 7146.81f, -483.81f, 2864.66f,
    798.0f, 7137.07f, -476.43f, 2888.3f,
    802.0f, 7127.95f, -469.06f, 2909.43f,
    806.0f, 7119.33f, -461.68f, 2928.62f,
    810.0f, 7109.58f, -458.0f, 2959.66f,
    814.0f, 7099.77f, -458.0f, 2998.29f,
    818.0f, 7091.47f, -458.0f, 3030.96f,
    822.0f, 7086.27f, -458.0f, 3051.44f,
    826.0f, 7085.35f, -458.0f, 3055.06f,
    898.0f, 7085.35f, -458.0f, 3055.06f,
    902.0f, 7063.91f, -447.84f, 3057.57f,
    906.0f, 7019.79f, -426.92f, 3062.74f,
    910.0f, 6983.22f, -409.6f, 3067.03f,
    912.0f, 6977.23f, -406.76f, 3067.73f,
    936.0f, 6977.23f, -406.76f, 3067.73f,
    940.0f, 6977.23f, -244.27f, 3067.73f,
    943.0f, 6977.23f, -119.66f, 3067.73f,
    -1.0f, 6977.23f, -119.66f, 3067.73f,
};
f32 em016_demo_opening_keys_c[90] = {
    944.0f, 1295.11f, 333.97f, 948.0f,
    1262.11f, 371.89f, 952.0f, 1231.19f,
    406.07f, 956.0f, 1202.22f, 436.62f,
    960.0f, 1175.05f, 463.66f, 964.0f,
    1149.56f, 487.3f, 968.0f, 1125.59f,
    507.67f, 972.0f, 1103.01f, 524.87f,
    976.0f, 1081.69f, 539.03f, 980.0f,
    1061.47f, 550.25f, 984.0f, 1042.23f,
    558.66f, 988.0f, 1023.82f, 564.37f,
    992.0f, 1006.1f, 567.5f, 996.0f,
    988.93f, 568.15f, 1000.0f, 972.18f,
    566.46f, 1004.0f, 955.7f, 562.53f,
    1008.0f, 939.36f, 556.48f, 1012.0f,
    923.02f, 548.42f, 1016.0f, 906.53f,
    538.48f, 1020.0f, 889.76f, 526.76f,
    1024.0f, 873.49f, 511.58f, 1028.0f,
    857.97f, 491.54f, 1032.0f, 842.2f,
    467.17f, 1036.0f, 825.21f, 439.0f,
    1040.0f, 806.0f, 407.57f, 1044.0f,
    783.6f, 373.42f, 1048.0f, 757.01f,
    337.08f, 1052.0f, 725.26f, 299.09f,
    1055.0f, 697.47f, 269.84f, -1.0f,
    697.47f, 269.84f,
};

/* 0x8018B418 - the run's opening step machine (states 0..10) on the work's +0x5 byte. */
void em016_demo_opening(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        em_demo_reset(self, 0);
        em_demo_pos_set(self, 7946.29f, -637.43f, 2108.5f);
        em_demo_rot_set(self, 0.0f, -104.25f, 0.0f);
        return;
    case 1:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state++;
            em_mot_set(self, 0x37, 0, 0);
            em_demo_enable(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 30.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, 0.0f, 50.0f);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, 0.7f);
        }
        if (em_demo_time_ck(0x1DE) == 1U) {
            self->state++;
            em_mot_set_blend(self, 0x2C, 0x18, 0, 1);
            em_demo_pos_set(self, 7296.27f, -637.43f, 1943.41f);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 8.0f, 40.0f) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, 0.0f, 0.0f, -100.0f);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, 1.0f);
        }
        if (em_demo_time_ck(0x1FA) == 1U) {
            self->state++;
            em_mot_set(self, 0x2C, 0, 0x1E);
            em_demo_pos_set(self, 7113.8f, -587.43f, 3348.87f);
            em_demo_rot_set(self, 0.0f, -104.25f, 0.0f);
            em_demo_key_apply(self, em_demo_frame_get(), em016_demo_opening_keys_a, NULL, 5, 0);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, 48.0f, 112.0f) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, 0.0f, 0.0f, -100.0f);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, 0.8f);
        }
        if (em_frame_check(self, 0, 182.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, 0.0f, 80.0f);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, 0.7f);
        }
        self->pos.y = -587.43f;
        em_demo_key_apply(self, em_demo_frame_get(), em016_demo_opening_keys_a, NULL, 5, 0);
        if (em_demo_time_ck(0x2B2) == 1U) {
            self->state++;
            em_mot_set_blend(self, 0x29, 0xE, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 3, 8.0f, 40.0f) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, 0.0f, 0.0f, -100.0f);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, 1.0f);
        }
        if (em_frame_check(self, 3, 48.0f, 112.0f) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, 0.0f, 0.0f, -100.0f);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, 0.8f);
        }
        if (em_frame_check(self, 0, 182.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, 0.0f, 80.0f);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, 0.7f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_opening_keys_b, 0);
        if (em_demo_time_ck(0x382) == 1U) {
            self->state++;
            em_fall_start_view1(self, 0.0f);
            em_mot_set_blend(self, 0x3C, 0xE, 0x6C, 1);
            em_demo_pos_set(self, 7085.349f, -457.9968f, 3055.059f);
        }
        break;
    case 6:
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, 1.0f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_opening_keys_b, 0);
        if (em_demo_time_ck(0x3B0) == 1U) {
            self->state++;
            em_mot_set(self, 0x40, 0, 0);
            em_mot_speed_set(self, 0.6f);
            em_demo_pos_set(self, 1295.11f, 333.97f, -473.6f);
            em_demo_rot_set(self, 0.0f, -91.71f, 0.0f);
            em_demo_key_apply(self, em_demo_frame_get(), em016_demo_opening_keys_c, NULL, 3, 0);
            eft009_spawn_at_joint(self, 3U, 0x11U, 0, 0.8f);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 4.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, -75.0f, 0.0f);
            eft_em_spawn(self, 0x32, 3, &sp8, 0.6f);
        }
        em_demo_key_apply(self, em_demo_frame_get(), em016_demo_opening_keys_c, NULL, 3, 0);
        if (em_demo_time_ck(0x420) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3D, 0, 0);
            em_demo_pos_set(self, 530.997f, 398.066f, -1397.1093f);
            em_demo_rot_set(self, 0.0f, -165.75f, 0.0f);
        }
        break;
    case 8:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 0x15U, 0x24U, 0, 0.4f);
        }
        if (em_demo_time_ck(0x456) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, 404.177f, 47.9447f, -1965.6384f);
        }
        break;
    case 9:
        if (em_demo_time_ck(0x4A6) == 1U) {
            self->state++;
            em_mot_set_blend(self, 0x1A, 0xA, 0, 1);
        }
        break;
    case 10:
        if (em_demo_time_ck(0x582) == 1U) {
            self->state++;
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BBD8 - the first arm/step pair: a two-state opener for the motion-table step. */
void em016_demo_pose_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 404.177f, 47.9447f, -1965.6384f);
        em_demo_rot_set(self, 0.0f, -165.75f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BC7C - the run's second step machine (states 0..7). */
void em016_demo_run_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 278.4006f, 32.3998f, -1253.7802f);
        em_demo_rot_set(self, 0.0f, 60.0f, 0.0f);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state++;
            em_mot_set(self, 5, 0, 0x2A);
            em_demo_pos_set(self, 271.3262f, 32.3998f, -1250.1615f);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 106.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, 1.5f);
        }
        if (em_demo_time_ck(0x134) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, 365.6587f, 20.3288f, -1325.7823f);
            em_demo_rot_set(self, 0.0f, 54.75f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state++;
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, -460.2759f, 34.049f, -2140.9143f);
            em_demo_rot_set(self, 0.0f, 98.25f, 0.0f);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x4AE) == 1U) {
            self->state++;
            em_mot_set_blend(self, 2, 0x1E, 0, 1);
            return;
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 22.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, 1.5f);
        }
        if (em_frame_check(self, 0, 84.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 0xDU, 0x92U, 0, 1.5f);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BF4C - the second arm/step pair opener. */
void em016_demo_pose_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, -77.3448f, 34.049f, -2194.4153f);
        em_demo_rot_set(self, 0.0f, 98.25f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BFF0 - the run's third step machine (states 0..8). */
void em016_demo_run_3(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 330.8214f, -10.4453f, -754.9971f);
        em_demo_rot_set(self, 0.0f, -164.25f, 0.0f);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state++;
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x134) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, 330.8214f, -10.4453f, -754.9971f);
            em_demo_rot_set(self, 0.0f, -287.25f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state++;
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x420) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, 582.8813f, -10.4453f, -889.3009f);
            em_demo_rot_set(self, 0.0f, -281.4f, 0.0f);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            em_demo_pos_set(self, 582.8813f, -10.4453f, -889.3009f);
            em_demo_rot_set(self, 0.0f, -281.4f, 0.0f);
            return;
        }
        break;
    case 7:
        em_turn_in_window(self, 18.0f, 58.0f, 0x4000);
        if (em_demo_time_ck(0x49A) == 1U) {
            self->state++;
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
            em_demo_rot_set(self, 0.0f, -191.4f, 0.0f);
            return;
        }
        break;
    case 8:
        if (em_demo_time_ck(0x51E) == 1U) {
            self->state++;
            em_mot_set_blend(self, 0x14, 0x28, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C2CC - the third arm/step pair opener. */
void em016_demo_pose_3(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 0, 0);
        em_demo_pos_set(self, 582.8813f, -10.4453f, -889.3009f);
        em_demo_rot_set(self, 0.0f, -348.6414f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C370 - the run's fourth step machine (states 0..3). */
void em016_demo_run_4(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 868.4803f, -24.3509f, -793.0232f);
        em_demo_rot_set(self, 0.0f, -126.75f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
            break;
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 20.0f, 0.0f) == 1U) {
            setVector3(&sp8, 0.0f, -20.0f, 15.0f);
            eft009_spawn_at_pos(self, 8, 0x16, 0, 2.5f, &sp8);
        }
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state++;
            em_demo_reset(self, 0);
            break;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state++;
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, 816.1667f, 45.915f, -2474.896f);
            em_demo_rot_set(self, 0.0f, -67.5f, 0.0f);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C528 - the fourth arm/step pair opener. */
void em016_demo_pose_4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 816.1667f, 45.915f, -2474.896f);
        em_demo_rot_set(self, 0.0f, -67.5f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

f32 em016_demo_run5_keys[180] = {
    360.0f, -785.74f, -224.12f, -628.29f,
    361.0f, -783.15f, -201.0f, -591.21f,
    365.0f, -772.78f, -116.79f, -442.9f,
    369.0f, -762.3f, -45.65f, -293.01f,
    373.0f, -751.7f, 12.72f, -141.54f,
    377.0f, -741.11f, 58.61f, 9.93f,
    381.0f, -730.52f, 92.29f, 161.39f,
    385.0f, -719.93f, 114.05f, 312.86f,
    389.0f, -709.34f, 124.47f, 464.33f,
    393.0f, -698.75f, 129.8f, 615.8f,
    397.0f, -688.15f, 131.16f, 767.26f,
    401.0f, -677.56f, 126.82f, 918.73f,
    405.0f, -666.97f, 114.95f, 1070.2f,
    409.0f, -656.38f, 92.83f, 1221.67f,
    412.0f, -648.44f, 70.47f, 1335.27f,
    414.0f, -644.27f, 37.3f, 1419.94f,
    418.0f, -644.09f, 23.15f, 1490.21f,
    420.0f, -645.51f, 16.08f, 1541.52f,
    422.0f, -646.2f, 16.22f, 1561.91f,
    428.0f, -642.47f, 16.64f, 1599.4f,
    436.0f, -634.28f, 17.2f, 1639.11f,
    444.0f, -627.11f, 17.76f, 1672.12f,
    452.0f, -623.54f, 18.32f, 1703.1f,
    460.0f, -621.3f, 18.88f, 1729.95f,
    468.0f, -619.23f, 19.43f, 1757.3f,
    476.0f, -617.76f, 19.99f, 1785.14f,
    484.0f, -617.45f, 21.17f, 1803.19f,
    492.0f, -617.58f, 22.95f, 1816.49f,
    500.0f, -616.93f, 24.73f, 1837.04f,
    508.0f, -620.58f, 26.28f, 1855.96f,
    516.0f, -627.17f, 27.61f, 1867.47f,
    524.0f, -640.77f, 28.93f, 1872.81f,
    532.0f, -648.37f, 30.26f, 1874.4f,
    540.0f, -644.77f, 31.19f, 1872.72f,
    548.0f, -639.16f, 31.71f, 1869.34f,
    556.0f, -638.14f, 32.24f, 1866.08f,
    564.0f, -638.95f, 32.77f, 1863.27f,
    572.0f, -640.55f, 33.3f, 1861.17f,
    580.0f, -641.73f, 35.37f, 1860.79f,
    588.0f, -641.79f, 38.34f, 1863.01f,
    596.0f, -640.93f, 38.01f, 1873.66f,
    604.0f, -639.81f, 38.09f, 1889.36f,
    612.0f, -639.13f, 38.11f, 1902.16f,
    618.0f, -638.68f, 38.16f, 1908.33f,
    -1.0f, -638.68f, 38.16f, 1908.33f,
};

/* 0x8018C5CC - the run's fifth step machine (states 0..6). */
void em016_demo_run_5(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x168) == 1U) {
            self->state++;
            em_demo_enable(self);
            em_fall_start_view1(self, 0.0f);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run5_keys, 0);
            em_demo_rot_set(self, 0.0f, 4.0f, 0.0f);
            break;
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0xDU, 0x8000, 1.5f);
            eft009_spawn_at_joint(self, 0x13U, 0xFU, 0, 1.5f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run5_keys, 0);
        if (em_demo_time_ck(0x19E) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 0x3B, 6, 6, 1);
            break;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            sp8.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        0.4f * get_em_chg_scale(self));
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run5_keys, 0);
        if (em_demo_time_ck(0x242) == 1U) {
            self->state++;
            em_mot_set(self, 2, 0xE, 0x6A);
            break;
        }
        break;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run5_keys, 0);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state++;
            em_demo_pos_set(self, -633.9523f, 29.4907f, 1917.6456f);
            em_demo_rot_set(self, 0.0f, 4.0f, 0.0f);
        }
        /* fallthrough */
    case 5:
        if (em_demo_time_ck(0x57A) == 1U) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            em_mot_speed_set(self, 0.498f);
            em_demo_pos_set(self, -988.2443f, 47.7081f, 2752.0198f);
            em_demo_rot_set(self, 0.0f, 44.0f, 0.0f);
            break;
        }
        break;
    case 6:
        self->field_0x1C0 = em_angle_approach(0x49F5, (u16)self->field_0x1C0, 0x160);
        if (em_demo_time_ck(0x652) == 1U) {
            self->state++;
            em_mot_set(self, 0x14, 0xA, 0);
            em_demo_pos_set(self, -988.2443f, 47.7081f, 2752.0198f);
            em_demo_rot_set(self, 0.0f, 104.0f, 0.0f);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C998 - the fifth arm/step pair opener. */
void em016_demo_pose_5(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, -988.2443f, 47.7081f, 2752.0198f);
        em_demo_rot_set(self, 0.0f, 104.0f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

f32 em016_demo_run6_keys[196] = {
    450.0f, -909.67f, -159.31f, -197.49f,
    452.0f, -892.05f, -155.52f, -136.04f,
    454.0f, -874.44f, -144.84f, -74.6f,
    458.0f, -839.2f, -107.1f, 48.29f,
    462.0f, -803.96f, -54.56f, 171.18f,
    466.0f, -768.72f, 4.27f, 294.07f,
    470.0f, -733.49f, 60.9f, 416.95f,
    474.0f, -698.25f, 106.85f, 539.84f,
    476.0f, -680.63f, 123.16f, 601.29f,
    478.0f, -663.01f, 133.61f, 662.73f,
    480.0f, -645.39f, 138.54f, 724.17f,
    482.0f, -627.77f, 139.54f, 785.62f,
    484.0f, -610.15f, 137.41f, 847.06f,
    486.0f, -592.54f, 132.98f, 908.51f,
    488.0f, -574.92f, 127.05f, 969.95f,
    492.0f, -539.68f, 110.9f, 1092.84f,
    496.0f, -504.44f, 90.41f, 1215.73f,
    500.0f, -469.2f, 69.04f, 1338.61f,
    504.0f, -433.97f, 50.29f, 1461.5f,
    508.0f, -431.01f, 32.25f, 1481.23f,
    512.0f, -425.29f, 15.22f, 1522.46f,
    516.0f, -416.66f, 6.02f, 1567.75f,
    520.0f, -409.42f, 4.48f, 1585.01f,
    524.0f, -402.54f, 5.31f, 1599.07f,
    528.0f, -396.63f, 7.07f, 1609.87f,
    532.0f, -390.98f, 8.42f, 1620.65f,
    536.0f, -386.41f, 9.85f, 1631.26f,
    540.0f, -382.68f, 11.48f, 1642.13f,
    544.0f, -380.18f, 12.9f, 1650.23f,
    548.0f, -377.69f, 14.22f, 1658.5f,
    552.0f, -375.21f, 15.24f, 1666.93f,
    556.0f, -372.73f, 15.54f, 1675.56f,
    560.0f, -370.24f, 15.17f, 1684.33f,
    564.0f, -368.03f, 14.35f, 1693.31f,
    568.0f, -366.66f, 13.17f, 1699.79f,
    572.0f, -366.03f, 11.57f, 1703.74f,
    576.0f, -365.35f, 9.68f, 1707.77f,
    580.0f, -364.09f, 7.81f, 1714.18f,
    584.0f, -361.66f, 6.16f, 1723.66f,
    588.0f, -358.9f, 4.95f, 1733.64f,
    592.0f, -357.55f, 4.58f, 1743.89f,
    596.0f, -357.31f, 4.75f, 1753.58f,
    600.0f, -358.08f, 4.93f, 1761.94f,
    604.0f, -360.05f, 5.11f, 1768.29f,
    608.0f, -363.51f, 5.28f, 1772.63f,
    612.0f, -369.81f, 5.46f, 1776.68f,
    616.0f, -376.92f, 5.64f, 1780.05f,
    618.0f, -379.2f, 5.72f, 1781.06f,
    -1.0f, -379.2f, 5.72f, 1781.06f,
};
f32 em016_demo_run6_keys_b[141] = {
    450.0f, -4.0f, 15.36f, 454.0f,
    -2.83f, 15.55f, 458.0f, -1.79f,
    15.72f, 462.0f, -0.95f, 15.85f,
    464.0f, -0.61f, 15.9f, 466.0f,
    -0.35f, 15.95f, 468.0f, -0.16f,
    15.98f, 470.0f, -0.04f, 15.99f,
    472.0f, -0.0f, 16.0f, 474.0f,
    -0.05f, 16.0f, 476.0f, -0.2f,
    16.0f, 478.0f, -0.43f, 15.99f,
    482.0f, -1.09f, 15.96f, 486.0f,
    -1.91f, 15.89f, 490.0f, -2.74f,
    15.77f, 494.0f, -3.46f, 15.63f,
    496.0f, -3.73f, 15.57f, 498.0f,
    -3.92f, 15.52f, 500.0f, -4.02f,
    15.5f, 502.0f, -4.04f, 15.5f,
    516.0f, -4.04f, 15.5f, 518.0f,
    -4.19f, 15.46f, 520.0f, -4.61f,
    15.35f, 522.0f, -5.19f, 15.2f,
    526.0f, -6.52f, 14.86f, 528.0f,
    -7.1f, 14.72f, 530.0f, -7.51f,
    14.61f, 534.0f, -8.05f, 14.48f,
    538.0f, -8.47f, 14.37f, 542.0f,
    -8.78f, 14.29f, 546.0f, -8.98f,
    14.24f, 550.0f, -9.07f, 14.22f,
    554.0f, -8.94f, 14.25f, 558.0f,
    -8.57f, 14.34f, 562.0f, -8.09f,
    14.47f, 566.0f, -7.59f, 14.59f,
    570.0f, -6.98f, 14.75f, 574.0f,
    -6.28f, 14.92f, 578.0f, -5.55f,
    15.11f, 582.0f, -4.89f, 15.28f,
    584.0f, -4.61f, 15.35f, 586.0f,
    -4.37f, 15.41f, 588.0f, -4.19f,
    15.46f, 590.0f, -4.08f, 15.48f,
    592.0f, -4.04f, 15.5f, 618.0f,
    -4.04f, 15.5f, -1.0f, -4.04f,
    15.5f,
};

/* 0x8018CA3C - the run's sixth step machine (states 0..7). */
void em016_demo_run_6(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x1C2) == 1U) {
            self->state++;
            em_demo_enable(self);
            em_fall_start_view1(self, 0.0f);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, -4.0f, 15.36f, 0.0f);
            em_demo_key_apply(self, em_demo_frame_get(), em016_demo_run6_keys, em016_demo_run6_keys_b, 7, 3);
            em_hit_arm_set(self);
            return;
        }
        break;
    case 2:
        em_demo_key_apply(self, em_demo_frame_get(), em016_demo_run6_keys, em016_demo_run6_keys_b, 7, 3);
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x1F8) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0xC, 6);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            sp8.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        0.4f * get_em_chg_scale(self));
        }
        em_demo_key_apply(self, em_demo_frame_get(), em016_demo_run6_keys, em016_demo_run6_keys_b, 7, 3);
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state++;
            em_demo_pos_set(self, -417.0997f, 20.0f, 1653.6072f);
            em_demo_rot_set(self, 0.0f, 16.0f, 0.0f);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x29C) == 1U) {
            self->state++;
            em_mot_set_blend(self, 2, 0x14, 0, 1);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x566) == 1U) {
            self->state++;
            em_mot_set(self, 0x19, 0, 0x14);
            em_demo_pos_set(self, -318.8185f, 30.3854f, 2733.7803f);
            em_demo_rot_set(self, 0.0f, -54.0f, 0.0f);
            return;
        }
        break;
    case 6:
        em_turn_in_window(self, 20.0f, 62.0f, -0x4000);
        if (em_demo_time_ck(0x5B8) == 1U) {
            self->state++;
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, -318.8185f, 30.3854f, 2733.7803f);
            em_demo_rot_set(self, 0.0f, -144.0f, 0.0f);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x6F2) == 1U) {
            self->state++;
            em_mot_set(self, 0x14, 0xE, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018CDE0 - the sixth arm/step pair opener. */
void em016_demo_pose_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, -318.8185f, 30.3854f, 2733.7803f);
        em_demo_rot_set(self, 0.0f, -144.0f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

f32 em016_demo_run7_keys[96] = {
    1192.0f, -688.44f, -230.0f, -260.0f,
    1196.0f, -688.44f, -102.26f, -154.83f,
    1200.0f, -688.44f, 27.95f, -49.67f,
    1204.0f, -688.44f, 153.89f, 55.5f,
    1208.0f, -688.44f, 268.79f, 160.66f,
    1212.0f, -688.44f, 365.9f, 265.83f,
    1214.0f, -688.44f, 405.67f, 318.41f,
    1216.0f, -688.44f, 438.46f, 370.99f,
    1218.0f, -688.44f, 463.41f, 423.58f,
    1220.0f, -688.44f, 479.7f, 476.16f,
    1222.0f, -688.44f, 485.4f, 538.49f,
    1224.0f, -688.44f, 480.08f, 600.82f,
    1226.0f, -688.44f, 465.1f, 663.16f,
    1228.0f, -688.44f, 441.84f, 725.49f,
    1230.0f, -688.44f, 411.66f, 787.82f,
    1234.0f, -688.44f, 335.99f, 912.49f,
    1238.0f, -688.44f, 249.04f, 1037.15f,
    1242.0f, -688.44f, 161.72f, 1161.82f,
    1246.0f, -688.44f, 84.98f, 1286.48f,
    1248.0f, -689.82f, 55.29f, 1304.38f,
    1250.0f, -692.49f, 34.63f, 1329.54f,
    1252.0f, -696.61f, 23.87f, 1363.8f,
    1254.0f, -701.46f, 22.48f, 1404.73f,
    -1.0f, -701.46f, 22.48f, 1404.73f,
};
f32 em016_demo_run7_keys_b[123] = {
    1254.0f, -701.46f, 1404.73f, 1258.0f,
    -701.42f, 1420.14f, 1262.0f, -699.18f,
    1433.27f, 1266.0f, -696.42f, 1443.28f,
    1270.0f, -693.69f, 1449.94f, 1274.0f,
    -691.21f, 1456.51f, 1278.0f, -689.71f,
    1462.59f, 1282.0f, -689.1f, 1468.69f,
    1286.0f, -688.9f, 1471.78f, 1290.0f,
    -688.76f, 1474.99f, 1294.0f, -688.67f,
    1478.33f, 1298.0f, -688.64f, 1481.78f,
    1302.0f, -688.64f, 1485.35f, 1306.0f,
    -688.96f, 1489.03f, 1310.0f, -689.41f,
    1492.81f, 1314.0f, -689.88f, 1496.68f,
    1318.0f, -690.32f, 1500.65f, 1322.0f,
    -690.86f, 1507.11f, 1326.0f, -691.11f,
    1516.86f, 1330.0f, -691.2f, 1527.2f,
    1334.0f, -692.71f, 1537.42f, 1338.0f,
    -695.14f, 1546.8f, 1342.0f, -698.16f,
    1554.63f, 1346.0f, -701.79f, 1560.19f,
    1350.0f, -706.3f, 1563.4f, 1354.0f,
    -713.46f, 1565.57f, 1358.0f, -721.2f,
    1566.84f, 1362.0f, -724.96f, 1567.35f,
    1366.0f, -725.25f, 1567.18f, 1370.0f,
    -723.41f, 1566.45f, 1374.0f, -719.61f,
    1565.27f, 1378.0f, -716.63f, 1563.74f,
    1382.0f, -715.22f, 1562.12f, 1386.0f,
    -714.61f, 1560.53f, 1390.0f, -714.54f,
    1559.0f, 1394.0f, -714.86f, 1557.57f,
    1398.0f, -715.46f, 1556.29f, 1402.0f,
    -716.19f, 1555.18f, 1406.0f, -716.94f,
    1554.29f, 1410.0f, -717.57f, 1553.66f,
    -1.0f, -717.57f, 1553.66f,
};

/* 0x8018CE84 - the run's seventh step machine (states 0..5). */
void em016_demo_run_7(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x4A8) == 1U) {
            self->state++;
            em_demo_enable(self);
            em_fall_start_view1(self, 0.0f);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
            em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run7_keys, 0);
            break;
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0x10U, 0, 0.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run7_keys, 0);
        if (em_demo_time_ck(0x4DE) == 1U) {
            self->state++;
            em_mot_set_blend(self, 0x3B, 8, 6, 1);
            break;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            sp8.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        0.4f * get_em_chg_scale(self));
        }
        if (em_demo_time_ck(0x4E6) == 0) {
            em_demo_key3_apply(self, em_demo_frame_get(), em016_demo_run7_keys, 0);
        } else {
            em_demo_key_apply(self, em_demo_frame_get(), em016_demo_run7_keys_b, NULL, 5, 0);
        }
        if (em_demo_time_ck(0x584) == 1U) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
            em_demo_pos_set(self, -517.6882f, 34.5374f, 1857.308f);
            break;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x662) == 1U) {
            self->state++;
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, -519.1123f, 47.735f, 2150.03f);
            break;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x768) == 1U) {
            self->state++;
            em_mot_set(self, 1, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018D1AC - the seventh arm/step pair opener. */
void em016_demo_pose_7(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -519.1123f, 47.735f, 2150.03f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018D250 - the `state_sub` class dispatcher (0..13) into this unit's twelve step machines. */
void em016_demo_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em016_demo_opening(self);
        return;
    case 1:
        em016_demo_pose_1(self);
        return;
    case 2:
        em016_demo_run_2(self);
        return;
    case 3:
        em016_demo_pose_2(self);
        return;
    case 4:
        em016_demo_run_3(self);
        return;
    case 5:
        em016_demo_pose_3(self);
        return;
    case 6:
        em016_demo_run_4(self);
        return;
    case 7:
        em016_demo_pose_4(self);
        return;
    case 8:
        em016_demo_run_5(self);
        return;
    case 9:
        em016_demo_pose_5(self);
        return;
    case 10:
        em016_demo_run_6(self);
        return;
    case 11:
        em016_demo_pose_6(self);
        return;
    case 12:
        em016_demo_run_7(self);
        return;
    case 13:
        em016_demo_pose_7(self);
        return;
    default:
        return;
    }
}

/* 0x8018D2B0 - the action dispatcher (0..13) plus the two shared post-checks. */
void em016_action_dispatch(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em016_idle_dispatch(self);
        break;
    case 1:
        em016_look_dispatch(self);
        break;
    case 2:
        em016_walk_dispatch(self);
        break;
    case 5:
        em016_swim_dispatch(self);
        break;
    case 6:
        em016_guard_dispatch(self);
        break;
    case 7:
        em016_atk_dispatch(self);
        break;
    case 10:
        em016_sound_prog(self);
        break;
    case 11:
        em016_sound_alt(self);
        break;
    case 12:
        em016_route_dispatch(self);
        break;
    case 13:
        em016_demo_dispatch(self);
        break;
    }
    if (self->team != 0x15 && self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* 0x8018D370 - the per-frame effect spawner: measures the joint point against the water/height
 * gates and picks one of the four ground/air effect ids. */
void em016_frame_effect(_ENEMY_WORK* self) {
    VEC3 sp8;
    MTX34 sp18;
    u16 ang;

    VEC3_ctor(&sp8);
    MTX34_ctor(&sp18);
    if (em_alt_mode_ck_c1(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            switch (em_get_mot_no(self)) {
            case 0x4A:
            case 0x57:
            case 0x5B:
            case 0x63:
            case 0xC8:
                break;
            default:
                setVector3(&sp8, 0.0f, 0.0f, 50.0f);
                get_joint_wmat_em(self, 0x16, &sp18);
                mulVecMat(&sp8, &sp18);
                sp8.x += sp18.m[0][3];
                sp8.y += sp18.m[1][3];
                sp8.z += sp18.m[2][3];
                if (em_water_check_view1(self) == 1U && sp8.y < self->field_0x210) {
                    if ((system_w.field_0x0c & 0x1F) == 0) {
                        if ((u16)(ang + 0x8000) > 0x6E38U) {
                            eft_spawn_type10(self, 3, 0x16, &sp8, 0.7f);
                            return;
                        }
                        eft_spawn_type10(self, 5, 0x16, &sp8, 0.7f);
                    }
                } else if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38U) {
                        eft_spawn_type10(self, 0x1D, 0x16, &sp8, 1.0f);
                        return;
                    }
                    eft_spawn_type10(self, 0x1C, 0x16, &sp8, 1.0f);
                }
                break;
            }
        }
    }
}

/* 0x8018D558 - the effect-spawn router: `arg1` picks the position (+0x210 height, a joint, +0x20C height),
 * `arg3` the joint (0xFF: the work), then `eft009_set_pos` or `eft009_spawn_at_joint`. */
void em016_effect_spawn(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    VEC3 vec;
    u8 var;

    var = arg2;
    VEC3_ctor(&vec);
    switch (arg1) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (arg2) {
            case 2:
                var = 0xC;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xE, arg4, arg5);
                }
                break;
            case 4:
            case 6:
                var = 0x13;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xF, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xF, arg4, arg5);
                }
                break;
            default:
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            }
        } else {
            switch (arg2) {
            case 0xC:
            case 0xD:
            case 0xE:
            case 0xF:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x26:
                return;
            }
            if (arg3 == 0xFF) {
                vec.x = self->pos.x;
                vec.y = 5.0f + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (arg3 == 0xFF) {
            eft009_set_pos(var, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
            return;
        }
        eft009_spawn_at_joint(self, arg3, var, arg4, arg5);
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            switch (arg2) {
            case 0:
            case 3:
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x11, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                    return;
                }
                eft009_spawn_at_joint(self, arg3, 0x11, arg4, arg5);
                return;
            }
        } else {
            if (arg3 == 0xFF) {
                copyVec3(&vec, &self->pos);
            } else {
                get_joint_wpos_em(self, arg3, &vec);
            }
            vec.y = self->field_0x20C;
            arg5 *= get_em_chg_scale(self);
            eft_spawn_pos_in_area(&vec, self->area_no, arg2, arg4, arg5);
        }
        return;
    case 2:
        if (arg3 == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        arg5 *= get_em_chg_scale(self);
        if (self->pos.y <= 900.0f + self->field_0x20C) {
            eft_spawn_type11_view1(self, &vec, arg2, arg5);
        }
        return;
    default:
        return;
    }
}

/* 0x8018D8C8 - the per-(team, motion) frame-window driver: each `em_after_frame_check` window that fires
 * spawns through `em016_effect_spawn` or an effect call (`field_0x228 & 6` picks the large/air variant). */
void em016_motion_events(_ENEMY_WORK* self) {
    VEC3 sp2C;
    VEC3 sp20;
    VEC3 sp14;
    VEC3 sp8;
    f32 temp_f1;
    u16 temp_r3;
    u16 temp_r3_2;
    u16 temp_r3_3;
    u8 temp_r0;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    temp_r0 = self->team;
    switch ((s32) temp_r0) {
    case 16:
        em016_frame_effect(self);
        temp_r3 = em_get_mot_no(self);
        switch (temp_r3) {
        case 0x2:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
            }
            break;
        case 0x9:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.7f);
            }
            break;
        case 0x15:
            if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 32.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.9f);
            }
            if ((em_after_frame_check(self, 0, 46.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 80.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 170.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.9f);
            }
            break;
        case 0x16:
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.9f);
            }
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.9f);
            }
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            break;
        case 0x18:
            if ((em_after_frame_check(self, 0, 14.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 24.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            break;
        case 0x19:
            if ((em_after_frame_check(self, 0, 14.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 24.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            break;
        case 0x1B:
            if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 28.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.9f);
            }
            if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.9f);
            }
            break;
        case 0x2D:
            if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x46, 0x15, &sp14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0.6f, 0x3C);
            }
            break;
        case 0x33:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em_hit_window_set_default(self, 0, 9);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1U) {
                setVector3(&sp14, 120.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 8, 0xF, &sp14, 0.4f, 0x10);
                setVector3(&sp14, -140.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 8, 0x11, &sp14, 0.8f, 0x3C);
            }
            break;
        case 0x34:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em_hit_window_set_default(self, 0, 0xA);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1U) {
                setVector3(&sp14, -120.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 0xD, 0xF, &sp14, 0.4f, 0x10);
                setVector3(&sp14, -140.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 0xD, 0x11, &sp14, 0.8f, 0x3C);
            }
            break;
        case 0x39:
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1U) {
                em_eft_fx_spawn_ff(self, 0, 0x26, 0.7f, 8);
                sp14.x = 0.0f;
                sp14.y = -70.0f;
                sp14.z = 70.0f;
                ((EmShellJobFn)shell_set_func_ptr->method_0x2C)(self, 3, 1, &sp14, 0.5f, 0xFFFF);
            }
            break;
        case 0x3B:
            if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0xFU, self->field_0x1C0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 0xFU, self->field_0x1C0, 0.8f);
                }
            }
            break;
        case 0x3D:
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 1U, 3U, 3U, self->field_0x1C0, 1.0f);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x3E:
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 0x1DU, 0, 0.6f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 0x1DU, self->field_0x1C0, 0.6f);
                }
            }
            break;
        case 0x43:
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0x10, &sp14, 0.7f, 0x1E);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0.7f, 0x3C);
            }
            break;
        case 0x46:
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1U) {
                em_eft_fx_spawn_pair_ff(self, 2, 0x26, 0x15, 0.7f, 0x18);
            }
            break;
        case 0x47:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em_eft_fx_spawn_pair_ff(self, 2, 0x26, 0x15, 0.7f, 0x18);
            }
            break;
        case 0x49:
            if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1U) {
                em_eft_fx_spawn_pair_ff(self, 2, 0x26, 0x15, 0.7f, 0x18);
            }
            break;
        case 0x4D:
            if ((s32) (self->flags_0x836 & 1) == 0) {
                if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1U) {
                    em_eft_fx_spawn_anchor(self, 8, 0x28, 1.0f);
                }
                if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1U) {
                    em_eft_fx_spawn_anchor(self, 9, 0x29, 1.0f);
                }
                if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1U) {
                    setVector3(&sp14, 0.0f, 0.0f, 570.0f * get_em_chg_scale(self));
                    fn_80117E58_c1(self, 1, &sp14, 3, 0.7f * get_em_chg_scale(self));
                    setVector3(&sp14, 0.0f, 0.0f, 500.0f * get_em_chg_scale(self));
                    rotVecY(&sp14, self->field_0x1C0);
                    addVec3(&sp8, &self->pos, &sp14);
                    copyVec3(&sp2C, copyVec3(&sp20, &sp8));
                    temp_f1 = 370.0f * get_em_chg_scale(self);
                    sp2C.y -= temp_f1;
                    sp20.y += 370.0f * get_em_chg_scale(self);
                    ((EmShellLineJobFn)shell_set_func_ptr->method_0x30)(self, 4, &sp2C, &sp20, 0.5f, 0xFFFF);
                }
            }
            if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1U) {
                em_camera_req(self, 0x28, 7);
            }
            break;
        case 0x4E:
            if ((em_after_frame_check(self, 0, 44.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U)) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0.7f, 0x10);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0.7f, 0x3C);
            }
            break;
        case 0x4F:
            if ((em_after_frame_check(self, 0, 40.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1U)) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0.7f, 0x10);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0.7f, 0x3C);
            }
            break;
        case 0x50:
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.9f);
            }
            if (em_after_frame_check(self, 0, 116.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.9f);
            }
            break;
        case 0x51:
            if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0xDU, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0xDU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0xDU, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x25U, 0xE000, 0.8f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x25U, 0xE000, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x25U, 0, 0.8f);
                }
            }
            if (em_after_frame_check(self, 0, 176.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 1.0f);
            }
            break;
        case 0x52:
            if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 234.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.8f);
            }
            if ((em_after_frame_check(self, 0, 44.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 218.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0xDU, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0xDU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x21U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x21U, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x21U, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 3U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 3U, self->field_0x1C0, 0.8f);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x55:
        case 0x56:
            if (em_after_frame_check(self, 0, 280.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 284.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.3f);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x57:
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.3f);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x58:
            if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0xEU, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 0xEU, 0, 0.7f);
                }
                em_camera_req(self, 0xE, 1);
            }
            break;
        case 0x59:
            if ((em_after_frame_check(self, 0, 38.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x25U, 0, 0.8f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x25U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x25U, 0, 0.8f);
                }
            }
            if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 8U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 8U, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 8U, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x21U, 0, 0.8f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x21U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x21U, 0, 0.8f);
                }
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0xDU, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0xDU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0xDU, 0, 1.0f);
                }
            }
            break;
        case 0x5B:
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 168.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.3f);
            }
            break;
        case 0x5C:
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x26U, 0, 2.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x26U, 0, 2.0f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x26U, 0, 2.0f);
                }
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0x11U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 0x11U, self->field_0x1C0, 0.8f);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, 174.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 208.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            break;
        case 0x5E:
            if ((em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 162.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 3U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 3U, self->field_0x1C0, 0.8f);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.8f);
            }
            break;
        case 0x5F:
            if ((em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 162.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 112.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 3U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 3U, self->field_0x1C0, 0.8f);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.8f);
            }
            break;
        case 0x60:
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 6U, 0xDU, 0x2AAB, 1.2f);
            }
            break;
        case 0x61:
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 6U, 8U, 0xD555, 1.2f);
            }
            break;
        case 0x62:
            if ((em_after_frame_check(self, 0, 102.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x28U, 0, 1.5f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x28U, 0, 1.5f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x28U, 0, 1.5f);
                }
            }
            break;
        case 0x63:
            if (em_after_frame_check(self, 0, 280.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 284.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.3f);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x6F:
            if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0.7f, 0x3C);
            }
            if (em_after_frame_check(self, 0, 118.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0.7f, 0x3C);
            }
            if (em_after_frame_check(self, 0, 306.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x46, 3, &sp14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 418.0f, 0.0f) == 1U) {
                setVector3(&sp14, 0.0f, 0.0f, 80.0f);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0.7f, 0x3C);
            }
            break;
        case 0x79:
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            break;
        case 0x7A:
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 1U, 0U, 3U, self->field_0x1C0 + 0x4000, 1.2f);
            }
            break;
        case 0x7B:
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 3U, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x1DU, self->field_0x1C0, 1.5f);
                }
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 3U, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0xFU, self->field_0x1C0, 1.5f);
                }
            }
            break;
        case 0x7C:
            if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            break;
        case 0x7D:
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 1U, 0U, 3U, self->field_0x1C0 + 0xC000, 1.2f);
            }
            break;
        case 0x7E:
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 3U, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x1DU, self->field_0x1C0, 1.5f);
                }
            }
            if (em_after_frame_check(self, 0, 104.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 3U, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0xFU, self->field_0x1C0, 1.5f);
                }
            }
            break;
        case 0x7F:
            if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 0.8f);
            }
            if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x11U, 0, 1.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x11U, self->field_0x1C0, 1.5f);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.5f);
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, 326.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x10U, 0xFU, 0, 1.3f);
                } else {
                    em016_effect_spawn(self, 1U, 1U, 0xFU, self->field_0x1C0, 1.0f);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x84:
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0xFU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 1U, 3U, 3U, self->field_0x1C0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0xFU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x1DU, self->field_0x1C0, 1.5f);
                }
            }
            break;
        case 0x88:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 3U, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 1U, 2U, 3U, self->field_0x1C0, 1.0f);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x89:
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.2f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.2f);
            }
            break;
        case 0x8B:
            if (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 1U, 0U, 0xFU, self->field_0x1C0, 1.0f);
            }
            break;
        case 0xC8:
            if ((em_after_frame_check(self, 0, 84.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 218.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x25U, 0, 0.8f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x25U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x25U, 0, 0.8f);
                }
            }
            if ((em_after_frame_check(self, 0, 122.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 192.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
                em_camera_req(self, 8, 0);
            }
            if ((em_after_frame_check(self, 0, 116.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 182.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x21U, 0, 0.8f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x21U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x21U, 0, 0.8f);
                }
            }
            if ((em_after_frame_check(self, 0, 156.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 226.0f, 0.0f) == 1U)) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                em_camera_req(self, 0xD, 0);
            }
            break;
        case 0xC9:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 4U, 0xDU, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.3f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            break;
        case 0xCA:
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0xFU, self->field_0x1C0, 1.0f);
                } else {
                    em016_effect_spawn(self, 1U, 3U, 3U, self->field_0x1C0, 0.8f);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0xCB:
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 1.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0xEU, self->field_0x1C0, 1.5f);
                }
            }
            if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x21U, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            break;
        case 0xCC:
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 1.5f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0xEU, self->field_0x1C0, 1.5f);
                }
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 0x25U, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1U) {
                em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
            }
            break;
        }
        break;
    case 17:
        temp_r3_2 = em_get_mot_no(self);
        switch ((s32) temp_r3_2) {
        case 0x3B:
            if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x11U, 0x10U, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 0x10U, self->field_0x1C0, 0.4f);
                }
            }
            break;
        case 0x3C:
            if ((em_after_frame_check(self, 0, 136.0f, 0.0f) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                em016_effect_spawn(self, 0U, 0x11U, 0x10U, 0, 0.6f);
            }
            break;
        case 0x3E:
            if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x17U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0x17U, 0, 1.0f);
                }
            }
            break;
        case 0x3F:
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                em016_effect_spawn(self, 0U, 0x11U, 3U, 0, 0.8f);
                em016_effect_spawn(self, 0x27U, 0x30U, 3U, 0, 0.8f);
            }
            break;
        case 0x5C:
            if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0x17U, 0x8000, 1.0f);
                    em016_effect_spawn(self, 0U, 0xFU, 0x17U, 0x8000, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0x17U, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 2U, 2U, 0x10U, 0, 0.7f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0x10U, 0x8000, 1.0f);
                }
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x10U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x10U, self->field_0x1C0, 0.8f);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 8U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xCU, 0xDU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                }
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x10U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0x10U, 0, 1.2f);
                }
            }
            break;
        case 0x85:
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 0xDU, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 8U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
                }
            }
            break;
        case 0xC9:
            if ((em_after_frame_check(self, 0, 4.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 0xCU, 0, 0.6f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 0xCU, 0, 0.6f);
                }
            }
            if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xDU, 7U, 0, 0.6f);
                } else {
                    em016_effect_spawn(self, 0U, 0U, 7U, 0, 0.6f);
                }
            }
            if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 0.6f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0xEU, 0, 0.8f);
                }
            }
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0xEU, 0, 1.1f);
                }
            }
            if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0xEU, 0, 1.2f);
                }
            }
            if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0xEU, 0, 0.5f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0xEU, 0, 1.3f);
                }
            }
            break;
        }
        break;
    case 21:
        temp_r3_3 = em_get_mot_no(self);
        switch ((s32) temp_r3_3) {
        case 0x70:
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, 0.8f * get_em_scale(self));
            }
            em_shake_req_set(self);
            break;
        case 0x73:
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x10U, 0, 0.9f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0x10U, self->field_0x1C0, 1.4f);
                }
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x10U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 1U, 6U, 0x10U, self->field_0x1C0, 0.8f);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 8U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 0xCU, 0xDU, 0, 1.0f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
                    em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                }
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 0x10U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 6U, 0x10U, 0, 1.2f);
                }
            }
            break;
        case 0x85:
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 0xDU, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 0xDU, 0, 1.0f);
                }
            }
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0xCU, 8U, 0, 0.8f);
                } else {
                    em016_effect_spawn(self, 0U, 2U, 8U, 0, 1.0f);
                }
            }
            break;
        case 0xCD:
        case 0xC9:
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, 0.5f * get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x68U, 0, 0.5f * get_em_scale(self));
            }
            if ((em_after_frame_check(self, 3, 40.0f, 136.0f) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                eft009_spawn_at_joint(self, 0x13U, 0x69U, 0, 0.5f * get_em_scale(self));
            }
            break;
        case 0xD0:
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x66U, 0, get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, 0.8f * get_em_scale(self));
            }
            break;
        case 0xD2:
            if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    em016_effect_spawn(self, 0U, 0x14U, 3U, 0, 0.7f);
                } else {
                    em016_effect_spawn(self, 1U, 4U, 3U, self->field_0x1C0, 0.25f);
                }
            }
            break;
        }
        break;
    }
    if ((u8) self->team == 0x10) {
        fn_80191EF8_view1(self);
        fn_80192080_view1(self);
    }
}

/* 0x80191038 - the team-0x10 TEV tint stepper: fades the two +0x350/+0x352 highlight words and the
 * +0x354..+0x358 fade triple toward the team's key colour. */
void em016_tev_color_update(_ENEMY_WORK* self) {
    _GXColor color;
    s16 v;
    u8 dim;
    u8 flag;

    if (self->team == 0x10) {
        dim = fn_8012EC3C_c1(self) == 1;
        flag = self->field_0x1E2 == 2;
        if (dim == 1) {
            self->tev_0x350 -= 2;
            if (self->tev_0x350 < 0xAA) {
                self->tev_0x350 = 0xAA;
            }
            self->tev_0x352 -= 2;
            if (self->tev_0x352 < 0xB4) {
                self->tev_0x352 = 0xB4;
            }
        } else {
            self->tev_0x350 += 2;
            if (self->tev_0x350 > 0xFF) {
                self->tev_0x350 = 0xFF;
            }
            self->tev_0x352 += 2;
            if (self->tev_0x352 > 0xFF) {
                self->tev_0x352 = 0xFF;
            }
        }
        v = self->tev_0x350;
        color.b = (u8)v;
        color.g = (u8)v;
        color.r = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
        v = self->tev_0x352;
        color.b = (u8)v;
        color.g = (u8)v;
        color.r = (u8)v;
        ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
        if (flag == 1) {
            self->tev_0x354 += 7;
            if (self->tev_0x354 > 0xFF) {
                self->tev_0x354 = 0xFF;
            }
            self->tev_0x356 += 6;
            if (self->tev_0x356 > 0xFF) {
                self->tev_0x356 = 0xFF;
            }
            self->tev_0x358 += 6;
            if (self->tev_0x358 > 0xFF) {
                self->tev_0x358 = 0xFF;
            }
        } else {
            self->tev_0x354 -= 7;
            if (self->tev_0x354 < 0x28) {
                self->tev_0x354 = 0x28;
            }
            self->tev_0x356 -= 6;
            if (self->tev_0x356 < 0x3C) {
                self->tev_0x356 = 0x3C;
            }
            self->tev_0x358 -= 6;
            if (self->tev_0x358 < 0x3C) {
                self->tev_0x358 = 0x3C;
            }
        }
        color.r = (u8)self->tev_0x354;
        color.g = (u8)self->tev_0x356;
        color.b = (u8)self->tev_0x358;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
        if (em_flags836_ck(self, 1) == 1U) {
            if (self->tev_0x35E == 1) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xFF;
                color.a = 0;
            } else {
                color.r = 0x28;
                color.g = 0x3C;
                color.b = 0x3C;
                color.a = 0;
            }
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
        } else {
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
            self->tev_0x35E = flag;
        }
        ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
        if (fn_8012EC3C_c1(self) == 1U) {
            color.a = 0;
        } else if (em_alt_mode_ck_c1(self) == 1U) {
            color.a = (s32)(30.0f *
                            (0.5f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c * 0x2000))))) +
                      0xE1;
        } else {
            color.a = (s32)(30.0f *
                            (0.5f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c * 0x2000))))) +
                      0x87;
        }
        ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    }
}

/* 0x801913FC - per-(team,kind) "the effect may fire" predicate: 0 = no, 1 = the main window,
 * 2 = the secondary window. */
s32 em016_effect_window_ck(_ENEMY_WORK* self, u8 arg1) {
    f32 d;

    switch (self->team) {
    case 16:
        switch (arg1) {
        case 0:
            d = self->vec_0x36C.y - self->pos.y;
            if (d >= 300.0f) {
                return 1;
            }
            if (d <= -300.0f) {
                return 2;
            }
            return 0;
        case 1:
            if (self->tev_0x35A <= 0) {
                return 1;
            }
            break;
        case 2:
            if (self->field_0x1E2 == 2) {
                if (self->pos.y >= self->field_0x210 - (20.0f + em_float_depth_get(self)) * get_em_chg_scale(self)) {
                    return 1;
                }
            }
            break;
        case 5:
            if (self->tev_0x35C > 0) {
                return 1;
            }
            break;
        }
        break;
    case 17:
        switch (arg1) {
        case 4:
            if (self->field_0x011 != 0) {
                return 1;
            }
            break;
        }
        break;
    case 21:
        switch (arg1) {
        case 3:
            if (em_flag_0x8AE_ck(self) == 1U) {
                return 1;
            }
            break;
        case 4:
            if (self->field_0x011 != 0) {
                return 1;
            }
            break;
        }
        break;
    }
    return 0;
}

/* Picks the caller's class/state pair from the work's team, map key and action id. */
void em016_action_select(EmActWork* self, u8* out_class, u8* out_state) {
    switch (self->team) {
    case 16:
        em_move_mode_set_view1(self, 0);
        *out_class = 12;
        *out_state = 2;
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 6:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 8;
                fn_80126324_view1(self, 14, 15, 0.0f);
                break;
            case 7:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 13, 14, 0.0f);
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 2:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 6, 4, 0.0f);
                break;
            case 3:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 15, 16, 0.0f);
                break;
            }
            break;
        case 9:
        case 11:
            switch (self->act_id) {
            case 1:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 0, 1, 0.0f);
                break;
            }
            break;
        }
        break;
    case 17:
        fn_80182D5C_view1();
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 5:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
            case 12:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 1:
            case 5:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 6;
                break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            case 10:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 7;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        default:
            *out_class = 12;
            if (self->field_0x00A == 1) {
                em_move_mode_set_view1(self, 2);
                *out_state = 3;
            } else {
                em_move_mode_set_view1(self, 0);
                *out_state = 2;
            }
            break;
        }
        break;
    case 21:
        em_move_mode_set_view1(self, 4);
        *out_class = 12;
        *out_state = 1;
        break;
    }
}

/* Re-arms the action the work's map key and action id select. */
void em016_action_rearm(EmActWork* self) {
    u32 state = 0;

    switch (stage_map_kind_get_view3(self->field_0x1E0)) {
    case 3:
        switch (self->act_id) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            switch (self->state_0x9F6) {
            case 2:
                state = 1;
                break;
            }
            break;
        }
        break;
    }
    if (state == 1) {
        em_move_mode_set_view1(self, 0);
        em_mot_set_view1(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set_view1(self, 2);
        em_mot_set_view1(self, 0x28, 0, 0);
    }
}

/* Reports whether the work is in one of the two team-specific ready states. */
u32 em016_ready_ck(EmActWork* self) {
    if (self->team == 21) {
        if (self->state_0x1E2 == 4 && fn_801321DC_view1(self) == 1) {
            return 1;
        }
    } else if (self->state_0x1E2 == 2 && em_alt_mode_ck_view1(self) == 0) {
        return 1;
    }
    return 0;
}

/* Reports whether the work's state byte is clear. */
u32 em016_state_clear_ck(EmActWork* self) {
    return self->state_0x1E2 == 0;
}

/* Re-seats the work at its own position while the damage part admits the action. */
void em016_part_drop(EmActWork* self, u32 kind) {
    switch ((u8)kind) {
    case 1:
        if (em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1) == 2 && self->state_0x1E2 != 1) {
            em_drop_item_spawn(self, &self->pos_0x188, 100);
        }
        break;
    }
}

/* Reports whether the work may run its current action: the team/state gate, the entry probe and the
 * two restart checks. */
s32 em016_action_allow_ck(EmActWork* self, u32 arg) {
    u8 kind = stage_map_kind_get_view3(self->field_0x1E0);
    s32 hit;
    u32 probe;

    if (self->team != 16) {
        return 0;
    }
    switch (kind) {
    case 1:
    case 3:
        break;
    default:
        return 0;
    }
    if (fn_80129D3C_c1(self) == 1) {
        return 1;
    }
    hit = 0;
    switch (kind) {
    case 1:
        probe = 8;
        break;
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        switch (em_area_break_ck_c1(self, probe)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        switch (kind) {
        case 1:
            probe = 12;
            break;
        case 3:
            probe = 4;
            break;
        default:
            probe = 0xFF;
            break;
        }
        if (fn_8012A014_view1(self, 0, probe, (u16)arg, 0, (u8*)em016_pack_area_rec) == 1) {
            return 1;
        }
    }
    if (fn_80129A70_view1(self, (u16)arg) == 1) {
        return 1;
    }
    return (fn_8012A204_view1(self) - 1) == 0;
}

/* 0x80191CDC (0x8): Sets the accessor's flag word to 3 once it is installed. */
void Em016UserData::Attach() {
    fn_8013A654_view1((EmUserData*)this, 3);
}

/* 0x80191CE4 (0x14C): Applies the aim block's scale or rotation to the animation result of nodes 16..26. */
void Em016UserData::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* holder, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 kind, UserDataItem* item) {
    MTX34 mtx;
    EmMtx33 dst;
    EmRotVec rot;
    EmActWork* work;

    work = (EmActWork*)this->work;
    MTX34_ctor(&mtx);
    MTX33_ctor(&dst);
    switch (item->field_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
        case 18:
        case 20:
            anm_chr_result_scale_set(holder, &work->aim_0x328.vec_0x1C);
            break;
        case 24:
            rot.x = work->aim_0x328.angle_0x00;
            rot.y = 0;
            rot.z = 0;
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            anm_chr_result_rot_mul(holder, &dst);
            break;
        case 25:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x04);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            anm_chr_result_rot_mul(holder, &dst);
            break;
        case 26:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x10);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            anm_chr_result_rot_mul(holder, &dst);
            break;
        }
        break;
    }
}

/* 0x80191E30 (0xC8): Moves the matrix of node 16/18/20 onto its live cluster's position. */
void Em016UserData::CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 kind, UserDataItem* item) {
    MTX34 mtx;
    u32 index;
    EmActWork* work;

    work = (EmActWork*)this->work;
    MTX34_ctor(&mtx);
    switch (item->field_0x04) {
    case 0xFF:
        switch (kind) {
        default:
            return;
        case 16:
            index = 0;
            break;
        case 18:
            index = 1;
            break;
        case 20:
            index = 2;
            break;
        }
        EmCluster* cluster = &work->clusters_0x590[index];
        if (cluster->live_0x03 >= 1) {
            joint_mtx_load((EmMtxHolder*)pManip, &mtx);
            mtx34_set_trans(&mtx, &cluster->pos_0x24);
            joint_mtx_store((EmMtxHolder*)pManip, &mtx);
        }
        break;
    }
}

/* Steps the five aim angles by their per-frame increments and clamps them. */
void em016_aim_step(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    u32 v;

    if (em_alt_mode_ck_view1(self) == 1) {
        v = rec->angle_0x00 + 546;
        rec->angle_0x00 = v;
        if ((s16)v > 0) {
            rec->angle_0x00 = 0;
        }
        v = rec->rot_0x04.x + 455;
        rec->rot_0x04.x = v;
        if ((s16)v > 0) {
            rec->rot_0x04.x = 0;
        }
        v = rec->rot_0x04.y + 455;
        rec->rot_0x04.y = v;
        if ((s16)v > 0) {
            rec->rot_0x04.y = 0;
        }
        v = rec->rot_0x10.x + 455;
        rec->rot_0x10.x = v;
        if ((s16)v > 0) {
            rec->rot_0x10.x = 0;
        }
        v = rec->rot_0x10.y - 455;
        rec->rot_0x10.y = v;
        if ((s16)v < 0) {
            rec->rot_0x10.y = 0;
        }
    } else {
        v = rec->angle_0x00 - 546;
        rec->angle_0x00 = v;
        if ((s16)v < -5460) {
            rec->angle_0x00 = (u16)-5460;
        }
        v = rec->rot_0x04.x - 455;
        rec->rot_0x04.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.x = (u16)-4550;
        }
        v = rec->rot_0x04.y - 455;
        rec->rot_0x04.y = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.y = (u16)-4550;
        }
        v = rec->rot_0x10.x - 455;
        rec->rot_0x10.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x10.x = (u16)-4550;
        }
        v = rec->rot_0x10.y + 455;
        rec->rot_0x10.y = v;
        if ((s16)v > 4551) {
            rec->rot_0x10.y = 4551;
        }
    }
}

/* The aim fade's per-frame step (-0.005), a pool word `em016_aim_fade` subtracts and adds (a literal folds into the
 * opposite operation). */
__declspec(section ".sdata2") f32 em016_aim_fade_step = -0.0050000004f;

/* Runs the aim vector's fade in or out and mirrors it into its follower. */
void em016_aim_fade(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    f32 v;

    if (fn_8012EC3C_view1(self) == 0) {
        v = rec->vec_0x1C.x - em016_aim_fade_step;
        rec->vec_0x1C.x = v;
        if (v > 1.0f) {
            rec->vec_0x1C.x = 1.0f;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    } else {
        v = rec->vec_0x1C.x + em016_aim_fade_step;
        rec->vec_0x1C.x = v;
        if (v < 0.7f) {
            rec->vec_0x1C.x = 0.7f;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    }
}

/* Fills in one effect request (id 0x17) from the default vector. */
void em016_shot_req_init(EmSpawnRec* out, u8 a, u16 b, u16 c) {
    if (em016_shot_default_ready == 0) {
        setVec3(&em016_shot_default_pos, 0.0f, 0.0f, 30.0f);
        em016_shot_default_ready = 1;
    }
    out->id = 0x17;
    copyVec3(&out->pos, &em016_shot_default_pos);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* 0x801921A8 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em016UserData::~Em016UserData() {
}

/* Builds the five static vectors the effect requests start from. */
void em016_static_init(void) {
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    VEC3 v4;
    VEC3 v5;
    VEC3 v6;
    VEC3 v7;
    VEC3 v8;

    assignVec3_view1(&em016_aim_vec_pair_0.vec_0x00,
                setVec3(&v1, 0.0f, 6.0f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_0.vec_0x0C,
                setVec3(&v2, 0.0f, 0.0f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_1.vec_0x00,
                setVec3(&v3, 0.0f, -3.0f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_1.vec_0x0C,
                setVec3(&v4, 0.0f, 0.0f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_2.vec_0x00,
                setVec3(&v5, 0.0f, 48.0f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_2.vec_0x0C,
                setVec3(&v6, 0.0f, -1.92f, 0.0f));
    assignVec3_view1(&em016_aim_vec_pair_3.vec_0x00,
                setVec3(&v7, 0.0f, 0.0f, 30.0f));
    assignVec3_view1(&em016_aim_vec_pair_3.vec_0x0C,
                setVec3(&v8, 0.0f, -5.5f, 0.0f));
}
