/*
 * enemy/em007_act.cpp - enemy 007's program: the `em007_prog_tbl` entry points (init, frame update, action dispatch,
 *   event hook, motion events, tev colour, part/area/action-selection slots), the per-action state steps the
 *   `state_sub` dispatchers select, the two sound programs, the effect router, the user-data class hooks and the
 *   model effect's creation/release.
 * RANGE. .text 0x801D71C4-0x801E0ADC (97 functions, defined in address order); extab 0x80010164-0x800103D4,
 *   extabindex 0x8002C100-0x8002C4A8, .rodata 0x80570500-0x805706C0 (the seven turn tables), .data
 *   0x805B69D8-0x805B75E8 (`em007_prog_tbl` first, the user-data class table last), .sdata2 0x807994F8-0x80799788.
 *   Left edge: `enemy/em005_act.cpp`'s static initializer ends at 0x801D71C4 and this pool opens at 0x807994F8.
 *   Right edge: the pool's last entry (176.0f, 0x80799780) is followed by a zero alignment word, and the 0.75f at
 *   0x80799788 that `em007_model_eft_create` reads is the first entry of `lobby/fn_801E0ADC.cpp`'s pool, as are the
 *   `.data` objects `em007_model_eft_create`/`Em007ModelCopy`'s constructor read (`em007_model_res_ids` 0x805B7608,
 *   the 0x805B76B8 table): the functions from 0x801E0574 on belong to that unit's TU (seam request enemy-em007#1);
 *   the model effect from 0x801E0240 (its lobby-NPC entry point, its `_EFT` hooks) is a candidate for the same TU.
 *   Until the seam moves they are written here against the split as it stands: the two data rows are read through
 *   `lobby/em007_model_copy_bind.h`.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`+`cmpwi`/`cmplwi` forms),
 *   `#pragma fp_contract off` (the target has no fused multiply-add) and `#pragma pool_data off` (each table is
 *   addressed with its own `lis`/`addi`).  `em007_model_eft_create` is `#pragma optimization_level 0` (retail keeps
 *   every local on the stack and reloads each `work->models_0x08[i]`; levels 1/2 measure lower).
 * NAMES. The file name follows the runtime dump's `em007_prog_tbl`, which opens the TU's `.data`; the `_act` suffix
 *   is a GUESS.  GUESS (the `Em024ProgTbl` slot each fills): em007_init, em007_frame_update, em007_action_dispatch,
 *   em007_noop, em007_event_notify, em007_motion_events, em007_tev_color_update, em007_part_damage_ck,
 *   em007_area_start_set, em007_reserved_noop, em007_part_level_ck, em007_action_select.  GUESS (dispatcher slot):
 *   em007_act<action>_<first sub-state>, em007_act<action>_dispatch, em007_act10_se_play, em007_act11_se_play.
 *   GUESS (from the bodies): em007_partner_front_ck, em007_alt_parts_update, em007_alt_mode_effect,
 *   em007_effect_spawn, em007_alt_scale_step, em007_aim_point_get, em007_ally_wait_near_ck, em007_act13_early_ck,
 *   `Em007UserData` (Attach/CalcAnmItemEx/CalcWorldEnd), `Em007ModelCopy`, em007_model_eft_start/_alloc/_release/
 *   _step/_create, em007_npc_model_eft_start, em007_model_copy_bind_nodes, em007_res_node_name, em007_res_name_at.
 *   GUESS (lobby/fn_801E0ADC.cpp's data rows, registry): em007_model_res_ids, em007_model_eft_scale.
 *   GUESS (data): em007_turn_seq_0..6, em007_pick_tbl_0/1, em007_se_seq_<sub-state>, em007_se_alt_seq_<sub-state>
 *   and their em007_se_rec/pos records; the types Em007ProgTbl, Em007UserData, Em007EftWork, Em007ModelCopy,
 *   EmUserDataItemView.
 * RESIDUALS. Partial rows (register colouring, scheduling or stack slots only): `em007_act7_8` (the angle's
 *   u16 result is coloured r3, retail keeps it in r27 with the s32 term; the shell call loads its table before the
 *   arguments, retail after), `em007_act9_1` (retail loads the 1.0f gauge argument after `mr r3,r30`, ours before),
 *   `em007_model_eft_create` (two extra `clrlwi`: the res-id argument and the made-flag store; its extab record is
 *   0x18, retail 0x8 - the new-expression's landing pad), `em007_model_copy_bind_nodes` (the two node temporaries'
 *   stack slots swap), `em007_action_select` (retail keeps the first switch's answer in r28, ours r30),
 *   `em007_act2_14`/`em007_act7_6` (the motion-choice sum lands in r0, retail r4), `em007_act7_10` (the
 *   `fn_8012D3E0` answer is narrowed into r0, retail r6).
 *   Data: `Em007UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table
 *   (0x805B75B8); it is built with placement `new` over `operator new` (a new-expression adds a landing pad).
 *   `.sdata2` stops at 0x28C: the trailing alignment word is the link's.
 *   flipcheck: `em007_model_res_ids`/`em007_model_eft_scale` resolve only once their registry names are in the map;
 *   extab/extabindex differ by `em007_model_eft_create`'s landing pad.
 * SHAPES. A one-case test retail compares signed (`cmpwi`) is a `switch` with `default:` first (`em007_init`,
 *   `em007_event_notify`, `em007_act1_4`, `Em007UserData::CalcAnmItemEx`); a two-value area test is a nested `switch`
 *   with shared cases (`em007_tev_color_update`).  The hit-window id choices are `cond ? a : b` arguments, the step
 *   counters `self->state++`.  The shell-table slots +0x14/+0x2C/+0x54 are called without the table argument
 *   (`Em007ShellPosJobFn`, `Em007ShellSpawnFn`, `Em007ShellPointFn`).  `em_act_ck` is read unsigned through
 *   `em_act_ck_u`.  The pool's order follows the source text: `em007_act2_11`'s turn angle is one expression and
 *   `em007_act7_8` computes the XZ distance before the angle.  A value retail computes before an argument list is a
 *   local assigned first (`em007_act7_8`'s hit mask, `em007_act7_3`'s speed, `em007_act9_1`'s gauge scale) and a
 *   product whose operands retail swaps takes its call result through a local (`em007_act2_10`/`7_1`/`7_11`).  The data is defined between the functions in retail
 *   order (the jump tables fall between the definitions).
 */

#include "types.h"
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em007_act.h"
#include "Pl/pl_hit_sphere.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "ef/eft009.h" /* eft009_set_pos, eft009_spawn_at_joint (rule 2: the owner's header) */
#include "sound/mhchar.h"
#include "stage/stg_w.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_8012BDF4.h"
#include "fn_8004CAD8.h"
#include "g3d/anm_chr_result_scale_get.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcview.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "lobby/lb_npc.h"
#include "enemy/fn_80138074.h"
#include "MSL/new.h" /* the placement `operator new` */
#include "draw_shape/mtx34_copy.h"
#include "g3d/g3d_anmchr.h" /* res_node_ref (rule 2) */
#include "g3d/fn_8005AA28.h" /* res_node_is_valid (rule 2) */
#include "enemy/em007_res_node_name.h"
#include "lobby/em007_model_copy_bind.h"
#include "light/light.h" /* light_id_next (rule 2) */
#include "ai/ai_npc.h" /* _AINPC_W (rule 1) */
#include "enemy/fn_80147CE0.h"
#include "enemy/em_hit_mask_get.h"
#include "enemy/fn_80137604.h"
#include "enemy/fn_8011D448.h"
#include "enemy/em_common.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_se_record.h"
#include "enemy/CancelFade.h"
#include "unsplit/enemy.h"

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* The shell-table slots +0x14, +0x2C and +0x54 as this unit calls them: without the table argument the header spells
 * (the table base only reaches r8/r9 to fetch the slot). */
typedef void (*Em007ShellPosJobFn)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 kind, u32 area, u16 group);
typedef void (*Em007ShellPointFn)(struct _ENEMY_WORK* self, u32 kind, u32 id, nw4r::math::VEC3* pos);
typedef void (*Em007ShellSpawnFn)(struct _ENEMY_WORK* self, u32 kind, u32 id, nw4r::math::VEC3* pos, f32 scale,
                                  u16 group);

/* ----------------------------------------------------------------------------------------------------
 * the program table and the tables the action steps read
 * -------------------------------------------------------------------------------------------------- */

extern "C" {
void em007_init(struct _ENEMY_WORK* self, u8 mode);
void em007_frame_update(struct _ENEMY_WORK* self);
void em007_action_dispatch(struct _ENEMY_WORK* self);
void em007_noop(void);
void em007_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em007_motion_events(struct _ENEMY_WORK* self);
void em007_tev_color_update(struct _ENEMY_WORK* self);
u32 em007_part_damage_ck(struct _ENEMY_WORK* self, u8 part);
void em007_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub);
void em007_reserved_noop(void);
u32 em007_part_level_ck(struct _ENEMY_WORK* self, u8 part);
u32 em007_action_select(struct _ENEMY_WORK* self, u16 arg);
}

/* The em007 program table (`em007_prog_tbl`): the entry points the shared enemy program driver calls (the slot
 * names follow `Em024ProgTbl`; the empty slots are zero).
 * size: 0x6C */
struct Em007ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u32 (*part_damage_ck_0x24)(struct _ENEMY_WORK* self, u8 part);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*reserved_0x34)(void);
    /* +0x38 */ u32 pad_0x38[3];
    /* +0x44 */ u32 (*part_level_ck_0x44)(struct _ENEMY_WORK* self, u8 part);
    /* +0x48 */ u32 pad_0x48[3];
    /* +0x54 */ u32 (*action_select_0x54)(struct _ENEMY_WORK* self, u16 arg);
    /* +0x58 */ u32 pad_0x58[5];
};

extern "C" {
Em007ProgTbl em007_prog_tbl = {
    0,
    em007_init,
    em007_frame_update,
    em007_action_dispatch,
    em007_noop,
    em007_event_notify,
    em007_motion_events,
    0,
    em007_tev_color_update,
    em007_part_damage_ck,
    {0, 0},
    em007_area_start_set,
    em007_reserved_noop,
    {0, 0, 0},
    em007_part_level_ck,
    {0, 0, 0},
    em007_action_select,
    {0, 0, 0, 0, 0},
};
}

/* The two pick records `em007_action_select` hands `em_pack_area_pick` (an action pair and its weights). */
u8 em007_pick_tbl_0[12] = {0x00, 0x00, 0x00, 0x0A, 0x00, 0xFA, 0x00, 0x05, 0xFF, 0xFF, 0x00, 0x00};
u8 em007_pick_tbl_1[12] = {0x00, 0x00, 0x00, 0x0A, 0x00, 0x64, 0x00, 0x05, 0xFF, 0xFF, 0x00, 0x00};

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * `{motion, frames}` steps. */

