/*
 * enemy/em003_prog.cpp - enemy 003's program: the `em003_prog_tbl` entry points (work clear, init, frame update,
 *   action run, motion hand-over, event hook, motion events, TEV colour update, state query, death action, call
 *   reset, part guard, idle test, area-start test), the per-action dispatchers and their state steps, the
 *   reinforcement-call logic (call group/kind picks over the area table and the move works), the sound programs of
 *   actions 10 and 11, the arena demo, the shared effect spawn and the user-data record methods; MWCC's
 *   `__sinit_\em003_prog_cpp` builds the three `.bss` vector pairs.
 * RANGE. .text 0x80154E40-0x8015D860 (108 functions); extab 0x8000DCFC-0x8000DFA4, extabindex
 *   0x80028A64-0x80028E60, .ctors 0x8056F324-0x8056F328 (`__sinit_\em003_prog_cpp`), .rodata
 *   0x8056FA38-0x8056FB50 (the three turn tables and two fly tables), .data 0x805A44A8-0x805A5D48 (`em003_prog_tbl` first), .bss
 *   0x806A7820-0x806A7868 (`em003_se_shift_a/_b/_c`), .sdata 0x80791A38-0x80791A58, .sdata2 0x807970C0-0x80797330.
 * FLAGS. `cflags_main`; file-wide `#pragma fp_contract off` (no fused multiply-add in the target), `#pragma
 *   pool_data off` (each table is addressed with its own `lis`/`addi`) and `#pragma peephole off` (retail keeps the
 *   unfused `clrlwi`/`rlwinm` + `cmpwi` pairs), with `#pragma peephole on` around `em003_frame_update` and
 *   `em003_act7_7`; `#pragma opt_common_subs off` around `em003_act4_10`, `em003_act7_2`, `em003_act7_10`,
 *   `em003_act7_13`, `em003_act7_19` and `em003_act7_20` (retail loads `shell_set_func_ptr` after the arguments).
 * NAMES. `em003_prog_tbl`, `em003_call_act_type_ck`, `em003_atama_koware_act_ck` and `em003_yobi_boss_em_ck` are the
 *   runtime dump's names; every other row is named here.
 *   GUESS (the program-table slot each fills): em003_timers_clear, em003_init, em003_frame_update,
 *   em003_action_run, em003_motion_next, em003_event_notify, em003_motion_events, em003_tev_color_update,
 *   em003_state_query, em003_death_action_get, em003_call_state_clear, em003_guard_ck, em003_idle_ck,
 *   em003_area_start_ck.
 *   GUESS (dispatcher slot): em003_act<action>_<first sub-state that runs it>, em003_act<N>_dispatch,
 *   em003_act2_slide, em003_act10_se_play, em003_act11_se_play; GUESS (from the bodies): em003_finish_act3,
 *   em003_skip_ck, em003_alt_mode_fx, em003_ground_impact_fx, em003_effect_spawn, em003_color_approach,
 *   em003_color_update, em003_spawn_rec_set, em003_call_group_pick, em003_call_kind_pick.
 *   GUESS (data): the key curves (em003_lift_keys ... em003_swipe_aim_keys, em003_glow_keys_<motion>,
 *   em003_demo_pos_keys/_rot_keys), em003_call_group_tbl/_kind_tbl, em003_area_pick_tbl,
 *   em003_turn_seq_left/_right/_fly, em003_fly_tbl_a/_b, em003_se_seq_<sub-state>/em003_se_alt_seq_<sub-state> with
 *   their em003_se_rec/_loop/_ofs records, em003_se_shift_a/_b/_c; the `Em003ProgTbl` slot names, `Em003VecPair`, the
 *   `em003_0x328` view's field names, `Em003UserData` and its `Attach` hook.
 *   The foreign callees em_team_member_claim, em_area_buff_apply, pl_area_buff_apply, eft_area_slot_bind,
 *   em003_call_se_reset and em003_call_se_step are this unit's registry proposals for 0x8012C220, 0x8012D618,
 *   0x8027D0D4, 0x80306B10, 0x8043B410 and 0x8043B424 (declared in their owners' leaf headers).
 * RESIDUALS. `em003_act7_3`: retail narrows the approach rate after the `0x80`/`0x40` merge (`clrlwi r5,r5,16`,
 *   4 bytes); a `(u16)` cast, u16/int locals, a switch, a u16-parameter view and an inline helper all fold it.
 *   Register allocation only: `em003_call_group_pick` (each loop has its own index/pair, retail colours them
 *   r28/r22 and r31/r28), `em003_color_update` (retail keeps `b` in r30, ours r28; every declaration order and
 *   reusing the first block's variables were measured).
 *   .sdata2: the target's 4-byte tail pad after `lbl_80797328` (620 of 624 bytes emitted).
 *   flipcheck: the rows above.
 * SHAPES. `Em003UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   A one-case compare retail emits as `cmpwi` on a u8 is a `switch` (its other path as `default:`, written
 *   first when retail lays it out first); a case-level `return` that retail lays out as a `break` costs a branch, so
 *   the state switches use `break`.  The swipe hit index is assigned after its last frame test (`else hit = -1`).
 *   The unsigned shift of the swipe angle byte goes through `(u32)(u16)`; the area-table loops count to 128 over
 *   `&em_area_entry_tbl[0][0]`; a `get_move_work_max` count is held as u32 and narrowed at the loop.  Data is
 *   defined between the functions in retail order (the act-7 aim curve before `em003_act7_0`, the act-10 and act-11
 *   sound programs before their players, the demo keys before `em003_act12_0`).  A swipe angle that depends on
 *   `arg1` is an `if`/`else` pair; `get_em_base_scale` goes into a local before
 *   `em_mot_speed_set`, and the blend ratio into a local before `em_motion_blend_step`.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em003_prog.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "ef.h"              /* VEC3_ctor */
#include "ef/fn_80105314.h"
#include "draw_shape.h"      /* draw_shape_arm */
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "MSL/new.h" /* the placement `operator new` */
#include "enemy/EnemyData.h"
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "ef/eft_slot.h"
#include "stage/stg_w.h"
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "enemy/em_se_record.h"  /* EmSeEntry */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play */
#include "enemy/em_se_tbl_play_alt.h"
#include "ef/eft009.h"          /* eft009_spawn_at_joint / eft009_set_pos / eft009_spawn_at_pos */
#include "enemy/em_move_vec2_clr.h" /* em_move_vec2_clr */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step */
#include "enemy/CancelFade.h" /* CancelFade */
#include "ef/fn_8010D1A8.h"     /* eft_spawn_pos_in_area */
#include "ef/cp_vector.h"       /* _CP_VECTOR */
#include "enemy/em_camera_req.h" /* em_camera_req */
#include "enemy/em_area_entry_tbl.h" /* em_area_entry_tbl */
#include "lobby/fn_8030121C.h" /* eft_em_spawn */
#include "enemy/em_model.h" /* em_drop_item_spawn */
#include "enemy/em_any_player_seen_ck.h"
#include "stage/stg_ground_impact_set.h" /* stg_area_ceiling_get */
#include "enemy/fn_8012EC74.h" /* get_em_chg_scale */
#include "Pl/hit_w.h" /* get_hit_id */
#include "ef/eft007.h" /* eft007_part_set */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "ef/vec3_div.h"
#include "ef/fn_800CDB2C.h" /* move_work_state_ck */
#include "menu/get_hit_id.h"
#include "enemy/em_timer_0x94A_ck.h"
#include "enemy/em_area_enter_ck.h"
#include "enemy/em_ground_impact_set.h"
#include "gx.h"
#include "sound/mhchar.h" /* MHchar, with the pointer-taking K-colour accessors */
#include "ef/get_move_work_adrs.h"
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "enemy/em_team_member_claim.h"
#include "enemy/em_area_buff_apply.h"
#include "Pl/pl_area_buff_apply.h"
#include "ef/eft_area_slot_bind.h"
#include "menu/em003_call_se.h"

/* Call-site views: each macro casts a callee to the signature its call sites use (the same direct call). */
/* the motion test is called with `self` still in r3 and no argument set up */
#define em_mot_finished_ck_view1 ((u32 (*)(void))em_mot_finished_ck)
/* the fall start takes the height `em_fall_height_get` leaves in f1 */
/* the user-data record methods are called on the record (the owners spell the parameter as the work record) */
#define em_res_user_data_flag_set_view1 ((void (*)(Em003UserData*, u32))em_res_user_data_flag_set)
/* the action test and the death test answer through `cmplwi` (u32) */
#define em_act_ck_view1 ((u32 (*)(_ENEMY_WORK*, u8, u8))em_act_ck)
#define em_die_ck_view1 ((u32 (*)(_ENEMY_WORK*))em_die_ck)
/* the static initialiser hands `setVec3`'s returned pointer straight on */
#define assignVec3_view1 ((void (*)(nw4r::math::VEC3*, VEC3*))assignVec3)
#define em_fall_start_f ((void (*)(_ENEMY_WORK*, f32))em_fall_start)
/* the offset step reports whether the move landed */
#define em_move_offset_step_update_v ((u32 (*)(_ENEMY_WORK*, void*))em_move_offset_step_update)


/* One entry of the call tables `em003_call_group_pick`/`em003_call_kind_pick` and the call event scan: a group
 * (or kind) id and the team it must carry (0xFF = any); a zero id ends the table.  size: 0x2 */
struct Em003CallPair {
    /* +0x0 */ u8 id;
    /* +0x1 */ u8 team;
};

/* The user-data accessor `em003_init` installs: the base accessor with its own destructor and attach hook (its
 * table is `.data` 0x805A5D18).  size: 0xC (`operator new(0xC)`) */
class Em003UserData : public ResUserDataAc {
public:
    Em003UserData();
    virtual ~Em003UserData();
    virtual void Attach();
};

extern "C" {
void em003_timers_clear(_ENEMY_WORK *self);
void em003_init(_ENEMY_WORK *self, u8 arg1);
void em003_motion_next(_ENEMY_WORK* self, u8* state, u8* sub);
void em003_event_notify(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void em003_frame_update(_ENEMY_WORK *self);
void em003_finish_act3(_ENEMY_WORK *self);
void em003_act0_0(_ENEMY_WORK *self);
void em003_act0_1(_ENEMY_WORK *self);
void em003_act0_2(_ENEMY_WORK *self);
void em003_act0_3(_ENEMY_WORK *self);
void em003_act0_6(_ENEMY_WORK *self);
void em003_act0_dispatch(_ENEMY_WORK *self);
void em003_act1_0(_ENEMY_WORK *self);
void em003_act1_1(_ENEMY_WORK *self);
void em003_act1_2(_ENEMY_WORK *self);
void em003_act1_3(_ENEMY_WORK *self);
void em003_act1_4(_ENEMY_WORK *self, u8 arg1);
void em003_act1_5(_ENEMY_WORK *self);
void em003_act1_6(_ENEMY_WORK *self);
void em003_act1_7(_ENEMY_WORK *self);
void em003_act1_8(_ENEMY_WORK *self, u8 arg1);
void em003_act1_9(_ENEMY_WORK *self, u8 arg1);
void em003_act1_10(_ENEMY_WORK *self, u8 arg1);
void em003_act1_18(_ENEMY_WORK *self);
void em003_act1_21(_ENEMY_WORK *self);
void em003_act1_22(_ENEMY_WORK *self);
void em003_act1_37(_ENEMY_WORK *self);
void em003_act1_dispatch(_ENEMY_WORK *self);
void em003_act2_0(_ENEMY_WORK *self, u8 arg1);
void em003_act2_1(_ENEMY_WORK *self);
void em003_act2_2(_ENEMY_WORK *self);
void em003_act2_3(_ENEMY_WORK *self, u8 arg1);
void em003_act2_slide(_ENEMY_WORK *self, u8 arg1);
void em003_act2_4(_ENEMY_WORK* self, u8 arg1);
void em003_act2_6(_ENEMY_WORK *self);
void em003_act2_7(_ENEMY_WORK *self);
void em003_act2_dispatch(_ENEMY_WORK *self);
void em003_act3_0(_ENEMY_WORK *self);
void em003_act3_1(_ENEMY_WORK *self);
void em003_act3_2(_ENEMY_WORK *self);
void em003_act3_3(_ENEMY_WORK *self);
void em003_act3_4(_ENEMY_WORK *self);
void em003_act3_5(_ENEMY_WORK *self, u8 arg1);
void em003_act3_6(_ENEMY_WORK *self, u8 arg1);
void em003_act3_7(_ENEMY_WORK *self);
void em003_act3_8(_ENEMY_WORK *self);
void em003_act3_10(_ENEMY_WORK *self);
void em003_act3_11(_ENEMY_WORK *self);
void em003_act3_12(_ENEMY_WORK *self);
void em003_act3_14(_ENEMY_WORK *self);
void em003_act3_dispatch(_ENEMY_WORK *self);
void em003_act4_0(_ENEMY_WORK *self);
void em003_act4_1(_ENEMY_WORK *self);
void em003_act4_2(_ENEMY_WORK *self);
void em003_act4_3(_ENEMY_WORK *self);
void em003_act4_4(_ENEMY_WORK *self);
void em003_act4_5(_ENEMY_WORK *self, u8 arg1);
void em003_act4_6(_ENEMY_WORK *self);
void em003_act4_7(_ENEMY_WORK *self);
void em003_act4_8(_ENEMY_WORK *self, u8 arg1);
void em003_act4_10(_ENEMY_WORK *self);
void em003_act4_dispatch(_ENEMY_WORK *self);
void em003_act7_0(_ENEMY_WORK *self);
void em003_act7_1(_ENEMY_WORK *self);
void em003_act7_2(_ENEMY_WORK *self, u8 arg1);
void em003_act7_3(_ENEMY_WORK *self, u8 arg1);
void em003_act7_7(_ENEMY_WORK *self);
void em003_act7_8(_ENEMY_WORK *self);
void em003_act7_9(_ENEMY_WORK *self, u8 arg1, u8 arg2, u8 arg3);
void em003_act7_10(_ENEMY_WORK *self, u8 arg1);
void em003_act7_12(_ENEMY_WORK *self);
void em003_act7_13(_ENEMY_WORK* self, u8 arg1, u8 arg2, u8 arg3);
void em003_act7_15(_ENEMY_WORK *self);
void em003_act7_19(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void em003_act7_20(_ENEMY_WORK *self, u8 arg1);
void em003_act7_dispatch(_ENEMY_WORK *self);
void em003_act10_se_play(_ENEMY_WORK *self);
void em003_act11_se_play(_ENEMY_WORK *self);
void em003_act12_0(_ENEMY_WORK* self);
void em003_act12_dispatch(_ENEMY_WORK *self);
void em003_act13_0(_ENEMY_WORK *self);
void em003_act13_1(_ENEMY_WORK *self);
void em003_act13_dispatch(_ENEMY_WORK *self);
void em003_action_run(_ENEMY_WORK *self);
void em003_alt_mode_fx(_ENEMY_WORK *self);
void em003_ground_impact_fx(_ENEMY_WORK *self, u8 arg1);
void em003_effect_spawn(_ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, s32 angle, f32 scale);
void em003_motion_events(_ENEMY_WORK *self);
u8 em003_color_approach(u8 self, u8 arg1, u8 arg2);
void em003_color_update(_ENEMY_WORK *self);
void em003_tev_color_update(_ENEMY_WORK *self);
u8 em003_state_query(_ENEMY_WORK *self, u8 arg1);
u32 em003_area_start_ck(_ENEMY_WORK* self, u16 arg1);
void em003_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub);
void em003_call_state_clear(_ENEMY_WORK *self);
s32 em003_guard_ck(_ENEMY_WORK *self, u8 arg1);
s32 em003_idle_ck(_ENEMY_WORK *self);
void em003_spawn_rec_set(EmSpawnRec* out, u8 kind, u16 angle_a, u16 angle_b);
u8 em003_call_group_pick(_ENEMY_WORK* self);
u8 em003_call_kind_pick(_ENEMY_WORK* self);
}
u8 em003_call_act_type_ck(_ENEMY_WORK* self);
u32 em003_atama_koware_act_ck(_ENEMY_WORK *self);
s32 em003_yobi_boss_em_ck(u8 self);


#pragma fp_contract off
#pragma pool_data off
#pragma peephole off

/* The em003 program table (`em003_prog_tbl`): the entry points the shared enemy program driver calls (slot names
 * follow `Em008ProgTbl`'s scheme, a GUESS from the functions they hold).
 * size: 0x70 */
struct Em003ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(_ENEMY_WORK* self, u8* state, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u8 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*aim_reset_0x34)(_ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[3];
    /* +0x44 */ s32 (*part_guard_ck_0x44)(_ENEMY_WORK* self, u8 part);
    /* +0x48 */ s32 (*idle_ck_0x48)(_ENEMY_WORK* self);
    /* +0x4C */ u32 pad_0x4C[2];
    /* +0x54 */ u32 (*area_start_ck_0x54)(_ENEMY_WORK* self, u16 arg);
    /* +0x58 */ u32 pad_0x58[6];
};

extern "C" Em003ProgTbl em003_prog_tbl = {
    em003_timers_clear,
    em003_init,
    em003_frame_update,
    em003_action_run,
    em003_motion_next,
    em003_event_notify,
    em003_motion_events,
    0,
    em003_tev_color_update,
    em003_state_query,
    {0, 0},
    em003_death_action_get,
    em003_call_state_clear,
    {0, 0, 0},
    em003_guard_ck,
    em003_idle_ck,
    {0, 0},
    em003_area_start_ck,
    {0, 0, 0, 0, 0, 0},
};

/* A point and its per-frame step, built by MWCC's `__sinit` (0x8015D764): each vector is set through `setVec3` into
 * a temporary and copied with `assignVec3`; the action-10 sound records point at the first vector.  size: 0x18 */
struct Em003VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, y, 0) and (0, dy, 0). */
    Em003VecPair(f32 y, f32 dy)
    {
        VEC3 b;
        VEC3 a;

        assignVec3_view1(&pos, setVec3(&a, 0.0f, y, 0.0f));
        assignVec3_view1(&step, setVec3(&b, 0.0f, dy, 0.0f));
    }
};

