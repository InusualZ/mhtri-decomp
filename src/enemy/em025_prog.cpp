/*
 * enemy/em025_prog.cpp - enemy 025's program: the `em025_prog_tbl` entry points (reset, init, frame update, action
 *   dispatch, event hook, motion events, model effect, state query, area start, leave, part gates), the per-action
 *   state steps the `state_sub` dispatchers select, the two sound programs, the cut-scene steps, the head tracking
 *   and sway passes, and the user-data helper's callbacks.
 * RANGE. .text 0x8019E670-0x801AA154 (93 functions); extab 0x8000F124-0x8000F374, extabindex 0x8002A8A0-0x8002AC18,
 *   .rodata 0x80570150-0x805701A0 (`em025_turn_seq`, then the head tracking's limit initializer), .data
 *   0x805AE750-0x805B06D0 (`em025_prog_tbl` first, the user-data class table last), .sdata 0x80791A80-0x80791A88,
 *   .sdata2 0x80798518-0x80798910.  Left edge: `em018_static_init` (`enemy/em018_prog.cpp`) ends there and the 0.0
 *   pool entry repeats from here.  Right edge: `Em025UserData::~Em025UserData`, a deleting destructor like `em018_prog.cpp`'s,
 *   closes the TU (the 0.0 entry repeats again at `lbl_8079893C`, first read by `fn_801AA154`).
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide (retail keeps `clrlwi` + `cmpwi` pairs), `#pragma pool_data
 *   off`.
 * NAMES. The file name follows the runtime dump's `em025_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em025ProgTbl` slot each fills, after `Em012ProgTbl`): em025_reset, em025_init, em025_frame_update,
 *   em025_action_dispatch, em025_noop, em025_event_notify, em025_motion_events, em025_model_effect_update,
 *   em025_state_ck, em025_slots_release_hook, em025_area_start_set, em025_leave, em025_part_level_ck,
 *   em025_part_drop, em025_area_request.  GUESS (dispatcher slot): em025_act<action>_<first sub-state>,
 *   em025_act<action>_dispatch, em025_act10_se_play, em025_act11_se_play.  GUESS (from the bodies and callers): the
 *   other em025_* functions, `Em025Block` and its fields, `Em025HitRec` and its fields, the em025_*_tbl capsule
 *   tables, the em025_*_curve key curves, em025_sway_keys, em025_swim_speeds, em025_turn_seq, the em025_se_*
 *   records, em025_se_height, em025_se_next_act, `Em025UserData`.
 *   Foreign GUESS names proposed in the registry (lane enemy-em025) and declared in leaf headers:
 *   em_busy_net_notify, world_mtx_attr_scale_set, camera_quake_pos_req, camera_mode_req, eft_em_model_pair_attach, bgm_event_flag_set, bgm_track15_req, anim_key_curve_eval.
 * RESIDUALS. `em025_head_track` - the block pointer's induction register is set up before the joint/limit table
 *   pointers, and the turn angle lands in r4 with the `i == 0` compare scheduled after the subtraction.
 *   flipcheck: the 8 foreign names above are registry proposals, not map names yet.
 * SHAPES. `Em025UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   A `subi`/`cmplwi`/`ble` range test on a `u8` is the explicit `(u32)((u8)x - lo) <= n` (a `switch` or an `||`
 *   chain gives the `cmpwi` tree); a `(s32)` cast gives retail's signed `cmpwi` on a `u8`/`u16` value; a one-case
 *   test compared signed is a `switch` with the other path as `default:`; the shell-table slots are called without
 *   their trailing table argument; `em_ground_ck_c1`/`curve_key_eval_c1` keep the float argument where retail
 *   evaluates it; `(u8)(cond) & 0xFF` keeps the `clrlwi` on a returned bool; `Em025Block* blk = &self->em025_0x328`
 *   gives retail's `addi rN,r3,0x328` base.  The data is defined in retail order between the functions (the jump
 *   tables fall between the definitions; the sound programs sit before `em025_act10_se_play`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck */
#include "enemy/fn_8012EC74.h" /* em_get_mot_no, em_after_frame_check, get_joint_wmat_em */
#include "enemy/em025_prog.h"
#include "enemy/em_busy_net_notify.h"
#include "g3d/world_mtx_attr_scale_set.h"
#include "camera/camera_quake_pos_req.h"
#include "camera/camera_mode_req.h"
#include "menu/eft_em_model_pair_attach.h"
#include "sound/bgm_event_flag_set.h"
#include "sound/bgm_track15_req.h"
#include "anim_key_curve_eval.h"
#include "enemy/fn_8019ED34.h"
#include "lobby/lb_quest_screen.h" /* quest_rand_next (the owner's header, rule 2) */
#include "fn_8004CAD8.h" /* MTX34_ctor, mulVecMatAddTrans */
#include "unsplit/enemy.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor (rule 2) */
#include "enemy/enemy_control.h"
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "enemy/fn_801B0010.h"
#include "enemy/fn_8011D448.h"
#include "ef/fn_800CDB2C.h"
#include "g3d/g3d_anmchr.h"
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "sys_mem.h"
#include "enemy/fn_80138074.h"
#include "MSL/new.h" /* the placement `operator new` */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "Pl/plw.h"
#include "enemy/em_se_record.h"
#include "stage/shell_set_func_ptr.h"
#include "ef/get_move_work_adrs.h"
#include "draw_shape.h"
#include "enemy/em_common.h"
#include "Pl/fn_80262940.h"
#include "g3d/g3d_draw.h"
#include "g3d/g3d_calcworld.h"
#include "vec3_scale.h"
#include "enemy/em_wave_amp.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "camera/camera_demo_event_set.h"
#include "enemy/joint_mtx_store.h"
#include "calcMtxAng3.h"

/* The user-data accessor `em025_init` installs: the base accessor with its own destructor, attach hook and matrix
 * item hook (its table is `.data` 0x805B06A0).  size: 0xC (`operator new(0xC)`) */
class Em025UserData : public ResUserDataAc {
public:
    Em025UserData();
    virtual ~Em025UserData();
    virtual void Attach();
    virtual void CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};

extern "C" {

/* `enemy/fn_8011D448.cpp`'s status-bit helpers (`bits_0x824`), declared with these call sites' spelling. */
void em_status_bits_init(struct _ENEMY_WORK* self);
void em_status_bits_set(struct _ENEMY_WORK* self, u32 mask);
void em_status_bits_clr(struct _ENEMY_WORK* self, u32 mask);

/* `stage/stg_w.cpp`'s blend helper: r3 is the `_ENEMY_WORK` (set by the caller), f1 the scalar. */
void stage_anim_rate_target_set(f32 a);

void em025_act0_dispatch(struct _ENEMY_WORK* self);
void em025_act2_0(struct _ENEMY_WORK* self);
void em025_act2_1(struct _ENEMY_WORK* self);
void em025_act2_2(struct _ENEMY_WORK* self);
void em025_act2_3(struct _ENEMY_WORK* self);
void em025_act2_4(struct _ENEMY_WORK* self);
void em025_act3_0(struct _ENEMY_WORK* self, u32 a);
void em025_act6_1(struct _ENEMY_WORK* self, u32 a);
void em025_act1_1(struct _ENEMY_WORK* self, u32 a);
void em025_act1_3(struct _ENEMY_WORK* self);
void em025_act1_4(struct _ENEMY_WORK* self);
void em025_act1_5(struct _ENEMY_WORK* self);
void em025_act1_dispatch(struct _ENEMY_WORK* self);
void em025_act2_dispatch(struct _ENEMY_WORK* self);
void em025_act3_dispatch(struct _ENEMY_WORK* self);
void em025_act6_6(struct _ENEMY_WORK* self, u32 a);
void em025_act6_7(struct _ENEMY_WORK* self);
void em025_act6_8(struct _ENEMY_WORK* self, u32 a);
void em025_act6_9(struct _ENEMY_WORK* self, u32 a);
void em025_act6_10(struct _ENEMY_WORK* self, u32 a);
void em025_act6_11(struct _ENEMY_WORK* self);
void em025_act6_12(struct _ENEMY_WORK* self);
void em025_act6_13(struct _ENEMY_WORK* self);
void em025_act6_14(struct _ENEMY_WORK* self);
void em025_act6_15(struct _ENEMY_WORK* self, u32 a);
void em025_act6_16(struct _ENEMY_WORK* self);
void em025_act6_18(struct _ENEMY_WORK* self);
void em025_act6_19(struct _ENEMY_WORK* self);
void em025_act6_20(struct _ENEMY_WORK* self, u32 a);
void em025_act6_dispatch(struct _ENEMY_WORK* self);
void em025_act7_0(struct _ENEMY_WORK* self);
void em025_act7_1(struct _ENEMY_WORK* self);
void em025_act7_2(struct _ENEMY_WORK* self, u32 a);
void em025_act7_3(struct _ENEMY_WORK* self);
void em025_act7_4(struct _ENEMY_WORK* self);
void em025_act7_5(struct _ENEMY_WORK* self);
void em025_act7_6(struct _ENEMY_WORK* self);
void em025_act7_7(struct _ENEMY_WORK* self);
void em025_act7_8(struct _ENEMY_WORK* self, u32 a);
void em025_act7_9(struct _ENEMY_WORK* self, u32 a);
void em025_act7_10(struct _ENEMY_WORK* self, u32 a);
void em025_act7_dispatch(struct _ENEMY_WORK* self);
void em025_act10_se_play(struct _ENEMY_WORK* self);
void em025_act11_se_play(struct _ENEMY_WORK* self);
void em025_act13_2(struct _ENEMY_WORK* self);
void em025_act13_3(struct _ENEMY_WORK* self);
void em025_act13_4(struct _ENEMY_WORK* self);
void em025_act13_dispatch(struct _ENEMY_WORK* self);
void em025_flags_update(struct _ENEMY_WORK* self);
void em025_posture_update(struct _ENEMY_WORK* self);
void em025_status_bits_update(struct _ENEMY_WORK* self);
void em025_joint_point_get(struct _ENEMY_WORK* self, u32 joint, Vec3* out, Vec3* tmp);
void em025_effect_spawn(struct _ENEMY_WORK* self, u32 family, u32 type, u32 joint, s32 arg, f32 scale);
}

/* `lobby/fn_8030121C.cpp`'s `eft_em_spawn`, in the `void*` spelling these call sites use (its callers
 * disagree on the parameter types). */
extern "C" void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);

/* One record of the effect table at `get_enemy_data(self)+0x2C`: a joint id and the joint-local offset.
 * size: 0x10 */
struct EmDataEntry {
    /* +0x00 */ s32 joint;
    /* +0x04 */ nw4r::math::VEC3 vec;
};

/* This unit's view of the `get_enemy_data` record's +0x2C effect-table pointer (`EnemyData` pads it).
 * size: 0x30 */
struct _ENEMY_DATA_VIEW {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ EmDataEntry* table_0x2C;
};

#pragma peephole off

/* The +0x14 and +0x2C shell-table slots as this unit calls them: without the trailing table argument (the call site
 * loads the table last, only as the base register of the slot load). */
typedef void (*ShellSetPosJobFn)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, u32 b, u16 c);
typedef void (*ShellSetScaleJobFn)(struct _ENEMY_WORK* self, u32 kind, u32 id, nw4r::math::VEC3* pos, f32 scale,
                                   u16 flags);

/* The em025 block seen through to the action bit set after it: the bit helpers address the set from the block's
 * base.  size: 0x30 */
struct Em025MaskView {
    /* +0x00 (+0x328) */ u8 pad_0x00[0x2C];
    /* +0x2C (+0x354) */ u8 mask_0x354[4];   /* the action bit set: bit `id % 8` of byte `id / 8` (clamped to 3) */
};

/* `em_vec3_copy` as this unit calls it: two arguments, the copy returned through the first. */
#define em_vec3_copy_c1 ((void* (*)(void*, void*))em_vec3_copy)
/* `em_ground_ck` with the f1 height its callers here pass. */
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)
/* `anim_key_curve_eval` in its argument order: the key table, the curve position, the key cursor and the result. */
#define curve_key_eval_c1 ((f32 (*)(f32*, f32, f32*, f32*))anim_key_curve_eval)
/* `em_act_ck` with the unsigned result the `== 1` tests compare (`cmplwi`). */
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)

#pragma pool_data off

/* One capsule of the attack/body tables the work record latches at +0x8C8/+0x8CC: two joints, the shape and the
 * part it belongs to, a mask word, the radius and the two joint-local ends.  The field names are a GUESS from the
 * values.  size: 0x2C */
struct Em025HitRec {
    /* +0x00 */ u16 joint_a_0x00;    /* 0xFFFF ends the table */
    /* +0x02 */ u16 joint_b_0x02;
    /* +0x04 */ u16 shape_0x04;      /* 0 sphere, 1 one-joint capsule, 2 two-joint capsule */
    /* +0x06 */ u16 attr_0x06;
    /* +0x08 */ u16 part_0x08;
    /* +0x0A */ u16 flags_0x0A;
    /* +0x0C */ u32 mask_0x0C;
    /* +0x10 */ f32 radius_0x10;
    /* +0x14 */ nw4r::math::VEC3 end_a_0x14;
    /* +0x20 */ nw4r::math::VEC3 end_b_0x20;
};

extern "C" {
void em025_reset(struct _ENEMY_WORK* self);
void em025_init(struct _ENEMY_WORK* self, u8 mode);
void em025_frame_update(struct _ENEMY_WORK* self);
void em025_action_dispatch(struct _ENEMY_WORK* self);
void em025_noop(void);
void em025_event_notify(struct _ENEMY_WORK* self, u32 kind, u32 sub);
void em025_motion_events(struct _ENEMY_WORK* self);
void em025_model_effect_update(struct _ENEMY_WORK* self);
int em025_state_ck(struct _ENEMY_WORK* self, u32 kind);
void em025_slots_release_hook(struct _ENEMY_WORK* self);
void em025_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub);
void em025_leave(struct _ENEMY_WORK* self);
s32 em025_part_level_ck(struct _ENEMY_WORK* self, u8 kind);
void em025_part_drop(struct _ENEMY_WORK* self, u8 kind);
void em025_area_request(struct _ENEMY_WORK* self);
}

/* The em025 program table (`em025_prog_tbl`): the entry points the shared enemy program driver calls (the slot names
 * follow `Em012ProgTbl`; the empty slots are zero).
 * size: 0x70 */