extern "C" const u32 em007_turn_seq_0[16] = {
    0x00000006, 0x0000002A, 0x00000000, 0x40001555,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x00080000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_1[16] = {
    0x0000000E, 0x00000032, 0x00000000, 0x40001555,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x001B0000, 0x00000004, 0x00000000, 0x00000000,
    0x001A0000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_2[16] = {
    0x00000008, 0x0000002A, 0x00000000, 0x80000000,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x000D0000, 0x00000002, 0x00000000, 0x00000000,
    0x000C0000, 0x00000002, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_3[16] = {
    0x00000000, 0x00000028, 0x00000000, 0x80000000,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x002F0000, 0x00000004, 0x00000000, 0x00000000,
    0x002E0000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_4[16] = {
    0x00000000, 0x00000020, 0x00000000, 0x40001555,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x002D0000, 0x00000004, 0x00000000, 0x00000000,
    0x002C0000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_5[16] = {
    0x00000000, 0x00000018, 0x00000000, 0x55550000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00060000, 0x0000000A, 0x00000000, 0x00000000,
    0x00050000, 0x0000000A, 0x00000000, 0x00000000,
};

extern "C" const u32 em007_turn_seq_6[16] = {
    0x0000000C, 0x0000003E, 0x00000000, 0x40000000,
    0x00310000, 0x00000004, 0x00000000, 0x00000000,
    0x00310000, 0x00000004, 0x00000000, 0x00000000,
    0x00310000, 0x00000004, 0x00000000, 0x00000000,
};

/* ===================================================================================================
 * the callees' declarations
 * =================================================================================================== */

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotVecY(nw4r::math::VEC3* v, u32 angle);   /* rotVecY__FPQ34nw4r4math4VEC3Ul */
void calcVecAngXY(nw4r::math::VEC3* v, u32* out1, u32* out2);
                                                /* calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl */
s32 findInterSection(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, u8 d, u32 e,
                     u8 f, u16 g, u8* h);
            /* findInterSection__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3PQ34nw4r4math4VEC3UcUlUcUsPUc */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void mulVecMat(nw4r::math::VEC3* v, nw4r::math::MTX34* m);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                /* em_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self); /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
                                                /* get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34 */
void* get_move_work_adrs(u8 index);             /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                /* get_move_work_max__FUc */

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures: `enemy/em_common.cpp`'s first. */
void em_part_rec_reset(struct _ENEMY_WORK* self, u32 a);
void em_part_rec_alt_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_action_finish(struct _ENEMY_WORK* self);
void em_action_finish_stand(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_target_pos_set(struct _ENEMY_WORK* self, void* p);
void em_hit_window_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);

void em_reaction_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_busy_set(struct _ENEMY_WORK* self);
u32 em_busy_ck(struct _ENEMY_WORK* self);

u32 em_eft_active_ck(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
f32 get_em_base_scale(struct _ENEMY_WORK* self);
f32 em_mot_frame_end_get(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
f32 em_height_base_get(struct _ENEMY_WORK* self);
void em_height_set(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void em_stamina_add(struct _ENEMY_WORK* self, u32 a);
void em_busy_timer_reset(struct _ENEMY_WORK* self);
void em_timer_0x81C_arm(struct _ENEMY_WORK* self);
void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);
void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);
u32 em_turn_seq_step(struct _ENEMY_WORK* self, void* tbl);
void em_move_vec_clr(struct _ENEMY_WORK* self);
void em_move_vec2_clr(struct _ENEMY_WORK* self);
void em_move_offset_apply(struct _ENEMY_WORK* self);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_shake_req_set(struct _ENEMY_WORK* self);

/* `enemy/fn_8011D448.cpp`'s damage/knockback helper */

/* This unit's own functions, declared before use. */
void em007_act0_dispatch(struct _ENEMY_WORK* self);
void em007_act1_dispatch(struct _ENEMY_WORK* self);
void em007_aim_point_get(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 scale);
u32 em007_ally_wait_near_ck(struct _ENEMY_WORK* self, u8 a);                     

/* Helpers of other units: `fn_8004CAD8.cpp`'s vector math, `g3d/g3d_calcworld.cpp`, `ef/eft007.cpp`,
 * `enemy/em001_prog.cpp`, `stage/stg_w.cpp`, `camera/camera_main.cpp`, `lobby/fn_8030121C.cpp` and
 * `enemy/em_model.cpp`. */
f32 vec3_distance(void* a, void* b);
void addVec3To(void* a, void* b);
void eft007_part_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void stg_ground_impact_set(void* pos, u8 a, u16 b);
void camera_flag_0x145_set(void);
void camera_target_slot_clear(u32 a);
void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, void* pos, f32 c);
void em_drop_item_spawn(struct _ENEMY_WORK* self, void* pos, u32 a);

#ifdef __cplusplus
}
#endif

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);  /* setVector3__FPQ34nw4r4math4VEC3fff */
void rotVecY(nw4r::math::VEC3* v, u32 angle);               /* rotVecY__FPQ34nw4r4math4VEC3Ul */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                            /* em_frame_check__FP11_ENEMY_WORKUsff */
s32 em_die_ck(struct _ENEMY_WORK* self);                    /* em_die_ck__FP11_ENEMY_WORK */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                /* em_get_mot_no__FP11_ENEMY_WORK */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                                            /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);             /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                            /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void* get_move_work_adrs(u8 index);                         /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                            /* get_move_work_max__FUc */

/* The runtime's allocator pair (`__nw__FUl`/`__dl__FPv`), declared as the C++ functions (rule 9). */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures: `enemy/enemy_control.cpp`'s first. */

/* `enemy/em_common.cpp` */
void em_data_point_get(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
void em_action_finish(struct _ENEMY_WORK* self);
void em_action_finish_stand(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_act_arm_unless_down(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_hit_window_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 em_area_roam_pick(struct _ENEMY_WORK* self, u16 a);
u8 em_area_break_ck(struct _ENEMY_WORK* self, u8 area);
u32 em_pack_area_pick(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 em_area_leave_ck(struct _ENEMY_WORK* self);

void em_busy_set(struct _ENEMY_WORK* self);
u32 em_act11_wait_ck(struct _ENEMY_WORK* self);

/* `fn_8012EC3C` and `em_alt_mode_ck` read the work record (+0x89F, +0x8AA). */
u32 em_eft_active_ck(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

void em_mot_set_blend(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
void em_stamina_add(struct _ENEMY_WORK* self, u32 a);
void em_bait_react(struct _ENEMY_WORK* self);
struct _ENEMY_WORK* em_ally_find(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void em_gauge_tick(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void em_tint_hold_start(struct _ENEMY_WORK* self);
void em_tint_hold_end(struct _ENEMY_WORK* self);
void em_part_hit_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_act_flag1_set(struct _ENEMY_WORK* self);

/* `enemy/fn_80138074.c` */
void em_res_user_data_set(struct _ENEMY_WORK* self, void* helper);
s32 em_res_user_data_ck(struct _ENEMY_WORK* self);

/* ef module */
void eft009_spawn_at_pos(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, f32 scale,
                 nw4r::math::VEC3* pos);
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void eft013_spawn_id(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, s32 d);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);
void draw_shape_arm(struct _ENEMY_WORK* self, u32 a, u32 b);

void stg_ground_impact_set(void* pos, u8 a, u16 b);

/* ----------------------------------------------------------------------------------------------------
 * the records this unit needs locally
 * -------------------------------------------------------------------------------------------------- */

/* The user-data accessor `em007_init` installs: the base accessor with its own destructor, attach hook, animation
 * item hook and end-of-tree hook (its table is `.data` 0x805B75B8).  size: 0xC (`operator new(0xC)`) */
class Em007UserData : public ResUserDataAc {
public:
    Em007UserData();
    virtual ~Em007UserData();
    virtual void Attach();
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    virtual void CalcWorldEnd(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl,
                              nw4r::g3d::FuncObjCalcWorld* pFuncObj);
};

/* ====================================================================================================
 * bodies
 * ================================================================================================== */


void em007_init(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;
    EmActionBlock* blk;
    Em007UserData* helper;

    VEC3_ctor(&pos);
    blk = &self->action_0x328;
    setVector3(&blk->vec_0x328, 1.0f, 1.0f, 1.0f);
    setVector3(&blk->vec_0x334, 1.0f, 1.0f, 1.0f);
    blk->field_0x340 = 0.0f;
    blk->field_0x344 = 0;
    blk->field_0x345 = 0;
    if (em_res_user_data_ck(self) == 0) {
        helper = (Em007UserData*)operator new(sizeof(Em007UserData));
        new (helper) Em007UserData;
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, 0.0f, 0.0f, 10.0f);
        eft013_spawn_id(self, 0x14, 0x18, &pos, 1.0f, 0);
    }
    switch (arg1) {
    default:
        break;
    case 2:
            switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 4:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                    break;
                case 5:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                    break;
                }
                break;
            case 2:
                switch (self->area_no) {
                case 4:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 6), &self->pos);
                    break;
                case 6:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 1:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                    break;
                case 3:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                    break;
                }
                break;
            case 4:
                switch (self->area_no) {
                case 3:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                    break;
                case 6:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                    break;
                }
                break;
            case 5:
                switch (self->area_no) {
                case 4:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                    break;
                case 6:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                    break;
                }
                break;
            case 8:
                switch (self->area_no) {
                case 1:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
                    break;
                }
                break;
            case 9:
                switch (self->area_no) {
                case 0:
                    em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
                    break;
                }
                break;
            }
            em_move_mode_set(self, 4);
            em_act_arm_unless_down(self, 6, 5);
        break;
    }
}

/* 0x801D752C (0x3C): Constructs the accessor: the base first, then this class's table. */
Em007UserData::Em007UserData() {
}

void em007_noop(void) {
}

void em007_event_notify(struct _ENEMY_WORK* self, u8 arg1, u8 arg2) {
    switch (arg1) {
    case 1:
        switch (arg2) {
        case 0:
            em_act_flag1_set(self);
            break;
        case 5:
        case 9:
        case 12:
            em_bait_react(self);
            break;
        }
        break;
    case 10:
        switch (arg2) {
        default:
            break;
        case 201:
            em_part_hit_set(self, 0, 0);
        }
        break;
    }
}

void em007_frame_update(struct _ENEMY_WORK* self) {
    u16 max;
    s32 found;
    u16 i;
    struct _ENEMY_WORK* other;

    found = 0;
    max = get_move_work_max(3);
    other = (struct _ENEMY_WORK*)get_move_work_adrs(3);
    if (em_die_ck(self) == 0) {
        for (i = 0; i < max; i++, other++) {
            if (other->active != 0 && (other->field_0x1C8 & 1) != 0 && self != other &&
                self->area_no == other->area_no &&
                em_phase_get(self) == em_phase_get(other) && other->action == 0xB) {
                copyVec3(&self->aim, &other->pos);
                self->action_0x328.field_0x345 = 1;
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        self->action_0x328.field_0x345 = 0;
    }
    if (em_alt_mode_ck(self) == 1) {
        if ((self->flags_0x836 & 0x8000) != 0) {
            self->flags_0x836 &= 0x7FFF;
        }
    } else {
        if ((self->flags_0x836 & 0x8000) == 0) {
            self->flags_0x836 |= 0x8000;
        }
    }
}

void em007_act0_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
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

void em007_act0_1(struct _ENEMY_WORK* self) {
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

void em007_act0_2(struct _ENEMY_WORK* self) {
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

void em007_act0_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

void em007_act0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em007_act0_0(self);
        break;
    case 1:
        em007_act0_1(self);
        break;
    case 2:
        em007_act0_2(self);
        break;
    case 3:
        em007_act0_0(self);
        break;
    case 4:
        em007_act0_0(self);
        break;
    case 6:
        em007_act0_0(self);
        break;
    case 7:
        em007_act0_7(self);
        break;
    }
}

void em007_act1_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_1(struct _ENEMY_WORK* self) {
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

void em007_act1_2(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xB, 5);
            draw_shape_arm(self, 0x17, 0xA);
            stg_ground_impact_set(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn(self, 0, 0x16, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 106.0f, 206.0f) == 1 &&
            (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, 0.0f, -50.0f, 50.0f);
            eft_em_spawn(self, 1, 0x16, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 5);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_4(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0x3C);
        }
        if (em_mot_end_ck(self) == 1) {
            switch (arg1) {
            default:
                em_state_set(self, 1, 5);
                break;
            case 1:
                self->state_0x006++;
                if (self->state_0x006 >= 4) {
                    em_state_set(self, 1, 9);
                }
                break;
            case 2:
                em_state_set(self, 1, 0xC);
                break;
            }
        }
        break;
    }
}

void em007_act1_5(struct _ENEMY_WORK* self, u32 arg1) {
    if ((u8)arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set_blend(self, 0xC9, 6, 0, 1);
        em_hit_window_set_default(self, 0, 0x3D);
        if ((u8)arg1 == 1) {
            em_stamina_add(self, 0x3E8);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x36, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em007_act1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em007_act1_0(self);
        break;
    case 1:
        em007_act1_1(self);
        break;
    case 2:
        em007_act1_2(self);
        break;
    case 3:
        em007_act1_3(self);
        break;
    case 4:
        em007_act1_4(self, 0);
        break;
    case 5:
        em007_act1_5(self, 0);
        break;
    case 6:
        em007_act1_6(self);
        break;
    case 7:
        em007_act1_7(self);
        break;
    case 8:
        em007_act1_4(self, 1);
        break;
    case 9:
        em007_act1_5(self, 1);
        break;
    case 10:
        em007_act1_10(self);
        break;
    case 11:
        em007_act1_4(self, 2);
        break;
    case 12:
        em007_act1_5(self, 2);
        break;
    }
}

#ifdef __cplusplus
}
#endif

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

extern "C" void em007_act2_0(struct _ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_hit_window_set(self, 0, 0xC, 2);
        em_stamina_add(self, (u32)-0x14);
        switch ((u8)arg1) {
        case 0: em_approach_start(self, -500.0f, 0); return;
        case 1: em_approach_start(self, -1000.0f, 0); return;
        case 2: em_approach_start(self, 0.0f, 0); return;
        case 3: em_approach_start(self, -1600.0f, 0); return;
        case 4: em_approach_start(self, -1200.0f, 0); return;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            switch ((u8)arg2) {
            case 0:
                self->state++;
                em_mot_set(self, 0xB, 6, 0);
                break;
            case 1:
                em_action_finish(self);
                break;
            }
        } else {
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

extern "C" void em007_act2_6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_0, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act2_9(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x10, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act2_10(struct _ENEMY_WORK* self) {
    f32 temp_f1;
    f32 temp_f31;
    s32 temp_r3;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_2, 0, 1, 0);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        temp_f1 = get_em_base_scale(self);
        self->field_0x318 = -6.481132f * temp_f1 * temp_f31;
        self->field_0x324 = 0.0f;
        temp_r3 = calcVecAng2(&self->pos, &self->vec_0x36C);
        rotVecY(&self->offset_0x30C.vec_0x310, temp_r3);
        rotVecY(&self->vec_0x31C, temp_r3);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 106.0f) == 1) {
            CancelFade(self);
        }
        if (em_turn_seq_step(self, (void*)em007_turn_seq_2) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act2_11(struct _ENEMY_WORK* self) {
    f32 temp_f1;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 48.0f) == 1) {
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * (2.5f * get_em_base_scale(self))) / 360.0f));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act2_14(struct _ENEMY_WORK* self, u8 arg1) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)((((s32)~((arg1 - 1) | (1 - arg1)) >> 0x1F) + 0x25)), 2, 0);
        break;
    case 1:
        if (em_frame_check(self, 3, 14.0f, 78.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act2_20(struct _ENEMY_WORK* self, u8 arg1) {
    f32 var_f1;
    f32 temp_f31;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        switch (arg1) {
        case 0:
            var_f1 = -0.5f * vec3_distance(&self->pos, &self->vec_0x36C);
            break;
        case 1:
            var_f1 = -400.0f + (-475.0f * get_em_chg_scale(self));
            break;
        }
        em_approach_start(self, var_f1, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 6, 0);
            em_approach_start(self, 0.0f, 0x10);
            break;
        }
        break;
    case 2:
        em_approach_step(self, 0, 0x80);
        if ((em_mot_end_ck(self) == 1) ||
            (temp_f31 = 400.0f * get_em_chg_scale(self),
             calcVecDistXZ(&self->pos, &self->vec_0x36C) < temp_f31)) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act2_21(struct _ENEMY_WORK* self) {
    f32 var_f1;
    f32 var_f2;
    s32 temp_r0;
    s32 var_r31;

    var_r31 = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if ((s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) < 0) {
            var_f1 = 10.0f;
            var_f2 = 15.0f;
        } else {
            var_f1 = 15.0f;
            var_f2 = 10.0f;
        }
        temp_r0 = self->bits_0x1EC & 3;
        switch (temp_r0) {
        case 0:
            var_r31 = (s32)(u16)(s32)(0.5f + ((65536.0f * var_f1) / 360.0f));
            break;
        case 1:
            var_r31 = -(s32)(u16)(s32)(0.5f + ((65536.0f * var_f2) / 360.0f));
            break;
        }
        em_turn_seq_start(self, (void*)em007_turn_seq_5, 0, 1, (u16)var_r31);
        return;
    case 1:
        em_turn_seq_step(self, (void*)em007_turn_seq_5);
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em007_act2_0(self, 0, 0); break;
    case 1: em007_act2_0(self, 0, 1); break;
    case 2: em007_act2_0(self, 1, 1); break;
    case 3: em007_act2_0(self, 2, 1); break;
    case 4: em007_act2_0(self, 3, 1); break;
    case 5: em007_act2_0(self, 4, 1); break;
    case 6: em007_act2_6(self); break;
    case 9: em007_act2_9(self); break;
    case 10: em007_act2_10(self); break;
    case 11: em007_act2_11(self); break;
    case 14: em007_act2_14(self, 0); break;
    case 15: em007_act2_14(self, 1); break;
    case 20: em007_act2_20(self, 0); break;
    case 21: em007_act2_21(self); break;
    case 22: em007_act2_20(self, 1); break;
    }
}

extern "C" void em007_act6_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_busy_set(self);
        em_busy_timer_reset(self);
        return;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        }
        return;
    }
}