/* The three vector pairs (`.bss`, defined at the end). */
extern Em003VecPair em003_se_shift_a;
extern Em003VecPair em003_se_shift_b;
extern Em003VecPair em003_se_shift_c;

/* The `em_turn_seq_start`/`em_turn_seq_step` tables and the two `em_fly_start` tables (`.rodata`).  The names are a
 * GUESS from their users. */
extern "C" const u32 em003_turn_seq_left[16] = {
    0x00000032, 0x00000064, 0x0000006E, 0x40000E39,
    0x00060000, 0x00000004, 0x00000000, 0x00000000,
    0x000A0000, 0x00000004, 0x00000000, 0x00000000,
    0x000B0000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em003_turn_seq_right[16] = {
    0x00000000, 0x00000000, 0x00000000, 0x40001555,
    0x000E0000, 0x00000002, 0x00000000, 0x00000000,
    0x000C0000, 0x00000006, 0x00000000, 0x00000000,
    0x000D0000, 0x00000006, 0x00000000, 0x00000000,
};

extern "C" const u32 em003_turn_seq_fly[16] = {
    0x00000000, 0x0000003C, 0x00000000, 0x40000E39,
    0x001B0000, 0x00000006, 0x00000000, 0x00000000,
    0x00250000, 0x00000006, 0x00000000, 0x00000000,
    0x00260000, 0x00000006, 0x00000000, 0x00000000,
};

extern "C" const u32 em003_fly_tbl_a[11] = {
    0x001F0000, 0x00000004, 0x00000000, 0x00000000,
    0x00200000, 0x0000000A, 0x00000000, 0x00000001,
    0x00230024, 0x42F00000, 0x41F00000,
};

extern "C" const u32 em003_fly_tbl_b[11] = {
    0x00D00000, 0x00000006, 0x00000000, 0x00000000,
    0x001B0000, 0x00000006, 0x00000000, 0x00000000,
    0x00270028, 0x42C80000, 0x42480000,
};

/* The key curves (frame/value pairs, a -1 frame ends them) the action steps evaluate, the glow alpha curves of the
 * colour update, the call tables and the packed area-pick record (`.data`, in retail order).  Names are a GUESS
 * from their users. */
f32 em003_lift_keys[20] = {
    0.0f, 14.0f, 10.0f, 21.6f, 20.0f, 18.0f, 30.0f, 1.2f,
    50.0f, -1.2f, 70.0f, 6.6f, 90.0f, 9.0f, 100.0f, -1.2f,
    120.0f, 1.2f, -1.0f, 9.6f,
};

f32 em003_hop_height_keys[10] = {
    60.0f, -19.5f, 90.0f, 6.5f, 100.0f, 6.5f, 144.0f, 0.0f,
    -1.0f, 0.0f,
};

f32 em003_hop_move_keys[18] = {
    0.0f, 0.0f, 30.0f, -12.0f, 54.0f, 0.0f, 60.0f, 14.0f,
    70.0f, 30.0f, 90.0f, 18.0f, 100.0f, 6.0f, 144.0f, 0.0f,
    -1.0f, 0.0f,
};

f32 em003_back_step_keys[6] = {
    0.0f, 5.0f, 42.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_turn_step_keys[14] = {
    0.0f, 45.0f, 8.0f, 40.0f, 16.0f, 17.0f, 24.0f, -4.0f,
    32.0f, -10.0f, 48.0f, -9.5f, -1.0f, 0.0f,
};

f32 em003_swipe_move_keys[10] = {
    0.0f, -6.0f, 14.0f, -18.0f, 24.0f, -11.0f, 42.0f, -10.0f,
    -1.0f, -10.0f,
};

f32 em003_slide_move_keys[10] = {
    0.0f, -14.0f, 14.0f, -26.0f, 24.0f, -12.0f, 42.0f, -10.0f,
    -1.0f, -10.0f,
};

f32 em003_lunge_move_keys[14] = {
    0.0f, 0.0f, 1.0f, 18.0f, 8.0f, 9.0f, 18.0f, 7.0f,
    32.0f, 2.0f, 40.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_05[10] = {
    154.0f, 0.0f, 166.0f, 255.0f, 214.0f, 255.0f, 246.0f, 0.0f,
    -1.0f, 0.0f,
};

f32 em003_glow_keys_C8[8] = {
    0.0f, 0.0f, 10.0f, 255.0f, 180.0f, 255.0f, -1.0f, 255.0f,
};

f32 em003_glow_keys_C9[6] = {
    0.0f, 255.0f, 42.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_CA[6] = {
    0.0f, 255.0f, 218.0f, 255.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_CB[8] = {
    0.0f, 0.0f, 18.0f, 255.0f, 254.0f, 255.0f, -1.0f, 255.0f,
};

f32 em003_glow_keys_CC[8] = {
    0.0f, 255.0f, 148.0f, 255.0f, 186.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_CD[6] = {
    0.0f, 255.0f, 72.0f, 255.0f, -1.0f, 255.0f,
};

f32 em003_glow_keys_DA[12] = {
    0.0f, 0.0f, 56.0f, 0.0f, 64.0f, 255.0f, 184.0f, 255.0f,
    198.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_DC[6] = {
    0.0f, 255.0f, 46.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_glow_keys_E1[12] = {
    0.0f, 0.0f, 24.0f, 0.0f, 58.0f, 255.0f, 112.0f, 255.0f,
    150.0f, 0.0f, -1.0f, 0.0f,
};

f32 em003_swipe_speed_keys[10] = {
    0.0f, 1.0f, 18.0f, 0.9f, 70.0f, 0.9f, 90.0f, 1.0f,
    -1.0f, 1.0f,
};

Em003CallPair em003_call_group_tbl[8] = {
    {0x01, 0xFF}, {0x02, 0xFF}, {0x0C, 0xFF}, {0x08, 0xFF}, {0x10, 0xFF}, {0x06, 0xFF}, {0x07, 0xFF}, {0x00, 0xFF},
};

Em003CallPair em003_call_kind_tbl[6] = {
    {0x0A, 0x01}, {0x1F, 0xFF}, {0x11, 0xFF}, {0x1D, 0xFF}, {0x00, 0xFF}, {0x00, 0x00},
};

u32 em003_area_pick_tbl[3] = {0x0000001E, 0x00FA000A, 0xFFFF0000};

void em003_timers_clear(_ENEMY_WORK *self) {
    self->em003_0x328.kcolor1_r = 0xFF;
    self->em003_0x328.kcolor1_g = 0xFF;
    self->em003_0x328.kcolor1_b = 0xFF;
    self->em003_0x328.kcolor2_r = 0xFF;
    self->em003_0x328.kcolor2_g = 0xFF;
    self->em003_0x328.kcolor2_b = 0xFF;
    self->em003_0x328.kcolor2_a = 0;
    self->em003_0x328.kcolor3_a = 0;
    self->em003_0x328.call_group = 0;
    self->em003_0x328.call_done = 0;
    self->em003_0x328.call_count = 0;
    self->em003_0x328.call_timer = 0;
    self->em003_0x328.rage = 0;
    self->em003_0x328.call_kind = 0;
    self->em003_0x328.call_kind_done = 0;
    self->em003_0x328.flag_0x336 = 0;
    self->em003_0x328.tick = 0;
}

/* 0x80154E90 (0xE0): the init hook: seeds the motion parameters on a fresh start, attaches the user-data record
 * when there is none, arms the attached 0x1C72 effect and sets the attached flag. */
void em003_init(_ENEMY_WORK* self, u8 arg1) {
    VEC3 pos;
    Em003UserData* helper;

    VEC3_ctor(&pos);
    if (arg1 != 0) {
        em_motion_param_set(self, 0.0f, 0);
        em_motion_param_set(self, 1.0f, 0xA);
    }
    if (em_res_user_data_ck(self) == 0) {
        helper = (Em003UserData*)operator new(sizeof(Em003UserData));
        new (helper) Em003UserData;
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, 0.0f, -70.0f, 50.0f);
        eft_em_attach_set(self, 0x1A, &pos, 1.0f, 0x1C72);
    }
    self->flags_0x836 = (u16)(self->flags_0x836 | 0x8000);
}

/* 0x80154F70 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em003UserData::Em003UserData() {
}

/* 0x80154FAC (0x150): the motion hand-over: in action 1 the call sub-states move to their follow-up once the
 * motion has finished (or the anger level is past 0x28); in action 2 three sub-states step on. */
void em003_motion_next(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
    case 1:
        if (em_mot_finished_ck_view1() == 1 || self->field_0x7C8 >= 0x29) {
            switch (*sub) {
            case 8:
                *sub = 0x19;
                break;
            case 13:
                *sub = 0x1A;
                break;
            case 14:
                *sub = 0x1B;
                break;
            case 15:
                *sub = 0x1C;
                break;
            case 16:
                *sub = 0x1D;
                break;
            case 17:
                *sub = 0x1E;
                break;
            case 33:
                *sub = 0x23;
                break;
            case 34:
                *sub = 0x24;
                break;
            }
        }
        break;
    case 2:
        switch (*sub) {
        case 0:
            if (em_mot_finished_ck_view1() == 1) {
                *sub = 3;
            }
            break;
        case 8:
            if (em_mot_finished_ck_view1() == 1) {
                *sub = 9;
            }
            break;
        case 10:
            if (em_mot_finished_ck_view1() == 1) {
                *sub = 0xB;
            }
            break;
        }
        break;
    }
}

/* 0x801550FC (0x568): the event hook: action 1 arms the call group/kind and their timer, sends the calls and
 * applies the call buffs; action 2 arms the rage flag, 3 and 13 latch flags, 10 buffs the players and 11 summons
 * the boss-kind entries. */
void em003_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub) {
    u8 prev;
    EmAreaEntry* entry;
    s32 i;

    prev = self->field_0x1E4;
    self->field_0x1E4 = 0;
    self->flags_0x836 |= 0x8000;
    switch (kind) {
    case 1:
        switch (sub) {
        case 5:
            em_bait_react(self);
            break;
        case 7:
            self->em003_0x328.call_kind = 10;
            break;
        case 8:
        case 0x19:
            self->em003_0x328.call_group = 12;
            self->em003_0x328.call_timer = 900;
            break;
        case 9:
            self->field_0x1E4 = 1;
            break;
        case 0xD:
        case 0x1A:
            self->em003_0x328.call_group = 1;
            self->em003_0x328.call_timer = 900;
            break;
        case 0xE:
        case 0x1B:
            self->em003_0x328.call_group = 2;
            self->em003_0x328.call_timer = 900;
            break;
        case 0xF:
        case 0x1C:
            self->em003_0x328.call_group = 8;
            self->em003_0x328.call_timer = 900;
            break;
        case 0x10:
        case 0x1D:
            self->em003_0x328.call_group = 16;
            self->em003_0x328.call_timer = 900;
            break;
        case 0x11:
        case 0x1E:
            self->em003_0x328.call_group = 6;
            self->em003_0x328.call_timer = 900;
            break;
        case 0x21:
        case 0x23:
            self->em003_0x328.call_group = 15;
            self->em003_0x328.call_timer = 900;
            break;
        case 0x22:
        case 0x24:
            self->em003_0x328.call_group = 7;
            self->em003_0x328.call_timer = 900;
            break;
        case 0x13:
            self->field_0x1E4 = 2;
            break;
        case 0x14:
            self->field_0x1E4 = 3;
            break;
        case 0x15:
            if (em_busy_ck(self) == 1) {
                u8 call_kind = self->em003_0x328.call_kind;
                if (call_kind != 0) {
                    const Em003CallPair* pair = em003_call_kind_tbl;
                    u32 team = 0xFF;

                    for (; pair->id != 0; pair++) {
                        if (pair->id == call_kind) {
                            team = pair->team;
                        }
                    }
                    if (team == 0xFF) {
                        em_spawn_request(self->field_0x01A, call_kind, 0, self->area_no, 0xFF, 2, 0xFF, 1, 0xFF, 0, 0);
                    } else {
                        em_spawn_request(self->field_0x01A, call_kind, team, self->area_no, 0xFF, 2, 0xFF, 9, 0xFF, 0,
                                         0);
                    }
                    if (self->em003_0x328.call_kind == 10) {
                        self->em003_0x328.call_kind_done = 1;
                    }
                }
            }
            break;
        case 0x16: {
            s32 found;
            u8 group;
            s32 j;

            self->em003_0x328.call_done = 1;
            if (self->em003_0x328.call_count < 0xFF) {
                self->em003_0x328.call_count++;
            }
            found = 0;
            group = self->em003_0x328.call_group;
            if (group != 0) {
                entry = &em_area_entry_tbl[0][0];
                for (j = 0; j < 128; j++, entry++) {
                    if (entry->active != 0 && entry->kind_0x08 == 1 && entry->group == group) {
                        found = 1;
                        break;
                    }
                }
                if (found == 0) {
                    em_team_member_claim(group, self->area_no);
                } else if (em_busy_ck(self) == 1) {
                    em_spawn_request(self->field_0x01A, self->em003_0x328.call_group, 0, 0xFF, 0xFF, 1, 1, 0,
                                     self->area_no, 0, 0);
                }
            }
            break;
        }
        case 0x12:
            switch (prev) {
            case 1:
                em_area_buff_apply(self, self->area_no, 0);
                break;
            case 2:
                em_area_buff_apply(self, self->area_no, 2);
                break;
            case 3:
                em_area_buff_apply(self, self->area_no, 1);
                break;
            }
            break;
        case 0x17:
            self->em003_0x328.call_kind = 0x1F;
            break;
        case 0x1F:
            self->em003_0x328.call_kind = 0x11;
            break;
        case 0x20:
            self->em003_0x328.call_kind = 0x1D;
            break;
        case 0x25:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 2:
        switch (sub) {
        case 6:
            self->em003_0x328.rage = 1;
            if (self->value_0x44C < 3600) {
                self->value_0x44C = 3600;
            }
            break;
        }
        break;
    case 3:
        switch (sub) {
        case 9:
            self->field_0x1FB = 1;
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 0x5D:
            switch (prev) {
            case 1:
                pl_area_buff_apply(self->area_no, 0);
                break;
            case 2:
                pl_area_buff_apply(self->area_no, 2);
                break;
            case 3:
                pl_area_buff_apply(self->area_no, 1);
                break;
            }
            break;
        }
        break;
    case 11:
        if (move_work_state_ck() == 1) {
            entry = &em_area_entry_tbl[0][0];
            for (i = 0; i < 128; i++, entry++) {
                if (entry->active != 0 && entry->kind_0x08 == 1 && em003_yobi_boss_em_ck(entry->group) != 0) {
                    em_spawn_request(self->field_0x01A, entry->group, 0, 0xFF, 0xFF, 1, 1, 0, self->area_no, 0, 0);
                }
            }
        }
        break;
    case 13:
        switch (sub) {
        case 1:
            self->em003_0x328.flag_0x336 = 1;
            break;
        }
        break;
    }
}

#pragma peephole on
void em003_frame_update(_ENEMY_WORK* self) {
    if (++self->em003_0x328.tick >= 50) {
        self->em003_0x328.tick = 0;
    }
    if (self->em003_0x328.call_timer > 0) {
        self->em003_0x328.call_timer--;
    }
}
#pragma peephole off

void em003_finish_act3(_ENEMY_WORK *self) {
    em_fall_start_f(self, em_fall_height_get(self));
    em_act_arm_unless_act11(self, 3, 0xB);
    em_finish_flag_set(self);
}

void em003_act0_0(_ENEMY_WORK *self) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        self->timer_0x020 = 0x96;
        em_mot_set_ck(self, 1, 0xA, 0);
        return;
    case 0x1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 < 0) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act0_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 0xA, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act0_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 6, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act0_3(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set_ck(self, 0x1A, 6, 0);
        return;
    case 0x1:
        temp_ret = em_mot_end_ck(self);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
        }
        return;
    }
}

void em003_act0_6(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 0x1B, 6, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        return;
    }
}

void em003_act0_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act0_0(self);
        return;
    case 0x1:
        em003_act0_1(self);
        return;
    case 0x2:
        em003_act0_2(self);
        return;
    case 0x3:
        em003_act0_3(self);
        return;
    case 0x4:
        em003_act0_0(self);
        return;
    case 0x6:
        em003_act0_6(self);
        return;
    }
}