struct Em025ProgTbl {
    /* +0x00 */ void (*reset_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u32 kind, u32 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ int (*state_ck_0x24)(struct _ENEMY_WORK* self, u32 kind);
    /* +0x28 */ u32 pad_0x28;
    /* +0x2C */ void (*slot_release_0x2C)(struct _ENEMY_WORK* self);
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*leave_0x34)(struct _ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[3];
    /* +0x44 */ s32 (*part_level_ck_0x44)(struct _ENEMY_WORK* self, u8 part);
    /* +0x48 */ u32 pad_0x48[2];
    /* +0x50 */ void (*part_drop_0x50)(struct _ENEMY_WORK* self, u8 part);
    /* +0x54 */ u32 pad_0x54;
    /* +0x58 */ void (*area_request_0x58)(struct _ENEMY_WORK* self);
    /* +0x5C */ u32 pad_0x5C[5];
};

Em025ProgTbl em025_prog_tbl = {
    em025_reset, em025_init, em025_frame_update, em025_action_dispatch, em025_noop, em025_event_notify, em025_motion_events, 0,
    em025_model_effect_update, em025_state_ck, 0, em025_slots_release_hook, em025_area_start_set, em025_leave, {0, 0, 0}, em025_part_level_ck, {0, 0},
    em025_part_drop, 0, em025_area_request, {0, 0, 0, 0, 0},
};

/* The `em_turn_seq_start`/`em_turn_seq_step` table `em025_act6_15` runs (`.rodata`, 0x40 B): a four-word head, then
 * three four-word rows whose first word carries the motion in its high half. */
extern "C" const u32 em025_turn_seq[16] = {
    0x00000000, 0x000000B4, 0x000000B4, 0x80000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
    0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
};

/* The attack capsule table `em025_frame_update` latches at +0x8C8 outside action 11 (GUESS name); a joint of -1 ends it. */
Em025HitRec em025_hit_tbl[62] = {
    {0x14, 0, 1, 1, 3, 0x0, 0x00000000, 147.0f, {0.0f, 255.0f, 525.0f}, {0.0f, 320.0f, 85.0f}},
    {0x14, 18, 2, 1, 3, 0x0, 0x00000000, 220.0f, {0.0f, 380.0f, -98.0f}, {0.0f, 380.0f, -118.0f}},
    {0x5, 0, 0, 4, 6, 0x0, 0x80000000, 280.0f, {0.0f, 380.0f, 132.0f}, {0.0f, 0.0f, 0.0f}},
    {0x5, 0, 0, 4, 6, 0x0, 0x00000000, 190.0f, {0.0f, 765.0f, -8.0f}, {0.0f, 0.0f, 0.0f}},
    {0x5, 3, 2, 1, 3, 0x0, 0x00000000, 260.0f, {0.0f, 380.0f, -237.0f}, {0.0f, 417.0f, 50.0f}},
    {0x24, 3, 2, 1, 3, 0x0, 0x00000000, 235.0f, {0.0f, 270.0f, -257.0f}, {0.0f, 347.0f, 50.0f}},
    {0x24, 38, 2, 1, 3, 0x0, 0x00000000, 200.0f, {0.0f, 270.0f, -257.0f}, {0.0f, 332.0f, 50.0f}},
    {0x27, 38, 2, 1, 3, 0x0, 0x00000000, 225.0f, {0.0f, 270.0f, -27.0f}, {0.0f, 332.0f, 50.0f}},
    {0x26, 0, 0, 4, 3, 0x0, 0x00000000, 210.0f, {0.0f, 571.5f, -495.0f}, {0.0f, 0.0f, 0.0f}},
    {0x27, 0, 0, 4, 7, 0x0, 0x40000000, 255.5f, {0.0f, 310.0f, -300.0f}, {0.0f, 0.0f, 0.0f}},
    {0x28, 41, 2, 1, 3, 0x0, 0x00000000, 271.0f, {0.0f, 340.0f, -200.0f}, {0.0f, 360.0f, -125.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 160.0f, {0.0f, 450.0f, -225.0f}, {667.0f, 130.0f, -140.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 160.0f, {0.0f, 450.0f, -225.0f}, {-667.0f, 130.0f, -140.0f}},
    {0x14, 0, 1, 0, 1, 0x80, 0x00000000, 180.0f, {380.0f, 25.0f, 1915.0f}, {430.0f, -165.0f, 820.0f}},
    {0x14, 0, 1, 0, 1, 0x80, 0x20000000, 120.0f, {390.0f, 20.0f, 1980.0f}, {255.0f, 385.0f, 3100.0f}},
    {0x14, 0, 1, 0, 1, 0x100, 0x00000000, 155.0f, {440.0f, -130.0f, 885.0f}, {390.0f, 35.0f, 1940.0f}},
    {0x14, 0, 1, 0, 1, 0x100, 0x20000000, 70.0f, {255.0f, 410.0f, 3165.0f}, {400.0f, 10.0f, 2020.0f}},
    {0x14, 0, 1, 0, 0, 0x80, 0x00000000, 180.0f, {-360.0f, 155.0f, 2500.0f}, {-430.0f, -165.0f, 820.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x10000000, 105.0f, {-345.0f, 205.0f, 2650.0f}, {-250.0f, 390.0f, 3120.0f}},
    {0x14, 0, 1, 0, 0, 0x100, 0x20000000, 70.0f, {-255.0f, 410.0f, 3165.0f}, {-400.0f, 10.0f, 2020.0f}},
    {0x14, 0, 1, 0, 0, 0x100, 0x00000000, 85.0f, {-440.0f, -25.0f, 1815.0f}, {-340.0f, 190.0f, 2640.0f}},
    {0x14, 0, 1, 0, 0, 0x100, 0x00000000, 155.0f, {-440.0f, -130.0f, 885.0f}, {-390.0f, 35.0f, 1940.0f}},
    {0x14, 0, 0, 5, 3, 0x0, 0x04000000, 350.0f, {0.0f, -240.0f, 485.0f}, {0.0f, 0.0f, 0.0f}},
    {0x14, 0, 1, 1, 3, 0x80, 0x00000000, 170.0f, {0.0f, 70.0f, 0.0f}, {0.0f, 115.0f, 1495.0f}},
    {0x12, 20, 2, 1, 3, 0x0, 0x02000000, 320.0f, {520.0f, -20.0f, -220.0f}, {285.0f, -70.0f, 540.0f}},
    {0x12, 20, 2, 1, 3, 0x0, 0x02000000, 320.0f, {-520.0f, -20.0f, -220.0f}, {-285.0f, -70.0f, 540.0f}},
    {0x12, 20, 2, 1, 3, 0x100, 0x04000000, 320.0f, {520.0f, -20.0f, -220.0f}, {285.0f, -70.0f, 540.0f}},
    {0x12, 20, 2, 1, 3, 0x100, 0x04000000, 320.0f, {-520.0f, -20.0f, -220.0f}, {-285.0f, -70.0f, 540.0f}},
    {0x14, 0, 1, 3, 2, 0x0, 0x00000000, 500.0f, {290.0f, -420.0f, 60.0f}, {-290.0f, -420.0f, 60.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x00000000, 215.0f, {0.0f, -155.0f, 190.0f}, {0.0f, -170.0f, -280.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x00000000, 200.0f, {-25.0f, -155.0f, 190.0f}, {-115.0f, -145.0f, -280.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x00000000, 200.0f, {25.0f, -155.0f, 190.0f}, {115.0f, -145.0f, -280.0f}},
    {0x1A, 0, 0, 2, 2, 0x0, 0x00000000, 220.0f, {0.0f, -215.0f, 315.0f}, {0.0f, 0.0f, 0.0f}},
    {0x1E, 27, 2, 3, 2, 0x0, 0x08000000, 850.0f, {0.0f, -400.0f, 150.0f}, {0.0f, -350.0f, 0.0f}},
    {0x1E, 5, 2, 3, 2, 0x0, 0x08000000, 850.0f, {0.0f, -400.0f, 0.0f}, {0.0f, -900.0f, 0.0f}},
    {0x19, 0, 1, 3, 2, 0x0, 0x00000000, 335.0f, {0.0f, -625.0f, 295.0f}, {0.0f, -540.0f, -250.0f}},
    {0x19, 0, 1, 3, 2, 0x0, 0x00000000, 280.0f, {-140.0f, -660.0f, 220.0f}, {-280.0f, -520.0f, -250.0f}},
    {0x19, 0, 1, 3, 2, 0x0, 0x00000000, 280.0f, {140.0f, -660.0f, 220.0f}, {280.0f, -520.0f, -250.0f}},
    {0x5, 37, 2, 1, 3, 0x0, 0x00000000, 360.0f, {550.0f, -15.0f, -130.0f}, {300.0f, -45.0f, -130.0f}},
    {0x12, 4, 2, 3, 2, 0xA0, 0x00000000, 800.0f, {260.0f, -581.0f, 315.0f}, {350.0f, -680.0f, 585.0f}},
    {0x12, 4, 2, 3, 2, 0xA0, 0x00000000, 800.0f, {-260.0f, -581.0f, 315.0f}, {-350.0f, -680.0f, 585.0f}},
    {0x12, 4, 2, 1, 3, 0x0, 0x00000000, 780.0f, {260.0f, -581.0f, 220.0f}, {350.0f, -580.0f, 0.0f}},
    {0x5, 37, 2, 1, 3, 0x0, 0x00000000, 360.0f, {-550.0f, -15.0f, -130.0f}, {-300.0f, -45.0f, -130.0f}},
    {0x12, 4, 2, 1, 3, 0x0, 0x00000000, 780.0f, {-260.0f, -581.0f, 220.0f}, {-350.0f, -580.0f, 0.0f}},
    {0x3, 37, 2, 1, 3, 0x0, 0x00000000, 727.0f, {370.0f, -570.0f, 150.0f}, {10.0f, -454.0f, 70.0f}},
    {0x3, 37, 2, 1, 3, 0x0, 0x00000000, 727.0f, {-370.0f, -570.0f, 150.0f}, {-10.0f, -454.0f, 70.0f}},
    {0x26, 40, 2, 1, 3, 0x0, 0x00000000, 420.0f, {140.0f, -490.0f, 0.0f}, {10.0f, -220.0f, 0.0f}},
    {0x26, 40, 2, 1, 3, 0x0, 0x00000000, 420.0f, {-140.0f, -490.0f, 0.0f}, {-10.0f, -220.0f, 0.0f}},
    {0x28, 42, 2, 1, 3, 0x0, 0x00000000, 450.0f, {0.0f, -190.0f, 0.0f}, {0.0f, 15.0f, 0.0f}},
    {0x2A, 45, 2, 1, 3, 0x0, 0x00000000, 415.0f, {0.0f, -50.0f, 0.0f}, {0.0f, 10.0f, 200.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 248.0f, {0.0f, 210.0f, 0.0f}, {0.0f, 240.0f, -930.0f}},
    {0x7, 8, 2, 2, 5, 0x0, 0x00000000, 320.0f, {0.0f, -40.0f, -20.0f}, {-60.0f, -60.0f, 0.0f}},
    {0x8, 9, 2, 2, 5, 0x0, 0x00000000, 250.0f, {0.0f, -90.0f, -80.0f}, {20.0f, -25.0f, 0.0f}},
    {0x9, 0, 1, 2, 5, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {460.0f, -65.0f, -15.0f}},
    {0xD, 14, 2, 2, 4, 0x0, 0x00000000, 320.0f, {0.0f, -40.0f, -20.0f}, {60.0f, -60.0f, 0.0f}},
    {0xE, 15, 2, 2, 4, 0x0, 0x00000000, 250.0f, {0.0f, -90.0f, -80.0f}, {-20.0f, -25.0f, 0.0f}},
    {0xF, 0, 1, 2, 4, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {-460.0f, -65.0f, -15.0f}},
    {0x25, 0, 1, 1, 3, 0x0, 0x00000000, 255.0f, {418.0f, -429.0f, -324.0f}, {772.0f, -449.0f, -429.0f}},
    {0x25, 0, 1, 1, 3, 0x0, 0x00000000, 255.0f, {-418.0f, -429.0f, -324.0f}, {-772.0f, -449.0f, -429.0f}},
    {0x14, 0, 1, 1, 3, 0x100, 0x04000000, 205.0f, {300.0f, -100.0f, 650.0f}, {350.0f, 50.0f, 1570.0f}},
    {0x14, 0, 1, 1, 3, 0x100, 0x04000000, 205.0f, {-300.0f, -100.0f, 650.0f}, {-350.0f, 50.0f, 1570.0f}},
    {0xFFFF, 0, 0, 0, 0, 0x0, 0x00000000, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
};

/* The body capsule table it latches at +0x8CC beside it (GUESS name). */
Em025HitRec em025_body_tbl[43] = {
    {0x14, 0, 1, 1, 3, 0x0, 0x00000000, 207.0f, {0.0f, 255.0f, 525.0f}, {0.0f, 320.0f, 85.0f}},
    {0x14, 18, 2, 1, 3, 0x0, 0x00000000, 285.0f, {0.0f, 380.0f, -98.0f}, {0.0f, 380.0f, -118.0f}},
    {0x5, 0, 0, 4, 6, 0x0, 0x80000000, 350.0f, {0.0f, 380.0f, 132.0f}, {0.0f, 0.0f, 0.0f}},
    {0x5, 0, 0, 4, 6, 0x0, 0x00000000, 200.0f, {0.0f, 765.0f, -8.0f}, {0.0f, 0.0f, 0.0f}},
    {0x5, 3, 2, 1, 3, 0x0, 0x00000000, 345.0f, {0.0f, 380.0f, -237.0f}, {0.0f, 417.0f, 50.0f}},
    {0x24, 3, 2, 1, 3, 0x0, 0x00000000, 320.0f, {0.0f, 270.0f, -257.0f}, {0.0f, 347.0f, 50.0f}},
    {0x24, 38, 2, 1, 3, 0x0, 0x00000000, 310.0f, {0.0f, 270.0f, -257.0f}, {0.0f, 332.0f, 50.0f}},
    {0x27, 38, 2, 1, 3, 0x0, 0x00000000, 310.0f, {0.0f, 270.0f, -27.0f}, {0.0f, 332.0f, 50.0f}},
    {0x26, 0, 0, 4, 3, 0x0, 0x00000000, 270.0f, {0.0f, 571.5f, -495.0f}, {0.0f, 0.0f, 0.0f}},
    {0x27, 0, 0, 4, 7, 0x0, 0x40000000, 315.5f, {0.0f, 310.0f, -300.0f}, {0.0f, 0.0f, 0.0f}},
    {0x28, 41, 2, 1, 3, 0x0, 0x00000000, 371.0f, {0.0f, 340.0f, -200.0f}, {0.0f, 360.0f, -125.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 200.0f, {0.0f, 275.0f, -300.0f}, {667.0f, 130.0f, -15.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 200.0f, {0.0f, 275.0f, -225.0f}, {-667.0f, 130.0f, -140.0f}},
    {0x14, 0, 1, 0, 1, 0x0, 0x00000000, 225.0f, {380.0f, 25.0f, 1915.0f}, {430.0f, -165.0f, 820.0f}},
    {0x14, 0, 1, 0, 1, 0x0, 0x20000000, 165.0f, {390.0f, 20.0f, 1980.0f}, {255.0f, 385.0f, 3100.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 206.5f, {-360.0f, 155.0f, 2500.0f}, {-430.0f, -165.0f, 820.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x10000000, 100.0f, {-320.0f, 225.0f, 2650.0f}, {-250.0f, 435.0f, 3175.0f}},
    {0x14, 0, 1, 1, 3, 0x80, 0x00000000, 170.0f, {0.0f, 70.0f, 0.0f}, {0.0f, 115.0f, 1495.0f}},
    {0x12, 20, 2, 1, 3, 0x0, 0x00000000, 420.0f, {520.0f, -20.0f, -220.0f}, {285.0f, -70.0f, 540.0f}},
    {0x12, 20, 2, 1, 3, 0x0, 0x00000000, 420.0f, {-520.0f, -20.0f, -220.0f}, {-285.0f, -70.0f, 540.0f}},
    {0x14, 18, 2, 3, 2, 0x0, 0x00000000, 980.0f, {0.0f, -280.0f, -105.0f}, {0.0f, -620.0f, 60.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x02000000, 250.0f, {0.0f, -155.0f, 190.0f}, {0.0f, -170.0f, -280.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x02000000, 250.0f, {-25.0f, -155.0f, 190.0f}, {-115.0f, -145.0f, -280.0f}},
    {0x1A, 0, 1, 3, 2, 0x0, 0x02000000, 250.0f, {25.0f, -155.0f, 190.0f}, {115.0f, -145.0f, -280.0f}},
    {0x1A, 0, 0, 2, 2, 0x0, 0x02000000, 290.0f, {0.0f, -215.0f, 315.0f}, {0.0f, 0.0f, 0.0f}},
    {0x1E, 27, 2, 3, 2, 0x0, 0x08000000, 930.0f, {0.0f, -400.0f, 150.0f}, {0.0f, -350.0f, 0.0f}},
    {0x1E, 5, 2, 1, 3, 0x0, 0x08000000, 950.0f, {0.0f, -400.0f, 0.0f}, {0.0f, -900.0f, 0.0f}},
    {0x14, 0, 1, 3, 2, 0x0, 0x00000000, 760.0f, {0.0f, -100.0f, 635.0f}, {0.0f, -445.0f, 0.0f}},
    {0x12, 3, 2, 0, 0, 0x0, 0x00000000, 1200.0f, {0.0f, -500.0f, 0.0f}, {0.0f, -400.0f, 0.0f}},
    {0x3, 37, 2, 0, 0, 0x0, 0x00000000, 1100.0f, {0.0f, -500.0f, 0.0f}, {0.0f, -200.0f, 0.0f}},
    {0x25, 39, 2, 0, 0, 0x0, 0x00000000, 750.0f, {0.0f, -500.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {0x28, 42, 2, 1, 3, 0x0, 0x00000000, 470.0f, {0.0f, -190.0f, 0.0f}, {0.0f, 15.0f, 0.0f}},
    {0x2A, 45, 2, 1, 3, 0x0, 0x00000000, 430.0f, {0.0f, -50.0f, 0.0f}, {0.0f, 10.0f, 200.0f}},
    {0x2D, 0, 1, 1, 3, 0x0, 0x00000000, 270.0f, {0.0f, 210.0f, 0.0f}, {0.0f, 240.0f, -925.0f}},
    {0x7, 8, 2, 2, 5, 0x0, 0x00000000, 400.0f, {0.0f, -40.0f, -20.0f}, {-60.0f, -60.0f, 0.0f}},
    {0x8, 9, 2, 2, 5, 0x0, 0x00000000, 320.0f, {0.0f, -90.0f, -80.0f}, {20.0f, -25.0f, 0.0f}},
    {0x9, 0, 1, 2, 5, 0x0, 0x00000000, 260.0f, {0.0f, 0.0f, 0.0f}, {460.0f, -65.0f, -15.0f}},
    {0xD, 14, 2, 2, 4, 0x0, 0x00000000, 400.0f, {0.0f, -40.0f, -20.0f}, {60.0f, -60.0f, 0.0f}},
    {0xE, 15, 2, 2, 4, 0x0, 0x00000000, 320.0f, {0.0f, -90.0f, -80.0f}, {-20.0f, -25.0f, 0.0f}},
    {0xF, 0, 1, 2, 4, 0x0, 0x00000000, 260.0f, {0.0f, 0.0f, 0.0f}, {-460.0f, -65.0f, -15.0f}},
    {0x25, 0, 1, 1, 3, 0x0, 0x00000000, 320.0f, {650.0f, -440.0f, -330.0f}, {850.0f, -530.0f, -570.0f}},
    {0x25, 0, 1, 1, 3, 0x0, 0x00000000, 320.0f, {-650.0f, -440.0f, -330.0f}, {-850.0f, -530.0f, -570.0f}},
    {0xFFFF, 0, 0, 0, 0, 0x0, 0x00000000, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
};

/* The attack table of action 11 (GUESS name). */
Em025HitRec em025_hit_tbl_act11[13] = {
    {0x3, 18, 2, 1, 3, 0x0, 0x00000000, 1000.0f, {0.0f, -460.0f, 0.0f}, {0.0f, -685.0f, 0.0f}},
    {0x14, 20, 2, 1, 3, 0x0, 0x00000000, 790.0f, {0.0f, -500.0f, 215.0f}, {0.0f, -500.0f, 0.0f}},
    {0x24, 36, 2, 1, 3, 0x0, 0x00000000, 905.0f, {0.0f, -600.0f, 0.0f}, {0.0f, -655.0f, -515.0f}},
    {0x29, 37, 2, 1, 3, 0x0, 0x00000000, 600.0f, {0.0f, -270.0f, 0.0f}, {0.0f, -450.0f, -515.0f}},
    {0x29, 42, 2, 1, 3, 0x0, 0x00000000, 615.0f, {0.0f, -265.0f, 0.0f}, {100.0f, -65.0f, -140.0f}},
    {0x2B, 44, 2, 1, 3, 0x0, 0x00000000, 550.0f, {50.0f, -115.0f, 0.0f}, {5.0f, -65.0f, -140.0f}},
    {0x7, 8, 2, 2, 5, 0x0, 0x00000000, 320.0f, {0.0f, -40.0f, -20.0f}, {-60.0f, -60.0f, 0.0f}},
    {0x8, 9, 2, 2, 5, 0x0, 0x00000000, 250.0f, {0.0f, -90.0f, -80.0f}, {20.0f, -25.0f, 0.0f}},
    {0x9, 0, 1, 2, 5, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {460.0f, -65.0f, -15.0f}},
    {0xD, 14, 2, 2, 4, 0x0, 0x00000000, 350.0f, {0.0f, -110.0f, -20.0f}, {175.0f, -60.0f, -40.0f}},
    {0xE, 15, 2, 2, 4, 0x0, 0x00000000, 250.0f, {0.0f, -90.0f, -80.0f}, {-20.0f, -25.0f, 0.0f}},
    {0xF, 0, 1, 2, 4, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {-460.0f, -65.0f, -15.0f}},
    {0xFFFF, 0, 0, 0, 0, 0x0, 0x00000000, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
};

/* The body table of action 11 (GUESS name). */
Em025HitRec em025_body_tbl_act11[21] = {
    {0x3, 18, 2, 1, 3, 0x0, 0x00000000, 1150.0f, {0.0f, -460.0f, 0.0f}, {0.0f, -685.0f, 0.0f}},
    {0x14, 20, 2, 1, 3, 0x0, 0x00000000, 850.0f, {0.0f, -500.0f, 465.0f}, {0.0f, -500.0f, 0.0f}},
    {0x24, 36, 2, 1, 3, 0x0, 0x00000000, 1100.0f, {0.0f, -600.0f, 0.0f}, {0.0f, -655.0f, -515.0f}},
    {0x29, 37, 2, 1, 3, 0x0, 0x00000000, 750.0f, {0.0f, -270.0f, 0.0f}, {0.0f, -450.0f, -515.0f}},
    {0x29, 42, 2, 1, 3, 0x0, 0x00000000, 765.0f, {0.0f, -265.0f, 0.0f}, {100.0f, -265.0f, -140.0f}},
    {0x2B, 44, 2, 1, 3, 0x0, 0x00000000, 700.0f, {50.0f, -265.0f, 0.0f}, {5.0f, -265.0f, -140.0f}},
    {0x2C, 45, 2, 1, 3, 0x0, 0x00000000, 550.0f, {50.0f, 250.0f, 0.0f}, {-200.0f, 450.0f, -690.0f}},
    {0x7, 8, 2, 2, 5, 0x0, 0x00000000, 420.0f, {0.0f, -40.0f, -20.0f}, {-60.0f, -60.0f, 0.0f}},
    {0x8, 9, 2, 2, 5, 0x0, 0x00000000, 350.0f, {0.0f, -90.0f, -80.0f}, {20.0f, -25.0f, 0.0f}},
    {0x9, 0, 1, 2, 5, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {460.0f, -65.0f, -15.0f}},
    {0xD, 14, 2, 2, 4, 0x0, 0x00000000, 450.0f, {0.0f, -110.0f, -20.0f}, {175.0f, -60.0f, -40.0f}},
    {0xE, 15, 2, 2, 4, 0x0, 0x00000000, 350.0f, {0.0f, -90.0f, -80.0f}, {-20.0f, -25.0f, 0.0f}},
    {0xF, 0, 1, 2, 4, 0x0, 0x00000000, 160.0f, {0.0f, 0.0f, 0.0f}, {-460.0f, -65.0f, -15.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 345.0f, {0.0f, 105.0f, 0.0f}, {0.0f, 250.0f, 1450.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 340.0f, {-395.0f, 0.0f, 750.0f}, {-395.0f, 0.0f, 1400.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 340.0f, {395.0f, 100.0f, 750.0f}, {395.0f, 100.0f, 1400.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 260.0f, {400.0f, -135.0f, 750.0f}, {395.0f, -45.0f, 1400.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 340.0f, {-395.0f, 0.0f, 1400.0f}, {-395.0f, 290.0f, 2425.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 340.0f, {395.0f, 100.0f, 1400.0f}, {395.0f, 155.0f, 1835.0f}},
    {0x14, 0, 1, 0, 0, 0x0, 0x00000000, 340.0f, {395.0f, 155.0f, 1835.0f}, {325.0f, 305.0f, 2400.0f}},
    {0xFFFF, 0, 0, 0, 0, 0x0, 0x00000000, 0.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
};

/* tail 4 bytes */

/* The leap's (frame, height step) key curve; a frame of -1 ends it. */
f32 em025_leap_height_curve[28] = {
    0.0f, 0.0f, 100.0f, -0.5f, 340.0f, 2.0f, 420.0f, -10.0f, 540.0f, -30.0f, 590.0f, 0.0f, 640.0f, 80.0f,
    720.0f, 30.0f, 815.0f, 0.0f, 900.0f, -26.0f, 960.0f, -48.0f, 1040.0f, -20.0f, 1100.0f, -20.0f, -1.0f, 0.0f,
};

/* The leap's side drift curve (mirrored for the second leap). */
f32 em025_leap_side_curve[28] = {
    0.0f, 0.0f, 100.0f, 0.0f, 340.0f, 0.0f, 420.0f, 0.0f, 540.0f, 0.0f, 590.0f, -8.7f, 640.0f, 7.0f, 720.0f, 28.0f,
    815.0f, 24.0f, 900.0f, 27.0f, 960.0f, 26.0f, 1040.0f, 8.0f, 1100.0f, 0.0f, -1.0f, 0.0f,
};

/* The leap's forward drift curve. */
f32 em025_leap_fwd_curve[28] = {
    0.0f, 0.0f, 100.0f, -5.0f, 340.0f, -13.0f, 420.0f, -18.0f, 540.0f, 0.0f, 590.0f, 73.0f, 640.0f, 32.0f,
    720.0f, 13.0f, 815.0f, 6.0f, 900.0f, 6.5f, 960.0f, 5.0f, 1040.0f, 1.0f, 1100.0f, 0.0f, -1.0f, 0.0f,
};

/* The dive's height step curve. */
f32 em025_dive_curve[20] = {
    0.0f, 0.0f, 300.0f, 1.0f, 500.0f, 0.0f, 645.0f, -5.0f, 720.0f, -6.0f, 780.0f, -16.0f, 880.0f, -4.0f,
    1045.0f, 0.0f, 1200.0f, 4.0f, -1.0f, 0.0f,
};

/* The surfacing's height step curve. */
f32 em025_surface_curve[16] = {
    0.0f, 10.0f, 100.0f, 10.0f, 200.0f, 7.0f, 300.0f, 4.0f, 340.0f, 2.2f, 370.0f, 0.0f, 650.0f, 0.0f, -1.0f, 0.0f,
};

/* The sway curve keys the joint X rotation is read from. */
f32 em025_sway_keys[12] = {
    0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 3.0f, 1.0f, 0.01f, 5.0f, -1.0f, 0.01f, 5.0f,
};

/* The three swim speeds `em025_act6_1` picks from. */
f32 em025_swim_speeds[3] = {
    2.0f, 1.0f, 4.0f,
};

extern "C" {
/* ------------------------------------------------------------------------------------------------
 * This unit's own functions.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8019E670 - the band's range test on the record's byte at +0x961: mode 0 `==`, 1 `>=`, 2 `<=`,
 * 3 the inclusive window. */
u32 em025_byte_stack_range_ck(struct _ENEMY_WORK* self, u32 mode, u32 lo, u32 hi) {
    switch ((u8)mode) {
    case 0:
        if (self->stack_0x961[0] == (u8)lo) {
            return 1;
        }
        break;
    case 1:
        if (self->stack_0x961[0] >= (u8)lo) {
            return 1;
        }
        break;
    case 2:
        if (self->stack_0x961[0] <= (u8)lo) {
            return 1;
        }
        break;
    case 3:
        if (self->stack_0x961[0] >= (u8)lo && self->stack_0x961[0] <= (u8)hi) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019E70C - the "which sub-phase of the shore action" query: 255 when the record is not the one
 * this table describes, else the phase the byte at +0x961 falls into. */
u32 em025_shore_phase_get(struct _ENEMY_WORK* self) {
    if (self == 0) {
        return 255;
    }
    if (self->team != 25) {
        return 255;
    }
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1 && quest_flag_200000_ck(0) != 1) {
        if (em025_byte_stack_range_ck(self, 3, 0, 3) == 1) {
            return 0;
        }
        if (em025_byte_stack_range_ck(self, 3, 4, 19) == 1) {
            return 1;
        }
        if (em025_byte_stack_range_ck(self, 3, 20, 59) == 1) {
            return 3;
        }
        if (em025_byte_stack_range_ck(self, 3, 60, 70) == 1) {
            return 4;
        }
        if (em025_byte_stack_range_ck(self, 3, 71, 91) == 1) {
            return 2;
        }
    }
    return 255;
}

/* 0x8019E840 - the matching yes/no form of the same table. */
u32 em025_shore_window_ck(struct _ENEMY_WORK* self) {
    if (self == 0) {
        return 0;
    }
    if (self->team != 25) {
        return 0;
    }
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1 && quest_flag_200000_ck(0) != 1) {
        if (em025_byte_stack_range_ck(self, 3, 30, 59) == 1) {
            return 1;
        }
        if (em025_byte_stack_range_ck(self, 3, 90, 92) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8019E908 - arms/steps the +0x1E4 countdown: mode 0 steps it while bit 0 is set, mode 1 sets bit 0. */
void em025_step_timer_update(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 0) {
        if ((self->field_0x1E4 & 1) == 0) {
            return;
        }
        self->field_0x1E4--;
    } else if (mode == 1) {
        self->field_0x1E4 |= 1;
    }
}

/* 0x8019E948 - clears the four-byte action bit set at +0x354 (the target's four `stb`s are the
 * compiler's inlined `memset`). */
void em025_act_bits_clear(struct _ENEMY_WORK* self) {
    self->mask_0x354[0] = 0;
    self->mask_0x354[1] = 0;
    self->mask_0x354[2] = 0;
    self->mask_0x354[3] = 0;
}

/* 0x8019E960 - sets bit `id` of that set: slot `id / 8` clamped to the last of the four bytes. */
void em025_act_bit_set(struct _ENEMY_WORK* self, s32 id) {
    Em025MaskView* view = (Em025MaskView*)&self->em025_0x328;
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    view->mask_0x354[slot] |= 1 << (id % 8);
}

/* 0x8019E9AC - the matching test. */
s32 em025_act_bit_ck(struct _ENEMY_WORK* self, s32 id) {
    Em025MaskView* view = (Em025MaskView*)&self->em025_0x328;
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    if (view->mask_0x354[slot] & (1 << (id % 8))) {
        return 1;
    }
    return 0;
}

/* 0x8019EA04 - releases every live enemy-control handle and re-arms the slot states. */
void em025_slots_release(struct _ENEMY_WORK* self) {
    Em025Block* blk = &self->em025_0x328;
    u16 i;

    for (i = 0; i < 4; i++) {
        if (blk->handles_0x328[i] != -1) {
            em_roster_record_release(blk->handles_0x328[i]);
            blk->handles_0x328[i] = -1;
        }
        blk->states_0x338[i] = 255;
    }
    self->field_0x761 = 0;
}

/* 0x8019EA80 - picks one of the free slot indices at random: the low 8 bits of +0x761 mark the used
 * slots, the RSO random source is folded down to the free count. */
u8 em025_free_area_pick(struct _ENEMY_WORK* self, u16 idx) {
    u8 avail[8];
    u8 n = 0;
    u8 i;
    s32 value;

    for (i = 0; i < 8; i++) {
        if ((self->field_0x761 & (1 << i)) == 0) {
            avail[n] = i;
            n++;
        }
    }
    if (n <= 4) {
        return 255;
    }
    value = (s32)((quest_rand_next() & 0xFFFF) + 0x157E7) >> (idx & 0xFFFF);
    value = (value * 13) & 0xFFFF;
    return avail[value % (s32)n];
}

/* 0x8019EBAC - steps the rotation word at +0x1C0 by 4 (clamped to the 0x4000..0x4004 band) and keeps
 * the height at +0x190 inside its two pool bounds, snapping to the middle value otherwise. */
void em025_shore_drift(struct _ENEMY_WORK* self) {
    u32 angle = self->field_0x1C0 & 0xFFFF;
    self->field_0x1C0 = angle;
    if (angle >= 16388) {
        self->field_0x1C0 = angle - 4;
    } else if (angle <= 16380) {
        self->field_0x1C0 = angle + 4;
    } else {
        self->field_0x1C0 = 16384;
    }
    if (self->pos.z >= 49.1f) {
        self->pos.z -= 0.1f;
        return;
    }
    if (self->pos.z <= 48.9f) {
        self->pos.z += 0.1f;
        return;
    }
    self->pos.z = 49.0f;
}

/* 0x8019EC38 (0x9C): shakes the camera at joint `joint`'s position (the work position for -1), at the work's
 * ground height. */
void em025_joint_quake(struct _ENEMY_WORK* self, s32 joint, s32 kind, s32 level) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((u32)joint == 0xFFFFFFFF) {
        copyVec3(&pos, &self->pos);
    } else {
        get_joint_wpos_em(self, joint, &pos);
    }
    pos.y = self->field_0x20C;
    camera_quake_pos_req(self, kind, &pos, level);
}

/* 0x8019ECD4 - arms the four enemy-control slots at +0x328 and seeds the record's remaining state,
 * then clears the bit set. */
void em025_reset(struct _ENEMY_WORK* self) {
    self->handles_0x328[0] = -1;
    self->states_0x338[0] = 255;
    self->handles_0x328[1] = -1;
    self->states_0x338[1] = 255;
    self->handles_0x328[2] = -1;
    self->states_0x338[2] = 255;
    self->handles_0x328[3] = -1;
    self->states_0x338[3] = 255;
    self->field_0x33C = 0;
    self->field_0x340 = 0;
    self->field_0x344 = 0;
    self->field_0x348 = 1.0f;
    self->field_0x352 = 1;
    self->field_0x34C = 0.5f;
    self->field_0x350 = 0;
    self->field_0x353 = 0;
    self->field_0x358 = 0;
    em025_act_bits_clear(self);
}

/* 0x8019ED34 (0x1D0): the program's init: fills the four roster slots from the free area-table entries, places
 * the shore action on map 6 / area 1 and attaches the user-data helper. */
void em025_init(struct _ENEMY_WORK* self, u8 mode) {
    Em025Block* blk = &self->em025_0x328;
    Em025UserData* data;
    u8 state;
    s32* handle;
    u8 i;

    if (quest_flag_200000_ck(0) == 0 && (s32)stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        self->field_0x7B0 = 0.2f;
    }
    self->field_0x761 = 0;
    for (i = 0; i < 4; i++) {
        state = em025_free_area_pick(self, i);
        if ((u8)state != 0xFF) {
            handle = &blk->handles_0x328[i];
            em_roster_slot_effect_set(self, 2, handle, 0, 0);
            if (*handle != -1) {
                EmPopRec* rec = em_roster_record_get(*handle);
                if (rec != NULL) {
                    rec->state_0x1F8 = 3;
                    rec->state_0x1FC = 3;
                }
                blk->states_0x338[i] = state;
                self->field_0x761 |= 1 << (u8)state;
            }
        }
    }
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        self->pos.x = 5400.0f;
        self->pos.y = 0.0f;
        self->pos.z = 2700.0f;
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x8000;
        self->field_0x1C4 = 0;
        em_height_set(self, em_height_base_get(self));
        em_move_mode_set(self, 6);
        em_act_arm_unless_down(self, 6, 16);
        em_state_refresh(self);
    }
    eft_em_model_pair_attach(self);
    if (em_res_user_data_ck(self) == 0) {
        data = (Em025UserData*)operator new(sizeof(Em025UserData));
        new (data) Em025UserData;
        em_res_user_data_set(self, data);
    }
}

} /* extern "C" */

/* 0x8019EF04 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em025UserData::Em025UserData() {
}

extern "C" {

/* 0x8019EF40 - an empty virtual body (the class's no-op slot). */
void em025_noop(void) {
}

/* 0x8019EF44 (0x138): the program's event hook: arms the +0x1E4 step for the kind 1/2/7 events, counts event
 * 7/7, runs the shell table's reset for events 10/0xBD..0xBE, releases the slots on event 11. */
void em025_event_notify(struct _ENEMY_WORK* self, u32 kind, u32 sub) {
    u8 step = 0;

    switch ((u8)kind) {
    case 1:
        switch ((u8)sub) {
        case 0:
        case 4:
            step = 1;
        }
        break;
    case 2:
        switch ((u8)sub) {
        case 1:
        case 3:
            step = 1;
        }
        break;
    case 7:
        switch ((u8)sub) {
        case 2:
            step = 1;
            break;
        case 7:
            self->em025_0x328.flag_0x353++;
            break;
        }
        break;
    case 10:
        if ((u32)((u8)sub - 0xBD) > 1) {
            switch ((u8)sub) {
            case 0xF1:
                bgm_event_flag_set();
                break;
            }
        } else {
            shell_set_func_ptr->method_0x70(self, shell_set_func_ptr);
        }
        break;
    case 11:
        em025_slots_release(self);
        break;
    case 13:
        if ((u32)((u8)sub - 2) <= 1) {
            em_combat_reset(self);
        }
        break;
    }
    em025_step_timer_update(self, step);
    em025_act_bits_clear(self);
}

/* 0x8019F07C - selects the record's two dispatch words (0x8C8/0x8CC) from the area/motion, then
 * asks `em025_shore_window_ck` which of the two pooled alpha values `fn_802B08DC` should blend. */
void em025_frame_update(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        self->field_0x8C8 = fn_801260BC(self);
        self->field_0x8CC = fn_801260E0(self);
    } else if (self->action == 11) {
        self->field_0x8C8 = (u32)em025_hit_tbl_act11;
        self->field_0x8CC = (u32)em025_body_tbl_act11;
    } else {
        self->field_0x8C8 = (u32)em025_hit_tbl;
        self->field_0x8CC = (u32)em025_body_tbl;
    }
    if (em025_shore_window_ck(self) == 1) {
        stage_anim_rate_target_set(0.5f);
    } else {
        stage_anim_rate_target_set(1.0f);
    }
}

/* 0x8019F12C - the band's motion arming tail shared by several states: arm mode 4, set the
 * `em_act_arm_unless_act11` window (6, 9) and refresh through `em_finish_flag_set`. */
void em025_finish_to_swim(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    em_act_arm_unless_act11(self, 6, 9);
    em_finish_flag_set(self);
}

/* 0x8019F174 - the same tail arming mode 0 with the `em_act_arm_unless_act11` window (1, 0). */
void em025_finish_to_stand(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11(self, 1, 0);
    em_finish_flag_set(self);
}

/* 0x8019F1BC - the two-state motion step: state 0 arms `em_mot_set_ck(self, 13, 30, 0)` plus the
 * `0.0f` timer, state 1 waits for `em_mot_end_ck` and then runs `em_action_finish`. */
void em025_act0_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F244 - the same shape arming `em_mot_set_ck(self, 1, 30, 0)` and completing through
 * `em_action_finish_stand`. */
void em025_act0_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x8019F2C0 - the per-sub-state dispatcher of the motion above: sub-states 0/1/2 arm through
 * `em025_act0_0`, sub-state 7 runs `em025_act0_7`. */
void em025_act0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act0_0(self);
        break;
    case 1:
        em025_act0_0(self);
        break;
    case 2:
        em025_act0_0(self);
        break;
    case 7:
        em025_act0_7(self);
        break;
    }
}

/* 0x8019F2FC - state 0 arms `em_mot_set_ck(self, 21, 30, 0)`, state 1 completes through
 * `em025_finish_to_stand`. */
void em025_act1_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 21, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em025_finish_to_stand(self);
        }
        break;
    }
}

/* 0x8019F378 (0x1F4): the roar: motion 24 with the shout window at 350, the hit windows at 550 and the breath
 * effects every tenth frame of the 388..528 window. */
void em025_act1_1(struct _ENEMY_WORK* self, u32 shore) {
    nw4r::math::VEC3 pos;

    if ((u8)shore == 1) {
        em_flag_0x116_set(self);
    }
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 24, 30, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 350.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 26, 5);
            draw_shape_arm((u32)self, 25, 120);
        }
        if (em_frame_check(self, 0, 550.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 29);
            em_hit_window_set_default(self, 1, 32);
        }
        if ((u8)shore == 1) {
            em025_shore_drift(self);
        }
        setVector3(&pos, 0.0f, 0.0f, 1000.0f);
        if (em_frame_check(self, 0, 352.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 25, &pos, 1.3f);
        }
        if (em_frame_check(self, 3, 388.0f, 528.0f) == 1) {
            if (self->timer_0x020 % 10 == 0) {
                eft_em_spawn(self, 1, 25, &pos, 3.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F56C - state 0 arms mode 13 (window 30) and the 150-frame `timer_0x020`; state 1 runs the
 * timer down and completes through `em_action_finish`. */
void em025_act1_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        self->timer_0x020 = 150;
        break;
    case 1: {
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* 0x8019F5F4 - state 0 arms `em_mot_set(self, 26, 20, 0)` plus the two `em_hit_window_set_default` windows
 * (0,20)/(1,21), state 1 completes through `em025_finish_to_stand`. */
void em025_act1_4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        em_hit_window_set_default(self, 0, 20);
        em_hit_window_set_default(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em025_finish_to_stand(self);
        }
        break;
    }
}

/* 0x8019F690 - state 0 arms `em_mot_set(self, 27, 20, 0)`, state 1 completes through
 * `em_action_finish`. */
void em025_act1_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 27, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F70C - the per-sub-state dispatcher of the motion above. */
void em025_act1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act1_0(self);
        break;
    case 1:
        em025_act1_1(self, 1);
        break;
    case 2:
        em025_act1_1(self, 0);
        break;
    case 3:
        em025_act1_3(self);
        break;
    case 4:
        em025_act1_4(self);
        break;
    case 5:
        em025_act1_5(self);
        break;
    }
}

/* 0x8019F768 - the first `em025_act2_dispatch` motion step: arm `em_mot_set(self, 20, 100, 0)`, then a ten-window
 * hit-window pass ending in the `em_approach_step` completion. */
void em025_act2_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 600.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, 780.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 1210.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, 668.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, 804.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 1316.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_approach_step(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F9BC - the second motion step: the same ten-window pass against the other pooled floats
 * (`590.0f`..) arming mode 22, completing through `em025_finish_to_stand`. */
void em025_act2_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 590.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, 780.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, 172.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 1200.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, 680.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, 806.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, 172.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 1310.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_approach_step(self, 0, 1) == 1) {
            em025_finish_to_stand(self);
        }
        break;
    }
}

/* 0x8019FC10 - the same ten-window pass as `em025_act2_0`, completing through `em_mot_end_ck` first
 * and `em_approach_step` second. */
void em025_act2_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 600.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, 780.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 1210.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, 668.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, 804.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 112.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, 170.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 1316.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (em_approach_step(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019FE80 - the second motion step with the `em025_act2_2` completion pair. */
void em025_act2_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 590.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, 780.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, 172.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 1200.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, 680.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, 806.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, 172.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, 1310.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            em025_finish_to_stand(self);
        } else if (em_approach_step(self, 0, 1) == 1) {
            em025_finish_to_stand(self);
        }
        break;
    }
}

/* 0x801A00F0 - the fourth `em025_act2_dispatch` motion step: motion 26 with two hit windows, then motion 27 blended,
 * then `em_action_finish`. */
void em025_act2_4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        em_hit_window_set_default(self, 0, 20);
        em_hit_window_set_default(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 27, 20, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A01C8 - the five-way motion dispatcher of the `em025_action_dispatch` action 2. */
void em025_act2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act2_0(self);
        break;
    case 1:
        em025_act2_1(self);
        break;
    case 2:
        em025_act2_2(self);
        break;
    case 3:
        em025_act2_3(self);
        break;
    case 4:
        em025_act2_4(self);
        break;
    }
}

/* 0x801A0210 (0x3A4): the leap from the water: motion 12 or its mirror 31, the height and the side drift from the
 * three key curves, the landing check and the ground return. */
void em025_act3_0(struct _ENEMY_WORK* self, u32 mirror) {
    f32 height;

    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        if ((u8)mirror == 0) {
            em_mot_set_blend(self, 12, 100, 0, 1);
        } else {
            em_mot_set_blend(self, 31, 100, 0, 1);
        }
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        break;
    case 1:
        height = em_key_curve_eval(self, em025_leap_height_curve);
        if ((u8)mirror == 0) {
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, em025_leap_side_curve);
        } else {
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, em025_leap_side_curve);
        }
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, em025_leap_fwd_curve);
        em_height_add(self, height);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->field_0x1AC < 0.0f) {
            self->state++;
        }
        break;
    case 2:
        height = em_key_curve_eval(self, em025_leap_height_curve);
        if ((u8)mirror == 0) {
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, em025_leap_side_curve);
        } else {
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, em025_leap_side_curve);
        }
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, em025_leap_fwd_curve);
        em_height_add(self, height);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->field_0x1AC >= 0.0f) {
            self->state++;
            em_joint_drop_start(self, 2);
            em_height_set(self, 0.0f);
            self->offset_0x30C.vec_0x310.y = 0.0f;
        }
        break;
    case 3:
        if ((u8)mirror == 0) {
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, em025_leap_side_curve);
        } else {
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, em025_leap_side_curve);
        }
        self->offset_0x30C.vec_0x310.y = em_key_curve_eval(self, em025_leap_height_curve);
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, em025_leap_fwd_curve);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_ground_ck_c1(self, 0.0f) == 1) {
            self->state++;
            em_height_set(self, self->pos.y - self->field_0x20C);
            em_move_mode_set(self, 4);
        }
        break;
    case 4:
        height = em_key_curve_eval(self, em025_leap_height_curve);
        if ((u8)mirror == 0) {
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, em025_leap_side_curve);
        } else {
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, em025_leap_side_curve);
        }
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, em025_leap_fwd_curve);
        em_height_add(self, height);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A05B4 - action 3's two-way sub-state selector into `em025_act3_0` (0/1). */
void em025_act3_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act3_0(self, 0);
        break;
    case 1:
        em025_act3_0(self, 1);
        break;
    }
}