extern "C" void em007_act6_1(struct _ENEMY_WORK* self) {
    s32 temp_r0;

    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            em_action_finish_stand(self);
        }
        return;
    }
}

extern "C" void em007_act6_3(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_approach_start(self, 0.0f, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

extern "C" void em007_act6_4(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em007_turn_seq_0, 0, 0, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_0) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

extern "C" void em007_act6_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x27, 0, 0);
        em_height_set(self, 0.0f);
        em_hit_window_set_default(self, 0, 0xA);
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_shake_req_set(self);
        break;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, 46.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act6_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em007_act6_0(self); return;
    case 1: em007_act6_1(self); return;
    case 3: em007_act6_3(self); return;
    case 4: em007_act6_4(self); return;
    case 5: em007_act6_5(self); return;
    }
}

extern "C" void em007_act7_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_1, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_1) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act7_1(struct _ENEMY_WORK* self) {
    f32 temp_f1;
    f32 temp_f31;

    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_hit_window_set(self, 0, 2, 0x88);
        em_hit_window_set(self, 1, 0x34, 0x18);
        em_stamina_add(self, (u32)-0x14);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        temp_f1 = get_em_base_scale(self);
        self->field_0x318 = -2.631579f * temp_f1 * temp_f31;
        self->field_0x324 = 0.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        break;
    case 1:
        if (em_frame_check(self, 3, 24.0f, 100.0f) == 1) {
            CancelFade(self);
        }
        em_turn_in_window2(self, 24.0f, 100.0f, 0x8000);
        if ((self->state_0x006 == 0) && (self->field_0xA69 == 0)) {
            self->state_0x006++;
            em_hit_window_set(self, 1, 0x35, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_2(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 6, 0);
        em_hit_window_set(self, 0, arg1 == 1 ? 0x24 : 3, 0x88);
        em_hit_window_set(self, 1, 0x37, 0x18);
        em_stamina_add(self, (u32)-0x14);
        break;
    case 1:
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x38, 0x18);
        }
        if (em_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x23, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_3(struct _ENEMY_WORK* self, u8 arg1) {
    s32 temp_r3_2;
    u32 temp_r30;
    u8 temp_r0;
    u8 temp_r3;

    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 2, 0);
        em_hit_window_set(self, 0, arg1 == 1 ? 0x25 : 4, 0x88);
        em_hit_window_set(self, 1, 0x39, 0x18);
        em_stamina_add(self, (u32)-0x32);
        self->timer_0x020 = 0x3E8;
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x20, 0x90);
        }
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, arg1 == 1 ? 0x26 : 5, 0x88);
            em_hit_window_set(self, 1, 0x3A, 0x18);
        }
        if (em_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x21, 0x90);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, arg1 == 1 ? 0x27 : 6, 0x88);
            em_hit_window_set(self, 1, 0x3B, 0x18);
        }
        if ((em_frame_check(self, 0, 92.0f, 0.0f) == 1) ||
            (em_frame_check(self, 0, 158.0f, 0.0f) == 1)) {
            if (self->state_0x006 < 5) {
                f32 speed = 1.0f + (0.05f * (f32)++self->state_0x006);
                em_mot_speed_set(self, speed);
            }
        }
        temp_r3_2 = self->timer_0x020;
        if (temp_r3_2 > 0) {
            self->timer_0x020 = temp_r3_2 - 1;
        }
        temp_r30 = em_approach_step(self, 0, 0x100);
        if ((em_mot_end_ck(self) == 1) && ((temp_r30 == 1) || (self->timer_0x020 <= 0))) {
            self->state++;
            em_mot_set(self, 0x30, 2, 0);
            em_hit_window_set(self, 1, 0x22, 0x90);
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

extern "C" void em007_act7_4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    u32 sp8;
    u16 var_r3;

    VEC3_ctor(&sp1C);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        em_hit_window_set(self, 0, 7, 0x80);
        em_stamina_add(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        var_r3 = sp8 - (u16)(self->field_0x1C0 + 0xD334);
        if ((u32)(var_r3 - 1) <= 0x7FFE) {
            if (var_r3 > 0xE39) {
                var_r3 = 0xE39;
            }
        } else {
            var_r3 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r3 >> 8);
        return;
    case 1:
        em_turn_in_window(self, 2.0f, 30.0f, self->state_0x006 << 8);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act7_5(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    u32 sp8;
    u16 temp_r0;
    u32 var_r0;

    VEC3_ctor(&sp1C);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 6, 0);
        em_hit_window_set(self, 0, 8, 0x80);
        em_stamina_add(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        temp_r0 = sp8 - (u16)(self->field_0x1C0 + 0x2CCD);
        if ((s32)temp_r0 > 0x8000) {
            var_r0 = 0x10000 - temp_r0;
            if (var_r0 > 0xE39) {
                var_r0 = 0xE39;
            }
        } else {
            var_r0 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r0 >> 8);
        return;
    case 1:
        em_turn_in_window(self, 2.0f, 30.0f, -(self->state_0x006 << 8));
        if ((arg1 == 1) && (em_frame_check(self, 0, 100.0f, 0.0f) == 1)) {
            em_drop_item_spawn(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act7_6(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)((arg1 == 1) + 0x2A), 6, 0);
        em_stamina_add(self, (u32)-0x32);
        break;
    case 1:
        if ((em_frame_check(self, 3, 40.0f, 100.0f) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            setVector3(&sp8, 0.0f, -20.0f, 0.0f);
            ((Em007ShellPointFn)shell_set_func_ptr->method_0x54)(self, 2, 0x16, &sp8);
        }
        if (em_frame_check(self, 0, 220.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 0x1C, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, 256.0f, 268.0f) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, 290.0f, 302.0f) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_8(struct _ENEMY_WORK* self, u8 arg1, u32 arg2) {
    nw4r::math::MTX34 sp60;
    EmSpawnRec rec;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    u32 var_r4;
    u32 var_r28;
    u32 var_r29;
    u16 var_r27;
    f32 temp_f1;
    s32 temp_r27;
    u32 var_r31;
    s32 temp_r29;
    u16 mask;

    var_r31 = arg2;
    VEC3_ctor(&sp38);
    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    MTX34_ctor(&sp60);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            var_r4 = 0x34;
            var_r28 = 0x1B;
            var_r29 = 0x33;
            break;
        case 1:
            var_r4 = 0x29;
            var_r28 = 0x1A;
            var_r29 = 0x32;
            break;
        case 2:
            var_r4 = 0x34;
            var_r28 = 0x31;
            var_r29 = 0x33;
            break;
        case 3:
            var_r4 = 0x29;
            var_r28 = 0x30;
            var_r29 = 0x32;
            break;
        }
        em_mot_set(self, var_r4, 2, 0);
        em_hit_window_set(self, 0, var_r28, 0x88);
        em_hit_window_set(self, 1, var_r29, 0x90);
        em_stamina_add(self, (u32)-0x14);
        break;
    case 1:
        if (em_frame_check(self, 2, 50.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0xC0);
        }
        if ((em_frame_check(self, 3, 88.0f, 112.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp38, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x47, 0x16, &sp38, 1.0f);
        }
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            rec.id = 0x17;
            copyVec3(&rec.pos, setVec3(&sp8, 0.0f, 200.0f, 60.0f));
            copyVec3(&sp38, &rec.pos);
            get_joint_wmat_em(self, rec.id, &sp60);
            setVector3(&sp2C, sp60.m[0][3], sp60.m[1][3], sp60.m[2][3]);
            mulVecMat(&sp38, &sp60);
            addVec3To(&sp2C, &sp38);
            copyVec3(&sp14, &self->pos);
            sp14.y += 50.0f;
            mask = em_hit_mask_get(self);
            temp_r29 = findInterSection(&sp14, &sp2C, &sp20, 1, 0xFFFF, self->area_no, mask | 0xC0, 0);
            temp_r27 = -(s32)(48.0f * ((65.0f + (self->vec_0x36C.y - sp2C.y)) / 10.0f));
            temp_f1 = calcVecDistXZ(&self->vec_0x36C, &sp2C) - 2100.0f;
            var_r27 = (u16)(temp_r27 - (s32)(96.0f * (temp_f1 / 50.0f)));
            if ((s32)var_r27 < 0x8000) {
                if ((s32)var_r27 > 0x1000) {
                    var_r27 = 0x1000;
                }
            } else if ((s32)var_r27 < 0xF800) {
                var_r27 = 0xF800;
            }
            rec.field_0x10 = 0;
            rec.field_0x12 = (u16)var_r27;
            rec.field_0x14 = 0;
            eft007_part_spawn(self, 0x24, var_r27, 0);
            switch ((u8)var_r31) {
            case 0:
                var_r31 = 0xD;
                break;
            case 1:
                var_r31 = 0x1D;
                break;
            case 2:
                var_r31 = 0x1E;
                break;
            }
            if (temp_r29 == 0) {
                shell_set_func_ptr->method_0x3C(self, &rec, var_r31, shell_set_func_ptr);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_10(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;

    VEC3_ctor(&sp14);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_hit_window_set(self, 0, 9, 0x88);
        em_stamina_add(self, (u32)-0x14);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - 476.0f;
        if (var_f31 > 684.0f) {
            var_f31 = 684.0f;
        } else if (var_f31 < 0.0f) {
            var_f31 = 0.0f;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / 10.0f;
        break;
    case 1:
        if (em_frame_check(self, 3, 2.0f, 30.0f) == 1) {
            em_turn_to_target(self, 0x3A0);
        }
        if (em_frame_check(self, 3, 72.0f, 94.0f) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, 104.0f, 0.0f) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, 0.0f, 0.0f, 0.0f));
            ((Em007ShellSpawnFn)shell_set_func_ptr->method_0x2C)(self, 5, 0x21, &sp14, 1.0f, self->field_0xAEA);
        }
        if ((em_frame_check(self, 1, 138.0f, 0.0f) == 1) && (em_busy_ck(self) == 1)) {
            if (((u8 (*)(struct _ENEMY_WORK*, u32))fn_8012D3E0)(self, 0) != 0xFF) {
                em_reaction_req(self, 1, 2);
                em_state_set(self, 8, 0);
                break;
            }
            em_state_set(self, 1, 6);
            break;
        }
        break;
    }
}

extern "C" void em007_act7_11(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    f32 temp_f1;
    f32 temp_f31;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        em_hit_window_set(self, 0, arg1 == 1 ? 0x2A : 0x11, 0x80);
        em_move_vec_clr(self);
        temp_f31 = get_em_chg_scale(self);
        temp_f1 = get_em_base_scale(self);
        self->field_0x318 = -13.703704f * temp_f1 * temp_f31;
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 54.0f) == 1) {
            em_turn_to_target(self, 0x280);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_12(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_3, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_3) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act7_13(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;

    VEC3_ctor(&sp14);
    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        em_hit_window_set(self, 0, 1, 0x88);
        em_stamina_add(self, (u32)-0x14);
        break;
    case 1:
        if (em_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 0x29, &sp14);
            ((Em007ShellPosJobFn)shell_set_func_ptr->method_0x14)(self, &sp14, 0x18, self->area_no, self->field_0xAEA);
        }
        if (em_frame_check(self, 0, 132.0f, 0.0f) == 1) {
            em_camera_req(self, 0x29, 5);
            setVector3(&sp8, 150.0f, 0.0f, 0.0f);
            ((Em007ShellSpawnFn)shell_set_func_ptr->method_0x2C)(self, 9, 0x29, &sp8, 1.5f, self->field_0xAEA);
            stg_ground_impact_set(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act7_14(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_4, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_4) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act7_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em007_act7_0(self); break;
    case 1: em007_act7_1(self); break;
    case 2: em007_act7_2(self, 0); break;
    case 3: em007_act7_3(self, 0); break;
    case 4: em007_act7_4(self); break;
    case 5: em007_act7_5(self, 0); break;
    case 6: em007_act7_6(self, 0); break;
    case 7: em007_act7_6(self, 1); break;
    case 8: em007_act7_8(self, 0, 0); break;
    case 9: em007_act7_8(self, 1, 0); break;
    case 10: em007_act7_10(self); break;
    case 11: em007_act7_11(self, 0); break;
    case 12: em007_act7_12(self); break;
    case 13: em007_act7_13(self); break;
    case 14: em007_act7_14(self); break;
    case 15: em007_act7_0(self); break;
    case 16: em007_act7_11(self, 1); break;
    case 17: em007_act7_2(self, 1); break;
    case 18: em007_act7_3(self, 1); break;
    case 19: em007_act7_8(self, 2, 0); break;
    case 20: em007_act7_8(self, 3, 0); break;
    case 21: em007_act7_8(self, 0, 1); break;
    case 22: em007_act7_8(self, 1, 1); break;
    case 23: em007_act7_8(self, 2, 1); break;
    case 24: em007_act7_8(self, 3, 1); break;
    case 25: em007_act7_8(self, 0, 2); break;
    case 26: em007_act7_8(self, 1, 2); break;
    case 27: em007_act7_8(self, 2, 2); break;
    case 28: em007_act7_8(self, 3, 2); break;
    case 29: em007_act7_12(self); break;
    case 30: em007_act7_14(self); break;
    case 31: em007_act7_5(self, 1); break;
    }
}

extern "C" u32 em007_partner_front_ck(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 temp_r31;
    u8* temp_r31_2;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    temp_r31 = (s32)get_move_work_adrs(2);
    if ((s32)arg1 < (s32)get_move_work_max(2)) {
        temp_r31_2 = (u8*)(temp_r31 + (arg1 * 0xB20));
        if (*temp_r31_2 != 0) {
            setVector3(&sp2C, 0.0f, 0.0f, 1.0f);
            rotVecY(&sp2C, self->field_0x1C0);
            vec3_scale(&sp8, &sp2C, 300.0f * get_em_chg_scale(self));
            addVec3(&sp14, &self->pos, &sp8);
            copyVec3(&sp20, &sp14);
            if (vec3_dist_sq(temp_r31_2 + 0x3C, &sp20) <= 40000.0f) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void em007_act8_0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp3C;
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;
    f32 temp_f31;
    s32 temp_r0;

    VEC3_ctor(&sp3C);
    VEC3_ctor(&sp30);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em007_aim_point_get(self, &sp3C, &sp8, 250.0f);
        subVec3(&sp24, &sp3C, &self->pos);
        copyVec3(&sp30, &sp24);
        em_target_pos_set(self, &sp3C);
        em_turn_seq_start(self, (void*)em007_turn_seq_6, 2, 1, sp8);
        em_move_vec_clr(self);
        temp_f31 = get_em_base_scale(self);
        vec3_scale(&spC, &sp30, 0.02f);
        vec3_scale(&sp18, &spC, temp_f31);
        copyVec3(&self->offset_0x30C.vec_0x310, &sp18);
        self->field_0x314 = 0.0f;
        break;
    case 1:
        em_timer_0x81C_arm(self);
        if (em_frame_check(self, 3, 12.0f, 62.0f) == 1) {
            em_move_offset_apply(self);
            em_turn_seq_step(self, (void*)em007_turn_seq_6);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->timer_0x020 = 0x5A;
            em_mot_set(self, 0x35, 2, 0);
            break;
        }
        break;
    case 2:
        em_timer_0x81C_arm(self);
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 < 0) {
            self->timer_0x020 = 0;
        }
        if (em_busy_ck(self) == 1) {
            if (em007_partner_front_ck(self, self->field_0x382) == 1) {
                if (em_team_mate_state_ck(self, 1, self->field_0x382) == 1) {
                    self->field_0x1E7 = 5;
                    if (em_eft_active_ck(self) == 1) {
                        em_state_set(self, 9, 3);
                        break;
                    }
                    em_state_set(self, 9, 1);
                    break;
                }
                if ((self->timer_0x020 <= 0) || (em_team_mate_state_ck(self, 0, self->field_0x382) == 0)) {
                    em_state_set(self, 1, 7);
                    break;
                }
                break;
            }
            em_state_set(self, 1, 7);
            break;
        }
        break;
    }
}

extern "C" void em007_act8_0(struct _ENEMY_WORK* self);

extern "C" void em007_act8_dispatch(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em007_act8_0(self);
    }
}

extern "C" void em007_act9_0(struct _ENEMY_WORK* self) {
    em_timer_0x81C_arm(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x35, 4, 0);
        break;
    case 1:
        if (em_busy_ck(self) == 1) {
            if ((em_team_mate_state_ck(self, 1, self->field_0x382) == 1) &&
                (em007_partner_front_ck(self, self->field_0x382) == 1)) {
                self->field_0x1E7--;
                if (self->field_0x1E7 == 0) {
                    em_state_set(self, 9, 2);
                    break;
                }
                if (em_eft_active_ck(self) == 1) {
                    em_state_set(self, 9, 3);
                    break;
                }
                em_state_set(self, 9, 1);
                break;
            }
            em_state_set(self, 1, 7);
        } else {
            break;
        }
        break;
    }
}

extern "C" void em007_act9_1(struct _ENEMY_WORK* self, u8 arg1) {
    em_timer_0x81C_arm(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        em_hit_window_set_default(self, 0, arg1 == 1 ? 0x2D : 0x14);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                f32 scale = 1.0f;
                em_gauge_period_step(self, (s32)(0.005f * (f32)(s32)self->field_0x7A4), 1, scale);
                em_stamina_regen(self, 1.0f, 0x32, 1);
            }
            if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                em_camera_req(self, 0x16, 7);
            }
            if (em_mot_end_ck(self) != 1) {
                break;
            }
            self->state_0x006++;
            em_mot_set(self, 0x35, 4, 0);
        case 1:
            if (em_busy_ck(self) == 1) {
                if (em_team_mate_state_ck(self, 1, self->field_0x382) == 1 && em007_partner_front_ck(self, self->field_0x382) == 1) {
                    em_state_set(self, 9, 0);
                    break;
                }
                em_state_set(self, 1, 7);
            }
            break;
        }
        break;
    }
}