void em003_act1_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 0xA, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0xA, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_3(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xDD, 4, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_4(_ENEMY_WORK *self, u8 arg1) {
    VEC3 vec;
    s32 temp_r0;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        self->timer_0x020 = 0x12C;
        return;
    case 0x1:
        if (arg1 == 0) {
            if ((em_frame_check(self, 0, 94.0f, 0.0f) == 1) || (em_frame_check(self, 0, 258.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x18, 0x29, 0, 1.0f);
            }
            if (em_frame_check(self, 0, 96.0f, 0.0f) == 1) {
                eft_area_slot_bind(self, 6, 0x30);
            }
            if ((em_frame_check(self, 0, 146.0f, 0.0f) == 1) || (em_frame_check(self, 0, 314.0f, 0.0f) == 1)) {
                eft009_spawn_at_joint(self, 0x18, 0x2A, 0, 1.0f);
            }
            if ((em_frame_check(self, 0, 150.0f, 0.0f) == 1) || (em_frame_check(self, 0, 318.0f, 0.0f) == 1) || (em_frame_check(self, 0, 350.0f, 0.0f) == 1)) {
                setVector3(&vec, 0.0f, -10.0f, 80.0f);
                eft_em_spawn(self, 0x35, 0x1A, &vec, 1.0f);
            }
            if (em_frame_check(self, 0, 264.0f, 0.0f) == 1) {
                eft_area_slot_bind(self, 6, 0x58);
            }
        }
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em_state_set(self, 1, 5);
        }
        return;
    }
}

void em003_act1_5(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x14, 8, 0, 2);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_6(_ENEMY_WORK *self) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x18, 0, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
            break;
        }
        break;
    case 0x2:
        em_gauge_tick(self, 0.5f, 1, 0xF);
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            self->state += 1;
            em_mot_set(self, 0x19, 4, 0);
            em_tint_hold_end(self);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em003_act1_7(_ENEMY_WORK *self) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        if (em003_atama_koware_act_ck(self) == 1) {
            self->timer_0x020 = 0x12C;
        } else {
            self->timer_0x020 = 0x96;
        }
        if ((u8) self->field_0x7C8 >= 0x29) {
            f32 s = get_em_base_scale(self);
            em_mot_speed_set(self, 2.0f * (1.2f / s));
            self->timer_0x020 = ((f32) self->timer_0x020 / 1.2f);
        }
        em003_call_se_reset(self);
        return;
    case 0x1:
        em003_call_se_step(self, self->em003_0x328.call_kind, (u8)(em003_atama_koware_act_ck(self) == 1));
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em_state_set(self, 1, 0x15);
        }
        return;
    }
}

void em003_act1_8(_ENEMY_WORK *self, u8 arg1) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        if (arg1 == 1) {
            f32 s = get_em_base_scale(self);
            em_mot_speed_set(self, 2.0f * (1.5f / s));
        }
        self->timer_0x020 = 0x6E;
        if (em003_atama_koware_act_ck(self) == 1) {
            self->timer_0x020 *= 2;
        }
        self->timer_0x020 = ((f32) self->timer_0x020 * (2.0f / get_em_base_scale(self)));
        break;
    case 0x1:
        if (em_frame_check(self, 1, 68.0f, 0.0f) == 1) {
            self->state += 1;
            self->state_0x006 = 0;
            em_mot_set(self, 0xCA, 0, 0);
            if (arg1 == 1) {
                f32 s = get_em_base_scale(self);
                em_mot_speed_set(self, 2.0f * (1.5f / s));
                break;
            }
        }
        break;
    case 0x2:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em_state_set(self, 1, 0x16);
        }
        break;
    }
}

void em003_act1_9(_ENEMY_WORK *self, u8 arg1) {
    VEC3 vec;
    s32 temp_r0;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 6, 0);
        if (em003_atama_koware_act_ck(self) == 1) {
            self->timer_0x020 = 0x13B;
        } else {
            self->timer_0x020 = 0xD2;
        }
        if ((u8) self->field_0x7C8 >= 0x29) {
            f32 s = get_em_base_scale(self);
            em_mot_speed_set(self, 2.0f * (1.2f / s));
            self->timer_0x020 = ((f32) self->timer_0x020 / 1.2f);
            break;
        }
        break;
    case 0x1:
        if (em_frame_check(self, 0, 204.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 50.0f, 150.0f);
            switch (arg1) {
            case 0x0:
                eft_em_spawn(self, 0x2D, 0x18, &vec, 1.0f);
                break;
            case 0x1:
                eft_em_spawn(self, 0x2E, 0x18, &vec, 1.0f);
                break;
            case 0x2:
                eft_em_spawn(self, 0x2F, 0x18, &vec, 1.0f);
                break;
            }
        }
        if ((em_frame_check(self, 3, 236.0f, 254.0f) == 1) && ((system_w.field_0x0c & 0xF) == 0)) {
            setVector3(&vec, 0.0f, 50.0f, 150.0f);
            switch (arg1) {
            case 0x0:
                eft_em_spawn(self, 0x15, 0x18, &vec, 1.0f);
                break;
            case 0x1:
                eft_em_spawn(self, 0x16, 0x18, &vec, 1.0f);
                break;
            case 0x2:
                eft_em_spawn(self, 0x17, 0x18, &vec, 1.0f);
                break;
            }
        }
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em_state_set(self, 1, 0x12);
        }
        break;
    }
}

void em003_act1_10(_ENEMY_WORK *self, u8 arg1) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1) {
        case 0x0:
            em_mot_set(self, 0xE6, 4, 0);
            return;
        case 0x1:
            em_mot_set(self, 0xE7, 6, 0);
            return;
        case 0x2:
            em_mot_set(self, 0xE8, 6, 0);
            return;
        }
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_18(_ENEMY_WORK *self) {
    em_busy_set(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCC, 0, 0);
        return;
    case 0x1:
        em_turn_in_window(self, 4.0f, 38.0f, -0x4000);
        em_turn_in_window(self, 48.0f, 74.0f, -0x4000);
        em_turn_in_window(self, 84.0f, 112.0f, -0x4000);
        em_turn_in_window(self, 112.0f, 130.0f, -0x4000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_21(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 4, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_22(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDC, 6, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_37(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 6, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act1_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act1_0(self);
        return;
    case 0x1:
        em003_act1_1(self);
        return;
    case 0x2:
        em003_act1_2(self);
        return;
    case 0x3:
        em003_act1_3(self);
        return;
    case 0x4:
        em003_act1_4(self, 0);
        return;
    case 0x5:
        em003_act1_5(self);
        return;
    case 0x6:
        em003_act1_6(self);
        return;
    case 0x7:
        em003_act1_7(self);
        return;
    case 0x8:
        em003_act1_8(self, 0);
        return;
    case 0x9:
        em003_act1_9(self, 0);
        return;
    case 0xA:
        em003_act1_10(self, 0);
        return;
    case 0xB:
        em003_act1_10(self, 1);
        return;
    case 0xC:
        em003_act1_10(self, 2);
        return;
    case 0xD:
        em003_act1_8(self, 0);
        return;
    case 0xE:
        em003_act1_8(self, 0);
        return;
    case 0xF:
        em003_act1_8(self, 0);
        return;
    case 0x10:
        em003_act1_8(self, 0);
        return;
    case 0x11:
        em003_act1_8(self, 0);
        return;
    case 0x12:
        em003_act1_18(self);
        return;
    case 0x13:
        em003_act1_9(self, 1);
        return;
    case 0x14:
        em003_act1_9(self, 2);
        return;
    case 0x15:
        em003_act1_21(self);
        return;
    case 0x16:
        em003_act1_22(self);
        return;
    case 0x17:
        em003_act1_7(self);
        return;
    case 0x18:
        em003_act1_4(self, 1);
        return;
    case 0x19:
        em003_act1_8(self, 1);
        return;
    case 0x1A:
        em003_act1_8(self, 1);
        return;
    case 0x1B:
        em003_act1_8(self, 1);
        return;
    case 0x1C:
        em003_act1_8(self, 1);
        return;
    case 0x1D:
        em003_act1_8(self, 1);
        return;
    case 0x1E:
        em003_act1_8(self, 1);
        return;
    case 0x1F:
        em003_act1_7(self);
        return;
    case 0x20:
        em003_act1_7(self);
        return;
    case 0x21:
        em003_act1_8(self, 0);
        return;
    case 0x22:
        em003_act1_8(self, 0);
        return;
    case 0x23:
        em003_act1_8(self, 1);
        return;
    case 0x24:
        em003_act1_8(self, 1);
        return;
    case 0x25:
        em003_act1_37(self);
        return;
    default:
        return;
    }
}

void em003_act2_0(_ENEMY_WORK *self, u8 arg1) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            em_approach_start(self, -300.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            break;
        case 2:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        em_mot_set(self, 6, 6, 0);
        return;
    case 0x1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act2_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em003_turn_seq_left, 0, 0, 0);
        return;
    case 0x1:
        if (em_turn_seq_step(self, (void*)em003_turn_seq_left) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act2_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em003_turn_seq_right, 0, 1, 0);
        return;
    case 0x1:
        if (em_turn_seq_step(self, (void*)em003_turn_seq_right) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act2_3(_ENEMY_WORK *self, u8 arg1) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            em_approach_start(self, -300.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            break;
        case 2:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        em_mot_set(self, 7, 0xA, 0);
        return;
    case 0x1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act2_slide(_ENEMY_WORK *self, u8 arg1) {
    EmSpawnRec rec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD6, 4, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0xD7, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x318 = em_key_curve_eval(self, em003_slide_move_keys);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            self->state_0x006 = 0;
            self->field_0x324 = 0.5f;
            em_move_offset_step(self, &self->field_0x1BC);
            em_mot_set(self, 0x1B, 6, 0);
            break;
        }
        self->field_0x318 = em_key_curve_eval(self, em003_slide_move_keys);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    case 0x3:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > 0.0f) {
            self->field_0x318 = 0.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            switch (arg1) {
            case 0x0:
                switch (self->state_0x006) {
                case 0x0:
                    self->state_0x006++;
                    em_mot_set(self, 0x1B, 0, 0);
                    break;
                case 0x1:
                    self->state += 1;
                    em_mot_set(self, 0xD8, 6, 0);
                    break;
                }
                break;
            case 0x1:
                em_action_finish_walk(self);
                break;
            }
        }
        break;
    case 0x4:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD9, 0, 0);
            break;
        }
        break;
    case 0x5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em003_act2_4(_ENEMY_WORK* self, u8 arg1) {
    em003_act2_slide(self, arg1);
}

void em003_act2_6(_ENEMY_WORK *self) {
    u32 temp_r31;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_approach_start(self, 0.0f, 0);
        em_mot_set(self, 0xD0, 6, 0);
        return;
    case 0x1:
        temp_r31 = self->field_0x1C0;
        if (em_approach_step(self, 0, 0x100) == 1) {
            self->state += 1;
            em_mot_set(self, 0xDA, 6, 0);
            return;
        }
        f32 turn = (360.0f * (f32) (s16) (self->field_0x1C0 - temp_r31)) / 65536.0f;
        em_motion_blend_step(self, turn, 0.02f);
        em_mot_layer_blend_step(self, 0xD4, 0xD5, (s32)em_mot_frame_get(self), self->field_0x464);
        return;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act2_7(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCF, 0, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = -15.5f * get_em_base_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        break;
    case 0x1:
        em_turn_in_window(self, 60.0f, 84.0f, 0x4000);
        em_turn_in_window(self, 84.0f, 96.0f, 0x4000);
        if (em_frame_check(self, 1, 78.0f, 0.0f) == 1) {
            em_move_offset_apply(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0xDB, 0, 0);
            break;
        }
        break;
    case 0x2:
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em_drop_item_spawn(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x6B, 4, 0);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em003_act2_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act2_0(self, 0);
        return;
    case 0x1:
        em003_act2_1(self);
        return;
    case 0x2:
        em003_act2_2(self);
        return;
    case 0x3:
        em003_act2_3(self, 0);
        return;
    case 0x4:
        em003_act2_4(self, 0);
        return;
    case 0x5:
        em003_act2_4(self, 1);
        return;
    case 0x6:
        em003_act2_6(self);
        return;
    case 0x7:
        em003_act2_7(self);
        return;
    case 0x8:
        em003_act2_0(self, 1);
        return;
    case 0x9:
        em003_act2_3(self, 1);
        return;
    case 0xA:
        em003_act2_0(self, 2);
        return;
    case 0xB:
        em003_act2_3(self, 2);
        return;
    default:
        return;
    }
}

void em003_act3_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_start_f(self, em_fall_height_get(self));
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
            break;
        }
        break;
    case 0x2:
        self->field_0x314 = em_key_curve_eval(self, em003_turn_step_keys);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x1A, 6, 0);
            break;
        }
        break;
    case 0x3:
        em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1) {
            em_act_set_unless_act11(self, 3, 2);
        }
        break;
    }
}