/* 0x801A05E0 - action 6's first motion: arm `em_mot_set_blend_ck(self, 1, 30, 0, 1)`, then wait on
 * `em_approach_step(self, 0, 0)` and complete through `em_action_finish_stand`. */
void em025_act6_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend_ck(self, 1, 30, 0, 1);
        em_height_set(self, 630.0f);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A0684 (0x164): swims toward the target position at the speed `speed` selects, until the distance runs
 * out. */
void em025_act6_1(struct _ENEMY_WORK* self, u32 speed) {
    f32 step;

    if ((u8)speed <= 2) {
        step = em025_swim_speeds[(u8)speed];
    } else {
        step = 2.0f;
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend_ck(self, 1, 30, 0, 0);
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        self->value_0x378 = calcVecDistXZ(&self->pos, &self->vec_0x36C);
        break;
    case 1: {
        s32 angle = calcVecAng2(&self->vec_0x36C, &self->pos);
        setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, -step);
        rotVecY(&self->offset_0x30C.vec_0x310, angle);
        addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        self->value_0x378 -= vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= 0.0f) {
            self->state++;
        }
        break;
    }
    case 2:
        if (em_mot_frame_started_ck(self) == 0) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A07E8 (0xE0): the dive: motion 2 (at double speed for `fast`), the height from its key curve. */