extern "C" void em007_act9_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        em_hit_window_set_default(self, 0, 0x19);
        em_timer_0x81C_arm(self);
        camera_flag_0x145_set();
        break;
    case 1:
        if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            camera_target_slot_clear(0);
        }
        if (em_frame_check(self, 2, 58.0f, 0.0f) == 1) {
            em_timer_0x81C_arm(self);
        }
        if (em_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em_drop_item_spawn(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void em007_act9_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em007_act9_0(self); return;
    case 1: em007_act9_1(self, 0); return;
    case 2: em007_act9_2(self); return;
    case 3: em007_act9_1(self, 1); return;
    }
}

/* The sound programs `em007_act10_se_play`/`em007_act11_se_play` run, keyed by sub-state. */

EmSeRecord em007_se_rec_17 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_19 = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_19[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_19},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_18 = {107, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_18},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em007_se_pos_1B = {88.0f, 160.0f, -1, -16384, 0};

EmSeRecord em007_se_rec_1B = {103, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_1B[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em007_se_pos_1B},
    {0x00, {0, 0, 0}, &em007_se_rec_1B},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em007_se_pos_1C = {88.0f, 160.0f, 0, 16384, 0};

EmSeRecord em007_se_rec_1C = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_1C[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em007_se_pos_1C},
    {0x00, {0, 0, 0}, &em007_se_rec_1C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_58 = {19, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_58[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_58},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_5C_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em007_se_rec_5C_b = {121, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em007_se_rec_5C_c = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_5C[5] = {
    {0x00, {0, 0, 0}, &em007_se_rec_5C_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em007_se_rec_5C_b},
    {0x00, {0, 0, 0}, &em007_se_rec_5C_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_5D_a = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em007_se_rec_5D_b = {120, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em007_se_rec_5D_c = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_5D[5] = {
    {0x00, {0, 0, 0}, &em007_se_rec_5D_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em007_se_rec_5D_b},
    {0x00, {0, 0, 0}, &em007_se_rec_5D_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_1D = {123, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_1D[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_1D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_7A = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_7B = {110, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_9F = {112, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_7C = {111, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_7C},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_8D = {15, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_78 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_78},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_7D_a = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em007_se_rec_7D_b = {120, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_7D[3] = {
    {0x00, {0, 0, 0}, &em007_se_rec_7D_a},
    {0x00, {0, 0, 0}, &em007_se_rec_7D_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_8E = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_8E},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_A8 = {100, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_B6 = {117, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_B6},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_B7 = {118, 0, 2, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_B7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_B8 = {124, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_B8[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_B8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_B9 = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_B9},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_BA = {119, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em007_se_rec_BA},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_BB = {119, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em007_se_rec_BB},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em007_se_rec_BF_a = {107, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em007_se_rec_BF_b = {111, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em007_se_seq_BF[5] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em007_se_rec_BF_a},
    {0x00, {0, 0, 0}, &em007_se_rec_BF_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSePosRec em007_se_pos_C9_a = {146.0f, 160.0f, -1, -16384};

EmSePosRec em007_se_pos_C9_b = {270.0f, 324.0f, -1, -16384};

EmSeRecord em007_se_rec_C9_a = {122, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em007_se_rec_C9_b = {122, 0, 0, 200, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_seq_C9[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC8)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em007_se_pos_C9_a},
    {0x00, {0, 0, 0}, &em007_se_rec_C9_a},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em007_se_pos_C9_b},
    {0x00, {0, 0, 0}, &em007_se_rec_C9_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_alt_rec_00 = {108, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_alt_seq_00[3] = {
    {0x00, {0, 0, 0}, &em007_se_alt_rec_00},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_alt_rec_0F = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_alt_seq_0F[2] = {
    {0x00, {0, 0, 0}, &em007_se_alt_rec_0F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_alt_rec_1C = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_alt_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em007_se_alt_rec_1C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em007_se_alt_rec_1A_a = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em007_se_alt_rec_1A_b = {110, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em007_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em007_se_alt_rec_1A_a},
    {0x00, {0, 0, 0}, &em007_se_alt_rec_1A_b},
    {0xFF, {0, 0, 0}, 0},
};

extern "C" void em007_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: em_se_tbl_play(self, em007_se_seq_17, 0, 0x17); return;
    case 0x18: em_se_tbl_play(self, em007_se_seq_18, 0, 0x18); return;
    case 0x19: em_se_tbl_play(self, em007_se_seq_19, 0, 0x19); return;
    case 0x1A: em_se_tbl_play(self, em007_se_seq_19, 0, 0x1A); return;
    case 0x1B: em_se_tbl_play(self, em007_se_seq_1B, 0, 0x1B); return;
    case 0x1C: em_se_tbl_play(self, em007_se_seq_1C, 0, 0x1C); return;
    case 0x1D: em_se_tbl_play(self, em007_se_seq_1D, 0, 0x1D); return;
    case 0x58: em_se_tbl_play(self, em007_se_seq_58, 0, 0x58); return;
    case 0x5C: em_se_tbl_play(self, em007_se_seq_5C, 0, 0x5C); return;
    case 0x5D: em_se_tbl_play(self, em007_se_seq_5D, 0, 0x5D); return;
    case 0x7A: em_se_tbl_play(self, em007_se_seq_7A, 0, 0x7A); return;
    case 0x7B: em_se_tbl_play(self, em007_se_seq_7B, 0, 0x7B); return;
    case 0x9F: em_se_tbl_play(self, em007_se_seq_9F, 0, 0x9F); return;
    case 0xA0: em_se_tbl_play(self, em007_se_seq_9F, 0, 0xA0); return;
    case 0x7C: em_se_tbl_play(self, em007_se_seq_7C, 0, 0x7C); return;
    case 0x8D: em_se_tbl_play(self, em007_se_seq_8D, 0, 0x8D); return;
    case 0x7D: em_se_tbl_play(self, em007_se_seq_7D, 0, 0x7D); return;
    case 0x8E: em_se_tbl_play(self, em007_se_seq_8E, 0, 0x8E); return;
    case 0x78: em_se_tbl_play(self, em007_se_seq_78, 0, 0x78); return;
    case 0xA8: em_se_tbl_play(self, em007_se_seq_A8, 0, 0xA8); return;
    case 0xB6: em_se_tbl_play(self, em007_se_seq_B6, 0, 0xB6); return;
    case 0xB7: em_se_tbl_play(self, em007_se_seq_B7, 0, 0xB7); return;
    case 0xB8: em_se_tbl_play(self, em007_se_seq_B8, 0, 0xB8); return;
    case 0xB9: em_se_tbl_play(self, em007_se_seq_B9, 0, 0xB9); return;
    case 0xBA: em_se_tbl_play(self, em007_se_seq_BA, 0, 0xBA); return;
    case 0xBB: em_se_tbl_play(self, em007_se_seq_BB, 0, 0xBB); return;
    case 0xBC: em_se_tbl_play(self, em007_se_seq_BB, 0, 0xBC); return;
    case 0xBF: em_se_tbl_play(self, em007_se_seq_BF, 0, 0xBF); return;
    case 0xC1: em_se_tbl_play(self, em007_se_seq_8D, 0, 0xC1); return;
    case 0xC9: em_se_tbl_play(self, em007_se_seq_C9, 0, 0xC9); return;
    case 0xF0: em_se_tbl_play(self, em007_se_seq_18, 0, 0xF0); return;
    default: em_action_finish(self); return;
    }
}

extern "C" void em007_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em_se_tbl_play_alt(self, em007_se_alt_seq_00, 0, 0); return;
    case 15: em_se_tbl_play_alt(self, em007_se_alt_seq_0F, 0, 0xF); return;
    case 26: em_se_tbl_play_alt(self, em007_se_alt_seq_1A, 0, 0x1A); return;
    case 28: em_se_tbl_play_alt(self, em007_se_alt_seq_1C, 0, 0x1C); return;
    default: em_se_tbl_play_alt(self, em007_se_alt_seq_00, 0, 0); return;
    }
}

extern "C" void em007_act12_0(struct _ENEMY_WORK* self) {
    em007_act6_5(self);
}

extern "C" void em007_act12_dispatch(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em007_act12_0(self);
    }
}

extern "C" void em007_act13_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em007_turn_seq_0, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em007_turn_seq_0) == 1) {
            em_state_set(self, 0xD, 1);
        }
        return;
    }
}

extern "C" void em007_act13_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_hit_window_set(self, 0, 0xC, 2);
        em_stamina_add(self, (u32)-0x14);
        em_approach_start(self, -1000.0f, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 0xD, 2);
        }
        return;
    }
}

extern "C" void em007_act13_2(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;

    VEC3_ctor(&sp14);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_hit_window_set(self, 0, 0x1D, 8);
        em_hit_window_set(self, 1, 0x36, 0x110);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - 476.0f;
        if (var_f31 > 684.0f) {
            var_f31 = 684.0f;
        } else if (var_f31 < 0.0f) {
            var_f31 = 0.0f;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / 12.0f;
        break;
    case 1:
        if (em_frame_check(self, 3, 2.0f, 30.0f) == 1) {
            em_turn_to_target(self, 0x300);
        }
        if (em_frame_check(self, 3, 72.0f, 94.0f) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, 104.0f, 0.0f) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, 0.0f, 0.0f, 0.0f));
            ((Em007ShellSpawnFn)shell_set_func_ptr->method_0x2C)(self, 5, 0x21, &sp14, 1.0f, self->field_0xAEA);
        }
        if ((em_frame_check(self, 1, 138.0f, 0.0f) == 1) && (em_busy_ck(self) == 1)) {
            if (em007_ally_wait_near_ck(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
                break;
            }
            em_state_set(self, 1, 6);
            break;
        }
        break;
    }
}

extern "C" void em007_act13_3(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;

    VEC3_ctor(&sp30);
    VEC3_ctor(&sp24);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em007_aim_point_get(self, &sp30, &sp8, 300.0f);
        subVec3(&sp18, &sp30, &self->pos);
        copyVec3(&sp24, &sp18);
        em_target_pos_set(self, &sp30);
        em_turn_seq_start(self, (void*)em007_turn_seq_6, 2, 1, sp8);
        em_move_vec_clr(self);
        vec3_scale(&spC, &sp24, 1.0f / em_mot_frame_end_get(self));
        copyVec3(&self->offset_0x30C.vec_0x310, &spC);
        self->field_0x314 = 0.0f;
        break;
    case 1:
        em_move_offset_apply(self);
        em_turn_seq_step(self, (void*)em007_turn_seq_6);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x35, 2, 0);
            break;
        }
        break;
    case 2:
        if (em_busy_ck(self) == 1) {
            if (em007_ally_wait_near_ck(self, 1) == 1) {
                em_state_set(self, 0xD, 4);
                break;
            }
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void em007_act13_4(struct _ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        em_drop_item_spawn(self, &self->pos, 0x64);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state_0x006++;
            if (self->state_0x006 >= 3) {
                em_state_set(self, 0xD, 5);
            }
        }
        break;
    }
}

extern "C" void em007_act13_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        em_stamina_add(self, 0x3E8);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void em007_act13_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em007_act13_0(self); return;
    case 1: em007_act13_1(self); return;
    case 2: em007_act13_2(self); return;
    case 3: em007_act13_3(self); return;
    case 4: em007_act13_4(self); return;
    case 5: em007_act13_5(self); return;
    }
}

extern "C" void em007_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0: em007_act0_dispatch(self); break;
    case 1: em007_act1_dispatch(self); break;
    case 2: em007_act2_dispatch(self); break;
    case 6: em007_act6_dispatch(self); break;
    case 7: em007_act7_dispatch(self); break;
    case 8: em007_act8_dispatch(self); break;
    case 9: em007_act9_dispatch(self); break;
    case 10: em007_act10_se_play(self); break;
    case 11: em007_act11_se_play(self); break;
    case 12: em007_act12_dispatch(self); break;
    case 13: em007_act13_dispatch(self); break;
    }
    if (self->field_0x1E2 == 4) {
        em_busy_timer_reset(self);
    }
}

extern "C" void em007_alt_parts_update(struct _ENEMY_WORK* self) {
    if (em_alt_mode_ck(self) == 1) {
        if (self->action_0x328.field_0x344 == 0) {
            em_part_rec_alt_set(self, 0, 0);
            em_part_rec_alt_set(self, 1, 1);
            em_part_rec_alt_set(self, 2, 2);
            em_part_rec_alt_set(self, 3, 3);
            em_part_rec_alt_set(self, 4, 4);
            em_part_rec_alt_set(self, 5, 5);
            self->action_0x328.field_0x344 = 1;
        }
    } else if (self->action_0x328.field_0x344 == 1) {
        em_part_rec_reset(self, 0);
        em_part_rec_reset(self, 1);
        em_part_rec_reset(self, 2);
        em_part_rec_reset(self, 3);
        em_part_rec_reset(self, 4);
        em_part_rec_reset(self, 5);
        self->action_0x328.field_0x344 = 0;
    }
}

/* ===================================================================================================
 * the effect router, the motion events, the program-table slots and the model effect
 * =================================================================================================== */

/* The C++-mangled callees, declared with the signature each mangling encodes (rule 9). */

void rotVecY(nw4r::math::VEC3* v, u32 angle);                    /* rotVecY__FPQ34nw4r4math4VEC3Ul */
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotMatrixX(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixX__FUlPQ34nw4r4math5MTX34 */
void rotMatrixZ(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixZ__FUlPQ34nw4r4math5MTX34 */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                          /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                       /* em_get_mot_no__FP11_ENEMY_WORK */
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                                  /* em_after_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);                  /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                        /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */

/* The runtime's allocator pair (`__nw__FUl`/`__dl__FPv`), declared as the C++ functions (rule 9). */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();


/* The 0x60-byte work block `em007_model_eft_alloc` asks `eft_res_slot_get` for, hung off the effect's +0x38.
 * size: 0x60 */
struct Em007EftWork {
    /* +0x00 */ s32 model_count_0x00;            /* always 1 */
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ struct MHchar* models_0x08[1];   /* the pooled models */
    /* +0x0C */ nw4r::ef::Effect* effect_0x0C;   /* the main effect */
    /* +0x10 */ nw4r::ef::Effect* effects_0x10[5];
    /* +0x24 */ Em007ModelCopy* copy_0x24;       /* the model copy */
    /* +0x28 */ u8 mode_0x28;                    /* 0: an enemy's effect, 1: the lobby NPC's */
    /* +0x29 */ u8 node_count_0x29;
    /* +0x2A */ u8 kind_0x2A;                    /* the model kind (the enemy's +0x420 byte) */
    /* +0x2B */ u8 pad_0x2B[0x50 - 0x2B];
    /* +0x50 */ s32 angle_0x50;                  /* cleared on creation for the kind-0/5 companions */
    /* +0x54 */ u8 pad_0x54[0x58 - 0x54];
    /* +0x58 */ u16 light_0x58;                  /* the light id (`light_id_next` for the kind-1 companion, else 0xFFFF) */
    /* +0x5A */ u8 field_0x5A;                   /* cleared on creation */
    /* +0x5B */ u8 field_0x5B;                   /* cleared on creation */
    /* +0x5C */ u8 pad_0x5C;
    /* +0x5D */ u8 area_0x5D;                    /* the kind-4 companion's area */
    /* +0x5E */ u8 pad_0x5E[0x60 - 0x5E];
};

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-band callees, declared with the call sites' signatures (`enemy/em_common.cpp`'s first). */
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);
u32 em_eft_active_ck(struct _ENEMY_WORK* self);

void em_data_point_get(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
u32 em_area_stun_move_ck(struct _ENEMY_WORK* self);
u32 em_area_roam_pick(struct _ENEMY_WORK* self, u16 a);
u8 em_area_break_ck(struct _ENEMY_WORK* self, u8 area);
u32 em_pack_area_pick(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 em_area_leave_ck(struct _ENEMY_WORK* self);

u32 em_act11_wait_ck(struct _ENEMY_WORK* self);

void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
struct _ENEMY_WORK* em_ally_find(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 em_flags836_ck(struct _ENEMY_WORK* self, u32 a);
void em_res_user_data_flag_set(struct _ENEMY_WORK* self, u32 a);

/* `enemy/fn_80138074.c` */

/* The model effect's step functions: `em007_model_eft_create` and three of
 * `lobby/fn_801E0ADC.cpp`. */
void em007_model_eft_create(_EFT* self);
void em007_model_eft_step(_EFT* self);
void em007_model_eft_release(_EFT* eft);
_EFT* em007_model_eft_alloc(void);
void em007_model_eft_update(_EFT* self);
void em007_model_eft_next(_EFT* self);
void em007_model_eft_retire(_EFT* self);

/* The effect clusters. */
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void eft_spawn_type11(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);

/* Runtime helpers. */
f32 vec3_distance(void* a, void* b);
void res_node_assign(void* out, void* src);
void* em007_res_name_at(void** base, u32 offset);
void em007_model_copy_bind_nodes(struct Em007EftWork* work, u8 index);

/* `stage_map_kind_get` and `get_now_areano` come from `stage/stg_w.h`, `get_now_mapno` from
 * `unsplit/unknown.h`. */

/* ----------------------------------------------------------------------------------------------------
 * the definitions (C linkage: they keep the map's own `fn_XXXXXXXX` names)
 * -------------------------------------------------------------------------------------------------- */

/* 0x801DB8E0 (0x98): arms the +0x1BC/-0x20C effect spawn once the work record answers the map query
 * and `system_w`'s counter has run a multiple of 24 frames. */
void em007_alt_mode_effect(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) == 1) {
        if (system_w.field_0x0c % 0x18 == 0) {
            setVector3(&pos, 0.0f, -30.0f, 120.0f);
            eft_spawn_type10(self, 0x13, 0x16, &pos, 1.0f);
        }
    }
}

/* 0x801DB978 (0x494): the effect-spawn dispatcher: per effect id, `eft009_set_pos` when the joint is 0xFF,
 * else the joint goes to `eft009_spawn_at_joint`/`eft_spawn_pos_in_area`/`eft_spawn_type11`. */
void em007_effect_spawn(struct _ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, u32 id, f32 scale) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (mode) {
    case 0:
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
                kind = 0xd;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xf, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xf, id, scale);
                }
                break;
            case 2:
                kind = 0xc;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xe, id, scale);
                }
                break;
            case 4:
                kind = 0x13;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xe, id, scale);
                }
                break;
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 0xa:
            case 0xb:
            case 0x24:
            case 0x25:
            case 0x29:
            case 0x2a:
                return;
            default:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            }
        } else {
            switch (kind) {
            case 0xc:
            case 0xd:
            case 0xe:
            case 0xf:
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
            default:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x20C;
                    pos.z = self->pos.z;
                }
                break;
            }
        }
        if (joint == 0xff) {
            eft009_set_pos(kind, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, kind, id, scale);
        }
        break;
    case 1:
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
            case 6:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x11, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x11, id, scale);
                }
                return;
            case 1:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x10, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x10, id, scale);
                }
                return;
            case 5:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x14, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x14, id, scale);
                }
                return;
            }
        }
        if (joint == 0xff) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale = scale * get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, kind, id, scale);
        break;
    case 2:
        if (joint == 0xff) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_type11(self, &pos, kind, scale);
        break;
    }
}