void em003_act3_1(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_start_f(self, em_fall_height_get(self));
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
            break;
        }
        break;
    case 0x2:
        self->field_0x314 = em_key_curve_eval(self, em003_turn_step_keys);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        temp_ret = em_mot_end_ck(self);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

void em003_act3_2(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 0x1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x1D, 6, 0);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x2E, 6, 0);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
            break;
        }
        break;
    case 0x4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em003_act3_3(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 30.0f;
        self->field_0x324 = 0.5f;
        return;
    case 0x1:
        temp_ret = em_approach_step(self, 0, 0x80);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
            return;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > 60.0f) {
            self->field_0x318 = 60.0f;
        }
        return;
    }
}

void em003_act3_4(_ENEMY_WORK *self) {
    f32 var_f1;
    u16 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mat_blend_reset(self);
        em_mot_set(self, 0x1F, 0, 0);
        return;
    case 0x1:
        em_target_pos_set(self, 0);
        temp_r0 = (u16)calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0;
        if (temp_r0 != 0) {
            var_f1 = ((360.0f * (f32) (s16) temp_r0) / 65536.0f) / 60.0f;
        } else {
            var_f1 = 0.0f;
        }
        em_motion_blend_step(self, var_f1, 0.04f);
        em_mot_layer_blend_step(self, 0x23, 0x24, (s32)em_mot_frame_get(self), self->field_0x464);
        return;
    }
}

void em003_act3_5(_ENEMY_WORK* self, u8 arg1) {
    u8 area;

    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fly_start(self, (void*)em003_fly_tbl_a);
        em_approach_start(self, 0.0f, 0x19);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        break;
    case 1:
        switch (arg1) {
        case 1:
            switch (self->state_0x006) {
            case 0:
                if (self->field_0x1F9 == 0) {
                    self->state_0x006++;
                    if (self->field_0x43D == 1 && em_any_player_seen_ck(self) == 1) {
                        self->state_0x007 = 1;
                    }
                }
                break;
            case 1:
                switch (self->state_0x007) {
                case 1:
                    if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= 16000000.0f) {
                        em_state_set(self, 3, 8);
                        em_user_data_reenter(self);
                        return;
                    }
                    break;
                }
                break;
            }
            break;
        }
        if (em_approach_step(self, 0, 0) == 1) {
            switch (arg1) {
            default:
                em003_finish_act3(self);
                break;
            case 1:
                area = self->field_0x9F8;
                if (self->area_no == area || area == 0xFF) {
                    em_state_set(self, 3, 0xA);
                } else {
                    em_state_set(self, 3, 9);
                }
                break;
            }
        } else {
            switch (arg1) {
            default:
                em_fly_step(self, 1, (void*)em003_fly_tbl_a);
                break;
            case 1:
                em_fly_step(self, 0, (void*)em003_fly_tbl_a);
                break;
            }
            em_fall_height_get(self);
            em_move_rot_ground_step(self, &self->field_0x1BC);
        }
        break;
    }
}

void em003_act3_6(_ENEMY_WORK *self, u8 arg1) {
    f32 temp_f2;
    f32 var_f3;
    u8 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_move_vec_clr(self);
        return;
    case 0x1:
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0x0:
            self->field_0x314 = em_key_curve_eval(self, em003_lift_keys);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1) {
                if (++self->state_0x007 >= 2) {
                    self->state_0x006 += 1;
                    em_lift_start(self);
                }
            }
            break;
        case 0x1:
            em_lift_step(self);
            break;
        }
        switch (arg1) {
        default:
            var_f3 = stg_area_ceiling_get(self->area_no) - 1000.0f;
            temp_f2 = self->field_0x20C;
            if ((var_f3 - temp_f2) < 2000.0f) {
                var_f3 = 2000.0f + temp_f2;
            }
            break;
        case 1:
            var_f3 = 1600.0f + self->vec_0x36C.y;
            break;
        }
        if (self->pos.y >= var_f3) {
            em_action_finish_fall(self);
        }
        return;
    }
}

void em003_act3_7(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        return;
    case 0x1:
        temp_ret = em_turn_to_target(self, 0x200);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
        }
        return;
    }
}

void em003_act3_8(_ENEMY_WORK *self) {
    u32 temp_ret;
    u32 var_r30;
    u8 temp_r0;
    u8 temp_r0_2;

    em_busy_set(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x3D, 6, 0);
            self->vec_0x36C.y += 500.0f * get_em_chg_scale(self);
            self->state_0x006 = 0;
            break;
        }
        break;
    case 0x2:
        em_hit_arm_set(self);
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0x0:
            em_turn_to_target_3d(self, 0x100, 0x100);
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                self->state_0x006 += 1;
                em_move_vec2_clr(self);
                self->field_0x318 = 5.0f;
                self->field_0x324 = 3.0f;
                em_approach_start(self, -200.0f, 0x10);
            }
            break;
        case 0x1:
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            em_approach_step(self, 0, 0x80);
            if (self->field_0x318 > 70.0f) {
                self->field_0x318 = 70.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            break;
        }
        break;
    case 0x3:
        em_hit_arm_set(self);
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state += 1;
            em_mot_set(self, 0x2D, 6, 0);
            self->state_0x006 = 0;
            self->field_0x314 = -10.0f;
            self->field_0x324 = -3.2f;
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            break;
        }
        em_fall_height_get(self);
        em_move_offset_step_update(self, &self->field_0x1BC);
        if (self->field_0x318 > 70.0f) {
            self->field_0x318 = 70.0f;
            break;
        }
        break;
    case 0x4:
        temp_r0_2 = self->state_0x006;
        switch (temp_r0_2) {
        case 0x0:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                self->state_0x006 += 1;
                self->field_0x324 = -6.8f;
            }
            break;
        case 0x1:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (self->field_0x318 < 0.0f) {
                self->field_0x318 = 0.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            if (var_r30 == 1) {
                self->state += 1;
                em_move_mode_set(self, 3);
                em_mot_set(self, 0x1D, 6, 0);
                break;
            }
            temp_ret = em_busy_ck(self);
            if (temp_ret == 1) {
                em_action_finish_fall(self);
                break;
            }
        }
        break;
    case 0x5:
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 3);
            em_action_finish_walk(self);
        }
        break;
    }
}

void em003_act3_10(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x2D, 6, 0);
        return;
    case 0x1:
        temp_ret = em_mot_end_ck(self);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
        }
        return;
    }
}

void em003_act3_11(_ENEMY_WORK *self) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set_ck(self, 0x1F, 4, 0);
        self->timer_0x020 = 0x3C;
        return;
    case 0x1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em003_finish_act3(self);
        }
        return;
    }
}

void em003_act3_12(_ENEMY_WORK *self) {
    u8 temp_r0;

    em_busy_set(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        self->state_0x006 = 0;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x3D, 6, 0);
        return;
    case 0x1:
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0x0:
            if (em_frame_check(self, 1, 70.0f, 0.0f) == 1) {
                self->state_0x006 += 1;
                em_move_vec2_clr(self);
                self->field_0x318 = 5.0f;
                self->field_0x324 = 3.0f;
            }
            break;
        case 0x1:
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->field_0x1BC);
            if (self->field_0x318 > 90.0f) {
                self->field_0x318 = 90.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em003_finish_act3(self);
        }
        return;
    }
}

void em003_act3_14(_ENEMY_WORK *self) {
    f32 temp_f2;
    f32 temp_f3;
    f32 var_f1;
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        self->timer_0x020 = 0x64;
        return;
    case 0x1:
        var_f1 = 0.95f * (self->vec_0x36C.y - self->pos.y);
        if (var_f1 > 60.0f) {
            var_f1 = 60.0f;
        } else if (var_f1 < -60.0f) {
            var_f1 = -60.0f;
        }
        temp_f3 = self->pos.y + var_f1;
        self->pos.y = temp_f3;
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if ((temp_r0 <= 0) || ((temp_f2 = self->vec_0x36C.y, temp_f2 >= temp_f3 - 50.0f) && temp_f2 <= 50.0f + temp_f3)) {
            self->state++;
            em_action_finish_fall(self);
        }
        return;
    }
}

void em003_act3_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act3_0(self);
        return;
    case 0x1:
        em003_act3_1(self);
        return;
    case 0x2:
        em003_act3_2(self);
        return;
    case 0x3:
        em003_act3_3(self);
        return;
    case 0x4:
        em003_act3_4(self);
        return;
    case 0x5:
        em003_act3_5(self, 0);
        return;
    case 0x6:
        em003_act3_6(self, 0);
        return;
    case 0x7:
        em003_act3_7(self);
        return;
    case 0x8:
        em003_act3_8(self);
        return;
    case 0x9:
        em003_act3_5(self, 1);
        return;
    case 0xA:
        em003_act3_10(self);
        return;
    case 0xB:
        em003_act3_11(self);
        return;
    case 0xC:
        em003_act3_12(self);
        return;
    case 0xD:
        em003_act3_6(self, 1);
        return;
    case 0xE:
        em003_act3_14(self);
        return;
    default:
        return;
    }
}

void em003_act4_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x44, 0, 0);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

void em003_act4_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2E, 6, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em003_act4_2(_ENEMY_WORK *self) {
    u32 temp_ret;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1E, 6, 0);
        break;
    case 0x1:
        if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
            self->state += 1;
            em_fall_start_f(self, em_fall_height_get(self));
            em_move_vec2_clr(self);
            self->field_0x314 = 5.0f * get_em_base_scale(self);
            self->field_0x320 = -0.2f * get_em_base_scale(self);
            break;
        }
        break;
    case 0x2:
        if (em_frame_check(self, 1, 76.0f, 0.0f) == 0) {
            em_move_offset_apply(self);
        } else {
            CancelFade(self);
        }
        temp_ret = em_mot_end_ck(self);
        if (temp_ret == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

void em003_act4_3(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 0x1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x1D, 6, 0);
            break;
        }
        break;
    case 0x2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

void em003_act4_4(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, (void*)em003_turn_seq_fly, 0, 0, 0);
        return;
    case 0x1:
        if (em_turn_seq_step(self, (void*)em003_turn_seq_fly) == 1) {
            em_action_finish_walk(self);
        }
        return;
    }
}

void em003_act4_5(_ENEMY_WORK *self, u8 arg1) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        switch (arg1) {
        default:
            em_approach_start(self, -200.0f, 0);
            break;
        case 1:
            em_approach_start(self, -500.0f, 0);
            break;
        }
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 0x1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_walk(self);
            break;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > 70.0f) {
            self->field_0x318 = 70.0f;
        }
        break;
    }
}

void em003_act4_6(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_fly_start(self, (void*)em003_fly_tbl_b);
        em_approach_start(self, 0.0f, 9);
        return;
    case 0x1:
        if (em_approach_step(self, 0, 0) == 1) {
            em_action_finish_walk(self);
            return;
        }
        em_fly_step(self, 0, (void*)em003_fly_tbl_b);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        return;
    }
}

void em003_act4_7(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        em_move_vec_clr(self);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x42, 0, 0);
            self->field_0x318 = em_key_curve_eval(self, em003_back_step_keys);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x2:
        self->field_0x318 = em_key_curve_eval(self, em003_back_step_keys);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

void em003_act4_8(_ENEMY_WORK *self, u8 arg1) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        em_move_vec2_clr(self);
        if ((u32) (arg1 - 1) > 1) {
            if (arg1 == 0) {
                self->field_0x318 = -20.0f;
                self->field_0x324 = 0.3f;
                self->timer_0x020 = 0x16;
                break;
            }
            break;
        }
        self->field_0x318 = -22.0f;
        self->field_0x324 = 0.15f;
        self->timer_0x020 = 0x1E;
        break;
    case 0x1:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > 0.0f) {
            self->field_0x318 = 0.0f;
        }
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            switch (arg1) {
            case 0:
            case 2:
                em_act_set_unless_act11(self, 4, 1);
                break;
            default:
                em_action_finish_walk(self);
                break;
            }
        }
        break;
    }
}

#pragma opt_common_subs off
void em003_act4_10(_ENEMY_WORK *self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2B, 6, 0);
        return;
    case 0x1:
        if (em_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            shell_set_func_ptr->method_0x2C(self, 6, 0x18, &vec, 1.0f, 0xFFFF, shell_set_func_ptr);
            draw_shape_arm((u32)self, 0x18, 0xA);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        return;
    }
}
#pragma opt_common_subs reset

void em003_act4_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act4_0(self);
        return;
    case 0x1:
        em003_act4_1(self);
        return;
    case 0x2:
        em003_act4_2(self);
        return;
    case 0x3:
        em003_act4_3(self);
        return;
    case 0x4:
        em003_act4_4(self);
        return;
    case 0x5:
        em003_act4_5(self, 0);
        return;
    case 0x6:
        em003_act4_6(self);
        return;
    case 0x7:
        em003_act4_7(self);
        return;
    case 0x8:
        em003_act4_8(self, 0);
        return;
    case 0x9:
        em003_act4_8(self, 1);
        return;
    case 0xA:
        em003_act4_10(self);
        return;
    case 0xB:
        em003_act4_8(self, 2);
        return;
    case 0xC:
        em003_act4_5(self, 1);
        return;
    default:
        return;
    }
}