void em025_act6_6(struct _ENEMY_WORK* self, u32 fast) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 2, 30, 0, 1);
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        if ((u8)fast == 1) {
            em_mot_speed_set(self, 2.0f);
        }
        break;
    case 1:
        em_height_add(self, em_key_curve_eval(self, em025_dive_curve));
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A08C8 (0xC0): the surfacing: motion 17 from the deep height, rising along its key curve to the surface. */
void em025_act6_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 17, 0, 0);
        em_height_set(self, -1930.0f);
        em_move_vec2_clr(self);
        break;
    case 1:
        em_height_add(self, em_key_curve_eval(self, em025_surface_curve));
        if (self->field_0x1AC >= 630.0f) {
            self->field_0x1AC = 630.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A0988 (0x148): the fast swim toward the target position (90 or 30 per frame), finishing standing. */
void em025_act6_8(struct _ENEMY_WORK* self, u32 fast) {
    f32 step;

    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    if ((s32)(u8)fast == 1) {
        step = 90.0f;
    } else {
        step = 30.0f;
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_move_vec2_clr(self);
        self->value_0x378 = calcVecDistXZ(&self->pos, &self->vec_0x36C);
        break;
    case 1: {
        s32 angle = calcVecAng2(&self->vec_0x36C, &self->pos);
        setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, -step);
        rotVecY(&self->offset_0x30C.vec_0x310, angle);
        addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        self->value_0x378 -= vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= 0.0f) {
            em_action_finish_stand(self);
        }
        break;
    }
    }
}

/* 0x801A0AD0 (0x178): the submerged wait: motion 1 for 1, 30, 150 or 60 frames by `kind`; the two long waits
 * put the local player into the 0x29 model state once. */
void em025_act6_9(struct _ENEMY_WORK* self, u32 kind) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        self->timer_0x020 = 0;
        if ((u32)((u8)kind - 2) > 1) {
            switch ((u8)kind) {
            case 0:
                self->timer_0x020 = 1;
                break;
            case 1:
                self->timer_0x020 = 30;
                break;
            }
        } else {
            if ((u8)kind == 2) {
                self->timer_0x020 = 150;
            } else {
                self->timer_0x020 = 60;
            }
            if (self->field_0x358 == 0) {
                _PLW* players = (_PLW*)get_move_work_adrs(2);
                pl_model_state_set(&players[(s8)my_player_no()], 1, 0x29, 0);
                bgm_track15_req();
                self->field_0x358 = 1;
            }
        }
        break;
    case 1:
        if ((u32)((u8)kind - 1) <= 2) {
            self->timer_0x020--;
        }
        if (em_mot_end_ck(self) == 1 || self->timer_0x020 <= 0) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A0C48 (0x3A4): the side swim: accelerates along the target direction up to `speed`, blends the swim layer
 * by the drift, turns toward the drift and brakes once the distance runs out. */