void em007_alt_scale_step(struct _ENEMY_WORK* self);

/* `em_act_ck` read unsigned (retail compares its answer with `cmplwi`). */
#define em_act_ck_u ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)

/* 0x801DBE0C (0x34EC): the per-motion events: effect, camera and hit-window cues keyed by the current motion
 * and its frame. */
void em007_motion_events(struct _ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    _CP_VECTOR rot;
    f32 scale;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    em007_alt_parts_update(self);
    em007_alt_mode_effect(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (em_after_frame_check(self, 0, 168.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.8f);
        }
        break;
    case 0x7:
        if ((self->field_0x1E2 != 4) && (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x8:
        if ((self->field_0x1E2 != 4) && (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x9:
        if ((em_after_frame_check(self, 0, 130.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 282.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0xA:
        if (self->field_0x1E2 != 4) {
            if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 192.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 280.0f, 0.0f) == 1)) {
                em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
                em_camera_req(self, 0x23, 0);
            }
            if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 236.0f, 0.0f) == 1)) {
                em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
                em_camera_req(self, 0x2B, 0);
            }
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0xC:
        if ((em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0xD:
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0xF:
        if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 264.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.8f);
        }
        if (em_after_frame_check(self, 0, 230.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x10:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x13:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x1C, 0x80);
        }
        if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 1);
        }
        if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 0, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 174.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 242.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if ((em_after_frame_check(self, 3, 110.0f, 158.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -80.0f, 80.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 1.0f);
            }
        }
        break;
    case 0x14:
        if ((em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 28.0f, 64.0f) == 1) || (em_after_frame_check(self, 3, 120.0f, 138.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -80.0f, 80.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1)) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 1);
        }
        break;
    case 0x15:
        if (em_after_frame_check(self, 0, 182.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x16:
        if ((em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 2.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 28.0f, 46.0f) == 1) || (em_after_frame_check(self, 3, 100.0f, 118.0f) == 1) || (em_after_frame_check(self, 3, 172.0f, 190.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -80.0f, 80.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1)) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 2.5f);
            em_camera_req(self, 0x2B, 1);
        }
        if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 1);
        }
        break;
    case 0x17:
        if ((em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0xD, 0x2B, 0, 1.5f);
                em007_effect_spawn(self, 0, 0xF, 0x2B, 0, 1.5f);
            } else {
                em007_effect_spawn(self, 0, 0, 0x2B, 0, 1.8f);
            }
        }
        if (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
            em_camera_req(self, 0x2B, 1);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0xD, 0x2B, 0, 1.5f);
                em007_effect_spawn(self, 0, 0xF, 0x2B, 0, 1.5f);
            } else {
                em007_effect_spawn(self, 0, 0, 0x2B, 0, 1.8f);
            }
        }
        if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0xD, 0x2B, 0, 1.5f);
                em007_effect_spawn(self, 0, 0xF, 0x2B, 0, 1.5f);
            } else {
                em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
            }
        }
        if (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
            em_camera_req(self, 0x23, 1);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 0, 0x23, 0, 1.8f);
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 0, 0x2B, 0, 1.8f);
        }
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0x11, 3, 0, 1.3f);
            } else {
                em007_effect_spawn(self, 1, 4, 3, 0, 1.1f);
            }
        }
        break;
    case 0x1A:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0xF) == 1 ? 0x28 : 0xD, 0x88);
            em_hit_window_set(self, 1, 0xE, 0x90);
        }
        if (em_eft_active_ck(self) == 1) {
            if ((em_after_frame_check(self, 3, 20.0f, 38.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0xF) == 1 ? 0x29 : 0xF, 0x88);
            em_hit_window_set(self, 1, 0x10, 0x90);
        }
        if (em_eft_active_ck(self) == 1) {
            if ((em_after_frame_check(self, 3, 20.0f, 38.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x1C:
        if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 60.0f, 78.0f) == 1) || (em_after_frame_check(self, 3, 110.0f, 128.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
            if (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 1.0f);
            }
        }
        break;
    case 0x1D:
        if (em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 0x29, 0x4A, 0, 1.2f);
                eft009_spawn_at_joint(self, 0x29, 0x4B, 0, 1.2f);
            } else {
                eft009_spawn_at_joint(self, 0x29, 0x31, 0, 1.2f);
                eft009_spawn_at_joint(self, 0x29, 0x49, 0, 1.2f);
            }
        }
        break;
    case 0x20:
        if (self->field_0x1E2 != 4) {
            if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
            }
            if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
                em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
            }
        }
        break;
    case 0x22:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x24:
        if ((em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x25:
        if ((em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x26:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x16, 0x3B, 0, 0.8f);
            em_camera_req(self, 0x16, 1);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x16, 0x3A, 0xC000, 1.0f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x16, 0x3B, 0, 0.8f);
            em_camera_req(self, 0x16, 1);
        }
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x16, 0x3A, 0x4000, 1.0f);
        }
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x16, 0x3B, 0, 1.0f);
            em_camera_req(self, 0x16, 1);
        }
        if ((em_after_frame_check(self, 3, 178.0f, 298.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            eft009_spawn_at_joint(self, 0x16, 0x3C, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 206.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x27:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 2, 2, 3, self->field_0x1C0, 0.5f);
            } else {
                em007_effect_spawn(self, 1, 4, 3, self->field_0x1C0, 0.5f);
            }
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x39, 0, get_em_scale(self));
            em_camera_req(self, -1, 5);
        }
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 230.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x28:
        if (em_after_frame_check(self, 0, 270.0f, 0.0f) == 1) {
            setVector3(&pos, 100.0f, 0.0f, 0.0f);
            get_joint_wmat_em(self, 3, &mtx);
            mulVecMatAddTrans(&pos, &mtx);
            rot.x = self->field_0x1BC;
            rot.y = self->field_0x1C0 - 0x4000;
            rot.z = self->field_0x1C4;
            scale = 1.3f * get_em_chg_scale(self);
            if ((self->field_0x228 & 6) != 0) {
                pos.y = 5.0f + self->field_0x210;
                eft009_set_pos(0x11, &pos, &rot, scale, self->area_no);
            } else {
                pos.y = 5.0f + self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 0, rot.y, scale);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x29:
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 214.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 238.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em_camera_req(self, 0x16, 5);
        }
        break;
    case 0x2A:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 116.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x2B:
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x2C:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0x1E) == 1 ? 0x2B : 0x12, 0x88);
        }
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 40.0f, 52.0f) == 1) || (em_after_frame_check(self, 3, 70.0f, 82.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if ((em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 156.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 7);
        }
        break;
    case 0x2D:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0x1E) == 1 ? 0x2C : 0x13, 0x88);
        }
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 40.0f, 52.0f) == 1) || (em_after_frame_check(self, 3, 70.0f, 82.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if ((em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2B, 7);
        }
        break;
    case 0x2E:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0x1D) == 1 ? 0x2E : 0x15, 0x88);
            em_hit_window_set(self, 1, 0x16, 0x90);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 50.0f, 68.0f) == 1) || (em_after_frame_check(self, 3, 100.0f, 112.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x2F:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_act_ck_u(self, 7, 0x1D) == 1 ? 0x2F : 0x17, 0x88);
            em_hit_window_set(self, 1, 0x18, 0x90);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 50.0f, 68.0f) == 1) || (em_after_frame_check(self, 3, 100.0f, 112.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
                eft_em_spawn(self, 0x6E, 0x16, &pos, 1.0f);
            }
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                setVector3(&pos, 0.0f, 0.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x30:
        if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2B, 1);
        }
        break;
    case 0x31:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x32:
        if (em_eft_active_ck(self) == 1) {
            if (((em_after_frame_check(self, 3, 2.0f, 20.0f) == 1) || (em_after_frame_check(self, 3, 170.0f, 188.0f) == 1)) && ((system_w.field_0x0c & 3) == 0)) {
                setVector3(&pos, 0.0f, -50.0f, 50.0f);
                eft_em_spawn(self, 0x6D, 0x16, &pos, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                setVector3(&pos, 0.0f, -20.0f, 80.0f);
                eft_em_spawn(self, 0x6F, 0x16, &pos, 1.5f);
            }
        }
        break;
    case 0x33:
        if (em_after_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 204.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x34:
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 214.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 238.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em_camera_req(self, 0x16, 5);
        }
        break;
    case 0x36:
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x64:
        if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 198.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x66:
        if ((em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 104.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 236.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_camera_req(self, 0x2B, 1);
        }
        break;
    case 0x67:
        if ((em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 236.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 1);
        }
        break;
    case 0x68:
    case 0x69:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0x14, 3, 0, 2.0f);
            } else {
                em007_effect_spawn(self, 1, 5, 3, 0, 1.0f);
            }
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.8f);
        }
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 186.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x6C:
        if (em_after_frame_check(self, 0, 292.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 1, 1, 3, 0, 1.3f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x6D:
        if (em_after_frame_check(self, 0, 196.0f, 0.0f) == 1) {
            setVector3(&pos, 180.0f, 0.0f, 0.0f);
            get_joint_wmat_em(self, 3, &mtx);
            mulVecMatAddTrans(&pos, &mtx);
            rot.x = self->field_0x1BC;
            rot.y = self->field_0x1C0 - 0x4000;
            rot.z = self->field_0x1C4;
            if ((self->field_0x228 & 6) != 0) {
                pos.y = 5.0f + self->field_0x210;
                eft009_set_pos(0x11, &pos, &rot, 1.3f * get_em_chg_scale(self), self->area_no);
            } else {
                pos.y = 5.0f + self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 0, rot.y, 1.8f * get_em_chg_scale(self));
            }
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x70:
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 2.0f);
            em_camera_req(self, 0x23, 7);
        }
        if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 220.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 2.0f);
            em_camera_req(self, 0x2B, 7);
        }
        if (em_after_frame_check(self, 0, 172.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 218.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x75:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0x14, 3, 0, 2.0f);
            } else {
                em007_effect_spawn(self, 1, 2, 3, 0, 1.0f);
            }
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x76:
        if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            get_em_chg_scale(self);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x16, 0xC, (s32) self->field_0x1C0, 1.3f, &pos);
                eft009_spawn_at_pos(self, 0x16, 0xE, (s32) self->field_0x1C0, 1.3f, &pos);
            } else {
                eft009_spawn_at_pos(self, 0x16, 2, (s32) self->field_0x1C0, 1.5f * get_em_chg_scale(self), &pos);
            }
        }
        break;
    case 0x77:
        if (em_after_frame_check(self, 0, 118.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 1, 0, 0x14, 0, 1.2f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x7A:
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0x11, 3, 0, 1.3f);
            } else {
                em007_effect_spawn(self, 1, 5, 3, 0, 0.8f);
            }
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em007_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em007_effect_spawn(self, 1, 6, 3, 0, 2.0f);
            }
        }
        if (em_after_frame_check(self, 0, 302.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 414.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x7B:
        if ((em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 104.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if ((em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0x7C:
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 198.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    case 0xC8:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em007_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
        }
        break;
    case 0xC9:
        if ((em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 248.0f, 0.0f) == 1)) {
            em007_effect_spawn(self, 0, 2, 0x2B, 0, 1.5f);
        }
        break;
    }
    em007_alt_scale_step(self);
}

/* 0x801DF2F8 (0x248): fades the four K-colour alphas of the `MHchar` with `field_0x340` (stepped toward
 * `255.0f`/0), scaled and clamped for part kind 4 in area 4/6. */
void em007_tev_color_update(struct _ENEMY_WORK* self) {
    u8 kind;
    u8 alpha;
    _GXColor color;
    u32 fade;
    f32 v;

    if (em_alt_mode_ck(self) == 1) {
        if ((self->action_0x328.field_0x340 += 1.275f) > 255.0f) {
            self->action_0x328.field_0x340 = 255.0f;
        }
    } else {
        if ((self->action_0x328.field_0x340 -= 1.275f) < 0.0f) {
            self->action_0x328.field_0x340 = 0.0f;
        }
    }
    alpha = 0;
    kind = stage_map_kind_get(self->field_0x1E0);
    switch (kind) {
    case 4:
        switch (self->area_no) {
        case 4:
        case 6:
            alpha = 1;
            break;
        }
        break;
    }
    if (alpha == 1) {
        v = 102.0f + 0.6f * self->action_0x328.field_0x340;
        if (v > 255.0f) {
            v = 255.0f;
        }
        alpha = v;
    } else {
        alpha = self->action_0x328.field_0x340;
    }
    ((MHchar*)self->char_0x024)->getTevKColor(0, GX_KCOLOR3, &color);
    color.a = alpha;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR3, &color);
    color.a = alpha;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(2, GX_KCOLOR3, &color);
    color.a = alpha;
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    if (em_flags836_ck(self, 1) == 1) {
        fade = 0;
        switch (kind) {
        case 4:
            switch (self->field_0x48E) {
            case 4:
            case 6:
                fade = 1;
                break;
            }
            break;
        }
        if (fade == 1) {
            if ((f32)color.a > 127.0f) {
                color.a--;
            }
        } else if (color.a != 0) {
            color.a--;
        }
    } else {
        color.a = alpha;
    }
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
}

/* 0x801DF540 (0x28): "is part `a` the aim target": only true for part 0 and when the action block's
 * +0x345 byte is 1. */
u32 em007_part_damage_ck(struct _ENEMY_WORK* self, u8 a) {
    if (a == 0 && self->action_0x328.field_0x345 == 1) {
        return 1;
    }
    return 0;
}

/* 0x801DF568 (0x2A4): arms the part's motion (`em_move_mode_set`) and picks the `em_data_point_get` effect id
 * from the work record's map kind (`stage_map_kind_get`) and `area_no`. */
void em007_area_start_set(struct _ENEMY_WORK* self, u8* outA, u8* outB) {
    u32 kind;
    em_move_mode_set(self, 4);
    *outA = 0xc;
    *outB = 0;
    kind = stage_map_kind_get(self->field_0x1E0);
    switch ((u8)kind) {
    case 1:
        switch (self->area_no) {
        case 4:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 5:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 4:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 6), &self->pos);
            break;
        case 6:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 3:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 3:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 6:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 5:
        switch (self->area_no) {
        case 4:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 6:
            em_data_point_get(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 8:
        switch (self->area_no) {
        case 1:
            em_data_point_get(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    case 9:
        switch (self->area_no) {
        case 0:
            em_data_point_get(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    }
}

/* 0x801DF80C (0x4): empty body (the band's null override). */
void em007_reserved_noop(void) {
}

/* 0x801DF810 (0xDC): "may this part be damaged": the work record's +0x1E2 gate plus the map- and
 * part-level checks for parts 0/4/5. */
u32 em007_part_level_ck(struct _ENEMY_WORK* self, u8 part) {
    if (self->field_0x1E2 == 0) {
        switch (part) {
        case 0:
            if (em_alt_mode_ck(self) == 1) {
                return 1;
            }
            break;
        case 4:
            if (em_eft_active_ck(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 4) & 1) == 0) {
                return 1;
            }
            break;
        case 5:
            if (em_eft_active_ck(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 5) & 1) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x801DF8EC (0x31C): the part-state step: gate on map kind 1..5, test the state (`em_area_break_ck`), request
 * a special part (`em_pack_area_pick`) and arm +0x1FC/+0x1FE/+0x1FF. */
u32 em007_action_select(struct _ENEMY_WORK* self, u16 a) {
    u32 found;
    s32 pick;
    u32 area;
    u8 id;
    u8 state;
    u8 mode;
    u8 kind;

    kind = stage_map_kind_get(self->field_0x1E0);
    if ((u32)(kind - 1) > 4) {
        return 0;
    }
    if (em_area_stun_move_ck(self) == 1) {
        return 1;
    }
    found = 0;
    switch (kind) {
    case 1:
        id = 9;
        break;
    case 2:
        id = 9;
        break;
    case 3:
        id = 0xa;
        break;
    case 4:
        id = 9;
        break;
    case 5:
        id = 4;
        break;
    default:
        id = 0xff;
        break;
    }
    if (id != 0xff) {
        switch (em_area_break_ck(self, id)) {
        case 1:
            found = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (found == 0) {
        switch (kind) {
        case 1:
            pick = 0x1b;
            area = 9;
            break;
        case 2:
            pick = 0x1b;
            area = 6;
            break;
        case 3:
            pick = 0x1b;
            area = 3;
            break;
        case 4:
            pick = 0x1c;
            area = 5;
            break;
        case 5:
            pick = 0x1b;
            area = 3;
            break;
        default:
            pick = 0;
            area = 0xff;
            break;
        }
        if ((pick != 0 || area != 0xff) && em_pack_area_pick(self, pick, area, a, em007_pick_tbl_0, em007_pick_tbl_1) == 1) {
            return 1;
        }
    }
    if (em_area_roam_pick(self, a) == 1) {
        return 1;
    }
    if ((self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) &&
        ((kind == 4 && self->area_no == 1) || (kind == 2 && self->area_no == 9)) && a % 100 < 0x32) {
        if (self->field_0x382 != 0xff) {
            switch (self->field_0x380) {
            default:
                state = 0xff;
                break;
            case 1:
                state = em_move_work_pick(self->state_0x381, self->field_0x382)[0x5a6] & 0x7f;
                break;
            }
            if (state != 0xff && state != em_phase_get(self)) {
                switch (kind) {
                case 2:
                    mode = 8;
                    break;
                case 4:
                    mode = 2;
                    break;
                default:
                    mode = 0xff;
                    break;
                }
                if (mode != 0xff) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 0xa;
                    self->field_0x1FF = mode;
                    return 1;
                }
            }
        }
    }
    return em_area_leave_ck(self) == 1;
}

/* 0x801DFC08 (0x134): steps the four action-block floats toward their clamps (up when `em_alt_mode_ck`
 * answers 1, down otherwise). */
void em007_alt_scale_step(struct _ENEMY_WORK* self) {
    EmActionBlock* blk = &self->action_0x328;
    if (em_alt_mode_ck(self) == 1) {
        blk->vec_0x328.x += 0.083333336f;
        if (blk->vec_0x328.x > 1.5f) {
            blk->vec_0x328.x = 1.5f;
        }
        blk->vec_0x328.y += 1.0f;
        if (blk->vec_0x328.y > 7.0f) {
            blk->vec_0x328.y = 7.0f;
        }
        blk->vec_0x334.x += 0.33333334f;
        if (blk->vec_0x334.x > 3.0f) {
            blk->vec_0x334.x = 3.0f;
        }
        blk->vec_0x334.y += 0.9166667f;
        if (blk->vec_0x334.y > 6.5f) {
            blk->vec_0x334.y = 6.5f;
        }
    } else {
        blk->vec_0x328.x -= 0.083333336f;
        if (blk->vec_0x328.x < 1.0f) {
            blk->vec_0x328.x = 1.0f;
        }
        blk->vec_0x328.y -= 1.0f;
        if (blk->vec_0x328.y < 1.0f) {
            blk->vec_0x328.y = 1.0f;
        }
        blk->vec_0x334.x -= 0.33333334f;
        if (blk->vec_0x334.x < 1.0f) {
            blk->vec_0x334.x = 1.0f;
        }
        blk->vec_0x334.y -= 0.9166667f;
        if (blk->vec_0x334.y < 1.0f) {
            blk->vec_0x334.y = 1.0f;
        }
    }
}

/* 0x801DFD3C (0x170): aims at `self->vec_0x36C`: turns the angle from `pos` into a rotation about y
 * and writes the resulting point into `out`. */
void em007_aim_point_get(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 scale) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 rot;
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 res;
    u16 ang;
    u16 delta;
    VEC3_ctor(&dir);
    VEC3_ctor(&rot);
    copyVec3(&dir, &self->vec_0x36C);
    ang = calcVecAng2(&self->pos, &dir);
    *angleOut = ang;
    delta = (u16)(ang - self->field_0x1C0);
    scale = scale * get_em_chg_scale(self);
    if ((u16)(delta + 0xbfff) > 0x7ffe) {
        f32 base = vec3_distance(&dir, &self->pos);
        setVector3(&rot, 0.0f, 0.0f, base - scale);
        rotVecY(&rot, *angleOut);
        addVec3(&tmp, &self->pos, &rot);
        copyVec3(out, &tmp);
    } else {
        u16 a2 = (delta < 0x8000) ? 0x4000 : 0xc000;
        *angleOut = (u16)(a2 + self->field_0x1C0);
        setVector3(&rot, 0.0f, 0.0f, -scale);
        rotVecY(&rot, *angleOut);
        addVec3(&res, &dir, &rot);
        copyVec3(out, &res);
    }
}

/* 0x801DFEAC (0x40): Sets the accessor's flag word to 7 once it is installed. */
void Em007UserData::Attach() {
    nw4r::g3d::ResNode tmp((void*)0);
    em_res_user_data_flag_set((struct _ENEMY_WORK*)this, 7);
}

/* 0x801DFEEC (0x94): For an item of kind 0xFF, node 0x1B takes its result scale from the work's +0x328 vector and
 * node 0x1A from +0x334. */
void Em007UserData::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 joint, UserDataItem* item) {
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3* scale;

    VEC3_ctor(&unused);
    scale = &this->work->action_0x328.vec_0x328;
    switch (item->field_0x04) {
    default:
        break;
    case 0xFF:
        switch (joint) {
        case 0x1B:
            anm_chr_result_scale_set((struct ResAnmChrObj*)pResult, &scale->x);
            break;
        case 0x1A:
            anm_chr_result_scale_set((struct ResAnmChrObj*)pResult, &scale[1].x);
            break;
        }
    }
}

/* 0x801DFF80 (0xD8): Tilts node 0x11's world matrix by the work's X/Z rotation words (+0x608/+0x610), keeping its
 * translation. */
void Em007UserData::CalcWorldEnd(nw4r::math::MTX34* mtx, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
    nw4r::g3d::ResNode node((void*)0);
    nw4r::math::MTX34 rot;
    nw4r::math::MTX34 world;
    s32 data;
    struct _ENEMY_WORK* work;

    MTX34_ctor(&rot);
    MTX34_ctor(&world);
    data = (s32)mdl.GetResNode(0x11).mpData;
    res_node_assign(&node, &data);
    work = this->work;
    mtx34_copy(&world, &mtx[res_node_mtx_id_get(&node)]);
    mtx34_identity(&rot);
    rotMatrixX(work->field_0x608, &rot);
    rotMatrixZ(work->field_0x610, &rot);
    mtx34_concat_assign(&world, &rot);
    copyMat33(&mtx[res_node_mtx_id_get(&node)], &world);
}

/* 0x801E0058 (0x164): "is either part 0x1B or 0x1C within (scale * 1000.0f)^2 of me". */
u32 em007_ally_wait_near_ck(struct _ENEMY_WORK* self, u8 a) {
    struct _ENEMY_WORK* p;
    f32 d;
    f32 lim;
    p = em_ally_find(self, 0x1b, 0);
    if (p != NULL) {
        if (em_act11_wait_ck(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (1000.0f * get_em_chg_scale(self)) *
                  (1000.0f * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    p = em_ally_find(self, 0x1c, 0);
    if (p != NULL) {
        if (em_act11_wait_ck(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (1000.0f * get_em_chg_scale(self)) *
                  (1000.0f * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801E01BC (0x28): "action 0xD with sub-state <= 5". */
u32 em007_act13_early_ck(struct _ENEMY_WORK* self) {
    if (self->action == 0xd && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* 0x801E01E4 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em007UserData::~Em007UserData() {
}

/* 0x801E0240 (0x58): starts the model effect for an enemy: mode 0, the enemy's +0x420 kind, and arms it. */
void em007_model_eft_start(struct _ENEMY_WORK* self) {
    _EFT* eft;
    struct Em007EftWork* work;

    eft = em007_model_eft_alloc();
    if (eft != NULL) {
        work = (struct Em007EftWork*)eft->work_0x38;
        eft->source_0x30 = self;
        work->mode_0x28 = 0;
        work->kind_0x2A = self->flag_0x420;
        eft_state_flags_set(eft, 1, 0);
    }
}

/* 0x801E0298 (0x90): starts the model effect for the lobby NPC 1 (mode 1, `kind`), unless the NPC is missing
 * on map 0x16 outside area 1. */
void em007_npc_model_eft_start(u8 kind) {
    _EFT* eft;
    struct _LB_NPC* npc;
    struct Em007EftWork* work;

    npc = lb_npc_find(1);
    if (npc != NULL || get_now_mapno() != 0x16 || get_now_areano() == 1) {
        eft = em007_model_eft_alloc();
        if (eft != NULL) {
            work = (struct Em007EftWork*)eft->work_0x38;
            eft->source_0x30 = npc;
            work->mode_0x28 = 1;
            work->kind_0x2A = kind;
            eft_state_flags_set(eft, 0, 0);
        }
    }
}

/* 0x801E0328 (0xD8): takes an effect slot with its step/release hooks and one pooled model; NULL when either
 * pool is empty. */
_EFT* em007_model_eft_alloc(void) {
    s32 i;
    _EFT* eft;
    struct Em007EftWork* work;

    eft = (_EFT*)eft_res_slot_get(0x60);
    if (eft == NULL) {
        return NULL;
    }
    eft->dispatch_0x34 = em007_model_eft_step;
    eft->release_0x40 = em007_model_eft_release;
    work = (struct Em007EftWork*)eft->work_0x38;
    work->model_count_0x00 = 1;
    for (i = 0; i < work->model_count_0x00; i++) {
        work->models_0x08[i] = (struct MHchar*)eft_res_model_get();
        if (work->models_0x08[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }
    eft->field_0x03 = 0x20;
    work->field_0x5A = 0;
    work->field_0x5B = 0;
    return eft;
}

/* 0x801E0400 (0xDC): the effect's release hook: returns its effects to the heap, its models to the pool and
 * deletes the model copy. */
void em007_model_eft_release(_EFT* eft) {
    s32 i;
    struct Em007EftWork* work;

    work = (struct Em007EftWork*)eft->work_0x38;
    if (work->effect_0x0C != NULL) {
        push_eft_effect_heap_num(&work->effect_0x0C, 1);
    }
    for (i = 0; i < 5; i++) {
        if (work->effects_0x10[i] != NULL) {
            push_eft_effect_heap_num(&work->effects_0x10[i], 1);
        }
    }
    for (i = 0; i < work->model_count_0x00; i++) {
        eft_res_models_release(&work->models_0x08[i], 1);
    }
    delete work->copy_0x24;
}

/* 0x801E04DC (0x5C): Destroys the model copy through the world-callback base (called out of line). */
#pragma dont_inline on
Em007ModelCopy::~Em007ModelCopy() {
}
#pragma dont_inline reset

/* 0x801E0538 (0x3C): the part-manager state dispatcher (index at +0x5). */
void em007_model_eft_step(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        em007_model_eft_create(self);
        break;
    case 1:
        em007_model_eft_update(self);
        break;
    case 2:
        em007_model_eft_next(self);
        break;
    case 3:
        em007_model_eft_retire(self);
        break;
    }
}

/* 0x801E0574 (0x3C4): Builds the effect's models and model copy, binds the copy to its source (the companion's or the
 * lobby NPC's model) and enters the update state; retires the effect when a model or the copy cannot be made. */
#pragma push
#pragma optimization_level 0
void em007_model_eft_create(_EFT* self) {
    nw4r::math::VEC3 scratch_a;
    nw4r::math::VEC3 scratch_b;
    u32 light;
    _AINPC_W* npc;
    struct _LB_NPC* lb_npc;
    u8 made;
    struct Em007EftWork* work;
    u32 i;

    work = (struct Em007EftWork*)self->work_0x38;
    VEC3_ctor(&scratch_a);
    VEC3_ctor(&scratch_b);
    for (i = 0; i < work->model_count_0x00; i++) {
        if (work->mode_0x28 == 0) {
            light = 6;
        } else {
            light = 0;
        }
        made = (u32)res_eft_model_create_light(work->models_0x08[i], em007_model_res_ids[work->kind_0x2A], 0x2C, light);
        if (made == 0) {
            em007_model_eft_retire(self);
            return;
        }
    }
    work->copy_0x24 = new Em007ModelCopy;
    if (work->copy_0x24 == NULL) {
        em007_model_eft_retire(self);
        return;
    }
    work->copy_0x24->eft_0x04 = self;
    em007_model_copy_bind(work->copy_0x24);
    for (i = 0; i < 18; i++) {
        work->copy_0x24->nodes_0x0C[i] = 0xFF;
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    for (i = 0; i < work->model_count_0x00; i++) {
        setVector3(&work->models_0x08[i]->scale_0x1C, em007_model_eft_scale, em007_model_eft_scale,
                   em007_model_eft_scale);
        work->models_0x08[i]->field_0x28 = 0;
        work->models_0x08[i]->field_0x2C = 0;
        work->models_0x08[i]->field_0x28 = 0;
        work->models_0x08[i]->ready = 0;
        work->node_count_0x29 = ((nw4r::g3d::ResMdl*)&work->models_0x08[i]->field_0x114)->GetResNodeNumEntries();
        switch (work->mode_0x28) {
        case 0:
            npc = (_AINPC_W*)self->source_0x30;
            work->copy_0x24->model_0x08 = &npc->model;
            if (npc->field_0x420 == 1) {
                work->light_0x58 = light_id_next();
            } else {
                work->light_0x58 = 0xFFFF;
            }
            em007_model_copy_bind_nodes(work, i);
            switch (npc->field_0x420) {
            case 0:
                work->effect_0x0C = res_eft_create(0x314, 0x1E, 0);
                break;
            case 4:
                work->area_0x5D = npc->area;
                break;
            default:
                work->effect_0x0C = NULL;
                break;
            }
            break;
        case 1:
            lb_npc = (struct _LB_NPC*)self->source_0x30;
            work->effect_0x0C = NULL;
            if (lb_npc == NULL) {
                self->area_0x44 = get_now_areano();
            } else {
                self->area_0x44 = lb_npc->field_0x004;
                work->copy_0x24->model_0x08 = (struct MHchar*)lb_npc->body_0x058;
                em007_model_copy_bind_nodes(work, i);
            }
            break;
        }
        if (work->kind_0x2A == 4) {
            work->models_0x08[i]->setVisibility(6, false);
            work->models_0x08[i]->setVisibility(7, false);
            work->models_0x08[i]->setVisibility(8, false);
            work->models_0x08[i]->setVisibility(9, false);
        }
    }
    if (work->mode_0x28 == 0) {
        switch (npc->field_0x420) {
        case 0:
        case 5:
            work->angle_0x50 = 0;
            break;
        }
    }
    em007_model_eft_update(self);
}
#pragma pop

/* 0x801E0938 (0x3C): Builds the model copy over the world-callback base. */
Em007ModelCopy::Em007ModelCopy() {
}

/* 0x801E0974 (0x110): Binds each node of the model copy to the matrix of the node of the same name in pooled model
 * `index`. */
void em007_model_copy_bind_nodes(struct Em007EftWork* work, u8 index) {
    nw4r::g3d::ResNode node((void*)0);
    u32 i;

    for (i = 0; i < work->node_count_0x29; i++) {
        nw4r::g3d::ResNode src = ((nw4r::g3d::ResMdl*)&work->models_0x08[index]->field_0x114)->GetResNode(i);
        nw4r::g3d::ResNode found = ((nw4r::g3d::ResMdl*)&work->copy_0x24->model_0x08->field_0x114)
                                       ->GetResNode(em007_res_node_name((ResHandle*)&src));
        res_node_assign(&node, &found);
        nw4r::g3d::ResNode src2 = ((nw4r::g3d::ResMdl*)&work->models_0x08[index]->field_0x114)->GetResNode(i);
        ((nw4r::g3d::ResMdl*)&work->copy_0x24->model_0x08->field_0x114)
            ->GetResNode(em007_res_node_name((ResHandle*)&src2));
        if (res_node_is_valid((ResHandle*)&node)) {
            work->copy_0x24->nodes_0x0C[i] = res_node_mtx_id_get(&node);
        }
    }
}

/* 0x801E0A84 (0x3C): Returns the name of resource node `node`. */
const char* em007_res_node_name(const struct ResHandle* node) {
    return (const char*)em007_res_name_at((void**)node, res_node_ref(node)->mToResName);
}

/* 0x801E0AC0 (0x1C): `base + offset`, or NULL when the offset is 0. */
void* em007_res_name_at(void** base, u32 offset) {
    u8* p = (u8*)*base;
    if (offset != 0) {
        return p + offset;
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