/* The act-7 swipe aim curve (`em003_act7_1`). */
f32 em003_swipe_aim_keys[8] = {
    0.0f, 180.0f, 46.0f, 180.0f, 68.0f, 0.0f, -1.0f, 0.0f,
};

void em003_act7_0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2F, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em003_act7_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        em_hit_window_set_default(self, 0, 3);
        break;
    case 0x1:
                em_mot_speed_set(self, getKeyData((f32*)em003_swipe_speed_keys, em_mot_frame_get(self)));
        em_turn_in_window2(self, 18.0f, 56.0f, -0x7333);
        if (self->field_0xA0D != 0) {
                        self->hit_angle_0xA4C = (u8) ((u32)(u16) (0.5f + ((65536.0f * getKeyData((f32*)em003_swipe_aim_keys, em_mot_frame_get(self))) / 360.0f)) >> 8);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x32, 0, 0);
            em_hit_window_set_default(self, 0, 3);
            break;
        }
        break;
    case 0x2:
                em_mot_speed_set(self, getKeyData((f32*)em003_swipe_speed_keys, em_mot_frame_get(self)));
        em_turn_in_window2(self, 18.0f, 56.0f, -0x7333);
        if (self->field_0xA0D != 0) {
                        self->hit_angle_0xA4C = (u8) ((u32)(u16) (0.5f + ((65536.0f * getKeyData((f32*)em003_swipe_aim_keys, em_mot_frame_get(self))) / 360.0f)) >> 8);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80158E24 (0x1CC): the three-hit swipe: on frames 126/130/134 it hands the shell job a spawn record whose
 * angles sweep across (wider on the first variant), then walks out. */
#pragma opt_common_subs off
void em003_act7_2(_ENEMY_WORK* self, u8 arg1) {
    EmSpawnRec rec;
    int angle_a;
    int angle_b;
    s8 hit;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_stamina_add(self, -0xC);
        em_mot_set(self, 0xDF, 6, 0);
        self->em003_0x328.hit_id = get_hit_id();
        break;
    case 1:
        if (em_frame_check(self, 0, 126.0f, 0.0f) == 1) {
            hit = 0;
        } else if (em_frame_check(self, 0, 130.0f, 0.0f) == 1) {
            hit = 1;
        } else if (em_frame_check(self, 0, 134.0f, 0.0f) == 1) {
            hit = 2;
        } else {
            hit = -1;
        }
        if (hit != -1) {
            switch (hit) {
            default:
                if (arg1 == 0) {
                    angle_a = 0x271C;
                } else {
                    angle_a = 0x11C7;
                }
                angle_b = 0xE38F;
                break;
            case 1:
                if (arg1 == 0) {
                    angle_a = 0x271C;
                } else {
                    angle_a = 0xE39;
                }
                angle_b = 0xF1C8;
                break;
            case 2:
                if (arg1 == 0) {
                    angle_a = 0x238E;
                } else {
                    angle_a = 0xC17;
                }
                angle_b = 0;
                break;
            }
            em003_spawn_rec_set(&rec, 0, angle_a, angle_b);
            shell_set_func_ptr->method_0x40(self, &rec, 7, self->em003_0x328.hit_id, shell_set_func_ptr);
            eft007_part_set(self, 9);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}
#pragma opt_common_subs reset

void em003_act7_3(_ENEMY_WORK *self, u8 arg1) {
    s32 temp_r0;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_stamina_add(self, -0xC);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 4, 3);
        self->timer_0x020 = 0;
        switch (arg1) {
        default:
            em_approach_start(self, 300.0f, 0x10);
            self->timer_0x020 = 0x14;
            break;
        case 0x1:
            em_approach_start(self, -250.0f, 0x10);
            self->timer_0x020 = 0xA;
            break;
        case 0x2:
            em_approach_start(self, -100.0f, 0);
            if (self->value_0x378 >= 800.0f) {
                self->value_0x378 = 800.0f;
                break;
            }
            break;
        case 0x3:
            em_approach_start(self, -1600.0f, 0x10);
            break;
        case 0x4:
            em_approach_start(self, -500.0f, 0x10);
            self->timer_0x020 = 0xA;
            break;
        case 0x5:
            em_approach_start(self, 1500.0f, 0x10);
            self->timer_0x020 = 0x14;
            break;
        }
        break;
    case 0x1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            self->timer_0x020 = 0;
        }
        if (em_approach_step(self, 0, (u16)(arg1 == 0 ? 0x80 : 0x40)) == 1 && self->timer_0x020 <= 0) {
            self->state += 1;
            self->state_0x006 = 0;
            switch (arg1) {
            default:
                em_hit_window_clear(self, 0);
                em_mot_set(self, 9, 4, 0);
                break;
            case 0x2:
                em_mot_set(self, 0x40, 4, 0);
                break;
            case 0x3:
                em_hit_window_clear(self, 0);
                em_act_set_unless_act11(self, 4, 7);
                break;
            case 0x4:
                em_hit_window_clear(self, 0);
                em_act_set_unless_act11(self, 7, 7);
                break;
            }
        }
        break;
    case 0x2:
        switch (arg1) {
        case 2:
            if (self->state_0x006 == 0 && em_frame_check(self, 1, 120.0f, 0.0f) == 1) {
                self->state_0x006++;
                em_hit_window_clear(self, 0);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

#pragma peephole on
void em003_act7_7(_ENEMY_WORK *self) {
    u8 temp_r3;

    switch (self->state) {
    case 0x0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDE, 4, 0);
        em_hit_window_set_default(self, 0, 6);
        return;
    case 0x1:
        if (em_frame_check(self, 1, 20.0f, 0.0f) == 0) {
            em_turn_to_target(self, 0x60);
        }
        temp_r3 = self->state_0x006;
        switch (temp_r3) {
        case 0x0:
            if (self->field_0xA0D == 0) {
                self->state_0x006 = temp_r3 + 1;
                em_hit_window_set_default(self, 0, 7);
            }
            break;
        case 0x1:
            if (self->field_0xA0D == 0) {
                self->state_0x006 = temp_r3 + 1;
                em_hit_window_set_default(self, 0, 8);
            }
            break;
        case 0x2:
            if (self->field_0xA0D == 0) {
                self->state_0x006 = temp_r3 + 1;
                em_hit_window_set_default(self, 0, 9);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}
#pragma peephole off

void em003_act7_8(_ENEMY_WORK *self) {
    f32 temp_f1;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_stamina_add(self, -0xC);
        em_mot_set(self, 0x36, 2, 0);
        em_hit_window_set_default(self, 0, 5);
        em_move_vec_clr(self);
        temp_f1 = vec3_distance(&self->pos, &self->vec_0x36C);
        if (temp_f1 > 500.0f) {
            self->field_0x318 = 12.0f;
            return;
        }
        if (temp_f1 > 300.0f) {
            self->field_0x318 = (12.0f * temp_f1) / 200.0f;
            return;
        }
        self->field_0x318 = 0.0f;
        return;
    case 0x1:
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        return;
    }
}

void em003_act7_9(_ENEMY_WORK *self, u8 arg1, u8 arg2, u8 arg3) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCE, 4, 0);
        return;
    case 0x1:
        if (em_frame_check(self, 3, 76.0f, 104.0f) == 1) {
            em_turn_to_target(self, 0x600);
        }
        if ((em_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            setVector3(&vec, -50.0f, 0.0f, 50.0f);
            eft_em_spawn(self, 4, 0x10, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            switch (arg1) {
            default:
                self->field_0x1E7 = 3;
                break;
            case 1:
                self->field_0x1E7 = 2;
                break;
            case 2:
                self->field_0x1E7 = 1;
                break;
            }
            switch (arg2) {
            default:
                if (arg3 == 0) {
                    em_act_arm_unless_down(self, 7, 0xD);
                } else {
                    em_act_arm_unless_down(self, 7, 0x1D);
                }
                break;
            case 1:
                em_act_arm_unless_down(self, 7, 0x19);
                break;
            }
        }
        break;
    }
}

#pragma opt_common_subs off
void em003_act7_10(_ENEMY_WORK *self, u8 arg1) {
    VEC3 sp14;
    VEC3 vec;

    VEC3_ctor(&sp14);
    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD1, 4, 0);
        break;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0xD3, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x318 = em_key_curve_eval(self, em003_swipe_move_keys);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x2:
        if (em_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            setVector3(&sp14, 0.0f, 0.0f, 150.0f * get_em_chg_scale(self));
            setVector3(&vec, 0.0f, 0.0f, 230.0f * get_em_chg_scale(self));
            rotVecY(&sp14, self->field_0x1C0);
            rotVecY(&vec, self->field_0x1C0);
            addVec3To(&sp14, &self->pos);
            addVec3To(&vec, &self->pos);
            shell_set_func_ptr->method_0x30(self, 7, &sp14, &vec, 1.0f, 0xFFFF, shell_set_func_ptr);
            eft_spawn_type11(self, &self->pos, 4, 1.8f);
        }
        if (em_mot_end_ck(self) == 1) {
            switch (arg1) {
            case 0x0:
                self->state += 1;
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xD2, 0, 0);
                break;
            case 0x1:
                em_action_finish_walk(self);
                break;
            case 0x2:
                em_act_set_unless_act11(self, 4, 8);
                break;
            }
        } else {
            self->field_0x318 = em_key_curve_eval(self, em003_swipe_move_keys);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            break;
        }
        break;
    case 0x3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
#pragma opt_common_subs reset

void em003_act7_12(_ENEMY_WORK *self) {
    f32 temp_f1;

    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 3);
        em_stamina_add(self, -0xC);
        em_mot_set(self, 0x38, 6, 0);
        em_move_vec2_clr(self);
        em_height_set(self, 0.0f);
        em_hit_window_set_default(self, 0, 2);
        return;
    case 0x1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            em_turn_to_target(self, 0x100);
        }
        em_height_add(self, em_key_curve_eval(self, em003_hop_height_keys));
        temp_f1 = self->field_0x1AC;
        if (temp_f1 > 0.0f) {
            self->field_0x1AC = 0.0f;
        } else if (temp_f1 < -100.0f) {
            self->field_0x1AC = -100.0f;
        }
        self->field_0x318 = em_key_curve_eval(self, em003_hop_move_keys);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        return;
    }
}