void em025_act6_10(struct _ENEMY_WORK* self, u32 slow) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 accel;
    nw4r::math::VEC3 scaled;
    nw4r::math::VEC3 clamp;
    nw4r::math::VEC3 brake;
    nw4r::math::VEC3 back;
    nw4r::math::VEC3 neg;
    f32 speed;
    f32 rate;
    u32 done;

    setVec3(&dir, 0.0f, 0.0f, 1.0f);
    done = 0;
    switch ((u8)slow) {
    case 1:
        speed = 5.0f;
        break;
    default:
        speed = 10.0f;
        break;
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        self->frames_0x454[4] = 0.0f;
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        self->value_0x378 = calcVecDistXZ(&self->pos, &self->vec_0x36C);
        break;
    case 1:
        rotVecY(&dir, calcVecAng2(&self->pos, &self->vec_0x36C));
        vec3_scale(&scaled, &dir, speed);
        vec3_scale(&accel, &scaled, 0.005f);
        addVec3To(&self->offset_0x30C.vec_0x310, &accel);
        if (vec3_len(&self->offset_0x30C.vec_0x310.x) > speed) {
            vec3_scale(&clamp, &dir, speed);
            copyVec3(&self->offset_0x30C.vec_0x310, &clamp);
        }
        rate = -(self->offset_0x30C.vec_0x310.x / 10.0f);
        em_motion_blend_step(self, rate, 1.0f);
        em_mot_layer_blend_step(self, 15, 16, (s32)em_mot_frame_get(self), self->frames_0x454[4]);
        rate = -(self->offset_0x30C.vec_0x310.x / 10.0f);
        if (rate > 2.0f) {
            rate = 2.0f;
        } else if (rate < -2.0f) {
            rate = -2.0f;
        }
        self->field_0x1C0 = em_angle_approach(
            (u16)(s32)(0.5f + (65536.0f * (30.0f * self->frames_0x454[4])) / 360.0f) + 0x8000,
            self->field_0x1C0, 0x40);
        addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        self->value_0x378 -= vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= 0.0f) {
            self->state++;
            vec3_negate(&neg, &dir);
            vec3_scale(&back, &neg, speed);
            vec3_scale(&brake, &back, 0.005f);
            copyVec3(&self->vec_0x31C, &brake);
        }
        break;
    case 2:
        CancelFade(self);
        if (vec3_dot(&self->offset_0x30C.vec_0x310.x, &self->vec_0x31C.x) > 0.0f) {
            em_move_vec2_clr(self);
            done = 1;
        }
        rate = -(self->offset_0x30C.vec_0x310.x / 10.0f);
        em_motion_blend_step(self, rate, 1.0f);
        em_mot_layer_blend_step(self, 15, 16, (s32)em_mot_frame_get(self), self->frames_0x454[4]);
        rate = -(self->offset_0x30C.vec_0x310.x / 10.0f);
        if (rate > 2.0f) {
            rate = 2.0f;
        } else if (rate < -2.0f) {
            rate = -2.0f;
        }
        self->field_0x1C0 = em_angle_approach(
            (u16)(s32)(0.5f + (65536.0f * (30.0f * self->frames_0x454[4])) / 360.0f) + 0x8000,
            self->field_0x1C0, 0x40);
        if (done == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A0FEC - action 6's sub-state 11: the shared `em_busy_set`/`em_busy_timer_reset` gate then a
 * two-state timer whose state 1 completes through `em025_finish_to_swim`. */
void em025_act6_11(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em025_turn_seq, 0, 16, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em025_turn_seq) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A10AC (0x104): the tail slap: motion 3, shaking the local player's camera every eighth frame while the
 * player is active and in the alternate act path. */
void em025_act6_12(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 3, 60, 0);
        em_height_set(self, 630.0f);
        break;
    case 1:
        if ((system_w.field_0x0c & 7) == 0) {
            _PLW* players = (_PLW*)get_move_work_adrs(2);
            _PLW* pl = &players[(s8)my_player_no()];
            if (pl->slot_active != 0 && pl->field_0x585 != 0) {
                em_camera_req(self, 2, 200);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A11B0 (0x118): the head butt: motion 11 with hit window 3, the camera request at frame 78. */
void em025_act6_13(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 11, 40, 0, 1);
        em_height_set(self, 630.0f);
        em_hit_window_set_default(self, 0, 3);
        break;
    case 1:
        if (em_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            _PLW* players = (_PLW*)get_move_work_adrs(2);
            _PLW* pl = &players[(s8)my_player_no()];
            if (pl->slot_active != 0 && pl->field_0x585 != 0) {
                em_camera_req(self, 2, 133);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A12C8 - action 6's sub-state 14: arm mode 8 and the scaled `field_0x318`, then step the move offset
 * and complete through `em_action_finish_stand`. */
void em025_act6_14(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 60, 0);
        em_mot_speed_set(self, 0.4f);
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        self->field_0x318 = -2.5f * get_em_base_scale(self);
        break;
    case 1:
        em_move_offset_step(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A1384 - action 6's sub-state 15/23: state 0 arms mode 1 and seeds `timer_0x020` from the
 * (mode) selector (240/150), state 1 runs the timer down and completes through `em_action_finish_stand`. */
void em025_act6_15(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        em_height_set(self, 630.0f);
        switch ((u8)mode) {
        case 0:
            self->timer_0x020 = 240;
            break;
        case 1:
            self->timer_0x020 = 150;
            break;
        default:
            self->timer_0x020 = 150;
            break;
        }
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A1454 - action 6's sub-state 16: state 0 arms mode 1 and the 150-frame timer, state 1 runs
 * it down and completes through `em025_finish_to_swim`. */
void em025_act6_16(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 150;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A14FC - action 6's sub-state 18: the busy gate, mode 9 and the `field_0x318` scale, two frame
 * windows, then `em025_finish_to_swim`. */
void em025_act6_18(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 9, 30, 0, 0);
        em_height_set(self, 630.0f);
        em_move_vec2_clr(self);
        self->field_0x318 = -6.0f * get_em_base_scale(self);
        break;
    case 1:
        if (em_frame_check(self, 1, 572.0f, 0.0f) == 1) {
            em_height_add(self, -7.0f * get_em_base_scale(self));
        }
        if (em_frame_check(self, 1, 2.0f, 0.0f) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A1624 - action 6's sub-state 19: state 0 arms mode 1, state 1 waits for the 64-bit
 * `em_turn_to_target` predicate and advances, state 2 completes through `em_action_finish_stand`. */
void em025_act6_19(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        em_height_set(self, 630.0f);
        break;
    case 1:
        if (em_turn_to_target(self, 64) == 1) {
            self->state++;
        }
        break;
    case 2:
        if (em_mot_frame_started_ck(self) == 0) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A16D4 - action 6's sub-state 20/21: drive the `field_0x1AC` height against the mode's blend window,
 * then `em025_finish_to_swim` (mode 0) or `em_action_finish_stand`. */
void em025_act6_20(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend_ck(self, 1, 30, 0, 0);
        em_move_vec2_clr(self);
        break;
    case 1:
        if ((u8)mode == 0) {
            em_height_add(self, -10.0f);
            if (self->field_0x1AC <= -1500.0f) {
                em_height_set(self, em_height_base_get(self));
                em025_finish_to_swim(self);
            }
        } else {
            em_height_add(self, 8.0f);
            if (self->field_0x1AC >= 630.0f) {
                em_height_set(self, 630.0f);
                em_action_finish_stand(self);
            }
        }
        break;
    }
}

/* 0x801A17B8 - action 6's `state_sub` dispatcher (27 entries): the first slot runs this band's
 * `em025_act6_0`, the rest dispatch into the motion-mode table by (function, mode). */
void em025_act6_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act6_0(self);
        break;
    case 1:
        em025_act6_1(self, 0);
        break;
    case 2:
        em025_act6_1(self, 1);
        break;
    case 3:
        em025_act6_1(self, 2);
        break;
    case 4:
        em025_act6_1(self, 0);
        break;
    case 5:
        em025_act6_1(self, 0);
        break;
    case 6:
        em025_act6_6(self, 0);
        break;
    case 7:
        em025_act6_7(self);
        break;
    case 8:
        em025_act6_8(self, 0);
        break;
    case 9:
        em025_act6_9(self, 0);
        break;
    case 10:
        em025_act6_10(self, 0);
        break;
    case 11:
        em025_act6_11(self);
        break;
    case 12:
        em025_act6_12(self);
        break;
    case 13:
        em025_act6_13(self);
        break;
    case 14:
        em025_act6_14(self);
        break;
    case 15:
        em025_act6_15(self, 0);
        break;
    case 16:
        em025_act6_16(self);
        break;
    case 17:
        em025_act6_8(self, 1);
        break;
    case 18:
        em025_act6_18(self);
        break;
    case 19:
        em025_act6_19(self);
        break;
    case 20:
        em025_act6_20(self, 0);
        break;
    case 21:
        em025_act6_20(self, 1);
        break;
    case 22:
        em025_act6_9(self, 1);
        break;
    case 23:
        em025_act6_15(self, 1);
        break;
    case 24:
        em025_act6_9(self, 2);
        break;
    case 25:
        em025_act6_6(self, 1);
        break;
    case 26:
        em025_act6_9(self, 3);
        break;
    }
}

/* 0x801A1894 (0xD0): the bite: motion 10 with hit window 1, the shell drop at frame 410. */
void em025_act7_0(struct _ENEMY_WORK* self) {
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 10, 100, 0, 1);
        em_height_set(self, 630.0f);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 0, 410.0f, 0.0f) == 1) {
            em_busy_net_notify(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A1964 (0xE8): the double bite: motion 18 with hit windows 5 and 6, the shell drop at frame 190. */
void em025_act7_1(struct _ENEMY_WORK* self) {
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 18, 100, 0, 1);
        em_height_set(self, 630.0f);
        em_hit_window_set(self, 0, 5, 8);
        em_hit_window_set(self, 1, 6, 16);
        break;
    case 1:
        if (em_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            em_busy_net_notify(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A1A4C (0x16C): the shore sweep: motion 23 with the windows at frames 200/266, the first hit record's
 * attack byte at frame 80 and the shell drop at 286 past x = 5000. */
void em025_act7_2(struct _ENEMY_WORK* self, u32 shore) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 23, 30, 0);
        em_height_set(self, 0.0f);
        em_hit_window_set(self, 0, 33, 8);
        break;
    case 1:
        if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            self->hit_slots_0xA08[0].bytes[0x44] = 0xD8;
        }
        if (em_frame_check(self, 0, 200.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 2, 24);
            em_hit_window_set(self, 1, 30, 24);
        }
        if (em_frame_check(self, 0, 266.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 34, 16);
        }
        if (em_frame_check(self, 0, 286.0f, 0.0f) == 1 && self->pos.x > 5000.0f) {
            em_busy_net_notify(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            em025_finish_to_stand(self);
        }
        break;
    }
}

/* 0x801A1BB8 (0x2A8): the tidal wave: motion 25, the wave shells 4000 and 8000 ahead and at the far shore. */
void em025_act7_3(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 near;
    nw4r::math::VEC3 far;

    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 25, 30, 0);
        em_height_set(self, 0.0f);
        em_hit_window_set_default(self, 0, 23);
        em_hit_window_set_default(self, 1, 31);
        break;
    case 1:
        if (em_frame_check(self, 0, 700.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 24);
        }
        if (em_frame_check(self, 0, 520.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 43, 0, 0);
        }
        if (em_frame_check(self, 0, 516.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 4000.0f);
            rotVecY(&ofs, self->field_0x1C0);
            addVec3(&near, &self->pos, &ofs);
            copyVec3(&pos, &near);
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.5f, &pos);
        }
        if (em_frame_check(self, 0, 518.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 8000.0f);
            rotVecY(&ofs, self->field_0x1C0);
            addVec3(&far, &self->pos, &ofs);
            copyVec3(&pos, &far);
            em_wave_amp(&pos, &pos, 1000.0f, 0, (self->bits_0x1EC + 0x27EAD) >> 5);
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.5f, &pos);
        }
        if (em_frame_check(self, 0, 520.0f, 0.0f) == 1) {
            setVector3(&pos, 9642.0f, 0.0f, 0.0f);
            em_wave_amp(&pos, &pos, 1000.0f, 0, self->bits_0x1EC);
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.5f, &pos);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A1E60 (0xF4): the roar under water: motion 29 with hit window 7, the camera focus and shell drop at
 * frame 366. */
void em025_act7_4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 29, 0, 0);
        em_height_set(self, 630.0f);
        em_hit_window_set_default(self, 0, 7);
        break;
    case 1:
        if (em_frame_check(self, 0, 366.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 6, &ofs);
            em_busy_net_notify(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A1F54 (0xC0): the body slam: motion 4 with hit window 4, the shell drop at frame 306. */
void em025_act7_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 4, 60, 0);
        em_height_set(self, 630.0f);
        em_hit_window_set_default(self, 0, 4);
        break;
    case 1:
        if (em_frame_check(self, 0, 306.0f, 0.0f) == 1) {
            em_busy_net_notify(self, 40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A2014 (0x33C): the shore charge: slides from (-10200, -19000) to (220, -19000) while turning to 0, then
 * motion 7 with the speed steps, the shell drop and the shell-table hit at frame 576. */
void em025_act7_6(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 lim;
    nw4r::math::VEC3 clampv;
    nw4r::math::VEC3 end;
    f32 x0;

    VEC3_ctor(&ofs);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 284);
        em_height_set(self, em_height_base_get(self));
        copyVec3(&self->pos, setVec3(&start, -10200.0f, 0.0f, -19000.0f));
        self->field_0x1C0 = 0x8000;
        break;
    case 1:
        x0 = setVec3(&from, -10200.0f, 0.0f, -19000.0f)->x;
        self->pos.x += (setVec3(&to, 220.0f, 0.0f, -19000.0f)->x - x0) / 360.0f;
        if (self->pos.x > setVec3(&lim, 220.0f, 0.0f, -19000.0f)->x) {
            self->pos.x = setVec3(&clampv, 220.0f, 0.0f, -19000.0f)->x;
        }
        self->field_0x1C0 -= 91;
        if ((u32)(self->field_0x1C0 - 91) > 65353) {
            self->state++;
            em_mot_set(self, 7, 0, 0);
            self->field_0x1C0 = 0;
            copyVec3(&self->pos, setVec3(&end, 220.0f, 0.0f, -19000.0f));
            em_hit_window_set(self, 0, 8, 8);
            break;
        }
        em_height_add(self, (630.0f - em_height_base_get(self)) / 180.0f);
        if (self->field_0x1AC >= 630.0f) {
            self->field_0x1AC = 630.0f;
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 270.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 1.2f);
        }
        if (em_frame_check(self, 0, 408.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 1.5f);
        }
        if (em_frame_check(self, 0, 564.0f, 0.0f) == 1) {
            em_busy_net_notify(self, 100);
        }
        if (em_frame_check(self, 0, 576.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, -2300.0f);
            ((ShellSetPosJobFn)shell_set_func_ptr->method_0x14)(self, &ofs, 27, self->area_no, self->field_0xAEA);
        }
        em_height_add(self, (630.0f - em_height_base_get(self)) / 180.0f);
        if (self->field_0x1AC >= 630.0f) {
            self->field_0x1AC = 630.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A2350 (0x11C): the shore rise: motion 19 placed at (-8000, 100) facing +X, the 0x81E timer in the
 * 160..400 window and the shell drop at frame 406. */
void em025_act7_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 19, 0, 0);
        em_height_set(self, 630.0f);
        setVector3(&self->pos, -8000.0f, 0.0f, 100.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x4000;
        self->field_0x1C4 = 0;
        em_hit_window_set_default(self, 0, 9);
        break;
    case 1:
        if (em_frame_check(self, 3, 160.0f, 400.0f) == 1) {
            em_timer_0x81E_arm(self);
        }
        if (em_frame_check(self, 0, 406.0f, 0.0f) == 1) {
            em_busy_net_notify(self, 80);
        }
        if (em_mot_end_ck(self) == 1) {
            em_height_set(self, em_height_base_get(self));
            em025_finish_to_swim(self);
        }
        break;
    }
}

/* 0x801A246C (0x248): the water spout: motion 5 with hit window 25, three wave shells at frames 140..148. */
void em025_act7_8(struct _ENEMY_WORK* self, u32 shore) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 5, 60, 0, 1);
        em_height_set(self, 630.0f);
        em_hit_window_set_default(self, 0, 25);
        break;
    case 1:
        if (em_frame_check(self, 0, 136.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 43, 0, 0);
        }
        if (em_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            if ((u8)shore == 1) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                em_wave_amp(&pos, &pos, 1000.0f, 0, self->bits_0x1EC);
            } else {
                copyVec3(&pos, &self->vec_0x36C);
            }
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.0f, &pos);
        }
        if (em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 2000.0f);
            em_wave_amp(&pos, &pos, 1000.0f, 0, self->bits_0x1EC);
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.0f, &pos);
        }
        if (em_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, -3000.0f);
            em_wave_amp(&pos, &pos, 1000.0f, 0, self->bits_0x1EC);
            shell_set_func_ptr->method_0x68(self, 0, 18, 1.0f, &pos);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801A26B4 (0x214): the head slam: motion 14 with hit windows 10 and 22, the camera shakes at frames 660/680
 * and the shell-table impact at 680. */
void em025_act7_9(struct _ENEMY_WORK* self, u32 shore) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&pos);
    if ((u8)shore == 1) {
        em_flag_0x116_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 14, 30, 0);
        em_hit_window_set(self, 0, 10, 8);
        em_hit_window_set(self, 1, 22, 24);
        break;
    case 1:
        if ((u8)shore == 1) {
            em025_shore_drift(self);
        }
        if (em_frame_check(self, 3, 400.0f, 630.0f) == 1) {
            em_timer_0x81E_arm(self);
        }
        if (em_frame_check(self, 0, 650.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            em_hit_window_set(self, 0, 11, 16);
        }
        if (em_frame_check(self, 0, 660.0f, 0.0f) == 1) {
            em025_joint_quake(self, 25, 6, 3);
        }
        if (em_frame_check(self, 0, 680.0f, 0.0f) == 1) {
            em025_joint_quake(self, 25, 6, 3);
            copyVec3(&pos, setVec3(&ofs, 0.0f, -840.0f, -450.0f));
            ((ShellSetScaleJobFn)shell_set_func_ptr->method_0x2C)(self, 9, 20, &pos, 6.8f, 0xFFFF);
            if (self->pos.x > 5200.0f) {
                em_busy_net_notify(self, 80);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A28C8 (0x4E4): the shore ram: motion 28 with the windows at 290/326, the shell-table impact at 326 and
 * the three aimed wave shells at 330 (straight at the target when the target is out of the ring). */
void em025_act7_10(struct _ENEMY_WORK* self, u32 shore) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 ahead;
    nw4r::math::VEC3 left;
    nw4r::math::VEC3 right;
    u32 aim;

    VEC3_ctor(&pos);
    VEC3_ctor(&joint);
    VEC3_ctor(&ofs);
    aim = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 28, 30, 0);
        em_height_set(self, 0.0f);
        em_hit_window_set_default(self, 0, 27);
        em_hit_window_set(self, 1, 35, 8);
        break;
    case 1:
        if ((u8)shore == 1) {
            em025_shore_drift(self);
        }
        if (em_frame_check(self, 0, 72.0f, 0.0f) == 1) {
            self->hit_slots_0xA08[1].bytes[0x44] = 0x28;
        }
        if (em_frame_check(self, 0, 290.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 28, 16);
        }
        if (em_frame_check(self, 0, 326.0f, 0.0f) == 1) {
            em025_joint_quake(self, 25, 5, 3);
            copyVec3(&ofs, setVec3(&tmp, 0.0f, -800.0f, 400.0f));
            ((ShellSetScaleJobFn)shell_set_func_ptr->method_0x2C)(self, 9, 20, &ofs, 4.0f, 0xFFFF);
            if (self->pos.x > 5380.0f) {
                em_busy_net_notify(self, 40);
            }
        }
        if (em_frame_check(self, 0, 330.0f, 0.0f) == 1) {
            eft007_part_spawn(self, 43, 0, 0);
        }
        if (em_frame_check(self, 0, 330.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 380.0f);
            em025_joint_point_get(self, 20, &joint, &ofs);
            if (calcDistanceSqXZ(&joint, &self->vec_0x36C) > 1000000.0f) {
                aim = 1;
            }
            setVector3(&ofs, 0.0f, 0.0f, 200.0f);
            em025_joint_point_get(self, 5, &joint, &ofs);
            if (calcDistanceSqXZ(&joint, &self->vec_0x36C) > 1000000.0f) {
                aim = 1;
            }
            setVector3(&ofs, 0.0f, 0.0f, 200.0f);
            em025_joint_point_get(self, 2, &joint, &ofs);
            if (calcDistanceSqXZ(&joint, &self->vec_0x36C) > 1000000.0f) {
                aim = 1;
            }
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            em025_joint_point_get(self, 2, &joint, &ofs);
            if (calcDistanceSqXZ(&joint, &self->vec_0x36C) < 36000000.0f) {
                aim = 1;
            }
            if (aim == 1) {
                copyVec3(&pos, &self->vec_0x36C);
            } else {
                setVector3(&ofs, 0.0f, 0.0f, 6000.0f);
                rotVecY(&ofs, self->field_0x1C0);
                addVec3(&ahead, &self->pos, &ofs);
                copyVec3(&pos, &ahead);
                em_wave_amp(&pos, &pos, 2000.0f, 0, self->bits_0x1EC);
            }
            shell_set_func_ptr->method_0x68(self, 1, 18, 1.0f, &pos);
            setVector3(&ofs, 2000.0f, 0.0f, 4000.0f);
            rotVecY(&ofs, self->field_0x1C0);
            addVec3(&left, &self->pos, &ofs);
            copyVec3(&pos, &left);
            em_wave_amp(&pos, &pos, 1500.0f, 0, (self->bits_0x1EC + 0x157E7) >> 4);
            shell_set_func_ptr->method_0x68(self, 1, 18, 1.0f, &pos);
            setVector3(&ofs, -2000.0f, 0.0f, 4000.0f);
            rotVecY(&ofs, self->field_0x1C0);
            addVec3(&right, &self->pos, &ofs);
            copyVec3(&pos, &right);
            em_wave_amp(&pos, &pos, 1500.0f, 0, (self->bits_0x1EC + 0x27EAD) >> 5);
            shell_set_func_ptr->method_0x68(self, 1, 18, 1.0f, &pos);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A2DAC - the state_sub dispatcher of the `state`-band's second half: 0..14, with the double
 * entries 2/12, 8/11, 9/13, 10/14 passing the 0/1 selector to the four two-argument motion steps. */
void em025_act7_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em025_act7_0(self);
        break;
    case 1:
        em025_act7_1(self);
        break;
    case 2:
        em025_act7_2(self, 0);
        break;
    case 3:
        em025_act7_3(self);
        break;
    case 4:
        em025_act7_4(self);
        break;
    case 5:
        em025_act7_5(self);
        break;
    case 6:
        em025_act7_6(self);
        break;
    case 7:
        em025_act7_7(self);
        break;
    case 8:
        em025_act7_8(self, 0);
        break;
    case 9:
        em025_act7_9(self, 0);
        break;
    case 10:
        em025_act7_10(self, 0);
        break;
    case 11:
        em025_act7_8(self, 1);
        break;
    case 12:
        em025_act7_2(self, 1);
        break;
    case 13:
        em025_act7_9(self, 1);
        break;
    case 14:
        em025_act7_10(self, 1);
        break;
    }
}

/* The sound programs `em025_act10_se_play` plays per sub-state (`em_se_tbl_play`) and `em025_act11_se_play` plays
 * (`em_se_tbl_play_alt`): each record precedes the table that points at it. */
EmSeRecord em025_se_rec_34 = {0x65, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_34[2] = {
    {0x00, {0, 0, 0}, &em025_se_rec_34},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em025_se_rec_F2 = {0x66, 0, 10, 0, 1, {0, 1, 0, 0}, 0};

/* The `.sdata` words the 0x16/0xFE entries of the two water-exit programs point at: the height and the action
 * pair (action 6, sub-state 22) they hand on (GUESS names). */
f32 em025_se_height = -2200.0f;
u8 em025_se_next_act[4] = {6, 22, 0, 0};

EmSeEntry em025_se_seq_F2[4] = {
    {0x0C, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em025_se_rec_F2},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em025_se_height},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em025_se_next_act},
};

EmSeRecord em025_se_rec_F6 = {0x67, 0, 30, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_F6[3] = {
    {0x00, {0, 0, 0}, &em025_se_rec_F6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em025_se_height},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em025_se_next_act},
};

EmSeRecord em025_se_rec_BE_a = {0x6F, 0, 10, 0, 1, {0, 1, 0, 0}, 0};

EmSeRecord em025_se_rec_BE_b = {0x72, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_BE[3] = {
    {0x00, {0, 0, 0}, &em025_se_rec_BE_a},
    {0x00, {0, 0, 0}, &em025_se_rec_BE_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em025_se_rec_19 = {0x68, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_19[2] = {
    {0x00, {0, 0, 0}, &em025_se_rec_19},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em025_se_rec_5A_a = {0x69, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeRecord em025_se_rec_5A_b = {0x6D, 0, 10, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em025_se_rec_5A_c = {0x6A, 0, 30, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_5A[5] = {
    {0x00, {0, 0, 0}, &em025_se_rec_5A_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(450)},
    {0x00, {0, 0, 0}, &em025_se_rec_5A_b},
    {0x00, {0, 0, 0}, &em025_se_rec_5A_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em025_se_rec_17 = {0x6E, 0, 10, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em025_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em025_se_rec_BD_a = {0x70, 0, 10, 0, 1, {0, 1, 0, 0}, 0};

EmSeRecord em025_se_rec_BD_b = {0x71, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_BD[3] = {
    {0x00, {0, 0, 0}, &em025_se_rec_BD_a},
    {0x00, {0, 0, 0}, &em025_se_rec_BD_b},
    {0xFF, {0, 0, 0}, 0},
};

/* The sound position record of the alternate program: the 110..250 turn window at angle -0x3000. */
EmSePos em025_se_pos_alt = {110.0f, 250.0f, -1, -0x3000, 0};

EmSeRecord em025_se_rec_alt = {0x6B, 0, 20, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em025_se_seq_alt[4] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em025_se_pos_alt},
    {0x00, {0, 0, 0}, &em025_se_rec_alt},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801A2E30 (0x2A4): action 10's sound programs: one `em_se_tbl_play` table per sub-state, finishing the action
 * for any other. */
void em025_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em025_se_seq_17, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em025_se_seq_17, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0x1E);
        break;
    case 0x5A:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5A);
        break;
    case 0x5B:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5B);
        break;
    case 0x5C:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5C);
        break;
    case 0x5D:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5D);
        break;
    case 0x5E:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5E);
        break;
    case 0x5F:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0x5F);
        break;
    case 0x34:
        em_se_tbl_play(self, em025_se_seq_34, 4, 0x34);
        break;
    case 0xF1:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0xF1);
        break;
    case 0xF2:
        em_se_tbl_play(self, em025_se_seq_F2, 4, 0xF2);
        break;
    case 0xF3:
        em_se_tbl_play(self, em025_se_seq_19, 0, 0xF3);
        break;
    case 0xF4:
        em_se_tbl_play(self, em025_se_seq_5A, 0, 0xF4);
        break;
    case 0xF5:
        em_se_tbl_play(self, em025_se_seq_34, 4, 0xF5);
        break;
    case 0xF6:
        em_se_tbl_play(self, em025_se_seq_F6, 4, 0xF6);
        break;
    case 0xBD:
        em_se_tbl_play(self, em025_se_seq_BD, 0, 0xBD);
        break;
    case 0xBE:
        em_se_tbl_play(self, em025_se_seq_BE, 4, 0xBE);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* 0x801A30D4 - the tail thunk into `em_se_tbl_play_alt(self, &em025_se_seq_alt, 0, 0)`. */
void em025_act11_se_play(struct _ENEMY_WORK* self) {
    em_se_tbl_play_alt(self, em025_se_seq_alt, 0, 0);
}

/* 0x801A30E8 (0x4E4): the cut-scene emergence: motion 103 at the demo position, the 160-frame wait, the effect
 * bursts of the 350..596 window, then the dive to the deep height and the surfacing swim. */
void em025_act13_2(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 103, 0, 0);
        camera_demo_event_set(0x39, self);
        em_demo_pos_set(self, -3515.6f, 0.0f, -2467.8f);
        em_demo_rot_set(self, 0.0f, 240.0f, 0.0f);
        em_height_set(self, 630.0f);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (++self->timer_0x020 >= 160) {
            self->state++;
            em_mot_set(self, 103, 0, 338);
            em_mot_speed_set(self, 0.5f);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 350.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 700.0f);
            eft_em_spawn(self, 0xB8, 20, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 380.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 18, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 404.0f, 0.0f) == 1 || em_frame_check(self, 0, 414.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 5, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 428.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 36, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 442.0f, 0.0f) == 1 || em_frame_check(self, 0, 460.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 38, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 480.0f, 0.0f) == 1 || em_frame_check(self, 0, 496.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 40, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 506.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 41, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 516.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 42, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 540.0f, 0.0f) == 1 || em_frame_check(self, 0, 556.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0x7E, 44, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 580.0f, 0.0f) == 1 || em_frame_check(self, 0, 596.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, -200.0f, -300.0f);
            eft_em_spawn(self, 0x7E, 45, &ofs, 1.0f);
        }
        if (++self->timer_0x020 >= 480) {
            self->state++;
            em_mot_set(self, 17, 0, 0);
            em_demo_pos_set(self, -13200.0f, 0.0f, 12000.0f);
            em_demo_rot_set(self, 0.0f, -35.0f, 0.0f);
            em_height_set(self, -1930.0f);
            em025_act_bits_clear(self);
        }
        break;
    case 3:
        em_height_add(self, em_key_curve_eval(self, em025_surface_curve));
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->field_0x1AC >= 630.0f) {
            self->field_0x1AC = 630.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 1, 30, 0);
            em_height_set(self, 630.0f);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = 12.5f;
            em025_act_bits_clear(self);
        }
        break;
    case 4:
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    }
}