#pragma opt_common_subs off
void em003_act7_13(_ENEMY_WORK* self, u8 arg1, u8 arg2, u8 arg3) {
    VEC3 vec;
    s32 winding;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            em_mot_set(self, 0xE0, 2, 0);
            if (arg3 != 0) {
                em_hit_window_set_default(self, 0, 0xC);
            }
            break;
        case 1:
            em_mot_set(self, 0xE2, 2, 0);
            break;
        }
        em_move_vec_clr(self);
        self->em003_0x328.dist_ratio = em_target_dist_ratio_get(self, 4.0f, 0.8f, 300.0f);
        break;
    case 1:
        winding = 0;
        switch (arg1) {
        case 1:
            if (self->state_0x006 == 0) {
                winding = 1;
                if (em_frame_check(self, 3, 0.0f, 20.0f) == 1) {
                    em_turn_to_target(self, 0x800);
                }
                if (em_mot_end_ck(self) == 1) {
                    self->state_0x006++;
                    em_mot_set(self, 0xE0, 2, 0);
                    if (arg3 != 0) {
                        em_hit_window_set_default(self, 0, 0xC);
                    }
                }
            }
            break;
        }
        if (winding == 0) {
            self->field_0x318 = self->em003_0x328.dist_ratio * em_key_curve_eval(self, em003_lunge_move_keys);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (arg3 == 0 && em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 110.0f, 255.0f);
                shell_set_func_ptr->method_0x54(self, 0, 1, &vec, shell_set_func_ptr);
                setVector3(&vec, -50.0f, 0.0f, 50.0f);
                eft_em_spawn(self, 5, 0x10, &vec, 1.0f);
                vec.x = 50.0f;
                eft_em_spawn(self, 5, 9, &vec, 1.0f);
            }
            if (em_frame_check(self, 1, 32.0f, 0.0f) == 1) {
                self->state++;
                self->field_0x1E7--;
                switch (arg2) {
                default:
                    if (self->field_0x1E7 == 0) {
                        em_state_set(self, 7, 0xF);
                    } else if (arg3 == 0) {
                        em_state_set(self, 7, 0xE);
                    } else {
                        em_state_set(self, 7, 0x1E);
                    }
                    break;
                case 1:
                    if (self->field_0x1E7 == 0) {
                        em_state_set(self, 2, 4);
                    } else {
                        em_state_set(self, 7, 0x1A);
                    }
                    break;
                }
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

void em003_act7_15(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE1, 2, 0);
        return;
    case 0x1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

#pragma opt_common_subs off
void em003_act7_19(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE3, 4, 0);
        if (arg1 == 1) {
            em_hit_window_set_default(self, 0, 0xB);
            break;
        }
        break;
    case 0x1:
        if ((arg2 == 1) && (em_frame_check(self, 2, 30.0f, 0.0f) == 1)) {
            em_turn_to_target(self, 0x300);
        }
        if ((arg1 == 0) && (em_frame_check(self, 0, 46.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, 110.0f, 220.0f);
            shell_set_func_ptr->method_0x54(self, 0, 1, &vec, shell_set_func_ptr);
            setVector3(&vec, -50.0f, 0.0f, 50.0f);
            eft_em_spawn(self, 5, 0x10, &vec, 1.0f);
            vec.x = 50.0f;
            eft_em_spawn(self, 5, 9, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
#pragma opt_common_subs reset

/* 0x80159FF0 (0x1CC): the standing three-hit swipe: the same sweep on frames 100/108/118. */
#pragma opt_common_subs off
void em003_act7_20(_ENEMY_WORK* self, u8 arg1) {
    EmSpawnRec rec;
    int angle_a;
    int angle_b;
    s8 hit;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_stamina_add(self, -0xC);
        em_mot_set(self, 0xE4, 6, 0);
        self->em003_0x328.hit_id = get_hit_id();
        break;
    case 1:
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            hit = 0;
        } else if (em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            hit = 1;
        } else if (em_frame_check(self, 0, 118.0f, 0.0f) == 1) {
            hit = 2;
        } else {
            hit = -1;
        }
        if (hit != -1) {
            switch (hit) {
            default:
                if (arg1 == 0) {
                    angle_a = 0x271C;
                } else {
                    angle_a = 0x11C7;
                }
                angle_b = 0xE38F;
                break;
            case 1:
                if (arg1 == 0) {
                    angle_a = 0x271C;
                } else {
                    angle_a = 0xE39;
                }
                angle_b = 0xF1C8;
                break;
            case 2:
                if (arg1 == 0) {
                    angle_a = 0x238E;
                } else {
                    angle_a = 0xC17;
                }
                angle_b = 0;
                break;
            }
            em003_spawn_rec_set(&rec, 0, angle_a, angle_b);
            shell_set_func_ptr->method_0x40(self, &rec, 7, self->em003_0x328.hit_id, shell_set_func_ptr);
            eft007_part_set(self, 9);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
#pragma opt_common_subs reset

void em003_act7_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act7_0(self);
        return;
    case 0x1:
        em003_act7_1(self);
        return;
    case 0x2:
        em003_act7_2(self, 0);
        return;
    case 0x3:
        em003_act7_3(self, 0);
        return;
    case 0x4:
        em003_act7_3(self, 1);
        return;
    case 0x5:
        em003_act7_3(self, 2);
        return;
    case 0x6:
        em003_act7_3(self, 3);
        return;
    case 0x7:
        em003_act7_7(self);
        return;
    case 0x8:
        em003_act7_8(self);
        return;
    case 0x9:
        em003_act7_9(self, 0, 0, 0);
        return;
    case 0xA:
        em003_act7_10(self, 0);
        return;
    case 0xB:
        em003_act7_10(self, 1);
        return;
    case 0xC:
        em003_act7_12(self);
        return;
    case 0xD:
        em003_act7_13(self, 0, 0, 0);
        return;
    case 0xE:
        em003_act7_13(self, 1, 0, 0);
        return;
    case 0xF:
        em003_act7_15(self);
        return;
    case 0x10:
        em003_act7_10(self, 2);
        return;
    case 0x11:
        em003_act7_2(self, 1);
        return;
    case 0x12:
        em003_act7_3(self, 4);
        return;
    case 0x13:
        em003_act7_19(self, 0, 0);
        return;
    case 0x14:
        em003_act7_20(self, 0);
        return;
    case 0x15:
        em003_act7_20(self, 1);
        return;
    case 0x16:
        em003_act7_3(self, 0);
        return;
    case 0x17:
        em003_act7_9(self, 1, 0, 0);
        return;
    case 0x18:
        em003_act7_9(self, 1, 1, 0);
        return;
    case 0x19:
        em003_act7_13(self, 0, 1, 0);
        return;
    case 0x1A:
        em003_act7_13(self, 1, 1, 0);
        return;
    case 0x1B:
        em003_act7_9(self, 1, 0, 1);
        return;
    case 0x1C:
        em003_act7_9(self, 2, 0, 1);
        return;
    case 0x1D:
        em003_act7_13(self, 0, 0, 1);
        return;
    case 0x1E:
        em003_act7_13(self, 1, 0, 1);
        return;
    case 0x1F:
        em003_act7_19(self, 1, 0);
        return;
    case 0x20:
        em003_act7_19(self, 0, 1);
        return;
    case 0x21:
        em003_act7_19(self, 1, 1);
        return;
    default:
        return;
    }
}

/* The action-10 sound programs and their records (`.data`/`.sdata`), in retail order. */
EmSeRecord em003_se_rec_17_0 = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_17_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_1C_0 = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_1C_2 = {104, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em003_se_rec_1C_3 = {106, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_1C[5] = {
    {0x00, {0, 0, 0}, &em003_se_rec_1C_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x96)},
    {0x00, {0, 0, 0}, &em003_se_rec_1C_2},
    {0x00, {0, 0, 0}, &em003_se_rec_1C_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_5D_0 = {205, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

u32 em003_se_loop_1[2] = {0x02070000, 0x00000000};
EmSeEntry em003_se_seq_5D[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_5D_0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em003_se_loop_1},
};

EmSeRecord em003_se_rec_1D_0 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_1D[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_1D_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_1A_0 = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_1A_2 = {104, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em003_se_rec_1A_3 = {106, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_1A[5] = {
    {0x00, {0, 0, 0}, &em003_se_rec_1A_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em003_se_rec_1A_2},
    {0x00, {0, 0, 0}, &em003_se_rec_1A_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_1B_0 = {103, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_1B_2 = {105, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em003_se_rec_1B_3 = {107, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_1B[5] = {
    {0x00, {0, 0, 0}, &em003_se_rec_1B_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em003_se_rec_1B_2},
    {0x00, {0, 0, 0}, &em003_se_rec_1B_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_18_0 = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_18_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_23_2 = {112, 0, 2, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em003_se_shift_a.pos};

EmSeOffset em003_se_ofs_0 = {-50.0f, -10.0f};
EmSeRecord em003_se_rec_23_3 = {0, 0, 0, 0, 0, {0, 2, 1, 0}, &em003_se_ofs_0};

EmSeRecord em003_se_rec_23_5 = {113, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_23_7 = {105, 0, 8, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em003_se_rec_23_8 = {107, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_23[10] = {
    {0x0B, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em003_se_rec_23_2},
    {0x01, {0, 0, 0}, &em003_se_rec_23_3},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_23_5},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em003_se_rec_23_7},
    {0x00, {0, 0, 0}, &em003_se_rec_23_8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_7A_0 = {116, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_7A_0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_7B_0 = {24, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_7B_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_9F_0 = {25, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_9F_1 = {124, 0, 10, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_9F[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_9F_0},
    {0x00, {0, 0, 0}, &em003_se_rec_9F_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_7C_0 = {115, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_7C_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_8D_0 = {131, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_8D_1 = {124, 0, 0, 22, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_8D[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_8D_0},
    {0x00, {0, 0, 0}, &em003_se_rec_8D_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_7E_0 = {103, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_7E_1 = {105, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em003_se_rec_7E_2 = {107, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_7E[4] = {
    {0x00, {0, 0, 0}, &em003_se_rec_7E_0},
    {0x00, {0, 0, 0}, &em003_se_rec_7E_1},
    {0x00, {0, 0, 0}, &em003_se_rec_7E_2},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_7F_0 = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_7F_1 = {104, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em003_se_rec_7F_2 = {106, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_7F[4] = {
    {0x00, {0, 0, 0}, &em003_se_rec_7F_0},
    {0x00, {0, 0, 0}, &em003_se_rec_7F_1},
    {0x00, {0, 0, 0}, &em003_se_rec_7F_2},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_8E_0 = {124, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_8E_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_84_2 = {112, 0, 2, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em003_se_shift_a.pos};

EmSeRecord em003_se_rec_84_3 = {0, 0, 0, 0, 0, {0, 2, 1, 0}, &em003_se_ofs_0};

EmSeRecord em003_se_rec_84_5 = {113, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_84_6 = {105, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_84_7 = {107, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_84[9] = {
    {0x0B, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em003_se_rec_84_2},
    {0x01, {0, 0, 0}, &em003_se_rec_84_3},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_84_5},
    {0x00, {0, 0, 0}, &em003_se_rec_84_6},
    {0x00, {0, 0, 0}, &em003_se_rec_84_7},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_78_0 = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_78_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_A8_0[2] = {
    {108, 0, 2, 0, 0, {0, 1, 0, 0}, 0},
    {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0},
};

EmSeEntry em003_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, em003_se_rec_A8_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_AF_0 = {117, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_AF_1 = {14, 0, 10, 0, 1, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_AF[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_AF_0},
    {0x00, {0, 0, 0}, &em003_se_rec_AF_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_B6_0 = {127, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_B6_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_B7_0 = {128, 0, 4, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_B7_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_B8_1 = {67, 0, 4, 4, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em003_se_shift_c.pos};

EmSeOffset em003_se_ofs_1 = {-5.0f, 60.0f};
EmSeRecord em003_se_rec_B8_3 = {26, 0, 6, 0, 0, {0, 3, 4, 0}, &em003_se_ofs_1};

EmSeRecord em003_se_rec_B8_5 = {46, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_B8_7 = {65, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_B8[9] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_B8_1},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1F4)},
    {0x00, {0, 0, 0}, &em003_se_rec_B8_3},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3)},
    {0x00, {0, 0, 0}, &em003_se_rec_B8_5},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_B8_7},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_BB_0 = {130, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_BB_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_B9_0 = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_B9_0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_BA_0 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em003_se_rec_BA_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_BF_2 = {108, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em003_se_rec_BF_4 = {115, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em003_se_seq_BF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em003_se_rec_BF_2},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_BF_4},
    {0xFE, {0, 0, 0}, 0},
};

void em003_act10_se_play(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em003_se_seq_17, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em003_se_seq_18, 0, 0x18);
        return;
    case 0x19:
        em_se_tbl_play(self, em003_se_seq_18, 0, 0x19);
        return;
    case 0x1A:
        em_se_tbl_play(self, em003_se_seq_1A, 0, 0x1A);
        return;
    case 0x1B:
        em_se_tbl_play(self, em003_se_seq_1B, 0, 0x1B);
        return;
    case 0x1C:
        em_se_tbl_play(self, em003_se_seq_1C, 0, 0x1C);
        return;
    case 0x1D:
        em_se_tbl_play(self, em003_se_seq_1D, 0, 0x1D);
        return;
    case 0x1E:
        em_se_tbl_play(self, em003_se_seq_17, 0, 0x1E);
        return;
    case 0x5D:
        em_se_tbl_play(self, em003_se_seq_5D, 0, 0x5D);
        return;
    case 0x23:
        em_se_tbl_play(self, em003_se_seq_23, 1, 0x23);
        return;
    case 0x7A:
        em_se_tbl_play(self, em003_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em003_se_seq_7B, 0, 0x7B);
        return;
    case 0x7C:
        em_se_tbl_play(self, em003_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em003_se_seq_8D, 0, 0x8D);
        return;
    case 0x7E:
        em_se_tbl_play(self, em003_se_seq_7E, 0, 0x7E);
        return;
    case 0x7F:
        em_se_tbl_play(self, em003_se_seq_7F, 0, 0x7F);
        return;
    case 0x8E:
        em_se_tbl_play(self, em003_se_seq_8E, 0, 0x8E);
        return;
    case 0x78:
        em_se_tbl_play(self, em003_se_seq_78, 0, 0x78);
        return;
    case 0x84:
        em_se_tbl_play(self, em003_se_seq_84, 1, 0x84);
        return;
    case 0x9F:
        em_se_tbl_play(self, em003_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em003_se_seq_9F, 0, 0xA0);
        return;
    case 0xA8:
        em_se_tbl_play(self, em003_se_seq_A8, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play(self, em003_se_seq_23, 1, 0xA9);
        return;
    case 0xAF:
        em_se_tbl_play(self, em003_se_seq_AF, 0, 0xAF);
        return;
    case 0xB6:
        em_se_tbl_play(self, em003_se_seq_B6, 0, 0xB6);
        return;
    case 0xB7:
        em_se_tbl_play(self, em003_se_seq_B7, 0, 0xB7);
        return;
    case 0xB8:
        em_se_tbl_play(self, em003_se_seq_B8, 1, 0xB8);
        return;
    case 0xB9:
        em_se_tbl_play(self, em003_se_seq_B9, 0, 0xB9);
        return;
    case 0xBA:
        em_se_tbl_play(self, em003_se_seq_BA, 0, 0xBA);
        return;
    case 0xBB:
        em_se_tbl_play(self, em003_se_seq_BB, 0, 0xBB);
        return;
    case 0xBC:
        em_se_tbl_play(self, em003_se_seq_BB, 0, 0xBC);
        return;
    case 0xBF:
        em_se_tbl_play(self, em003_se_seq_BF, 0, 0xBF);
        return;
    case 0xC1:
        em_se_tbl_play(self, em003_se_seq_8E, 0, 0xC1);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

/* The action-11 sound programs (`.data`/`.sdata`). */
EmSeRecord em003_se_rec_alt_00_0 = {125, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_alt_seq_00[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_alt_00_0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeOffset em003_se_ofs_2 = {-50.0f, -10.0f};
EmSeRecord em003_se_rec_alt_05_1 = {112, 0, 2, 0, 0, {0, 2, 1, 0}, &em003_se_ofs_2};

EmSeRecord em003_se_rec_alt_05_3 = {113, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_alt_05_4 = {114, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_alt_seq_05[7] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_alt_05_1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em003_se_rec_alt_05_3},
    {0x00, {0, 0, 0}, &em003_se_rec_alt_05_4},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_alt_0F_0 = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_alt_seq_0F[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_alt_0F_0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_alt_1A_0 = {116, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em003_se_rec_alt_1A_1 = {24, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em003_se_rec_alt_1A_0},
    {0x00, {0, 0, 0}, &em003_se_rec_alt_1A_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em003_se_rec_alt_1C_0 = {130, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em003_se_alt_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em003_se_rec_alt_1C_0},
    {0xFF, {0, 0, 0}, 0},
};

void em003_act11_se_play(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {
    case 0:
        em_se_tbl_play_alt(self, em003_se_alt_seq_00, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt(self, em003_se_alt_seq_05, 1, 5);
        return;
    case 15:
        em_se_tbl_play_alt(self, em003_se_alt_seq_0F, 0, 0xF);
        return;
    case 26:
        em_se_tbl_play_alt(self, em003_se_alt_seq_1A, 0, 0x1A);
        return;
    case 28:
        em_se_tbl_play_alt(self, em003_se_alt_seq_1C, 0, 0x1C);
        return;
    default:
        em_se_tbl_play_alt(self, em003_se_alt_seq_00, 0, 0);
        return;
    }
}

/* The arena-demo position and rotation key tables `em003_act13_0` replays. */
f32 em003_demo_pos_keys[36] = {
    1108.0f, -813.28f, 790.0f, 176.93f, 1437.0f, -813.28f, 790.0f, 176.93f,
    1440.0f, -794.46f, 790.0f, 187.74f, 1445.0f, -763.09f, 812.15f, 205.75f,
    1452.0f, -719.17f, 975.27f, 230.97f, 1541.0f, 1184.56f, 1993.28f, 1625.09f,
    1636.0f, 2392.56f, 3493.96f, 6535.53f, 1800.0f, 3167.29f, 1880.22f, 9638.54f,
    -1.0f, 3167.29f, 1880.22f, 9638.54f,
};

f32 em003_demo_rot_keys[24] = {
    1108.0f, 0.0f, 65.25f, 0.0f, 1452.0f, 0.0f, 65.25f, 0.0f,
    1541.0f, 0.0f, 47.33f, 0.0f, 1636.0f, 0.0f, 17.25f, 0.0f,
    1800.0f, 0.0f, 17.25f, 0.0f, -1.0f, 0.0f, 17.25f, 0.0f,
};

void em003_act12_0(_ENEMY_WORK* self) {
    em003_act3_2(self);
}

void em003_act12_dispatch(_ENEMY_WORK *self) {
    if (self->state_sub == 0) {
        em003_act12_0(self);
    }
}

void em003_act13_0(_ENEMY_WORK *self) {
    VEC3 target;
    VEC3 vel;
    VEC3 step;
    VEC3 delta;
    u8 temp_r0;

    VEC3_ctor(&target);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 0x1:
        if (em_demo_time_ck(0x298) == 1) {
            self->state += 1;
            em_mot_set(self, 0xCA, 0, 0);
            em_mot_speed_set(self, 1.17f);
            em_demo_pos_set(self, -809.0916f, 790.0f, 61.2847f);
            em_demo_rot_set(self, 0.0f, 62.25f, 0.0f);
            em_demo_enable(self);
            break;
        }
    default:
        break;
    case 0x2:
        if (em_demo_time_ck(0x332) == 1) {
            self->state += 1;
            em_mot_set(self, 0xCA, 0, 0x96);
            break;
        }
        break;
    case 0x3:
        if (em_demo_time_ck(0x374) == 1) {
            self->state += 1;
            self->state_0x006 = 0;
            em_mot_set(self, 0xDC, 4, 4);
            break;
        }
        break;
    case 0x4:
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0x0:
            if (em_demo_time_ck(0x37A) == 1) {
                self->state_0x006 += 1;
                em_move_vec_clr(self);
                setVector3(&target, -806.5184f, 790.0f, 116.2548f);
                subVec3(&delta, &target, &self->pos);
                vec3_div(&step, &delta, 18.0f);
                vec3_scale(&vel, &step, 2.0f);
                copyVec3(&self->offset_0x30C.vec_0x310, &vel);
            }
            break;
        case 0x1:
            em_move_offset_apply(self);
            if (em_demo_time_ck(0x38C) == 1) {
                self->state_0x006 += 1;
                setVector3(&self->pos, -806.5184f, 790.0f, 116.2548f);
            }
            break;
        }
        if (em_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 2, 0x18, 0x10, 2);
            break;
        }
        break;
    case 0x5:
        if (em_demo_time_ck(0x454) == 1) {
            self->state += 1;
            em_mot_set(self, 0xE8, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), em003_demo_pos_keys, em003_demo_rot_keys);
            break;
        }
        break;
    case 0x6:
        em_demo_key3_apply(self, em_demo_frame_get(), em003_demo_pos_keys, em003_demo_rot_keys);
        if (em_frame_check(self, 0, 218.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x29, 0x1A, 0x10);
            break;
        }
        break;
    case 0x7:
        em_demo_key3_apply(self, em_demo_frame_get(), em003_demo_pos_keys, em003_demo_rot_keys);
        if (em_frame_check(self, 0, 109.0f, 0.0f) == 1) {
            self->state += 1;
            em_mot_set(self, 0x20, 8, 0x22);
            eft_spawn_type11(self, &self->pos, 0, 0.5f);
            break;
        }
        break;
    case 0x8:
        em_shake_req_set(self);
        em_demo_key3_apply(self, em_demo_frame_get(), em003_demo_pos_keys, em003_demo_rot_keys);
        if (em_demo_time_ck(0x5DC) == 1) {
            self->state += 1;
            em_mot_set(self, 0x20, 0, 0);
            break;
        }
        break;
    case 0x9:
        em_shake_req_set(self);
        em_demo_key3_apply(self, em_demo_frame_get(), em003_demo_pos_keys, em003_demo_rot_keys);
        break;
    }
}

void em003_act13_1(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0x0:
        self->state++;
        em_fall_start_f(self, em_fall_height_get(self));
        setVector3(&self->pos, 3167.29f, 1880.22f, 9638.54f);
        copyVec3(&self->prev_pos, &self->pos);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xC44;
        self->field_0x1C4 = 0;
        em_mot_set(self, 0x20, 0, 0);
        return;
    case 0x1:
        em003_finish_act3(self);
        return;
    }
}

void em003_act13_dispatch(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0x0:
        em003_act13_0(self);
        return;
    case 0x1:
        em003_act13_1(self);
        return;
    }
}

void em003_action_run(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->action;
    switch (temp_r0) {
    case 0x0:
        em003_act0_dispatch(self);
        return;
    case 0x1:
        em003_act1_dispatch(self);
        return;
    case 0x2:
        em003_act2_dispatch(self);
        return;
    case 0x3:
        em003_act3_dispatch(self);
        return;
    case 0x4:
        em003_act4_dispatch(self);
        return;
    case 0x7:
        em003_act7_dispatch(self);
        return;
    case 0xA:
        em003_act10_se_play(self);
        return;
    case 0xB:
        em003_act11_se_play(self);
        return;
    case 0xC:
        em003_act12_dispatch(self);
        return;
    case 0xD:
        em003_act13_dispatch(self);
        return;
        return;
    default:
        return;
    }
}

void em003_alt_mode_fx(_ENEMY_WORK *self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    if ((em_alt_mode_ck(self) == 1) && ((system_w.field_0x0c % 20) == 0)) {
        setVector3(&vec, 0.0f, 30.0f, 140.0f);
        eft_spawn_type10(self, 1, 0x18, &vec, 1.0f);
    }
}

void em003_ground_impact_fx(_ENEMY_WORK *self, u8 arg1) {
    VEC3 vec;

    VEC3_ctor(&vec);
    if (self->em003_0x328.tick == 0) {
        setVector3(&vec, 0.0f, 0.0f, 100.0f);
        em_ground_impact_set(self, arg1, 1, 0.65f, &vec);
    }
}

/* 0x8015AD2C (0x3A0): the shared effect spawn: mode 0 is the footstep/landing dust (its kind remapped by the ground
 * state, at the joint or under the body), mode 1 the area effect at a joint, mode 2 the type-11 effect at the feet. */
void em003_effect_spawn(_ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, s32 angle, f32 scale) {
    VEC3 pos;

    VEC3_ctor(&pos);
    switch (mode) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (kind) {
            case 0:
                kind = 0xD;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xF, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xF, angle, scale);
                }
                break;
            case 2:
            case 4:
                kind = 0xC;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xE, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, angle, scale);
                }
                break;
            case 6:
            case 8:
                kind = 0xC;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xF, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, angle, scale);
                }
                break;
            case 0x27:
            case 0x28:
                kind = 0xD;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            case 1:
            case 3:
            case 5:
            case 7:
            case 9:
            case 10:
            case 11:
            case 0x1A:
            case 0x24:
            case 0x25:
            case 0x29:
            case 0x2A:
                return;
            default:
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            }
        } else {
            switch (kind) {
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
            case 0x27:
            case 0x2A:
            case 0x2B:
                kind = 2;
                break;
            case 0x28:
                kind = 6;
                break;
            case 0x29:
                kind = 4;
                break;
            }
            if (joint == 0xFF) {
                pos.x = self->pos.x;
                pos.y = 5.0f + self->field_0x20C;
                pos.z = self->pos.z;
            }
        }
        if (joint == 0xFF) {
            eft009_set_pos(kind, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, kind, angle, scale);
        }
        break;
    case 1:
        if ((self->field_0x228 & 6) == 0) {
            if (joint == 0xFF) {
                copyVec3(&pos, &self->pos);
            } else {
                get_joint_wpos_em(self, joint, &pos);
            }
            pos.y = self->field_0x20C;
            scale *= get_em_chg_scale(self);
            eft_spawn_pos_in_area(&pos, self->area_no, kind, self->field_0x1C0 + angle, scale);
        }
        break;
    case 2:
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        eft_spawn_type11(self, &pos, kind, scale);
        break;
    }
}

void em003_motion_events(_ENEMY_WORK *self) {
    VEC3 vec;
    u16 temp_r3;

    VEC3_ctor(&vec);
    em003_alt_mode_fx(self);
    temp_r3 = em_get_mot_no(self);
    switch (temp_r3) {
    case 0x5:
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 2, 0x25, 0, 0.8f);
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.8f);
            break;
        }
    default:
        break;
    case 0x6:
        if ((em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 2, 0x25, 0, 0.6f);
        }
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.6f);
            break;
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x25, 0, 0.6f);
            break;
        }
        break;
    case 0x8:
        if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x28, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 0, 0x2F, 0, 0.6f);
            break;
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 4, 0x25, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 4, 0x2C, 0, 1.4f);
            break;
        }
        break;
    case 0xE:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 4, 0xFF, 0, 1.4f);
            break;
        }
        break;
    case 0x1A:
        em003_ground_impact_fx(self, 2);
        break;
    case 0x1B:
        em003_ground_impact_fx(self, 2);
        break;
    case 0x25:
    case 0x26:
        em003_ground_impact_fx(self, 2);
        break;
    case 0x2A:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 100.0f);
            em_ground_impact_set(self, 2, 1, 0.65f, &vec);
            break;
        }
        break;
    case 0x32:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 6, 0x25, 0x8000, 1.0f);
            break;
        }
        break;
    case 0x36:
        if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 2, 0, 0xFF, 0, 0.3f);
            break;
        }
        break;
    case 0x38:
        if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 2, 0, 0xFF, 0, 0.3f);
            break;
        }
        break;
    case 0x40:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x25, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x11, 0x15, 0, 0.4f);
                break;
            }
            em003_effect_spawn(self, 1, 3, 0x15, 0, 0.4f);
            break;
        }
        break;
    case 0x41:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x14, 0xFF, 0, 0.7f);
            } else {
                em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.7f);
            }
            em_ground_impact_set(self, 1, 0, 0.65f, NULL);
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0x42:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 2, 0, 0xFF, 0, 0.4f);
            break;
        }
        break;
    case 0x43:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x12, 0xFF, 0, 1.2f);
            } else {
                em003_effect_spawn(self, 0, 5, 0xFF, 0, 0.5f);
            }
            em_ground_impact_set(self, 1, 1, 0.65f, NULL);
            break;
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 0x2C, 0, 0.5f);
            } else {
                em003_effect_spawn(self, 1, 6, 0x2C, 0, 0.5f);
            }
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 0x2C, 0, 0.4f);
            } else {
                em003_effect_spawn(self, 1, 6, 0x2C, 0, 0.4f);
            }
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 0x25, 0, 0.4f);
                break;
            }
            em003_effect_spawn(self, 1, 6, 0x25, 0, 0.4f);
            break;
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x11, 0xFF, 0x4000, 0.5f);
            } else {
                em003_effect_spawn(self, 1, 0, 0xFF, 0x4000, 0.5f);
            }
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x11, 0xFF, 0xC000, 0.5f);
            } else {
                em003_effect_spawn(self, 1, 0, 0xFF, 0xC000, 0.5f);
            }
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x11, 0xFF, 0, 0.4f);
            } else {
                em003_effect_spawn(self, 1, 5, 0xFF, 0, 0.4f);
            }
            em_camera_req(self, -1, 1);
            break;
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, 228.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x14, 0xFF, 0, 0.5f);
                break;
            }
            em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.5f);
            break;
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x10, 0xFF, 0, 0.4f);
            } else {
                em003_effect_spawn(self, 1, 1, 0xFF, 0, 0.4f);
            }
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x14, 0xFF, 0, 1.0f);
            } else {
                em003_effect_spawn(self, 1, 2, 0xFF, 0, 0.5f);
            }
        }
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0xC8:
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 100.0f, 70.0f);
            eft_em_spawn(self, 8, 0x18, &vec, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 146.0f, 214.0f) == 1) && ((system_w.field_0x0c & 0xF) == 0)) {
            setVector3(&vec, 0.0f, 580.0f, 120.0f);
            eft_em_spawn(self, 6, 1, &vec, 1.0f);
            break;
        }
        break;
    case 0xCA:
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 8, 0x18, &vec, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 54.0f, 218.0f) == 1) && ((system_w.field_0x0c & 7) == 0)) {
            setVector3(&vec, 0.0f, 50.0f, 150.0f);
            eft_em_spawn(self, 7, 0x18, &vec, 1.0f);
            break;
        }
        break;
    case 0xCB:
        if ((em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, 0.0f, 60.0f);
            eft_em_spawn(self, 0x14, 0x10, &vec, 0.5f);
        }
        if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 164.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&vec, -80.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 0x25, 0xC, 0, 1.3f, &vec);
                eft009_spawn_at_pos(self, 0x25, 0xE, 0, 1.3f, &vec);
                break;
            }
            em003_effect_spawn(self, 1, 4, 3, 0, 0.2f);
            break;
        }
        break;
    case 0xCC:
        if ((em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 170.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&vec, -80.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 0x25, 0xC, 0, 1.3f, &vec);
                eft009_spawn_at_pos(self, 0x25, 0xE, 0, 1.3f, &vec);
                break;
            }
            em003_effect_spawn(self, 1, 4, 3, 0, 0.2f);
            break;
        }
        break;
    case 0xCD:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&vec, -80.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 0x25, 0xC, 0, 1.3f, &vec);
                eft009_spawn_at_pos(self, 0x25, 0xE, 0, 1.3f, &vec);
                break;
            }
            em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.2f);
            break;
        }
        break;
    case 0xCE:
        if ((em_after_frame_check(self, 0, 132.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 172.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 202.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 240.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 290.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 0xFF, 0, 0.2f);
                em003_effect_spawn(self, 0, 0xE, 0xFF, 0, 0.2f);
                break;
            }
            em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.2f);
            break;
        }
        break;
    case 0xD0:
        if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x2F, 0, 0.6f);
        }
        if ((em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x28, 0, 0.6f);
            break;
        }
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x14, 0xFF, 0, 0.4f);
            } else {
                em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.4f);
            }
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0xD4:
    case 0xD5:
        if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x2F, 0, 0.6f);
        }
        if ((em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x28, 0, 0.6f);
            break;
        }
        break;
    case 0xD7:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_ground_impact_set(self, 1, 1, 0.65f, NULL);
            break;
        }
        break;
    case 0xD8:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 2, 0, 0xFF, 0, 0.4f);
            break;
        }
        break;
    case 0xD9:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_ground_impact_set(self, 1, 1, 0.65f, NULL);
        }
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
            break;
        }
        break;
    case 0xDA:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 0, 0x2F, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 0, 0x25, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x11, 0xFF, 0, 0.6f);
            } else {
                em003_effect_spawn(self, 1, 0, 0, 0, 0.6f);
            }
        }
        if (em_after_frame_check(self, 0, 232.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 2, 0x2C, 0, 0.6f);
            break;
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em003_effect_spawn(self, 0, 0, 0x2F, 0, 0.6f);
        }
        if ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1)) {
            em003_effect_spawn(self, 0, 0, 0x28, 0, 0.6f);
        }
        if ((em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x12, 0xFF, 0, 0.6f);
                break;
            }
            em003_effect_spawn(self, 1, 0, 0xFF, 0, 0.6f);
            break;
        }
        break;
    case 0xDC:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 0xFF, 0, 1.2f);
                em003_effect_spawn(self, 0, 0xE, 0xFF, 0, 1.2f);
                break;
            }
            em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.2f);
            break;
        }
        break;
    case 0xDD:
        if ((em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1)) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0xC, 3, 0, 1.2f);
                em003_effect_spawn(self, 0, 0xE, 3, 0, 1.2f);
                break;
            }
            em003_effect_spawn(self, 1, 4, 3, 0, 0.2f);
            break;
        }
        break;
    case 0xE0:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em003_effect_spawn(self, 0, 0x14, 0xFF, 0, 0.5f);
            } else {
                em003_effect_spawn(self, 1, 4, 0xFF, 0, 0.5f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    }
}