/* 0x801A35CC (0x688): the cut-scene shore landing: motion 30 at the demo position with its effect bursts,
 * then the swim motion at the ground height. */
void em025_act13_3(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 30, 0, 0);
        camera_demo_event_set(0x3A, self);
        em_demo_pos_set(self, 1854.0f, 0.0f, 0.0f);
        em_demo_rot_set(self, 0.0f, 87.0f, 0.0f);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 0, 388.0f, 0.0f) == 1) {
            setVector3(&ofs, 500.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0xCE, 9, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 396.0f, 0.0f) == 1) {
            setVector3(&ofs, 500.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0xCE, 9, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 542.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, -200.0f, 1000.0f);
            eft_em_spawn(self, 0xB8, 20, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 600.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 9, NULL, 1.0f);
            setVector3(&ofs, 0.0f, -200.0f, 1000.0f);
            eft_em_spawn(self, 0xB8, 20, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 670.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, -600.0f, 500.0f);
            eft_em_spawn(self, 0xB8, 20, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 680.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 15, NULL, 1.0f);
            setVector3(&ofs, 0.0f, -600.0f, 500.0f);
            eft_em_spawn(self, 0xB8, 20, &ofs, 1.0f);
        }
        if (em_frame_check(self, 0, 710.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 20, NULL, 0.9f);
        }
        if (em_frame_check(self, 0, 736.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 9, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 740.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 9, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 768.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 20, NULL, 0.9f);
        }
        if (em_frame_check(self, 0, 790.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 15, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 798.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 20, NULL, 0.9f);
        }
        if (em_frame_check(self, 0, 820.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 18, NULL, 0.9f);
        }
        if (em_frame_check(self, 0, 836.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 9, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 840.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 18, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 842.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xCE, 9, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 860.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 18, NULL, 0.9f);
        }
        if (em_frame_check(self, 0, 900.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 3, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 920.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 37, NULL, 0.8f);
        }
        if (em_frame_check(self, 0, 960.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 38, NULL, 0.8f);
        }
        if (em_frame_check(self, 0, 1000.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 41, NULL, 0.8f);
        }
        if (em_frame_check(self, 0, 1020.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 42, NULL, 0.8f);
        }
        if (em_frame_check(self, 0, 1040.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0xB8, 42, NULL, 0.8f);
        }
        if (em_frame_check(self, 0, 1100.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0x7E, 45, NULL, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 1, 0, 0);
            em_height_set(self, em_height_base_get(self));
        }
        break;
    }
}

/* 0x801A3C54 (0x168): the cut-scene shore bellow: motion 107 with the camera's 0x3B mode, the turn window and
 * the splash effects at frame 262. */
void em025_act13_4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 107, 0, 0);
        camera_mode_req(0x3B, (u32)self);
        self->field_0x1C0 = 0x4000;
        break;
    case 1:
        em_turn_in_window(self, 110.0f, 250.0f, -0x3000);
        if (em_frame_check(self, 0, 262.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 20, &pos);
            pos.y = 5.0f + self->field_0x20C;
            eft_spawn_type11(self, &pos, 0, 4.0f);
        }
        if (em_frame_check(self, 0, 262.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 20, &pos);
            pos.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&pos, self->area_no, 1, self->field_0x1C0, 5.0f * get_em_chg_scale(self));
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
        }
        break;
    }
}

/* 0x801A3DBC - the action dispatcher: 2 -> `em025_act13_2`, 3 -> `em025_act13_3`, 4 -> `em025_act13_4`. */
void em025_act13_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        em025_act13_2(self);
        break;
    case 3:
        em025_act13_3(self);
        break;
    case 4:
        em025_act13_4(self);
        break;
    }
}

/* 0x801A3DEC - the action-id dispatcher (0x1E5) plus the block's shared tail: the `field_0x1E2 == 4`
 * gate runs `em_shake_req_set`. */
void em025_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em025_act0_dispatch(self);
        break;
    case 1:
        em025_act1_dispatch(self);
        break;
    case 2:
        em025_act2_dispatch(self);
        break;
    case 3:
        em025_act3_dispatch(self);
        break;
    case 6:
        em025_act6_dispatch(self);
        break;
    case 7:
        em025_act7_dispatch(self);
        break;
    case 10:
        em025_act10_se_play(self);
        break;
    case 11:
        em025_act11_se_play(self);
        break;
    case 13:
        em025_act13_dispatch(self);
        break;
    }
    if (self->field_0x1E2 == 4) {
        em_shake_req_set(self);
    }
}

/* 0x801A3E90 (0x148): the per-frame flag update: bit 0x200/0x400 by the cut-scene motions, bit 0x800 cleared in
 * the three guarded motion windows. */
void em025_flags_update(struct _ENEMY_WORK* self) {
    u16 mot = em_get_mot_no(self);

    if ((u32)(mot - 105) <= 1 || (s32)mot == 109) {
        self->flags_0x836 = (self->flags_0x836 & 0xFBFF) | 0x200;
    } else {
        self->flags_0x836 = (self->flags_0x836 | 0x400) & 0xFDFF;
    }
    if (em_act_ck_c1(self, 7, 9) == 1 && mot == 14 && em_after_frame_check(self, 3, 298.0f, 998.0f) == 1) {
        self->flags_0x836 &= 0xF7FF;
        return;
    }
    if (em_act_ck_c1(self, 7, 7) == 1) {
        self->flags_0x836 &= 0xF7FF;
        return;
    }
    if (em_act_ck_c1(self, 1, 2) == 1 && mot == 24 && em_after_frame_check(self, 3, 336.0f, 618.0f) == 1) {
        self->flags_0x836 &= 0xF7FF;
        return;
    }
    self->flags_0x836 |= 0x800;
}

/* 0x801A3FD8 (0x240): picks the +0x38D posture from the motion: on map 6 / area 1 by motion and frame, elsewhere
 * 1 for the bellow motions and 2 otherwise. */
void em025_posture_update(struct _ENEMY_WORK* self) {
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        switch (em_get_mot_no(self)) {
        case 3:
        case 4:
        case 23:
        case 101:
        case 111:
        case 114:
            self->field_0x38D = 3;
            break;
        case 11:
            if (em_frame_check(self, 1, 80.0f, 0.0f) != 0) {
                self->field_0x38D = 2;
            } else {
                self->field_0x38D = 3;
            }
            break;
        case 2:
        case 7:
        case 9:
        case 10:
        case 19:
        case 102:
        case 103:
            self->field_0x38D = 2;
            break;
        case 109:
            self->field_0x38D = 1;
            break;
        case 18:
            if (em_frame_check(self, 3, 180.0f, 240.0f) != 0) {
                if (self->flags_0x836 & 0x2000) {
                    self->field_0x38D = 6;
                } else {
                    self->field_0x38D = 5;
                }
            } else {
                if (self->flags_0x836 & 0x2000) {
                    self->field_0x38D = 4;
                } else {
                    self->field_0x38D = 0;
                }
            }
            break;
        case 5:
        case 6:
        case 8:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 20:
        case 21:
        case 22:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 110:
        case 112:
        case 113:
        default:
            if (self->flags_0x836 & 0x2000) {
                self->field_0x38D = 4;
            } else {
                self->field_0x38D = 0;
            }
            break;
        }
    } else if (em_get_mot_no(self) == 109 || (em_get_mot_no(self) == 106 && em_frame_check(self, 2, 230.0f, 0.0f) != 0)) {
        self->field_0x38D = 1;
    } else {
        self->field_0x38D = 2;
    }
}

/* 0x801A4218 - action 7's status-bit pass: the (7,6) and (7,7) windows re-arm bits 0x40000/0x80000,
 * otherwise `em_status_bits_init` seeds and area 1 clears 0x40000. */
void em025_status_bits_update(struct _ENEMY_WORK* self) {
    if (em_act_ck_c1(self, 7, 6) == 1) {
        em_status_bits_clr(self, -1);
        if ((u16)em_get_mot_no(self) == 7) {
            if (em_after_frame_check(self, 2, 558.0f, 0.0f) == 1) {
                em_status_bits_set(self, 0x40000);
            }
        }
    } else if (em_act_ck_c1(self, 7, 7) == 1) {
        em_status_bits_clr(self, -1);
        em_status_bits_set(self, 0x80000);
    } else {
        em_status_bits_init(self);
        if (self->area_no == 1) {
            em_status_bits_clr(self, 0x40000);
        }
    }
}

/* 0x801A42F4 - transforms `tmp` through the joint's world matrix `joint`, then copies it into
 * `out`.  The four-argument call sites are the distance probes in `em025_act7_10`. */
void em025_joint_point_get(struct _ENEMY_WORK* self, u32 joint, Vec3* out, Vec3* tmp) {
    nw4r::math::MTX34 m;
    MTX34_ctor(&m);
    get_joint_wmat_em(self, joint, &m);
    mulVecMatAddTrans(tmp, &m);
    out->x = tmp->x;
    out->y = tmp->y;
    out->z = tmp->z;
}

/* 0x801A437C (0x188): spawns effect `type` of family `family` (0 the ground burst, 1 the area splash, 2 the
 * type-11 splash) at joint `joint` (the work position for 0xFF), at the ground height. */
void em025_effect_spawn(struct _ENEMY_WORK* self, u32 family, u32 type, u32 joint, s32 arg, f32 scale) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch ((u8)family) {
    case 0:
        if (joint == 0xFF) {
            pos.x = self->pos.x;
            pos.y = 5.0f + self->field_0x20C;
            pos.z = self->pos.z;
            eft009_set_pos(type, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, (u8)type, arg, scale);
        }
        break;
    case 1:
        if (joint == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, type, arg, scale);
        break;
    case 2:
        if (joint == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_type11(self, &pos, (u8)type, scale);
        break;
    }
}

/* The per-motion action dispatcher.  Runs the five per-frame setup steps, then dispatches on the
 * motion number (0x73 cases) and finally walks the four per-slot joint effects. */