/* 0x8015C890 (0x50): steps a colour byte toward its target by `step`, landing on the target. */
u8 em003_color_approach(u8 cur, u8 target, u8 step) {
    if (cur < target) {
        if (cur + step < target) {
            return cur + step;
        }
    } else if (cur > target && cur > target + step) {
        return cur - step;
    }
    return target;
}

/* 0x8015C8E0 (0x294): eases the three K-colours toward the alt-mode / effect / idle tints and the glow alpha
 * toward the current motion's key curve. */
void em003_color_update(_ENEMY_WORK* self) {
    s32 alpha;
    u8 r2;
    u8 g2;
    u8 r;
    u8 b2;
    u8 a2;
    f32* keys;
    u8 b;
    u8 g;

    if (em_alt_mode_ck(self) == 1) {
        r = 0xCF;
        g = 0x48;
        b = 0x30;
    } else if (em_eft_active_ck(self) == 1) {
        r = 0x40;
        g = 0xBC;
        b = 0xBF;
    } else {
        r = 0xFF;
        g = 0xFF;
        b = 0xFF;
    }
    self->em003_0x328.kcolor1_r = em003_color_approach(self->em003_0x328.kcolor1_r, r, 0x10);
    self->em003_0x328.kcolor1_g = em003_color_approach(self->em003_0x328.kcolor1_g, g, 0x10);
    self->em003_0x328.kcolor1_b = em003_color_approach(self->em003_0x328.kcolor1_b, b, 0x10);
    if (em_timer_0x94A_ck(self) == 0) {
        r2 = 0xFF;
        g2 = 0xFF;
        b2 = 0xFF;
        a2 = 0;
    } else {
        r2 = 0xFF;
        g2 = 0;
        b2 = 0;
        a2 = 0xFF;
    }
    self->em003_0x328.kcolor2_r = em003_color_approach(self->em003_0x328.kcolor2_r, r2, 0x20);
    self->em003_0x328.kcolor2_g = em003_color_approach(self->em003_0x328.kcolor2_g, g2, 0x20);
    self->em003_0x328.kcolor2_b = em003_color_approach(self->em003_0x328.kcolor2_b, b2, 0x20);
    self->em003_0x328.kcolor2_a = em003_color_approach(self->em003_0x328.kcolor2_a, a2, 0x20);
    if ((s32)em_get_mot_no(self) == 5) {
        em_after_frame_check(self, 3, 156.0f, 190.0f);
    }
    switch ((s32)em_get_mot_no(self)) {
    case 5:
        keys = em003_glow_keys_05;
        break;
    case 0xC8:
        keys = em003_glow_keys_C8;
        break;
    case 0xC9:
        keys = em003_glow_keys_C9;
        break;
    case 0xCA:
        keys = em003_glow_keys_CA;
        break;
    case 0xCB:
        keys = em003_glow_keys_CB;
        break;
    case 0xCC:
        keys = em003_glow_keys_CC;
        break;
    case 0xCD:
        keys = em003_glow_keys_CD;
        break;
    case 0xDA:
        keys = em003_glow_keys_DA;
        break;
    case 0xDC:
        keys = em003_glow_keys_DC;
        break;
    case 0xE1:
        keys = em003_glow_keys_E1;
        break;
    default:
        keys = 0;
        break;
    }
    if (keys != 0) {
        alpha = getKeyData(keys, em_mot_frame_get(self));
    } else {
        alpha = 0;
    }
    self->em003_0x328.kcolor3_a = em003_color_approach(self->em003_0x328.kcolor3_a, alpha, 0x40);
}