void em025_motion_events(_ENEMY_WORK* self) {
    Em025Block* blk = &self->em025_0x328;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v2C;
    nw4r::math::VEC3 v20;
    nw4r::math::VEC3 v14;
    nw4r::math::VEC3 rec8;
    u16 temp_r3;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v2C);
    VEC3_ctor(&v20);
    VEC3_ctor(&v14);
    em025_flags_update(self);
    em025_posture_update(self);
    em025_status_bits_update(self);
    em025_sway_update(self);
    em025_head_track(self);
    temp_r3 = em_get_mot_no(self);
    switch (temp_r3) {
    case 0x1:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    case 0x2:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 300.0f, 500.0f) == 1U) && (em025_act_bit_ck(self, 0) == 0)) {
            setVector3(&v14, 0.0f, 0.0f, 2000.0f);
            em025_joint_point_get(self, 0x14, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 1500.0f);
                eft_em_spawn_param(self, 0x74, 0x14, &v14, 1.0f, 0);
                em025_act_bit_set(self, 0);
            }
        }
        setVector3(&v14, 0.0f, 0.0f, 800.0f);
        em025_joint_point_get(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 3, &v20);
        if ((v2C.y < self->field_0x20C) && (v20.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, 800.0f, 0.0f, 800.0f);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xF4A0);
            setVector3(&v14, -800.0f, 0.0f, 800.0f);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xB61);
        }
        get_joint_wpos_em(self, 5, &v2C);
        get_joint_wpos_em(self, 0x25, &v20);
        if ((v2C.y < self->field_0x20C) && (v20.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, 800.0f, 0.0f, 400.0f);
            eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF4A0);
            setVector3(&v14, -800.0f, 0.0f, 400.0f);
            eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xB61);
        }
        if (em_after_frame_check(self, 3, 700.0f, 1100.0f) == 1U) {
            if (em025_act_bit_ck(self, 1) == 0) {
                get_joint_wpos_em(self, 0x25, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn_param(self, 0x74, 0x25, &v14, 1.0f, 0);
                    em025_act_bit_set(self, 1);
                }
            }
            if (em025_act_bit_ck(self, 2) == 0) {
                get_joint_wpos_em(self, 0x29, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn_param(self, 0x74, 0x29, &v14, 1.0f, 0);
                    em025_act_bit_set(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, 800.0f, 1100.0f) == 1U) && (em025_act_bit_ck(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_param(self, 0x7E, 0x2D, &v14, 1.0f, 0);
                em025_act_bit_set(self, 3);
            }
        }
        break;
    case 0x3:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    case 0x4:
        if (em_after_frame_check(self, 0, 184.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 300.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 800.0f);
            eft_em_spawn_param(self, 0x9F, 0x19, &v14, 1.0f, 0);
            setVector3(&v2C, 0.0f, 0.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, 356.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 9, &v14, 1.0f, 0x8000);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    case 0x5:
        if ((em_after_frame_check(self, 0, 6.0f, 0.0f) == 1U) || (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1U)) {
            setVector3(&v14, 0.0f, 200.0f, 0.0f);
            eft_em_spawn(self, 0x84, 0x12, &v14, 1.0f);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    case 0x6:
        get_joint_wpos_em(self, 5, &v2C);
        if (v2C.y > self->field_0x20C) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 800.0f, 0.0f, 2000.0f);
                eft_em_spawn_param(self, 0x5B, 4, &v14, 1.0f, 0xF8E5);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -800.0f, 0.0f, 2000.0f);
                eft_em_spawn_param(self, 0x5B, 4, &v14, 1.0f, 0x71C);
            }
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, 228.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x7E, 0x2D, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 3, 420.0f, 502.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1000.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x25, &v20);
            if ((v2C.y > self->field_0x20C) && (v20.y < self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, 320.0f, 400.0f, 900.0f);
                eft_em_spawn(self, 0x71, 0x19, &v14, 1.0f);
            }
        }
        if (em_after_frame_check(self, 3, 420.0f, 520.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1000.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x26, &v20);
            if ((v2C.y > self->field_0x20C) && (v20.y < self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, -220.0f, -800.0f, 800.0f);
                eft_em_spawn(self, 0x71, 0x19, &v14, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 402.0f, 0.0f) == 1U) {
            setVector3(&v14, 1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, 1.0f, 0xEAAC);
            setVector3(&v14, -1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, 1.0f, 0x1555);
        }
        if (em_after_frame_check(self, 0, 414.0f, 0.0f) == 1U) {
            setVector3(&v14, 1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x13, &v14, 1.0f, 0xEAAC);
            setVector3(&v14, -1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x13, &v14, 1.0f, 0x1555);
        }
        if (em_after_frame_check(self, 0, 428.0f, 0.0f) == 1U) {
            setVector3(&v14, 1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, 1.0f, 0xEAAC);
            setVector3(&v14, -1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, 1.0f, 0x1555);
        }
        if (em_after_frame_check(self, 0, 446.0f, 0.0f) == 1U) {
            setVector3(&v14, 1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 5, &v14, 1.0f, 0xEAAC);
            setVector3(&v14, -1000.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0xA3, 5, &v14, 1.0f, 0x1555);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 500.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 172.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
            setVector3(&v2C, 0.0f, 0.0f, -1500.0f);
            camera_enemy_focus_pos_set(self, 2, &v2C);
        }
        if (em_after_frame_check(self, 0, 284.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1500.0f);
            eft_em_spawn(self, 0xA2, 0x14, &v14, 1.0f);
            setVector3(&v2C, 0.0f, 0.0f, -1500.0f);
            camera_enemy_focus_pos_set(self, 4, &v2C);
        }
        if (em_after_frame_check(self, 0, 384.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 2000.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
            setVector3(&v2C, 0.0f, 0.0f, -1500.0f);
            camera_enemy_focus_pos_set(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, 420.0f, 0.0f) == 1U) {
            setVector3(&v2C, 0.0f, 0.0f, -1500.0f);
            camera_enemy_focus_pos_set(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, 564.0f, 0.0f) == 1U) {
            em_camera_req(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 0, 576.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1000.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 754.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x7E, 0x2D, &v14, 1.0f);
        }
        break;
    case 0x8:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    case 0x9:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 3, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 3, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 3, 660.0f, 900.0f) == 1U) {
            if (em025_act_bit_ck(self, 0) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 800.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -800.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 0);
                }
            }
            if (em025_act_bit_ck(self, 1) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x24, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x24, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x24, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 1);
                }
            }
            if (em025_act_bit_ck(self, 2) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 2);
                }
            }
            if (em025_act_bit_ck(self, 3) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 4, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 4, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 4, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 3);
                }
            }
            if (em025_act_bit_ck(self, 4) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -1000.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 4);
                }
            }
            if (em025_act_bit_ck(self, 5) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x12, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 800.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x12, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -800.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x12, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 5);
                }
            }
            if (em025_act_bit_ck(self, 6) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 600.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xF334);
                    setVector3(&v14, -600.0f, 0.0f, 2000.0f);
                    eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xCCD);
                    em025_act_bit_set(self, 6);
                }
            }
            if (em025_act_bit_ck(self, 7) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 800.0f);
                em025_joint_point_get(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x8A, 0x19, &v14, 1.0f, 0);
                    em025_act_bit_set(self, 7);
                }
            }
        }
        break;
    case 0xA:
        if (em_after_frame_check(self, 0, 244.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 9, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 414.0f, 0.0f) == 1U) {
            setVector3(&v14, 900.0f, 0.0f, 500.0f);
            eft_em_spawn_param(self, 0x9E, 4, &v14, 1.0f, 0x471C);
            setVector3(&v2C, 0.0f, 0.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 6, &v2C);
        }
        if ((em_after_frame_check(self, 3, 290.0f, 660.0f) == 0) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        if ((em_after_frame_check(self, 3, 340.0f, 720.0f) == 0) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 3, 86.0f, 736.0f) == 0) {
            setVector3(&v14, 0.0f, -1000.0f, 0.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
        }
        if ((s32) (system_w.field_0x0c & 7) == 0) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x9D, 9, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 210.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x9D, 0xF, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 380.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x9D, 9, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 674.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x9D, 9, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 714.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x9D, 0xF, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 600.0f);
            eft_em_spawn(self, 0xA1, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 734.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 700.0f);
            eft_em_spawn_param(self, 0xA1, 0x19, &v14, 1.0f, 0);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 2, 550.0f, 0.0f) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            if ((v2C.y < self->field_0x20C) && (v20.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, 800.0f, 0.0f, 2500.0f);
                eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF4A0);
                setVector3(&v14, -800.0f, 0.0f, 2500.0f);
                eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xB61);
            }
        }
        if (em_after_frame_check(self, 0, 480.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1500.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 600.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1300.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 610.0f, 0.0f) == 1U) {
            setVector3(&v2C, 200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, 660.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 710.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 400.0f);
            eft_em_spawn(self, 0x7E, 0x2A, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 920.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, -500.0f, 800.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 940.0f, 0.0f) == 1U) {
            setVector3(&v2C, -200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x45, &v2C);
        }
        if (em_after_frame_check(self, 0, 970.0f, 0.0f) == 1U) {
            setVector3(&v2C, -200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, 1000.0f, 0.0f) == 1U) {
            setVector3(&v14, -800.0f, 0.0f, -1000.0f);
            eft_em_spawn(self, 0x8A, 0x25, &v14, 1.0f);
            setVector3(&v2C, -200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, 1040.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x74, 0x2D, &v14, 1.0f, 0);
        }
        break;
    case 0xE:
        if (em_after_frame_check(self, 0, 664.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1200.0f);
            eft_em_spawn_param(self, 0xAD, 0x14, &v14, 1.0f, 0);
        }
        break;
    case 0x11:
        setVector3(&v14, 0.0f, 0.0f, 1500.0f);
        em025_joint_point_get(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 0x25, &v20);
        if ((v2C.y > self->field_0x20C) && (v20.y < self->field_0x20C) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, 700.0f, 0.0f, 3100.0f);
            eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF556);
            setVector3(&v14, -700.0f, 0.0f, 3100.0f);
            eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xAAB);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 380.0f, 0.0f, 800.0f);
            eft_em_spawn_param(self, 0x71, 0x19, &v14, 1.0f, 0);
            setVector3(&v14, -380.0f, 0.0f, 800.0f);
            eft_em_spawn_param(self, 0x71, 0x19, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1500.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        break;
    case 0x12:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1U) {
            setVector3(&v2C, 0.0f, 0.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 9, &v2C);
        }
        break;
    case 0x13:
        if (em_after_frame_check(self, 3, 20.0f, 300.0f) == 1U) {
            if (em025_act_bit_ck(self, 0) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 5500.0f);
                em025_joint_point_get(self, 0x14, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 2500.0f);
                    eft_em_spawn(self, 0x7E, 0x14, &v14, 1.0f);
                    em025_act_bit_set(self, 0);
                    setVector3(&v2C, -1500.0f, 0.0f, 0.0f);
                    camera_enemy_focus_pos_set(self, 5, &v2C);
                }
            }
            if (em025_act_bit_ck(self, 3) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x14, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                    eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
                    em025_act_bit_set(self, 3);
                    setVector3(&v2C, -1500.0f, 0.0f, 0.0f);
                    camera_enemy_focus_pos_set(self, 5, &v2C);
                }
            }
            if (em025_act_bit_ck(self, 4) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x25, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 1500.0f);
                    eft_em_spawn(self, 0x8A, 0x25, &v14, 1.0f);
                    em025_act_bit_set(self, 4);
                }
            }
        }
        if (em_after_frame_check(self, 0, 400.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0xA0, 0x14, &v14, 1.0f);
            em_camera_req(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 3, 800.0f, 1200.0f) == 1U) {
            if (em025_act_bit_ck(self, 5) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, 1.0f, 0xF8E5);
                    setVector3(&v14, -1000.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, 1.0f, 0x71C);
                    em025_act_bit_set(self, 5);
                }
            }
            if (em025_act_bit_ck(self, 6) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 800.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF8E5);
                    setVector3(&v14, -800.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0x71C);
                    em025_act_bit_set(self, 6);
                }
            }
            if (em025_act_bit_ck(self, 7) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 1000.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0xF8E5);
                    setVector3(&v14, -1000.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0x71C);
                    em025_act_bit_set(self, 7);
                }
            }
            if (em025_act_bit_ck(self, 8) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x13, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 700.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 0x13, &v14, 1.0f, 0xF8E5);
                    setVector3(&v14, -700.0f, 0.0f, 1000.0f);
                    eft_em_spawn_param(self, 0x70, 0x13, &v14, 1.0f, 0x71C);
                    em025_act_bit_set(self, 8);
                }
            }
            if (em025_act_bit_ck(self, 9) == 0) {
                setVector3(&v14, 0.0f, 0.0f, 1500.0f);
                em025_joint_point_get(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 1500.0f);
                    eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
                    em025_act_bit_set(self, 9);
                }
            }
        }
        break;
    case 0x14:
    case 0x16:
        if ((s32) (system_w.field_0x0c & 3) == 0) {
            if (em_after_frame_check(self, 3, 10.0f, 100.0f) == 1U) {
                em025_joint_quake(self, 0x12, 7, 1);
            }
            if (em_after_frame_check(self, 3, 530.0f, 630.0f) == 1U) {
                em025_joint_quake(self, 0x12, 7, 1);
            }
            if (em_after_frame_check(self, 3, 1140.0f, 1240.0f) == 1U) {
                em025_joint_quake(self, 0x12, 7, 1);
            }
        }
        if (em_after_frame_check(self, 0, 174.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 0xF, 0, 1.0f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, 802.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 9, 0, 1.0f);
            em025_joint_quake(self, 9, 7, 0);
        }
        break;
    case 0x17:
        if (em_after_frame_check(self, 0, 260.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, -200.0f, 3500.0f);
            eft_em_spawn_param(self, 0x9F, 2, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 556.0f, 0.0f) == 1U) {
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 590.0f, 0.0f) == 1U) {
            em025_joint_quake(self, 9, 7, 0);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, 616.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 9, 0, 4.5f);
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 620.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 0xF, 0, 4.5f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, 640.0f, 0.0f) == 1U) {
            em025_joint_quake(self, 0x14, 5, 3);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 0, 520.0f, 0.0f) == 1U) {
            em025_joint_quake(self, 0x12, 9, 2);
        }
        break;
    case 0x1A:
        if ((s32) (system_w.field_0x0c & 3) == 0) {
            if (em_after_frame_check(self, 3, 2.0f, 42.0f) == 1U) {
                em025_joint_quake(self, 9, 7, 0);
            }
            if (em_after_frame_check(self, 3, 100.0f, 160.0f) == 1U) {
                em025_joint_quake(self, 0xF, 7, 0);
            }
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 9, 0, 1.0f);
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 0xF, 0, 1.0f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        break;
    case 0x1C:
        if (em_after_frame_check(self, 0, 320.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0x19, 0, 2.5f);
        }
        break;
    case 0x1D:
        if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 2000.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1U) {
            setVector3(&v14, 500.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xF1C8);
            setVector3(&v14, -500.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, 1.0f, 0xE39);
        }
        if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
            setVector3(&v14, 650.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 0x13, &v14, 1.0f, 0xF1C8);
            setVector3(&v14, -650.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 0x13, &v14, 1.0f, 0xE39);
        }
        if (em_after_frame_check(self, 0, 188.0f, 0.0f) == 1U) {
            setVector3(&v14, 800.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0xF1C8);
            setVector3(&v14, -800.0f, 0.0f, 1500.0f);
            eft_em_spawn_param(self, 0x70, 5, &v14, 1.0f, 0xE39);
        }
        get_joint_wpos_em(self, 0x14, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 4, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, 1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            setVector3(&v14, -1200.0f, 0.0f, 800.0f);
            eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
        }
        break;
    case 0x1E:
        if (em_act_ck(self, 0xD, 3) == 0) {
            if (em_after_frame_check(self, 0, 420.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 9, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 540.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 0xF, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 596.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 9, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 700.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 0xF, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 600.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 9, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 800.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xA3, 0xF, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 552.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                eft_em_spawn(self, 0xB8, 0x14, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 640.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 500.0f);
                eft_em_spawn(self, 0xB8, 0x14, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 780.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0xB8, 0x14, &v14, 1.0f);
            }
            if (em_after_frame_check(self, 0, 866.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_param(self, 0x74, 3, &v14, 1.0f, 0);
            }
            if (em_after_frame_check(self, 0, 936.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_param(self, 0x74, 0x24, &v14, 1.0f, 0);
            }
            if (em_after_frame_check(self, 0, 1080.0f, 0.0f) == 1U) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x7E, 0x2D, &v14, 1.0f);
            }
        }
        break;
    case 0x1F:
        if (em_after_frame_check(self, 2, 550.0f, 0.0f) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            if ((v2C.y < self->field_0x20C) && (v20.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, 800.0f, 0.0f, 2500.0f);
                eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xF4A0);
                setVector3(&v14, -800.0f, 0.0f, 2500.0f);
                eft_em_spawn_param(self, 0x70, 3, &v14, 1.0f, 0xB61);
            }
        }
        if (em_after_frame_check(self, 0, 480.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1500.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 600.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 1300.0f);
            eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 610.0f, 0.0f) == 1U) {
            setVector3(&v2C, -200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 6, &v2C);
        }
        if (em_after_frame_check(self, 0, 660.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 710.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 400.0f);
            eft_em_spawn(self, 0x7E, 0x2A, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 920.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, -500.0f, 800.0f);
            eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
        }
        if (em_after_frame_check(self, 0, 940.0f, 0.0f) == 1U) {
            setVector3(&v2C, 200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x45, &v2C);
        }
        if (em_after_frame_check(self, 0, 970.0f, 0.0f) == 1U) {
            setVector3(&v2C, 200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, 1000.0f, 0.0f) == 1U) {
            setVector3(&v14, 800.0f, 0.0f, -1000.0f);
            eft_em_spawn(self, 0x8A, 0x25, &v14, 1.0f);
            setVector3(&v2C, 200.0f, 200.0f, 0.0f);
            camera_enemy_focus_pos_set(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, 1040.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x74, 0x2D, &v14, 1.0f, 0);
        }
        break;
    case 0x65:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 1000.0f)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        if (em_after_frame_check(self, 3, 2.0f, 400.0f) == 1U) {
            if (em025_act_bit_ck(self, 1) == 0) {
                get_joint_wpos_em(self, 9, &v2C);
                if (v2C.y > self->field_0x20C) {
                    em025_act_bit_set(self, 1);
                }
            } else if (em025_act_bit_ck(self, 2) == 0) {
                get_joint_wpos_em(self, 9, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x9D, 9, &v14, 1.0f);
                    em025_act_bit_set(self, 2);
                }
            }
            if (em025_act_bit_ck(self, 3) == 0) {
                get_joint_wpos_em(self, 0xF, &v2C);
                if (v2C.y > self->field_0x20C) {
                    em025_act_bit_set(self, 3);
                }
            } else if (em025_act_bit_ck(self, 4) == 0) {
                get_joint_wpos_em(self, 0xF, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x9D, 0xF, &v14, 1.0f);
                    em025_act_bit_set(self, 4);
                }
            }
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 9, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 234.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 9, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 0, 346.0f, 0.0f) == 1U) {
            setVector3(&v14, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, 1.0f, 0);
        }
        if (em_after_frame_check(self, 3, 300.0f, 640.0f) == 1U) {
            if (em025_act_bit_ck(self, 0) == 0) {
                setVector3(&v14, 0.0f, -500.0f, 0.0f);
                em025_joint_point_get(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x8A, 0x19, &v14, 1.0f);
                    em025_act_bit_set(self, 0);
                    setVector3(&v2C, 500.0f, 0.0f, -2000.0f);
                    camera_enemy_focus_pos_set(self, 5, &v2C);
                }
            }
            if (em025_act_bit_ck(self, 1) == 0) {
                get_joint_wpos_em(self, 4, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x8A, 4, &v14, 1.0f);
                    em025_act_bit_set(self, 1);
                }
            }
            if (em025_act_bit_ck(self, 2) == 0) {
                get_joint_wpos_em(self, 0x26, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x8A, 0x26, &v14, 1.0f);
                    em025_act_bit_set(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, 620.0f, 640.0f) == 1U) && (em025_act_bit_ck(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x7E, 0x2D, &v14, 1.0f);
                em025_act_bit_set(self, 3);
            }
        }
        break;
    case 0x67:
        if (em_act_ck(self, 0xD, 2) == 0) {
            if ((em_after_frame_check(self, 3, 200.0f, 320.0f) == 1U) && (em025_act_bit_ck(self, 2) == 0)) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 9, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x9D, 9, &v14, 1.0f);
                    em025_act_bit_set(self, 2);
                }
            }
            if ((em_after_frame_check(self, 3, 300.0f, 400.0f) == 1U) && (em025_act_bit_ck(self, 0) == 0)) {
                setVector3(&v14, 500.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 600.0f);
                    eft_em_spawn(self, 0x8A, 0x14, &v14, 1.0f);
                    em025_act_bit_set(self, 0);
                    setVector3(&v2C, -1000.0f, 0.0f, -700.0f);
                    camera_enemy_focus_pos_set(self, 5, &v2C);
                }
            }
            if ((em_after_frame_check(self, 3, 520.0f, 620.0f) == 1U) && (em025_act_bit_ck(self, 1) == 0)) {
                setVector3(&v14, 0.0f, 0.0f, 0.0f);
                em025_joint_point_get(self, 0x2D, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, 0.0f, 0.0f, 0.0f);
                    eft_em_spawn(self, 0x7E, 0x2D, &v14, 1.0f);
                    em025_act_bit_set(self, 1);
                }
            }
        }
        break;
    case 0x68:
        if (em_after_frame_check(self, 0, 202.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 9, 0, 4.5f);
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 468.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 0xF, 0, 4.5f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        break;
    case 0x69:
        if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0xF, 0, 1.0f);
            em025_joint_quake(self, 0xF, 1, 0);
        }
        if (em_after_frame_check(self, 0, 252.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0x14, 0, 2.5f);
            em025_joint_quake(self, 0xF, 1, 3);
        }
        if (em_after_frame_check(self, 0, 424.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 9, 0, 4.0f);
        }
        break;
    case 0x6A:
        if (em_after_frame_check(self, 0, 230.0f, 0.0f) == 1U) {
            em_hit_window_set_default(self, 0, 0x24);
        }
        if ((em_after_frame_check(self, 3, 2.0f, 240.0f) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            em025_joint_quake(self, 0x19, 2, 0);
        }
        if (em_after_frame_check(self, 0, 282.0f, 0.0f) == 1U) {
            em025_joint_quake(self, 0x12, 9, 3);
        }
        if (em_after_frame_check(self, 0, 340.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 4, 9, 0, 1.0f);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 160.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0xF, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 252.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0x14, 0, 2.5f);
            em025_joint_quake(self, 0x14, 5, 3);
        }
        if (em_after_frame_check(self, 0, 424.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 9, 0, 4.0f);
        }
        break;
    case 0x6E:
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 0xF, 0, 1.0f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 7, 9, 0, 1.0f);
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 288.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 4, 0x19, 0, 2.5f);
            em025_joint_quake(self, 0x19, 5, 3);
        }
        break;
    case 0x6F:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 500.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        if ((em_after_frame_check(self, 3, 8.0f, 50.0f) == 1U) && (em025_act_bit_ck(self, 0) == 0)) {
            setVector3(&v14, 0.0f, -900.0f, 800.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            if (v2C.y > self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                eft_em_spawn(self, 0xA1, 0x19, &v14, 1.0f);
                em025_act_bit_set(self, 0);
            }
        }
        if ((em_after_frame_check(self, 3, 180.0f, 220.0f) == 1U) && (em025_act_bit_ck(self, 1) == 0)) {
            setVector3(&v14, 0.0f, -900.0f, 800.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                eft_em_spawn(self, 0xA1, 0x19, &v14, 1.0f);
                em025_act_bit_set(self, 1);
            }
        }
        if ((em_after_frame_check(self, 3, 420.0f, 460.0f) == 1U) && (em025_act_bit_ck(self, 2) == 0)) {
            setVector3(&v14, 0.0f, -900.0f, 800.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                eft_em_spawn(self, 0xA1, 0x19, &v14, 1.0f);
                em025_act_bit_set(self, 2);
            }
        }
        if ((em_after_frame_check(self, 3, 670.0f, 710.0f) == 1U) && (em025_act_bit_ck(self, 3) == 0)) {
            setVector3(&v14, 0.0f, -900.0f, 800.0f);
            em025_joint_point_get(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, 0.0f, 0.0f, 1000.0f);
                eft_em_spawn(self, 0xA1, 0x19, &v14, 1.0f);
                em025_act_bit_set(self, 3);
            }
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 0xF, 0, 4.5f);
        }
        if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 0, 2, 9, 0, 4.5f);
        }
        if (em_after_frame_check(self, 0, 232.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 9, 0, 0.8f);
            em025_joint_quake(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, 254.0f, 0.0f) == 1U) {
            em025_effect_spawn(self, 1, 1, 0xF, 0, 0.8f);
            em025_joint_quake(self, 0xF, 7, 0);
        }
        break;
    case 0x72:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - 500.0f)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, 1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, -1200.0f, 0.0f, 800.0f);
                eft_em_spawn(self, 0x5B, 4, &v14, 1.0f);
            }
        }
        break;
    }
    if ((u8) self->action != 0xB) {
        EmDataEntry* e;
        EmDataEntry* table = ((_ENEMY_DATA_VIEW*) get_enemy_data(self))->table_0x2C;
        s32 i = 0;
        do {
            if (blk->handles_0x328[i] != -1) {
                u8 idx = blk->states_0x338[i];
                if (idx != 0xFF) {
                    e = &table[idx];
                    get_joint_wmat_em(self, e->joint, &mtx);
                    copyVec3(&v2C, (const nw4r::math::VEC3*)em_vec3_copy_c1(&rec8, &e->vec));
                    mulVecMatAddTrans(&v2C, &mtx);
                    em_roster_record_pos_set(blk->handles_0x328[i], &v2C, self->area_no);
                }
            }
            i++;
        } while (i < 4);
    }
}