/* 0x8015CB74 (0x118): the K-colour material update: eases the colours, writes K-colours 1 and 2 of material 0 and
 * the K-colour 3 alpha of material 1, and keeps the "glowing" flag bit 0x8000 clear while the glow is past half. */
void em003_tev_color_update(_ENEMY_WORK* self) {
    _GXColor color;

    em003_color_update(self);
    ((MHchar*)self->char_0x024)->getTevKColor(0, GX_KCOLOR1, &color);
    color.r = self->em003_0x328.kcolor1_r;
    color.g = self->em003_0x328.kcolor1_g;
    color.b = self->em003_0x328.kcolor1_b;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR1, &color);
    color.r = self->em003_0x328.kcolor2_r;
    color.g = self->em003_0x328.kcolor2_g;
    color.b = self->em003_0x328.kcolor2_b;
    color.a = self->em003_0x328.kcolor2_a;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR2, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR3, &color);
    color.a = self->em003_0x328.kcolor3_a;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &color);
    if ((f32)self->em003_0x328.kcolor3_a >= 128.0f) {
        self->flags_0x836 &= 0x7FFF;
    } else {
        self->flags_0x836 |= 0x8000;
    }
}

/* 0x8015CC8C (0x2D8): the state query: 0/1 pick a group / kind to call, 2 whether the called group is in the area,
 * 3 the rage flag, 4 the area-3 phase-1 test, 5 whether a call is due at this health, 6 the health band, 7 whether
 * part 6 is broken (damage level 3 or more). */
u8 em003_state_query(_ENEMY_WORK* self, u8 kind) {
    f32 ratio;

    switch (kind) {
    case 0:
        if (self->em003_0x328.call_done == 0) {
            return em003_call_group_pick(self);
        }
        return 0;
    case 1:
        return em003_call_kind_pick(self);
    case 2:
        if (self->em003_0x328.call_done == 1 && self->em003_0x328.call_group != 0) {
            _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
            u32 max = get_move_work_max(3);
            u16 i;

            for (i = 0; i < (u16)max; i++, work++) {
                if (work->active != 0 && work->area_no == self->area_no && work->action != 0xB &&
                    work->action != 0xC && work->team == self->em003_0x328.call_group) {
                    return 1;
                }
            }
        }
        return 0;
    case 3:
        return self->em003_0x328.rage;
    case 4:
        if (self->field_0x1E2 == 0) {
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 3:
                    if (em_phase_get(self) == 1) {
                        return 1;
                    }
                    break;
                }
                break;
            }
        }
        return 0;
    case 5:
        if (self->em003_0x328.call_timer <= 0) {
            ratio = (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4;
            if (ratio < 0.7f && ratio > 0.4f && self->em003_0x328.call_count == 0 &&
                em003_state_query(self, 0) != 0) {
                return 1;
            }
            if (ratio < 0.3f && self->em003_0x328.call_count <= 1 && em003_state_query(self, 0) != 0) {
                return 1;
            }
        }
        return 0;
    case 6:
        ratio = (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4;
        if (ratio > 0.8f) {
            return 1;
        }
        if (ratio > 0.3f) {
            return 0;
        }
        return 2;
    case 7:
        return em_parts_damage_level_get(self, 6) >= 3;
    }
    return 0;
}

/* 0x8015CF64 (0x1DC): the area-start test: on maps 1/2 it hands over to the area-break check, the packed area pick
 * (unless the call group is already present), the roam pick and the leave test; map 2 with the 0x336 flag arms the
 * special exit. */
u32 em003_area_start_ck(_ENEMY_WORK* self, u16 arg1) {
    u8 map;
    s32 broken;
    u8 ref;
    u32 area;

    map = stage_map_kind_get(self->field_0x1E0);
    if ((u32)(map - 1) > 1) {
        return 0;
    }
    if (map == 2 && self->em003_0x328.flag_0x336 == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 4;
        return 1;
    }
    broken = 0;
    switch (map) {
    case 1:
        ref = 0xA;
        break;
    case 2:
        ref = 9;
        break;
    default:
        ref = 0xFF;
        break;
    }
    if (ref != 0xFF) {
        switch (em_area_break_ck(self, ref)) {
        case 1:
            broken = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (broken == 0 && em003_state_query(self, 2) == 0) {
        switch (map) {
        case 1:
            area = 0xC;
            break;
        case 2:
            area = 9;
            break;
        default:
            area = 0xFF;
            break;
        }
        if (em_pack_area_pick(self, 0, area, arg1, 0, em003_area_pick_tbl) == 1) {
            return 1;
        }
    }
    if ((em_hit_timer_ck(self) == 1 || em003_state_query(self, 2) == 0) && em_area_roam_pick(self, arg1) == 1) {
        return 1;
    }
    if (em_area_leave_ck(self) == 1) {
        return 1;
    }
    return 0;
}

/* 0x8015D140 (0x54): the death hand-over: action 0xC sub-state 0, raised by 5000 and dropped from there. */
void em003_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub) {
    *action = 0xC;
    *sub = 0;
    self->pos.y += 5000.0f;
    em_fall_start_f(self, em_fall_height_get(self));
}

/* 0x8015D194 (0x1C): clears the call state (group, sent flag, rage, kind latch and the 0x336 flag). */
void em003_call_state_clear(_ENEMY_WORK* self) {
    self->em003_0x328.call_group = 0;
    self->em003_0x328.call_done = 0;
    self->em003_0x328.rage = 0;
    self->em003_0x328.call_kind_done = 0;
    self->em003_0x328.flag_0x336 = 0;
}

s32 em003_guard_ck(_ENEMY_WORK* self, u8 part) {
    u8 sub;

    if (part == 5 && (s32)self->action == 1 && ((u32)((sub = self->state_sub) - 0x13) <= 1 || (s32)sub == 9)) {
        return 1;
    }
    return 0;
}

/* 0x8015D1F0 (0x10): is the record in its idle mode (0x1E2 == 0)? */
s32 em003_idle_ck(_ENEMY_WORK* self) {
    return self->field_0x1E2 == 0;
}

/* 0x8015D200 (0x8): Sets the accessor's flag word to 3 once it is installed. */
void Em003UserData::Attach() {
    em_res_user_data_flag_set_view1(this, 3);
}

/* 0x8015D208 (0x80): fills a spawn record: effect type 26 at (0, -20, 30), the kind and the two angles. */
void em003_spawn_rec_set(EmSpawnRec* out, u8 kind, u16 angle_a, u16 angle_b) {
    VEC3 pos;

    setVec3(&pos, 0.0f, -20.0f, 30.0f);
    out->id = 26;
    copyVec3(&out->pos, &pos);
    out->field_0x10 = kind;
    out->field_0x12 = angle_a;
    out->field_0x14 = angle_b;
}

/* 0x8015D288 (0x204): picks, at random among the candidates, a call group: a placed area entry or a live enemy in
 * another area whose group/team pair is in `em003_call_group_tbl` and that may enter this area. */
u8 em003_call_group_pick(_ENEMY_WORK* self) {
    s32 k;
    const Em003CallPair* tbl;
    u8 pick;
    s32 count;
    s32 i;
    s32 j;
    const Em003CallPair* pair;
    _ENEMY_WORK* work;
    const Em003CallPair* work_pair;
    u32 max;
    EmAreaEntry* entry;

    tbl = em003_call_group_tbl;
    count = 0;
    pick = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    entry = &em_area_entry_tbl[0][0];
    for (i = 0; i < 128; i++, entry++) {
        if (entry->active != 0 && entry->kind_0x08 == 1 && entry->spawn_0x30 != 0) {
            for (j = 0, pair = tbl; pair->id != 0; pair++, j++) {
                if (entry->group == pair->id && (pair->team == 0xFF || entry->team_0x03 == pair->team) &&
                    em_area_entry_enter_ck(entry, self->field_0x1E0, self->area_no) == 1 &&
                    ran_suu(1) % (count + 1) == 0) {
                    pick = tbl[j].id;
                    count++;
                    break;
                }
            }
        }
    }
    for (i = 0; i < (u16)max; i++, work++) {
        if (work->active != 0 && work->area_no != self->area_no && em_die_ck_view1(work) != 1) {
            for (k = 0, work_pair = tbl; work_pair->id != 0; work_pair++, k++) {
                if (work->team == work_pair->id && (work_pair->team == 0xFF || work->field_0x00A == work_pair->team) &&
                    em_area_enter_ck(work, self->field_0x1E0, self->area_no) == 1 &&
                    ran_suu(1) % (count + 1) == 0) {
                    pick = tbl[k].id;
                    count++;
                    break;
                }
            }
        }
    }
    return pick;
}

/* 0x8015D48C (0x130): picks, at random, a call kind present in this area: a placed area entry whose group/team
 * pair is in `em003_call_kind_tbl`, each table row tried once. */
u8 em003_call_kind_pick(_ENEMY_WORK* self) {
    u8 tried[4];
    u8* mark;
    const Em003CallPair* pair;
    EmAreaEntry* entry;
    s32 i;
    s32 j;
    s32 count = 0;
    const Em003CallPair* tbl = em003_call_kind_tbl;
    u8 pick = 0;

    tried[0] = 0;
    tried[1] = 0;
    tried[2] = 0;
    tried[3] = 0;
    entry = &em_area_entry_tbl[0][0];
    for (i = 0; i < 128; i++, entry++) {
        if (entry->active != 0 && entry->kind_0x08 == 1 && self->area_no == entry->area_no) {
            for (j = 0, mark = tried, pair = tbl; pair->id != 0; mark++, pair++, j++) {
                if (*mark == 0 && entry->group == pair->id && (pair->team == 0xFF || entry->team_0x03 == pair->team)) {
                    *mark = 1;
                    if (ran_suu(1) % (count + 1) == 0) {
                        pick = tbl[j].id;
                        count++;
                        break;
                    }
                }
            }
        }
    }
    return pick;
}

/* 0x8015D5BC (0x54): the call type of the current sub-state: 1 for the group calls, 2 for the kind calls, 0 else. */
u8 em003_call_act_type_ck(_ENEMY_WORK* self) {
    switch (self->action) {
    case 1:
        switch (self->state_sub) {
        case 0x8:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
            return 1;
        case 0x7:
        case 0x17:
        case 0x1F:
        case 0x20:
            return 2;
        default:
            return 0;
        }
        break;
    }
    return 0;
}

/* 0x8015D610 (0x98): is a head-breaking (call or roar) sub-state running with its entry block done? */
u32 em003_atama_koware_act_ck(_ENEMY_WORK* self) {
    if ((em003_call_act_type_ck(self) != 0 || em_act_ck_view1(self, 1, 9) == 1 || em_act_ck_view1(self, 1, 0x13) == 1 ||
         em_act_ck_view1(self, 1, 0x14) == 1) &&
        self->field_0x1E7 == 1) {
        return 1;
    }
    return 0;
}

/* 0x8015D6A8 (0x28): is this an em003 of team 3 whose kind-10 call has gone out? */
u8 em003_skip_ck(struct _ENEMY_WORK* work) {
    if (work->team == 3 && work->em003_0x328.call_kind_done == 1) {
        return 1;
    }
    return 0;
}

/* 0x8015D6D0 (0x38): is `group` one of the callable groups of `em003_call_group_tbl`? */
s32 em003_yobi_boss_em_ck(u8 group) {
    const Em003CallPair* pair;

    for (pair = em003_call_group_tbl; pair->id != 0; pair++) {
        if (pair->id == group) {
            return 1;
        }
    }
    return 0;
}

/* 0x8015D708 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em003UserData::~Em003UserData() {
}

/* The unit's `.bss`: the three vector pairs MWCC's `__sinit` seeds (GUESS names). */
Em003VecPair em003_se_shift_a(10.0f, -0.4f);
Em003VecPair em003_se_shift_b(10.0f, -0.4f);
Em003VecPair em003_se_shift_c(70.0f, -0.6f);