/* The model-effect refresh: on map 6 in area 1 copy the scene model's effect matrix (Y row wrapped into
 * [0,1)), then push the material colour (alpha scaled by `field_0x1D4`). */
void em025_model_effect_update(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*) self->field_0x13C, 6);
        if (copied_mat_access_valid_ck(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            u32 handle = access.GetResTexSrt(false);
            res_tex_srt_copy_ctor(&srt, handle);
            srt.GetEffectMtx(0, &mtx);
            mtx.m[0][3] -= 0.1f;
            if (mtx.m[0][3] < 0.0f) {
                mtx.m[0][3] += 1.0f;
            }
            srt.SetEffectMtx(0, &mtx);
        }
        if (((MHchar*) &self->char_0x024)->getMatColor(6, GX_COLOR0A0, &color) == 1U) {
            color.a = (u8)(255.0f * self->field_0x1D4);
            ((MHchar*) &self->char_0x024)->setMatColor(6, GX_COLOR0A0, color, false);
        }
    } else {
        if (((MHchar*) &self->char_0x024)->getMatColor(6, GX_COLOR0A0, &color) == 1U) {
            color.a = 0;
            ((MHchar*) &self->char_0x024)->setMatColor(6, GX_COLOR0A0, color, false);
        }
    }
}

/* The per-kind predicate the action code polls: 0/1 for kinds 0/1/4/5, a 0..3 band for kinds 2/3 (an `int`
 * return; the switch narrows the argument once). */
int em025_state_ck(_ENEMY_WORK* self, u32 kind) {
    u8 narrowed = (u8) kind;
    switch (narrowed) {
    case 0:
        return (self->field_0x1E4 & 1) != 0;
    case 1:
        return self->em025_0x328.flag_0x353 >= 1;
    case 2:
        if (self->field_0x1AC >= 630.0f) {
            return 0;
        }
        if (self->field_0x1AC > 0.0f) {
            return 1;
        }
        if (self->field_0x1AC > -1300.0f) {
            return 2;
        }
        return 3;
    case 3:
        if (self->pos.x > 6500.0f) {
            return 2;
        }
        return (u8)(self->pos.x > 5380.0f) & 0xFF;
    case 4:
        return (u8)(quest_flag_200000_ck(0) == 1) & 0xFF;
    case 5:
        return self->field_0x358 != 0;
    default:
        return 0;
    }
}

/* The 4-byte thunk into the per-joint slot release; forwards `self` unchanged. */
void em025_slots_release_hook(_ENEMY_WORK* self) {
    em025_slots_release(self);
}

/* Clears the two bytes and resets the position and the three rotation words. */
void em025_area_start_set(_ENEMY_WORK* self, u8* a, u8* b) {
    em_move_mode_set(self, 0);
    *a = 0;
    *b = 0;
    setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
    self->field_0x1BC = 0;
    self->field_0x1C0 = 0;
    self->field_0x1C4 = 0;
}

/* r3 the work record; the per-action entry: resets the joint-effect slots on the request, ticks the
 * motion, and walks the two live slots through `em_roster_slot_effect_set`. */
void em025_leave(struct _ENEMY_WORK* self) {
    Em025Block* blk = &self->em025_0x328;

    if (em_busy_ck(self) == 1U) {
        lb_area_change_send((u8)my_player_no(), 0);
    }
    fn_8013A9F4(self);
    if ((s32)self->area_no == 2) {
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 0, 0);
        em_height_set(self, 0.0f);
        self->field_0x7B0 = -1.0f;
    }
    em025_slots_release(self);
    em_roster_slot_effect_set(self, 3, blk->handles_0x328, 0, 0);
    if (blk->handles_0x328[0] != -1) {
        blk->states_0x338[0] = 8;
    }
}

/* r3 the work record, r4 the part kind (a byte); the "this part is attackable" predicate: the part
 * must be 2 or 3, its damage level must not be odd, and the per-part flag must be clear. */
s32 em025_part_level_ck(struct _ENEMY_WORK* self, u8 kind) {
    if ((u32)(kind - 2) <= 1U && (em_parts_damage_level_get(self, kind) & 1) == 0 &&
        self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

/* r3 the work record, r4 the part kind; raises the per-part damage flag the kind maps to.  Kinds 6
 * and 7 only raise theirs at damage level 2 or above. */
void em025_part_drop(struct _ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        self->flags_0x836 |= 0x1000;
        break;
    case 1:
        self->flags_0x836 |= 0x2000;
        break;
    case 6:
        if (em_parts_damage_level_get(self, 6) >= 2U) {
            self->flags_0x836 |= 0x8000;
        }
        break;
    case 7:
        if (em_parts_damage_level_get(self, 7) >= 2U) {
            self->flags_0x836 |= 0x4000;
        }
        break;
    }
}

/* r3 the work record; asks the area table for action 13's slot, 2 in area 1 and 3 otherwise. */
void em025_area_request(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        fn_80128AEC(self, 13, 2);
        return;
    }
    fn_80128AEC(self, 13, 3);
}

/* 0x801A9748 (0x1B0): steps the sway curve position by its blend mode (easing out in action 10, holding near 0.5
 * for 180 frames, easing back in) and turns the curve into the joints' X rotation. */
void em025_sway_update(struct _ENEMY_WORK* self) {
    f32 angle;

    if (self->action == 10) {
        self->em025_0x328.blend_mode_0x352 = 0;
    }
    switch (self->em025_0x328.blend_mode_0x352) {
    case 0:
        if (self->em025_0x328.blend_0x34C <= 0.0f) {
            self->em025_0x328.blend_0x34C = 0.0f;
        } else {
            self->em025_0x328.blend_0x34C -= 0.05f;
        }
        if (self->action != 10) {
            self->em025_0x328.blend_mode_0x352 = 2;
        }
        break;
    case 1:
        if (self->em025_0x328.blend_0x34C >= 0.475f && self->em025_0x328.blend_0x34C <= 0.525f) {
            self->em025_0x328.blend_0x34C = 0.5f;
        } else if (self->em025_0x328.blend_0x34C < 0.475f) {
            self->em025_0x328.blend_0x34C += 0.025f;
        } else if (self->em025_0x328.blend_0x34C > 0.525f) {
            self->em025_0x328.blend_0x34C -= 0.025f;
        }
        if (self->em025_0x328.hold_0x350 >= 180) {
            self->em025_0x328.blend_mode_0x352 = 2;
            self->em025_0x328.hold_0x350 = 0;
        } else {
            self->em025_0x328.hold_0x350++;
        }
        break;
    case 2:
        if (self->em025_0x328.blend_0x34C >= 1.0f) {
            self->em025_0x328.blend_0x34C = 1.0f;
            self->em025_0x328.blend_mode_0x352 = 1;
            self->em025_0x328.hold_0x350 = 0;
        } else {
            self->em025_0x328.blend_0x34C += 0.025f;
        }
        break;
    }
    curve_key_eval_c1(em025_sway_keys, self->em025_0x328.blend_0x34C, &self->em025_0x328.scale_0x348, &angle);
    self->em025_0x328.rot_x_0x344 = (u16)(s32)(0.5f + (65536.0f * angle) / 360.0f);
}

/* 0x801A98F8 (0x374): the head tracking: turns joints 23 and 24 toward the nearest player on their side who is in
 * the area, within 600 and at the joint's height, inside each joint's angle limits. */
void em025_head_track(struct _ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 head;
    nw4r::math::VEC3 target;
    Em025Block* blk;
    u32 base;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&head);
    VEC3_ctor(&target);
    u32 joints[2] = {23, 24};
    f32 limits[2][2] = {{10.0f, -15.0f}, {15.0f, -10.0f}};
    get_joint_wmat_em(self, 20, &mtx);
    base = calcMtxAng3(&mtx);
    for (i = 0; i < 2; i++) {
        _PLW* pl;
        u32 count;
        s32 j;
        f32 dist;
        f32 range;
        f32 best;
        u32 turn;

        get_joint_wmat_em(self, joints[i], &mtx);
        setVector3(&head, mtx.m[0][3], mtx.m[1][3], mtx.m[2][3]);
        count = get_move_work_max(2);
        pl = (_PLW*)get_move_work_adrs(2);
        best = -1.0f;
        for (j = 0; j < (u16)count; j++, pl++) {
            if (pl->slot_active != 0 && em_pl_slot_leaving_ck(pl->chunk_ofs) == 0 && self->area_no == pl->area_0x16) {
                if (best < 0.0f) {
                    range = 600.0f;
                } else {
                    range = best;
                }
                dist = calcVecDistXZ(&head, &pl->vec_0x03C);
                if (dist < range && pl->motion_pos_0x40 <= head.y + 100.0f && pl->motion_pos_0x40 >= head.y - 100.0f) {
                    s32 rel = calcVecAng2(&head, &pl->vec_0x03C) - base;
                    if (i == 0) {
                        if ((u16)rel <= 0x8000) {
                            copyVec3(&target, &pl->vec_0x03C);
                            best = dist;
                        }
                    } else if ((u16)rel >= 0x8000) {
                        copyVec3(&target, &pl->vec_0x03C);
                        best = dist;
                    }
                }
            }
        }
        if (best > 0.0f) {
            u32 rel = calcVecAng2(&head, &target) - base;
            turn = (u16)(rel + 0x4000);
            if (i == 0) {
                turn = (u16)(rel + 0xC000);
            }
            if (turn <= 0x8000) {
                if (turn > (u16)(s32)(0.5f + (65536.0f * limits[i][0]) / 360.0f)) {
                    turn = (u16)(s32)(0.5f + (65536.0f * limits[i][0]) / 360.0f);
                }
            } else if (turn < (u16)(s32)(0.5f + (65536.0f * limits[i][1]) / 360.0f)) {
                turn = (u16)(s32)(0.5f + (65536.0f * limits[i][1]) / 360.0f);
            }
        } else {
            turn = 0;
        }
        blk = &self->em025_0x328;
        blk->rot_y_0x33C[i] = em_angle_approach(turn, blk->rot_y_0x33C[i], 0x80);
    }
}

} /* extern "C" */

/* 0x801A9C6C (0x40): Sets the accessor's flag word to 3 once it is installed. */
void Em025UserData::Attach() {
    nw4r::g3d::ResNode sp8((void*)0);
    em_res_user_data_flag_set((struct _ENEMY_WORK*)this, 3);
}

/* 0x801A9CAC (0x148): The head tracking's Y rotations on nodes 23/24 and the sway scale and X rotation on nodes
 * 28/29, for an item of kind 0xFF. */
void Em025UserData::CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 joint, UserDataItem* item) {
    struct _ENEMY_WORK* work = this->work;
    nw4r::math::MTX34 mtx;

    MTX34_ctor(&mtx);
    if ((s32)item->field_0x04 == 0xFF) {
        switch (joint) {
        case 23:
            joint_mtx_load((struct MtxHolder*)pManip, &mtx);
            rotMatrixY(work->em025_0x328.rot_y_0x33C[0], &mtx);
            joint_mtx_store((struct MtxHolder*)pManip, &mtx);
            break;
        case 24:
            joint_mtx_load((struct MtxHolder*)pManip, &mtx);
            rotMatrixY(work->em025_0x328.rot_y_0x33C[1], &mtx);
            joint_mtx_store((struct MtxHolder*)pManip, &mtx);
            break;
        case 28:
            world_mtx_attr_scale_set((struct VtxAttrRef*)pManip, 1.0f, work->em025_0x328.scale_0x348, 1.0f);
            joint_mtx_load((struct MtxHolder*)pManip, &mtx);
            rotMatrixX(work->em025_0x328.rot_x_0x344, &mtx);
            joint_mtx_store((struct MtxHolder*)pManip, &mtx);
            break;
        case 29:
            world_mtx_attr_scale_set((struct VtxAttrRef*)pManip, 1.0f, work->em025_0x328.scale_0x348, 1.0f);
            joint_mtx_load((struct MtxHolder*)pManip, &mtx);
            rotMatrixX(work->em025_0x328.rot_x_0x344, &mtx);
            joint_mtx_store((struct MtxHolder*)pManip, &mtx);
            break;
        }
    }
}

extern "C" {

/* r3 the work record; releases the joint-effect slots and re-arms the two it owns. */
void em025_effect_slots_reset(struct _ENEMY_WORK* self) {
    s32* slot = self->handles_0x328;
    u16 i;

    em025_slots_release(self);
    for (i = 0; i < 2; i++) {
        em_roster_slot_effect_set(self, i, slot + i, 5, 0);
    }
}

/* 0x801A9E60 (0x298): the shore action's target test: 1 while the shore rise or sweep runs inside one of the
 * byte-stack windows, 2 for the late shore charge on motion 8, else 0. */
u32 em025_shore_target_ck(struct _ENEMY_WORK* self) {
    if (self == NULL) {
        return 0;
    }
    if (self->team != 25) {
        return 0;
    }
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        if (quest_flag_200000_ck(0) == 1) {
            if (em025_byte_stack_range_ck(self, 3, 10, 18) == 1) {
                if (em_act_ck_c1(self, 6, 15) == 1 || em_act_ck_c1(self, 6, 23) == 1) {
                    return 1;
                }
            } else if (em025_byte_stack_range_ck(self, 3, 35, 39) == 1) {
                if (em_act_ck_c1(self, 6, 15) == 1 || em_act_ck_c1(self, 6, 23) == 1) {
                    return 1;
                }
            } else if (em025_byte_stack_range_ck(self, 3, 44, 51) == 1) {
                if (em_act_ck_c1(self, 6, 15) == 1 || em_act_ck_c1(self, 6, 23) == 1) {
                    return 1;
                }
            } else if (em025_byte_stack_range_ck(self, 3, 63, 71) == 1) {
                if (em_act_ck_c1(self, 6, 15) == 1 || em_act_ck_c1(self, 6, 23) == 1) {
                    return 1;
                }
            }
        } else if (em025_byte_stack_range_ck(self, 3, 11, 18) == 1) {
            if (em_act_ck_c1(self, 6, 15) == 1) {
                return 1;
            }
        } else if (em025_byte_stack_range_ck(self, 3, 73, 82) == 1) {
            if (em_act_ck_c1(self, 6, 15) == 1) {
                return 1;
            }
        } else if (em_act_ck_c1(self, 6, 14) == 1 && em_get_mot_no(self) == 8 && em_frame_check(self, 3, 0.0f, 290.0f) == 1) {
            return 2;
        }
    }
    return 0;
}

}

/* 0x801AA0F8 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em025UserData::~Em025UserData() {
}
